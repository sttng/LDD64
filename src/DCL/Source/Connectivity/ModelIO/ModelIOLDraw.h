// LEGO Digital Designer — restored source
// LDraw model files (.ldr / .dat / .mpd as plain text): LEGO::ModelLoaderLDraw (reader) and
// LEGO::ModelSaverLDraw (writer). Both work on the version 1 StorageScene; their bases
// (ModelIOStorageScene.h) convert from / to StorageScene2. The LDraw <-> LEGO lookup tables come
// from the mapping file read by LdrawMapXMLLoader (LdrawMapXMLLoader.h).
//
// Mac TU 337 "ModelIOLDraw.cpp" (0x2f7c80-0x2fc20f; `__FILE__` of the assert in Parse is
// .../DCL/Source/Connectivity/ModelIO/ModelIOLDraw.cpp), Win 0x5a2060-0x5a70a0 (+ 0x6037b6).
// The ModelLoaderLDraw constructor and the AssemblyBagItem / BasicMap template instances it
// needs were emitted in TU 336 (ModelLoaderAbstract.cpp, Mac
// 0x2f7740).
//
// Reading (ModelLoaderLDraw::Parse): only type 1 lines ("1 <colour> x y z a b c d e f g h i
// <file>", single-blank separated; tabs are not separators) and "0 CLEAR" are understood; all
// else (comments, other meta commands, types 2-5, .mpd "0 FILE" sub-models) is ignored. All
// parts go into one Rigid of one Model; "0 CLEAR" drops the parts read so far.
// NOTE (platform difference): String::ScanWord skips leading white space on Mac (NSScanner) but
// not on Win, so on Win "0 CLEAR" is never recognised and a line starting with white space is
// ignored; a tab right after the "1" works on both.
// A file naming a mapped <Assembly> becomes the assembly's parts; known assembly pieces are
// grouped into assemblies by position afterwards (leftovers dropped). Unknown files / colours /
// transformations give no part (no error); a type 1 line with fewer than 14 fields adds a
// PartError(misc). The result is true whenever the mapping file could be read.
//
// Coordinates: LDraw has -Y up and 20 LDU per stud; LDD has +Y up and 0.8 per stud, so
// 1 LDD unit = 25 LDU. Conversion: rotation axis (x, -y, -z) with angle -angle, translation
// (x, -y, -z) / 25, then the per-part <Transformation> (the saver inverts all of it).
//
// Offsets: Mac (String 16 bytes, BasicMap 0x1c) / Win (String 4 bytes, BasicMap 0x24).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "ModelIOStorageScene.h"
#include "StorageScene.h"
#include "LdrawMapXMLLoader.h"

namespace LEGO {

class Path;

// ---------------------------------------------------------------------------------------------
// ModelLoaderLDraw. sizeof Mac 0x114 / Win 0x140.
// Mac vtable 0x74b4f8 {D1 0x2fb9c0, D0 0x2fb9d0, Object …, Load 0x2efbd0, Load 0x2f7c80};
// Win vtable 0xa2c9bc {sdd 0x5a2140, Object …, Load 0x4d8560, Load 0x5a45f0}.
// ---------------------------------------------------------------------------------------------
class ModelLoaderLDraw : public ModelLoaderStorageScene {
public:
    // An LDraw part that is a piece of some assembly, waiting to be grouped (Parse). Mac 0x1c /
    // Win 0x10. Constructor Win 0x5a4550 (inlined on Mac).
    struct AssemblyBagItem {
        AssemblyBagItem() : m_designID(0), m_used(false) {}
        AssemblyBagItem(StorageScene::Part* part, UInt32 designID, const String& name)  // Win 0x5a4550
            : m_part(part), m_designID(designID), m_name(name), m_used(false) {}

        RefPointer<StorageScene::Part> m_part;       // +0x00
        UInt32                         m_designID;   // +0x04 design id of the first assembly
                                                     // listing the piece (not used afterwards)
        String                         m_name;       // +0x08 LDraw file name
        bool                           m_used;       // Mac +0x18 / Win +0x0c taken by the
                                                     // assembly being tried
    };

    ModelLoaderLDraw();                               // Mac 0x2f7740 (in TU 336), Win inlined
    virtual ~ModelLoaderLDraw();                      // Mac 0x2fbda0 (D1 0x2fb9c0 / D0 0x2fb9d0), Win 0x5a2060 (sdd 0x5a2140)

    using ModelLoaderStorageScene::Load;
    // Clears the message, `scene` and the errors and parses `content` (8-bit, taken as
    // ASCII/Latin-1). `flags` is not used.
    virtual bool Load(const AString& content, StorageScene& scene, bool flags); // Mac 0x2f7c80, Win 0x5a45f0

    // Last message: why the mapping could not be read, or the last unknown colour /
    // transformation / malformed line. (accessor, name inferred)
    const String& GetMessage() const { return m_message; }

private:
    typedef StorageScene SS1;

    // Line types of ClassifyLine.
    enum LineType {
        kLineClear = 0,   // "0 CLEAR" (case-insensitive)
        kLinePart  = 1,   // "1 …"
        kLineOther = 2
    };

