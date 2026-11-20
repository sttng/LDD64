// LEGO Digital Designer — restored source
// LEGO::PrimitiveElement — one bone of one part's primitive in the connectivity scene graph:
// the cloned connection fields of the bone (with this element as their ConnectionFieldContext),
// the PhysicsPrimitiveElement (collision geometry: the <Collision> volumes plus one knob per
// "fixed" point of the odd-kind planar fields), the PrimitiveConnections to other elements and
// the transform relative to its PrimitiveRigidElement.
// Also: PrimitiveConnectionInfo (one possible connection of two fields, found by the Scene's
// CanConnect), FilterConnectionFieldObserver and the four MiniFig observers (special connection
// rules of minifigure torsos, hips and legs).
//
// PrimitiveElement.cpp = sub-TU 025j of the original Scene.cpp (Mac 0x9d7f0-0xa0210, inline
// virtuals 025s 0xc4200-0xc4360, observers 0xc5e90-0xc7040; Win 0x475ed0-0x477d10).
// RTTI: class LEGO::PrimitiveElement : BehaviorElement, ConnectionFieldContext (Win RTTI
// attribute 3: multiple + virtual inheritance).
//   Mac sizeof 0xb8: vptr +0 (vtable 0x7060dc), ConnectionFieldContext vptr +0x14 (vtable
//       0x706108 {thunks 0xc4360, 0x9d820, 0x9d880}), Object at +0xb0 (vtable 0x70612c
//       {thunks 0x9fd20, 0x9fe00, 0x9ff20 (OnFinalRelease), AddRef, Release}). VTT 0x706140.
//   Win sizeof 0xc8: vfptr +0 (0xa23b60 {0x46f8a0, 0x4705c0, 0x475ed0}), vbptr +4, behaviors
//       +8, ConnectionFieldContext vfptr +0x18 (0xa23b70 {0x470500, 0x475ee0, 0x475f10}),
//       members +4 from +0x1c on up to m_connections (+0x90, a 0x24-byte BasicMap on Win),
//       then +0xc (m_physics +0xb4, m_flagAC +0xb8), vtordisp +0xbc, Object at +0xc0 (vftable 0xa23b80 {sdd
//       0x4a1b30, OnFinalRelease 0x4a1ae0, …}).
// Mac primary vtable slots: 0 GetBoundingBox 0xc4200, 1 GetScene 0xc4230, 2/3 D1 0x9fcc0 /
//   D0 0x9fd90, 4 GetType 0x9d7f0, 5 GetTransform 0xc4260, 6 IsSameRigid 0x9d800, 7 GetBrick-
//   Connect 0x9d840 (the ConnectionFieldContext overrides get new primary slots on Mac),
//   8 OnFinalRelease 0x9fe80.
// Every member and method name is inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "BehaviorElement.h"
#include "ConnectionField.h"
#include "Primitive.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "BrickPhysics.h"

namespace LEGO {

class Scene;
class Part;
class BrickConnect;
class PrimitiveElement;
class PrimitiveRigidElement;
class PrimitiveConnection;
struct ConnectionPoint;

// ---------------------------------------------------------------------------------------------
// One connection of a field of `m_element` with a field of `m_otherElement` (the point pairs in
// m_indices: index of m_field first). sizeof 0x24 (both builds). RTTI of the array:
// BasicArray<PrimitiveConnectionInfo> (Mac dtor 0xc79c0 / 0xc7a30, Win 0x49db10).
// ---------------------------------------------------------------------------------------------
struct PrimitiveConnectionInfo {
    PrimitiveConnectionInfo() : m_element(0), m_otherElement(0), m_field(0), m_otherField(0) {}

