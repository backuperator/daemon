#include "DaemonListener.hpp"
#include "Logging.hpp"
#include "IOLib.h"

/**
 * Daemon entry point
 */
int main(int argc, char *argv[]) {
	Logging::setUp(argv);

	// Set up the IOLib
    iolibLoadLib();

	LOG(INFO) << "Initializing iolib...";
	iolib_error_t ioErr = iolibInit();
	CHECK(ioErr == 0) << "Error initializing IOLib: " << ioErr;

    sleep(5);

    // Set up the listener
    DaemonListener listener;

    // Enter the main socket wait loop
    listener.startListening();

    return 0;
}
