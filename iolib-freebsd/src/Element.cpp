#include "Element.hpp"
#include "Loader.hpp"

#include <glog/logging.h>
#include <sys/chio.h>

namespace iolibbsd {

/**
 * Creates a new element in the given loader, copying all relevant information
 * out of the chio structure.
 *
 * NOTE: We don't copy the structure, or reference it after this constructor has
 * completed, since it's assumed it's part of a shared buffer.
 */
Element::Element(Loader *loader, iolib_storage_element_type_t type,
                 struct changer_element_status *chElement) {
    CHECK(loader != NULL) << "Element must be created with a parent";
    CHECK(chElement != NULL) << "chio struct must be specified";

    this->parent = loader;

    // Parse the changer element structure
    this->type = type;
    this->address = chElement->ces_addr;
    this->volTag = _stringFromChVolTag(chElement->ces_pvoltag);

    _parseChElementFlags(chElement->ces_flags);
}

/**
 * There's not really anything to do here.
 */
Element::~Element() {

}

/**
 * Parses the ces_flags entry of the changer_element_status structure, and sets
 * the corresponding iolib_storage_element_flags_t.
 */
void Element::_parseChElementFlags(u_char flagsIn) {
    iolib_storage_element_flags_t flags = kStorageElementNoFlags;

    // Is there something in this element?
    if(flagsIn & CES_STATUS_FULL) {
        flags = flags | kStorageElementFull;
    }
    // Was the medium inserted by the operator?
    if(flagsIn & CES_STATUS_IMPEXP) {
        flags = flags | kStorageElementPlacedByOperator;
    }
    // Is the medium in an "exceptional" state (i.e. invalid barcode)?
    if(flagsIn & CES_STATUS_EXCEPT) {
        flags = flags | kStorageElementInvalidLabel;
    }
    // Can the medium be accessed by the picker?
    if(flagsIn & CES_STATUS_ACCESS) {
        flags = flags | kStorageElementAccessible;
    }

    // Does the element support medium export?
    if(flagsIn & CES_STATUS_EXENAB) {
        flags = flags | kStorageElementSupportsExport;
    }
    // Does the element support medium import?
    if(flagsIn & CES_STATUS_INENAB) {
        flags = flags | kStorageElementSupportsImport;
    }

    // asisgn it
    this->flags = flags;
}

/**
 * Extracts the char * component of a changer_voltag_t structure, then creates
 * a string from it. The "serial number" is ignored.
 */
std::string Element::_stringFromChVolTag(changer_voltag_t tag) {
    return std::string(reinterpret_cast<char*>(&tag.cv_volid));
}

} // namespace iolibbsd
