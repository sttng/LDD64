// LEGO Digital Designer — restored source
// LEGO::BrickGeometry — triangle mesh of one brick primitive in db.lif
// ("Primitives/LOD0/<designID>.g" main mesh, ".g1", ".g2" … sub-meshes with their own
// material/decoration). Only fills arrays; the renderer builds vertex buffers from them.
// TU "BrickGeometry.cpp": Mac 0x1bf610-0x1c069f (13 functions), Win 0x4dd3a0-0x4de01f.
// Directory Connectivity: on Win the object sits between ModelIOStorageScene (0x4d7d30-0x4db280)
// / the ConnectionField* objects (0x4dbb50-) and ModelIOLXFML4 (0x4e1aa0-); users are
// BrickDatabase (Mac 0x82a20 / 0x82d10, "Lod<n>/…") and the render scene manager.
//
// File format (little endian, every block a raw dump of the in-memory array):
//   header  (16 bytes)  UInt32 magic 0x42473031 ("10GB"), UInt32 vertexCount,
//                       UInt32 indexCount, UInt32 flags (BrickGeometry::Flags)
//   Vector3 positions[vertexCount]
//   Vector3 normals[vertexCount]                    if flags & kNormals
//   Vector2 textureCoordinates[vertexCount]         if flags & kTextureCoordinates
//   UInt32  indices[indexCount]                     (triangle list)
//   if flags & kRoundEdgeData:   UInt32 n; Real32 data[n]; UInt32 offsets[indexCount]
//                                (per triangle corner: offset into data, 12 floats each)
//   if flags & kAverageNormals:  UInt32 n; Vector3 normals[n]; UInt32 indices[indexCount]
//                                (per triangle corner: index into the normals; entry 0 of the
//                                shipped files is the marker (83, 0, 0))
//   if flags & kBoneWeights:     UInt32 size; UInt8 data[size]; UInt32 offsets[vertexCount]
//                                (per vertex: offset of its record {UInt32 n; n x {UInt32 bone,
//                                Real32 weight}} in data; kept as pointers in memory)
//   if flags & kCulling:         UInt32 count; UInt32 size; UInt8 data[size]
//                                (`count` culling records, first word of each = record size)
// The round-edge and average-normal blocks are only loaded while SetLoadShaderData(true) is in
// effect (the round-edge shader is enabled); otherwise they are skipped with a Seek.
// All shipped LOD0 files have kNormals|kCulling|kRoundEdgeData|kAverageNormals, some also
// kTextureCoordinates and/or kBoneWeights (flexible elements).
//
// Names of the class and of Read come from RTTI and the error string; every other name is
// inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"

namespace LEGO {

class File;

// RTTI: class LEGO::BrickGeometry : virtual Object.
// Mac sizeof 0x54 (Object at +0x4c), primary vtable 0x71f65c {Allocate 0x1bf6d0, Read 0x1bf870,
//   Write 0x1bf910, ReadData 0x1bf9f0, WriteData 0x1bffc0, D1 0x1c0530, D0 0x1c05b0}, Object
//   vtable 0x71f690 {virtual thunks 0x1c0570 / 0x1c0600, Object slots}.
// Win sizeof 0x58 (vptr +0, vbptr +4, members +8.., Object at +0x50), vtable 0xa258f4
//   {0x4dd480, 0x4dd710, 0x4dd7d0, 0x4dd890, 0x4ddca0}, Object vtable 0xa2590c {sdd 0x4dd450, …}.
// Member offsets below: Mac / Win.
class BrickGeometry : public virtual Object {
public:
    // Bits of the header flags (names inferred).
    enum Flags {
        kTextureCoordinates = 0x01,
        kNormals            = 0x02,
        kBoneWeights        = 0x04,
        kCulling            = 0x08,
        kRoundEdgeData      = 0x10,
        kAverageNormals     = 0x20
    };
    enum { kMagic = 0x42473031 };    // "10GB"

    BrickGeometry();                                                  // Mac 0x1bf610, Win 0x4dd3a0 (complete ctor, `this` on the stack)

    // Allocates positions (and normals / texture coordinates per `flags`) and indices; sets the
    // counts. Old arrays are not freed. (name inferred)
    virtual void Allocate(UInt32 vertexCount, UInt32 indexCount, UInt32 flags); // Mac 0x1bf6d0, Win 0x4dd480
    // Header, Allocate, ReadData. false on a read error or a wrong magic.
    virtual bool Read(File* file);                                    // Mac 0x1bf870, Win 0x4dd710
    // Header (flags derived from the arrays present), WriteData. (name inferred)
    virtual bool Write(File* file) const;                             // Mac 0x1bf910, Win 0x4dd7d0
    // Everything after the header. (name inferred)
    virtual bool ReadData(File* file, UInt32 flags);                  // Mac 0x1bf9f0, Win 0x4dd890
    virtual bool WriteData(File* file) const;                         // Mac 0x1bffc0, Win 0x4ddca0 (name inferred)

