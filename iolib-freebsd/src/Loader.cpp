#include <glog/logging.h>

#include <sys/chio.h>
#include <sys/ioctl.h>

#include "Loader.hpp"

namespace iolibbsd {

/**
 * Initializes the loader.
 */
Loader::Loader(const char *ch, const char *pass) {
    this->devCh = ch;
    this->devPass = pass;

    // Attempt to open the device first.
    _openCh();

    // Determine the properties of this changer, then fetch the inventory
    _fetchLoaderParams();
    _fetchInventory();

    // Close the file handle to the device
    _closeCh();
}

Loader::~Loader() {

}

/**
 * Opens the changer device.
 */
void Loader::_openCh() {
    // Only open the device if it's not already open
    if(this->fdCh == -1) {
        this->fdCh = open(this->devCh, O_RDWR | O_EXLOCK);
        PCHECK(this->fdCh != -1) << "Couldn't open " << this->devCh << ": ";
    }

    this->fdChRefs++;
}

/**
 * Closes the changer device, if opened.
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

    LOG(INFO) << "\t" << this->numPickers << " pickers, "
                      << this->numSlots << " slots, "
                      << this->numPortals << " portals, "
                      << this->numDrives << " drives";
}

/**
 * Performs a SCSI READ ELEMENT STATUS command against the changer to get info
 * on each of the kinds of storage elements.
 */
void Loader::_fetchInventory() {
    int err = 0;
    struct changer_element_status_request request;

    static const u_int types[] = {
        CHET_MT, CHET_ST, CHET_IE, CHET_DT
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
            Element el(this, &elementStat[j]);
            this->elements.push_back(el);
        }
    }

    // Delete the memory we allocated previously
    delete[] elementStat;
}

} // namespace iolibbsd
