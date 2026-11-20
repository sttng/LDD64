// LEGO Digital Designer — restored source
// LXFML 2 reader (ModelLoaderLXFML2). See the header. Plan item 2.7, Mac TU 339.
// A copy of the LXFML 4 reader cut down to the LXFML 2 elements (the original object has its own
// copies of every handler).
#include "ModelIOLXFML2.h"
#include "TransformAttributes.h"
#include "BasicDeque.h"
#include "BasicMap.h"
#include "MathUtil.h"
#include "LEGO/StdLib.h"
#include "StringConversion.h"
#include "Version.h"

#ifndef XML_STATIC   // (the projects define it as well)
#define XML_STATIC   // Expat is linked statically into both programs
#endif
#include "expat.h"

#include <iostream>

namespace LEGO {

typedef StorageScene  SS1;
typedef StorageScene2 SS2;

// ============================================================================================
// ModelLoaderLXFML2::Parser — the Expat user data. Mac 0xac bytes (Load's frame 0xdc),
// constructor 0x2fe6a0, destructor 0x3029b0; Win constructor 0x5b1890, destructor 0x5b3a30.
// (struct and member names inferred)
// ============================================================================================
struct ModelLoaderLXFML2::Parser {
    typedef void (*StartHandler)(void* data, const char* name, const char** attributes);
    typedef void (*EndHandler)(void* data, const char* name);

    Parser(StorageScene* scene, UInt32 flags, ModelLoaderLXFML2* loader, XML_Parser* parser)
        : m_found(false), m_unknown4(0), m_unknown8(0), m_scene(scene), m_flags(flags),
          m_loader(loader), m_parser(parser)
    {
    }

