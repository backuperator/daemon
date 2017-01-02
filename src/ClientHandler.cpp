#include "ClientHandler.hpp"

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

/**
 * Initializes the client handler, creating a thread for the client, and reading
 * from the socket.
 */
ClientHandler::ClientHandler(int socket) {
	this->socket = socket;

	// create thread
}

/**
 * Clean up
 */
ClientHandler::~ClientHandler() {
	close(this->socket);
}

/**
 * Starts and enters the handler thread.
 */
void ClientHandler::start() {

}
