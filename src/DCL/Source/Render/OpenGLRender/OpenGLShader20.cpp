// LEGO Digital Designer — restored source
// OpenGLShader20.cpp (TU 201; `__FILE__` "…/DCL/Source/Render/OpenGLRender/OpenGLShader20.cpp").
#include "OpenGLShader20.h"
#include "OpenGLShaderFactory.h"
#include "OpenGLRender.h"
#include "OpenGLExtensions.h"
#include "OpenGLWindow.h"

namespace LEGO {

// Mac D2 0x24f840 (D1 0x24f8f0, D0 0x24f9b0, thunks 0x24f950/0x24fa20), Win 0x5048a0
OpenGLShader20::~OpenGLShader20()
{
    LDD_GL_TRY
    m_render->MakeCurrent();
    glDeleteProgram(m_program);
    LDD_GL_CATCH
}

// Mac 0x24f7a0, Win 0x5047f0 (EDX = this, ECX = pixel shader, stack: vertex shader)
bool OpenGLShader20::Link(OpenGLSharedShader* vertexShader, OpenGLSharedShader* pixelShader,
                          const XMLDOMAString* descriptor)
{
    (void)descriptor;
    LDD_GL_TRY
    m_program = glCreateProgram();
    glAttachShader(m_program, vertexShader->GetHandle());
    if (pixelShader)
        glAttachShader(m_program, pixelShader->GetHandle());
    glLinkProgram(m_program);
    LDD_GL_CATCH
    // NOTE: the original does not look at the link status (Mac 0x24f7a0 returns 1 after
    // glLinkProgram), so a program that does not link is used: nothing drawn, GL errors at every
    // glUseProgram / glUniform*; fixed: GL_LINK_STATUS is checked as OpenGLShaderARB::Link does,
    // and a failure is traced and returned, so the factory drops the shader (the path of a
    // failed compile) and the caller draws without it (M25).
    LDD_GL_TRY
    GLint linked = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint length = 0;
        glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &length);
        if (length < 1)
            length = 1;
        char* log = new char[(UInt32)length];
        log[0] = 0;
        GLsizei written = 0;
        glGetProgramInfoLog(m_program, length, &written, log);
        String message("PROGRAM ERROR INFO:\n\n");
        TraceVerbose(message << String(log), true);
        delete[] log;
        return false;
    }
    LDD_GL_CATCH
    return true;
}

// Mac 0x24faa0, Win 0x504950
void OpenGLShader20::Bind(Render* render)
{
    (void)render;
    glUseProgram(m_program);

    for (VariableType* v = m_uniforms.Begin(); v != m_uniforms.End(); ++v) {
        const Real32* f = v->matrix;
        switch (v->type) {
        case VariableType::kFloat1: glUniform1f(v->location, f[0]); break;
        case VariableType::kFloat2: glUniform2f(v->location, f[0], f[1]); break;
        case VariableType::kFloat3: glUniform3f(v->location, f[0], f[1], f[2]); break;
        case VariableType::kFloat4: glUniform4f(v->location, f[0], f[1], f[2], f[3]); break;
        case VariableType::kInt1:   glUniform1i(v->location, v->ints[0]); break;
        case VariableType::kInt2:   glUniform2i(v->location, v->ints[0], v->ints[1]); break;
        case VariableType::kInt3:   glUniform3i(v->location, v->ints[0], v->ints[1], v->ints[2]); break;
        case VariableType::kInt4:   glUniform4i(v->location, v->ints[0], v->ints[1], v->ints[2], v->ints[3]); break;
        case VariableType::kMatrix: glUniformMatrix4fv(v->location, 1, v->transpose, f); break;
        }
    }
    m_uniforms.Clear();

    // Only float values reach generic vertex attributes; other types are dropped.
    for (VariableType* v = m_attributes.Begin(); v != m_attributes.End(); ++v) {
        const Real32* f = v->matrix;
        switch (v->type) {
        case VariableType::kFloat1: glVertexAttrib1f(v->location, f[0]); break;
        case VariableType::kFloat2: glVertexAttrib2f(v->location, f[0], f[1]); break;
        case VariableType::kFloat3: glVertexAttrib3f(v->location, f[0], f[1], f[2]); break;
        case VariableType::kFloat4: glVertexAttrib4f(v->location, f[0], f[1], f[2], f[3]); break;
        }
    }
    m_attributes.Clear();
}

// Mac 0x24fdb0, Win 0x504b70
void OpenGLShader20::Unbind(Render* render)
{
    (void)render;
    glUseProgram(0);
}

// Mac 0x24fdd0, Win 0x504b80
Int32 OpenGLShader20::GetUniformLocation(const AString& name)
{
    LocationMap::Iterator it = m_locations.Find(name);
    if (it != m_locations.End())
        return it->second;
    Int32 location = glGetUniformLocation(m_program, name.GetBuffer());
    if (location == -1)
        return -1;
    m_locations[name] = location;
    return location;
}

// Mac 0x24fe50, Win 0x504c30 (same cache as the uniforms)
Int32 OpenGLShader20::GetAttributeLocation(const AString& name)
{
    LocationMap::Iterator it = m_locations.Find(name);
    if (it != m_locations.End())
        return it->second;
    Int32 location = glGetAttribLocation(m_program, name.GetBuffer());
    if (location == -1)
        return -1;
    m_locations[name] = location;
    return location;
}

} // namespace LEGO
