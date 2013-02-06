// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once

#include <immintrin.h>
#include "abstract_random_number_generator.h"


class RdRand16 : public AbstractRandomNumberGenerator<unsigned __int16>
{
public:
	RdRand16(void) { /* ... */ }
	inline unsigned short operator()() {
		unsigned short x;
		_rdrand16_step(&x);
		return x;
	}
	inline void next(unsigned short& r) {
		int tries = RETRY_LIMIT;
		do {
			if (_rdrand16_step(&r))
				return;
			++mInvalid;
		}
		while (tries--);
		++mLimitExceeded;
	}
	static const char* name(void) { return "rdrand16"; }
};


class RdRand32 : public AbstractRandomNumberGenerator<unsigned __int32>
{
public:
	RdRand32(void) { /* ... */ }
	inline unsigned int operator()() {
		unsigned int x;
		_rdrand32_step(&x);
		return x;
	}
	inline void next(unsigned int& r) {
		int tries = RETRY_LIMIT;
		do {
			if (_rdrand32_step(&r)) 
				return;
			++mInvalid;
		}
		while (tries--);
		++mLimitExceeded;
	}
	static const char* name(void) { return "rdrand32"; }
};


#if defined(_M_X64)
class RdRand64 : public AbstractRandomNumberGenerator<unsigned __int64>
{
public:
	RdRand64(void) { /* ... */ }
	inline unsigned __int64 operator()() {
		unsigned __int64 x;
		_rdrand64_step(&x);
		return x;
	}
	inline void next(unsigned __int64& r) {
		int tries = RETRY_LIMIT;
		do {
			if (_rdrand64_step(&r))
				return;
			++mInvalid;
		}
		while (tries--);
		++mLimitExceeded;
	}
	static const char* name(void) { return "rdrand64"; }
};
#endif
