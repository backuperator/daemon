/**
 * Wrapper around a loader device.
 *
 * This attempts to cache as much information as possible internally, such as
 * the loader's current inventory. If the loader's inventory changed, perhaps
 * doe to operator intervention, it must be explicitly refreshed.
 */
#ifndef LOADER_HPP
#define LOADER_HPP

#include <vector>
#include <atomic>

#include <fcntl.h>

#include "Element.hpp"

namespace iolibbsd {

class Loader {
    public:
        Loader(const char *, const char *);
        ~Loader();

        size_t getNumElementsForType(iolib_storage_element_type_t);

    private:
        const char *devCh, *devPass;
        int fdCh = -1, fdPass = -1;
        std::atomic_int fdChRefs, fdPassRefs;

        // This is information about the loader
        size_t numPickers, numSlots, numPortals, numDrives;

        std::vector<Element> elements;

        void _openCh();
        void _closeCh();

        void _fetchLoaderParams();
        void _fetchInventory();
};

} // namespace iolibbsd

#endif
