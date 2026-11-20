// LEGO Digital Designer — restored source
// LEGO::VortexBrickPhysics — LDD's physics factory on CM Labs Vortex (BrickPhysics slots 3-22:
// worlds, bodies, primitive / scenery elements, geometries, joints) and CreateBrickPhysics, which
// sets up the VxFrame and hands out the shared instance.
// VortexBrickPhysics.cpp (Mac TU 422, 0x39e7c0-0x39f54f; Win 0x51d730-0x51e36f). Its
// BasicArray<VortexConnectionAttributeDatas::AttributesData> destructors (Mac 0x39f1c0 /
// 0x39f1f0, Win sdd 0x51e340) are the template's (VortexConnectionAttributeDatas.h).

// RTTI: class LEGO::VortexBrickPhysics : BaseBrickPhysics : BrickPhysics : IUnknown. sizeof 0x20
//   (operator new(0x20) in CreateBrickPhysics; +8 .. +0x18 zeroed).
//   Mac vtable 0x7581f8 {BaseBrickPhysics QI / AddRef / Release, 0x39e9f0, 0x39ea00, 0x39ea10,
//     0x39eb10, 0x39eb50, 0x39ebb0, 0x39ec10, 0x39ec80, 0x39ed00, 0x39ed70, 0x39edf0, 0x39ee30,
//     0x39eea0, 0x39eeb0, 0x39f130, 0x39eec0, 0x39ef80, 0x39ef90, 0x39efa0, 0x39f060, D1 0x39e9a0,
//     D0 0x39e9b0}.
//   Win vtable 0xa29024 {0x5c4cb0, 0x5c7750, 0x5c4cf0, 0x459790, 0x459790, 0x51d940, 0x51da50,
//     0x51dab0, 0x51db30, 0x51dba0, 0x51dc40, 0x51dcf0, 0x51dd90, 0x51de40, 0x51dea0, 0x424e90,
//     0x51df20, 0x51e0f0, 0x51df30, 0x51dfc0, 0x51dfc0, 0x51dfd0, 0x51e060, sdd 0x51d880}.
//
// Licensing: the Mac build has no license code. The Win original's Vortex is
// FLEXlm-gated (features mdt / mcd / vxadvcon / vxcable); without a license the dynamics degrade
// (no contact / constraint forces). The restored library implements the licensed path only
// (the VxLicense switch is not restored); this factory does not touch licensing.
//
// Every name is inferred.
#pragma once
#include "LEGO/Types.h"
#include "BaseBrickPhysics.h"
#include "Vx/VxArray.h"

namespace Vx { class VxUniverse; }

namespace LEGO {

class VortexPhysicsGeometry;

class VortexBrickPhysics : public BaseBrickPhysics {
public:
    // Mac inlined into CreateBrickPhysics 0x39e7c0, Win 0x51d810
    VortexBrickPhysics() : m_unknown08(0), m_unknown0c(0) {}
    // Forgets the shared instance and destroys the VxFrame (with every universe left in it).
    // Mac D1 0x39e9a0 (body 0x39e900) / D0 0x39e9b0, Win 0x51d8a0 / sdd 0x51d880
    virtual ~VortexBrickPhysics();

