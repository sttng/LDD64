// LEGO Digital Designer — restored source
// LEGO::Scene <-> StorageScene2: building a scene from a loaded model, saving it, and the
// snapshot dump of unplaceable bricks. (file name inferred)
//
// Original object file: Scene.cpp (Mac TU 025): Load Mac 0xb3690 (end of 025n), ApplyBehaviors
// 0xa5a70 (025n) and sub-TU 025o (Mac 0xb49c0-0xb8ec0: DumpSnapshot, LoadGroup, Save, SaveGroup,
// LoadBatchInsertProgressCallback, the debug helpers GetSceneCVPoints Mac 0xb7c30 and
// GetStatistics Mac 0xb7e10 ). Win: Load 0x482e30, LoadGroup 0x482b60, SaveGroup
// 0x4849d0 (the group array push), GetSceneCVPoints 0x484ea0, GetStatistics 0x484ff0.
#include "Scene.h"
#include "Brick.h"
#include "Part.h"
#include "BrickGroup.h"
#include "BrickConnect.h"
#include "BrickDatabase.h"
#include "HingedElement.h"
#include "RigidElement.h"
#include "RigidConnection.h"
#include "PrimitiveRigidElement.h"
#include "PrimitiveElement.h"
#include "PrimitiveConnection.h"
#include "Primitive.h"
#include "ConnectionField.h"
#include "ConnectionFieldPlanar.h"
#include "SceneGraphUtil.h"
#include "ModelIOLXFML5.h"
#include "Platform.h"
#include "Path.h"
#include "LEGO/String.h"
#include "Trace.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

namespace {

// RTTI: LEGO::LoadBatchInsertProgressCallback : Scene::BatchInsertProgressCallback.
// Mac vtable 0x7625d4 {D1 0xb5ad0, D0 0xc89e0, OnProgress 0xc89f0}: the batch insert is
// 25-100 % of a load.
class LoadBatchInsertProgressCallback : public Scene::BatchInsertProgressCallback {
public:
    explicit LoadBatchInsertProgressCallback(Scene::LoadCallback* callback) : m_callback(callback), m_continue(true) {}
    // Mac 0xb5ad0 / 0xc89e0
    virtual ~LoadBatchInsertProgressCallback() {}
    // Mac 0xc89f0
    virtual bool OnProgress(UInt32 percent)
    {
        if (m_callback == 0)
            return true;
        return m_callback->OnProgress((UInt32)(Int64)((Real32)percent * 0.75f) + 25);
    }

    Scene::LoadCallback* m_callback;   // +0x04
    // NOTE: never cleared; Load tests it after BatchInsert (a cancel is not seen there).
    bool                 m_continue;   // +0x08
};

// The BrickConnect cache lock (recursive), held while Load creates the objects.
class BrickConnectLock {
public:
#if defined(_WIN32)
    explicit BrickConnectLock(BrickConnect* brickConnect) : m_lock(brickConnect->m_lock) { EnterCriticalSection(&m_lock); }
    ~BrickConnectLock() { LeaveCriticalSection(&m_lock); }
private:
    CRITICAL_SECTION& m_lock;
#else
    explicit BrickConnectLock(BrickConnect* brickConnect) : m_lock(brickConnect->m_lock) { pthread_mutex_lock(&m_lock); }
    ~BrickConnectLock() { pthread_mutex_unlock(&m_lock); }
private:
    pthread_mutex_t& m_lock;
#endif
    BrickConnectLock(const BrickConnectLock&);
    BrickConnectLock& operator=(const BrickConnectLock&);
};

// Drops the map entries whose value is one of the part's elements (linear search, Mac inline).
void ForgetElements(Scene::StorageMap& map, const Part* part)
{
    for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = part->m_elements.Begin();
         e != part->m_elements.End(); ++e)
        for (BasicMap<const StorageScene2::Bone*, PrimitiveElement*>::Iterator it = map.m_elements.Begin();
             it != map.m_elements.End(); ++it)
            if (it->second == *e) {
                map.m_elements.Erase(it);
                break;
            }
}

// The flex check of a multi-bone part: for every bone with a check connection (a: field of
// this bone's element, b: other bone, c: field of b's element) the two fields must be able
// to connect at the element transforms. (inline in Mac 0xb3690)
bool CheckFlexConnections(const Primitive* primitive, const Part* part)
{
    const BasicArray<RefPointer<PrimitiveElement> >& elements = part->m_elements;
    for (UInt32 i = 0; i < elements.GetSize(); ++i) {
        const Primitive::Bone& bone = primitive->GetBone(i);
        UInt32 otherIndex = bone.m_flexCheckConnection[1];
        if (i == otherIndex)
            continue;
        const PrimitiveElement* element = elements[i];
        const ConnectionField* field = element->m_fields[bone.m_flexCheckConnection[0]];
        Matrix3D transform(element->m_transform, field->GetTransform());
        const PrimitiveElement* other = elements[otherIndex];
        const ConnectionField* otherField = other->m_fields[bone.m_flexCheckConnection[2]];
        Matrix3D otherTransform(other->m_transform, otherField->GetTransform());
        ConnectionField::ConnectionIndices indices;
        if ((field->CanConnect(transform, *otherField, otherTransform, indices) & 6) == 0)
            return false;
    }
    return true;
}

} // namespace

