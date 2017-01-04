#include "ChunkPostprocessor.hpp"

#include <glog/logging.h>
#include <boost/thread.hpp>

/**
 * Creates the chunk postprocessor, including the worker thread. The worker
 * thread will sleep until it is signaled that a new chunk is available.
 */
ChunkPostprocessor::ChunkPostprocessor(std::mutex *mutex, std::queue<Chunk *> *queue) {
	this->queueMutex = mutex;
	this->queue = queue;

	this->lastChunkIndex = 0;

	// Create worker thread
	this->threadPool = new ctpl::thread_pool(POSTPROCESSOR_THREAD_POOL_SIZE);
	LOG(INFO) << "Using " << POSTPROCESSOR_THREAD_POOL_SIZE
			  << " threads for chunk postprocessing";

  this->threadPool->push(boost::bind(&ChunkPostprocessor::_workerEntry, this));
}

/**
 * Cleans up the postprocessor.
 */
ChunkPostprocessor::~ChunkPostprocessor() {

}

/**
 * Signals the condition variable that the worker thread is sleeping on, telling
 * it that a new chunk is available to process.
 */
void ChunkPostprocessor::newChunkAvailable() {
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
		this->queueMutex->lock();

		Chunk *chunk = this->queue->front();
		this->queue->pop();

		this->queueMutex->unlock();


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
	chunk_header_t *header = (chunk_header_t *) chunk->backingStore;

	header->chunk_index = this->lastChunkIndex++;

	// When we're done, forward it to the tape writer.
}
