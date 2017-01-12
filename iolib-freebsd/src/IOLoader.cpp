#include <glog/logging.h>

#include <sys/chio.h>
#include <sys/ioctl.h>

#include "Loader.hpp"

namespace iolibbsd {

/**
 * Initializes the loader.
 */
Loader::Loader(const char *ch, const char *pass) {
    CHECK(ch != NULL) << "Changer device file must be specified";

    this->devCh = ch;
    this->devPass = pass;

    // Attempt to open the device first.
    _openCh();

    // Attempt to perform an inventory of this loader's storage elements.
    // performInventory();

    // Determine the properties of this changer, then fetch the inventory
    _fetchLoaderParams();
    _fetchInventory();

    // Close the file handle to the device
    _closeCh();
}

Loader::~Loader() {
    // Use the close method, but force it by setting reference count to 1
    LOG_IF(WARNING, this->fdChRefs > 1) << "More than one open reference on file descriptor at dellocation";

    this->fdChRefs = 1;
    _closeCh();
}


/**
 * Returns a copy of the drive file's path.
 */
iolib_string_t Loader::getDeviceFile() {
	iolib_string_t string = static_cast<iolib_string_t>(malloc(256));
	strncpy(string, this->devCh, 256);
	return string;
}

/**
 * Returns the number of elements for a given element type.
 */
size_t Loader::getNumElementsForType(iolib_storage_element_type_t type) {
    if(type == kStorageElementTransport) { // Transport element (picker)
        return this->numPickers;
    } else if(type == kStorageElementSlot) { // Storage element (slot)
        return this->numSlots;
    } else if(type == kStorageElementPortal) { // Portal
        return this->numPortals;
    } else if(type == kStorageElementDrive) { // Data transfer element (drive)
        return this->numDrives;
    } else if(type == kStorageElementAny) {
        return this->numPickers + this->numSlots + this->numPortals
                                + this->numDrives;
    }

    // we should NOT get down here
    return -1;
}

/**
 * Gets n elements of the specified type, writing pointers to them into the
 * given buffer.
 */
void Loader::getElementsForType(iolib_storage_element_type_t type, size_t len, Element **out) {
    size_t itemsWritten = 0;

    CHECK(len > 0) << "Length may not be zero!";
    CHECK(out != NULL) << "Output array may not be NULL";

    // Iterate through all elements
    for(auto it = this->elements.begin(); it < this->elements.end(); it++) {
        // If we've writen as many items as the array is long, exit.
        if(itemsWritten == len) {
            break;
        }

        // Check if this element is of the appropriate type
        if(it->getType() == type) {
            out[itemsWritten++] = &(*it);
        }
    }
}

/**
 * Moves the element from `source` to `dest`. This assumes that dest is empty;
 * if it isn't, the results are undefined, and Bad Things may happen.
 */
iolib_error_t Loader::moveElement(Element *src, Element *dest) {
    int err = 0;

    // Check that these are our elements
    CHECK(src != NULL) << "Source must be specified";
    CHECK(dest != NULL) << "Destination must be specified";

    CHECK(src->parent == this) << "Source element is not from this loader";
    CHECK(dest->parent == this) << "Destination element is not from this loader";

    // Ensure driver is open
    _openCh();

    // Prepare the move struct
    struct changer_move move;

    move.cm_fromtype = _convertToChType(src->getType());
    move.cm_fromunit = src->getAddress();
    move.cm_totype = _convertToChType(dest->getType());
    move.cm_tounit = dest->getAddress();

    // Run the ioctl
    err = ioctl(this->fdCh, CHIOMOVE, &move);
    PLOG_IF(ERROR, err != 0) << "Couldn't execute CHIOMOVE on " << this->devCh;

    // Close driver
    _closeCh();

    return err;
}


/**
 * Sends the SCSI INITIALIZE ELEMENT STATUS command, to force the drive to re-
 * scan its inventory. We don't really know when this command will finish, but
 * we do have a timeout we can specify - the default is 30 seconds.
 */
iolib_error_t Loader::performInventory() {
    int err = 0;

    // Ensure driver is open
    _openCh();

    // Run command
    LOG(INFO) << "Waiting for ioctl CHIOIELEM to cook magic smoke for " << this->devCh;
    err = ioctl(this->fdCh, CHIOIELEM, &this->inventoryTimeout);
    PLOG_IF(ERROR, err != 0) << "Couldn't execute CHIOIELEM on " << this->devCh;

    // Close driver
    _closeCh();

    return err;
}

/**
 * Opens the changer device.
 *
 * NOTE: This works on a reference counter principle. This function can be
 * called as many times as desired, and a counter is incremented every time,
 * but the file is only opened if it isn't open yet.
 */
void Loader::_openCh() {
    // Only open the device if it's not already open
    if(this->fdCh == -1) {
        this->fdCh = open(this->devCh, O_RDWR | O_EXLOCK);
        PCHECK(this->fdCh != -1) << "Couldn't open " << this->devCh << ": ";

        this->fdChRefs++;
    } else {
        this->fdChRefs++;
    }
}

/**
 * Closes the changer device, if opened.
 *
 * NOTE: This works on a reference counter principle. This function can be
 * called as many times as desired, and a counter is decremented every time,
 * but the file is only closed if the counter reaches zero.
 */
void Loader::_closeCh() {
    int err = 0;

    if(this->fdCh != -1 && --this->fdChRefs == 0) {
        err = close(this->fdCh);

        if(err != 0) {
            PLOG(ERROR) << "Couldn't close " << this->devCh << ": ";
        }

        this->fdCh = -1;
    }
}

/**
 * Performs the CHIOGPARAMS ioctl against the ch device to determine how many
 * elements of each kind this changer has.
 */
void Loader::_fetchLoaderParams() {
    _openCh();

    int err = 0;
    struct changer_params params;

    // Run the ioctl
    err = ioctl(this->fdCh, CHIOGPARAMS, &params);
    PCHECK(err == 0) << "Couldn't execute CHIOGPARAMS on " << this->devCh;

    // Extract the relevant info
    this->numPickers = params.cp_npickers;
    this->numSlots = params.cp_nslots;
    this->numPortals = params.cp_nportals;
    this->numDrives = params.cp_ndrives;

    VLOG(2) << "\t\t" << this->numPickers << " pickers, "
                      << this->numSlots << " slots, "
                      << this->numPortals << " portals, "
                      << this->numDrives << " drives";

    _closeCh();
}

/**
 * Performs a SCSI READ ELEMENT STATUS command against the changer to get info
 * on each of the kinds of storage elements.
 */
void Loader::_fetchInventory() {
    _openCh();

    int err = 0;
    struct changer_element_status_request request;

    static const u_int types[] = {
        CHET_MT, CHET_ST, CHET_IE, CHET_DT
    };
    static const iolib_storage_element_type_t nativeType[] = {
        kStorageElementTransport, kStorageElementSlot,
        kStorageElementPortal, kStorageElementDrive
    };
    size_t elementsForType[] = {
        this->numPickers, this->numSlots, this->numPortals, this->numDrives
    };

    // Determine the largest out of pickers, slots, portals and drives
    int largest = 0;

    if(this->numPickers > largest) largest = this->numPickers;
    if(this->numSlots > largest) largest = this->numSlots;
    if(this->numPortals > largest) largest = this->numPortals;
    if(this->numDrives > largest) largest = this->numDrives;

    // Allocate a buffer of changer_element_status structs
    struct changer_element_status *elementStat = new changer_element_status[largest];

    // We have four different 'types' of items to query for.
    for(size_t i = 0; i < 4; i++) {
        // Skip if there's no elements of this type
        if(elementsForType[i] == 0) continue;

        // Prepare the struct
        request.cesr_element_type = types[i];
        request.cesr_element_base = 0;
        request.cesr_element_count = elementsForType[i];
        request.cesr_flags = CESR_VOLTAGS; // fetch volume tags, if available
        request.cesr_element_status = elementStat;

        // Perform ioctl
        err = ioctl(this->fdCh, CHIOGSTATUS, &request);
        PCHECK(err == 0) << "Couldn't execute CHIOGSTATUS on " << this->devCh;

        // Create a class for each storage element we've found.
        for(size_t j = 0; j < request.cesr_element_count; j++) {
            Element el(this, nativeType[i], &elementStat[j]);
            this->elements.push_back(el);
        }
    }

    // Delete the memory we allocated previously
    delete[] elementStat;

    _closeCh();
}


/**
 * Converts our internal type to that used by the ch driver.
 */
u_int Loader::_convertToChType(iolib_storage_element_type_t type) {
    if(type == kStorageElementTransport) { // Transport element (picker)
        return CHET_MT;
    } else if(type == kStorageElementSlot) { // Storage element (slot)
        return CHET_ST;
    } else if(type == kStorageElementPortal) { // Portal
        return CHET_IE;
    } else if(type == kStorageElementDrive) { // Data transfer element (drive)
        return CHET_DT;
    }

    // we should not get down here
    return -1;
}

} // namespace iolibbsd
