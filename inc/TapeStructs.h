/**
 * Various stuctures and defines for chunks.
 *
 * NOTE: The version of the chunk header defines what versions of the embedded
 * structures are used.
 */
#ifndef TAPESTRUCTS_H
#define TAPESTRUCTS_H

#include <cstdint>

#define CHUNK_ENCRYPTION_NONE	0x4E4F4E4520202020LL
#define CHUNK_ENCRYPTION_AES128	0x4145532D31323820LL
#define CHUNK_ENCRYPTION_AES256	0x4145532D32353620LL

/**
 * File types to back up
 */
 typedef enum {
	 kTypeFile			= 0x0001,
	 kTypeDirectory		= 0x1000,
 } chunk_file_type_t;

/**
 * File entry; specifies information about a single file in a chunk.
 */
typedef struct __attribute__((packed)) {
	// Unique identifier for this file
	uint8_t fileUuid[16];

	// What type of file it is
	chunk_file_type_t type;

	// Timestamp for last modification date.
	uint64_t timeModified;
	// Full size of the file
	uint64_t size;

	// Owner and group
	uint32_t owner, group;
	// File mode
	uint32_t mode;

	// CRC32 (using the Castagnoli polynomial) over the data in this blob
	uint32_t checksum;

	// Offset within the chunk to the file's data.
	uint64_t blobStartOff;
	// Length of the blob, in bytes.
	uint64_t blobLenBytes;
	// Byte offset in the original file where this blob goes.
	uint64_t blobFileOffset;

	// Length of the filename (in bytes)
	uint32_t nameLenBytes;
	// Filename (UTF-8 encoded)
	char name[];
} chunk_file_entry_t;

/**
 * Chunk header definition
 */
typedef struct  __attribute__((packed)) {
	// Chunk header version; currently 0x00010000.
	uint32_t version;
	// Identifier of the backup job; can be cross-referenced with database.
    uint8_t jobUuid[16];
	// Index of this chunk in the backup; first chunk is zero.
    uint64_t chunkIndex;
	// Size of this chunk, in bytes.
	uint64_t chunkLenBytes;

	// Encryption data
	struct {
		// Specifies the encryption methodl 0 if cleartext.
		uint64_t method;

		// IV used to encrypt this block
		uint8_t iv[32];
	} encryption;

	// Reserved for future expansion
	uint8_t reserved[0x4000];

	// Number of files contained in this chunk.
	uint32_t numFileEntries;
	// An array of file entries, containing num_file_entries entries.
	chunk_file_entry_t entry[];
} chunk_header_t;


#endif