    PrimitiveElement*                  m_element;       // +0x00
    PrimitiveElement*                  m_otherElement;  // +0x04
    ConnectionField*                   m_field;         // +0x08
    ConnectionField*                   m_otherField;    // +0x0c
    ConnectionField::ConnectionIndices m_indices;       // +0x10
};

// A collision feature of a PhysicsPrimitiveElement (the element and the feature index).
// 8 bytes; RTTI of the array: BasicArray<PhysicsGeometryIdentifier> (Mac 0xc85d0 / 0xc8800).
struct PhysicsGeometryIdentifier {
    PhysicsPrimitiveElement* m_element;   // +0x00
    UInt32                   m_feature;   // +0x04
};

// ---------------------------------------------------------------------------------------------
// FilterConnectionFieldObserver — set as the observer of one field of a minifigure primitive:
// connecting that field clears bit 0 of the m_flags of the fields in m_freeFields and sets it
// for the fields in m_connectedFields; disconnecting does the opposite. Field indices are
// indices into PrimitiveElement::m_fields. Not reference counted; no virtual destructor
// (vtable Mac 0x7622fc / Win 0xa23eec {OnConnect, OnDisconnect}). sizeof 0x28 (both builds).
// ---------------------------------------------------------------------------------------------
class FilterConnectionFieldObserver : public ConnectionFieldObserver {
public:
    FilterConnectionFieldObserver() : m_fieldIndex(-1) {}                   // Win 0x4764f0
    ~FilterConnectionFieldObserver() {}                                     // Mac 0x9e680 (one atexit copy per static)

    // Mac 0xc6490, Win 0x476410
    virtual void OnConnect(bool first, ConnectionField& field, ConnectionField& other,
                           const ConnectionField::ConnectionIndices& indices,
                           ConnectionField::Attributes* attributes);
    // Mac 0xc65b0, Win 0x476480
    virtual void OnDisconnect(bool first, ConnectionField& field, ConnectionField& other,
                              const ConnectionField::ConnectionIndices& indices);

    // Installs the observer on the fields of a new element (unconnected state). Win 0x4763a0
    // (Mac inlined four times into the PrimitiveElement constructor).
    void Apply(PrimitiveElement& element);

    Int32              m_fieldIndex;        // +0x04 observed field (-1: none found)
    BasicArray<UInt32> m_freeFields;        // +0x08 bit 0 set while the observed field is free
    BasicArray<UInt32> m_connectedFields;   // +0x18 bit 0 set while it is connected
};

// Minifigure torso 3814: fixed field kind 31 at y 1.28 is observed; hinge fields kind 201
// at y 1.43 are free-only, the other kind 201 hinges connected-only.
// Mac vtable 0x7622c4 (the slots are FilterConnectionFieldObserver's). Constructor Mac 0xc5e90,
// Win 0x476560 (function-local static of the PrimitiveElement constructor).
class MiniFigTorso3814ConnectionFieldObserver : public FilterConnectionFieldObserver {
public:
    explicit MiniFigTorso3814ConnectionFieldObserver(const Primitive::Bone& bone);
};
// Minifigure skeleton body 60115: as the torso with y 1.76 / 1.91. Mac 0xc66d0
class MiniFigSkeletonBody60115ConnectionFieldObserver : public FilterConnectionFieldObserver {
public:
    explicit MiniFigSkeletonBody60115ConnectionFieldObserver(const Primitive::Bone& bone);
};
// Minifigure hip 3815: fixed field kind 5 at y 0 is observed, the other kind 5 fields are
// free-only. Mac 0xc6cd0
class MiniFigHip3815ConnectionFieldObserver : public FilterConnectionFieldObserver {
public:
    explicit MiniFigHip3815ConnectionFieldObserver(const Primitive::Bone& bone);
};
// Mini legs 41879: as the hip with y 0.9588. Mac 0xc7040
class MiniFigMiniLegs41879ConnectionFieldObserver : public FilterConnectionFieldObserver {
public:
    explicit MiniFigMiniLegs41879ConnectionFieldObserver(const Primitive::Bone& bone);
};

// ---------------------------------------------------------------------------------------------
// PrimitiveElement
// ---------------------------------------------------------------------------------------------
class PrimitiveElement : public BehaviorElement, public ConnectionFieldContext {
public:
    typedef BasicMap<PrimitiveElement*, RefPointer<PrimitiveConnection> > ConnectionMap;

