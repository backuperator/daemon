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

#include <IOLib_types.h>

// forrward declare these structs; it's defined in sys/chio.h
struct changer_element_status;
struct changer_voltag;
typedef changer_voltag changer_voltag_t;

namespace iolibbsd {

class Loader;

class Element {
    friend class Loader;

    public:
        Element(Loader *, iolib_storage_element_type_t, struct changer_element_status *);
        ~Element();

        iolib_storage_element_type_t getType() {
            return this->type;
        }
        off_t getAddress() {
            return this-> address;
        }
        iolib_storage_element_flags_t getFlags() {
            return this->flags;
        }
        std::string getVolumeTag() {
            return this->volTag;
        }

    private:
        Loader *parent;

        // Type of the element
        iolib_storage_element_type_t type;
        // Logical address of this element in the changer
        off_t address;
        // Flags that define the state of this element
        iolib_storage_element_flags_t flags;
        // Volume tag (barcode) if present
        std::string volTag;


        void _parseChElementFlags(u_char);
        std::string _stringFromChVolTag(changer_voltag_t);
};

} // namespace iolibbsd

#endif