// Mac 0xb3690, Win 0x482e30
bool Scene::Load(const StorageScene2& storage, StorageMap& map, BasicArray<RefPointer<StorageScene2::Brick> >& collided,
                 BasicArray<RefPointer<StorageScene2::Brick> >& missing, BrickDatabase* database,
                 LoadCallback* callback, BatchInsertCollisionReporter* reporter)
{
    if (storage.FindMeta(String("BrickSet")) == 0)
        Trace(String("File does not contain brick set version, no hot fixes possible."), 1);

    BasicArray<RefPointer<Brick> > bricks;
    bool cancelled = false;
    {
        BrickConnectLock lock(m_brickConnect);
        const Real32 count = (Real32)storage.m_bricks.GetSize();
        Real32 left = count;
        for (BasicArray<RefPointer<StorageScene2::Brick> >::ConstIterator b = storage.m_bricks.Begin();
             b != storage.m_bricks.End(); ++b) {
            StorageScene2::Brick* storageBrick = *b;
            if (callback != 0) {
                // 0-25 %. Win (x87) keeps the quotient and the product in double precision, Mac
                // (SSE) rounds them to float, so one value can differ by 1 (C17).
                left += -1.0f;
#if defined(_WIN32)
                const UInt32 percent = (UInt32)(Int64)((1.0 - (Real64)left / (Real64)count) * 25.0);
#else
                const UInt32 percent = (UInt32)(Int64)((1.0f - left / count) * 25.0f);
#endif
                if (!callback->OnProgress(percent)) {
                    cancelled = true;
                    break;
                }
            }
            RefPointer<Brick> brick = m_brickConnect->CreateBrick(storageBrick->m_designID);
            if (brick == 0) {
                missing.PushBack(RefPointer<StorageScene2::Brick>(storageBrick));
                continue;
            }
            brick->m_name = storageBrick->m_itemNos;
            if (callback == 0 || callback->OnBrick(storageBrick, brick)) {
                bool ok = true;
                bool unknown = false;
                for (BasicArray<RefPointer<StorageScene2::Part> >::ConstIterator p = storageBrick->m_parts.Begin();
                     p != storageBrick->m_parts.End() && ok; ++p) {
                    const StorageScene2::Part* storagePart = *p;
                    const Primitive* primitive = database->GetPrimitive(storagePart->m_designID);
                    RefPointer<Part> part = primitive != 0 ? m_brickConnect->CreatePart(primitive) : 0;
                    if (part == 0) {
                        // NOTE: the elements of the brick's earlier parts stay in the map
                        unknown = true;
                        break;
                    }
                    // NOTE: the part's bones are not counted against the primitive's (both
                    // builds): more bones than the primitive has index past its bone array
                    // (CreatePrimitiveElement), a part without bones gets no element, and an
                    // empty bone slot (a flex of an old file with a missing control point) is
                    // dereferenced; fixed: such a brick is not loaded (reported missing; H14,
                    // H15).
                    bool badBones = storagePart->m_bones.IsEmpty() ||
                                    storagePart->m_bones.GetSize() > primitive->GetBoneCount();
                    for (UInt32 i = 0; i < storagePart->m_bones.GetSize() && !badBones; ++i)
                        badBones = storagePart->m_bones[i].IsNull();
                    if (badBones) {
                        unknown = true;
                        break;
                    }
                    part->SetBrick(brick);
                    for (UInt32 i = 0; i < storagePart->m_bones.GetSize(); ++i) {
                        const StorageScene2::Bone* bone = storagePart->m_bones[i];
                        RefPointer<PrimitiveElement> element =
                            m_brickConnect->CreatePrimitiveElement(primitive, i, bone->m_transformation);
                        if (element == 0) {
                            ForgetElements(map, part);
                            ok = false;
                            break;
                        }
                        element->SetPart(part);
                        map.m_elements[bone] = element;
                    }
                    if (!ok)
                        break;
                    if (primitive->m_bones.GetSize() > 1 && !part->m_elements.IsEmpty() &&
                        !CheckFlexConnections(primitive, part)) {
                        ForgetElements(map, part);
                        ok = false;
                        break;
                    }
                    part->SetMaterials(storagePart->m_materials);
                    part->SetDecorations(storagePart->m_decorations);
                    map.m_parts[storagePart] = part;
                }
                if (unknown || !ok) {
                    missing.PushBack(RefPointer<StorageScene2::Brick>(storageBrick));
                    continue;
                }
                bricks.PushBack(brick);
                map.m_bricks[storageBrick] = brick;
            }
            // (a brick the callback refuses is dropped: not loaded, not reported missing)
        }
    }
    if (cancelled)
        return false;

    // 25-100 %
    BasicArray<RefPointer<Brick> > failed;
    LoadBatchInsertProgressCallback progress(callback);
    BatchInsert(bricks, failed, callback != 0 ? &progress : 0, true, reporter);
    if (!progress.m_continue)
        return false;

    if (!failed.IsEmpty()) {
        // the bricks that could not be placed are written to a snapshot (red, 0x15)
        BasicMap<PrimitiveElement*, UInt32> colors;
        BasicMap<RefPointer<PrimitiveElement>, Pair<UInt32, Matrix3D> > extra;
        for (BasicArray<RefPointer<Brick> >::ConstIterator b = failed.Begin(); b != failed.End(); ++b)
            for (BasicArray<RefPointer<Part> >::ConstIterator p = (*b)->m_parts.Begin(); p != (*b)->m_parts.End(); ++p)
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*p)->m_elements.Begin();
                     e != (*p)->m_elements.End(); ++e)
                    extra[*e] = MakePair((UInt32)0x15, (*e)->m_transform);
        if (extra.GetSize() != 0 || colors.GetSize() != 0)
            DumpSnapshot(colors, extra, String("UnplaceableBricksDump"));
        colors.Clear();
        extra.Clear();

        // ... and leave the maps; their storage bricks are reported as collided
        for (BasicArray<RefPointer<Brick> >::ConstIterator b = failed.Begin(); b != failed.End(); ++b) {
            const Brick* brick = *b;
            for (BasicMap<const StorageScene2::Brick*, Brick*>::Iterator it = map.m_bricks.Begin();
                 it != map.m_bricks.End(); ++it)
                if (it->second == brick) {
                    collided.PushBack(RefPointer<StorageScene2::Brick>(const_cast<StorageScene2::Brick*>(it->first)));
                    map.m_bricks.Erase(it);
                    break;
                }
            for (BasicArray<RefPointer<Part> >::ConstIterator p = brick->m_parts.Begin(); p != brick->m_parts.End(); ++p) {
                for (BasicMap<const StorageScene2::Part*, Part*>::Iterator it = map.m_parts.Begin();
                     it != map.m_parts.End(); ++it)
                    if (it->second == *p) {
                        map.m_parts.Erase(it);
                        break;
                    }
                ForgetElements(map, *p);
            }
        }
    }
    ApplyBehaviors(map);
    for (BasicArray<StorageScene2::GroupSystem>::ConstIterator s = storage.m_groupSystems.Begin();
         s != storage.m_groupSystems.End(); ++s)
        for (BasicArray<RefPointer<StorageScene2::Group> >::ConstIterator g = s->m_groups.Begin();
             g != s->m_groups.End(); ++g)
            LoadGroup(map, **g, 0);
    AddBehaviors(storage.m_behaviors);
    return true;
}

