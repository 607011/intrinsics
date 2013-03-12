// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#if defined(WIN32)
#include <Windows.h>
#endif

#include <stdlib.h>
#include <iostream>

#include "sharedutil.h"

int main(int argc, char* argv[])
{
  CPUFeatures cpuFeatures;
  const bool supported = cpuFeatures.isRdRandSupported();
  const char* msg = supported? "Der Prozessor unterstuetzt RDRAND.\n" : "Der Prozessor unterstuetzt RDRAND *nicht*.\n";
  std::cout <<  msg << std::endl;
  return supported? EXIT_SUCCESS : EXIT_FAILURE;
}
