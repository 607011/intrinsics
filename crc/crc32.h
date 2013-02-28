// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once


// CRC32 als Funktion

template <unsigned int CRC0, unsigned int POLYNOMIAL, bool REVERSE>
inline unsigned int crc32_fast(const unsigned char* buf, unsigned int len)
{
	unsigned int poly = POLYNOMIAL;
	// ggf. Bitfolge des Polynoms umkehren
	if (REVERSE) {
		poly = ((poly & 0xaaaaaaaa) >> 1) | ((poly & 0x55555555) << 1);
		poly = ((poly & 0xcccccccc) >> 2) | ((poly & 0x33333333) << 2);
		poly = ((poly & 0xf0f0f0f0) >> 4) | ((poly & 0x0f0f0f0f) << 4);
		poly = ((poly & 0xff00ff00) >> 8) | ((poly & 0x00ff00ff) << 8);
		poly = (poly >> 16) | (poly << 16);
	}
	// Tabelle generieren
	unsigned int bits_table[256];
	for (int i = 0; i < 256; ++i) {
		unsigned int bits = i;
		for (int j = 0; j < 8; ++j)
			bits = bits & 1 ? (bits >> 1) ^ poly : bits >> 1;
		bits_table[i] = bits;
	}
	// CRC berechnen
	unsigned int crc = CRC0;
	while (len--)
		crc = bits_table[(crc ^ *buf++) & 0xff] ^ (crc >> 8);
	return crc;
}


// CRC32 als Klasse

template <unsigned int V0 = 0U, unsigned int POLYNOMIAL = 0x1edc6f41U, bool REV = true>
class CRC32 {
public:
	CRC32(void) : mCRC(V0)
	{
		unsigned int poly = POLYNOMIAL;
		if (REV) {
			poly = ((poly & 0xaaaaaaaaU) >> 1) | ((poly & 0x55555555U) << 1);
			poly = ((poly & 0xccccccccU) >> 2) | ((poly & 0x33333333U) << 2);
			poly = ((poly & 0xf0f0f0f0U) >> 4) | ((poly & 0x0f0f0f0fU) << 4);
			poly = ((poly & 0xff00ff00U) >> 8) | ((poly & 0x00ff00ffU) << 8);
			poly = (poly >> 16) | (poly << 16);
		}
		for (int i = 0; i < 256; ++i) {
			unsigned int bits = i;
			for (int j = 0; j < 8; ++j)
				bits = bits & 1 ? (bits >> 1) ^ poly : bits >> 1;
			mBits[i] = bits;
		}
	}
	unsigned int processBlock(const unsigned char* buf, const unsigned char* const bufEnd)
	{
		unsigned int crc = mCRC;
		while (buf < bufEnd)
			crc = mBits[(crc ^ *buf++) & 0xff] ^ (crc >> 8);
		mCRC = crc;
		return crc;
	}
	inline unsigned int process(const unsigned char* buf, unsigned int len)
	{
		return processBlock(buf, buf + len);
	}

private:
	unsigned int mCRC;
	unsigned int mBits[256];
};
