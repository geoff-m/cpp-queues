#pragma once
#include <chrono>

class Stopwatch {
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime = {};
    std::chrono::milliseconds elapsedTime = {};
public:
    void start();
    void stop();
    void reset();
    long getElapsedMilliseconds();
};