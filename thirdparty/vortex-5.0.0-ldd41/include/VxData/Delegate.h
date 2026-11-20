// Vortex 5.0.0 build 800 "vortexldd41" - VxData change delegates (restored header).
//
// Delegate<Arg> is a multicast callback list: connect(f) wraps any functor (in Vortex always a
// boostVx::bind expression) in implementation::DelegateCB<Arg, F>, stores it in the enabled list
// and returns a DelegateConnection handle. A handle can disable / enable its callback (moving it
// between the two lists) and unregisters itself when destroyed; when the delegate dies first it
// invalidates every registered handle. Callbacks are owned (deleted) by the delegate.
//
// RTTI names: VxData::DelegateBase, VxData::Delegate<Field<T> const&>,
// VxData::implementation::NoTypeDelegateCB, ::BaseDelegateCB<Arg>, ::DelegateCB<Arg, F>.
// The handle class has no RTTI; its name is inferred.
//
// Layouts (Mac = Win): DelegateBase 0x14 (vptr, VxArray of handles), Delegate<Arg> 0x34,
// DelegateCB<Arg, bind_t<mf0, list1>> Mac 0x10 / Win 0xc (vptr + bound functor).
#ifndef VXDATA_DELEGATE_H
#define VXDATA_DELEGATE_H

#include "Vx/VxArray.h"

namespace VxData {

class DelegateBase;

namespace implementation {

// The untyped callback base (Win vtable 0x9fe288: one slot, dtor 0x787060; no Mac vtable).
class NoTypeDelegateCB {
public:
    virtual ~NoTypeDelegateCB() {}
};

// Mac 4 slots (D1, D0, clone, invoke), Win 3. The dtors (Mac e.g. Field<int>: D1 0x417ad0 /
// D0 0x417a0c) only reset the vptr.
template <class Arg>
class BaseDelegateCB : public NoTypeDelegateCB {
public:
    virtual ~BaseDelegateCB() {}
    virtual BaseDelegateCB* clone() const = 0;   // Mac slot 2, Win 1
    virtual void invoke(Arg arg) = 0;            // Mac slot 3, Win 2 (name inferred)
};

// Mac e.g. ConnectionFunctor<int,int>: clone 0x40be64 (new 0x10), invoke 0x40b872, D1 0x40efaa,
// D0 0x40edce; Win clone 0x786ca0, invoke 0x7878f0.
template <class Arg, class F>
class DelegateCB : public BaseDelegateCB<Arg> {
public:
    explicit DelegateCB(const F& f) : mFunctor(f) {}
    virtual ~DelegateCB() {}
    virtual BaseDelegateCB<Arg>* clone() const { return new DelegateCB(*this); }
    virtual void invoke(Arg arg) { mFunctor(arg); }

    F mFunctor; // +0x04
};

} // namespace implementation

// The handle returned by Delegate::connect (Mac 8 bytes; name inferred).
class DelegateConnection {
public:
    // Mac C2 0x445386 / C1 0x44539c, Win 0x7f7380
    DelegateConnection();
    // Mac C2 0x445486 / C1 0x4454ac: registers the handle with the delegate (addConnection)
    DelegateConnection(implementation::NoTypeDelegateCB* callback, DelegateBase* delegate);
    // Mac C2 0x445406 / C1 0x445424: unregisters (removeConnection)
    ~DelegateConnection();
    // Mac 0x445438: re-registers with the other handle's delegate (addConnectionUnique)
    DelegateConnection& operator=(const DelegateConnection& o);

    // Mac 0x4453b0: delegate->disable(callback) (name inferred)
    void disable();
    // Mac 0x4453d0: delegate->enable(callback) (name inferred)
    void enable();
    // Mac 0x4453f0 (called through a member pointer by DelegateBase::disconnectAll): forget both
    // pointers without touching the delegate (name inferred)
    void invalidate();

    bool isConnected() const { return mDelegate != 0; } // (name inferred)

    // Not in either binary (connect's result is always constructed in place); defined so that a
    // copy is registered like the original (C++03 needs it accessible for the by-value return).
    DelegateConnection(const DelegateConnection& o);

    implementation::NoTypeDelegateCB* mCallback; // +0x00
    DelegateBase* mDelegate;                     // +0x04
};

// Mac vtable 0x75dae8 (D1 0x444f40, D0 0x444e14, 2 pure), Win 0xa08fec (0x81b210 + 2 pure).
class DelegateBase {
public:
    DelegateBase();             // Mac 0x444d30 (inlined on Win)
    // Mac D1 0x444f40 (body 0x444eae) / D0 0x444e14, Win 0x81b210: disconnectAll
    virtual ~DelegateBase();
    // move the callback from the enabled to the disabled list            Mac 2, Win 1
    virtual void disable(implementation::NoTypeDelegateCB* callback) = 0;
    // and back                                                           Mac 3, Win 2
    virtual void enable(implementation::NoTypeDelegateCB* callback) = 0;

