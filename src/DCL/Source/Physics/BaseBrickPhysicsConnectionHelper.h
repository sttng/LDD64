// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsConnectionHelper — engine-independent joint state: both bodies, the
// body-space frames (axis / normal / position), scene registration, dirty bookkeeping. Second
// base (+4) of every BaseBrickPhysicsConnection*; first base of VortexHingeSolverHinge /
// VortexMoveTogetherHinge (Mac TU 435).
// BaseBrickPhysicsConnectionHelper.cpp (Mac TU 204, 0x2511b0-0x2524df; Win 0x5c5160 ctor, the
// rest inlined into the Win callers). Mac 0x2522a0 is PhysicsArrayRemove<…Helper>.

// RTTI: class LEGO::BaseBrickPhysicsConnectionHelper (no base). sizeof 0x64 (both builds).
//   Mac vtable 0x736620 {7 pure, D1 0x252270, D0 0x252290}; D2 0x252250 (vptr reset only).
//   Win vtable 0xa2d520 {7 pure, sdd 0x5c5270}.
// The helper slots, as implemented by the joints (names inferred):
//   0 GetType (the joint type), 1 GetNativeConstraint (Vortex: the Vx constraint, Mac 0x3a2cc0),
//   2 HasNativeConstraint (Mac 0x3a2ce0), 3 (bool, Vortex Mac 0x3a2cf0), 4 CheckSeparation,
//   5 UpdateConstraint (rebuilds the engine joint after a scene update, Mac 0x3a2520),
//   6 DestroyConstraint (Mac 0x3a2ac0; also called by Detach / SetFrames).
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "BrickPhysics.h"

namespace LEGO {

class BaseBrickPhysicsRigidElement;
class BaseBrickPhysicsScene;

// The limits CheckSeparation compares with (the squared distance of the two joint positions
// and |axis . otherAxis|) and the number of consecutive failed checks before the joint breaks.
// 0x14 bytes, read-only. (names inferred; filled by the Vortex scene)
struct PhysicsSeparationLimits {
    Real32 m_distance;        // +0x00 squared distance below which the joint holds (result 0)
    Real32 m_maxDistance;     // +0x04 squared distance below which it is stretched (result 1)
    Real32 m_alignment;       // +0x08 |cos| above which a hinge holds
    Real32 m_minAlignment;    // +0x0c |cos| above which a hinge is stretched
    UInt32 m_breakCount;      // +0x10 checks beyond the limits until result 2
};

class BaseBrickPhysicsConnectionHelper {
public:
    // The helper of a joint between `body` and `otherBody` with the given frames (body space);
    // registers itself with the bodies' scene (body->m_scene) and both bodies.
    // Mac 0x251890, Win 0x5c5160 (ESI = this, register arguments)
    BaseBrickPhysicsConnectionHelper(BaseBrickPhysicsRigidElement* body, const Vector3& axis,
                                     const Vector3& normal, const Vector3& position,
                                     BaseBrickPhysicsRigidElement* otherBody, const Vector3& otherAxis,
                                     const Vector3& otherNormal, const Vector3& otherPosition);
    // (additive) A second joint between the same bodies with the same frames,
    // registered like the first (the spring mirrors of VortexPhysicsScene, Mac TU 435). Mac
    // 0x251d70 (the scene is other.m_body->m_scene; Win inlined)
    BaseBrickPhysicsConnectionHelper(const BaseBrickPhysicsConnectionHelper& other);
    virtual ~BaseBrickPhysicsConnectionHelper() {}            // Mac D1 0x252270 / D0 0x252290 (D2 0x252250), Win sdd 0x5c5270

    virtual UInt32 GetType() const = 0;                                      // slot 0
    virtual void*  GetNativeConstraint() = 0;                                // slot 1
    virtual bool   HasNativeConstraint() = 0;                                // slot 2
    virtual bool   Unknown3() = 0;                                           // slot 3
    // 0: the joint holds, 1: stretched, 2: broken (m_breakCount checks in a row beyond the
    // limits; the counter is m_separationCount).
    virtual UInt32 CheckSeparation(const PhysicsSeparationLimits& limits) = 0; // slot 4
    virtual bool   UpdateConstraint() = 0;                                   // slot 5
    virtual void   DestroyConstraint() = 0;                                  // slot 6

    // Unregisters the joint (bodies, scene) and forgets both bodies and the scene; the joints'
    // Release does this before deleting. Mac 0x2511b0 (Win inlined)
    void Detach();
    // PhysicsConnection slot 5. Mac 0x251220
    void GetLocalFrame(bool first, Vector3* axis, Vector3* normal, Vector3* position) const;
    // PhysicsConnection slot 6: the first body's frame in world space (its GetTransform).
    // Mac 0x2512a0
    void GetWorldFrame(Vector3* axis, Vector3* normal, Vector3* position) const;
    // PhysicsConnection slot 7: new bodies / frames (the engine joint is destroyed and the
    // helper is marked dirty). Mac 0x251350
    void SetFrames(BaseBrickPhysicsRigidElement* body, const Vector3& axis, const Vector3& normal,
                   const Vector3& position, BaseBrickPhysicsRigidElement* otherBody,
                   const Vector3& otherAxis, const Vector3& otherNormal, const Vector3& otherPosition);
    // Queues the helper for UpdateConstraint at the end of the scene update (once). Mac 0x251860
    void MarkDirty();

    // ---- data -------------------------------------------------------------------------------
    UInt32                        m_mark;             // +0x04 0; visited flag of the scene's graph walks
    UInt32                        m_separationCount;  // +0x08 0; CheckSeparation counter
    BaseBrickPhysicsRigidElement* m_body;             // +0x0c
    BaseBrickPhysicsRigidElement* m_otherBody;        // +0x10
    BaseBrickPhysicsScene*        m_scene;            // +0x14 body->m_scene at construction
    Vector3                       m_axis;             // +0x18
    Vector3                       m_otherAxis;        // +0x24
    Vector3                       m_position;         // +0x30
    Vector3                       m_otherPosition;    // +0x3c
    Vector3                       m_normal;           // +0x48
    Vector3                       m_otherNormal;      // +0x54
    UInt8                         m_unknown60;        // +0x60 not initialised, not referenced here
    bool                          m_dirty;            // +0x61 queued in the scene's dirty list

private:
    BaseBrickPhysicsConnectionHelper& operator=(const BaseBrickPhysicsConnectionHelper&);
};

} // namespace LEGO
