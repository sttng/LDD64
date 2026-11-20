// LEGO Digital Designer — restored source
// LEGO::StorageScene2 — the in-memory model ("storage scene", version 2) that the LXFML 5
// loader/saver (ModelIOLXFML5.cpp), the StorageScene v1 converters (ModelIOStorageScene.cpp),
// BrickDatabase assemblies and Scene <-> storage (025o) work on. Every nested type is a
// `struct` in the Win RTTI (.?AUStorageScene2@LEGO@@, .?AUBrick@StorageScene2@LEGO@@ …), so
// all data members are public.
//
// The whole file is header-inline in the original: Mac emits the members as weak functions
// coalesced into the first object that used them (TU 024f BrickDatabase tail, 025s Scene tail,
// 021d, 025a, 335, 342c); Win /LTCG kept some of them out of line (addresses below). Member
// layouts come from the constructors/destructors and from the LXFML 5 loader/saver field uses:
//
//   LXFML 5 element / attribute               member
//   <LXFML name=…>                            StorageScene2::m_name
//   <Meta><X a=… …>text</X></Meta>            m_metas[i] {m_name = X, m_attributes, m_text}
//   <Cameras><Camera refID name distance      m_cameras (refID = index assigned by the saver)
//            fieldOfView orthographic transformation>
//   <Behaviors><Behavior BehaviorName=…>      m_behaviors (also Brick/Part/Bone/Group)
//              <Attribute Key=… Value=…/>     Behavior::m_attributes
//   <Bricks cameraRef=…>                      m_camera
//     <Brick refID designID itemNos>          m_bricks
//       <Part refID designID materials        Brick::m_parts
//             decoration>
//         <Bone refID transformation>         Part::m_bones
//   <RigidSystems><RigidSystem>               m_rigidSystems
//       <Rigid refID transformation boneRefs> RigidSystem::m_rigids
//       <Joint name type="hinge|ball">        RigidSystem::m_joints
//         <RigidRef rigidRef a z t/> ×2
//   <GroupSystems><GroupSystem>               m_groupSystems
//       <Group name transformation pivot      GroupSystem::m_groups, Group::m_groups (nested)
//              partRefs>
//   <BuildingInstructions>                    m_buildingInstructions
//     <BuildingInstruction name>              BI
//       <Step name exploded a>                BIStep (nested steps, BICamera, BIPart events)
//         <Camera cameraRef/>                 BICamera
//         <PartRef partRef exploded           BIPart
//                  transformation/>
//
// Offsets: Mac (String = 16 bytes) / Win (String = 4 bytes).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "Geometry.h"

namespace LEGO {

// sizeof Mac 0x8c / Win 0x80. Mac vtable 0x761df8 {D1 0x8a730, D0 0x8a740, Object slots},
// Win vtable 0x9cdfc4 {sdd 0x46eb00, …}. No user-declared constructor: Mac value-initialises
// (`new StorageScene2()` zero-fills the block before the inlined constructor, TU 440 0x3b6529),
// Win keeps the implicit constructor out of line (0x4901e0).
// Destructor (implicit): Mac 0x8ace0 (via D1 0x8a730 / D0 0x8a740), Win 0x46e950.
struct StorageScene2 : public Object {
    // ---- value types ----------------------------------------------------------------------

    // <Attribute Key=… Value=…/> of a <Behavior>. Mac 0x20 / Win 0x08.
    // Copy constructor (implicit): Win 0x4a2500.
    struct Attribute {
        String m_key;      // Mac +0x00 / Win +0x00 "Key"
        String m_value;    // Mac +0x10 / Win +0x04 "Value"
    };

    // <Behavior BehaviorName=…> with its attributes. Mac 0x20 / Win 0x14.
    // Destructor (implicit): Mac 0x92650, Win 0x49ae70 (array element loop). Copy constructor:
    // Win 0x4a2560.
    struct Behavior {
        String                m_name;        // Mac +0x00 / Win +0x00 "BehaviorName"
        BasicArray<Attribute> m_attributes;  // Mac +0x10 / Win +0x04
    };

    // One child element of <Meta>: <name attr=… …>text</name>. Mac 0x30 / Win 0x18.
    // Destructor (implicit): Mac 0x53f50, Win 0x4902e0. Copy constructor: Win 0x4a2a80.
    struct Meta {
        // Mac 0x20 / Win 0x08. Copy constructor: Win 0x4a2a20.
        struct Attribute {
            String m_name;     // Mac +0x00 / Win +0x00
            String m_value;    // Mac +0x10 / Win +0x04
        };

