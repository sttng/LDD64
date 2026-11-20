// LEGO Digital Designer — restored source
// LXFML 4 reader / writer (ModelLoaderLXFML4, ModelSaverLXFML4). See the header.
// Plan item 2.6, Mac TU 341.
#include "ModelIOLXFML4.h"
#include "TransformAttributes.h"
#include "BasicDeque.h"
#include "BasicMap.h"
#include "LEGO/StdLib.h"
#include "StringConversion.h"
#include "Version.h"
#include "Path.h"
#include "File.h"

#ifndef XML_STATIC   // (the projects define it as well)
#define XML_STATIC   // Expat is linked statically into both programs
#endif
#include "expat.h"

#include <iostream>

namespace LEGO {

typedef StorageScene  SS1;
typedef StorageScene2 SS2;

// ============================================================================================
// ModelLoaderLXFML4::Parser — the Expat user data. Mac 0x118 bytes (Load's frame 0x144),
// constructor 0x30e810, destructor 0x314400; Win constructor 0x4e0890, destructor 0x4e42d0.
// (struct and member names inferred)
// ============================================================================================
struct ModelLoaderLXFML4::Parser {
    typedef void (*StartHandler)(void* data, const char* name, const char** attributes);
    typedef void (*EndHandler)(void* data, const char* name);

    Parser(StorageScene* scene, UInt32 flags, ModelLoaderLXFML4* loader, XML_Parser* parser)
        : m_scene(scene), m_flags(flags), m_loader(loader), m_parser(parser), m_lxfmlCount(0)
    {
    }