// Mac 0xa5a70
void Scene::ApplyBehaviors(StorageMap& map)
{
    for (BasicMap<const StorageScene2::Brick*, Brick*>::Iterator it = map.m_bricks.Begin(); it != map.m_bricks.End();
         ++it)
        it->second->AddBehaviors(it->first->m_behaviors);
    for (BasicMap<const StorageScene2::Part*, Part*>::Iterator it = map.m_parts.Begin(); it != map.m_parts.End(); ++it)
        it->second->AddBehaviors(it->first->m_behaviors);
    for (BasicMap<const StorageScene2::Bone*, PrimitiveElement*>::Iterator it = map.m_elements.Begin();
         it != map.m_elements.End(); ++it)
        it->second->AddBehaviors(it->first->m_behaviors);
}

// Mac 0xb5800, Win 0x482b60
void Scene::LoadGroup(StorageMap& map, const StorageScene2::Group& group, BrickGroup* parent)
{
    BasicArray<RefPointer<Part> > parts;
    for (BasicArray<RefPointer<StorageScene2::Part> >::ConstIterator p = group.m_parts.Begin(); p != group.m_parts.End();
         ++p) {
        BasicMap<const StorageScene2::Part*, Part*>::Iterator found = map.m_parts.Find(*p);
        if (found != map.m_parts.End())
            parts.PushBack(RefPointer<Part>(found->second));
    }
    BrickGroup* loaded = CreateGroup(parent, parts);
    loaded->m_transform = group.m_transformation;
    loaded->m_vector8c = group.m_pivot;
    loaded->m_name = group.m_name;
    loaded->AddBehaviors(group.m_behaviors);
    for (BasicArray<RefPointer<StorageScene2::Group> >::ConstIterator g = group.m_groups.Begin();
         g != group.m_groups.End(); ++g)
        LoadGroup(map, **g, loaded);
}

