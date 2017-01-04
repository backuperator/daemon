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

	// TODO: Make this run on its own thread pls
	this->chunkCreatorEntry();
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
	this->backupFiles.reserve(10000);

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
			DLOG_EVERY_N(INFO, 100) << "Found " << this->backupFiles.size() << " items so far";

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

/**
 * Pulls files out of the queue one by one, creating new chunks for them. When
 * a chunk is completed, it's pushed onto the chunk queue.
 */
void BackupJob::chunkCreatorEntry() {
	LOG(INFO) << "Beginning chunk creation (chunk size = " << CHUNK_MAX_SIZE << ")";

	Chunk *chunk = NULL;
	int status;

	for(auto it = this->backupFiles.begin(); it != this->backupFiles.end(); it++) {
		do {
			// If there isn't a chunk, create one
			if(chunk == NULL) {
				chunk = new Chunk(CHUNK_MAX_SIZE);
				DLOG(INFO) << "Crated new chunk";
			}

			// Attempt to add it
			status = this->chunkAddFile(*it, chunk);

			// Check for errors
			if(status == -1) {
				LOG(FATAL) << "Error adding file " << (*it)->getPath();
				break;
			}

			// If this chunk is done, get rid of it.
			if(status == 1) {
				chunk->finalize();
				this->chunkQueue.push(chunk);
				DLOG(INFO) << "Finished chunk: " << chunk->getUsedSpace()
						   << " bytes used (out of " << CHUNK_MAX_SIZE << ")";

				// NULL will create a new chunk on the next iteration
				chunk = NULL;
			}

			// Status is 0, so the file was added. Get the next file.
		} while(status != 0);
	}

	// Push the last chunk into the array.
	chunk->finalize();
	this->chunkQueue.push(chunk);
	DLOG(INFO) << "Finished chunk: " << chunk->getUsedSpace()
			   << " bytes used (out of " << CHUNK_MAX_SIZE << ")";

	LOG(INFO) << "Finished chunk creation.";
}

/**
 * Adds the given file to the chunk.
 *
 * If 0 is returned, we're ready for the next file. If -1 is returned, an error
 * occurred. If 1 is returned, call this function again with the same file, but a
 * new chunk.
 */
int BackupJob::chunkAddFile(BackupFile *file, Chunk *chunk) {
	Chunk::Add_File_Status status;

	// Attempt to add this file to the chunk
	status = chunk->addFile(file);

	// DLOG(INFO) << "Added " << file->getPath() << ": " << status;

	switch(status) {
		/*
		 * Only part of the chunk was able to be fit into this chunk. Mark this
		 * chunk as done, generate a new one, and add the file again.
		 */
		case Chunk::Add_File_Status::Partial:
			return 1;

		/*
		 * There is insufficient space remaining in this chunk to add this file
		 * to the chunk. Mark the chunk as done, generate a new one, and retry.
		 */
		case Chunk::Add_File_Status::NoSpace:
			return 1;

		/*
		 * An undefined error occurred while attempting to add the file. This is
		 * most likely the result of some sort of I/O error or permissions mis-
		 * match, and is considered fatal for the backup.
		 */
		case Chunk::Add_File_Status::Error:
			return -1;

		// The file was added successfully, so nothing more has to be done.
		case Chunk::Add_File_Status::Success:
			return 0;
	}
}
