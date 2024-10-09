#pragma once
#include "Queue.h"
#include <shared_mutex>
#include <condition_variable>
#include <chrono>

namespace queues {
    template<typename T>
    class MultipleProducerMultipleConsumerBlockingQueue :
            public NonblockingQueue<T>,
            public BlockingReadQueue<T>,
            public BlockingWriteQueue<T> {
        const size_t capacity;
        T* items;
        size_t writeIndex = 0;
        size_t readIndex = 0;
        size_t size = 0;
        std::timed_mutex dataMutex;
        std::condition_variable_any readyToRead, readyToWrite;

        [[nodiscard]] size_t getNextIndex(const size_t index) const {
            const auto nextIndex = index + 1;
            if (nextIndex == capacity)
                return 0;
            return nextIndex;
        }

        void advanceIndex(size_t &index) {
            index = getNextIndex(index);
        }

        void unsafeEnqueue(std::unique_lock<std::timed_mutex> &lock, const T &item) {
            items[writeIndex] = item;
            advanceIndex(writeIndex);
            ++size;
            lock.unlock();
            readyToRead.notify_one();
        }

        void unsafeDequeue(std::unique_lock<std::timed_mutex> &lock, T &item) {
            item = items[readIndex];
            advanceIndex(readIndex);
            --size;
            lock.unlock();
            readyToWrite.notify_one();
        }

    public:
        explicit MultipleProducerMultipleConsumerBlockingQueue(size_t capacity)
            : capacity(capacity), items(new T[capacity]) {
        }

        MultipleProducerMultipleConsumerBlockingQueue(MultipleProducerMultipleConsumerBlockingQueue &&other) = delete;

        ~MultipleProducerMultipleConsumerBlockingQueue() {
            delete[] items;
        }

        bool tryEnqueue(const T &item) override {
            std::unique_lock lock(dataMutex, std::try_to_lock);
            if (!lock.owns_lock()) {
                return false;
            }
            if (size == capacity)
                return false;
            unsafeEnqueue(lock, item);
            return true;
        }

        template<class Rep, class Period = std::ratio<1>>
        bool tryEnqueue(const T &item, std::chrono::duration<Rep, Period> timeout) {
            using namespace std::chrono;
            const auto functionStartTime = steady_clock::now();
            std::unique_lock lock(dataMutex, timeout);
            const auto timeToAcquireLock = duration_cast<typeof(timeout)>(steady_clock::now() - functionStartTime);
            if (!lock.owns_lock() || timeToAcquireLock >= timeout)
                return false; // Timed out waiting for lock.
            timeout -= timeToAcquireLock;
            if (!readyToWrite.wait_for(lock, timeout, [&] { return size < capacity; }))
                return false; // Timed out waiting for space in queue.
            unsafeEnqueue(lock, item);
            return true;
        }

        bool tryEnqueue(const T &item, const long timeoutMilliseconds) override {
            return tryEnqueue(item, std::chrono::milliseconds(timeoutMilliseconds));
        }

        bool tryDequeue(T &item) override {
            std::unique_lock lock(dataMutex, std::try_to_lock);
            if (!lock.owns_lock())
                return false;
            if (size == 0)
                return false;
            unsafeDequeue(lock, item);
            return true;
        }

        template<class Rep, class Period = std::ratio<1>>
        bool tryDequeue(T &item, const std::chrono::duration<Rep, Period> &timeout) {
            using namespace std::chrono;
            const auto functionStartTime = steady_clock::now();
            std::unique_lock lock(dataMutex, timeout);
            const auto timeToAcquireLock = steady_clock::now() - functionStartTime;
            if (!lock.owns_lock() || timeToAcquireLock >= timeout)
                return false; // Timed out waiting for lock.
            const auto remainingTimeout = timeout - timeToAcquireLock;
            if (!readyToRead.wait_for(lock, timeout, [&] { return size > 0; }))
                return false; // Timed out waiting for space in queue.
            unsafeDequeue(lock, item);
            return true;
        }

        bool tryDequeue(T &item, const long timeoutMilliseconds) override {
            return tryDequeue(item, std::chrono::milliseconds(timeoutMilliseconds));
        }
    };
}
