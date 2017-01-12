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
 * Enumerates the libraries, converting them into the shared iolib type that
 * contains references to the drives and loaders in it.
 */
int BSDIOLib::enumerateLibraries(iolib_library_t *libOut, size_t max) {
    size_t librariesOutput = 0;
    int i = 0;

    // Iterate over all of the libraries
    for(auto it = this->libraries.begin(); it < this->libraries.end(); it++) {
        // Check if we've filled the buffer
        if(librariesOutput > max) {
            return max;
        }

        memset(&libOut[librariesOutput], 0, sizeof(iolib_library_t));

		// Set the ID to the offset in the array of the library struct
        char *idStr = reinterpret_cast<char *>(malloc(64));
        snprintf(idStr, 64, "%zu", librariesOutput);
        libOut[librariesOutput].id = idStr;

		// Create a name
        char *nameStr = reinterpret_cast<char *>(malloc(64));
        snprintf(nameStr, 64, "Library %zu", librariesOutput);
        libOut[librariesOutput].name = nameStr;

        // Copy drives
        libOut[librariesOutput].numDrives = it->drives.size();
        for(int j = 0; j < it->drives.size(); j++) {
            libOut[librariesOutput].drives[j] = reinterpret_cast<iolib_drive_t>(it->drives.at(j));
        }

        // Copy loaders
        libOut[librariesOutput].numLoaders = it->loaders.size();
        for(int j = 0; j < it->loaders.size(); j++) {
            libOut[librariesOutput].loaders[j] = reinterpret_cast<iolib_loader_t>(it->loaders.at(j));
        }

        // When done, increment the counter.
        librariesOutput++;
    }

    // Return the number of libraries we output.
    return librariesOutput;
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
    VLOG(1) << "Processing library with " << lib.numDrives << " drives, "
            << lib.numLoaders << " loaders";

    library_t library;

    // First, process the drives
    for(size_t i = 0; i < lib.numDrives; i++) {
        VLOG(1) << "\tDrive " << i << ": " << lib.drives[i].blockDev;

        Drive *drive = new Drive(lib.drives[i].blockDev, lib.drives[i].passDev);
        library.drives.push_back(drive);
    }

    // Then, process the loaders
    for(size_t i = 0; i < lib.numLoaders; i++) {
        VLOG(1) << "\tLoader " << i << ": " << lib.loaders[i].changerDev;

        Loader *loader = new Loader(lib.loaders[i].changerDev, lib.loaders[i].passDev);
        library.loaders.push_back(loader);
    }

    // Done!
    this->libraries.push_back(library);
}

} // namespace iolibbsd
