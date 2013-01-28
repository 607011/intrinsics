// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once

#include <immintrin.h>
#include "abstract_random_number_generator.h"

class RdRand32 : public AbstractRandomNumberGenerator<unsigned int>
{
public:
	RdRand32(void) { }
	inline unsigned int operator()() {
		unsigned int x;
		_rdrand32_step(&x);
		return x;
	}
	inline void next(unsigned int& r) {
		_rdrand32_step(&r);
	}
};


class RdRand16 : public AbstractRandomNumberGenerator<unsigned short>
{
public:
	RdRand16(void) { }
	inline unsigned short operator()() {
		unsigned short x;
		_rdrand16_step(&x);
		return x;
	}
	inline void next(unsigned short& r) {
		_rdrand16_step(&r);
	}
};
