#pragma once
#include <gtest/gtest.h>
#include "Stopwatch.h"
#include "queues.h"

template<typename T>
void enqueueCopies(queues::NonblockingQueue<T> &queue, const T &item, int count) {
    for (int i = 1; i <= count; ++i)
        if (!queue.tryEnqueue(item))
            FAIL();
}

template<typename T>
void assertReadingBlocks(queues::BlockingReadQueue<T> &queue, long timeoutMilliseconds) {
    T value;
    Stopwatch sw;
    sw.start();
    const bool readSuccess = queue.tryDequeue(value, timeoutMilliseconds);
    sw.stop();

    // Assert that the operation failed.
    ASSERT_FALSE(readSuccess);

    // Assert that the operation took at least as long as the timeout.
    ASSERT_TRUE(sw.getElapsedMilliseconds() >= timeoutMilliseconds);
}
