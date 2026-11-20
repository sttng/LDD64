// LEGO Digital Designer — restored source
// LXFML 5 reader / writer (ModelLoaderLXFML5, ModelSaverLXFML5, LXFML5Tester). See the header.
// Plan items 2.3 / 2.4, Mac TU 342 (sub-TUs 342a, 342b, 342c).
#include "ModelIOLXFML5.h"
#include "BrickDatabase.h"
#include "XMLDOMAttribute.h"
#include "StringStream.h"
#include "MathUtil.h"
#include "LEGO/StdLib.h"
#include "Path.h"
#include "File.h"

#include <iostream>

namespace LEGO {

typedef StorageScene2 SS;

// ============================================================================================
// file-local helpers (inline / file-static in the original)
// ============================================================================================
namespace {

// Element name == `name` (exact; the original inlines a std::string compare: StrNCmp over the
// shorter length plus a length check, Win strncmp).
bool IsNamed(const XMLDOMNodeAString& node, const char* name)
{
    const AString& n = node.GetName();
    UInt32 length = StrLen(name);
    return n.GetLength() == length && (length == 0 || StrNCmp(n.GetBuffer(), name, length) == 0);
}

bool Equals(const AString& a, const char* b)
{
    UInt32 length = StrLen(b);
    return a.GetLength() == length && (length == 0 || StrNCmp(a.GetBuffer(), b, length) == 0);
}

// Splits at every `separator`; a trailing separator adds no empty part. Mac: the XMLDOM helper
// 0x22de30 (TU 171, file-static in the restored XMLDOMNode.cpp, so repeated here); Win inlines
// it as 0x469a50. (name inferred)
void SplitAString(const AString& s, char separator, BasicArray<AString>& out)
{
    out.Clear();
    UInt32 start = 0;
    UInt32 length = s.GetLength();
    while (start < length) {
        const char* p = static_cast<const char*>(MemChr(s.GetBuffer() + start, separator, length - start));
        if (!p)
            break;
        UInt32 pos = (UInt32)(p - s.GetBuffer());
        out.PushBack(AString(s.GetBuffer() + start, pos - start));
        start = pos + 1;
    }
    if (start < length)
        out.PushBack(AString(s.GetBuffer() + start, length - start));
}

// A three-component attribute: name="x,y,z" (one arbitrary separator character between the
// numbers), otherwise the separate attributes namex / namey / namez. Missing or unparseable
// components keep their value. Mac 0x31bf50, Win 0x4a8730 (ECX = node, EDX = name).
// (name inferred)
void ReadVector(const XMLDOMNodeAString& node, const AString& name, Vector3& v)
{
    AString text;
    if (!node.GetAttribute(name, text)) {
        AString x(name);
        x.Append("x", 1);
        GetXMLAttribute(node, x, v.x);
        AString y(name);
        y.Append("y", 1);
        GetXMLAttribute(node, y, v.y);
        AString z(name);
        z.Append("z", 1);
        GetXMLAttribute(node, z, v.z);
        return;
    }
    Real32* f = v;
    UInt32 position = 0;
    for (UInt32 i = 0; i < 3; ++i) {
        FromString(f[i], String(text), position);
        if (text.GetLength() != 0 && position < text.GetLength())
            ++position;
    }
}

// transformation="12 numbers" (the 3x4 part of the column-major matrix, column by column; the
// fourth row becomes 0 0 0 1). Without it: angle (degrees) / a (axis, default 0,1,0) / t
// (translation). A matrix that RemoveScaling cannot normalise is replaced by its translation.
// Mac 0x317df0, Win 0x4a8450 (ECX = node). (name inferred)
void ReadTransformation(const XMLDOMNodeAString& node, Matrix3D& m)
{
    AString text;
    if (node.GetAttribute(AString("transformation"), text)) {
        Real32* f = m;
        UInt32 position = 0;
        for (UInt32 column = 0; column < 4; ++column, f += 4) {
            for (UInt32 row = 0; row < 3; ++row) {
                FromString(f[row], String(text), position);
                if (text.GetLength() != 0 && position < text.GetLength())
                    ++position;
            }
            f[3] = 0.0f;
        }
        m.m[15] = 1.0f;
    } else {
        Vector3 axis(0.0f, 1.0f, 0.0f);
        Real32  angle = 0.0f;
        Vector3 translation(0.0f, 0.0f, 0.0f);
        Vector3 center;
        GetXMLAttribute(node, AString("angle"), angle);
        ReadVector(node, AString("a"), axis);
        ReadVector(node, AString("t"), translation);
#if defined(_WIN32)
        // x87: (PI / 180.0) * angle in double precision (Win 0x4a869b)
        angle = (Real32)((PI / 180.0) * angle);
#else
        angle = angle * (PI / 180.0f);        // Mac: mulss by the constant 0.0174533f
#endif
        // Win 0x4a2ce0 (inline on Mac)
        if (!axis.IsZero() && angle != 0.0f) {
            m.SetRotation(axis, angle, translation, center);
        } else {
            m.SetIdentity();
            m.SetTranslation(translation);
        }
    }
    if (!m.RemoveScaling()) {
        Vector3 translation(m.GetTranslation());
        m.SetTranslationMatrix(translation);
    }
}

// Design IDs that were renumbered in later brick sets. Win 0x4a8060 (ECX = &designID), inlined
// twice on Mac (0x318c8e, 0x31965a). (name inferred)
void UpdateDesignID(UInt32& designID)
{
    switch (designID) {
    case 30007: designID = 4215;  break;   // 0x7537 -> 0x1077
    case 32239: designID = 6588;  break;   // 0x7def -> 0x19bc
    case 41070: designID = 32008; break;   // 0xa06e -> 0x7d08
    case 45173: designID = 42060; break;   // 0xb075 -> 0xa44c
    case 46212: designID = 2454;  break;   // 0xb484 -> 0x996
    case 47577: designID = 93565; break;   // 0xb9d9 -> 0x16d7d
    case 52035: designID = 89536; break;   // 0xcb43 -> 0x15dc0
    case 57722: designID = 71986; break;   // 0xe17a -> 0x11932
    case 59577: designID = 57558; break;   // 0xe8b9 -> 0xe0d6
    case 60677: designID = 55676; break;   // 0xed05 -> 0xd97c
    case 60700: designID = 6015;  break;   // 0xed1c -> 0x177f
    case 61988: designID = 53788; break;   // 0xf224 -> 0xd21c
    case 63021: designID = 62699; break;   // 0xf62d -> 0xf4eb
    case 63153: designID = 61804; break;   // 0xf6b1 -> 0xf16c
    case 64871: designID = 32278; break;   // 0xfd67 -> 0x7e16
    case 82610: designID = 4034;  break;   // 0x142b2 -> 0xfc2
    case 86209: designID = 60601; break;   // 0x150c1 -> 0xecb9
    case 86210: designID = 60603; break;   // 0x150c2 -> 0xecbb
    case 88513: designID = 53451; break;   // 0x159c1 -> 0xd0cb
    case 88701: designID = 87777; break;   // 0x15a7d -> 0x156e1
    case 89500: designID = 85961; break;   // 0x15d9c -> 0x14fc9
    default: break;
    }
}

// Removes the first element equal to `value` by moving the last element into its place.
// Mac 0x31ef70, Win 0x4aeaf0 (after a std::find). (name inferred)
template <class T>
void EraseUnordered(BasicArray<RefPointer<T> >& array, T* value)
{
    for (typename BasicArray<RefPointer<T> >::Iterator it = array.Begin(); it != array.End(); ++it) {
        if (it->Get() == value) {
            if (it != &array.Back())
                *it = array.Back();
            array.PopBack();
            return;
        }
    }
}

// refIDs="a,b,c": the referenced elements (null for numbers that do not parse or name no
// element, the result is then false), after `leading` null entries. A missing or empty
// attribute is not an error. Mac 0x31c340 (parts) / 0x31dab0 (bones), Win 0x4adc00 (one body).
// (name inferred)
template <class T>
bool ReadRefList(const XMLDOMNodeAString& node, const AString& name, BasicArray<RefPointer<T> >& refs,
                 const BasicMap<UInt32, T*>& elements, UInt32 leading)
{
    AString text;
    if (!node.GetAttribute(name, text))
        return true;
    bool result = true;
    BasicArray<AString> tokens;
    SplitAString(text, ',', tokens);
    if (text.GetLength() != 0) {
        for (UInt32 i = 0; i < leading; ++i)
            refs.PushBack(RefPointer<T>());
        for (BasicArray<AString>::Iterator it = tokens.Begin(); it != tokens.End(); ++it) {
            UInt32 refID;
            if (!FromString(refID, String(*it))) {
                refs.PushBack(RefPointer<T>());
                result = false;
                continue;
            }
            typename BasicMap<UInt32, T*>::ConstIterator found = elements.Find(refID);
            if (found == elements.End()) {
                refs.PushBack(RefPointer<T>());
                result = false;
                continue;
            }
            refs.PushBack(RefPointer<T>(found->second));
        }
    }
    return result;
}

// Joint type names, indexed by StorageScene2::Joint::Type (Mac 0x777438, Win 0xae1310).
const char* const kJointTypes[2] = { "hinge", "ball" };

} // namespace

// ============================================================================================
// ModelLoaderLXFML5
// ============================================================================================

ModelLoaderLXFML5::ModelLoaderLXFML5(BrickDatabase* database)               // Mac 0x315860, Win 0x4a4520
    : m_database(database)
{
}

ModelLoaderLXFML5::~ModelLoaderLXFML5()                                     // Mac 0x317a20, Win 0x4a4980
{
}

void ModelLoaderLXFML5::Reset()                                             // Mac 0x315fc0, Win 0x4a4a40
{
    m_errors.Clear();
    m_errorMask = 0;
    m_cameras.Clear();
    m_bricks.Clear();
    m_parts.Clear();
    m_bones.Clear();
    m_rigids.Clear();
}

bool ModelLoaderLXFML5::Load(const AString& content, StorageScene2& scene, bool ignoreJoints) // Mac 0x315b00, Win 0x4a4630
{
    Reset();
    scene.Clear();

    // NOTE: the tester holds a reference to this loader while parsing, so the loader must be
    // held by a RefPointer (a loader with a reference count of 0 deletes itself here).
    RefPointer<XMLDOMAString> document = new XMLDOMAString();
    RefPointer<LXFML5Tester> tester = new LXFML5Tester(this);
    if (!document->Parse(content, tester)) {
        // the tester's errors (version / not LXFML) take precedence
        if (m_errorMask == 0)
            AddError(new ParserError);
        return false;
    }
    if (!ReadRoot(*document, scene))
        return false;

    ReadMeta(*document, scene);
    bool result = ReadCameras(*document, scene);
    const XMLDOMNodeAString* behaviors = document->FindChild(AString("Behaviors"));
    if (behaviors) {
        if (!ReadBehaviors(*behaviors, scene.m_behaviors))
            result = false;
    }
    if (!ReadBricks(*document, scene))
        return false;
    bool groups = ReadGroupSystems(*document, scene, ignoreJoints);
    bool rigids = ReadRigidSystems(*document, scene, ignoreJoints);
    bool instructions = ReadBuildingInstructions(*document, scene, ignoreJoints);
    return result && groups && rigids && instructions;
}

bool ModelLoaderLXFML5::CheckRoot(const XMLDOMNodeAString& root)            // (inline: Mac 0x316110 / 0x3242f0)
{
    if (!IsNamed(root, "LXFML")) {
        AddError(new ContentError(ContentError::missingLXFMLTag));
        return false;
    }
    Version expected(5, 0, 0, 0);
    Version actual;
    GetXMLAttribute(root, AString("versionMajor"), actual.m_major);
    GetXMLAttribute(root, AString("versionMinor"), actual.m_minor);
    if (expected != actual) {
        AddError(new VersionError(expected, actual));
        return false;
    }
    return true;
}

bool ModelLoaderLXFML5::ReadRoot(const XMLDOMNodeAString& root, SS& scene)  // Mac 0x316110, Win 0x4a4b20
{
    if (!CheckRoot(root))
        return false;
    root.GetAttribute(AString("name"), scene.m_name);
    return true;
}

void ModelLoaderLXFML5::ReadMeta(const XMLDOMNodeAString& root, SS& scene)  // Mac 0x3165a0, Win 0x4a4d40
{
    const XMLDOMNodeAString* meta = root.FindChild(AString("Meta"));
    if (!meta)
        return;
    const XMLDOMNodeAString::NodeArray& children = meta->GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        const XMLDOMNodeAString& child = **it;
        SS::Meta entry;
        child.GetName(entry.m_name);
        child.GetText(entry.m_text);
        const XMLDOMNodeAString::AttributeArray& attributes = child.GetAttributes();
        for (XMLDOMNodeAString::AttributeArray::ConstIterator a = attributes.Begin(); a != attributes.End(); ++a) {
            SS::Meta::Attribute attribute;
            meta->Decode(a->first, attribute.m_name);
            meta->Decode(a->second, attribute.m_value);
            entry.m_attributes.PushBack(attribute);
            if (entry.m_name == String("BrickSet") && attribute.m_name == String("version")) {
                UInt32 position = 0;
                FromString(m_brickSetVersion, attribute.m_value, position);
            }
        }
        scene.m_metas.PushBack(entry);
    }
}

