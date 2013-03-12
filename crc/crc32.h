// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
// All rights reserved.

#ifndef __CRC32_H_
#define __CRC32_H_

// Basis-Template-Klasse für CRC32-Implementierungen
template<uint32_t V0, uint32_t POLYNOMIAL, bool REV>
class CRC32Base {
public:
  CRC32Base(void)
    : mCRC(V0)
    , mPolynomial(REV? reverseBits(POLYNOMIAL) : POLYNOMIAL)
  { /* ... */ }

  virtual uint32_t processBlock(const uint8_t* buf, const uint8_t* const bufEnd) = 0;

  inline uint32_t process(const uint8_t* buf, int len)
  {
    return processBlock(buf, buf + len);
  }

  inline void reset(void)
  {
    mCRC = V0;
  }

protected:
  uint32_t mCRC;
  uint32_t mPolynomial;

private:
  static inline uint32_t reverseBits(uint32_t x)
  {
    x = ((x & 0xaaaaaaaaU) >> 1) | ((x & 0x55555555U) << 1);
    x = ((x & 0xccccccccU) >> 2) | ((x & 0x33333333U) << 2);
    x = ((x & 0xf0f0f0f0U) >> 4) | ((x & 0x0f0f0f0fU) << 4);
    x = ((x & 0xff00ff00U) >> 8) | ((x & 0x00ff00ffU) << 8);
    return (x >> 16) | (x << 16);
  }
};


// naiver CRC32 als Template-Klasse
template<uint32_t V0, uint32_t POLYNOMIAL, bool REV>
class CRC32Naive : public CRC32Base<V0, POLYNOMIAL, REV> {
public:
  CRC32Naive(void)
    : CRC32Base<V0, POLYNOMIAL, REV>()
  { /* ... */ }

  uint32_t processBlock(const uint8_t* buf, const uint8_t* const bufEnd)
  {
    uint32_t crc = mCRC;
    while (buf < bufEnd) {
      crc ^= *buf++;
      int bits = 8;
      while (bits--)
         crc = (crc & 1)? (crc >> 1) ^ mPolynomial : (crc >> 1);
    }
    mCRC = crc;
    return crc;
  }

private:
};


// optimierter CRC32 als Template-Klasse
template<uint32_t V0, uint32_t POLYNOMIAL, bool REV>
class CRC32Optimized : public CRC32Base<V0, POLYNOMIAL, REV> {
public:
  CRC32Optimized(void)
    : CRC32Base<V0, POLYNOMIAL, REV>()
  {
    makeTable();
  }

  uint32_t processBlock(const uint8_t* buf, const uint8_t* const bufEnd)
  {
    uint32_t crc = mCRC;
    while (buf < bufEnd)
      crc = mTab[(crc & 0xffU) ^ *buf++] ^ (crc >> 8);
    mCRC = crc;
    return crc;
  }

protected:
  void makeTable(void)
  {
    for (int i = 0; i < 256; ++i) {
      uint32_t bits = i;
      int j = 8;
      while (j--)
        bits = (bits & 1)? (bits >> 1) ^ mPolynomial : bits >> 1;
      mTab[i] = bits;
    }
  }

private:
  uint32_t mTab[256];
};

#endif
