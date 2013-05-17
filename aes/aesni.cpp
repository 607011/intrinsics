// Copyright (c) 2010 Intel Corporation
// Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#include <wmmintrin.h>
#include <stdint.h>
#include <assert.h>
#include "aesni.h"

#ifndef NULL
#define NULL (0)
#endif

inline __m128i AES_128_ASSIST(__m128i temp1, __m128i temp2)
{
  __m128i temp3;
  temp2 = _mm_shuffle_epi32(temp2 ,0xff);
  temp3 = _mm_slli_si128(temp1, 0x4);
  temp1 = _mm_xor_si128(temp1, temp3);
  temp3 = _mm_slli_si128(temp3, 0x4);
  temp1 = _mm_xor_si128(temp1, temp3);
  temp3 = _mm_slli_si128(temp3, 0x4);
  temp1 = _mm_xor_si128(temp1, temp3);
  temp1 = _mm_xor_si128(temp1, temp2);
  return temp1;
}

void AES_128_Key_Expansion(const unsigned char* userkey,  unsigned char* key)
{
  assert(userkey != NULL);
  assert(key != NULL);
  __m128i temp1, temp2;
  __m128i *Key_Schedule = (__m128i*)key;
  temp1 = _mm_loadu_si128((__m128i*)userkey);
  Key_Schedule[0] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1 ,0x1);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[1] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1,0x2);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[2] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1,0x4);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[3] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1,0x8);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[4] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1,0x10);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[5] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1,0x20);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[6] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1,0x40);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[7] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1,0x80);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[8] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1,0x1b);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[9] = temp1;
  temp2 = _mm_aeskeygenassist_si128(temp1,0x36);
  temp1 = AES_128_ASSIST(temp1, temp2);
  Key_Schedule[10] = temp1;
}

inline void KEY_192_ASSIST(__m128i* temp1, __m128i* temp2, __m128i* temp3)
{
  assert(temp1 != NULL);
  assert(temp2 != NULL);
  assert(temp3 != NULL);
  __m128i temp4;
  *temp2 = _mm_shuffle_epi32(*temp2, 0x55);
  temp4 = _mm_slli_si128(*temp1, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  *temp1 = _mm_xor_si128(*temp1, *temp2);
  *temp2 = _mm_shuffle_epi32(*temp1, 0xff);
  temp4 = _mm_slli_si128(*temp3, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  *temp3 = _mm_xor_si128(*temp3, *temp2);
}

void AES_192_Key_Expansion(const unsigned char* userkey, unsigned char* key)
{
  assert(userkey != NULL);
  assert(key != NULL);
#if defined(USE_AES_192)
  __m128i temp1, temp2, temp3, temp4;
  __m128i *Key_Schedule = (__m128i*)key;
  temp1 = _mm_loadu_si128((__m128i*)userkey);
  temp3 = _mm_loadu_si128((__m128i*)(userkey+16));
  Key_Schedule[0] = temp1;
  Key_Schedule[1] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x1);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[1] = (__m128i)_mm_shuffle_pd((__m128d)Key_Schedule[1], (__m128d)temp1, 0);
  Key_Schedule[2] = (__m128i)_mm_shuffle_pd((__m128d)temp1, (__m128d)temp3, 1);
  temp2 = _mm_aeskeygenassist_si128(temp3,0x2);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[3] = temp1;
  Key_Schedule[4] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3,0x4);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[4] = (__m128i)_mm_shuffle_pd((__m128d)Key_Schedule[4], (__m128d)temp1, 0);
  Key_Schedule[5] = (__m128i)_mm_shuffle_pd((__m128d)temp1, (__m128d)temp3, 1);
  temp2 = _mm_aeskeygenassist_si128(temp3,0x8);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[6]=temp1;
  Key_Schedule[7]=temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3,0x10);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[7] = (__m128i)_mm_shuffle_pd((__m128d)Key_Schedule[7], (__m128d)temp1, 0);
  Key_Schedule[8] = (__m128i)_mm_shuffle_pd((__m128d)temp1, (__m128d)temp3, 1);
  temp2 = _mm_aeskeygenassist_si128 (temp3,0x20);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[9]=temp1;
  Key_Schedule[10]=temp3;
  temp2 = _mm_aeskeygenassist_si128 (temp3,0x40);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[10] = (__m128i)_mm_shuffle_pd((__m128d)Key_Schedule[10], (__m128d)temp1, 0);
  Key_Schedule[11] = (__m128i)_mm_shuffle_pd((__m128d)temp1, (__m128d)temp3, 1);
  temp2 = _mm_aeskeygenassist_si128 (temp3,0x80);
  KEY_192_ASSIST(&temp1, &temp2, &temp3);
  Key_Schedule[12] = temp1;
