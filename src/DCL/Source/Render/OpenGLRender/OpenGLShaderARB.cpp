// LEGO Digital Designer — restored source
// OpenGLShaderARB.cpp (TU 202; `__FILE__` "…/DCL/Source/Render/OpenGLRender/OpenGLShaderARB.cpp").
#include "OpenGLShaderARB.h"
#include "OpenGLShaderFactory.h"
#include "OpenGLRender.h"
#include "OpenGLExtensions.h"
#include "OpenGLWindow.h"
#include "LEGO/String.h"
#include "Trace.h"

namespace LEGO {

// Mac D2 0x250b10 (D1 0x250bc0, D0 0x250c80, thunks 0x250c20/0x250cf0), Win 0x503c60
OpenGLShaderARB::~OpenGLShaderARB()
{
    LDD_GL_TRY
    m_render->MakeCurrent();
    glDeleteObjectARB(ToGLhandleARB(m_program));
    LDD_GL_CATCH
}

// Mac 0x250960, Win 0x503af0
bool OpenGLShaderARB::Link(OpenGLSharedShader* vertexShader, OpenGLSharedShader* pixelShader)
{
    LDD_GL_TRY
    m_program = FromGLhandleARB(glCreateProgramObjectARB());
    glAttachObjectARB(ToGLhandleARB(m_program), ToGLhandleARB(vertexShader->GetHandle()));
    if (pixelShader)
        glAttachObjectARB(ToGLhandleARB(m_program), ToGLhandleARB(pixelShader->GetHandle()));
    glLinkProgramARB(ToGLhandleARB(m_program));
    glValidateProgramARB(ToGLhandleARB(m_program));

    GLint linked;
    glGetObjectParameterivARB(ToGLhandleARB(m_program), GL_OBJECT_LINK_STATUS_ARB, &linked);
    if (linked)
        return true;

    GLint length;
    glGetObjectParameterivARB(ToGLhandleARB(m_program), GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
    char* log = new char[length < -1 ? 0xffffffffu : (UInt32)length];
    GLsizei written;
    glGetInfoLogARB(ToGLhandleARB(m_program), length, &written, log);
    String message("PROGRAM ERROR INFO:\n\n");
    TraceVerbose(message << String(log), true);
    delete[] log;
    return false;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0x250d70, Win 0x503d10
void OpenGLShaderARB::Bind(Render* render)
{
    (void)render;
    glUseProgramObjectARB(ToGLhandleARB(m_program));

    for (VariableType* v = m_uniforms.Begin(); v != m_uniforms.End(); ++v) {
        const Real32* f = v->matrix;
        switch (v->type) {
        case VariableType::kFloat1: glUniform1fARB(v->location, f[0]); break;
        case VariableType::kFloat2: glUniform2fARB(v->location, f[0], f[1]); break;
        case VariableType::kFloat3: glUniform3fARB(v->location, f[0], f[1], f[2]); break;
        case VariableType::kFloat4: glUniform4fARB(v->location, f[0], f[1], f[2], f[3]); break;
        case VariableType::kInt1:   glUniform1iARB(v->location, v->ints[0]); break;
        case VariableType::kInt2:   glUniform2iARB(v->location, v->ints[0], v->ints[1]); break;
        case VariableType::kInt3:   glUniform3iARB(v->location, v->ints[0], v->ints[1], v->ints[2]); break;
        case VariableType::kInt4:   glUniform4iARB(v->location, v->ints[0], v->ints[1], v->ints[2], v->ints[3]); break;
        // NOTE: count 0 in the original (Mac 0x250ea4) — matrices never reach the ARB program.
        case VariableType::kMatrix: glUniformMatrix4fvARB(v->location, 0, v->transpose, f); break;
        }
    }
    m_uniforms.Clear();
}

// Mac 0x250f40, Win 0x503e70
void OpenGLShaderARB::Unbind(Render* render)
{
    (void)render;
    glUseProgramObjectARB(0);
}

// Mac 0x250f60, Win 0x503e80
Int32 OpenGLShaderARB::GetUniformLocation(const AString& name)
{
    LocationMap::Iterator it = m_locations.Find(name);
    if (it != m_locations.End())
        return it->second;
    Int32 location = glGetUniformLocationARB(ToGLhandleARB(m_program), name.GetBuffer());
    if (location == -1)
        return -1;
    m_locations[name] = location;
    return location;
}

} // namespace LEGO
