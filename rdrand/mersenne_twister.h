// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once

#include "abstract_random_number_generator.h"

class MersenneTwister : public UIntRandomNumberGenerator
{
public:
	MersenneTwister(void) { /* ... */ }
	unsigned int operator()();
	void seed(unsigned int _Seed = 9);
	static const char* name(void) { return "Mersenne-Twister"; }

private:
	static const int N = 624;
	static const int M = 397;
	static const unsigned int LO = 0x7fffffff;
	static const unsigned int HI = 0x80000000;
	static const unsigned int A[2];
	unsigned int mY[N];
	int mIndex;

private: // methods
	void warmup(void);
};

