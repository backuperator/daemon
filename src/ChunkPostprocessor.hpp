/**
 * Performs various post-processing tasks on chunks, like filling in the last
 * few fields in the chunk header, performing checksumming, and (optionally)
 * encrypting the data.
 */
#ifndef CHUNKPOSTPROCESSOR_H
#define CHUNKPOSTPROCESSOR_H

/**
 * Defines how many threads should be allocated for post-processing chunks prior
 * to writing them to the tape.
 */
#define POSTPROCESSOR_THREAD_POOL_SIZE	4

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include <CTPL/ctpl.h>

#include "Chunk.hpp"

class ChunkPostprocessor {
	public:
		ChunkPostprocessor(std::mutex *, std::queue<Chunk *>*);
		~ChunkPostprocessor();

		void newChunkAvailable();

	protected:

	private:
		ctpl::thread_pool *threadPool;

		std::mutex *queueMutex;
		std::queue<Chunk *> *queue;

		std::condition_variable chunkSignal;

		bool shouldRun = true;

		std::atomic<uint64_t> lastChunkIndex;


		void _workerEntry();
		void _processChunk(Chunk *);
};

#endif