bool ModelLoaderLXFML5::ReadCameras(const XMLDOMNodeAString& root, SS& scene) // Mac 0x3169a0, Win 0x4a50a0
{
    bool result = true;
    const XMLDOMNodeAString* cameras = root.FindChild(AString("Cameras"));
    if (!cameras)
        return true;
    const XMLDOMNodeAString::NodeArray& children = cameras->GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        const XMLDOMNodeAString& node = **it;
        if (!IsNamed(node, "Camera"))
            continue;
        UInt32 refID;
        if (!GetXMLAttribute(node, AString("refID"), refID)) {
            AddError(new CameraError(CameraError::missingRefID));
            result = false;
            continue;
        }
        if (m_cameras.Contains(refID)) {
            AddError(new CameraError(CameraError::reusedRefID));
            result = false;
            continue;
        }
        RefPointer<SS::Camera> camera = new SS::Camera;
        node.GetAttribute(AString("name"), camera->m_name);
        GetXMLAttribute(node, AString("distance"), camera->m_distance);
        GetXMLAttribute(node, AString("fieldOfView"), camera->m_fieldOfView);
        // NOTE: "orthographic" is read into the field of view (both builds); the saver writes
        // orthographic="true", which does not parse as a number.
        GetXMLAttribute(node, AString("orthographic"), camera->m_fieldOfView);
        ReadTransformation(node, camera->m_transformation);
        m_cameras[refID] = camera;
        scene.m_cameras.PushBack(camera);
    }
    return result;
}

