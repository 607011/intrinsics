// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#ifndef __RDRAND_H__
#define __RDRAND_H__

#if defined(WIN32)
#include <immintrin.h>
#include "gnutypes.h"
#endif

#include "abstract_random_number_generator.h"


class RdRand16 : public AbstractRandomNumberGenerator<uint16_t>
{
public:
  RdRand16(void) { /* ... */ }
  inline uint16_t operator()() {
    uint16_t x;
    _rdrand16_step(&x);
    return x;
  }
  inline void next(uint16_t& r) {
    int tries = AbstractRandomNumberGenerator::RETRY_LIMIT;
    do {
      if (_rdrand16_step(&r))
	return;
      ++mInvalid;
    }
    while (tries--);
    ++mLimitExceeded;
  }
  static const char* name(void) { return "rdrand16"; }
};


class RdRand32 : public AbstractRandomNumberGenerator<uint32_t>
{
 public:
  RdRand32(void) { /* ... */ }
  inline uint32_t operator()() {
    uint32_t x;
    _rdrand32_step(&x);
    return x;
  }
  inline void next(uint32_t& r) {
    int tries = AbstractRandomNumberGenerator<uint32_t>::RETRY_LIMIT;
    do {
      if (_rdrand32_step(&r)) 
	return;
      ++mInvalid;
    }
    while (tries--);
    ++mLimitExceeded;
  }
  static const char* name(void) { return "rdrand32"; }
};


#if defined(_M_X64) || defined(__x86_64__)
class RdRand64 : public AbstractRandomNumberGenerator<uint64_t>
{
 public:
  RdRand64(void) { /* ... */ }
  inline uint64_t operator()() {
    uint64_t x;
    _rdrand64_step(&x);
    return x;
  }
  inline void next(uint64_t& r) {
    int tries = AbstractRandomNumberGenerator<uint64_t>::RETRY_LIMIT;
    do {
      if (_rdrand64_step(&r))
	return;
      ++mInvalid;
    }
    while (tries--);
    ++mLimitExceeded;
  }
  static const char* name(void) { return "rdrand64"; }
};
#endif


#endif // __RDRAND_H__
