// LEGO Digital Designer — restored source
// LEGO::PlanarAligner — the Aligner of planar (stud / tube) fields: the anchor pulls the own
//   connection point in front of the target point (one unit off the target grid along its
//   normal; from behind the target plane it is first pulled around it), then a Cylindrical joint
//   on the point normals guides it in; when the solver reports it settled the knob collision
//   features of the male point are switched off and the points are joined (spring and
//   ball-and-socket). The joint is released again when the point is pulled 0.25 away.
//
// TU "PlanarAligner.cpp" (Mac TU 352 0x333560-0x33463f; Win 0x4d5345-0x4d61ff).
// RTTI: PlanarAligner : Aligner. Mac sizeof 0xa0 (operator new in Mac 0x32e800), vtable
//   0x74ce80 {D1 0x3345b0, D0 0x3345c0, OnFinalRelease, AddRef, Release, Start 0x333690,
//   Stop 0x333d70, IsDone 0x333f50, Step 0x333f70}. Win vtable 0xa25680 {sdd 0x4d53f0, ...,
//   Start 0x4d5410, Stop 0x4d55d0, IsDone 0x4d5770, Step 0x4d5780}.
// Member and method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Aligner.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "PrimitiveElement.h"

namespace LEGO {

class PlanarAligner : public Aligner {
public:
    // State m_state.
    enum State {
        STATE_NEW = 0, STATE_AROUND = 1, STATE_APPROACH = 2, STATE_GUIDED = 3, STATE_JOINING = 4,
        STATE_JOINED = 5, STATE_HELD = 6, STATE_DONE = 7
    };

    // Mac 0x333560 (C1 0x333680), Win inlined into the ClickPull anchor (0x4b2410)
    PlanarAligner(AnchorPhysicsSolverAnchor* anchor, const Field& field, const Field& target);
    // In state 5 the knob collision features are switched on again. Mac 0x3345b0 / 0x3345c0
    // (body 0x3344f0), Win 0x4d5af0 / sdd 0x4d53f0
    virtual ~PlanarAligner();

    // States 0 / 7: the frames are computed; behind the target plane the anchor is pulled
    // around it (1), otherwise to the approach point (2) unless the point already sits on the
    // target normal within 1.0 (guided, then joined). State 6: joined. Then Step(0).
    // Mac 0x333690, Win 0x4d5410
    virtual void Start();
    // Lets go: from 0 a point already at the target on its normal is held (6), else done; from
    // 1-3 the anchor follows its own target (done); from 4 / 5 the ball-and-socket goes and the
    // point is held (6: pulled towards m_hold).
    // Mac 0x333d70, Win 0x4d55d0
    virtual void Stop();
    // Mac 0x333f50, Win 0x4d5770
    virtual bool IsDone() const;
    // The state machine (see the states). Mac 0x333f70, Win 0x4d5780
    virtual void Step(Real32 time);

    // ---- test access ----
    UInt32 GetState() const { return m_state; }
    UInt32 GetStepCount() const { return m_stepCount; }
    const Vector3& GetVector(UInt32 i) const { return (&m_point)[i]; }
    const Plane& GetPlane() const { return m_plane; }
    const BasicArray<PhysicsGeometryIdentifier>& GetFeatures() const { return m_features; }

    // The frames (see the members) and m_plane. Mac 0x333900, Win 0x4d5ba0
    void Compute();

private:
    // Joining: the knob features of the male point off, spring and ball-and-socket of the two
    // points. Mac 0x333cc0, Win 0x4d6170
    void Join();
    // The anchor pulled around the target plane: towards (projection on the plane - target
    // point), unit length, plus twice the plane normal from the target point. (inlined three
    // times in Mac 0x333690 / 0x333f70)
    void PullAround(const Matrix3D& body);

    // ---- data (Mac offsets) -----------------------------------------------------------------
    Vector3 m_point;            // +0x24 the own point (body space), = m_position
    Vector3 m_approach;         // +0x30 1.0 in front of the target point (world)
    Vector3 m_hold;             // +0x3c 10.0 in front of the target point (world)
    Vector3 m_position;         // +0x48 the own grid point (body space)
    Vector3 m_normal;           // +0x54 the own field normal (body space)
    Vector3 m_targetPosition;   // +0x60 the target grid point (world)
    Vector3 m_targetNormal;     // +0x6c the target field normal (world)
    Plane   m_plane;            // +0x78 the target field plane, facing the approach side
    UInt32  m_state;            // +0x88 State
    BasicArray<PhysicsGeometryIdentifier> m_features;   // +0x8c knob features switched off
    UInt32  m_stepCount;        // +0x9c steps since the guide / join started
};

} // namespace LEGO
