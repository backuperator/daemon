#include "Chunk.hpp"

#include <glog/logging.h>

#include <sys/mman.h>

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
void Chunk::allocateBackingStore() {
	if(use_superpages == true) {
		// Allocate the backing store with superpages
		this->backingStore = mmap(NULL, this->backingStoreActualSize,
								   PROT_READ | PROT_WRITE,
								   MAP_ALIGNED_SUPER | MAP_ANON, -1, 0);

		// If this fails, attempt a regular allocation.
		if(this->backingStore == MAP_FAILED) {
			PLOG(ERROR) << "Couldn't allocate backing store size "
						<< this->backingStoreActualSize << " using superpages";

			use_superpages = false;
			this->allocateBackingStore();
		}
	} else {
		// Attempt a regular allocation.
		this->backingStore = mmap(NULL, this->backingStoreActualSize,
								   PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);

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

		// TODO: beginReading, if needed?
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

		// Add to storage
		this->files.push_back(file);
		this->backingStoreBytesUsed += file->size + file->fileEntrySize;

		/*bytesFree = this->backingStoreMaxSize - this->backingStoreBytesUsed;
		DLOG(INFO) << "\tUsed " << file->size << " bytes for file, "
								<< file->fileEntrySize << " bytes for file record";*/

		return Add_File_Status::Success;
	}

	// The file needs to be split.

	// TODO: Split file

	return Add_File_Status::Partial;
}