// Mac 0xb5ae0, Win 0x483ed0
void Scene::Save(StorageScene2& storage, SaveMap& map) const
{
    GetBehaviors(storage.m_behaviors);
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator h = m_hingedElements.Begin();
         h != m_hingedElements.End(); ++h) {
        storage.m_rigidSystems.PushBack(StorageScene2::RigidSystem());
        StorageScene2::RigidSystem& system = storage.m_rigidSystems.Back();
        BasicMap<RigidElement*, StorageScene2::Rigid*> rigids;
        const BasicArray<RefPointer<RigidElement> >& bodies = (*h)->m_elements;

        // a <Rigid> per body, the bricks / parts on first use, a <Bone> per element
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator b = bodies.Begin(); b != bodies.End(); ++b) {
            RigidElement* body = *b;
            Matrix3D bodyTransform;
            body->m_body->GetTransform(bodyTransform);
            RefPointer<StorageScene2::Rigid> rigid = new StorageScene2::Rigid();
            rigid->m_transformation = bodyTransform;
            system.m_rigids.PushBack(rigid);
            rigids[body] = rigid;
            for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator r = body->m_elements.Begin();
                 r != body->m_elements.End(); ++r) {
                Matrix3D rigidTransform(bodyTransform, (*r)->m_transform);
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = (*r)->m_elements.Begin();
                     e != (*r)->m_elements.End(); ++e) {
                    const PrimitiveElement* element = *e;
                    const Part* part = element->m_part;
                    const Brick* brick = part->m_brick;
                    RefPointer<StorageScene2::Brick> storageBrick;
                    BasicMap<const Brick*, StorageScene2::Brick*>::Iterator foundBrick = map.m_bricks.Find(brick);
                    if (foundBrick != map.m_bricks.End()) {
                        storageBrick = foundBrick->second;
                    } else {
                        storageBrick = new StorageScene2::Brick(brick->GetDesignID());
                        storageBrick->m_itemNos = brick->m_name;
                        brick->GetBehaviors(storageBrick->m_behaviors);
                        storage.m_bricks.PushBack(storageBrick);
                        map.m_bricks[brick] = storageBrick;
                    }
                    RefPointer<StorageScene2::Part> storagePart;
                    BasicMap<const Part*, StorageScene2::Part*>::Iterator foundPart = map.m_parts.Find(part);
                    if (foundPart != map.m_parts.End()) {
                        storagePart = foundPart->second;
                    } else {
                        storagePart = new StorageScene2::Part(part->GetDesignID());
                        storagePart->m_materials = part->m_materials;
                        storagePart->m_decorations = part->m_decorations;
                        storagePart->m_bones.Resize(part->m_elements.GetSize(), RefPointer<StorageScene2::Bone>());
                        part->GetBehaviors(storagePart->m_behaviors);
                        storagePart->m_brick = storageBrick;
                        storageBrick->m_parts.PushBack(storagePart);
                        map.m_parts[part] = storagePart;
                    }
                    RefPointer<StorageScene2::Bone> bone = new StorageScene2::Bone();
                    bone->m_transformation.Multiply(rigidTransform, element->m_transform);
                    element->GetBehaviors(bone->m_behaviors);
                    bone->m_part = storagePart;
                    storagePart->m_bones[element->m_boneIndex] = bone;
                    rigid->m_bones.PushBack(bone);
                    map.m_elements[element] = bone;
                }
            }
        }

        // a <Joint> per rigid connection (slide joints are not saved)
        BasicArray<RigidConnection*> saved;
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator b = bodies.Begin(); b != bodies.End(); ++b)
            for (RigidElement::ConnectionMap::Iterator c = (*b)->m_connections.Begin(); c != (*b)->m_connections.End();
                 ++c) {
                RigidConnection* connection = c->second;
                if (connection->m_joint != 0 && connection->m_joint->GetType() == PrimitiveConnection::JointSlide)
                    continue;
                if (SceneGraph::Contains(saved, connection))
                    continue;
                system.m_joints.PushBack(StorageScene2::Joint());
                StorageScene2::Joint& joint = system.m_joints.Back();
                joint.m_rigidA = rigids[connection->m_element];
                joint.m_rigidB = rigids[connection->m_otherElement];
                joint.m_axisA = connection->m_axis;
                joint.m_zeroA = connection->m_normal;
                joint.m_positionA = connection->m_position;
                if (connection->m_element == connection->m_otherElement) {
                    joint.m_axisB = connection->m_axis;
                    joint.m_zeroB = connection->m_normal;
                    joint.m_positionB = connection->m_position;
                } else {
                    joint.m_axisB = connection->m_otherAxis;
                    joint.m_zeroB = connection->m_otherNormal;
                    joint.m_positionB = connection->m_otherPosition;
                }
                if (connection->m_joint != 0) {
                    UInt32 type = connection->m_joint->GetType();
                    if (type == PrimitiveConnection::JointHinge)
                        joint.m_type = StorageScene2::Joint::Hinge;
                    else if (type == PrimitiveConnection::JointBall)
                        joint.m_type = StorageScene2::Joint::Ball;
                }
                saved.PushBack(connection);
            }
    }

    // one <GroupSystem> with the top level groups
    storage.m_groupSystems.PushBack(StorageScene2::GroupSystem());
    BasicArray<RefPointer<StorageScene2::Group> >& groups = storage.m_groupSystems.Back().m_groups;
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator g = m_groups.Begin(); g != m_groups.End(); ++g)
        SaveGroup(map, *g, groups);
}