        BasicArray<Attribute> m_attributes;  // Mac +0x00 / Win +0x00
        String                m_name;        // Mac +0x10 / Win +0x10 element name
        String                m_text;        // Mac +0x20 / Win +0x14 element text ("" -> "/>")
    };

    struct Part;
    struct Bone;

    // <Camera>. sizeof Mac 0x64 / Win 0x58. Mac vtable 0x7606b0 {D1 0x54480, D0 0x544d0},
    // Win vtable 0x9cf128 {sdd 0x4a2bc0}.
    struct Camera : public Object {
        // Mac 0x54400, Win 0x4a2400 (ECX = this)
        Camera() : m_distance(0.0f), m_fieldOfView(80.0f), m_orthographic(false) {}

        String   m_name;             // Mac +0x08 / Win +0x08 "name"
        Real32   m_distance;         // Mac +0x18 / Win +0x0c "distance" (written when > 1e-5)
        Matrix3D m_transformation;   // Mac +0x1c / Win +0x10 "transformation"
        Real32   m_fieldOfView;      // Mac +0x5c / Win +0x50 "fieldOfView" (degrees, default 80)
        bool     m_orthographic;     // Mac +0x60 / Win +0x54 "orthographic"
    };

    // <Brick>. sizeof Mac 0x3c / Win 0x30. Mac vtable 0x761f58 {D1 0x8b4d0, D0 0x8b4e0},
    // Win vtable 0xa23a68 {sdd 0x46eaa0}.
    // Destructor (implicit): Mac 0x8b520, Win 0x46e4f0.
    struct Brick : public Object {
        // Win 0x46e460 (this on the stack, designID in EDX); inlined on Mac (025o 0xb49c0…).
        explicit Brick(UInt32 designID = 0) : m_designID(designID) {}

        UInt32                      m_designID;   // Mac +0x08 / Win +0x08 "designID"
        String                      m_itemNos;    // Mac +0x0c / Win +0x0c "itemNos" (omitted when empty)
        BasicArray<RefPointer<Part> > m_parts;    // Mac +0x1c / Win +0x10 <Part> children
        BasicArray<Behavior>        m_behaviors;  // Mac +0x2c / Win +0x20
    };

    // <Part>. sizeof 0x50 (both). Mac vtable 0x762640 {D1 0xc8c90, D0 0xc8ca0},
    // Win vtable {sdd 0x46ea80}. Destructor (implicit): Mac 0xc8ce0, Win 0x46e3b0.
    struct Part : public Object {
        // Win 0x46e300 (this on the stack, designID in EDX); Mac inlined (342a 0x318ec0).
        explicit Part(UInt32 designID = 0) : m_designID(designID), m_brick(0) {}

        UInt32                      m_designID;     // +0x08 "designID"
        // one material per surface ("materials", comma separated)
        BasicArray<UInt32>          m_materials;    // +0x0c
        // decoration per surface; element 0 is never written: the saver emits "decoration"
        // from index 1 and only when the array has more than one element (342b 0x321db0)
        BasicArray<UInt32>          m_decorations;  // +0x1c
        Brick*                      m_brick;        // +0x2c owning <Brick> (not reference counted)
        BasicArray<RefPointer<Bone> > m_bones;      // +0x30 <Bone> children
        BasicArray<Behavior>        m_behaviors;    // +0x40
    };

    // <Bone>. sizeof 0x5c (both). Mac vtable 0x776f90 {D1 0xc8e70, D0 0xc8f00},
    // Win vtable 0xa23a7c {sdd 0x46ea60}. Destructor (implicit): Win 0x46e2a0.
    struct Bone : public Object {
        // Win 0x46e220 (this on the stack); Mac inlined (342a 0x31a7b0).
        Bone() : m_part(0) {}

        Matrix3D             m_transformation;  // +0x08 "transformation"
        Part*                m_part;            // +0x48 owning <Part> (not reference counted)
        BasicArray<Behavior> m_behaviors;       // +0x4c
    };

    // <Rigid>. sizeof 0x58 (both). Mac vtable 0x762600 {D1 0xc8a60, D0 0xc8b00},
    // Win vtable {sdd 0x46eac0}. Implicit constructor: Win 0x46e570, destructor: Win 0x46e5f0.
    struct Rigid : public Object {
        Matrix3D                      m_transformation;  // +0x08 "transformation"
        BasicArray<RefPointer<Bone> > m_bones;           // +0x48 "boneRefs"
    };

