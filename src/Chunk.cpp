#include "Chunk.hpp"

#include <glog/logging.h>

#include <iostream>
#include <sys/mman.h>

using std::cerr;

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
 * Creates a chunk of the given size.
 */
Chunk::Chunk(std::size_t size) {
	// Store the size of the chunk
	this->backing_store_size = size;
	this->backing_store_bytes_allocated = this->header_area_size;

	// Allocate backing store
	this->allocateBackingStore();
}

/**
 * Allocates the backing store.
 */
void Chunk::allocateBackingStore() {
	if(use_superpages == true) {
		// Allocate the backing store with superpages
		this->backing_store = mmap(NULL, this->backing_store_size,
								   PROT_READ | PROT_WRITE,
								   MAP_ALIGNED_SUPER | MAP_ANON, -1, 0);

		// If this fails, attempt a regular allocation.
		if(this->backing_store == MAP_FAILED) {
			cerr << "Couldn't allocate backing store sized " << this->backing_store_size
				 << " with superpages; errno = " << errno << std::endl;

			use_superpages = false;
			this->allocateBackingStore();
		}
	} else {
		// Attempt a regular allocation.
		this->backing_store = mmap(NULL, this->backing_store_size,
								   PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);

		// If this fails, we're fucked.
		if(this->backing_store == MAP_FAILED) {
			cerr << "Couldn't allocate backing store sized " << this->backing_store_size
				 << " using normal pages; errno = " << errno << std::endl;

			// we really cannot recover from this
			abort();
		}
	}
}

/**
 * Cleans up the chunk. This WILL free its memory backing.
 */
Chunk::~Chunk() {
	int err = 0;

	// Unmamp memory
	err = munmap(this->backing_store, this->backing_store_size);

	if(err != 0) {
		cerr << "Couldn't unmap 0x" << std::hex << this->backing_store
			 << std::dec << " (chunk backing store): errno = " << errno
			 << std::endl;
	}
}
