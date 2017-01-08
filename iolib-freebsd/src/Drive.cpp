#include <glog/logging.h>

#include "Drive.hpp"

namespace iolibbsd {

/**
 * Opens a drive with the given pass-through and sequential access devices.
 */
Drive::Drive(const char *sa, const char *pass) {
    this->devSa = sa;
    this->devPass = pass;
}

/**
 * De-allocates the drive, closing any presently open file handles.
 */
Drive::~Drive() {

}

} // namespace iolibbsd
