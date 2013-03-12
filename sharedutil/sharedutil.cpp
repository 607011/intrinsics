// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag


#ifdef WIN32
#define _CRT_RAND_S
#include <Windows.h>
#include <intrin.h>
#else
#include <unistd.h>
#include <cpuid.h>
#endif

#include "sharedutil.h"

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


CPUFeatures::CPUFeatures(void) 
  : cores(-1)
  , threads_per_package(-1)
{
  cpuid_result_t r;
#ifdef WIN32
  __cpuid(r.reg, 1);
#else
  __get_cpuid(1, &r.eax, &r.ebx, &r.ecx, &r.edx);
#endif
  cpu_type = (r.eax >> 12) & 0x3;
  cpu_family = (r.eax >> 8) & 0xf;
  cpu_ext_family = (r.eax >> 20) & 0xff;
  cpu_model = (r.eax >> 4) & 0xf;
  cpu_ext_model = (r.eax >> 16) & 0xf;
  cpu_stepping = r.eax & 0xf;
  logical_cores = (r.ebx >> 16) & 0xff;
  clflush_linesize = 8 * ((r.ebx >> 8) & 0xff);
  sse3_supported = (r.ecx & (1<<0)) != 0;
  ssse3_supported = (r.ecx & (1<<9)) != 0;
  monitor_wait_supported = (r.ecx & (1<<3)) != 0;
  vmx_supported = (r.ecx & (1<<5)) != 0;
  sse41_supported = (r.ecx & (1<<19)) != 0;
  sse42_supported = (r.ecx & (1<<20)) != 0;
  fma_supported = (r.ecx & (1<<12)) != 0;
  popcnt_supported = (r.ecx & (1<<23)) != 0;
  aes_supported = (r.ecx & (1<<25)) != 0;
  avx_supported = (r.ecx & (1<<28)) != 0;
  f16c_supported = (r.ecx & (1<<29)) != 0;
  rdrand_supported = (r.ecx & (1<<30)) != 0;
  mmx_supported = (r.edx & (1<<23)) != 0;
  sse_supported = (r.edx & (1<<25)) != 0;
  sse2_supported = (r.edx & (1<<26)) != 0;
  ht_supported = (r.edx & (1<<28)) != 0;
#ifdef WIN32
  if (isGenuineIntelCPU()) {
    __cpuid(r.reg, 4);
    cores = ((r.eax >> 26) & 0x3f) + 1;
    threads_per_package = ((r.eax >> 14) & 0xfff) + 1;
  }
  else if (isAuthenticAMDCPU()) {
    __cpuid(r.reg, 0x80000008);
    cores = (r.ecx & 0xff) + 1;
  }
#else
  if (isGenuineIntelCPU()) {
    __get_cpuid(4, &r.eax, &r.ebx, &r.ecx, &r.edx);
    cores = ((eax >> 26) & 0x3f) + 1;
    threads_per_package = ((r.eax >> 14) & 0xfff) + 1;
  }
  else if (isAuthenticAMDCPU()) {
    __get_cpuid(0x80000008, &r.eax, &r.ebx, &r.ecx, &r.edx);
    cores = ((unsigned int)(r.ecx & 0xff)) + 1;
  }
#endif
}


int CPUFeatures::getNumCores(void)
{
#ifdef WIN32
  SYSTEM_INFO pInfo;
  GetSystemInfo(&pInfo);
  return (int)pInfo.dwNumberOfProcessors;
#else
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}


std::string CPUFeatures::cpuVendor(void) {
  union {
    char str[12];
    struct {
      uint32_t reg[3];
    };
  } vendor;
  cpuid_result_t r;
#if defined(WIN32)
  __cpuid(r.reg, 0);
  vendor.reg[0] = r.ebx;
  vendor.reg[1] = r.edx;
  vendor.reg[2] = r.ecx;
#else
  __get_cpuid(0, &r.eax, &r.ebx, &r.ecx, &r.edx);
  vendor.reg[0] = r.ebx;
  vendor.reg[1] = r.edx;
  vendor.reg[2] = r.ecx;
#endif
  return std::string(vendor.str, 12);
}


bool CPUFeatures::isGenuineIntelCPU(void) {
  return cpuVendor() == "GenuineIntel";
}


bool CPUFeatures::isAuthenticAMDCPU(void) {
  return cpuVendor() == "AuthenticAMD";
}


bool CPUFeatures::isCRCSupported(void) {
  return isGenuineIntelCPU() && sse42_supported;
}


bool CPUFeatures::isRdRandSupported(void) {
  return isGenuineIntelCPU() && rdrand_supported;
}
