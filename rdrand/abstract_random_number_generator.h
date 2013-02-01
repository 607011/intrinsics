// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// Alle Rechte vorbehalten.

#pragma once

#define _CRT_RAND_S
#include <stdlib.h>
#include <Windows.h>
#include "util.h"

template <typename T>
class AbstractRandomNumberGenerator
{
public:
	AbstractRandomNumberGenerator(void) { /* ... */ }
	virtual ~AbstractRandomNumberGenerator() { /* ... */ }
	virtual T operator()(void) = 0;
	virtual T next(void) {return (*this)(); }
	virtual void next(T& dst) { dst = next(); }
	virtual void seed(T) { /* ... */ }
	virtual void seed(void) { seed(makeSeed()); }
	static T makeSeed(void);
	static const char* name(void) { return "Please overwrite AbstractRandomNumberGenerator::name() to provide a sensible name for your RNG!"; }
	static int result_size(void) { return sizeof(T); }

	typedef T result_t;
};


template <typename T>
T AbstractRandomNumberGenerator<T>::makeSeed(void)
{
	T seed = 0;
	if (isRdRandSupported()) {
		seed = (T)getRdRand32();
	}
	else if (hasRand_s()) {
		rand_s((unsigned int*)&seed);
	}
	else { 
		seed = (T)GetTickCount();
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


class DummyUIntGenerator : public UIntRandomNumberGenerator
{
public:
	DummyUIntGenerator(void) { }
	unsigned int operator()() { return 0x00000000U; }
	static const char* name(void) { return "Dummy Int"; }
};
