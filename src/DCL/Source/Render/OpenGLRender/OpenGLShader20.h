// LEGO Digital Designer — restored source
// LEGO::OpenGLShader20 — GLSL program through the GL 2.0 entry points (factory's choice with GL
// 2.0 support). TU 201 (`__FILE__` Mac 0x24f7a0, Win 0x504876/0x504905): Mac 0x24f7a0-0x25095f
// (incl. map code 0x24fed0-0x250870), Win 0x5047f0-0x504c30 (+ ctor 0x4fac50, sdd 0x4facf0 in
// TU 400). Vtable Mac 0x7362c0 VTT (17 slots), Win 15. m_program Mac +0x48 / Win +0x54, Object
// +0x4c / +0x58 (sizeof 0x54 / 0x60).
#pragma once
#include "OpenGLShader.h"
#include "LEGO/String.h"
#include "Trace.h"   // the link-status trace of Link

namespace LEGO {

class OpenGLSharedShader;
class XMLDOMAString;

class OpenGLShader20 : public OpenGLShader {
public:
    // Inline; Mac 0x3873f0 (emitted in TU 400), Win 0x4fac50.
    explicit OpenGLShader20(OpenGLRender* render) : OpenGLShader(render), m_program(0) {}
    // MakeCurrent, glDeleteProgram. Mac D2 0x24f840, D1 0x24f8f0, D0 0x24f9b0; Win 0x5048a0.
    virtual ~OpenGLShader20();

    // glUseProgram, then uploads and clears the recorded uniforms and attributes.
    virtual void Bind(Render* render);                                // Mac 0x24faa0, Win 0x504950
    virtual void Unbind(Render* render);                              // Mac 0x24fdb0, Win 0x504b70 (glUseProgram(0))
    virtual Int32 GetUniformLocation(const AString& name);            // Mac 0x24fdd0, Win 0x504b80
    virtual Int32 GetAttributeLocation(const AString& name);          // Mac 0x24fe50, Win 0x504c30

    // Creates the program, attaches the compiled shaders (pixel shader optional) and links.
    // Always true: the link status is not checked here. `descriptor` (the parsed .sh file) is
    // passed by the factory but not used. (name inferred)
    bool Link(OpenGLSharedShader* vertexShader, OpenGLSharedShader* pixelShader,
              const XMLDOMAString* descriptor);                       // Mac 0x24f7a0, Win 0x5047f0

    UInt32 GetProgram() const { return m_program; }                   // (accessor, not in the binaries)

protected:
    UInt32 m_program;   // Mac +0x48, Win +0x54 (GLuint)
};

} // namespace LEGO
