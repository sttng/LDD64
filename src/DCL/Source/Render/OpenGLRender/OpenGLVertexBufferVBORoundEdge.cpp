// LEGO Digital Designer — restored source
// OpenGLVertexBufferVBORoundEdge.cpp (TU 395; `__FILE__`
// "…/DCL/Source/Render/OpenGLRender/OpenGLVertexBufferVBORoundEdge.cpp").
#include "OpenGLVertexBufferVBORoundEdge.h"
#include "OpenGLRender.h"
#include "OpenGLExtensions.h"
#include "OpenGLFactories.h"
#include "OpenGLWindow.h"
#include <string.h>

namespace LEGO {

// Mac 0x37e970, Win 0x4ff870
OpenGLVertexBufferVBORoundEdge::OpenGLVertexBufferVBORoundEdge(
    OpenGLRender* render, UInt32 vertexCount, UInt32 indexCount, UInt32 roundEdgeDataCount,
    UInt32 roundEdgePointCount, UInt32 colorSetCount, bool dynamic)
    : OpenGLVertexBuffer(render, vertexCount, indexCount, colorSetCount, dynamic),
      m_roundEdgeData(0), m_roundEdgePoints(0), m_roundEdgeDataCount(roundEdgeDataCount),
      m_roundEdgePointCount(roundEdgePointCount), m_roundEdgePointIndices(0),
      m_roundEdgeDataIndices(0), m_bufferId(0), m_vertexCoordinateOffset(0),
      m_pointCoordinateOffset(0), m_dataCoordinateOffset(0)
{
}

// Mac 0x37f800 (D0 0x37f8d0), Win 0x500350
OpenGLVertexBufferVBORoundEdge::~OpenGLVertexBufferVBORoundEdge()
{
}

// Mac 0x37ecd0, Win 0x4ffb80
UInt32 OpenGLVertexBufferVBORoundEdge::GetRoundEdgePointCount() const
{
    return m_roundEdgePointCount;
}

// Mac 0x37ece0, Win 0x46e0b0
UInt32 OpenGLVertexBufferVBORoundEdge::GetRoundEdgeDataCount() const
{
    return m_roundEdgeDataCount;
}

// Mac 0x37f6f0, Win 0x500250
bool OpenGLVertexBufferVBORoundEdge::SetRoundEdgePoints(const Vector3* data, UInt32 count)
{
    memcpy((void*)m_roundEdgePoints, data, count * 12);
    return true;
}

// Mac 0x37f720, Win 0x500280
bool OpenGLVertexBufferVBORoundEdge::SetRoundEdgeData(const UInt32* data, UInt32 count)
{
    memcpy(m_roundEdgeData, data, count * 4);
    return true;
}

// Mac 0x37f750, Win 0x5002b0
bool OpenGLVertexBufferVBORoundEdge::SetRoundEdgePointIndices(const UInt32* data, UInt32 count)
{
    memcpy(m_roundEdgePointIndices, data, count * 4);
    return true;
}

// Mac 0x37f780, Win 0x5002e0
bool OpenGLVertexBufferVBORoundEdge::SetRoundEdgeDataIndices(const UInt32* data, UInt32 count)
{
    memcpy(m_roundEdgeDataIndices, data, count * 4);
    return true;
}

// Mac 0x37f7b0, Win 0x500310
const void* OpenGLVertexBufferVBORoundEdge::GetRoundEdgePoints(UInt32& stride) const
{
    stride = 12;
    return m_roundEdgePoints;
}

// Mac 0x37f7d0, Win 0x500320
const void* OpenGLVertexBufferVBORoundEdge::GetRoundEdgeData() const
{
    return m_roundEdgeData;
}

// Mac 0x37f7e0, Win 0x500330
const void* OpenGLVertexBufferVBORoundEdge::GetRoundEdgePointIndices() const
{
    return m_roundEdgePointIndices;
}

// Mac 0x37f7f0, Win 0x500340
const void* OpenGLVertexBufferVBORoundEdge::GetRoundEdgeDataIndices() const
{
    return m_roundEdgeDataIndices;
}

namespace {

// new[] of `count` 32-bit values with the original's overflow saturation.
void* AllocWords(UInt32 count)
{
    return operator new[](count > 0x3fffffffu ? 0xffffffffu : count * 4);
}

} // namespace

// Mac 0x37ea70, Win 0x4ff9a0
bool OpenGLVertexBufferVBORoundEdge::Create(bool hasNormals,
                                            const BasicArray<UInt32>& texCoordDimensions,
                                            UInt32 frameCount)
{
    LDD_GL_TRY
        if (!OpenGLVertexBuffer::Create(hasNormals, texCoordDimensions, frameCount))
            return false;

        if (m_roundEdgeDataCount != 0) {
            m_roundEdgeData = (UInt32*)AllocWords(m_roundEdgeDataCount);
            m_roundEdgeDataIndices = (UInt32*)AllocWords(m_maxIndexCount);
        }
        if (m_roundEdgePointCount != 0) {
            m_roundEdgePoints = (Vector3*)operator new[](m_roundEdgePointCount * 12);
            m_roundEdgePointIndices = (UInt32*)AllocWords(m_maxIndexCount);
        }

        // One 4-byte entry per index for every stream: vertex texel coordinates first, then the
        // texture coordinate sets, the round-edge-data and the average-normal coordinates.
        m_vertexCoordinateOffset = 0;
        UInt32 offset = m_maxIndexCount * 4;
        if (m_texCoordSetCount != 0) {
            while (m_texCoordOffsets.GetSize() > m_texCoordSetCount)
                m_texCoordOffsets.PopBack();
            if (m_texCoordOffsets.GetSize() < m_texCoordSetCount)
                m_texCoordOffsets.Insert(m_texCoordOffsets.End(),
                                         m_texCoordSetCount - m_texCoordOffsets.GetSize(), 0);
            for (UInt32 i = 0; i < m_texCoordSetCount; ++i) {
                m_texCoordOffsets[i] = offset;
                offset += m_texCoords[i].dimension * m_maxIndexCount * 4;
            }
        }
        m_dataCoordinateOffset = offset;
        offset += m_maxIndexCount * 4;
        m_pointCoordinateOffset = offset;

        glGenBuffersARB(1, (GLuint*)&m_bufferId);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_bufferId);
        glBufferDataARB(GL_ARRAY_BUFFER_ARB, offset + m_maxIndexCount * 4, 0,
                        m_dynamic ? GL_DYNAMIC_DRAW_ARB : GL_STATIC_DRAW_ARB);
        if (glGetError() != GL_NO_ERROR)
            return false;   // NOTE: the buffer stays bound
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        return true;
    LDD_GL_CATCH
    return true;   // (after a reported crash, as the original: Mac 0x37eca6, Win 0x4ffb3f)
}

