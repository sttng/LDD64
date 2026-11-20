// LEGO Digital Designer — restored source
// LEGO::VertexBuffer — abstract vertex/index buffer; CreateArgs(+RoundEdge). Header-only: D1/D0
// in TU 037 (Mac 0xff140/0xff160), argument blocks' in 021a (0x2f910/0x529b0) and 021g
// (0x50a40/0x56990). OpenGLVertexBufferFactory::CreateObject (Mac 0xf7a70, Win 0x4ebe30) picks the
// VBORoundEdge / VBO / client-array class, then calls Create (slot 34).
//
// Vtables: Mac 0x70971c (34 slots: 32 + D1/D0 last), Win 0xa27364 (32 slots). MSVC reversed
// three overload pairs: Mac 9/10 = Win 10/9 (SetPositions), Mac 11/12 = Win 12/11
// (SetNormals), Mac 19/20 = Win 20/19 (SetTexCoords); every other slot has the same number.
// Layout: Mac {vptr, virtual base Object +4} (no members); Win {vptr, vbptr, Object +8}.
// OpenGLVertexBuffer's members start at Mac +0x04 / Win +0x08 (all field offsets Win = Mac + 4).
// Slot names inferred from the OpenGLVertexBuffer (Mac 0xfe380-0xfef50) and TU 395 bodies.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ObjectFactory.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Color.h"

namespace LEGO {

class VertexBuffer : public virtual Object {
public:
    // Mac vtable 0x777c68 {D1 0x2f910, D0 0x529b0}, Win sdd 0x50ce65. Mac sizeof 0x28.
    // Typical user (RenderGrid Mac 0x1d2bb0..): {4, 4, 0, dynamic, false, 0, {2}}.
    struct CreateArgs : public ObjectFactoryArgs {
        CreateArgs()
            : vertexCount(0), indexCount(0), frameCount(0), dynamic(false), hasNormals(false),
              colorSetCount(0) {}
        virtual ~CreateArgs() {}
        UInt32             vertexCount;          // +0x04
        UInt32             indexCount;           // +0x08
        UInt32             frameCount;           // +0x0c (0 = 1; positions/colours per frame)
        bool               dynamic;              // +0x10 true: client arrays even with VBOs
        bool               hasNormals;           // +0x11
        UInt32             colorSetCount;        // +0x14 (0: no colour array)
        BasicArray<UInt32> texCoordDimensions;   // +0x18 one entry per texture coordinate set;
                                                 // more than one needs ARB_multitexture
    };

    // Round-edge geometry (OpenGLVertexBufferVBORoundEdge, needs vertex texture fetch:
    // Render slot 69). Mac vtable 0x7789c8 {D1 0x50a40, D0 0x56990}. Mac sizeof 0x30.
    struct CreateArgsRoundEdge : public CreateArgs {
        CreateArgsRoundEdge() : roundEdgeDataCount(0), roundEdgePointCount(0) {}
        virtual ~CreateArgsRoundEdge() {}
        UInt32 roundEdgeDataCount;    // +0x28 (4-byte records, VB +0x4c)
        UInt32 roundEdgePointCount;   // +0x2c (Vector3 records, VB +0x50)
    };

    // SetPrimitiveType values index a GL mode table (Mac 0x763d80). (values not yet mapped)

    // ---- Mac slots 0-8 (Win same) ----
    virtual UInt32 GetVertexCount() const = 0;                        // 0  (+0x0c) vertices in use
    virtual void   SetVertexCount(UInt32 count) = 0;                  // 1
    virtual UInt32 GetMaxVertexCount() const = 0;                     // 2  (+0x10) allocated
    virtual UInt32 GetIndexCount() const = 0;                         // 3  (+0x18)
    virtual void   SetIndexCount(UInt32 count) = 0;                   // 4
    virtual UInt32 GetMaxIndexCount() const = 0;                      // 5  (+0x1c)
    virtual UInt32 GetRoundEdgePointCount() const = 0;                // 6  (0; RoundEdge +0x50)
    virtual UInt32 GetRoundEdgeDataCount() const = 0;                 // 7  (0; RoundEdge +0x4c)
    virtual UInt32 GetFrameCount() const = 0;                         // 8  (+0x20)

    // ---- Mac 9-21: upload (true on success) ----
    // Positions of `count` vertices from `start` in animation frame `frame` (12 bytes each).
    virtual bool SetPositions(const Vector3* data, UInt32 start, UInt32 count, UInt32 frame) = 0; // 9 (Win 10)
    // Same from raw 32-bit words, byte-swapped unless `nativeOrder` (big-endian geometry files).
    virtual bool SetPositions(const UInt32* data, UInt32 start, UInt32 count, UInt32 frame,
                              bool nativeOrder) = 0;                  // 10 (Win 9)
    virtual bool SetNormals(const Vector3* data, UInt32 start, UInt32 count) = 0; // 11 (Win 12)
    virtual bool SetNormals(const UInt32* data, bool nativeOrder) = 0; // 12 (Win 11) all vertices
    // Round-edge data (false except in OpenGLVertexBufferVBORoundEdge). (names inferred)
    virtual bool SetRoundEdgePoints(const Vector3* data, UInt32 count) = 0;       // 13 (+0x48)
    virtual bool SetRoundEdgeData(const UInt32* data, UInt32 count) = 0;          // 14 (+0x44)
    virtual bool SetRoundEdgePointIndices(const UInt32* data, UInt32 count) = 0;  // 15 (+0x54)
    virtual bool SetRoundEdgeDataIndices(const UInt32* data, UInt32 count) = 0;   // 16 (+0x58)
    virtual bool SetColors(const Color* data, UInt32 start, UInt32 count, UInt32 frame) = 0; // 17
    virtual UInt32 GetTexCoordSetCount() const = 0;                   // 18 (+0x08)
    // All vertices of set `set` (dimension floats each).
    virtual bool SetTexCoords(const Real32* data, UInt32 set) = 0;    // 19 (Win 20)
    virtual bool SetTexCoords(const UInt32* data, UInt32 set, bool nativeOrder) = 0; // 20 (Win 19)
    // Indices of the buffer; non-zero `baseVertex` is added to every index.
    virtual bool SetIndices(const UInt32* data, UInt32 start, UInt32 count, UInt32 baseVertex) = 0; // 21

    // ---- Mac 22-31: CPU copies, primitive type ----
    virtual const void* GetPositions(UInt32& stride, UInt32 frame) const = 0; // 22 (stride 12)
    virtual const void* GetNormals(UInt32& stride) const = 0;        // 23 (stride 12)
    virtual const void* GetRoundEdgePoints(UInt32& stride) const = 0; // 24 (null; RoundEdge +0x48)
    virtual const void* GetRoundEdgeData() const = 0;                 // 25 (null; RoundEdge +0x44)
    virtual const void* GetRoundEdgePointIndices() const = 0;         // 26 (null; RoundEdge +0x54)
    virtual const void* GetRoundEdgeDataIndices() const = 0;          // 27 (null; RoundEdge +0x58)
    virtual const void* GetTexCoords(UInt32& stride, UInt32 set) const = 0; // 28
    virtual const void* GetColors(UInt32& stride) const = 0;          // 29 (stride 16)
    virtual const UInt32* GetIndices() const = 0;                     // 30
    // type: 0 points, 1 lines, 2 line strip, 3 triangles, 4 triangle strip, 5 triangle fan
    virtual void SetPrimitiveType(UInt32 type) = 0;                   // 31 (GL mode table 0x763d80)

    virtual ~VertexBuffer() {}                                        // Mac 32/33 0xff140/0xff160
};

} // namespace LEGO
