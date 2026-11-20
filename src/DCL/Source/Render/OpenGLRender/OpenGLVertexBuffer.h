// LEGO Digital Designer — restored source
// LEGO::OpenGLVertexBuffer — VertexBuffer in client memory drawn with GL client arrays; base of
// the VBO and VBORoundEdge buffers. TU 037 (Mac 0xfdeb0-0xff24f, Win 0x4f8e90-0x4f97f0; also the
// VertexBuffer D1/D0 Mac 0xff140/0xff160). Names inferred.
// Vtables: Mac 0x70944c (34 VertexBuffer slots, Create 34, BindArrays 35, Begin 36, End 37, Draw
// 38, OnFinalRelease 39) + Object 0x709504; Win 0xa273fc (Create 32 .. Draw 36) + Object 0xa27494.
// Mac sizeof 0x48 (Object +0x40), Win 0x50 (vbptr +4, members +4, Object +0x48). Win ctor
// 0x4f8f20: stdcall(this, render, colorSetCount, dynamic, mostDerived), EDI vertexCount,
// ESI indexCount.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "VertexBuffer.h"

namespace LEGO {

class OpenGLRender;

class OpenGLVertexBuffer : public VertexBuffer {
public:
    // Mac 0xfdf40 (complete) / 0xfdeb0 (base-object ctor with VTT), Win 0x4f8f20.
    // Nothing is allocated before Create. The render is not referenced.
    OpenGLVertexBuffer(OpenGLRender* render, UInt32 vertexCount, UInt32 indexCount,
                       UInt32 colorSetCount, bool dynamic);
    virtual ~OpenGLVertexBuffer() {}                                  // Mac 0xfef80/0xfefc0

    // ---- VertexBuffer (Mac slot / Win slot) ----
    virtual UInt32 GetVertexCount() const;                            // 0 Mac 0xfe380, Win 0x42c9c0
    virtual void   SetVertexCount(UInt32 count);                      // 1 Mac 0xfe390, Win 0x4f9310
    virtual UInt32 GetMaxVertexCount() const;                         // 2 Mac 0xfe3a0, Win 0x4f9320
    virtual UInt32 GetIndexCount() const;                             // 3 Mac 0xfe3b0, Win 0x4f9330
    virtual void   SetIndexCount(UInt32 count);                       // 4 Mac 0xfe3c0, Win 0x4f9340
    virtual UInt32 GetMaxIndexCount() const;                          // 5 Mac 0xfe3d0, Win 0x46f470
    virtual UInt32 GetRoundEdgePointCount() const;                    // 6 Mac 0xfe3f0, Win 0x51e7a0
    virtual UInt32 GetRoundEdgeDataCount() const;                     // 7 Mac 0xfe400, Win 0x51e7a0
    virtual UInt32 GetFrameCount() const;                             // 8 Mac 0xfe3e0, Win 0x46f460
    virtual bool SetPositions(const Vector3* data, UInt32 start, UInt32 count, UInt32 frame); // 9/10 Mac 0xfe410, Win 0x4f9350
    virtual bool SetPositions(const UInt32* data, UInt32 start, UInt32 count, UInt32 frame,
                              bool nativeOrder);                      // 10/9 Mac 0xfe450, Win 0x4f9390
    virtual bool SetNormals(const Vector3* data, UInt32 start, UInt32 count); // 11/12 Mac 0xfe6f0, Win 0x4f9430
    virtual bool SetNormals(const UInt32* data, bool nativeOrder);    // 12/11 Mac 0xfe730, Win 0x4f9460
    virtual bool SetRoundEdgePoints(const Vector3* data, UInt32 count);       // 13 Mac 0xfe990, Win 0x4a9530
    virtual bool SetRoundEdgeData(const UInt32* data, UInt32 count);          // 14 Mac 0xfe9a0
    virtual bool SetRoundEdgePointIndices(const UInt32* data, UInt32 count);  // 15 Mac 0xfe9b0
    virtual bool SetRoundEdgeDataIndices(const UInt32* data, UInt32 count);   // 16 Mac 0xfe9c0
    virtual bool SetColors(const Color* data, UInt32 start, UInt32 count, UInt32 set); // 17 Mac 0xfe9d0, Win 0x4f94e0
    virtual UInt32 GetTexCoordSetCount() const;                       // 18 Mac 0xfea10, Win 0x46ee40
    virtual bool SetTexCoords(const Real32* data, UInt32 set);        // 19/20 Mac 0xfea20, Win 0x4f9510
    virtual bool SetTexCoords(const UInt32* data, UInt32 set, bool nativeOrder); // 20/19 Mac 0xfea60, Win 0x4f9540
    virtual bool SetIndices(const UInt32* data, UInt32 start, UInt32 count, UInt32 baseVertex); // 21 Mac 0xfec90, Win 0x4f95d0
    virtual const void* GetPositions(UInt32& stride, UInt32 frame) const; // 22 Mac 0xfee70, Win 0x4f9640
    virtual const void* GetNormals(UInt32& stride) const;            // 23 Mac 0xfeea0, Win 0x4f9660
    virtual const void* GetRoundEdgePoints(UInt32& stride) const;    // 24 Mac 0xfeec0, Win 0x4f9670
    virtual const void* GetRoundEdgeData() const;                     // 25 Mac 0xfeed0, Win 0x51e7a0
    virtual const void* GetRoundEdgePointIndices() const;             // 26 Mac 0xfeee0
    virtual const void* GetRoundEdgeDataIndices() const;              // 27 Mac 0xfeef0
    virtual const void* GetTexCoords(UInt32& stride, UInt32 set) const; // 28 Mac 0xfef00, Win 0x4f9680
    virtual const void* GetColors(UInt32& stride) const;              // 29 Mac 0xfef20, Win 0x4f96a0
    virtual const UInt32* GetIndices() const;                         // 30 Mac 0xfef40, Win 0x4f96b0
    virtual void SetPrimitiveType(UInt32 type);                       // 31 Mac 0xfef50, Win 0x4f96c0

