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
		typedef enum {
			// The entire file was added.
			Success = 0,
			/**
			 * Indicates that part of the file was written, but that the chunk
			 * has no insufficient free space to fit the entire file. A new
			 * chunk should be allocated, and the same file should be added to
			 * this chunk, repeating this process until Success is returned.
			 */
			Partial = 1,
			/**
			 * No part of the file was written, because this chunk has no more
			 * space for the file. This can occurr either because the chunk hit
			 * its maximum size, or because there is no more space for metadata.
			 */
			NoSpace = 2,

			// Some error occurred (most likely I/O failure)
			Error = -1,
		} Add_File_Status;

	public:
		Chunk(std::size_t);
		~Chunk();

		Add_File_Status addFile(BackupFile *);

	protected:

	private:
		std::size_t backingStoreActualSize;
		std::size_t backingStoreMaxSize;

		std::size_t backingStoreBytesUsed;

		void *backingStore;

		// length of the reserved header area, in bytes
		static const std::size_t header_area_size = (1024 * 512);
		// minimum amount of free space to add a file
		static const std::size_t min_free_space = (1024 * 64);

		std::vector<BackupFile *> files;

		// allocates backing store
		void allocateBackingStore();
};

#endif
