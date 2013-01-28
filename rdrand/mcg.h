// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once

#include "abstract_random_number_generator.h"


template <unsigned long long R0>
class MultiplicativeCongruential : public AbstractRandomNumberGenerator<unsigned char>
{
public:
	MultiplicativeCongruential(unsigned long long _Seed = R0)
		: mR(_Seed)
	{ }
	unsigned char operator()() {
		mR = (mR * 16807) & 0x7fffffffULL;
		return (unsigned char) ((mR >> 11) & 0xff);
	}
	inline void seed(unsigned long long _Seed) { mR = _Seed; }

private:
	unsigned long long mR;
};

typedef MultiplicativeCongruential<1> MCG;
