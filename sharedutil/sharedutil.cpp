// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#if defined(WIN32)
#define _CRT_RAND_S
#include <Windows.h>
#endif

#include "sharedutil.h"

#if defined(WIN32)
bool hasRand_s(void) 
{
  OSVERSIONINFO os;
  GetVersionEx(&os);
  return (os.dwMajorVersion > 5) && (os.dwMinorVersion >= 1);
}
#endif