#else
  (void)userkey;
  (void)key;
#endif
}

inline void KEY_256_ASSIST_1(__m128i* temp1, __m128i* temp2)
{
  assert(temp1 != NULL);
  assert(temp2 != NULL);
  __m128i temp4;
  *temp2 = _mm_shuffle_epi32(*temp2, 0xff);
  temp4 = _mm_slli_si128(*temp1, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp1 = _mm_xor_si128(*temp1, temp4);
  *temp1 = _mm_xor_si128(*temp1, *temp2);
}

inline void KEY_256_ASSIST_2(__m128i* temp1, __m128i* temp3)
{
  assert(temp1 != NULL);
  assert(temp3 != NULL);
  __m128i temp2, temp4;
  temp4 = _mm_aeskeygenassist_si128(*temp1, 0x0);
  temp2 = _mm_shuffle_epi32(temp4, 0xaa);
  temp4 = _mm_slli_si128(*temp3, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  temp4 = _mm_slli_si128(temp4, 0x4);
  *temp3 = _mm_xor_si128(*temp3, temp4);
  *temp3 = _mm_xor_si128(*temp3, temp2);
}

void AES_256_Key_Expansion (const unsigned char* userkey, unsigned char* key)
{
  assert(userkey != NULL);
  assert(key != NULL);
  __m128i temp1, temp2, temp3;
  __m128i *Key_Schedule = (__m128i*)key;
  temp1 = _mm_loadu_si128((__m128i*)userkey);
  temp3 = _mm_loadu_si128((__m128i*)(userkey+16));
  Key_Schedule[0] = temp1;
  Key_Schedule[1] = temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x01);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[2]=temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[3]=temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x02);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[4]=temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[5]=temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x04);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[6]=temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[7]=temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x08);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[8]=temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[9]=temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x10);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[10]=temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[11]=temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x20);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[12]=temp1;
  KEY_256_ASSIST_2(&temp1, &temp3);
  Key_Schedule[13]=temp3;
  temp2 = _mm_aeskeygenassist_si128(temp3, 0x40);
  KEY_256_ASSIST_1(&temp1, &temp2);
  Key_Schedule[14]=temp1;
}

int AESNI_set_encrypt_key(const unsigned char* userKey, const int bits, AES_KEY_ALIGNED* key)
{
  assert(userKey != NULL);
  assert(key != NULL);
  switch (bits) {
  case 128:
    AES_128_Key_Expansion(userKey, (unsigned char*)key->rd_key);
    key->rounds = 10;
    return 0;
  case 192:
    AES_192_Key_Expansion(userKey, (unsigned char*)key->rd_key);
    key->rounds = 12;
    return 0;
  case 256:
    AES_256_Key_Expansion(userKey, (unsigned char*)key->rd_key);
    key->rounds = 14;
    return 0;
  default:
    break;
  }
  return -2;
}

int AESNI_set_decrypt_key(const unsigned char *userKey, const int bits, AES_KEY_ALIGNED *key)
{
  AES_KEY_ALIGNED temp_key;
  __m128i *Key_Schedule = (__m128i*)key->rd_key;
  __m128i *Temp_Key_Schedule = (__m128i*)temp_key.rd_key;
  if (!userKey || !key)
    return -1;
  if (AESNI_set_encrypt_key(userKey, bits, &temp_key) == -2)
    return -2;
  int nr = temp_key.rounds;
  key->rounds = nr;
  Key_Schedule[nr] = Temp_Key_Schedule[0];
  Key_Schedule[nr-1] = _mm_aesimc_si128(Temp_Key_Schedule[1]);
  Key_Schedule[nr-2] = _mm_aesimc_si128(Temp_Key_Schedule[2]);
  Key_Schedule[nr-3] = _mm_aesimc_si128(Temp_Key_Schedule[3]);
  Key_Schedule[nr-4] = _mm_aesimc_si128(Temp_Key_Schedule[4]);
  Key_Schedule[nr-5] = _mm_aesimc_si128(Temp_Key_Schedule[5]);
  Key_Schedule[nr-6] = _mm_aesimc_si128(Temp_Key_Schedule[6]);
  Key_Schedule[nr-7] = _mm_aesimc_si128(Temp_Key_Schedule[7]);
  Key_Schedule[nr-8] = _mm_aesimc_si128(Temp_Key_Schedule[8]);
  Key_Schedule[nr-9] = _mm_aesimc_si128(Temp_Key_Schedule[9]);
  if (nr > 10) {
    Key_Schedule[nr-10] = _mm_aesimc_si128(Temp_Key_Schedule[10]);
    Key_Schedule[nr-11] = _mm_aesimc_si128(Temp_Key_Schedule[11]);
  }
  if (nr > 12) {
    Key_Schedule[nr-12] = _mm_aesimc_si128(Temp_Key_Schedule[12]);
    Key_Schedule[nr-13] = _mm_aesimc_si128(Temp_Key_Schedule[13]);
  }
  Key_Schedule[0] = Temp_Key_Schedule[nr];
  return 0;
}

