#include "BackupFile.hpp"

#include <glog/logging.h>

#include <iostream>
#include <cstdio>
#include <sys/stat.h>
#include <sys/mman.h>

#include <boost/uuid/uuid_generators.hpp>

using std::cerr;
using boost::filesystem::path;

/**
 * Creates a file object from the file with the given path. This does not load
 * any data from disk yet - metadata is only loaded when requested.
 */
BackupFile::BackupFile(boost::filesystem::path path, BackupFile *parent) {
	this->path = path;
	this->parent = parent;

	// Create an UUID for the file
	boost::uuids::basic_random_generator<boost::mt19937> gen;
	this->uuid = gen();
}

/**
 * Cleans up any stored information.
 */
BackupFile::~BackupFile() {
	// finish reading, if needed
	this->finishedReading();
}

/**
 * Fetches metadata in a blocking fashion.
 */
int BackupFile::fetchMetadata() {
	int err = 0;

	// Check if metadata for this file was already fetched.
	if(this->hasMetadata == true) {
		return 0;
	}

	// Check that the path exists.
	if(exists(this->path) == false) {
		return -1;
	}

	// Is it a directory?
	this->isDirectory = is_directory(this->path);

	// Run stat(2) to get info about the file.
	const char *filepath = this->path.c_str();

	struct stat info;
	memset(&info, 0, sizeof(info));

	err = stat(filepath, &info);

	if(err != 0) {
		cerr << "Couldn't get info on file " << filepath << ": " << errno << std::endl;
		return -1;
	}

	// Extract relevant info
	this->lastModified = info.st_mtime;

	this->mode = info.st_mode;
	this->owner = info.st_uid;
	this->group = info.st_gid;

	this->size = info.st_size;

	// assume success
	this->hasMetadata = true;

	return 0;
}


/**
 * Prepares the file for reading. This will allocate its metadata structure for
 * more accurate file size tracking, as well as mapping the entire file into
 * address space for memory-mapped access.
 */
void BackupFile::beginReading() {
	// Fetch file data
	this->fetchMetadata();

	// Calculate the size of the metadata header
	size_t structSize = sizeof(chunk_file_entry_t);
	size_t nameLength = strlen(this->path.c_str()) + 1; // +1 for NULL byte

	size_t fullSize = (structSize + nameLength);

	// Allocate the struct and write all the info we have into it
	this->fileEntry = (chunk_file_entry_t *) malloc(fullSize);
	memset(this->fileEntry, 0, fullSize);

	this->fileEntry->nameLenBytes = nameLength;
	memcpy(&this->fileEntry->name, this->path.c_str(), nameLength);

	this->fileEntry->type = (this->isDirectory) ? kTypeDirectory : kTypeFile;

	this->fileEntry->timeModified = this->lastModified;

	this->fileEntry->owner = this->owner;
	this->fileEntry->group = this->group;
	this->fileEntry->mode = this->mode;

	this->fileEntry->size = this->size;

	// open the file and map it into memory.
	this->fd = fopen(this->path.c_str(), "rb");
	PLOG_IF(FATAL, this->fd == NULL) << "Couldn't open file " << this->path
									 << "for reading";

	int fd = fileno(this->fd);

	this->mappedFile = mmap(NULL, this->size, PROT_READ, MAP_PRIVATE, fd, 0);
	PLOG_IF(FATAL, this->mappedFile == MAP_FAILED) << "Couldn't map file";
}

/**
 * Unmaps the file from memory, and cleans up some temporary buffers that were
 * used while the file was being read.
 */
void BackupFile::finishedReading() {
	if(this->mappedFile != NULL) {
		// get rid of the mapping, then close the file
		munmap(this->mappedFile, this->size);
		fclose(this->fd);

		this->mappedFile = this->fd = NULL;
	}
}

/**
 * Calculates how many data bytes the file has that still need to be read.
 */
size_t BackupFile::bytesRemaining() {
	return (this->size - this->lastByte);
}

/**
 * Returns an offset inside of the memory-mapped region of the file, and then
 * advance the internal read pointer by `size` bytes.
 */
void *BackupFile::getDataOfLength(size_t size) {
	// get the current head of the read pointer
	void * ptr = (void *) (((uint8_t *) this->mappedFile) + this->lastByte);

	// increment read pointer
	this->lastByte += size;

	return ptr;
}
