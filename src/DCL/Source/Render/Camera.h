// LEGO Digital Designer — restored source
// LEGO::Camera — perspective camera (position/target/up, focal length on 35 mm film, near/far,
// max orbit distance; modes: orbit, orbit with free up, first person). Names inferred.
// TU 411: Mac 0x38ff60-0x39187f (26 functions), Win
// 0x5b7290-0x5b8db0 (+ helpers 0x5b8db0, 0x5b9010, 0x5b9030). Vtable Mac 0x7574a0 {D1 0x391650,
// D0 0x391660, ..}, Win 0xa2cdbc {sdd 0x5b7380, ..}; sizeof 0xc0. EvalReal floating point
// (Geometry.cpp convention), checked by the restoration's diff tests.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"

namespace LEGO {

class Camera : public Object {
public:
    // Navigation mode (m_type). (names inferred)
    enum Type {
        kOrbit     = 0,   // Rotate*/Zoom move the position around the target; RotateVertical also turns `up`
        kOrbitLock = 1,   // like kOrbit, RotateVertical refuses to flip over the poles, `up` kept
        kFly       = 2    // Rotate* turn the target around the position; Zoom moves both
    };

    Camera(Real32 zNear, Real32 zFar, Real32 focalLength, Real32 maxDistance, UInt32 type); // Mac 0x38ff60 (C1 0x390040), Win 0x5b7290
    Camera(const Camera& other);                                      // Mac 0x390050 (C1 0x3901f0), Win 0x5b73a0
    virtual ~Camera();                                                // Mac 0x391650/0x391660, Win 0x5b8d60/0x5b7380

    // Sets position, target and up, then Update().
    void SetLookAt(const Vector3& position, const Vector3& target, const Vector3& up); // Mac 0x390200, Win 0x5b7570
    void SetLookAt(const Vector3& position, const Vector3& target, const Vector3& up,
                   Real32 focalLength);                               // Mac 0x390320, Win 0x5b7610
    // Clamps the position to [1, maxDistance] from the target and rebuilds the view matrix.
    void Update();                                                    // Mac 0x390260, Win 0x5b7eb0

    // Moves the camera (keeping its direction) so that every sphere is inside the view of a
    // width x height viewport, then aims at the projection of `center` on the view direction.
    // NOTE: does not call Update() (both builds): the view matrix is stale until the next update.
    void FitToSpheres(const Vector3& center, const BasicArray<Sphere>& spheres, Real32 width,
                      Real32 height);                                 // Mac 0x390390, Win 0x5b7640

    Real32 GetFieldOfViewX() const;                                   // Mac 0x390c90, Win 0x5b8b10
    Real32 GetFieldOfViewY(Real32 width, Real32 height) const;        // Win 0x5b8b40 (EAX = this; Mac inlined)
    // Projection for a width x height viewport.
    void SetViewportSize(Real32 width, Real32 height);                // Mac 0x390ce0, Win 0x5b7fb0

    // Rotation about the up vector by `angle` radians.
    void Rotate(Real32 angle);                                        // Mac 0x390da0, Win 0x5b8000
    // Rotation about the view-space x axis.
    void RotateVertical(Real32 angle);                                // Mac 0x390f10, Win 0x5b81c0
    void Zoom(Real32 amount);                                         // Mac 0x391190, Win 0x5b8560
    // Pans by (to - from) screen units scaled by the distance. NOTE: no Update() (both builds).
    void PanHorizontal(Real32 from, Real32 to);                       // Mac 0x391280, Win 0x5b8700
    void PanVertical(Real32 from, Real32 to);                         // Mac 0x391340, Win 0x5b8850
    // Sets the target; with `fit`, moves the position to see `bounds` (radius / sin(pi/16), or
    // 10 / sin(pi/16) for a zero radius).
    void SetTarget(const Sphere& bounds, const Vector3& target, bool fit); // Mac 0x391410, Win 0x5b89e0

    Real32 GetFocalLength() const;                                    // Mac 0x3914d0, Win 0x5b8b80
    void   SetFocalLength(Real32 focalLength);                        // Mac 0x3914e0, Win 0x5b8b90
    const Matrix3D& GetViewMatrix() const;                            // Mac 0x391500, Win 0x5b8ba0
    // `up` made orthogonal to the view direction.
    Vector3 GetUpDirection() const;                                   // Mac 0x391510, Win 0x5b8bb0
    const Vector3& GetPosition() const;                               // Mac 0x3915a0, Win 0x5b8c90
    const Vector3& GetTarget() const;                                 // Mac 0x3915b0, Win 0x5b8ce0
    const Vector3& GetUp() const;                                     // Mac 0x3915c0, Win 0x5b8d30
    const Matrix3D& GetProjectionMatrix() const;                      // Mac 0x3915d0, Win 0x5b8c80
    void   SetPosition(const Vector3& position);                      // Mac 0x3915e0, Win 0x5b8ca0 (no update)
    void   SetTarget(const Vector3& target);                          // Mac 0x391600, Win 0x5b8cf0 (no update)
    Real32 GetNear() const;                                           // Mac 0x391620, Win 0x5b8d40
    Real32 GetFar() const;                                            // Mac 0x391630, Win 0x5b8d50
    UInt32 GetType() const;                                           // Mac 0x391640 (Win inlined)

private:
    Camera& operator=(const Camera&);

    UInt32   m_type;          // +0x08 (Type)
    Matrix3D m_projection;    // +0x0c
    Matrix3D m_view;          // +0x4c
    Real32   m_near;          // +0x8c
    Real32   m_far;           // +0x90
    Real32   m_focalLength;   // +0x94 (mm; 35 mm film width)
    Real32   m_maxDistance;   // +0x98
    Vector3  m_target;        // +0x9c
    Vector3  m_position;      // +0xa8
    Vector3  m_up;            // +0xb4
};

} // namespace LEGO
