#include "BackupJob.hpp"

#include <glog/logging.h>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/iterator_range.hpp>

using namespace boost::filesystem;

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

	// Create the thread pool
	this->directoryScannerPool = new ctpl::thread_pool(DIR_ITERATOR_POOL_SZ);
	LOG(INFO) << "Using " << DIR_ITERATOR_POOL_SZ << " threads for directory iteration";
}

/**
 * Terminates all threads related to the backup job, and performs any cleanup.
 */
BackupJob::~BackupJob() {
	// Kill all the threads
}

/**
 * Starts the backup job.
 */
void BackupJob::start() {
	// Start the directory scan.
	this->beginDirectoryScan();
}

/**
 * Gracefully cancels the backup job before it can be de-allocated. This is a
 * blocking call.
 */
void BackupJob::cancel() {

}


/**
 * Builds the list of files to be backed up, i.e. iterating a directory in a
 * recursive manner.
 */
void BackupJob::beginDirectoryScan() {
	// Submit the initial job to the thread pool
	this->directoryScannerPool->push(boost::bind(&BackupJob::directoryScannerEntry, this));

	// Wait for the thread pool to complete
	this->directoryScannerPool->stop(true);
	LOG(INFO) << "Found " << this->backupFiles.size() << " files/directories";
}

/**
 * Entry point for the directory scanner thread.
 */
void BackupJob::directoryScannerEntry() {
	LOG(INFO) << "Beginning directory scan of " << this->rootPath;

	// Create a file entry for the root directory
	BackupFile *root = new BackupFile(this->rootPath, NULL);
	this->backupFiles.push_back(root);

	// Begin scanning the root directory.
	this->scanDirectory(this->rootPath, root);
}

/**
 * Scans a single directory. This function is called recursively.
 */
void BackupJob::scanDirectory(path inPath, BackupFile *parent) {
	// if it's a directory, recurse through it
    if(is_directory(inPath)) {
        for(auto& entry : boost::make_iterator_range(directory_iterator(inPath), {})) {
			// DLOG(INFO) << "Found " << entry;
			LOG_EVERY_N(INFO, 100) << "Found " << this->backupFiles.size() << " items so far";

			// create a backup file for this entry
			BackupFile *file = new BackupFile(path(entry), parent);
			this->backupFiles.push_back(file);

            // Is what we found a directory?
			if(is_directory(entry)) {
				// If so, submit a job to scan it to the thread pool
				this->directoryScannerPool->push(boost::bind(&BackupJob::scanDirectory, this, path(entry), file));
			}
		}
    }
}
