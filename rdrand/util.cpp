// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#include <Windows.h>
#include <intrin.h>
#include <cstring>
#include <iostream>

bool hasRand_s(void) 
{
    OSVERSIONINFO os;
    GetVersionEx(&os);
    return (os.dwMajorVersion > 5) && (os.dwMinorVersion >= 1);
}


DWORD getNumCores(void)
{
	SYSTEM_INFO pInfo;
	GetSystemInfo(&pInfo);
	return pInfo.dwNumberOfProcessors;
}


bool isGenuineIntelCPU(void) {
	int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
	__cpuid(cpureg, 0);
	return memcmp((char*)&cpureg[1], "Genu", 4) == 0
		&& memcmp((char*)&cpureg[2], "ntel", 4) == 0
		&& memcmp((char*)&cpureg[3], "ineI", 4) == 0;
}


bool isRdRandSupported(void) {
	if (!isGenuineIntelCPU())
		return false;
	int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
	__cpuid(cpureg, 1);
	return (cpureg[2] & (1<<30)) != 0;
}


unsigned __int32 getRdRand32(void)
{
	unsigned __int32 x;
	_rdrand32_step(&x);
	return x;
}


#if defined(_M_X64)
unsigned __int64 getRdRand64(void)
{
	unsigned __int64 x;
	_rdrand64_step(&x);
	return x;
}
#endif


void evaluateCPUFeatures(void) {
	extern int gVerbose;
	static const char* B[2] = { "false", "true" };
	int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
	__cpuid(cpureg, 1);
	bool sse3_supported =   (cpureg[2] & (1<<0))  != 0;
	bool sse41_supported =  (cpureg[2] & (1<<19)) != 0;
	bool sse42_supported =  (cpureg[2] & (1<<20)) != 0;
	bool popcnt_supported = (cpureg[2] & (1<<23)) != 0;
	bool aes_supported =    (cpureg[2] & (1<<25)) != 0;
	bool avx_supported =    (cpureg[2] & (1<<28)) != 0;
	bool b29_supported =    (cpureg[2] & (1<<29)) != 0;
	bool rdrand_supported = (cpureg[2] & (1<<30)) != 0;
	bool b31_supported =    (cpureg[2] & (1<<31)) != 0;
	if (gVerbose > 1) {
		std::cout << ">>> #Cores : " << getNumCores() << std::endl;
		std::cout << ">>> SSE3   : " << B[sse3_supported] << std::endl;
		std::cout << ">>> SSE4.1 : " << B[sse41_supported] << std::endl;
		std::cout << ">>> SSE4.2 : " << B[sse42_supported] << std::endl;
		std::cout << ">>> POPCNT : " << B[popcnt_supported] << std::endl;
		std::cout << ">>> AVX    : " << B[avx_supported] << std::endl;
		std::cout << ">>> AES    : " << B[aes_supported] << std::endl;
		std::cout << ">>> 1<<29  : " << B[b29_supported] << std::endl;
		std::cout << ">>> RDRAND : " << B[rdrand_supported] << std::endl;
		std::cout << ">>> 1<<31  : " << B[b31_supported] << std::endl;
	}
}