    // Visitor over the collision volumes (RTTI: PrimitiveElement::CollisionVolumeVisitor; used
    // by GetSceneCVPointsCollisionVolumeVisitor, InternalGetDebugInfoCollisionVolumeVisitor,
    // BICameraChangeFinder::TestDistancesCollisionVolumeVisitor). One overload set: MSVC
    // reverses the slot order (Win calls sphere +8, box +4, capsule +0). Slot 0 is the sphere
    // (kind 0): see the note at PhysicsGeometry::Kind (BrickPhysics.h).
    class CollisionVolumeVisitor {
    public:
        virtual void Visit(PhysicsGeometrySphere* sphere, const Matrix3D& transform) = 0;    // Mac slot 0
        virtual void Visit(PhysicsGeometryBox* box, const Matrix3D& transform) = 0;          // Mac slot 1
        virtual void Visit(PhysicsGeometryCapsule* capsule, const Matrix3D& transform) = 0;  // Mac slot 2
    };

    // Clones the fields of bone `boneIndex` of `primitive`, builds the collision geometry and
    // creates the PhysicsPrimitiveElement (geometry objects shared through `brickConnect`).
    // `transform`: element in its PrimitiveRigidElement (initially the world transform).
    // Mac 0x9e7f0 (called by BrickConnect::CreatePrimitiveElement Mac 0xc2ae0).
    PrimitiveElement(const Primitive* primitive, UInt32 boneIndex, const Matrix3D& transform,
                     BrickPhysics* physics, BrickConnect* brickConnect);

    // --- slot 0: the bone's bounding box (element space; `recompute` ignored). Mac 0xc4200 (025s), Win 0x46f8a0
    virtual void GetBoundingBox(BoundingBox& box, bool recompute) const;
    // --- slot 1: Mac 0xc4230 (025s), Win 0x4705c0
    virtual Scene* GetScene() const;
    // --- slots 2/3: Mac 0x9fcc0 / 0x9fd90 (base 0x9fa60), Win sdd 0x4a1b30
    virtual ~PrimitiveElement();
    // --- slot 4: Element::TypePrimitiveElement. Mac 0x9d7f0, Win 0x475ed0
    virtual UInt32 GetType() const { return TypePrimitiveElement; }
    // --- ConnectionFieldContext (Mac primary slots 5-7)
    // World transform: m_transform in the PrimitiveRigidElement in the body. Mac 0xc4260 (025s,
    // thunk 0xc4360), Win 0x470500
    virtual void GetTransform(Matrix3D& transform) const;
    // Both elements are in the same RigidElement. Mac 0x9d800 (thunk 0x9d820), Win 0x475ee0
    virtual bool IsSameRigid(const ConnectionFieldContext* other) const;
    // NOTE: declared GetRigid() by ConnectionField.h; it returns the Scene's +0x38 member
    // (Scene::GetBrickConnect, the owner) or NULL. Mac 0x9d840 (thunk 0x9d880), Win 0x475f10
    virtual void* GetRigid() const;

    // ---- non-virtual API (names inferred) -----------------------------------------------

    // All fields are occupied. Mac 0x9dbc0, Win 0x476190
    bool IsFullyConnected() const;
    // Appends the collision feature of point `point` of the field with index `fieldIndex`
    // (nothing if the point has none). Mac 0x9dc30 / 0x9dc10 (by field), Win 0x4761d0
    void GetFeature(UInt32 fieldIndex, UInt32 point, BasicArray<PhysicsGeometryIdentifier>& features) const;
    void GetFeature(const ConnectionField& field, UInt32 point,
                    BasicArray<PhysicsGeometryIdentifier>& features) const;
    // Calls `visitor` for every enabled collision volume (configurable geometries recursively);
    // `transform` is prepended to the volume transforms. Mac 0x9d8b0 (+ 0x9d940), Win 0x476100
    // (+ 0x475f30)
    void VisitCollisionVolumes(CollisionVolumeVisitor& visitor, const Matrix3D& transform) const;
    // Owner part; registers this element as bone m_boneIndex of the part. Mac 0x9e020, Win 0x476350
    void SetPart(Part* part);
    // Connects m_field of `info` (one of this element's fields) with info.m_otherField and
    // records it in the PrimitiveConnection to `other` (created when there is none yet;
    // `connection` receives it). Returns true when the joint of the connection changed
    // (always for a new connection). Mac 0x9ffd0 (callers: Scene, Mac 0xbe6c0)
    bool Connect(const PrimitiveConnectionInfo& info, PrimitiveElement* other,
                 PrimitiveConnection*& connection);
    // Disconnects every field connection of `connection` and resets the knob collision types
    // (this element is not used: the elements come from `connection`). Mac 0x97310 (025f)
    void DisconnectFields(PrimitiveConnection* connection);