// Mac 0xb78a0 (array push Mac 0xb7630, Win 0x4849d0)
void Scene::SaveGroup(SaveMap& map, const BrickGroup* group, BasicArray<RefPointer<StorageScene2::Group> >& groups)
{
    groups.PushBack(RefPointer<StorageScene2::Group>(new StorageScene2::Group()));
    StorageScene2::Group* saved = groups.Back();
    group->GetBehaviors(saved->m_behaviors);
    saved->m_name = group->m_name;
    saved->m_pivot = group->m_vector8c;
    saved->m_transformation = group->m_transform;
    for (BasicArray<RefPointer<Part> >::ConstIterator p = group->m_parts.Begin(); p != group->m_parts.End(); ++p) {
        // NOTE: operator[]: a part that was not saved adds a null entry to the map and a null
        // part reference to the group (once)
        RefPointer<StorageScene2::Part> part = map.m_parts[*p];
        if (!SceneGraph::Contains(saved->m_parts, part))
            saved->m_parts.PushBack(part);
    }
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator g = group->m_groups.Begin(); g != group->m_groups.End(); ++g)
        SaveGroup(map, *g, saved->m_groups);
}

// Mac 0xb49c0
void Scene::DumpSnapshot(const BasicMap<PrimitiveElement*, UInt32>& colors,
                         const BasicMap<RefPointer<PrimitiveElement>, Pair<UInt32, Matrix3D> >& extra,
                         const String& name) const
{
    RefPointer<StorageScene2> storage = new StorageScene2();
    StorageScene2::Meta brand;
    brand.m_name = String("Brand");
    StorageScene2::Meta::Attribute attribute;
    attribute.m_name = String("name");
    attribute.m_value = String("FreeBuild");
    brand.m_attributes.PushBack(attribute);
    storage->m_metas.PushBack(brand);

    SaveMap map;
    Save(*storage, map);
    // every part one material (24, overridden by `colors`), no decorations
    for (BasicArray<RefPointer<StorageScene2::Brick> >::Iterator b = storage->m_bricks.Begin();
         b != storage->m_bricks.End(); ++b)
        for (BasicArray<RefPointer<StorageScene2::Part> >::Iterator p = (*b)->m_parts.Begin(); p != (*b)->m_parts.End();
             ++p) {
            BasicArray<UInt32>& materials = (*p)->m_materials;
            // NOTE: the original writes 24 before resizing (an empty list ends up as {0})
            if (materials.IsEmpty())
                materials.Resize(1, 0);
            else
                materials.Resize(1, 0), materials[0] = 24;
            (*p)->m_decorations.Clear();
        }
    for (BasicMap<PrimitiveElement*, UInt32>::ConstIterator it = colors.Begin(); it != colors.End(); ++it)
        map.m_elements[it->first]->m_part->m_materials[0] = it->second;

    // the extra elements as bricks of their own (designID of the primitive) at the given
    // transforms, all materials the given color
    for (BasicMap<RefPointer<PrimitiveElement>, Pair<UInt32, Matrix3D> >::ConstIterator it = extra.Begin();
         it != extra.End(); ++it) {
        const PrimitiveElement* element = it->first;
        UInt32 designID = element->m_primitive->GetDesignID();
        RefPointer<StorageScene2::Brick> brick = new StorageScene2::Brick(designID);
        storage->m_bricks.PushBack(brick);
        RefPointer<StorageScene2::Part> part = new StorageScene2::Part(designID);
        if (element->m_part == 0)
            part->m_materials.Resize(1, 1);
        else
            part->m_materials = element->m_part->m_materials;
        part->m_bones.Resize(1, RefPointer<StorageScene2::Bone>());
        part->m_brick = brick;
        brick->m_parts.PushBack(part);
        RefPointer<StorageScene2::Bone> bone = new StorageScene2::Bone();
        bone->m_transformation = it->second.second;
        bone->m_part = part;
        part->m_bones[0] = bone;
        for (UInt32 i = 0; i < part->m_materials.GetSize(); ++i)
            part->m_materials[i] = it->second.first;
    }

    // "<app data>/LEGO Company/<name>.lxfml"; the result messages are built but not shown
    RefPointer<Directory> directory = Platform::GetInstance()->GetSystemDirectory(2, true);
    if (directory == 0) {
        String message("Scene snapshot dump: Unable to open app data directory");
        return;
    }
    Path path;
    path.SetElement(directory);
    String relative("LEGO Company/");
    relative << name << ".lxfml";
    path.Append(relative);
    RefPointer<ModelSaverLXFML5> saver = new ModelSaverLXFML5();
    if (saver->Save(path, *storage)) {
        String message("Scene snapshot dump: saved model file: ");
        message << path.GetPath(true);
    } else {
        String message("Scene snapshot dump: Unable to save model file: ");
        message << path.GetPath(true);
    }
}

