// LEGO Digital Designer — restored source
// LEGO::JPGBitmapCodec — JFIF reader/writer on libjpeg (original 6b, LDD64 10); Windows only (Mac:
// ImageIO, MacImageIO.h). Win 0x69e340-0x69f9af. The libjpeg wrapper classes are private to the
// .cpp. Class names from RTTI.
#pragma once
#include "LEGO/Types.h"
#include "BitmapCodec.h"

namespace LEGO {

// Win sizeof 0x28 (m_header +0x1c, Object +0x20), vtables 0x9de530 / 0x9de51c, sdd 0x69f980.
class JPGBitmapCodec : public BitmapCodecSurfaceHelper {
public:
    // Encoder options: libjpeg quality 0..100 (default and upper limit 100). RTTI struct,
    // derived from BitmapCodec::EncodeArgs (Win cast helper 0x69eb20). sizeof 8.
    struct EncodeArgs : public BitmapCodec::EncodeArgs {
        explicit EncodeArgs(UInt32 quality = 100) : m_quality(quality) {}
        UInt32 m_quality;                                            // +0x04
    };

    // No fields of its own. Win sizeof 0x1c, ctor 0x69f070, dtor 0x69f110, sdd 0x69f950.
    class JPEGHeader : public BitmapCodec::Header {
    public:
        JPEGHeader() {}
        virtual ~JPEGHeader() {}
    };

    JPGBitmapCodec() {}                                              // Win 0x69f820
    virtual ~JPGBitmapCodec() {}                                     // Win 0x69f8d0

    using BitmapCodecSurfaceHelper::Decode;
    using BitmapCodecSurfaceHelper::GetEncodedSize;
    using BitmapCodecSurfaceHelper::Encode;

    virtual void SetStream(DataStream* stream);                      // Win 0x69ae40 (ICF with BMP)
    virtual RefPointer<Header> ReadHeader();                         // Win 0x69f160
    virtual bool Decode(Bitmap& bitmap, const Color* colorKey);      // Win 0x69f460
    virtual UInt32 GetEncodedSize(const Bitmap& bitmap);             // Win 0x69eae0 (ICF with PNG)
    virtual bool Encode(const Bitmap& bitmap, const BitmapCodec::EncodeArgs* args); // Win 0x69ed80

private:
    RefPointer<JPEGHeader> m_header;                                 // Win +0x1c
};

} // namespace LEGO