    bool Parse(const String& text, StorageScene& scene);                      // Mac 0x2f7e00, Win 0x5a46b0
    // Reads the mapping file (preference "LDrawMapFile" in the resources directory).
    // Mac 0x2f9a40, Win 0x5a5f50. (name inferred)
    bool LoadMapping();
    // First word of `line` (case-insensitive): "0" + "clear" -> kLineClear, "1" -> kLinePart,
    // else kLineOther. `rest` = the line without the words read. Mac 0x2f9d20, Win 0x5a5a70.
    // (name inferred)
    void ClassifyLine(const String& line, UInt32& type, String& rest);
    // The fields of a type 1 line after the "1". false (message set) with fewer than 14
    // fields; numbers that do not parse leave the value as it was. The scaling is removed from
    // the 3x3 part. Mac 0x2f9f20, Win 0x5a5bc0.
    // (name inferred)
    bool ParsePartLine(const String& rest, UInt32& color, Matrix3D& transformation, String& file);
    // Design id (mapping, else the number the file name starts with), material and
    // transformation of `part`. Mac 0x2fa330, Win 0x5a5e70. (name inferred)
    bool FillPart(SS1::Part* part, UInt32 color, const Matrix3D& transformation,
                  const String& file, const String& transformationName);
    // Mac inlined in FillPart, Win 0x5a6170. (name inferred)
    void LookupDesignID(const String& file, UInt32& designID);
    // Mac 0x2fa900, Win 0x5a6200. (name inferred)
    bool LookupMaterial(UInt32 color, UInt32& material);
    // LDraw placement -> LXF placement of the part `name`. Mac 0x2fa9d0, Win 0x5a62f0.
    // (name inferred)
    bool ToLXFTransformation(const Matrix3D& ldraw, Matrix3D& lxf, const String& name);

protected:
    UInt32                                m_unknown80;        // Mac +0x80 / Win +0x98: never
                                                              // initialised or used (name inferred)
    LdrawMapXMLLoader::BrickMap           m_bricks;           // Mac +0x84 / Win +0x9c
    LdrawMapXMLLoader::MaterialMap        m_materials;        // Mac +0xa0 / Win +0xc0
    LdrawMapXMLLoader::TransformationMap  m_transformations;  // Mac +0xbc / Win +0xe4
    LdrawMapXMLLoader::AssemblyMap        m_assemblies;       // Mac +0xd8 / Win +0x108
    BasicArray<AssemblyBagItem>           m_bag;              // Mac +0xf4 / Win +0x12c (unused:
                                                              // Parse has its own; name inferred)
    String                                m_message;          // Mac +0x104 / Win +0x13c
};

// ---------------------------------------------------------------------------------------------
// ModelSaverLDraw. sizeof Mac 0x94 / Win 0xbc.
// Mac vtable 0x74b52c {D1 0x2fba10, D0 0x2fba20, Object …, Save 0x2f1d00, Save 0x2fabd0};
// Win vtable 0x9ce1b8 {sdd 0x603814, Object …, Save 0x4d98b0, Save 0x5a64a0}.
// Output: "0 <name>", "0 Author: <Application name> <versionMajor>.<versionMinor>" (from the
// <Meta><Application …/> entry) and one type 1 line per part with a known colour, "\r\n" line
// ends, UTF-8.
// ---------------------------------------------------------------------------------------------
class ModelSaverLDraw : public ModelSaverStorageScene {
public:
    ModelSaverLDraw() {}                              // inlined in its callers
    virtual ~ModelSaverLDraw();                       // Mac 0x2fc040 (D1 0x2fba10 / D0 0x2fba20), Win 0x6037b6 (sdd 0x603814)

    using ModelSaverStorageScene::Save;
    // false when the mapping cannot be read or the file cannot be written.
    virtual bool Save(const Path& path, const StorageScene& scene);          // Mac 0x2fabd0, Win 0x5a64a0

private:
    typedef StorageScene SS1;

    // Mac 0x2fb160, Win 0x5a6f10. (name inferred)
    bool LoadMapping();
    // Writes the parts of every rigid of `model` (WritePart). Win 0x5a6c70, inlined on Mac.
    // (name inferred)
    bool WriteModel(String& out, const SS1::Model& model);
    // "1 <colour> x y z a b c d e f g h i <file>\r\n" for `part` placed by `rigid`; a part that
    // cannot be converted is skipped (the "Unsaved part: <design id>" message is built and
    // dropped). Always true. Mac 0x2fb3c0, Win 0x5a6ce0. (name inferred)
    bool WritePart(String& out, const Matrix3D& rigid, const SS1::Part* part);
    // File name, colour and LDraw placement of `part`. NOTE: a part without materials leaves
    // `color` unchanged (uninitialised in WritePart; both builds). Mac 0x2fb680, Win 0x5a6e70.
    // (name inferred)
    bool ConvertPart(const SS1::Part* part, const Matrix3D& rigid, UInt32& color,
                     Matrix3D& ldraw, String& file);
    // Mapped LDraw name, else "<designID>.dat". Always true. Mac 0x2fb740, Win 0x5a70a0.
    // (name inferred)
    bool GetLDrawName(String& file, UInt32 designID);
    // LDraw colour of a LEGO material. Win 0x5a7190, inlined on Mac. (name inferred)
    bool LookupColor(UInt32 material, UInt32& color);
    // LXF placement -> LDraw placement (inverse of ModelLoaderLDraw::ToLXFTransformation).
    // false when `name` has no <Transformation>. Mac 0x2fb820, Win 0x5a7200. (name inferred)
    bool ToLDrawTransformation(Matrix3D& ldraw, const Matrix3D& lxf, const String& name);

    LdrawMapXMLLoader::BrickNameMap       m_bricks;           // Mac +0x24 / Win +0x2c
    LdrawMapXMLLoader::MaterialMap        m_materials;        // Mac +0x40 / Win +0x50 material -> colour
    LdrawMapXMLLoader::TransformationMap  m_transformations;  // Mac +0x5c / Win +0x74
    LdrawMapXMLLoader::AssemblyMap        m_assemblies;       // Mac +0x78 / Win +0x98 (filled, unused)
};

} // namespace LEGO
