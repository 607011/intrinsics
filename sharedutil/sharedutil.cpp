// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag


#ifdef WIN32
#define _CRT_RAND_S
#include <Windows.h>
#include <intrin.h>
#include "gnutypes.h"
#else
#include <unistd.h>
#include <cpuid.h>
#endif

#include "sharedutil.h"

#include <string>
#include <iostream>
#include <iomanip>

int gVerbose = 0;


#ifdef WIN32
bool hasRand_s(void) 
{
  OSVERSIONINFO os;
  GetVersionEx(&os);
  return (os.dwMajorVersion > 5) && (os.dwMinorVersion >= 1);
}
#endif


int getNumCores(void)
{
#ifdef WIN32
  SYSTEM_INFO pInfo;
  GetSystemInfo(&pInfo);
  return (int)pInfo.dwNumberOfProcessors;
#else
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}


typedef union {
    uint32_t reg[4];
    struct {
      uint32_t eax, ebx, ecx, edx;
    };
} cpuid_result_t;
  

std::string cpuVendor(void) {
  union {
    char str[12];
    struct {
      uint32_t reg[3];
    };
  } vendor;
#if defined(WIN32)
  cpuid_result_t r;
  __cpuid(r.reg, 0);
  vendor.reg[0] = r.ebx;
  vendor.reg[1] = r.edx;
  vendor.reg[2] = r.ecx;
#else
  unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
  __get_cpuid(0, &eax, &ebx, &ecx, &edx);
  vendor.reg[0] = ebx;
  vendor.reg[1] = edx;
  vendor.reg[2] = ecx;
#endif
  return std::string(vendor.str, 12);
}




bool isGenuineIntelCPU(void) {
  return cpuVendor() == "GenuineIntel";
}


bool isAuthenticAMDCPU(void) {
  return cpuVendor() == "AuthenticAMD";
}



bool isCRCSupported(void) {
#ifdef WIN32
  int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
  __cpuid(cpureg, 1);
  return (cpureg[2] & (1<<20)) != 0;
#else
  uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
  return (ecx & (1<<20)) != 0;
#endif
}


bool isRdRandSupported(void) {
#ifdef WIN32
  if (!isGenuineIntelCPU())
    return false;
  int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
  __cpuid(cpureg, 1);
  return (cpureg[2] & (1<<30)) != 0;
#else
  uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
  return (ecx & (1<<30)) != 0;
#endif
}


int clFlushLineSize(void) {
  if (!isGenuineIntelCPU())
    return -1;
#ifdef WIN32
  int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
  __cpuid(cpureg, 1);
  return 8 * ((cpureg[1] >> 8) & 0xff);
#else
  uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
  return 8 * ((ebx >> 16) & 0xff);
#endif
}


uint32_t getRdRand32(void)
{
  uint32_t x;
  _rdrand32_step(&x);
  return x;
}


#if defined(_M_X64)
uint64_t getRdRand64(void)
{
  uint64_t x;
  _rdrand64_step(&x);
  return x;
}
#endif


