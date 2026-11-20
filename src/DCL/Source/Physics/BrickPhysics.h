// LEGO Digital Designer — restored source
// The abstract physics layer the connectivity scene graph talks to: LEGO::IUnknown-style
// interfaces (QueryInterface / AddRef / Release, no virtual destructor), implemented by the
// BaseBrickPhysics* and Vortex* classes (Mac TUs 203-211, 421-438). Header-only. Placeholder
// names Unknown<N> = vtable slot N (the same on Mac and Win).
//
// Which object is which (evidence: TU 025):
//   BrickConnect +0x04, Scene +0x40   BrickPhysics (factory: bodies, primitive elements,
//                                     geometries, joints; VortexBrickPhysics: 23 slots + dtor)
//   Scene +0x44                       PhysicsScene (the simulated world)
//   RigidElement +0x5c                PhysicsRigidElement (one rigid body)
//   PrimitiveElement +0xa8            PhysicsPrimitiveElement (collision geometry of one bone,
//                                     attached to a body with a transform)
//   RigidConnection +0x64             PhysicsConnection (a joint between two bodies)
// Names are inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "ConnectionField.h"

#include <string.h>

namespace LEGO {

// 16-byte interface id (QueryInterface argument). The connectivity code builds them in
// function-local statics (e.g. Mac 0x762220 {0xc501e20b, 0x05471ba9, 0xf7788a88, 0x83899348}).
struct InterfaceID {
    UInt32 m_data[4];
};

// The three IUnknown methods use the COM convention on Windows: __stdcall, `this` as the first
// stack argument (Win BrickConnect 0x4904e3 `push eax; call [ecx+4]` AddRef, Scene destructor
// 0x48a94d `push eax; call [edx+8]` Release); every other slot of the physics interfaces is
// thiscall.
#if defined(_WIN32)
#define LEGO_COM_CALL __stdcall
#else
#define LEGO_COM_CALL
#endif

// (added, performance) Scene::Pick's spatial index caches the transforms of the bodies. A body
// (PhysicsRigidElement) that answers this interface id in QueryInterface reports every change of
// the transform GetTransform returns, through NotePhysicsTransformChanged with its user data;
// the index compares the transforms of other bodies on every Pick.
struct PhysicsTransformReporter {
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x6c1d2b7e, 0x4f0a9d31, 0xa8e35c42, 0x1d9b07f6}};
        return id;
    }
};
typedef void (*PhysicsTransformListener)(void* userData);
inline PhysicsTransformListener& GetPhysicsTransformListener()
{
    static PhysicsTransformListener listener = 0;
    return listener;
}
inline void NotePhysicsTransformChanged(void* userData)
{
    PhysicsTransformListener listener = GetPhysicsTransformListener();
    if (listener != 0 && userData != 0)
        listener(userData);
}

// RTTI: LEGO::IUnknown (no base class).
class IUnknown {
public:
    virtual Int32  LEGO_COM_CALL QueryInterface(const InterfaceID& id, void** object) = 0;  // slot 0 (< 0: failure)
    virtual UInt32 LEGO_COM_CALL AddRef() = 0;                                             // slot 1
    virtual UInt32 LEGO_COM_CALL Release() = 0;                                            // slot 2
};

class PhysicsCollisionCallback;
class PhysicsProgressCallback;
class PhysicsSolverCallback;
class PhysicsSolver;
class AnimatePhysicsSolver;
class PhysicsSceneDebugInfo;   // Connectivity/SimplePhysicsSceneDebugInfo.h
class PhysicsScene;   // (additive: BrickPhysics::CreatePhysicsScene)
struct PhysicsGeometryIdentifier;   // PrimitiveElement.h

// ---- geometries ------------------------------------------------------------------------------

// RTTI: PhysicsGeometry : IUnknown. Kinds (slot 3) seen in PrimitiveElement's collision volume
// walk (Mac 0x9d940): 0 sphere, 1 box, 2 capsule, 4 configurable (a compound of geometries).
// Kinds 0 / 1 (and the ids below) are settled by the debug drawing: the visitor slot kind 0
// reaches (Mac 0xca080, Win 0x48d710 via slot +8) reads a Real32 from geometry slot 4 and
// tessellates a sphere, the kind 1 slot (Mac 0xca100, Win 0x48d660) reads a Vector3 (half size)
// and draws a box (SimplePhysicsSceneDebugInfo.cpp).
class PhysicsGeometry : public IUnknown {
public:
    enum Kind { KindSphere = 0, KindBox = 1, KindCapsule = 2, KindCylinder = 3, KindConfigurable = 4,
                KindSampledHeightField = 6 };   // (6 from VortexPhysicsGeometrySampledHeightField Mac 0x3a7960)
    // (additive) The id every geometry answers besides its own (the Vortex
    // geometries' QueryInterface, function-local static Mac 0x778d00).
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0xc501e20b, 0x05471ba9, 0xf7788a88, 0x81899348}};
        return id;
    }
    virtual UInt32 GetKind() const = 0;                                      // slot 3
};
// A geometry placed in a primitive: the element of the array handed to
// BrickPhysics::CreatePrimitiveElement (PrimitiveElement constructor, Mac 0x9e7f0) and
// returned by PhysicsPrimitiveElement::GetGeometry. 0x48 bytes (both builds).
// `m_type` is the collision type of the volume: 1 for the <Collision> volumes, 1-7 for the
// knob / tube features of planar fields (see PrimitiveElement::GetFeatureType).
// RTTI name of the array instance: BasicArray<PhysicsPlacedConfiguredGeometry> (Mac 0x76ed00).
struct PhysicsPlacedConfiguredGeometry {
    class PhysicsGeometry* m_geometry;    // +0x00 (not referenced)
    Matrix3D         m_transform;   // +0x04
    UInt32           m_type;        // +0x44
};

