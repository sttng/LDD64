// LEGO Digital Designer — restored source
// LEGO::BMPBitmapCodec — Windows/OS2 bitmap reader/writer (the only Mac codec). TU 299: Mac
// 0x2a6d90-0x2aac2f (+ inline BitmapCodec code), Win 0x699e30-0x699eaf, 0x69ae40,
// 0x69b010-0x69cd0f. Class names from RTTI; members inferred.
#pragma once
#include "LEGO/Types.h"
#include "BitmapCodec.h"

namespace LEGO {

// Mac sizeof 0x24 (BitmapCodec part, m_header +0x18, Object +0x1c), vtables 0x74428c/0x7442dc,
// ctor 0x2a91c0 (inlined in the factory), D1 0x2a9300, D0 0x2a93c0, thunks 0x2a9360/0x2a9430.
// Win sizeof 0x28 (m_header +0x1c, Object +0x20), vtables 0x9de3a4 / 0x9de390, ctor 0x69cb70
// thiscall(mostDerived), dtor 0x69cc20, sdd 0x69cce0.
class BMPBitmapCodec : public BitmapCodecSurfaceHelper {
public:
    // BITMAPFILEHEADER + BITMAPV5HEADER fields. Mac/Win sizeof 0xa8 (Object at +0xa0); Mac
    // vtables 0x775790 {D1 0x2a9da0, D0 0x2a9e50} / 0x7757b0; Win ctor 0x69c1a0 (vtable
    // 0x9de374, sdd 0x69cca0, dtor 0x69c260). Offsets are the same in both builds.
    class BMPHeader : public BitmapCodec::Header {
    public:
        BMPHeader()                                                  // Win 0x69c1a0 (memset of +0x24..+0x9f)
            : m_fileType(0), m_fileSize(0), m_reserved1(0), m_reserved2(0), m_dataOffset(0),
              m_infoSize(0), m_width(0), m_height(0), m_planes(0), m_bitCount(0),
              m_compression(0), m_imageSize(0), m_xPixelsPerMeter(0), m_yPixelsPerMeter(0),
              m_colorsUsed(0), m_colorsImportant(0), m_redMask(0), m_greenMask(0),
              m_blueMask(0), m_alphaMask(0), m_colorSpaceType(0), m_gammaRed(0),
              m_gammaGreen(0), m_gammaBlue(0), m_intent(0), m_profileData(0),
              m_profileSize(0), m_reserved(0)
        {
            for (int i = 0; i < 9; ++i)
                m_endpoints[i] = 0;
        }
        virtual ~BMPHeader() {}

        UInt16 m_fileType;          // +0x14 'BM' read big-endian: 0x424d
        UInt32 m_fileSize;          // +0x18
        UInt16 m_reserved1;         // +0x1c
        UInt16 m_reserved2;         // +0x1e
        UInt32 m_dataOffset;        // +0x20
        UInt32 m_infoSize;          // +0x24 (an OS/2 12-byte header is stored as 40)
        Int32  m_width;             // +0x28
        Int32  m_height;            // +0x2c (> 0: bottom-up rows)
        UInt16 m_planes;            // +0x30
        UInt16 m_bitCount;          // +0x32
        UInt32 m_compression;       // +0x34 0 BI_RGB, 1 RLE8, 2 RLE4, 3 BI_BITFIELDS
        UInt32 m_imageSize;         // +0x38
        Int32  m_xPixelsPerMeter;   // +0x3c
        Int32  m_yPixelsPerMeter;   // +0x40
        UInt32 m_colorsUsed;        // +0x44
        UInt32 m_colorsImportant;   // +0x48
        UInt32 m_redMask;           // +0x4c (V2+)
        UInt32 m_greenMask;         // +0x50
        UInt32 m_blueMask;          // +0x54
        UInt32 m_alphaMask;         // +0x58 (V3+)
        UInt32 m_colorSpaceType;    // +0x5c (V4+)
        UInt32 m_endpoints[9];      // +0x60
        UInt32 m_gammaRed;          // +0x84
        UInt32 m_gammaGreen;        // +0x88
        UInt32 m_gammaBlue;         // +0x8c
        UInt32 m_intent;            // +0x90 (V5)
        UInt32 m_profileData;       // +0x94
        UInt32 m_profileSize;       // +0x98
        UInt32 m_reserved;          // +0x9c
    };

    BMPBitmapCodec() {}                                              // Mac 0x2a91c0, Win 0x69cb70
    virtual ~BMPBitmapCodec() {}                                     // Mac 0x2a9300/0x2a93c0, Win 0x69cc20

    using BitmapCodecSurfaceHelper::Decode;
    using BitmapCodecSurfaceHelper::GetEncodedSize;
    using BitmapCodecSurfaceHelper::Encode;

    virtual void SetStream(DataStream* stream);                      // Mac 0x2a9ae0, Win 0x69ae40 (ICF)
    virtual RefPointer<Header> ReadHeader();                         // Mac 0x2a6d90, Win 0x69c2c0
    virtual bool Decode(Bitmap& bitmap, const Color* colorKey);      // Mac 0x2a7f00, Win 0x69b680
    virtual UInt32 GetEncodedSize(const Bitmap& bitmap);             // Mac 0x2a8b90, Win 0x69b640
    virtual bool Encode(const Bitmap& bitmap, const EncodeArgs* args); // Mac 0x2a8bb0, Win 0x69bbb0

    // BI_BITFIELDS masks -> BitmapFormat (0 when unknown). Win 0x69b010 cdecl(bpp, r, g, b, a);
    // inlined in the Mac ReadHeader. (name inferred)
    static UInt32 GetFormatFromMasks(UInt32 bitCount, UInt32 red, UInt32 green, UInt32 blue, UInt32 alpha);

private:
    // RLE4 / RLE8 into a buffer of (width + 1) / 2 resp. width bytes per row (top row first as
    // stored, i.e. still bottom-up). Mac 0x2a86f0 / inlined in 0x2a7f00; Win 0x69b2d0 / 0x69b510.
    void DecodeRLE4(const UInt8* source, UInt8* destination, UInt32 width, UInt32 height) const;
    void DecodeRLE8(const UInt8* source, UInt8* destination, UInt32 width, UInt32 height) const;

    RefPointer<BMPHeader> m_header;                                  // Mac +0x18, Win +0x1c (cache)
};

} // namespace LEGO