void AESNI_cbc_encrypt(const unsigned char* in, unsigned char* out,
                       unsigned char ivec[16], unsigned long length,
                       AES_KEY_ALIGNED* key)
{
  assert(key->rounds == 10 || key->rounds == 12 || key->rounds == 14);
  length = (length % 16)? length / 16 + 1 : length / 16;
  __m128i* plain = (__m128i*)in;
  const __m128i* const plainEnd = plain + length;
  __m128i* enc = (__m128i*)out;
  const __m128i* const k = (__m128i*)key->rd_key;
  __m128i feedback = _mm_loadu_si128((__m128i*)ivec);
  switch (key->rounds) {
  case 10:
    while (plain < plainEnd) {
      __m128i data = _mm_loadu_si128(plain++);
      feedback = _mm_xor_si128(data, feedback);
      feedback = _mm_xor_si128(feedback, k[0]);
      feedback = _mm_aesenc_si128(feedback, k[1]);
      feedback = _mm_aesenc_si128(feedback, k[2]);
      feedback = _mm_aesenc_si128(feedback, k[3]);
      feedback = _mm_aesenc_si128(feedback, k[4]);
      feedback = _mm_aesenc_si128(feedback, k[5]);
      feedback = _mm_aesenc_si128(feedback, k[6]);
      feedback = _mm_aesenc_si128(feedback, k[7]);
      feedback = _mm_aesenc_si128(feedback, k[8]);
      feedback = _mm_aesenc_si128(feedback, k[9]);
      feedback = _mm_aesenclast_si128(feedback, k[10]);
      _mm_storeu_si128(enc++, feedback);
    }
    break;
  case 12:
    while (plain < plainEnd) {
      __m128i data = _mm_loadu_si128(plain++);
      feedback = _mm_xor_si128(data, feedback);
      feedback = _mm_xor_si128(feedback, k[0]);
      feedback = _mm_aesenc_si128(feedback, k[1]);
      feedback = _mm_aesenc_si128(feedback, k[2]);
      feedback = _mm_aesenc_si128(feedback, k[3]);
      feedback = _mm_aesenc_si128(feedback, k[4]);
      feedback = _mm_aesenc_si128(feedback, k[5]);
      feedback = _mm_aesenc_si128(feedback, k[6]);
      feedback = _mm_aesenc_si128(feedback, k[7]);
      feedback = _mm_aesenc_si128(feedback, k[8]);
      feedback = _mm_aesenc_si128(feedback, k[9]);
      feedback = _mm_aesenc_si128(feedback, k[10]);
      feedback = _mm_aesenc_si128(feedback, k[11]);
      feedback = _mm_aesenclast_si128(feedback, k[12]);
      _mm_storeu_si128(enc++, feedback);
    }
    break;
  case 14:
    while (plain < plainEnd) {
      __m128i data = _mm_loadu_si128(plain++);
      feedback = _mm_xor_si128(data, feedback);
      feedback = _mm_xor_si128(feedback, k[0]);
      feedback = _mm_aesenc_si128(feedback, k[1]);
      feedback = _mm_aesenc_si128(feedback, k[2]);
      feedback = _mm_aesenc_si128(feedback, k[3]);
      feedback = _mm_aesenc_si128(feedback, k[4]);
      feedback = _mm_aesenc_si128(feedback, k[5]);
      feedback = _mm_aesenc_si128(feedback, k[6]);
      feedback = _mm_aesenc_si128(feedback, k[7]);
      feedback = _mm_aesenc_si128(feedback, k[8]);
      feedback = _mm_aesenc_si128(feedback, k[9]);
      feedback = _mm_aesenc_si128(feedback, k[10]);
      feedback = _mm_aesenc_si128(feedback, k[11]);
      feedback = _mm_aesenc_si128(feedback, k[12]);
      feedback = _mm_aesenc_si128(feedback, k[13]);
      feedback = _mm_aesenclast_si128(feedback, k[14]);
      _mm_storeu_si128(enc++, feedback);
    }
    break;
  }
}

