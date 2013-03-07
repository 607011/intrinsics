// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __INTRINSICS_MCG_H_
#define __INTRINSICS_MCG_H_

#include "abstract_random_number_generator.h"


template <uint64_t R0>
class MultiplicativeCongruential : public ByteRandomNumberGenerator
{
public:
	MultiplicativeCongruential(uint64_t _Seed = R0)
		: mR(_Seed)
	{ }
	uint8_t operator()() {
		mR = (mR * 16807) & 0x7fffffffULL;
		return (uint8_t) ((mR >> 11) & 0xffU);
	}
	inline void seed(uint64_t _Seed) { mR = _Seed; }
	inline void seed(void) { seed(makeSeed()); }
	static const char* name(void) { return "MCG"; }

private:
	uint64_t mR;
};

typedef MultiplicativeCongruential<1> MCG;

#endif