    // Mac 0x444f54 (push_back) (name inferred)
    void addConnection(DelegateConnection* c);
    // Mac 0x445032 (push_back unless present) (name inferred)
    void addConnectionUnique(DelegateConnection* c);
    // Mac 0x444dbc (erase the first match) (name inferred)
    void removeConnection(DelegateConnection* c);
    // Mac 0x444d5e, Win 0x7f7440: invalidate() every handle, forget them (name inferred)
    void disconnectAll();

    Vx::VxArray<DelegateConnection*> mConnections; // +0x04

private:
    DelegateBase(const DelegateBase&);
    DelegateBase& operator=(const DelegateBase&);
};

// Mac 4 slots (Field<bool>: vtable 0x75dbe8, D1 0x41a678 / body 0x41a52e, D0 0x41a8b8, disable
// 0x41b41e, enable 0x41a3ce), Win 3 (Field<bool>: 0xa01874 = 0x7c2b40, 0x7c3920, 0x7c3980).
template <class Arg>
class Delegate : public DelegateBase {
public:
    typedef implementation::BaseDelegateCB<Arg> Callback;
    typedef Vx::VxArray<Callback*> Callbacks;

    Delegate() {}
    virtual ~Delegate();
    virtual void disable(implementation::NoTypeDelegateCB* callback);
    virtual void enable(implementation::NoTypeDelegateCB* callback);

    // e.g. Mac 0x410d92 (Field<VxCable*>, ConnectionFunctor) / 0x41ce2e (VxControllerBase):
    // new DelegateCB, append to the enabled list, return a handle for it
    template <class F>
    DelegateConnection connect(const F& f)
    {
        Callback* cb = new implementation::DelegateCB<Arg, F>(f);
        mCallbacks.push_back(cb);
        return DelegateConnection(cb, this);
    }

    // calls every enabled callback in order (inlined in Field<T>::notify and setValue)
    void operator()(Arg arg) const
    {
        for (typename Callbacks::const_iterator i = mCallbacks.begin(); i != mCallbacks.end(); ++i)
            (*i)->invoke(arg);
    }

    Callbacks mCallbacks; // +0x14
    Callbacks mDisabled;  // +0x24

private:
    static typename Callbacks::iterator find(Callbacks& a, Callback* cb)
    {
        typename Callbacks::iterator i = a.begin();
        for (; i != a.end(); ++i)
            if (*i == cb)
                break;
        return i;
    }
    static void erase(Callbacks& a, typename Callbacks::iterator i)
    {
        for (typename Callbacks::iterator j = i + 1; j != a.end(); ++j)
            *(j - 1) = *j;
        a.pop_back();
    }
};

// Field<bool>: Mac 0x41a52e. disconnectAll first, then delete every callback of both lists.
template <class Arg>
Delegate<Arg>::~Delegate()
{
    disconnectAll();
    for (typename Callbacks::iterator i = mCallbacks.begin(); i != mCallbacks.end(); ++i)
        delete *i;
    for (typename Callbacks::iterator i = mDisabled.begin(); i != mDisabled.end(); ++i)
        delete *i;
    mCallbacks.clear();
    mDisabled.clear();
}

// Field<bool>: Mac 0x41b41e, Win 0x7c3920
template <class Arg>
void Delegate<Arg>::disable(implementation::NoTypeDelegateCB* callback)
{
    Callback* cb = callback ? dynamic_cast<Callback*>(callback) : 0;
    if (cb == 0)
        return;
    typename Callbacks::iterator i = find(mCallbacks, cb);
    if (i != mCallbacks.end()) {
        erase(mCallbacks, i);
        mDisabled.push_back(cb);
    }
}

// Field<bool>: Mac 0x41a3ce, Win 0x7c3980
template <class Arg>
void Delegate<Arg>::enable(implementation::NoTypeDelegateCB* callback)
{
    Callback* cb = callback ? dynamic_cast<Callback*>(callback) : 0;
    if (cb == 0)
        return;
    typename Callbacks::iterator i = find(mDisabled, cb);
    if (i != mDisabled.end()) {
        erase(mDisabled, i);
        mCallbacks.push_back(cb);
    }
}

} // namespace VxData

#endif
