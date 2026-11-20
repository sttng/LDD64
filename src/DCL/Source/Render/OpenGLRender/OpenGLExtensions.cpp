// LEGO Digital Designer — restored source
// OpenGLExtensions.cpp (TU 033, name inferred) — see OpenGLExtensions.h. Mac 0xf2010
// Initialize, 0xf2730 BasicString<char>::Find (weak), 0xf2820 ctor; Win 0x4f20c0
// GetExtensionProc, 0x4f20e0 ctor/Initialize, 0x4f3670 AppendExtensions.
#include "OpenGLExtensions.h"
#include "BasicString.h"
#include "LEGO/String.h"
#include "Platform.h"
#include "LEGO/StdLib.h"

#if defined(_WIN32)
PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT = 0;   // Win 0xb425e4
PFNGLTEXIMAGE3DPROC glTexImage3D = 0;                             // Win 0xb426e0
PFNGLTEXSUBIMAGE3DPROC glTexSubImage3D = 0;                       // Win 0xb424f4
PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB = 0;             // Win 0xb426a4
PFNGLMULTITEXCOORD1FVARBPROC glMultiTexCoord1fvARB = 0;           // Win 0xb42628
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = 0;             // Win 0xb425d0
PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord2fvARB = 0;           // Win 0xb42618
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = 0;                 // Win 0xb4264c
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB = 0;     // Win 0xb42544
PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glCompressedTexImage3DARB = 0;   // Win 0xb426ac
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB = 0;   // Win 0xb4272c
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glCompressedTexImage1DARB = 0;   // Win 0xb424bc
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB = 0;// Win 0xb42648
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB = 0;// Win 0xb425a4
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB = 0;// Win 0xb424b8
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC glGetCompressedTexImageARB = 0; // Win 0xb42554
PFNGLARRAYELEMENTEXTPROC glArrayElementEXT = 0;                   // Win 0xb42548
PFNGLCOLORPOINTEREXTPROC glColorPointerEXT = 0;                   // Win 0xb425f8
PFNGLDRAWARRAYSEXTPROC glDrawArraysEXT = 0;                       // Win 0xb42564
PFNGLEDGEFLAGPOINTEREXTPROC glEdgeFlagPointerEXT = 0;             // Win 0xb42624
PFNGLGETPOINTERVEXTPROC glGetPointervEXT = 0;                     // Win 0xb42644
PFNGLINDEXPOINTEREXTPROC glIndexPointerEXT = 0;                   // Win 0xb42620
PFNGLNORMALPOINTEREXTPROC glNormalPointerEXT = 0;                 // Win 0xb425bc
PFNGLTEXCOORDPOINTEREXTPROC glTexCoordPointerEXT = 0;             // Win 0xb42614
PFNGLVERTEXPOINTEREXTPROC glVertexPointerEXT = 0;                 // Win 0xb42570
PFNGLBINDPROGRAMNVPROC glBindProgramNV = 0;                       // Win 0xb426b4
PFNGLDELETEPROGRAMSNVPROC glDeleteProgramsNV = 0;                 // Win 0xb425fc
PFNGLEXECUTEPROGRAMNVPROC glExecuteProgramNV = 0;                 // Win 0xb425f4
PFNGLGENPROGRAMSNVPROC glGenProgramsNV = 0;                       // Win 0xb42534
PFNGLLOADPROGRAMNVPROC glLoadProgramNV = 0;                       // Win 0xb42510
PFNGLPROGRAMPARAMETER4FNVPROC glProgramParameter4fNV = 0;         // Win 0xb42530
PFNGLTRACKMATRIXNVPROC glTrackMatrixNV = 0;                       // Win 0xb426b8
PFNGLVERTEXATTRIBPOINTERNVPROC glVertexAttribPointerNV = 0;       // Win 0xb42738
PFNGLDELETEFENCESNVPROC glDeleteFencesNV = 0;                     // Win 0xb425cc
PFNGLGENFENCESNVPROC glGenFencesNV = 0;                           // Win 0xb425a8
PFNGLISFENCENVPROC glIsFenceNV = 0;                               // Win 0xb4268c
PFNGLTESTFENCENVPROC glTestFenceNV = 0;                           // Win 0xb42540
PFNGLGETFENCEIVNVPROC glGetFenceivNV = 0;                         // Win 0xb42640
PFNGLFINISHFENCENVPROC glFinishFenceNV = 0;                       // Win 0xb425dc
PFNGLSETFENCENVPROC glSetFenceNV = 0;                             // Win 0xb4256c
PFNGLBINDBUFFERARBPROC glBindBufferARB = 0;                       // Win 0xb4253c
PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB = 0;                 // Win 0xb4255c
PFNGLGENBUFFERSARBPROC glGenBuffersARB = 0;                       // Win 0xb426c0
PFNGLISBUFFERARBPROC glIsBufferARB = 0;                           // Win 0xb42658
PFNGLBUFFERDATAARBPROC glBufferDataARB = 0;                       // Win 0xb42514
PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB = 0;                 // Win 0xb426f8
PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubDataARB = 0;           // Win 0xb42674
PFNGLMAPBUFFERARBPROC glMapBufferARB = 0;                         // Win 0xb425c8
PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB = 0;                     // Win 0xb425b0
PFNGLGETBUFFERPARAMETERIVARBPROC glGetBufferParameterivARB = 0;   // Win 0xb42688
PFNGLGETBUFFERPOINTERVARBPROC glGetBufferPointervARB = 0;         // Win 0xb42728
PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT = 0;               // Win 0xb42598
PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = 0;           // Win 0xb424f0
PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT = 0;     // Win 0xb42664
PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = 0;           // Win 0xb424d4
PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = 0;     // Win 0xb425e0
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT = 0;// Win 0xb426a8
PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT = 0;                 // Win 0xb426cc
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = 0;             // Win 0xb4258c
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = 0;       // Win 0xb4257c
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = 0;             // Win 0xb42578
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = 0;// Win 0xb424c4
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT = 0;   // Win 0xb42600
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = 0;   // Win 0xb424f8
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT = 0;   // Win 0xb425ac
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = 0;// Win 0xb424b4
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT = 0;// Win 0xb426c4
PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT = 0;               // Win 0xb42694
PFNGLDRAWBUFFERSPROC glDrawBuffers = 0;                           // Win 0xb4271c
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = 0;       // Win 0xb4267c
PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB = 0;               // Win 0xb425d8
PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB = 0;                 // Win 0xb426ec
PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB = 0;         // Win 0xb4252c
PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB = 0;             // Win 0xb42588
PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB = 0;                 // Win 0xb426e8
PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB = 0;                 // Win 0xb42558
PFNWGLRELEASETEXIMAGEARBPROC wglReleaseTexImageARB = 0;           // Win 0xb425a0
PFNWGLSETPBUFFERATTRIBARBPROC wglSetPBufferAttribARB = 0;         // Win 0xb42650
PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB = 0;     // Win 0xb42760
PFNGLCREATEPROGRAMPROC glCreateProgram = 0;                       // Win 0xb42660
PFNGLDELETEPROGRAMPROC glDeleteProgram = 0;                       // Win 0xb424d0
PFNGLCREATESHADERPROC glCreateShader = 0;                         // Win 0xb42528
PFNGLDELETESHADERPROC glDeleteShader = 0;                         // Win 0xb424dc
PFNGLSHADERSOURCEPROC glShaderSource = 0;                         // Win 0xb42668
PFNGLCOMPILESHADERPROC glCompileShader = 0;                       // Win 0xb425e8
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = 0;   // Win 0xb42720
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = 0;           // Win 0xb42560
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = 0; // Win 0xb42670
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = 0;             // Win 0xb424c0
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = 0;             // Win 0xb4266c
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = 0;               // Win 0xb4259c
PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib = 0;                   // Win 0xb4250c
PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform = 0;                 // Win 0xb42608
PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders = 0;             // Win 0xb42638
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = 0;               // Win 0xb42500
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = 0;                 // Win 0xb426fc
PFNGLGETSHADERSOURCEPROC glGetShaderSource = 0;                   // Win 0xb42604
PFNGLGETSHADERIVPROC glGetShaderiv = 0;                           // Win 0xb4262c
PFNGLVALIDATEPROGRAMPROC glValidateProgram = 0;                   // Win 0xb42550
PFNGLGETPROGRAMIVPROC glGetProgramiv = 0;                         // Win 0xb42508
PFNGLATTACHSHADERPROC glAttachShader = 0;                         // Win 0xb424c8
PFNGLDETACHSHADERPROC glDetachShader = 0;                         // Win 0xb426e4
PFNGLLINKPROGRAMPROC glLinkProgram = 0;                           // Win 0xb425f0
PFNGLUSEPROGRAMPROC glUseProgram = 0;                             // Win 0xb426dc
PFNGLUNIFORM1FPROC glUniform1f = 0;                               // Win 0xb4265c
PFNGLUNIFORM2FPROC glUniform2f = 0;                               // Win 0xb42568
PFNGLUNIFORM3FPROC glUniform3f = 0;                               // Win 0xb4260c
PFNGLUNIFORM4FPROC glUniform4f = 0;                               // Win 0xb426f4
PFNGLUNIFORM1IPROC glUniform1i = 0;                               // Win 0xb42734
PFNGLUNIFORM2IPROC glUniform2i = 0;                               // Win 0xb42520
PFNGLUNIFORM3IPROC glUniform3i = 0;                               // Win 0xb425d4
PFNGLUNIFORM4IPROC glUniform4i = 0;                               // Win 0xb425c0
PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv = 0;                 // Win 0xb424fc
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv = 0;                 // Win 0xb426d0
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = 0;                 // Win 0xb42524
PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f = 0;                     // Win 0xb424cc
PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f = 0;                     // Win 0xb42690
PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f = 0;                     // Win 0xb42708
PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f = 0;                     // Win 0xb42718
PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB = 0;     // Win 0xb426a0
PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB = 0;       // Win 0xb426f0
PFNGLDELETEOBJECTARBPROC glDeleteObjectARB = 0;                   // Win 0xb425c4
PFNGLSHADERSOURCEARBPROC glShaderSourceARB = 0;                   // Win 0xb426d8
PFNGLCOMPILESHADERARBPROC glCompileShaderARB = 0;                 // Win 0xb424e4
PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB = 0;       // Win 0xb42580
PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB = 0;       // Win 0xb42610
PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB = 0;         // Win 0xb426bc
PFNGLGETACTIVEATTRIBARBPROC glGetActiveAttribARB = 0;             // Win 0xb42594
PFNGLGETACTIVEUNIFORMARBPROC glGetActiveUniformARB = 0;           // Win 0xb4261c
PFNGLGETATTACHEDOBJECTSARBPROC glGetAttachedObjectsARB = 0;       // Win 0xb42590
PFNGLGETINFOLOGARBPROC glGetInfoLogARB = 0;                       // Win 0xb425b8
PFNGLGETSHADERSOURCEARBPROC glGetShaderSourceARB = 0;             // Win 0xb424ec
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB = 0;   // Win 0xb426d4
PFNGLGETOBJECTPARAMETERFVARBPROC glGetObjectParameterfvARB = 0;   // Win 0xb4269c
PFNGLVALIDATEPROGRAMARBPROC glValidateProgramARB = 0;             // Win 0xb42704
PFNGLATTACHOBJECTARBPROC glAttachObjectARB = 0;                   // Win 0xb42584
PFNGLDETACHOBJECTARBPROC glDetachObjectARB = 0;                   // Win 0xb42634
PFNGLLINKPROGRAMARBPROC glLinkProgramARB = 0;                     // Win 0xb425b4
PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB = 0;           // Win 0xb42710
PFNGLUNIFORM1FARBPROC glUniform1fARB = 0;                         // Win 0xb424d8
PFNGLUNIFORM2FARBPROC glUniform2fARB = 0;                         // Win 0xb42714
PFNGLUNIFORM3FARBPROC glUniform3fARB = 0;                         // Win 0xb426c8
PFNGLUNIFORM4FARBPROC glUniform4fARB = 0;                         // Win 0xb42630
PFNGLUNIFORM1IARBPROC glUniform1iARB = 0;                         // Win 0xb426b0
PFNGLUNIFORM2IARBPROC glUniform2iARB = 0;                         // Win 0xb424e0
PFNGLUNIFORM3IARBPROC glUniform3iARB = 0;                         // Win 0xb4270c
PFNGLUNIFORM4IARBPROC glUniform4iARB = 0;                         // Win 0xb42654
PFNGLUNIFORMMATRIX2FVARBPROC glUniformMatrix2fvARB = 0;           // Win 0xb42684
PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB = 0;           // Win 0xb424e8
PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB = 0;           // Win 0xb42538
PFNGLGENQUERIESARBPROC glGenQueriesARB = 0;                       // Win 0xb42724
PFNGLDELETEQUERIESARBPROC glDeleteQueriesARB = 0;                 // Win 0xb42680
PFNGLBEGINQUERYARBPROC glBeginQueryARB = 0;                       // Win 0xb42574
PFNGLENDQUERYARBPROC glEndQueryARB = 0;                           // Win 0xb42698
PFNGLGETQUERYIVPROC glGetQueryiv = 0;                             // Win 0xb42678
PFNGLGETQUERYOBJECTIVARBPROC glGetQueryObjectivARB = 0;           // Win 0xb4263c
PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuivARB = 0;         // Win 0xb4254c
PFNGLGENOCCLUSIONQUERIESNVPROC glGenOcclusionQueriesNV = 0;       // Win 0xb42730
PFNGLDELETEOCCLUSIONQUERIESNVPROC glDeleteOcclusionQueriesNV = 0; // Win 0xb42504
PFNGLBEGINOCCLUSIONQUERYNVPROC glBeginOcclusionQueryNV = 0;       // Win 0xb425ec
PFNGLENDOCCLUSIONQUERYNVPROC glEndOcclusionQueryNV = 0;           // Win 0xb4251c
PFNGLGETOCCLUSIONQUERYIVNVPROC glGetOcclusionQueryivNV = 0;       // Win 0xb42700
PFNGLGETOCCLUSIONQUERYUIVNVPROC glGetOcclusionQueryuivNV = 0;     // Win 0xb42518