// The interface ids are function-local statics of PrimitiveElement's collision volume walk
// (Mac 0x9d940: 0x762220 / 0x762238 / 0x762250 / 0x7626dc; Win 0x470800 / 0x470730 / 0x4708d0 /
// 0x4709a0).
// The accessors (slots 4 / 5) are read by the debug drawing only (InternalGetDebugInfo-
// CollisionVolumeVisitor, Mac 0xca080-0xca1a0). (names inferred)
class PhysicsGeometrySphere : public PhysicsGeometry {
public:
    static const InterfaceID& GetIID()   // kind 0 (Mac 0x762220)
    {
        static const InterfaceID id = {{0xc501e20b, 0x05471ba9, 0xf7788a88, 0x83899348}};
        return id;
    }
    virtual Real32 GetRadius() const = 0;                                    // slot 4
};
class PhysicsGeometryBox : public PhysicsGeometry {
public:
    static const InterfaceID& GetIID()   // kind 1 (Mac 0x762238)
    {
        static const InterfaceID id = {{0xc501e20b, 0x05471ba9, 0xf7788a88, 0x82899348}};
        return id;
    }
    virtual Vector3 GetHalfSize() const = 0;                                 // slot 4
};
class PhysicsGeometryCapsule : public PhysicsGeometry {
public:
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x16f8462c, 0xb94c890a, 0xc469a096, 0x8fd133d8}};
        return id;
    }
    virtual Real32 GetRadius() const = 0;                                    // slot 4
    virtual Real32 GetLength() const = 0;                                    // slot 5
};
// (additive) RTTI PhysicsGeometryCylinder : PhysicsGeometry (kind 3; implemented by
// VortexPhysicsGeometryCylinder, Mac TU 431; BrickPhysics slot 10 creates it). (names inferred)
class PhysicsGeometryCylinder : public PhysicsGeometry {
public:
    static const InterfaceID& GetIID()   // Mac static 0x778c7c
    {
        static const InterfaceID id = {{0xc501e20b, 0x05471ba9, 0xf7788a88, 0x84899348}};
        return id;
    }
    virtual Real32 GetRadius() const = 0;                                    // slot 4
    virtual Real32 GetLength() const = 0;                                    // slot 5
};
// A height field sampled on a grid (RTTI PhysicsGeometrySampledHeightField : PhysicsGeometry;
// VortexPhysicsGeometrySampledHeightField Mac vtable {.., 0x3a6fe0, 0x3a7970, 0x3a79a0, 0x3a7010,
// 0x3a79b0, D1/D0}). Created by BrickPhysics::CreateSampledHeightField; the terrain of
// BrickConnectSceneControl::LoadTerrain reads it back for its mesh. (names inferred)
class PhysicsGeometrySampledHeightField : public PhysicsGeometry {
public:
    static const InterfaceID& GetIID()   // Mac static 0x778cb0
    {
        static const InterfaceID id = {{0x653d49be, 0x504df45f, 0xe82fff95, 0x241c8162}};
        return id;
    }
    virtual UIntVector2 GetSize() const = 0;                                 // slot 4 (samples x, y)
    virtual Vector3     GetScale() const = 0;                                // slot 5 (sample spacing, height scale)
    virtual bool        GetFlag() const = 0;                                 // slot 6 (creation flag; meaning unknown)
    virtual Real32      GetHeight(const UIntVector2& sample) const = 0;      // slot 7 (unscaled)
    virtual void*       GetNativeGeometry() const = 0;                       // slot 8 (the Vx geometry)
};
// A geometry with its placement (0x44 bytes): the configurable geometry's parts.
struct PhysicsPlacedGeometry {
    class PhysicsGeometry* m_geometry;   // +0x00
    Matrix3D               m_transform;  // +0x04
};

// A compound geometry: its parts per "configuration" (slots 6, 7; Mac 0x9d940 case 4).
class PhysicsGeometryConfigable : public PhysicsGeometry {
public:
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0xfd22c9d0, 0xe7426ece, 0x26fc0f9f, 0xe5840aee}};
        return id;
    }
    // The parts of `configuration` (BrickConnect constructor Mac 0xc1fc0 / Win 0x490460); false
    // for configuration 0 or one already set (Vortex Mac 0x3a6180).
    virtual bool   SetGeometries(UInt32 configuration, const struct PhysicsPlacedGeometry* geometries,
                                 UInt32 count) = 0;                          // slot 4
    // Number of configurations (Vortex Mac 0x3a6660). (name inferred)
    virtual UInt32 GetConfigurationCount() = 0;                              // slot 5
    virtual UInt32 GetGeometryCount(UInt32 configuration) = 0;               // slot 6
    virtual bool   GetGeometry(UInt32 configuration, UInt32 index,
                               PhysicsPlacedConfiguredGeometry& geometry) = 0; // slot 7
};


// ---- elements --------------------------------------------------------------------------------

class PrimitiveElement;
class RigidElement;

struct PhysicsMassProperties;   // below

// RTTI: PhysicsPrimitiveElement : IUnknown. 12 interface slots (0-11); the implementations add
// their destructor (BaseBrickPhysicsPrimitiveElement Mac D1/D0 slots 12/13, Win sdd slot 12).
class PhysicsPrimitiveElement : public IUnknown {
public:
    // QueryInterface id (BaseBrickPhysicsPrimitiveElement Mac 0x253790, static Mac 0x77144c).
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x5bc36453, 0xfc4696ee, 0xa7b4c2b0, 0x74a7a90c}};
        return id;
    }
    // Collision type of one feature (index into the creation array). Mac 0x97310, 0x9ffd0.
    virtual void   SetFeatureType(UInt32 feature, UInt32 type) = 0;           // slot 3
    virtual UInt32 GetGeometryCount() const = 0;                             // slot 4
    // Copy of a placed geometry (the geometry is referenced: the caller releases it).
    virtual bool   GetGeometry(UInt32 index, PhysicsPlacedConfiguredGeometry& geometry) const = 0; // slot 5
    // The mass properties given at creation (BaseBrickPhysicsPrimitiveElement +0x10, Mac
    // 0x253870). (name inferred)
    virtual const PhysicsMassProperties& GetMassProperties() const = 0;     // slot 6
    // The element set by SetUserData (the collision callbacks of the Scene, Mac 0xc7b10 /
    // 0xc82b0 / 0xc7d70). (TU 025 Scene)
    virtual PrimitiveElement* GetUserData() const = 0;                       // slot 7
    virtual void   SetUserData(PrimitiveElement* element) = 0;               // slot 8
    // Flexible connections (Attributes::m_isFlexible): stop / restart collisions between the
    // two elements. Mac 0x9ffd0 / 0x97310.
    virtual void   AddFlexibleConnection(PhysicsPrimitiveElement* other) = 0;     // slot 9
    virtual void   RemoveFlexibleConnection(PhysicsPrimitiveElement* other) = 0;  // slot 10
    // Placement of the element in its body (BaseBrickPhysicsRigidElement::AddPrimitiveElement,
    // Mac 0x253ac0; VortexPhysicsPrimitiveElement Mac 0x3a9bf0 stores it). (name
    // inferred)
    virtual void   SetTransform(const Matrix3D& transform) = 0;              // slot 11
};

