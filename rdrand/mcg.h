// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once

#include "abstract_random_number_generator.h"


template <unsigned __int64 R0>
class MultiplicativeCongruential : public AbstractRandomNumberGenerator<unsigned char>
{
public:
	MultiplicativeCongruential(unsigned __int64 _Seed = R0)
		: mR(_Seed)
	{ }
	unsigned char operator()() {
		mR = (mR * 16807) & 0x7fffffffULL;
		return (unsigned char) ((mR >> 11) & 0xffU);
	}
	inline void seed(unsigned __int64 _Seed) { mR = _Seed; }
	static const char* name(void) { return "MCG"; }

private:
	unsigned __int64 mR;
};

typedef MultiplicativeCongruential<1> MCG;
