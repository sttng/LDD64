// Vortex 5.0.0 build 800 "vortexldd41" - VxState: the saved state of an assembly (restored).
// Mac TU 518 0x4795b6-0x47a73e (static initialiser 0x47a6d9: <iostream>
// only), Win 0x78ab30-0x78b3a0, 0x7f3d80, 0x7f3f20, 0x7f4010. The algorithm instances of the TU
// (Mac 0x47a3dc-0x47a5f9: binders, std::find, std::remove, std::for_each over VxStateBase*;
// Win 0x786930) are expanded as loops.
#include "Vx/VxState.h"
#include "Vx/VxStateEntity.h"
#include "Vx/VxStateConstraint.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxEntity.h"
#include "Vx/VxPart.h"
#include "Vx/VxConstraint.h"
#include "Vx/VxFrame.h"
#include "Vx/VxTransform.h"
#include "Vx/VxMessage.h"
#include <iostream>

namespace Vx {

namespace {

// Mac 0x4795d7 (register argument): the first VxStateEntity of the tree, depth first.
// NOTE: the original dereferences a null state (never called with one).
VxStateEntity* firstEntityState(VxStateBase* s)
{
    if (s != 0) {
        VxStateEntity* e = dynamic_cast<VxStateEntity*>(s);
        if (e != 0)
            return e;
    }
    for (VxStateBase** i = s->mChildren.begin(); i != s->mChildren.end(); ++i) {
        VxStateEntity* e = firstEntityState(*i);
        if (e != 0)
            return e;
    }
    return 0;
}

// Mac 0x479640 (register arguments): the entity state of `entity` in the tree, or the first one
VxStateEntity* findEntityState(VxStateBase* state, VxBase* entity)
{
    if (entity != 0) {
        VxStateBase* s = state->findStateRecursive(entity);
        if (s != 0) {
            VxStateEntity* e = dynamic_cast<VxStateEntity*>(s);
            if (e != 0)
                return e;
        }
    }
    return firstEntityState(state);
}

// Mac 0x4796a8, Win 0x78ab30: puts the assembly back where it was saved. A mechanism is moved
// so that its reference part gets the saved transform; another assembly is moved by the
// relative transform that takes its first saved part from where it is to where it was.
void restorePlacement(VxState* state, VxAssemblyBase* assembly)
{
    VxMechanism* m = assembly != 0 ? dynamic_cast<VxMechanism*>(assembly) : 0;
    if (m != 0) {
        VxStateEntity* s = findEntityState(state, m->getReferencePart());
        if (s != 0)
            m->setTransform(s->mTransform);
        return;
    }
    VxStateEntity* s = findEntityState(state, 0);
    if (s == 0 || s->mObject == 0)
        return;
    VxEntity* e = dynamic_cast<VxEntity*>(s->mObject);
    if (e == 0)
        return;
    // rel = T^-1 * S (row vectors): T * rel = S
    const VxTransform& t = *e->mTransformPtr;
    const VxTransform& sv = s->mTransform;
    VxTransform rel;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j)
            rel.m[i][j] = t.m[0][i] * sv.m[0][j] + t.m[1][i] * sv.m[1][j] + t.m[2][i] * sv.m[2][j];
        rel.m[i][3] = 0;
    }
    for (int j = 0; j < 3; ++j)
        rel.m[3][j] = ((sv.m[3][j] - t.m[3][0] * rel.m[0][j]) - t.m[3][1] * rel.m[1][j]) - t.m[3][2] * rel.m[2][j];
    rel.m[3][3] = 1.0;
    rel.orthonormalize(0);
    if (!rel.isIdentity(1e-7))
        assembly->transform(rel);
}

// Mac 0x479a0c, Win 0x78aca0: restore mode 2 (placement only)
void restorePlacementOnly(VxState* state, VxAssemblyBase* assembly, bool notify)
{
    if (assembly != 0 && assembly->getM3d4() != 3) {
        restorePlacement(state, assembly);
        if (notify)
            VxFrame::instance()->updateSceneGraph();
    }
}

// Mac 0x479a5e, Win 0x78ad70
void restoreKinematicImpl(VxState* state, VxAssemblyBase* assembly, bool notify)
{
    if (assembly != 0 && assembly->getM3d4() != 3) {
        state->removeDisconnectedStates();
        for (VxStateBase** i = state->mChildren.begin(); i != state->mChildren.end(); ++i)
            (*i)->restoreKinematic(false);
        if (notify)
            VxFrame::instance()->updateSceneGraph();
    }
}

// Mac 0x479aea, Win 0x78ace0
void restoreImpl(VxState* state, VxAssemblyBase* assembly, bool notify)
{
    if (assembly != 0 && assembly->getM3d4() != 3) {
        if (dynamic_cast<VxMechanism*>(assembly) != 0)
            restorePlacement(state, assembly);
        state->removeDisconnectedStates();
        for (VxStateBase** i = state->mChildren.begin(); i != state->mChildren.end(); ++i)
            (*i)->restore(false);
        if (notify)
            VxFrame::instance()->updateSceneGraph();
    }
}

