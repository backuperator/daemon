#include "Chunk.hpp"

#include <glog/logging.h>

#include <sys/mman.h>
#include <unistd.h>

#include "crc32.h"

/**
 * When this is set, we attempt to use superpages to allocate the backing store,
 * as this can reduce the amount of page walks and improve memory access
 * performance somewhat. This is enabled by default, but when a superpage-based
 * memory allocation fails, it is assumed memory has gotten too fragmented to
 * service such allocations until a reboot, and we do not ask for superpages
 * on any subsequent allocations.
 */
#ifdef MAP_ALIGNED_SUPER
static bool use_superpages = true;
#else
// superpages aren't supported on this system
#define MAP_ALIGNED_SUPER 0
static bool use_superpages = false;

#warning Disabling superpage support due to missing MAP_ALIGNED_SUPER.
#endif

/**
 * Creates a chunk, which may grow to be NO LARGER than the given size.
 */
Chunk::Chunk(size_t size) {
	// Store the size of the chunk
	this->backingStoreMaxSize = size;
	this->backingStoreBytesUsed = this->backingStoreActualSize = 0;
}

/**
 * Allocates the backing store.
 */
void Chunk::_allocateBackingStore() {
	if(use_superpages == true) {
		// Allocate the backing store with superpages
		this->backingStore = mmap(NULL, this->backingStoreActualSize,
								   PROT_READ | PROT_WRITE,
								   MAP_ALIGNED_SUPER | MAP_ANON | MAP_PRIVATE,
								   -1, 0);

		// If this fails, attempt a regular allocation.
		if(this->backingStore == MAP_FAILED) {
			PLOG(ERROR) << "Couldn't allocate backing store size "
						<< this->backingStoreActualSize << " using superpages";

			use_superpages = false;
			_allocateBackingStore();
		}
	} else {
		// Attempt a regular allocation.
		this->backingStore = mmap(NULL, this->backingStoreActualSize,
								   PROT_READ | PROT_WRITE,
								   MAP_ANON | MAP_PRIVATE, -1, 0);

		// If this fails, we're fucked.
		if(this->backingStore == MAP_FAILED) {
			PLOG(FATAL) << "Couldn't allocate backing store size "
						<< this->backingStoreActualSize << " using normal pages";
		}
	}
}

/**
 * Cleans up the chunk. This WILL free its memory backing.
 */
Chunk::~Chunk() {
	int err = 0;

	// Unmamp memory
	err = munmap(this->backingStore, this->backingStoreActualSize);

	if(err != 0) {
		PLOG(ERROR) << "Couldn't unmap 0x" << std::hex << this->backingStore
					<< std::dec << " (chunk backing store)";
	}
}

/**
 * Adds a file. This will cause the file to be mapped into memory, metadata read
 * and several buffers prepared.
 */
Chunk::Add_File_Status Chunk::addFile(BackupFile *file) {
	// Check the difference between allocated and max size
	off_t difference = (this->backingStoreMaxSize - this->backingStoreBytesUsed);
	if(difference <= kMinFreeSpace) {
		return Add_File_Status::NoSpace;
	}

	// Prepare file
	CHECK(file->fetchMetadata() == 0) << "Could not read file metadata";

	size_t bytesFree = this->backingStoreMaxSize - this->backingStoreBytesUsed
												 - kHeaderAreaReservedSpace;

	/*
	 * If the file we've been handed is a directory, add it to the chunk without any
	 * checking. We allocate ~1MB overhead for the header, and this just goes into
	 * that space, as directories have no real data.
	 */
	if(file->isDirectory) {
		this->files.push_back(file);

		file->wasWrittenToChunk = file->fullyWrittenToChunk = true;
		return Add_File_Status::Success;
	}

	/**
	 * If this chunk can accomodate less than 50% of the file, given that the
	 * file is no larger than the chunk, sans chunk overhead, cut the chunk
	 * short and force the file into the next chunk.
	 */
	if(file->size < (this->backingStoreMaxSize - kHeaderAreaReservedSpace)) {
		// is 50% of the file's size able to fit in this chunk?
		size_t halfSize = file->size / 2;

		if(halfSize > bytesFree) {
			return Add_File_Status::NoSpace;
		}
	}
	/**
	 * If this chunk has less than half its space free, given that the file is
	 * larger than the maximum chunk size, cut the chunk short and force the
	 * file into the next chunk.
	 */
	else {
		if(bytesFree < (this->backingStoreMaxSize / 2)) {
			return Add_File_Status::NoSpace;
		}
	}

	// The file can (at least partially) fit in this chunk. Make it so.

	// Check if we have enough space for the entire file in the chunk.
	if(file->size < bytesFree) {
		// Read the file's metadata, and create the struct in memory
		file->prepareChunkMetadata();

		file->rangeInChunk.fileOffset = 0;
		file->rangeInChunk.length = file->size;

		// Add to storage
		this->files.push_back(file);
		this->backingStoreBytesUsed += file->size + file->fileEntrySize;

		/*bytesFree = this->backingStoreMaxSize - this->backingStoreBytesUsed;
		DLOG(INFO) << "\tUsed " << file->size << " bytes for file, "
								<< file->fileEntrySize << " bytes for file record";*/

		// mark the file as fully written
		file->wasWrittenToChunk = file->fullyWrittenToChunk = true;

		return Add_File_Status::Success;
	}


	// The file needs to be split.
	return _addFilePartial(file);
}

