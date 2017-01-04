#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

/**
 * Main daemon class; this listens for requests on a network socket.
 */
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
