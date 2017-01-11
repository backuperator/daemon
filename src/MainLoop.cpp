#include "MainLoop.hpp"

#include "ClientHandler.hpp"
#include "BackupJob.hpp"

#include <glog/logging.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <vector>
#include <algorithm>

using namespace std;

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

/// Sends the specified resource.
static void default_resource_send(const HttpServer &server, const shared_ptr<HttpServer::Response> &response, const shared_ptr<ifstream> &ifs);

/**
 * Creates the listener, and initializes a synchronous TCP socket that will be
 * listened on for requests.
 */
MainLoop::MainLoop() {
    // Set up the HTTP server
    this->server.config.port = 7890;

    // Default resource: get contents of "webui" directory
    this->server.default_resource["GET"]=[=](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request> request) {
        try {
            auto web_root_path = boost::filesystem::canonical("webui");
            auto path = boost::filesystem::canonical(web_root_path / request->path);

            //Check if path is within web_root_path
            if(distance(web_root_path.begin(), web_root_path.end())>distance(path.begin(), path.end()) ||
            !equal(web_root_path.begin(), web_root_path.end(), path.begin())) {
                throw invalid_argument("path must be within root path");
            } if(boost::filesystem::is_directory(path)) {
                path /= "index.html";
            } if(!(boost::filesystem::exists(path) && boost::filesystem::is_regular_file(path))) {
                throw invalid_argument("file does not exist");
            }

            string cache_control, etag;

            // Uncomment the following line to enable Cache-Control
            // cache_control="Cache-Control: max-age=86400\r\n";

            #ifdef HAVE_OPENSSL
            // Uncomment the following lines to enable ETag
            // {
            //     ifstream ifs(path.string(), ifstream::in | ios::binary);
            //     if(ifs) {
            //         auto hash=SimpleWeb::Crypto::to_hex_string(SimpleWeb::Crypto::md5(ifs));
            //         etag = "ETag: \""+hash+"\"\r\n";
            //         auto it=request->header.find("If-None-Match");
            //         if(it!=request->header.end()) {
            //             if(!it->second.empty() && it->second.compare(1, hash.size(), hash)==0) {
            //                 *response << "HTTP/1.1 304 Not Modified\r\n" << cache_control << etag << "\r\n\r\n";
            //                 return;
            //             }
            //         }
            //     }
            //     else
            //         throw invalid_argument("could not read file");
            // }
            #endif

            auto ifs = make_shared<ifstream>();
            ifs->open(path.string(), ifstream::in | ios::binary | ios::ate);

            if(*ifs) {
                auto length=ifs->tellg();
                ifs->seekg(0, ios::beg);

                *response << "HTTP/1.1 200 OK\r\n"
                          << cache_control
                          << etag
                          << "Content-Length: " << length
                          << "\r\n\r\n";
                default_resource_send(server, response, ifs);
            }
            // We couldn't open the file :(
            else {
                throw invalid_argument("could not read file");
            }
        }

        // If any exceptions were thrown, report them.
        catch(const exception &e) {
            stringstream message;

            message << "<!doctype html><html><head><style type=\"text/css\">"
                    << "body {"
                    << "font-family: \"DejaVu Sans\", Helvetica, sans-serif;"
                    << "font-size: 11pt; line-spacing: 1.2;"
                    << "}"
                    << "code, pre {"
                    << "font-family: \"DejaVu Sans Mono\", monospaced;"
                    << "}"
                    << "i {"
                    << "font-size: 80%;"
                    << "}"
                    << "</style><body>"
                    << "<h1>An Error Occurred</h1>"
                    << "<p>Could not open <code>" << request->path << "</code>.</p>"
                    << "<h3>Exception Information</h3>"
                    << "<p>Type: <code>" << e.what() << "</code></p>"
                    << "<hr />"
                    << "<i>backuperator-daemon</i>"
                    << "</body></html>";

            // Secrete it to the client
            string content = message.str();
            *response << "HTTP/1.1 400 Bad Request\r\n"
                      << "Content-Length: " << content.length()
                      << "\r\n\r\n" << content;
        }
    };
}

/**
* Destroys the socket and frees resources.
*/
MainLoop::~MainLoop() {
    // Kill the server.
    this->server.stop();
    this->serverThread.join();
}

/**
 * Enters the runloop.
 */
void MainLoop::run() {
    bool shouldRun = true;

    // Start the web server.
    this->serverThread = thread([=](){
        LOG(INFO) << "Started server on port " << this->server.config.port;
        this->server.start();
    });

    // Wait for any events.
    while(shouldRun) {
        sleep(5);
    }

    LOG(INFO) << "Main loop exited; shutting down...";


    /*BackupJob *boop = new BackupJob("../backuptest/");
    boop->start();*/

    // If we get here, the loop should exit. Stop the server.
    this->server.stop();
    this->serverThread.join();
}

/**
 * Sends the given resource to the server.
 */
static void default_resource_send(const HttpServer &server, const shared_ptr<HttpServer::Response> &response,
     const shared_ptr<ifstream> &ifs) {
         // read and send 128 KB at a time
         static vector<char> buffer(131072);
         streamsize read_length;

         // Read until EOF
         if((read_length = ifs->read(&buffer[0], buffer.size()).gcount()) > 0) {
             response->write(&buffer[0], read_length);

             // Was the entire file read?
             if(read_length == static_cast<streamsize>(buffer.size())) {
                 // If so, send it to the server.
                 server.send(response, [&server, response, ifs](const boost::system::error_code &ec) {
                     if(!ec) {
                         default_resource_send(server, response, ifs);
                     } else {
                         cerr << "Connection interrupted" << endl;
                     }
                 });
             }
         }
     }