// RTTI: PhysicsSceneElement : IUnknown.
class PhysicsSceneElement : public IUnknown {
public:
    // QueryInterface id (second id of BaseBrickPhysicsRigidElement's QI, Mac static 0x779088).
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x8e69aedc, 0x9a4cbf59, 0xc8bd42b4, 0x53f3bc8c}};
        return id;
    }
    virtual void GetTransform(Matrix3D& transform) const = 0;                // slot 3
    // BaseBrickPhysicsRigidElement +0x14 (Mac 0x254050). (name inferred)
    virtual const PhysicsMassProperties& GetMassProperties() const = 0;     // slot 4
    // The element set by SetUserData (BaseBrickPhysicsRigidElement Mac 0x254060 returns +0x10,
    // the field slot 6 writes). Read by BrickConnectSceneControl::RestorePhysicsSnapshot
    // (Mac 0x39480). (name inferred)
    virtual void* GetUserData() const = 0;                                   // slot 5
};

// RTTI: PhysicsRigidElement : PhysicsSceneElement (VortexPhysicsRigidElement: 19 slots).
class PhysicsRigidElement : public PhysicsSceneElement {
public:
    virtual void SetUserData(RigidElement* element) = 0;                     // slot 6
    virtual void AddPrimitiveElement(PhysicsPrimitiveElement* element, const Matrix3D& transform) = 0; // slot 7
    virtual void RemovePrimitiveElement(PhysicsPrimitiveElement* element) = 0;  // slot 8
    // ---- slots 9-16 (from BaseBrickPhysics* and VortexPhysicsRigidElement Mac
    // 0x3aa340-0x3aaa00; names inferred, the two OnXxxPrimitiveElement from its log strings)
    // The engine's current transform of the body (Vortex part). Mac 0x3aa340
    virtual void GetEngineTransform(Matrix3D& transform) const = 0;          // slot 9
    // Moves the engine's body. Mac 0x3aa1c0
    virtual void SetEngineTransform(const Matrix3D& transform) = 0;          // slot 10
    // Linear and angular velocity both above 1e-6. Mac 0x3aa3c0
    virtual bool IsMoving() const = 0;                                       // slot 11
    // Zero velocities. Mac 0x3aa490
    virtual void Stop() = 0;                                                 // slot 12
    // Called by AddPrimitiveElement / RemovePrimitiveElement. Mac 0x3aa5a0 / 0x3aa680
    virtual void OnAddPrimitiveElement(PhysicsPrimitiveElement* element) = 0;    // slot 13
    virtual void OnRemovePrimitiveElement(PhysicsPrimitiveElement* element) = 0; // slot 14
    // End of a scene update for a changed body (BaseBrickPhysicsScene::UnlockDirty).
    // BaseBrickPhysicsRigidElement Mac 0x254080 clears the dirty flag; Vortex Mac 0x3aa730
    // rebuilds the body first.
    virtual void Update() = 0;                                               // slot 15
    // Removing the body from its scene (BaseBrickPhysicsRigidElement::RemoveFromScene). Mac 0x3aaa00
    virtual void ReleaseEngineBody() = 0;                                    // slot 16
    static const InterfaceID& GetIID()   // BaseBrickPhysicsRigidElement QI (Mac static 0x771484)
    {
        static const InterfaceID id = {{0xdbc42a45, 0x954e3b9d, 0x5a10888b, 0x69e636d1}};
        return id;
    }

    // The RigidElement of the body (GetUserData). (helper, not a slot)
    RigidElement* GetRigidElement() const { return static_cast<RigidElement*>(GetUserData()); }
};

// RTTI: PhysicsSceneryElement : PhysicsSceneElement (VortexPhysicsSceneryElement: 15 slots). A
// static collision object of the world outside the model (the terrain): created by
// BrickPhysics::CreateSceneryElement, added with PhysicsScene::AddSceneryElement.
class PhysicsSceneryElement : public PhysicsSceneElement {
public:
    // ---- slots 6-12 (ViewMode's ball game uses 9 and 11). 13 slots, Win vtable
    // 0xa2a250; VortexPhysicsSceneryElement Mac 0x758da8 {.., 0x3b55c0, 0x3b55d0, 0x3b5170,
    // 0x3b5190, 0x3b5280, 0x3b55f0, 0x3b52e0, 0x3b5320, D1, D0}. Names from the Vortex bodies.
    // QueryInterface id (VortexPhysicsSceneryElement Mac 0x3b4f70, function-local static Mac
    // 0x779070).
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0xf6756411, 0x5d4d00f8, 0x26d12bbf, 0x7ef85eae}};
        return id;
    }
    // Stores the user data GetUserData returns (Vortex +0xa4). Mac 0x3b55d0
    virtual void SetUserData(void* data) = 0;                                // slot 6
    // The placed geometries of the element, as BrickPhysics::CreateSceneryElement got them
    // (GetGeometry references the geometry for the caller; false for a bad index). Vortex Mac
    // 0x3b5170 / 0x3b5190.
    virtual UInt32 GetGeometryCount() const = 0;                             // slot 7
    virtual bool   GetGeometry(UInt32 index, PhysicsPlacedConfiguredGeometry& geometry) const = 0; // slot 8
    // The element moves with the simulation (Vortex flag +0xac, the VxPart's control mode).
    // ViewMode::AddBall (Mac 0x1d0d10). Mac 0x3b5280
    virtual void SetDynamic(bool dynamic) = 0;                               // slot 9
    virtual bool IsDynamic() const = 0;                                      // slot 10, Mac 0x3b55f0
    // An impulse on the VxPart (VxPart::addImpulse, Mac 0x469194; not a velocity despite the
    // name). ViewMode::PlayBall (Mac 0x1d0290).
    virtual void SetLinearVelocity(const Vector3& velocity) = 0;             // slot 11, Mac 0x3b52e0
    // VxPart::addImpulseAtPosition. Vortex Mac 0x3b5320, Win 0x529950
    virtual void AddImpulseAtPosition(const Vector3& impulse, const Vector3& position) = 0; // slot 12
};

// The mass properties handed to BrickPhysics::CreateSceneryElement (Mac: read by the
// VortexPhysicsSceneryElement constructor 0x3b4af0 at +0x00 / +0x40 / +0x4c; Win constructor
// 0x6121e3 (identity, zero, 0, flags)). 0x54 bytes. (names inferred)
struct PhysicsMassProperties {
    PhysicsMassProperties(Real32 mass = 0.0f, UInt32 flags = 0) : m_mass(mass), m_flags(flags) {}
    Matrix3D m_inertia;        // +0x00 identity
    Vector3  m_centerOfMass;   // +0x40 zero
    Real32   m_mass;           // +0x4c
    UInt32   m_flags;          // +0x50
};

