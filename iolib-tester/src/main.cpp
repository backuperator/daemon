#include "Logging.hpp"
#include "IOLib.h"
#include "crc32.h"

#include <iostream>
#include <random>
#include <vector>
#include <algorithm>

using namespace std;

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, unsigned char>;

// Maximum space of elements for which to reserve space
#define MAX_ELEMENTS 128

static void WriteBufferToFile(void *buf, size_t len, const char *name);
static iolib_storage_element_t GetSlotAtIndex(iolib_loader_t loader, off_t index);
static iolib_storage_element_t GetDriveAtIndex(iolib_loader_t loader, off_t index);
static iolib_storage_element_t GetElementAtIndex(iolib_loader_t loader, off_t index, iolib_storage_element_type_t type);
static string ElementFlagsToString(iolib_storage_element_flags_t flags);
static int ReadInt();

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
	int selectedLib = 0;

	while(true) {
		cout << "Which library should I use? ";
		selectedLib = ReadInt();

		if(selectedLib >= numLibs || selectedLib < 0) {
			LOG(ERROR) << "Please enter a valid libary index from the list above.";
			continue;
		}

		break;
	}

	LOG(INFO) << "* * * Using library " << selectedLib << " * * *";

	// Print an inventory, if there's a loader
	iolib_library_t lib = libs[selectedLib];

	if(lib.numLoaders > 0) {
		// Get loader
		LOG(INFO) << "Current inventory:";
		iolib_loader_t loader = lib.loaders[0];

		// Specify some data
		iolib_storage_element_t elements[MAX_ELEMENTS];
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
			iolibLoaderGetElements(loader, types[i], (iolib_storage_element_t *) &elements, MAX_ELEMENTS);
			numElementsInType = iolibLoaderGetNumElements(loader, types[i]);

			LOG(INFO) << "" << numElementsInType << " elements of type " << typesStrings[i];

			for(int j = 0; j < numElementsInType; j++) {
				iolib_storage_element_t element = elements[j];

				LOG(INFO) << "\tElement " << iolibElementGetAddress(element) << ": "
						  << "voltag = " << iolibElementGetLabel(element) << " "
						  << "flags: " << ElementFlagsToString(iolibElementGetFlags(element));
			}
		}
	}

	// Ask the user which tape shall be loaded
	int selectedTape = 0;

	cout << "* * * This step will destroy approximately the first gigabyte of "
		 << "data on the selected tape. * * *\n"
		 << "If you don't have an autoloader, enter 0. ";
	cout << "Which tape should I write to? ";
	selectedTape = ReadInt();

	LOG(INFO) << "* * * Using tape " << selectedTape << " * * *";

	if(lib.numLoaders > 0) {
		LOG(INFO) << "### Loading tape from slot " << selectedTape << " to drive 0";

		// If there's an autoloader, load the tape.
		iolib_loader_t loader = lib.loaders[0];
		iolib_storage_element_t element = GetSlotAtIndex(loader, selectedTape);
		iolib_storage_element_t drive = GetDriveAtIndex(loader, 0);

		iolibLoaderMove(loader, element, drive);
	}

	// ... do rw tests ...
	if(lib.numDrives > 0) {
		random_bytes_engine rbe;
		iolib_drive_t drive = lib.drives[0];

		size_t bytesWritten, bytesRead;
		off_t pos;

		LOG(INFO) << "### Generating random buffers...";

		// Allocate two buffers, fill it with random data and calculate CRC
		size_t firstBufSz = (1024 * 1024 * 128) + (1024 * 512);
		size_t secondBufSz = (1024 * 1024 * 50) + (1024 * 332) + 8;

		char *firstBuf = new char[firstBufSz];
		for(size_t i = 0; i < firstBufSz; i++) {
			firstBuf[i] = arc4random() % 255;
		}
		uint32_t firstCrc = crc32c(0, firstBuf, firstBufSz);

		WriteBufferToFile(firstBuf, firstBufSz, "buf1_wr.bin");
		LOG(INFO) << "Generated " << firstBufSz << " bytes of random; CRC = 0x"
				  << hex << firstCrc << dec;

		char *secondBuf = new char[firstBufSz];
		for(size_t i = 0; i < firstBufSz; i++) {
			secondBuf[i] = arc4random() % 255;
		}
		uint32_t secondCrc = crc32c(0, secondBuf, secondBufSz);

		WriteBufferToFile(secondBuf, secondBufSz, "buf2_wr.bin");
		LOG(INFO) << "Generated " << secondBufSz << " bytes of random; CRC = 0x"
				  << hex << secondCrc << dec;

		LOG(INFO) << "### Writing buffers";

		// Write the first buffer.
		LOG(INFO) << "Writing first buffer...";
		bytesWritten = iolibDriveWrite(drive, firstBuf, firstBufSz, true, NULL);
		LOG(INFO) << "\tWrote " << bytesWritten << " bytes, expected " << firstBufSz;

		pos = iolibDriveGetPosition(drive, NULL);
		LOG(INFO) << "Drive ended at block " << pos;


		// Write the second buffer
		LOG(INFO) << "Writing second buffer...";
		bytesWritten = iolibDriveWrite(drive, secondBuf, secondBufSz, true, NULL);
		LOG(INFO) << "\tWrote " << bytesWritten << " bytes, expected " << secondBufSz;

		pos = iolibDriveGetPosition(drive, NULL);
		LOG(INFO) << "Drive ended at block " << pos;


		// Rewind tape before reading
		LOG(INFO) << "### Rewinding tape to beginning...";
		iolibDriveRewind(drive);

		// Clear the buffers before reading into them.
		memset(firstBuf, 0, firstBufSz);
		memset(secondBuf, 0, secondBufSz);

		LOG(INFO) << "### Reading buffers";

		// Attempt to read the first block.
		pos = iolibDriveGetPosition(drive, NULL);
		LOG(INFO) << "Drive starting at block " << pos;

		LOG(INFO) << "Reading first buffer...";
		bytesRead = iolibDriveRead(drive, firstBuf, firstBufSz, NULL);
		WriteBufferToFile(firstBuf, firstBufSz, "buf1_rd.bin");
		LOG(INFO) << "\tRead " << bytesRead << " bytes, expected " << firstBufSz;

		// Since this is an exact read, skip the file mark.
		iolibDriveSkipFile(drive);

		pos = iolibDriveGetPosition(drive, NULL);
		LOG(INFO) << "Drive ended at block " << pos;

		// Now, read the second block
		LOG(INFO) << "Reading second buffer...";
		bytesRead = iolibDriveRead(drive, secondBuf, secondBufSz, NULL);
		WriteBufferToFile(secondBuf, secondBufSz, "buf2_rd.bin");
		LOG(INFO) << "\tRead " << bytesRead << " bytes, expected " << secondBufSz;

		pos = iolibDriveGetPosition(drive, NULL);
		LOG(INFO) << "Drive ended at block " << pos;


		// Calculate the CRC of both blocks
		LOG(INFO) << "### Calculating CRC of read buffers";
		LOG(INFO) << "Calculating CRC of first block";
		uint32_t firstCrcRead = crc32c(0, firstBuf, firstBufSz);

		if(firstCrcRead != firstCrc) {
			LOG(ERROR) << "\tCRC MISMATCH ON FIRST BLOCK! "
					   << "Got 0x" << hex << firstCrcRead << ", expected 0x"
					   << firstCrc << dec;
		} else {
			LOG(INFO) << "\tCRC check on first block succeeded.";
		}

		// Check the second block's CRC
		LOG(INFO) << "Calculating CRC of second block";
		uint32_t secondCrcRead = crc32c(0, secondBuf, secondBufSz);

		if(secondCrcRead != secondCrc) {
			LOG(ERROR) << "\tCRC MISMATCH ON SECOND BLOCK! "
					   << "Got 0x" << hex << secondCrcRead << ", expected 0x"
					   << secondCrc << dec;
		} else {
			LOG(INFO) << "\tCRC check on second block succeeded.";
		}

		// Rewind the tape one last time.
		LOG(INFO) << "### Rewinding tape to beginning...";
		iolibDriveRewind(drive);

		// Eject the tape out of the drive, so that the autoloader can get at it
		LOG(INFO) << "### Ejecting media in drive";
		iolibDriveEject(drive);
	}

	// Unload the tape back into its origin slot.
	if(lib.numLoaders > 0) {
		LOG(INFO) << "### Unloading tape back to original slot...";

		// Unload the tape to the slot where it came from.
		iolib_loader_t loader = lib.loaders[0];
		iolib_storage_element_t element = GetSlotAtIndex(loader, selectedTape);
		iolib_storage_element_t drive = GetDriveAtIndex(loader, 0);

		iolibLoaderMove(loader, drive, element);
	}

    return 0;
}

