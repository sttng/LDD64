// LEGO Digital Designer — restored source
// LXFML 5 model files: LEGO::ModelLoaderLXFML5 (reader), LEGO::ModelSaverLXFML5 (writer) and
// LEGO::LXFML5Tester (root element check run while the XML is parsed).
//
// Mac TU 342 "ModelIOLXFML5.cpp" (one object): sub-TUs
//   342a  Mac 0x315860-0x31f0e0  Win 0x4a4520-0x4a89d0, 0x4ad940, 0x4ada10-0x4ade8f, 0x4ade90
//         loader
//   342b  Mac 0x31f0e0-0x324220  Win 0x4a4190, 0x4a89e0-0x4ab630, 0x4adf60-0x4ae5ff   saver
//   342c  Mac 0x324220-0x326990  Win 0x4a4230-0x4a451f, 0x4ab660-0x4ac28f   LXFML5Tester, the
//         weak ModelLoader::*Error copies (restored in ModelLoader.cpp) and the BasicMap /
//         std::_Rb_tree instances of the reference maps
//
// Format (element / attribute -> StorageScene2 member) is documented in StorageScene2.h. Every
// element name is compared exactly (case sensitive); unknown elements and attributes are
// ignored. Numbers use the String parsers (FromString, StringStream.h) and printers
// (String::operator<<).
//
// Offsets: Mac / Win. BasicMap is 0x1c bytes on Mac (vptr + libstdc++ std::map) and 0x24 on
// Win (vptr + VC9 std::map).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicString.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "Version.h"
#include "XMLDOMNode.h"
#include "ModelLoader.h"
#include "StorageScene2.h"

namespace LEGO {

class BrickDatabase;
struct StorageScene;   // StorageScene.h (a struct in the Win RTTI)
class Path;

// ---------------------------------------------------------------------------------------------
// ModelLoaderLXFML5. sizeof Mac 0xbc / Win 0xe4.
// Mac vtable 0x74b638 {D1 0x317cc0, D0 0x317cd0, Object …, Load 0x315b00};
// Win vtable 0xa245cc {sdd 0x4a4610, Object …, Load 0x4a4630}.
// ---------------------------------------------------------------------------------------------
class ModelLoaderLXFML5 : public ModelLoader {
public:
    // Mac 0x315860 (C1 0x315af0), Win 0x4a4520 (thiscall, database on the stack)
    explicit ModelLoaderLXFML5(BrickDatabase* database);
    // Mac 0x317a20 (D1 0x317cc0 / D0 0x317cd0), Win 0x4a4980 (sdd 0x4a4610)
    virtual ~ModelLoaderLXFML5();

    // Parses `content` into `scene` (cleared first; NOTE: StorageScene2::Clear keeps the scene
    // behaviours). Order: root check, <Meta>, <Cameras>, <Behaviors>, <Bricks> (missing <Bricks>:
    // false without an error), <GroupSystems>, <RigidSystems> (the <Joint>s are skipped when
    // `ignoreJoints` is set — every caller in the program passes true), <BuildingInstructions>.
    // The result is false when any part reported a problem; what could be read stays in
    // `scene`. (parameter name inferred)
    virtual bool Load(const AString& content, StorageScene2& scene, bool ignoreJoints); // Mac 0x315b00, Win 0x4a4630

    const Version& GetBrickSetVersion() const { return m_brickSetVersion; }   // (accessor, inferred)

private:
    friend class LXFML5Tester;
    typedef StorageScene2 SS;

