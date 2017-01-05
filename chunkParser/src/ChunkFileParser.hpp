/**
 * Opens the file specified, maps it into memory, and attempts to parse it as
 * a chunk.
 */
#ifndef CHUNKFILEPARSER_H
#define CHUNKFILEPARSER_H

#include <boost/filesystem.hpp>

#include <cstdio>

class ChunkFileParser {
	public:
		ChunkFileParser(boost::filesystem::path);
		~ChunkFileParser();

		void listFiles();

	private:
		FILE *fd;
		void *mappedFile;
		size_t size;


		void _parseHeader();

		const char *_nameForUid(int);
		const char *_nameForGid(int);
};

#endif
