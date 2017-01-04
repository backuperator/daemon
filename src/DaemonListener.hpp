/**
 * Main daemon class; this listens for requests on a network socket.
 */
#ifndef DAEMONLISTENER_H
#define DAEMONLISTENER_H

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

class DaemonListener {
    public:
        DaemonListener();
        ~DaemonListener();

        void startListening();

    protected:
        void handleClient(int, struct sockaddr_storage);

    private:
        int socket_fd;
};

#endif
