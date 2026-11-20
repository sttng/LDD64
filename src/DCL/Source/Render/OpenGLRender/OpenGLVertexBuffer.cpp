// LEGO Digital Designer — restored source
// LEGO::OpenGLVertexBuffer — see OpenGLVertexBuffer.h. TU 037 (Mac 0xfdeb0-0xff24f,
// Win 0x4f8e90-0x4f97f0).
#include "OpenGLVertexBuffer.h"
#include "OpenGLRender.h"
#include "OpenGLFactories.h"
#include "LEGO/StdLib.h"

#include <string.h>

namespace LEGO {

namespace {

// SetPrimitiveType index → GL mode (Mac 0x763d80, Win 0xae100c).
const UInt32 kPrimitiveModes[6] = {
    GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN
};

inline UInt32 SwapBytes(UInt32 v)
{
    return (v >> 24) | ((v & 0xff0000) >> 8) | ((v & 0xff00) << 8) | (v << 24);
}

// The inlined copy loops of the UInt32 overloads (Mac 0xfe450, 0xfe730, 0xfea60): straight copy
// when the data is in native order, byte-swapped otherwise.
// Win (0x4f93a4, 0x4f9460, 0x4f9540) moves every word through the x87 FPU (fld/fstp dword: the
// source word, and in the swapping loop the swapped word again), which turns signalling-NaN bit
// patterns into quiet NaNs; QuietNaN reproduces that on the Win branch.
#if defined(_WIN32)
inline UInt32 QuietNaN(UInt32 v)
{
    if ((v & 0x7f800000) == 0x7f800000 && (v & 0x007fffff) != 0)
        v |= 0x00400000;
    return v;
}
#else
inline UInt32 QuietNaN(UInt32 v) { return v; }
#endif

void CopyWords(void* dst, const UInt32* src, UInt32 count, bool nativeOrder)
{
    UInt32* d = static_cast<UInt32*>(dst);
    if (nativeOrder) {
        for (UInt32 i = 0; i < count; ++i)
            d[i] = QuietNaN(src[i]);
    } else {
        for (UInt32 i = 0; i < count; ++i)
            d[i] = QuietNaN(SwapBytes(QuietNaN(src[i])));
    }
}

} // namespace

// Mac 0xfdf40 / 0xfdeb0, Win 0x4f8f20
OpenGLVertexBuffer::OpenGLVertexBuffer(OpenGLRender* render, UInt32 vertexCount, UInt32 indexCount,
                                       UInt32 colorSetCount, bool dynamic)
    : m_render(render), m_texCoordSetCount(0), m_vertexCount(vertexCount),
      m_maxVertexCount(vertexCount), m_colorSetCount(colorSetCount), m_indexCount(indexCount),
      m_maxIndexCount(indexCount), m_frameCount(0), m_positions(0), m_normals(0), m_colors(0),
      m_texCoords(0), m_indices(0), m_dynamic(dynamic), m_primitive(GL_TRIANGLES)
{
}

// Mac 0xfdfe0, Win 0x4f9040
bool OpenGLVertexBuffer::Create(bool hasNormals, const BasicArray<UInt32>& texCoordDimensions,
                                UInt32 frameCount)
{
    UInt32 vertices = m_maxVertexCount;
    if (vertices == 0 || m_maxIndexCount == 0)
        return false;
    m_frameCount = frameCount ? frameCount : 1;
    m_positions = new Real32[m_frameCount * vertices * 3];
    if (hasNormals)
        m_normals = new Real32[vertices * 3];
    if (m_colorSetCount != 0)
        m_colors = new Real32[m_colorSetCount * vertices * 4];
    m_indices = new UInt32[m_maxIndexCount];
    m_texCoordSetCount = texCoordDimensions.GetSize();
    if (m_texCoordSetCount != 0) {
        m_texCoords = new TexCoordSet[m_texCoordSetCount];
        for (UInt32 i = 0; i < m_texCoordSetCount; ++i) {
            m_texCoords[i].dimension = texCoordDimensions[i];
            m_texCoords[i].data = new Real32[texCoordDimensions[i] * vertices];
        }
    }
    return true;
}

// Mac 0xfe210, Win 0x4f9200
void OpenGLVertexBuffer::BindArrays()
{
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, m_positions);
    if (m_normals == 0) {
        glDisableClientState(GL_NORMAL_ARRAY);
    } else {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, m_normals);
    }
    if (m_colors == 0) {
        glDisable(GL_COLOR_MATERIAL);
        glDisableClientState(GL_COLOR_ARRAY);
    } else {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, 0, m_colors);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glEnable(GL_COLOR_MATERIAL);
    }
    if (m_texCoordSetCount == 0) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    } else {
        // NOTE: only set 0 is bound (with more than one set the client unit is reset to 0 first).
        if (m_texCoordSetCount != 1)
            glClientActiveTextureARB(GL_TEXTURE0_ARB);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(m_texCoords[0].dimension, GL_FLOAT, 0, m_texCoords[0].data);
    }
}