    // Empties the error list, the mask and the five reference maps (not the BrickSet version).
    // Mac 0x315fc0, Win 0x4a4a40 (EAX = this). (name inferred)
    void Reset();
    // <LXFML versionMajor="5" versionMinor="0">; adds ContentError(missingLXFMLTag) or
    // VersionError(5.0, actual). Inlined twice in the original (ReadRoot and LXFML5Tester::Test).
    // (name inferred)
    bool CheckRoot(const XMLDOMNodeAString& root);
    // CheckRoot + scene name. Mac 0x316110, Win 0x4a4b20. (name inferred)
    bool ReadRoot(const XMLDOMNodeAString& root, SS& scene);
    // <Meta> children -> scene.m_metas; <BrickSet version=…> -> m_brickSetVersion.
    // Mac 0x3165a0, Win 0x4a4d40. (name inferred)
    void ReadMeta(const XMLDOMNodeAString& root, SS& scene);
    bool ReadCameras(const XMLDOMNodeAString& root, SS& scene);                 // Mac 0x3169a0, Win 0x4a50a0
    // false when the attribute names an unknown camera; a missing attribute leaves `camera`.
    bool ReadCameraRef(const XMLDOMNodeAString& node, RefPointer<SS::Camera>& camera); // Mac 0x318360, Win 0x4a88d0
    bool ReadBricks(const XMLDOMNodeAString& root, SS& scene);                  // Mac 0x3171d0, Win 0x4a5420
    bool ReadBrick(const XMLDOMNodeAString& node, SS& scene);                   // Mac 0x3184c0, Win 0x4a5590
    // Replaces a brick 74340 by two bricks 3679 + 3680 sharing its parts. Mac 0x319b70,
    // Win 0x4a81b0. (name inferred)
    void SplitBrick(SS& scene, SS::Brick* brick, UInt32 refID);
    bool ReadPart(const XMLDOMNodeAString& node, SS::Brick* brick);             // Mac 0x318ec0, Win 0x4a58d0
    // Comma separated numbers; `leading` zeros are inserted first (decoration: 1).
    // Mac 0x319eb0, Win 0x4ada10 (ECX = node, EDX = name). (name inferred)
    bool ReadNumberList(const XMLDOMNodeAString& node, const AString& name, BasicArray<UInt32>& values,
                        UInt32 leading);
    // Corrections for parts saved with older brick sets (BrickSet version <= 68 / <= 279) and
    // the flex bones of the part's primitive. Mac 0x31ac00, Win 0x4a7ae0 (EBX = part).
    // (name inferred)
    void FixPart(SS::Part* part);
    bool ReadBone(const XMLDOMNodeAString& node, SS::Part* part);               // Mac 0x31a7b0, Win 0x4a5cc0
    // <Behavior BehaviorName=…> with <Attribute Key=… Value=…/> children; other elements are
    // accepted (true). Mac 0x31b400, Win 0x4a5ed0 (Win loops over the children in 0x4a5ea0).
    bool ReadBehavior(const XMLDOMNodeAString& node, BasicArray<SS::Behavior>& behaviors);
    static bool ReadBehaviorAttribute(const XMLDOMNodeAString& node, BasicArray<SS::Attribute>& attributes); // Mac 0x31b600, Win 0x4a6090
    bool ReadBehaviors(const XMLDOMNodeAString& node, BasicArray<SS::Behavior>& behaviors); // Win 0x4a5ea0 (inlined on Mac)
    bool ReadGroupSystems(const XMLDOMNodeAString& root, SS& scene, bool ignoreJoints);      // Mac 0x317480, Win 0x4a6280
    bool ReadGroupSystem(const XMLDOMNodeAString& node, SS& scene, bool ignoreJoints);       // Mac 0x31b8e0, Win 0x4a63b0
    bool ReadGroup(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::Group> >& groups, bool ignoreJoints); // Mac 0x31bac0, Win 0x4a64e0
    bool ReadRigidSystems(const XMLDOMNodeAString& root, SS& scene, bool ignoreJoints);      // Mac 0x317660, Win 0x4a6700
    bool ReadRigidSystem(const XMLDOMNodeAString& node, SS& scene, bool ignoreJoints);       // Mac 0x31c730, Win 0x4a6830
    bool ReadRigid(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::Rigid> >& rigids); // Mac 0x31c9b0, Win 0x4a69d0
    bool ReadJoint(const XMLDOMNodeAString& node, BasicArray<SS::Joint>& joints);             // Mac 0x31ce60, Win 0x4a6be0
    bool ReadBuildingInstructions(const XMLDOMNodeAString& root, SS& scene, bool ignoreJoints); // Mac 0x317840, Win 0x4a7180
    bool ReadBuildingInstruction(const XMLDOMNodeAString& node, SS& scene, bool ignoreJoints);  // Mac 0x31dea0, Win 0x4a7290
    bool ReadStep(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::BIEvent> >& events, bool ignoreJoints); // Mac 0x31e250, Win 0x4a7440
    bool ReadStepCamera(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::BIEvent> >& events); // Mac 0x31e7b0, Win 0x4a7720
    bool ReadPartRef(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::BIEvent> >& events);   // Mac 0x31eae0, Win 0x4a7860

