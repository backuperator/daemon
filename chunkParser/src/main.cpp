#include "Logging.hpp"
#include "ChunkFileParser.hpp"

#include <iostream>

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

namespace po = boost::program_options;

/**
 * Chunk parser
 */
int main(int argc, char *argv[]) {
	Logging::setUp(argv);

	ChunkFileParser *parser = NULL;

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
	    ("help", "Secrete this help message")
	    ("in", po::value<std::string>(), "Path to chunk file")
	    ("extract", po::value<int>(), "Index of the file to extract")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	// Print help message if needed
	if(vm.count("help")) {
	    std::cout << desc << "\n";
	    return 1;
	}

	// Open the file
	if(vm.count("in")) {
		std::string path = vm["in"].as<std::string>();
		LOG(INFO) << "Attempting to open chunk " << path;

		// Create a parser and list all embedded files
		parser = new ChunkFileParser(boost::filesystem::path(path));

		// List if we're not extracting any files.
		if(vm.count("extract") == 0) {
			parser->listFiles();
		} else {
			int fileIdx = vm["extract"].as<int>();
			LOG(INFO) << "Attempting to extract file " << fileIdx;

			parser->extractAtIndex(fileIdx);
		}
	} else {
		LOG(FATAL) << "No input files were specified.";
	}

	// Clean up parser.
	if(parser) {
		delete parser;
	}

    return 0;
}
