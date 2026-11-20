// LEGO Digital Designer — restored source
// Flexible-part geometry on the GPU (file and function names inferred): a vertex buffer for a
// flex geometry (BrickConnectSceneManager::GetFlexGeometry, a .g file with bone weights) whose
// positions and normals are skinned with the part's bone transforms. Used by
// BrickDraw::BuildFlexMeshes (TU 173) and by BrickConnectSceneControl (Mac 0x48320: LDDPart
// flex surfaces, re-skinned with UpdateFlexVertexBuffer when bones move).
//
// Bodies in FlexVertexBuffer.cpp: Mac TU 415 (0x3943e0-0x394eff, initialiser 0x394ec0; its
// "BasicArray.cpp" guess comes from BasicArray<Matrix3D> code), Win 0x5a9d10-0x5aa470. In LDDLib
// beside its users (as in the Win image).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "BrickGeometry.h"
#include "VertexBuffer.h"

namespace LEGO {

class Primitive;
class Render;

// Creates (through the render's ObjectFactory) a vertex buffer for `geometry`:
// CreateArgsRoundEdge when BrickGeometry::GetLoadShaderData() (round-edge data uploaded too),
// else CreateArgs; normals, texture coordinates (dimension 2) when the geometry has them,
// `dynamic` as given; indices uploaded, then the skinned positions/normals
// (UpdateFlexVertexBuffer). `vertexBuffer` receives the buffer (or null); false when none could
// be created. Mac 0x394790, Win 0x5aa1b0
bool CreateFlexVertexBuffer(const Primitive* primitive, const BrickGeometry* geometry,
                            const BasicArray<Matrix3D>& boneTransforms, Render* render,
                            bool dynamic, RefPointer<VertexBuffer>& vertexBuffer);

// Skins `geometry` (SkinFlexGeometry) and uploads positions and normals of all its vertices.
// Mac 0x394630, Win 0x5aa050
void UpdateFlexVertexBuffer(const Primitive* primitive, const BrickGeometry* geometry,
                            const BasicArray<Matrix3D>& boneTransforms, VertexBuffer* vertexBuffer);

// Linear blend skinning: per vertex the weighted sum over its bone weights of the position
// transformed by boneTransforms[b] * inverse bone transform b of the primitive (normals rotated
// with the same matrices, not renormalised). NOTE: no range checks of the bone indices (both
// builds). Mac 0x3943e0, Win 0x5a9d10
void SkinFlexGeometry(const Primitive* primitive, const BrickGeometry* geometry,
                      const BasicArray<Matrix3D>& boneTransforms, Vector3* positions,
                      Vector3* normals);

} // namespace LEGO