    // Makes (start, end) the current handlers. Mac 0x30bd00 (Win inlines the deque push and
    // XML_SetElementHandler).
    void Push(StartHandler start, EndHandler end)
    {
        m_handlers.PushBack(Pair<StartHandler, EndHandler>(start, end));
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    // Back to the previous handlers (inlined into every end handler).
    void Pop()
    {
        m_handlers.PopBack();
        XML_SetElementHandler(*m_parser, m_handlers.Back().first, m_handlers.Back().second);
    }
    void AddError(ModelLoader::Error* error) { m_loader->AddError(error); }

    // The current model / rigid / part / building step.
    SS1::Model& CurrentModel() { return m_scene->m_models.Back(); }
    SS1::Part*  CurrentPart()  { return CurrentModel().m_rigids.Back()->m_parts.Back(); }

    RefPointer<SS1>                         m_scene;            // +0x00
    UInt32                                  m_flags;            // +0x04 bit 0: skip <Joint>s,
                                                                //       bit 1: no BIError for unknown cameras
    ModelLoaderLXFML4*                      m_loader;           // +0x08 (not reference counted)
    XML_Parser*                             m_parser;           // +0x0c
    UInt32                                  m_lxfmlCount;       // +0x10 <LXFML> elements seen
    Version                                 m_brickSetVersion;  // +0x14 <Meta><BrickSet version>
    BasicDeque<SS1::BIStep*>                m_steps;            // +0x24 open <Step>s
    BasicMap<UInt32, SS2::Camera*>          m_cameras;          // +0x50 refID -> element
    BasicMap<UInt32, SS1::Rigid*>           m_rigids;           // +0x6c
    BasicMap<UInt32, SS1::Part*>            m_parts;            // +0x88
    BasicMap<UInt32, SS1::Assembly*>        m_assemblies;       // +0xa4
    BasicMap<UInt32, SS1::Flex*>            m_flexes;           // +0xc0
    BasicArray<SS1::Flex*>                  m_flexList;         // +0xdc every Flex created
    BasicDeque<Pair<StartHandler, EndHandler> > m_handlers;     // +0xec
};

namespace {

typedef ModelLoaderLXFML4::Parser Parser;

inline Parser& P(void* data) { return *static_cast<Parser*>(data); }

inline String FromUTF8(const char* s)                           // Win 0x5ae270
{
    return DecodeString(kTextEncodingUTF8, s, StrLen(s));
}

inline UInt32 ToUInt32(const char* s) { return StrToUInt32(s, 0, 10); }

// ---- the generic handlers ------------------------------------------------------------------

void IgnoreEnd(void* data, const char*);

// Skips an element and everything in it. Mac 0x30efa0, Win 0x4e3f70.
void IgnoreStart(void* data, const char*, const char**)
{
    P(data).Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x30efd0, Win 0x4e3fd0
void IgnoreEnd(void* data, const char*)
{
    P(data).Pop();
}

// The end handler of the document level. Mac 0x30c070, Win 0x459790 (shared empty function).
void NoEnd(void*, const char*)
{
}

// Pops when `name` closes (the end handlers of the container elements).
inline void PopIf(void* data, const char* name, const char* element)
{
    if (StrCmp(name, element) == 0)
        P(data).Pop();
}

// ---- <BuildingInstructions> ----------------------------------------------------------------

void StepStart(void* data, const char* name, const char** attributes);
void StepEnd(void* data, const char* name);

// <Step> attributes into `step`. (inline in the original: Mac 0x313290 / 0x313960)
void ReadStep(Parser& parser, SS1::BIStep* step, const char** a)
{
    for (; *a; a += 2) {
        if (StrCmp(a[0], "name") == 0) {
            step->m_name = FromUTF8(a[1]);
        } else if (StrCmp(a[0], "cameraRefID") == 0) {
            BasicMap<UInt32, SS2::Camera*>::Iterator it = parser.m_cameras.Find(ToUInt32(a[1]));
            if (it != parser.m_cameras.End())
                step->m_camera = it->second;
            else if ((parser.m_flags & 2) == 0)
                parser.AddError(new ModelLoader::BIError(ModelLoader::BIError::nonExistingCamera));
        } else if (StrCmp(a[0], "exploded") == 0) {
            step->m_exploded = true;   // NOTE: whatever the value (both builds)
        } else if (StrCmp(a[0], "ax") == 0) {
            step->m_explode.x = StrToReal32(a[1], 0);
        } else if (StrCmp(a[0], "ay") == 0) {
            step->m_explode.y = StrToReal32(a[1], 0);
        } else if (StrCmp(a[0], "az") == 0) {
            step->m_explode.z = StrToReal32(a[1], 0);
        }
    }
}

// Children of <Step>: <PartRef> (no handlers pushed) and nested <Step>s. Mac 0x313960,
// Win 0x4e3880.
void StepStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "PartRef") == 0) {
        SS1::BIPart part;
        TransformAttributes transform;
        UInt32 refID = 0xffffffff;
        for (; *a; a += 2) {
            if (StrCmp(a[0], "partRefID") == 0)
                refID = ToUInt32(a[1]);
            else if (StrCmp(a[0], "exploded") == 0)
                part.m_exploded = true;   // NOTE: whatever the value
            else
                transform.ParseAttribute(a[0], a[1]);
        }
        BasicMap<UInt32, SS1::Part*>::Iterator it = parser.m_parts.Find(refID);
        if (it == parser.m_parts.End()) {
            parser.AddError(new ModelLoader::BIError(ModelLoader::BIError::nonExistingPart));
            return;
        }
        part.m_part = it->second;
        if (part.m_exploded)
            transform.GetMatrix(part.m_transformation);
        parser.m_steps.Back()->m_parts.PushBack(part);
        return;
    }
    if (StrCmp(name, "Step") == 0) {
        RefPointer<SS1::BIStep> step = new SS1::BIStep;
        ReadStep(parser, step, a);
        SS1::BIStep* parent = parser.m_steps.Back();
        parent->m_steps.PushBack(step);
        parser.m_steps.PushBack(parent->m_steps.Back());
        parser.Push(StepStart, StepEnd);
        return;
    }
    parser.Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x3141d0, Win 0x4e3f10
void StepEnd(void* data, const char* name)
{
    if (StrCmp(name, "Step") == 0) {
        P(data).m_steps.PopBack();
        P(data).Pop();
    }
}

// Children of <BuildingInstruction>: <Step>. Mac 0x313290, Win 0x4e3400.
void InstructionStepStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Step") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    RefPointer<SS1::BIStep> step = new SS1::BIStep;
    ReadStep(parser, step, a);
    BasicArray<RefPointer<SS1::BIStep> >& steps = parser.m_scene->m_buildingInstructions.Back().m_steps;
    steps.PushBack(step);
    parser.m_steps.PushBack(steps.Back());
    parser.Push(StepStart, StepEnd);
}

// Mac 0x313760, Win 0x4e3830
void InstructionEnd(void* data, const char* name)
{
    PopIf(data, name, "BuildingInstruction");
}

// Children of <BuildingInstructions>: <BuildingInstruction>. Mac 0x30ffb0, Win 0x4e3120.
void InstructionStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "BuildingInstruction") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    SS1::BI instruction;
    for (; *a; a += 2) {
        if (StrCmp(a[0], "name") == 0) {
            instruction.m_name = FromUTF8(a[1]);
        } else if (StrCmp(a[0], "cameraRefID") == 0) {
            BasicMap<UInt32, SS2::Camera*>::Iterator it = parser.m_cameras.Find(ToUInt32(a[1]));
            if (it != parser.m_cameras.End())
                instruction.m_camera = it->second;
            else if ((parser.m_flags & 2) == 0)
                parser.AddError(new ModelLoader::BIError(ModelLoader::BIError::nonExistingCamera));
        }
    }
    parser.m_scene->m_buildingInstructions.PushBack(instruction);
    parser.Push(InstructionStepStart, InstructionEnd);
}