#endif

namespace LEGO {

namespace {

// Name of the running platform (Win 0x41fdd0 member of the platform singleton, "WIN"; inlined
// on the Mac as the literal "MAC"). Same helper as in TextLocale.cpp. (name inferred)
inline String GetPlatformName()
{
#if defined(_WIN32)
    return String("WIN");
#else
    return String("MAC");
#endif
}

// "NAME " occurs in the extension list (substring search, not a token match). (helper; the
// original calls BasicString<char>::Find inline, Mac 0xf2730 / Win 0x43a9f0)
inline bool HasExtension(const AString& extensions, const char* name)
{
    return extensions.Find(name, 0, StrLen(name)) != AString::npos;
}

#if defined(_WIN32)
// Win 0x4f20c0 (EAX = name, stack: bool* ok): wglGetProcAddress; clears *ok on failure.
// (name inferred)
PROC GetExtensionProc(const char* name, bool* ok)
{
    PROC proc = wglGetProcAddress(name);
    if (!proc)
        *ok = false;
    return proc;
}

// Win 0x4f3670 (EDX = string, stack: AString*): appends a C string. (name inferred)
void AppendExtensions(const char* s, AString& extensions)
{
    extensions.Append(s, StrLen(s));
}

// Stores the entry point `name` in its global. (helper; inlined in the original)
template <class T>
inline void LoadExtensionProc(T& function, const char* name, bool* ok)
{
    function = (T)GetExtensionProc(name, ok);
}
#define LDD_GL_LOAD(name) LoadExtensionProc(name, #name, &ok)
#endif

} // namespace

// Mac 0xf2010 (+ ctor 0xf2820), Win 0x4f20e0 (ESI = this, stack: level)
void OpenGLExtensions::Initialize(UInt32 level)
{
    m_textureCompression = m_textureCubeMap = m_multitexture = m_vertexBufferObject = false;
    m_texture3D = m_textureCubeMapEXT = m_textureFloat = m_vertexArrayEXT = false;
    m_fenceNV = m_vertexArrayRange2NV = m_vertexProgramNV = m_pbuffer = false;
    m_framebufferObject = m_drawBuffers = m_shaders20 = false;
    m_glsl120 = false;     // NOTE: Win 0x4f2111.. leaves +0x0f alone (set below in any case)
    m_shaderObjectsARB = m_vertexBlend = m_occlusionQuery = m_occlusionQueryNV = false;
    m_occlusionTestHP = m_anyOcclusion = false;
    m_glVersion = Version(0, 0, 0, 0);
    m_glslVersion = Version(0, 0, 0, 0);

    const char* glExtensions = (const char*)glGetString(GL_EXTENSIONS);
    if (!glExtensions)
        return;
    AString extensions(glExtensions, StrLen(glExtensions));
    extensions.Append(" ", 1);
    {
        String version((const char*)glGetString(GL_VERSION));
        UInt32 position = 0;
        FromString(m_glVersion, version, position);
    }
#if defined(_WIN32)
    {
        bool ok = true;
        LDD_GL_LOAD(wglGetExtensionsStringEXT);
        if (ok && wglGetExtensionsStringEXT) {
            const char* wglExtensions = wglGetExtensionsStringEXT();
            if (wglExtensions) {
                AppendExtensions(wglExtensions, extensions);
                extensions.Append(" ", 1);
            }
        }
    }
#endif

    m_texture3D          = HasExtension(extensions, "GL_EXT_texture3D ");
    m_multitexture       = HasExtension(extensions, "ARB_multitexture ");
    m_textureCompression = HasExtension(extensions, "GL_ARB_texture_compression ");
    m_vertexArrayEXT     = HasExtension(extensions, "EXT_vertex_array ");
    m_textureCubeMap     = HasExtension(extensions, "GL_ARB_texture_cube_map ");
    m_textureCubeMapEXT  = !m_textureCubeMap && HasExtension(extensions, "GL_EXT_texture_cube_map ");
    m_textureFloat       = HasExtension(extensions, "GL_ARB_texture_float ");
    m_vertexProgramNV    = HasExtension(extensions, "GL_NV_vertex_program ");
    m_fenceNV            = HasExtension(extensions, "GL_NV_fence ");
    m_vertexArrayRange2NV = HasExtension(extensions, "GL_NV_vertex_array_range2 ");
    if (HasExtension(extensions, "GL_ARB_vertex_buffer_object ")) {
        // NOTE: meant to exclude Mac OS X before 10.4.3, but GetPlatformName() is "MAC" (Mac) or
        // "WIN", never "OSX": VBOs are always enabled when the extension exists.
        m_vertexBufferObject = GetPlatformName() != String("OSX") ||
                               Platform::GetInstance()->GetOSVersion().m_minor > 4 ||
                               (Platform::GetInstance()->GetOSVersion().m_minor == 4 &&
                                Platform::GetInstance()->GetOSVersion().m_build >= 3);
    } else {
        m_vertexBufferObject = false;
    }
    m_vertexBlend        = HasExtension(extensions, "GL_ARB_vertex_blend ");
    m_framebufferObject  = HasExtension(extensions, "GL_EXT_framebuffer_object ");
    m_drawBuffers        = m_framebufferObject && HasExtension(extensions, "GL_ARB_draw_buffers ");
    m_shaders20          = m_glVersion >= Version(2, 0, 0, 0);
    if (m_shaders20) {
        String version((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
        UInt32 position = 0;
        FromString(m_glslVersion, version, position);
    }
    m_glsl120            = m_glslVersion >= Version(1, 20, 0, 0);
    m_shaderObjectsARB   = HasExtension(extensions, "GL_ARB_shading_language_100 ") &&
                           HasExtension(extensions, "GL_ARB_shader_objects ") &&
                           HasExtension(extensions, "GL_ARB_fragment_shader ") &&
                           HasExtension(extensions, "GL_ARB_vertex_shader ");
    m_occlusionQuery     = HasExtension(extensions, "GL_ARB_occlusion_query ");
    m_occlusionQueryNV   = HasExtension(extensions, "GL_NV_occlusion_query ");
    m_occlusionTestHP    = HasExtension(extensions, "GL_HP_occlusion_test ");
    m_anyOcclusion       = m_occlusionQuery || m_occlusionQueryNV || m_occlusionTestHP;

    // compatibility mode level
    if (level >= 20) {
        m_shaders20 = m_glsl120 = m_shaderObjectsARB = m_textureFloat = m_drawBuffers = false;
    }
    if (level >= 40)
        m_framebufferObject = false;
    if (level >= 60)
        m_vertexBufferObject = false;
    if (level >= 80) {
        m_texture3D = m_multitexture = m_textureCompression = m_vertexArrayEXT = false;
        m_textureCubeMap = m_textureCubeMapEXT = m_vertexProgramNV = m_fenceNV = false;
        m_vertexArrayRange2NV = m_vertexBlend = m_occlusionQuery = m_occlusionQueryNV = false;
        m_occlusionTestHP = m_anyOcclusion = false;
    }

    // entry points, one group per extension; a group with a missing function is switched off
    if (m_texture3D) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glTexImage3D);
        LDD_GL_LOAD(glTexSubImage3D);
#endif
        if (!ok)
            m_texture3D = false;
    }
    if (m_multitexture) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glMultiTexCoord1fARB);
        LDD_GL_LOAD(glMultiTexCoord1fvARB);
        LDD_GL_LOAD(glMultiTexCoord2fARB);
        LDD_GL_LOAD(glMultiTexCoord2fvARB);
        LDD_GL_LOAD(glActiveTextureARB);
        LDD_GL_LOAD(glClientActiveTextureARB);
#endif
        if (!ok)
            m_multitexture = false;
    }
    if (m_textureCompression) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glCompressedTexImage3DARB);
        LDD_GL_LOAD(glCompressedTexImage2DARB);
        LDD_GL_LOAD(glCompressedTexImage1DARB);
        LDD_GL_LOAD(glCompressedTexSubImage3DARB);
        LDD_GL_LOAD(glCompressedTexSubImage2DARB);
        LDD_GL_LOAD(glCompressedTexSubImage1DARB);
        LDD_GL_LOAD(glGetCompressedTexImageARB);
