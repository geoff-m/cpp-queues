
add_library(queues INTERFACE include/queues.h
        include/Queue.h
        include/SingleProducerSingleConsumerNonblockingQueue.h
        include/MultipleProducerMultipleConsumerBlockingQueue.h)

#target_include_directories(queues PRIVATE include)