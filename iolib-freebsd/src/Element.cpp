#include "Element.hpp"
#include "Loader.hpp"

#include <sys/chio.h>

namespace iolibbsd {

/**
 * Creates a new element in the given loader, copying all relevant information
 * out of the chio structure.
 *
 * NOTE: We don't copy the structure, or reference it after this constructor has
 * completed, since it's assumed it's part of a shared buffer.
 */
Element::Element(Loader *loader, struct changer_element_status *chElement) {
    this->parent = loader;

    // Perform a SCSI request against the loader to get info about this volume
}

Element::~Element() {

}

} // namespace iolibbsd
