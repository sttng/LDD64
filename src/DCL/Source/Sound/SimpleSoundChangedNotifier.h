// LEGO Digital Designer — restored source
// LEGO::SimpleSoundChangedObserver / LEGO::SimpleSoundChangedNotifier — "the background sound
// changed" notification (LDDSkybox -> ViewMode, which plays the sound).
// TU 294 "SimpleSoundChangedNotifier.cpp": Mac 0x2a2660-0x2a344f, Win 0x659a7f-0x65a12a. Apart
// from OnSoundChanged and NotifySoundChanged the TU holds only template instances (Notifier.h,
// ObserverN.h): ObserverN<SimpleSoundChangedNotifier, SimpleSoundChangedObserver>,
// NotifierT<…>, Notifier<SimpleSoundChangedObserver>, BasicArray<SimpleSoundChangedNotifier*>,
// BasicArray<SimpleSoundChangedObserver*>, BasicArray<NotifierT<…>::Iterator*>.
//  * SimpleSoundChangedObserver : ObserverN<…>. Mac vtable 0x72110c {D1 0x2a2700, D0 0x2a27b0,
//    OnFinalRelease 0x1cff30, OnAttached 0x1d1700, OnDetached 0x1d1710, OnSoundChanged 0x2a2660};
//    Win own vftable 0x9d6ae8 {OnAttached, OnDetached (0x65a12a), OnSoundChanged 0x659a7f}.
//  * SimpleSoundChangedNotifier : Notifier<SimpleSoundChangedObserver>. Mac vtable 0x757d3c
//    {D1 0x2a2c90, D0 0x2a2d80, OnFinalRelease 0x2a2f90 (NotifierT: every observer Detach(this),
//    last first)}; Win Object-in vftable 0x9d6b20 {sdd 0x659f90 -> 0x65a00b}; ctor Win 0x659f2b.
// Method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Notifier.h"
#include "ObserverN.h"
#include "SimpleSound.h"

namespace LEGO {

class SimpleSoundChangedNotifier;

class SimpleSoundChangedObserver
    : public ObserverN<SimpleSoundChangedNotifier, SimpleSoundChangedObserver> {
public:
    SimpleSoundChangedObserver() {}
    virtual ~SimpleSoundChangedObserver();                           // Mac 0x2a2700/0x2a27b0 (thunks 0x2a2750/0x2a2820)  Win 0x659cc3/0x659e34

    // `sound` (null: sounds off or no file) should be played (`play`). Default: nothing.
    virtual void OnSoundChanged(const RefPointer<SimpleSound>& sound, bool play); // Mac 0x2a2660  Win 0x659a7f
};

class SimpleSoundChangedNotifier : public Notifier<SimpleSoundChangedObserver> {
public:
    SimpleSoundChangedNotifier() {}                                  // Win 0x659f2b (+ NotifierT 0x659db9, Notifier 0x659e53)
    virtual ~SimpleSoundChangedNotifier();                           // Mac 0x2a2c90/0x2a2d80 (thunks 0x2a2d00/0x2a2e00)  Win 0x659f98/0x65a00b

    // Every observer registered when the pass starts, in order (observers leaving during the
    // pass are skipped: NotifierT::Iterator); each gets its own RefPointer copy of `sound`.
    void NotifySoundChanged(const RefPointer<SimpleSound>& sound, bool play); // Mac 0x2a28a0  Win 0x659cd6
};

} // namespace LEGO
