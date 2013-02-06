// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once

extern bool hasRand_s(void);
extern bool isGenuineIntelCPU(void);
extern bool isRdRandSupported(void);
extern unsigned __int32 getRdRand32(void);
#if defined(_M_X64)
extern unsigned __int64 getRdRand64(void);
#endif
extern void evaluateCPUFeatures(void);
extern DWORD getNumCores(void);