// Vortex 5.0.0 build 800 "vortexldd41" - VxNamePathBaseInterface and the name-path interfaces of
// the dynamics objects (restored). Mac TU 505 0x464682-0x4673a3 (static
// initialiser 0x46667d: <iostream> and the seven registrations below, each followed by its
// atexit removal 0x4650a6 / 0x464e9d / 0x4656c1 / 0x4654b8 / 0x464a79 / 0x4652af / 0x465a22;
// the registry's own atexit 0x4646e9), Win 0x822710-0x823060.
// The getters (Mac 0x4661c3-0x4663fd: function-local static interfaces, guards 0x779398-0x7793c0)
// and type tests (Mac 0x466470-0x466632, Win 0x822f00-0x823020) are the templates of
// Vx/VxNamePathBaseInterface.h.
// Not restored here (Win only, dead-stripped on Mac; they belong with the Win-only modules):
// the name-path lookup Win 0x822840 (vehicle / assembly class-name rules),
// 0x822930, 0x822b90, 0x822df0, 0x8231b0, used by VxBaseReference (Win 0x7f74e0-0x7f78d0, the
// cable code), and the eighth Win interface "VxNameBaseBaseUnknownInterface".
#include "Vx/VxNamePathAssemblyInterface.h"
#include "Vx/VxFrame.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxEntity.h"
#include "Vx/VxPart.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxConstraint.h"
#include <iostream>

