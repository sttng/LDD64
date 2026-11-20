// LEGO Digital Designer — restored source
// LEGO::ModelLoaderAbstract — the loader the application uses for any model file: it tries the
// format loaders newest first (LXFML 5, 4, 3, 2, then LDraw) and keeps the first success.
// Plan item 2.2, Mac TU 336 (0x2f6e90-0x2f7c7f: Load, the destructors, and the template
// instances BasicArray<RefPointer<Error>>::operator= (0x2f7520), BasicArray<ModelLoaderLDraw::
// AssemblyBagItem>, BasicMap<String, UInt32, StringCaseInsensitiveLess> of the inlined
// ModelLoaderLDraw constructor). Win: Load 0x5a2160, ctor 0x5fd8b6, dtor 0x5fd8ea, sdd 0x5fd90e.
#pragma once
#include "ModelLoader.h"

namespace LEGO {

class BrickDatabase;

// sizeof 0x20. Mac vtable 0x74b4c8 {D1 0x2f73d0, D0 0x2f7470, …, Load 0x2f6e90};
// Win vtable 0x9cdb98 {sdd 0x5fd90e, …, Load 0x5a2160}.
class ModelLoaderAbstract : public ModelLoader {
public:
    // Win 0x5fd8b6 (thiscall, database on the stack); inlined on Mac (0x14a680).
    explicit ModelLoaderAbstract(BrickDatabase* database);
    virtual ~ModelLoaderAbstract();             // Mac 0x2f73d0, Win 0x5fd8ea

    // Clears the errors and `scene`, then tries ModelLoaderLXFML5. If that fails without a
    // versionError or parserError its errors are the result; otherwise ModelLoaderLXFML4 (same
    // rule), then ModelLoaderLXFML3, ModelLoaderLXFML2 and ModelLoaderLDraw unconditionally.
    virtual bool Load(const AString& content, StorageScene2& scene, bool flag);  // Mac 0x2f6e90, Win 0x5a2160

private:
    BrickDatabase* m_database;   // +0x1c (not reference counted here)
};

} // namespace LEGO