#endif
        if (!ok)
            m_textureCompression = false;
    }
#if defined(_WIN32)
    if (m_vertexArrayEXT) {
        bool ok = true;
        LDD_GL_LOAD(glArrayElementEXT);
        LDD_GL_LOAD(glColorPointerEXT);
        LDD_GL_LOAD(glDrawArraysEXT);
        LDD_GL_LOAD(glEdgeFlagPointerEXT);
        LDD_GL_LOAD(glGetPointervEXT);
        LDD_GL_LOAD(glIndexPointerEXT);
        LDD_GL_LOAD(glNormalPointerEXT);
        LDD_GL_LOAD(glTexCoordPointerEXT);
        LDD_GL_LOAD(glVertexPointerEXT);
        if (!ok)
            m_vertexArrayEXT = false;
    }
    if (m_vertexProgramNV) {
        bool ok = true;
        LDD_GL_LOAD(glBindProgramNV);
        LDD_GL_LOAD(glDeleteProgramsNV);
        LDD_GL_LOAD(glExecuteProgramNV);
        LDD_GL_LOAD(glGenProgramsNV);
        LDD_GL_LOAD(glLoadProgramNV);
        LDD_GL_LOAD(glProgramParameter4fNV);
        LDD_GL_LOAD(glTrackMatrixNV);
        LDD_GL_LOAD(glVertexAttribPointerNV);
        if (!ok)
            m_vertexProgramNV = false;
    }