// ---------------------------------------------------------------------------------------------
// GetSceneCVPoints (Mac 0xb7c30 in this sub-TU; the visitor's slots are emitted in 025s)
// ---------------------------------------------------------------------------------------------

namespace {

// RTTI: LEGO::GetSceneCVPointsCollisionVolumeVisitor : PrimitiveElement::CollisionVolumeVisitor.
// Mac vtable 0x7626bc {0xc9500, 0xc9580, 0xc9900}; Win slots reversed {0x484a40 capsule,
// 0x484ab0 box, 0x484db0 sphere}. Layout {vptr, points +4, margin +8}.
class GetSceneCVPointsCollisionVolumeVisitor : public PrimitiveElement::CollisionVolumeVisitor {
public:
    GetSceneCVPointsCollisionVolumeVisitor(BasicArray<Pair<Vector3, Real32> >& points, Real32 margin)
        : m_points(points), m_margin(margin) {}

    // Mac 0xc9500, Win 0x484db0: the centre, radius + margin
    virtual void Visit(PhysicsGeometrySphere* sphere, const Matrix3D& transform)
    {
        Vector3 center = Vector3() * transform;
        Real32 radius = m_margin + sphere->GetRadius();
        m_points.PushBack(Pair<Vector3, Real32>(Vector3(center), radius));
    }
    // Mac 0xc9580, Win 0x484ab0: the eight corners of the box grown by the margin, radius 0
    virtual void Visit(PhysicsGeometryBox* box, const Matrix3D& transform)
    {
        Vector3 half = box->GetHalfSize() + m_margin;
        BoundingBox bounds(half);
        bounds.m_min = -bounds.m_max;
        const Vector3& lo = bounds.m_min;
        const Vector3& hi = bounds.m_max;
        Push(lo, transform);
        Push(hi, transform);
        Push(Vector3(lo.x, lo.y, hi.z), transform);
        Push(Vector3(lo.x, hi.y, lo.z), transform);
        Push(Vector3(hi.x, lo.y, lo.z), transform);
        Push(Vector3(hi.x, hi.y, lo.z), transform);
        Push(Vector3(hi.x, lo.y, hi.z), transform);
        Push(Vector3(lo.x, hi.y, hi.z), transform);
    }
    // Mac 0xc9900, Win 0x484a40: both end centres (0 and length along x), radius + margin
    virtual void Visit(PhysicsGeometryCapsule* capsule, const Matrix3D& transform)
    {
        Vector3 start = Vector3() * transform;
        Real32 radius = m_margin + capsule->GetRadius();
        m_points.PushBack(Pair<Vector3, Real32>(Vector3(start), radius));
        Vector3 end = Vector3(capsule->GetLength(), 0.0f, 0.0f) * transform;
        Real32 endRadius = m_margin + capsule->GetRadius();
        m_points.PushBack(Pair<Vector3, Real32>(Vector3(end), endRadius));
    }

private:
    void Push(const Vector3& corner, const Matrix3D& transform)
    {
        Vector3 point = corner * transform;
        m_points.PushBack(Pair<Vector3, Real32>(Vector3(point), 0.0f));
    }

    BasicArray<Pair<Vector3, Real32> >& m_points;   // +0x04
    Real32                              m_margin;   // +0x08
};

// The number of points the visitor above pushes (Scene::GetSceneCVSpheres).
class CountSceneCVPointsVisitor : public PrimitiveElement::CollisionVolumeVisitor {
public:
    CountSceneCVPointsVisitor() : m_count(0) {}
    virtual void Visit(PhysicsGeometrySphere*, const Matrix3D&) { m_count += 1; }
    virtual void Visit(PhysicsGeometryBox*, const Matrix3D&) { m_count += 8; }
    virtual void Visit(PhysicsGeometryCapsule*, const Matrix3D&) { m_count += 2; }
    UInt32 m_count;
};

// The visitor above with spheres for its points: the same values in the same order.
class GetSceneCVSpheresVisitor : public PrimitiveElement::CollisionVolumeVisitor {
public:
    GetSceneCVSpheresVisitor(BasicArray<Sphere>& spheres, Real32 margin) : m_spheres(spheres), m_margin(margin) {}

