// LEGO Digital Designer — restored source
// LEGO::OpenGLVertexBufferVBO — OpenGLVertexBuffer also kept in two ARB VBOs (attributes /
// indices); every Set* updates its range with glBufferSubDataARB. TU 039 (Mac 0xff290-0x100fbf,
// Win 0x4f9800-0x4fa590). Vtables Mac 0x70953c (+ Object 0x7095f4), Win 0xa274f4. Object Mac
// +0x60 (sizeof 0x68), Win +0x68 (0x70). Win ctor 0x4f9800: stdcall(this, dynamic), EDX render,
// ECX colorSetCount, EDI vertexCount, ESI indexCount; returns this. Member names inferred.
#pragma once
#include "OpenGLVertexBuffer.h"

namespace LEGO {

class OpenGLVertexBufferVBO : public OpenGLVertexBuffer {
public:
    // Mac 0xff290, Win 0x4f9800
    OpenGLVertexBufferVBO(OpenGLRender* render, UInt32 vertexCount, UInt32 indexCount,
                          UInt32 colorSetCount, bool dynamic);
    virtual ~OpenGLVertexBufferVBO() {}                               // Mac 0x100080/0x1001a0

    virtual bool SetPositions(const Vector3* data, UInt32 start, UInt32 count, UInt32 frame); // Mac 0xff8c0, Win 0x4f9cf0
    virtual bool SetPositions(const UInt32* data, UInt32 start, UInt32 count, UInt32 frame,
                              bool nativeOrder);                      // Mac 0xff9c0, Win 0x4f9de0
    virtual bool SetNormals(const Vector3* data, UInt32 start, UInt32 count); // Mac 0xffac0, Win 0x4f9ed0
    virtual bool SetNormals(const UInt32* data, bool nativeOrder);    // Mac 0xffbb0, Win 0x4f9fc0
    virtual bool SetColors(const Color* data, UInt32 start, UInt32 count, UInt32 set); // Mac 0xffc90, Win 0x4fa0a0
    virtual bool SetTexCoords(const Real32* data, UInt32 set);        // Mac 0xffd80, Win 0x4fa190
    virtual bool SetTexCoords(const UInt32* data, UInt32 set, bool nativeOrder); // Mac 0xffe70, Win 0x4fa270
    virtual bool SetIndices(const UInt32* data, UInt32 start, UInt32 count, UInt32 baseVertex); // Mac 0xfff60, Win 0x4fa360

    // Base Create, then lays out one vertex buffer {positions (frame 0 only), normals, colour
    // set 0.., texture coordinate sets} and an index buffer and allocates them (no data).
    virtual bool Create(bool hasNormals, const BasicArray<UInt32>& texCoordDimensions,
                        UInt32 frameCount);                           // Mac 0xff360, Win 0x4f98f0
    // Nothing: the arrays are set up by Draw.
    virtual void BindArrays();                                        // Mac 0xff5a0, Win 0x459790
    virtual void Draw();                                              // Mac 0xff5b0, Win 0x4f9aa0

protected:
    // Deletes the GL buffers (render made current), then the CPU arrays.
    virtual void OnFinalRelease();                                    // Mac 0x100250 (thunk 0x100300)

    UInt32             m_vertexBufferID;   // Mac +0x40, Win +0x44
    UInt32             m_indexBufferID;    // Mac +0x44
    UInt32             m_normalsOffset;    // Mac +0x48 byte offsets inside the vertex buffer
    UInt32             m_colorsOffset;     // Mac +0x4c
    BasicArray<UInt32> m_texCoordOffsets;  // Mac +0x50 one per texture coordinate set

private:
    OpenGLVertexBufferVBO(const OpenGLVertexBufferVBO&);
    OpenGLVertexBufferVBO& operator=(const OpenGLVertexBufferVBO&);
};

} // namespace LEGO
