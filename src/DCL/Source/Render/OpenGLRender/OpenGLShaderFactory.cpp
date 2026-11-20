// LEGO Digital Designer — restored source
// OpenGLShaderFactory.cpp (TU 400; file name inferred). See OpenGLShaderFactory.h.
#include "OpenGLShaderFactory.h"
#include "OpenGLShader20.h"
#include "OpenGLShaderARB.h"
#include "OpenGLRender.h"
#include "OpenGLExtensions.h"
#include "OpenGLFactories.h"
#include "Shader.h"
#include "ReadFile.h"
#include "XMLDOMAttribute.h"
#include "Trace.h"

namespace LEGO {

namespace {

bool EqualsAString(const AString& a, const AString& b)
{
    return a.GetLength() == b.GetLength() &&
           memcmp(a.GetBuffer(), b.GetBuffer(), a.GetLength()) == 0;
}

// Mac 0x3892d0 (find, inlined on Win): first occurrence of `what` at or after `start`, -1.
Int32 FindAString(const AString& s, const AString& what, UInt32 start)
{
    UInt32 n = s.GetLength(), m = what.GetLength();
    if (m == 0 || n < m)
        return -1;
    for (UInt32 i = start; i + m <= n; ++i)
        if (memcmp(s.GetBuffer() + i, what.GetBuffer(), m) == 0)
            return (Int32)i;
    return -1;
}

// Mac 0x387b80: splits at `separator`; empty pieces between separators are kept, an empty last
// piece is not.
void SplitAString(const AString& s, const AString& separator, BasicArray<AString>& out)
{
    out.Clear();
    UInt32 start = 0;
    Int32 pos = FindAString(s, separator, 0);
    while (pos != -1) {
        out.PushBack(AString(s.GetBuffer() + start, (UInt32)pos - start));
        start = (UInt32)pos + separator.GetLength();
        pos = FindAString(s, separator, start);
    }
    if (start < s.GetLength())
        out.PushBack(AString(s.GetBuffer() + start, s.GetLength() - start));
}

// The path is "empty" without a directory and without a name (inlined test of Path).
bool IsEmptyPath(const Path& path)
{
    return path.GetParentDirectory() == 0 && path.GetName().IsEmpty();
}

} // namespace

// ---------------------------------------------------------------------------------------------
// OpenGLSharedShader
// ---------------------------------------------------------------------------------------------

// Mac D1 0x387eb0, D0 0x387f10; Win 0x4fa970, sdd 0x4fa9f0
OpenGLSharedShader20::~OpenGLSharedShader20()
{
    m_render->MakeCurrent();
    glDeleteShader(m_handle);
}

// Mac D1 0x387f80, D0 0x387fe0; Win 0x4faa80, sdd 0x4fab00
OpenGLSharedShaderARB::~OpenGLSharedShaderARB()
{
    m_render->MakeCurrent();
    glDeleteObjectARB(ToGLhandleARB(m_handle));
}

// ---------------------------------------------------------------------------------------------
// OpenGLShaderFactory
// ---------------------------------------------------------------------------------------------

// Mac 0x384360, Win 0x4fadf0
OpenGLShaderFactory::OpenGLShaderFactory(OpenGLRender* render)
    : m_render(render)
{
}

// Mac 0x387a10 (D1 0x387b30, D0 0x387b40), Win 0x4fc400 (sdd 0x4fae70)
OpenGLShaderFactory::~OpenGLShaderFactory()
{
}

// Mac 0x386840, Win 0x4fc9f0
bool OpenGLShaderFactory::ReadRequirement(const XMLDOMNodeAString& node, const AString& tag,
                                          const AString& attributes, BasicArray<UInt32>& values)
{
    values.Clear();
    if (!EqualsAString(node.GetName(), tag))
        return false;

    BasicArray<AString> names;
    SplitAString(attributes, AString(","), names);
    for (UInt32 i = 0; i < names.GetSize(); ++i) {
        UInt32 value = 0;
        if (!GetXMLAttribute<UInt32>(node, names[i], value)) {
            // NOTE: the message is built but never traced (Mac 0x386c11-0x386c6e, Win the same).
            String message("OpenGLShaderFactory: Unable to get attribute '");
            message += names[i];
            message << String("' for tag '");
            message += tag;
            message << "'";
            values.Clear();
            return false;
        }
        values.PushBack(value);
    }
    return true;
}

// Mac 0x386dd0, Win 0x4fcbe0
bool OpenGLShaderFactory::CheckRequirement(const String& vertexShader, const String& pixelShader,
                                           const String& tag, const String& attribute,
                                           UInt32 supported, UInt32 required)
{
    if (supported < required) {
        String message("OpenGLShaderFactory: Unable to create shader pair ");
        message << vertexShader;
        message << ",";
        message << pixelShader;
        message << String(". Requirement for '");
        message << tag;
        message << String("' attribute '");
        message << attribute;
        message << String("' is ");
        message << required;
        message << String(". Supported value is ");
        message << supported;
        message << ".";
        TraceVerbose(message, true);
    }
    return supported >= required;
}

// Mac 0x386fd0, Win 0x4fc470
OpenGLSharedShader20* OpenGLShaderFactory::CompileShader20(const Path& path, UInt32 type,
                                                           bool recompile)
{
    if (!recompile) {
        SharedShaderMap::Iterator it = m_sharedShaders.Find(path);
        if (it != m_sharedShaders.End())
            return it->second ? dynamic_cast<OpenGLSharedShader20*>(it->second.Get()) : 0;
    }

    AString source;
    if (!ReadFile(path, source))
        return 0;

    GLuint shader = glCreateShader(type);
    const GLchar* text = source.GetBuffer();
    GLint length = (GLint)source.GetLength();
    glShaderSource(shader, 1, &text, &length);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        // NOTE: the shader object is not deleted.
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        char* log = new char[logLength < -1 ? 0xffffffffu : (UInt32)logLength];
        GLsizei written;
        glGetShaderInfoLog(shader, logLength, &written, log);
        String head("SHADER ERROR INFO FOR FILE: ");
        String message(head << String(path.GetName()) << String(":\n\n"));
        TraceVerbose(message << String(log), true);
        delete[] log;
        return 0;
    }