#endif
    if (m_fenceNV) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glDeleteFencesNV);
        LDD_GL_LOAD(glGenFencesNV);
        LDD_GL_LOAD(glIsFenceNV);
        LDD_GL_LOAD(glTestFenceNV);
        LDD_GL_LOAD(glGetFenceivNV);
        LDD_GL_LOAD(glFinishFenceNV);
        LDD_GL_LOAD(glSetFenceNV);
#else
        ok = false;   // not in OpenGL.framework (Mac 0xf263a clears the flag)
#endif
        if (!ok)
            m_fenceNV = false;
    }
    if (m_vertexBufferObject) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glBindBufferARB);
        LDD_GL_LOAD(glDeleteBuffersARB);
        LDD_GL_LOAD(glGenBuffersARB);
        LDD_GL_LOAD(glIsBufferARB);
        LDD_GL_LOAD(glBufferDataARB);
        LDD_GL_LOAD(glBufferSubDataARB);
        LDD_GL_LOAD(glGetBufferSubDataARB);
        LDD_GL_LOAD(glMapBufferARB);
        LDD_GL_LOAD(glUnmapBufferARB);
        LDD_GL_LOAD(glGetBufferParameterivARB);
        LDD_GL_LOAD(glGetBufferPointervARB);
#endif
        if (!ok)
            m_vertexBufferObject = false;
    }
    if (m_framebufferObject) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glIsRenderbufferEXT);
        LDD_GL_LOAD(glBindRenderbufferEXT);
        LDD_GL_LOAD(glDeleteRenderbuffersEXT);
        LDD_GL_LOAD(glGenRenderbuffersEXT);
        LDD_GL_LOAD(glRenderbufferStorageEXT);
        LDD_GL_LOAD(glGetRenderbufferParameterivEXT);
        LDD_GL_LOAD(glIsFramebufferEXT);
        LDD_GL_LOAD(glBindFramebufferEXT);
        LDD_GL_LOAD(glDeleteFramebuffersEXT);
        LDD_GL_LOAD(glGenFramebuffersEXT);
        LDD_GL_LOAD(glCheckFramebufferStatusEXT);
        LDD_GL_LOAD(glFramebufferTexture1DEXT);
        LDD_GL_LOAD(glFramebufferTexture2DEXT);
        LDD_GL_LOAD(glFramebufferTexture3DEXT);
        LDD_GL_LOAD(glFramebufferRenderbufferEXT);
        LDD_GL_LOAD(glGetFramebufferAttachmentParameterivEXT);
        LDD_GL_LOAD(glGenerateMipmapEXT);
        LDD_GL_LOAD(glDrawBuffers);
