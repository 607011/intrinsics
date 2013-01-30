// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// Portiert von http://www.agner.org/random/

#pragma once

#include "abstract_random_number_generator.h"

class MultiplyWithCarry : public AbstractRandomNumberGenerator<unsigned int>
{
public:
	MultiplyWithCarry(unsigned int _Seed = 0x9908b0df) {
		seed(_Seed);
	}
	unsigned int operator()() {
		unsigned long long sum =
			2111111111ULL * (unsigned long long)mR[3] +
			1492ULL       * (unsigned long long)mR[2] +
			1776ULL       * (unsigned long long)mR[1] +
			5115ULL       * (unsigned long long)mR[0] +
			(unsigned long long)mR[4];
		mR[3] = mR[2];
		mR[2] = mR[1];
		mR[1] = mR[0];
		mR[4] = (unsigned int)(sum >> 32);
		mR[0] = (unsigned int)(sum & 0xffffffffLL);
		return mR[0];
	}
	inline void seed(unsigned int _Seed) {
		warmup(_Seed);
	}
	static const char* name(void) { return "Marsaglia"; }

private:
	inline void warmup(unsigned int s) {
		for (size_t i = 0; i < 5; ++i) {
			s = s * 29943829 - 1;
			mR[i] = s;
		}
		for (size_t i = 0; i < 19; ++i)
			(*this)();
	}
	unsigned int mR[5];
};

