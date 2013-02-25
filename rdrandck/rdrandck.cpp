// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#include <Windows.h>
#include <tchar.h>
#include <string.h>
#include <intrin.h>
#include <util.h>

int _tmain(int argc, _TCHAR* argv[])
{
	bool supported = isRdRandSupported();
	const _TCHAR* msg = supported? TEXT("Der Prozessor unterst�tzt RDRAND.\n") : TEXT("Der Prozessor unterst�tzt RDRAND *nicht*.\n");
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), msg, lstrlen(msg), NULL, NULL);
	return supported? EXIT_SUCCESS : EXIT_FAILURE;
}