bool ModelLoaderLXFML5::ReadCameraRef(const XMLDOMNodeAString& node, RefPointer<SS::Camera>& camera) // Mac 0x318360, Win 0x4a88d0
{
    UInt32 refID;
    if (!GetXMLAttribute(node, AString("cameraRef"), refID))
        return true;
    BasicMap<UInt32, SS::Camera*>::Iterator it = m_cameras.Find(refID);
    if (it == m_cameras.End())
        return false;
    camera = it->second;
    return true;
}

bool ModelLoaderLXFML5::ReadBricks(const XMLDOMNodeAString& root, SS& scene) // Mac 0x3171d0, Win 0x4a5420
{
    const XMLDOMNodeAString* bricks = root.FindChild(AString("Bricks"));
    if (!bricks)
        return false;   // NOTE: no error is added
    if (!ReadCameraRef(*bricks, scene.m_camera))
        AddError(new CameraError(CameraError::nonExistingCamera));   // NOTE: does not fail the load
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = bricks->GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "Brick") && !ReadBrick(**it, scene))
            result = false;
    }
    return result;
}

bool ModelLoaderLXFML5::ReadBrick(const XMLDOMNodeAString& node, SS& scene)  // Mac 0x3184c0, Win 0x4a5590
{
    UInt32 refID;
    if (!GetXMLAttribute(node, AString("refID"), refID)) {
        AddError(new BrickError(BrickError::missingRefID));
        return false;
    }
    if (m_bricks.Contains(refID)) {
        AddError(new BrickError(BrickError::reusedRefID));
        return false;
    }
    UInt32 designID;
    if (!GetXMLAttribute(node, AString("designID"), designID) || designID == 0) {
        AddError(new BrickError(BrickError::missingDesignID));
        return false;
    }
    RefPointer<SS::Brick> brick = new SS::Brick;
    brick->m_designID = designID;
    node.GetAttribute(AString("itemNos"), brick->m_itemNos);
    m_bricks[refID] = brick;
    scene.m_bricks.PushBack(brick);

    bool result = ReadBehaviors(node, brick->m_behaviors);
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "Part") && !ReadPart(**it, brick))
            result = false;
    }
    UpdateDesignID(brick->m_designID);
    SplitBrick(scene, brick, refID);
    return result;
}

void ModelLoaderLXFML5::SplitBrick(SS& scene, SS::Brick* brick, UInt32 refID) // Mac 0x319b70, Win 0x4a81b0
{
    if (brick->m_designID != 74340)
        return;
    RefPointer<SS::Brick> first = new SS::Brick;
    first->m_designID = 3679;
    scene.m_bricks.PushBack(first);
    RefPointer<SS::Brick> second = new SS::Brick;
    second->m_designID = 3680;
    scene.m_bricks.PushBack(second);
    // NOTE: the behaviour array is assigned once per behaviour (both builds).
    for (UInt32 i = 0; i < brick->m_behaviors.GetSize(); ++i) {
        if (first.Get() != brick)
            first->m_behaviors = brick->m_behaviors;
        if (second.Get() != brick)
            second->m_behaviors = brick->m_behaviors;
    }
    // NOTE: the moved parts keep m_brick pointing at the removed brick (not reference
    // counted: it dangles once the brick is released); fixed: they point at their new brick,
    // and the parts that are not moved at nothing (H12).
    for (UInt32 i = 0; i < brick->m_parts.GetSize(); ++i) {
        UInt32 designID = brick->m_parts[i]->m_designID;
        brick->m_parts[i]->m_brick = 0;
        if (designID == 3680) {
            second->m_parts.PushBack(brick->m_parts[i]);
            brick->m_parts[i]->m_brick = second;
        } else if (designID == 3679) {
            first->m_parts.PushBack(brick->m_parts[i]);
            brick->m_parts[i]->m_brick = first;
        }
    }
    m_bricks.Erase(refID);
    RefPointer<SS::Brick> keep = brick;
    EraseUnordered(scene.m_bricks, brick);
}

bool ModelLoaderLXFML5::ReadNumberList(const XMLDOMNodeAString& node, const AString& name,
                                       BasicArray<UInt32>& values, UInt32 leading) // Mac 0x319eb0, Win 0x4ada10
{
    AString text;
    if (!node.GetAttribute(name, text))
        return true;
    bool result = true;
    BasicArray<AString> tokens;
    SplitAString(text, ',', tokens);
    if (text.GetLength() != 0) {
        for (UInt32 i = 0; i < leading; ++i)
            values.PushBack(0);
        for (BasicArray<AString>::Iterator it = tokens.Begin(); it != tokens.End(); ++it) {
            UInt32 value;
            if (FromString(value, String(*it))) {
                values.PushBack(value);
            } else {
                values.PushBack(0);
                result = false;
            }
        }
    }
    return result;
}

bool ModelLoaderLXFML5::ReadPart(const XMLDOMNodeAString& node, SS::Brick* brick) // Mac 0x318ec0, Win 0x4a58d0
{
    UInt32 refID;
    if (!GetXMLAttribute(node, AString("refID"), refID)) {
        AddError(new PartError(0, PartError::missingRefID));
        return false;
    }
    if (m_parts.Contains(refID)) {
        AddError(new PartError(0, PartError::reusedRefID));
        return false;
    }
    UInt32 designID;
    if (!GetXMLAttribute(node, AString("designID"), designID) || designID == 0) {
        AddError(new PartError(0, PartError::missingDesignID));
        return false;
    }
    RefPointer<SS::Part> part = new SS::Part(designID);
    if (!ReadNumberList(node, AString("materials"), part->m_materials, 0))
        AddError(new PartError(part, PartError::wrongMaterial));
    if (part->m_materials.IsEmpty()) {
        AddError(new PartError(part, PartError::missingMaterial));
        return false;
    }
    if (!ReadNumberList(node, AString("decoration"), part->m_decorations, 1))
        AddError(new PartError(part, PartError::wrongDecoration));
    m_parts[refID] = part;
    part->m_brick = brick;
    brick->m_parts.PushBack(part);

    bool result = ReadBehaviors(node, part->m_behaviors);
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "Bone") && !ReadBone(**it, part))
            result = false;
    }
    UpdateDesignID(part->m_designID);
    FixPart(part);
    return result;
}

bool ModelLoaderLXFML5::ReadBone(const XMLDOMNodeAString& node, SS::Part* part) // Mac 0x31a7b0, Win 0x4a5cc0
{
    UInt32 refID;
    if (!GetXMLAttribute(node, AString("refID"), refID)) {
        AddError(new BoneError(BoneError::missingRefID));
        return false;
    }
    if (m_bones.Contains(refID)) {
        AddError(new BoneError(BoneError::reusedRefID));
        return false;
    }
    RefPointer<SS::Bone> bone = new SS::Bone;
    m_bones[refID] = bone;
    bone->m_part = part;
    part->m_bones.PushBack(bone);
    ReadTransformation(node, bone->m_transformation);
    return ReadBehaviors(node, bone->m_behaviors);
}

