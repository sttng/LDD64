// LEGO Digital Designer — restored source
// LXFML 4 model files: LEGO::ModelLoaderLXFML4 (reader, StorageScene based) and
// LEGO::ModelSaverLXFML4 (writer).
//
// Mac TU 341 "ModelIOLXFML4.cpp" (0x30b8a0-0x31585f),
// Win 0x4e0520-0x4e7190 (+ ctor/dtor 0x4b3360/0x4b3410, 0x601572/0x6015a6/0x6015da).
//
// The reader is an Expat SAX parser with a stack of (start, end) element handler pairs: a start
// handler pushes the pair for the element's children (or the "ignore" pair for unknown elements),
// the end handler of that pair pops it when the element closes. Some handlers push nothing for
// their element (<Camera>, <SubMaterial>, <Decoration>, <GroupRef>, <PartRef>, and a <Part> or
// <LXFML> with an error), so the element's children reach the parent's start handler again.
//
//   <LXFML versionMajor="4" versionMinor="0">            other versions: VersionError
//     <Meta><X a="…" …>text</X></Meta>                   -> StorageScene::m_metas (ASCII names /
//                                                          values; <BrickSet version> is kept)
//     <Cameras><Camera refID name distance fieldOfView orthographic angle ax … tz/></Cameras>
//     <Scene cameraRefID>
//       <Model name>
//         <Group refID name angle ax ay az tx ty tz>     -> Rigid
//           <Part refID designID materialID name itemNos flexRefID controlPoint
//                 assemblyRefID assemblyID angle … tz>   -> Part (+ Flex, Assembly)
//             <SubMaterial surfaceID materialID/>
//             <Decoration surfaceID decorationID/>
//           </Part>
//         </Group>
//         <Joint name type="hinge|…">                    (skipped with flag bit 0)
//           <GroupRef groupRef ax ay az zx zy zz tx ty tz/>  ×2
//         </Joint>
//       </Model>
//     </Scene>
//     <BuildingInstructions>
//       <BuildingInstruction name cameraRefID>
//         <Step name cameraRefID exploded ax ay az>      (nested steps)
//           <PartRef partRefID exploded angle … tz/>
//
// Offsets: Mac / Win.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "ModelIOStorageScene.h"

namespace LEGO {

class Path;

// ---------------------------------------------------------------------------------------------
// ModelLoaderLXFML4. sizeof Mac 0x80 / Win 0x98 (no members of its own).
// Mac vtable 0x74b5d0 {D1 0x30e690, D0 0x30e6a0, Object …, Load 0x2efbd0, Load 0x30b9f0};
// Win vtable 0xa2502c {sdd 0x4b3410, Object …, Load 0x4d8560, Load 0x4e4020}.
// ---------------------------------------------------------------------------------------------
class ModelLoaderLXFML4 : public ModelLoaderStorageScene {
public:
    ModelLoaderLXFML4();                         // Win 0x4b3360 (thiscall); inlined on Mac
    virtual ~ModelLoaderLXFML4();                // Mac 0x30e690 (D0 0x30e6a0), Win sdd 0x4b3410

    using ModelLoaderStorageScene::Load;
    // Clears `scene` (StorageScene::Clear) and the errors, parses `content`. False on a parser
    // error (ParserError), without an <LXFML> element (ContentError missingLXFMLTag), with any
    // error collected while reading, or when the parts of one Flex are not all in the same
    // Assembly (no error added). Mac 0x30b9f0, Win 0x4e4020.
    virtual bool Load(const AString& content, StorageScene& scene, bool flags);

    struct Parser;   // the Expat user data (ModelIOLXFML4.cpp)
};

// ---------------------------------------------------------------------------------------------
// ModelSaverLXFML4. sizeof Mac 0x34 / Win 0x30.
// Mac vtable 0x74b604 {D1 0x30e6e0, D0 0x30e730, Object …, Save 0x2f1d00, Save 0x30c080};
// Win vtable 0x9ce02c {sdd 0x6015da, Object …, Save 0x4d98b0, Save 0x4e6050}.
// Element ids (refID, cameraRefID, partRefID …) are assigned in writing order, per element kind.
// Attribute values are written as they are (not XML-escaped).
// ---------------------------------------------------------------------------------------------
class ModelSaverLXFML4 : public ModelSaverStorageScene {
public:
    ModelSaverLXFML4();                          // Win 0x601572 (thiscall); inlined on Mac
    virtual ~ModelSaverLXFML4();                 // Mac 0x30e6e0 (D0 0x30e730), Win 0x6015a6 (sdd 0x6015da)

    using ModelSaverStorageScene::Save;
    // Writes Write()'s text, UTF-8 encoded, to `path` (created / truncated). On failure
    // GetErrorMessage() says "Unable to open file: <path>" or "Unable to write to file: <path>".
    // Mac 0x30c080, Win 0x4e6050.
    virtual bool Save(const Path& path, const StorageScene& scene);

    // The document text (inlined into Save in both builds; split out for the restoration so it
    // can be compared without a file). (name inferred)
    static void Write(String& out, const StorageScene& scene);

    const String& GetErrorMessage() const { return m_errorMessage; }   // (accessor, inferred)

    struct Writer;   // the text being built and the element ids (ModelIOLXFML4.cpp)

private:
    String m_errorMessage;   // Mac +0x24 / Win +0x2c (name inferred)
};

} // namespace LEGO