// RTTI: PhysicsSnapshot : IUnknown (BaseBrickPhysicsSnapshot, VortexPhysicsSnapshot Mac vtable
// 0x758d40 {.., 0x3ad3d0, 0x3b2b30, 0x3b2b50, 0x3b2ba0, D1 0x3b2bb0, D0 0x3b2c40}): the
// transforms of a set of bodies, taken by PhysicsScene::CreateSnapshot and put back by
// PhysicsScene::RestoreSnapshot. (names inferred)
class PhysicsSnapshot : public IUnknown {
public:
    static const InterfaceID& GetIID()   // BaseBrickPhysicsSnapshot QI (Mac static 0x771560)
    {
        static const InterfaceID id = {{0x4a9b8132, 0x09496236, 0x222125a5, 0xbf31bf2e}};
        return id;
    }
    virtual void                 Update() = 0;                              // slot 3 (takes the current transforms again)
    virtual UInt32               GetCount() const = 0;                       // slot 4
    virtual PhysicsRigidElement* GetRigidElement(UInt32 index) const = 0;    // slot 5
    virtual void                 Clear() = 0;                                // slot 6
};

// ---- joints ----------------------------------------------------------------------------------

// RTTI: PhysicsConnection : IUnknown; PhysicsHingeConnection, PhysicsSlideHingeConnection,
// PhysicsBallConnection, PhysicsFreeConnection derive from it. GetType of the BaseBrickPhysics
// implementations: hinge 1, slide hinge 4 (NOTE: not 2), ball 5, free 6.
class PhysicsConnection : public IUnknown {
public:
    static const InterfaceID& GetIID()   // answered by every joint (Mac static 0x7713ac)
    {
        static const InterfaceID id = {{0x66ea22dc, 0xa0490f7c, 0x14c4a581, 0xb549560b}};
        return id;
    }
    virtual UInt32 GetType() const = 0;                                      // slot 3
    // ---- slots 4-6 (BaseBrickPhysicsConnectionHelper Mac
    // 0x252af0 / 0x251220 / 0x2512a0; names inferred). Optional outputs may be NULL.
    // The first (`first`) or the second body.
    virtual PhysicsRigidElement* GetRigidElement(bool first) const = 0;      // slot 4
    // The frame of one body in body space.
    virtual void   GetLocalFrame(bool first, Vector3* axis, Vector3* normal,
                                 Vector3* position) const = 0;              // slot 5
    // The frame of the first body in world space (its scene transform).
    virtual void   GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const = 0; // slot 6
    // New frames of both bodies (axis, normal, position each; body space). Mac 0x9c550
    virtual void   SetFrames(PhysicsRigidElement* body, const Vector3& axis, const Vector3& normal,
                             const Vector3& position, PhysicsRigidElement* otherBody,
                             const Vector3& otherAxis, const Vector3& otherNormal,
                             const Vector3& otherPosition) = 0;              // slot 7
    // Joint limits / springs of one field connection (ConnectionField::Attributes, world
    // frames); returns a handle for RemoveConstraint. Mac 0x9c6e0
    virtual void*  AddConstraint(const ConnectionField::Attributes& attributes) = 0;  // slot 8
    virtual void   RemoveConstraint(void* constraint) = 0;                   // slot 9
};

// ---- the four joint interfaces. The ball id 0xbb696a14 is the one
// BaseBrickPhysicsConnectionBall's QueryInterface answers (Mac 0x2524e0). Slot counts from the
// abstract vtables (Mac 0x778b74 ball 13, 0x7713dc hinge 14, 0x778b40 slide hinge 10, 0x7790e4
// free 10); the implementations' destructors follow. Names inferred.

// RTTI: PhysicsHingeConnection : PhysicsConnection (type 1).
class PhysicsHingeConnection : public PhysicsConnection {
public:
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x54d8ce53, 0xa84cdd3d, 0xfef12ca8, 0xa1a8b450}};
        return id;
    }
    // Turns the hinge to `angle`: Rotate(interactive, angle - GetAngle()). Base Mac 0x252f10
    virtual bool   SetAngle(bool interactive, Real32 angle) = 0;             // slot 10
    // Turns the hinge by `delta` (radians); false when it cannot move. Mac 0x9cc30 (caller),
    // VortexPhysicsHingeConnection Mac 0x3a32a0
    virtual bool   Rotate(bool interactive, Real32 delta) = 0;               // slot 11
    // Rotate without the joint limits, the log and EnableManualMotion. Vortex Mac 0x3a3840,
    // Win 0x52a4b0. (name inferred)
    virtual bool   RotateUnlimited(bool interactive, Real32 delta) = 0;      // slot 12
    // Angle between the two normals about the first axis, (-PI, PI]. Base Mac 0x252f60
    virtual Real32 GetAngle() = 0;                                           // slot 13
};

// RTTI: PhysicsSlideHingeConnection : PhysicsConnection (type 4). No slots of its own.
class PhysicsSlideHingeConnection : public PhysicsConnection {
public:
    static const InterfaceID& GetIID()   // Mac static 0x771424
    {
        static const InterfaceID id = {{0x63958b1f, 0x5841de1e, 0x7b0ec39f, 0xb72312ee}};
        return id;
    }
};

// RTTI: PhysicsBallConnection : PhysicsConnection (type 5).
class PhysicsBallConnection : public PhysicsConnection {
public:
    static const InterfaceID& GetIID()   // Mac static 0x762208
    {
        static const InterfaceID id = {{0xbb696a14, 0x3143dd87, 0xd2144faf, 0x69dedcb3}};
        return id;
    }
    virtual bool   Unknown10() = 0;                                          // Vortex Mac 0x3a1f40 (false)
    // Turns the joint by `delta` about `axis` (body space of the first body; the hinge tool
    // passes its ball axis, Mac 0x121010 `this + 0xa4`; Vortex Mac 0x3a1f50 builds a rotation
    // about it). Also called by ConnectionManipulator::Slide Mac 0x9cda0.
    virtual bool   Rotate(bool interactive, Real32 delta, const Vector3& axis) = 0;  // slot 11
    // Euler angles of the second frame relative to the first (Matrix3D::GetEulerAngles).
    // Base Mac 0x252620
    virtual void   GetAngles(Real32& pitch, Real32& yaw, Real32& roll) = 0;  // slot 12
};

// RTTI: PhysicsFreeConnection : PhysicsConnection (type 6, BrickPhysics::CreateFixedConnection).
class PhysicsFreeConnection : public PhysicsConnection {
public:
    static const InterfaceID& GetIID()   // Mac static 0x7790c4
    {
        static const InterfaceID id = {{0xe7b7a5e2, 0x2b461ec8, 0xa83aa8a1, 0x1a234682}};
        return id;
    }
};

// ---- factories / world -----------------------------------------------------------------------