    OpenGLSharedShader* shared = new OpenGLSharedShader20(m_render, shader);
    RefPointer<OpenGLSharedShader>& slot = m_sharedShaders[path];
    slot = shared;
    return slot ? dynamic_cast<OpenGLSharedShader20*>(slot.Get()) : 0;
}

// Mac 0x3874f0, Win 0x4fc730 (the same through ARB_shader_objects)
OpenGLSharedShaderARB* OpenGLShaderFactory::CompileShaderARB(const Path& path, UInt32 type,
                                                             bool recompile)
{
    if (!recompile) {
        SharedShaderMap::Iterator it = m_sharedShaders.Find(path);
        if (it != m_sharedShaders.End())
            return it->second ? dynamic_cast<OpenGLSharedShaderARB*>(it->second.Get()) : 0;
    }

    AString source;
    if (!ReadFile(path, source))
        return 0;

    GLhandleARB shader = glCreateShaderObjectARB(type);
    const GLcharARB* text = source.GetBuffer();
    GLint length = (GLint)source.GetLength();
    glShaderSourceARB(shader, 1, &text, &length);
    glCompileShaderARB(shader);
    GLint compiled;
    glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
    if (!compiled) {
        // NOTE: the shader object is not deleted.
        GLint logLength;
        glGetObjectParameterivARB(shader, GL_OBJECT_INFO_LOG_LENGTH_ARB, &logLength);
        char* log = new char[logLength < -1 ? 0xffffffffu : (UInt32)logLength];
        GLsizei written;
        glGetInfoLogARB(shader, logLength, &written, log);
        String head("SHADER ERROR INFO FOR FILE: ");
        String message(head << String(path.GetName()) << String(":\n\n"));
        TraceVerbose(message << String(log), true);
        delete[] log;
        return 0;
    }

    OpenGLSharedShader* shared = new OpenGLSharedShaderARB(m_render, FromGLhandleARB(shader));
    RefPointer<OpenGLSharedShader>& slot = m_sharedShaders[path];
    slot = shared;
    return slot ? dynamic_cast<OpenGLSharedShaderARB*>(slot.Get()) : 0;
}

