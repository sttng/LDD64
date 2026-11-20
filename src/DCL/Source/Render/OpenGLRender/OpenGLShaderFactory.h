// LEGO Digital Designer — restored source
// LEGO::OpenGLShaderFactory — creates OpenGLShader20 / ARB programs from `.sh` descriptors
// (<GLSL VertexShader= PixelShader=>, optional <Requirements> checked against the render's
// limits); compiled shaders (OpenGLSharedShader20 / ARB) cached per source path, descriptors
// per `.sh` path. TU 400 (file name inferred): Mac 0x384330-0x3893ff, Win 0x4fa780-0x4fd5b0
// (+ map COMDATs to 0x4fe4xx). Registered by OpenGLRender::Initialize (Mac 0xf2f00).
//
// Layouts (Mac; Win in brackets):
//   OpenGLSharedShader   {vptr, refcount, OpenGLRender* m_render +0x08, UInt32 m_handle +0x0c}
//                        sizeof 0x10 [same]; vtables Mac 0x778848 (20) / 0x778884 (ARB).
//   OpenGLShaderFactory  SpecificObjectFactory (0x10) + OpenGLRender* m_render +0x10,
//                        BasicMap<Path, Triple<Path, Path, RefPointer<XMLDOMAString>>> +0x14,
//                        BasicMap<Path, RefPointer<OpenGLSharedShader>> +0x30; sizeof 0x4c.
// Names: classes from RTTI; members and methods inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BasicString.h"
#include "LEGO/String.h"
#include "Path.h"
#include "XMLDOMNode.h"

namespace LEGO {

class OpenGLRender;

class OpenGLSharedShader : public Object {
public:
    // Win 0x4fa7e0 (Mac inline). The render pointer is not reference counted.
    OpenGLSharedShader(OpenGLRender* render, UInt32 handle) : m_render(render), m_handle(handle) {}
    virtual ~OpenGLSharedShader() {}                                  // Win 0x4fa840/0x4fa890

    UInt32 GetHandle() const { return m_handle; }                     // (inlined: reads +0x0c)

protected:
    OpenGLRender* m_render;   // +0x08
    UInt32        m_handle;   // +0x0c GLuint / GLhandleARB
};

class OpenGLSharedShader20 : public OpenGLSharedShader {
public:
    OpenGLSharedShader20(OpenGLRender* render, UInt32 handle)        // Win 0x4fa900
        : OpenGLSharedShader(render, handle) {}
    // MakeCurrent, glDeleteShader.                                   Mac 0x387eb0/0x387f10, Win 0x4fa970/0x4fa9f0
    virtual ~OpenGLSharedShader20();
};

class OpenGLSharedShaderARB : public OpenGLSharedShader {
public:
    OpenGLSharedShaderARB(OpenGLRender* render, UInt32 handle)       // Win 0x4faa10
        : OpenGLSharedShader(render, handle) {}
    // MakeCurrent, glDeleteObjectARB.                                Mac 0x387f80/0x387fe0, Win 0x4faa80/0x4fab00
    virtual ~OpenGLSharedShaderARB();
};

class OpenGLShaderFactory : public SpecificObjectFactory {
public:
    explicit OpenGLShaderFactory(OpenGLRender* render);              // Mac 0x384360, Win 0x4fadf0
    virtual ~OpenGLShaderFactory();                                   // Mac 0x387a10 (D1 0x387b30, D0 0x387b40), Win 0x4fc400/0x4fae70

    // Shader::LoadArgs -> OpenGLShader20 (GL 2.0, requirements checked) or OpenGLShaderARB
    // (ARB_shader_objects, requirements not checked); null when shaders are not supported,
    // the descriptor or a source cannot be read, compiling fails or a requirement is not met.
    virtual Object* CreateObject(const ObjectFactoryArgs& args);      // Mac 0x3843f0, Win 0x4fae90

    // Reads the comma separated `attributes` of `node` as UInt32 into `values` when the node is
    // named `tag`; false otherwise (or when one attribute is missing / not a number).
    static bool ReadRequirement(const XMLDOMNodeAString& node, const AString& tag,
                                const AString& attributes, BasicArray<UInt32>& values); // Mac 0x386840, Win 0x4fc9f0
    // `supported >= required`; otherwise traces (verbose) why the shader pair is rejected.
    static bool CheckRequirement(const String& vertexShader, const String& pixelShader,
                                 const String& tag, const String& attribute,
                                 UInt32 supported, UInt32 required);          // Mac 0x386dd0, Win 0x4fcbe0

protected:
    // Compiles `path` as a shader of `type` (GL_VERTEX_SHADER / GL_FRAGMENT_SHADER). With
    // `recompile` false a cached object of the same path is returned. Compile errors are traced
    // (verbose) as "SHADER ERROR INFO FOR FILE: <name>:\n\n<log>".
    OpenGLSharedShader20* CompileShader20(const Path& path, UInt32 type, bool recompile);   // Mac 0x386fd0, Win 0x4fc470
    OpenGLSharedShaderARB* CompileShaderARB(const Path& path, UInt32 type, bool recompile); // Mac 0x3874f0, Win 0x4fc730

    typedef Triple<Path, Path, RefPointer<XMLDOMAString> > Descriptor;  // vertex, pixel, parsed .sh
    typedef BasicMap<Path, Descriptor>                       DescriptorMap;
    typedef BasicMap<Path, RefPointer<OpenGLSharedShader> >  SharedShaderMap;

    OpenGLRender*   m_render;          // Mac +0x10
    DescriptorMap   m_descriptors;     // Mac +0x14
    SharedShaderMap m_sharedShaders;   // Mac +0x30

private:
    OpenGLShaderFactory(const OpenGLShaderFactory&);
    OpenGLShaderFactory& operator=(const OpenGLShaderFactory&);
};

} // namespace LEGO