// RTTI: BrickPhysics : IUnknown (VortexBrickPhysics: 23 interface slots, then its destructor).
class BrickPhysics : public IUnknown {
public:
    static const InterfaceID& GetIID()   // BaseBrickPhysics QI (Mac 0x251020, static 0x771374)
    {
        static const InterfaceID id = {{0x6217e974, 0xd4477c2c, 0x9415aa8f, 0x8fbb4b57}};
        return id;
    }
    virtual void Unknown3() = 0;
    virtual void Unknown4() = 0;
    // A new physics world (`flags` passed through; one reference); BrickConnect::NewScene Mac
    // 0xc2810. (name inferred)
    virtual PhysicsScene* CreatePhysicsScene(UInt32 flags) = 0;              // slot 5
    virtual PhysicsRigidElement* CreateRigidElement() = 0;                   // slot 6  (Mac 0xa4880)
    // A new primitive element from `count` placed geometries (the geometries are referenced
    // by the element); `properties` = the bone's <PhysicsAttributes> block (Primitive::Bone +0xa8:
    // inertia tensor, centre of mass, mass, friction type = a PhysicsMassProperties). Caller Mac
    // 0x9e7f0; VortexBrickPhysics Mac 0x39eb50, Win 0x51dab0 (VortexPhysicsPrimitiveElement
    // reads the whole block).
    virtual PhysicsPrimitiveElement* CreatePrimitiveElement(const PhysicsPlacedConfiguredGeometry* geometries,
                                                            UInt32 count,
                                                            const PhysicsMassProperties& properties) = 0; // slot 7
    // A static scenery object of `count` placed geometries (the terrain, LoadTerrain Mac
    // 0x2d5c0). VortexBrickPhysics Mac 0x39ebb0. (name inferred)
    virtual PhysicsSceneryElement* CreateSceneryElement(const PhysicsPlacedConfiguredGeometry* geometries,
                                                        UInt32 count,
                                                        const PhysicsMassProperties& properties) = 0; // slot 8
    virtual PhysicsGeometryBox* CreateBox(const Vector3& halfSize) = 0;       // slot 9  (Mac 0x9e0b0)
    // A cylinder; `length` by reference in both builds (Mac 0x39ec80 reads *param_3, Win
    // 0x51dc40 *param_2). No LDD caller. (name inferred)
    virtual PhysicsGeometryCylinder* CreateCylinder(Real32 radius, const Real32& length) = 0; // slot 10
    virtual PhysicsGeometrySphere* CreateSphere(Real32 radius) = 0;          // slot 11 (Mac 0x9e280)
    // (Primitive::CollisionVolume::m_size.y, .x) = (radius, length)
    virtual PhysicsGeometryCapsule* CreateCapsule(Real32 radius, Real32 length) = 0; // slot 12 (Mac 0x9e440)
    // BrickConnect constructor (knob geometry). Mac 0xc1fc0, Win 0x490460
    virtual PhysicsGeometryConfigable* CreateConfigurableGeometry() = 0;   // slot 13
    // A height field of size.x * size.y samples (`heights`, row by row), sample spacing and
    // height scale `scale`. VortexBrickPhysics Mac 0x39ee30. (name inferred)
    virtual PhysicsGeometrySampledHeightField* CreateSampledHeightField(const UIntVector2& size,
                                                                        const Vector3& scale, bool flag,
                                                                        const Real32* heights) = 0; // slot 14
    // NOTE: VortexBrickPhysics slots 15 / 16 return NULL (Mac 0x39eea0 / 0x39eeb0,
    // Win 0x424e90 / 0x51df20); their arguments are unknown (no caller).
    virtual void Unknown15() = 0;
    virtual void Unknown16() = 0;
    // Joints between two bodies (RigidConnection::Update, Mac 0x99f00 jump table 0x9a544).
    virtual PhysicsConnection* CreateFixedConnection(PhysicsRigidElement* body,
                                                     PhysicsRigidElement* otherBody) = 0; // slot 17 (type 6)
    virtual PhysicsConnection* CreateHingeConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                     const Vector3& normal, const Vector3& position,
                                                     PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                     const Vector3& otherNormal,
                                                     const Vector3& otherPosition) = 0; // slot 18 (type 1)
    virtual PhysicsConnection* CreateSlideHingeConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                          const Vector3& normal, const Vector3& position,
                                                          PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                          const Vector3& otherNormal,
                                                          const Vector3& otherPosition) = 0; // slot 19 (type 2)
    // NOTE: VortexBrickPhysics slot 19 (Mac 0x39ef80, Win 0x51dfc0) and slot 20
    // return NULL; the slide hinge (VortexPhysicsSlideHingeConnection, vtable 0x7585e0) is made by
    // slot 21 (Mac 0x39efa0, Win 0x51dfd0) with the same arguments as slot 18.
    virtual void Unknown20() = 0;
    // The slide hinge joint (VortexPhysicsSlideHingeConnection, type 4) between the frames, as
    // slot 18. NOTE: no LDD code calls it (RigidConnection::Update calls slot 19 for a sliding
    // joint, which the Vortex factory answers with NULL). (name inferred)
    virtual PhysicsConnection* CreateSlidingHingeConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                            const Vector3& normal, const Vector3& position,
                                                            PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                            const Vector3& otherNormal,
                                                            const Vector3& otherPosition) = 0; // slot 21
    virtual PhysicsConnection* CreateBallConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                    const Vector3& normal, const Vector3& position,
                                                    PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                    const Vector3& otherNormal,
                                                    const Vector3& otherPosition) = 0; // slot 22 (type 5)
};

