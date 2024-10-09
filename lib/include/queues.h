#pragma once
#include "Queue.h"
#include "SingleProducerSingleConsumerNonblockingQueue.h"
#include "MultipleProducerMultipleConsumerBlockingQueue.h"

namespace queues {
    // Represents a set of options for creating a fixed-size queue.
    struct QueueOptions {
        bool concurrentReaders = false;
        bool concurrentWriters = false;
        bool supportsBlockingRead = false;
        bool supportsBlockingWrite = false;
    };

}
