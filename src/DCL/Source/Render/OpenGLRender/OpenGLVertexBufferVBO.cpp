// LEGO Digital Designer — restored source
// LEGO::OpenGLVertexBufferVBO — see OpenGLVertexBufferVBO.h. TU 039 (Mac 0xff290-0x100fbf,
// Win 0x4f9800-0x4fa590).
// NOTE: the Win build wraps Create/Draw in the OpenGL crash guard (catch → Win 0x51cd0f with
// __FILE__ "..\..\..\Source\Render\OpenGLRender\OpenGLVertexBufferVBO.cpp" and the line); it
// only reports access violations inside the driver (/EHa) and is not reproduced here.
#include "OpenGLVertexBufferVBO.h"
#include "OpenGLRender.h"
#include "OpenGLCrashReport.h"
#include "OpenGLFactories.h"

namespace LEGO {

// Mac 0xff290, Win 0x4f9800
OpenGLVertexBufferVBO::OpenGLVertexBufferVBO(OpenGLRender* render, UInt32 vertexCount,
                                             UInt32 indexCount, UInt32 colorSetCount, bool dynamic)
    : OpenGLVertexBuffer(render, vertexCount, indexCount, colorSetCount, dynamic),
      m_vertexBufferID(0), m_indexBufferID(0), m_normalsOffset(0), m_colorsOffset(0)
{
}

// Mac 0xff360, Win 0x4f98f0.
// Vertex buffer layout: positions (one frame: further animation frames are never uploaded),
// normals, colour set 0 (only one set fits: size 16 bytes x vertices), texture coordinate sets.
bool OpenGLVertexBufferVBO::Create(bool hasNormals, const BasicArray<UInt32>& texCoordDimensions,
                                   UInt32 frameCount)
{
    LDD_GL_TRY
    if (!OpenGLVertexBuffer::Create(hasNormals, texCoordDimensions, frameCount))
        return false;
    UInt32 vertices = m_maxVertexCount;
    UInt32 size = vertices * 12;
    if (m_normals != 0) {
        m_normalsOffset = size;
        size = vertices * 24;
    }
    if (m_colors != 0) {
        m_colorsOffset = size;
        size += vertices * 16;
    }
    if (m_texCoordSetCount != 0) {
        m_texCoordOffsets.Resize(m_texCoordSetCount, 0);
        for (UInt32 i = 0; i < m_texCoordSetCount; ++i) {
            m_texCoordOffsets[i] = size;
            size += m_texCoords[i].dimension * m_maxVertexCount * 4;
        }
    }
    GLenum usage = m_dynamic ? GL_DYNAMIC_DRAW_ARB : GL_STATIC_DRAW_ARB;
    glGenBuffersARB(1, (GLuint*)&m_vertexBufferID);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBufferID);
    glBufferDataARB(GL_ARRAY_BUFFER_ARB, size, 0, usage);
    if (glGetError() != GL_NO_ERROR)
        return false;
    glGenBuffersARB(1, (GLuint*)&m_indexBufferID);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_indexBufferID);
    glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_maxIndexCount * 4, 0, usage);
    if (glGetError() != GL_NO_ERROR)
        return false;
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xff5a0, Win 0x459790 (ICF)
void OpenGLVertexBufferVBO::BindArrays()
{
}

// Mac 0xff5b0, Win 0x4f9aa0
void OpenGLVertexBufferVBO::Draw()
{
    LDD_GL_TRY
    glEnableClientState(GL_VERTEX_ARRAY);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBufferID);
    if (m_normals == 0) {
        glDisableClientState(GL_NORMAL_ARRAY);
    } else {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, (const GLvoid*)(size_t)m_normalsOffset);
    }
    if (m_colors == 0) {
        glDisable(GL_COLOR_MATERIAL);
        glDisableClientState(GL_COLOR_ARRAY);
    } else {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, 0, (const GLvoid*)(size_t)m_colorsOffset);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);
    }
    if (m_texCoordSetCount == 0) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    } else if (m_texCoordSetCount == 1) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(m_texCoords[0].dimension, GL_FLOAT, 0,
                          (const GLvoid*)(size_t)m_texCoordOffsets[0]);
    } else {
        glClientActiveTextureARB(GL_TEXTURE0_ARB);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(m_texCoords[0].dimension, GL_FLOAT, 0,
                          (const GLvoid*)(size_t)m_texCoordOffsets[0]);
        for (UInt32 i = 1; i < m_texCoordSetCount; ++i) {
            glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glTexCoordPointer(m_texCoords[i].dimension, GL_FLOAT, 0,
                              (const GLvoid*)(size_t)m_texCoordOffsets[i]);
        }
    }
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_indexBufferID);
    m_render->ApplyMaterial();
    glDrawElements(m_primitive, m_indexCount, GL_UNSIGNED_INT, 0);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    if (m_texCoordSetCount != 0) {
        if (m_texCoordSetCount != 1) {
            for (UInt32 i = 1; i < m_texCoordSetCount; ++i) {
                glClientActiveTextureARB(GL_TEXTURE0_ARB + i);
                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            }
            glClientActiveTextureARB(GL_TEXTURE0_ARB);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    LDD_GL_CATCH
}

// Mac 0xff8c0, Win 0x4f9cf0.
// NOTE: uploads the frame-0 range [start, start + count) whatever `frame` is (only frame 0
// lives in the GPU buffer).
bool OpenGLVertexBufferVBO::SetPositions(const Vector3* data, UInt32 start, UInt32 count, UInt32 frame)
{
    LDD_GL_TRY
    bool ok = OpenGLVertexBuffer::SetPositions(data, start, count, frame);
    if (ok) {
        m_render->MakeCurrent();
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBufferID);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, start * 12, count * 12, m_positions + start * 3);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
    return ok;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xff9c0, Win 0x4f9de0 (same NOTE)
bool OpenGLVertexBufferVBO::SetPositions(const UInt32* data, UInt32 start, UInt32 count, UInt32 frame,
                                         bool nativeOrder)
{
    LDD_GL_TRY
    bool ok = OpenGLVertexBuffer::SetPositions(data, start, count, frame, nativeOrder);
    if (ok) {
        m_render->MakeCurrent();
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBufferID);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, start * 12, count * 12, m_positions + start * 3);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
    return ok;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xffac0, Win 0x4f9ed0
bool OpenGLVertexBufferVBO::SetNormals(const Vector3* data, UInt32 start, UInt32 count)
{
    LDD_GL_TRY
    if (!OpenGLVertexBuffer::SetNormals(data, start, count))
        return false;
    m_render->MakeCurrent();
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBufferID);
    glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_normalsOffset + start * 12, count * 12,
                       m_normals + start * 3);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xffbb0, Win 0x4f9fc0