    virtual ~BrickGeometry();                                         // Mac 0x1c0460 (body) 0x1c0530/0x1c05b0, Win 0x4ddfa0 (body) / sdd 0x4dd450

    // Load the round-edge / average-normal blocks (global switch, default false). (names inferred)
    static void SetLoadShaderData(bool load);                         // Mac 0x1bf830, Win 0x4dd700
    static bool GetLoadShaderData();                                  // Mac 0x1bf850 (Win: inlined)

    UInt32 GetAverageNormalCount() const;                             // Mac 0x1bf810 (Win: inlined) (name inferred)
    UInt32 GetRoundEdgeDataCount() const;                             // Mac 0x1bf820, Win 0x4dd6f0 (name inferred)

    // Accessors (inlined in both builds; names inferred)
    UInt32         GetVertexCount() const          { return m_vertexCount; }
    UInt32         GetIndexCount() const           { return m_indexCount; }
    const Vector3* GetPositions() const            { return m_positions; }
    const Vector3* GetNormals() const              { return m_normals; }
    const Vector2* GetTextureCoordinates() const   { return m_textureCoordinates; }
    const UInt32*  GetIndices() const              { return m_indices; }
    const Vector3* GetAverageNormals() const       { return m_averageNormals; }
    const UInt32*  GetAverageNormalIndices() const { return m_averageNormalIndices; }
    const Real32*  GetRoundEdgeData() const        { return m_roundEdgeData; }
    const UInt32*  GetRoundEdgeIndices() const     { return m_roundEdgeIndices; }
    const UInt8*   GetBoneWeights() const          { return m_boneWeights; }
    UInt32         GetBoneWeightsSize() const      { return m_boneWeightsSize; }
    const UInt8* const* GetVertexBoneWeights() const { return m_vertexBoneWeights; }
    const UInt8*   GetCullingData() const          { return m_cullingData; }
    UInt32         GetCullingCount() const         { return m_cullingCount; }
    UInt32         GetCullingDataSize() const      { return m_cullingDataSize; }

private:
    // Set the count, allocate the block and a zeroed per-index array. Separate functions in the
    // Win build (/LTCG custom convention: count in EAX resp. on the stack, `this` in ESI);
    // inlined into ReadData on Mac. (names inferred)
    void AllocateRoundEdgeData(UInt32 count);                         // Win 0x4dd5e0
    void AllocateAverageNormals(UInt32 count);                        // Win 0x4dd630

    BrickGeometry(const BrickGeometry&);
    BrickGeometry& operator=(const BrickGeometry&);

    Vector3* m_positions;             // +0x04 / +0x08  new Vector3[vertexCount]
    Vector3* m_normals;               // +0x08 / +0x0c  (kNormals)
    Vector2* m_textureCoordinates;    // +0x0c / +0x10  (kTextureCoordinates)
    UInt32   m_vertexCount;           // +0x10 / +0x14
    UInt32*  m_averageNormalIndices;  // +0x14 / +0x18  [indexCount] (kAverageNormals)
    Vector3* m_averageNormals;        // +0x18 / +0x1c  [m_averageNormalCount]
    UInt32   m_averageNormalCount;    // +0x1c / +0x20
    UInt32*  m_roundEdgeIndices;      // +0x20 / +0x24  [indexCount] (kRoundEdgeData)
    Real32*  m_roundEdgeData;         // +0x24 / +0x28  [m_roundEdgeDataCount]
    UInt32   m_roundEdgeDataCount;    // +0x28 / +0x2c
    UInt32*  m_indices;               // +0x2c / +0x30  [indexCount]
    UInt32   m_indexCount;            // +0x30 / +0x34
    UInt8*   m_boneWeights;           // +0x34 / +0x38  [m_boneWeightsSize] (kBoneWeights)
    UInt8**  m_vertexBoneWeights;     // +0x38 / +0x3c  [vertexCount] pointers into m_boneWeights
    UInt32   m_boneWeightsSize;       // +0x3c / +0x40
    UInt8*   m_cullingData;           // +0x40 / +0x44  [m_cullingDataSize] (kCulling)
    UInt32   m_cullingCount;          // +0x44 / +0x48
    UInt32   m_cullingDataSize;       // +0x48 / +0x4c
};

} // namespace LEGO
