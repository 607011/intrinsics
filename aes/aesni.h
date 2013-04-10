// Copyright (c) 2010, Intel Corporation
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

extern "C" {
  void AES_128_Key_Expansion(const unsigned char *userkey, unsigned char *key);
#if 0
  void AES_192_Key_Expansion(const unsigned char *userkey, unsigned char *key);
#endif // 0
  void AES_256_Key_Expansion(const unsigned char *userkey, unsigned char *key);

  void AES_CBC_encrypt(const unsigned char *in,
                     unsigned char *out,
                     unsigned char ivec[16],
                     unsigned long length,
                     unsigned char *key,
                     int number_of_rounds);
  void AES_CBC_decrypt(const unsigned char *in,
                     unsigned char *out,
                     unsigned char ivec[16],
                     unsigned long length,
                     unsigned char *key,
                     int number_of_rounds);
}

#endif // __AESNI_H_
