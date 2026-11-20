// LEGO Digital Designer — restored source
// LEGO::DelayedCommand — a Command that runs itself once on the next pass of the main updater
//   (it observes MainUpdater from construction; Update runs Do and leaves the updater).
//   Base of StartupDelayedCommand.
//
// TU 236 "DelayedCommand.cpp" (Mac 0x26e9b0-0x26eecf; Win 0x5c9c06-0x5c9da6, Update 0x609098).
// The constructor is header-inline (Win 0x5c9f2e; inlined on the Mac).
// Mac layout: Command +0x00..+0x28, UpdateObserver +0x28 (subject +0x2c), Object +0x30
// (sizeof 0x38). Win: UpdateObserver +0x2c (subject +0x34), Object +0x3c.
// Vtables: Mac 0x73ab9c {D1 0x26ea50, D0 0x26eac0, OnFinalRelease 0x26eb50, Do 0x26ebe0 (empty),
// SetEnable, GetEnable, UpdateEnable 0x26ebf0, Update 0x26e9b0 (Mac-only primary entry of the
// UpdateObserver overrider)}; Win 0x9ca238 {Do 0x5c8021, SetEnable, GetEnable, UpdateEnable
// 0x630465}, UpdateObserver-in 0x9ca228 {OnAttached, OnDetached, Update 0x609098}.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "Updater.h"

namespace LEGO {

class DelayedCommand : public Command, public UpdateObserver {
public:
    // Win 0x5c9f2e (name of the argument inferred)
    explicit DelayedCommand(bool enabled) : Command(enabled)
    {
        RefPointer<Updater> updater = MainUpdater::GetInstance();
        UpdateObserver::Attach(updater);
    }
    virtual ~DelayedCommand();                                              // Mac 0x26ea50/0x26eac0, Win 0x5c9c0e (sdd 0x5c9da6)

    virtual void Do();                                                      // 3/0 Mac 0x26ebe0 (empty), Win 0x5c8021
    virtual void UpdateEnable();                                            // 6/3 Mac 0x26ebf0 (SetEnable(true)), Win 0x630465
    // Do, then leave the updater (runs once).
    virtual void Update();                                                  // 7/– Mac 0x26e9b0 (thunk 0x26ea00), Win 0x609098

protected:
    // Observer<Updater> part, then NotifierT<Command>'s. Mac 0x26eb50 (thunks 0x26ed20, 0x26ee70)
    virtual void OnFinalRelease();
};

} // namespace LEGO
