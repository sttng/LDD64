// LEGO Digital Designer — restored source
// LXFML 2 model files: LEGO::ModelLoaderLXFML2 (reader only, StorageScene based).
//
// Mac TU 339 "ModelIOLXFML2.cpp" (0x2fdfe0-0x302b5f; the object also holds the weak JointError
// members and BasicArray<StorageScene::Part*> insert),
// Win 0x5b1890-0x5b3a30 (+ ctor 0x5a1de0, dtors 0x5a1e40 / 0x5a1e90).
//
// The same Expat handler-stack reader as LXFML 4 (see ModelIOLXFML4.h) for the subset
//   <LXFML versionMajor="2" versionMinor="0">
//     <Meta><X a="…" …>text</X></Meta>
//     <Scene><Model name>
//       <Group refID name angle ax ay az tx ty tz>
//         <Part designID materialID name flexRefID controlPoint assemblyRefID assemblyID angle … tz>
//           <SubMaterial surfaceID materialID/> <Decoration surfaceID decorationID/>
//       <Joint name type> <GroupRef groupRef ax … tz/> ×2 </Joint>
// (no cameras, no building instructions, no part refIDs, no BrickSet corrections).
#pragma once
#include "LEGO/Types.h"
#include "ModelIOStorageScene.h"

namespace LEGO {

// ---------------------------------------------------------------------------------------------
// ModelLoaderLXFML2. sizeof Mac 0x80 / Win 0x98 (no members of its own).
// Mac vtable 0x74b560 {D1 0x2fe650, D0 0x2fe660, Object …, Load 0x2efbd0, Load 0x2fdfe0};
// Win vtable 0xa2c97c {sdd 0x5a1e90, Object …, Load 0x4d8560, Load 0x5b3780}.
// ---------------------------------------------------------------------------------------------
class ModelLoaderLXFML2 : public ModelLoaderStorageScene {
public:
    ModelLoaderLXFML2();                         // Win 0x5a1de0 (this on the stack); inlined on Mac
    virtual ~ModelLoaderLXFML2();                // Mac 0x2fe650 (D0 0x2fe660), Win 0x5a1e40 (sdd 0x5a1e90)

    using ModelLoaderStorageScene::Load;
    // As ModelLoaderLXFML4::Load. Mac 0x2fdfe0, Win 0x5b3780.
    virtual bool Load(const AString& content, StorageScene& scene, bool flags);

    struct Parser;   // the Expat user data (ModelIOLXFML2.cpp)
};

} // namespace LEGO