void evaluateCPUFeatures(void) {
  extern int gVerbose;
  static const char* B[2] = { "false", "true" };
  int cores = -1;
  int threads_per_package = -1;
#ifdef WIN32
  cpuid_result_t r;
  __cpuid(r.reg, 1);
  int cpu_type = (r.eax >> 12) & 0x3;
  int cpu_family = (r.eax >> 8) & 0xf;
  int cpu_ext_family = (r.eax >> 20) & 0xff;
  int cpu_model = (r.eax >> 4) & 0xf;
  int cpu_ext_model = (r.eax >> 16) & 0xf;
  int cpu_stepping = r.eax & 0xf;
  // EBX
  int logicalCores = (r.ebx >> 16) & 0xff;
  // ECX
  bool sse3_supported = (r.ecx & (1<<0)) != 0;
  bool ssse3_supported = (r.ecx & (1<<9)) != 0;
  bool monitor_wait_supported = (r.ecx & (1<<3)) != 0;
  bool vmx_supported = (r.ecx & (1<<5)) != 0;
  bool sse41_supported = (r.ecx & (1<<19)) != 0;
  bool sse42_supported = (r.ecx & (1<<20)) != 0;
  bool fma_supported = (r.ecx & (1<<12)) != 0;
  bool popcnt_supported = (r.ecx & (1<<23)) != 0;
  bool aes_supported = (r.ecx & (1<<25)) != 0;
  bool avx_supported = (r.ecx & (1<<28)) != 0;
  bool f16c_supported = (r.ecx & (1<<29)) != 0;
  bool rdrand_supported = (r.ecx & (1<<30)) != 0;
  bool b31_supported = (r.ecx & (1<<31)) != 0;
  // EDX
  bool mmx_supported = (r.edx & (1<<23)) != 0;
  bool sse_supported = (r.edx & (1<<25)) != 0;
  bool sse2_supported = (r.edx & (1<<26)) != 0;
  bool ht_supported = (r.edx & (1<<28)) != 0;
  if (isGenuineIntelCPU()) {
    __cpuid(r.reg, 4);
    cores = ((r.eax >> 26) & 0x3f) + 1; // EAX[31:26] + 1
    threads_per_package = ((r.eax >> 14) & 0xfff) + 1;
  }
  else if (isAuthenticAMDCPU()) {
    __cpuid(r.reg, 0x80000008);
    cores = ((unsigned int)(r.ecx & 0xff)) + 1; // ECX[7:0] + 1
  }
#else
  uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
  int cpu_type = (eax >> 12) & 0x3;
  int cpu_family = (eax >> 8) & 0xf;
  int cpu_ext_family = (eax >> 20) & 0xff;
  int cpu_model = (eax >> 4) & 0xf;
  int cpu_ext_model = (eax >> 16) & 0xf;
  int cpu_stepping = eax & 0xf;
  int logicalCores = (ebx >> 16) & 0xff;
  bool sse3_supported = (ecx & (1<<0)) != 0;
  bool ssse3_supported = (ecx & (1<<9)) != 0;
  bool monitor_wait_supported = (ecx & (1<<3)) != 0;
  bool vmx_supported = (ecx & (1<<5)) != 0;
  bool sse41_supported = (ecx & (1<<19)) != 0;
  bool sse42_supported = (ecx & (1<<20)) != 0;
  bool fma_supported = (ecx & (1<<12)) != 0;
  bool popcnt_supported = (ecx & (1<<23)) != 0;
  bool aes_supported = (ecx & (1<<25)) != 0;
  bool avx_supported = (ecx & (1<<28)) != 0;
  bool f16c_supported = (ecx & (1<<29)) != 0;
  bool rdrand_supported = (ecx & (1<<30)) != 0;
  bool mmx_supported = (edx & (1<<23)) != 0;
  bool sse_supported = (edx & (1<<25)) != 0;
  bool sse2_supported = (edx & (1<<26)) != 0;
  bool ht_supported = (edx & (1<<28)) != 0;
  if (isGenuineIntelCPU()) {
    __get_cpuid(4, &eax, &ebx, &ecx, &edx);
    cores = ((eax >> 26) & 0x3f) + 1;
    threads_per_package = ((eax >> 14) & 0xfff) + 1;
  }
  else if (isAuthenticAMDCPU()) {
    __get_cpuid(0x80000008, &eax, &ebx, &ecx, &edx);
    cores = ((unsigned int)(ecx & 0xff)) + 1;
  }
#endif
  if (gVerbose > 1) {
    std::cout << ">>> CPU vendor       : " << cpuVendor() << std::endl;
    std::cout << ">>> #cores           : " << getNumCores() << std::endl;
    std::cout << ">>> #logical cores   : " << logicalCores << std::endl;
    std::cout << ">>> #cores           : " << cores << std::endl;
    std::cout << ">>> #threads per pkg : " << threads_per_package << std::endl;
    std::cout << ">>> Hyper-Threading  : " << B[ht_supported] << std::endl;
    std::cout << ">>> CPU type         : " << cpu_type << std::endl;
    std::cout << ">>> CPU family       : " << cpu_family << std::endl;
    std::cout << ">>> CPU ext family   : " << cpu_ext_family << std::endl;
    std::cout << ">>> CPU model        : " << cpu_model << std::endl;
    std::cout << ">>> CPU ext model    : " << cpu_ext_model << std::endl;
    std::cout << ">>> CPU stepping     : " << cpu_stepping << std::endl;
    std::cout << ">>> CLFLUSH line size: " << clFlushLineSize() << std::endl;
    std::cout << ">>> MMX              : " << B[mmx_supported] << std::endl;
    std::cout << ">>> SSE              : " << B[sse_supported] << std::endl;
    std::cout << ">>> SSE2             : " << B[sse2_supported] << std::endl;
    std::cout << ">>> SSE3             : " << B[sse3_supported] << std::endl;
    std::cout << ">>> SSSE3            : " << B[ssse3_supported] << std::endl;
    std::cout << ">>> SSE4.1           : " << B[sse41_supported] << std::endl;
    std::cout << ">>> SSE4.2           : " << B[sse42_supported] << std::endl;
    std::cout << ">>> FMA              : " << B[fma_supported] << std::endl;
    std::cout << ">>> MONITOR/WAIT     : " << B[monitor_wait_supported] << std::endl;
    std::cout << ">>> VMX              : " << B[vmx_supported] << std::endl;
    std::cout << ">>> F16C             : " << B[f16c_supported] << std::endl;
    std::cout << ">>> AVX              : " << B[avx_supported] << std::endl;
    std::cout << ">>> POPCNT           : " << B[popcnt_supported] << std::endl;
    std::cout << ">>> RDRAND           : " << B[rdrand_supported] << std::endl;
    std::cout << ">>> AES              : " << B[aes_supported] << std::endl;
  }
}