void ModelLoaderLXFML5::FixPart(SS::Part* part)                             // Mac 0x31ac00, Win 0x4a7ae0
{
    if (m_brickSetVersion.m_major == 0)
        return;   // no <BrickSet version>: nothing to correct (not even the flex bones)
    if (part->m_designID == 46212)
        part->m_designID = 2454;   // NOTE: already done by UpdateDesignID

    // NOTE: the corrections below use the first bone without checking that there is one;
    // fixed: a part without bones is not corrected (H15).
    if (part->m_bones.IsEmpty())
        return;
    if (m_brickSetVersion <= Version(68) && (part->m_designID == 32059 || part->m_designID == 30249)) {
        Matrix3D correction;
        correction.SetTranslation(Vector3(0.0f, 0.0f, -0.4f));
        part->m_bones[0]->m_transformation *= correction;
    }
    if (m_brickSetVersion <= Version(279)) {
        Matrix3D correction;
        bool apply = true;
        switch (part->m_designID) {
        case 3896:  correction.SetTranslation(Vector3(0.0f, 0.96f, 0.0f)); break;
        case 6088:  correction.SetTranslation(Vector3(0.0f, 0.105f, 0.0f)); break;
        case 6131:
            correction.RotateX((Real32)((PI / 180.0) * 15.0));
            correction.SetTranslation(Vector3(0.0f, 0.367f, 0.097f));
            break;
        case 30105: correction.SetTranslation(Vector3(0.0f, -0.11f, 0.0f)); break;
        case 30114: correction.SetTranslation(Vector3(0.0f, 0.174f, 0.0f)); break;
        case 30124: correction.SetTranslation(Vector3(0.0f, 0.1285f, 0.0f)); break;
        case 30138: correction.SetTranslation(Vector3(0.0f, 0.204f, 0.0f)); break;
        case 30167: correction.SetTranslation(Vector3(0.0f, 0.239f, 0.0f)); break;
        case 30172:
            correction.RotateX((Real32)((PI / 180.0) * 9.0));
            correction.SetTranslation(Vector3(0.0f, 0.193f, 0.05f));
            break;
        case 30273: correction.SetTranslation(Vector3(0.0f, 0.96f, 0.0f)); break;
        case 30325: correction.SetTranslation(Vector3(0.0f, 0.263f, 0.0f)); break;
        case 30408: correction.SetTranslation(Vector3(0.0f, 0.27f, 0.0f)); break;
        case 40238: correction.SetTranslation(Vector3(0.0f, 0.17f, 0.0f)); break;
        case 71966: correction.SetTranslation(Vector3(0.0f, 0.128f, 0.0f)); break;
        case 85945: correction.SetTranslation(Vector3(0.0f, 0.14f, 0.002f)); break;
        case 87556: correction.SetTranslation(Vector3(0.0f, -0.054f, 0.0f)); break;
        default:    apply = false; break;
        }
        if (apply)
            part->m_bones[0]->m_transformation *= correction;
    }

    // Flex parts: one bone in the file, several bones in the primitive -> one bone per
    // primitive bone (the file's bone times the primitive bone's transformation). The new bones
    // get no refID. The primitive is held for the loop (BrickDatabase::GetPrimitive, Mac 0x81c30
    // / Win 0x4b9180, AddRef ... Release in both builds).
    if (m_database) {
        RefPointer<const Primitive> primitive = m_database->GetPrimitive(part->m_designID);
        if (primitive.Get()) {
            const BasicArray<Primitive::Bone>& bones = primitive->GetBones();
            if (bones.GetSize() > 1 && part->m_bones.GetSize() == 1) {
                Matrix3D base(part->m_bones[0]->m_transformation);
                for (UInt32 i = 0; i < bones.GetSize(); ++i) {
                    Matrix3D m(base, bones[i].m_transform);
                    if (i == 0) {
                        part->m_bones[0]->m_transformation = m;
                    } else {
                        RefPointer<SS::Bone> bone = new SS::Bone;
                        bone->m_transformation = m;
                        bone->m_part = part;
                        part->m_bones.PushBack(bone);
                    }
                }
            }
        }
    }
}

bool ModelLoaderLXFML5::ReadBehaviors(const XMLDOMNodeAString& node, BasicArray<SS::Behavior>& behaviors) // Win 0x4a5ea0
{
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (!ReadBehavior(**it, behaviors))
            result = false;
    }
    return result;
}

bool ModelLoaderLXFML5::ReadBehavior(const XMLDOMNodeAString& node, BasicArray<SS::Behavior>& behaviors) // Mac 0x31b400, Win 0x4a5ed0
{
    if (!IsNamed(node, "Behavior"))
        return true;
    behaviors.PushBack(SS::Behavior());
    SS::Behavior& behavior = behaviors.Back();
    bool named = false;
    const XMLDOMNodeAString::AttributeArray& attributes = node.GetAttributes();
    for (XMLDOMNodeAString::AttributeArray::ConstIterator a = attributes.Begin(); a != attributes.End(); ++a) {
        if (Equals(a->first, "BehaviorName")) {
            node.Decode(a->second, behavior.m_name);
            named = true;
        }
    }
    // NOTE: a <Behavior> without BehaviorName stays in the array (empty) and fails the load.
    if (!named)
        return false;
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (!ReadBehaviorAttribute(**it, behavior.m_attributes))
            result = false;
    }
    return result;
}

bool ModelLoaderLXFML5::ReadBehaviorAttribute(const XMLDOMNodeAString& node, BasicArray<SS::Attribute>& attributes) // Mac 0x31b600, Win 0x4a6090
{
    if (!IsNamed(node, "Attribute"))
        return true;
    SS::Attribute attribute;
    const XMLDOMNodeAString::AttributeArray& list = node.GetAttributes();
    for (XMLDOMNodeAString::AttributeArray::ConstIterator a = list.Begin(); a != list.End(); ++a) {
        if (Equals(a->first, "Key"))
            node.Decode(a->second, attribute.m_key);
        if (Equals(a->first, "Value"))
            node.Decode(a->second, attribute.m_value);
    }
    if (attribute.m_key.IsEmpty())
        return false;
    attributes.PushBack(attribute);
    return true;
}

bool ModelLoaderLXFML5::ReadGroupSystems(const XMLDOMNodeAString& root, SS& scene, bool ignoreJoints) // Mac 0x317480, Win 0x4a6280
{
    const XMLDOMNodeAString* systems = root.FindChild(AString("GroupSystems"));
    if (!systems)
        return true;
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = systems->GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "GroupSystem") && !ReadGroupSystem(**it, scene, ignoreJoints))
            result = false;
    }
    return result;
}

bool ModelLoaderLXFML5::ReadGroupSystem(const XMLDOMNodeAString& node, SS& scene, bool ignoreJoints) // Mac 0x31b8e0, Win 0x4a63b0
{
    scene.m_groupSystems.PushBack(SS::GroupSystem());
    BasicArray<RefPointer<SS::Group> >& groups = scene.m_groupSystems.Back().m_groups;
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "Group") && !ReadGroup(**it, groups, ignoreJoints))
            result = false;
    }
    return result;
}

bool ModelLoaderLXFML5::ReadGroup(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::Group> >& groups,
                                  bool ignoreJoints)                         // Mac 0x31bac0, Win 0x4a64e0
{
    RefPointer<SS::Group> group = new SS::Group();
    groups.PushBack(group);
    node.GetAttribute(AString("name"), group->m_name);
    ReadTransformation(node, group->m_transformation);
    ReadVector(node, AString("pivot"), group->m_pivot);
    // NOTE: unknown parts and the nested groups' results do not fail the load
    ReadRefList(node, AString("partRefs"), group->m_parts, m_parts, 0);
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "Group"))
            ReadGroup(**it, group->m_groups, ignoreJoints);
    }
    return ReadBehaviors(node, group->m_behaviors);
}

