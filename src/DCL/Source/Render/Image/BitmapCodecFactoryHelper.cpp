// LEGO Digital Designer — restored source
// TU 298 "BitmapCodecFactoryHelper.cpp": Mac 0x2a6a50-0x2a6d8f, Win 0x6985d0-0x699120.
// Names inferred.
#include "BitmapCodecFactoryHelper.h"
#include "BMPBitmapCodec.h"
#if defined(_WIN32)
#include "PCXBitmapCodec.h"
#include "JPGBitmapCodec.h"
#include "PNGBitmapCodec.h"
#endif

namespace LEGO {

namespace {

template <class T>
void RegisterCodec(ObjectFactory* factory)
{
    RefPointer<SpecificObjectFactory> helper = new BitmapCodecFactoryHelper<T>;
    factory->RegisterFactory(typeid(T), helper.Get(), typeid(BitmapCodec));
}

} // namespace

// Mac 0x2a6a50, Win 0x698f10
void RegisterBitmapCodecFactories()
{
    RefPointer<ObjectFactory> factory = ObjectFactory::GetInstance();
    RegisterCodec<BMPBitmapCodec>(factory.Get());
#if defined(_WIN32)
    RegisterCodec<PCXBitmapCodec>(factory.Get());
    RegisterCodec<JPGBitmapCodec>(factory.Get());
    RegisterCodec<PNGBitmapCodec>(factory.Get());
#endif
}

} // namespace LEGO
