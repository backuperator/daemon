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
Chunk::Chunk(std::size_t size) {
	// Store the size of the chunk
	this->backingStoreMaxSize = size;
	this->backingStoreBytesUsed = this->header_area_size;
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
	// Prepare file
	file->beginReading();
}
