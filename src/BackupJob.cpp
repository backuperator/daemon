#include "BackupJob.hpp"

#include <boost/uuid/uuid_generators.hpp>

/**
 * Creates a backup job, backing up the entire directory tree underneath the
 * specified root.
 */
BackupJob::BackupJob(std::string root) {
    this->root = root;
	this->rootPath = boost::filesystem::path(this->root);

    // Generate an UUID for the job
	boost::uuids::basic_random_generator<boost::mt19937> gen;
	this->uuid = gen();
}

/**
 * Terminates all threads related to the backup job, and performs any cleanup.
 */
BackupJob::~BackupJob() {

}
