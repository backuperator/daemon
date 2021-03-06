/**
 * Handles API requests for the web API. This will take in a property tree as
 * input (if applicable), then output a property tree. The web server will
 * serialize/unserialize as needed.
 *
 * NOTE: The API does absolutely no verification or authentication of the
 * client. It should probably be served behind some sort of reverse-proxy (such
 * as nginx) with HTTP authentication to protect it.
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
		nlohmann::json _jsonForElement(iolib_storage_element_t);

		std::string _stdStringFromIoLibString(iolib_string_t);
};

#endif