    Version                     m_brickSetVersion;   // +0x1c <Meta><BrickSet version=…> (name inferred)
    // NOTE: a RefPointer<BrickDatabase> in the original (reference taken in the constructor,
    // released in the destructor); a plain pointer here (restoration glue).
    BrickDatabase*              m_database;          // +0x2c
    // refID -> element of the file being read (not reference counted)
    BasicMap<UInt32, SS::Camera*> m_cameras;         // Mac +0x30 / Win +0x30
    BasicMap<UInt32, SS::Brick*>  m_bricks;          // Mac +0x4c / Win +0x54
    BasicMap<UInt32, SS::Part*>   m_parts;           // Mac +0x68 / Win +0x78
    BasicMap<UInt32, SS::Bone*>   m_bones;           // Mac +0x84 / Win +0x9c
    BasicMap<UInt32, SS::Rigid*>  m_rigids;          // Mac +0xa0 / Win +0xc0
};

// ---------------------------------------------------------------------------------------------
// LXFML5Tester — rejects documents whose root is not <LXFML versionMajor="5" versionMinor="0">
// while they are parsed (the errors go to the loader). sizeof 0x0c.
// Mac vtable 0x777448 {D1 0x324220, D0 0x324280, Object …, Test 0x3242f0};
// Win vtable 0xa24650 {sdd 0x4a4500, Object …, Test 0x4a42a0}.
// ---------------------------------------------------------------------------------------------
class LXFML5Tester : public XMLDOMNodeAStringTester {
public:
    // Win 0x4a4230 (this on the stack, loader in EDI); inlined on Mac (0x315b00).
    explicit LXFML5Tester(ModelLoaderLXFML5* loader) : m_loader(loader) {}
    virtual ~LXFML5Tester() {}                                  // Mac 0x324220 / 0x324280, Win 0x4a4490 (sdd 0x4a4500)
    virtual bool Test(const XMLDOMNodeAString* root);           // Mac 0x3242f0, Win 0x4a42a0

private:
    RefPointer<ModelLoaderLXFML5> m_loader;                     // +0x08
};

// ---------------------------------------------------------------------------------------------
// ModelSaverLXFML5. sizeof Mac 0xb8 / Win 0xc8.
// Mac vtable 0x74b658 {D1 0x321ba0, D0 0x321bb0, Object …, Save 0x321740, Save 0x321990};
// Win vtable 0xa2460c {sdd 0x4a8ac0, Object …, Save 0x4a9300, Save 0x4a9530}.
// refIDs are assigned in writing order (0, 1, 2 … per element kind); references to elements
// that were not written come out as 0.
// ---------------------------------------------------------------------------------------------
class ModelSaverLXFML5 : public ModelSaver {
public:
    ModelSaverLXFML5();                                         // Mac 0x31f0e0 (C1 0x31f2d0), Win 0x4a89e0
    virtual ~ModelSaverLXFML5();                                // Mac 0x3219a0 (D1 0x321ba0 / D0 0x321bb0), Win 0x4a9540 (sdd 0x4a8ac0)

    // Writes Write()'s text to `path` (created / truncated). On failure GetErrorMessage() says
    // "Unable to open file: <path>" or "Unable to write to file: <path>".
    virtual bool Save(const Path& path, const StorageScene2& scene);   // Mac 0x321740, Win 0x4a9300
    // Vtable slot 5, `return false` (Win shares the body 0x4a9530 with other `return false`
    // two-argument functions). ModelSaverStorageScene leaves this slot pure and its Save
    // (StorageScene2) converts to a StorageScene, so the slot is most likely declared by
    // ModelSaver as Save(const Path&, const StorageScene&). (name and parameter types inferred)
    virtual bool Save(const Path& path, const StorageScene& scene);    // Mac 0x321990, Win 0x4a9530

    // The whole document, UTF-8 encoded, "\r\n" line ends. Mac 0x31f2e0, Win 0x4a8ae0
    // (thiscall(out, scene)). (name inferred)
    void Write(AString& out, const StorageScene2& scene);

