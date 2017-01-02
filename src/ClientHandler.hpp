#include <sys/socket.h>

/**
 * Client request handler
 */
class ClientHandler {
    public:
        ClientHandler(int);
        ~ClientHandler();

		void start();

    protected:

    private:
        int socket;
};
