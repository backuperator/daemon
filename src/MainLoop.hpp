/**
 * Main daemon class; this listens for requests on a network socket.
 */
#ifndef DAEMONLISTENER_H
#define DAEMONLISTENER_H

#include <thread>

#include "Simple-Web-Server/server_http.hpp"
#include "WWWAPIHandler.hpp"

class MainLoop {
    public:
        MainLoop();
        ~MainLoop();

        // enters the main run loop, waiting for events
        void run();

    private:
        SimpleWeb::Server<SimpleWeb::HTTP> server;
        std::thread serverThread;

        WWWAPIHandler handler;

		void errorForException(std::exception, std::string, std::string, std::shared_ptr<SimpleWeb::Server<SimpleWeb::HTTP>::Response>);
};

#endif
