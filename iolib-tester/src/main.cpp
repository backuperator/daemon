#include "Logging.hpp"
#include "IOLib.h"

#include <iostream>

using namespace std;

static string ElementFlagsToString(iolib_storage_element_flags_t flags);

/**
 * Daemon entry point
 */
int main(int argc, char *argv[]) {
	FLAGS_v = 4;
	Logging::setUp(argv);

	// Set up the IOLib
    iolibLoadLib();

	LOG(INFO) << "Initializing iolib...";
	iolib_error_t ioErr = iolibInit();
	CHECK(ioErr == 0) << "Error initializing IOLib: " << ioErr;

	// Enumerate devices
	LOG(INFO) << "Attempting to enumerate devices...";

	iolib_library_t libs[8];
	size_t numLibs = iolibEnumerateDevices((iolib_library_t *) &libs, 8, NULL);

	LOG(INFO) << "Got " << numLibs << " libraries:";

	for(int i = 0; i < numLibs; i++) {
		LOG(INFO) << "\tLibrary " << i;
		LOG(INFO) << "\t\tDrives: " << libs[i].numDrives << "; " <<
					 "Loaders: " << libs[i].numLoaders;
	}

	// Prompt user to select a library
	string input;
	int selectedLib = 0;

	while(true) {
		cout << "Please enter the library to target: ";
		getline(cin, input);

		// This code converts from string to number safely.
		stringstream myStream(input);
		if(myStream >> selectedLib) {
			// ensure it's in bounds
			if(selectedLib >= numLibs || selectedLib < 0) {
				LOG(ERROR) << "Please enter a valid libary index from the list above.";
				continue;
			}

			break;
		}

		cout << "Invalid number, please try again" << endl;
	}

	LOG(INFO) << "* * * Using library " << selectedLib;

	// Print an inventory, if there's a loader
	iolib_library_t lib = libs[selectedLib];

	if(lib.numLoaders > 0) {
		// Get loader
		LOG(INFO) << "Current inventory:";
		iolib_loader_t loader = lib.loaders[0];

		// Specify some data
		iolib_storage_element_t elements[32];
		size_t numElementsInType = 0;

		static const iolib_storage_element_type_t types[] = {
			kStorageElementTransport, kStorageElementSlot,
			kStorageElementPortal, kStorageElementDrive
		};
		static const string typesStrings[] = {
			"Picker", "Slot", "Portal", "Drive"
		};

		// List off all four types of elements
		for(int i = 0; i < 4; i++) {
			iolibLoaderGetElements(loader, types[i], (iolib_storage_element_t *) &elements, 32);
			size_t numElementsInType = iolibLoaderGetNumElements(loader, types[i], NULL);

			LOG(INFO) << "" << numElementsInType << " elements of type " << typesStrings[i];

			for(int j = 0; j < numElementsInType; j++) {
				iolib_storage_element_t element = elements[j];

				LOG(INFO) << "\tElement " << iolibElementGetAddress(element, NULL) << ": "
						  << "voltag = " << iolibElementGetLabel(element) << " "
						  << "flags: " << ElementFlagsToString(iolibElementGetFlags(element, NULL));
			}
		}
	}

    return 0;
}

static string ElementFlagsToString(iolib_storage_element_flags_t flags) {
	std::stringstream ss;

	ss << "0x" << hex << flags << dec << " ";

	if(flags & kStorageElementFull) {
		ss << "FULL ";
	}
	if(flags & kStorageElementPlacedByOperator) {
		ss << "PLACED_BY_OP ";
	}
	if(flags & kStorageElementInvalidLabel) {
		ss << "LABEL_INVALID ";
	}
	if(flags & kStorageElementAccessible) {
		ss << "ACCESSIBLE ";
	}

	if(flags & kStorageElementSupportsExport) {
		ss << "EXP_SUPPORT ";
	}
	if(flags & kStorageElementSupportsImport) {
		ss << "IMP_SUPPORT ";
	}

	return ss.str();
}
