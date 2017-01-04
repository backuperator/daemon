/**
 * Writes chunks directly out to tape as they come in. Autoloader interfacing is
 * also done in this class - this mostly extends to swapping tapes when they
 * are full, however.
 */
#ifndef TAPEWRITER_H
#define TAPEWRITER_H

/**
 * Defines the maximum number of chunks that may be waiting in the queue at a
 * time, before further calls to add chunks will block.
 */
#define MAX_CHUNKS_WAITING		2

#include <queue>
#include <mutex>
#include <thread>

#include "Chunk.hpp"

class TapeWriter {
	public:
		TapeWriter();
		~TapeWriter();

		void addChunkToQueue(Chunk *);

	private:
		std::queue<Chunk *> writeQueue;
		std::mutex writeQueueMutex;

		std::thread workerThread;

		std::condition_variable chunkProcessedSignal;
		std::condition_variable newChunksAvailableSignal;
		bool shouldRun = true;


		void _workerEntry();
		void _writeChunk(Chunk *chunk);
};

#endif
