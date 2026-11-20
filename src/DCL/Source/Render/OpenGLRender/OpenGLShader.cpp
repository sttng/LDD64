// LEGO Digital Designer — restored source
// OpenGLShader.cpp (TU 180; `__FILE__` "…/DCL/Source/Render/OpenGLRender/OpenGLShader.cpp").
// Also emitted here in the original: Shader's D1/D0 (Mac 0x2364a0/0x2364c0) and
// Shader::SetMaterialColors (Mac 0x235f90, Win 0x4fa5a0) — both written inline in Shader.h.
#include "OpenGLShader.h"
#include "OpenGLRender.h"
#include "OpenGLWindow.h"

namespace LEGO {

// Inline in the original (Mac: part of 0x3873f0 / 0x387910, Win 0x4fab20).
OpenGLShader::OpenGLShader(OpenGLRender* render)
    : m_render(render)
{
}

// Mac D2 0x234dd0 (arrays, map, render reference), D1 0x234ef0, D0 0x234fb0.
OpenGLShader::~OpenGLShader()
{
}

// Mac 0x236570, Win 0x4fac00
bool OpenGLShader::IsOpaque() const
{
    return m_opaque;
}

// Mac 0x236580, Win 0x4fac10 (ICF: every `return -1` slot)
Int32 OpenGLShader::GetAttributeLocation(const AString& name)
{
    (void)name;
    return -1;
}

// Mac 0x2352a0: push_back on the uniform (+0x28) or attribute (+0x38) list.
void OpenGLShader::Record(bool attribute, Int32 location, const VariableType& value)
{
    VariableType v(value);
    v.location = location;
    if (attribute)
        m_attributes.PushBack(v);
    else
        m_uniforms.PushBack(v);
}

namespace {

// The float overloads build the record with the 16-float Matrix3D constructor (Mac 0x1df4e0):
// the values in elements 0-3, the rest 0.
OpenGLShader::VariableType MakeFloats(UInt32 type, Real32 x, Real32 y, Real32 z, Real32 w)
{
    OpenGLShader::VariableType v;
    v.type = type;
    v.matrix = Matrix3D(x, y, z, w, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    v.ints[0] = v.ints[1] = v.ints[2] = v.ints[3] = 0;
    v.transpose = false;
    v.location = -1;
    return v;
}

// The integer overloads default-construct the matrix (identity, Mac 0x1df350).
OpenGLShader::VariableType MakeInts(UInt32 type, Int32 x, Int32 y, Int32 z, Int32 w)
{
    OpenGLShader::VariableType v;
    v.type = type;
    v.ints[0] = x;
    v.ints[1] = y;
    v.ints[2] = z;
    v.ints[3] = w;
    v.transpose = false;
    v.location = -1;
    return v;
}

} // namespace

// Shared body of the nine overloads (each one inlines it, inside the try/catch that reports
// an OpenGL crash with this file's name). NOTE: a uniform is recorded even when its location
// is -1 (glUniform* ignores -1); an attribute is dropped.
#define LDD_RECORD_VARIABLE(record)                                    \
    LDD_GL_TRY                                                         \
    if (!attribute) {                                                  \
        Int32 location = GetUniformLocation(name);                     \
        Record(false, location, (record));                             \
    } else {                                                           \
        Int32 location = GetAttributeLocation(name);                   \
        if (location != -1)                                            \
            Record(true, location, (record));                          \
    }                                                                  \
    LDD_GL_CATCH

// Mac 0x2350a0, Win 0x504e50
void OpenGLShader::SetVariable(const AString& name, Real32 x, bool attribute)
{
    LDD_RECORD_VARIABLE(MakeFloats(VariableType::kFloat1, x, 0, 0, 0))
}

// Mac 0x2353f0, Win 0x504f30
void OpenGLShader::SetVariable(const AString& name, Real32 x, Real32 y, bool attribute)
{
    LDD_RECORD_VARIABLE(MakeFloats(VariableType::kFloat2, x, y, 0, 0))
}

// Mac 0x2355f0, Win 0x505020
void OpenGLShader::SetVariable(const AString& name, Real32 x, Real32 y, Real32 z, bool attribute)
{
    LDD_RECORD_VARIABLE(MakeFloats(VariableType::kFloat3, x, y, z, 0))
}

// Mac 0x235800, Win 0x505120
void OpenGLShader::SetVariable(const AString& name, Real32 x, Real32 y, Real32 z, Real32 w,
                               bool attribute)
{
    LDD_RECORD_VARIABLE(MakeFloats(VariableType::kFloat4, x, y, z, w))
}

// Mac 0x235a10, Win 0x505220
void OpenGLShader::SetVariable(const AString& name, Int32 x, bool attribute)
{
    LDD_RECORD_VARIABLE(MakeInts(VariableType::kInt1, x, 0, 0, 0))
}

// Mac 0x235b30, Win 0x5052f0
void OpenGLShader::SetVariable(const AString& name, Int32 x, Int32 y, bool attribute)
{
    LDD_RECORD_VARIABLE(MakeInts(VariableType::kInt2, x, y, 0, 0))
}

// Mac 0x235c50, Win 0x5053d0
void OpenGLShader::SetVariable(const AString& name, Int32 x, Int32 y, Int32 z, bool attribute)
{
    LDD_RECORD_VARIABLE(MakeInts(VariableType::kInt3, x, y, z, 0))
}

// Mac 0x235d70, Win 0x5054b0
void OpenGLShader::SetVariable(const AString& name, Int32 x, Int32 y, Int32 z, Int32 w,
                               bool attribute)
{
    LDD_RECORD_VARIABLE(MakeInts(VariableType::kInt4, x, y, z, w))
}

// Mac 0x235e90, Win 0x505590 (only ints[] stays uninitialised in the original record)
void OpenGLShader::SetVariable(const AString& name, const Matrix3D& m, bool transpose,
                               bool attribute)
{
    VariableType v;
    v.type = VariableType::kMatrix;
    v.matrix = m;
    v.ints[0] = v.ints[1] = v.ints[2] = v.ints[3] = 0;
    v.transpose = transpose;
    v.location = -1;
    LDD_RECORD_VARIABLE(v)
}

#undef LDD_RECORD_VARIABLE

} // namespace LEGO