// RTTI: PhysicsScene : IUnknown (VortexPhysicsScene: 49 slots).
class PhysicsScene : public IUnknown {
public:
    static const InterfaceID& GetIID()   // BaseBrickPhysicsScene QI (Mac static 0x771538)
    {
        static const InterfaceID id = {{0x874238d5, 0x414fa8a4, 0x95c4639a, 0x8874d75c}};
        return id;
    }
    // The engine's world (Vortex: the Vx::VxUniverse, Mac 0x3b2cf0).
    virtual void* GetNativeScene() = 0;                                      // slot 3
    // A new debug drawing of the engine objects (Vortex: the constraint frames and forces, the
    // parts' collision boxes; Mac 0x3aaf90), one reference for the caller.
    virtual PhysicsSceneDebugInfo* GetDebugInfo() = 0;                       // slot 4
    // BGAutomaticMode::Enter passes true, Exit false; Vortex 0x3abd30 runs element slot 4
    // (constant 0xbbe56042) resp. slot 5 over its list +0x28/+0x10.
    virtual void Unknown5(bool on) = 0;
    virtual void Unknown6() = 0;
    virtual void Unknown7() = 0;
    // Scene::CreateRigidElement (Mac 0xa4880) / RigidElement merge and split (Mac 0xa0cb0,
    // 0xa38a0).
    virtual void AddRigidElement(PhysicsRigidElement* body, const Matrix3D& transform) = 0; // slot 8
    virtual void RemoveRigidElement(PhysicsRigidElement* body) = 0;          // slot 9
    // ---- slots 10-24 (used by Scene.cpp, TU 025; Unknown6 / Unknown7 above are the update
    // bracket the Scene calls around every structural change: slot 6 before, slot 7 after)
    // Collision test of `count` primitive elements at the given world transforms against the
    // world; false when a collision was found (the callback decides per feature pair) or the
    // progress callback cancelled. Mac 0xa7450 (BatchInsert), 0xa9f20, 0xb0940
    virtual bool TestCollisions(UInt32 count, PhysicsPrimitiveElement* const* elements,
                                const Matrix3D* const* transforms, PhysicsCollisionCallback* callback,
                                PhysicsProgressCallback* progress) = 0;              // slot 10
    // The bodies of `other` moved by `transform` against this world. Mac 0xaacc0
    virtual bool TestCollision(PhysicsScene* other, const Matrix3D& transform,
                               PhysicsCollisionCallback* callback) = 0;               // slot 11
    // Drag scene for TestDragCollision. Mac 0xab0e0 / 0xab280. Slots 12 / 14 (Vortex): false when
    // the drag scene has no geometry / always true.
    virtual bool SetDragScene(PhysicsScene* dragScene) = 0;                            // slot 12
    virtual bool TestDragCollision(const Matrix3D& transform, PhysicsCollisionCallback* callback) = 0; // slot 13
    virtual bool ClearDragScene() = 0;                                                  // slot 14
    // Sweeps the drag scene (`terrainOnly`: a segment against the terrain only) from `from`
    // towards `to`; on a hit `distance` receives how far it got. Caller ClickStickCastUtility
    // Mac 0x363b40, VortexPhysicsScene Mac 0x3acbe0. (name inferred)
    virtual bool CastDragScene(const Matrix3D& from, const Vector3& to, Real32& distance,
                               bool terrainOnly) = 0;                                   // slot 15
    // Collision test of one primitive element of another scene placed at `position` against
    // this world; false when a collision was found (the callback decides per feature pair).
    // Caller BGAutomaticScene::IsMoveFree (Mac 0x35a290, Win 0x59bf50). (name inferred)
    virtual bool TestElementCollision(PhysicsPrimitiveElement* element, const Vector3& position,
                                      PhysicsCollisionCallback* callback) = 0;          // slot 16
    // The current transforms of `count` bodies (a new snapshot, referenced for the caller).
    // VortexPhysicsScene Mac 0x3ad500. (name inferred)
    virtual PhysicsSnapshot* CreateSnapshot(PhysicsRigidElement* const* bodies, UInt32 count) = 0; // slot 17
    // Puts the snapshot's bodies back to its transforms (BaseBrickPhysicsScene Mac 0x255ad0 ->
    // VortexPhysicsScene slot 41, Mac 0x3aea60). (name inferred)
    virtual void RestoreSnapshot(PhysicsSnapshot* snapshot) = 0;                         // slot 18
    // Moves two bodies so that the given frames meet (HingeAlignTool). Arguments as passed by
    // Scene Mac 0xb1410; the zero arguments' meaning is unknown. Returns the new solver (one
    // reference for the caller; NULL on failure): VortexPhysicsScene Mac 0x3adaf0 returns its
    // VortexMoveTogetherSolver's PhysicsSolver, HingeAlignTool Mac 0x159fa0 releases it.
    // Scene::MoveTogether passes, per body, the feature's axis direction first (the parameter
    // named `position` here) and its position second (`axis`), both in body space; `features`
    // may be NULL (count 0); `context` is the MoveTogetherSolverContext (its QueryInterface(
    // PhysicsSolverCallback::GetIID()) gives the callback).
    virtual PhysicsSolver* SolveMoveTogether(PhysicsRigidElement* body, const Vector3& position, UInt32 zero0,
                                     const Vector3& axis, PhysicsRigidElement* otherBody,
                                     const Vector3& otherPosition, UInt32 zero1, const Vector3& otherAxis,
                                     PhysicsCollisionCallback* callback, const PhysicsGeometryIdentifier* features,
                                     UInt32 featureCount, IUnknown* context) = 0;     // slot 19
    // Registers a solver callback; returns a handle object (released at once by the Scene).
    // Mac 0xb1b90 / 0xb1bf0
    virtual IUnknown* AddSolverCallback(PhysicsSolverCallback* callback) = 0;          // slot 20
    // ---- slots 21-24 (BaseBrickPhysicsScene Mac 0x255af0 / 0x255f50 / 0x255ff0 /
    // 0x256100; names inferred)
    // Stops the running solver and starts an animation (gravity) of `count` bodies; `gravity`
    // NULL keeps the default (0, -9.82, 0). The result carries one reference for the caller.
    virtual AnimatePhysicsSolver* CreateAnimatingSolver(PhysicsRigidElement* const* bodies, UInt32 count,
                                                        const Vector3* gravity) = 0;   // slot 21
    // One step of the running solver (`time` in seconds; Win Scene::PhysicsCall5 0x480ed0 passes
    // a float). 0 running, 1 finished, 2 failed / stopped, 3 no solver.
    virtual UInt32 Step(Real32 time) = 0;                                               // slot 22
    // Stops the running solver; `result` as passed to its callbacks (they may clear it).
    virtual bool StopSolver(bool result) = 0;                                           // slot 23
    // The running solver (NULL when none).
    virtual PhysicsSolver* GetSolver() = 0;                                             // slot 24
    // Helpers (not slots) for Scene::PhysicsCall5 / PhysicsCall6 (Mac 0xb1c50 / 0xb1ca0);
    // Simulate5 reinterprets the UInt32 argument as the float time.
    UInt32 Simulate5(UInt32 argument)
    {
        Real32 time;
        memcpy(&time, &argument, sizeof time);
        return Step(time);
    }
    UInt32 Simulate6(bool argument) { return StopSolver(argument); }
    PhysicsSolver* IsSimulating() { return GetSolver(); }
    // ---- slots 25 / 26 (BrickConnectSceneControl's scenery elements)
    // Adds a scenery element at `transform` (`flags`: 2 for the terrain; meaning unknown).
    // VortexPhysicsScene Mac 0x3ae100. (names inferred)
    virtual void AddSceneryElement(PhysicsSceneryElement* element, const Matrix3D& transform,
                                   UInt32 flags) = 0;                                  // slot 25
    // VortexPhysicsScene Mac 0x3ae380.
    virtual void RemoveSceneryElement(PhysicsSceneryElement* element) = 0;             // slot 26
    // ---- slots 27-32 (VortexPhysicsScene Mac 0x3ae5d0-0x3ae0f0; names inferred)
    // Number of scenery elements whose flags (AddSceneryElement) share a bit with `flags`.
    virtual UInt32 GetSceneryElementCount(UInt32 flags) = 0;                            // slot 27
    virtual PhysicsSceneryElement* GetSceneryElement(UInt32 index, UInt32 flags) = 0;   // slot 28
    // Two engine objects of the VortexPhysicsScene (+0xe0 / +0xa4; meaning unknown).
    virtual void  Unknown29(void* object) = 0;                                          // slot 29
    virtual void* Unknown30() = 0;                                                      // slot 30
    virtual void  Unknown31(void* object) = 0;                                          // slot 31
    virtual void* Unknown32() = 0;                                                      // slot 32
    // Slots 33-44 are BaseBrickPhysicsScene's own (they take BaseBrickPhysics* types), then
    // the destructor (BaseBrickPhysicsScene.h).
};