#endif
        if (!ok)
            m_framebufferObject = false;
    }
    if (m_drawBuffers) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glDrawBuffers);
#endif
        if (!ok)
            m_drawBuffers = false;
    }
#if defined(_WIN32)
    if (m_pbuffer) {   // NOTE: dead code, m_pbuffer is never set (the three checks are unused)
        HasExtension(extensions, "WGL_ARB_pbuffer ");
        HasExtension(extensions, "WGL_ARB_pixel_format ");
        HasExtension(extensions, "WGL_ARB_extensions_string ");
        bool ok = true;
        LDD_GL_LOAD(wglChoosePixelFormatARB);
        LDD_GL_LOAD(wglCreatePbufferARB);
        LDD_GL_LOAD(wglGetPbufferDCARB);
        LDD_GL_LOAD(wglReleasePbufferDCARB);
        LDD_GL_LOAD(wglDestroyPbufferARB);
        LDD_GL_LOAD(wglQueryPbufferARB);
        LDD_GL_LOAD(wglBindTexImageARB);
        LDD_GL_LOAD(wglReleaseTexImageARB);
        LDD_GL_LOAD(wglSetPBufferAttribARB);
        if (HasExtension(extensions, "WGL_ARB_make_current_read "))
            LDD_GL_LOAD(wglMakeContextCurrentARB);
        if (!ok)
            m_pbuffer = false;
    }
