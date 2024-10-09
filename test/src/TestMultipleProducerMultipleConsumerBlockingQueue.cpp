#include "TestUtilities.h"
#include "queues.h"
#include <type_traits>
#include <thread>
#include <sstream>
#include <iostream>

TEST(MultipleProducerMultipleConsumerBlockingQueue, WritingToFullQueueBlocks) {
    constexpr int CAPACITY = 5;
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(CAPACITY);
    enqueueCopies(queue, 123, CAPACITY);

    constexpr auto TIMEOUT_MS = 1000;
    Stopwatch sw;
    sw.start();
    const bool writeSuccess = queue.tryEnqueue(456, TIMEOUT_MS);
    sw.stop();

    // Assert that the operation failed.
    ASSERT_FALSE(writeSuccess);

    // Assert that the operation took at least as long as the timeout.
    ASSERT_TRUE(sw.getElapsedMilliseconds() >= TIMEOUT_MS);
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, ReadingFromEmptyQueueBlocks) {
    constexpr int CAPACITY = 5;
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(CAPACITY);

    constexpr auto TIMEOUT_MS = 1000;
    assertReadingBlocks(queue, TIMEOUT_MS);
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, NonblockingReadFailsWhenEmpty) {
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(10);
    int value;
    const bool readOk = queue.tryDequeue(value);
    ASSERT_FALSE(readOk);
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, NonblockingWriteFailsWhenFull) {
    constexpr int CAPACITY = 5;
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(CAPACITY);
    enqueueCopies(queue, 123, CAPACITY);
    const bool writeOk = queue.tryEnqueue(456);
    ASSERT_FALSE(writeOk);
}