    virtual void Visit(PhysicsGeometrySphere* sphere, const Matrix3D& transform)
    {
        Vector3 center = Vector3() * transform;
        Real32 radius = m_margin + sphere->GetRadius();
        m_spheres.PushBack(Sphere(Vector3(center), radius));
    }
    virtual void Visit(PhysicsGeometryBox* box, const Matrix3D& transform)
    {
        Vector3 half = box->GetHalfSize() + m_margin;
        BoundingBox bounds(half);
        bounds.m_min = -bounds.m_max;
        const Vector3& lo = bounds.m_min;
        const Vector3& hi = bounds.m_max;
        Push(lo, transform);
        Push(hi, transform);
        Push(Vector3(lo.x, lo.y, hi.z), transform);
        Push(Vector3(lo.x, hi.y, lo.z), transform);
        Push(Vector3(hi.x, lo.y, lo.z), transform);
        Push(Vector3(hi.x, hi.y, lo.z), transform);
        Push(Vector3(hi.x, lo.y, hi.z), transform);
        Push(Vector3(lo.x, hi.y, hi.z), transform);
    }
    virtual void Visit(PhysicsGeometryCapsule* capsule, const Matrix3D& transform)
    {
        Vector3 start = Vector3() * transform;
        Real32 radius = m_margin + capsule->GetRadius();
        m_spheres.PushBack(Sphere(Vector3(start), radius));
        Vector3 end = Vector3(capsule->GetLength(), 0.0f, 0.0f) * transform;
        Real32 endRadius = m_margin + capsule->GetRadius();
        m_spheres.PushBack(Sphere(Vector3(end), endRadius));
    }

private:
    void Push(const Vector3& corner, const Matrix3D& transform)
    {
        Vector3 point = corner * transform;
        m_spheres.PushBack(Sphere(Vector3(point), 0.0f));
    }

    BasicArray<Sphere>& m_spheres;
    Real32              m_margin;
};

} // namespace

void Scene::GetSceneCVSpheres(BasicArray<Sphere>& spheres, Real32 margin) const
{
    // the walk of GetSceneCVPoints
    auto walk = [this](PrimitiveElement::CollisionVolumeVisitor& visitor) {
        for (UInt32 h = 0; h < m_hingedElements.GetSize(); ++h) {
            const HingedElement* hinged = m_hingedElements[h];
            for (UInt32 b = 0; b < hinged->m_elements.GetSize(); ++b) {
                const RigidElement* body = hinged->m_elements[b];
                Matrix3D bodyTransform;
                body->GetTransform(bodyTransform);
                for (UInt32 r = 0; r < body->m_elements.GetSize(); ++r) {
                    const PrimitiveRigidElement* rigid = body->m_elements[r];
                    Matrix3D rigidTransform(bodyTransform, rigid->m_transform);
                    for (UInt32 e = 0; e < rigid->m_elements.GetSize(); ++e) {
                        const PrimitiveElement* element = rigid->m_elements[e];
                        Matrix3D transform(rigidTransform, element->m_transform);
                        element->VisitCollisionVolumes(visitor, transform);
                    }
                }
            }
        }
    };
    CountSceneCVPointsVisitor counter;
    walk(counter);
    spheres.Reserve(spheres.GetSize() + counter.m_count);
    GetSceneCVSpheresVisitor visitor(spheres, margin);
    walk(visitor);
}

// Mac 0xb7c30, Win 0x484ea0
void Scene::GetSceneCVPoints(BasicArray<Pair<Vector3, Real32> >& points, Real32 margin) const
{
    GetSceneCVPointsCollisionVolumeVisitor visitor(points, margin);
    for (UInt32 h = 0; h < m_hingedElements.GetSize(); ++h) {
        const HingedElement* hinged = m_hingedElements[h];
        for (UInt32 b = 0; b < hinged->m_elements.GetSize(); ++b) {
            const RigidElement* body = hinged->m_elements[b];
            Matrix3D bodyTransform;
            body->GetTransform(bodyTransform);
            for (UInt32 r = 0; r < body->m_elements.GetSize(); ++r) {
                const PrimitiveRigidElement* rigid = body->m_elements[r];
                Matrix3D rigidTransform(bodyTransform, rigid->m_transform);
                for (UInt32 e = 0; e < rigid->m_elements.GetSize(); ++e) {
                    const PrimitiveElement* element = rigid->m_elements[e];
                    Matrix3D transform(rigidTransform, element->m_transform);
                    element->VisitCollisionVolumes(visitor, transform);
                }
            }
        }
    }
}

// =============================================================================================
// Statistics
// =============================================================================================

