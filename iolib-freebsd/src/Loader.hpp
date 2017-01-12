/**
 * Wrapper around a loader device.
 *
 * This attempts to cache as much information as possible internally, such as
 * the loader's current inventory. If the loader's inventory changed, perhaps
 * doe to operator intervention, it must be explicitly refreshed.
 */
#ifndef LOADER_HPP
#define LOADER_HPP

#include <fcntl.h>
#include <vector>
#include <atomic>

#include <IOLib_types.h>

#include "Element.hpp"

namespace iolibbsd {

class Loader {
    public:
        Loader(const char *, const char *);
        ~Loader();

		iolib_string_t getDeviceFile();

        size_t getNumElementsForType(iolib_storage_element_type_t);
        void getElementsForType(iolib_storage_element_type_t, size_t, Element **);

        iolib_error_t performInventory();
        iolib_error_t moveElement(Element *, Element *);

    private:
        const char *devCh, *devPass;
        int fdCh = -1, fdPass = -1;
        std::atomic_int fdChRefs, fdPassRefs;

        // Timeout for the INITIALIZE ELEMENT STATUS command, in seconds
        const uint32_t inventoryTimeout = 30;

        // This is information about the loader
        size_t numPickers, numSlots, numPortals, numDrives;

        std::vector<Element> elements;


        void _openCh();
        void _closeCh();

        void _fetchLoaderParams();
        void _fetchInventory();

        u_int _convertToChType(iolib_storage_element_type_t);
};

} // namespace iolibbsd

#endif