bool ModelLoaderLXFML5::ReadRigidSystems(const XMLDOMNodeAString& root, SS& scene, bool ignoreJoints) // Mac 0x317660, Win 0x4a6700
{
    const XMLDOMNodeAString* systems = root.FindChild(AString("RigidSystems"));
    if (!systems)
        return true;
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = systems->GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "RigidSystem") && !ReadRigidSystem(**it, scene, ignoreJoints))
            result = false;
    }
    return result;
}

bool ModelLoaderLXFML5::ReadRigidSystem(const XMLDOMNodeAString& node, SS& scene, bool ignoreJoints) // Mac 0x31c730, Win 0x4a6830
{
    scene.m_rigidSystems.PushBack(SS::RigidSystem());
    SS::RigidSystem& system = scene.m_rigidSystems.Back();
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "Rigid") && !ReadRigid(**it, system.m_rigids))
            result = false;
    }
    if (!ignoreJoints) {
        for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
            if (IsNamed(**it, "Joint") && !ReadJoint(**it, system.m_joints))
                result = false;
        }
    }
    return result;
}

bool ModelLoaderLXFML5::ReadRigid(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::Rigid> >& rigids) // Mac 0x31c9b0, Win 0x4a69d0
{
    UInt32 refID;
    if (!GetXMLAttribute(node, AString("refID"), refID)) {
        AddError(new RigidError(RigidError::missingRefID));
        return false;
    }
    if (m_rigids.Contains(refID)) {
        AddError(new RigidError(RigidError::reusedRefID));
        return false;
    }
    RefPointer<SS::Rigid> rigid = new SS::Rigid;
    rigids.PushBack(rigid);
    m_rigids[refID] = rigid;
    ReadTransformation(node, rigid->m_transformation);
    // NOTE: unknown bones do not fail the load
    ReadRefList(node, AString("boneRefs"), rigid->m_bones, m_bones, 0);
    return true;
}

bool ModelLoaderLXFML5::ReadJoint(const XMLDOMNodeAString& node, BasicArray<SS::Joint>& joints) // Mac 0x31ce60, Win 0x4a6be0
{
    joints.PushBack(SS::Joint());
    SS::Joint& joint = joints.Back();
    node.GetAttribute(AString("name"), joint.m_name);
    String type;
    node.GetAttribute(AString("type"), type);
    if (type == String(kJointTypes[SS::Joint::Hinge])) {
        joint.m_type = SS::Joint::Hinge;
    } else if (type == String(kJointTypes[SS::Joint::Ball])) {
        joint.m_type = SS::Joint::Ball;
    } else {
        AddError(new JointError(0));
        joints.PopBack();
        return false;
    }
    // The first two <RigidRef>s; a RigidRef without a readable rigidRef still provides the
    // axis / zero / position vectors.
    bool first = true;
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        const XMLDOMNodeAString& ref = **it;
        if (!IsNamed(ref, "RigidRef"))
            continue;
        UInt32 refID;
        if (GetXMLAttribute(ref, AString("rigidRef"), refID)) {
            BasicMap<UInt32, SS::Rigid*>::Iterator found = m_rigids.Find(refID);
            if (found == m_rigids.End()) {
                AddError(new JointError(0));
                joints.PopBack();
                return false;
            }
            if (first)
                joint.m_rigidA = found->second;
            else
                joint.m_rigidB = found->second;
        }
        if (first) {
            ReadVector(ref, AString("a"), joint.m_axisA);
            ReadVector(ref, AString("z"), joint.m_zeroA);
            ReadVector(ref, AString("t"), joint.m_positionA);
            first = false;
        } else {
            ReadVector(ref, AString("a"), joint.m_axisB);
            ReadVector(ref, AString("z"), joint.m_zeroB);
            ReadVector(ref, AString("t"), joint.m_positionB);
            return true;
        }
    }
    return true;
}

bool ModelLoaderLXFML5::ReadBuildingInstructions(const XMLDOMNodeAString& root, SS& scene, bool ignoreJoints) // Mac 0x317840, Win 0x4a7180
{
    const XMLDOMNodeAString* instructions = root.FindChild(AString("BuildingInstructions"));
    if (!instructions)
        return true;
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = instructions->GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "BuildingInstruction") && !ReadBuildingInstruction(**it, scene, ignoreJoints))
            result = false;
    }
    return result;
}

bool ModelLoaderLXFML5::ReadBuildingInstruction(const XMLDOMNodeAString& node, SS& scene, bool ignoreJoints) // Mac 0x31dea0, Win 0x4a7290
{
    scene.m_buildingInstructions.PushBack(SS::BI());
    SS::BI& instruction = scene.m_buildingInstructions.Back();
    node.GetAttribute(AString("name"), instruction.m_name);
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "Step") && !ReadStep(**it, instruction.m_events, ignoreJoints))
            result = false;
        if (IsNamed(**it, "Camera") && !ReadStepCamera(**it, instruction.m_events))
            result = false;
    }
    return result;
}

bool ModelLoaderLXFML5::ReadStep(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::BIEvent> >& events,
                                 bool ignoreJoints)                          // Mac 0x31e250, Win 0x4a7440
{
    RefPointer<SS::BIStep> step = new SS::BIStep;
    events.PushBack(RefPointer<SS::BIEvent>(step));
    node.GetAttribute(AString("name"), step->m_name);
    GetXMLAttribute(node, AString("exploded"), step->m_exploded);
    if (step->m_exploded)
        ReadVector(node, AString("a"), step->m_explode);
    bool result = true;
    const XMLDOMNodeAString::NodeArray& children = node.GetChildren();
    for (XMLDOMNodeAString::NodeArray::ConstIterator it = children.Begin(); it != children.End(); ++it) {
        if (IsNamed(**it, "PartRef") && !ReadPartRef(**it, step->m_events))
            result = false;
        if (IsNamed(**it, "Step") && !ReadStep(**it, step->m_events, ignoreJoints))
            result = false;
        if (IsNamed(**it, "Camera") && !ReadStepCamera(**it, step->m_events))
            result = false;
    }
    return result;
}

bool ModelLoaderLXFML5::ReadStepCamera(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::BIEvent> >& events) // Mac 0x31e7b0, Win 0x4a7720
{
    RefPointer<SS::Camera> camera;
    if (!ReadCameraRef(node, camera)) {
        AddError(new BIError(BIError::nonExistingCamera));
        return false;
    }
    events.PushBack(RefPointer<SS::BIEvent>(new SS::BICamera(camera)));
    return true;
}

bool ModelLoaderLXFML5::ReadPartRef(const XMLDOMNodeAString& node, BasicArray<RefPointer<SS::BIEvent> >& events) // Mac 0x31eae0, Win 0x4a7860
{
    RefPointer<SS::BIPart> event = new SS::BIPart(RefPointer<SS::Part>());
    events.PushBack(RefPointer<SS::BIEvent>(event));
    UInt32 refID;
    BasicMap<UInt32, SS::Part*>::Iterator found = m_parts.End();
    if (GetXMLAttribute(node, AString("partRef"), refID))
        found = m_parts.Find(refID);
    if (found == m_parts.End()) {
        AddError(new BIError(BIError::nonExistingPart));
        events.PopBack();
        return false;
    }
    event->m_part = found->second;
    GetXMLAttribute(node, AString("exploded"), event->m_exploded);
    if (event->m_exploded)
        ReadTransformation(node, event->m_transformation);
    return true;
}

