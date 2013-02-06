// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#include <Windows.h>
#include <tchar.h>
#include <string.h>
#include <intrin.h>


bool isGenuineIntelCPU(void) {
	int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
	__cpuid(cpureg, 0);
	return memcmp((char*)&cpureg[1], "Genu", 4) == 0
		&& memcmp((char*)&cpureg[2], "ntel", 4) == 0
		&& memcmp((char*)&cpureg[3], "ineI", 4) == 0;
}


bool isRdRandSupported(void) {
	if (!isGenuineIntelCPU())
		return false;
	int cpureg[4] = { 0x0, 0x0, 0x0, 0x0 };
	__cpuid(cpureg, 1);
	return (cpureg[2] & (1<<30)) != 0;
}


int _tmain(int argc, _TCHAR* argv[])
{
	bool supported = isRdRandSupported();
	const _TCHAR* msg = supported? TEXT("Der Prozessor unterstützt RDRAND.\n") : TEXT("Der Prozessor unterstützt RDRAND *nicht*.\n");
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), msg, lstrlen(msg), NULL, NULL);
	return supported? EXIT_SUCCESS : EXIT_FAILURE;
}
