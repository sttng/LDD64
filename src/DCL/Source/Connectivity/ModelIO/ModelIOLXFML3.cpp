// LEGO Digital Designer — restored source
// LXFML 3 reader (ModelLoaderLXFML3). See the header. Plan item 2.7, Mac TU 340.
// The reading half is the LXFML 2 reader with part refIDs (the original object has its own
// copies of every handler); the parts go through the LXFML 3 -> current design conversions.
#include "ModelIOLXFML3.h"
#include "TransformAttributes.h"
#include "BrickDatabase.h"
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
// ModelLoaderLXFML3::Parser — the Expat user data. Mac 0x84 bytes (Load's frame), constructor
// 0x308c00, destructor 0x30b670; Win constructor 0x5ae2e0, destructor 0x5b1780.
// (struct and member names inferred)
// ============================================================================================
struct ModelLoaderLXFML3::Parser {
    typedef void (*StartHandler)(void* data, const char* name, const char** attributes);
    typedef void (*EndHandler)(void* data, const char* name);

    Parser(StorageScene* scene, UInt32 flags, ModelLoaderLXFML3* loader, XML_Parser* parser,
           BrickDatabase* database)
        : m_found(false), m_scene(scene), m_flags(flags), m_loader(loader), m_parser(parser),
          m_database(database), m_unknown54(0)
    {
    }

