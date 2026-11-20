// LEGO Digital Designer — restored source
// LEGO::OpenGLVertexBufferVBORoundEdge — buffer of the round-edge shaders (Shaders/RoundEdge/
// RE*.vsh), needs vertex texture fetch. Expanded per index: the VBO holds the vertex's 16-bit
// texel coordinates (as 2D position), the texture coordinates and the average-normal / round-edge
// texel coordinates (units 2, 3); positions, normals and round-edge data are float textures
// (RoundEdgeDataTextureCreator) on units 4-6. Derives from OpenGLVertexBuffer, whose CPU arrays
// keep the geometry. TU 395 (`__FILE__` Win 0x4ffb55 …): Mac 0x37e970-0x37fa9f, Win
// 0x4ff870-0x5004f0; made by OpenGLVertexBufferFactory::CreateObject (Mac 0xf7a70). Vtables Mac
// 0x75430c (+ Object 0x7543c4, VTT 0x7543e4). Object Mac +0x7c (sizeof 0x84). Names inferred.
#pragma once
#include "OpenGLVertexBuffer.h"
#include "RoundEdgeDataTextureCreator.h"

namespace LEGO {

class OpenGLVertexBufferVBORoundEdge : public OpenGLVertexBuffer {
public:
    // Mac 0x37e970, Win 0x4ff870
    OpenGLVertexBufferVBORoundEdge(OpenGLRender* render, UInt32 vertexCount, UInt32 indexCount,
                                   UInt32 roundEdgeDataCount, UInt32 roundEdgePointCount,
                                   UInt32 colorSetCount, bool dynamic);
    virtual ~OpenGLVertexBufferVBORoundEdge();                        // Mac 0x37f800 (D0 0x37f8d0), Win 0x500350

    virtual UInt32 GetRoundEdgePointCount() const;                    // 6  Mac 0x37ecd0, Win 0x4ffb80
    virtual UInt32 GetRoundEdgeDataCount() const;                     // 7  Mac 0x37ece0, Win 0x46e0b0
    virtual bool SetRoundEdgePoints(const Vector3* data, UInt32 count);       // 13 Mac 0x37f6f0, Win 0x500250
    virtual bool SetRoundEdgeData(const UInt32* data, UInt32 count);          // 14 Mac 0x37f720, Win 0x500280
    virtual bool SetRoundEdgePointIndices(const UInt32* data, UInt32 count);  // 15 Mac 0x37f750, Win 0x5002b0
    virtual bool SetRoundEdgeDataIndices(const UInt32* data, UInt32 count);   // 16 Mac 0x37f780, Win 0x5002e0
    virtual const void* GetRoundEdgePoints(UInt32& stride) const;     // 24 Mac 0x37f7b0, Win 0x500310
    virtual const void* GetRoundEdgeData() const;                     // 25 Mac 0x37f7d0, Win 0x500320
    virtual const void* GetRoundEdgePointIndices() const;             // 26 Mac 0x37f7e0, Win 0x500330
    virtual const void* GetRoundEdgeDataIndices() const;              // 27 Mac 0x37f7f0, Win 0x500340

    // OpenGLVertexBuffer::Create + the round-edge arrays (sized by the maximum index count) and
    // the VBO layout.
    virtual bool Create(bool hasNormals, const BasicArray<UInt32>& texCoordDimensions,
                        UInt32 frameCount);                           // 34 Mac 0x37ea70, Win 0x4ff9a0
    virtual void BindArrays();                                        // 35 Mac 0x37ecf0, Win 0x459790 (empty)
    // Builds the textures on first use (the vertex texture again for dynamic buffers), uploads
    // the per-index streams and binds the textures on units 4-6.
    virtual void Begin();                                             // 36 Mac 0x37ed00, Win 0x4ffb90
    virtual void End();                                               // 37 Mac 0x37f3c0, Win 0x4fffd0
    virtual void Draw();                                              // 38 Mac 0x37f410, Win 0x500010

    RoundEdgeDataTextureCreator* GetTextureCreator() const { return m_textureCreator; }

protected:
    virtual void OnFinalRelease();                                    // 39 Mac 0x37f950 (thunk 0x37fa40), Win 0x500400

    RefPointer<RoundEdgeDataTextureCreator> m_textureCreator;   // Mac +0x40
    UInt32*            m_roundEdgeData;            // Mac +0x44 dataCount values
    Vector3*           m_roundEdgePoints;          // Mac +0x48 pointCount average normals
    UInt32             m_roundEdgeDataCount;       // Mac +0x4c
    UInt32             m_roundEdgePointCount;      // Mac +0x50
    UInt32*            m_roundEdgePointIndices;    // Mac +0x54 one per index
    UInt32*            m_roundEdgeDataIndices;     // Mac +0x58 one per index
    UInt32             m_bufferId;                 // Mac +0x5c GL_ARRAY_BUFFER_ARB object
    UInt32             m_vertexCoordinateOffset;   // Mac +0x60 (0)
    UInt32             m_pointCoordinateOffset;    // Mac +0x64 (Win +0x68) average-normal coordinates, texture unit 3
    UInt32             m_dataCoordinateOffset;     // Mac +0x68 (Win +0x6c) round-edge-data coordinates, texture unit 2
    BasicArray<UInt32> m_texCoordOffsets;          // Mac +0x6c one per texture coordinate set
};

} // namespace LEGO