    // Hidden flag (m_flagAC): blocks / unblocks every connection point of the fields; with
    // `updateBodies` the body and (Scene m_flag120) the bodies of the connected, not hidden
    // elements are marked changed in `scene`. Mac 0x95bb0 (025d, restored in Part.cpp),
    // Win 0x477c10. (name inferred)
    void SetHidden(bool hidden, bool updateBodies, Scene* scene);
    // Part::OnMaterialsChanged: applies the point groups to every field; the Scene then marks
    // the body (m_flag121) and (m_flag120) the bodies of elements connected through a fixed
    // planar point of a changed group as changed. Mac 0x95790 (025d, restored in Part.cpp),
    // Win 0x477950. (name inferred)
    void ApplyPointGroups(const BasicArray<ConnectionField::PointGroup>& groups);
    // Manipulator for the joints of this element's primitive connections (the Scene's
    // active manipulator; the previous one is detached); NULL when the element has no
    // connection or none of them can move. Mac 0x9dea0 (025j position, restored in
    // ConnectionManipulator.cpp). (name inferred)
    class ConnectionManipulator* CreateManipulator();

    // Collision type of a planar field point (the PhysicsPlacedConfiguredGeometry::m_type of
    // its knob feature). Inlined three times (Mac 0x9e7f0, 0x97310).
    static UInt32 GetFeatureType(const ConnectionPoint& point);

    // ---- accessors (inline in the original) ------------------------------------------------
    const Matrix3D&            GetLocalTransform() const { return m_transform; }
    PrimitiveRigidElement*     GetRigidElement() const { return m_rigid; }
    Part*                      GetPart() const { return m_part; }
    const Primitive*           GetPrimitive() const { return m_primitive; }
    const Primitive::Bone&     GetBone() const { return *m_bone; }
    UInt32                     GetBoneIndex() const { return m_boneIndex; }
    UInt32                     GetFieldCount() const { return m_fields.GetSize(); }
    ConnectionField*           GetField(UInt32 index) const { return m_fields[index]; }
    const ConnectionMap&       GetConnections() const { return m_connections; }
    PhysicsPrimitiveElement*   GetPhysicsElement() const { return m_physics; }

    // ---- data (Mac offsets; Win +4, after m_connections +0xc) ---------------------------------------------------------
    Matrix3D                          m_transform;       // +0x18 in the PrimitiveRigidElement
    PrimitiveRigidElement*            m_rigid;           // +0x58 (not referenced)
    Part*                             m_part;            // +0x5c (not referenced)
    const Primitive::Bone*            m_bone;            // +0x60 bone m_boneIndex of m_primitive
    UInt32                            m_boneIndex;       // +0x64
    Sphere                            m_boundingSphere;  // +0x68 of the bone's bounding box
    RefPointer<const Primitive>       m_primitive;       // +0x78
    BasicArray<RefPointer<ConnectionField> > m_fields;   // +0x7c clones; m_reserved = index
    ConnectionMap                     m_connections;     // +0x8c by the other element
    PhysicsPrimitiveElement*          m_physics;         // +0xa8 (one reference, released by the destructor)
    bool                              m_flagAC;          // +0xac false (not used in TU 025)

protected:
    // Releases the fields and the connections before the last reference goes away.
    // Mac 0x9fe80 (thunk 0x9ff20), Win 0x4a1ae0
    virtual void OnFinalRelease();
};

} // namespace LEGO