#endif
    if (m_shaders20) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glCreateProgram);
        LDD_GL_LOAD(glDeleteProgram);
        LDD_GL_LOAD(glCreateShader);
        LDD_GL_LOAD(glDeleteShader);
        LDD_GL_LOAD(glShaderSource);
        LDD_GL_LOAD(glCompileShader);
        LDD_GL_LOAD(glEnableVertexAttribArray);
        LDD_GL_LOAD(glVertexAttribPointer);
        LDD_GL_LOAD(glDisableVertexAttribArray);
        LDD_GL_LOAD(glBindAttribLocation);
        LDD_GL_LOAD(glGetUniformLocation);
        LDD_GL_LOAD(glGetAttribLocation);
        LDD_GL_LOAD(glGetActiveAttrib);
        LDD_GL_LOAD(glGetActiveUniform);
        LDD_GL_LOAD(glGetAttachedShaders);
        LDD_GL_LOAD(glGetProgramInfoLog);
        LDD_GL_LOAD(glGetShaderInfoLog);
        LDD_GL_LOAD(glGetShaderSource);
        LDD_GL_LOAD(glGetShaderiv);
        LDD_GL_LOAD(glValidateProgram);
        LDD_GL_LOAD(glGetProgramiv);
        LDD_GL_LOAD(glAttachShader);
        LDD_GL_LOAD(glDetachShader);
        LDD_GL_LOAD(glLinkProgram);
        LDD_GL_LOAD(glUseProgram);
        LDD_GL_LOAD(glUniform1f);
        LDD_GL_LOAD(glUniform2f);
        LDD_GL_LOAD(glUniform3f);
        LDD_GL_LOAD(glUniform4f);
        LDD_GL_LOAD(glUniform1i);
        LDD_GL_LOAD(glUniform2i);
        LDD_GL_LOAD(glUniform3i);
        LDD_GL_LOAD(glUniform4i);
        LDD_GL_LOAD(glUniformMatrix2fv);
        LDD_GL_LOAD(glUniformMatrix3fv);
        LDD_GL_LOAD(glUniformMatrix4fv);
        LDD_GL_LOAD(glVertexAttrib1f);
        LDD_GL_LOAD(glVertexAttrib2f);
        LDD_GL_LOAD(glVertexAttrib3f);
        LDD_GL_LOAD(glVertexAttrib4f);
