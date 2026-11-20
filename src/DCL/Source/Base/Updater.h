// LEGO Digital Designer — restored source
// LEGO::Updater (Notifier calling Update() on each UpdateObserver), UpdateObserver, MainUpdater
// (main loop, 'UPDR'; releases ReleaseLater objects after each pass). TU "SystemUpdater.cpp"
// (Mac 0x1ddd0-0x2046b, Win 0x424060-0x424ab0 + 0x40bc80-0x40bdb0, 0x501150). Class names RTTI.
// Vtables Mac: Updater {D1 0x1e8b0, D0 0x1e920, OnFinalRelease 0x1e9a0, Update 0x1e9f0};
//   MainUpdater {D1 0x1f200, D0 0x1f270, 0x1e9a0, Update 0x1f2f0}; UpdateObserver {D1 0x1e2b0,
//   D0 0x1e2e0, 0x1e330, OnAttached 0x1e380, OnDetached 0x1e390, Update pure}.
// Win: Updater {Update 0x40bd00}, Object part {sdd 0x412410, ...}; MainUpdater {Update 0x424060};
//   UpdateObserver {OnAttached 0x40be80, OnDetached 0x40be80, _purecall}.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"
#include "ObjectRegistry.h"

namespace LEGO {

class Updater;

// Mac layout: vptr +0, Updater* subject +4, virtual base Object +8 (sizeof 0x10).
// Win: vptr +0, vbptr +4, subject +8, Object +0xc.
class UpdateObserver : public Observer<Updater, UpdateObserver> {
public:
    UpdateObserver() {}
    virtual ~UpdateObserver() {}                                      // Mac 0x1e2b0/0x1e2e0

    // Called once per pass of the subject's Update(). (name inferred)
    virtual void Update() = 0;                                        // Mac slot 5, Win slot 2
};

// Mac layout: vptr +0, BasicArray<UpdateObserver*> +0x04, BasicArray<Iterator*> +0x14,
// virtual base Object +0x24 (sizeof 0x2c). Win: vptr +0, vbptr +4, arrays +0x08/+0x18,
// Object +0x2c (sizeof 0x34).
class Updater : public Notifier<UpdateObserver> {
public:
    Updater() {}                                                      // Win 0x40bc80 (inlined on Mac)
    virtual ~Updater() {}                                             // Mac 0x1e8b0/0x1e920

    // Calls Update() on the observers registered when the pass starts (observers removed
    // meanwhile are skipped, see NotifierT::Iterator). (name inferred)
    virtual void Update();                                            // Mac 0x1e9f0, Win 0x40bd00
};

// The application's main-loop updater, created on demand under 'UPDR'.
class MainUpdater : public Updater {
public:
    enum { kRegistryId = 0x55504452 };                                // 'UPDR' (name inferred)

    MainUpdater() {}                                                  // inlined (Mac 0x1e800)
    virtual ~MainUpdater() {}                                         // Mac 0x1f200/0x1f270

    // Updater::Update, then releases the objects queued with ReleaseLater. (name inferred)
    virtual void Update();                                            // Mac 0x1f2f0, Win 0x424060

    // The shared main updater (null if it could not be created). (name inferred)
    static RefPointer<Updater> GetInstance();                         // Mac 0x1e0f0, Win 0x424670

    // Keeps `object` alive until the current thread's next MainUpdater pass
    // (= Thread::AddObject). Callers hand over a reference they are about to drop.
    // (name and class inferred)
    static void ReleaseLater(Object* object);                         // Mac 0x1ddd0, Win 0x424400
};

// Creates the MainUpdater for ObjectRegistry::Get('UPDR', ...).
// Mac vtable 0x75fd4c {D1 0x1e1d0, D0 0x1e7f0, CreateObject 0x1e800};
// Win 0xa1f76c {sdd 0x424600, CreateObject 0x424550}.
class MainUpdaterInstantiator : public ObjectRegistryInstantiator {
public:
    MainUpdaterInstantiator() {}
    virtual ~MainUpdaterInstantiator() {}                             // Mac 0x1e1d0/0x1e7f0, Win 0x424600
    virtual RefPointer<Object> CreateObject() const;                  // Mac 0x1e800, Win 0x424550
};

} // namespace LEGO
