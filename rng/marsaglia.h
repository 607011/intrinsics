// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// Portiert von http://www.agner.org/random/
// All rights reserved.

#ifndef __INTRINSICS_MARSAGLIA_H_
#define __INTRINSICS_MARSAGLIA_H_

#include "abstract_random_number_generator.h"

class MultiplyWithCarry : public UInt32RandomNumberGenerator
{
public:
  MultiplyWithCarry(uint32_t _Seed = 0x9908b0dfU) {
    seed(_Seed);
  }
  uint32_t operator()() {
    uint64_t sum =
      2111111111ULL * (uint64_t)mR[3] +
      1492ULL       * (uint64_t)mR[2] +
      1776ULL       * (uint64_t)mR[1] +
      5115ULL       * (uint64_t)mR[0] +
      (uint64_t)mR[4];
    mR[3] = mR[2];
    mR[2] = mR[1];
    mR[1] = mR[0];
    mR[4] = (uint32_t)(sum >> 32);
    mR[0] = (uint32_t)(sum & 0xffffffffULL);
    return mR[0];
  }
  inline void seed(uint32_t _Seed) { warmup(_Seed); }
  inline void seed(void) { seed(makeSeed()); }
  static const char* name(void) { return "Marsaglia"; }
  
 private:
  inline void warmup(uint32_t s) {
    for (int i = 0; i < 5; ++i)
      mR[i] = s = s * 29943829U - 1U;
    for (size_t i = 0; i < 19; ++i)
      (*this)();
  }
  uint32_t mR[5];
};

#endif