    const String& GetErrorMessage() const { return m_errorMessage; }   // (accessor, inferred)

private:
    typedef StorageScene2 SS;

    // ` name="value"` with the value XML-escaped (String::ToXML). Mac 0x31ffa0, Win 0x4a4190.
    void WriteAttribute(const String& name, const String& value);
    // ` name="x,y,z"`. Win 0x4ab460 (inlined on Mac).
    void WriteVector(const String& name, const Vector3& v);
    // ` transformation="…"`: the 3x4 part, column by column. Mac 0x321bf0, Win 0x4ab4e0 (ESI = m).
    void WriteTransformation(const Matrix3D& m);
    // `values` from index `start` joined with ','. Mac 0x322230, Win 0x4ae0f0.
    void WriteNumberList(const String& name, const BasicArray<UInt32>& values, UInt32 start);
    // refIDs of `items` from index `start` joined with ','; `ids` is a copy (by value in the
    // original: Mac 0x323380 bones / 0x3238d0 parts, Win 0x4ae230 / 0x4ae460), so unknown items
    // are written as 0 without changing the saver's maps.
    // NOTE: the copy of the whole map for every rigid and group makes a save quadratic (a model of
    // 524,288 bricks in 2,048 rigids copies a 524,288-entry map 2,048 times: minutes, the window
    // not responding); fixed: the map is passed by reference and looks each item up (unknown
    // ones still 0, the maps unchanged): the same text.
    template <class T>
    void WriteRefList(const String& name, const BasicArray<RefPointer<T> >& items,
                      const BasicMap<const T*, UInt32>& ids, UInt32 start);

    void WriteMetas(UInt32 indent);                                     // Mac 0x320050, Win 0x4a9720
    void WriteCameras(UInt32 indent);                                   // Mac 0x320280, Win 0x4a9800
    void WriteBehaviors(UInt32 indent, const BasicArray<SS::Behavior>& behaviors); // Mac 0x320750, Win 0x4aa180
    void WriteBricks(UInt32 indent);                                    // Mac 0x320b30, Win 0x4a9a70
    void WriteParts(UInt32 indent, const SS::Brick& brick);             // Mac 0x321db0, Win 0x4a9ce0
    void WriteBones(UInt32 indent, const SS::Part& part);               // Mac 0x322300, Win 0x4a9f90
    void WriteRigidSystem(UInt32 indent, const SS::RigidSystem& system); // Mac 0x320f50, Win 0x4aa460
    void WriteRigid(UInt32 indent, const SS::Rigid* rigid);             // Mac 0x322650, Win 0x4aa5e0
    void WriteJoint(UInt32 indent, const SS::Joint& joint);             // Mac 0x322920, Win 0x4aa730
    void WriteGroupSystem(UInt32 indent, const SS::GroupSystem& system); // Mac 0x321190, Win 0x4aab50
    void WriteGroup(UInt32 indent, const SS::Group* group);             // Mac 0x3234e0, Win 0x4aac80
    void WriteBuildingInstructions(UInt32 indent);                      // Mac 0x321300, Win 0x4aaec0
    void WriteEvents(const BasicArray<RefPointer<SS::BIEvent> >& events, UInt32 indent); // Mac 0x323a30, Win 0x4ab0b0

    String                           m_errorMessage;   // Mac +0x08 / Win +0x08 (name inferred)
    String                           m_text;           // Mac +0x18 / Win +0x0c document being built
    RefPointer<SS>                   m_scene;          // Mac +0x28 / Win +0x10 scene being written
    // element -> refID written
    BasicMap<const SS::Camera*, UInt32> m_cameraIDs;  // Mac +0x2c / Win +0x14
    BasicMap<const SS::Brick*, UInt32>  m_brickIDs;   // Mac +0x48 / Win +0x38
    BasicMap<const SS::Part*, UInt32>   m_partIDs;    // Mac +0x64 / Win +0x5c
    BasicMap<const SS::Bone*, UInt32>   m_boneIDs;    // Mac +0x80 / Win +0x80
    BasicMap<const SS::Rigid*, UInt32>  m_rigidIDs;   // Mac +0x9c / Win +0xa4
};

} // namespace LEGO
