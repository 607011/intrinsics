// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// Alle Rechte vorbehalten.

#pragma once

#define _CRT_RAND_S
#include <Windows.h>

#include <limits>

template <typename VariateType>
class AbstractRandomNumberGenerator
{
public:
	AbstractRandomNumberGenerator(void) {}
	virtual ~AbstractRandomNumberGenerator() {}
	virtual VariateType operator()(void) = 0;
	virtual VariateType next(void) {return (*this)(); }
	virtual void next(VariateType& dst) { dst = next(); }
	virtual void seed(VariateType) { }
	virtual int size(void) const { return sizeof(VariateType); }
	static VariateType makeSeed(void);
};


template <typename VariateType>
VariateType AbstractRandomNumberGenerator<VariateType>::makeSeed(void)
{
	VariateType seed = 0;
	if (hasRand_s()) {
		seed = 0;
		rand_s((unsigned int*)&seed);
	}
	else { 
		seed = GetTickCount();
	}
	return seed;
}


typedef AbstractRandomNumberGenerator<unsigned long long> UInt64RandomNumberGenerator;
typedef AbstractRandomNumberGenerator<unsigned int> UIntRandomNumberGenerator;


class DummyGenerator : public AbstractRandomNumberGenerator<unsigned char>
{
public:
	DummyGenerator(void) { }
	unsigned char operator()() { return 0; }
};
