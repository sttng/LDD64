// LEGO Digital Designer — restored source
// TU 130 (file name inferred)  Mac 0x1e88e0-0x1e8d2f (6 functions), Win 0x444f10-0x44521f.
// Raw Base64 codec on caller-provided buffers (see Base64.h). Static init 0x1e8cf0.
// Data: alphabet Mac 0x636d00, decode table Mac 0x636d40 (128 entries); Win keeps both after
// the bytes "'7" at Win 0xa1ee8e (alphabet +2, table +0x42).
#include "Base64.h"

#include <string.h>
#include <iostream>

namespace LEGO {

namespace {

const char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Character -> 6-bit value. Copied from the Mac binary (0x636d40). Only characters accepted
// by IsBase64Char are looked up. NOTE: '=' maps to 61 (never used for valid input).
const signed char kDecodeTable[128] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0, 61,  0,  0,
    0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
    0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0
};

// A-Z, a-z, 0-9, '+', '/', '=' (Mac: bit test 0x40011 on c - '+').
inline bool IsBase64Char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
           c == '+' || c == '/' || c == '=';
}

inline UInt32 Value(char c) { return (UInt32)(Int32)kDecodeTable[(UInt8)c & 0x7F]; }

// Encodes the last 1..3 bytes (`count` < 3 in practice). Mac 0x1e8950, Win 0x444f10.
void EncodeTail(const UInt8* in, UInt32 count, char* out)
{
    UInt32 bits = (UInt32)in[0] << 24;
    if (count > 1)
        bits |= (UInt32)in[1] << 16;
    if (count > 2)
        bits |= (UInt32)in[2] << 8;
    switch (count) {
    case 1:
        out[0] = kAlphabet[bits >> 26];
        out[1] = kAlphabet[(bits >> 20) & 0x3f];
        out[2] = '=';
        out[3] = '=';
        break;
    case 2:
        out[0] = kAlphabet[bits >> 26];
        out[1] = kAlphabet[(bits >> 20) & 0x3f];
        out[2] = kAlphabet[(bits >> 14) & 0x3f];
        out[3] = '=';
        break;
    case 3:
        out[0] = kAlphabet[bits >> 26];
        out[1] = kAlphabet[(bits >> 20) & 0x3f];
        out[2] = kAlphabet[(bits >> 14) & 0x3f];
        out[3] = kAlphabet[(bits >> 8) & 0x3f];
        break;
    }
}

// Decodes one group of four characters; returns the number of bytes written.
// Inlined on the Mac (0x1e8b60), Win 0x444ff0.
UInt32 DecodeQuad(const char* quad, UInt8* out)
{
    if (quad[3] != '=') {
        UInt32 v = (((Value(quad[0]) << 6 | Value(quad[1])) << 6 | Value(quad[2])) << 6) | Value(quad[3]);
        out[0] = (UInt8)(v >> 16);
        out[1] = (UInt8)(v >> 8);
        out[2] = (UInt8)v;
        return 3;
    }
    UInt32 v = Value(quad[0]) << 6 | Value(quad[1]);
    if (quad[2] == '=') {
        out[0] = (UInt8)(v >> 4);
        return 1;
    }
    v = v << 6 | Value(quad[2]);
    out[0] = (UInt8)(v >> 10);
    out[1] = (UInt8)(v >> 2);
    return 2;
}

} // namespace

// Mac 0x1e88e0 (Win inlined)
UInt32 Base64EncodedSize(UInt32 size)
{
    UInt32 groups = size / 3;
    return size == groups * 3 ? groups * 4 + 1 : groups * 4 + 5;
}

// Mac 0x1e8910 (Win inlined)
// NOTE: reads text[-1] / text[-2] for an empty text, and text[-1] for a single "=" (the Mac
// binary's static "" sits after zero bytes); fixed: checked.
UInt32 Base64DecodedSize(const char* text)
{
    UInt32 length = (UInt32)strlen(text);
    if (length == 0)
        return 0;
    UInt32 size = (length >> 2) * 3;
    if (text[length - 1] == '=')
        size -= (length >= 2 && text[length - 2] == '=') ? 2 : 1;
    return size;
}

// Mac 0x1e8a50, Win 0x4450c0
void Base64EncodeBuffer(const void* data, UInt32 size, char* out)
{
    const UInt8* in = (const UInt8*)data;
    memset(out, 0, Base64EncodedSize(size));
    for (UInt32 i = 0; i < size; i += 3) {
        UInt32 rest = size - i;
        if (rest < 3) {
            EncodeTail(in + i, rest, out);
            return;
        }
        UInt32 bits = (UInt32)in[i] << 16 | (UInt32)in[i + 1] << 8 | in[i + 2];
        out[0] = kAlphabet[bits >> 18];
        out[1] = kAlphabet[(bits >> 12) & 0x3f];
        out[2] = kAlphabet[(bits >> 6) & 0x3f];
        out[3] = kAlphabet[bits & 0x3f];
        out += 4;
    }
}

// Mac 0x1e8b60, Win 0x4451a0
// NOTE: the original collects four alphabet characters per group without checking for the
// end of the text (text with trailing non-alphabet characters reads past the terminator);
// fixed: an incomplete final group ends the decoding.
UInt32 Base64DecodeBuffer(const char* text, void* out)
{
    UInt8* bytes = (UInt8*)out;
    UInt32 length = (UInt32)strlen(text);
    UInt32 written = 0;
    UInt32 i = 0;
    while (i < length) {
        char quad[4];
        int n = 0;
        while (n < 4) {
            if (i >= length)
                return written;
            char c = text[i++];
            quad[n] = c;
            if (IsBase64Char(c))
                ++n;
        }
        written += DecodeQuad(quad, bytes + written);
    }
    return written;
}

} // namespace LEGO
