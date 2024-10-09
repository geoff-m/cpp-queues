#pragma once

namespace queues {
    template<typename T>
    struct NonblockingQueue {
        virtual bool tryEnqueue(const T &item) = 0;

        virtual bool tryDequeue(T &item) = 0;

        virtual ~NonblockingQueue() = default;
    };

    template<typename T>
    struct BlockingWriteQueue {
        virtual bool tryEnqueue(const T &item, long timeoutMilliseconds) = 0;

        virtual ~BlockingWriteQueue() = default;
    };

    template<typename T>
    struct BlockingReadQueue {
        virtual bool tryDequeue(T &item, long timeoutMilliseconds) = 0;

        virtual ~BlockingReadQueue() = default;
    };
}
