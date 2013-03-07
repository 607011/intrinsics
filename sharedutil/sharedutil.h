// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#ifndef __INTRINSICS_UTIL_H_
#define __INTRINSICS_UTIL_H_

extern "C" {

extern int gVerbose;

#if defined(WIN32)
extern bool hasRand_s(void);
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif

#if defined(__GNUC__)
#include <stdint.h>
#include <inttypes.h>
#endif

extern bool isGenuineIntelCPU(void);
extern bool isRdRandSupported(void);
extern bool isCRCSupported(void);
extern unsigned int getRdRand32(void);
#if defined(_M_X64) || defined(__x86_64__)
extern unsigned long long getRdRand64(void);
#endif
extern void evaluateCPUFeatures(void);
extern unsigned int getNumCores(void);

#if defined(__GNUC__)
inline  unsigned int _rdrand8_step(unsigned char* x) {
  (void)x; 
  return 0; // TODO
}
inline  unsigned int _rdrand16_step(unsigned short* x) {
  return __builtin_ia32_rdrand16_step(x);
}
inline  unsigned int _rdrand32_step(unsigned int* x) {
  return __builtin_ia32_rdrand32_step(x);
}
inline  unsigned int _rdrand64_step(unsigned long long* x) {
  return __builtin_ia32_rdrand64_step(x);
}
#endif

}

#endif // __INTRINSICS_UTIL_H_