// Mac 0xb7e10, Win 0x484ff0 (+ chunk 0x485345)
void Scene::GetStatistics(Statistics& statistics) const
{
    UInt32* s = statistics.m_counts;
    BasicArray<UInt32> brickDesigns;        // [10]
    BasicArray<UInt32> materials;           // [13]
    BasicArray<UInt32> decorations;         // [14]
    BasicArray<UInt32> partDesigns;         // [11]
    BasicArray<const Brick*> bricks;        // [15] (Win RTTI BasicArray<Brick const *>)
    BasicArray<const Part*> parts;          // [16] (Win RTTI BasicArray<Part const *>)
    for (UInt32 i = 0; i < StatCount; ++i)
        s[i] = 0;
    for (BasicArray<RefPointer<HingedElement> >::ConstIterator h = m_hingedElements.Begin();
         h != m_hingedElements.End(); ++h) {
        ++s[StatHingedElements];
        const HingedElement* hinged = *h;
        for (BasicArray<RefPointer<RigidElement> >::ConstIterator b = hinged->m_elements.Begin();
             b != hinged->m_elements.End(); ++b) {
            const RigidElement* body = *b;
            ++s[StatRigidElements];
            s[StatRigidConnections] += body->m_connections.GetSize();
            for (BasicArray<RefPointer<PrimitiveRigidElement> >::ConstIterator r = body->m_elements.Begin();
                 r != body->m_elements.End(); ++r) {
                const PrimitiveRigidElement* rigid = *r;
                ++s[StatPrimitiveRigidElements];
                s[StatPrimitiveRigidConnections] += rigid->m_connections.GetSize();
                for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = rigid->m_elements.Begin();
                     e != rigid->m_elements.End(); ++e) {
                    const PrimitiveElement* element = *e;
                    ++s[StatPrimitiveElements];
                    s[StatPrimitiveConnections] += element->m_connections.GetSize();
                    for (PrimitiveElement::ConnectionMap::ConstIterator c = element->m_connections.Begin();
                         c != element->m_connections.End(); ++c)
                        s[StatFieldConnections] += c->second->m_connections.GetSize();
                    s[StatConnectionFields] += element->m_fields.GetSize();

                    const Part* part = element->m_part;
                    const Brick* brick = part->m_brick;
                    SceneGraph::PushBackUnique(brickDesigns, brick->GetDesignID());
                    UInt32 partDesign = part->GetDesignID();
                    if (!SceneGraph::Contains(partDesigns, partDesign)) {
                        partDesigns.PushBack(part->GetDesignID());
                        s[StatUniqueBones] += element->m_primitive->m_bones.GetSize();
                    }
                    SceneGraph::PushBackUnique(bricks, brick);
                    SceneGraph::PushBackUnique(parts, part);
                    ++s[StatInstanceBones];

                    // materials: the part's slots; an instance counts unless the primitive
                    // redirects the slot to another one
                    const BasicArray<UInt32>& redirect = element->m_primitive->m_subMaterialRedirect;
                    for (UInt32 i = 0; i < part->m_materials.GetSize(); ++i) {
                        SceneGraph::PushBackUnique(materials, part->GetMaterial(i));
                        if (redirect.IsEmpty() || redirect[i] == i)
                            ++s[StatInstanceMaterials];
                    }
                    for (BasicArray<UInt32>::ConstIterator d = part->m_decorations.Begin();
                         d != part->m_decorations.End(); ++d) {
                        if (*d == 0)
                            continue;
                        SceneGraph::PushBackUnique(decorations, *d);
                        ++s[StatInstanceDecorations];
                    }

                    // collision volumes: a configurable geometry (the knobs) counts its parts
                    PhysicsPrimitiveElement* physics = element->m_physics;
                    PhysicsPlacedConfiguredGeometry placed;
                    for (UInt32 i = 0; i < physics->GetGeometryCount(); ++i) {
                        if (!physics->GetGeometry(i, placed))
                            continue;
                        if (placed.m_geometry->GetKind() == PhysicsGeometry::KindConfigurable) {
                            void* object = 0;
                            if (placed.m_geometry->QueryInterface(PhysicsGeometryConfigable::GetIID(), &object) >= 0) {
                                PhysicsGeometryConfigable* configurable = static_cast<PhysicsGeometryConfigable*>(object);
                                s[StatActiveCollisionVolumes] += configurable->GetGeometryCount(placed.m_type);
                                const Primitive::CollisionVolumeToAndFromFeatureLookup& lookup =
                                    element->m_bone->m_featureLookup;
                                Int32 index = (Int32)(i - lookup.m_count);
                                if (index >= 0) {
                                    const Primitive::CollisionVolumeToAndFromFeatureLookup::Feature& feature =
                                        lookup.m_features[index];
                                    const ConnectionField* field = element->m_fields[feature.m_volume];
                                    UInt32 type = 1;
                                    if (field->GetType() == ConnectionField::TYPE_PLANAR)
                                        type = PrimitiveElement::GetFeatureType(
                                            static_cast<const ConnectionFieldPlanar*>(field)->GetPoint(feature.m_feature));
                                    s[StatCollisionVolumes] += configurable->GetGeometryCount(type);
                                }
                                configurable->Release();
                            }
                        } else {
                            ++s[StatCollisionVolumes];
                            if (placed.m_type != 0)
                                ++s[StatActiveCollisionVolumes];
                        }
                        placed.m_geometry->Release();
                    }
                }
            }
        }
    }
    // the connections were counted from both sides
    s[StatRigidConnections] >>= 1;
    s[StatPrimitiveRigidConnections] >>= 1;
    s[StatPrimitiveConnections] >>= 1;
    s[StatFieldConnections] >>= 1;
    s[StatUniqueBricks] = brickDesigns.GetSize();
    s[StatUniqueParts] = partDesigns.GetSize();
    s[StatUniqueMaterials] = materials.GetSize();
    s[StatUniqueDecorations] = decorations.GetSize();
    s[StatInstanceBricks] = bricks.GetSize();
    s[StatInstanceParts] = parts.GetSize();
}

} // namespace LEGO
