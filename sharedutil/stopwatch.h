// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __INTRINSICS_STOPWATCH_H_
#define __INTRINSICS_STOPWATCH_H_

#if defined(WIN32)
#include <Windows.h>
#include <immintrin.h>
#include "gnutypes.h"
#endif

#include <limits>

#if defined(__GNUC__)
inline volatile uint64_t __rdtsc(void) {
  uint32_t lo, hi;
  asm volatile (
    "cpuid\n"
    "rdtsc\n"
    "mov %%edx, %0\n"
    "mov %%eax, %1\n"
    : "=r" (hi), "=r" (lo)
    :
  : "%rax", "%rbx", "%rcx", "%rdx");
  return (uint64_t)hi << 32 | lo;
}
inline volatile uint64_t __rdtscp(void) {
  uint32_t lo, hi;
  asm volatile (
    "rdtscp\n"
    "mov %%edx, %0\n"
    "mov %%eax, %1\n"
    "cpuid\n"
    : "=r" (hi), "=r" (lo)
    :
  : "%rax", "%rbx", "%rcx", "%rdx");
  return (uint64_t)hi << 32 | lo;
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
    start();
  }

  inline ~Stopwatch()
  {
    stop();
  }

  inline void start(void)
  {
    mT = INVALID;
    mTicks = INVALID;
#ifdef WIN32
    QueryPerformanceCounter(&mPC0);
#else
    mPC0 = currentMS();
#endif
    mTicks0 = (int64_t)__rdtsc();
  }

  inline void stop(void)
  {
    mTicks = (int64_t)__rdtsc() - mTicks0;
#ifdef WIN32
    LARGE_INTEGER pc, freq;
    QueryPerformanceCounter(&pc);
    QueryPerformanceFrequency(&freq);
    mT = RESOLUTION * (pc.QuadPart - mPC0.QuadPart) / freq.QuadPart;
#else
    mT = currentMS() - mPC0;
#endif
  }

#ifndef WIN32
  inline int64_t currentMS(void) const {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return RESOLUTION * (int64_t)t.tv_sec + RESOLUTION / 1000000000 * (int64_t)t.tv_nsec;
  }
#endif

  static int64_t getAccuracy(void)
  {
    int64_t dt = 0;
    int64_t ticks = 0;
    int64_t tbest = LLONG_MAX;
    Stopwatch stopwatch(dt, ticks);
    for (int i = 0; i < 1000; ++i) {
      do {
        stopwatch.start();
        do {
          stopwatch.stop();
        }
        while (dt == 0);
      }
      while (dt < 0); // nochmal, falls Timer "gestolpert" ist
      if (dt < tbest)
        tbest = dt;
    }
    return tbest;
  }

  static const int64_t INVALID = -1;
  static const int64_t RESOLUTION = 1000000; // 1/RESOLUTION s

private:
  int64_t& mT;
  int64_t& mTicks;
#if defined(WIN32)
  LARGE_INTEGER mPC0;
#elif defined(__GNUC__)
  int64_t mPC0;
#endif
  int64_t mTicks0;
};

#endif
