// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __INTRINSICS_MT_H_
#define __INTRINSICS_MT_H_

#include "abstract_random_number_generator.h"

class MersenneTwister : public UInt32RandomNumberGenerator
{
public:
  MersenneTwister(void) { /* ... */ }
  uint32_t operator()();
  void seed(uint32_t);
  inline void seed(void) { seed(makeSeed()); }
  static const char* name(void) { return "Mersenne-Twister"; }

private:
  static const int N = 624;
  static const int M = 397;
  static const uint32_t LO = 0x7fffffffU;
  static const uint32_t HI = 0x80000000U;
  static const uint32_t A[2];
  uint32_t mY[N];
  int mIndex;

private: // methods
  void warmup(void);
};

#endif
