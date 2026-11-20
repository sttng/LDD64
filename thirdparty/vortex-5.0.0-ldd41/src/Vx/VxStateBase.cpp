// Vortex 5.0.0 build 800 "vortexldd41" - VxStateBase (restored).
// Mac TU 519 0x47a73e-0x47b735 (static initialiser 0x47b6d0: <iostream> only), Win
// 0x7f39a0-0x7f4090. The TU also emits the weak VxBase::Subscriber destructors (Mac 0x47b590 /
// 0x47b566, Win 0x7feb60; inline in Vx/VxBase.h) and the std::for_each / std::transform /
// vector-insert instances over VxStateBase* (Mac 0x47b2a5, 0x47b2fe, 0x47b31e, 0x47b442,
// 0x47b5a9; Win 0x70f290, 0x7821d0, 0x7f3fc0), expanded below as plain loops.
#include "Vx/VxStateBase.h"
#include "Vx/VxTransform.h"
#include <iostream>

namespace Vx {

namespace {

// the subscriber collection of VxBase (removeSubscriber of every list, as inlined in
// setBaseObject: the all-event list, then both event lists)
void unsubscribeFromAll(VxBase* object, VxBase::Subscriber* s)
{
    VxSubscriberCollection<VxBase::Event, VxBase::Subscriber, 2>& c = object->mSubscribers;
    if (c.mSubscribers != 0)
        c.mSubscribers->remove(s);
    if (c.mEventSubscribers != 0)
        for (int e = 0; e < 2; ++e)
            c.mEventSubscribers[e].remove(s);
}

// delete every state of [b, e) through its virtual destructor (Mac 0x47a430, Win 0x7f3f20)
void deleteStates(VxStateBase** b, VxStateBase** e)
{
    for (; b != e; ++b)
        delete *b;
}

} // namespace

// Mac 0x47ab3c (C2; the C1 copy is inlined into the derived constructors)
VxStateBase::VxStateBase(VxBase* object) : mChildren(), mObject(0), mObjectName()
{
    VxStateBase::setBaseObject(object);
}

// Mac 0x47b14c: the children are cloned (std::transform with mem_fun(&VxStateBase::clone))
VxStateBase::VxStateBase(const VxStateBase& other)
    : VxBase::Subscriber(), mChildren(), mObject(0), mObjectName()
{
    VxStateBase::setBaseObject(other.mObject);
    mObjectName = other.mObjectName.c_str();
    for (VxStateBase* const* i = other.mChildren.begin(); i != other.mChildren.end(); ++i)
        mChildren.push_back((*i)->clone());
}

// Win 0x7f3bf0 (Win only)
VxStateBase& VxStateBase::operator=(const VxStateBase& other)
{
    if (this != &other) {
        setBaseObject(other.mObject);
        mObjectName = other.mObjectName.c_str();
        deleteStates(mChildren.begin(), mChildren.end());
        mChildren.clear();
        for (VxStateBase* const* i = other.mChildren.begin(); i != other.mChildren.end(); ++i)
            mChildren.push_back((*i)->clone());
    }
    return *this;
}

// Mac D2 0x47af06 (D1 0x47b02c, D0 0x47ac24), Win 0x7f3e50 (sdd 0x7f4060)
VxStateBase::~VxStateBase()
{
    VxStateBase::setBaseObject(0);
    deleteStates(mChildren.begin(), mChildren.end());
}

// Mac 0x47a73e, Win 0x7f39a0: the only event sent to a state is the object's destruction
void VxStateBase::notify(VxBase::Event, VxBase*)
{
    mObject = 0;
}

// Mac 0x47a74e, Win 0x7f39b0
VxStateBase* VxStateBase::findStateRecursive(VxBase* object)
{
    if (mObject == object)
        return this;
    for (VxStateBase** i = mChildren.begin(); i != mChildren.end(); ++i) {
        VxStateBase* s = (*i)->findStateRecursive(object);
        if (s != 0)
            return s;
    }
    return 0;
}

// Mac 0x47a7be, Win 0x7f3a60: for_each(children, mem_fun(&VxStateBase::reconnectBaseObject))
void VxStateBase::reconnectBaseObject()
{
    for (VxStateBase** i = mChildren.begin(); i != mChildren.end(); ++i)
        (*i)->reconnectBaseObject();
}

// Mac 0x47a7ee, Win 0x7f39f0
VxStateBase* VxStateBase::findChildState(VxBase* object)
{
    if (mObject == object)
        return this;
    for (VxStateBase** i = mChildren.begin(); i != mChildren.end(); ++i)
        if ((*i)->mObject == object)
            return *i;
    return 0;
}

// Mac 0x47a82a, Win 0x7f3a30
void VxStateBase::transform(const VxTransform& t)
{
    for (VxStateBase** i = mChildren.begin(); i != mChildren.end(); ++i)
        (*i)->transform(t);
}

// Mac 0x47a860, Win 0x7f3ac0
void VxStateBase::removeChildState(VxStateBase* s)
{
    mChildren.remove(s);
}

// Mac 0x47a8b8, Win 0x7f3ca0: unsubscribe from the old object, take the new one's name and
// subscribe to all of its events. NOTE: the subscription is a plain push_back (no uniqueness
// test), as in the original.
void VxStateBase::setBaseObject(VxBase* object)
{
    if (mObject == object)
        return;
    if (mObject != 0)
        unsubscribeFromAll(mObject, this);
    mObjectName.clear();
    mObject = object;
    if (object != 0) {
        mObjectName = object->getName();
        VxSubscriberCollection<VxBase::Event, VxBase::Subscriber, 2>& c = mObject->mSubscribers;
        if (c.mSubscribers == 0)
            c.mSubscribers = new VxSubscriberCollection<VxBase::Event, VxBase::Subscriber, 2>::List;
        c.mSubscribers->push_back(this);
    }
}

// Mac 0x47ad52, Win 0x7f3b00
void VxStateBase::removeDisconnectedStates()
{
    VxArray<VxStateBase*> stale;
    for (VxStateBase** i = mChildren.begin(); i != mChildren.end(); ++i) {
        if ((*i)->mObject == 0)
            stale.push_back(*i);
        else
            (*i)->removeDisconnectedStates();
    }
    for (VxStateBase** i = stale.begin(); i != stale.end(); ++i) {
        removeChildState(*i);
        delete *i;
    }
}

// Mac 0x47b040 (Win: inlined into the savers)
void VxStateBase::addChildState(VxStateBase* s)
{
    if (s == 0)
        return;
    for (VxStateBase** i = mChildren.begin(); i != mChildren.end(); ++i)
        if (*i == s)
            return;
    mChildren.push_back(s);
}

} // namespace Vx
