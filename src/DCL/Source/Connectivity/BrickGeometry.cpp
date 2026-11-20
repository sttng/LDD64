// LEGO Digital Designer — restored source
// LEGO::BrickGeometry — reader/writer of the db.lif geometry files (.g, .g1, …).
// TU "BrickGeometry.cpp": Mac 0x1bf610-0x1c069f, Win 0x4dd3a0-0x4de01f. See BrickGeometry.h.
#include "BrickGeometry.h"
#include "File.h"
#include "LEGO/String.h"

#include <string.h>
#include <iostream>   // Mac static initialiser 0x1c0660 (ios_base::Init only)

namespace LEGO {

namespace {

// The original builds an error String and drops it (the reporting call is compiled out in
// both release builds).
inline void DebugMessage(const String&) {}

// Mac 0x77db79 (next to the TU's ios_base::Init object 0x77db78), Win 0xb42746.
bool s_loadShaderData = false;

} // namespace

// Mac 0x1bf610, Win 0x4dd3a0
BrickGeometry::BrickGeometry()
    : m_positions(0), m_normals(0), m_textureCoordinates(0), m_vertexCount(0),
      m_averageNormalIndices(0), m_averageNormals(0), m_averageNormalCount(0),
      m_roundEdgeIndices(0), m_roundEdgeData(0), m_roundEdgeDataCount(0),
      m_indices(0), m_indexCount(0),
      m_boneWeights(0), m_vertexBoneWeights(0), m_boneWeightsSize(0),
      m_cullingData(0), m_cullingCount(0), m_cullingDataSize(0)
{
}

// Mac 0x1c0460 (body, called by D1 0x1c0530 / D0 0x1c05b0 and the virtual thunks 0x1c0570 /
// 0x1c0600), Win 0x4ddfa0 (body) / 0x4dd450 (scalar deleting dtor). Same delete order in both.
BrickGeometry::~BrickGeometry()
{
    delete[] m_positions;
    delete[] m_normals;
    delete[] m_textureCoordinates;
    delete[] m_indices;
    delete[] m_roundEdgeIndices;
    delete[] m_roundEdgeData;
    delete[] m_averageNormalIndices;
    delete[] m_averageNormals;
    delete[] m_boneWeights;
    delete[] m_vertexBoneWeights;
    delete[] m_cullingData;
}

// Mac 0x1bf6d0, Win 0x4dd480
// NOTE: arrays of an earlier Allocate/Read are not freed (leak in both builds); fixed: freed,
// and the optional ones not requested again are cleared (low).
void BrickGeometry::Allocate(UInt32 vertexCount, UInt32 indexCount, UInt32 flags)
{
    delete[] m_positions;
    delete[] m_normals;
    delete[] m_textureCoordinates;
    delete[] m_indices;
    m_normals = 0;
    m_textureCoordinates = 0;
    m_vertexCount = vertexCount;
    m_indexCount = indexCount;
    m_positions = new Vector3[vertexCount];
    if (flags & kNormals)
        m_normals = new Vector3[m_vertexCount];
    if (flags & kTextureCoordinates)
        m_textureCoordinates = new Vector2[m_vertexCount];
    m_indices = new UInt32[m_indexCount];
}

// Mac 0x1bf810 (Win: inlined)
UInt32 BrickGeometry::GetAverageNormalCount() const
{
    return m_averageNormalCount;
}

// Mac 0x1bf820, Win 0x4dd6f0
UInt32 BrickGeometry::GetRoundEdgeDataCount() const
{
    return m_roundEdgeDataCount;
}

// Mac 0x1bf830, Win 0x4dd700
void BrickGeometry::SetLoadShaderData(bool load)
{
    s_loadShaderData = load;
}

// Mac 0x1bf850 (Win: inlined)
bool BrickGeometry::GetLoadShaderData()
{
    return s_loadShaderData;
}

// Mac 0x1bf870, Win 0x4dd710
bool BrickGeometry::Read(File* file)
{
    struct Header {
        UInt32 magic;
        UInt32 vertexCount;
        UInt32 indexCount;
        UInt32 flags;
    } header;
    if (!file->Read(sizeof(header), &header, 0))
        return false;
    if (header.magic != kMagic) {
        DebugMessage(String("BrickGeometry::Read: Wrong file type."));
        return false;
    }
    Allocate(header.vertexCount, header.indexCount, header.flags);
    return ReadData(file, header.flags);
}

// Mac 0x1bf910, Win 0x4dd7d0
bool BrickGeometry::Write(File* file) const
{
    struct Header {
        UInt32 magic;
        UInt32 vertexCount;
        UInt32 indexCount;
        UInt32 flags;
    } header;
    header.magic = kMagic;
    header.vertexCount = m_vertexCount;
    header.indexCount = m_indexCount;
    UInt32 flags = 0;
    if (m_textureCoordinates)
        flags |= kTextureCoordinates;
    if (m_normals)
        flags |= kNormals;
    if (m_vertexBoneWeights)
        flags |= kBoneWeights;
    if (m_cullingData)
        flags |= kCulling;
    if (m_roundEdgeData && m_roundEdgeIndices)
        flags |= kRoundEdgeData;
    if (m_averageNormals && m_averageNormalIndices)
        flags |= kAverageNormals;
    header.flags = flags;
    if (!file->Write(sizeof(header), &header, 0))
        return false;
    return WriteData(file);
}

// Win 0x4dd5e0 (Mac: inlined in ReadData)
void BrickGeometry::AllocateRoundEdgeData(UInt32 count)
{
    m_roundEdgeDataCount = count;
    delete[] m_roundEdgeData;      // an earlier Read's (see Allocate; low)
    delete[] m_roundEdgeIndices;
    m_roundEdgeData = new Real32[count];
    m_roundEdgeIndices = new UInt32[m_indexCount];
    memset(m_roundEdgeIndices, 0, m_indexCount * (UInt32)sizeof(UInt32));
}

// Win 0x4dd630 (Mac: inlined in ReadData)
void BrickGeometry::AllocateAverageNormals(UInt32 count)
{
    m_averageNormalCount = count;
    delete[] m_averageNormals;     // an earlier Read's (see Allocate; low)
    delete[] m_averageNormalIndices;
    m_averageNormals = new Vector3[count];
    m_averageNormalIndices = new UInt32[m_indexCount];
    memset(m_averageNormalIndices, 0, m_indexCount * (UInt32)sizeof(UInt32));
}

// Mac 0x1bf9f0, Win 0x4dd890
// NOTE: while the shader data is requested, a file without both shader blocks is rejected.
// NOTE: the result of the skip Seek is ignored: on a truncated file the next count is read
// from wherever the position stayed (a garbage size can then make new[] throw).
bool BrickGeometry::ReadData(File* file, UInt32 flags)
{
    if ((flags & (kRoundEdgeData | kAverageNormals)) != (kRoundEdgeData | kAverageNormals) && s_loadShaderData)
        return false;

    if (!file->Read(m_vertexCount * (UInt32)sizeof(Vector3), m_positions, 0))
        return false;
    if (m_normals && !file->Read(m_vertexCount * (UInt32)sizeof(Vector3), m_normals, 0))
        return false;
    if (m_textureCoordinates && !file->Read(m_vertexCount * (UInt32)sizeof(Vector2), m_textureCoordinates, 0))
        return false;
    if (!file->Read(m_indexCount * (UInt32)sizeof(UInt32), m_indices, 0))
        return false;

    if (flags & kRoundEdgeData) {
        UInt32 count = 0;
        if (!file->Read(sizeof(count), &count, 0))
            return false;
        if (!s_loadShaderData) {
            file->Seek(file->GetPosition() + count * (UInt32)sizeof(Real32) + m_indexCount * (UInt32)sizeof(UInt32));
        } else {
            AllocateRoundEdgeData(count);
            if (!file->Read(m_roundEdgeDataCount * (UInt32)sizeof(Real32), m_roundEdgeData, 0))
                return false;
            if (m_roundEdgeIndices && !file->Read(m_indexCount * (UInt32)sizeof(UInt32), m_roundEdgeIndices, 0))
                return false;
        }
    }

    if (flags & kAverageNormals) {
        UInt32 count = 0;
        if (!file->Read(sizeof(count), &count, 0))
            return false;
        if (!s_loadShaderData) {
            file->Seek(file->GetPosition() + count * (UInt32)sizeof(Vector3) + m_indexCount * (UInt32)sizeof(UInt32));
        } else {
            AllocateAverageNormals(count);
            if (!file->Read(m_averageNormalCount * (UInt32)sizeof(Vector3), m_averageNormals, 0))
                return false;
            if (m_averageNormalIndices && !file->Read(m_indexCount * (UInt32)sizeof(UInt32), m_averageNormalIndices, 0))
                return false;
        }
    }

    if (flags & kBoneWeights) {
        if (!file->Read(sizeof(m_boneWeightsSize), &m_boneWeightsSize, 0))
            return false;
        UInt32 size = m_boneWeightsSize;
        m_boneWeights = new UInt8[size];
        if (!file->Read(size, m_boneWeights, 0))
            return false;
        m_vertexBoneWeights = new UInt8*[m_vertexCount];
        UInt32* offsets = new UInt32[m_vertexCount];
        if (!file->Read(m_vertexCount * (UInt32)sizeof(UInt32), offsets, 0)) {
            delete[] offsets;
            return false;
        }
        for (UInt32 i = 0; i < m_vertexCount; ++i)
            m_vertexBoneWeights[i] = m_boneWeights + offsets[i];
        delete[] offsets;
    }

    if (!(flags & kCulling))
        return true;
    if (!file->Read(sizeof(m_cullingCount), &m_cullingCount, 0))
        return false;
    if (!file->Read(sizeof(m_cullingDataSize), &m_cullingDataSize, 0))
        return false;
    UInt32 size = m_cullingDataSize;
    UInt8* data = new UInt8[size];
    if (!file->Read(size, data, 0)) {
        delete[] data;
        return false;
    }
    m_cullingData = data;
    return true;
}

// Mac 0x1bffc0, Win 0x4ddca0
bool BrickGeometry::WriteData(File* file) const
{
    if (!file->Write(m_vertexCount * (UInt32)sizeof(Vector3), m_positions, 0))
        return false;
    if (m_normals && !file->Write(m_vertexCount * (UInt32)sizeof(Vector3), m_normals, 0))
        return false;
    if (m_textureCoordinates && !file->Write(m_vertexCount * (UInt32)sizeof(Vector2), m_textureCoordinates, 0))
        return false;
    if (!file->Write(m_indexCount * (UInt32)sizeof(UInt32), m_indices, 0))
        return false;

    if (m_roundEdgeData) {
        if (!file->Write(sizeof(m_roundEdgeDataCount), &m_roundEdgeDataCount, 0))
            return false;
        if (m_roundEdgeData && !file->Write(m_roundEdgeDataCount * (UInt32)sizeof(Real32), m_roundEdgeData, 0))
            return false;
    }
    if (m_roundEdgeIndices && !file->Write(m_indexCount * (UInt32)sizeof(UInt32), m_roundEdgeIndices, 0))
        return false;

    if (m_averageNormals) {
        if (!file->Write(sizeof(m_averageNormalCount), &m_averageNormalCount, 0))
            return false;
        if (m_averageNormals && !file->Write(m_averageNormalCount * (UInt32)sizeof(Vector3), m_averageNormals, 0))
            return false;
    }
    if (m_averageNormalIndices && !file->Write(m_indexCount * (UInt32)sizeof(UInt32), m_averageNormalIndices, 0))
        return false;

    if (m_boneWeights) {
        UInt32 size = m_boneWeightsSize;
        if (!file->Write(sizeof(size), &size, 0))
            return false;
        if (!file->Write(m_boneWeightsSize, m_boneWeights, 0))
            return false;
        UInt32* offsets = new UInt32[m_vertexCount];
        for (UInt32 i = 0; i < m_vertexCount; ++i)
            offsets[i] = (UInt32)(m_vertexBoneWeights[i] - m_boneWeights);
        bool ok = file->Write(m_vertexCount * (UInt32)sizeof(UInt32), offsets, 0);
        delete[] offsets;
        if (!ok)
            return false;
    }

    if (m_cullingData) {
        UInt32 count = m_cullingCount;
        UInt32 size = m_cullingDataSize;
        if (!file->Write(sizeof(count), &count, 0) || !file->Write(sizeof(size), &size, 0) ||
            !file->Write(m_cullingDataSize, m_cullingData, 0))
            return false;
    }
    return true;
}

} // namespace LEGO
