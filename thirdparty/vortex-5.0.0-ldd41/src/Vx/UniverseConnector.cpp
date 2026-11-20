// Vortex 5.0.0 build 800 "vortexldd41" - Vx::UniverseConnector + NotificationVisitor<VxUniverse>
// (restored). Mac TU 453 (0x422e20-0x42405b; file guess "NotificationVisitor.cpp",
// static initialiser 0x423ff6: <iostream> only), Win 0x750160-0x750790. The TU's print-file helper
// (Mac 0x422e20-0x422f26) is VxPrintFile (Vx/VxMaterial.h); the empty Mac 0x4236fa / Win 0x70d070
// is VxArray's element-destroy loop.
#include "UniverseConnector.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxEntity.h"
#include "Vx/VxConstraint.h"
#include "Vx/VxCollisionGeometry.h"
#include <iostream>

namespace Vx {

// Mac 0x422f40 (Win: inlined)
void UniverseConnector::connectEntity(VxAssemblyBase* a, VxEntity* e)
{
    e->mAssembly = a;
    e->setUniverse(a->mUniverse);
}

// Mac 0x422f86, Win 0x750200
void UniverseConnector::insertBody(VxUniverse* u, MdtBodyID body)
{
    u->insertBody(body);
}

// Mac 0x422fa0, Win 0x7501e0
void UniverseConnector::removeCollisionModel(VxUniverse* u, McdModelID model)
{
    u->removeCollisionModel(model);
}

// Mac 0x422fba, Win 0x7501c0
void UniverseConnector::insertCollisionModel(VxUniverse* u, McdModelID model)
{
    u->insertCollisionModel(model);
}

// Mac 0x422fd4, Win 0x750180 (VxConstraint +0x34: the owning assembly)
void UniverseConnector::disconnectConstraint(VxConstraint* c)
{
    c->m34 = 0;
    c->setUniverse(0);
}

// Mac 0x422ff6, Win 0x750160
void UniverseConnector::connectConstraint(VxAssemblyBase* a, VxConstraint* c)
{
    c->m34 = a;
    c->setUniverse(a->mUniverse);
}

// Win 0x7501a0 / 0x7501b0 (dead-stripped on the Mac)
void UniverseConnector::connectCollisionGeometry(VxCollisionSensor* s, VxCollisionGeometry* cg)
{
    cg->mEntity = s;
}

void UniverseConnector::disconnectCollisionGeometry(VxCollisionGeometry* cg)
{
    cg->mEntity = 0;
}

namespace {

// Vx::UniverseConnector::disconnectEntity(Vx::VxEntity*)::DisconnectEntityVisitor (a local
// class in the original: Mac vtable 0x75cf08, visitEntity 0x42301a, dtor 0x42307e / 0x423046;
// Win 0x750140)
class DisconnectEntityVisitor : public VxVisitor {
public:
    explicit DisconnectEntityVisitor(VxEntity* e) : mEntity(e) {}
    virtual ~DisconnectEntityVisitor() {}
    virtual int visitEntity(VxEntity* e)
    {
        if (e->mNodeParent == mEntity)
            e->setNodeParent(0);
        return 0;
    }
    VxEntity* mEntity;  // +0x04
};

} // namespace

// Mac 0x4230a8, Win 0x750350
void UniverseConnector::disconnectEntity(VxEntity* e)
{
    if (e->mUniverse != 0) {
        DisconnectEntityVisitor v(e);
        e->mUniverse->accept(&v);
    }
    e->mAssembly = 0;
    e->setUniverse(0);
}

// Mac 0x423138, Win 0x7502a0
void UniverseConnector::disconnectAssembly(VxAssemblyBase* a)
{
    a->setMergeRoot(0);
    VxUniverse* u = a->mUniverse;
    a->setUniverse(0);
    if (u != 0) {
        NotificationVisitor<VxUniverse> v(u, u, 1);
        a->accept(&v);
    }
}

// Mac 0x4231f0, Win 0x750430
void UniverseConnector::removeChildAssembly(VxAssemblyBase* child)
{
    VxAssemblyBase* parent = child->mParent;
    disconnectAssembly(child);
    child->setParent(0);
    if (parent != 0)
        parent->notifyCollection(&VxEntityConstraintCollection<VxAssemblyBase>::Subscriber::notifyAssembly, parent,
                                 VxEntityConstraintCollectionBase::kAssemblyRemoved, child);
}

// Mac 0x423424, Win 0x750220
void UniverseConnector::connectAssembly(VxUniverse* u, VxAssemblyBase* a)
{
    a->setUniverse(u);
    NotificationVisitor<VxUniverse> v(u, u, 0);
    a->accept(&v);
}

// Mac 0x4234be, Win 0x7503d0
void UniverseConnector::addChildAssembly(VxAssemblyBase* parent, VxAssemblyBase* child)
{
    child->setParent(parent);
    if (parent != 0)
        parent->notifyCollection(&VxEntityConstraintCollection<VxAssemblyBase>::Subscriber::notifyAssembly, parent,
                                 VxEntityConstraintCollectionBase::kAssemblyAdded, child);
    if (parent->mUniverse != 0)
        connectAssembly(parent->mUniverse, child);
}

} // namespace Vx

// the class instance of the TU (Mac 0x423700-0x423dd0, Win 0x7505e0-0x750630)
template class NotificationVisitor<Vx::VxUniverse>;
