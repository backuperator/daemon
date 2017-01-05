#include "Logging.hpp"

/**
 * Sets up the logger
 */
void Logging::setUp(char *argv[]) {
	// Set some defaults
	FLAGS_logtostderr = 1;

	// Init logging library and signal handler
	google::InitGoogleLogging(argv[0]);
	google::InstallFailureSignalHandler();
}
