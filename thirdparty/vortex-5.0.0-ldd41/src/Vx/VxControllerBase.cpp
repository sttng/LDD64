// Vortex 5.0.0 build 800 "vortexldd41" - VxControllerBase, VxInputOutputContainer,
// VxNamePathControllerInterface (restored).
// Mac TU 446 (0x417d2c-0x41d6bf), Win 0x7f9890-0x7fb3e0 and 0x78bbb0-0x78cf10.
// The VxData::Field<bool> / Delegate<Field<bool> const&> / DelegateCB<..., mf2<...eStateChange...>>
// template instances that the Mac build emitted in this TU come from the VxData headers.
#include "Vx/VxControllerBase.h"
#include "Vx/VxCompositeController.h"
#include "Vx/VxControllerGraph.h"
#include "Vx/VxControllerState.h"
#include "Vx/VxNamePathControllerInterface.h"
// LDD64: Boost 1.92 in place of Vortex's private boostVx copy (Boost 1.42, namespace boostVx).
#include <boost/bind/bind.hpp>

namespace Vx {

// Mac 0x779184 (__data = 1)
bool VxControllerBase::sControllersEnabled = true;

// --------------------------------------------------------------------------------------
// VxInputOutputContainer

// Mac 0x41c976, Win 0x7fa960
VxInputOutputContainer::VxInputOutputContainer(VxControllerBase* owner)
    : VxData::Container(VxID("ControllerFields"), 0),
      mOwner(owner),
      mInputs(VxID("Inputs"), this),
      mOutputs(VxID("Outputs"), this),
      mParameters(VxID("Parameters"), this)
{
}

// Mac D1 0x41b57e / D0 0x419f70, Win 0x7f97b0
VxInputOutputContainer::~VxInputOutputContainer()
{
}

// Mac 0x417d63, Win 0x7f9990
VxData::Container* VxInputOutputContainer::getInputs(VxControllerBase* owner)
{
    return owner != 0 ? &owner->mFields.mInputs : 0;
}

// Mac 0x417d78, Win 0x7f99b0
VxData::Container* VxInputOutputContainer::getOutputs(VxControllerBase* owner)
{
    return owner != 0 ? &owner->mFields.mOutputs : 0;
}

// Mac 0x417d8d, Win 0x7f99d0
VxData::Container* VxInputOutputContainer::getParameters(VxControllerBase* owner)
{
    return owner != 0 ? &owner->mFields.mParameters : 0;
}

// --------------------------------------------------------------------------------------
// VxControllerBase

// Mac 0x417d2c (0x17f), 0x417d36 (isOfClassType), 0x418548 (types), 0x4188d0 (isOfType);
// Win 0x7f99f0, 0x7f9d70, 0x7f9df0
VX_IMPLEMENT_CLASS_TYPES(VxControllerBase, VxBaseUserData, 0x17f)

// Mac 0x418dc0, Win 0x7f9ee0
VxControllerBase::VxControllerBase()
    : VxBaseUserData(),
      mFields(this),
      mEnable(true, "Enable", this),
      mReset(false, "Reset", this),
      mDone(false, "Done", this),
      mResetOut(false, "Reset", this),
      mActive(true, "Active", this),
      mParent(0),
      mInputsChanged(true),
      mState(kStateActive),
      mSteppingEnabled(true),
      mListeners()
{
    // the returned handles are temporaries (Mac 0x41ce2e + 0x445424 on a local)
    mEnable.getDelegate().connect(boost::bind(&VxControllerBase::stateChanged, this, kEnableChanged, boost::placeholders::_1));
    mReset.getDelegate().connect(boost::bind(&VxControllerBase::stateChanged, this, kResetChanged, boost::placeholders::_1));
    VxControllerGraph::instance()->registerWithFrame();
    VxControllerGraph::instance()->invalidate();
}

// Mac D1 0x418dac (body 0x418b20) / D0 0x41948c, Win 0x7fb3e0 (body 0x7fa180).
// The listeners are owned; the fields and containers go with the members.
VxControllerBase::~VxControllerBase()
{
    while (mListeners.begin() != mListeners.end()) {
        VxControllerListener** i = mListeners.begin();
        delete *i;
        for (VxControllerListener** j = i + 1; j != mListeners.end(); ++j)
            *(j - 1) = *j;
        mListeners.pop_back();
    }
}

// Mac 0x419a68, Win 0x78ce70
VxControllerBase::FieldInfo VxControllerBase::getInput(const VxID& name)
{
    VxData::Container::Fields::iterator i = mFields.mInputs.findField(name);
    FieldInfo r;
    r.field = i == mFields.mInputs.mFields.end() ? &VxData::FieldBase::getInvalidField() : *i;
    r.kind = kFieldInput;
    return r;
}

// Mac 0x41a23a, Win 0x78cec0
VxControllerBase::FieldInfo VxControllerBase::getParameter(const VxID& name)
{
    VxData::Container::Fields::iterator i = mFields.mParameters.findField(name);
    FieldInfo r;
    r.field = i == mFields.mParameters.mFields.end() ? &VxData::FieldBase::getInvalidField() : *i;
    r.kind = kFieldParameter;
    return r;
}

// Mac 0x41a1f4, Win 0x78cf10
VxControllerBase::FieldInfo VxControllerBase::getOutput(const VxID& name)
{
    VxData::Container::Fields::iterator i = mFields.mOutputs.findField(name);
    FieldInfo r;
    r.field = i == mFields.mOutputs.mFields.end() ? &VxData::FieldBase::getInvalidField() : *i;
    r.kind = kFieldOutput;
    return r;
}

// Mac 0x4184ce (new 0x28), Win 0x7f9890 (new 0x40)
VxStateBase* VxControllerBase::createState()
{
    return new VxControllerState(this);
}

// Mac 0x419756 / 0x41975c / 0x419762 / 0x419768, Win 0x7076a0 (one shared empty body)
void VxControllerBase::onActivate()
{
}

void VxControllerBase::onDeactivate()
{
}

void VxControllerBase::onReset()
{
}

void VxControllerBase::onDone()
{
}

// Mac 0x4184a6, Win 0x7f9940
void VxControllerBase::setParent(VxBase* parent)
{
    mParent = parent;
    setSteppingEnabled(mSteppingEnabled);
}

// Mac 0x418442, Win 0x7f9900
void VxControllerBase::setSteppingEnabled(bool enabled)
{
    mSteppingEnabled = enabled;
    if (mParent != 0) {
        VxCompositeController* c = dynamic_cast<VxCompositeController*>(mParent);
        if (c != 0)
            c->updateSteppingSet(this);
    }
}

// for_each(mListeners, bind2nd(mem_fun(fn), this)) (Mac 0x419801, Win 0x7fa540 / 0x7fa670)
void VxControllerBase::notifyListeners(void (VxControllerListener::*fn)(VxControllerBase*))
{
    VxControllerListener** e = mListeners.end();
    for (VxControllerListener** i = mListeners.begin(); i != e; ++i)
        ((*i)->*fn)(this);
}

// Mac 0x417eda, Win 0x7f9a50
void VxControllerBase::stateChanged(eStateChange change, bool value)
{
    if (change == kEnableChanged) {
        if (!value) {
            if ((unsigned)(mState - 1) < 2) {
                mState = kStateInactive;
                mDone.setValue(false);
                mActive.setValue(false);
                onDeactivate();
                notifyListeners(&VxControllerListener::onDeactivate);
            }
        } else if (mState == kStateInactive) {
            mState = kStateActive;
            mDone.setValue(false);
            mActive.setValue(true);
            onActivate();
            notifyListeners(&VxControllerListener::onActivate);
        }
    } else if (change == kResetChanged) {
        mResetOut.setValue(mReset.mValue);
        if (value) {
            if ((unsigned)(mState - 1) < 2) {
                mState = kStateActive;
                mDone.setValue(false);
                mActive.setValue(true);
                onReset();
                notifyListeners(&VxControllerListener::onReset);
            } else if (mState == kStateInactive) {
                mState = kStateInactive;
                mDone.setValue(false);
                mActive.setValue(false);
                onReset();
                notifyListeners(&VxControllerListener::onReset);
            }
        }
    } else if (change == kDoneChanged && mState == kStateActive && value) {
        mState = kStateDone;
        mDone.setValue(true);
        mActive.setValue(false);
        onDone();
        notifyListeners(&VxControllerListener::onDone);
    }
}

// Mac 0x4182e4, Win 0x7f9d00
void VxControllerBase::reset()
{
    stateChanged(kResetChanged, true);
}

// Mac 0x418308, Win 0x7f9c80
void VxControllerBase::setDone()
{
    stateChanged(kDoneChanged, true);
}

// Mac 0x417e82 (with 0x417e14 / 0x417da6), Win 0x7f9e20
void VxControllerBase::execute()
{
    if (mState == kStateActive && sControllersEnabled) {
        notifyListeners(&VxControllerListener::preExecute);
        if (mInputsChanged) {
            mInputsChanged = false;
            update();
        }
        notifyListeners(&VxControllerListener::postExecute);
    }
}

// Mac 0x417d4c, Win 0x7f9980
void VxControllerBase::notifyInputs()
{
    mFields.mInputs.notify();
}

// Win 0x7f9ea0 (no Mac code left)
void VxControllerBase::addListener(VxControllerListener* l)
{
    for (VxControllerListener** i = mListeners.begin(); i != mListeners.end(); ++i)
        if (*i == l)
            return;
    mListeners.push_back(l);
}

// Win 0x7f9a10 (no Mac code left)
void VxControllerBase::removeListener(VxControllerListener* l)
{
    mListeners.remove(l);
}

// --------------------------------------------------------------------------------------
// VxNamePathControllerInterface

// Mac D1 0x419df6 / D0 0x419dbe, Win 0x7fa410
VxNamePathControllerInterface::~VxNamePathControllerInterface()
{
}

// Mac 0x419d6e, Win 0x7fa3d0
VxBase* VxNamePathControllerInterface::getParent(VxBase* object) const
{
    if (object != 0) {
        VxControllerBase* c = dynamic_cast<VxControllerBase*>(object);
        if (c != 0)
            return c->mParent;
    }
    return 0;
}

// Mac 0x41a282, Win 0x7fa8c0
void VxNamePathControllerInterface::getChildren(VxBase* object, VxArray<VxBase*>& children) const
{
    if (object == 0)
        return;
    VxCompositeController* c = dynamic_cast<VxCompositeController*>(object);
    if (c == 0)
        return;
    VxControllerBase** e = c->mControllers.end();
    for (VxControllerBase** i = c->mControllers.begin(); i != e; ++i)
        children.push_back(*i);
}

// The static initialiser of the Mac TU (0x41d49a): registers the interface for
// VxControllerBase (getInterface Mac 0x419aab with its static Mac 0x779180, type test
// Mac 0x419d22) and unregisters it at exit (Mac 0x418916).
static VxNamePathInterfaceRegistration<VxControllerBase, VxNamePathControllerInterface> sNamePathRegistration;

} // namespace Vx
