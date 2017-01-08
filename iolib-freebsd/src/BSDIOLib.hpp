/**
 * This is the wrapper that contains all the other objects for the FreeBSD IO
 * lib.
 */
#ifndef BSDIOLIB_H
#define BSDIOLIB_H

#include <cstdint>
#include <vector>

#include "Drive.hpp"
#include "Loader.hpp"

namespace iolibbsd {

/**
 * Structure describing a tape library; this will be parsed from a user-supplied
 * configuration file.
 */
typedef struct {
	// How many tape drives are there?
	size_t numDrives;
	// Tape drive structures; this just describes the block and pass-thru device
	struct {
		// Pass-through device path
		const char *passDev;
		// Block non-rewind device
		const char *blockDev;
	} drives[16];

	// How many loaders are there?
	size_t numLoaders;
	// Loader structures
	struct {
		// Pass-through device
		const char *passDev;
		// ch(4) device
		const char *changerDev;
	} loaders[4];
} iolib_config_library_t;

typedef struct {
	std::vector<Drive> drives;
	std::vector<Loader> loaders;
} library_t;


class BSDIOLib {
    public:
		BSDIOLib();
		~BSDIOLib();

    private:
		std::vector<iolib_config_library_t> configLibraries;
		std::vector<library_t> libraries;


		void _parseConfigFile();
		void _initLibrary(iolib_config_library_t);
};

} // namespace iolibbsd
#endif
