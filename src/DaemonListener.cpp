#include "DaemonListener.hpp"
#include "ClientHandler.hpp"

#include <iostream>

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

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = (AI_PASSIVE | AI_ADDRCONFIG);

    struct addrinfo *res = NULL;
    int err = getaddrinfo(hostname, "5583", &hints, &res);

    if(err != 0) {
        cout << "Failed to resolve local address: " << err << endl;
        exit(-1);
    }

    // Create the server socket
    this->socket_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(this->socket_fd == -1) {
        cerr << "Couldn't crerate socket: " << strerror(errno) << endl;
        exit(-1);
    }

    // Allow re-use of address
    int reuseaddr = 1;
    if(setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
                  sizeof(reuseaddr)) == -1) {
        cerr << "Couldn't set SO_REUSEADDR: " << strerror(errno) << endl;
    }

    // Bind to address
    bind(this->socket_fd, res->ai_addr, res->ai_addrlen);

    if(errno != 0) {
        cerr << "Couldn't bind socket: " << strerror(errno) << endl;
        exit(-1);
    }

    freeaddrinfo(res);

    // Listen on the address
    if(listen(this->socket_fd, SOMAXCONN)) {
        cout << "Could not open socket for listening: " << errno << endl;
        exit(-1);
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
     cout << "Waiting for connections..." << endl << flush;

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

             cout << "Could not accept connection: " << errno << endl;
         }

         // Handle the client's requests
         this->handleClient(session_fd);
     }
 }

/**
 * Handles a client, given their socket handle. This will create a new handler
 * object, and in turn, a client handling thread.
 */
void DaemonListener::handleClient(int fd) {
	// Create a client handler
	ClientHandler *handler = new ClientHandler(fd);

	// Start the handler thread
	handler->start();
}