// ============================================================================================
// LXFML5Tester
// ============================================================================================

bool LXFML5Tester::Test(const XMLDOMNodeAString* root)                       // Mac 0x3242f0, Win 0x4a42a0
{
    return m_loader->CheckRoot(*root);
}

// ============================================================================================
// ModelSaverLXFML5
// ============================================================================================

ModelSaverLXFML5::ModelSaverLXFML5() {}                                     // Mac 0x31f0e0, Win 0x4a89e0

ModelSaverLXFML5::~ModelSaverLXFML5() {}                                    // Mac 0x3219a0, Win 0x4a9540

bool ModelSaverLXFML5::Save(const Path& path, const StorageScene2& scene)    // Mac 0x321740, Win 0x4a9300
{
    AString text;
    Write(text, scene);
    RefPointer<File> file = path.OpenFile(OpenMode(false, true), true);
    if (file.IsNull()) {
        String message("Unable to open file: ");
        message << path.GetPath(true);
        m_errorMessage = message;
        return false;
    }
    if (!file->Write(text.GetLength(), text.GetBuffer(), 0)) {
        String message("Unable to write to file: ");
        message << path.GetPath(true);
        m_errorMessage = message;
        return false;
    }
    file->SetSize(text.GetLength());
    return true;
}

bool ModelSaverLXFML5::Save(const Path&, const StorageScene&)                // Mac 0x321990, Win 0x4a9530
{
    return false;
}

void ModelSaverLXFML5::WriteAttribute(const String& name, const String& value) // Mac 0x31ffa0, Win 0x4a4190
{
    m_text << " " << name << "=\"" << value.ToXML() << "\"";
}

void ModelSaverLXFML5::WriteVector(const String& name, const Vector3& v)      // Win 0x4ab460
{
    m_text << " " << name << "=\"" << v.x << "," << v.y << "," << v.z << "\"";
}

void ModelSaverLXFML5::WriteTransformation(const Matrix3D& m)                 // Mac 0x321bf0, Win 0x4ab4e0
{
    const Real32* f = m.m;
    m_text << " transformation=\"" << f[0] << "," << f[1] << "," << f[2] << "," << f[4] << "," << f[5]
           << "," << f[6] << "," << f[8] << "," << f[9] << "," << f[10] << "," << f[12] << "," << f[13]
           << "," << f[14] << "\"";
}

void ModelSaverLXFML5::WriteNumberList(const String& name, const BasicArray<UInt32>& values, UInt32 start) // Mac 0x322230, Win 0x4ae0f0
{
    String text;
    for (UInt32 i = start; i < values.GetSize(); ++i) {
        if (i != start)
            text << ",";
        text << values[i];
    }
    WriteAttribute(name, text);
}

// Mac 0x323380 / 0x3238d0, Win 0x4ae230 / 0x4ae460 (the map by reference: see the header)
template <class T>
void ModelSaverLXFML5::WriteRefList(const String& name, const BasicArray<RefPointer<T> >& items,
                                    const BasicMap<const T*, UInt32>& ids, UInt32 start)
{
    String text;
    for (UInt32 i = start; i < items.GetSize(); ++i) {
        if (i != start)
            text << ",";
        typename BasicMap<const T*, UInt32>::ConstIterator it = ids.Find(items[i].Get());
        text << (it != ids.End() ? it->second : UInt32(0));
    }
    WriteAttribute(name, text);
}

void ModelSaverLXFML5::Write(AString& out, const StorageScene2& scene)       // Mac 0x31f2e0, Win 0x4a8ae0
{
    m_errorMessage.Clear();
    m_text.Clear();
    m_cameraIDs.Clear();
    m_brickIDs.Clear();
    m_partIDs.Clear();
    m_boneIDs.Clear();
    m_rigidIDs.Clear();
    m_scene = const_cast<StorageScene2*>(&scene);

    m_text << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\r\n";
    String lxfml("LXFML");
    m_text << "<" << lxfml;
    {
        String value;
        ToString(UInt32(5), value);
        WriteAttribute(String("versionMajor"), value);
    }
    {
        String value;
        ToString(UInt32(0), value);
        WriteAttribute(String("versionMinor"), value);
    }
    if (!scene.m_name.IsEmpty())
        WriteAttribute(String("name"), scene.m_name);
    m_text << ">\r\n";

    if (!scene.m_metas.IsEmpty()) {
        String tag("Meta");
        m_text.Append(2, ' ');
        m_text << "<" << tag;
        m_text << ">\r\n";
        WriteMetas(4);
        m_text.Append(2, ' ');
        m_text << "</" << tag << ">\r\n";
    }
    if (!scene.m_cameras.IsEmpty()) {
        String tag("Cameras");
        m_text.Append(2, ' ');
        m_text << "<" << tag;
        m_text << ">\r\n";
        WriteCameras(4);
        m_text.Append(2, ' ');
        m_text << "</" << tag << ">\r\n";
    }
    if (!scene.m_behaviors.IsEmpty()) {
        String tag("Behaviors");
        m_text.Append(2, ' ');
        m_text << "<" << tag;
        m_text << ">\r\n";
        WriteBehaviors(4, scene.m_behaviors);
        m_text.Append(2, ' ');
        m_text << "</" << tag << ">\r\n";
    }
    {
        String tag("Bricks");
        m_text.Append(2, ' ');
        m_text << "<" << tag;
        if (!scene.m_camera.IsNull()) {
            String value;
            ToString(m_cameraIDs[scene.m_camera.Get()], value);
            WriteAttribute(String("cameraRef"), value);
        }
        m_text << ">\r\n";
        WriteBricks(4);
        m_text.Append(2, ' ');
        m_text << "</" << tag << ">\r\n";
    }
    if (!scene.m_rigidSystems.IsEmpty()) {
        String tag("RigidSystems");
        m_text.Append(2, ' ');
        m_text << "<" << tag;
        m_text << ">\r\n";
        for (BasicArray<SS::RigidSystem>::ConstIterator it = scene.m_rigidSystems.Begin();
             it != scene.m_rigidSystems.End(); ++it)
            WriteRigidSystem(4, *it);
        m_text.Append(2, ' ');
        m_text << "</" << tag << ">\r\n";
    }
    if (!scene.m_groupSystems.IsEmpty()) {
        String tag("GroupSystems");
        m_text.Append(2, ' ');
        m_text << "<" << tag;
        m_text << ">\r\n";
        for (BasicArray<SS::GroupSystem>::ConstIterator it = scene.m_groupSystems.Begin();
             it != scene.m_groupSystems.End(); ++it)
            WriteGroupSystem(4, *it);
        m_text.Append(2, ' ');
        m_text << "</" << tag << ">\r\n";
    }
    if (!scene.m_buildingInstructions.IsEmpty()) {
        String tag("BuildingInstructions");
        m_text.Append(2, ' ');
        m_text << "<" << tag;
        m_text << ">\r\n";
        WriteBuildingInstructions(4);
        m_text.Append(2, ' ');
        m_text << "</" << tag << ">\r\n";
    }
    m_text << "</" << lxfml << ">\r\n";
    out = m_text.ToUTF8();
}

