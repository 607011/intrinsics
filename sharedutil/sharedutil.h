// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __INTRINSICS_UTIL_H_
#define __INTRINSICS_UTIL_H_

#if defined(WIN32)
#include "gnutypes.h"
#endif

#include <string>

#if defined(__GNUC__)
#include <stdint.h>
#include <inttypes.h>
#endif

extern "C" {
#if defined(WIN32)
  extern bool hasRand_s(void);
#endif
}


#if defined(__GNUC__)
inline  unsigned int _rdrand8_step(uint8_t* x) {
  (void)x; 
  return 0; // TODO
}
inline  unsigned int _rdrand16_step(uint16_t* x) {
  return __builtin_ia32_rdrand16_step(x);
}
inline  unsigned int _rdrand32_step(uint32_t* x) {
  return __builtin_ia32_rdrand32_step(x);
}
inline  unsigned int _rdrand64_step(uint64_t* x) {
  return __builtin_ia32_rdrand64_step(reinterpret_cast<long long unsigned int*>(x));
}
#endif


class CPUFeatures {
public:
  CPUFeatures(void);
  std::string cpuVendor(void);
  bool isGenuineIntelCPU(void);
  bool isAuthenticAMDCPU(void);
  bool isCRCSupported(void);
  bool isRdRandSupported(void);
  void evaluateCPUFeatures(void);
  int getNumCores(void);
  int count(int& numaNodeCount, int& processorCoreCount, int& logicalProcessorCount, int& processorPackageCount);

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

  int cores;
  int threads_per_package;
  int clflush_linesize;
  int cpu_type;
  int cpu_family;
  int cpu_ext_family;
  int cpu_model;
  int cpu_ext_model;
  int cpu_stepping;
  int logical_cores;
  bool sse3_supported;
  bool ssse3_supported;
  bool monitor_wait_supported;
  bool vmx_supported;
  bool sse41_supported;
  bool sse42_supported;
  bool fma_supported;
  bool popcnt_supported;
  bool aes_supported;
  bool avx_supported;
  bool f16c_supported;
  bool rdrand_supported;
  bool mmx_supported;
  bool sse_supported;
  bool sse2_supported;
  bool htt_supported;
  bool ht_supported;

  typedef union {
    int reg[4];
    struct {
      uint32_t eax;
      uint32_t ebx;
      uint32_t ecx;
      uint32_t edx;
    };
  } cpuid_result_t;
};


#endif // __INTRINSICS_UTIL_H_
