// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// Alle Rechte vorbehalten.

#pragma once

#define _CRT_RAND_S
#include <stdlib.h>
#include <Windows.h>
#include "util.h"

template <typename VariateType>
class AbstractRandomNumberGenerator
{
public:
	AbstractRandomNumberGenerator(void) { /* ... */ }
	virtual ~AbstractRandomNumberGenerator() { /* ... */ }
	virtual VariateType operator()(void) = 0;
	virtual VariateType next(void) {return (*this)(); }
	virtual void next(VariateType& dst) { dst = next(); }
	virtual void seed(VariateType) { /* ... */ }
	virtual int size(void) const { return sizeof(VariateType); }
	static VariateType makeSeed(void);
	static const char* name(void) { return "Please overwrite AbstractRandomNumberGenerator::name() to provide a sensible name for your RNG!"; }
	static int result_size(void) { return sizeof(VariateType); }

	typedef VariateType result_t;
};


template <typename VariateType>
VariateType AbstractRandomNumberGenerator<VariateType>::makeSeed(void)
{
	VariateType seed = 0;
	if (isRdRandSupported()) {
		seed = (VariateType)getRdRand32();
	}
	else if (hasRand_s()) {
		rand_s((unsigned int*)&seed);
	}
	else { 
		seed = (VariateType)GetTickCount();
	}
	return seed;
}


typedef AbstractRandomNumberGenerator<unsigned __int64> UInt64RandomNumberGenerator;
typedef AbstractRandomNumberGenerator<unsigned int> UIntRandomNumberGenerator;
typedef AbstractRandomNumberGenerator<unsigned char> ByteRandomNumberGenerator;


class DummyByteGenerator : public ByteRandomNumberGenerator
{
public:
	DummyByteGenerator(void) { }
	unsigned char operator()() { return 0x00U; }
	static const char* name(void) { return "Dummy Byte"; }
};


class DummyIntGenerator : public UIntRandomNumberGenerator
{
public:
	DummyIntGenerator(void) { }
	unsigned int operator()() { return 0x00000000U; }
	static const char* name(void) { return "Dummy Int"; }
};
