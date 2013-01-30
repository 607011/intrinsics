// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// Alle Rechte vorbehalten.

#pragma once

#define _CRT_RAND_S
#include <stdlib.h>
#include <Windows.h>


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
	static const char* name(void) { return "<invalid>"; }
	static int result_size(void) { return sizeof(VariateType); }

	typedef VariateType result_t;
};


template <typename VariateType>
VariateType AbstractRandomNumberGenerator<VariateType>::makeSeed(void)
{
	VariateType seed = 0;
	if (hasRand_s()) {
		rand_s((unsigned int*)&seed);
	}
	else { 
		seed = (VariateType)GetTickCount();
	}
	return seed;
}


typedef AbstractRandomNumberGenerator<unsigned __int64> UInt64RandomNumberGenerator;
typedef AbstractRandomNumberGenerator<unsigned int> UIntRandomNumberGenerator;


class DummyGenerator : public AbstractRandomNumberGenerator<unsigned char>
{
public:
	DummyGenerator(void) { }
	unsigned char operator()() { return 0x00U; }
	static const char* name(void) { return "Dummy"; }
};
