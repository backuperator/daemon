/**
 * A single file in a backup job. This serves as a small encapsulation around
 * its file path, name, and some metadata, and is mostly used to create chunks.
 */
#ifndef BACKUPFILE_H
#define BACKUPFILE_H

#include <string>
#include <ctime>

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>

#include <TapeStructs.h>

class BackupFile {
	// allow the Chunk class to access private methods
	friend class Chunk;

	public:
		BackupFile(boost::filesystem::path, BackupFile *);
		~BackupFile();

		int fetchMetadata();

		boost::filesystem::path getPath() {
			return this->path;
		}

	protected:
		boost::filesystem::path path;
		BackupFile *parent;

	// Chunk accessors
	private:
		// approximate length of the file's file struct written to tape
		size_t approxFileHeaderLength = 0;

		// when chunk writing starts, this is set
		bool wasWrittenToChunk = false;
		// once all bytes are written, this is set
		bool fullyWrittenToChunk = false;
		// this is the first byte returned by the next call to getDataOfLength
		off_t lastByte = 0;

		// file descriptor for the file
		FILE *fd = NULL;
		// memory-mapped file
		void *mappedFile = NULL;
		// buffer to the generated file entry structure
		chunk_file_entry_t *fileEntry = NULL;
		size_t fileEntrySize = 0;

		void prepareChunkMetadata();
		void beginReading();
		void finishedReading();

		// how many bytes of the file are left to read
		size_t bytesRemaining();
		// pointer to a buffer containing size bytes; lastByte is advanced.
		void *getDataOfLength(size_t);

	private:
		boost::uuids::uuid uuid;

		bool hasMetadata = false;

		bool isDirectory = false;

		std::time_t lastModified;

		uint32_t mode;
		uint32_t owner, group;

		std::size_t size;
};

#endif
