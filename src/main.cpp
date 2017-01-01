#include "DaemonListener.hpp"

/**
 * Daemon entry point
 */
int main(int argc, char *argv[]) {
    // Set up the listener
    DaemonListener listener;

    // Enter the main socket wait loop
    listener.startListening();

    return 0;
}
