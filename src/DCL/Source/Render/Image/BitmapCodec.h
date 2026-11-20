// LEGO Digital Designer — restored source
// LEGO::BitmapCodec — abstract decoder/encoder on a DataStream holding the whole file. Codecs:
//   BMP (both builds), PNG / JPG / PCX (Win only; Mac uses ImageIO, MacImageIO.h); created via
//   the ObjectFactory with LoadArgs / SaveArgs (BitmapCodecFactoryHelper.h).
// BitmapCodecSurfaceHelper — the array overloads for one-bitmap codecs (base of all four).
// Inline members emitted in Mac TU 299 (0x2a94b0-0x2aa150) and Win 0x699e30-0x69a240.
//
// Virtual slot order (Mac, Itanium = declaration order):
//   0 SetStream            BitmapCodec 0x2a94b0, BMP 0x2a9ae0           Win 0x699f10 / 0x69ae40
//   1 GetStream            0x2a9500                                     Win 0x699f50
//   2 ReadHeader           pure (BMP 0x2a6d90)                          Win BMP 0x69c2c0
//   3 Decode(Bitmap&)      pure (BMP 0x2a7f00)                          Win slot 5 (BMP 0x69b680)
//   4 Decode(BasicArray<Bitmap>&)               helper 0x2a9540        Win slot 4 0x69a120
//   5 Decode(BasicArray<BasicArray<Bitmap> >&)  helper 0x2a9580        Win slot 3 0x69a150
//   6 GetEncodedSize(const Bitmap&) pure (BMP 0x2a8b90)                 Win slot 8 (BMP 0x69b640)
//   7 GetEncodedSize(array)                     helper 0x2a95c0        Win slot 7 0x69a180
//   8 GetEncodedSize(array of arrays)           helper 0x2a95f0        Win slot 6 0x69a1b0
//   9 Encode(const Bitmap&) pure (BMP 0x2a8bb0)                         Win slot 11 (BMP 0x69bbb0)
//  10 Encode(array)                             helper 0x2a9620        Win slot 10 0x69a1e0
//  11 Encode(array of arrays)                   helper 0x2a9660        Win slot 9 0x69a210
//  12/13 D1/D0                                  Win: in the Object vtable (sdd)
// MSVC groups overloaded virtuals in reverse declaration order, hence the Win numbering.
// Names inferred except RTTI ones (Surface, Header, LoadArgs, SaveArgs, EncodeArgs, SurfaceHelper).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Color.h"
#include "ObjectFactory.h"
#include "Bitmap.h"
#include "DataStream.h"

namespace LEGO {

// Mac sizeof 0x20 (vptr, m_stream +4, m_palette +8, Object +0x18), vtables 0x77592c/0x77597c
// (the abstract class itself; D1 0x2a98c0, D0 0x2a9950, thunks 0x2a9990/0x2a9a30).
// Win sizeof 0x24 (vfptr, vbptr +4, m_stream +8, m_palette +0xc, Object +0x1c), vtables 0x9de27c
// {0x699f10, 0x699f50, _purecall...} / 0x9de268 {sdd 0x69af50 (body 0x699fd0), ...}.
class BitmapCodec : public virtual Object {
public:
    // Description of one image in a file (header): Mac/Win sizeof 0x10, struct (RTTI .?AU).
    struct Surface {
        UInt32 m_width;      // +0x00
        UInt32 m_height;     // +0x04
        UInt32 m_format;     // +0x08 BitmapFormat of the stored pixels
        UInt32 m_dataSize;   // +0x0c BMP: pixel bytes after the palette; PNG/JPG: whole stream;
                             //       PCX: stream minus the header (and the 256-colour palette)
    };
    typedef BasicArray<Surface>        SurfaceArray;      // mip levels (one entry in every codec)
    typedef BasicArray<SurfaceArray>   SurfaceArrayArray; // faces/images (one entry in every codec)

    // Parsed file header. Mac sizeof 0x1c (vptr, m_surfaces +4, Object +0x14), vtables
    // 0x77585c {D1 0x2a9b50, D0 0x2a9bf0} / 0x77587c {thunks 0x2a9c30, 0x2a9ce0}.
    // Win sizeof 0x1c (vbptr, m_surfaces +4, Object +0x14), vtable 0x9de254 {sdd 0x69af20}.
    class Header : public virtual Object {
    public:
        Header() {}
        virtual ~Header() {}                                         // Mac 0x2a9b50/0x2a9bf0, Win 0x699eb0

        // m_surfaces[image][level]
        SurfaceArrayArray m_surfaces;                                // +0x04
    };

    // Decode request for the ObjectFactory. RTTI struct; Mac vtable 0x760460 {D1 0x2f9e0, D0
    // 0x527a0} (emitted in TU 021), Win 0x9cf418 {sdd 0x4f6b90}. Mac/Win sizeof 8.
    struct LoadArgs : public ObjectFactoryArgs {
        LoadArgs() {}
        explicit LoadArgs(DataStream* stream) : m_stream(stream) {}
        RefPointer<DataStream> m_stream;                             // +0x04
    };
    // Encode request (no members; only ever the target of a dynamic_cast in the codec
    // factories, no vtable emitted). Mac typeinfo 0x775758, Win .?AUSaveArgs@BitmapCodec@LEGO@@.
    struct SaveArgs : public ObjectFactoryArgs {
    };
    // Optional per-call encoder options (base of JPGBitmapCodec::EncodeArgs). Polymorphic (the
    // JPG encoder dynamic_casts it, Win 0x69eb20) but no vtable was emitted in either build.
    struct EncodeArgs {
        virtual ~EncodeArgs() {}
    };

