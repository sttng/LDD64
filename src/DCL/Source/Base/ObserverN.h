// LEGO Digital Designer — restored source (header, template)
// LEGO::ObserverN<N, O> — observer of several subjects at once (ControlObserver, Slider/Dial/
// DropDownControlObserver, NewContentCheckerObserver, ...; SimpleSoundChangedNotifier.h models
// its own). Mac <Control, ControlObserver>: vtable 0x70595c {D1 0x6e6d0, D0 0x6e720,
// OnFinalRelease 0x6e370, OnAttached 0x6e3c0, OnDetached 0x6e3d0}, Object part 0x705988 {0x6e790,
// 0x6e7f0, 0x6e680, ..}; subjects +0x04, Object +0x14. Win: vfptr, vbptr +4, subjects +8.
// NotifierT::OnFinalRelease detaches these via Detach (Mac 0x2a2f90). Names and Attach/Detach
// bodies inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Notifier.h"

namespace LEGO {

template <class N, class O>
class ObserverN : public virtual Object {
public:
    typedef N Subject;

    ObserverN() {}
    virtual ~ObserverN() {}                                          // Mac 0x6e6d0/0x6e720

    UInt32 GetSubjectCount() const { return m_subjects.GetSize(); }
    N*     GetSubject(UInt32 index) const { return m_subjects[index]; }

    // Registers with `subject` (once). (name inferred)
    void Attach(N* subject)
    {
        for (UInt32 i = 0; i < m_subjects.GetSize(); ++i)
            if (m_subjects[i] == subject)
                return;
        m_subjects.PushBack(subject);
        static_cast<NotifierT<N, O>*>(subject)->m_observers.PushBack(static_cast<O*>(this));
        OnAttached(subject);
    }

    // Leaves `subject` (no-op when not attached). (name inferred; Mac 0x1cee00 is the
    // SimpleSoundChangedObserver instance)
    void Detach(N* subject)
    {
        for (UInt32 i = 0; i < m_subjects.GetSize(); ++i) {
            if (m_subjects[i] != subject)
                continue;
            m_subjects.Erase(i);
            static_cast<NotifierT<N, O>*>(subject)->RemoveObserver(static_cast<O*>(this));
            OnDetached(subject);
            return;
        }
    }

    // Leaves every subject (last first). (name inferred)
    void DetachAll()
    {
        while (!m_subjects.IsEmpty())
            Detach(m_subjects.Back());
    }

protected:
    // Leaves every subject before the last reference goes away.
    virtual void OnFinalRelease()                                    // Mac 0x6e370 (thunk 0x6e680)
    {
        DetachAll();
        Object::OnFinalRelease();
    }
    virtual void OnAttached(N* /*subject*/) {}                       // Mac 0x6e3c0 (name inferred)
    virtual void OnDetached(N* /*subject*/) {}                       // Mac 0x6e3d0 (name inferred)

    BasicArray<N*> m_subjects;                                       // Mac +0x04, Win +0x08
};

} // namespace LEGO
