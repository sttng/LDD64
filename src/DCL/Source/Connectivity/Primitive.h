// LEGO Digital Designer — restored source
// LEGO::Primitive — one parsed db.lif "Primitives/<designID>.xml" (<LEGOPrimitive>): bounding
// boxes, the bones (one for a rigid primitive, one per <Flex><Bone> for a flexible one) with
// their collision volumes, connection fields and physics attributes, the decoration surfaces,
// the <Paths>, the default orientation/camera (kept by the loader, see BrickDatabase::Brick)
// and the <Annotations>.
//
// Header-only in the original: the class has no out-of-line member (Mac: constructor 0x8bd00,
// destructor 0x8c070 (D1 0x8bf20, D0 0x8bf30), the Bone constructor 0x8b620 / destructor
// 0x8baf0 and the BasicArray instances are weak functions coalesced into TU 024 = BrickDatabase
// .cpp, sub-TU 024f; Win: destructor 0x4b2ef0 / sdd 0x4b2fc0, Bone constructor 0x4be600 /
// destructor 0x4be6e0 (inlined into the BasicArray code as well)). The members are filled by
// the LEGOPrimitive parser in PrimitiveLoaderXML.cpp. Plan item 1.5.
//
// Names from RTTI: Primitive, Primitive::CollisionVolume, Primitive::Bone, Primitive::Path,
// Primitive::CollisionVolumeToAndFromFeatureLookup(::Feature), BasicMap<String, String,
// Less<String>> (the annotations). Every member and method name is inferred (from the XML
// element / attribute names the parser stores into them).
//
// Offsets: Mac / Win (no String member directly in Primitive: the layouts only differ in the
// annotation map, BasicMap = vptr + std::map: Mac 0x1c, Win 0x24).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "ConnectionField.h"

namespace LEGO {

class BrickDatabase;

// RTTI: class LEGO::Primitive : Object (non-virtual base).
// Mac sizeof 0x88, vtable 0x761fe0 {D1 0x8bf20, D0 0x8bf30, Object 0x9250 0x9260 0x9280}.
// Win sizeof 0x90, vtable 0xa24f30 {sdd 0x4b2fc0, Object slots}.
class Primitive : public Object {
public:
    // <Box sX sY sZ …>, <Sphere radius …>, <Capsule radius length …> of a <Collision> block.
    // sizeof 0x50 (both). (<Tube> elements are ignored by the parser.)
    struct CollisionVolume {
        enum Type {                      // (names from the element names)
            Box     = 0,                 // m_size = (sX, sY, sZ) half extents
            Sphere  = 1,                 // m_size.x = radius
            Capsule = 2                  // m_size.x = length, m_size.y = radius
        };
        // The parser value-initialises the 0x50 bytes and constructs the matrix (Win 0x4be780:
        // size 0, identity).
        CollisionVolume() : m_type(Box), m_size(0.0f, 0.0f, 0.0f) {}

        UInt32   m_type;        // +0x00 Type
        Vector3  m_size;        // +0x04
        Matrix3D m_transform;   // +0x10 "angle ax ay az tx ty tz"
    };

    // Collision volume <-> physics feature tables of a bone. Constructed empty with
    // m_count = 0x7fffffff (Mac Bone ctor 0x8b620, Win 0x4b2d90); not filled by the database but
    // by the bone's first PrimitiveElement (constructor, PrimitiveElement.cpp).
    struct CollisionVolumeToAndFromFeatureLookup {
        // NOTE: layout unknown (only delete[] is ever applied to the array); 8 bytes assumed.
        struct Feature {
            UInt32 m_volume;    // (name inferred)
            UInt32 m_feature;   // (name inferred)
        };
        CollisionVolumeToAndFromFeatureLookup() : m_count(0x7fffffff) {}

        UInt32                          m_count;      // +0x00 0x7fffffff (meaning unknown)
        BasicArray<Feature>             m_features;   // +0x04
        BasicArray<BasicArray<UInt32> > m_volumes;    // +0x14 (vtable BasicArray<BasicArray<unsigned long>>)
    };

    // <Paths><Path type points>x y z, …</Path>: a polyline (cable / hose route). 0x14 (both).
    struct Path {
        enum Type {                       // the parser accepts 1 and 2 only (names inferred)
            TypeOpen   = 1,
            TypeClosed = 2
        };
        Path() : m_type(0) {}             // Win 0x4b2d30 (Mac inlined)

        UInt32              m_type;       // +0x00
        BasicArray<Vector3> m_points;     // +0x04 "points" entries, filled from the element text
    };

