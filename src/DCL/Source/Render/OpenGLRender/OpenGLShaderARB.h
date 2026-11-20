// LEGO Digital Designer — restored source
// LEGO::OpenGLShaderARB — GLSL program through ARB_shader_objects (fallback without GL 2.0); no
// generic vertex attributes (GetAttributeLocation returns -1). TU 202 (`__FILE__` Win 0x503cc5):
// Mac 0x250960-0x25101f, Win 0x503af0-0x503e80 (+ ctor 0x4fad20, sdd 0x4fadc0 in TU 400).
// Vtable Mac 0x736470 VTT. m_program Mac +0x48, Win +0x54.
#pragma once
#include "OpenGLShader.h"

namespace LEGO {

class OpenGLSharedShader;

class OpenGLShaderARB : public OpenGLShader {
public:
    // Inline; Mac 0x387910 (emitted in TU 400), Win 0x4fad20.
    explicit OpenGLShaderARB(OpenGLRender* render) : OpenGLShader(render), m_program(0) {}
    // MakeCurrent, glDeleteObjectARB. Mac D2 0x250b10, D1 0x250bc0, D0 0x250c80; Win 0x503c60.
    virtual ~OpenGLShaderARB();

    // glUseProgramObjectARB, then uploads and clears the recorded uniforms (attributes are
    // never recorded: GetAttributeLocation is -1).
    virtual void Bind(Render* render);                                // Mac 0x250d70, Win 0x503d10
    virtual void Unbind(Render* render);                              // Mac 0x250f40, Win 0x503e70
    virtual Int32 GetUniformLocation(const AString& name);            // Mac 0x250f60, Win 0x503e80

    // Creates, attaches, links and validates; false (with "PROGRAM ERROR INFO" in the verbose
    // trace) when GL_OBJECT_LINK_STATUS_ARB is 0. (name inferred)
    bool Link(OpenGLSharedShader* vertexShader, OpenGLSharedShader* pixelShader); // Mac 0x250960, Win 0x503af0

    UInt32 GetProgram() const { return m_program; }                   // (accessor, not in the binaries)

protected:
    UInt32 m_program;   // Mac +0x48, Win +0x54 (GLhandleARB)
};

} // namespace LEGO
