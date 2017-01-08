/**
 * Wrapper around a storage element.
 *
 * This carries with it the metadata on a specific tape that might be contained
 * within as well. Note that we don't know anything about which magazine a
 * certain element falls into, since this geometry is not made available via
 * SCSI calls.
 */
#ifndef ELEMENT_HPP
#define ELEMENT_HPP

#include <sys/types.h>
#include <cstdint>
#include <string>

// forrward declare this; it's defined in sys/chio.h
struct changer_element_status;

namespace iolibbsd {

class Loader;

class Element {
    public:
        Element(Loader *, struct changer_element_status *);
        ~Element();

    private:
        off_t index;
        Loader *parent;
};

} // namespace iolibbsd

#endif