    // <Joint> with its two <RigidRef> children. sizeof Mac 0x64 / Win 0x58.
    // Destroyed inline (Mac 0x53a80 loop: m_rigidB, m_rigidA, m_name).
    struct Joint {
        // "type" attribute: the saver writes it from the table {"hinge", "ball"} (Mac 0x777438)
        // and omits it for other values. (enumerator names from those strings)
        enum Type {
            Hinge = 0,
            Ball  = 1
        };

        // Mac 0xc8fa0 (coalesced into 025s; the StorageScene v1 Joint has the same shape,
        // Mac 0x2fc9f0), Win 0x46e660.
        Joint()
            : m_axisA(0.0f, 1.0f, 0.0f), m_zeroA(0.0f, 0.0f, 1.0f),
              m_axisB(0.0f, 1.0f, 0.0f), m_zeroB(0.0f, 0.0f, 1.0f), m_type(Ball)
        {
        }

        String            m_name;      // Mac +0x00 / Win +0x00 "name" (omitted when empty)
        RefPointer<Rigid> m_rigidA;    // Mac +0x10 / Win +0x04 first <RigidRef rigidRef>
        RefPointer<Rigid> m_rigidB;    // Mac +0x14 / Win +0x08 second <RigidRef rigidRef>
        Vector3           m_axisA;     // Mac +0x18 / Win +0x0c "a" of the first RigidRef
        Vector3           m_zeroA;     // Mac +0x24 / Win +0x18 "z" (name inferred)
        Vector3           m_positionA; // Mac +0x30 / Win +0x24 "t" (name inferred)
        Vector3           m_axisB;     // Mac +0x3c / Win +0x30 "a" of the second RigidRef
        Vector3           m_zeroB;     // Mac +0x48 / Win +0x3c "z"
        Vector3           m_positionB; // Mac +0x54 / Win +0x48 "t"
        UInt32            m_type;      // Mac +0x60 / Win +0x54 Type
    };

    // <RigidSystem>. 0x20 (both). Implicit constructor: Win 0x46e710.
    // Destructor (implicit): Mac 0x53a80, Win 0x49b2c0.
    struct RigidSystem {
        BasicArray<RefPointer<Rigid> > m_rigids;  // +0x00 <Rigid> children
        BasicArray<Joint>              m_joints;  // +0x10 <Joint> children
    };

    struct Group;

    // <GroupSystem>: just the top level groups. 0x10 (both).
    struct GroupSystem {
        BasicArray<RefPointer<Group> > m_groups;  // +0x00
    };

    // <Group>. sizeof Mac 0x94 / Win 0x88. Mac vtable 0x762690 {D1 0xc92c0, D0 0xc92d0},
    // Win vtable 0xa23ab4 {sdd 0x46eae0}. No user-declared constructor (Mac 342a 0x31bac0
    // value-initialises): implicit constructor Mac 0xc91e0, Win 0x483e00; destructor Mac
    // 0xc9310, Win 0x46e780.
    struct Group : public Object {
        String                        m_name;            // Mac +0x08 / Win +0x08 "name"
        Matrix3D                      m_transformation;  // Mac +0x18 / Win +0x0c "transformation"
        Vector3                       m_pivot;           // Mac +0x58 / Win +0x4c "pivot"
        BasicArray<RefPointer<Part> > m_parts;           // Mac +0x64 / Win +0x58 "partRefs"
        BasicArray<Behavior>          m_behaviors;       // Mac +0x74 / Win +0x68
        BasicArray<RefPointer<Group> > m_groups;         // Mac +0x84 / Win +0x78 nested <Group>s
    };

    // ---- building instructions ------------------------------------------------------------

    // Base of the <BuildingInstruction> events. Win vtable 0xa24424 {sdd 0x4a2be0}; Mac never
    // emits it (every constructor is inlined into a derived one).
    struct BIEvent : public Object {
        // m_type values (names inferred from the element names the saver writes)
        enum Type {
            TypeStep   = 0,   // <Step>    -> BIStep
            TypeCamera = 1,   // <Camera>  -> BICamera
            TypePart   = 2    // <PartRef> -> BIPart
        };

        // Win 0x4a25c0 (this, type on the stack); destructor Win 0x4a2620.
        explicit BIEvent(Type type) : m_type(type) {}

        UInt32 m_type;   // +0x08 Type
    };

    // <Step>. sizeof Mac 0x3c / Win 0x30. Mac vtable 0x777550 {D1 0x2f45e0, D0 0x2f45f0},
    // Win vtable 0xa24438 {sdd 0x4a2c50}. Destructor (implicit): Mac 0x2f4630, Win 0x4a2700.
    struct BIStep : public BIEvent {
        // Mac 0x2f44f0, Win 0x4a2670 (this on the stack)
        BIStep() : BIEvent(TypeStep), m_exploded(false) {}

