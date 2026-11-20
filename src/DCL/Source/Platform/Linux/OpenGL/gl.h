// LDD64 Linux platform layer (not restored code): the Mac branch's <OpenGL/gl.h> on the system's
// libglvnd headers, with GL_GLEXT_PROTOTYPES for the extension functions the Mac build links
// directly; libGL dispatches them to the current context (GLX or EGL).
#pragma once
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
// Apple's GLhandleARB is void* (10.7+ SDKs), Mesa's unsigned int: Mesa's ARB_shader_objects
// declarations are renamed away and redeclared with Apple's type (same symbols, same register).
#define GLhandleARB compat_host_GLhandleARB
#define glDeleteObjectARB compat_host_glDeleteObjectARB
#define glGetHandleARB compat_host_glGetHandleARB
#define glDetachObjectARB compat_host_glDetachObjectARB
#define glCreateShaderObjectARB compat_host_glCreateShaderObjectARB
#define glShaderSourceARB compat_host_glShaderSourceARB
#define glCompileShaderARB compat_host_glCompileShaderARB
#define glCreateProgramObjectARB compat_host_glCreateProgramObjectARB
#define glAttachObjectARB compat_host_glAttachObjectARB
#define glLinkProgramARB compat_host_glLinkProgramARB
#define glUseProgramObjectARB compat_host_glUseProgramObjectARB
#define glValidateProgramARB compat_host_glValidateProgramARB
#define glGetObjectParameterfvARB compat_host_glGetObjectParameterfvARB
#define glGetObjectParameterivARB compat_host_glGetObjectParameterivARB
#define glGetInfoLogARB compat_host_glGetInfoLogARB
#define glGetAttachedObjectsARB compat_host_glGetAttachedObjectsARB
#define glGetUniformLocationARB compat_host_glGetUniformLocationARB
#define glGetActiveUniformARB compat_host_glGetActiveUniformARB
#define glGetUniformfvARB compat_host_glGetUniformfvARB
#define glGetUniformivARB compat_host_glGetUniformivARB
#define glGetShaderSourceARB compat_host_glGetShaderSourceARB
#define glBindAttribLocationARB compat_host_glBindAttribLocationARB
#define glGetActiveAttribARB compat_host_glGetActiveAttribARB
#define glGetAttribLocationARB compat_host_glGetAttribLocationARB
#include <GL/gl.h>
#include <GL/glext.h>
#undef GLhandleARB
#undef glDeleteObjectARB
#undef glGetHandleARB
#undef glDetachObjectARB
#undef glCreateShaderObjectARB
#undef glShaderSourceARB
#undef glCompileShaderARB
#undef glCreateProgramObjectARB
#undef glAttachObjectARB
#undef glLinkProgramARB
#undef glUseProgramObjectARB
#undef glValidateProgramARB
#undef glGetObjectParameterfvARB
#undef glGetObjectParameterivARB
#undef glGetInfoLogARB
#undef glGetAttachedObjectsARB
#undef glGetUniformLocationARB
#undef glGetActiveUniformARB
#undef glGetUniformfvARB
#undef glGetUniformivARB
#undef glGetShaderSourceARB
#undef glBindAttribLocationARB
#undef glGetActiveAttribARB
#undef glGetAttribLocationARB
typedef void* GLhandleARB;
#ifdef __cplusplus
extern "C" {
#endif
void glDeleteObjectARB(GLhandleARB obj);
GLhandleARB glGetHandleARB(GLenum pname);
void glDetachObjectARB(GLhandleARB containerObj, GLhandleARB attachedObj);
GLhandleARB glCreateShaderObjectARB(GLenum shaderType);
void glShaderSourceARB(GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length);
void glCompileShaderARB(GLhandleARB shaderObj);
GLhandleARB glCreateProgramObjectARB(void);
void glAttachObjectARB(GLhandleARB containerObj, GLhandleARB obj);
void glLinkProgramARB(GLhandleARB programObj);
void glUseProgramObjectARB(GLhandleARB programObj);
void glValidateProgramARB(GLhandleARB programObj);
void glGetObjectParameterfvARB(GLhandleARB obj, GLenum pname, GLfloat *params);
void glGetObjectParameterivARB(GLhandleARB obj, GLenum pname, GLint *params);
void glGetInfoLogARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
void glGetAttachedObjectsARB(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
GLint glGetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name);
void glGetActiveUniformARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
void glGetUniformfvARB(GLhandleARB programObj, GLint location, GLfloat *params);
void glGetUniformivARB(GLhandleARB programObj, GLint location, GLint *params);
void glGetShaderSourceARB(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
void glBindAttribLocationARB(GLhandleARB programObj, GLuint index, const GLcharARB *name);
void glGetActiveAttribARB(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GLint glGetAttribLocationARB(GLhandleARB programObj, const GLcharARB *name);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// LDD64 Linux: a window's default framebuffer is its GL view's offscreen framebuffer
// (LinuxShell.cpp): binding 0 binds it, and GL_BACK / GL_FRONT name its colour attachment.
void LinuxGLBindFramebuffer(GLenum target, GLuint framebuffer);
void LinuxGLReadBuffer(GLenum mode);
void LinuxGLDrawBuffer(GLenum mode);
#define glBindFramebufferEXT LinuxGLBindFramebuffer
#define glBindFramebuffer LinuxGLBindFramebuffer
#define glReadBuffer LinuxGLReadBuffer
#define glDrawBuffer LinuxGLDrawBuffer
#endif
