// LEGO Digital Designer — restored source
// LEGO::PNGBitmapCodec — PNG reader/writer on libpng (original 1.5.13, LDD64 1.6.59); Windows only
// (Mac: ImageIO, MacImageIO.h). Win 0x699160-0x69afe0 (with shared BitmapCodec helpers). Class
// names RTTI.
#pragma once
#include "LEGO/Types.h"
#include "BitmapCodec.h"

namespace LEGO {

// Win sizeof 0x28 (m_header +0x1c, Object +0x20), vtables 0x9de320 / 0x9de30c, ctor 0x69ad90,
// dtor 0x69aea0, sdd 0x69afe0.
class PNGBitmapCodec : public BitmapCodecSurfaceHelper {
public:
    // No fields of its own. Win sizeof 0x1c, ctor 0x69a2a0, dtor 0x69a340, sdd 0x69afb0.
    class PNGHeader : public BitmapCodec::Header {
    public:
        PNGHeader() {}
        virtual ~PNGHeader() {}
    };

    PNGBitmapCodec() {}                                              // Win 0x69ad90
    virtual ~PNGBitmapCodec() {}                                     // Win 0x69aea0

    using BitmapCodecSurfaceHelper::Decode;
    using BitmapCodecSurfaceHelper::GetEncodedSize;
    using BitmapCodecSurfaceHelper::Encode;

    virtual void SetStream(DataStream* stream);                      // Win 0x69ae40 (ICF with BMP)
    virtual RefPointer<Header> ReadHeader();                         // Win 0x69a390
    virtual bool Decode(Bitmap& bitmap, const Color* colorKey);      // Win 0x69a7e0
    virtual UInt32 GetEncodedSize(const Bitmap& bitmap);             // Win 0x69eae0 (ICF with JPG)
    virtual bool Encode(const Bitmap& bitmap, const EncodeArgs* args); // Win 0x6994e0

private:
    RefPointer<PNGHeader> m_header;                                  // Win +0x1c
};

} // namespace LEGO
