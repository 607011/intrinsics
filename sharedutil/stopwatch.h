// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#ifndef __INTRINSICS_STOPWATCH_H_
#define __INTRINSICS_STOPWATCH_H_

#ifdef WIN32
#include <Windows.h>
#include <immintrin.h>
#endif

#ifndef WIN32
inline volatile long long __rdtsc() {
  register long long TSC asm("eax");
  asm volatile (".byte 15, 49" : : : "eax", "edx");
  return TSC;
}

#include <time.h>
#include <inttypes.h>
#include <stdint.h>
#endif

class Stopwatch {
 public:
  inline Stopwatch(int64_t& dTime, int64_t& dTicks)
    : mT(dTime)
    , mTicks(dTicks)
  {
    mT = INVALID;
    mTicks = INVALID;
#ifdef WIN32
    QueryPerformanceCounter(&mPC0);
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    mPC0 = t.tv_sec * 1000 + t.tv_nsec / 1000;
#endif
    mTicks0 = (int64_t)__rdtsc();
  }
  inline ~Stopwatch()
  {
    stop();
  }
  inline void stop(void)
  {
    mTicks = (int64_t)__rdtsc() - mTicks0;
#ifdef WIN32
    LARGE_INTEGER pc, freq;
    QueryPerformanceCounter(&pc);
    QueryPerformanceFrequency(&freq);
    mT = 1000 * (pc.QuadPart - mPC0.QuadPart) / freq.QuadPart;
#else
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    mT = (t.tv_sec * 1000 + t.tv_nsec / 1000) - mPC0;
#endif
  }
  static const int64_t INVALID = -1;
  
 private:
  int64_t& mT;
  int64_t& mTicks;
#ifdef WIN32
  LARGE_INTEGER mPC0;
#else
  int64_t mPC0;
#endif
  int64_t mTicks0;
};

#endif
