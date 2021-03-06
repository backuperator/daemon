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
	friend class ChunkPostprocessor;
	friend class TapeWriter;

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

		size_t getUsedSpace() { return this->backingStoreBytesUsed; }

		void finalize();
		void stopWriting();

		void setChunkNumber(uint64_t idx) {
			((chunk_header_t *) this->backingStore)->chunkIndex = idx;
		}
		uint64_t getChunkNumber() {
			return ((chunk_header_t *) this->backingStore)->chunkIndex;
		}

		void setJobUuid(boost::uuids::uuid);

	protected:

	private:
		std::size_t backingStoreActualSize = 0;
		std::size_t backingStoreMaxSize = 0;

		std::size_t backingStoreBytesUsed = 0;

		void *backingStore;

		// length of the reserved header area, in bytes
		static const std::size_t kHeaderAreaReservedSpace = (1024 * 512);
		// minimum amount of free space to add a file
		static const std::size_t kMinFreeSpace = (1024 * 1024);

		std::vector<BackupFile *> files;


		Add_File_Status _addFilePartial(BackupFile *file);

		// allocates backing store
		void _allocateBackingStore();
};

#endif
