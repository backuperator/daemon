/**
 * Various stuctures and defines for chunks.
 *
 * NOTE: The version of the chunk header defines what versions of the embedded
 * structures are used.
 */
#ifndef CHUNK_H
#define CHUNK_H

/**
 * File entry; specifies information about a single file in a chunk.
 */
typedef struct __attribute__((packed)) {
	// Unique identifier for this file
	uint8_t[16] file_uuid;

	// Timestamp for last modification date.
	uint64_t time_modified;

	// Owner and group
	uint32_t owner, group;
	// File mode
	uint32_t mode;

	// CRC32 (using the Castagnoli polynomial) over the data
	uint32_t checksum;

	// Offset within the chunk to the file's data.
	uint64_t blob_start;
	// Length of the blob, in bytes.
	uint64_t num_blob_bytes;
	// Byte offset in the original file where this blob goes.
	uint64_t blob_file_start;

	// How many parts this file has been split into; if it is not 1, the file is
	// split across multiple chunks.
	uint32_t num_file_parts;
	// Index of the current part of the file.
	uint32_t file_part;

	// Length of the filename (in bytes)
	uint32_t num_name_bytes;
	// Filename (UTF-8 encoded)
	char[] name;
} chunk_file_entry_t;

/**
 * Chunk header definition
 */
typedef struct  __attribute__((packed)) {
	// Chunk header version; currently 0x00010000.
	uint32_t version;
	// Identifier of the backup job; can be cross-referenced with database.
    uint8_t[16] backup_uuid;
	// Index of this chunk in the backup; first chunk is zero.
    uint64_t chunk_index;
	// Size of this chunk, in bytes.
	uint64_t chunk_length;
	// Identifier of the tape that contains this chunk
	char[8] tape_label;

	// Encryption data
	struct {
		// Specifies the encryption methodl 0 if cleartext.
		uint8_t method;

		// IV used to encrypt this block
		uint8_t[32] iv;
	} encryption;

	// Reserved for future expansion
	uint8_t[0x4000] reserved;

	// Number of files contained in this chunk.
	uint32_t num_file_entries;
	// An array of file entries, containing num_file_entries entries.
	chunk_file_entry_t[] entry;
} chunk_header_t;


#endif
