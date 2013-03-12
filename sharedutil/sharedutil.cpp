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
  return (unsigned int)pInfo.dwNumberOfProcessors;
#else
  return sysconf( _SC_NPROCESSORS_ONLN );
#endif
}


std::string cpuVendor(void) {
  char vendor[12];
#if defined(WIN32)
  int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
  __cpuid(cpureg, 0);
  ((unsigned int*)vendor)[0] = cpureg[1]; // EBX
  ((unsigned int*)vendor)[1] = cpureg[3]; // EDX
  ((unsigned int*)vendor)[2] = cpureg[2]; // ECX
#else
  unsigned int eax, ebx, ecx, edx;
  __get_cpuid(0, &eax, &ebx, &ecx, &edx);
  ((unsigned int*)vendor)[0] = ebx;
  ((unsigned int*)vendor)[1] = edx;
  ((unsigned int*)vendor)[2] = ecx;
#endif
  return std::string(vendor, 12);
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


int clFlushLineSize(void) {
  if (!isGenuineIntelCPU())
    return -1;
#ifdef WIN32
  int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
  __cpuid(cpureg, 1);
  return 8 * ((cpureg[1] >> 8) & 0xff);
#else
  uint32_t eax, ebx, ecx = 0, edx;
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
#ifdef WIN32
  int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
  __cpuid(cpureg, 1);
  // EAX
  int cpu_type = (cpureg[0] >> 12) & 0x3;
  int cpu_family = (cpureg[0] >> 8) & 0xf;
  int cpu_ext_family = (cpureg[0] >> 20) & 0xff;
  int cpu_model = (cpureg[0] >> 4) & 0xf;
  int cpu_ext_model = (cpureg[0] >> 16) & 0xf;
  int cpu_stepping = cpureg[0] & 0xf;
  // EBX
  int logicalCores = (cpureg[1] >> 16) & 0xff;
  // ECX
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
  // EDX
  bool mmx_supported = (cpureg[3] & (1<<23)) != 0;
  bool sse_supported = (cpureg[3] & (1<<25)) != 0;
  bool sse2_supported = (cpureg[3] & (1<<26)) != 0;
  bool ht_supported = (cpureg[3] & (1<<28)) != 0;
  if (isGenuineIntelCPU()) {
    // Get DCP cache info
    __cpuid(cpureg, 4);
    cores = ((cpureg[0] >> 26) & 0x3f) + 1; // EAX[31:26] + 1

  }
  else if (isAuthenticAMDCPU()) {
    // Get NC: Number of CPU cores - 1
    __cpuid(cpureg, 0x80000008);
    cores = ((unsigned int)(cpureg[2] & 0xff)) + 1; // ECX[7:0] + 1
  }
#else
  uint32_t eax, ebx, ecx = 0, edx;
  __get_cpuid(1, &eax, &ebx, &ecx, &edx);
  int cpu_type = (eax] >> 12) & 0x3;
  int cpu_family = (eax] >> 8) & 0xf;
  int cpu_ext_family = (eax >> 20) & 0xff;
  int cpu_model = (eax >> 4) & 0xf;
  int cpu_ext_model = (eax >> 16) & 0xf;
  int cpu_stepping = eax & 0xf;
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
  bool mmx_supported = (edx & (1<<23)) != 0;
  bool sse_supported = (edx & (1<<25)) != 0;
  bool sse2_supported = (edx & (1<<26)) != 0;
  bool ht_supported = (edx & (1<<28)) != 0;
#endif
  if (gVerbose > 1) {
    std::cout << ">>> CPU vendor       : " << cpuVendor() << std::endl;
    std::cout << ">>> #cores           : " << getNumCores() << std::endl;
    std::cout << ">>> #logical cores   : " << logicalCores << std::endl;
    std::cout << ">>> #cores           : " << cores << std::endl;
    std::cout << ">>> CPU type         : " << cpu_type << std::endl;
    std::cout << ">>> CPU family       : " << cpu_family << std::endl;
    std::cout << ">>> CPU ext family   : " << cpu_ext_family << std::endl;
    std::cout << ">>> CPU model        : " << cpu_model << std::endl;
    std::cout << ">>> CPU ext model    : " << cpu_ext_model << std::endl;
    std::cout << ">>> CPU stepping     : " << cpu_stepping << std::endl;
    std::cout << ">>> Hyper-Threading  : " << B[ht_supported] << std::endl;
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
