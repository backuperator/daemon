#include "BackupFile.hpp"

#include <iostream>
#include <sys/stat.h>

#include <boost/uuid/uuid_generators.hpp>

using std::cerr;
using boost::filesystem::path;

/**
 * Creates a file object from the file with the given path. This does not load
 * any data from disk yet - metadata is only loaded when requested.
 */
BackupFile::BackupFile(boost::filesystem::path path) {
	this->path = path;

	// Create an UUID for the file
	boost::uuids::basic_random_generator<boost::mt19937> gen;
	this->uuid = gen();
}

/**
 * Cleans up any stored information.
 */
BackupFile::~BackupFile() {

}

/**
 * Fetches metadata in a blocking fashion.
 */
int BackupFile::fetchMetadata() {
	int err = 0;

	// Check if metadata for this file was already fetched.
	if(this->hasMetadata == true) {
		return 0;
	}

	// Check that the path exists.
	if(exists(this->path) == false) {
		return -1;
	}

	// Run stat(2) to get info about the file.
	const char *filepath = this->path.c_str();

	struct stat info;
	memset(&info, 0, sizeof(info));

	err = stat(filepath, &info);

	if(err != 0) {
		cerr << "Couldn't get info on file " << filepath << ": " << errno << std::endl;
		return -1;
	}

	// Extract relevant info
	this->last_modified = info.st_mtime;

	this->mode = info.st_mode;
	this->owner = info.st_uid;
	this->group = info.st_gid;

	this->size = info.st_size;

	// assume success
	this->hasMetadata = true;

	return 0;
}
