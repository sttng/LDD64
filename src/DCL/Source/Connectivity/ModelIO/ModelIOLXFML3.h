// LEGO Digital Designer — restored source
// LXFML 3 model files: LEGO::ModelLoaderLXFML3 (reader only, StorageScene based).
//
// Mac TU 340 "ModelIOLXFML3.cpp" (0x302b60-0x30b89f),
// Win 0x5ae2e0-0x5b1820 (+ ctor 0x5a1eb0, dtor 0x5a1f10 / sdd 0x5a1f60).
//
// The same Expat handler-stack reader as LXFML 2 (see ModelIOLXFML2.h) for
//   <LXFML versionMajor="3" versionMinor="0">
//     <Meta><X a="…" …>text</X></Meta>
//     <Scene><Model name>
//       <Group refID name angle ax ay az tx ty tz>
//         <Part refID designID materialID name flexRefID controlPoint assemblyRefID assemblyID
//               angle … tz>
//           <SubMaterial surfaceID materialID/> <Decoration surfaceID decorationID/>
//       <Joint name type> <GroupRef groupRef ax … tz/> ×2 </Joint>
// The parts are not stored as they are read: LXFML 3 used older design ids, and every part goes
// through a per-design conversion (ModelIOLXFML3.cpp) that renames it, adds extra parts with
// fixed placements, or builds a whole assembly from the BrickDatabase. Flex and assembly
// references of the file are not kept (no Flex / Assembly from the file; assembly parts are
// rebuilt from the database for a few design ids and dropped otherwise).
#pragma once
#include "LEGO/Types.h"
#include "ModelIOStorageScene.h"

namespace LEGO {

class BrickDatabase;

// ---------------------------------------------------------------------------------------------
// ModelLoaderLXFML3. sizeof Mac 0x84 / Win 0x9c.
// Mac vtable {D1 0x308bb0, D0 0x308bc0, Object …, Load 0x2efbd0, Load 0x3085c0};
// Win vtable 0xa2c998 {sdd 0x5a1f60, Object …, Load 0x4d8560, Load 0x5b1530}.
// ---------------------------------------------------------------------------------------------
class ModelLoaderLXFML3 : public ModelLoaderStorageScene {
public:
    // Win 0x5a1eb0 (this and database on the stack); inlined on Mac
    explicit ModelLoaderLXFML3(BrickDatabase* database);
    virtual ~ModelLoaderLXFML3();                // Mac 0x308bb0 (D0 0x308bc0), Win 0x5a1f10 (sdd 0x5a1f60)

    using ModelLoaderStorageScene::Load;
    // Clears `scene` (StorageScene::Clear) and the errors, parses `content`. False on a parser
    // error (ParserError), without an <LXFML> element (ContentError missingLXFMLTag) or with any
    // error collected while reading; otherwise the pending 3680/3679 parts are paired up
    // (ModelIOLXFML3.cpp) and the result is true. The database must contain the bricks the
    // conversions look up (NOTE: a missing one is dereferenced, both builds).
    // Mac 0x3085c0, Win 0x5b1530.
    virtual bool Load(const AString& content, StorageScene& scene, bool flags);

    struct Parser;   // the Expat user data (ModelIOLXFML3.cpp)

private:
    BrickDatabase* m_database;   // Mac +0x80 / Win +0x98 (not reference counted)
};

} // namespace LEGO
