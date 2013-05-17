# AES implementation in raw Python
# Copyright (c) 2013 Oliver Lau <ola@ct.de>, Heise Zeitschriften Verlag
# All rights reserved.

import hashlib
from math import ceil

Rcon = [
    [ 0x00, 0x00, 0x00, 0x00 ],
    [ 0x01, 0x00, 0x00, 0x00 ],
    [ 0x02, 0x00, 0x00, 0x00 ],
    [ 0x04, 0x00, 0x00, 0x00 ],
    [ 0x08, 0x00, 0x00, 0x00 ],
    [ 0x10, 0x00, 0x00, 0x00 ],
    [ 0x20, 0x00, 0x00, 0x00 ],
    [ 0x40, 0x00, 0x00, 0x00 ],
    [ 0x80, 0x00, 0x00, 0x00 ],
    [ 0x1b, 0x00, 0x00, 0x00 ],
    [ 0x36, 0x00, 0x00, 0x00 ]
]                     

S = [
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
]

def RotWord(w):
    return w[1:] + w[:1]

def SubWord(w):
    return list(map(lambda x: S[x], list(w)))

def XorWord(a, b):
    return list(map(lambda x, y: x ^ y, list(a), list(b)))

"""
    key: bytearray
"""
def KeyToBlock(key):
    keylen = len(key) * 8
    n = keylen // 32
    block = [ [ None for i in range(4) ] for j in range(n) ]
    k, i = 0, 0
    while i < n:
        j = 0
        while j < 4:
            block[i][j] = key[k]
            j, k = j + 1, k + 1
        i = i + 1
    return block

"""
    key: bytearray
"""
def ExpandKey(key):
    block = KeyToBlock(key)
    keylen = len(key) * 8
    if keylen == 128: Nr = 10
    elif keylen == 192: Nr = 12
    elif keylen == 256: Nr = 14
    else: raise Exception("key size must be 128, 192 or 256 bits")
    Nb = 4
    Nk = keylen // 32
    i = 0
    w = []
    while i < Nk:
        w.append(block[i])
        i = i + 1
    i = Nk
    while i < Nb * (Nr+1):
        print(str(i) + "\t", end="")
        temp = w[i-1]
        PrintWord(temp)
        print(" ", end="")
        if i % Nk == 0:
            temp = RotWord(temp)
            PrintWord(temp, " ")
            temp = SubWord(temp)
            PrintWord(temp, " ")
            PrintWord(Rcon[i//Nk]); print(" ", end="")
            temp = XorWord(temp, Rcon[i//Nk])
            PrintWord(temp, " ")
        elif Nk > 6 and (i % Nk) == 4:
            temp = SubWord(temp)
            print(9*" ", end=""); PrintWord(temp); print(19*" ", end="")
        else: print(36*" ", end="")
        PrintWord(w[i-Nk], " ")
        xored = XorWord(w[i-Nk], temp)
        PrintWord(xored); print(" ", end="")
        w.append(xored)
        i = i + 1
        print()
    return w


def Mul123(a, b):
    if b == 1: return a & 0xff
    elif b == 2:
        c = a << 1
        if a & 0x80: c ^= 0x1b
        return c & 0xff
    elif b == 3: return Mul123(a, 2) ^ a
    else: raise Exception("b must be 1, 2 or 3")


def mix(a, b, c, d):
    r0 = Mul123(a, 2) ^ Mul123(b, 3) ^ Mul123(c, 1) ^ Mul123(d, 1)
    r1 = Mul123(a, 1) ^ Mul123(b, 2) ^ Mul123(c, 3) ^ Mul123(d, 1)
    r2 = Mul123(a, 1) ^ Mul123(b, 1) ^ Mul123(c, 2) ^ Mul123(d, 3)
    r3 = Mul123(a, 3) ^ Mul123(b, 1) ^ Mul123(c, 1) ^ Mul123(d, 2)
    return r0, r1, r2, r3


def MixColumns(m):
    transposed = list(zip(*m))
    m = map(lambda x: mix(x[0], x[1], x[2], x[3]), transposed)
    return list(zip(*m))


def PrintArray(m, *args):
    if len(args) > 0: print(args[0], end="")
    print(str(list(map(lambda x: hex(x), m))))


def ByteToHex(a):
    def ToHex(n): return "0123456789abcdef"[n]
    return "".join([ ToHex(a >> 4), ToHex(a & 0x0f) ])


def PrintWord(w, *args):
    print("".join(map(lambda x: str(ByteToHex(x)), w)) + " ".join(args), end="")


def PrintMatrix(m):
    for row in m:
        print("    " + str(list(map(lambda x: hex(x), row))))


def PrintKeySchedule(m):
    i, j = 0, 1
    Nr = len(m) // 4
    while i < Nr:
        print("ROUND KEY ", i);
        PrintMatrix(m[i*4:j*4])
        i, j = i + 1, j + 1


def PBKDF2(P, S, C, kLen):
    def XorBytes(dst, src):
        assert len(dst) == len(src)
        N = len(dst)
        for i in range(N):
            dst[i] ^= src[i]
    S = S.encode("utf-8")
    P = P.encode("utf-8")
    m = hashlib.sha224(P)
    hLen = m.digest_size * 8
    length = ceil(kLen / hLen)
    mk = bytearray()
    for i in range(length):
        m.update(S + str(i).encode("utf-8"))
        T = bytearray(m.digest_size)
        for j in range(C):
            m.update(P)
            XorBytes(T, m.digest())
        mk += T
    return mk[:kLen // 8]


def PasswordToKey(password):
    return PBKDF2(password, "", 17, 128)


def demo():
    key = PasswordToKey("s3cR37")
    print(list(map(lambda x: hex(x), key)))

    # key = bytearray.fromhex("f6 c5 82 03 cc 55 54 ad 34 c5 26 3e cd 41 02 cd")
    # key = bytearray.fromhex("2b 7e 15 16 28 ae d2 a6 ab f7 15 88 09 cf 4f 3c")
    # key = bytearray.fromhex("8e 73 b0 f7 da 0e 64 52 c8 10 f3 2b 80 90 79 e5 62 f8 ea d2 52 2c 6b 7b")
    # key = bytearray.fromhex("60 3d eb 10 15 ca 71 be 2b 73 ae f0 85 7d 77 81 1f 35 2c 07 3b 61 08 d7 2d 98 10 a3 09 14 df f4")

    print("KEY SCHEDULE")
    PrintKeySchedule(ExpandKey(key))


    print("MixColumns")
    state = [
        [ 0xa9, 0x91, 0xa1, 0xa3 ],
        [ 0xf7, 0xfd, 0x64, 0xa7 ],
        [ 0xb1, 0xb2, 0x85, 0xc8 ],
        [ 0xbd, 0x55, 0xef, 0x85 ]
    ]
    print("  Vorher:")
    PrintMatrix(state)
    print("  Nachher:")
    PrintMatrix(MixColumns(state))


def main():
    demo()

if __name__ == "__main__":
    main()