// Mac 0xfe330 / 0xfe340, Win 0x459790 (ICF)
void OpenGLVertexBuffer::Begin()
{
}

void OpenGLVertexBuffer::End()
{
}

// Mac 0xfe350, Win 0x4f92e0
void OpenGLVertexBuffer::Draw()
{
    m_render->ApplyMaterial();
    glDrawElements(m_primitive, m_indexCount, GL_UNSIGNED_INT, m_indices);
}

UInt32 OpenGLVertexBuffer::GetVertexCount() const { return m_vertexCount; }            // Mac 0xfe380
void   OpenGLVertexBuffer::SetVertexCount(UInt32 count) { m_vertexCount = count; }     // Mac 0xfe390
UInt32 OpenGLVertexBuffer::GetMaxVertexCount() const { return m_maxVertexCount; }      // Mac 0xfe3a0
UInt32 OpenGLVertexBuffer::GetIndexCount() const { return m_indexCount; }              // Mac 0xfe3b0
void   OpenGLVertexBuffer::SetIndexCount(UInt32 count) { m_indexCount = count; }       // Mac 0xfe3c0
UInt32 OpenGLVertexBuffer::GetMaxIndexCount() const { return m_maxIndexCount; }        // Mac 0xfe3d0
UInt32 OpenGLVertexBuffer::GetFrameCount() const { return m_frameCount; }              // Mac 0xfe3e0
UInt32 OpenGLVertexBuffer::GetRoundEdgePointCount() const { return 0; }                // Mac 0xfe3f0
UInt32 OpenGLVertexBuffer::GetRoundEdgeDataCount() const { return 0; }                 // Mac 0xfe400

// Mac 0xfe410, Win 0x4f9350
bool OpenGLVertexBuffer::SetPositions(const Vector3* data, UInt32 start, UInt32 count, UInt32 frame)
{
    memcpy(m_positions + (m_maxVertexCount * frame + start) * 3, data, count * 12);
    return true;
}

// Mac 0xfe450, Win 0x4f9390
bool OpenGLVertexBuffer::SetPositions(const UInt32* data, UInt32 start, UInt32 count, UInt32 frame,
                                      bool nativeOrder)
{
    CopyWords(m_positions + (m_maxVertexCount * frame + start) * 3, data, count * 3, nativeOrder);
    return true;
}

// Mac 0xfe6f0, Win 0x4f9430
bool OpenGLVertexBuffer::SetNormals(const Vector3* data, UInt32 start, UInt32 count)
{
    memcpy(m_normals + start * 3, data, count * 12);
    return true;
}

// Mac 0xfe730, Win 0x4f9460: the vertices in use.
bool OpenGLVertexBuffer::SetNormals(const UInt32* data, bool nativeOrder)
{
    CopyWords(m_normals, data, m_vertexCount * 3, nativeOrder);
    return true;
}

// Mac 0xfe990-0xfe9c0, Win 0x4a9530 (ICF): not supported here.
bool OpenGLVertexBuffer::SetRoundEdgePoints(const Vector3*, UInt32) { return false; }
bool OpenGLVertexBuffer::SetRoundEdgeData(const UInt32*, UInt32) { return false; }
bool OpenGLVertexBuffer::SetRoundEdgePointIndices(const UInt32*, UInt32) { return false; }
bool OpenGLVertexBuffer::SetRoundEdgeDataIndices(const UInt32*, UInt32) { return false; }

// Mac 0xfe9d0, Win 0x4f94e0
bool OpenGLVertexBuffer::SetColors(const Color* data, UInt32 start, UInt32 count, UInt32 set)
{
    memcpy(m_colors + (m_maxVertexCount * set + start) * 4, data, count * 16);
    return true;
}

