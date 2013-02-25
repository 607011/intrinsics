// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#include <Windows.h>
#include <iostream>
#include <util.h>

int main(int argc, char* argv[])
{
	(void)argc;
	(void)argv;
	gVerbose = 2;
	evaluateCPUFeatures();

	if (!isCRCSupported()) {
		std::cout << "CRC wird nicht unterstützt." << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