    // Mac 0x2fe2f0, Win 0x5b19a0 (EDI = this)
    void Push(StartHandler start, EndHandler end)
    {
        m_handlers.PushBack(Pair<StartHandler, EndHandler>(start, end));
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    // Win 0x5b19f0 (EBX = this); inlined on Mac
    void Pop()
    {
        m_handlers.PopBack();
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    void AddError(ModelLoader::Error* error) { m_loader->AddError(error); }

    SS1::Model& CurrentModel() { return m_scene->m_models.Back(); }
    SS1::Part*  CurrentPart()  { return CurrentModel().m_rigids.Back()->m_parts.Back(); }

    bool                                    m_found;       // +0x00 an <LXFML> element was seen
    UInt32                                  m_unknown4;    // +0x04 zeroed, never used (name inferred)
    UInt32                                  m_unknown8;    // +0x08 zeroed, never used (name inferred)
    RefPointer<SS1>                         m_scene;       // +0x0c
    UInt32                                  m_flags;       // +0x10 bit 0: skip <Joint>s
    ModelLoaderLXFML2*                      m_loader;      // +0x14 (not reference counted)
    XML_Parser*                             m_parser;      // +0x18
    BasicMap<UInt32, SS1::Rigid*>           m_rigids;      // +0x1c refID -> element
    BasicMap<UInt32, SS1::Assembly*>        m_assemblies;  // +0x38
    BasicMap<UInt32, SS1::Flex*>            m_flexes;      // +0x54
    BasicArray<SS1::Flex*>                  m_flexList;    // +0x70 every Flex created
    BasicDeque<Pair<StartHandler, EndHandler> > m_handlers; // +0x84
};

namespace {

typedef ModelLoaderLXFML2::Parser Parser;

inline Parser& P(void* data) { return *static_cast<Parser*>(data); }

inline String FromUTF8(const char* s)
{
    return DecodeString(kTextEncodingUTF8, s, StrLen(s));
}

inline UInt32 ToUInt32(const char* s) { return StrToUInt32(s, 0, 10); }

void IgnoreEnd(void* data, const char*);

// Mac 0x2fecc0, Win 0x5b36d0
void IgnoreStart(void* data, const char*, const char**)
{
    P(data).Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x2fecf0, Win 0x5b3730
void IgnoreEnd(void* data, const char*)
{
    P(data).Pop();
}

// Mac 0x2fe640, Win 0x459790 (shared empty function)
void NoEnd(void*, const char*)
{
}

inline void PopIf(void* data, const char* name, const char* element)
{
    if (StrCmp(name, element) == 0)
        P(data).Pop();
}

// ---- <Scene> ---------------------------------------------------------------------------------

// Children of <Part>: <SubMaterial> and <Decoration>. Mac 0x301670, Win 0x5b3340.
void PartChildStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "SubMaterial") == 0) {
        UInt32 surface = 0, material = 0;
        for (; *a; a += 2) {
            if (StrCmp(a[0], "surfaceID") == 0)
                surface = ToUInt32(a[1]);
            else if (StrCmp(a[0], "materialID") == 0)
                material = ToUInt32(a[1]);
        }
        if (surface == 0 || material == 0)
            return;
        // NOTE: the surface id grows the array to surface + 1 unchecked (both builds; an id
        // near 4G wraps the size and writes past it); fixed: ids from kMaxModelFileIndex on are
        // ignored, as 0 is.
        if (surface >= kMaxModelFileIndex)
            return;
        BasicArray<UInt32>& materials = parser.CurrentPart()->m_materials;
        if (materials.GetSize() <= surface)
            materials.Resize(surface + 1, 0);
        materials[surface] = material;
        return;
    }
    if (StrCmp(name, "Decoration") == 0) {
        SS1::Decoration decoration;
        decoration.m_surface = 0;
        decoration.m_decoration = 0;
        for (; *a; a += 2) {
            if (StrCmp(a[0], "surfaceID") == 0)
                decoration.m_surface = ToUInt32(a[1]);
            else if (StrCmp(a[0], "decorationID") == 0)
                decoration.m_decoration = ToUInt32(a[1]);
        }
        parser.CurrentPart()->m_decorations.PushBack(decoration);
        return;
    }
    parser.Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x301ad0
void PartEnd(void* data, const char* name)
{
    PopIf(data, name, "Part");
}

// Children of <Group>: <Part> (no refID in LXFML 2). Mac 0x2ffaa0, Win 0x5b2750.
void PartStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Part") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    RefPointer<SS1::Part> part = new SS1::Part;
    TransformAttributes transform;
    UInt32 flexRefID = 0xffffffff;
    UInt32 controlPoint = 0xffffffff;
    UInt32 assemblyRefID = 0xffffffff;
    UInt32 assemblyID = 0;
    for (; *a; a += 2) {
        if (StrCmp(a[0], "designID") == 0)
            part->m_designID = ToUInt32(a[1]);
        else if (StrCmp(a[0], "materialID") == 0)
            part->m_materials.PushBack(ToUInt32(a[1]));
        else if (StrCmp(a[0], "flexRefID") == 0)
            flexRefID = ToUInt32(a[1]);
        else if (StrCmp(a[0], "controlPoint") == 0)
            controlPoint = ToUInt32(a[1]);
        else if (StrCmp(a[0], "assemblyRefID") == 0)
            assemblyRefID = ToUInt32(a[1]);
        else if (StrCmp(a[0], "assemblyID") == 0)
            assemblyID = ToUInt32(a[1]);
        else if (StrCmp(a[0], "name") == 0)
            part->m_name = FromUTF8(a[1]);
        else
            transform.ParseAttribute(a[0], a[1]);
    }

    // NOTE: a rejected part pushes no handlers, so its children reach this handler again
    UInt32 detail;
    if (part->m_materials.GetSize() != 1)
        detail = ModelLoader::PartError::notOneMaterial;
    else if ((assemblyRefID == 0xffffffff) != (assemblyID == 0))
        detail = ModelLoader::PartError::assembly;
    else if ((flexRefID == 0xffffffff) != (controlPoint == 0xffffffff))
        detail = ModelLoader::PartError::flex;
    // NOTE: the control point grows the flex's part array to controlPoint + 1 unchecked (both
    // builds; 0xFFFFFFFE wraps it); fixed: a control point from kMaxModelFileIndex on is a
    // flex error.
    else if (controlPoint != 0xffffffff && controlPoint >= kMaxModelFileIndex)
        detail = ModelLoader::PartError::flex;
    else
        detail = 0xffffffff;
    if (detail != 0xffffffff) {
        parser.AddError(new ModelLoader::PartError(0, detail));
        return;
    }

    transform.GetMatrix(part->m_transformation);

    if (flexRefID != 0xffffffff) {
        BasicMap<UInt32, SS1::Flex*>::Iterator it = parser.m_flexes.Find(flexRefID);
        if (it != parser.m_flexes.End()) {
            part->m_flex = it->second;
            BasicArray<SS1::Part*>& parts = part->m_flex->m_parts;
            if (parts.GetSize() <= controlPoint)
                parts.Resize(controlPoint + 1, 0);
            parts[controlPoint] = part;
        } else {
            part->m_flex = new SS1::Flex(part->m_designID);
            BasicArray<SS1::Part*>& parts = part->m_flex->m_parts;
            parts.Resize(controlPoint + 1, 0);
            parts[controlPoint] = part;
            parser.m_flexList.PushBack(part->m_flex);
            parser.m_flexes[flexRefID] = part->m_flex;
        }
    }
    if (assemblyID != 0) {
        BasicMap<UInt32, SS1::Assembly*>::Iterator it = parser.m_assemblies.Find(assemblyRefID);
        if (it != parser.m_assemblies.End()) {
            part->m_assembly = it->second;
            part->m_assembly->m_parts.PushBack(part);
        } else {
            part->m_assembly = new SS1::Assembly(assemblyID);
            part->m_assembly->m_parts.PushBack(part);
            parser.m_assemblies[assemblyRefID] = part->m_assembly;
        }
    }
    parser.CurrentModel().m_rigids.Back()->m_parts.PushBack(part);
    parser.Push(PartChildStart, PartEnd);
}

// Mac 0x300df0
void GroupEnd(void* data, const char* name)
{
    PopIf(data, name, "Group");
}

// Children of <Joint>: <GroupRef>. Mac 0x300ec0, Win 0x5b2f00.
void GroupRefStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "GroupRef") != 0)
        return;   // NOTE: no handlers pushed for other elements either
    SS1::Joint& joint = parser.CurrentModel().m_joints.Back();
    const UInt32 detail = (UInt32)(size_t)&joint;   // the joint's address (both builds)
    RefPointer<SS1::Rigid>* rigid;
    Vector3* axis;
    Vector3* zero;
    Vector3* position;
    if (joint.m_rigidA.IsNull()) {
        rigid = &joint.m_rigidA;
        axis = &joint.m_axisA;
        zero = &joint.m_zeroA;
        position = &joint.m_positionA;
    } else {
        if (!joint.m_rigidB.IsNull()) {
            parser.AddError(new ModelLoader::JointError(detail));
            return;
        }
        rigid = &joint.m_rigidB;
        axis = &joint.m_axisB;
        zero = &joint.m_zeroB;
        position = &joint.m_positionB;
    }
    for (; *a; a += 2) {
        if (StrCmp(a[0], "groupRef") == 0) {
            BasicMap<UInt32, SS1::Rigid*>::Iterator it = parser.m_rigids.Find(ToUInt32(a[1]));
            if (it == parser.m_rigids.End()) {
                parser.AddError(new ModelLoader::JointError(detail));
                return;
            }
            *rigid = it->second;
            continue;
        }
        if (StrCmp(a[0], "ax") == 0)
            axis->x = StrToReal32(a[1], 0);
        else if (StrCmp(a[0], "ay") == 0)
            axis->y = StrToReal32(a[1], 0);
        else if (StrCmp(a[0], "az") == 0)
            axis->z = StrToReal32(a[1], 0);
        if (StrCmp(a[0], "zx") == 0)
            zero->x = StrToReal32(a[1], 0);
        else if (StrCmp(a[0], "zy") == 0)
            zero->y = StrToReal32(a[1], 0);
        else if (StrCmp(a[0], "zz") == 0)
            zero->z = StrToReal32(a[1], 0);
        if (StrCmp(a[0], "tx") == 0)
            position->x = StrToReal32(a[1], 0);
        else if (StrCmp(a[0], "ty") == 0)
            position->y = StrToReal32(a[1], 0);
        else if (StrCmp(a[0], "tz") == 0)
            position->z = StrToReal32(a[1], 0);
    }
    if (Abs(axis->GetLength() - 1.0f) < 1e-5f && Abs(zero->GetLength() - 1.0f) < 1e-5f)
        return;
    parser.AddError(new ModelLoader::JointError(detail));
}

