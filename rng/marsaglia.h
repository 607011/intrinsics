// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// Portiert von http://www.agner.org/random/

#ifndef __INTRINSICS_MARSAGLIA_H_
#define __INTRINSICS_MARSAGLIA_H_

#include "abstract_random_number_generator.h"

class MultiplyWithCarry : public UIntRandomNumberGenerator
{
public:
	MultiplyWithCarry(unsigned int _Seed = 0x9908b0dfU) {
		seed(_Seed);
	}
	unsigned int operator()() {
		unsigned __int64 sum =
			2111111111ULL * (unsigned __int64)mR[3] +
			1492ULL       * (unsigned __int64)mR[2] +
			1776ULL       * (unsigned __int64)mR[1] +
			5115ULL       * (unsigned __int64)mR[0] +
			(unsigned __int64)mR[4];
		mR[3] = mR[2];
		mR[2] = mR[1];
		mR[1] = mR[0];
		mR[4] = (unsigned int)(sum >> 32);
		mR[0] = (unsigned int)(sum & 0xffffffffULL);
		return mR[0];
	}
	inline void seed(unsigned int _Seed) { warmup(_Seed); }
	inline void seed(void) { seed(makeSeed()); }
	static const char* name(void) { return "Marsaglia"; }

private:
	inline void warmup(unsigned int s) {
		for (int i = 0; i < 5; ++i)
			mR[i] = s = s * 29943829U - 1U;
		for (size_t i = 0; i < 19; ++i)
			(*this)();
	}
	unsigned int mR[5];
};

#endif
