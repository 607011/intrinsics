// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __AESNI_H_
#define __AESNI_H_

#include "../sharedutil/sharedutil.h"

#if !defined (ALIGN16)
# if defined (__GNUC__)
#  define ALIGN16 __attribute__ ((aligned(16)))
# else
#  define ALIGN16 __declspec(align(16))
# endif
#endif

struct AES_KEY_ALIGNED {
#if 1
  AES_KEY_ALIGNED(void) 
  {
    rd_key = (unsigned char*)_aligned_malloc(15*16, 16);
  }
  ~AES_KEY_ALIGNED()
  {
    safeAlignedFree(rd_key);
  }
  ALIGN16 unsigned char* rd_key;
#else
  unsigned char rd_key[15*16];
#endif
  ALIGN16 unsigned int rounds;
};

#include <openssl/aes.h>

// Intrinsics
int AESNI_set_encrypt_key(const unsigned char* userKey, const int bits, AES_KEY_ALIGNED* key);
int AESNI_set_decrypt_key(const unsigned char* userKey, const int bits, AES_KEY_ALIGNED* key);
void AESNI_cbc_encrypt(const unsigned char* in, unsigned char* out, unsigned char ivec[16], unsigned long length, AES_KEY_ALIGNED* key);
void AESNI_cbc_decrypt(const unsigned char* in, unsigned char* out, unsigned char ivec[16], unsigned long length, AES_KEY_ALIGNED* key);

#endif // __AESNI_H_
