// LEGO Digital Designer — restored source
// LEGO::BitmapMask — 8-bit RLE8 BMP kept compressed, queried per pixel; BitmapMaskFactory
// creates and shares them by file name. Mac TU 026 (0xd69f0-0xd712f); factory emitted in Mac
// TU 060 (0x13ff20-0x140700), Win 0x5cadbf-0x5cb659. Win BitmapMask 0x5124c0 / 0x5125b6 /
// 0x5caee4. Names inferred except the classes (RTTI).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "BasicMap.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "Path.h"

namespace LEGO {

class BitmapMaskFactory;

// Mac sizeof 0x30 {vptr, m_size +4, m_data +0xc, m_ownsData +0x10, m_name +0x14, m_factory +0x24,
// Object +0x28}; vtables 0x706b7c {D1 0xd6fc0, D0 0xd7050, OnFinalRelease 0xd6e40} / 0x706ba0.
// Win sizeof 0x28 {vbptr, m_size +4, m_data +0xc, m_ownsData +0x10, m_name +0x14, m_factory +0x18,
// Object +0x20}; Object vtable 0x9ca6b0 {sdd 0x5caf64, OnFinalRelease 0x5caf5c, ...}.
class BitmapMask : public virtual Object {
public:
    // Mac vtable 0x768ff0 {D1 0x173d40, D0 0x17f7b0}, Win 0x9cd5d8 {sdd 0x5c3720}.
    struct LoadArgs : public ObjectFactoryArgs {
        explicit LoadArgs(const Path& path) : m_path(path) {}
        Path m_path;   // +0x04
    };

    BitmapMask();                                                     // Mac 0x1402e0 (TU 060), Win 0x5caee4
    virtual ~BitmapMask();                                            // Mac 0xd6fc0 (D1), 0xd7050 (D0)

    // Reads the file of `path`: an uncompressed-header BMP with BITMAPINFOHEADER, 1 plane,
    // 8 bits per pixel, BI_RLE8 and a bfSize equal to the file size; keeps the RLE8 data
    // (from bfOffBits to the end of the file) and the absolute width / height. The name
    // becomes path.GetPath(false).
    bool Load(const Path& path);                                      // Mac 0xd6bc0, Win 0x5125b6

    // The palette index at `point` (pixels, y down from the top row; the file's rows are
    // bottom-up), 0 outside the image, in delta gaps and after an end-of-bitmap code.
    UInt8 GetValue(const Vector2& point) const;                       // Mac 0xd69f0, Win 0x5124c0

    const Vector2& GetSize() const { return m_size; }
    const String&  GetName() const { return m_name; }

    // Unregisters the mask from its factory before the object is destroyed.
    virtual void OnFinalRelease();                                    // Mac 0xd6e40 (thunk 0xd6f70), Win 0x5caf5c

private:
    friend class BitmapMaskFactory;
    void SetData(UInt8* data, bool ownsData);                         // inlined in Load

    BitmapMask(const BitmapMask&);
    BitmapMask& operator=(const BitmapMask&);

    Vector2            m_size;       // Mac +0x04, Win +0x04 (width, height as floats)
    UInt8*             m_data;       // Mac +0x0c, Win +0x0c RLE8 stream
    bool               m_ownsData;   // Mac +0x10, Win +0x10
    String             m_name;       // Mac +0x14, Win +0x14 key in the factory's map
    BitmapMaskFactory* m_factory;    // Mac +0x24, Win +0x18 (cleared when the factory dies)
};

// Mac sizeof 0x2c {SpecificObjectFactory 0x10, BasicMap +0x10 (std::map +0x14)}; vtable
// 0x766148 {D1 0x13ff20, D0 0x13ff30, .., CreateObject 0x13ff70, Initialize, Shutdown}.
// Win vtable 0x9ca690 {sdd 0x5cb659, .., CreateObject 0x5cb4e7, ..}. Registered for
// typeid(BitmapMask) by the application start-up (TU 060).
class BitmapMaskFactory : public SpecificObjectFactory {
public:
    typedef BasicMap<String, BitmapMask*, StringCaseInsensitiveLess<String> > MaskMap;

    BitmapMaskFactory() {}
    virtual ~BitmapMaskFactory();                                     // Mac 0x1401e0 (via 0x13ff20/0x13ff30), Win 0x5cb5d6

    // BitmapMask::LoadArgs -> the shared mask of that file name (loaded on first use); null for
    // other arguments or when the file cannot be loaded.
    virtual Object* CreateObject(const ObjectFactoryArgs& args);      // Mac 0x13ff70, Win 0x5cb4e7

    // Removes `name` under the factory mutex (the factory must be registered: the mutex is
    // the ObjectFactory's). NOTE: erases Find(name) without checking it (both builds; the name
    // is always present).
    void Remove(const String& name);                                  // Mac 0xd6e80

    const MaskMap& GetMasks() const { return m_masks; }

private:
    MaskMap m_masks;   // Mac +0x10
};

} // namespace LEGO
