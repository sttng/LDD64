// LEGO Digital Designer — restored source
// TU 411 "Camera.cpp": Mac 0x38ff60-0x39187f, Win 0x5b7290-0x5b9040. See Camera.h.
// The static initialiser (Mac 0x391840) is only <iostream>.
#include "Camera.h"
#include "MathUtil.h"

namespace LEGO {

namespace {

// Closest distance kept between position and target (Mac literal 0x62fff8, Win fld1).
const Real32 MIN_DISTANCE = 1.0f;                                     // (name inferred)
// Film width in mm of the focal-length model (Win double 0xa2d720). (name inferred)
const Real32 FILM_WIDTH = 35.0f;
// SetTarget: distance = radius / sin(PI / 16); radius 10 when the bounds are empty.
const Real32 FIT_ANGLE_FACTOR = 0.125f;                               // Win double 0xa2dc00
const Real32 FIT_DEFAULT_RADIUS = 10.0f;                              // Win 0xa1ead4
// Pan speed per unit of distance (Mac 0x63f0e0 / 0x63f0e4; Win double 0xa2d880 = the float).
const Real32 PAN_SPEED_HORIZONTAL = 0.000347f;
const Real32 PAN_SPEED_VERTICAL = 0.000325f;

// Pushes each of the four planes back (parallel) so that the sphere lies on its positive side.
// Mac 0x3916a0 (static, after the dtor), Win 0x5b8db0 (EDX = center, ECX = planes; the sphere
// loop is Win 0x5b9010).                                                 (name inferred)
void PushPlanesBehindSphere(const Vector3& center, Real32 radius, Plane* planes)
{
    for (int i = 0; i < 4; ++i) {
        if (planes[i].GetDistance(center) < radius)
            planes[i].SetPoint(center - planes[i].m_normal * radius);
    }
}

// -1, 0, 1 (Mac inline, Win 0x5b9030 for the z components). (name inferred)
int Sign(Real32 v)
{
    int s = 0.0f < v ? 1 : 0;
    if (v < 0.0f)
        s = -1;
    return s;
}

} // namespace

// Mac 0x38ff60 (C2) / 0x390040 (C1), Win 0x5b7290
Camera::Camera(Real32 zNear, Real32 zFar, Real32 focalLength, Real32 maxDistance, UInt32 type)
    : m_type(type),
      m_projection(),
      m_view(),
      m_near(zNear),
      m_far(zFar),
      m_focalLength(focalLength),
      m_maxDistance(maxDistance),
      m_target(),
      m_position(),
      m_up(0.0f, 1.0f, 0.0f)
{
}

// Mac 0x390050 (C2) / 0x3901f0 (C1), Win 0x5b73a0
Camera::Camera(const Camera& other)
    : Object(other),
      m_type(other.m_type),
      m_projection(other.m_projection),
      m_view(other.m_view),
      m_near(other.m_near),
      m_far(other.m_far),
      m_focalLength(other.m_focalLength),
      m_maxDistance(other.m_maxDistance),
      m_target(other.m_target),
      m_position(other.m_position),
      m_up(other.m_up)
{
}

// Mac 0x391650 (D1) / 0x391660 (D0), Win 0x5b8d60 / 0x5b7380 (sdd)
Camera::~Camera()
{
}

// Mac 0x390200, Win 0x5b7570
void Camera::SetLookAt(const Vector3& position, const Vector3& target, const Vector3& up)
{
    m_position = position;
    m_target = target;
    m_up = up;
    Update();
}

// Mac 0x390320, Win 0x5b7610
void Camera::SetLookAt(const Vector3& position, const Vector3& target, const Vector3& up,
                       Real32 focalLength)
{
    m_focalLength = focalLength;
    SetLookAt(position, target, up);
}

// Mac 0x390260, Win 0x5b7eb0
void Camera::Update()
{
    Vector3 direction = m_target - m_position;
    Real32 length = direction.GetLength();
#if defined(_WIN32)
    // NOTE: Win keeps a NaN length (x87 compare), Mac clamps it to MIN_DISTANCE.
    Real32 distance = length;
    if (length < MIN_DISTANCE)
        distance = MIN_DISTANCE;
    else if (m_maxDistance < length)
        distance = m_maxDistance;
#else
    Real32 distance = MIN_DISTANCE;
    if (MIN_DISTANCE <= length) {
        distance = length;
        if (m_maxDistance < length)
            distance = m_maxDistance;
    }
#endif
    direction.SetLength(distance);
    m_position = m_target - direction;
    m_view.SetLookAt(m_position, m_target, m_up);
}

// Mac 0x390390, Win 0x5b7640
void Camera::FitToSpheres(const Vector3& center, const BasicArray<Sphere>& spheres, Real32 width,
                          Real32 height)
{
    Matrix3D inverse;
    inverse.SetInverse(m_view);

    // Half-angles between the view direction and the four side planes.
    Real32 angleX = (Real32)(((EvalReal)PI - GetFieldOfViewX()) * 0.5f);
    Real32 sinX = Sin(angleX);
    Real32 angleY = (Real32)(((EvalReal)PI - GetFieldOfViewY(width, height)) * 0.5f);
    Real32 sinY = Sin(angleY);
    Real32 cosX = Cos(angleX);
    Real32 cosY = Cos(angleY);

    // Side planes (view-space normals rotated to world space) through `center`.
    Plane planes[4];
    Vector3 normals[4] = {Vector3(sinX, 0.0f, -cosX), Vector3(-sinX, 0.0f, -cosX),
                          Vector3(0.0f, -sinY, -cosY), Vector3(0.0f, sinY, -cosY)};
    for (int i = 0; i < 4; ++i) {
        RotateInPlace(normals[i], inverse);
        normals[i].Normalize();
        planes[i] = Plane(normals[i], m_position);
    }
    for (int i = 0; i < 4; ++i)
        planes[i].SetPoint(center);
    for (BasicArray<Sphere>::ConstIterator it = spheres.Begin(); it != spheres.End(); ++it)
        PushPlanesBehindSphere(it->m_center, it->m_radius, planes);

    // The left/right planes meet in line A, the bottom/top planes in line B; the new position is
    // the farther (from `center`) of the closest points between the two lines.
    Vector3 pointA, directionA, pointB, directionB;
    PlanePlaneIntersect(planes[0], planes[1], pointA, directionA);
    PlanePlaneIntersect(planes[2], planes[3], pointB, directionB);
    Vector3 endA = pointA + directionA * 10.0f;
    Vector3 endB = pointB + directionB * 10.0f;

    Vector3 w = pointA - pointB;
    Vector3 v = endB - pointB;
    Vector3 u = endA - pointA;
    Real32 e = v * w;
    Real32 b = u * v;
    Real32 c = v * v;
    Real32 d = u * w;
    Real32 numerator = (Real32)((EvalReal)b * e - (EvalReal)d * c);
    Real32 a = u * u;
    Real32 denominator = (Real32)((EvalReal)a * c - (EvalReal)b * b);
    Real32 s = (Real32)((EvalReal)numerator / denominator);
    Real32 t = (Real32)(((EvalReal)s * b + e) / c);
    Vector3 closestA((Real32)((EvalReal)u.x * s + pointA.x), (Real32)((EvalReal)u.y * s + pointA.y),
                     (Real32)((EvalReal)u.z * s + pointA.z));
    Vector3 closestB((Real32)((EvalReal)v.x * t + pointB.x), (Real32)((EvalReal)v.y * t + pointB.y),
                     (Real32)((EvalReal)v.z * t + pointB.z));

    Vector3 view = m_target - m_position;
    Real32 distanceA = (closestA - center).GetLengthSquared();
    Real32 distanceB = (closestB - center).GetLengthSquared();
    if (distanceB < distanceA)
        m_position = closestA;
    else
        m_position = closestB;
    m_target = view + m_position;

    // Aim at `center` projected onto the view direction.
    Vector3 direction = m_target - m_position;
    direction.Normalize();
    direction.SetLength(direction * (center - m_position));
    m_target = m_position + direction;
}

// Mac 0x390c90, Win 0x5b8b10
Real32 Camera::GetFieldOfViewX() const
{
    Real32 a = ATan((Real32)((EvalReal)FILM_WIDTH / ((EvalReal)m_focalLength + m_focalLength)));
    return a + a;
}

// Win 0x5b8b40 (Mac: inlined in SetViewportSize and FitToSpheres)
Real32 Camera::GetFieldOfViewY(Real32 width, Real32 height) const
{
    Real32 a = ATan((Real32)(((EvalReal)height * FILM_WIDTH) / (((EvalReal)width + width) * m_focalLength)));
    return a + a;
}

// Mac 0x390ce0, Win 0x5b7fb0
void Camera::SetViewportSize(Real32 width, Real32 height)
{
    // NOTE: the original divides by the width (GetFieldOfViewY: 35 h / (2 w f)), so an empty
    // viewport (a 0x0 scene control or render target, e.g. the skybox projection of
    // BrickConnectSceneControl::Draw / the image render) gives an infinite or NaN projection;
    // fixed: the projection is kept for an empty viewport (M14).
    if (!(width > 0.0f && height > 0.0f))
        return;
    Real32 fovY = GetFieldOfViewY(width, height);
    Real32 fovX = GetFieldOfViewX();
    m_projection.SetPerspective(fovX, fovY, m_near, m_far);
}

// Mac 0x390da0, Win 0x5b8000
void Camera::Rotate(Real32 angle)
{
    if (m_type < 2) {
        Matrix3D rotation;
        rotation.SetRotation(m_up, angle);
        m_position = m_target + LEGO::Rotate(m_position - m_target, rotation);
    } else if (m_type == kFly) {
        Matrix3D rotation;
        Matrix3D inverse;
        rotation.SetRotation(m_up, angle);
        inverse.SetInverse(m_view);
        m_target = LEGO::Rotate(m_target * m_view, rotation) * inverse;
    }
    Update();
}

// Mac 0x390f10, Win 0x5b81c0
void Camera::RotateVertical(Real32 angle)
{
    Matrix3D rotation;
    Matrix3D inverse;
    rotation.SetRotationX(angle);
    inverse.SetInverse(m_view);
    if (m_type == kFly) {
        m_target = LEGO::Rotate(m_target * m_view, rotation) * inverse;
    } else if (m_type == kOrbitLock) {
        Vector3 offset = m_position - m_target;
        Vector3 rotated = LEGO::Rotate(LEGO::Rotate(Vector3(0.0f, 0.0f, offset.GetLength()), rotation), inverse);
        // Refuses to go over the top: the horizontal direction must not flip.
        if (Sign(rotated.x) != Sign(offset.x) || Sign(rotated.z) != Sign(offset.z)) {
            Update();
            return;
        }
        m_position = rotated + m_target;
    } else if (m_type == kOrbit) {
        Vector3 offset = m_position - m_target;
        Vector3 rotated = LEGO::Rotate(LEGO::Rotate(Vector3(0.0f, 0.0f, offset.GetLength()), rotation), inverse);
        m_position = rotated + m_target;
        m_up = LEGO::Rotate(Vector3(0.0f, 1.0f, 0.0f), inverse);
    }
    Update();
}

// Mac 0x391190, Win 0x5b8560
void Camera::Zoom(Real32 amount)
{
    Vector3 direction = m_target - m_position;
    Real32 length = direction.GetLength();
    if (m_type < 2) {
        Real32 distance = length - amount;
#if defined(_WIN32)
        if (distance < MIN_DISTANCE)                                  // NOTE: Win keeps NaN
            distance = MIN_DISTANCE;
#else
        if (!(MIN_DISTANCE <= distance))
            distance = MIN_DISTANCE;
#endif
        direction.SetLength(distance);
        m_position = m_target - direction;
    } else if (m_type == kFly) {
        direction.SetLength(amount);
        m_position += direction;
        m_target += direction;
    }
    Update();
}

// Mac 0x391280, Win 0x5b8700
void Camera::PanHorizontal(Real32 from, Real32 to)
{
    Vector3 direction = m_target - m_position;
    Vector3 side = direction ^ m_up;
    Real32 length = direction.GetLength();
    side.SetLength((Real32)(((EvalReal)from - to) * length * PAN_SPEED_HORIZONTAL));
    m_position += side;
    m_target += side;
}

// Mac 0x391340, Win 0x5b8850
void Camera::PanVertical(Real32 from, Real32 to)
{
    Vector3 direction = m_target - m_position;
    Vector3 side = direction ^ m_up;
    Vector3 up = direction ^ side;
    Real32 length = direction.GetLength();
    up.SetLength((Real32)(((EvalReal)from - to) * length * PAN_SPEED_VERTICAL));
    m_position += up;
    m_target += up;
}

// Mac 0x391410, Win 0x5b89e0
void Camera::SetTarget(const Sphere& bounds, const Vector3& target, bool fit)
{
    m_target = target;
    if (fit) {
        Vector3 offset = m_position - m_target;
        Real32 radius = bounds.m_radius != 0.0f ? bounds.m_radius : FIT_DEFAULT_RADIUS;
        Real32 halfAngle = (Real32)((EvalReal)PI * FIT_ANGLE_FACTOR * 0.5f);
        offset.SetLength((Real32)((EvalReal)radius / Sin(halfAngle)));
        m_position = m_target + offset;
    }
    Update();
}

// Mac 0x3914d0, Win 0x5b8b80
Real32 Camera::GetFocalLength() const { return m_focalLength; }

// Mac 0x3914e0, Win 0x5b8b90
void Camera::SetFocalLength(Real32 focalLength) { m_focalLength = focalLength; }

// Mac 0x391500, Win 0x5b8ba0
const Matrix3D& Camera::GetViewMatrix() const { return m_view; }

// Mac 0x391510, Win 0x5b8bb0
Vector3 Camera::GetUpDirection() const
{
    Vector3 forward = GetPosition() - GetTarget();
    forward.Normalize();
    Vector3 up = GetUp() - forward * GetUp().Dot(forward);
    up.Normalize();
    return up;
}

// Mac 0x3915a0, Win 0x5b8c90
const Vector3& Camera::GetPosition() const { return m_position; }

// Mac 0x3915b0, Win 0x5b8ce0
const Vector3& Camera::GetTarget() const { return m_target; }

// Mac 0x3915c0, Win 0x5b8d30
const Vector3& Camera::GetUp() const { return m_up; }

// Mac 0x3915d0, Win 0x5b8c80
const Matrix3D& Camera::GetProjectionMatrix() const { return m_projection; }

// Mac 0x3915e0, Win 0x5b8ca0
void Camera::SetPosition(const Vector3& position) { m_position = position; }

// Mac 0x391600, Win 0x5b8cf0
void Camera::SetTarget(const Vector3& target) { m_target = target; }

// Mac 0x391620, Win 0x5b8d40
Real32 Camera::GetNear() const { return m_near; }

// Mac 0x391630, Win 0x5b8d50
Real32 Camera::GetFar() const { return m_far; }

// Mac 0x391640 (Win inlined)
UInt32 Camera::GetType() const { return m_type; }

} // namespace LEGO