    // ---- OpenGLVertexBuffer (Mac 34-38, Win 32-36) ----
    // Allocates the CPU arrays: positions for max(frameCount, 1) frames, normals when
    // `hasNormals`, colours for the colour sets, one texture coordinate set per entry of
    // `texCoordDimensions`, indices. False when the vertex or index count is 0.
    virtual bool Create(bool hasNormals, const BasicArray<UInt32>& texCoordDimensions,
                        UInt32 frameCount);                           // 34 Mac 0xfdfe0, Win 0x4f9040
    // Sets the client array pointers (Render::SetVertexBuffer).
    virtual void BindArrays();                                        // 35 Mac 0xfe210, Win 0x4f9200
    // Around a draw (Render::BeginVertexBuffer / EndVertexBuffer); empty here.
    virtual void Begin();                                             // 36 Mac 0xfe330, Win 0x459790
    virtual void End();                                               // 37 Mac 0xfe340, Win 0x459790
    // OpenGLRender::ApplyMaterial + glDrawElements (Render::DrawVertexBuffer).
    virtual void Draw();                                              // 38 Mac 0xfe350, Win 0x4f92e0

    OpenGLRender* GetRender() const { return m_render; }

protected:
    // Frees the CPU arrays before the object goes away.
    virtual void OnFinalRelease();                                    // 39 Mac 0xff050 (thunk 0xff120)

    // One texture coordinate set: `dimension` floats per vertex. Allocated with new[] (Mac array
    // cookie: count at -4), dimension 2 until Create sets it. (name inferred)
    struct TexCoordSet {
        TexCoordSet() : data(0), dimension(2) {}
        ~TexCoordSet()
        {
            if (data)
                delete[] data;
            data = 0;
        }
        Real32* data;        // +0x00
        UInt32  dimension;   // +0x04
    };

    OpenGLRender* m_render;              // Mac +0x04, Win +0x08
    UInt32        m_texCoordSetCount;    // Mac +0x08
    UInt32        m_vertexCount;         // Mac +0x0c vertices in use (SetVertexCount)
    UInt32        m_maxVertexCount;      // Mac +0x10 allocated
    UInt32        m_colorSetCount;       // Mac +0x14
    UInt32        m_indexCount;          // Mac +0x18 indices drawn
    UInt32        m_maxIndexCount;       // Mac +0x1c allocated
    UInt32        m_frameCount;          // Mac +0x20
    Real32*       m_positions;           // Mac +0x24 frames x maxVertexCount x 3
    Real32*       m_normals;             // Mac +0x28 maxVertexCount x 3 or null
    Real32*       m_colors;              // Mac +0x2c sets x maxVertexCount x 4 or null
    TexCoordSet*  m_texCoords;           // Mac +0x30 m_texCoordSetCount entries or null
    UInt32*       m_indices;             // Mac +0x34
    bool          m_dynamic;             // Mac +0x38 (VBO usage GL_DYNAMIC_DRAW)
    UInt32        m_primitive;           // Mac +0x3c GL mode (GL_TRIANGLES = 4 initially)

private:
    OpenGLVertexBuffer(const OpenGLVertexBuffer&);
    OpenGLVertexBuffer& operator=(const OpenGLVertexBuffer&);
};

} // namespace LEGO
