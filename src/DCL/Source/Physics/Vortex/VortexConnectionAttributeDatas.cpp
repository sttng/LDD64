// LEGO Digital Designer — restored source
// LEGO::VortexConnectionAttributeDatas (see VortexConnectionAttributeDatas.h). The original emits
// these functions in VortexPhysicsScene.cpp (Mac TU 435); restored with the joints.
#include "VortexConnectionAttributeDatas.h"
#include "VortexPhysicsScene.h"
#include "BaseBrickPhysicsConnectionHelper.h"

#include "Vx/VxConstraint.h"
#include "Vx/VxAssemblyBase.h"

namespace LEGO {

// Mac 0x778d70 (1000.0f), Win folded (0xae0ed8 = 1000.0).
extern const Real32 g_vortexConstraintStiffnessScale = 1000.0f;

// Mac 0x779014, Win 0xb44854
UInt32 VortexConnectionAttributeDatas::s_lastId = 0;

// Mac 0x3b2100 (push_back 0x3b2180), Win 0x528100
void* VortexConnectionAttributeDatas::Add(BaseBrickPhysicsConnectionHelper* helper,
                                          const ConnectionField::Attributes& attributes)
{
    helper->DestroyConstraint();
    helper->MarkDirty();
    AttributesData data;
    data.m_attributes = attributes;
    data.m_constraint = 0;
    data.m_constraint2 = 0;
    data.m_id = ++s_lastId;
    m_datas.PushBack(data);
    return (void*)(size_t)m_datas.Back().m_id;
}

// Mac 0x3b22b0, Win 0x528190
void VortexConnectionAttributeDatas::Remove(BaseBrickPhysicsConnectionHelper* helper, void* handle)
{
    helper->DestroyConstraint();
    helper->MarkDirty();
    UInt32 index = 0;
    for (;; ++index) {
        if (index == m_datas.GetSize())
            return;
        if (m_datas[index].m_id == (UInt32)(size_t)handle)
            break;
    }
    AttributesData& data = m_datas[index];
    Vx::VxMechanism* mechanism = static_cast<VortexPhysicsScene*>(helper->m_scene)->m_mechanism;
    if (data.m_constraint != 0) {
        mechanism->removeConstraint(data.m_constraint);
        delete data.m_constraint;
    }
    if (data.m_constraint2 != 0) {
        mechanism->removeConstraint(data.m_constraint2);
        delete data.m_constraint2;
    }
    m_datas.Erase(index);
}

// Mac 0x3b23f0, Win 0x528250
bool VortexConnectionAttributeDatas::HasMotors() const
{
    for (const AttributesData* it = m_datas.Begin(); it != m_datas.End(); ++it) {
        if (it->m_attributes.m_hasLinear)
            return true;
        if (it->m_attributes.m_hasSpring)
            return true;
    }
    return false;
}

// Mac 0x3b2450, Win 0x528290
void VortexConnectionAttributeDatas::DestroyConstraints(Vx::VxMechanism* mechanism)
{
    for (AttributesData* it = m_datas.Begin(); it != m_datas.End(); ++it) {
        if (it->m_constraint != 0) {
            mechanism->removeConstraint(it->m_constraint);
            delete it->m_constraint;
            it->m_constraint = 0;
        }
        if (it->m_constraint2 != 0) {
            mechanism->removeConstraint(it->m_constraint2);
            delete it->m_constraint2;
            it->m_constraint2 = 0;
        }
    }
}

} // namespace LEGO
