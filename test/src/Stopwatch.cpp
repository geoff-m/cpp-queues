#include "Stopwatch.h"

void Stopwatch::start() {
  startTime = std::chrono::high_resolution_clock::now();
}

void Stopwatch::stop() {
  using namespace std::chrono;
  elapsedTime += duration_cast<milliseconds>(high_resolution_clock::now() - startTime);
}

void Stopwatch::reset() {
  elapsedTime = {};
}

long Stopwatch::getElapsedMilliseconds() {
  return elapsedTime.count();
}