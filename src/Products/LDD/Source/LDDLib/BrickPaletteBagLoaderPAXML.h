// LEGO Digital Designer — restored source
// LEGO::BrickPaletteBagLoaderPAXML — reads one palette ("bag") folder: Info.baxml (name, brand,
// version, countable, brandFilter), every "*.paxml" brick list (an Expat SAX parse with a stack
// of handler pairs) and, for brand-filter bags, MaterialSelector.xml (the bag's colour grid); and
// writes a bag back as a .paxml file (SavePalettesCommand, TU 439).
// LEGO::BrickPaletteBagLoaderModel — builds a bag from a model file (LXF / LXFML / LDraw used as
// a palette); LoadBag is TU 440 (Mac 0x3b6500,
// BrickPaletteBagLoaderModel.cpp).
//
// TU 057 "BrickPaletteBagLoaderPAXML.cpp" (Mac 0x1298c0-0x12e78b, modmap guess Path.cpp; static
// initialiser 0x12e710 = `#include <iostream>`), Win 0x4e4490 (WritePart), 0x64d50a (bag info),
// 0x64e500 (context), 0x64ee77 (Load); the handlers are scattered /LTCG code.
// Both loader classes have no key function: their vtables and inline destructors are emitted in
// LDDApplication.cpp (082c): BrickPaletteBagLoaderPAXML Mac 0x76912c {D1 0x17fd90, D0 0x17fda0},
// Win 0x9cd5fc {sdd 0x5fc1a6}; BrickPaletteBagLoaderModel Mac 0x76905c {D1 0x17eb20, D0
// 0x17fb70}, Win 0x9cd550 {sdd 0x5fbcbc}. Both are used as stack objects (sizeof 4).
//
// .paxml format (element / attribute names compared exactly; numbers StrToUInt32, base 10):
//   <PAXML versionMajor="1" versionMinor="0">            (major != 1: the parse stops, error)
//     <Bag>                                               (any other root child: skipped)
//       <Brick designID materialID quantity ItemNos>      one-part brick; materialID defaults to
//         <SubMaterial surfaceID materialID/>             21 for an LDD Extended bag, else 0
//         <Decoration surfaceID decorationID/>
//       </Brick>
//       <Assembly designID quantity ItemNos>              multi-part brick (parts in material 21)
//         <Part designID materialID>                      every part with that design ID
//           <SubMaterial …/> <Decoration …/>
//         </Part>
//       </Assembly>
//       <Meta><Name attr="…">text</Name>…</Meta>          collected, not used
//     </Bag>
//   </PAXML>
// Bricks that do not match the database (unknown design, material, decoration, surface, part
// count) are traced (TraceVerbose) and dropped with their subtree; the file still loads.
// ItemNos becomes the palette brick's name (BrickPalette::Brick::m_name). Every name except the
// class names and the strings is inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BrickPalette.h"
#include "PaletteNames.h"

namespace LEGO {

class Directory;
class Path;
class BrickDatabase;
struct StorageScene2;

class BrickPaletteBagLoaderPAXML {
public:
    BrickPaletteBagLoaderPAXML() {}
    virtual ~BrickPaletteBagLoaderPAXML() {}                                // Mac 0x17fd90 / 0x17fda0, Win sdd 0x5fc1a6

    // Empties the bag's bricks, reads `directory`/Info.baxml into the bag (false when missing or
    // invalid), then every file of the directory: "*.paxml" are parsed into the bag (a file that
    // cannot be read or parsed ends the enumeration: false), "MaterialSelector.xml" fills the
    // colour grid of a brandFilter bag. On success `names` gets (directory name, bag version)
    // and the bag's path becomes the directory. Mac 0x1298c0, Win 0x64ee77
    bool Load(Directory* directory, BrickPalette::Bag& bag, const BrickDatabase* database,
              PaletteNames* names);
    // Writes the bag's bricks as a .paxml file (UTF-8, CRLF; one-part bricks as <Brick>, others
    // as <Assembly> with each part design ID once). True when the file was written.
    // (SavePalettesCommand, TU 439) Mac 0x12ae20
    bool Save(const Path& path, const BrickPalette::Bag& bag);
};

class BrickPaletteBagLoaderModel {
public:
    BrickPaletteBagLoaderModel() {}
    virtual ~BrickPaletteBagLoaderModel() {}                                // Mac 0x17eb20 / 0x17fb70, Win sdd 0x5fbcbc

    // The model at `path` as a bag (one palette brick per scene brick, checked against the
    // database); traces what does not match (the brick is dropped). The bag is named after the
    // model ("User Palette" without a name), countable, brand 0, version 0; `names` (must not be
    // null) gets (model name, bag version). False only when the model does not load.
    // TU 440 (BrickPaletteBagLoaderModel.cpp). Mac 0x3b6500, Win 0x64f402
    bool LoadBag(const Path& path, BrickPalette::Bag& bag, BrickDatabase* database, PaletteNames* names);

private:
    // The bag description of a model. Mac 0x3b7280, Win 0x64f26e
    // (name inferred)
    void InitBag(const StorageScene2& scene, BrickPalette::Bag& bag);
    // Names the brick and gives it one part per database part (bone
    // transforms copied, one material per decoration face: `material` or 21 first, the rest 0);
    // false when the database has no such brick or it has no parts. Win 0x64f303 (Mac inlined)
    // (name inferred)
    bool CreateParts(BrickPalette::Brick& brick, const UInt32* material, const String& name,
                     const BrickDatabase& database);
};

} // namespace LEGO
