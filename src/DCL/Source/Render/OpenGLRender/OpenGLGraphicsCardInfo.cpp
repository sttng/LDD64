// LEGO Digital Designer — restored source
// TU 420 "OpenGLGraphicsCardInfo.cpp": Mac 0x399660-0x39982f, Win 0x4f1ee0-0x4f2070. See
// OpenGLGraphicsCardInfo.h. The static initialiser (Mac 0x3997f0) is only <iostream>.
#include "OpenGLGraphicsCardInfo.h"
#include "OpenGLRender.h"

namespace LEGO {

// Mac 0x399660, Win 0x4f1ee0 (GraphicsCardInfo ctor Win 0x5037b0)
OpenGLGraphicsCardInfo::OpenGLGraphicsCardInfo(OpenGLRender* render)
    : m_render(render)
{
}

// Mac 0x3997a0 (D1 -> GraphicsCardInfo D2 0x399530) / 0x3997b0, Win sdd 0x4f1f40
OpenGLGraphicsCardInfo::~OpenGLGraphicsCardInfo()
{
}

// Mac 0x399690, Win 0x4f1f60
String OpenGLGraphicsCardInfo::GetDescription() const
{
    String text = GraphicsCardInfo::GetDescription();
    String version;
    ToString(m_render->GetExtensions().m_glVersion, version);
    text << "OpenGL version: " << version << "\n";
    version.Clear();
    ToString(m_render->GetExtensions().m_glslVersion, version);
    text << "GLSL version: " << version << "\n";
    return text;
}

} // namespace LEGO
