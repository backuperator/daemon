/**
 * Client request handler
 */
#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <sys/socket.h>

class ClientHandler {
    public:
        ClientHandler(int);
        ~ClientHandler();

		void start();

    protected:

    private:
        int socket;
};

#endif
