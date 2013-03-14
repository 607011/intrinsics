// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __INTRINSICS_UTIL_H_
#define __INTRINSICS_UTIL_H_

#include <string>
#include <vector>

#if defined(WIN32)
#include "gnutypes.h"
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


struct LogicalProcessorData 
{
  unsigned int localApicId;
  bool HTT;
  unsigned int logicalProcessorCount;
};


class CPUFeatures {
private: // Singleton
  CPUFeatures(void); 

  void detectVendor(void);
  void getCoresForCurrentProcessor(unsigned int& nCores, unsigned int& nThreadsPerCore);
  void detectFeatures(void);

  void lockToLogicalProcessor(int core);

public:
  static CPUFeatures& instance(void) { 
    static CPUFeatures INSTANCE;
    return INSTANCE;
  }
  bool isGenuineIntelCPU(void);
  bool isAuthenticAMDCPU(void);
  bool isCRCSupported(void);
  bool isRdRandSupported(void);
  void evaluateCPUFeatures(void);
  int getNumCores(void);
  int count(int& numaNodeCount,
	    int& processorCoreCount,
	    int& logicalProcessorCount,
	    int& processorPackageCount);

public: // member variables
  unsigned int max_func;
  unsigned int cores;
  unsigned int threads_per_package;
  unsigned int logical_cores_from_system;
  unsigned int logical_cores;
  unsigned int logical_threads_apic_id_0bh;
  unsigned int logical_threads_0bh;
  unsigned int logical_cores_apic_id_0bh;
  unsigned int logical_cores_0bh;
  unsigned int clflush_linesize;
  unsigned int cpu_type;
  unsigned int cpu_family;
  unsigned int cpu_ext_family;
  unsigned int cpu_model;
  unsigned int cpu_ext_model;
  unsigned int cpu_stepping;
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
  std::string vendor;
  std::vector<LogicalProcessorData> logical_cpu_data;

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
