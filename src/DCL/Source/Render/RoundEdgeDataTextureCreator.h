// LEGO Digital Designer — restored source
// LEGO::RoundEdgeDataTextureCreator — packs a round-edge vertex buffer's data into float textures
// for the RE* vertex shaders, plus one UInt16 (x, y) texel pair per index for the vertex stream:
//   vertex texture         RGB32F (0x260): texel 2i = position i, texel 2i+1 = normal i;
//                          coordinates: x = (2*index) % width, y = ((2*index) / width << 14) / height
//   average normal texture RGB32F: one texel per round-edge point;
//                          coordinates normalised to 0..16384 (x / width, y / height)
//   round-edge data        RGBA32F (0x40000880, min width 64): 4 floats per texel;
//                          coordinates: x = (index/4) % width, y normalised to 0..16384
// Texture sizes are powers of two, at least the minimum, at most min(GL_MAX_TEXTURE_SIZE,
// 16384); a larger request is traced and marks the creator failed (all textures then read
// as null).
//
// TU 394 (Mac 0x37db30-0x37e96f, Win 0x505840-0x506240); TU 393 (Mac 0x37dab0-0x37db2f, inlined
// on Win): the two layout helpers at the end. Mac vtable 0x7542d8 {D1 0x37e8e0, D0 0x37e8f0},
// Win sdd 0x5058c0. Layout (both, sizeof 0x28): render +0x08, textures +0x0c / +0x10 / +0x14,
// coordinates +0x18 / +0x1c / +0x20, m_failed +0x24. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "Texture.h"

namespace LEGO {

class Render;

class RoundEdgeDataTextureCreator : public Object {
public:
    explicit RoundEdgeDataTextureCreator(Render* render);           // Mac 0x37db30, Win 0x505840
    virtual ~RoundEdgeDataTextureCreator();                          // Mac 0x37e7d0 (D1 0x37e8e0, D0 0x37e8f0), Win 0x506150

    // Replaces the vertex texture and its coordinates (the only one that is rebuilt: dynamic
    // buffers call it before every draw).
    void CreateVertexTexture(const Vector3* positions, const Vector3* normals, UInt32 vertexCount,
                             const UInt32* indices, UInt32 indexCount);       // Mac 0x37db90, Win 0x5058e0
    void CreateAverageNormalTexture(const Vector3* normals, UInt32 count,
                                    const UInt32* indices, UInt32 indexCount); // Mac 0x37e090, Win 0x505b50
    // `count` 32-bit values (4 per texel).
    void CreateRoundEdgeDataTexture(const UInt32* data, UInt32 count,
                                    const UInt32* indices, UInt32 indexCount); // Mac 0x37e4f0, Win 0x505d90

    Texture* GetVertexTexture() const { return m_failed ? 0 : m_vertexTexture.Get(); }                  // Mac 0x37e730, Win 0x505f90
    const UInt16* GetVertexCoordinates() const { return m_vertexCoordinates; }                          // Mac 0x37e750, Win 0x505fa0
    Texture* GetRoundEdgeDataTexture() const { return m_failed ? 0 : m_roundEdgeDataTexture.Get(); }    // Mac 0x37e760, Win 0x505fb0
    const UInt16* GetRoundEdgeDataCoordinates() const { return m_roundEdgeDataCoordinates; }            // Mac 0x37e780, Win 0x505fc0
    Texture* GetAverageNormalTexture() const { return m_failed ? 0 : m_averageNormalTexture.Get(); }    // Mac 0x37e790, Win 0x505fd0
    const UInt16* GetAverageNormalCoordinates() const { return m_averageNormalCoordinates; }            // Mac 0x37e7b0, Win 0x505fe0
    bool HasFailed() const { return m_failed; }                                                         // Mac 0x37e7c0, Win 0x505ff0

    struct Size { UInt32 width, height; };   // (Mac 0x1dc810/0x1dc830/0x1dc870 pair helpers)

    // Power-of-two texture size for `texels` texels. (Mac 0x37df20, Win 0x506000)
    Size ComputeTextureSize(UInt32 texels, const Size& minimum, UInt32 maxTextureSize);

private:
    RoundEdgeDataTextureCreator(const RoundEdgeDataTextureCreator&);
    RoundEdgeDataTextureCreator& operator=(const RoundEdgeDataTextureCreator&);

    Texture* CreateFloatTexture(const Size& size, UInt32 format, UInt32 bytesPerTexel,
                                const void* data, UInt32 dataSize);

    Render*             m_render;                     // +0x08 (not referenced)
    RefPointer<Texture> m_vertexTexture;              // +0x0c
    RefPointer<Texture> m_roundEdgeDataTexture;       // +0x10
    RefPointer<Texture> m_averageNormalTexture;       // +0x14
    UInt16*             m_vertexCoordinates;          // +0x18
    UInt16*             m_roundEdgeDataCoordinates;   // +0x1c
    UInt16*             m_averageNormalCoordinates;   // +0x20
    bool                m_failed;                     // +0x24
};

// TU 393 (names inferred). Minimum width of the round-edge data texture.
UInt32 GetRoundEdgeDataTextureMinWidth();                            // Mac 0x37dab0 (64)
// Rounds a byte size of round-edge data up to a whole number of 64-value blocks
// ((bytes / 4) rounded up to a multiple of 64, times 4).
UInt32 AlignRoundEdgeDataSize(UInt32 bytes);                         // Mac 0x37dac0 (caller 022c 0x60a50)

} // namespace LEGO
