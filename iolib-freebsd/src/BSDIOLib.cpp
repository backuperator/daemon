#include <glog/logging.h>

#include "BSDIOLib.hpp"

namespace iolibbsd {

/**
 * Creates the IOLib; this parses the config file into library objects, then
 * creates the backing objects for all of those.
 */
BSDIOLib::BSDIOLib() {
    // Parse config to get the libraries
    _parseConfigFile();

    // Create objects for each library
    for(auto it = this->configLibraries.begin();
             it < this->configLibraries.end(); it++) {
        iolib_config_library_t lib = *it;
        _initLibrary(lib);
    }
}

/**
 * Cleans up by deleting any objects we created. This has the side effect of
 * also closing any open file descriptors.
 */
BSDIOLib::~BSDIOLib() {
    // Delete drives and loaders
    for(auto it = this->libraries.begin();
             it < this->libraries.end(); it++) {
        // delete drives
        for(auto it2 = *it->drives.begin(); it2 <  *it->drives.end(); it2++) {
            delete it2;
        }
        // delete loaders
        for(auto it2 = *it->loaders.begin(); it2 <  *it->loaders.end(); it2++) {
            delete it2;
        }
    }
}

/**
 * Parses the config file into library structs.
 *
 * TODO: Actually read from a config file…
 */
void BSDIOLib::_parseConfigFile() {
    // Put in test data
    this->configLibraries.push_back({
        .numDrives = 1,
        .drives = {
            {
                .passDev = "/dev/pass10",
                .blockDev = "/dev/nsa0"
            }
        },

        .numLoaders = 1,
        .loaders = {
            {
                .passDev = "/dev/pass11",
                .changerDev = "/dev/ch1"
            }
        }
    });

    this->configLibraries.push_back({
        .numDrives = 0,

        .numLoaders = 1,
        .loaders = {
            {
                .passDev = "/dev/pass9",
                .changerDev = "/dev/ch0"
            }
        }
    });
}

/**
 * Initializes a library.
 */
void BSDIOLib::_initLibrary(iolib_config_library_t lib) {
    // Print some info…
    LOG(INFO) << "Processing library with " << lib.numDrives << " drives, "
              << lib.numLoaders << " loaders:";

    library_t library;

    // First, process the drives
    for(size_t i = 0; i < lib.numDrives; i++) {
        LOG(INFO) << "\tDrive " << i << ": " << lib.drives[i].blockDev;

        Drive *drive = new Drive(lib.drives[i].blockDev, lib.drives[i].passDev);
        library.drives.push_back(drive);
    }

    // Then, process the loaders
    for(size_t i = 0; i < lib.numLoaders; i++) {
        LOG(INFO) << "\tLoader " << i << ": " << lib.loaders[i].changerDev;

        Loader *loader = new Loader(lib.loaders[i].changerDev, lib.loaders[i].passDev);
        library.loaders.push_back(loader);
    }

    // Done!
    this->libraries.push_back(library);
}

} // namespace iolibbsd
