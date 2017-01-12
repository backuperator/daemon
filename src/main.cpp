#include "MainLoop.hpp"
#include "Logging.hpp"
#include "IOLib.h"

/**
 * Daemon entry point
 */
int main(int argc, char *argv[]) {
	Logging::setUp(argv);

    LOG(INFO) << "Starting backuperator-daemon...";

	// Set up the IOLib
    iolibLoadLib();

	LOG(INFO) << "Initializing iolib...";
	iolib_error_t ioErr = iolibInit();
	CHECK(ioErr == 0) << "Error initializing IOLib: " << ioErr;

    // Set up the main run loop
    MainLoop listener;

    listener.run();

    return 0;
}