// Mac 0x3102e0, Win 0x4e33b0
void InstructionsEnd(void* data, const char* name)
{
    PopIf(data, name, "BuildingInstructions");
}

// ---- <Scene> ---------------------------------------------------------------------------------

// Corrections for parts saved with older brick sets (needs <Meta><BrickSet version>) and two
// renumbered design ids. Mac 0x30b8a0 (part, parser), Win 0x4e1fd0 (EBX = part, EAX = parser).
// (name inferred)
void FixPart(SS1::Part* part, Parser& parser)
{
    const Version& version = parser.m_brickSetVersion;
    if (version.m_major == 0)
        return;
    // NOTE: Win remaps first and then tests, Mac tests 30363 first; same result
    if (part->m_designID == 71842)
        part->m_designID = 22667;
    else if (part->m_designID == 46212)
        part->m_designID = 2454;
    UInt32 designID = part->m_designID;
    bool fix = (designID == 30363 && version < Version(38)) ||
               (designID == 30499 && version < Version(38)) ||
               (designID == 2460 && version < Version(38, 3)) ||
               (designID == 3788 && version < Version(39));
    if (fix) {
        Matrix3D correction;
        correction.SetTranslation(Vector3(-0.8f, 0.0f, 0.0f));
        part->m_transformation *= correction;
    }
}

// Children of <Part>: <SubMaterial> and <Decoration> (no handlers pushed). Mac 0x312d60,
// Win 0x4e2da0.
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

// Mac 0x3131c0
void PartEnd(void* data, const char* name)
{
    PopIf(data, name, "Part");
}

// Children of <Group>: <Part>. Mac 0x310f80, Win 0x4e2100.
void PartStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Part") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    RefPointer<SS1::Part> part = new SS1::Part;
    TransformAttributes transform;
    UInt32 refID = 0xffffffff;
    UInt32 flexRefID = 0xffffffff;
    UInt32 controlPoint = 0xffffffff;
    UInt32 assemblyRefID = 0xffffffff;
    UInt32 assemblyID = 0;
    for (; *a; a += 2) {
        if (StrCmp(a[0], "refID") == 0)
            refID = ToUInt32(a[1]);
        else if (StrCmp(a[0], "designID") == 0)
            part->m_designID = ToUInt32(a[1]);
        else if (StrCmp(a[0], "materialID") == 0)
            part->m_materials.PushBack(ToUInt32(a[1]));
        else if (StrCmp(a[0], "itemNos") == 0)
            part->m_itemNos = a[1];   // NOTE: ASCII (String(const char*)), unlike "name"
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
    if (refID == 0xffffffff)
        detail = ModelLoader::PartError::missingRefID;
    else if (part->m_materials.GetSize() != 1)
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
    FixPart(part, parser);
    parser.m_parts[refID] = part;
    parser.CurrentModel().m_rigids.Back()->m_parts.PushBack(part);
    parser.Push(PartChildStart, PartEnd);
}

// Mac 0x3124f0
void GroupEnd(void* data, const char* name)
{
    PopIf(data, name, "Group");
}

// Children of <Joint>: <GroupRef> (no handlers pushed): the first one fills the A side, the
// second the B side, a third one is an error. Mac 0x3125c0, Win 0x4e2960.
void GroupRefStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "GroupRef") != 0)
        return;   // NOTE: no handlers pushed for other elements either
    SS1::Joint& joint = parser.CurrentModel().m_joints.Back();
    // the joint's address is the error detail (both builds)
    const UInt32 detail = (UInt32)(size_t)&joint;
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
    // both directions must be unit vectors
    if (Abs(axis->GetLength() - 1.0f) < 1e-5f && Abs(zero->GetLength() - 1.0f) < 1e-5f)
        return;
    parser.AddError(new ModelLoader::JointError(detail));
}

// A joint needs two different rigids. Mac 0x312bb0, Win 0x4e2cd0.
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

