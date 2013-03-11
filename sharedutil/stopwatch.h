// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#ifndef __INTRINSICS_STOPWATCH_H_
#define __INTRINSICS_STOPWATCH_H_

#if defined(WIN32)
#include <Windows.h>
#include <immintrin.h>
#include "gnutypes.h"
#endif

#if defined(__GNUC__)
inline volatile uint64_t __rdtsc(void) {
  uint32_t lo, hi;
  asm volatile ("CPUID\n"
		"RDTSC\n"
		"mov %%edx, %0\n"
		"mov %%eax, %1\n"
		: "=r" (hi), "=r" (lo)
		:
		: "%rax", "%rbx", "%rcx", "%rdx");
  return (uint64_t)hi << 32 | lo;
}
inline volatile uint64_t __rdtscp(void) {
  uint32_t lo, hi;
  asm volatile ("RDTSC\n"
		"mov %%edx, %0\n"
		"mov %%eax, %1\n"
		"CPUID\n"
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
    mTicks = (int64_t)__rdtscp() - mTicks0;
#ifdef WIN32
    LARGE_INTEGER pc, freq;
    QueryPerformanceCounter(&pc);
    QueryPerformanceFrequency(&freq);
    mT = 1000 * (pc.QuadPart - mPC0.QuadPart) / freq.QuadPart;
#else
    mT = currentMS() - mPC0;
#endif
  }

#ifndef WIN32
  inline int64_t currentMS(void) const {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000 + (int64_t)t.tv_nsec / 1000000;
  }
#endif

  static const int64_t INVALID = -1;
  
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
