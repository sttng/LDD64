// Vortex 5.0.0 build 800 "vortexldd41" - VxControllerState (restored).
// Mac TU 448 (0x42051f-0x420e6d), Win 0x824110-0x8247d0 and 0x7f3d10.
#include "Vx/VxControllerState.h"
#include "Vx/VxCompositeController.h"
#include "Vx/VxControllerGraph.h"
#include "Vx/VxMessage.h"

namespace Vx {

// Mac C2 0x420d7c / C1 0x420dee, Win 0x7f3d10 / 0x8246e0
VxControllerState::VxControllerState(VxControllerBase* controller)
    : VxStateBase(controller), mInputs(0), mOutputs(0), mParameters(0)
{
    save(controller);
}

// Mac C2 0x4206fa / C1 0x4207ae, Win 0x824110
VxControllerState::VxControllerState(const VxControllerState& other)
    : VxStateBase(other), mInputs(0), mOutputs(0), mParameters(0)
{
    if (other.mInputs != 0)
        mInputs = other.mInputs->clone(0);
    if (other.mOutputs != 0)
        mOutputs = other.mOutputs->clone(0);
    if (other.mParameters != 0)
        mParameters = other.mParameters->clone(0);
}

// Mac D1 0x4206e6 (body 0x42067a) / D0 0x420606, Win 0x8247d0
VxControllerState::~VxControllerState()
{
    delete mInputs;
    delete mOutputs;
    delete mParameters;
}

// Mac 0x4207c8 (new 0x28), Win 0x824340 (new 0x40)
VxStateBase* VxControllerState::clone() const
{
    return new VxControllerState(*this);
}

// Mac 0x420d5e, Win 0x824750
void VxControllerState::save()
{
    if (mObject != 0)
        save(static_cast<VxControllerBase*>(mObject));
}

// Mac 0x420a86, Win 0x8244f0
void VxControllerState::save(VxControllerBase* controller)
{
    if (controller == 0)
        return;
    VxArray<VxStateBase*> stale(mChildren);
    delete mInputs;
    delete mOutputs;
    delete mParameters;
    mInputs = controller->mFields.mInputs.clone(0);
    mOutputs = controller->mFields.mOutputs.clone(0);
    mParameters = controller->mFields.mParameters.clone(0);

    VxCompositeController* composite = dynamic_cast<VxCompositeController*>(controller);
    if (composite != 0) {
        for (int i = 0; i < composite->getControllerCount(); ++i) {
            VxControllerBase* child = composite->getController(i);
            VxStateBase* s = findChildState(child);
            if (s == 0) {
                s = child->createState();
                addChildState(s);
            } else {
                s->save();
                // std::remove + erase: drop every occurrence from the stale list
                VxStateBase** w = stale.begin();
                for (VxStateBase** r = stale.begin(); r != stale.end(); ++r)
                    if (*r != s)
                        *w++ = *r;
                while (stale.end() != w)
                    stale.pop_back();
            }
        }
    }
    // the states of children that are gone
    for (VxStateBase** i = stale.begin(); i != stale.end(); ++i)
        removeChildState(*i);
    for (VxStateBase** i = stale.begin(); i != stale.end(); ++i)
        if (*i != 0)
            delete *i;
    setBaseObject(controller);
}

// Mac 0x420540, Win 0x8243b0. NOTE: the original unconditionally creates the graph singleton
// to invalidate it.
void VxControllerState::restoreTo(VxControllerBase* controller)
{
    if (controller == 0)
        return;
    controller->reset();
    controller->mFields.mInputs.copyValueOnly(*mInputs);
    controller->mFields.mOutputs.copyValueOnly(*mOutputs);
    controller->mFields.mParameters.copyValueOnly(*mParameters);
    VxControllerGraph::instance()->invalidate();
}

// Mac 0x420810, Win 0x824430
void VxControllerState::restore(bool notify)
{
    if (mObject == 0)
        VxInfo(0, "VxControllerState: cannot find object '%s' to restore state to.\n", mObjectName.c_str());
    else
        restoreTo(static_cast<VxControllerBase*>(mObject));
    VxStateBase** e = mChildren.end();
    for (VxStateBase** i = mChildren.begin(); i != e; ++i)
        (*i)->restore(notify);
}

// Mac 0x4209f6, Win 0x824490
void VxControllerState::restoreKinematic(bool notify)
{
    if (mObject == 0)
        VxInfo(0, "VxControllerState: cannot find object '%s' to restore kinematic state to.\n", mObjectName.c_str());
    VxStateBase** e = mChildren.end();
    for (VxStateBase** i = mChildren.begin(); i != e; ++i)
        (*i)->restoreKinematic(notify);
}

// Mac 0x4208b8, Win 0x824760
void VxControllerState::restoreMode(int mode, bool notify)
{
    if (mObject == 0)
        return;
    if (mode == 0) {
        restoreTo(static_cast<VxControllerBase*>(mObject));
        VxStateBase** e = mChildren.end();
        for (VxStateBase** i = mChildren.begin(); i != e; ++i)
            (*i)->restoreMode(0, notify);
    } else if (mode == 1) {
        VxStateBase** e = mChildren.end();
        for (VxStateBase** i = mChildren.begin(); i != e; ++i)
            (*i)->restoreMode(1, notify);
    }
}

// Mac 0x4205c4, Win 0x824320
void VxControllerState::reconnectBaseObject()
{
    if (mObject == 0)
        VxInfo(0, "VxControllerState::reconnectBaseObject() do not know what to do!.\n");
    VxStateBase::reconnectBaseObject();
}

} // namespace Vx
