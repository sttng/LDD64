// LEGO Digital Designer — restored source
// Helpers shared by the line fields (ConnectionFieldAxle, ConnectionFieldRail,
// ConnectionFieldSlider). (file and all names inferred)
//
// The Mac build has one identical copy of each function in each of the three TUs (internal
// linkage), so they are restored as file-static functions of a header included by the three
// .cpp files:
//                         Axle (TU 327)   Rail (TU 332)   Slider (TU 333)
//   TestLineOverlap       0x2dd2c0        0x2ea600        0x2eba20
//   FindClosestLines      0x2dba30        0x2e9cc0        0x2eaf50
//   FindClosestLinesRay   0x2dc4a0        0x2ea1d0        0x2eb680
// Win: inlined into the callers (e.g. Axle 0x4b08a0 / 0x4af780 / 0x4af9e0).
#pragma once
#include "ConnectionField.h"

#include <math.h>

namespace LEGO {

// Rounds `value` to a multiple of `step`, ties away from zero; the quotient is truncated first
// (cvttss2si on Mac). (inlined at every use)
static inline Real32 RoundToStep(Real32 value, Real32 step)
{
    Real32 q = (Real32)(Int32)(value / step) * step;
    Real32 r = value - q;
    if (r >= 0.0f) {
        if (r >= 0.5f * step)
            q += step;
    } else {
        if (-0.5f * step >= r)
            q -= step;
    }
    return q;
}

// Overlap of two collinear segments. The pair is sorted into the female (odd kind) and the male
// field; `offset` = position of the male origin along the female axis. Returns 0 (apart),
// 1 (blocked: a capped end is in the way, or `type` 1) or `type`.
static inline UInt32 TestLineOverlap(const ConnectionFieldLineHelper& field, bool sameDirection,
                              const Vector3& position, const Vector3& axis, Real32 length,
                              const Vector3& otherPosition, const Vector3& otherAxis,
                              Real32 otherLength, const ConnectionFieldLineHelper& other,
                              const ConnectionFieldLineHelper** female,
                              const ConnectionFieldLineHelper** male, Real32* offset, UInt32 type)
{
    if (type == 0)
        return 0;
    Real32 d, femaleLength, maleLength;
    if ((field.GetKind() & 1) == 0) {
        *male = &field;
        *female = &other;
        d = (position - otherPosition) * otherAxis;
        femaleLength = otherLength;
        maleLength = length;
    } else {
        *female = &field;
        *male = &other;
        d = (otherPosition - position) * axis;
        femaleLength = length;
        maleLength = otherLength;
    }
    *offset = d;
    if (sameDirection) {
        if (d < 0.0f) {
            if (!(0.005f <= maleLength + d))
                return 0;
        } else if (d >= femaleLength + -0.005f) {
            return 0;
        }
    } else {
        if (d <= 0.005f)
            return 0;
        if (0.005f + d >= femaleLength + maleLength)
            return 0;
    }
    if (type == 1)
        return 1;

    const ConnectionFieldLineHelper* f = *female;
    if (f->IsStartCapped()) {
        if (sameDirection) {
            if (-0.005f > d)
                return 1;
        } else if ((*male)->GetLength() + -0.005f > d) {
            return 1;
        }
    }
    if (f->IsEndCapped()) {
        if (sameDirection) {
            if ((*male)->GetLength() + d > f->GetLength() + 0.005f)
                return 1;
        } else if (d > f->GetLength() + 0.005f) {
            return 1;
        }
    }
    const ConnectionFieldLineHelper* m = *male;
    if (m->IsStartCapped()) {
        if (sameDirection) {
            if (d > 0.005f)
                return 1;
        } else if (f->GetLength() + -0.005f > d) {
            return 1;
        }
    }
    if (m->IsEndCapped()) {
        if (sameDirection) {
            if (f->GetLength() + -0.005f > d + m->GetLength())
                return 1;
        } else if (d > m->GetLength() + 0.005f) {
            return 1;
        }
    }
    return type;
}

// Closest approach of the two field segments along `up` (field space). Only for (anti)parallel
// axes (|dot| >= cos 45 deg). `offset` = position of `other` along this axis.
static inline bool FindClosestLines(const ConnectionFieldLineHelper& field, const Matrix3D& transform,
                             const ConnectionFieldLineHelper& other,
                             const Matrix3D& otherTransform, const Vector3& up,
                             Real32 maxDistance, Real32& distance, Vector3& axis,
                             Vector3& otherAxis, Real32& dot, bool& sameDirection, Real32& offset)
{
    axis = Rotate(up, transform);
    otherAxis = Rotate(up, otherTransform);
    Real32 d = axis * otherAxis;
    dot = d;
    sameDirection = 0.70710677f < d;
    if (dot <= -0.70710677f || 0.70710677f < d) {
        Real32 s, t;
        SegmentSegmentClosest(transform.GetTranslation(), axis, field.GetLength(),
                              otherTransform.GetTranslation(), otherAxis, other.GetLength(), s, t);
        Vector3 p = transform.GetTranslation() + axis * s;
        Vector3 q = otherTransform.GetTranslation() + otherAxis * t;
        Real32 dist = p.GetDistanceSquared(q);
        distance = dist;
        if (dist < maxDistance) {
            if (sameDirection)
                t = -t;
            offset = t + s;
            return true;
        }
    }
    return false;
}

// Same for a pick ray from `origin`: both segments are projected from `origin` onto the plane
// through the field origin with normal `direction`.
static inline bool FindClosestLinesRay(const ConnectionFieldLineHelper& field, const Matrix3D& transform,
                                const ConnectionFieldLineHelper& other,
                                const Matrix3D& otherTransform, const Vector3& origin,
                                const Vector3& direction, const Vector3& up,
                                Real32 maxLineDistance, Real32 maxDistance, Real32& distance,
                                Vector3& axis, Vector3& otherAxis, Real32& dot,
                                bool& sameDirection, Real32& offset)
{
    axis = Rotate(up, transform);
    otherAxis = Rotate(up, otherTransform);
    Real32 d = axis * otherAxis;
    dot = d;
    sameDirection = 0.70710677f < d;
    if (dot <= -0.70710677f || 0.70710677f < d) {
        Plane plane(direction, transform.GetTranslation());
        Vector3 end;
        Vector3 otherStart;
        Vector3 otherEnd;
        LinePlaneIntersectPoints(origin, transform.GetTranslation() + axis * field.GetLength(),
                                 plane, end);
        LinePlaneIntersectPoints(origin, otherTransform.GetTranslation(), plane, otherStart);
        LinePlaneIntersectPoints(origin,
                                 otherTransform.GetTranslation() + otherAxis * other.GetLength(),
                                 plane, otherEnd);
        Vector3 dirA;
        Vector3 dirB;
        Vector3 w;
        Real32 s, t;
        SegmentSegmentClosest(transform.GetTranslation(), end, otherStart, otherEnd, s, t, dirA,
                              dirB, w);
        Vector3 p = transform.GetTranslation() + dirA * s;
        Vector3 q = otherStart + dirB * t;
        if (p.GetDistanceSquared(q) <= maxLineDistance) {
            Real32 position = s * field.GetLength();
            Vector3 point = transform.GetTranslation() + axis * position;
            distance = point.GetDistanceSquared(origin);
            if (distance < maxDistance) {
                t = t * other.GetLength();
                if (sameDirection)
                    t = -t;
                offset = position + t;
                return true;
            }
        }
    }
    return false;
}

// Generic placement along a line: 0.08 steps, clamped to capped ends (up to 0.4 beyond them),
// false when the position is too far outside. (inlined in the three Snap functions)
static inline bool ClampLinePosition(const ConnectionFieldLineHelper& field,
                              const ConnectionFieldLineHelper& other, bool sameDirection,
                              Real32 offset, Real32& position)
{
    Real32 q = RoundToStep(offset, 0.08f);
    Real32 length = field.GetLength();
    Real32 otherLength = other.GetLength();
    if (field.IsStartCapped()) {
        if (sameDirection) {
            if (0.0f > q) {
                if (!(-0.4f <= q))
                    return false;
                q = 0.0f;
            }
        } else if (otherLength > q) {
            if (!(-0.4f + otherLength <= q))
                return false;
            q = otherLength;
        }
    }
    if (field.IsEndCapped()) {
        if (sameDirection) {
            Real32 end = q + otherLength;
            if (end > length) {
                if (end > 0.4f + length)
                    return false;
                q = length - otherLength;
            }
        } else if (q > length) {
            if (q > 0.4f + length)
                return false;
            q = length;
        }
    }
    if (other.IsStartCapped()) {
        if (sameDirection) {
            if (q > 0.0f) {
                if (q > 0.4f)
                    return false;
                q = 0.0f;
            }
        } else if (length > q) {
            if (!(-0.4f + length <= q))
                return false;
            q = length;
        }
    }
    if (other.IsEndCapped()) {
        if (sameDirection) {
            Real32 end = q + otherLength;
            if (length > end) {
                if (-0.4f + length > end)
                    return false;
                q = length - otherLength;
            }
        } else if (q > otherLength) {
            if (q > 0.4f + otherLength)
                return false;
            q = otherLength;
        }
    }
    position = q;
    return true;
}

} // namespace LEGO
