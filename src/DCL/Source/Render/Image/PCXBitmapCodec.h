// LEGO Digital Designer — restored source
// LEGO::PCXBitmapCodec — ZSoft PCX (version 5) reader/writer, Windows only: Win
// 0x69cd10-0x69e33f. Class names from RTTI; members inferred.
#pragma once
#include "LEGO/Types.h"
#include "BitmapCodec.h"

namespace LEGO {

// Win sizeof 0x28 (m_header +0x1c, Object +0x20), vtables 0x9de424 / 0x9de410, ctor 0x69e1a0,
// dtor 0x69e250, sdd 0x69e310.
class PCXBitmapCodec : public BitmapCodecSurfaceHelper {
public:
    // The 128-byte file header. Win sizeof 0x9c (Object +0x94), ctor 0x69db40, dtor 0x69dc10,
    // sdd 0x69e2d0.
    class PCXHeader : public BitmapCodec::Header {
    public:
        PCXHeader()                                                  // Win 0x69db40 (memset of +0x14..+0x93)
            : m_manufacturer(0), m_version(0), m_encoding(0), m_bitsPerPixel(0), m_xMin(0),
              m_yMin(0), m_xMax(0), m_yMax(0), m_horizontalDpi(0), m_verticalDpi(0),
              m_reserved(0), m_planes(0), m_bytesPerLine(0), m_paletteInfo(0),
              m_horizontalScreenSize(0), m_verticalScreenSize(0)
        {
            for (int i = 0; i < 48; ++i)
                m_palette[i] = 0;
            for (int i = 0; i < 54; ++i)
                m_filler[i] = 0;
        }
        virtual ~PCXHeader() {}

        UInt8  m_manufacturer;          // +0x14 10
        UInt8  m_version;               // +0x15 5
        UInt8  m_encoding;              // +0x16 0 raw, 1 RLE
        UInt8  m_bitsPerPixel;          // +0x17 1 or 8
        Int16  m_xMin;                  // +0x18
        Int16  m_yMin;                  // +0x1a
        Int16  m_xMax;                  // +0x1c
        Int16  m_yMax;                  // +0x1e
        UInt16 m_horizontalDpi;         // +0x20
        UInt16 m_verticalDpi;           // +0x22
        UInt8  m_palette[48];           // +0x24 16-colour EGA palette (ignored)
        UInt8  m_reserved;              // +0x54
        UInt8  m_planes;                // +0x55
        UInt16 m_bytesPerLine;          // +0x56 (ignored by Decode)
        UInt16 m_paletteInfo;           // +0x58
        UInt16 m_horizontalScreenSize;  // +0x5a
        UInt16 m_verticalScreenSize;    // +0x5c
        UInt8  m_filler[54];            // +0x5e (not read)
    };

    PCXBitmapCodec() {}                                              // Win 0x69e1a0
    virtual ~PCXBitmapCodec() {}                                     // Win 0x69e250

    using BitmapCodecSurfaceHelper::Decode;
    using BitmapCodecSurfaceHelper::GetEncodedSize;
    using BitmapCodecSurfaceHelper::Encode;

    virtual void SetStream(DataStream* stream);                      // Win 0x69ae40 (ICF with BMP)
    virtual RefPointer<Header> ReadHeader();                         // Win 0x69dc70
    virtual bool Decode(Bitmap& bitmap, const Color* colorKey);      // Win 0x69d020
    virtual UInt32 GetEncodedSize(const Bitmap& bitmap);             // Win 0x69cfd0
    virtual bool Encode(const Bitmap& bitmap, const EncodeArgs* args); // Win 0x69d5b0

private:
    RefPointer<PCXHeader> m_header;                                  // Win +0x1c
};

} // namespace LEGO