UInt32 OpenGLVertexBuffer::GetTexCoordSetCount() const { return m_texCoordSetCount; }  // Mac 0xfea10

// Mac 0xfea20, Win 0x4f9510: the vertices in use.
bool OpenGLVertexBuffer::SetTexCoords(const Real32* data, UInt32 set)
{
    memcpy(m_texCoords[set].data, data, m_vertexCount * m_texCoords[set].dimension * 4);
    return true;
}

// Mac 0xfea60, Win 0x4f9540
bool OpenGLVertexBuffer::SetTexCoords(const UInt32* data, UInt32 set, bool nativeOrder)
{
    CopyWords(m_texCoords[set].data, data, m_texCoords[set].dimension * m_vertexCount, nativeOrder);
    return true;
}

// Mac 0xfec90, Win 0x4f95d0
bool OpenGLVertexBuffer::SetIndices(const UInt32* data, UInt32 start, UInt32 count, UInt32 baseVertex)
{
    if (baseVertex == 0) {
        memcpy(m_indices + start, data, count * 4);
    } else {
        for (UInt32 i = 0; i < count; ++i)
            m_indices[start + i] = data[i] + baseVertex;
    }
    return true;
}

// Mac 0xfee70, Win 0x4f9640
const void* OpenGLVertexBuffer::GetPositions(UInt32& stride, UInt32 frame) const
{
    stride = 12;
    return m_positions + frame * m_maxVertexCount * 3;
}

// Mac 0xfeea0, Win 0x4f9660
const void* OpenGLVertexBuffer::GetNormals(UInt32& stride) const
{
    stride = 12;
    return m_normals;
}

// Mac 0xfeec0, Win 0x4f9670: stride not written.
const void* OpenGLVertexBuffer::GetRoundEdgePoints(UInt32&) const { return 0; }
const void* OpenGLVertexBuffer::GetRoundEdgeData() const { return 0; }                 // Mac 0xfeed0
const void* OpenGLVertexBuffer::GetRoundEdgePointIndices() const { return 0; }         // Mac 0xfeee0
const void* OpenGLVertexBuffer::GetRoundEdgeDataIndices() const { return 0; }          // Mac 0xfeef0

// Mac 0xfef00, Win 0x4f9680
const void* OpenGLVertexBuffer::GetTexCoords(UInt32& stride, UInt32 set) const
{
    stride = m_texCoords[set].dimension * 4;
    return m_texCoords[set].data;
}

// Mac 0xfef20, Win 0x4f96a0
const void* OpenGLVertexBuffer::GetColors(UInt32& stride) const
{
    stride = 16;
    return m_colors;
}

const UInt32* OpenGLVertexBuffer::GetIndices() const { return m_indices; }             // Mac 0xfef40

// Mac 0xfef50, Win 0x4f96c0 (no range check)
void OpenGLVertexBuffer::SetPrimitiveType(UInt32 type)
{
    m_primitive = kPrimitiveModes[type];
}

// Mac 0xff050 (Object-in thunk 0xff120)
void OpenGLVertexBuffer::OnFinalRelease()
{
    if (m_indices)
        delete[] m_indices;
    m_indices = 0;
    if (m_positions)
        delete[] m_positions;
    m_positions = 0;
    if (m_normals)
        delete[] m_normals;
    m_normals = 0;
    if (m_colors)
        delete[] m_colors;
    m_colors = 0;
    if (m_texCoords)
        delete[] m_texCoords;   // ~TexCoordSet frees each set, last first
    m_texCoords = 0;
    Object::OnFinalRelease();
}

// OpenGLFactories.h seam: the client-array branch of OpenGLVertexBufferFactory::CreateObject
// (Mac 0xf7a70, Win 0x4ebe30): `new OpenGLVertexBuffer(...)` + Create.
VertexBuffer* CreateOpenGLVertexBuffer(OpenGLRender* render, const VertexBuffer::CreateArgs& args)
{
    OpenGLVertexBuffer* buffer = new OpenGLVertexBuffer(render, args.vertexCount, args.indexCount,
                                                        args.colorSetCount, args.dynamic);
    if (!buffer->Create(args.hasNormals, args.texCoordDimensions, args.frameCount)) {
        // the reference count is 0: Release deletes the object
        buffer->Release();
        return 0;
    }
    return buffer;
}

} // namespace LEGO