TEST(MultipleProducerMultipleConsumerBlockingQueue, ReaderUnblockedByNonblockingWrite) {
    constexpr int CAPACITY = 5;
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(CAPACITY);
    constexpr auto VALUE = 123;
    constexpr auto READ_TIMEOUT_MS = 2000;
    std::thread reader([&]() {
        int value;
        const bool readSuccess = queue.tryDequeue(value, READ_TIMEOUT_MS);
        ASSERT_TRUE(readSuccess);
        ASSERT_EQ(VALUE, value);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(READ_TIMEOUT_MS / 2));
    queue.tryEnqueue(123);
    reader.join();
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, ReaderUnblockedByBlockingWrite) {
    constexpr int CAPACITY = 5;
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(CAPACITY);
    constexpr auto VALUE = 123;
    constexpr auto READ_TIMEOUT_MS = 2000;
    constexpr auto WRITE_TIMEOUT_MS = READ_TIMEOUT_MS / 2;
    std::thread reader([&]() {
        int value;
        const bool readSuccess = queue.tryDequeue(value, READ_TIMEOUT_MS);
        ASSERT_TRUE(readSuccess);
        ASSERT_EQ(VALUE, value);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(READ_TIMEOUT_MS / 2));
    queue.tryEnqueue(123, WRITE_TIMEOUT_MS);
    reader.join();
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, WriterUnblockedByNonblockingRead) {
    constexpr int CAPACITY = 5;
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(CAPACITY);
    constexpr auto VALUE = 123;
    constexpr auto WRITE_TIMEOUT_MS = 2000;
    std::thread writer([&]() {
        const bool writeSuccess = queue.tryEnqueue(VALUE, WRITE_TIMEOUT_MS);
        ASSERT_TRUE(writeSuccess);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(WRITE_TIMEOUT_MS / 2));
    int value;
    const bool readSuccess = queue.tryDequeue(value);
    ASSERT_TRUE(readSuccess);
    ASSERT_EQ(VALUE, value);
    writer.join();
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, WriterUnblockedByBlockingRead) {
    constexpr int CAPACITY = 5;
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(CAPACITY);
    constexpr auto VALUE = 123;
    constexpr auto WRITE_TIMEOUT_MS = 2000;
    constexpr auto READ_TIMEOUT_MS = WRITE_TIMEOUT_MS / 2;
    std::thread writer([&]() {
        const bool writeSuccess = queue.tryEnqueue(VALUE, WRITE_TIMEOUT_MS);
        ASSERT_TRUE(writeSuccess);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(WRITE_TIMEOUT_MS / 2));
    int value;
    const bool readSuccess = queue.tryDequeue(value, READ_TIMEOUT_MS);
    ASSERT_TRUE(readSuccess);
    ASSERT_EQ(VALUE, value);
    writer.join();
}

// value: the value that will be enqueued repeatedly
// itemCount: the number of times to enqueue the given value
template<typename T>
void blockingStressTest(queues::MultipleProducerMultipleConsumerBlockingQueue<T> &queue, const T &value,
                        const int itemCount,
                        const int readerThreadCount,
                        const int writerThreadCount) {
    constexpr auto READ_TIMEOUT_MS = 1000;
    constexpr auto WRITE_TIMEOUT_MS = 1000;

    std::thread readers[readerThreadCount];
    std::thread writers[writerThreadCount];
    const auto itemsPerReader = itemCount / readerThreadCount;
    const auto itemsPerWriter = itemCount / writerThreadCount;
    Stopwatch sw;
    sw.start();
    for (int readerThreadIdx = 0; readerThreadIdx < readerThreadCount; ++readerThreadIdx) {
        readers[readerThreadIdx] = std::thread([&]() {
            for (long i = 0; i < itemsPerReader; ++i) {
                int readValue;
                const bool readSuccess = queue.tryDequeue(readValue, READ_TIMEOUT_MS);
                ASSERT_TRUE(readSuccess);
                ASSERT_EQ(value, readValue);
            }
        });
    }

    for (int writerThreadIdx = 0; writerThreadIdx < writerThreadCount; ++writerThreadIdx) {
        writers[writerThreadIdx] = std::thread([&]() {
            for (long i = 0; i < itemsPerWriter; ++i) {
                const bool writeSuccess = queue.tryEnqueue(value, WRITE_TIMEOUT_MS);
                ASSERT_TRUE(writeSuccess);
            }
        });
    }

    for (int readerThreadIdx = 0; readerThreadIdx < readerThreadCount; ++readerThreadIdx) {
        readers[readerThreadIdx].join();
    }
    for (int writerThreadIdx = 0; writerThreadIdx < writerThreadCount; ++writerThreadIdx) {
        writers[writerThreadIdx].join();
    }
    sw.stop();
    assertReadingBlocks(queue, 1000);
    const auto elapsedSeconds = sw.getElapsedMilliseconds() / 1000.0;
    std::stringstream ss;
    ss << std::setprecision(3);
    ss << "Under high contention with " << readerThreadCount << " readers and " << writerThreadCount
            << " writers, enqueued and dequeued " << itemCount << " items in "
            << elapsedSeconds << " sec ("
            << itemCount / elapsedSeconds << " items/sec)\n";
    std::cout << ss.str();
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, BlockingStressOneReaderOneWriter) {
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(1024);
    blockingStressTest(queue, 123, 1024 * 1024 * 5, 1, 1);
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, BlockingStressCapacityOne) {
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(1);
    blockingStressTest(queue, 123, 1024 * 5, 1, 1);
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, BlockingStressTwoReadersTwoWriters) {
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(1024);
    blockingStressTest(queue, 123, 1024 * 1024 * 5, 2, 2);
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, BlockingStressTwoReadersOneWriter) {
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(1024);
    blockingStressTest(queue, 123, 1024 * 1024 * 5, 2, 1);
}

TEST(MultipleProducerMultipleConsumerBlockingQueue, NoContention) {
    constexpr auto CAPACITY = 1024;
    constexpr auto VALUE = 123;
    constexpr auto FILL_EMPTY_CYCLE_COUNT = 1024 * 10;
    queues::MultipleProducerMultipleConsumerBlockingQueue<int> queue(CAPACITY);
    Stopwatch sw;
    sw.start();
    for (int fillEmptyCycle = 1; fillEmptyCycle <= FILL_EMPTY_CYCLE_COUNT; ++fillEmptyCycle) {
        // Fill the queue.
        enqueueCopies(queue, VALUE, CAPACITY);

        // Empty the queue.
        for (int i = 0; i < CAPACITY; ++i) {
            int value;
            const bool readOk = queue.tryDequeue(value);
            ASSERT_TRUE(readOk);
            ASSERT_EQ(VALUE, value);
        }
    }
    sw.stop();
    const auto itemCount = CAPACITY * FILL_EMPTY_CYCLE_COUNT;
    const auto elapsedSeconds = sw.getElapsedMilliseconds() / 1000.0;
    std::stringstream ss;
    ss << std::setprecision(3);
    ss << "Under no contention, enqueued and dequeued " << itemCount
            << " items in " << elapsedSeconds << " sec ("
            << itemCount / elapsedSeconds << " items/sec)\n";
    std::cout << ss.str();
}
