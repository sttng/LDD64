// LEGO Digital Designer — restored source
// LEGO::Observer<N, O>, NotifierT<N, O>, Notifier<O> — subject/observer templates, header-only.
// ld64 merged identical instances (e.g. RemoveObserver Mac 0x58ca0, Observer OnFinalRelease
// 0x45080). Mac, Preferences instance unless noted; names other than RTTI ones inferred:
//  * Observer : virtual Object, vtable {D1 0x521a0, D0 0x521d0, OnFinalRelease 0x45080,
//    OnAttached 0x52080, OnDetached 0x52090}, Object part {0x52220, 0x52260, 0x52150, ..};
//    N* subject +0x04. Attach / Detach inline (Mac 0x38b3b0 / 0x38b690).
//  * NotifierT : virtual Object, vtable {D1 0x11ca70, D0 0x11cae0, OnFinalRelease 0x11c8a0};
//    observers +0x04, iterators +0x14, Object +0x24. Notifier<O>: Mac {0x11c7b0, 0x11c820, 0x11c8a0}.
//  * Iterator: stack object registered while observers are called (Preferences::SetValue Mac
//    0x11bd50) so RemoveObserver can fix up running passes. Win: same, Preferences Object +0x12c.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"

namespace LEGO {

template <class N, class O> class NotifierT;
template <class N, class O> class ObserverN;   // ObserverN.h (several subjects)

template <class N, class O>
class Observer : public virtual Object {
public:
    typedef N Subject;

    Observer() : m_subject(0) {}
    virtual ~Observer() {}                                           // Mac 0x521a0/0x521d0

    N* GetSubject() const { return m_subject; }

    // Registers with `subject` (detaching from the previous one first). (name inferred)
    // NOTE: `subject` must not be null (the inlined original pushes without a check).
    void Attach(N* subject)                                          // inlined, Mac 0x38b3b0
    {
        if (m_subject == subject)
            return;
        Detach();
        m_subject = subject;
        // Through NotifierT<N, O>: N may derive from several NotifierT (Window). (additive fix)
        static_cast<NotifierT<N, O>*>(subject)->m_observers.PushBack(static_cast<O*>(this));
        OnAttached(m_subject);
    }

    // (name inferred)
    void Detach()                                                    // inlined, Mac 0x38b690
    {
        N* subject = m_subject;
        if (subject) {
            static_cast<NotifierT<N, O>*>(subject)->RemoveObserver(static_cast<O*>(this));
            m_subject = 0;
            OnDetached(subject);
        }
    }

protected:
    // Leaves the subject before the last reference goes away.
    virtual void OnFinalRelease()                                    // Mac 0x45080 (thunk 0x52150)
    {
        Detach();
        Object::OnFinalRelease();
    }
    virtual void OnAttached(N* /*subject*/) {}                       // Mac 0x52080 (name inferred)
    virtual void OnDetached(N* /*subject*/) {}                       // Mac 0x52090 (name inferred)

private:
    friend class NotifierT<N, O>;

    N* m_subject;   // Mac +0x04
};

template <class N, class O>
class NotifierT : public virtual Object {
public:
    // Guards one pass over the observers.
    struct Iterator {
        explicit Iterator(NotifierT& notifier)
            : m_list(&notifier.m_iterators), m_index(0), m_count(notifier.m_observers.GetSize())
        {
            m_list->PushBack(this);
        }
        ~Iterator() { m_list->PopBack(); }

        // Next observer or null at the end. (name inferred)
        O* Next(const BasicArray<O*>& observers)
        {
            return m_index < m_count ? observers[m_index++] : 0;
        }

        BasicArray<Iterator*>* m_list;   // +0x00
        UInt32                 m_index;  // +0x04 next observer
        UInt32                 m_count;  // +0x08 observers to visit
    };

    NotifierT() {}
    virtual ~NotifierT() {}                                          // Mac 0x11ca70/0x11cae0

    UInt32 GetObserverCount() const { return m_observers.GetSize(); }

    // Removes the first occurrence of `observer`. Without a running iteration the last
    // observer takes its place (order not kept); during an iteration the rest moves up and
    // the running iterations are adjusted. (name inferred)
    // NOTE: only the first: the helper (Mac 0x58e90 = 0x1f6e0, Win 0x45b120) is std::find +
    // `*it = back; erase(end - 1)`, called once (Win 0x501150).
    void RemoveObserver(O* observer)                                 // Mac 0x58ca0 (+ 0x58e90)
    {
        if (m_iterators.IsEmpty()) {
            typename BasicArray<O*>::Iterator it = m_observers.Begin();
            while (it != m_observers.End() && *it != observer)
                ++it;
            if (it != m_observers.End()) {
                typename BasicArray<O*>::Iterator last = m_observers.End() - 1;
                if (last != it)
                    *it = *last;
                m_observers.PopBack();
            }
            return;
        }
        for (UInt32 i = 0; i < m_observers.GetSize(); ++i) {
            if (m_observers[i] != observer)
                continue;
            m_observers.Erase(i);
            for (typename BasicArray<Iterator*>::Iterator it = m_iterators.Begin(); it != m_iterators.End(); ++it) {
                if ((*it)->m_index != 0 && i < (*it)->m_index)
                    --(*it)->m_index;
                if ((*it)->m_count != 0 && i < (*it)->m_count)
                    --(*it)->m_count;
            }
            return;
        }
    }

protected:
    // Detaches every observer (last first) before the last reference goes away.
    virtual void OnFinalRelease()                                    // Mac 0x11c8a0 (thunk 0x11ca10)
    {
        while (!m_observers.IsEmpty())
            DetachObserver(m_observers.Back(), static_cast<N*>(this));
        Object::OnFinalRelease();
    }

    // One observer leaves (OnFinalRelease). Observer<N, O>: removed here, subject cleared,
    // OnDetached — the body of Mac 0x11c8a0. ObserverN<N, O> (several subjects): its Detach
    // (removes the subject from its array, the observer from this notifier, OnDetached), as in
    // SimpleSoundChangedNotifier Mac 0x2a2f90. (additive helper: lets NotifierT
    // compile for ObserverN observers)
    static void DetachObserver(Observer<N, O>* observer, N* subject)
    {
        static_cast<NotifierT*>(subject)->RemoveObserver(static_cast<O*>(observer));
        observer->m_subject = 0;
        observer->OnDetached(subject);
    }
    static void DetachObserver(ObserverN<N, O>* observer, N* subject)
    {
        observer->Detach(subject);
    }

    friend class Observer<N, O>;
    friend class ObserverN<N, O>;                                    // (additive)

    BasicArray<O*>         m_observers;   // Mac +0x04
    BasicArray<Iterator*>  m_iterators;   // Mac +0x14
};

template <class O>
class Notifier : public NotifierT<typename O::Subject, O> {
public:
    Notifier() {}
    virtual ~Notifier() {}                                           // Mac 0x11c7b0/0x11c820
};

} // namespace LEGO
