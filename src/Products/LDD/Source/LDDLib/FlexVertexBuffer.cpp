// LEGO Digital Designer — restored source
// Flexible-part vertex buffers (Mac TU 415, 0x3943e0-0x394eff; Win 0x5a9d10-0x5aa470). See
// FlexVertexBuffer.h. All names inferred.
#include "FlexVertexBuffer.h"
#include "Primitive.h"
#include "Render.h"
#include "ObjectFactory.h"
#include "LEGO/StdLib.h"

namespace LEGO {

// Mac 0x3943e0, Win 0x5a9d10 (the vector maths inlined on Win, x87; same evaluation order)
void SkinFlexGeometry(const Primitive* primitive, const BrickGeometry* geometry,
                      const BasicArray<Matrix3D>& boneTransforms, Vector3* positions,
                      Vector3* normals)
{
    BasicArray<Matrix3D> skin;
    skin.Reserve(boneTransforms.GetSize());
    const Matrix3D* inverse = primitive->m_inverseBoneTransforms.Begin();
    for (BasicArray<Matrix3D>::ConstIterator t = boneTransforms.Begin(); t != boneTransforms.End();
         ++t, ++inverse)
        skin.PushBack(Matrix3D(*t, *inverse));

    UInt32 count = geometry->GetVertexCount();
    for (UInt32 v = 0; v < count; ++v) {
        // {UInt32 n; n x {UInt32 bone; Real32 weight}}
        const UInt32* weights = reinterpret_cast<const UInt32*>(geometry->GetVertexBoneWeights()[v]);
        Vector3 position;
        Vector3 normal;
        for (UInt32 i = 0; i < weights[0]; ++i) {
            const Matrix3D& m = skin[weights[1 + 2 * i]];
            Real32 weight;
            MemCopy(&weight, &weights[2 + 2 * i], sizeof weight);   // a Real32 in the record
            position += (geometry->GetPositions()[v] * m) * weight;
            normal += Rotate(geometry->GetNormals()[v], m) * weight;
        }
        positions[v] = position;
        normals[v] = normal;
    }
}

// Mac 0x394630, Win 0x5aa050
void UpdateFlexVertexBuffer(const Primitive* primitive, const BrickGeometry* geometry,
                            const BasicArray<Matrix3D>& boneTransforms, VertexBuffer* vertexBuffer)
{
    UInt32 count = geometry->GetVertexCount();
    Vector3* positions = new Vector3[count];
    Vector3* normals = new Vector3[count];
    SkinFlexGeometry(primitive, geometry, boneTransforms, positions, normals);
    vertexBuffer->SetPositions(positions, 0, count, 0);
    vertexBuffer->SetNormals(normals, 0, count);
    delete[] normals;
    delete[] positions;
}

// Mac 0x394790, Win 0x5aa1b0
bool CreateFlexVertexBuffer(const Primitive* primitive, const BrickGeometry* geometry,
                            const BasicArray<Matrix3D>& boneTransforms, Render* render,
                            bool dynamic, RefPointer<VertexBuffer>& vertexBuffer)
{
    UInt32 vertexCount = geometry->GetVertexCount();
    UInt32 indexCount = geometry->GetIndexCount();
    BasicArray<UInt32> texCoordDimensions;
    if (geometry->GetTextureCoordinates() != 0)
        texCoordDimensions.PushBack(2);

    if (BrickGeometry::GetLoadShaderData()) {
        VertexBuffer::CreateArgsRoundEdge args;
        args.vertexCount = vertexCount;
        args.indexCount = indexCount;
        args.frameCount = 0;
        args.dynamic = dynamic;
        args.hasNormals = true;
        args.colorSetCount = 0;
        args.texCoordDimensions = texCoordDimensions;
        args.roundEdgeDataCount = geometry->GetRoundEdgeDataCount();
        args.roundEdgePointCount = geometry->GetAverageNormalCount();
        vertexBuffer = render->GetObjectFactory()->Create<VertexBuffer>(args);
    } else {
        VertexBuffer::CreateArgs args;
        args.vertexCount = vertexCount;
        args.indexCount = indexCount;
        args.frameCount = 0;
        args.dynamic = dynamic;
        args.hasNormals = true;
        args.colorSetCount = 0;
        args.texCoordDimensions = texCoordDimensions;
        vertexBuffer = render->GetObjectFactory()->Create<VertexBuffer>(args);
    }
    if (vertexBuffer.IsNull())
        return false;

    if (BrickGeometry::GetLoadShaderData()) {
        vertexBuffer->SetRoundEdgePoints(geometry->GetAverageNormals(), geometry->GetAverageNormalCount());
        vertexBuffer->SetRoundEdgeData(reinterpret_cast<const UInt32*>(geometry->GetRoundEdgeData()),
                                       geometry->GetRoundEdgeDataCount());
        vertexBuffer->SetRoundEdgePointIndices(geometry->GetAverageNormalIndices(), geometry->GetIndexCount());
        vertexBuffer->SetRoundEdgeDataIndices(geometry->GetRoundEdgeIndices(), geometry->GetIndexCount());
    }
    vertexBuffer->SetIndices(geometry->GetIndices(), 0, indexCount, 0);
    if (geometry->GetTextureCoordinates() != 0)
        vertexBuffer->SetTexCoords(reinterpret_cast<const Real32*>(geometry->GetTextureCoordinates()), 0);
    UpdateFlexVertexBuffer(primitive, geometry, boneTransforms, vertexBuffer);
    return true;
}

} // namespace LEGO
