// LEGO Digital Designer — restored source
// LEGO::Trigger — a one-shot timer on the main updater: Start(ms) registers with the MainUpdater;
// the first Update() at or after the due time unregisters and calls OnTrigger. Base of
// TooltipManager and several LDD classes (13 Mac vtable copies).
// Header-only (weak): Mac D1 0x114100, D0 0x114130, Update 0x114180, OnTrigger 0x1141d0 (empty),
// Start 0x113ac0 (emitted in TU 049, TooltipManager.cpp); Win sdd 0x512ee0, 0x512e74.
// RTTI (Mac): vmi, bases UpdateObserver +0 (non-virtual), virtual Object. Vtable 0x70b0dc
// {D1, D0, OnFinalRelease 0x1e330, OnAttached 0x1e380, OnDetached 0x1e390, Update 0x114180,
// OnTrigger 0x1141d0}. Mac layout: vptr, Updater* +4, UInt64 due time +8, Object +0x10.
// Class name from RTTI; the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Updater.h"
#include "SystemTime.h"

namespace LEGO {

class Trigger : public UpdateObserver {
public:
    Trigger() : m_dueTime(0) {}
    virtual ~Trigger() {}                                                   // Mac 0x114100/0x114130

    // Fires once after `milliseconds` (GetMilliseconds based): registers with the main updater
    // (a running timer keeps its registration and gets the new due time). Mac 0x113ac0
    void Start(UInt32 milliseconds)
    {
        m_dueTime = (UInt64)GetMilliseconds() + (UInt64)milliseconds;
        RefPointer<Updater> updater = MainUpdater::GetInstance();
        Attach(updater.Get());
    }
    // Unregisters (no OnTrigger). (inlined everywhere: Observer::Detach)
    void Stop() { Detach(); }
    bool IsRunning() const { return GetSubject() != 0; }

    // Past the due time: unregister, then OnTrigger. Mac 0x114180
    virtual void Update()
    {
        if ((UInt64)GetMilliseconds() <= m_dueTime)
            return;
        Detach();
        OnTrigger();
    }
    // The timer fired. Mac 0x1141d0 (empty)
    virtual void OnTrigger() {}

protected:
    UInt64 m_dueTime;   // Mac +0x08
};

} // namespace LEGO