        String                           m_name;      // Mac +0x0c / Win +0x0c "name"
        BasicArray<RefPointer<BIEvent> > m_events;    // Mac +0x1c / Win +0x10 child events
        bool                             m_exploded;  // Mac +0x2c / Win +0x20 "exploded"
        Vector3                          m_explode;   // Mac +0x30 / Win +0x24 "a" (written with exploded)
    };

    // <Camera cameraRef=…/> event. sizeof 0x10 (both). Mac vtable 0x776fb0 {D1 0x2f4420,
    // D0 0x2f4480}, Win vtable 0xa2444c {sdd 0x4a2c70}. Destructor: Win 0x4a2830.
    struct BICamera : public BIEvent {
        // Mac inlined (342a 0x31e7b0), Win 0x4a27a0 (this, camera by value on the stack)
        explicit BICamera(RefPointer<Camera> camera) : BIEvent(TypeCamera), m_camera(camera) {}

        RefPointer<Camera> m_camera;   // +0x0c "cameraRef" (omitted when null)
    };

    // <PartRef partRef=… exploded=… transformation=…/> event. sizeof 0x54 (both).
    // Mac vtable 0x776ffc {D1 0x2f4780, D0 0x2f47e0}, Win vtable 0xa24460 {sdd 0x4a2c90}.
    // Destructor: Win 0x4a2950.
    struct BIPart : public BIEvent {
        // Mac 0x2f46e0, Win 0x4a28a0 (this, part by value on the stack)
        explicit BIPart(RefPointer<Part> part) : BIEvent(TypePart), m_part(part), m_exploded(false) {}

        RefPointer<Part> m_part;             // +0x0c "partRef"
        bool             m_exploded;         // +0x10 "exploded"
        Matrix3D         m_transformation;   // +0x14 "transformation" (written with exploded)
    };

    // <BuildingInstruction name=…>: the top level steps. Mac 0x20 / Win 0x14.
    // Copy constructor: Win 0x4a29c0. Array destructor: Mac 0x8a490, Win 0x49b440.
    struct BI {
        String                           m_name;    // Mac +0x00 / Win +0x00 "name" (the saver
                                                    // writes "BuildingGuide<n>" when empty)
        BasicArray<RefPointer<BIEvent> > m_events;  // Mac +0x10 / Win +0x04
    };

    // ---- the scene ------------------------------------------------------------------------

    // Empties the scene for loading. Inline; Mac copy 0x35890 (coalesced into TU 021), Win
    // 0x4a2af0.
    // NOTE: m_behaviors is not cleared (both builds).
    void Clear()
    {
        m_name.Clear();
        m_cameras.Clear();
        m_bricks.Clear();
        m_rigidSystems.Clear();
        m_groupSystems.Clear();
        m_buildingInstructions.Clear();
        m_metas.Clear();
        m_camera = 0;
    }

    // First <Meta> child with the given element name (exact compare), or null.
    // Win 0x46e8b0 (cdecl(this, name)); inlined on Mac (025n 0xb36a0: "BrickSet"). (name inferred)
    const Meta* FindMeta(const String& name) const
    {
        for (BasicArray<Meta>::ConstIterator it = m_metas.Begin(); it != m_metas.End(); ++it)
            if (it->m_name == name)
                return it;
        return 0;
    }
    Meta* FindMeta(const String& name)
    {
        return const_cast<Meta*>(static_cast<const StorageScene2*>(this)->FindMeta(name));
    }

    String                          m_name;                  // Mac +0x08 / Win +0x08 <LXFML name=…>
    BasicArray<RefPointer<Camera> > m_cameras;               // Mac +0x18 / Win +0x0c
    BasicArray<RefPointer<Brick> >  m_bricks;                // Mac +0x28 / Win +0x1c
    BasicArray<RigidSystem>         m_rigidSystems;          // Mac +0x38 / Win +0x2c
    BasicArray<GroupSystem>         m_groupSystems;          // Mac +0x48 / Win +0x3c
    BasicArray<BI>                  m_buildingInstructions;  // Mac +0x58 / Win +0x4c
    BasicArray<Meta>                m_metas;                 // Mac +0x68 / Win +0x5c
    RefPointer<Camera>              m_camera;                // Mac +0x78 / Win +0x6c <Bricks cameraRef=…>
    BasicArray<Behavior>            m_behaviors;             // Mac +0x7c / Win +0x70 scene <Behaviors>
};

} // namespace LEGO
