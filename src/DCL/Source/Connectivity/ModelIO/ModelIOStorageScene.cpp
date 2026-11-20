// LEGO Digital Designer — restored source
// ModelLoaderStorageScene / ModelSaverStorageScene: StorageScene (v1) <-> StorageScene2. See the
// header. Plan item 2.5, Mac TU 335.
#include "ModelIOStorageScene.h"
#include "Path.h"

#include <iostream>

namespace LEGO {

typedef StorageScene  SS1;
typedef StorageScene2 SS2;

namespace {

// A matrix that RemoveScaling cannot normalise is replaced by its translation (inline; the same
// sequence as in the LXFML 5 reader).
void Normalize(Matrix3D& m)
{
    if (!m.RemoveScaling()) {
        Vector3 translation(m.GetTranslation());
        m.SetTranslationMatrix(translation);
    }
}

} // namespace

// ============================================================================================
// ModelLoaderStorageScene
// ============================================================================================

ModelLoaderStorageScene::ModelLoaderStorageScene() {}                       // Win 0x4b31f0

ModelLoaderStorageScene::~ModelLoaderStorageScene() {}                      // Mac 0x2f5740, Win 0x4b32a0

bool ModelLoaderStorageScene::Load(const AString& content, StorageScene2& scene, bool flags) // Mac 0x2efbd0, Win 0x4d8560
{
    RefPointer<SS1> storage = new SS1;
    // the slot 5 reader
    bool result = Load(content, *storage, flags);
    if (result)
        Convert(*storage, scene);
    return result;
}

void ModelLoaderStorageScene::Convert(StorageScene& from, StorageScene2& to) // Mac 0x2efd40, Win 0x4d8720
{
    to.m_name = from.m_name;
    to.m_cameras.Swap(from.m_cameras);
    to.m_camera = from.m_camera;
    to.m_metas.Swap(from.m_metas);

    m_assemblyBricks.Clear();
    m_flexParts.Clear();
    m_parts.Clear();
    m_flexes.Clear();

    for (BasicArray<SS1::Model>::Iterator model = from.m_models.Begin(); model != from.m_models.End(); ++model) {
        to.m_rigidSystems.PushBack(SS2::RigidSystem());
        SS2::RigidSystem& system = to.m_rigidSystems.Back();
        BasicMap<SS1::Rigid*, SS2::Rigid*> rigids;

        for (BasicArray<RefPointer<SS1::Rigid> >::Iterator r = model->m_rigids.Begin(); r != model->m_rigids.End(); ++r) {
            SS1::Rigid* rigid1 = r->Get();
            RefPointer<SS2::Rigid> rigid2 = new SS2::Rigid;
            Matrix3D& rigidMatrix = rigid1->m_transformation;
            Normalize(rigidMatrix);
            rigid2->m_transformation = rigidMatrix;
            system.m_rigids.PushBack(rigid2);
            rigids[rigid1] = rigid2;

            for (BasicArray<RefPointer<SS1::Part> >::Iterator p = rigid1->m_parts.Begin(); p != rigid1->m_parts.End(); ++p) {
                SS1::Part* part1 = p->Get();
                RefPointer<SS2::Brick> brick;
                RefPointer<SS2::Part> part2;

                // the brick of an assembly
                SS1::Assembly* assembly = part1->m_assembly;
                if (assembly) {
                    BasicMap<SS1::Assembly*, SS2::Brick*>::Iterator found = m_assemblyBricks.Find(assembly);
                    if (found != m_assemblyBricks.End()) {
                        brick = found->second;
                    } else {
                        brick = new SS2::Brick(assembly->m_designID);
                        brick->m_itemNos = part1->m_itemNos;
                        to.m_bricks.PushBack(brick);
                        m_assemblyBricks[assembly] = brick;
                    }
                }

                // the part of a flex (one bone per control point)
                SS1::Flex* flex = part1->m_flex;
                if (flex) {
                    BasicMap<SS1::Flex*, SS2::Part*>::Iterator found = m_flexParts.Find(flex);
                    if (found != m_flexParts.End()) {
                        part2 = found->second;
                        brick = part2->m_brick;
                    } else {
                        part2 = new SS2::Part(flex->m_designID);
                        part2->m_materials = part1->m_materials;
                        for (BasicArray<SS1::Decoration>::Iterator d = part1->m_decorations.Begin();
                             d != part1->m_decorations.End(); ++d) {
                            // NOTE: grows only when the surface is beyond the size + 1, so a
                            // surface equal to the size is written one past the end (both builds;
                            // the plain part below uses <=); and a surface near 4G wraps the size
                            // (both, here and below); fixed: grows as below, and surfaces from
                            // kMaxModelFileIndex on are dropped (H13).
                            if (d->m_surface >= kMaxModelFileIndex)
                                continue;
                            if (part2->m_decorations.GetSize() <= d->m_surface)
                                part2->m_decorations.Resize(d->m_surface + 1, 0);
                            part2->m_decorations.Begin()[d->m_surface] = d->m_decoration;
                        }
                        part2->m_bones.Resize(flex->m_parts.GetSize());
                        m_flexParts[flex] = part2;
                        if (!brick) {
                            brick = new SS2::Brick(flex->m_designID);
                            brick->m_itemNos = part1->m_itemNos;
                            to.m_bricks.PushBack(brick);
                        }
                        part2->m_brick = brick;
                        brick->m_parts.PushBack(part2);
                    }
                }

                if (!brick) {
                    brick = new SS2::Brick(part1->m_designID);
                    brick->m_itemNos = part1->m_itemNos;
                    to.m_bricks.PushBack(brick);
                }
                if (!part2) {
                    part2 = new SS2::Part(part1->m_designID);
                    part2->m_materials = part1->m_materials;
                    for (BasicArray<SS1::Decoration>::Iterator d = part1->m_decorations.Begin();
                         d != part1->m_decorations.End(); ++d) {
                        if (d->m_surface >= kMaxModelFileIndex)
                            continue;
                        if (part2->m_decorations.GetSize() <= d->m_surface)
                            part2->m_decorations.Resize(d->m_surface + 1, 0);
                        part2->m_decorations[d->m_surface] = d->m_decoration;
                    }
                    part2->m_bones.Resize(1);
                    part2->m_brick = brick;
                    brick->m_parts.PushBack(part2);
                }

                // the bone: rigid * part
                RefPointer<SS2::Bone> bone = new SS2::Bone;
                Normalize(part1->m_transformation);
                bone->m_transformation.Multiply(rigidMatrix, part1->m_transformation);
                bone->m_part = part2;
                UInt32 index = 0;
                if (flex) {
                    UInt32 count = flex->m_parts.GetSize();
                    for (index = 0; index < count; ++index)
                        if (flex->m_parts[index] == part1)
                            break;
                }
                // NOTE: a part missing from its flex writes one past the end of the bones;
                // fixed: such a part gets no bone (H13)
                if (index >= part2->m_bones.GetSize()) {
                    m_parts[part1] = part2;
                    continue;
                }
                part2->m_bones.Begin()[index] = bone;
                rigid2->m_bones.PushBack(bone);
                m_parts[part1] = part2;
            }
        }

        for (BasicArray<SS1::Joint>::Iterator j = model->m_joints.Begin(); j != model->m_joints.End(); ++j) {
            system.m_joints.PushBack(SS2::Joint());
            SS2::Joint& joint = system.m_joints.Back();
            joint.m_rigidA = rigids[j->m_rigidA.Get()];
            joint.m_rigidB = rigids[j->m_rigidB.Get()];
            joint.m_axisA = j->m_axisA;
            joint.m_zeroA = j->m_zeroA;
            joint.m_positionA = j->m_positionA;
            joint.m_axisB = j->m_axisB;
            joint.m_zeroB = j->m_zeroB;
            joint.m_positionB = j->m_positionB;
            joint.m_type = j->m_type != 0 ? 1 : 0;
            joint.m_name = j->m_name;
        }
    }

    for (BasicArray<SS1::BI>::Iterator bi = from.m_buildingInstructions.Begin(); bi != from.m_buildingInstructions.End(); ++bi) {
        to.m_buildingInstructions.PushBack(SS2::BI());
        SS2::BI& instruction = to.m_buildingInstructions.Back();
        instruction.m_name = bi->m_name;
        ConvertSteps(bi->m_steps, instruction.m_events);
    }
}

void ModelLoaderStorageScene::ConvertSteps(BasicArray<RefPointer<SS1::BIStep> >& steps,
                                           BasicArray<RefPointer<SS2::BIEvent> >& events) // Mac 0x2f1410, Win 0x4d94e0
{
    for (BasicArray<RefPointer<SS1::BIStep> >::Iterator s = steps.Begin(); s != steps.End(); ++s) {
        SS1::BIStep* step1 = s->Get();
        if (step1->m_camera)
            events.PushBack(RefPointer<SS2::BIEvent>(new SS2::BICamera(step1->m_camera)));
        events.PushBack(RefPointer<SS2::BIEvent>(new SS2::BIStep));
        SS2::BIStep* step2 = static_cast<SS2::BIStep*>(events.Back().Get());
        step2->m_name = step1->m_name;
        step2->m_exploded = step1->m_exploded;
        step2->m_explode = step1->m_explode;
        ConvertSteps(step1->m_steps, step2->m_events);

        for (BasicArray<SS1::BIPart>::Iterator bp = step1->m_parts.Begin(); bp != step1->m_parts.End(); ++bp) {
            SS1::Part* part1 = bp->m_part;
            if (!m_parts.Contains(part1))
                continue;
            if (SS1::Flex* flex = part1->m_flex) {
                bool seen = false;
                for (BasicArray<SS1::Flex*>::Iterator f = m_flexes.Begin(); f != m_flexes.End(); ++f) {
                    if (*f == flex) {
                        seen = true;
                        break;
                    }
                }
                if (seen)
                    continue;
                m_flexes.PushBack(flex);
            }
            step2->m_events.PushBack(RefPointer<SS2::BIEvent>(new SS2::BIPart(RefPointer<SS2::Part>())));
            SS2::BIPart* event = static_cast<SS2::BIPart*>(step2->m_events.Back().Get());
            event->m_part = m_parts[part1];
            event->m_exploded = bp->m_exploded;
            Normalize(bp->m_transformation);
            event->m_transformation = bp->m_transformation;
        }
    }
}

// ============================================================================================
// ModelSaverStorageScene
// ============================================================================================

ModelSaverStorageScene::ModelSaverStorageScene() {}                         // Win 0x6014ea

ModelSaverStorageScene::~ModelSaverStorageScene() {}                        // Mac 0x2f34e0, Win 0x601522

bool ModelSaverStorageScene::Save(const Path& path, const StorageScene2& scene) // Mac 0x2f1d00, Win 0x4d98b0
{
    RefPointer<SS1> storage = new SS1;
    storage->m_name = scene.m_name;
    storage->m_cameras = scene.m_cameras;
    storage->m_camera = scene.m_camera;
    storage->m_metas = scene.m_metas;

    // everything goes into one rigid of one model
    storage->m_models.PushBack(SS1::Model());
    RefPointer<SS1::Rigid> rigid = new SS1::Rigid;
    storage->m_models.Back().m_rigids.PushBack(rigid);

    for (BasicArray<RefPointer<SS2::Brick> >::ConstIterator b = scene.m_bricks.Begin(); b != scene.m_bricks.End(); ++b) {
        const SS2::Brick* brick = b->Get();
        RefPointer<SS1::Assembly> assembly;
        if (brick->m_parts.GetSize() > 1)
            assembly = new SS1::Assembly(brick->m_designID);
        const UInt32 start = rigid->m_parts.GetSize();
        for (BasicArray<RefPointer<SS2::Part> >::ConstIterator p = brick->m_parts.Begin(); p != brick->m_parts.End(); ++p) {
            const SS2::Part* part = p->Get();
            if (part->m_bones.GetSize() != 1) {
                // the brick is left out: take back the parts written for it
                while (rigid->m_parts.GetSize() != start) {
                    SS1::Part* last = rigid->m_parts.Back();
                    for (BasicMap<const SS2::Part*, SS1::Part*>::Iterator it = m_parts.Begin(); it != m_parts.End(); ++it) {
                        if (it->second == last) {
                            m_parts.Erase(it);
                            break;
                        }
                    }
                    rigid->m_parts.PopBack();
                }
                break;
            }
            RefPointer<SS1::Part> part1 = new SS1::Part;
            part1->m_designID = part->m_designID;
            part1->m_itemNos = brick->m_itemNos;
            for (UInt32 i = 0; i < part->m_decorations.GetSize(); ++i) {
                if (part->m_decorations[i] != 0) {
                    SS1::Decoration decoration;
                    decoration.m_surface = i;
                    decoration.m_decoration = part->m_decorations[i];
                    part1->m_decorations.PushBack(decoration);
                }
            }
            part1->m_materials = part->m_materials;
            part1->m_transformation = part->m_bones[0]->m_transformation;
            if (assembly) {
                part1->m_assembly = assembly;
                assembly->m_parts.PushBack(part1);
            }
            rigid->m_parts.PushBack(part1);
            m_parts[part] = part1;
        }
    }

    for (BasicArray<SS2::BI>::ConstIterator bi = scene.m_buildingInstructions.Begin(); bi != scene.m_buildingInstructions.End(); ++bi) {
        storage->m_buildingInstructions.PushBack(SS1::BI());
        SS1::BI& instruction = storage->m_buildingInstructions.Back();
        instruction.m_name = bi->m_name;
        ConvertEvents(bi->m_events, instruction.m_steps, 0);
    }

    // the slot 5 writer
    return Save(path, *storage);
}

void ModelSaverStorageScene::ConvertEvents(const BasicArray<RefPointer<SS2::BIEvent> >& events,
                                           BasicArray<RefPointer<SS1::BIStep> >& steps,
                                           SS1::BIStep* parent)             // Mac 0x2f2e20, Win 0x4d9f50
{
    RefPointer<SS2::Camera> camera;
    for (BasicArray<RefPointer<SS2::BIEvent> >::ConstIterator e = events.Begin(); e != events.End(); ++e) {
        const SS2::BIEvent* event = e->Get();
        if (event->m_type == SS2::BIEvent::TypeStep) {
            const SS2::BIStep* step2 = static_cast<const SS2::BIStep*>(event);
            steps.PushBack(RefPointer<SS1::BIStep>(new SS1::BIStep));
            SS1::BIStep* step1 = steps.Back();
            step1->m_name = step2->m_name;
            step1->m_camera = camera;
            camera = 0;
            step1->m_exploded = step2->m_exploded;
            step1->m_explode = step2->m_explode;
            ConvertEvents(step2->m_events, step1->m_steps, step1);
        } else if (event->m_type == SS2::BIEvent::TypePart) {
            const SS2::BIPart* part2 = static_cast<const SS2::BIPart*>(event);
            BasicMap<const SS2::Part*, SS1::Part*>::Iterator found = m_parts.Find(part2->m_part.Get());
            if (found != m_parts.End()) {
                SS1::BIPart part1;
                part1.m_part = found->second;
                part1.m_exploded = part2->m_exploded;
                part1.m_transformation = part2->m_transformation;
                parent->m_parts.PushBack(part1);
            }
        } else if (event->m_type == SS2::BIEvent::TypeCamera) {
            camera = static_cast<const SS2::BICamera*>(event)->m_camera;
        }
    }
}

} // namespace LEGO
