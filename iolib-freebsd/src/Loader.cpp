#include <glog/logging.h>

#include "Loader.hpp"

namespace iolibbsd {

/**
 * Initializes the loader.
 */
Loader::Loader(const char *ch, const char *pass) {
    this->devCh = ch;
    this->devPass = pass;
}

Loader::~Loader() {

}

} // namespace iolibbsd
