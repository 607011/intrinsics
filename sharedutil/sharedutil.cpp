// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#if defined(WIN32)
#define _CRT_RAND_S
#include <Windows.h>
#include <intrin.h>
#include <malloc.h>
#elif (__GNUC__)
#include <unistd.h>
#include <cpuid.h>
#endif

#include "sharedutil.h"

#include <iostream>
#include <iomanip>

#if defined(WIN32)
bool hasRand_s(void) 
{
  OSVERSIONINFO os;
  GetVersionEx(&os);
  return (os.dwMajorVersion > 5) && (os.dwMinorVersion >= 1);
}
#endif


CPUFeatures::CPUFeatures(void) 
  : cores(0)
  , threads_per_package(0)
  , logical_threads_apic_id_0bh(0)
  , logical_threads_0bh(0)
  , logical_cores_apic_id_0bh(0)
  , logical_cores_0bh(0)
{
  cpuid_result_t r;

#if defined(WIN32)
  __cpuid(r.reg, 0);
#elif (__GNUC__)
  __get_cpuid(0, &r.eax, &r.ebx, &r.ecx, &r.edx);
#endif
  max_func = r.eax;
  
  detectTopology();
  detectFeatures();
}


// http://wiki.osdev.org/Detecting_CPU_Topology_%2880x86%29
void CPUFeatures::detectTopology(void)
{
  cpuid_result_t r;

#if defined(WIN32)
  __cpuid(r.reg, 1);
#elif (__GNUC__)
  __get_cpuid(1, &r.eax, &r.ebx, &r.ecx, &r.edx);
#endif
  htt_supported = (r.edx & (1<<28)) != 0;
  if (!htt_supported) {
    cores = 1;
    threads_per_package = 1;
    return;
  }

  if (max_func >= 0x0000000b) {
    if (isGenuineIntelCPU()) {
#if defined(WIN32)
      __cpuidex(r.reg, 0x0000000b, 0);
#elif defined(__GNUC__)
      __get_cpuidex(0x0000000b, 0, &r.eax, &r.ebx, &r.ecx, &r.edx);
#endif
      if (((r.ecx >> 8) & 0x0f) == 1 /* thread level */) {
        logical_threads_apic_id_0bh = (r.eax & 0x07);
        logical_threads_0bh = (r.ebx & 0xffff);
      }
    }

#if defined(WIN32)
    __cpuidex(r.reg, 0x0000000b, 1);
#elif defined(__GNUC__)
    __get_cpuidex(0x0000000b, 1, &r.eax, &r.ebx, &r.ecx, &r.edx);
#endif
    if (((r.ecx >> 8) & 0x0f) == 2 /* core level */) {
      logical_cores_apic_id_0bh = (r.eax & 0x07);
      logical_cores_0bh = (r.ebx & 0xffff);
    }

    if (logical_cores_0bh > 0 && logical_threads_0bh > 0) {
      cores = logical_cores_0bh;
      threads_per_package = logical_threads_0bh;
      return;
    }
  }

#if defined(WIN32)
  if (isGenuineIntelCPU()) {
    __cpuid(r.reg, 4);
    cores = ((r.eax >> 26) & 0x3f) + 1; 
    threads_per_package = ((r.eax >> 14) & 0xfff) + 1;
  }
  else if (isAuthenticAMDCPU()) {
    __cpuid(r.reg, 0x80000008);
    cores = (r.ecx & 0xff) + 1;
  }
#elif (__GNUC__)
  if (isGenuineIntelCPU()) {
    __get_cpuid(4, &r.eax, &r.ebx, &r.ecx, &r.edx);
    cores = ((r.eax >> 26) & 0x3f) + 1;
    threads_per_package = ((r.eax >> 14) & 0xfff) + 1;
  }
  else if (isAuthenticAMDCPU()) {
    __get_cpuid(0x80000008, &r.eax, &r.ebx, &r.ecx, &r.edx);
    cores = ((unsigned int)(r.ecx & 0xff)) + 1;
  }
#endif
}


void CPUFeatures::detectFeatures(void)
{
  cpuid_result_t r;

#if defined(WIN32)
  __cpuid(r.reg, 1);
#elif (__GNUC__)
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
  ht_supported = (threads_per_package > 1) && htt_supported;
}


int CPUFeatures::getNumCores(void)
{
#ifdef WIN32
  SYSTEM_INFO pInfo = {{0}};
  GetSystemInfo(&pInfo);
  return (int)pInfo.dwNumberOfProcessors;
#else
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}


#if defined(WIN32) && !defined(HAVE_POPCNT)
int POPCNT(ULONG_PTR x)
{
  int i = 0;
  while (x) {
    x = x & (x - 1);
    ++i;
  }
  return i;
}
#endif


int CPUFeatures::count(int& numaNodeCount, int& processorCoreCount, int& logicalProcessorCount, int& processorPackageCount)
{
  numaNodeCount = 0;
  processorCoreCount = 0;
  logicalProcessorCount = 0;
  processorPackageCount = 0;
#if defined(WIN32)
  // source from http://msdn.microsoft.com/en-us/library/windows/desktop/ms683194(v=vs.85).aspx
  typedef BOOL (WINAPI *LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
  LPFN_GLPI glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation");
  if (glpi == NULL)
    return -1;
  SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer = NULL;
  DWORD len = 0;
  BOOL done = FALSE;
  while (!done) {
    BOOL ok = glpi(buffer, &len);
    if (!ok) {
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        if (buffer) 
          free(buffer);
        buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(len);
        if (buffer == NULL)
          return -2;
      } 
      else
        return -3;
    } 
    else
      done = TRUE;
  }
  SYSTEM_LOGICAL_PROCESSOR_INFORMATION* pBuf = buffer;
  DWORD byteOffset = 0;
  while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= len) {
    switch (pBuf->Relationship) {
    case RelationNumaNode:
      ++numaNodeCount;
      break;
    case RelationProcessorCore:
      ++processorCoreCount;
      logicalProcessorCount += POPCNT(pBuf->ProcessorMask);
      break;
    case RelationProcessorPackage:
      ++processorPackageCount;
      break;
    }
    byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    ++pBuf;
  }
  free(buffer);
#endif  
  return logicalProcessorCount;
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
