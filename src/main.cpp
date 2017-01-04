#include "DaemonListener.hpp"
#include "Logging.hpp"

/**
 * Daemon entry point
 */
int main(int argc, char *argv[]) {
	Logging::setUp(argv);

    // Set up the listener
    DaemonListener listener;

    // Enter the main socket wait loop
    listener.startListening();

    return 0;
}
