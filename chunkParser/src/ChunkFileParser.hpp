/**
 * Opens the file specified, maps it into memory, and attempts to parse it as
 * a chunk.
 */
#ifndef CHUNKFILEPARSER_H
#define CHUNKFILEPARSER_H

#include <boost/filesystem.hpp>

#include <cstdio>

#include "TapeStructs.h"

class ChunkFileParser {
	public:
		ChunkFileParser(boost::filesystem::path);
		~ChunkFileParser();

		void listFiles();

		void extractAtIndex(off_t);

	private:
		FILE *fd;
		void *mappedFile;
		size_t size;


		void _parseHeader();

		void _printFileInfo(size_t, chunk_file_entry_t *);
		const char *_nameForUid(int);
		const char *_nameForGid(int);
};

#endif