// RTTI: PhysicsCollisionCallback (no base): the collision query callback of the Scene's
// CanConnect / BatchInsert (CanConnectPhysicsCollisionCallback Mac 0xc7b10, …; 025n).
// Called per touching feature pair: CollisionResult (names inferred).
class PhysicsCollisionCallback {
public:
    enum CollisionResult { Collision = 0, Connection = 1, Ignore = 2 };
    virtual UInt32 OnCollision(const PhysicsGeometryIdentifier& feature,
                               const PhysicsGeometryIdentifier& otherFeature) = 0;   // slot 0
};

// RTTI: PhysicsProgressCallback (no base): BatchInsertPhysicsProgressCallback Mac 0xc8560.
class PhysicsProgressCallback {
public:
    // False stops the test (VortexPhysicsScene::TestCollisions Mac 0x3ac15f tests al,
    // BatchInsertPhysicsProgressCallback Mac 0xc85c6 returns m_continue in al).
    virtual bool OnProgress(UInt32 done, UInt32 total) = 0;                  // slot 0
};

class PhysicsSolver;

// RTTI: PhysicsSolverCallback : IUnknown (AnchorPhysicsSolverCallback, MoveTogetherSolverContext).
// Slots 3-6 from the three implementations: MoveTogetherSolverContext
// (Scene.cpp, Mac vtable 0x7624cc {.., 0xc86e0, 0xc8700, 0xc8710, 0xc8740}, Win 0xa23d14
// {.., 0x480700, 0x40be80, 0x480710, 0x40be80}), StringPullSolverCallback (Mac 0x32f5c0 stores
// the solver, 0x32f5d0 releases the anchors, 0x32f620 empty, 0x32f630 steps the anchors with
// the time) and ClickPullSolverCallback (Mac 0x32e6b0 .. 0x32e6e0). The destructor of the
// implementations comes after slot 6 (Mac D1/D0 slots 7/8 in MoveTogetherSolverContext); the
// interface itself has none (no virtual destructor). Every name is inferred.
class PhysicsSolverCallback : public IUnknown {
public:
    // The QueryInterface id that MoveTogetherSolverContext answers (Mac 0xc8600: function-local
    // static Mac 0x762520, Win 0x480730).
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x50b60af7, 0xf24fd744, 0x5f63a89c, 0x2e84ae96}};
        return id;
    }
    // slot 3: the solver starts (StringPullSolverCallback keeps `solver`; MoveTogether records
    // the transforms of the scene).
    virtual void OnBegin(PhysicsSolver* solver) = 0;
    // slot 4: the solver stops (StringPullSolverCallback releases its anchors); BaseBrickPhysicsScene
    // calls it with the result flag before the solver's Stop (Mac 0x256030).
    virtual void OnEnd(bool& result) = 0;
    // slot 5: the solver's result; `success` may be cleared (MoveTogether moves the elements
    // and clears it when that fails).
    virtual void OnResult(bool& success) = 0;
    // slot 6: one solver step of `time` (the anchors of String/ClickPull follow their targets).
    virtual void OnStep(Real32 time) = 0;
};

class AnchorPhysicsSolverAnchor;

// RTTI: AnchorPhysicsSolverCallback : PhysicsSolverCallback (no vtable of its own: abstract).
// Base of StringPullSolverCallback / ClickPullSolverCallback; three more slots
// before their destructors (Mac StringPull 0x32f7a0 / 0x32f960 / 0x32fb10, ClickPull 0x32e8a0
// / 0x32e8b0 / 0x32ea60). The anchor solver of the physics (VortexAnchorSolver, Mac TU 421,
// Vortex/VortexAnchorSolver.cpp) calls them for its anchors: slot 7 from CreateAnchor (Mac 0x39a020, after the
// anchor is registered), slot 8 when an anchor goes (Mac 0x39b1d0), slot 9 when its target
// transform is set (Mac 0x39b010). (names inferred)
class AnchorPhysicsSolverCallback : public PhysicsSolverCallback {
public:
    // The QueryInterface id that StringPull / ClickPullSolverCallback answer (function-local
    // static Mac 0x777694, Win QI 0x4d43a0).
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x9b92118e, 0xd3422ef0, 0x6f29e19a, 0x46891b3b}};
        return id;
    }
    virtual void OnAnchorAdded(AnchorPhysicsSolverAnchor* anchor) = 0;     // slot 7
    virtual void OnAnchorRemoved(AnchorPhysicsSolverAnchor* anchor) = 0;   // slot 8
    virtual void OnAnchorChanged(AnchorPhysicsSolverAnchor* anchor) = 0;   // slot 9
};

// RTTI: PhysicsSolver : IUnknown (Win vtable 0xa29d18: 4 pure slots).
class PhysicsSolver : public IUnknown {
public:
    static const InterfaceID& GetIID()   // BaseBrickPhysicsSimulationSolver QI (Mac static 0x771504)
    {
        static const InterfaceID id = {{0x70701ef5, 0x91410793, 0x30e94a9f, 0xc4647e05}};
        return id;
    }
    // The callback the solver was made with (not referenced). BaseBrickPhysicsSimulationSolver
    // Mac 0x254410. (name inferred)
    virtual PhysicsSolverCallback* GetCallback() = 0;                        // slot 3
};

