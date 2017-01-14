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
	vector<json> elements;

    // Fetch all libraries
	iolib_library_t libs[8];
	size_t numLibs = iolibEnumerateDevices((iolib_library_t *) &libs, 8, NULL);

	for(size_t i = 0; i < numLibs; i++) {
        vector<string> driveIds;
        vector<string> loaderIds;

        // Create JSON objects for any drives.
        for(size_t j = 0; j < libs[i].numDrives; j++) {
            iolib_drive_t drive = libs[i].drives[j];

			// Get UUID
			iolib_string_t rawUuid = iolibDriveGetUuid(drive);
			string uuid = string(rawUuid);
			iolibStringFree(rawUuid);

			// Create drive object
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

			// Get UUID
			iolib_string_t rawUuid = iolibLoaderGetUuid(loader);
			string uuid = string(rawUuid);
			iolibStringFree(rawUuid);


			// Process all of the elements
			vector<string> loaderElementIds;
			size_t numElements = 0;

			static const iolib_storage_element_type_t types[] = {
				kStorageElementTransport,
				kStorageElementDrive,
				kStorageElementPortal,
				kStorageElementSlot
			};

			for(size_t i = 0; i < (sizeof(types) / sizeof(*types)); i++) {
				// Get the number of elements
				size_t elmsOfType = iolibLoaderGetNumElements(loader, types[i]);
				numElements += elmsOfType;

				iolib_storage_element_t elms[elmsOfType];

				// Get all the elements
				iolibLoaderGetElements(loader, types[i],
					reinterpret_cast<iolib_storage_element_t *>(&elms),
					elmsOfType);

				// ...and now, process them.
				for(size_t j = 0; j < elmsOfType; j++) {
					iolib_storage_element_t element = elms[j];

					json elementJson = _jsonForElement(element);

					elements.push_back(elementJson);
					loaderElementIds.push_back(elementJson["id"]);
				}
			}


			// Create loader entry
            loaders.push_back({
                {"id", uuid},
                {"name", iolibLoaderGetName(loader)},
                {"file", iolibLoaderGetDevFile(loader)},
				{"elements", loaderElementIds}
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
        {"element", elements},
    };
}

/**
 * Constructs a json object for a loader's storage element.
 */
json WWWAPIHandler::_jsonForElement(iolib_storage_element_t element) {
	json elementJson;

	// Get UUID
	iolib_string_t rawUuid = iolibElementGetUuid(element);
	string uuid = string(rawUuid);
	iolibStringFree(rawUuid);

	elementJson["id"] = uuid;
	// Get logical element address
	elementJson["address"] = iolibElementGetAddress(element);
	// Check the flags - is it empty?
	elementJson["isEmpty"] = !(iolibElementGetFlags(element) & kStorageElementFull);

	// Populate the type
	switch(iolibElementGetType(element)) {
		case kStorageElementDrive:
			elementJson["kind"] = "drive";
			break;

		case kStorageElementSlot:
			elementJson["kind"] = "storage";
			break;

		case kStorageElementPortal:
			elementJson["kind"] = "portal";
			break;

		case kStorageElementTransport:
			elementJson["kind"] = "transport";
			break;

		default:
			break;
	}

	// And lastly, the volume tag.
	iolib_string_t rawLabel = iolibElementGetLabel(element);
	string label = string(rawLabel);
	iolibStringFree(rawLabel);

	elementJson["label"] = label;

	return elementJson;
}