bool OpenGLVertexBufferVBO::SetNormals(const UInt32* data, bool nativeOrder)
{
    LDD_GL_TRY
    bool ok = OpenGLVertexBuffer::SetNormals(data, nativeOrder);
    if (ok) {
        m_render->MakeCurrent();
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBufferID);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_normalsOffset, m_vertexCount * 12, m_normals);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
    return ok;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xffc90, Win 0x4fa0a0.
// NOTE: uploads the set-0 range whatever `set` is (the GPU buffer has room for one set).
bool OpenGLVertexBufferVBO::SetColors(const Color* data, UInt32 start, UInt32 count, UInt32 set)
{
    LDD_GL_TRY
    if (!OpenGLVertexBuffer::SetColors(data, start, count, set))
        return false;
    m_render->MakeCurrent();
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBufferID);
    glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_colorsOffset + start * 16, count * 16,
                       m_colors + start * 4);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xffd80, Win 0x4fa190
bool OpenGLVertexBufferVBO::SetTexCoords(const Real32* data, UInt32 set)
{
    LDD_GL_TRY
    bool ok = OpenGLVertexBuffer::SetTexCoords(data, set);
    if (ok) {
        m_render->MakeCurrent();
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBufferID);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_texCoordOffsets[set],
                           m_vertexCount * m_texCoords[set].dimension * 4, m_texCoords[set].data);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
    return ok;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xffe70, Win 0x4fa270
bool OpenGLVertexBufferVBO::SetTexCoords(const UInt32* data, UInt32 set, bool nativeOrder)
{
    LDD_GL_TRY
    bool ok = OpenGLVertexBuffer::SetTexCoords(data, set, nativeOrder);
    if (ok) {
        m_render->MakeCurrent();
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vertexBufferID);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_texCoordOffsets[set],
                           m_vertexCount * m_texCoords[set].dimension * 4, m_texCoords[set].data);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    }
    return ok;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0xfff60, Win 0x4fa360
bool OpenGLVertexBufferVBO::SetIndices(const UInt32* data, UInt32 start, UInt32 count, UInt32 baseVertex)
{
    LDD_GL_TRY
    if (!OpenGLVertexBuffer::SetIndices(data, start, count, baseVertex))
        return false;
    m_render->MakeCurrent();
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, m_indexBufferID);
    glBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, start * 4, count * 4, m_indices + start);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original)
}

// Mac 0x100250 (Object-in thunk 0x100300)
void OpenGLVertexBufferVBO::OnFinalRelease()
{
    m_render->MakeCurrent();
    LDD_GL_TRY
    if (m_vertexBufferID != 0)
        glDeleteBuffersARB(1, (const GLuint*)&m_vertexBufferID);
    if (m_indexBufferID != 0)
        glDeleteBuffersARB(1, (const GLuint*)&m_indexBufferID);
    OpenGLVertexBuffer::OnFinalRelease();
    LDD_GL_CATCH
}

// OpenGLFactories.h seam: the VBO branch of OpenGLVertexBufferFactory::CreateObject
// (Mac 0xf7a70, Win 0x4ebe30).
VertexBuffer* CreateOpenGLVertexBufferVBO(OpenGLRender* render, const VertexBuffer::CreateArgs& args)
{
    OpenGLVertexBufferVBO* buffer = new OpenGLVertexBufferVBO(render, args.vertexCount,
                                                              args.indexCount, args.colorSetCount,
                                                              args.dynamic);
    if (!buffer->Create(args.hasNormals, args.texCoordDimensions, args.frameCount)) {
        buffer->Release();
        return 0;
    }
    return buffer;
}

} // namespace LEGO
