#include "WWWAPIHandler.hpp"

#include "IOLib.h"

#include <stdlib.h>
#include <glog/logging.h>
#include <boost/regex.hpp>

using json = nlohmann::json;
using namespace boost;
using namespace std;

// Create regexes for all URLs we support.
static regex _exprLibraries("^\\/api\\/libraries$");


/**
 * Initializes the handler.
 */
WWWAPIHandler::WWWAPIHandler() {

}

WWWAPIHandler::~WWWAPIHandler() {

}

/**
 * Handles a request.
 *
 * @param method HTTP method; may be GET or POST.
 * @param params Input parameters for the request.
 */
json WWWAPIHandler::handle(string method, string url, json params) {
    cmatch what;


    // Print
    LOG(INFO) << "API Request " << method << " " << url << ": " << params;

    // Listing all libraries?
    if(regex_match(url.c_str(), what, _exprLibraries)) {
        return _getAllLibraries();
    }

    // Unknown API request; log it.
    else {
        LOG(WARNING) << "Unknown API Request " << method << " " << url << ": " << params;
    }

	// We should never get down here
	return {};
}

/**
 * Fetches all libraries, including drives and loaders.
 */
json WWWAPIHandler::_getAllLibraries() {
    vector<json> libraries;
    vector<json> drives;
    vector<json> loaders;

    // Fetch all libraries
	iolib_library_t libs[8];
	size_t numLibs = iolibEnumerateDevices((iolib_library_t *) &libs, 8, NULL);

	for(size_t i = 0; i < numLibs; i++) {
        vector<string> driveIds;
        vector<string> loaderIds;

        // Create JSON objects for any drives.
        for(size_t j = 0; j < libs[i].numDrives; j++) {
            iolib_drive_t drive = libs[i].drives[j];

			iolib_string_t rawUuid = iolibDriveGetUuid(drive);
			string uuid = string(rawUuid);
			iolibStringFree(rawUuid);

            drives.push_back({
                {"id", uuid},
                {"name", iolibDriveGetName(drive)},
                {"file", iolibDriveGetDevFile(drive)}
                // {"library", libs[i].id}
            });
            driveIds.push_back(uuid);
        }

        // Create JSON objects for any loaders.
        for(size_t j = 0; j < libs[i].numLoaders; j++) {
            iolib_loader_t loader = libs[i].loaders[j];

			iolib_string_t rawUuid = iolibLoaderGetUuid(loader);
			string uuid = string(rawUuid);
			iolibStringFree(rawUuid);

			// Determine the number of elements in this loader
			size_t numElements = _getNumElementsForLoader(loader);

			LOG(INFO) << "Have " << numElements << " elements in loader";

            loaders.push_back({
                {"id", uuid},
                {"name", iolibLoaderGetName(loader)},
                {"file", iolibLoaderGetDevFile(loader)}
                // {"library", libs[i].id}
            });
            loaderIds.push_back(uuid);
        }

        // Insert the JSON object for the library.
        libraries.push_back({
            {"id", libs[i].id},
            {"name", libs[i].name},
            {"drives", driveIds},
            {"loaders", loaderIds},
        });
    }

    // Construct the response
    return {
        {"libraries", libraries},
        {"drives", drives},
        {"loaders", loaders},
    };
}

/**
 * Counts the number of elements in this loader: in our case, this consists of
 * storage elements and drives.
 */
size_t WWWAPIHandler::_getNumElementsForLoader(iolib_loader_t loader) {
    size_t numDrives = 0;

    // Get number of storage elements
    numDrives += iolibLoaderGetNumElements(loader, kStorageElementSlot, NULL);

    // And number of drives
    numDrives += iolibLoaderGetNumElements(loader, kStorageElementDrive, NULL);

    return numDrives;
}
