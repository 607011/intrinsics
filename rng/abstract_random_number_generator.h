// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __INTRINSICS_ABSTRACT_RANDOM_NUMBER_GENERATOR_H_
#define __INTRINSICS_ABSTRACT_RANDOM_NUMBER_GENERATOR_H_

#if defined(WIN32)
#include <Windows.h>
#include "gnutypes.h"
#elif defined(__GNUC__)
#include <inttypes.h>
#include <stdint.h>
#include <time.h>
#endif

#include <stdlib.h>

#include "sharedutil.h"

template <typename T>
class AbstractRandomNumberGenerator
{
public:
 AbstractRandomNumberGenerator(void)
   : mInvalid(0)
   , mLimitExceeded(0) 
    { /* ... */ }
  virtual ~AbstractRandomNumberGenerator() { /* ... */ }
  virtual T operator()(void) = 0;
  virtual T next(void) {return (*this)(); }
  virtual void next(T& dst) { dst = next(); }
  virtual void seed(T) { /* ... */ }
  virtual void seed(void) { seed(makeSeed()); }
  static T makeSeed(void);
  static const char* name(void) { return "Please overwrite AbstractRandomNumberGenerator::name() to provide a sensible name for your RNG!"; }
  static int result_size(void) { return sizeof(T); }
  
  typedef T result_t;
  
  uint64_t invalid(void) const { return mInvalid; }
  uint64_t limitExceeded(void) const { return mLimitExceeded; }
  
  static const int RETRY_LIMIT = 10;
  
protected:
  uint64_t mInvalid;
  uint64_t mLimitExceeded;
};


template <typename T>
T AbstractRandomNumberGenerator<T>::makeSeed(void)
{
  T seed = 0;
#if defined(WIN32)
  if (isRdRandSupported()) {
    seed = (T)getRdRand32();
  }
  else { 
    seed = (T)GetTickCount();
  }
#elif defined(__GNUC__)
  seed = (T)clock();
#endif
  return seed;
}


typedef AbstractRandomNumberGenerator<uint8_t> ByteRandomNumberGenerator;
typedef AbstractRandomNumberGenerator<uint32_t> UInt32RandomNumberGenerator;
typedef AbstractRandomNumberGenerator<uint64_t> UInt64RandomNumberGenerator;


class DummyByteGenerator : public ByteRandomNumberGenerator
{
 public:
  DummyByteGenerator(void) { }
  uint8_t operator()() { return 0x00U; }
  static const char* name(void) { return "Dummy Byte"; }
};


class DummyUIntGenerator : public UInt32RandomNumberGenerator
{
 public:
  DummyUIntGenerator(void) { }
  uint32_t operator()() { return 0x00000000U; }
  static const char* name(void) { return "Dummy Int"; }
};


#endif
