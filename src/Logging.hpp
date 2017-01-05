/**
 * Defines a static class that defines how to do logging.
 */
#ifndef LOGGING_H
#define LOGGING_H

#include <glog/logging.h>

class Logging {
	public:
		static void setUp(char *argv[]);
};

#endif
