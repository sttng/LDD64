// LEGO Digital Designer — restored source
// TU 291 "CreateTestModelCommand.cpp": Mac 0x29ef20-0x29fdcb, Win 0x66b594-0x66bad5.
#include "CreateTestModelCommand.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickConnect.h"
#include "BrickDatabase.h"
#include "BrickPalette.h"
#include "BrickFilter.h"
#include "Primitive.h"
#include "Brick.h"
#include "Part.h"
#include "PrimitiveElement.h"
#include "LDDPart.h"
#include "LDDUtilities.h"
#include "Application.h"

namespace LEGO {

// Mac 0x29fa90 (D1) / 0x29faf0 (D0), Win sdd 0x62c232
CreateTestModelCommand::~CreateTestModelCommand()
{
}

// Mac 0x29ef20, Win 0x66b59b
void CreateTestModelCommand::Do()
{
    if (!Application::GetInstance()->CloseDocument())
        return;
    BrickConnectSceneManager* manager = m_sceneControl->GetSceneManager();
    RefPointer<Scene> scene = manager->GetBrickConnect()->NewScene(0);
    BrickPalette* palette = m_sceneControl->GetBrickPalette();

    UInt32 x = 0;   // the grid place: x runs fastest, then z, then y
    UInt32 z = 0;
    UInt32 y = 0;
    for (BrickPalette::BrickSet::ConstIterator it = palette->GetBricksBegin(); it != palette->GetBricksEnd(); ++it) {
        const BrickPalette::Brick* paletteBrick = it->Get();
        const CompositeBrickFilter* filter = m_palette->GetFilter();
        if (m_filtered && !filter->Accept(paletteBrick))
            continue;

        // The design's box (every bone box in brick space), 0.2 larger on each side.
        const BrickDatabase::Brick* design = manager->GetDatabase()->GetBrick(paletteBrick->m_designID);
        BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        box.Reset();
        for (BasicArray<BrickDatabase::Part>::ConstIterator part = design->m_parts.Begin();
             part != design->m_parts.End(); ++part) {
            const Primitive* primitive = manager->GetDatabase()->GetPrimitive(part->m_designID);
            const BasicArray<Primitive::Bone>& bones = primitive->GetBones();
            for (BasicArray<Primitive::Bone>::ConstIterator bone = bones.Begin(); bone != bones.End(); ++bone)
                box.Union(bone->m_bounding * bone->m_transform);
        }
        box.m_min -= Vector3(0.2f, 0.2f, 0.2f);
        box.m_max += Vector3(0.2f, 0.2f, 0.2f);

        // The next grid place where the box hits nothing.
        Vector3 position;
        bool hit;
        do {
            if (++x > 19) {
                x = 0;
                if (++z > 19) {
                    z = 0;
                    ++y;
                }
            }
            BoundingBox placed(box);
            position = Vector3((Real32)x * m_spacing, (Real32)y * m_spacing, (Real32)z * m_spacing);
            placed += position;
            CollidePicker picker((Sphere(placed)));
            picker.m_hit = false;
            scene->Pick(picker);
            hit = picker.m_hit;
        } while (hit);

        BrickConnect* brickConnect = manager->GetBrickConnect();
        RefPointer<Brick> brick = brickConnect->CreateBrick(paletteBrick->m_designID);
        static_cast<LDDBrick*>(brick.Get())->SetPaletteBrick(paletteBrick);
        for (BasicArray<RefPointer<BrickPalette::Part> >::ConstIterator it2 = paletteBrick->m_parts.Begin();
             it2 != paletteBrick->m_parts.End(); ++it2) {
            const BrickPalette::Part* palettePart = it2->Get();
            const Primitive* primitive = manager->GetDatabase()->GetPrimitive(palettePart->m_designID);
            if (!primitive)
                return;                                                     // NOTE: the whole command ends
            RefPointer<Part> part = manager->GetBrickConnect()->CreatePart(primitive);
            part->SetBrick(brick);
            for (UInt32 i = 0; i < palettePart->m_boneTransforms.GetSize(); ++i) {
                RefPointer<PrimitiveElement> element =
                    manager->GetBrickConnect()->CreatePrimitiveElement(primitive, i, palettePart->m_boneTransforms[i]);
                element->SetPart(part);
            }
            part->SetMaterials(palettePart->m_materials);
            part->SetDecorations(palettePart->m_decorations);
            part->UpdateState();
        }
        Matrix3D transform;
        transform.Translate(position);
        Scene::ConnectResult result;
        if (scene->CanConnect(brick.Get(), transform, result))
            scene->Insert(brick, result);
    }

    BrickConnectSceneManager::LazyLoadData data;
    AddLazyLoads(*scene, data);
    m_sceneControl->GetSceneManager()->Load(data);
    Scene::ConnectResult result;
    m_sceneControl->MergeScene(scene, result, false);
}

// Mac 0x29fb60, Win 0x630465
void CreateTestModelCommand::UpdateEnable()
{
    SetEnable(true);
}

} // namespace LEGO