// Mac 0x3014c0, Win 0x5b3270
void JointEnd(void* data, const char* name)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Joint") != 0)
        return;
    SS1::Joint& joint = parser.CurrentModel().m_joints.Back();
    if (joint.m_rigidA.IsNull() || joint.m_rigidB.IsNull() || joint.m_rigidA == joint.m_rigidB)
        parser.AddError(new ModelLoader::JointError((UInt32)(size_t)&joint));
    parser.Pop();
}

// Children of <Model>: <Group> and <Joint>. Mac 0x2ff440, Win 0x5b2270.
void GroupStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Group") == 0) {
        RefPointer<SS1::Rigid> rigid = new SS1::Rigid;
        TransformAttributes transform;
        UInt32 refID = 0xffffffff;
        for (; *a; a += 2) {
            if (StrCmp(a[0], "refID") == 0)
                refID = ToUInt32(a[1]);
            else if (StrCmp(a[0], "name") == 0)
                rigid->m_name = FromUTF8(a[1]);
            else
                transform.ParseAttribute(a[0], a[1]);
        }
        transform.GetMatrix(rigid->m_transformation);
        if (refID != 0xffffffff)
            parser.m_rigids[refID] = rigid;
        parser.CurrentModel().m_rigids.PushBack(rigid);
        parser.Push(PartStart, GroupEnd);
        return;
    }
    if ((parser.m_flags & 1) == 0 && StrCmp(name, "Joint") == 0) {
        SS1::Joint joint;
        for (; *a; a += 2) {
            if (StrCmp(a[0], "name") == 0) {
                joint.m_name = FromUTF8(a[1]);
            } else if (StrCmp(a[0], "type") == 0) {
                if (StrCmp(a[1], "hinge") == 0) {
                    StrCmp(a[1], "ball");   // NOTE: result unused (both builds)
                    joint.m_type = 1;
                } else {
                    joint.m_type = 0;
                }
            }
        }
        parser.CurrentModel().m_joints.PushBack(joint);
        parser.Push(GroupRefStart, JointEnd);
        return;
    }
    parser.Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x2ff9d0
