// LEGO Digital Designer — restored source
// RoundEdgeDataTextureCreator.cpp (TU 394). See RoundEdgeDataTextureCreator.h.
#include "RoundEdgeDataTextureCreator.h"
#include "Render.h"
#include "ObjectFactory.h"
#include "LEGO/String.h"
#include "Trace.h"
#include "LEGO/StdLib.h"
#include <string.h>

namespace LEGO {

namespace {

const UInt32 kMaxTextureSize = 0x4000;
const Real32 kCoordinateScale = 16384.0f;   // Mac 0x63eb20/0x63eb30

UInt16* AllocCoordinates(UInt32 indexCount)
{
    // operator new[](indexCount * 4), saturated on overflow like the original.
    UInt32 count = indexCount * 2;
    return (UInt16*)operator new[](count > 0x7fffffffu ? 0xffffffffu : count * 2);
}

} // namespace

// Mac 0x37db30, Win 0x505840
RoundEdgeDataTextureCreator::RoundEdgeDataTextureCreator(Render* render)
    : m_render(render), m_vertexCoordinates(0), m_roundEdgeDataCoordinates(0),
      m_averageNormalCoordinates(0), m_failed(false)
{
}

// Mac 0x37e7d0, Win 0x506150
RoundEdgeDataTextureCreator::~RoundEdgeDataTextureCreator()
{
    if (m_vertexCoordinates) {
        operator delete[](m_vertexCoordinates);
        m_vertexCoordinates = 0;
    }
    if (m_roundEdgeDataCoordinates) {
        operator delete[](m_roundEdgeDataCoordinates);
        m_roundEdgeDataCoordinates = 0;
    }
    if (m_averageNormalCoordinates) {
        operator delete[](m_averageNormalCoordinates);
        m_averageNormalCoordinates = 0;
    }
}

// Mac 0x37df20, Win 0x506000
RoundEdgeDataTextureCreator::Size RoundEdgeDataTextureCreator::ComputeTextureSize(
    UInt32 texels, const Size& minimum, UInt32 maxTextureSize)
{
    UInt32 limit = maxTextureSize < kMaxTextureSize ? maxTextureSize : kMaxTextureSize;
    // NOTE: the remainder test of the width is reused for the height.
    UInt32 remainder = (texels % limit != 0) ? 1 : 0;

    UInt32 width = 1;
    while (width < texels / limit + remainder)
        width *= 2;
    if (width < minimum.width)
        width = minimum.width;

    UInt32 height = 1;
    while (height < texels / width + remainder)
        height *= 2;
    if (height < minimum.height)
        height = minimum.height;

    Size size;
    size.width = width;
    size.height = height;
    if (limit < height || limit < width) {
        String message("RoundEdgeDataTextureCreator: Tried to allocate texture of size ");
        message << size.width;
        message << " , ";
        message << size.height;
        message << " . Max texture size is ";
        message << limit;
        TraceVerbose(message, true);
        size.width = 0;
        size.height = 0;
        m_failed = true;
    }
    return size;
}

// The texture creation shared by the three Create functions (inlined in each).
Texture* RoundEdgeDataTextureCreator::CreateFloatTexture(const Size& size, UInt32 format,
                                                         UInt32 bytesPerTexel, const void* data,
                                                         UInt32 dataSize)
{
    Texture::CreateArgs2D args;
    args.priority = 0.0f;
    args.wrapMode = Texture::kWrapClampToEdge;
    args.alphaMode = Texture::kAlphaNone;
    args.mipmapMode = Texture::kMipmapNone;
    args.bitmap.m_width = size.width;
    args.bitmap.m_height = size.height;
    args.bitmap.m_depth = 1;
    args.bitmap.m_format = format;
    args.bitmap.m_pitch = (Int32)(size.width * bytesPerTexel);
    args.bitmap.m_dataSize = size.height * size.width * bytesPerTexel;
    args.bitmap.m_palette = 0;
    args.filter = Texture::kFilterNearest;
    // NOTE: the texels beyond `dataSize` are left uninitialised (as in the original); fixed:
    // zero (low).
    UInt8* pixels = (UInt8*)operator new[](args.bitmap.m_dataSize);
    memset(pixels, 0, args.bitmap.m_dataSize);
    memcpy(pixels, data, dataSize);
    args.bitmap.m_data = pixels;
    Object* object = m_render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    Texture* texture = object ? dynamic_cast<Texture*>(object) : 0;
    operator delete[](pixels);
    return texture;
}

// Mac 0x37db90, Win 0x5058e0
void RoundEdgeDataTextureCreator::CreateVertexTexture(const Vector3* positions,
                                                      const Vector3* normals,
                                                      UInt32 vertexCount, const UInt32* indices,
                                                      UInt32 indexCount)
{
    if (m_vertexCoordinates) {
        operator delete[](m_vertexCoordinates);
        m_vertexCoordinates = 0;
    }
    m_vertexTexture = 0;

    Size minimum = { 2, 2 };
    Size size = ComputeTextureSize(vertexCount * 2, minimum, m_render->GetMaxTextureSize());
    if (size.width == 0 || size.height == 0)
        return;

    // Interleaved position / normal texels.
    UInt32 bytes = vertexCount * 0x18;
    Real32* texels = (Real32*)operator new[](bytes ? bytes : 1);
    for (UInt32 i = 0; i < vertexCount; ++i) {
        texels[i * 6 + 0] = positions[i].x;
        texels[i * 6 + 1] = positions[i].y;
        texels[i * 6 + 2] = positions[i].z;
    }
    for (UInt32 i = 0; i < vertexCount; ++i) {
        texels[i * 6 + 3] = normals[i].x;
        texels[i * 6 + 4] = normals[i].y;
        texels[i * 6 + 5] = normals[i].z;
    }
    m_vertexTexture = CreateFloatTexture(size, 0x260, 12, texels, bytes);
    operator delete[](texels);

    m_vertexCoordinates = AllocCoordinates(indexCount);
    for (UInt32 i = 0; i < indexCount; ++i) {
        UInt32 texel = indices[i] * 2;
        m_vertexCoordinates[i * 2] = (UInt16)(texel % size.width);
        // NOTE: unsigned division of the (sign-extended) 16-bit row shifted by 14.
        m_vertexCoordinates[i * 2 + 1] =
            (UInt16)((UInt32)((Int32)(Int16)(texel / size.width) << 14) / size.height);
    }
}

// Mac 0x37e090, Win 0x505b50
void RoundEdgeDataTextureCreator::CreateAverageNormalTexture(const Vector3* normals, UInt32 count,
                                                             const UInt32* indices,
                                                             UInt32 indexCount)
{
    Size minimum = { 2, 2 };
    Size size = ComputeTextureSize(count, minimum, m_render->GetMaxTextureSize());
    if (size.width == 0 || size.height == 0)
        return;

    m_averageNormalTexture = CreateFloatTexture(size, 0x260, 12, normals, count * 12);

    m_averageNormalCoordinates = AllocCoordinates(indexCount);
    Real32 width = (Real32)size.width;
    Real32 height = (Real32)size.height;
    for (UInt32 i = 0; i < indexCount; ++i) {
        UInt32 index = indices[i];
        Real32 x = (Real32)(Int32)(Int16)(index % size.width) / width;
        Real32 y = (Real32)(Int32)(Int16)(index / size.width) / height;
        m_averageNormalCoordinates[i * 2] = (UInt16)(Int16)(Int32)(x * kCoordinateScale);
        m_averageNormalCoordinates[i * 2 + 1] = (UInt16)(Int16)(Int32)(y * kCoordinateScale);
    }
}

// Mac 0x37e4f0, Win 0x505d90
void RoundEdgeDataTextureCreator::CreateRoundEdgeDataTexture(const UInt32* data, UInt32 count,
                                                             const UInt32* indices,
                                                             UInt32 indexCount)
{
    Size minimum = { GetRoundEdgeDataTextureMinWidth(), 2 };
    Size size = ComputeTextureSize(count >> 2, minimum, m_render->GetMaxTextureSize());
    if (size.width == 0 || size.height == 0)
        return;

    m_roundEdgeDataTexture = CreateFloatTexture(size, 0x40000880, 16, data, count * 4);

    m_roundEdgeDataCoordinates = AllocCoordinates(indexCount);
    Real32 height = (Real32)size.height;
    for (UInt32 i = 0; i < indexCount; ++i) {
        UInt32 texel = indices[i] >> 2;
        m_roundEdgeDataCoordinates[i * 2] = (UInt16)(texel % size.width);
        Real32 y = (Real32)(Int32)(Int16)(texel / size.width) / height;
        m_roundEdgeDataCoordinates[i * 2 + 1] = (UInt16)(Int16)(Int32)(y * kCoordinateScale);
    }
}

} // namespace LEGO
