// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#include "mersenne_twister.h"


const unsigned int MersenneTwister::A[2] = { 0, 0x9908b0df };


void MersenneTwister::seed(unsigned int _Seed)
{
	unsigned int r = _Seed;
	unsigned int s = 3402;
	for (int i = 0; i < N; ++i)
	{
		r = 509845221 * r + 3;
		s *= s + 1;
		mY[i] = s + (r >> 10);
	}
	mIndex = 0;
	warmup();
}


void MersenneTwister::warmup(void)
{
	for (int i = 0; i < 10000; ++i)
		(*this)();
}


unsigned int MersenneTwister::operator()()
{
	if (mIndex == N) {
		unsigned int h;
		for (int k = 0 ; k < N-M ; ++k) {
			h = (mY[k] & HI) | (mY[k+1] & LO);
			mY[k] = mY[k+M] ^ (h >> 1) ^ A[h & 1];
		}
		for (int k = N-M ; k < N-1 ; ++k) {
			h = (mY[k] & HI) | (mY[k+1] & LO);
			mY[k] = mY[k+(M-N)] ^ (h >> 1) ^ A[h & 1];
		}
		h = (mY[N-1] & HI) | (mY[0] & LO);
		mY[N-1] = mY[M-1] ^ (h >> 1) ^ A[h & 1];
		mIndex = 0;
	}

	unsigned int e = mY[mIndex++];
	e ^= (e >> 11);
	e ^= (e << 7) & 0x9d2c5680;
	e ^= (e << 15) & 0xefc60000;
	e ^= (e >> 18);
	return e;
}
