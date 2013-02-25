// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once

#include <Windows.h>
#include <immintrin.h>

class Stopwatch {
public:
	inline Stopwatch(__int64& dTime, __int64& dTicks)
		: mT(dTime)
		, mTicks(dTicks)
	{
		mT = INVALID;
		mTicks = INVALID;
		QueryPerformanceCounter(&mPC0);
		mTicks0 = (__int64)__rdtsc();
	}
	inline ~Stopwatch()
	{
		stop();
	}
	inline void stop(void)
	{
		LARGE_INTEGER pc, freq;
		mTicks = (__int64)__rdtsc() - mTicks0;
		QueryPerformanceCounter(&pc);
		QueryPerformanceFrequency(&freq);
		mT = 1000 * (pc.QuadPart - mPC0.QuadPart) / freq.QuadPart;
	}
	static const __int64 INVALID = MINLONGLONG;

private:
	__int64& mT;
	__int64& mTicks;
	LARGE_INTEGER mPC0;
	__int64 mTicks0;
};

