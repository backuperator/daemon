#include <glog/logging.h>

#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include "Drive.hpp"

namespace iolibbsd {

/**
 * Opens a drive with the given pass-through and sequential access devices.
 */
Drive::Drive(const char *sa, const char *pass) {
    CHECK(sa != NULL) << "Sequential access device file must be specified";

    this->devSa = sa;
    this->devPass = pass;

    // Determine maximum IO size (for writes/reads)
    _determineMaxIOSize();
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
 * Gets status information from the drive.
 */
iolib_error_t Drive::getDriveStatus(iolib_drive_status_t *status) {
    int err = 0;

    // Ensure device is open
    _openSa();

    // Perform the ioctl
    struct mtget mtStatus;
    err = ioctl(this->fdSa, MTIOCGET, &mtStatus);
    PLOG_IF(ERROR, err != 0) << "Couldn't execute MTIOCGET on " << this->devSa;

    // Extract the status and convert it to our value
    status->deviceStatus = _mtioToNativeStatus(mtStatus.mt_dsreg);
    status->deviceError = mtStatus.mt_erreg;

    // NOTE: The rest of the fields in the struct are not yet implemented

    // Close device once we're done.
    _closeSa();

    return err;

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
 * Gets the drive's current logical block position.
 *
 * NOTE: This may be inaccurate (or entirely wrong) if the drive is not idle.
 */
off_t Drive::getLogicalBlkPos() {
    int err = 0;
    u_int32_t pos;

    // Ensure device is open
    _openSa();

    // Perform the ioctl
    err = ioctl(this->fdSa, MTIOCRDSPOS, &pos);
    PCHECK(err != 0) << "Couldn't execute MTIOCRDSPOS on " << this->devSa;

    // Close device once we're done.
    _closeSa();

    return pos;
}

/**
 * Seeks the drive to the given logical block position.
 */
iolib_error_t Drive::seekToLogicalBlkPos(off_t inPos) {
    int err = 0;
    u_int32_t pos = inPos;

    // Ensure device is open
    _openSa();

    // Perform the ioctl
    err = ioctl(this->fdSa, MTIOCSLOCATE, &pos);
    PLOG_IF(ERROR, err != 0) << "Couldn't execute MTIOCSLOCATE on " << this->devSa;

    // Close device once we're done.
    _closeSa();
    return err;
}

/**
 * Rewinds the tape to the beginning.
 */
iolib_error_t Drive::rewind() {
    int err = 0;

    // Ensure device is open
    _openSa();

    // Perform the ioctl
    struct mtop mt_com;

    mt_com.mt_count = 1;
    mt_com.mt_op = MTREW;

    err = ioctl(this->fdSa, MTIOCTOP, &mt_com);
    PLOG_IF(ERROR, err != 0) << "Couldn't execute MTIOCTOP MTREW on " << this->devSa;

    // Close device once we're done.
    _closeSa();
    return err;
}

/**
 * Ejects the tape currently in the drive.
 *
 * This is implemented as taking the drive offline. Inserting a new tape should
 * bring the drive online again.
 */
iolib_error_t Drive::eject() {
    int err = 0;

    // Ensure device is open
    _openSa();

    // Perform the ioctl
    struct mtop mt_com;

    mt_com.mt_count = 1;
    mt_com.mt_op = MTOFFL;

    err = ioctl(this->fdSa, MTIOCTOP, &mt_com);
    PLOG_IF(ERROR, err != 0) << "Couldn't execute MTIOCTOP MTOFFL on " << this->devSa;

    // Close device once we're done.
    _closeSa();
    return err;
}

/**
 * Writes a file mark at the current position on tape.
 */
iolib_error_t Drive::writeFileMark() {
    int err = 0;

    // Ensure device is open
    _openSa();

    // Perform the ioctl
    struct mtop mt_com;

    mt_com.mt_count = 1;
    mt_com.mt_op = MTWEOF;

    err = ioctl(this->fdSa, MTIOCTOP, &mt_com);
    PLOG_IF(ERROR, err != 0) << "Couldn't execute MTIOCTOP MTWEOF on " << this->devSa;

    // Close device once we're done.
    _closeSa();
    return err;
}


/**
 * Determines the maximum IO size for this drive.
 */
void Drive::_determineMaxIOSize() {
    // Extract the unit number from the device string
    std::string devSaStr(this->devSa);

    size_t last_index = devSaStr.find_last_not_of("0123456789");
    std::string unitNumberStr = devSaStr.substr(last_index + 1);

    int unitNumber = stoi(unitNumberStr);

    // Get the maximum block size for this device
    char sysCtlName[64];
    snprintf(reinterpret_cast<char *>(&sysCtlName), 64,
             "kern.cam.sa.%d.maxio", unitNumber);

    int maxIO;
    size_t maxIOLen = sizeof(int);
    sysctlbyname(sysCtlName, &maxIO, &maxIOLen, NULL, 0);

    this->maxBlockSz = maxIO;

    LOG(INFO) << "\t\tMaximum IO size: " << this->maxBlockSz << " bytes";
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
