// Copyright (c) 2010 Intel Corporation
// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __AESNI_H_
#define __AESNI_H_

#if !defined (ALIGN16)
# if defined (__GNUC__)
# define ALIGN16 __attribute__ ( (aligned (16)))
# else
# define ALIGN16 __declspec (align (16))
# endif
#endif

typedef struct KEY_SCHEDULE {
  ALIGN16 unsigned char KEY[16*15];
  unsigned int nr;
} AES_KEY;


extern "C" {
  int AES_set_encrypt_key(const unsigned char* userKey, const int bits, AES_KEY* key);
  int AES_set_decrypt_key(const unsigned char* userKey, const int bits, AES_KEY* key);
  void AES_CBC_encrypt(const unsigned char* in, unsigned char* out,
                     unsigned char ivec[16], unsigned long length,
                     unsigned char* key, int number_of_rounds);
  void AES_CBC_decrypt(const unsigned char* in, unsigned char* out,
                     unsigned char ivec[16], unsigned long length,
                     unsigned char* key, int number_of_rounds);
}

#endif // __AESNI_H_
