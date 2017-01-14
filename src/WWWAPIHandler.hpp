/**
 * Handles API requests for the web API. This will take in a property tree as
 * input (if applicable), then output a property tree. The web server will
 * serialize/unserialize as needed.
 */
#ifndef WWWAPIHANDLER_H
#define WWWAPIHANDLER_H

#include <string>
#include <json.hpp>

#include "IOLib.h"

class WWWAPIHandler {
	public:
		WWWAPIHandler();
		~WWWAPIHandler();

		nlohmann::json handle(std::string, std::string, nlohmann::json);

	private:
		nlohmann::json _getAllLibraries();
		size_t _getNumElementsForLoader(iolib_loader_t);
};

#endif