// Children of <Model>: <Group> and <Joint>. Mac 0x310920, Win 0x4e1af0.
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
        // NOTE: a group without refID is kept (no error), it just cannot be referenced
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

// Mac 0x310eb0
void ModelEnd(void* data, const char* name)
{
    PopIf(data, name, "Model");
}

// Children of <Scene>: <Model>. Mac 0x30fd40, Win 0x4e1530.
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

// Mac 0x30fee0
void SceneEnd(void* data, const char* name)
{
    PopIf(data, name, "Scene");
}

// ---- <Cameras> -------------------------------------------------------------------------------

// Children of <Cameras>: <Camera> (no handlers pushed). Mac 0x30f7c0, Win 0x4e16e0.
void CameraStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Camera") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    RefPointer<SS2::Camera> camera = new SS2::Camera;
    TransformAttributes transform;
    UInt32 refID = 0xffffffff;
    for (; *a; a += 2) {
        if (StrCmp(a[0], "name") == 0)
            camera->m_name = FromUTF8(a[1]);
        else if (StrCmp(a[0], "refID") == 0)
            refID = ToUInt32(a[1]);
        else if (StrCmp(a[0], "distance") == 0)
            camera->m_distance = StrToReal32(a[1], 0);
        else if (StrCmp(a[0], "fieldOfView") == 0)
            camera->m_fieldOfView = StrToReal32(a[1], 0);
        else if (StrCmp(a[0], "orthographic") == 0)
            camera->m_orthographic = StrCmp(a[1], "true") == 0;
        else
            transform.ParseAttribute(a[0], a[1]);
    }
    if (refID == 0xffffffff) {
        parser.AddError(new ModelLoader::CameraError(ModelLoader::CameraError::missingRefID));
        return;
    }
    transform.GetMatrix(camera->m_transformation);
    parser.m_cameras[refID] = camera;
    parser.m_scene->m_cameras.PushBack(camera);
}

// Mac 0x30fc70, Win 0x4e1aa0
void CamerasEnd(void* data, const char* name)
{
    PopIf(data, name, "Cameras");
}

// ---- <Meta> ----------------------------------------------------------------------------------

void MetaChildStart(void* data, const char*, const char**);
void MetaChildEnd(void* data, const char*);

// Text of a <Meta> child. Mac 0x3104a0, Win 0x4e14b0.
void MetaText(void* data, const char* text, int length)
{
    P(data).m_scene->m_metas.Back().m_text.Append(DecodeString(kTextEncodingUTF8, text, (UInt32)length));
}

// Children of <Meta>: every element becomes a StorageScene2::Meta. Mac 0x30f3f0, Win 0x4e1040.
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
        if (meta.m_name == String("BrickSet") && attribute.m_name == String("version")) {
            UInt32 position = 0;
            FromString(parser.m_brickSetVersion, attribute.m_value, position);
        }
    }
    parser.m_scene->m_metas.PushBack(meta);
    parser.Push(MetaChildStart, MetaChildEnd);
    XML_SetCharacterDataHandler(*parser.m_parser, MetaText);
}

// An element inside a <Meta> child: ignored, and the text collection stops. Mac 0x3103b0,
// Win 0x4e13f0.
void MetaChildStart(void* data, const char*, const char**)
{
    XML_SetCharacterDataHandler(*P(data).m_parser, 0);
    P(data).Push(IgnoreStart, IgnoreEnd);
}

// Mac 0x3103f0, Win 0x4e1460
void MetaChildEnd(void* data, const char*)
{
    XML_SetCharacterDataHandler(*P(data).m_parser, 0);
    P(data).Pop();
}

// Mac 0x30f6f0
void MetaEnd(void* data, const char* name)
{
    PopIf(data, name, "Meta");
}

// ---- <LXFML> ---------------------------------------------------------------------------------

// Children of <LXFML>. Mac 0x30f070, Win 0x4e0d50.
void LXFMLChildStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "Meta") == 0) {
        parser.Push(MetaStart, MetaEnd);
    } else if (StrCmp(name, "Cameras") == 0) {
        parser.Push(CameraStart, CamerasEnd);
    } else if (StrCmp(name, "Scene") == 0) {
        for (; *a; a += 2) {
            if (StrCmp(a[0], "cameraRefID") == 0) {
                BasicMap<UInt32, SS2::Camera*>::Iterator it = parser.m_cameras.Find(ToUInt32(a[1]));
                if (it != parser.m_cameras.End())
                    parser.m_scene->m_camera = it->second;
                else
                    parser.AddError(new ModelLoader::CameraError(ModelLoader::CameraError::nonExistingCamera));
            }
        }
        parser.Push(ModelStart, SceneEnd);
    } else if (StrCmp(name, "BuildingInstructions") == 0) {
        parser.Push(InstructionStart, InstructionsEnd);
    } else {
        parser.Push(IgnoreStart, IgnoreEnd);
    }
}