    // Mac 0x308880 (Win inlines it: 0x5ae3c0 with the pair in EAX/ECX)
    void Push(StartHandler start, EndHandler end)
    {
        m_handlers.PushBack(Pair<StartHandler, EndHandler>(start, end));
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    // Win 0x5ae410 (EBX = this); inlined on Mac
    void Pop()
    {
        m_handlers.PopBack();
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    void AddError(ModelLoader::Error* error) { m_loader->AddError(error); }

    SS1::Model& CurrentModel() { return m_scene->m_models.Back(); }
    SS1::Rigid& CurrentRigid() { return *CurrentModel().m_rigids.Back(); }
    SS1::Part*  CurrentPart()  { return CurrentRigid().m_parts.Back(); }

    bool                                    m_found;       // +0x00 an <LXFML> element was seen
    RefPointer<SS1>                         m_scene;       // +0x04
    UInt32                                  m_flags;       // +0x08 bit 0: skip <Joint>s
    ModelLoaderLXFML3*                      m_loader;      // +0x0c (not reference counted)
    XML_Parser*                             m_parser;      // +0x10
    BrickDatabase*                          m_database;    // +0x14
    BasicArray<RefPointer<SS1::Part> >      m_pending3679; // +0x18 design 3679 parts, paired at the end
    BasicArray<RefPointer<SS1::Part> >      m_pending3680; // +0x28 design 3680 parts, paired at the end
    BasicMap<UInt32, SS1::Rigid*>           m_rigids;      // +0x38 refID -> element
    UInt32                                  m_unknown54;   // Mac +0x54 / Win +0x5c zeroed, never used
    BasicDeque<Pair<StartHandler, EndHandler> > m_handlers; // Mac +0x58 / Win +0x60
};

namespace {

typedef ModelLoaderLXFML3::Parser Parser;
typedef RefPointer<SS1::Part> PartRef;

const Real32 kPi = 3.1415927f;   // Win 0xae1b14

inline Parser& P(void* data) { return *static_cast<Parser*>(data); }

inline String FromUTF8(const char* s)
{
    return DecodeString(kTextEncodingUTF8, s, StrLen(s));
}

inline UInt32 ToUInt32(const char* s) { return StrToUInt32(s, 0, 10); }

// ---- conversions ---------------------------------------------------------------------------

// The float with the bit pattern `bits` (the conversion constants, exactly as in both builds).
inline Real32 F(UInt32 bits)
{
    union { UInt32 u; Real32 f; } v;
    v.u = bits;
    return v.f;
}

// A rotation about `axis` by `angle` followed by `translation`.
Matrix3D Placement(const Vector3& axis, Real32 angle, const Vector3& translation)
{
    Matrix3D m = Matrix3D::Identity();
    m.SetRotation(axis, angle);
    m.SetTranslation(translation);
    return m;
}

// A translation only.
Matrix3D Placement(const Vector3& translation)
{
    Matrix3D m = Matrix3D::Identity();
    m.SetTranslation(translation);
    return m;
}

// A new part `designID` placed at `part` * `offset`, with `part`'s material, added to the current
// rigid.
void AddPlacedPart(Parser& parser, const SS1::Part& part, UInt32 designID, const Matrix3D& offset)
{
    PartRef extra = new SS1::Part;
    extra->m_designID = designID;
    extra->m_transformation = part.m_transformation;
    extra->m_transformation *= offset;
    extra->m_materials.PushBack(part.m_materials[0]);
    parser.CurrentRigid().m_parts.PushBack(extra);
}

// The parts of the database brick `assemblyID` as a new Assembly, placed so that its part
// `mainDesignID` gets `transformation`; appended to `parts`. Mac 0x302b60, Win 0x5ae460
// (assemblyID in EDX, database in ECX). NOTE: the brick must exist (dereferenced unchecked,
// both builds), as must the first bone transformation of every part of it.
// (name inferred)
void BuildAssembly(BrickDatabase* database, UInt32 assemblyID, UInt32 mainDesignID,
                   const Matrix3D& transformation, BasicArray<PartRef>& parts)
{
    const BrickDatabase::Brick* brick = database->GetBrick(assemblyID);
    Matrix3D inverse = Matrix3D::Identity();
    for (BasicArray<BrickDatabase::Part>::ConstIterator it = brick->m_parts.Begin(); it != brick->m_parts.End(); ++it) {
        if (it->m_designID == mainDesignID) {
            inverse.SetInverse(*it->m_boneTransforms.Begin());
            break;
        }
    }
    RefPointer<SS1::Assembly> assembly = new SS1::Assembly(assemblyID);
    for (BasicArray<BrickDatabase::Part>::ConstIterator it = brick->m_parts.Begin(); it != brick->m_parts.End(); ++it) {
        PartRef part = new SS1::Part;
        part->m_designID = it->m_designID;
        part->m_transformation = transformation;
        if (it->m_designID != mainDesignID) {
            part->m_transformation *= inverse;
            part->m_transformation *= *it->m_boneTransforms.Begin();
        }
        assembly->m_parts.PushBack(part);
        part->m_assembly = assembly;
        parts.PushBack(part);
    }
}

// A part with assemblyID != 0: a few old assembly parts are rebuilt from the database (with
// fixed materials); every other one is dropped (the file's assembly is not kept). The part
// itself is never stored. Mac 0x302fd0, Win 0x5ae6d0 (part in ECX). (name inferred)
void ConvertAssemblyPart(Parser& parser, SS1::Part& part)
{
    BasicArray<PartRef> parts;
    switch (part.m_designID) {
    case 3828:   // -> brick 73081 (3828 / 3829)
        BuildAssembly(parser.m_database, 73081, 3829, part.m_transformation, parts);
        for (BasicArray<PartRef>::Iterator it = parts.Begin(); it != parts.End(); ++it) {
            PartRef p = *it;
            if (p->m_designID == 3828)
                p->m_materials.PushBack(26);
            else if (p->m_designID == 3829)
                p->m_materials.PushBack(21);
            parser.CurrentRigid().m_parts.PushBack(p);
        }
        break;

    case 3819: { // -> brick 76382 (3814 … 3820), materials by the part's colour (140 or 1)
        BuildAssembly(parser.m_database, 76382, 3814, part.m_transformation, parts);
        const UInt32 material = part.m_materials[0];
        for (BasicArray<PartRef>::Iterator it = parts.Begin(); it != parts.End(); ++it) {
            PartRef p = *it;
            switch (p->m_designID) {
            case 3814:
                if (material == 140) {
                    p->m_materials.PushBack(material);
                    SS1::Decoration decoration;
                    decoration.m_surface = 1;
                    decoration.m_decoration = 55660;
                    p->m_decorations.PushBack(decoration);
                } else if (material == 1) {
                    p->m_materials.PushBack(material);
                    SS1::Decoration decoration;
                    decoration.m_surface = 1;
                    decoration.m_decoration = 56288;
                    p->m_decorations.PushBack(decoration);
                }
                break;
            case 3818:
            case 3819:
                if (material == 140 || material == 1)
                    p->m_materials.PushBack(material);
                break;
            case 3820:
                if (material == 140)
                    p->m_materials.PushBack(24);
                else if (material == 1)
                    p->m_materials.PushBack(199);
                break;
            }
            parser.CurrentRigid().m_parts.PushBack(p);
        }
        break;
    }

    case 3816: { // -> brick 73200 (3815 / 3816 / 3817) in the part's colour
        BuildAssembly(parser.m_database, 73200, 3816, part.m_transformation, parts);
        const UInt32 material = part.m_materials[0];
        for (BasicArray<PartRef>::Iterator it = parts.Begin(); it != parts.End(); ++it) {
            PartRef p = *it;
            if (p->m_designID == 3815 || p->m_designID == 3816 || p->m_designID == 3817)
                p->m_materials.PushBack(material);
            parser.CurrentRigid().m_parts.PushBack(p);
        }
        break;
    }

    case 4106: { // -> part 73092
        PartRef p = new SS1::Part;
        p->m_designID = 73092;
        p->m_transformation = part.m_transformation;
        p->m_materials.PushBack(26);
        parser.CurrentRigid().m_parts.PushBack(p);
        break;
    }

    case 57878: { // -> part 74784
        PartRef p = new SS1::Part;
        p->m_designID = 74784;
        p->m_transformation = part.m_transformation;
        p->m_materials.PushBack(26);
        p->m_materials.PushBack(26);
        parser.CurrentRigid().m_parts.PushBack(p);
        break;
    }

    case 4593:   // -> brick 73587 (4592 / 4593)
        BuildAssembly(parser.m_database, 73587, 4592, part.m_transformation, parts);
        for (BasicArray<PartRef>::Iterator it = parts.Begin(); it != parts.End(); ++it) {
            PartRef p = *it;
            if (p->m_designID == 4592 || p->m_designID == 4593)
                p->m_materials.PushBack(26);
            parser.CurrentRigid().m_parts.PushBack(p);
        }
        break;
    }
}

// A part with assemblyID == 0: renamed, turned, completed with extra parts or kept for pairing,
// then (except the kept ones) added to the current rigid. Mac 0x3063c0, Win 0x5aed80 (part in
// EBX). (name inferred)
void ConvertPart(Parser& parser, const PartRef& partRef)
{
    SS1::Part& part = *partRef;
    switch (part.m_designID) {
    case 2377:
        part.m_designID = 60032;
        break;
    case 2865:
        part.m_designID = 74746;
        break;
    case 2867:
        part.m_designID = 74747;
        AddPlacedPart(parser, part, part.m_designID,
                      Placement(Vector3(0.0f, -1.0f, 0.0f), kPi * 0.125f, Vector3(F(0x4033550c), 0.0f, F(0xc154974a))));
        break;
    case 2920: {
        const Real32 s = Sqrt(0.5f);
        part.m_transformation *= Placement(Vector3(0.0f, s, -s), kPi, Vector3(0.0f, 0.0f, 0.0f));
        break;
    }
    case 3679:   // paired with a 3680 part at the end (PairPart)
        parser.m_pending3679.PushBack(partRef);
        return;
    case 3680:
        parser.m_pending3680.PushBack(partRef);
        return;
    case 70358: {
        part.m_materials.Clear();
        Matrix3D turn;
        turn.SetRotation(Vector3(0.0f, 1.0f, 0.0f), kPi * 0.5f);
        part.m_transformation *= turn;
        part.m_materials.PushBack(26);
        part.m_materials.PushBack(298);
        break;
    }
    case 75541:
        AddPlacedPart(parser, part, 74747,
                      Placement(Vector3(0.0f, 1.0f, 0.0f), kPi, Vector3(F(0x41933334), 0.0f, F(0xc2166666))));
        AddPlacedPart(parser, part, 74746, Placement(Vector3(0.0f, 0.0f, F(0xc1cccccd))));
        break;
    case 75542:
        AddPlacedPart(parser, part, 74747,
                      Placement(Vector3(0.0f, 1.0f, 0.0f), kPi * 0.125f, Vector3(F(0xc124dd83), 0.0f, F(0xc1c86ad8))));
        AddPlacedPart(parser, part, 74746, Placement(Vector3(0.0f, 0.0f, F(0xc1cccccd))));
        break;
    }
    parser.CurrentRigid().m_parts.PushBack(partRef);
}

// Removes element `index`: the last element takes its place (order not kept). Mac 0x308390
// (std::find + this erase), Win 0x531750. (name inferred)
void EraseUnordered(BasicArray<PartRef>& parts, UInt32 index)
{
    if (index + 1 != parts.GetSize())
        parts[index] = parts.Back();
    parts.PopBack();
}

// The marked points of a 3679 part placed at `a` and a 3680 part placed at `b` coincide (within
// 0.01). Win 0x5af400 (matrices passed by value, the products inlined); inlined on Mac.
// (name inferred)
bool MarksMeet(Matrix3D a, Matrix3D b)
{
    const Vector3 p = Vector3(0.4f, 0.0f, -0.4f) * a;
    const Vector3 q = Vector3(0.4f, F(0x3e937ac4), -0.4f) * b;   // y 0.288046
    return p.IsEqual(q, 0.01f);
}

// A pending 3679 part and the first pending 3680 part whose marked points meet (within 0.01)
// become brick 74340 (3679 in the 3679 part's colour at its place lifted by 0.03, 3680 in the
// 3680 part's colour); the 3680 part leaves the pending list (the last one takes its place).
// False if none fits. NOTE: the shipped database has no brick 74340, so a pair that fits
// dereferences null (both builds). Mac 0x307be0, Win 0x5af4f0 (point test MarksMeet).
// (name inferred)
bool PairPart(SS1::Part* part, Parser& parser)
{
    for (BasicArray<PartRef>::Iterator it = parser.m_pending3680.Begin(); it != parser.m_pending3680.End(); ++it) {
        SS1::Part* other = *it;
        if (!MarksMeet(part->m_transformation, other->m_transformation))
            continue;
        BasicArray<PartRef> parts;
        BuildAssembly(parser.m_database, 74340, 3680, other->m_transformation, parts);
        for (BasicArray<PartRef>::Iterator p = parts.Begin(); p != parts.End(); ++p) {
            PartRef created = *p;
            if (created->m_designID == 3679) {
                created->m_materials.PushBack(part->m_materials[0]);
                Matrix3D lift;
                lift.m[13] = F(0x3cf5c28f);   // 0.03
                created->m_transformation = part->m_transformation;
                created->m_transformation *= lift;
            } else if (created->m_designID == 3680) {
                created->m_materials.PushBack(other->m_materials[0]);
            }
            parser.CurrentRigid().m_parts.PushBack(created);
        }
        EraseUnordered(parser.m_pending3680, (UInt32)(it - parser.m_pending3680.Begin()));
        return true;
    }
    return false;
}

// After a successful read: pairs the pending parts, then the unpaired ones of both lists join
// the current (last) rigid. NOTE: the last model must have a rigid by then (read unchecked,
// both builds). Mac 0x308500, Win 0x5af760 (ESI = parser). (name inferred)
void FinishPending(Parser& parser)
{
    BasicArray<PartRef>& pending = parser.m_pending3679;
    for (UInt32 i = 0; i < pending.GetSize();) {
        if (PairPart(pending[i], parser))
            EraseUnordered(pending, i);   // the moved part is tried next
        else
            ++i;
    }
    for (UInt32 i = 0; i < pending.GetSize(); ++i)
        parser.CurrentRigid().m_parts.PushBack(pending[i]);
    for (UInt32 i = 0; i < parser.m_pending3680.GetSize(); ++i)
        parser.CurrentRigid().m_parts.PushBack(parser.m_pending3680[i]);
}

// ---- handlers ------------------------------------------------------------------------------

void IgnoreEnd(void* data, const char*);

// Mac 0x3090f0, Win 0x5b1480
void IgnoreStart(void* data, const char*, const char**)
{
    P(data).Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x309120, Win 0x5b14e0
void IgnoreEnd(void* data, const char*)
{
    P(data).Pop();
}

// Mac 0x308ba0, Win 0x459790 (shared empty function)
void NoEnd(void*, const char*)
{
}

inline void PopIf(void* data, const char* name, const char* element)
{
    if (StrCmp(name, element) == 0)
        P(data).Pop();
}

// ---- <Scene> ---------------------------------------------------------------------------------

// Children of <Part>: <SubMaterial> and <Decoration>, applied to the last part of the current
// rigid. NOTE: that is not the <Part> being read when the conversion kept it aside (3679, 3680),
// both builds. Mac 0x30b150, Win 0x5b10f0.
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
        // NOTE: CurrentPart() is Back() of the current rigid's parts, which is empty when the
        // conversion kept the first part of a group aside (both builds: reads before the
        // array); fixed: the element is then ignored, here and for <Decoration>.
        if (parser.CurrentRigid().m_parts.IsEmpty())
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
        if (parser.CurrentRigid().m_parts.IsEmpty())
            return;
        parser.CurrentPart()->m_decorations.PushBack(decoration);
        return;
    }
    parser.Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x30b5b0
void PartEnd(void* data, const char* name)
{
    PopIf(data, name, "Part");
}

// Children of <Group>: <Part>. Mac 0x309e80, Win 0x5b0630.
void PartStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Part") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    PartRef part = new SS1::Part;
    TransformAttributes transform;
    UInt32 refID = 0xffffffff;
    UInt32 flexRefID = 0xffffffff;
    UInt32 controlPoint = 0xffffffff;
    UInt32 assemblyRefID = 0xffffffff;
    UInt32 assemblyID = 0;
    for (; *a; a += 2) {
        if (StrCmp(a[0], "refID") == 0)
            refID = ToUInt32(a[1]);
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
    // NOTE: assemblyID without assemblyRefID is accepted (unlike LXFML 2 / 4)
    UInt32 detail;
    if (refID == 0xffffffff)
        detail = ModelLoader::PartError::missingRefID;
    else if (part->m_materials.GetSize() != 1)
        detail = ModelLoader::PartError::notOneMaterial;
    else if (assemblyID == 0 && assemblyRefID != 0xffffffff)
        detail = ModelLoader::PartError::assembly;
    else if ((flexRefID == 0xffffffff) != (controlPoint == 0xffffffff))
        detail = ModelLoader::PartError::flex;
    else
        detail = 0xffffffff;
    if (detail != 0xffffffff) {
        parser.AddError(new ModelLoader::PartError(0, detail));
        return;
    }

    transform.GetMatrix(part->m_transformation);
    // NOTE: a flex part pushes the ignore pair below the part's own pair; "</Part>" pops only the
    // top one, so the ignore pair then takes the part's siblings and the group's end (both builds)
    if (flexRefID != 0xffffffff)
        parser.Push(IgnoreStart, IgnoreEnd);
    if (assemblyID != 0) {
        ConvertAssemblyPart(parser, *part);
        parser.Push(IgnoreStart, IgnoreEnd);
    } else {
        ConvertPart(parser, part);
        parser.Push(PartChildStart, PartEnd);
    }
}

// Mac 0x30a8f0
void GroupEnd(void* data, const char* name)
{
    PopIf(data, name, "Group");
}

// Children of <Joint>: <GroupRef>. Mac 0x30a9b0, Win 0x5b0cb0.
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

// Mac 0x30afb0, Win 0x5b1020
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

// Children of <Model>: <Group> and <Joint>. Mac 0x309830, Win 0x5b0120.
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

// Mac 0x309dc0
void ModelEnd(void* data, const char* name)
{
    PopIf(data, name, "Model");
}

// Children of <Scene>: <Model>. Mac 0x309470, Win 0x5aff10.
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

// Mac 0x309610
void SceneEnd(void* data, const char* name)
{
    PopIf(data, name, "Scene");
}

// ---- <Meta> ----------------------------------------------------------------------------------

void MetaChildStart(void* data, const char*, const char**);
void MetaChildEnd(void* data, const char*);

// Mac 0x3097b0, Win 0x5afe90
void MetaText(void* data, const char* text, int length)
{
    P(data).m_scene->m_metas.Back().m_text.Append(DecodeString(kTextEncodingUTF8, text, (UInt32)length));
}

// Children of <Meta>. Mac 0x3091b0, Win 0x5afbb0.
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

// Mac 0x3096d0, Win 0x5afdd0
void MetaChildStart(void* data, const char*, const char**)
{
    XML_SetCharacterDataHandler(*P(data).m_parser, 0);
    P(data).Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x309710, Win 0x5afe40
void MetaChildEnd(void* data, const char*)
{
    XML_SetCharacterDataHandler(*P(data).m_parser, 0);
    P(data).Pop();
}

// Mac 0x3093b0
void MetaEnd(void* data, const char* name)
{
    PopIf(data, name, "Meta");
}

// ---- <LXFML> ---------------------------------------------------------------------------------

// Children of <LXFML>. Mac 0x308fa0, Win 0x5afa70.
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

// Mac 0x309030
void LXFMLEnd(void* data, const char* name)
{
    PopIf(data, name, "LXFML");
}

// The document element. Mac 0x308970, Win 0x5af810.
void RootStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "LXFML") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    parser.m_found = true;
    Version expected(3, 0, 0, 0);
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
// ModelLoaderLXFML3
// ============================================================================================

ModelLoaderLXFML3::ModelLoaderLXFML3(BrickDatabase* database)                // Win 0x5a1eb0
    : m_database(database)
{
}

ModelLoaderLXFML3::~ModelLoaderLXFML3() {}                                  // Mac 0x308bb0, Win 0x5a1f10

bool ModelLoaderLXFML3::Load(const AString& content, StorageScene& scene, bool flags) // Mac 0x3085c0, Win 0x5b1530
{
    scene.Clear();
    m_errors.Clear();
    m_errorMask = 0;

    XML_Parser expat = XML_ParserCreate(0);
    Parser parser(&scene, flags, this, &expat, m_database);
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
    FinishPending(parser);
    return true;
}

} // namespace LEGO