void ModelSaverLXFML5::WriteMetas(UInt32 indent)                              // Mac 0x320050, Win 0x4a9720
{
    const BasicArray<SS::Meta>& metas = m_scene->m_metas;
    for (BasicArray<SS::Meta>::ConstIterator it = metas.Begin(); it != metas.End(); ++it) {
        m_text.Append(indent, ' ');
        m_text << "<" << it->m_name;
        for (BasicArray<SS::Meta::Attribute>::ConstIterator a = it->m_attributes.Begin(); a != it->m_attributes.End(); ++a)
            WriteAttribute(a->m_name, a->m_value);
        if (it->m_text.IsEmpty()) {
            m_text << "/>\r\n";
        } else {
            // NOTE: the text is written as it is (not escaped, no indentation)
            m_text << ">\r\n";
            m_text << it->m_text;
            m_text << "</" << it->m_name << ">\r\n";
        }
    }
}

void ModelSaverLXFML5::WriteCameras(UInt32 indent)                            // Mac 0x320280, Win 0x4a9800
{
    const BasicArray<RefPointer<SS::Camera> >& cameras = m_scene->m_cameras;
    for (BasicArray<RefPointer<SS::Camera> >::ConstIterator it = cameras.Begin(); it != cameras.End(); ++it) {
        const SS::Camera* camera = it->Get();
        String tag("Camera");
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "<" << tag;
        UInt32 id = m_cameraIDs.GetSize();
        m_cameraIDs[camera] = id;
        {
            String value;
            ToString(m_cameraIDs[camera], value);
            WriteAttribute(String("refID"), value);
        }
        if (!camera->m_name.IsEmpty())
            WriteAttribute(String("name"), camera->m_name);
        {
            String value;
            ToString(camera->m_fieldOfView, value);
            WriteAttribute(String("fieldOfView"), value);
        }
        if (camera->m_orthographic) {
            String value;
            ToString("true", value);
            WriteAttribute(String("orthographic"), value);
        }
        if (1e-5f < camera->m_distance) {
            String value;
            ToString(camera->m_distance, value);
            WriteAttribute(String("distance"), value);
        }
        WriteTransformation(camera->m_transformation);
        m_text << "/>\r\n";
    }
}

void ModelSaverLXFML5::WriteBehaviors(UInt32 indent, const BasicArray<SS::Behavior>& behaviors) // Mac 0x320750, Win 0x4aa180
{
    for (BasicArray<SS::Behavior>::ConstIterator it = behaviors.Begin(); it != behaviors.End(); ++it) {
        {
            String tag("Behavior");
            if (indent != 0)
                m_text.Append(indent, ' ');
            m_text << "<" << tag;
        }
        WriteAttribute(String("BehaviorName"), it->m_name);
        m_text << ">\r\n";
        for (BasicArray<SS::Attribute>::ConstIterator a = it->m_attributes.Begin(); a != it->m_attributes.End(); ++a) {
            String tag("Attribute");
            if (indent + 2 != 0)
                m_text.Append(indent + 2, ' ');
            m_text << "<" << tag;
            WriteAttribute(String("Key"), a->m_key);
            WriteAttribute(String("Value"), a->m_value);
            m_text << "/>\r\n";
        }
        String tag("Behavior");
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "</" << tag << ">\r\n";
    }
}

void ModelSaverLXFML5::WriteBricks(UInt32 indent)                             // Mac 0x320b30, Win 0x4a9a70
{
    const BasicArray<RefPointer<SS::Brick> >& bricks = m_scene->m_bricks;
    for (BasicArray<RefPointer<SS::Brick> >::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        const SS::Brick* brick = it->Get();
        UInt32 id = m_brickIDs.GetSize();
        m_brickIDs[brick] = id;
        String tag("Brick");
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "<" << tag;
        {
            String value;
            ToString(m_brickIDs[brick], value);
            WriteAttribute(String("refID"), value);
        }
        {
            String value;
            ToString(brick->m_designID, value);
            WriteAttribute(String("designID"), value);
        }
        if (!brick->m_itemNos.IsEmpty())
            WriteAttribute(String("itemNos"), brick->m_itemNos);
        m_text << ">\r\n";
        WriteBehaviors(indent + 2, brick->m_behaviors);
        WriteParts(indent + 2, *brick);
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "</" << tag << ">\r\n";
    }
}

void ModelSaverLXFML5::WriteParts(UInt32 indent, const SS::Brick& brick)      // Mac 0x321db0, Win 0x4a9ce0
{
    for (BasicArray<RefPointer<SS::Part> >::ConstIterator it = brick.m_parts.Begin(); it != brick.m_parts.End(); ++it) {
        const SS::Part* part = it->Get();
        UInt32 id = m_partIDs.GetSize();
        m_partIDs[part] = id;
        String tag("Part");
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "<" << tag;
        {
            String value;
            ToString(m_partIDs[part], value);
            WriteAttribute(String("refID"), value);
        }
        {
            String value;
            ToString(part->m_designID, value);
            WriteAttribute(String("designID"), value);
        }
        WriteNumberList(String("materials"), part->m_materials, 0);
        if (part->m_decorations.GetSize() > 1)
            WriteNumberList(String("decoration"), part->m_decorations, 1);
        m_text << ">\r\n";
        WriteBehaviors(indent + 2, part->m_behaviors);
        WriteBones(indent + 2, *part);
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "</" << tag << ">\r\n";
    }
}

void ModelSaverLXFML5::WriteBones(UInt32 indent, const SS::Part& part)        // Mac 0x322300, Win 0x4a9f90
{
    for (BasicArray<RefPointer<SS::Bone> >::ConstIterator it = part.m_bones.Begin(); it != part.m_bones.End(); ++it) {
        const SS::Bone* bone = it->Get();
        UInt32 id = m_boneIDs.GetSize();
        m_boneIDs[bone] = id;
        String tag("Bone");
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "<" << tag;
        {
            String value;
            ToString(m_boneIDs[bone], value);
            WriteAttribute(String("refID"), value);
        }
        WriteTransformation(bone->m_transformation);
        m_text << ">\r\n";
        WriteBehaviors(indent + 2, bone->m_behaviors);
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "</" << tag << ">\r\n";
    }
}

void ModelSaverLXFML5::WriteRigidSystem(UInt32 indent, const SS::RigidSystem& system) // Mac 0x320f50, Win 0x4aa460
{
    String tag("RigidSystem");
    if (indent != 0)
        m_text.Append(indent, ' ');
    m_text << "<" << tag;
    m_text << ">\r\n";
    for (BasicArray<RefPointer<SS::Rigid> >::ConstIterator it = system.m_rigids.Begin(); it != system.m_rigids.End(); ++it) {
        const SS::Rigid* rigid = it->Get();
        UInt32 id = m_rigidIDs.GetSize();
        m_rigidIDs[rigid] = id;
        WriteRigid(indent + 2, rigid);
    }
    for (BasicArray<SS::Joint>::ConstIterator it = system.m_joints.Begin(); it != system.m_joints.End(); ++it)
        WriteJoint(indent + 2, *it);
    String end("RigidSystem");
    if (indent != 0)
        m_text.Append(indent, ' ');
    m_text << "</" << end << ">\r\n";
}

