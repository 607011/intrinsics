// Copyright (c) 2008-2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag

#pragma once


typedef unsigned char byte;

// CRC32 als Template-Funktion
template <unsigned int CRC0, unsigned int POLYNOMIAL, bool REVERSE>
unsigned int crc32_fast(const byte* buf, unsigned int len)
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
	const byte* const bufEnd = buf + len;
	while (buf < bufEnd)
		crc = bits_table[crc & 0xffU ^ *buf++] ^ (crc >> 8);
	return crc;
}


// CRC32 als Template-Klasse
template <unsigned int V0, unsigned int POLYNOMIAL, bool REV>
class CRC32 {
public:
	CRC32(void) : mCRC(V0) {
		makeTable(REV? reverseBits(POLYNOMIAL) : POLYNOMIAL);
	}

	int processBlock(const byte* buf, const byte* const bufEnd) {
		unsigned int crc = mCRC;
		while (buf < bufEnd) {
			const byte b = *buf++;
			crc = mBits[crc & 0xffU ^ b] ^ (crc >> 8);
		}
		mCRC = crc;
		return crc;
	}

	inline unsigned int process(const byte* buf, int len) {
		return processBlock(buf, buf + len);
	}

protected:
	inline void makeTable(unsigned int poly) {
		for (int i = 0; i < 256; ++i) {
			unsigned int bits = i;
			for (int j = 0; j < 8; ++j)
				bits = (bits & 1)? (bits >> 1) ^ poly : bits >> 1;
			mBits[i] = bits;
		}
	}

	static inline unsigned int reverseBits(unsigned int x) {
		x = ((x & 0xaaaaaaaaU) >> 1) | ((x & 0x55555555U) << 1);
		x = ((x & 0xccccccccU) >> 2) | ((x & 0x33333333U) << 2);
		x = ((x & 0xf0f0f0f0U) >> 4) | ((x & 0x0f0f0f0fU) << 4);
		x = ((x & 0xff00ff00U) >> 8) | ((x & 0x00ff00ffU) << 8);
		return (x >> 16) | (x << 16);
	}

private:
	unsigned int mCRC;
	unsigned int mBits[256];
};


typedef CRC32<0U, 0x1edc6f41U, true> CRC32C;
typedef CRC32C CRC32_SSE42;
typedef CRC32C CRC32_iSCSI;
typedef CRC32C CRC32_SCTP;
typedef CRC32C CRC32_Btrfs;
typedef CRC32C CRC32_ext4;

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