// Mac 0x479bc4, Win 0x78b200: the assembly's own mode wins when it is larger. NOTE: mode 2
// updates the scene graph twice when notify is set (the original's).
void restoreModeImpl(VxState* state, VxAssemblyBase* assembly, int mode, bool notify)
{
    if (assembly == 0)
        return;
    int m = assembly->getM3d4();
    if (m < mode)
        m = mode;
    state->removeDisconnectedStates();
    if (m == 1) {
        for (VxStateBase** i = state->mChildren.begin(); i != state->mChildren.end(); ++i)
            (*i)->restoreMode(m, false);
    } else if (m == 2) {
        restorePlacementOnly(state, assembly, notify);
    } else if (m == 0) {
        if (dynamic_cast<VxMechanism*>(assembly) != 0)
            restorePlacement(state, assembly);
        for (VxStateBase** i = state->mChildren.begin(); i != state->mChildren.end(); ++i)
            (*i)->restoreMode(m, false);
    }
    if (notify)
        VxFrame::instance()->updateSceneGraph();
}

// a child state: kept (and saved again) when it exists, created otherwise; the kept ones are
// taken out of `stale`
template <class Object, class Create>
void updateChildState(VxState* state, Object* object, VxArray<VxStateBase*>& stale, Create create)
{
    VxStateBase* s = state->findChildState(object);
    if (s == 0) {
        s = create(object);
        state->addChildState(s);
    } else {
        s->save();
        while (stale.remove(s)) {
        }
    }
}

VxStateBase* createEntityState(VxPart* p) { return new VxStateEntity(p); }
VxStateBase* createConstraintState(VxConstraint* c) { return new VxStateConstraint(c); }
VxStateBase* createAssemblyState(VxAssemblyBase* a) { return a->createState(); }
VxStateBase* createControllerState(VxCompositeController* c) { return c->createState(); }

} // namespace

// Mac 0x479f6c, Win 0x78add0
void VxState::save(VxAssemblyBase* a)
{
    VxArray<VxStateBase*> stale;
    stale.reserve(mChildren.size());
    for (VxStateBase** i = mChildren.begin(); i != mChildren.end(); ++i)
        stale.push_back(*i);
    for (VxPart** i = a->mParts.begin(); i != a->mParts.end(); ++i)
        updateChildState(this, *i, stale, createEntityState);
    for (VxConstraint** i = a->mConstraints.begin(); i != a->mConstraints.end(); ++i)
        updateChildState(this, *i, stale, createConstraintState);
    for (VxAssemblyBase** i = a->m44.begin(); i != a->m44.end(); ++i)
        updateChildState(this, *i, stale, createAssemblyState);
    updateChildState(this, &a->mController, stale, createControllerState);
    for (VxStateBase** i = stale.begin(); i != stale.end(); ++i)
        removeChildState(*i);
    for (VxStateBase** i = stale.begin(); i != stale.end(); ++i)
        delete *i;
    setBaseObject(a);
}

// C2 Mac 0x47a360 (C1 0x47a3c2), Win 0x78b180
VxState::VxState(VxAssemblyBase* a) : VxStateBase(a)
{
    if (a != 0)
        save(a);
}

// C2 Mac 0x479d8e (C1 0x479dc6), Win 0x7f4010 / 0x7f3d80
VxState::VxState(const VxState& other) : VxStateBase(other)
{
}

// Mac D2 0x479e18 (D1 0x479e42, D0 0x479de0), Win 0x78b370
VxState::~VxState()
{
}

// Mac 0x479e56, Win 0x78b150
void VxState::restoreKinematic(bool notify)
{
    VxAssemblyBase* a = static_cast<VxAssemblyBase*>(mObject);
    if (a != 0) {
        restoreKinematicImpl(this, a, notify);
        a->updateCollisionGeometryTransforms();
    }
}

// Mac 0x479e8c, Win 0x78b2d0
void VxState::restoreMode(int mode, bool notify)
{
    VxAssemblyBase* a = static_cast<VxAssemblyBase*>(mObject);
    if (a != 0) {
        restoreModeImpl(this, a, mode, notify);
        a->updateCollisionGeometryTransforms();
    }
}

// Mac 0x479eca, Win 0x78b120
void VxState::restore(bool notify)
{
    VxAssemblyBase* a = static_cast<VxAssemblyBase*>(mObject);
    if (a != 0) {
        restoreImpl(this, a, notify);
        a->updateCollisionGeometryTransforms();
    }
}

// Mac 0x479f00, Win 0x78ac40
void VxState::reconnectBaseObject()
{
    if (mObject == 0) {
        VxAssemblyBase* a = VxAssemblyBase::findFromName(mObjectName.c_str());
        if (a == 0)
            VxWarning(0, "VxState::reconnectBaseObject : cannot find assembly named '%s'\n", mObjectName.c_str());
        setBaseObject(a);
    }
    VxStateBase::reconnectBaseObject();
}

// Mac 0x47a342, Win 0x78b1f0
void VxState::save()
{
    if (mObject != 0)
        save(static_cast<VxAssemblyBase*>(mObject));
}

// Mac 0x47a692, Win 0x78b310
VxStateBase* VxState::clone() const
{
    return new VxState(*this);
}

} // namespace Vx
