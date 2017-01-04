/**
 * A single backup job; this class orchestrates the recursion through the fs,
 * creating chunks, post-processing them, as well as finally writing them all
 * out to tape.
 */
#ifndef BACKUPJOB_H
#define BACKUPJOB_H

/**
 * Defines how many threads should be allocated for iterating the directory to
 * be backed up.
 */
#define DIR_ITERATOR_POOL_SZ 4

/**
 * Maximum chunk size, in bytes.
 */
#define CHUNK_MAX_SIZE ((size_t) (1024LL * 1024 * 1024 * 2))

#include <string>
#include <ctime>
#include <queue>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/thread.hpp>
#include <CTPL/ctpl.h>

#include "Chunk.hpp"
#include "BackupFile.hpp"


class BackupJob {
    public:
        BackupJob(std::string);
        ~BackupJob();

		void start();
		void cancel();

    protected:
        std::string root;
    	boost::filesystem::path rootPath;

    	boost::uuids::uuid uuid;

        std::vector<BackupFile *> backupFiles;
        std::queue<Chunk *> chunkQueue;

		void beginDirectoryScan();
		void scanDirectory(boost::filesystem::path path, BackupFile *);

    private:
		ctpl::thread_pool *directoryScannerPool;

		void directoryScannerEntry();

		void chunkCreatorEntry();
		int chunkAddFile(BackupFile *file, Chunk *chunk);
};

#endif
