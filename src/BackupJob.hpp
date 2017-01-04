/**
 * A single backup job; this class orchestrates the recursion through the fs,
 * creating chunks, post-processing them, as well as finally writing them all
 * out to tape.
 */
#ifndef BACKUPJOB_H
#define BACKUPJOB_H

#include <string>
#include <ctime>
#include <queue>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>

#include "Chunk.hpp"
#include "BackupFile.hpp"

class BackupJob {
    public:
        BackupJob(std::string);
        ~BackupJob();

    protected:
        std::string root;
    	boost::filesystem::path rootPath;

    	boost::uuids::uuid uuid;

        std::vector<BackupFile> backupFiles;
        std::queue<Chunk> chunkQueue;

    private:

};

#endif
