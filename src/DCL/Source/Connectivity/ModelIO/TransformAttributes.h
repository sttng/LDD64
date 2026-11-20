// LEGO Digital Designer — restored source
// LEGO::TransformAttributes — the `angle ax ay az tx ty tz` attribute set of the old XML formats
// (LXFML 2/3/4 <Group>/<Part>/<Camera>/<PartRef>, LEGOPrimitive <DefaultOrientation>/<Flex><Bone>),
// read with Expat attribute pairs and turned into a matrix; the LXFML 2/3/4 writers produce it
// from a matrix.
//
// Header-inline in the original: Mac emits ParseAttribute as a weak function in the first object
// using it (0x87880, TU 024 BrickDatabase.cpp) and inlines the rest; Win /LTCG keeps the
// constructor (0x4a2cb0), GetMatrix (0x4a2ce0, EAX = out), ParseAttribute (0x4b3150) and the
// matrix -> attributes constructor of the writers (0x4e0520, ECX = this, EAX = matrix) out of
// line. PrimitiveLoaderXML.cpp has its own file-local copy of the reading half. (struct and
// member names inferred)
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "LEGO/StdLib.h"

namespace LEGO {

struct TransformAttributes {
    Vector3 m_axis;          // +0x00 (0, 1, 0)
    Real32  m_angle;         // +0x0c radians
    Vector3 m_translation;   // +0x10
    Vector3 m_center;        // +0x1c (never set from XML)

    TransformAttributes()                                                // Win 0x4a2cb0
        : m_axis(0.0f, 1.0f, 0.0f), m_angle(0.0f), m_translation(0.0f, 0.0f, 0.0f),
          m_center(0.0f, 0.0f, 0.0f)
    {
    }

    // The rotation (axis and angle) and translation of `m` (writers). Win 0x4e0520; inlined on Mac
    // (0x30c7c6). NOTE: the members start zeroed here (axis (0,0,0)), unlike the default
    // constructor.
    explicit TransformAttributes(const Matrix3D& m)
        : m_axis(0.0f, 0.0f, 0.0f), m_angle(0.0f), m_translation(0.0f, 0.0f, 0.0f),
          m_center(0.0f, 0.0f, 0.0f)
    {
        m.GetAxisAngle(m_axis, m_angle);
        m_translation = m.GetTranslation();
    }

    // One attribute; anything else is ignored. Mac 0x87880, Win 0x4b3150.
    // NOTE: after a failed "angle" compare the t* names are compared even when an a* name
    // matched (both builds) — harmless.
    void ParseAttribute(const char* name, const char* value)
    {
        if (StrCmp(name, "angle") == 0) {
#if defined(_WIN32)
            // Win 0x4b3150: pi (float) / 180.0 (double) * value, rounded once (x87, 53-bit).
            m_angle = (Real32)((Real64)3.1415927f / 180.0 * StrToReal32(value, 0));
#else
            m_angle = StrToReal32(value, 0) * 0.017453292f;     // Mac 0x3c8efa35
#endif
            return;
        }
        if (StrCmp(name, "ax") == 0)
            m_axis.x = StrToReal32(value, 0);
        else if (StrCmp(name, "ay") == 0)
            m_axis.y = StrToReal32(value, 0);
        else if (StrCmp(name, "az") == 0)
            m_axis.z = StrToReal32(value, 0);
        if (StrCmp(name, "tx") == 0)
            m_translation.x = StrToReal32(value, 0);
        else if (StrCmp(name, "ty") == 0)
            m_translation.y = StrToReal32(value, 0);
        else if (StrCmp(name, "tz") == 0)
            m_translation.z = StrToReal32(value, 0);
    }

    // Win 0x4a2ce0 (EAX = out), Mac inlined: a zero axis or angle gives a pure translation.
    void GetMatrix(Matrix3D& out) const
    {
        if (!m_axis.IsZero() && m_angle != 0.0f) {
            out.SetRotation(m_axis, m_angle, m_translation, m_center);
        } else {
            out.SetIdentity();
            out.SetTranslation(m_translation);
        }
    }

    // The angle in degrees for writing. Mac 0x3148eb: angle * 57.295776f (float); Win 0x4e07b1:
    // 180.0 / pi (float) in double precision, times the angle, rounded once.
    Real32 GetDegrees() const
    {
#if defined(_WIN32)
        return (Real32)(180.0 / (Real64)3.1415927f * m_angle);
#else
        return m_angle * 57.295776f;
#endif
    }
};

} // namespace LEGO