// Mac 0x30f320
void LXFMLEnd(void* data, const char* name)
{
    PopIf(data, name, "LXFML");
}

// The document element. Mac 0x30be20, Win 0x4e0b30.
void RootStart(void* data, const char* name, const char** a)
{
    Parser& parser = P(data);
    if (StrCmp(name, "LXFML") != 0) {
        parser.Push(IgnoreStart, IgnoreEnd);
        return;
    }
    // NOTE: a nested <LXFML> pushes the ignore handlers and then its own (both builds)
    if (++parser.m_lxfmlCount > 1)
        parser.Push(IgnoreStart, IgnoreEnd);
    Version expected(4, 0, 0, 0);
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
// ModelLoaderLXFML4
// ============================================================================================

ModelLoaderLXFML4::ModelLoaderLXFML4() {}                                   // Win 0x4b3360

ModelLoaderLXFML4::~ModelLoaderLXFML4() {}                                  // Mac 0x30e690, Win 0x4b3410

bool ModelLoaderLXFML4::Load(const AString& content, StorageScene& scene, bool flags) // Mac 0x30b9f0, Win 0x4e4020
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
    if (parser.m_lxfmlCount == 0) {
        AddError(new ContentError(ContentError::missingLXFMLTag));
        return false;
    }
    // the parts of a flex element must all exist and belong to the same assembly (no error)
    // NOTE: Win reads the first part's assembly before its null check (0x4e4267: a flex without
    // control point 0 crashes); Mac checks first, as here.
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

// ============================================================================================
// ModelSaverLXFML4::Writer — the text and the ids. Mac ctor 0x3146b0, Win ctor 0x4e43c0
// (ECX = scene), dtor 0x4e6630.
// (struct and member names inferred)
// ============================================================================================
struct ModelSaverLXFML4::Writer {
    explicit Writer(const StorageScene* scene) : m_scene(scene) {}

    // Next id for `element` (assigned on first sight).
    template <class T>
    static UInt32 Assign(BasicMap<const T*, UInt32>& ids, const T* element)
    {
        UInt32 id = ids.GetSize();
        ids[element] = id;
        return id;
    }

    void Indent(UInt32 n) { m_text.Append(n, ' '); }
    // ` name="value"`
    // NOTE: the original writes the value as it is (Win 0x4e4810, 0x4e5f00 … call no
    // String::ToXML), so a name or item number with & < > " ' breaks the file; fixed: the value
    // is escaped.
    void Attribute(const char* name, const String& value)
    {
        m_text << " " << name << "=\"" << value.ToXML() << "\"";
    }
    void Attribute(const char* name, UInt32 value)
    {
        m_text << " " << name << "=\"" << value << "\"";
    }
    // `angle="…" ax="…" … tz="…"` (no leading blank). Mac 0x3148c0, Win 0x4e07b0 (ESI = t).
    void Transformation(const Matrix3D& m)
    {
        TransformAttributes t(m);
        m_text << "angle=\"" << t.GetDegrees() << "\" ax=\"" << t.m_axis.x << "\" ay=\"" << t.m_axis.y
               << "\" az=\"" << t.m_axis.z << "\" tx=\"" << t.m_translation.x << "\" ty=\""
               << t.m_translation.y << "\" tz=\"" << t.m_translation.z << "\"";
    }
    // `px="…" py="…" pz="…"`. Mac 0x314a00, Win 0x4e52a0 (EDI = prefix, ESI = v).
    void Vector(const String& prefix, const Vector3& v)
    {
        m_text << prefix << "x=\"" << v.x << "\" " << prefix << "y=\"" << v.y << "\" " << prefix
               << "z=\"" << v.z << "\"";
    }

    void Metas();                                                       // Win 0x4e4490
    void Cameras();                                                     // Win 0x4e4590
    void Group(const SS1::Rigid* rigid);                                // Win 0x4e4810
    void Scene();                                                       // Win 0x4e5330
    void Steps(const BasicArray<RefPointer<SS1::BIStep> >& steps, UInt32 indent); // Mac 0x314db0, Win 0x4e5ad0
    void Instructions();                                                // Win 0x4e5f00
    void Document();                                                    // inlined into Save

    RefPointer<const SS1>                    m_scene;          // +0x00
    String                                   m_text;           // Mac +0x04 / Win +0x04
    BasicMap<const SS2::Camera*, UInt32>     m_cameraIDs;      // Mac +0x14 / Win +0x08
    BasicMap<const SS1::Rigid*, UInt32>      m_rigidIDs;       // Win +0x2c
    BasicMap<const SS1::Part*, UInt32>       m_partIDs;        // Win +0x50
    BasicMap<const SS1::Assembly*, UInt32>   m_assemblyIDs;    // Win +0x74
    BasicMap<const SS1::Flex*, UInt32>       m_flexIDs;        // Win +0x98
};

void ModelSaverLXFML4::Writer::Metas()                                       // Win 0x4e4490
{
    const BasicArray<SS2::Meta>& metas = m_scene->m_metas;
    for (BasicArray<SS2::Meta>::ConstIterator it = metas.Begin(); it != metas.End(); ++it) {
        Indent(4);
        m_text << "<" << it->m_name;
        // NOTE: the original writes the attribute values and the text as they are (no
        // String::ToXML, Win 0x4e4490); fixed: both are escaped.
        for (BasicArray<SS2::Meta::Attribute>::ConstIterator a = it->m_attributes.Begin(); a != it->m_attributes.End(); ++a)
            m_text << " " << a->m_name << "=\"" << a->m_value.ToXML() << "\"";
        if (it->m_text.IsEmpty())
            m_text << "/>\r\n";
        else
            m_text << ">" << it->m_text.ToXML() << "</" << it->m_name << ">\r\n";
    }
}

void ModelSaverLXFML4::Writer::Cameras()                                     // Win 0x4e4590
{
    const BasicArray<RefPointer<SS2::Camera> >& cameras = m_scene->m_cameras;
    for (BasicArray<RefPointer<SS2::Camera> >::ConstIterator it = cameras.Begin(); it != cameras.End(); ++it) {
        const SS2::Camera* camera = it->Get();
        Indent(4);
        m_text << "<" << "Camera";
        Attribute("refID", Assign(m_cameraIDs, camera));
        if (!camera->m_name.IsEmpty())
            Attribute("name", camera->m_name);
        m_text << " " << "fieldOfView" << "=\"" << camera->m_fieldOfView << "\"";
        if (camera->m_orthographic)
            m_text << " orthographic=\"true\"";
        if (1e-5f < camera->m_distance)
            m_text << " " << "distance" << "=\"" << camera->m_distance << "\"";
        m_text << " ";
        Transformation(camera->m_transformation);
        m_text << "/>\r\n";
    }
}

void ModelSaverLXFML4::Writer::Group(const SS1::Rigid* rigid)                // Win 0x4e4810
{
    Indent(6);
    m_text << "<" << "Group";
    Attribute("refID", Assign(m_rigidIDs, rigid));
    if (!rigid->m_name.IsEmpty())
        Attribute("name", rigid->m_name);
    m_text << " ";
    Transformation(rigid->m_transformation);
    m_text << ">\r\n";

    for (BasicArray<RefPointer<SS1::Part> >::ConstIterator p = rigid->m_parts.Begin(); p != rigid->m_parts.End(); ++p) {
        const SS1::Part* part = p->Get();
        Indent(8);
        m_text << "<" << "Part";
        Attribute("refID", Assign(m_partIDs, part));
        if (!part->m_name.IsEmpty())
            Attribute("name", part->m_name);
        const SS1::Flex* flex = part->m_flex;
        // NOTE: materialID is the first material; a part without one reads past the array
        m_text << " " << "designID" << "=\"" << (flex ? flex->m_designID : part->m_designID) << "\" "
               << "materialID" << "=\"" << part->m_materials[0] << "\"";
        if (!part->m_itemNos.IsEmpty())
            Attribute("itemNos", part->m_itemNos);
        if (flex) {
            if (!m_flexIDs.Contains(flex))
                Assign(m_flexIDs, flex);
            for (UInt32 i = 0; i < flex->m_parts.GetSize(); ++i) {
                if (flex->m_parts[i] == part) {
                    m_text << " " << "flexRefID" << "=\"" << m_flexIDs[flex] << "\" " << "controlPoint"
                           << "=\"" << i << "\"";
                    break;
                }
            }
        }
        if (const SS1::Assembly* assembly = part->m_assembly) {
            if (!m_assemblyIDs.Contains(assembly))
                Assign(m_assemblyIDs, assembly);
            m_text << " " << "assemblyID" << "=\"" << assembly->m_designID << "\" " << "assemblyRefID"
                   << "=\"" << m_assemblyIDs[assembly] << "\"";
        }
        m_text << " ";
        Transformation(part->m_transformation);
        if (part->m_decorations.IsEmpty() && part->m_materials.GetSize() == 1) {
            m_text << "/>\r\n";
            continue;
        }
        m_text << ">\r\n";
        for (BasicArray<SS1::Decoration>::ConstIterator d = part->m_decorations.Begin(); d != part->m_decorations.End(); ++d) {
            Indent(10);
            m_text << "<" << "Decoration" << " " << "surfaceID" << "=\"" << d->m_surface << "\" "
                   << "decorationID" << "=\"" << d->m_decoration << "\"/>\r\n";
        }
        for (UInt32 i = 1; i < part->m_materials.GetSize(); ++i) {
            UInt32 material = part->m_materials[i];
            if (material == 0)
                continue;
            Indent(10);
            m_text << "<" << "SubMaterial" << " " << "surfaceID" << "=\"" << i << "\" " << "materialID"
                   << "=\"" << material << "\"/>\r\n";
        }
        Indent(8);
        m_text << "</" << "Part" << ">\r\n";
    }
    Indent(6);
    m_text << "</" << "Group" << ">\r\n";
}

void ModelSaverLXFML4::Writer::Scene()                                       // Win 0x4e5330
{
    const BasicArray<SS1::Model>& models = m_scene->m_models;
    for (BasicArray<SS1::Model>::ConstIterator model = models.Begin(); model != models.End(); ++model) {
        // NOTE: a model without rigids is left out, with its joints
        if (model->m_rigids.IsEmpty())
            continue;
        Indent(4);
        m_text << "<" << "Model";
        if (!model->m_name.IsEmpty())
            Attribute("name", model->m_name);
        m_text << ">\r\n";
        for (BasicArray<RefPointer<SS1::Rigid> >::ConstIterator r = model->m_rigids.Begin(); r != model->m_rigids.End(); ++r)
            Group(r->Get());
        for (BasicArray<SS1::Joint>::ConstIterator j = model->m_joints.Begin(); j != model->m_joints.End(); ++j) {
            Indent(6);
            m_text << "<" << "Joint";
            if (!j->m_name.IsEmpty())
                Attribute("name", j->m_name);
            // NOTE: written 0 = "hinge", 1 = "ball"; the reader stores "hinge" as 1 (both builds)
            if (j->m_type == 0)
                m_text << " " << "type" << "=\"" << "hinge" << "\"";
            else if (j->m_type == 1)
                m_text << " " << "type" << "=\"" << "ball" << "\"";
            m_text << ">\r\n";
            for (int side = 0; side < 2; ++side) {
                const SS1::Rigid* rigid = side == 0 ? j->m_rigidA.Get() : j->m_rigidB.Get();
                Indent(8);
                m_text << "<" << "GroupRef" << " " << "groupRef" << "=\"" << m_rigidIDs[rigid] << "\" ";
                Vector(String("a"), side == 0 ? j->m_axisA : j->m_axisB);
                m_text << " ";
                Vector(String("z"), side == 0 ? j->m_zeroA : j->m_zeroB);
                m_text << " ";
                Vector(String("t"), side == 0 ? j->m_positionA : j->m_positionB);
                m_text << "/>\r\n";
            }
            Indent(6);
            m_text << "</" << "Joint" << ">\r\n";
        }
        Indent(4);
        m_text << "</" << "Model" << ">\r\n";
    }
}

void ModelSaverLXFML4::Writer::Steps(const BasicArray<RefPointer<SS1::BIStep> >& steps, UInt32 indent) // Mac 0x314db0, Win 0x4e5ad0
{
    for (BasicArray<RefPointer<SS1::BIStep> >::ConstIterator s = steps.Begin(); s != steps.End(); ++s) {
        const SS1::BIStep* step = s->Get();
        Indent(indent);
        m_text << "<" << "Step";
        Attribute("name", step->m_name);   // NOTE: written even when empty
        if (!step->m_camera.IsNull())
            Attribute("cameraRefID", m_cameraIDs[step->m_camera.Get()]);
        if (step->m_exploded) {
            m_text << " " << "exploded" << "=\"true\" " << "ax" << "=\"" << step->m_explode.x << "\" "
                   << "ay" << "=\"" << step->m_explode.y << "\" " << "az" << "=\"" << step->m_explode.z << "\"";
        }
        m_text << ">\r\n";
        Steps(step->m_steps, indent + 2);
        for (BasicArray<SS1::BIPart>::ConstIterator p = step->m_parts.Begin(); p != step->m_parts.End(); ++p) {
            Indent(indent + 2);
            m_text << "<" << "PartRef";
            Attribute("partRefID", m_partIDs[p->m_part.Get()]);
            if (p->m_exploded) {
                m_text << " " << "exploded" << "=\"true\" ";
                Transformation(p->m_transformation);
            }
            m_text << "/>\r\n";
        }
        Indent(indent);
        m_text << "</" << "Step" << ">\r\n";
    }
}

void ModelSaverLXFML4::Writer::Instructions()                                // Win 0x4e5f00
{
    const BasicArray<SS1::BI>& instructions = m_scene->m_buildingInstructions;
    for (BasicArray<SS1::BI>::ConstIterator bi = instructions.Begin(); bi != instructions.End(); ++bi) {
        // NOTE: an instruction without steps is left out
        if (bi->m_steps.IsEmpty())
            continue;
        Indent(4);
        m_text << "<" << "BuildingInstruction";
        if (!bi->m_name.IsEmpty())
            Attribute("name", bi->m_name);
        if (!bi->m_camera.IsNull())
            Attribute("cameraRefID", m_cameraIDs[bi->m_camera.Get()]);
        m_text << ">\r\n";
        Steps(bi->m_steps, 6);
        Indent(4);
        m_text << "</" << "BuildingInstruction" << ">\r\n";
    }
}

void ModelSaverLXFML4::Writer::Document()                                    // Mac 0x30c080, Win 0x4e6050
{
    const SS1& scene = *m_scene;
    m_text << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\r\n";
    m_text << "<" << "LXFML" << " " << "versionMajor" << "=\"4\" " << "versionMinor" << "=\"0\"";
    if (!scene.m_name.IsEmpty())
        Attribute("name", scene.m_name);
    m_text << ">\r\n";
    if (!scene.m_metas.IsEmpty()) {
        m_text << "  <" << "Meta" << ">\r\n";
        Metas();
        m_text << "  </" << "Meta" << ">\r\n";
    }
    if (!scene.m_cameras.IsEmpty()) {
        m_text << "  <" << "Cameras" << ">\r\n";
        Cameras();
        m_text << "  </" << "Cameras" << ">\r\n";
    }
    // NOTE: no <Scene> without models, and then no <BuildingInstructions> either
    if (!scene.m_models.IsEmpty()) {
        m_text << "  <" << "Scene";
        if (!scene.m_camera.IsNull())
            Attribute("cameraRefID", m_cameraIDs[scene.m_camera.Get()]);
        m_text << ">\r\n";
        Scene();
        m_text << "  </" << "Scene" << ">\r\n";
        if (!scene.m_buildingInstructions.IsEmpty()) {
            m_text << "  <" << "BuildingInstructions" << ">\r\n";
            Instructions();
            m_text << "  </" << "BuildingInstructions" << ">\r\n";
        }
    }
    m_text << "</" << "LXFML" << ">\r\n";
}

// ============================================================================================
// ModelSaverLXFML4
// ============================================================================================

ModelSaverLXFML4::ModelSaverLXFML4() {}                                     // Win 0x601572

ModelSaverLXFML4::~ModelSaverLXFML4() {}                                    // Mac 0x30e6e0, Win 0x6015a6

void ModelSaverLXFML4::Write(String& out, const StorageScene& scene)
{
    Writer writer(&scene);
    writer.Document();
    out = writer.m_text;
}

bool ModelSaverLXFML4::Save(const Path& path, const StorageScene& scene)    // Mac 0x30c080, Win 0x4e6050
{
    String text;
    Write(text, scene);
    AString bytes = EncodeString(kTextEncodingUTF8, text, 0, String::npos);
    RefPointer<File> file = path.OpenFile(OpenMode(false, true), true);
    if (file.IsNull()) {
        String message("Unable to open file: ");
        message << path.GetPath(true);
        m_errorMessage = message;
        return false;
    }
    if (!file->Write(bytes.GetLength(), bytes.GetBuffer(), 0)) {
        String message("Unable to write to file: ");
        message << path.GetPath(true);
        m_errorMessage = message;
        return false;
    }
    file->SetSize(bytes.GetLength());
    return true;
}

} // namespace LEGO
