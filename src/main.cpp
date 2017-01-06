#include "DaemonListener.hpp"
#include "Logging.hpp"
#include "IOLib.h"

/**
 * Daemon entry point
 */
int main(int argc, char *argv[]) {
	Logging::setUp(argv);
    iolibLoadLib();

    // Set up the listener
    DaemonListener listener;

    // Enter the main socket wait loop
    listener.startListening();

    return 0;
}
