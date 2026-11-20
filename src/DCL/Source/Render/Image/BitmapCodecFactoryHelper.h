// LEGO Digital Designer — restored source
// LEGO::BitmapCodecFactoryHelper<T> — per-codec SpecificObjectFactory under BitmapCodec: with
//   LoadArgs keeps a T only if T::ReadHeader accepts the stream (codecs are tried in turn), with
//   SaveArgs returns a fresh T. RegisterBitmapCodecFactories registers them at start-up.
// TU 298 (file name guessed, class name RTTI): Mac 0x2a6a50-0x2a6d8f (BMP), Win 0x6985d0-0x699120
// (BMP, PCX, JPG, PNG).
#pragma once
#include "LEGO/Types.h"
#include "ObjectFactory.h"
#include "BitmapCodec.h"

namespace LEGO {

// sizeof 0x10 (SpecificObjectFactory). Mac vtable 0x775728 {D1 0x2a6b30, D0 0x2a6b90,
// OnFinalRelease, AddRef, Release, CreateObject 0x2a6c00, Initialize 0x19070, Shutdown 0x19080}.
// Win (BMP/PCX/JPG/PNG): ctor 0x6985d0/0x6988e0/0x698af0/0x698d00, sdd 0x698730/0x698940/
// 0x698b50/0x698d60, CreateObject 0x698790/0x6989a0/0x698bb0/0x698dc0, vtables 0x9de19c/
// 0x9de1bc/0x9de1dc/0x9de1fc. dynamic_cast helpers Win 0x698630 (LoadArgs) / 0x6986b0 (SaveArgs).
template <class T>
class BitmapCodecFactoryHelper : public SpecificObjectFactory {
public:
    BitmapCodecFactoryHelper() {}
    virtual ~BitmapCodecFactoryHelper() {}

    virtual Object* CreateObject(const ObjectFactoryArgs& args)
    {
        if (const BitmapCodec::LoadArgs* load = dynamic_cast<const BitmapCodec::LoadArgs*>(&args)) {
            T* codec = new T;
            codec->SetStream(load->m_stream.Get());
            RefPointer<BitmapCodec::Header> header = codec->ReadHeader();
            if (!header.IsNull())
                return codec;
            // Not this format: the codec was never referenced, Release takes it from 0 to -1
            // and deletes it. Falls through to the SaveArgs test (fails for LoadArgs).
            codec->Release();
        }
        if (dynamic_cast<const BitmapCodec::SaveArgs*>(&args) != 0)
            return new T;
        return 0;
    }
};

// Registers BitmapCodecFactoryHelper<BMPBitmapCodec> (Mac) / BMP, PCX, JPG and PNG (Win) with
// the ObjectFactory under the abstract type BitmapCodec. Called once from the application
// start-up (Mac 0x13a1c0 area, TU 060). Mac 0x2a6a50, Win 0x698f10. (name inferred)
void RegisterBitmapCodecFactories();

} // namespace LEGO
