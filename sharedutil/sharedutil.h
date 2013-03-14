// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __SHAREDUTIL_H_
#define __SHAREDUTIL_H_

#if defined(WIN32)
#include "gnutypes.h"
#include <intrin.h>
#endif

#if defined(__GNUC__)
#include <stdint.h>
#include <inttypes.h>
#endif

extern "C" {
#if defined(WIN32)
bool hasRand_s(void);
#endif
}


#if defined(__GNUC__)
inline unsigned int _rdrand16_step(uint16_t* x) {
  return __builtin_ia32_rdrand16_step(x);
}
inline unsigned int _rdrand32_step(uint32_t* x) {
  return __builtin_ia32_rdrand32_step(x);
}
inline unsigned int _rdrand64_step(uint64_t* x) {
  return __builtin_ia32_rdrand64_step(reinterpret_cast<long long unsigned int*>(x));
}


inline void 
__get_cpuidex(/* IN */ uint32_t infoType, uint32_t ecxInput,
	      /* OUT */ uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx)
{
  asm volatile (
		"cpuid\n"
		: "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
		: "a"(infoType), "c"(ecxInput)
		);
}
#endif


inline uint32_t getRdRand32(void)
{
  uint32_t x;
  _rdrand32_step(&x);
  return x;
}


#if defined(_M_X64)
inline uint64_t getRdRand64(void)
{
  uint64_t x;
  _rdrand64_step(&x);
  return x;
}
#endif

#endif // __SHAREDUTIL_H_