    // ---- BrickPhysics
    virtual void Unknown3();                                                 // Mac 0x39e9f0, Win 0x459790 (empty)
    virtual void Unknown4();                                                 // Mac 0x39ea00, Win 0x459790 (empty)
    // A VortexPhysicsScene on `flags` taken as a Vx::VxUniverse* (0: a new universe of 10000 parts
    // / 100000 pairs, added to the frame, time step 0, no auto-disable). NOTE: every LDD caller
    // passes 0. Mac 0x39ea10, Win 0x51d940
    virtual PhysicsScene* CreatePhysicsScene(UInt32 flags);
    virtual PhysicsRigidElement* CreateRigidElement();                       // Mac 0x39eb10, Win 0x51da50
    virtual PhysicsPrimitiveElement* CreatePrimitiveElement(const PhysicsPlacedConfiguredGeometry* geometries,
                                                            UInt32 count,
                                                            const PhysicsMassProperties& properties); // Mac 0x39eb50, Win 0x51dab0
    virtual PhysicsSceneryElement* CreateSceneryElement(const PhysicsPlacedConfiguredGeometry* geometries,
                                                        UInt32 count,
                                                        const PhysicsMassProperties& properties);     // Mac 0x39ebb0, Win 0x51db30
    // The box / cylinder / sphere / capsule are also kept in m_geometries (not referenced).
    virtual PhysicsGeometryBox* CreateBox(const Vector3& halfSize);           // Mac 0x39ec10, Win 0x51dba0
    virtual PhysicsGeometryCylinder* CreateCylinder(Real32 radius, const Real32& length); // Mac 0x39ec80, Win 0x51dc40
    virtual PhysicsGeometrySphere* CreateSphere(Real32 radius);              // Mac 0x39ed00, Win 0x51dcf0
    virtual PhysicsGeometryCapsule* CreateCapsule(Real32 radius, Real32 length); // Mac 0x39ed70, Win 0x51dd90
    virtual PhysicsGeometryConfigable* CreateConfigurableGeometry();         // Mac 0x39edf0, Win 0x51de40
    virtual PhysicsGeometrySampledHeightField* CreateSampledHeightField(const UIntVector2& size,
                                                                        const Vector3& scale, bool flag,
                                                                        const Real32* heights); // Mac 0x39ee30, Win 0x51dea0
    virtual void Unknown15();                                                // Mac 0x39eea0, Win 0x424e90 (NULL)
    virtual void Unknown16();                                                // Mac 0x39eeb0, Win 0x51df20 (NULL)
    virtual PhysicsConnection* CreateFixedConnection(PhysicsRigidElement* body,
                                                     PhysicsRigidElement* otherBody);          // Mac 0x39f130, Win 0x51e0f0
    virtual PhysicsConnection* CreateHingeConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                     const Vector3& normal, const Vector3& position,
                                                     PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                     const Vector3& otherNormal,
                                                     const Vector3& otherPosition);            // Mac 0x39eec0, Win 0x51df30
    // NOTE: NULL (the sliding joint of RigidConnection::Update gets no engine joint).
    virtual PhysicsConnection* CreateSlideHingeConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                          const Vector3& normal, const Vector3& position,
                                                          PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                          const Vector3& otherNormal,
                                                          const Vector3& otherPosition);       // Mac 0x39ef80, Win 0x51dfc0
    virtual void Unknown20();                                                // Mac 0x39ef90, Win 0x51dfc0 (NULL)
    virtual PhysicsConnection* CreateSlidingHingeConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                            const Vector3& normal, const Vector3& position,
                                                            PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                            const Vector3& otherNormal,
                                                            const Vector3& otherPosition);     // Mac 0x39efa0, Win 0x51dfd0
    virtual PhysicsConnection* CreateBallConnection(PhysicsRigidElement* body, const Vector3& axis,
                                                    const Vector3& normal, const Vector3& position,
                                                    PhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                                    const Vector3& otherNormal,
                                                    const Vector3& otherPosition);             // Mac 0x39f060, Win 0x51e060

    // Empty in the shipped build; VortexPhysicsScene calls it after its universe changed
    // (StepPhysics, SetDragScene, EndUpdate). Mac 0x39e8f0 (5 bytes: ret), Win inlined (nothing).
    void UpdateUniverse(Vx::VxUniverse* universe);

    // The shared instance (Mac 0x77f4f8, Win 0xb4276c) and its "created here" flag (Mac 0x77f4fc,
    // Win 0xb42765; cleared by the destructor, read by nothing else).
    static VortexBrickPhysics* s_instance;
    static bool                s_created;

    UInt32                             m_unknown08;    // +0x08 0 (not referenced)
    UInt32                             m_unknown0c;    // +0x0c 0 (not referenced)
    // The geometries made by slots 9-12 (VortexPhysicsScene::Unknown5 inflates / resets them
    // around a collision test, Mac 0x3abd30: VortexPhysicsGeometry slots 4 / 5). NOTE: never
    // removed - a released geometry stays in the list (the original's dangling entries).
    Vx::VxArray<VortexPhysicsGeometry*> m_geometries;  // +0x10 (16 bytes: its allocator word +0x1c)
};

} // namespace LEGO