namespace Vx {

// ---------------------------------------------------------------------------------------------
// VxNamePathBaseInterface

// Mac D2 0x4646bc (D1 0x4646d6, D0 0x464692), Win 0x7fa410
VxNamePathBaseInterface::~VxNamePathBaseInterface()
{
}

// Mac 0x464682, Win 0x822710
VxBase* VxNamePathBaseInterface::getParent(VxBase*) const
{
    return 0;
}

// Mac 0x46468c, Win 0x822720
void VxNamePathBaseInterface::getChildren(VxBase*, VxArray<VxBase*>&) const
{
}

// ---------------------------------------------------------------------------------------------
// parents

// Mac 0x464734, Win 0x8227f0
VxBase* VxNamePathAssemblyInterface::getParent(VxBase* object) const
{
    VxAssemblyBase* a = object != 0 ? dynamic_cast<VxAssemblyBase*>(object) : 0;
    if (a == 0)
        return 0;
    if (a->mParent != 0)
        return a->mParent;
    return (VxBase*)a->mUniverse;
}

// Mac 0x464790, Win 0x8227b0
VxBase* VxNamePathCollisionSensorInterface::getParent(VxBase* object) const
{
    VxCollisionSensor* s = object != 0 ? dynamic_cast<VxCollisionSensor*>(object) : 0;
    if (s == 0)
        return 0;
    return s->mAssembly;
}

// Mac 0x4647dc, Win 0x822770: VxConstraint +0x34, the constraint's assembly (UniverseConnector)
VxBase* VxNamePathConstraintInterface::getParent(VxBase* object) const
{
    VxConstraint* c = object != 0 ? dynamic_cast<VxConstraint*>(object) : 0;
    if (c == 0)
        return 0;
    return static_cast<VxAssemblyBase*>(c->m34);
}

// Mac 0x464828, Win 0x822730
VxBase* VxNamePathCollisionGeometryInterface::getParent(VxBase* object) const
{
    VxCollisionGeometry* cg = object != 0 ? dynamic_cast<VxCollisionGeometry*>(object) : 0;
    if (cg == 0)
        return 0;
    return cg->mEntity;
}

// Mac 0x4661b6, Win 0x823050
VxBase* VxNamePathUniverseInterface::getParent(VxBase*) const
{
    return VxFrame::instance();
}

// ---------------------------------------------------------------------------------------------
// children

// Mac 0x464932, Win 0x822b20
void VxNamePathFrameInterface::getChildren(VxBase* object, VxArray<VxBase*>& children) const
{
    VxFrame* f = object != 0 ? dynamic_cast<VxFrame*>(object) : 0;
    if (f == 0)
        return;
    for (int i = 0; i < f->getUniverseCount(); ++i)
        children.push_back((VxBase*)f->getUniverse(i));
}

// Mac 0x464c82, Win 0x822a90
void VxNamePathUniverseInterface::getChildren(VxBase* object, VxArray<VxBase*>& children) const
{
    VxUniverse* u = object != 0 ? dynamic_cast<VxUniverse*>(object) : 0;
    if (u == 0)
        return;
    children.push_back(u->mRootMechanism);
    if (u->m8c) {
        for (int i = 0; i < u->mRootMechanism->getAssemblyCount(); ++i)
            children.push_back(u->mRootMechanism->getAssembly(i));
    }
}

// Mac 0x4658ca, Win 0x822940
void VxNamePathCollisionSensorInterface::getChildren(VxBase* object, VxArray<VxBase*>& children) const
{
    VxCollisionSensor* s = object != 0 ? dynamic_cast<VxCollisionSensor*>(object) : 0;
    if (s == 0)
        return;
    for (int i = 0; i < s->getCollisionGeometryCount(); ++i)
        children.push_back(s->getCollisionGeometry(i));
}

// Mac 0x465c2c, Win 0x8229b0
void VxNamePathAssemblyInterface::getChildren(VxBase* object, VxArray<VxBase*>& children) const
{
    VxAssemblyBase* a = object != 0 ? dynamic_cast<VxAssemblyBase*>(object) : 0;
    if (a == 0)
        return;
    for (VxPart** i = a->mParts.begin(); i != a->mParts.end(); ++i)
        children.push_back(*i);
    for (VxConstraint** i = a->mConstraints.begin(); i != a->mConstraints.end(); ++i)
        children.push_back(*i);
    for (VxAssemblyBase** i = a->m44.begin(); i != a->m44.end(); ++i)
        children.push_back(*i);
    children.push_back(&a->mController);
}

// ---------------------------------------------------------------------------------------------
// destructors (Mac D1 / D0; Win: one ICF-shared scalar deleting destructor 0x823060)

VxNamePathFrameInterface::~VxNamePathFrameInterface() {}                         // Mac 0x46619c / 0x466172
VxNamePathUniverseInterface::~VxNamePathUniverseInterface() {}                   // Mac 0x466158 / 0x46612e
VxNamePathAssemblyInterface::~VxNamePathAssemblyInterface() {}                   // Mac 0x466114 / 0x4660ea
VxNamePathCollisionSensorInterface::~VxNamePathCollisionSensorInterface() {}     // Mac 0x4660d0 / 0x4660a6
VxNamePathConstraintInterface::~VxNamePathConstraintInterface() {}               // Mac 0x466048 / 0x46601e
VxNamePathCollisionGeometryInterface::~VxNamePathCollisionGeometryInterface() {} // Mac 0x46608c / 0x466062

// ---------------------------------------------------------------------------------------------
// the registrations of Mac 0x46667d (in this order)

static VxNamePathInterfaceRegistration<VxFrame, VxNamePathFrameInterface> sFrameRegistration;
static VxNamePathInterfaceRegistration<VxUniverse, VxNamePathUniverseInterface> sUniverseRegistration;
static VxNamePathInterfaceRegistration<VxMechanism, VxNamePathAssemblyInterface> sMechanismRegistration;
static VxNamePathInterfaceRegistration<VxCollisionSensor, VxNamePathCollisionSensorInterface> sSensorRegistration;
static VxNamePathInterfaceRegistration<VxCollisionGeometry, VxNamePathCollisionGeometryInterface> sGeometryRegistration;
static VxNamePathInterfaceRegistration<VxAssemblyBase, VxNamePathAssemblyInterface> sAssemblyRegistration;
static VxNamePathInterfaceRegistration<VxConstraint, VxNamePathConstraintInterface> sConstraintRegistration;

} // namespace Vx
