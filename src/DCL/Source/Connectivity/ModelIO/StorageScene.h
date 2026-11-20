// LEGO Digital Designer — restored source
// LEGO::StorageScene — the older in-memory model ("storage scene" version 1) that the LXFML 2/3/4
// loaders (ModelIOLXFML2/3/4.cpp), the LXFML 4 saver and the LDraw saver work on. It is converted
// to / from StorageScene2 by ModelLoaderStorageScene / ModelSaverStorageScene
// (ModelIOStorageScene.cpp). Every nested type is a `struct` in the Win RTTI
// (.?AUStorageScene@LEGO@@, .?AUPart@StorageScene@LEGO@@ …), so all data members are public.
//
// Header-inline in the original: Mac emits the members as weak functions in the first object that
// used them (TU 335 ModelIOStorageScene.cpp: destructors, Part/Rigid/BIStep constructors; TU 337:
// Clear; TU 339/341: Flex). Layouts from those constructors/destructors and the converters.
//
// Model of the version 1 scene: a <Model> holds rigid bodies (<Group>s in LXFML 4) with their parts
// and the joints between them. A Part is one bone of the version 2 part: several Parts that belong
// to one flexible element share a Flex, several Parts that form one brick (an assembly) share an
// Assembly. Building instructions are a tree of steps with part events.
//
// Offsets: Mac (String = 16 bytes) / Win (String = 4 bytes).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "StorageScene2.h"

namespace LEGO {

// sizeof Mac 0x6c / Win 0x60. Mac vtable 0x776f04 {D1 0x2f3f10, D0 0x2f3f20}, Win vtable 0xa256d0
// {sdd 0x4d8080}. Constructor inlined everywhere (Mac 0x2efbd0, Win 0x4d8650); destructor Mac
// 0x2f4160, Win 0x4d7f30.
struct StorageScene : public Object {
    struct Part;

    // One decorated surface of a part. 8 bytes (both).
    struct Decoration {
        UInt32 m_surface;      // +0x00 surface index (index into StorageScene2::Part::m_decorations)
        UInt32 m_decoration;   // +0x04 decoration id
    };

    // The parts that form one brick (LXFML 4 <Part assemblyRefID=… assemblyID=…>). sizeof 0x1c.
    // Mac vtable 0x7771f0 {D1 0x2f4dc0, D0 0x2f4e00}, Win vtable 0xa256ec {sdd 0x4e0720, shared
    // with Flex by /OPT:ICF}. Constructor Win 0x4d7c30 (designID in EDX).
    struct Assembly : public Object {
        explicit Assembly(UInt32 designID = 0) : m_designID(designID) {}

        UInt32              m_designID;   // +0x08 the brick's design id
        BasicArray<Part*>   m_parts;      // +0x0c (not reference counted)
    };

    // The parts that are the bones of one flexible part (LXFML 4 <Part flexRefID=…
    // controlPoint=…>). sizeof 0x1c. Mac vtable 0x77738c {D1 0x301ba0, D0 0x301be0}, Win vtable
    // 0xa25b18 {sdd 0x4e0720}. Constructor Win 0x4e0580 (designID in EDX).
    struct Flex : public Object {
        explicit Flex(UInt32 designID = 0) : m_designID(designID) {}

        UInt32              m_designID;   // +0x08 design id of the flexible part
        BasicArray<Part*>   m_parts;      // +0x0c one per bone ("controlPoint" index; not ref counted)
    };

    // sizeof Mac 0x94 / Win 0x7c. Mac vtable 0x777090 {D1 0x2f5000, D0 0x2f5010}, Win vtable
    // 0xa25700 {sdd 0x4d8020}. Constructor Mac 0x2f4ec0, Win 0x4d7b60; destructor Mac 0x2f5080,
    // Win 0x4d80a0.
    struct Part : public Object {
        Part() : m_designID(0) {}

        String                    m_name;            // Mac +0x08 / Win +0x08 (never set by the
                                                     // converters; name inferred)
        UInt32                    m_designID;        // Mac +0x18 / Win +0x0c "designID"
        String                    m_itemNos;         // Mac +0x1c / Win +0x10 "itemNos"
        BasicArray<Decoration>    m_decorations;     // Mac +0x2c / Win +0x14
        BasicArray<UInt32>        m_materials;       // Mac +0x3c / Win +0x24 "materialID" / "materials"
        Matrix3D                  m_transformation;  // Mac +0x4c / Win +0x34 relative to the rigid
        RefPointer<Assembly>      m_assembly;        // Mac +0x8c / Win +0x74
        RefPointer<Flex>          m_flex;            // Mac +0x90 / Win +0x78
    };

    // A rigid body (LXFML 4 <Group>). sizeof Mac 0x68 / Win 0x5c. Mac vtable 0x777048
    // {D1 0x2f4cc0, D0 0x2f4cd0}, Win vtable 0xa25724 {sdd 0x4d8040}. Constructor Mac 0x2f4c30,
    // Win 0x4d7ca0; destructor Mac 0x2f4d10, Win 0x4d8170.
    struct Rigid : public Object {
        String                        m_name;            // Mac +0x08 / Win +0x08 "name"
        Matrix3D                      m_transformation;  // Mac +0x18 / Win +0x0c
        BasicArray<RefPointer<Part> > m_parts;           // Mac +0x58 / Win +0x4c
    };

