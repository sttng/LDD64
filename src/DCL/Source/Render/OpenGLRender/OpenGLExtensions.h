// LEGO Digital Designer — restored source
// LEGO::OpenGLExtensions — 0x38-byte capability object made by OpenGLRender's ctor (Mac 0xf2820
// at 0xf2940, Win 0x4f20e0 at 0x4ec580; at OpenGLRender Mac +0x14 / Win +0x20). Names inferred.
// TU 033: Mac 0xf2010-0xf286f (Initialize 0xf2010, weak Find 0xf2730, ctor
// 0xf2820), Win 0x4f20c0-0x4f3690 (GetExtensionProc 0x4f20c0, ctor/Initialize 0x4f20e0,
// AppendExtensions 0x4f3670).
// Mac links all entry points directly and only checks the extension string (NV_fence /
// NV_occlusion_query do not exist there); Win loads everything past GL 1.1 with
// wglGetProcAddress into the globals below (Win .data 0xb424b4-0xb42760) and switches off a
// group with a missing entry point. Flags are bytes; CompatibilityModeLevel (Render::m_level)
// switches groups off: >= 20 shaders / float textures / draw buffers, >= 40 FBOs, >= 60 VBOs,
// >= 80 everything.
#pragma once
#include "LEGO/Types.h"
#include "Version.h"

#if defined(_WIN32)
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

namespace LEGO {

// ARB_shader_objects handles are kept as UInt32 (OpenGLShaderARB::m_program,
// OpenGLSharedShader::m_handle). GLhandleARB is unsigned int in the Windows / Khronos glext.h but
// void* in Apple's (Mac OS X 10.7 and later SDKs; the values are small integers there as well):
// the conversions go through size_t (no code where GLhandleARB is an integer).
inline GLhandleARB ToGLhandleARB(UInt32 handle) { return (GLhandleARB)(size_t)handle; }
inline UInt32 FromGLhandleARB(GLhandleARB handle) { return (UInt32)(size_t)handle; }

struct OpenGLExtensions {
    explicit OpenGLExtensions(UInt32 level) { Initialize(level); }   // Mac 0xf2820, Win 0x4f20e0

    // Clears everything, reads GL_EXTENSIONS / GL_VERSION / GL_SHADING_LANGUAGE_VERSION of the
    // current context and fills the flags. (name inferred)
    void Initialize(UInt32 level);                                   // Mac 0xf2010, Win 0x4f20e0

    bool    m_textureCompression;    // +0x00 GL_ARB_texture_compression
    bool    m_textureCubeMap;        // +0x01 GL_ARB_texture_cube_map
    bool    m_multitexture;          // +0x02 ARB_multitexture
    bool    m_vertexBufferObject;    // +0x03 GL_ARB_vertex_buffer_object
    bool    m_texture3D;             // +0x04 GL_EXT_texture3D
    bool    m_textureCubeMapEXT;     // +0x05 GL_EXT_texture_cube_map (only without the ARB one)
    bool    m_textureFloat;          // +0x06 GL_ARB_texture_float
    bool    m_vertexArrayEXT;        // +0x07 EXT_vertex_array
    bool    m_fenceNV;               // +0x08 GL_NV_fence
    bool    m_vertexArrayRange2NV;   // +0x09 GL_NV_vertex_array_range2
    bool    m_vertexProgramNV;       // +0x0a GL_NV_vertex_program
    bool    m_pbuffer;               // +0x0b WGL_ARB_pbuffer (NOTE: never set by Initialize)
    bool    m_framebufferObject;     // +0x0c GL_EXT_framebuffer_object
    bool    m_drawBuffers;           // +0x0d FBO + GL_ARB_draw_buffers
    bool    m_shaders20;             // +0x0e OpenGL >= 2.0
    bool    m_glsl120;               // +0x0f GLSL >= 1.20
    bool    m_shaderObjectsARB;      // +0x10 GL_ARB_shading_language_100 + shader_objects +
                                     //       fragment_shader + vertex_shader
    bool    m_vertexBlend;           // +0x11 GL_ARB_vertex_blend
    bool    m_occlusionQuery;        // +0x12 GL_ARB_occlusion_query
    bool    m_occlusionQueryNV;      // +0x13 GL_NV_occlusion_query
    bool    m_occlusionTestHP;       // +0x14 GL_HP_occlusion_test
    bool    m_anyOcclusion;          // +0x15 one of the three above
    Version m_glVersion;             // +0x18 GL_VERSION
    Version m_glslVersion;           // +0x28 GL_SHADING_LANGUAGE_VERSION (only with m_shaders20)
};

} // namespace LEGO

