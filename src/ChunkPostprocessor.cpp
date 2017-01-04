#include "ChunkPostprocessor.hpp"

#include <glog/logging.h>
#include <boost/thread.hpp>

/**
 * Creates the chunk postprocessor, including the worker thread. The worker
 * thread will sleep until it is signaled that a new chunk is available.
 */
ChunkPostprocessor::ChunkPostprocessor(boost::uuids::uuid uuid) {
	this->lastChunkIndex = 0;

	this->backupJobUuid = uuid;

	// Create worker thread
	this->threadPool = new ctpl::thread_pool(POSTPROCESSOR_THREAD_POOL_SIZE);
	LOG(INFO) << "Using " << POSTPROCESSOR_THREAD_POOL_SIZE
			  << " threads for chunk postprocessing";

	this->threadPool->push(boost::bind(&ChunkPostprocessor::_workerEntry, this));

	// Set up tape writer
	this->writer = new TapeWriter();
}

/**
 * Cleans up the postprocessor.
 */
ChunkPostprocessor::~ChunkPostprocessor() {
	// Empty the chunk queue
	this->queueMutex.lock();

	while(!this->queue.empty()) {
		delete this->queue.front();
		this->queue.pop();
	}

	this->queueMutex.unlock();

	// Stop the main worker thread
	this->shouldRun = false;
	this->chunkSignal.notify_all();

	// Wait for all other worker processes to stop
	this->threadPool->stop(true);
	delete this->threadPool;

	// Delete the writer
	delete this->writer;
}

/**
 * Signals the condition variable that the worker thread is sleeping on, telling
 * it that a new chunk is available to process.
 */
void ChunkPostprocessor::newChunkAvailable(Chunk *chunk) {
	// Push chunk onto queue
    std::lock_guard<std::mutex> lock(this->queueMutex);
	this->queue.push(chunk);

	// Notify worker thread
	this->chunkSignal.notify_all();
}

/**
 * Worker thread entry point
 */
void ChunkPostprocessor::_workerEntry() {
	while(this->shouldRun) {
		// Wait for the thread to be woken
		std::mutex m;
	    std::unique_lock<std::mutex> lk(m);
		this->chunkSignal.wait(lk);


		// Fetch a chunk from the head of the queue
		this->queueMutex.lock();

		Chunk *chunk = this->queue.front();
		this->queue.pop();

		this->queueMutex.unlock();


		// Do shit to this chunk
		this->threadPool->push(boost::bind(&ChunkPostprocessor::_processChunk,
										   this, chunk));
	}
}

/**
 * Post-processes the given chunk.
 */
void ChunkPostprocessor::_processChunk(Chunk *chunk) {
	DLOG(INFO) << "Got chunk to post-process";

	// Set the chunk index, write backup UUID, then encrypt if needed
	chunk->setChunkNumber(this->lastChunkIndex++);
	chunk->setJobUuid(this->backupJobUuid);

	// Disallow any further writes to the chunk.
	chunk->stopWriting();

	// When we're done, forward it to the tape writer.
	DLOG(INFO) << "Finished post-processing chunk " << chunk->getChunkNumber();
	this->writer->addChunkToQueue(chunk);
}