static void WriteBufferToFile(void *buf, size_t len, const char *name) {
	// Open file
	FILE *fp = fopen(name, "w+");
	PCHECK(fp != NULL) << "Couldn't open " << name << " for writing";

	// Write
	fwrite(buf, len, 1, fp);

	// Close.
	fclose(fp);
}

static iolib_storage_element_t GetSlotAtIndex(iolib_loader_t loader, off_t index) {
	return GetElementAtIndex(loader, index, kStorageElementSlot);
}

static iolib_storage_element_t GetDriveAtIndex(iolib_loader_t loader, off_t index) {
	return GetElementAtIndex(loader, index, kStorageElementDrive);
}

static iolib_storage_element_t GetElementAtIndex(iolib_loader_t loader, off_t index, iolib_storage_element_type_t type) {
	iolib_storage_element_t elements[MAX_ELEMENTS];
	size_t numElementsInType = 0;

	// get all elements of this type pls
	iolibLoaderGetElements(loader, type,
		(iolib_storage_element_t *) &elements, MAX_ELEMENTS);
	numElementsInType = iolibLoaderGetNumElements(loader, type);

	// iterate through them all
	for(int j = 0; j < numElementsInType; j++) {
		iolib_storage_element_t element = elements[j];

		if(iolibElementGetAddress(element) == index) {
			return element;
		}
	}

	// no element found
	return NULL;
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

/// Read an int from stdin
static int ReadInt() {
	string input;
	int theInt;

	while(true) {
		getline(cin, input);

		// This code converts from string to number safely.
		stringstream myStream(input);
		if(myStream >> theInt) {
			return theInt;
		}

		cout << "Invalid number, please try again" << endl;
	}
}
