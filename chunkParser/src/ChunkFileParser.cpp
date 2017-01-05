#include "ChunkFileParser.hpp"

#include <glog/logging.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <pwd.h>
#include <grp.h>

#include "crc32.h"

/**
 * Initializes the chunk file parser, opening the chunk file at the specified
 * path, then mapping it into memory.
 */
ChunkFileParser::ChunkFileParser(boost::filesystem::path path) {
	// Open a file descriptor
	this->fd = fopen(path.c_str(), "rb");
	PLOG_IF(FATAL, this->fd == NULL) << "Couldn't open file " << path
									 << " for reading";

	// Determine its size
	fseek(this->fd, 0L, SEEK_END);
	this->size = ftell(this->fd);
	rewind(this->fd);

	LOG(INFO) << "File is " << this->size << " bytes";

	// Map it
	int fd = fileno(this->fd);

	this->mappedFile = mmap(NULL, this->size, PROT_READ, MAP_SHARED, fd, 0);
	PLOG_IF(FATAL, this->mappedFile == MAP_FAILED) << "Couldn't map file";

	// Parse the header
	_parseHeader();
}

/**
 * Cleans up the memory mapping of the file.
 */
ChunkFileParser::~ChunkFileParser() {
	if(this->mappedFile != NULL) {
		// get rid of the mapping, then close the file
		munmap(this->mappedFile, this->size);
		fclose(this->fd);

		this->mappedFile = this->fd = NULL;
	}
}

/**
 * Parse the chunk header to ensure it's valid.
 */
void ChunkFileParser::_parseHeader() {
	chunk_header_t *header = (chunk_header_t *) this->mappedFile;

	// Check version
	LOG(INFO) << "Chunk version 0x" << std::hex << header->version << std::dec;

	if(header->version != 0x00010000) {
		LOG(WARNING) << "\tThis tool only supports version 0x00010000.";
	}
}


/**
 * Extracts the file at the given index. Note that all but the file's actual
 * name are disregarded; the directory in which it originally existed is not
 * taken into account.
 */
void ChunkFileParser::extractAtIndex(off_t index) {
	off_t i = 0;

	// Locate this file's entry.
	chunk_header_t *header = (chunk_header_t *) this->mappedFile;
	uint8_t *fileEntryStart = (uint8_t *) &header->entry;

	for(i = 0; i < index; i++) {
		// Print info about it
		chunk_file_entry_t *fileEntry = (chunk_file_entry_t *) fileEntryStart;
		fileEntryStart += sizeof(chunk_file_entry_t) + fileEntry->nameLenBytes;
	}

	// Print file info
	chunk_file_entry_t *fileEntry = (chunk_file_entry_t *) fileEntryStart;
	_printFileInfo(i, fileEntry);

	if(fileEntry->blobLenBytes != fileEntry->size) {
		LOG(WARNING) << "NOTE: The file's entire data is not contained in this "
					 << "chunk. To get the entire file, re-run this utility "
					 << "with any subsequent chunks.";
	}

	// Seek to its data and calculate the CRC
	void *dataOffset = ((uint8_t *) this->mappedFile) + fileEntry->blobStartOff;

 	uint32_t crc = crc32c(0, dataOffset, fileEntry->blobLenBytes);

	if(crc != fileEntry->checksum) {
		LOG(ERROR) << "CRC MISMATCH DETECTED; THIS FILE MAY HAVE BEEN CORRUPTED!";
		LOG(ERROR) << "Calculated " << std::hex << crc << ", expected "
				   << fileEntry->checksum << std::dec
				   << "; proceeding with extraction anyways.";
	}

	// Gather the pathname
	boost::filesystem::path path(fileEntry->name);
	const char *name = path.filename().c_str();

	LOG(INFO) << "Attempting to open file for writing at " << name;

	// Open output file and seek to the correct position
	int outFp = open(name, O_RDWR | O_CREAT | O_EXLOCK);
	PCHECK(outFp != -1) << "Could not open file for writing";

	fchmod(outFp, fileEntry->mode);
	lseek(outFp, fileEntry->blobFileOffset, SEEK_SET);

	// Write to it, then close the file
	write(outFp, dataOffset, fileEntry->blobLenBytes);
	close(outFp);

	// Done!
	LOG(INFO) << "Wrote " << fileEntry->blobLenBytes << " bytes.";
}


/**
 * Lists all files found in this chunk.
 */
void ChunkFileParser::listFiles() {
	chunk_header_t *header = (chunk_header_t *) this->mappedFile;

	uint8_t *fileEntryStart = (uint8_t *) &header->entry;

	for(off_t i = 0; i < header->numFileEntries; i++) {
		// Print info about it
		chunk_file_entry_t *fileEntry = (chunk_file_entry_t *) fileEntryStart;
		_printFileInfo(i, fileEntry);

		// Increment by the size
		fileEntryStart += sizeof(chunk_file_entry_t) + fileEntry->nameLenBytes;
	}
}


/**
 * Prints info about a file, given its file entry structure.
 */
void ChunkFileParser::_printFileInfo(size_t i, chunk_file_entry_t *fileEntry) {
	LOG(INFO) << "File " << i;
	LOG(INFO) << "\tName: " << fileEntry->name;
	LOG(INFO) << "\tMode: " << std::oct << fileEntry->mode << std::dec
			  << "; owner " << _nameForUid(fileEntry->owner) << "("
			  << fileEntry->owner << ")"
			  << " group " << _nameForGid(fileEntry->group) << "("
			  << fileEntry->group << ")";
	LOG(INFO) << "\tSize: " << fileEntry->size << " (chunk offset = "
			  << fileEntry->blobStartOff << ", length = "
			  << fileEntry->blobLenBytes << ", original file offset = "
			  << fileEntry->blobFileOffset << ")";
}

/**
 * Returns a string containing the name of a given user, or a placeholder if the
 * system cannot locate the given user.
 */
const char *ChunkFileParser::_nameForUid(int uid) {
	struct passwd *user = getpwuid(uid);

	if(user != NULL) {
		return user->pw_name;
	}

	return "<<< User not found >>>";
}

/**
 * Returns a string containing the name of a given group, or a placeholder if
 * the system cannot locate the given group.
 */
const char *ChunkFileParser::_nameForGid(int gid) {
	struct group *group = getgrgid(gid);

	if(group != NULL) {
		return group->gr_name;
	}

	return "<<< Group not found >>>";
}
