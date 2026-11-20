// LEGO Digital Designer — restored source
// LEGO::BaseBrickPhysicsRigidElement (Mac TU 210). See BaseBrickPhysicsRigidElement.h.
#include "BaseBrickPhysicsRigidElement.h"
#include "BaseBrickPhysicsPrimitiveElement.h"
#include "BaseBrickPhysicsScene.h"
#include "BaseBrickPhysics.h"
#include "BaseBrickPhysicsAtomic.h"

#include <string.h>

namespace LEGO {

// Mac 0x253910, Win 0x5c7870
Int32 BaseBrickPhysicsRigidElement::QueryInterface(const InterfaceID& id, void** object)
{
    if (memcmp(&id, &PhysicsRigidElement::GetIID(), sizeof id) == 0) {
        *object = this;
    } else if (memcmp(&id, &PhysicsSceneElement::GetIID(), sizeof id) == 0) {
        *object = this;
    } else if (memcmp(&id, &PhysicsTransformReporter::GetIID(), sizeof id) == 0) {
        *object = this;   // (added) m_transform changes are reported
    } else {
        *object = 0;
        return kPhysicsNoInterface;
    }
    AddRef();
    return 0;
}

// Mac 0x2539e0, Win 0x5c78e0
UInt32 BaseBrickPhysicsRigidElement::AddRef()
{
    return (UInt32)PhysicsAtomicIncrement(m_refCount);
}

// Mac 0x253a00, Win 0x5c7900
UInt32 BaseBrickPhysicsRigidElement::Release()
{
    Int32 count = PhysicsAtomicDecrement(m_refCount);
    if (count == 0)
        delete this;
    return (UInt32)count;
}

// Mac 0x253a40, Win 0x5c7930
void BaseBrickPhysicsRigidElement::GetTransform(Matrix3D& transform) const
{
    transform = m_transform;
}

// Mac 0x254050, Win 0x52b800
const PhysicsMassProperties& BaseBrickPhysicsRigidElement::GetMassProperties() const
{
    return m_massProperties;
}

// Mac 0x254060, Win 0x42c9c0
void* BaseBrickPhysicsRigidElement::GetUserData() const
{
    return m_userData;
}

// Mac 0x254070, Win 0x4f9310
void BaseBrickPhysicsRigidElement::SetUserData(RigidElement* element)
{
    m_userData = element;
}

// (inline in Mac 0x253ac0 / 0x253d30 / 0x253f90)
void BaseBrickPhysicsRigidElement::MarkDirty()
{
    if (!m_dirty) {
        m_scene->MarkRigidElementDirty(this);
        m_dirty = true;
    }
}

// Mac 0x253ac0, Win 0x5c7950
void BaseBrickPhysicsRigidElement::AddPrimitiveElement(PhysicsPrimitiveElement* element,
                                                       const Matrix3D& transform)
{
    BaseBrickPhysicsPrimitiveElement* primitive = static_cast<BaseBrickPhysicsPrimitiveElement*>(element);
    m_primitives.PushBack(primitive);
    primitive->m_owner = this;
    primitive->SetTransform(transform);
    OnAddPrimitiveElement(element);
    // NOTE: the scene must be set (AddToScene) before the first primitive element is added
    MarkDirty();
}

// Mac 0x253d30, Win 0x5c79c0
void BaseBrickPhysicsRigidElement::RemovePrimitiveElement(PhysicsPrimitiveElement* element)
{
    BaseBrickPhysicsPrimitiveElement* primitive = static_cast<BaseBrickPhysicsPrimitiveElement*>(element);
    OnRemovePrimitiveElement(element);
    primitive->m_owner = 0;
    PhysicsArrayRemove(m_primitives, primitive);
    MarkDirty();
}

// Mac 0x254080, Win 0x52b810
void BaseBrickPhysicsRigidElement::Update()
{
    m_dirty = false;
}

// Mac 0x253f90 (Win inlined into BaseBrickPhysicsScene::AddRigidElement 0x5c5d90)
void BaseBrickPhysicsRigidElement::AddToScene(BaseBrickPhysicsScene* scene, const Matrix3D& transform)
{
    m_scene = scene;
    m_transform = transform;
    NoteTransformChanged();
    if (!m_dirty) {
        scene->MarkRigidElementDirty(this);
        m_dirty = true;
    }
}

// Mac 0x254030 (Win inlined into BaseBrickPhysicsScene::RemoveRigidElement 0x5c5e00)
void BaseBrickPhysicsRigidElement::RemoveFromScene()
{
    ReleaseEngineBody();
    m_scene = 0;
}

} // namespace LEGO
