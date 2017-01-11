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
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>

using namespace std;
using namespace CryptoPP;

typedef SimpleWeb::Server<SimpleWeb::HTTP> HttpServer;

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

            // Open the file…
            auto ifs = make_shared<ifstream>();
            ifs->open(path.string(), ifstream::in | ios::binary);

            // Check that it was opened
            if(!(*ifs)) {
               // The stream could not be opened.
               throw invalid_argument("could not read file");
            }

            // Determine its size
            ifs->seekg(0, ios::end);
            auto length = ifs->tellg();
            ifs->seekg(0, ios::beg);

            // Read it into a temporary buffer
            string fileContents;
            fileContents.reserve(length);

            fileContents.assign((istreambuf_iterator<char>(*ifs)), istreambuf_iterator<char>());

            // Calculate its hash
            SHA256 hash;
            string digest;

            StringSource s(fileContents, true, new HashFilter(hash, new HexEncoder(new StringSink(digest))));


            etag = "ETag: \"" + digest + "\"\r\n";

            // Does this etag match what the browser's asking for?
            auto it = request->header.find("If-None-Match");

            if(it != request->header.end()) {
                // If so, return a 304.
                if(!it->second.empty() && it->second.compare(1, digest.size(), digest) == 0) {
                    *response << "HTTP/1.1 304 Not Modified\r\n" << cache_control << etag << "\r\n\r\n";
                    return;
                }
            }

            // Then respond with its contents.
            *response << "HTTP/1.1 200 OK\r\n"
                      << cache_control
                      << etag
                      << "Content-Length: " << length
                      << "\r\n\r\n";
            *response << fileContents;
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

            LOG(WARNING) << "Error handling request for "
                         << request->path << ": " << e.what() << "; "
                         << "from " << request->remote_endpoint_address;

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
