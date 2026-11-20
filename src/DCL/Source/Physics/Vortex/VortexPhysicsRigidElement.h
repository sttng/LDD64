// LEGO Digital Designer — restored source
// LEGO::VortexPhysicsSceneElement / LEGO::VortexPhysicsRigidElement — a body of the Vortex physics
// (a Vx::VxPart) with the collision geometries of its elements.
// VortexPhysicsRigidElement.cpp (Mac TU 434, 0x3aa050-0x3aac2f; Win 0x52ba00-0x52becf, 0x52bed0
// CreateBody, 0x5c79c0, 0x529470). The VortexPhysicsSceneElement methods are emitted in the next
// object, LDDVortexCollisionFilter.cpp (Mac TU 436, 0x3b43c0-0x3b4980; Win 0x52da40, 0x52dc60,
// 0x52dd70), and restored there.

// RTTI: class LEGO::VortexPhysicsSceneElement : VortexRigidBodyUserData. Mac vtable 0x778d60
//   {D1 0x3aabc0, D0 0x3aabe0}; Win 0xa2a288 {sdd 0x529470}; constructor Win 0x5293b0 (Mac
//   inline). sizeof 0x18 (VortexPhysicsSceneryElement has its next base at +0x1c = 4 + 0x18).
// RTTI: class LEGO::VortexPhysicsRigidElement : BaseBrickPhysicsRigidElement (+0),
//   VortexPhysicsSceneElement (+0xcc). sizeof 0xe4.
//   Mac vtable 0x7589f8 {base 0-6, 0x3aa4f0, base 0x253d30, 0x3aa340, 0x3aa1c0, 0x3aa3c0,
//       0x3aa490, 0x3aa5a0, 0x3aa680, 0x3aa730, 0x3aaa00, D1 0x3aaa20, D0 0x3aaa80}; secondary
//       0x758a4c {0x3aaaf0, 0x3aab50}.
//   Win vtable 0xa2a4e4 {base 0-6, 0x52bc50, base 0x5c79c0, 0x52bb30, 0x52baa0, 0x52bb50,
//       0x52bc20, 0x52bcf0, 0x52bda0, 0x52be30, 0x52bec0, sdd 0x52ba00}; secondary 0xa2a530
//       {0x52c0a0}.
//   NOTE: the Win slots 9-12 are one off against the obvious pairing (Win 0x52bb30 is
//   GetEngineTransform, 0x52baa0 SetEngineTransform, 0x52bb50 IsMoving, 0x52bc20 Stop).
// The joints and geometries read the VxPart at +0xd4 (Mac 0x3a2520 "*(body + 0xd4)"). Every name is
// inferred unless noted.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "BaseBrickPhysicsRigidElement.h"
#include "VortexShapeUserData.h"

namespace Vx {
class VxPart;
class VxMechanism;
class VxCollisionGeometry;
}

namespace LEGO {

class VortexShapeCollisionFilter;
class VortexPhysicsPlacedGeometry;

// The Vx::VxPart of a body and how its collision geometries are grouped (directly on the part,
// or in composites of m_compositeSize). The VxPart's user data is this object.
class VortexPhysicsSceneElement : public VortexRigidBodyUserData {
public:
    // Inline on Mac (VortexPhysicsRigidElement Mac 0x3aa150 / VortexPhysicsSceneryElement Mac
    // 0x3b4af0), Win 0x5293b0
    explicit VortexPhysicsSceneElement(UInt32 kind)
        : VortexRigidBodyUserData(kind), m_part(0), m_massUnit(1.0f), m_useComposites(true), m_compositeSize(16)
    {
    }
    virtual ~VortexPhysicsSceneElement() {}               // Mac D1 0x3aabc0 / D0 0x3aabe0, Win sdd 0x529470

    // A new dynamic VxPart (user data: this) in `mechanism`. Mac 0x3b43c0 (Win inlined)
    void CreatePart(Vx::VxMechanism* mechanism);
    // Takes the part out of its mechanism, detaches its collision geometries and deletes it.
    // Mac 0x3b4440, Win 0x52da40
    void DestroyPart();
    // A dynamic part's mass scaled by `scale` / m_massUnit. Mac 0x3b4550 (Win inlined)
    void ScaleMass(Real32 scale);
    // The composite the next geometry goes into: `last` (the part's last collision geometry when
    // NULL) unless it is full; a new one (with `filter`) otherwise, also when the part has none.
    // NOTE: like the original, the part's last geometry is taken as a composite unchecked.
    // Mac 0x3b45b0 (Win inlined)
    Vx::VxCollisionGeometry* GetComposite(Vx::VxCollisionGeometry* last, VortexShapeCollisionFilter* filter);
    // Puts `placed`'s collision geometry (made when missing) on the part, at `transform` (the
    // element's placement in the body; may be NULL) times its own; `update`: the collision model
    // is refreshed at once. Logs "VortexPhysicsRigidElement::addRigidElements: ..." when the
    // geometry is on a body already. Mac 0x3b46c0, Win 0x52dc60
    void AddGeometry(VortexShapeCollisionFilter* filter, const Matrix3D* transform, VortexPhysicsPlacedGeometry* placed,
                     bool update);
    // Takes `placed`'s collision geometry off the part (when it is on it). Mac 0x3b4900 (Win
    // inlined into 0x52bda0)
    void RemoveGeometry(VortexPhysicsPlacedGeometry* placed);
    // Takes `geometry` out of its composite (deleting the composite when it becomes empty) or off
    // its entity. Mac 0x3b4920 (ECX = geometry), Win 0x52dd70
    static void RemoveCollisionGeometry(Vx::VxCollisionGeometry* geometry);