// Mac 0x3843f0, Win 0x4fae90
Object* OpenGLShaderFactory::CreateObject(const ObjectFactoryArgs& args)
{
    if (!m_render->SupportsShaders())
        return 0;
    const Shader::LoadArgs* load = dynamic_cast<const Shader::LoadArgs*>(&args);
    if (!load)
        return 0;
    // Only the low byte of LoadArgs +0x1c is read (Mac 0x384c0d `movzx … byte`).
    bool recompile = (UInt8)load->field1C != 0;

    Path vertexPath;
    Path pixelPath;
    DescriptorMap::Iterator it = m_descriptors.Find(load->path);
    if (it == m_descriptors.End()) {
        RefPointer<XMLDOMAString> xml = new XMLDOMAString();
        if (!xml->Parse(load->path, 0))
            return 0;
        AString vertexName;
        xml->GetAttribute(AString("VertexShader"), vertexName);
        if (vertexName.IsEmpty())
            return 0;
        AString pixelName;
        xml->GetAttribute(AString("PixelShader"), pixelName);
        vertexPath = Path(String(vertexName), load->path.GetParentDirectory());
        if (!pixelName.IsEmpty())
            pixelPath = Path(String(pixelName), load->path.GetParentDirectory());
        m_descriptors[load->path] = Descriptor(vertexPath, pixelPath, xml);
        it = m_descriptors.Find(load->path);
    } else {
        vertexPath = it->second.first;
        pixelPath = it->second.second;
    }

    m_render->MakeCurrent();
    const OpenGLExtensions& extensions = m_render->GetExtensions();

    if (!extensions.m_shaders20) {
        // ARB_shader_objects: no requirement check.
        if (!extensions.m_shaderObjectsARB)
            return 0;
        OpenGLSharedShaderARB* vertexShader =
            CompileShaderARB(vertexPath, GL_VERTEX_SHADER_ARB, recompile);
        if (!vertexShader)
            return 0;
        OpenGLSharedShaderARB* pixelShader = 0;
        if (!IsEmptyPath(pixelPath)) {
            pixelShader = CompileShaderARB(pixelPath, GL_FRAGMENT_SHADER_ARB, recompile);
            if (!pixelShader)
                return 0;
        }
        OpenGLShaderARB* shader = new OpenGLShaderARB(m_render);
        Object* object = shader;
        if (!shader->Link(vertexShader, pixelShader)) {
            object->Release();
            return 0;
        }
        return object;
    }

    // GL 2.0: every <Requirements> child must be known and met.
    XMLDOMNodeAString* requirements = it->second.third->FindChild(AString("Requirements"));
    if (requirements) {
        const XMLDOMNodeAString::NodeArray& children = requirements->GetChildren();
        for (UInt32 i = 0; i < children.GetSize(); ++i) {
            const XMLDOMNodeAString& node = *children[i];
            BasicArray<UInt32> values;
            const String vs(vertexPath.GetName());
            const String ps(pixelPath.GetName());
            bool met;
            if (ReadRequirement(node, AString("NumberOfVertexShaderTextureUnits"), AString("min"), values)) {
                met = CheckRequirement(vs, ps, String("NumberOfVertexShaderTextureUnits"), String("min"),
                                       m_render->GetMaxVertexTextureImageUnits(), values[0]);
            } else if (ReadRequirement(node, AString("NumberOfPixelShaderTextureUnits"), AString("min"), values)) {
                met = CheckRequirement(vs, ps, String("NumberOfPixelShaderTextureUnits"), String("min"),
                                       m_render->GetMaxTextureImageUnits(), values[0]);
            } else if (ReadRequirement(node, AString("NumberOfCombinedShaderTextureUnits"), AString("min"), values)) {
                met = CheckRequirement(vs, ps, String("NumberOfCombinedShaderTextureUnits"), String("min"),
                                       m_render->GetMaxCombinedTextureImageUnits(), values[0]);
            } else if (ReadRequirement(node, AString("NumberOfVertexShaderUniformFloats"), AString("min"), values)) {
                met = CheckRequirement(vs, ps, String("NumberOfVertexShaderUniformFloats"), String("min"),
                                       m_render->GetMaxVertexUniformComponents(), values[0]);
            } else if (ReadRequirement(node, AString("NumberOfPixelShaderUniformFloats"), AString("min"), values)) {
                met = CheckRequirement(vs, ps, String("NumberOfPixelShaderUniformFloats"), String("min"),
                                       m_render->GetMaxFragmentUniformComponents(), values[0]);
            } else if (ReadRequirement(node, AString("NumberOfVaryingFloats"), AString("min"), values)) {
                met = CheckRequirement(vs, ps, String("NumberOfVaryingFloats"), String("min"),
                                       m_render->GetMaxVaryingFloats(), values[0]);
            } else if (ReadRequirement(node, AString("GLSLVersion"), AString("major,minor"), values)) {
                met = CheckRequirement(vs, ps, String("GLSLVersion"), String("major"),
                                       extensions.m_glslVersion.m_major, values[0]);
                // NOTE: a newer major version passes without looking at the minor one.
                if (met && extensions.m_glslVersion.m_major == values[0])
                    met = CheckRequirement(vs, ps, String("GLSLVersion"), String("minor"),
                                           extensions.m_glslVersion.m_minor, values[1]);
            } else if (ReadRequirement(node, AString("RenderTarget"), AString("supported"), values)) {
                met = CheckRequirement(vs, ps, String("RenderTarget"), String("supported"),
                                       extensions.m_framebufferObject ? 1 : 0, values[0]);
            } else {
                // NOTE: built but not traced (Mac 0x385ef0, Win 0x4fbffd); the shader is refused.
                String message("OpenGLShaderFactory: Unknown requirements tag: ");
                message += node.GetName();
                met = false;
            }
            if (!met)
                return 0;
        }
    }

    OpenGLSharedShader20* vertexShader = CompileShader20(vertexPath, GL_VERTEX_SHADER, recompile);
    if (!vertexShader)
        return 0;
    OpenGLSharedShader20* pixelShader = 0;
    if (!IsEmptyPath(pixelPath)) {
        pixelShader = CompileShader20(pixelPath, GL_FRAGMENT_SHADER, recompile);
        if (!pixelShader)
            return 0;
    }
    OpenGLShader20* shader = new OpenGLShader20(m_render);
    Object* object = shader;
    if (!shader->Link(vertexShader, pixelShader, it->second.third)) {
        object->Release();
        return 0;
    }
    return object;
}

// OpenGLFactories.h hook: `new OpenGLShaderFactory(render)` in OpenGLRender::Initialize
// (Mac 0xf2f00: operator new(0x4c) + 0x384360).
SpecificObjectFactory* CreateOpenGLShaderFactory(OpenGLRender* render)
{
    return new OpenGLShaderFactory(render);
}

} // namespace LEGO