    // Same shape as StorageScene2::Joint. sizeof Mac 0x64 / Win 0x58. Constructor Mac 0x2fc9f0,
    // Win 0x4e05f0. NOTE: the LXFML loaders store type="hinge" as 1 and everything else as 0;
    // ModelLoaderStorageScene copies (m_type != 0) into StorageScene2::Joint::m_type, whose 1 is
    // "ball" (both builds).
    struct Joint {
        Joint()
            : m_axisA(0.0f, 1.0f, 0.0f), m_zeroA(0.0f, 0.0f, 1.0f),
              m_axisB(0.0f, 1.0f, 0.0f), m_zeroB(0.0f, 0.0f, 1.0f), m_type(1)
        {
        }

        String            m_name;      // Mac +0x00 / Win +0x00
        RefPointer<Rigid> m_rigidA;    // Mac +0x10 / Win +0x04
        RefPointer<Rigid> m_rigidB;    // Mac +0x14 / Win +0x08
        Vector3           m_axisA;     // Mac +0x18 / Win +0x0c
        Vector3           m_zeroA;     // Mac +0x24 / Win +0x18
        Vector3           m_positionA; // Mac +0x30 / Win +0x24
        Vector3           m_axisB;     // Mac +0x3c / Win +0x30
        Vector3           m_zeroB;     // Mac +0x48 / Win +0x3c
        Vector3           m_positionB; // Mac +0x54 / Win +0x48
        UInt32            m_type;      // Mac +0x60 / Win +0x54 (1 = "hinge" in the LXFML 4 reader)
    };

    // <Model>. Mac 0x30 / Win 0x24. Constructor Win 0x4d7d30 (as an array element), destructor
    // Mac 0x2f4ad0.
    struct Model {
        String                         m_name;     // Mac +0x00 / Win +0x00 "name"
        BasicArray<RefPointer<Rigid> > m_rigids;   // Mac +0x10 / Win +0x04
        BasicArray<Joint>              m_joints;   // Mac +0x20 / Win +0x14
    };

    // A part shown by a building step. Mac 0x48 / Win 0x48. Default constructor Win 0x4d7da0.
    struct BIPart {
        BIPart() : m_exploded(false) {}

        RefPointer<Part> m_part;             // +0x00
        bool             m_exploded;         // +0x04
        Matrix3D         m_transformation;   // +0x08
    };

    // A building step. sizeof Mac 0x4c / Win 0x40. Mac vtable 0x7770f0 {D1 0x2f54f0, D0 0x2f5500},
    // Win vtable 0xa25758 {sdd 0x4d8060}. Constructor Mac 0x2f5330, Win 0x4d7e10; destructor Mac
    // 0x2f55c0, Win 0x4d81e0.
    struct BIStep : public Object {
        BIStep() : m_exploded(false) {}

        String                          m_name;      // Mac +0x08 / Win +0x08
        BasicArray<BIPart>              m_parts;     // Mac +0x18 / Win +0x0c
        RefPointer<StorageScene2::Camera> m_camera;  // Mac +0x28 / Win +0x1c camera shown before the step
        BasicArray<RefPointer<BIStep> > m_steps;     // Mac +0x2c / Win +0x20 sub steps
        bool                            m_exploded;  // Mac +0x3c / Win +0x30
        Vector3                         m_explode;   // Mac +0x40 / Win +0x34
    };

    // One building instruction. Mac 0x24 / Win 0x18. Constructor Win 0x4d7ec0, destructor Mac
    // 0x2f5280, Win 0x4d9ed0.
    struct BI {
        String                          m_name;     // Mac +0x00 / Win +0x00
        // <BuildingInstruction cameraRefID> (LXFML 4 reader / writer). (type and name inferred)
        RefPointer<StorageScene2::Camera> m_camera; // Mac +0x10 / Win +0x04
        BasicArray<RefPointer<BIStep> > m_steps;    // Mac +0x14 / Win +0x08
    };

    // A named part selection. Mac 0x20 / Win 0x14. Array destructor Mac 0x2f3d80, Win 0x4db280.
    struct Selection {
        String                        m_name;    // Mac +0x00 / Win +0x00
        BasicArray<RefPointer<Part> > m_parts;   // Mac +0x10 / Win +0x04
    };

    // Empties the scene for loading. Inline; Mac copy 0x2f7d50 (coalesced into TU 337).
    // NOTE: m_metas and m_camera are kept (both builds).
    void Clear()
    {
        m_name.Clear();
        m_cameras.Clear();
        m_models.Clear();
        m_buildingInstructions.Clear();
        m_selections.Clear();
    }

    String                                      m_name;                  // Mac +0x08 / Win +0x08
    BasicArray<RefPointer<StorageScene2::Camera> > m_cameras;            // Mac +0x18 / Win +0x0c
    BasicArray<Model>                           m_models;                // Mac +0x28 / Win +0x1c
    BasicArray<BI>                              m_buildingInstructions;  // Mac +0x38 / Win +0x2c
    BasicArray<StorageScene2::Meta>             m_metas;                 // Mac +0x48 / Win +0x3c
    BasicArray<Selection>                       m_selections;            // Mac +0x58 / Win +0x4c
    RefPointer<StorageScene2::Camera>           m_camera;                // Mac +0x68 / Win +0x5c
};

} // namespace LEGO