void ModelSaverLXFML5::WriteRigid(UInt32 indent, const SS::Rigid* rigid)      // Mac 0x322650, Win 0x4aa5e0
{
    String tag("Rigid");
    if (indent != 0)
        m_text.Append(indent, ' ');
    m_text << "<" << tag;
    {
        String value;
        ToString(m_rigidIDs[rigid], value);
        WriteAttribute(String("refID"), value);
    }
    WriteTransformation(rigid->m_transformation);
    WriteRefList(String("boneRefs"), rigid->m_bones, m_boneIDs, 0);
    m_text << "/>\r\n";
}

void ModelSaverLXFML5::WriteJoint(UInt32 indent, const SS::Joint& joint)      // Mac 0x322920, Win 0x4aa730
{
    {
        String tag("Joint");
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "<" << tag;
    }
    if (!joint.m_name.IsEmpty())
        WriteAttribute(String("name"), joint.m_name);
    if (joint.m_type == SS::Joint::Ball || joint.m_type == SS::Joint::Hinge) {
        String value;
        ToString(kJointTypes[joint.m_type], value);
        WriteAttribute(String("type"), value);
    }
    m_text << ">\r\n";
    // NOTE: the rigid lookups insert unknown rigids (also null) into m_rigidIDs with refID 0,
    // which shifts the refIDs of the rigids written later.
    const SS::Rigid* rigids[2] = { joint.m_rigidA.Get(), joint.m_rigidB.Get() };
    const Vector3* vectors[2][3] = { { &joint.m_axisA, &joint.m_zeroA, &joint.m_positionA },
                                     { &joint.m_axisB, &joint.m_zeroB, &joint.m_positionB } };
    for (UInt32 i = 0; i < 2; ++i) {
        String tag("RigidRef");
        if (indent + 2 != 0)
            m_text.Append(indent + 2, ' ');
        m_text << "<" << tag;
        {
            String value;
            ToString(m_rigidIDs[rigids[i]], value);
            WriteAttribute(String("rigidRef"), value);
        }
        WriteVector(String("a"), *vectors[i][0]);
        WriteVector(String("z"), *vectors[i][1]);
        WriteVector(String("t"), *vectors[i][2]);
        m_text << "/>\r\n";
    }
    String tag("Joint");
    if (indent != 0)
        m_text.Append(indent, ' ');
    m_text << "</" << tag << ">\r\n";
}

void ModelSaverLXFML5::WriteGroupSystem(UInt32 indent, const SS::GroupSystem& system) // Mac 0x321190, Win 0x4aab50
{
    String tag("GroupSystem");
    if (indent != 0)
        m_text.Append(indent, ' ');
    m_text << "<" << tag;
    m_text << ">\r\n";
    for (BasicArray<RefPointer<SS::Group> >::ConstIterator it = system.m_groups.Begin(); it != system.m_groups.End(); ++it)
        WriteGroup(indent + 2, it->Get());
    String end("GroupSystem");
    if (indent != 0)
        m_text.Append(indent, ' ');
    m_text << "</" << end << ">\r\n";
}

void ModelSaverLXFML5::WriteGroup(UInt32 indent, const SS::Group* group)      // Mac 0x3234e0, Win 0x4aac80
{
    String tag("Group");
    if (indent != 0)
        m_text.Append(indent, ' ');
    m_text << "<" << tag;
    if (!group->m_name.IsEmpty())
        WriteAttribute(String("name"), group->m_name);
    WriteTransformation(group->m_transformation);
    WriteVector(String("pivot"), group->m_pivot);
    WriteRefList(String("partRefs"), group->m_parts, m_partIDs, 0);
    if (group->m_groups.IsEmpty() && group->m_behaviors.IsEmpty()) {
        m_text << "/>\r\n";
        return;
    }
    m_text << ">\r\n";
    WriteBehaviors(indent + 2, group->m_behaviors);
    for (BasicArray<RefPointer<SS::Group> >::ConstIterator it = group->m_groups.Begin(); it != group->m_groups.End(); ++it)
        WriteGroup(indent + 2, it->Get());
    String end("Group");
    if (indent != 0)
        m_text.Append(indent, ' ');
    m_text << "</" << end << ">\r\n";
}

void ModelSaverLXFML5::WriteBuildingInstructions(UInt32 indent)               // Mac 0x321300, Win 0x4aaec0
{
    UInt32 unnamed = 1;
    const BasicArray<SS::BI>& instructions = m_scene->m_buildingInstructions;
    for (BasicArray<SS::BI>::ConstIterator it = instructions.Begin(); it != instructions.End(); ++it) {
        if (it->m_events.IsEmpty())
            continue;   // instructions without events are not written
        String tag("BuildingInstruction");
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "<" << tag;
        if (!it->m_name.IsEmpty()) {
            WriteAttribute(String("name"), it->m_name);
        } else {
            String name("BuildingGuide");
            name << unnamed;
            WriteAttribute(String("name"), name);
            ++unnamed;
        }
        m_text << ">\r\n";
        WriteEvents(it->m_events, indent + 2);
        String end("BuildingInstruction");
        if (indent != 0)
            m_text.Append(indent, ' ');
        m_text << "</" << end << ">\r\n";
    }
}

void ModelSaverLXFML5::WriteEvents(const BasicArray<RefPointer<SS::BIEvent> >& events, UInt32 indent) // Mac 0x323a30, Win 0x4ab0b0
{
    for (BasicArray<RefPointer<SS::BIEvent> >::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        const SS::BIEvent* event = it->Get();
        if (event->m_type == SS::BIEvent::TypeStep) {
            const SS::BIStep* step = static_cast<const SS::BIStep*>(event);
            String tag("Step");
            if (indent != 0)
                m_text.Append(indent, ' ');
            m_text << "<" << tag;
            if (!step->m_name.IsEmpty())
                WriteAttribute(String("name"), step->m_name);
            if (step->m_exploded) {
                String value;
                ToString("true", value);
                WriteAttribute(String("exploded"), value);
                WriteVector(String("a"), step->m_explode);
            }
            m_text << ">\r\n";
            WriteEvents(step->m_events, indent + 2);
            String end("Step");
            if (indent != 0)
                m_text.Append(indent, ' ');
            m_text << "</" << end << ">\r\n";
        }
        if (event->m_type == SS::BIEvent::TypeCamera) {
            const SS::BICamera* camera = static_cast<const SS::BICamera*>(event);
            String tag("Camera");
            if (indent != 0)
                m_text.Append(indent, ' ');
            m_text << "<" << tag;
            if (!camera->m_camera.IsNull()) {
                String value;
                ToString(m_cameraIDs[camera->m_camera.Get()], value);
                WriteAttribute(String("cameraRef"), value);
            }
            m_text << "/>\r\n";
        }
        if (event->m_type == SS::BIEvent::TypePart) {
            const SS::BIPart* part = static_cast<const SS::BIPart*>(event);
            // NOTE: one level deeper than the other events (both builds)
            String tag("PartRef");
            if (indent + 2 != 0)
                m_text.Append(indent + 2, ' ');
            m_text << "<" << tag;
            {
                String value;
                ToString(m_partIDs[part->m_part.Get()], value);
                WriteAttribute(String("partRef"), value);
            }
            if (part->m_exploded) {
                String value;
                ToString("true", value);
                WriteAttribute(String("exploded"), value);
                WriteTransformation(part->m_transformation);
            }
            m_text << "/>\r\n";
        }
    }
}

} // namespace LEGO