void AESNI_cbc_decrypt(const unsigned char* in, unsigned char* out,
                       unsigned char ivec[16], unsigned long length,
                       AES_KEY_ALIGNED* key)
{
  assert(key->rounds == 10 ||  key->rounds == 12 ||  key->rounds == 14);
  length = (length % 16)? length / 16 + 1 : length / 16;
  __m128i* enc = (__m128i*)in;
  const __m128i* const encEnd = enc + length;
  __m128i* dec = (__m128i*)out;
  const __m128i* const k = (__m128i*)key->rd_key;
  __m128i feedback = _mm_loadu_si128((__m128i*)ivec);
  switch ( key->rounds) {
  case 10:
    while (enc < encEnd) {
      __m128i last_in = _mm_loadu_si128(enc++);
      __m128i data = _mm_xor_si128(last_in, k[0]);
      data = _mm_aesdec_si128(data, k[1]);
      data = _mm_aesdec_si128(data, k[2]);
      data = _mm_aesdec_si128(data, k[3]);
      data = _mm_aesdec_si128(data, k[4]);
      data = _mm_aesdec_si128(data, k[5]);
      data = _mm_aesdec_si128(data, k[6]);
      data = _mm_aesdec_si128(data, k[7]);
      data = _mm_aesdec_si128(data, k[8]);
      data = _mm_aesdec_si128(data, k[9]);
      data = _mm_aesdeclast_si128(data, k[10]);
      data = _mm_xor_si128(data, feedback);
      _mm_storeu_si128(dec++, data);
      feedback = last_in;
    }
    break;
  case 12:
    while (enc < encEnd) {
      __m128i last_in = _mm_loadu_si128(enc++);
      __m128i data = _mm_xor_si128(last_in, k[0]);
      data = _mm_aesdec_si128(data, k[1]);
      data = _mm_aesdec_si128(data, k[2]);
      data = _mm_aesdec_si128(data, k[3]);
      data = _mm_aesdec_si128(data, k[4]);
      data = _mm_aesdec_si128(data, k[5]);
      data = _mm_aesdec_si128(data, k[6]);
      data = _mm_aesdec_si128(data, k[7]);
      data = _mm_aesdec_si128(data, k[8]);
      data = _mm_aesdec_si128(data, k[9]);
      data = _mm_aesdec_si128(data, k[10]);
      data = _mm_aesdec_si128(data, k[11]);
      data = _mm_aesdeclast_si128(data, k[12]);
      data = _mm_xor_si128(data, feedback);
      _mm_storeu_si128(dec++, data);
      feedback = last_in;
    }
    break;
  case 14:
    while (enc < encEnd) {
      __m128i last_in = _mm_loadu_si128(enc++);
      __m128i data = _mm_xor_si128(last_in, k[0]);
      data = _mm_aesdec_si128(data, k[1]);
      data = _mm_aesdec_si128(data, k[2]);
      data = _mm_aesdec_si128(data, k[3]);
      data = _mm_aesdec_si128(data, k[4]);
      data = _mm_aesdec_si128(data, k[5]);
      data = _mm_aesdec_si128(data, k[6]);
      data = _mm_aesdec_si128(data, k[7]);
      data = _mm_aesdec_si128(data, k[8]);
      data = _mm_aesdec_si128(data, k[9]);
      data = _mm_aesdec_si128(data, k[10]);
      data = _mm_aesdec_si128(data, k[11]);
      data = _mm_aesdec_si128(data, k[12]);
      data = _mm_aesdec_si128(data, k[13]);
      data = _mm_aesdeclast_si128(data, k[14]);
      data = _mm_xor_si128(data, feedback);
      _mm_storeu_si128(dec++, data);
      feedback = last_in;
    }
    break;
  }
}