    // A rigid body of the primitive: the whole primitive, or one <Flex><Bone>. sizeof 0xfc
    // (both). Constructor Mac 0x8b620, Win 0x4be600; destructor Mac 0x8baf0, Win 0x4be6e0.
    struct Bone {
        Bone()
            : m_bounding(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
              m_centerOfMass(0.0f, 0.0f, 0.0f), m_mass(1.0f), m_frictionType(0)
        {
            m_flexCheckConnection[0] = 0;
            m_flexCheckConnection[1] = 0;
            m_flexCheckConnection[2] = 0;
        }

        Matrix3D                             m_transform;            // +0x00 <Bone angle ax … tz>
        BoundingBox                          m_bounding;             // +0x40 <Bounding><AABB>
        BasicArray<CollisionVolume>          m_collisionVolumes;     // +0x58 <Collision>
        BasicArray<RefPointer<ConnectionField> > m_connectionFields; // +0x68 <Connectivity>
        UInt32                               m_flexCheckConnection[3]; // +0x78 "a,b,c"
        CollisionVolumeToAndFromFeatureLookup m_featureLookup;       // +0x84
        Matrix3D                             m_inertiaTensor;        // +0xa8 <PhysicsAttributes> 3x3 part
        Vector3                              m_centerOfMass;         // +0xe8
        Real32                               m_mass;                 // +0xf4 default 1
        UInt32                               m_frictionType;         // +0xf8
    };

    typedef BasicMap<String, String> AnnotationMap;   // RTTI BasicMap<String, String, Less<String>>

    // Mac 0x8bd00 (weak), Win inlined (0x4b2e10 builds it in LoadXMLOperation's ctor).
    Primitive(BrickDatabase* database, UInt32 designID)
        : m_database(database), m_designID(designID), m_decorationFaceCount(0),
          m_bounding(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f)
    {
    }
    // Mac 0x8c070 (D1 0x8bf20, D0 0x8bf30), Win 0x4b2ef0 (sdd 0x4b2fc0). Implicit in the
    // original (members only).
    virtual ~Primitive() {}

    // ---- accessors (all inline in the original; names inferred) ----------------------------
    BrickDatabase*               GetDatabase() const { return m_database; }
    UInt32                       GetDesignID() const { return m_designID; }
    // <Decoration faces=…>: number of decoration surfaces (0 = not decorable).
    UInt32                       GetDecorationFaceCount() const { return m_decorationFaceCount; }
    // <Decoration subMaterialRedirectLookupTable=…>, padded with i -> i up to the face count.
    const BasicArray<UInt32>&    GetSubMaterialRedirect() const { return m_subMaterialRedirect; }
    // <GeometryBounding> (or the <Bounding> of a single-bone primitive when no
    // GeometryBounding came first).
    const BoundingBox&           GetBoundingBox() const { return m_bounding; }
    const BasicArray<Path>&      GetPaths() const { return m_paths; }
    const BasicArray<Bone>&      GetBones() const { return m_bones; }
    UInt32                       GetBoneCount() const { return m_bones.GetSize(); }
    const Bone&                  GetBone(UInt32 index) const { return m_bones[index]; }
    bool                         IsFlexible() const { return m_bones.GetSize() > 1; }
    // Inverse of every <Bone> transformation (index = boneId; empty for rigid primitives).
    const BasicArray<Matrix3D>&  GetInverseBoneTransforms() const { return m_inverseBoneTransforms; }
    const AnnotationMap&         GetAnnotations() const { return m_annotations; }

    // ---- data (public: filled by the loader in PrimitiveLoaderXML.cpp) ----------------------
    BrickDatabase*        m_database;              // +0x08 owner (not reference counted)
    UInt32                m_designID;              // +0x0c file name number
    UInt32                m_decorationFaceCount;   // +0x10 <Decoration faces>
    BasicArray<UInt32>    m_subMaterialRedirect;   // +0x14 <Decoration subMaterialRedirectLookupTable>
    BoundingBox           m_bounding;              // +0x24
    BasicArray<Path>      m_paths;                 // +0x3c
    BasicArray<Bone>      m_bones;                 // +0x4c
    BasicArray<Matrix3D>  m_inverseBoneTransforms; // +0x5c
    AnnotationMap         m_annotations;           // +0x6c (std::map at +0x70)

private:
    Primitive(const Primitive&);
    Primitive& operator=(const Primitive&);
};

} // namespace LEGO
