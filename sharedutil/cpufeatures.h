// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __CPUFEATURES_H_
#define __CPUFEATURES_H_

#include "sharedutil.h"

#include <string>
#include <vector>

#if defined(WIN32)
#include "gnutypes.h"
#endif

#if defined(__GNUC__)
#include <stdint.h>
#include <inttypes.h>
#endif


struct LogicalProcessorData {
  unsigned int localApicId;
  unsigned int logicalProcessorCount;
  bool HTT;
};


class CPUFeatures {
private: // Singleton
  CPUFeatures(void); 

  void detectVendor(void);
  void detectFeatures(void);
  void getCoresForCurrentProcessor(unsigned int& nCores, unsigned int& nThreadsPerCore);

public:
  static CPUFeatures& instance(void) { 
    static CPUFeatures INSTANCE;
    return INSTANCE;
  }
  bool isGenuineIntelCPU(void) const;
  bool isAuthenticAMDCPU(void) const;
  bool isCRCSupported(void) const;
  bool isRdRandSupported(void) const;
  void evaluateCPUFeatures(void);
  int getNumCores(void) const;
  static void lockToLogicalProcessor(int core);
  static void unlockFromLogicalProcessor(void);

#if defined(WIN32)
  int count(int& numaNodeCount, int& processorCoreCount, int& logicalProcessorCount, int& processorPackageCount);
#endif

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



#endif // __CPUFEATURES_H_
