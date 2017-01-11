/**
 * Main daemon class; this listens for requests on a network socket.
 */
#ifndef DAEMONLISTENER_H
#define DAEMONLISTENER_H

#include <thread>

#include "Simple-Web-Server/server_http.hpp"

class MainLoop {
    public:
        MainLoop();
        ~MainLoop();

        // enters the main run loop, waiting for events
        void run();

    private:
        SimpleWeb::Server<SimpleWeb::HTTP> server;
        std::thread serverThread;

};

#endif