void ModelEnd(void* data, const char* name)
{
    PopIf(data, name, "Model");
}

// Children of <Scene>: <Model>. Mac 0x2ff060, Win 0x5b20c0.
void ModelStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Model") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    SS1::Model model;
    for (; *a; a += 2) {
        if (StrCmp(a[0], "name") == 0)
            model.m_name = FromUTF8(a[1]);
    }
    parser.m_scene->m_models.PushBack(model);
    parser.Push(GroupStart, ModelEnd);
}

// Mac 0x2ff200
void SceneEnd(void* data, const char* name)
{
    PopIf(data, name, "Scene");
}

// ---- <Meta> ----------------------------------------------------------------------------------

void MetaChildStart(void* data, const char*, const char**);
void MetaChildEnd(void* data, const char*);

// Mac 0x2ff3c0, Win 0x5b2040
void MetaText(void* data, const char* text, int length)
{
    P(data).m_scene->m_metas.Back().m_text.Append(DecodeString(kTextEncodingUTF8, text, (UInt32)length));
}

// Children of <Meta> (no BrickSet version in LXFML 2). Mac 0x2fed90, Win 0x5b1d60.
void MetaStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    SS2::Meta meta;
    meta.m_name = name;   // NOTE: names and values are taken as ASCII
    for (; *a; a += 2) {
        SS2::Meta::Attribute attribute;
        attribute.m_name = a[0];
        attribute.m_value = a[1];
        meta.m_attributes.PushBack(attribute);
    }
    parser.m_scene->m_metas.PushBack(meta);
    parser.Push(MetaChildStart, MetaChildEnd);
    XML_SetCharacterDataHandler(*parser.m_parser, MetaText);
}