#if defined(_WIN32)
// Extension entry points loaded by OpenGLExtensions::Initialize (Win 0x4f20e0), in load order.
// Null when the group is not supported. (global names inferred: the GL names)
extern PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT;// Win 0xb425e4
extern PFNGLTEXIMAGE3DPROC glTexImage3D;                          // Win 0xb426e0
extern PFNGLTEXSUBIMAGE3DPROC glTexSubImage3D;                    // Win 0xb424f4
extern PFNGLMULTITEXCOORD1FARBPROC glMultiTexCoord1fARB;          // Win 0xb426a4
extern PFNGLMULTITEXCOORD1FVARBPROC glMultiTexCoord1fvARB;        // Win 0xb42628
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;          // Win 0xb425d0
extern PFNGLMULTITEXCOORD2FVARBPROC glMultiTexCoord2fvARB;        // Win 0xb42618
extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;              // Win 0xb4264c
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;  // Win 0xb42544
extern PFNGLCOMPRESSEDTEXIMAGE3DARBPROC glCompressedTexImage3DARB;// Win 0xb426ac
extern PFNGLCOMPRESSEDTEXIMAGE2DARBPROC glCompressedTexImage2DARB;// Win 0xb4272c
extern PFNGLCOMPRESSEDTEXIMAGE1DARBPROC glCompressedTexImage1DARB;// Win 0xb424bc
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3DARB;// Win 0xb42648
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2DARB;// Win 0xb425a4
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1DARB;// Win 0xb424b8
extern PFNGLGETCOMPRESSEDTEXIMAGEARBPROC glGetCompressedTexImageARB;// Win 0xb42554
extern PFNGLARRAYELEMENTEXTPROC glArrayElementEXT;                // Win 0xb42548
extern PFNGLCOLORPOINTEREXTPROC glColorPointerEXT;                // Win 0xb425f8
extern PFNGLDRAWARRAYSEXTPROC glDrawArraysEXT;                    // Win 0xb42564
extern PFNGLEDGEFLAGPOINTEREXTPROC glEdgeFlagPointerEXT;          // Win 0xb42624
extern PFNGLGETPOINTERVEXTPROC glGetPointervEXT;                  // Win 0xb42644
extern PFNGLINDEXPOINTEREXTPROC glIndexPointerEXT;                // Win 0xb42620
extern PFNGLNORMALPOINTEREXTPROC glNormalPointerEXT;              // Win 0xb425bc
extern PFNGLTEXCOORDPOINTEREXTPROC glTexCoordPointerEXT;          // Win 0xb42614
extern PFNGLVERTEXPOINTEREXTPROC glVertexPointerEXT;              // Win 0xb42570
extern PFNGLBINDPROGRAMNVPROC glBindProgramNV;                    // Win 0xb426b4
extern PFNGLDELETEPROGRAMSNVPROC glDeleteProgramsNV;              // Win 0xb425fc
extern PFNGLEXECUTEPROGRAMNVPROC glExecuteProgramNV;              // Win 0xb425f4
extern PFNGLGENPROGRAMSNVPROC glGenProgramsNV;                    // Win 0xb42534
extern PFNGLLOADPROGRAMNVPROC glLoadProgramNV;                    // Win 0xb42510
extern PFNGLPROGRAMPARAMETER4FNVPROC glProgramParameter4fNV;      // Win 0xb42530
extern PFNGLTRACKMATRIXNVPROC glTrackMatrixNV;                    // Win 0xb426b8
extern PFNGLVERTEXATTRIBPOINTERNVPROC glVertexAttribPointerNV;    // Win 0xb42738
extern PFNGLDELETEFENCESNVPROC glDeleteFencesNV;                  // Win 0xb425cc
extern PFNGLGENFENCESNVPROC glGenFencesNV;                        // Win 0xb425a8
extern PFNGLISFENCENVPROC glIsFenceNV;                            // Win 0xb4268c
extern PFNGLTESTFENCENVPROC glTestFenceNV;                        // Win 0xb42540
extern PFNGLGETFENCEIVNVPROC glGetFenceivNV;                      // Win 0xb42640
extern PFNGLFINISHFENCENVPROC glFinishFenceNV;                    // Win 0xb425dc
extern PFNGLSETFENCENVPROC glSetFenceNV;                          // Win 0xb4256c
extern PFNGLBINDBUFFERARBPROC glBindBufferARB;                    // Win 0xb4253c
extern PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB;              // Win 0xb4255c
extern PFNGLGENBUFFERSARBPROC glGenBuffersARB;                    // Win 0xb426c0
extern PFNGLISBUFFERARBPROC glIsBufferARB;                        // Win 0xb42658
extern PFNGLBUFFERDATAARBPROC glBufferDataARB;                    // Win 0xb42514
extern PFNGLBUFFERSUBDATAARBPROC glBufferSubDataARB;              // Win 0xb426f8
extern PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubDataARB;        // Win 0xb42674
extern PFNGLMAPBUFFERARBPROC glMapBufferARB;                      // Win 0xb425c8
extern PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB;                  // Win 0xb425b0
extern PFNGLGETBUFFERPARAMETERIVARBPROC glGetBufferParameterivARB;// Win 0xb42688
extern PFNGLGETBUFFERPOINTERVARBPROC glGetBufferPointervARB;      // Win 0xb42728
extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;            // Win 0xb42598
extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;        // Win 0xb424f0
extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;  // Win 0xb42664
extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;        // Win 0xb424d4
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;  // Win 0xb425e0
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;// Win 0xb426a8
extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;              // Win 0xb426cc
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;          // Win 0xb4258c
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;    // Win 0xb4257c
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;          // Win 0xb42578
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;// Win 0xb424c4
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;// Win 0xb42600
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;// Win 0xb424f8
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;// Win 0xb425ac
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;// Win 0xb424b4
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;// Win 0xb426c4
extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;            // Win 0xb42694
extern PFNGLDRAWBUFFERSPROC glDrawBuffers;                        // Win 0xb4271c
extern PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;    // Win 0xb4267c
extern PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB;            // Win 0xb425d8
extern PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB;              // Win 0xb426ec
extern PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB;      // Win 0xb4252c
extern PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB;          // Win 0xb42588
extern PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB;              // Win 0xb426e8
extern PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB;              // Win 0xb42558
extern PFNWGLRELEASETEXIMAGEARBPROC wglReleaseTexImageARB;        // Win 0xb425a0
extern PFNWGLSETPBUFFERATTRIBARBPROC wglSetPBufferAttribARB;      // Win 0xb42650
extern PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB;  // Win 0xb42760
extern PFNGLCREATEPROGRAMPROC glCreateProgram;                    // Win 0xb42660
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;                    // Win 0xb424d0
extern PFNGLCREATESHADERPROC glCreateShader;                      // Win 0xb42528
extern PFNGLDELETESHADERPROC glDeleteShader;                      // Win 0xb424dc
extern PFNGLSHADERSOURCEPROC glShaderSource;                      // Win 0xb42668
extern PFNGLCOMPILESHADERPROC glCompileShader;                    // Win 0xb425e8
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;// Win 0xb42720
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;        // Win 0xb42560
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;// Win 0xb42670
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;          // Win 0xb424c0
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;          // Win 0xb4266c
extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;            // Win 0xb4259c
extern PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib;                // Win 0xb4250c
extern PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;              // Win 0xb42608
extern PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders;          // Win 0xb42638
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;            // Win 0xb42500
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;              // Win 0xb426fc
extern PFNGLGETSHADERSOURCEPROC glGetShaderSource;                // Win 0xb42604
extern PFNGLGETSHADERIVPROC glGetShaderiv;                        // Win 0xb4262c
extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;                // Win 0xb42550
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;                      // Win 0xb42508
extern PFNGLATTACHSHADERPROC glAttachShader;                      // Win 0xb424c8
extern PFNGLDETACHSHADERPROC glDetachShader;                      // Win 0xb426e4
extern PFNGLLINKPROGRAMPROC glLinkProgram;                        // Win 0xb425f0
extern PFNGLUSEPROGRAMPROC glUseProgram;                          // Win 0xb426dc
extern PFNGLUNIFORM1FPROC glUniform1f;                            // Win 0xb4265c
extern PFNGLUNIFORM2FPROC glUniform2f;                            // Win 0xb42568
extern PFNGLUNIFORM3FPROC glUniform3f;                            // Win 0xb4260c
extern PFNGLUNIFORM4FPROC glUniform4f;                            // Win 0xb426f4
extern PFNGLUNIFORM1IPROC glUniform1i;                            // Win 0xb42734
extern PFNGLUNIFORM2IPROC glUniform2i;                            // Win 0xb42520
extern PFNGLUNIFORM3IPROC glUniform3i;                            // Win 0xb425d4
extern PFNGLUNIFORM4IPROC glUniform4i;                            // Win 0xb425c0
extern PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;              // Win 0xb424fc
extern PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;              // Win 0xb426d0
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;              // Win 0xb42524
extern PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f;                  // Win 0xb424cc
extern PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f;                  // Win 0xb42690
extern PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f;                  // Win 0xb42708
extern PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;                  // Win 0xb42718
extern PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;  // Win 0xb426a0
extern PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;    // Win 0xb426f0
extern PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;                // Win 0xb425c4
extern PFNGLSHADERSOURCEARBPROC glShaderSourceARB;                // Win 0xb426d8
extern PFNGLCOMPILESHADERARBPROC glCompileShaderARB;              // Win 0xb424e4
extern PFNGLBINDATTRIBLOCATIONARBPROC glBindAttribLocationARB;    // Win 0xb42580
extern PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;    // Win 0xb42610
extern PFNGLGETATTRIBLOCATIONARBPROC glGetAttribLocationARB;      // Win 0xb426bc
extern PFNGLGETACTIVEATTRIBARBPROC glGetActiveAttribARB;          // Win 0xb42594
extern PFNGLGETACTIVEUNIFORMARBPROC glGetActiveUniformARB;        // Win 0xb4261c
extern PFNGLGETATTACHEDOBJECTSARBPROC glGetAttachedObjectsARB;    // Win 0xb42590
extern PFNGLGETINFOLOGARBPROC glGetInfoLogARB;                    // Win 0xb425b8
extern PFNGLGETSHADERSOURCEARBPROC glGetShaderSourceARB;          // Win 0xb424ec
extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;// Win 0xb426d4
extern PFNGLGETOBJECTPARAMETERFVARBPROC glGetObjectParameterfvARB;// Win 0xb4269c
extern PFNGLVALIDATEPROGRAMARBPROC glValidateProgramARB;          // Win 0xb42704
extern PFNGLATTACHOBJECTARBPROC glAttachObjectARB;                // Win 0xb42584
extern PFNGLDETACHOBJECTARBPROC glDetachObjectARB;                // Win 0xb42634
extern PFNGLLINKPROGRAMARBPROC glLinkProgramARB;                  // Win 0xb425b4
extern PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;        // Win 0xb42710
extern PFNGLUNIFORM1FARBPROC glUniform1fARB;                      // Win 0xb424d8
extern PFNGLUNIFORM2FARBPROC glUniform2fARB;                      // Win 0xb42714
extern PFNGLUNIFORM3FARBPROC glUniform3fARB;                      // Win 0xb426c8
extern PFNGLUNIFORM4FARBPROC glUniform4fARB;                      // Win 0xb42630
extern PFNGLUNIFORM1IARBPROC glUniform1iARB;                      // Win 0xb426b0
extern PFNGLUNIFORM2IARBPROC glUniform2iARB;                      // Win 0xb424e0
extern PFNGLUNIFORM3IARBPROC glUniform3iARB;                      // Win 0xb4270c
extern PFNGLUNIFORM4IARBPROC glUniform4iARB;                      // Win 0xb42654
extern PFNGLUNIFORMMATRIX2FVARBPROC glUniformMatrix2fvARB;        // Win 0xb42684
extern PFNGLUNIFORMMATRIX3FVARBPROC glUniformMatrix3fvARB;        // Win 0xb424e8
extern PFNGLUNIFORMMATRIX4FVARBPROC glUniformMatrix4fvARB;        // Win 0xb42538
extern PFNGLGENQUERIESARBPROC glGenQueriesARB;                    // Win 0xb42724
extern PFNGLDELETEQUERIESARBPROC glDeleteQueriesARB;              // Win 0xb42680
extern PFNGLBEGINQUERYARBPROC glBeginQueryARB;                    // Win 0xb42574
extern PFNGLENDQUERYARBPROC glEndQueryARB;                        // Win 0xb42698
extern PFNGLGETQUERYIVPROC glGetQueryiv;                          // Win 0xb42678
extern PFNGLGETQUERYOBJECTIVARBPROC glGetQueryObjectivARB;        // Win 0xb4263c
extern PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuivARB;      // Win 0xb4254c
extern PFNGLGENOCCLUSIONQUERIESNVPROC glGenOcclusionQueriesNV;    // Win 0xb42730
extern PFNGLDELETEOCCLUSIONQUERIESNVPROC glDeleteOcclusionQueriesNV;// Win 0xb42504
extern PFNGLBEGINOCCLUSIONQUERYNVPROC glBeginOcclusionQueryNV;    // Win 0xb425ec
extern PFNGLENDOCCLUSIONQUERYNVPROC glEndOcclusionQueryNV;        // Win 0xb4251c
extern PFNGLGETOCCLUSIONQUERYIVNVPROC glGetOcclusionQueryivNV;    // Win 0xb42700
extern PFNGLGETOCCLUSIONQUERYUIVNVPROC glGetOcclusionQueryuivNV;  // Win 0xb42518
#endif
