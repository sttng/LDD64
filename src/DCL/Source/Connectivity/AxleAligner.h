// LEGO Digital Designer — restored source
// LEGO::AxleAligner — the Aligner of axle fields (ConnectionField::TYPE_AXLE): the anchor pulls
//   a point of the own axle to the target axle; within 2.0 of the approach point a Cylindrical
//   joint (or, for grabbing axles, an Angular1Position3 joint) ties the two lines, and once the
//   solver reports the anchor settled the point is locked onto its snapped position on the
//   target axle (ball-and-socket for axle / hole pairs).
//
// TU "AxleAligner.cpp" (Mac TU 347 0x32c250-0x32dc4f; Win 0x4d52a0-0x4d7b5f).
// RTTI: AxleAligner : Aligner. Mac sizeof 0x8c (operator new in Mac 0x32f700 / 0x32e7f0),
//   vtable 0x74c410 {D1 0x32db70, D0 0x32db80, OnFinalRelease, AddRef, Release, Start 0x32d3d0,
//   Stop 0x32d930, IsDone 0x32d990, Step 0x32d9b0}. Win vtable 0xa256a4 {sdd 0x4d6540, ...,
//   Start 0x4d73f0, Stop 0x4d7770, IsDone 0x4d77c0, Step 0x4d77d0}.
// Member and method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Aligner.h"
#include "Geometry.h"

namespace LEGO {

class ConnectionFieldAxle;

class AxleAligner : public Aligner {
public:
    // State m_state.
    enum State { STATE_NEW = 0, STATE_APPROACH = 1, STATE_GUIDED = 2, STATE_LOCKED = 3, STATE_DONE = 4 };
    // Constraint used while guided (m_mode): the grabbing axle's reference direction decides.
    enum Mode { MODE_TARGET_GRABS = 0, MODE_OWN_GRABS = 1, MODE_FREE = 2 };

    // Mac 0x32c250 (C1 0x32c330), Win inlined into the anchors (0x4b2410 / 0x4d4770)
    AxleAligner(AnchorPhysicsSolverAnchor* anchor, const Field& field, const Field& target);
    virtual ~AxleAligner() {}                                           // Mac 0x32db70/0x32db80, Win 0x4d7b00 / sdd 0x4d6540

    // State 0 / 4: compute the frames; lock at once if the own axle is on the target line, else
    // approach. States 1-3: recompute; on a change approach again (a locked one re-locks); then,
    // unless approaching, a settled anchor (GetState 2) locks and a lost one (1) stops. Then
    // Step(0). Mac 0x32d3d0, Win 0x4d73f0
    virtual void Start();
    // Clears every constraint of the anchor; done. Mac 0x32d930, Win 0x4d7770
    virtual void Stop();
    // Mac 0x32d990, Win 0x4d77c0
    virtual bool IsDone() const;
    // Approach: the guide constraint is set when the own point is within 2.0 of the approach
    // point. Guided: settled locks, lost stops. Mac 0x32d9b0, Win 0x4d77d0
    virtual void Step(Real32 time);

    // ---- test access ----
    UInt32 GetState() const { return m_state; }
    UInt32 GetMode() const { return m_mode; }
    const Vector3& GetVector(UInt32 i) const { return (&m_point)[i]; }

    // The eight frames of the alignment (see the members) and m_mode. Mac 0x32c340, Win 0x4d6560
    void Compute(Vector3& point, Vector3& approach, Vector3& position, Vector3& axis,
                 Vector3& targetPosition, Vector3& targetAxis, Vector3& reference,
                 Vector3& targetPoint);

private:
    // The guide constraint of m_mode (no-op when locked), the point spring m_point -> m_targetPoint
    // and, for axle / hole and end stop / axle pairs, the ball-and-socket of the two points.
    // Mac 0x32d7c0, Win 0x4d79c0
    void Lock();
    // The approach: the spring m_point -> m_approach, every other constraint cleared.
    // (Mac inlined in Start 0x32d754, Win 0x4d78a0)
    void Approach();

    // ---- data (Mac offsets) -----------------------------------------------------------------
    Vector3 m_point;            // +0x24 the own point (body space) that is pulled
    Vector3 m_approach;         // +0x30 where it is pulled first (world)
    Vector3 m_position;         // +0x3c the own axle's origin (body space)
    Vector3 m_axis;             // +0x48 the own axle's direction (body space)
    Vector3 m_targetPosition;   // +0x54 the target axle's origin (world)
    Vector3 m_targetAxis;       // +0x60 the target axle's direction (world)
    Vector3 m_reference;        // +0x6c reference direction of the grabbing field (MODE 0 / 1)
    Vector3 m_targetPoint;      // +0x78 where the own point ends (world, snapped)
    UInt32  m_state;            // +0x84 State
    UInt32  m_mode;             // +0x88 Mode (set by Compute)
};

} // namespace LEGO
