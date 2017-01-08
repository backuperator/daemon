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

} // namespace iolibbsd
