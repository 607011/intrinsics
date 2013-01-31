// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once

#include "abstract_random_number_generator.h"

template <typename T, T M, T X0>
class Circular : public AbstractRandomNumberGenerator<T>
{
public:
	Circular(T m = M, T seed = X0)
		: mM(m)
		, mR(seed)
	{ }
	T operator()() { return mR++ % mM; }
	inline void seed(T _Seed) { mR = _Seed; }
	static const char* name(void) { return "CircularBytes"; }

private:
	T mM;
	T mR;
};

typedef Circular<unsigned char, 255, 0> CircularBytes;
