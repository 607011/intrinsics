// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#ifndef __INTRINSICS_UTIL_H_
#define __INTRINSICS_UTIL_H_

extern int gVerbose;

#ifdef WIN32
extern bool hasRand_s(void);
#endif

extern bool isGenuineIntelCPU(void);
extern bool isRdRandSupported(void);
extern bool isCRCSupported(void);
extern unsigned __int32 getRdRand32(void);
#if defined(_M_X64)
extern unsigned __int64 getRdRand64(void);
#endif
extern void evaluateCPUFeatures(void);
extern unsigned int getNumCores(void);

#ifndef WIN32
inline  __attribute__((cdecl)) unsigned int _rdrand8_step(unsigned char* x) {
	return __builtin_ia32_rdrand8_step(x)
}
inline  __attribute__((cdecl)) unsigned int _rdrand16_step(unsigned short* x) {
	return __builtin_ia32_rdrand16_step(x)
}
inline  __attribute__((cdecl)) unsigned int _rdrand32_step(unsigned int* x) {
	return __builtin_ia32_rdrand32_step(x)
}
inline  __attribute__((cdecl)) unsigned int _rdrand64_step(unsigned long long* x) {
	return __builtin_ia32_rdrand64step(x)
}
#endif

#endif // __INTRINSICS_UTIL_H_