// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __CRC32_H_
#define __CRC32_H_

#ifndef WIN32
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#endif

// CRC32 als Template-Klasse
template<uint32_t V0, uint32_t POLYNOMIAL, bool REV>
class CRC32 {
public:
 CRC32(void) : mCRC(V0) {
    makeTable(REV? reverseBits(POLYNOMIAL) : POLYNOMIAL);
  }
  
  uint32_t processBlock(const uint8_t* buf, const uint8_t* const bufEnd) {
    uint32_t crc = mCRC;
    while (buf < bufEnd) {
      const uint8_t b = *buf++;
      crc = mBits[(crc & 0xffU) ^ b] ^ (crc >> 8);
    }
    mCRC = crc;
    return crc;
  }
  
  inline uint32_t process(const uint8_t* buf, int len) {
    return processBlock(buf, buf + len);
  }
  
  inline void reset(void) {
    mCRC = V0;
  }
  
protected:
  inline void makeTable(uint32_t poly) {
    for (int i = 0; i < 256; ++i) {
      uint32_t bits = i;
      for (int j = 0; j < 8; ++j)
	bits = (bits & 1)? (bits >> 1) ^ poly : bits >> 1;
      mBits[i] = bits;
    }
  }
  
  static inline uint32_t reverseBits(uint32_t x) {
    x = ((x & 0xaaaaaaaaU) >> 1) | ((x & 0x55555555U) << 1);
    x = ((x & 0xccccccccU) >> 2) | ((x & 0x33333333U) << 2);
    x = ((x & 0xf0f0f0f0U) >> 4) | ((x & 0x0f0f0f0fU) << 4);
    x = ((x & 0xff00ff00U) >> 8) | ((x & 0x00ff00ffU) << 8);
    return (x >> 16) | (x << 16);
  }
  
 private:
  uint32_t mCRC;
  uint32_t mBits[256];
};


typedef CRC32<0U, 0x1edc6f41U, true> CRC32C;
typedef CRC32C CRC32_SSE42;
typedef CRC32C CRC32_iSCSI;
typedef CRC32C CRC32_SCTP;
typedef CRC32C CRC32_Btrfs;
typedef CRC32C CRC32_Ext4;

typedef CRC32<0U, 0x04c11db7U, true> CRC32_Ethernet;
typedef CRC32_Ethernet CRC32_HDLC;
typedef CRC32_Ethernet CRC32_X3_66;
typedef CRC32_Ethernet CRC32_ITU_T_V42;
typedef CRC32_Ethernet CRC32_SATA;
typedef CRC32_Ethernet CRC32_MPEG2;
typedef CRC32_Ethernet CRC32_PKZIP;
typedef CRC32_Ethernet CRC32_GZIP;
typedef CRC32_Ethernet CRC32_BZIP2;
typedef CRC32_Ethernet CRC32_PNG;

typedef CRC32<0U, 0x741b8cd7U, true> CRC32K;
typedef CRC32K CRC32_Koopmann;

typedef CRC32<0U, 0x814141abU, true> CRC32Q;
typedef CRC32Q CRC32_Aviation;
typedef CRC32Q CRC32_AIXM;

#endif
