// LEGO Digital Designer — restored source
// BrickDraw (Mac TU 173, 0x231290-0x232c9b; Win 0x65736c-0x657ef6). See BrickDraw.h.
// Template code of the TU: BasicArray<RefPointer<Mesh>>::PushBack 0x231bd0,
// BasicArray<BrickDraw::SubPart>::PushBack 0x232170, SubPart copy constructor 0x232b10; the TU
// initialiser 0x232c20.
#include "BrickDraw.h"
#include "BrickConnectSceneManager.h"
#include "BrickDatabase.h"
#include "Primitive.h"
#include "Part.h"
#include "PrimitiveElement.h"
#include "PrimitiveElementBrick.h"
#include "RenderQueue.h"
#include "FlexVertexBuffer.h"

namespace LEGO {

namespace {

// Material slot `index`: materials[index], or materials[0] when that is missing or 0.
inline UInt32 SlotMaterial(const BasicArray<UInt32>& materials, UInt32 index)
{
    if (index < materials.GetSize() && materials[index] != 0)
        return materials[index];
    return materials[0];
}

// Number of material slots of a primitive: one per decoration face, at least one.
inline UInt32 SlotCount(const Primitive* primitive)
{
    return primitive->m_decorationFaceCount != 0 ? primitive->m_decorationFaceCount : 1;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// mesh making
// ---------------------------------------------------------------------------------------------

// Mac 0x231290, Win 0x65795e
bool BrickDraw::BuildMeshes(const Primitive* primitive, const BasicArray<UInt32>& materials,
                            const BasicArray<UInt32>& decorations,
                            BrickConnectSceneManager* sceneManager,
                            BasicArray<RefPointer<Mesh> >& meshes, BoundingBox& box,
                            UInt32 materialSet)
{
    if (primitive->m_bones.GetSize() >= 2) {
        BasicArray<Matrix3D> noTransforms;
        return BuildFlexMeshes(primitive, noTransforms, materials, decorations, sceneManager,
                               meshes, box, materialSet);
    }
    UInt32 i = 0;
    do {
        RefPointer<Mesh> mesh = sceneManager->CreateMesh(primitive, i, SlotMaterial(materials, i),
                                                         materialSet);
        if (mesh.IsNull())
            return false;
        meshes.PushBack(mesh);
        ++i;
    } while (i < SlotCount(primitive));
    for (UInt32 face = 1; face < decorations.GetSize(); ++face) {
        if (decorations[face] == 0)
            continue;
        RefPointer<Mesh> mesh = sceneManager->CreateDecorationMesh(primitive, face, decorations[face]);
        if (!mesh.IsNull())
            meshes.PushBack(mesh);
    }
    box = primitive->m_bounding;
    return true;
}

// Mac 0x231500, Win 0x65751c (+ 0x65756e, 0x657669)
bool BrickDraw::BuildFlexMeshes(const Primitive* primitive, const BasicArray<Matrix3D>& boneTransforms,
                                const BasicArray<UInt32>& materials,
                                const BasicArray<UInt32>& decorations,
                                BrickConnectSceneManager* sceneManager,
                                BasicArray<RefPointer<Mesh> >& meshes, BoundingBox& box,
                                UInt32 materialSet)
{
    if (primitive->m_bones.GetSize() < 2)
        return BuildMeshes(primitive, materials, decorations, sceneManager, meshes, box, materialSet);

    box.Reset();
    // Bone transforms for the skinning.
    BasicArray<Matrix3D> transforms;
    if (boneTransforms.GetSize() == 1) {
        Matrix3D base(boneTransforms[0]);
        transforms.Reserve(primitive->m_bones.GetSize());
        for (BasicArray<Primitive::Bone>::ConstIterator b = primitive->m_bones.Begin();
             b != primitive->m_bones.End(); ++b)
            transforms.PushBack(Matrix3D(base, b->m_transform));
    } else if (boneTransforms.GetSize() == 0) {
        transforms.Reserve(primitive->m_bones.GetSize());
        for (BasicArray<Primitive::Bone>::ConstIterator b = primitive->m_bones.Begin();
             b != primitive->m_bones.End(); ++b)
            transforms.PushBack(b->m_transform);
    } else {
        if (boneTransforms.GetSize() != primitive->m_bones.GetSize())
            return false;
        transforms = boneTransforms;
    }

    UInt32 face = 0;
    do {
        RefPointer<Material> material =
            sceneManager->GetMaterial(SlotMaterial(materials, face), materialSet);
        if (material.IsNull())
            return false;
        bool failed;
        RefPointer<const BrickGeometry> geometry = sceneManager->GetFlexGeometry(primitive, face);
        if (geometry.IsNull()) {
            failed = true;
        } else {
            RefPointer<VertexBuffer> vertexBuffer;
            if (!CreateFlexVertexBuffer(primitive, geometry, transforms, sceneManager->GetRender(),
                                        true, vertexBuffer)) {
                failed = true;
            } else {
                meshes.PushBack(RefPointer<Mesh>(new SingleMesh(material, vertexBuffer)));
                failed = false;
                if (face != 0 && face < decorations.GetSize() && decorations[face] != 0) {
                    // NOTE: a decorated face whose decal material is missing fails the whole
                    // part (both builds).
                    failed = true;
                    material = sceneManager->GetDecalMaterial(decorations[face]);
                    if (!material.IsNull()) {
                        meshes.PushBack(RefPointer<Mesh>(new SingleMesh(material, vertexBuffer)));
                        failed = false;
                    }
                }
            }
        }
        if (failed)
            return false;
        ++face;
    } while (face < SlotCount(primitive));

    // NOTE: the box is bone 0's box at every *given* bone transform (not the skinning
    // transforms); empty input leaves it reset (both builds).
    for (BasicArray<Matrix3D>::ConstIterator t = boneTransforms.Begin(); t != boneTransforms.End(); ++t)
        box.Union(primitive->m_bones[0].m_bounding * *t);
    return true;
}

// Mac 0x231d10, Win 0x657ab4
bool BrickDraw::BuildMeshes(UInt32 designID, const BasicArray<UInt32>& materials,
                            const BasicArray<UInt32>& decorations,
                            BrickConnectSceneManager* sceneManager,
                            BasicArray<RefPointer<Mesh> >& meshes, BoundingBox& box,
                            UInt32 materialSet)
{
    const Primitive* primitive = sceneManager->GetDatabase()->GetPrimitive(designID);
    if (primitive == 0)
        return false;
    return BuildMeshes(primitive, materials, decorations, sceneManager, meshes, box, materialSet);
}

// Mac 0x231d60, Win 0x657891
bool BrickDraw::BuildFlexMeshes(UInt32 designID, const BasicArray<Matrix3D>& boneTransforms,
                                const BasicArray<UInt32>& materials,
                                const BasicArray<UInt32>& decorations,
                                BrickConnectSceneManager* sceneManager,
                                BasicArray<RefPointer<Mesh> >& meshes, BoundingBox& box,
                                UInt32 materialSet)
{
    const Primitive* primitive = sceneManager->GetDatabase()->GetPrimitive(designID);
    if (primitive == 0)
        return false;
    return BuildFlexMeshes(primitive, boneTransforms, materials, decorations, sceneManager,
                           meshes, box, materialSet);
}

// ---------------------------------------------------------------------------------------------
// building
// ---------------------------------------------------------------------------------------------

// Mac 0x231e40, Win 0x657b5c
bool BrickDraw::AddPart(UInt32 designID, const Matrix3D& transform,
                        const BasicArray<UInt32>& materials, const BasicArray<UInt32>& decorations,
                        UInt32 materialSet)
{
    m_subParts.PushBack(SubPart());
    m_subParts.Back().m_transform = transform;
    BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    const Primitive* primitive = m_sceneManager->GetDatabase()->GetPrimitive(designID);
    if (primitive == 0
        || !BuildMeshes(primitive, materials, decorations, m_sceneManager,
                        m_subParts.Back().m_meshes, box, materialSet)) {
        m_subParts.PopBack();
        return false;
    }
    m_box.Union(box * transform);
    return true;
}

// Mac 0x2322e0, Win 0x657c27
bool BrickDraw::AddFlexPart(UInt32 designID, const BasicArray<Matrix3D>& boneTransforms,
                            const BasicArray<UInt32>& materials,
                            const BasicArray<UInt32>& decorations, UInt32 materialSet)
{
    m_subParts.PushBack(SubPart());
    BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    const Primitive* primitive = m_sceneManager->GetDatabase()->GetPrimitive(designID);
    if (primitive == 0
        || !BuildFlexMeshes(primitive, boneTransforms, materials, decorations, m_sceneManager,
                            m_subParts.Back().m_meshes, box, materialSet)) {
        m_subParts.PopBack();
        return false;
    }
    m_box.Union(box);
    return true;
}

// Mac 0x232590, Win 0x657cca
bool BrickDraw::AddPart(const Part* part, UInt32 materialSet)
{
    UInt32 count = part->m_elements.GetSize();
    if (count < 2) {
        Matrix3D transform;
        part->m_elements[0]->GetTransform(transform);
        return AddPart(part->GetDesignID(), transform, part->m_materials, part->m_decorations,
                       materialSet);
    }
    BasicArray<Matrix3D> bones;
    bones.Insert(bones.Begin(), count, Matrix3D());
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = part->m_elements.Begin();
         e != part->m_elements.End(); ++e)
        (*e)->GetTransform(bones[(*e)->GetBoneIndex()]);
    return AddFlexPart(part->GetDesignID(), bones, part->m_materials, part->m_decorations,
                       materialSet);
}

// Mac 0x232700, Win 0x657db7
bool BrickDraw::AddPart(const StorageScene2::Part& part, UInt32 materialSet)
{
    UInt32 count = part.m_bones.GetSize();
    if (count < 2)
        return AddPart(part.m_designID, part.m_bones[0]->m_transformation, part.m_materials,
                       part.m_decorations, materialSet);
    BasicArray<Matrix3D> bones;
    bones.Reserve(count);
    for (BasicArray<RefPointer<StorageScene2::Bone> >::ConstIterator b = part.m_bones.Begin();
         b != part.m_bones.End(); ++b)
        bones.PushBack((*b)->m_transformation);
    return AddFlexPart(part.m_designID, bones, part.m_materials, part.m_decorations, materialSet);
}

// Mac 0x232830, Win 0x657e76
bool BrickDraw::AddPart(const PrimitiveElementBrick& brick, UInt32 part, UInt32 materialSet)
{
    const BrickPalette::Part*   palettePart = brick.GetPalettePart(part);
    const BasicArray<UInt32>&   materials = brick.GetMaterials(part);
    const BasicArray<UInt32>&   decorations = brick.GetDecorations(part);
    const BasicArray<Matrix3D>& bones = brick.GetBoneTransforms(part);
    if (palettePart->m_boneTransforms.GetSize() < 2)
        return AddPart(palettePart->m_designID, bones[0], materials, decorations, materialSet);
    return AddFlexPart(palettePart->m_designID, bones, materials, decorations, materialSet);
}

// Mac 0x231db0, Win 0x65736c
void BrickDraw::SetMaterial(Material* material)
{
    for (BasicArray<SubPart>::Iterator sp = m_subParts.Begin(); sp != m_subParts.End(); ++sp)
        for (BasicArray<RefPointer<Mesh> >::Iterator m = sp->m_meshes.Begin();
             m != sp->m_meshes.End(); ++m)
            (*m)->SetMaterial(material);
}

// ---------------------------------------------------------------------------------------------
// drawing
// ---------------------------------------------------------------------------------------------

// Mac 0x2328d0, Win 0x6573a2
void BrickDraw::Draw(RenderQueue* queue, const Matrix3D& camera) const
{
    for (BasicArray<SubPart>::ConstIterator sp = m_subParts.Begin(); sp != m_subParts.End(); ++sp) {
        Real32 depth = TranslationDistanceSquared(sp->m_transform, camera);
        for (BasicArray<RefPointer<Mesh> >::ConstIterator m = sp->m_meshes.Begin();
             m != sp->m_meshes.End(); ++m)
            queue->Add(*m, sp->m_transform, depth, 0, 1.0f);
    }
}

// Mac 0x232980, Win 0x657401
void BrickDraw::Draw(RenderQueue* queue, const Matrix3D& camera, const Matrix3D& world) const
{
    for (BasicArray<SubPart>::ConstIterator sp = m_subParts.Begin(); sp != m_subParts.End(); ++sp) {
        Matrix3D transform(world, sp->m_transform);
        Real32 depth = TranslationDistanceSquared(transform, camera);
        for (BasicArray<RefPointer<Mesh> >::ConstIterator m = sp->m_meshes.Begin();
             m != sp->m_meshes.End(); ++m)
            queue->Add(*m, transform, depth, 0, 1.0f);
    }
}

// Mac 0x232a40, Win 0x65746c
void BrickDraw::DrawWithDepthPrepass(RenderQueue* queue, const Matrix3D& camera,
                                     const Matrix3D& world) const
{
    for (BasicArray<SubPart>::ConstIterator sp = m_subParts.Begin(); sp != m_subParts.End(); ++sp) {
        Matrix3D transform(world, sp->m_transform);
        Real32 depth = TranslationDistanceSquared(transform, camera);
        for (BasicArray<RefPointer<Mesh> >::ConstIterator m = sp->m_meshes.Begin();
             m != sp->m_meshes.End(); ++m)
            queue->AddWithDepthPrepass(*m, transform, depth, 0, 0.5f, false);
    }
}

} // namespace LEGO
