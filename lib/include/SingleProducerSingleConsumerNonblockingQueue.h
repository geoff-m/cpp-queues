#pragma once
#include "Queue.h"

namespace queues {
    template<typename T>
    class SingleProducerSingleConsumerNonblockingQueue
        : public NonblockingQueue<T> {
        const size_t capacity;
        T *items;
        size_t writeIndex = 0;
        size_t readIndex = 0;

        [[nodiscard]] size_t getNextIndex(const size_t index) const {
            const auto nextIndex = index + 1;
            if (nextIndex == capacity)
                return 0;
            return nextIndex;
        }

        void advanceIndex(size_t &index) {
            index = getNextIndex(index);
        }

        [[nodiscard]] bool isEmpty() const {
            return writeIndex == readIndex;
        }

        [[nodiscard]] bool isFull() const {
            return getNextIndex(writeIndex) == readIndex;
        }

    public:
        explicit SingleProducerSingleConsumerNonblockingQueue(size_t capacity)
            : capacity(capacity), items(new T[capacity]) {
        }

        SingleProducerSingleConsumerNonblockingQueue(SingleProducerSingleConsumerNonblockingQueue&& other) = delete;

        ~SingleProducerSingleConsumerNonblockingQueue() {
            delete[] items;
        }

        bool tryDequeue(const T &item) override {
            if (isFull())
                return false;
            items[writeIndex] = item;
            advanceIndex(writeIndex);
            return true;
        }

        bool tryDequeue(T &item) override {
            if (isEmpty())
                return false;
            item = items[readIndex];
            advanceIndex(readIndex);
            return true;
        }
    };
}