#endif
        if (!ok)
            m_shaders20 = false;
    } else if (m_shaderObjectsARB) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glCreateProgramObjectARB);
        LDD_GL_LOAD(glCreateShaderObjectARB);
        LDD_GL_LOAD(glDeleteObjectARB);
        LDD_GL_LOAD(glShaderSourceARB);
        LDD_GL_LOAD(glCompileShaderARB);
        LDD_GL_LOAD(glBindAttribLocationARB);
        LDD_GL_LOAD(glGetUniformLocationARB);
        LDD_GL_LOAD(glGetAttribLocationARB);
        LDD_GL_LOAD(glGetActiveAttribARB);
        LDD_GL_LOAD(glGetActiveUniformARB);
        LDD_GL_LOAD(glGetAttachedObjectsARB);
        LDD_GL_LOAD(glGetInfoLogARB);
        LDD_GL_LOAD(glGetShaderSourceARB);
        LDD_GL_LOAD(glGetObjectParameterivARB);
        LDD_GL_LOAD(glGetObjectParameterfvARB);
        LDD_GL_LOAD(glValidateProgramARB);
        LDD_GL_LOAD(glAttachObjectARB);
        LDD_GL_LOAD(glDetachObjectARB);
        LDD_GL_LOAD(glLinkProgramARB);
        LDD_GL_LOAD(glUseProgramObjectARB);
        LDD_GL_LOAD(glUniform1fARB);
        LDD_GL_LOAD(glUniform2fARB);
        LDD_GL_LOAD(glUniform3fARB);
        LDD_GL_LOAD(glUniform4fARB);
        LDD_GL_LOAD(glUniform1iARB);
        LDD_GL_LOAD(glUniform2iARB);
        LDD_GL_LOAD(glUniform3iARB);
        LDD_GL_LOAD(glUniform4iARB);
        LDD_GL_LOAD(glUniformMatrix2fvARB);
        LDD_GL_LOAD(glUniformMatrix3fvARB);
        LDD_GL_LOAD(glUniformMatrix4fvARB);