    Vx::VxPart* m_part;           // +0x08 the body (VxPart user data: this)
    Real32      m_massUnit;       // +0x0c 1 (ScaleMass divides by it)
    bool        m_useComposites;  // +0x10 true: the geometries go into composites
    UInt32      m_compositeSize;  // +0x14 16 geometries per composite

private:
    VortexPhysicsSceneElement(const VortexPhysicsSceneElement&);
    VortexPhysicsSceneElement& operator=(const VortexPhysicsSceneElement&);
};

class VortexPhysicsRigidElement : public BaseBrickPhysicsRigidElement, public VortexPhysicsSceneElement {
public:
    // Kind 1, no part yet (Update makes it). Mac 0x3aa150, Win 0x52ba20
    VortexPhysicsRigidElement() : VortexPhysicsSceneElement(KindRigid) {}
    // NOTE: does not destroy the part (ReleaseEngineBody does). Mac D1 0x3aaa20 / D0 0x3aaa80
    // (secondary thunks 0x3aaaf0 / 0x3aab50), Win sdd 0x52ba00
    virtual ~VortexPhysicsRigidElement() {}

    // The base, then a collision model update of the part's geometries from the last one before
    // the call on. Mac 0x3aa4f0, Win 0x52bc50
    virtual void AddPrimitiveElement(PhysicsPrimitiveElement* element, const Matrix3D& transform);
    // The VxPart's transform (as floats). Mac 0x3aa340, Win 0x52bb30
    virtual void GetEngineTransform(Matrix3D& transform) const;
    // Moves the part (re-orthonormalised), refreshes its geometries unless frozen, clears its
    // forces; also the cached transform. Mac 0x3aa1c0, Win 0x52baa0
    virtual void SetEngineTransform(const Matrix3D& transform);
    // NOTE: both the linear and the angular speed squared above 1e-6 (the original's &&).
    // Mac 0x3aa3c0, Win 0x52bb50
    virtual bool IsMoving() const;
    // The part's linear / angular velocity damping set to 0 (VxPart Mac 0x46860e / 0x468b06).
    // Mac 0x3aa490, Win 0x52bc20
    virtual void Stop();
    // The geometries of a VortexPhysicsPrimitiveElement go on the part (logs
    // "VortexPhysicsRigidElement::OnAddPrimitiveElement: rigidBody not a
    // VortexPhysicsPrimitiveElement." for another element). Mac 0x3aa5a0, Win 0x52bcf0
    virtual void OnAddPrimitiveElement(PhysicsPrimitiveElement* element);
    // ... come off it. Mac 0x3aa680, Win 0x52bda0
    virtual void OnRemovePrimitiveElement(PhysicsPrimitiveElement* element);
    // No elements: ReleaseEngineBody; else CreateBody when there is no part. Clears the dirty
    // flag. Mac 0x3aa730, Win 0x52be30
    virtual void Update();
    // DestroyPart. Mac 0x3aaa00, Win 0x52bec0
    virtual void ReleaseEngineBody();

    // The part at the cached transform with the elements' geometries; by the body kind m_kind:
    // 1-3 dynamic (mass properties from the geometries, then mass 1), 4 / 5 the geometries only
    // (rigid elements are always KindRigid); frozen; user data = the element; added to the
    // scene's mechanism. Mac 0x3aa770 (inlined into Update), Win 0x52bed0 (+ 0x52be70)
    void CreateBody();
    // The part's accumulated forces cleared (VxPart::resetForces). Mac 0x3aa4d0 (Win inlined)
    void ResetForces();

    // The VxPart of the body. (helper, not in the original)
    Vx::VxPart* GetPart() const { return m_part; }

private:
    VortexPhysicsRigidElement(const VortexPhysicsRigidElement&);
    VortexPhysicsRigidElement& operator=(const VortexPhysicsRigidElement&);
};

} // namespace LEGO