/**
 * Attempt to fit a file, splitting it as needed.
 */
Chunk::Add_File_Status Chunk::_addFilePartial(BackupFile *file) {
	const size_t pageSz = sysconf(_SC_PAGESIZE);

	// If this file wasn't written yet, we start from offset 0.
	if(file->wasWrittenToChunk == false) {
		file->rangeInChunk.fileOffset = 0;

		file->wasWrittenToChunk = true;
		file->fullyWrittenToChunk = false;
	}

	// Figure out (approximately) how much of the file can be fit in the chunk.
	size_t bytesFree = this->backingStoreMaxSize - this->backingStoreBytesUsed
												 - kHeaderAreaReservedSpace;
	size_t bytesLeft = file->bytesRemaining();

	// The entire rest of the file fits into this chunk.
	if(bytesFree > bytesLeft) {
		// Update the blob struct
		file->rangeInChunk.fileOffset += file->rangeInChunk.length;
		file->rangeInChunk.length = 0;

		file->rangeInChunk.length = file->bytesRemaining();

		file->fullyWrittenToChunk = true;

		// Store it
		this->files.push_back(file);
		return Add_File_Status::Success;
	}

	/*
	 * Otherwise, figure out approximately how much of this file can fit in the rest
	 * of the chunk. Once we arrive at a result, we round DOWN to the nearest
	 * multiple of the system's page size.
	 */
	size_t bytesInThisChunk = bytesFree;

	if((bytesInThisChunk % pageSz) != 0) {
		bytesInThisChunk -= (bytesInThisChunk % pageSz);
	}

	file->rangeInChunk.fileOffset += file->rangeInChunk.length;
	file->rangeInChunk.length = bytesInThisChunk;

	// This file can be partially added.
	return Add_File_Status::Partial;
}


/**
 * Actually creates the raw chunk data in memory for all files.
 */
void Chunk::finalize() {
	const size_t pageSz = sysconf(_SC_PAGESIZE);

	// Calculate how many bytes we need for headers.
	size_t headerSz = sizeof(chunk_header_t);
	size_t dataSz = 0;

	for(auto it = this->files.begin(); it != this->files.end(); it++) {
		headerSz += (*it)->fileEntrySize;

		// Calculate how much space in the blob area the file needs
		size_t blobSpaceUsed = (*it)->rangeInChunk.length;

		if((blobSpaceUsed % pageSz) != 0) {
			blobSpaceUsed += pageSz - (blobSpaceUsed % pageSz);
		}

		dataSz += blobSpaceUsed;
	}

	if((headerSz % pageSz) != 0) {
		headerSz += pageSz - (headerSz % pageSz);
	}
	DLOG(INFO) << "Need " << headerSz << " bytes for chunk headers";

	off_t dataOffset = headerSz;


	// Calculate how much space we need to allocate in RAM
	size_t bufferSize = headerSz + dataSz;

	if((bufferSize % pageSz) != 0) {
		bufferSize += pageSz - (bufferSize % pageSz);
	}

	this->backingStoreActualSize = bufferSize;
	_allocateBackingStore();

	DLOG(INFO) << "Allocated " << this->backingStoreActualSize << " bytes";


	// Fill chunk header
	chunk_header_t *header = (chunk_header_t *) this->backingStore;
	memset(header, 0, sizeof(chunk_header_t));

	header->version = 0x00010000;
	header->num_file_entries = this->files.size();


	// Copy all the file headers, as well as file data itself
	uint8_t *fileEntries = (uint8_t *) &header->entry;
	for(auto it = this->files.begin(); it != this->files.end(); it++) {
		BackupFile *file = *it;
		chunk_file_entry_t *entry = (chunk_file_entry_t *) fileEntries;

		// DLOG(INFO) << "Placing file " << file->path;

		// Copy the entry into the header and increment the write ptr
		memcpy(entry, file->fileEntry, file->fileEntrySize);
		fileEntries += file->fileEntrySize;


		// Perform additional steps if the file has data
		if(file->isDirectory == false) {
			file->beginReading();

			// Determine the location of the file
			file->rangeInChunk.blobOffsetInChunk = dataOffset;
			dataOffset += file->rangeInChunk.length;

			/*
			 * Round up the location where the next file is placed to be a multiple
			 * of the system's page size. This makes restoring the file easier,
			 * since the chunk can be read to disk, and only the part of the file
			 * we need will be mapped into memory.
			 */
			if((dataOffset % pageSz) != 0) {
				dataOffset += pageSz - (dataOffset % pageSz);
			}

			// Populate the location of the blob
			entry->blobFileOffset = file->rangeInChunk.fileOffset;
			entry->blobLenBytes = file->rangeInChunk.length;
			entry->blobStartOff = file->rangeInChunk.blobOffsetInChunk;

			/*DLOG(INFO) << "\tWriting " << entry->blobLenBytes << " to "
					   << entry->blobStartOff;*/


			// Copy the data
			void *dataDst = ((uint8_t *) this->backingStore) + entry->blobStartOff;
			file->getDataOfLength(entry->blobLenBytes, entry->blobFileOffset, dataDst);

			// Calculate CRC-32
			entry->checksum = crc32c(0, dataDst, entry->blobLenBytes);

			// We don't need the file's data anymore.
			file->finishedReading();
		}
	}
}