#endif
        if (!ok)
            m_shaderObjectsARB = false;
    }
    if (m_occlusionQuery) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glGenQueriesARB);
        LDD_GL_LOAD(glDeleteQueriesARB);
        LDD_GL_LOAD(glBeginQueryARB);
        LDD_GL_LOAD(glEndQueryARB);
        LDD_GL_LOAD(glGetQueryiv);
        LDD_GL_LOAD(glGetQueryObjectivARB);
        LDD_GL_LOAD(glGetQueryObjectuivARB);
#endif
        if (!ok)
            m_occlusionQuery = false;
    }
    if (m_occlusionQueryNV) {
        bool ok = true;
#if defined(_WIN32)
        LDD_GL_LOAD(glGenOcclusionQueriesNV);
        LDD_GL_LOAD(glDeleteOcclusionQueriesNV);
        LDD_GL_LOAD(glBeginOcclusionQueryNV);
        LDD_GL_LOAD(glEndOcclusionQueryNV);
        LDD_GL_LOAD(glGetOcclusionQueryivNV);
        LDD_GL_LOAD(glGetOcclusionQueryuivNV);
#else
        ok = false;   // not in OpenGL.framework (Mac 0xf2682 clears the flag)
#endif
        if (!ok)
            m_occlusionQueryNV = false;
    }
}

} // namespace LEGO