// Mac 0x37ecf0, Win 0x459790
void OpenGLVertexBufferVBORoundEdge::BindArrays()
{
}

// Mac 0x37ed00, Win 0x4ffb90. Only the buffer uploads are guarded, each on its own; after a
// reported crash the next statement runs (the texture creation and SetTexture are unguarded).
void OpenGLVertexBufferVBORoundEdge::Begin()
{
    if (m_textureCreator) {
        if (m_dynamic) {
            // Positions/normals may have changed: rebuild the vertex texture only.
            m_textureCreator->CreateVertexTexture((const Vector3*)m_positions,
                                                  (const Vector3*)m_normals, m_vertexCount,
                                                  m_indices, m_indexCount);
            if (m_textureCreator->HasFailed())
                return;
            LDD_GL_TRY
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_bufferId);
            glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_vertexCoordinateOffset, m_maxIndexCount * 4,
                               m_textureCreator->GetVertexCoordinates());
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
            LDD_GL_CATCH
        }
    } else {
        m_textureCreator = new RoundEdgeDataTextureCreator(m_render);
        m_textureCreator->CreateVertexTexture((const Vector3*)m_positions,
                                              (const Vector3*)m_normals, m_vertexCount,
                                              m_indices, m_indexCount);
        m_textureCreator->CreateAverageNormalTexture(m_roundEdgePoints, m_roundEdgePointCount,
                                                     m_roundEdgePointIndices, m_maxIndexCount);
        m_textureCreator->CreateRoundEdgeDataTexture(m_roundEdgeData, m_roundEdgeDataCount,
                                                     m_roundEdgeDataIndices, m_maxIndexCount);
        if (m_textureCreator->HasFailed())
            return;

        // NOTE: m_maxIndexCount * 4 bytes are uploaded although the creator wrote only
        // m_indexCount coordinate pairs for the vertex texture.
        LDD_GL_TRY
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_bufferId);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_vertexCoordinateOffset, m_maxIndexCount * 4,
                           m_textureCreator->GetVertexCoordinates());
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        LDD_GL_CATCH
        LDD_GL_TRY
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_bufferId);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_dataCoordinateOffset, m_maxIndexCount * 4,
                           m_textureCreator->GetRoundEdgeDataCoordinates());
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        LDD_GL_CATCH
        LDD_GL_TRY
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_bufferId);
        glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_pointCoordinateOffset, m_maxIndexCount * 4,
                           m_textureCreator->GetAverageNormalCoordinates());
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
        LDD_GL_CATCH

        // Texture coordinates expanded per index.
        for (UInt32 set = 0; set < m_texCoordSetCount; ++set) {
            UInt32 count = m_indexCount;
            UInt32 dimension = m_texCoords[set].dimension;
            UInt32 bytes = dimension * 4;
            // NOTE: four times the needed size is allocated (Mac 0x37ef40).
            UInt8* expanded = (UInt8*)AllocWords(count * bytes);
            for (UInt32 i = 0; i < count; ++i)
                memcpy(expanded + dimension * i * 4,
                       (const UInt8*)m_texCoords[set].data + dimension * m_indices[i] * 4, bytes);
            LDD_GL_TRY
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_bufferId);
            glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, m_texCoordOffsets[set], bytes * m_indexCount,
                               expanded);
            glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
            LDD_GL_CATCH
            operator delete[](expanded);
        }
    }

    Render* render = m_render;
    render->SetTexture(m_textureCreator->GetVertexTexture(), 4);
    render->SetTexture(m_textureCreator->GetAverageNormalTexture(), 5);
    render->SetTexture(m_textureCreator->GetRoundEdgeDataTexture(), 6);
}

