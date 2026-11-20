// LEGO Digital Designer — restored source
// LEGO::OpenGLShader — common part of the GLSL programs (OpenGLShader20 / OpenGLShaderARB).
// SetVariable only records value + location; the subclass's Bind uploads the recorded uniforms /
// attributes after glUseProgram and empties the lists. Names from RTTI, members inferred.
// TU 180 (`__FILE__` Mac 0x2350a0.., Win 0x504f14..): Mac 0x234dd0-0x2366bf, Win 0x504d70-0x5056ff
// + 0x4fac00/0x4fac10 (ICF'd IsOpaque / GetAttributeLocation) + COMDATs 0x4fce00, 0x4fd0d0,
// 0x4ff6f0. Inline ctor emitted in TU 400 (Mac 0x3873f0 / 0x387910; Win 0x4fab20 / 0x504d70).
// Vtable Mac 0x72f25c VTT (Shader + 15 GetUniformLocation, 16 GetAttributeLocation; Win 13/14).
// Layout Mac (sizeof 0x48 + Object 8):
//   +0x00 vptr  +0x04 bool m_opaque (Shader)  +0x08 RefPointer<OpenGLRender> m_render
//   +0x0c BasicMap<AString, Int32> m_locations (vptr +0x0c, std::map +0x10..+0x28)
//   +0x28 BasicArray<VariableType> m_uniforms  +0x38 BasicArray<VariableType> m_attributes
//   +0x48 virtual base Object (subclasses put their program handle at +0x48, Object at +0x4c)
// Win: vptr, vbptr +4, bool +8, render +0x0c, map +0x10, arrays +0x34/+0x44, program +0x54,
// Object +0x58 (OpenGLShader20/ARB sizeof 0x60).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BasicString.h"
#include "Geometry.h"
#include "Shader.h"

namespace LEGO {

class OpenGLRender;

class OpenGLShader : public Shader {
public:
    // One recorded SetVariable call, 0x5c bytes. (field names inferred)
    struct VariableType {
        enum Type {
            kFloat1 = 0, kFloat2 = 1, kFloat3 = 2, kFloat4 = 3,
            kInt1 = 4, kInt2 = 5, kInt3 = 6, kInt4 = 7,
            kMatrix = 8
        };
        UInt32   type;        // +0x00 Type
        Matrix3D matrix;      // +0x04 float values in elements 0-3 (rest 0); identity for ints
        Int32    ints[4];     // +0x44 integer values
        bool     transpose;   // +0x54 kMatrix only
        Int32    location;    // +0x58 uniform location / attribute index
    };

    // Inline; emitted inside the OpenGLShader20/ARB constructors (Mac 0x3873f0/0x387910,
    // Win 0x4fab20).
    explicit OpenGLShader(OpenGLRender* render);
    virtual ~OpenGLShader();                                          // Mac D2 0x234dd0, D1 0x234ef0, D0 0x234fb0 (Win part of 0x5048a0/0x503c60)

    virtual bool IsOpaque() const;                                    // Mac 0x236570, Win 0x4fac00
    // Each overload records {type, values, location}; uniforms use GetUniformLocation even when
    // it fails (-1), attributes are dropped when GetAttributeLocation returns -1.
    virtual void SetVariable(const AString& name, Real32 x, bool attribute);                     // Mac 0x2350a0, Win 0x504e50
    virtual void SetVariable(const AString& name, Real32 x, Real32 y, bool attribute);           // Mac 0x2353f0, Win 0x504f30
    virtual void SetVariable(const AString& name, Real32 x, Real32 y, Real32 z, bool attribute); // Mac 0x2355f0, Win 0x505020
    virtual void SetVariable(const AString& name, Real32 x, Real32 y, Real32 z, Real32 w,
                             bool attribute);                                                    // Mac 0x235800, Win 0x505120
    virtual void SetVariable(const AString& name, Int32 x, bool attribute);                      // Mac 0x235a10, Win 0x505220
    virtual void SetVariable(const AString& name, Int32 x, Int32 y, bool attribute);             // Mac 0x235b30, Win 0x5052f0
    virtual void SetVariable(const AString& name, Int32 x, Int32 y, Int32 z, bool attribute);    // Mac 0x235c50, Win 0x5053d0
    virtual void SetVariable(const AString& name, Int32 x, Int32 y, Int32 z, Int32 w,
                             bool attribute);                                                    // Mac 0x235d70, Win 0x5054b0
    virtual void SetVariable(const AString& name, const Matrix3D& m, bool transpose,
                             bool attribute);                                                    // Mac 0x235e90, Win 0x505590

    // Program uniform location, cached by name (-1 when unknown, not cached). (names inferred)
    virtual Int32 GetUniformLocation(const AString& name) = 0;        // Mac 15, Win 13
    // Generic vertex attribute index; -1 here (OpenGLShaderARB keeps this).
    virtual Int32 GetAttributeLocation(const AString& name);          // Mac 16 0x236580, Win 14 0x4fac10

    // Recorded values, for tests / diagnostics (not in the binaries).
    const BasicArray<VariableType>& GetPendingUniforms() const { return m_uniforms; }
    const BasicArray<VariableType>& GetPendingAttributes() const { return m_attributes; }

protected:
    // Mac 0x2352a0 (BasicArray<VariableType>::PushBack, emitted out of line).
    void Record(bool attribute, Int32 location, const VariableType& value);

    // Shared by the subclasses' GetUniformLocation/GetAttributeLocation (Mac 0x24fed0 find,
    // 0x250020 operator[]).
    typedef BasicMap<AString, Int32> LocationMap;

    RefPointer<OpenGLRender>  m_render;      // Mac +0x08, Win +0x0c
    LocationMap               m_locations;   // Mac +0x0c, Win +0x10 (uniforms and attributes share it)
    BasicArray<VariableType>  m_uniforms;    // Mac +0x28, Win +0x34
    BasicArray<VariableType>  m_attributes;  // Mac +0x38, Win +0x44

private:
    OpenGLShader(const OpenGLShader&);
    OpenGLShader& operator=(const OpenGLShader&);
};

} // namespace LEGO