// Mac 0x2ff2d0, Win 0x5b1f80
void MetaChildStart(void* data, const char*, const char**)
{
    XML_SetCharacterDataHandler(*P(data).m_parser, 0);
    P(data).Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x2ff310, Win 0x5b1ff0
void MetaChildEnd(void* data, const char*)
{
    XML_SetCharacterDataHandler(*P(data).m_parser, 0);
    P(data).Pop();
}

// Mac 0x2fef90
void MetaEnd(void* data, const char* name)
{
    PopIf(data, name, "Meta");
}

// ---- <LXFML> ---------------------------------------------------------------------------------

// Children of <LXFML>. Mac 0x2feb60, Win 0x5b1c50.
void LXFMLChildStart(void* data, const char* name, const char**)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Meta") == 0)
        parser.Push(MetaStart, MetaEnd);
    else if (StrCmp(name, "Scene") == 0)
        parser.Push(ModelStart, SceneEnd);
    else
        parser.Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x2febf0
void LXFMLEnd(void* data, const char* name)
{
    PopIf(data, name, "LXFML");
}

// The document element. Mac 0x2fe410, Win 0x5b1a40.
void RootStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "LXFML") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    parser.m_found = true;
    Version expected(2, 0, 0, 0);
    Version actual;
    for (; *a; a += 2) {
        if (StrCmp(a[0], "versionMajor") == 0)
            actual.m_major = ToUInt32(a[1]);
        else if (StrCmp(a[0], "versionMinor") == 0)
            actual.m_minor = ToUInt32(a[1]);
    }
    if (expected != actual) {
        // NOTE: no handlers pushed; the children are read as document elements
        parser.AddError(new ModelLoader::VersionError(expected, actual));
        return;
    }
    parser.Push(LXFMLChildStart, LXFMLEnd);
}

} // namespace

// ============================================================================================
// ModelLoaderLXFML2
// ============================================================================================

ModelLoaderLXFML2::ModelLoaderLXFML2() {}                                   // Win 0x5a1de0

ModelLoaderLXFML2::~ModelLoaderLXFML2() {}                                  // Mac 0x2fe650, Win 0x5a1e40

bool ModelLoaderLXFML2::Load(const AString& content, StorageScene& scene, bool flags) // Mac 0x2fdfe0, Win 0x5b3780
{
    scene.Clear();
    m_errors.Clear();
    m_errorMask = 0;

    XML_Parser expat = XML_ParserCreate(0);
    Parser parser(&scene, flags, this, &expat);
    XML_SetUserData(expat, &parser);
    parser.Push(RootStart, NoEnd);
    const char* text = content.GetLength() != 0 ? content.GetBuffer() : "";
    if (XML_Parse(expat, text, (int)content.GetLength(), 1) == XML_STATUS_ERROR) {
        XML_ParserFree(expat);
        AddError(new ParserError);
        return false;
    }
    XML_ParserFree(expat);
    if (m_errorMask != 0)
        return false;
    if (!parser.m_found) {
        AddError(new ContentError(ContentError::missingLXFMLTag));
        return false;
    }
    // the parts of a flex element must all exist and belong to the same assembly (no error)
    // NOTE: Win reads the first part's assembly before its null check (see ModelIOLXFML4.cpp)
    for (BasicArray<SS1::Flex*>::Iterator f = parser.m_flexList.Begin(); f != parser.m_flexList.End(); ++f) {
        const BasicArray<SS1::Part*>& parts = (*f)->m_parts;
        if (parts.IsEmpty())
            continue;
        const SS1::Part* first = parts[0];
        if (!first)
            return false;
        for (UInt32 i = 1; i < parts.GetSize(); ++i) {
            if (!parts[i] || parts[i]->m_assembly != first->m_assembly)
                return false;
        }
    }
    return true;
}

} // namespace LEGO
