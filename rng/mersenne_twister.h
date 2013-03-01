// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#ifndef __INTRINSICS_MT_H_
#define __INTRINSICS_MT_H_

#include "abstract_random_number_generator.h"

class MersenneTwister : public UIntRandomNumberGenerator
{
public:
  MersenneTwister(void) { /* ... */ }
  unsigned int operator()();
  void seed(unsigned int);
  inline void seed(void) { seed(makeSeed()); }
  static const char* name(void) { return "Mersenne-Twister"; }

private:
  static const int N = 624;
  static const int M = 397;
  static const unsigned int LO = 0x7fffffff;
  static const unsigned int HI = 0x80000000;
  static const unsigned int A[2];
  unsigned int mY[N];
  int mIndex;

private: // methods
  void warmup(void);
};

#endif
