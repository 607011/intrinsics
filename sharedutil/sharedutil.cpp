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

#include <cstring>
#include <iostream>


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
  return (unsigned int)pInfo.dwNumberOfProcessors;
#else
  return sysconf( _SC_NPROCESSORS_ONLN );
#endif
}


bool isGenuineIntelCPU(void) {
#ifdef WIN32
  int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
  __cpuid(cpureg, 0);
  return memcmp((char*)&cpureg[1], "Genu", 4) == 0
    && memcmp((char*)&cpureg[2], "ntel", 4) == 0
    && memcmp((char*)&cpureg[3], "ineI", 4) == 0;
#else
  unsigned int eax, ebx, ecx = 0, edx;
  __get_cpuid(0, &eax, &ebx, &ecx, &edx);
  return memcmp((char*)&ebx, "Genu", 4) == 0
    && memcmp((char*)&ecx, "ntel", 4) == 0
    && memcmp((char*)&edx, "ineI", 4) == 0;
#endif
}


bool isCRCSupported(void) {
#ifdef WIN32
  int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
  __cpuid(cpureg, 1);
  return (cpureg[2] & (1<<20)) != 0;
#else
  uint32_t eax, ebx, ecx = 0, edx;
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
  uint32_t eax, ebx, ecx = 0, edx;
  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
  return (ecx & (1<<30)) != 0;
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
#ifdef WIN32
  int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
  __cpuid(cpureg, 1);
  bool sse3_supported = (cpureg[2] & (1<<0)) != 0;
  bool ssse3_supported = (cpureg[2] & (1<<9)) != 0;
  bool monitor_wait_supported = (cpureg[2] & (1<<3)) != 0;
  bool vmx_supported = (cpureg[2] & (1<<5)) != 0;
  bool sse41_supported = (cpureg[2] & (1<<19)) != 0;
  bool sse42_supported = (cpureg[2] & (1<<20)) != 0;
  bool fma_supported = (cpureg[2] & (1<<12)) != 0;
  bool popcnt_supported = (cpureg[2] & (1<<23)) != 0;
  bool aes_supported = (cpureg[2] & (1<<25)) != 0;
  bool avx_supported = (cpureg[2] & (1<<28)) != 0;
  bool f16c_supported = (cpureg[2] & (1<<29)) != 0;
  bool rdrand_supported = (cpureg[2] & (1<<30)) != 0;
  bool b31_supported = (cpureg[2] & (1<<31)) != 0;
#else
  uint32_t eax, ebx, ecx = 0, edx;
  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
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
  bool b31_supported = (ecx & (1<<31)) != 0;
#endif
  if (gVerbose > 1) {
    std::cout << ">>> #Cores      : " << getNumCores() << std::endl;
    std::cout << ">>> SSE3        : " << B[sse3_supported] << std::endl;
    std::cout << ">>> SSSE3       : " << B[ssse3_supported] << std::endl;
    std::cout << ">>> SSE4.1      : " << B[sse41_supported] << std::endl;
    std::cout << ">>> SSE4.2      : " << B[sse42_supported] << std::endl;
    std::cout << ">>> FMA         : " << B[fma_supported] << std::endl;
    std::cout << ">>> MONITOR/WAIT: " << B[monitor_wait_supported] << std::endl;
    std::cout << ">>> VMX         : " << B[vmx_supported] << std::endl;
    std::cout << ">>> POPCNT      : " << B[popcnt_supported] << std::endl;
    std::cout << ">>> AVX         : " << B[avx_supported] << std::endl;
    std::cout << ">>> AES         : " << B[aes_supported] << std::endl;
    std::cout << ">>> F16C        : " << B[f16c_supported] << std::endl;
    std::cout << ">>> RDRAND      : " << B[rdrand_supported] << std::endl;
    std::cout << ">>> 1<<31       : " << B[b31_supported] << std::endl;
  }
}
