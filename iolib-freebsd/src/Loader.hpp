/**
 * Wrapper around a loader device.
 *
 * This attempts to cache as much information as possible internally, such as
 * the loader's current inventory. If the loader's inventory changed, perhaps
 * doe to operator intervention, it must be explicitly refreshed.
 */
#ifndef LOADER_HPP
#define LOADER_HPP

namespace iolibbsd {

class Loader {
    public:
        Loader(const char *, const char *);
        ~Loader();

    private:
        const char *devCh, *devPass;
};

} // namespace iolibbsd;

#endif