    BitmapCodec() {}                                                 // inlined (Win 0x69a060 part)

    // Replaces the encoded file this codec works on.
    virtual void SetStream(DataStream* stream)                       // Mac 0x2a94b0, Win 0x699f10
    {
        m_stream = stream;
    }
    virtual RefPointer<DataStream> GetStream() const                 // Mac 0x2a9500, Win 0x699f50
    {
        return m_stream;
    }
    // Parses (and caches) the file header; null when the stream is not in this codec's format.
    virtual RefPointer<Header> ReadHeader() = 0;
    // Decodes the first image into `bitmap` (which must be at least as large as the image;
    // m_data, m_pitch and m_format must be set). Pixels equal to *colorKey (RGB) become
    // transparent when the destination has alpha. Indexed destinations get the palette copied
    // to bitmap.m_palette.
    virtual bool Decode(Bitmap& bitmap, const Color* colorKey) = 0;
    virtual bool Decode(BasicArray<Bitmap>& levels, const Color* colorKey) = 0;
    virtual bool Decode(BasicArray<BasicArray<Bitmap> >& images, const Color* colorKey) = 0;
    // Upper bound of the encoded size; the caller allocates the DataStream with it.
    virtual UInt32 GetEncodedSize(const Bitmap& bitmap) = 0;
    virtual UInt32 GetEncodedSize(const BasicArray<Bitmap>& levels) = 0;
    virtual UInt32 GetEncodedSize(const BasicArray<BasicArray<Bitmap> >& images) = 0;
    // Encodes into the stream's buffer and shrinks the stream to the encoded size.
    virtual bool Encode(const Bitmap& bitmap, const EncodeArgs* args) = 0;
    virtual bool Encode(const BasicArray<Bitmap>& levels, const EncodeArgs* args) = 0;
    virtual bool Encode(const BasicArray<BasicArray<Bitmap> >& images, const EncodeArgs* args) = 0;

    // Declared last: the destructor occupies the last two Mac slots (12/13).
    virtual ~BitmapCodec() {}                                        // Mac 0x2a98c0/0x2a9950, Win 0x699fd0

protected:
    RefPointer<DataStream> m_stream;                                 // Mac +0x04, Win +0x08
    // Palette of the indexed formats of the current file (filled by ReadHeader). Decode
    // converts through it and copies it into Bitmap::m_palette.
    BasicArray<Color>      m_palette;                                // Mac +0x08, Win +0x0c
};

// Mac sizeof 0x20, vtables 0x74431c {.., 0x2a9540, 0x2a9580, .., 0x2a95c0, 0x2a95f0, ..,
// 0x2a9620, 0x2a9660, D1 0x2a96a0, D0 0x2a9730} / 0x74436c. Win vtable 0x9de2c4, ctor 0x69a060
// thiscall(mostDerived), dtor 0x69a240, sdd 0x69af80.
class BitmapCodecSurfaceHelper : public BitmapCodec {
public:
    BitmapCodecSurfaceHelper() {}
    virtual ~BitmapCodecSurfaceHelper() {}                           // Mac 0x2a96a0/0x2a9730, Win 0x69a240

    using BitmapCodec::Decode;
    using BitmapCodec::GetEncodedSize;
    using BitmapCodec::Encode;

    // The array forms accept exactly one element and forward it. (all Mac/Win bodies are the
    // same "size == 1 ? forward : 0")
    virtual bool Decode(BasicArray<Bitmap>& levels, const Color* colorKey)          // Mac 0x2a9540, Win 0x69a120
    {
        if (levels.GetSize() == 1)
            return Decode(levels[0], colorKey);
        return false;
    }
    virtual bool Decode(BasicArray<BasicArray<Bitmap> >& images, const Color* colorKey) // Mac 0x2a9580, Win 0x69a150
    {
        if (images.GetSize() == 1)
            return Decode(images[0], colorKey);
        return false;
    }
    virtual UInt32 GetEncodedSize(const BasicArray<Bitmap>& levels)                // Mac 0x2a95c0, Win 0x69a180
    {
        if (levels.GetSize() == 1)
            return GetEncodedSize(levels[0]);
        return 0;
    }
    virtual UInt32 GetEncodedSize(const BasicArray<BasicArray<Bitmap> >& images)   // Mac 0x2a95f0, Win 0x69a1b0
    {
        if (images.GetSize() == 1)
            return GetEncodedSize(images[0]);
        return 0;
    }
    virtual bool Encode(const BasicArray<Bitmap>& levels, const EncodeArgs* args)    // Mac 0x2a9620, Win 0x69a1e0
    {
        if (levels.GetSize() == 1)
            return Encode(levels[0], args);
        return false;
    }
    virtual bool Encode(const BasicArray<BasicArray<Bitmap> >& images, const EncodeArgs* args) // Mac 0x2a9660, Win 0x69a210
    {
        if (images.GetSize() == 1)
            return Encode(images[0], args);
        return false;
    }
};

} // namespace LEGO