// RTTI: AnchorPhysicsSolverAnchor : IUnknown (abstract; implemented by VortexAnchorSolver::Anchor
// at +8, Mac TU 421, secondary vtable 0x758124: 29 slots). One body of the model held by the
// anchor solver: a frame on the body (local transform) is pulled towards a target frame, and
// joint constraints (Vortex BallAndSocket, Cylindrical, RPRO, Angular1Position3) can tie the two.
// Used by the String / ClickPull solver callbacks and the Aligners. Slot meanings
// from the Vortex implementation (Mac 0x39af10-0x39cf10); every name is inferred.
class AnchorPhysicsSolverAnchor : public IUnknown {
public:
    // QueryInterface id (VortexAnchorSolver::Anchor Mac 0x39ad80: static Mac 0x778aa8).
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x9181e6d0, 0x10472a6a, 0xe55e8aac, 0x26997147}};
        return id;
    }
    virtual void SetUserData(void* data) = 0;                                // slot 3  Mac 0x39af10
    virtual void* GetUserData() = 0;                                         // slot 4  Mac 0x39af30
    // The anchored body (VortexPhysicsRigidElement).                         slot 5  Mac 0x39af50
    virtual PhysicsRigidElement* GetElement() = 0;
    // The anchor frame in body space / the target frame in world space.
    virtual const Matrix3D& GetLocalTransform() = 0;                        // slot 6  Mac 0x39af70
    virtual const Matrix3D& GetTargetTransform() = 0;                       // slot 7  Mac 0x39af90
    // The bodies moved with the anchored one (CreateAnchor's `bodies`).
    virtual UInt32 GetElementCount() = 0;                                    // slot 8  Mac 0x39afb0
    virtual PhysicsRigidElement* const* GetElements() = 0;                   // slot 9  Mac 0x39aff0
    // New target; the solver callback's OnAnchorChanged follows.           slot 10 Mac 0x39b010
    virtual void SetTargetTransform(const Matrix3D& target) = 0;
    // Takes the anchor out of its solver (OnAnchorRemoved).                 slot 11 Mac 0x39b190
    virtual void Remove() = 0;
    // A spring from `local` (body space) to `target` (world space).         slots 12 / 13
    virtual void ClearPointAttraction() = 0;                                 // Mac 0x39b410
    virtual void SetPointAttraction(const Vector3& local, const Vector3& target) = 0; // Mac 0x39b4d0
    // Ball-and-socket joint of the body point `local` and the world point `target`. slots 14 / 15
    virtual void ClearBallAndSocket() = 0;                                   // Mac 0x39b650
    virtual void SetBallAndSocket(const Vector3& local, const Vector3& target) = 0;  // Mac 0x39b770
    // Cylindrical joint of a body line (position, axis) and a world line.   slots 16 / 17
    virtual void ClearCylindrical() = 0;                                     // Mac 0x39ba90
    virtual void SetCylindrical(const Vector3& position, const Vector3& axis,
                                const Vector3& targetPosition, const Vector3& targetAxis) = 0; // Mac 0x39bb30
    // Relative orientation lock (VxRPRO) with a tolerance.                  slots 18 / 19
    virtual void ClearOrientation() = 0;                                     // Mac 0x39bee0
    virtual void SetOrientation(const Real32& tolerance) = 0;                // Mac 0x39bf80
    // Angular1Position3 constraints (the second part first / the first part first) with a
    // reference direction and an offset.                                    slots 20-23
    virtual void ClearAngular() = 0;                                         // Mac 0x39c0b0
    virtual void SetAngular(const Vector3& position, const Vector3& axis, const Vector3& targetPosition,
                            const Vector3& targetAxis, const Vector3& reference,
                            const Real32& offset) = 0;                       // Mac 0x39c270
    virtual void ClearAngularReversed() = 0;                                 // Mac 0x39c520
    virtual void SetAngularReversed(const Vector3& position, const Vector3& axis,
                                    const Vector3& targetPosition, const Vector3& targetAxis,
                                    const Vector3& reference, const Real32& offset) = 0; // Mac 0x39c630
    virtual void Unknown24() = 0;                                            // slot 24 Mac 0x39c8c0, Win 0x459790 (empty)
    // Two arguments (Win 0x424ec0 is `ret 8`). Mac 0x39c8e0 (empty).
    virtual void Unknown25(UInt32 a, UInt32 b) = 0;                          // slot 25
    // 0 still moving, 1 lost (blocked / no progress), 2 settled. Mac 0x39c900
    virtual UInt32 GetState() = 0;                                           // slot 26
    // The collision features of (element, feature) pairs back on / off.   slots 27 / 28
    virtual void EnableCollisions(const PhysicsGeometryIdentifier* features, UInt32 count) = 0;  // Mac 0x39ce10
    virtual void DisableCollisions(const PhysicsGeometryIdentifier* features, UInt32 count) = 0; // Mac 0x39ce90
    // 29 slots (Mac thunk vtable 0x758124, Win 0xa2aa3c). The next entry (Mac 0x39cf10, Win
    // 0x530910) is VortexAnchorSolver::Anchor::Update, the implementation's own.
};

// RTTI: AnchorPhysicsSolver : PhysicsSolver (abstract; VortexAnchorSolver at +0x18, Mac
// secondary vtable 0x758058). The solver behind PhysicsScene::AddSolverCallback (Vortex Mac
// 0x3ade30 creates a VortexAnchorSolver for the callback and returns this interface).
// 6.4; names inferred)
class AnchorPhysicsSolver : public PhysicsSolver {
public:
    // QueryInterface id (VortexAnchorSolver Mac 0x399ba0: static Mac 0x778a90).
    static const InterfaceID& GetIID()
    {
        static const InterfaceID id = {{0x3d86dae9, 0xda4013ec, 0x817d0dac, 0xd9661ac2}};
        return id;
    }
    // A new anchor on `body` (`local` in body space, `target` in world space) that also moves
    // `bodies`; the callback's OnAnchorAdded runs before it returns. The result carries one
    // reference for the caller (the solver keeps its own). Mac 0x39a020 (thunk 0x39a660)
    virtual AnchorPhysicsSolverAnchor* CreateAnchor(PhysicsRigidElement* body, const Matrix3D& local,
                                                    const Matrix3D& target, UInt32 count,
                                                    PhysicsRigidElement* const* bodies) = 0;  // slot 4
};

// RTTI: AnimatePhysicsSolver : PhysicsSolver (Win vtable 0xa2d544: 5 pure slots). The solver of
// PhysicsScene::CreateAnimatingSolver. (names inferred)
class AnimatePhysicsSolver : public PhysicsSolver {
public:
    static const InterfaceID& GetIID()   // BaseBrickPhysicsAnimatingSolver QI (Mac static 0x771520)
    {
        static const InterfaceID id = {{0xc3825011, 0xdc4b9b4c, 0x117634bb, 0x56b7db36}};
        return id;
    }
    // New gravity (NULL: no change); applied at once while this solver runs. Mac 0x254fe0
    virtual void SetGravity(const Vector3* gravity) = 0;                     // slot 4
};

// The physics of LDD: a shared VortexBrickPhysics (created on the first call; later calls add a
// reference), referenced for the caller (who releases it). Sets up the VxFrame first
// 9.2, VortexBrickPhysics.cpp). NOTE: `xml` / `size` (the physics element settings of db "pes")
// are not read. (name inferred; Mac attribution VortexBrickPhysics)
BrickPhysics* CreateBrickPhysics(const char* xml, UInt32 size);              // Mac 0x39e7c0, Win 0x51d730

} // namespace LEGO
