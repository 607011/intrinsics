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
  ALIGN16 unsigned char rd_key[16*15];
  unsigned int rounds;
} AES_KEY;

#ifdef  __cplusplus
extern "C" {
#endif

  void AES_CBC_encrypt_Intrinsic(const unsigned char *in, unsigned char *out, size_t len, const AES_KEY *key, unsigned char *ivec, const int enc);

  // Intrinsics
  int AES_set_encrypt_key(const unsigned char* userKey, const int bits, AES_KEY* key);
  int AES_set_decrypt_key(const unsigned char* userKey, const int bits, AES_KEY* key);
  void AES_CBC_encrypt(const unsigned char* in, unsigned char* out,
    unsigned char ivec[16], unsigned long length,
    unsigned char* key, int number_of_rounds);
  void AES_CBC_decrypt(const unsigned char* in, unsigned char* out,
    unsigned char ivec[16], unsigned long length,
    unsigned char* key, int number_of_rounds);

  // OpenSSL
  typedef void (*block128_f)(const unsigned char in[16], unsigned char out[16], const void *key);

#define AES_ENCRYPT	1
#define AES_DECRYPT	0

  void CRYPTO_cbc128_encrypt(const unsigned char *in, unsigned char *out, size_t len, const void *key, unsigned char ivec[16], block128_f block);
  void CRYPTO_cbc128_decrypt(const unsigned char *in, unsigned char *out, size_t len, const void *key, unsigned char ivec[16], block128_f block);

  void AES_encrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key);
  void AES_decrypt(const unsigned char *in, unsigned char *out, const AES_KEY *key);

  void AES_CBC_encrypt_OpenSSL(const unsigned char *in, unsigned char *out, size_t len, const AES_KEY *key, unsigned char *ivec, const int enc);

  int AES_set_encrypt_key_OpenSSL(const unsigned char *userKey, const int bits, AES_KEY *key);
  int AES_set_decrypt_key_OpenSSL(const unsigned char *userKey, const int bits, AES_KEY *key);

#ifdef  __cplusplus
}
#endif

#endif // __AESNI_H_