// Mac 0x37f3c0, Win 0x4fffd0
void OpenGLVertexBufferVBORoundEdge::End()
{
    Render* render = m_render;
    render->SetTexture(0, 4);
    render->SetTexture(0, 5);
    render->SetTexture(0, 6);
}

// Mac 0x37f410, Win 0x500010 (body 0x500020-0x500224)
void OpenGLVertexBufferVBORoundEdge::Draw()
{
    LDD_GL_TRY
        if (m_textureCreator->HasFailed())
            return;

        glEnableClientState(GL_VERTEX_ARRAY);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_bufferId);
        glDisable(GL_COLOR_MATERIAL);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        if (m_texCoordSetCount == 0) {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
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
        glClientActiveTextureARB(GL_TEXTURE2_ARB);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_SHORT, 0, (const GLvoid*)(size_t)m_dataCoordinateOffset);
        glClientActiveTextureARB(GL_TEXTURE3_ARB);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_SHORT, 0, (const GLvoid*)(size_t)m_pointCoordinateOffset);
        glVertexPointer(2, GL_SHORT, 0, (const GLvoid*)(size_t)m_vertexCoordinateOffset);

        m_render->ApplyMaterial();
        glDrawArrays(m_primitive, 0, m_indexCount);

        glClientActiveTextureARB(GL_TEXTURE3_ARB);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glClientActiveTextureARB(GL_TEXTURE2_ARB);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glClientActiveTextureARB(GL_TEXTURE0_ARB);
        glDisableClientState(GL_VERTEX_ARRAY);
        glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
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

// Mac 0x37f950 (thunk 0x37fa40), Win 0x500400
void OpenGLVertexBufferVBORoundEdge::OnFinalRelease()
{
    LDD_GL_TRY
        m_render->MakeCurrent();
        if (m_roundEdgeData) {
            operator delete[](m_roundEdgeData);
            m_roundEdgeData = 0;
        }
        if (m_roundEdgePoints) {
            operator delete[](m_roundEdgePoints);
            m_roundEdgePoints = 0;
        }
        if (m_roundEdgeDataIndices) {
            operator delete[](m_roundEdgeDataIndices);
            m_roundEdgeDataIndices = 0;
        }
        if (m_roundEdgePointIndices) {
            operator delete[](m_roundEdgePointIndices);
            m_roundEdgePointIndices = 0;
        }
        if (m_bufferId != 0)
            glDeleteBuffersARB(1, (GLuint*)&m_bufferId);
        OpenGLVertexBuffer::OnFinalRelease();
    LDD_GL_CATCH
}

// OpenGLFactories.h hook: the RoundEdge branch of OpenGLVertexBufferFactory::CreateObject
// (Mac 0xf7ad2-0xf7b60: operator new(0x84), ctor 0x37e970, Create = slot 34).
VertexBuffer* CreateOpenGLVertexBufferVBORoundEdge(OpenGLRender* render,
                                                   const VertexBuffer::CreateArgsRoundEdge& args)
{
    OpenGLVertexBufferVBORoundEdge* buffer = new OpenGLVertexBufferVBORoundEdge(
        render, args.vertexCount, args.indexCount, args.roundEdgeDataCount,
        args.roundEdgePointCount, args.colorSetCount, args.dynamic);
    if (!buffer->Create(args.hasNormals, args.texCoordDimensions, args.frameCount)) {
        static_cast<Object*>(buffer)->Release();
        return 0;
    }
    return buffer;
}

} // namespace LEGO
