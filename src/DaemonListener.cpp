#include "DaemonListener.hpp"

#include <glog/logging.h>

#include "ClientHandler.hpp"
#include "BackupJob.hpp"

using namespace std;

/**
 * Creates the listener, and initializes a synchronous TCP socket that will be
 * listened on for requests.
 */
DaemonListener::DaemonListener() {
    // Resolve the local address
    const char* hostname = NULL;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET; // could be AF_UNSPEC, but that's broken somehow
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = (AI_PASSIVE);

    struct addrinfo *res = NULL;
    int err = getaddrinfo(hostname, "5583", &hints, &res);

    if(err != 0) {
        LOG(FATAL) << "Failed to resolve local address: " << err;
    }

    // Create the server socket
    this->socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(this->socket_fd == -1 || errno != 0) {
        PLOG(FATAL) << "Couldn't crerate socket";
    }

    // Allow re-use of address
    int reuseaddr = 1;
    if(setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
                  sizeof(reuseaddr)) == -1) {
        PLOG(FATAL) << "Couldn't set SO_REUSEADDR";
    }

    // Bind to address
	errno = 0;
    bind(this->socket_fd, res->ai_addr, res->ai_addrlen);

    if(errno != 0) {
        PLOG(FATAL) << "Couldn't bind socket";
    }

    freeaddrinfo(res);

    // Listen on the address
    if(listen(this->socket_fd, SOMAXCONN)) {
        PLOG(FATAL) << "Could not open socket for listening";
    }
}

/**
 * Destroys the socket and frees resources.
 */
DaemonListener::~DaemonListener() {
    close(this->socket_fd);
}

/**
 * Open the listening socket and accept connections. Each connection will have
 * its own process created via the use of fork(2).
 */
 void DaemonListener::startListening() {
     LOG(INFO) << "Waiting for connections..." << endl;

	 // lel
	 BackupJob *boop = new BackupJob("/Users/tristan/temp/backuptest/");
	 boop->start();

     for(;;) {
         // Accept connection, blocking if needed
         struct sockaddr_storage sa;
         socklen_t sa_len = sizeof(sa);

         int session_fd = accept(this->socket_fd, (struct sockaddr *) &sa, &sa_len);

         // Check for error
         if(session_fd == -1) {
             if(errno == EINTR) {
                 continue;
             }

             PLOG(ERROR) << "Could not accept connection: ";
         }

         // Handle the client's requests
         this->handleClient(session_fd, sa);
     }
 }

/**
 * Handles a client, given their socket handle. This will create a new handler
 * object, and in turn, a client handling thread.
 */
void DaemonListener::handleClient(int fd, struct sockaddr_storage client_addr) {
	// Create a client handler
	ClientHandler *handler = new ClientHandler(fd);

	// Start the handler thread
	handler->start();
}
