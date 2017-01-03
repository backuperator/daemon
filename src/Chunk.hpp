/**
 * The chunk is the container for various files' data on tape. It also holds
 * metadata for each file, and some info about the job as a whole.
 */
#ifndef CHUNK_H
#define CHUNK_H

#include <vector>
#include <cstdint>

#include "BackupFile.hpp"

class Chunk {
	public:
		Chunk(std::size_t);
		~Chunk();

	protected:

	private:
		std::size_t backing_store_size;
		std::size_t backing_store_bytes_allocated;
		void *backing_store;

		// length of the reserved header area, in bytes
		static const std::size_t header_area_size = (1024 * 512);

		std::vector<BackupFile> files;


		void allocateBackingStore();
};

#endif
