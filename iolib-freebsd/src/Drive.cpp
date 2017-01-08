#include <glog/logging.h>

#include <sys/mtio.h>
#include <sys/ioctl.h>

#include "Drive.hpp"

namespace iolibbsd {

/**
 * Opens a drive with the given pass-through and sequential access devices.
 */
Drive::Drive(const char *sa, const char *pass) {
    CHECK(sa != NULL) << "Sequential access device file must be specified";

    this->devSa = sa;
    this->devPass = pass;
}

/**
 * De-allocates the drive, closing any presently open file handles.
 */
Drive::~Drive() {
    // Use the close method, but force it by setting reference count to 1
    LOG_IF(WARNING, this->fdSaRefs > 1) << "More than one open reference on file descriptor at dellocation";

    this->fdSaRefs = 1;
    _closeSa();
}


/**
 * Queries the drive with a GET UNIT STATUS command to determine its current
 * state, using the MTIOCGET ioctl.
 */
iolib_drive_operation_t Drive::getDriveOp() {
    iolib_drive_operation_t status = kDriveStatusUnknown;
    int err = 0;

    // Ensure device is open
    _openSa();

    // Perform the ioctl
    struct mtget mtStatus;
    err = ioctl(this->fdSa, MTIOCGET, &mtStatus);
    PLOG_IF(ERROR, err != 0) << "Couldn't execute MTIOCGET on " << this->devSa;

    // Extract the status and convert it to our value
    status = _mtioToNativeStatus(mtStatus.mt_dsreg);

    // Close device once we're done.
    _closeSa();

    return status;
}



/**
 * Opens the sequential access device.
 *
 * NOTE: This works on a reference counter principle. This function can be
 * called as many times as desired, and a counter is incremented every time,
 * but the file is only opened if it isn't open yet.
 */
void Drive::_openSa() {
    // Only open the device if it's not already open
    if(this->fdSa == -1) {
        this->fdSa = open(this->devSa, O_RDWR | O_EXLOCK);
        PCHECK(this->fdSa != -1) << "Couldn't open " << this->devSa << ": ";

        this->fdSaRefs++;
    } else {
        this->fdSaRefs++;
    }
}

/**
 * Closes the sequential access device, if opened.
 *
 * NOTE: This works on a reference counter principle. This function can be
 * called as many times as desired, and a counter is decremented every time,
 * but the file is only closed if the counter reaches zero.
 */
void Drive::_closeSa() {
    int err = 0;

    if(this->fdSa != -1 && --this->fdSaRefs == 0) {
        err = close(this->fdSa);

        if(err != 0) {
            PLOG(ERROR) << "Couldn't close " << this->devSa << ": ";
        }

        this->fdSa = -1;
    }
}


/**
 * Converts an mtio status value to the iolib native type.
 */
iolib_drive_operation_t Drive::_mtioToNativeStatus(short dsreg) {
    switch(dsreg) {
        case MTIO_DSREG_REST:
            return kDriveStatusIdle;

        case MTIO_DSREG_WR:
            return kDriveStatusWritingData;
        case MTIO_DSREG_FMK:
            return kDriveStatusWritingMetadata;
        case MTIO_DSREG_ZER:
            return kDriveStatusErasing;
        case MTIO_DSREG_RD:
            return kDriveStatusReading;

        case MTIO_DSREG_FWD:
            return kDriveStatusSeekingForwards;
        case MTIO_DSREG_REV:
            return kDriveStatusSeekingBackwards;
        case MTIO_DSREG_REW:
            return kDriveStatusRewinding;

        case MTIO_DSREG_TEN:
            return kDriveStatusRetensioning;
        case MTIO_DSREG_UNL:
            return kDriveStatusUnloading;
        case MTIO_DSREG_LD:
            return kDriveStatusLoading;

        // We have an "other" status
        default:
            return kDriveStatusOther;
    }
}

} // namespace iolibbsd
