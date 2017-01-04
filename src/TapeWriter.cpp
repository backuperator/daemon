#include "TapeWriter.hpp"

#include <chrono>

#include <glog/logging.h>
#include <boost/thread.hpp>

/**
 * Initializes the tape writer.
 */
TapeWriter::TapeWriter() {
	// Initialize the worker thread
	this->workerThread = std::thread(boost::bind(&TapeWriter::_workerEntry, this));
}

/**
 * Instructs the tape writing thread to stop. This will finish writing any data
 * that was started prior to this call, but no new work will be begun.
 */
TapeWriter::~TapeWriter() {
	// Set flag to false, and wait for the thread to finish.
	this->shouldRun = false;
	this->newChunksAvailableSignal.notify_all();

	this->workerThread.join();
}

/**
 * Adds a chunk to the write queue.
 */
void TapeWriter::addChunkToQueue(Chunk *chunk) {
	// Check the number of chunks in the queue; block if too many
	// FIXME: this is horrendously defective
	/*while(this->writeQueue.size() >= MAX_CHUNKS_WAITING) {
		DLOG(INFO) << "More than " << MAX_CHUNKS_WAITING << " chunks waiting "
				   << "to be written to tape; waiting";

		// Wait on the "chunk processor completed" signal.
		std::mutex m;
		std::unique_lock<std::mutex> lk(m);
		this->chunkProcessedSignal.wait(lk);
	}*/

	// Push the chunk onto the queue
    std::lock_guard<std::mutex> lock(this->writeQueueMutex);
	this->writeQueue.push(chunk);

	// notify the worker thread
	this->newChunksAvailableSignal.notify_all();
}

/**
 * Worker thread entry point.
 */
void TapeWriter::_workerEntry() {
	while(this->shouldRun) {
		// Wait for the thread to be woken
		std::mutex m;
	    std::unique_lock<std::mutex> lk(m);
		this->newChunksAvailableSignal.wait(lk);

		// Get the chunk out of the queue…
		this->writeQueueMutex.lock();

		Chunk *chunk = this->writeQueue.front();
		this->writeQueue.pop();

		this->writeQueueMutex.unlock();

		// …then write it.
		_writeChunk(chunk);

		// When we've finished this chunk, notify any waiting threads.
		this->chunkProcessedSignal.notify_all();
	}
}

/**
 * Writes a chunk to tape. This is a blocking operation; if an error occurs
 * during writing, determine whether the tape is at the end (in which case the
 * a new tape is swapped in and the write is retried), or if there was some
 * other unrecoverable I/O error.
 */
void TapeWriter::_writeChunk(Chunk *chunk) {
	LOG(INFO) << "Writing chunk " << chunk->getChunkNumber() << " to tape";

	// For now, just write to a file.
	std::ostringstream nameStr;
	nameStr << chunk->getChunkNumber() << ".chunk";
	std::string name  = nameStr.str();
	const char *nameC = name.c_str();

	FILE *fp = fopen(nameC, "w+b");
	fwrite(chunk->backingStore, chunk->backingStoreActualSize, 1, fp);
	fclose(fp);


	// When done writing, delete the chunk.
	LOG(INFO) << "Finished writing chunk " << chunk->getChunkNumber();
	delete chunk;
}
