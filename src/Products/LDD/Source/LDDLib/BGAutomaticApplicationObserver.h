// LEGO Digital Designer — restored source
// LEGO::BGAutomaticApplicationObserver — observer of a BGAutomaticApplication (BGAutomaticMode,
//   BuildingGuideGenerator). Every name is inferred.
//
// Home TU 284 "BuildingGuideGenerator.cpp" (Mac: destructor 0x298420 / 0x298450, empty slots
// 0x298510-0x298540; Win sdd 0x624b31).
// RTTI: class LEGO::BGAutomaticApplicationObserver : Observer<BGAutomaticApplication,
// BGAutomaticApplicationObserver> (: virtual Object). Mac vtable 0x7408dc {D1, D0,
// OnFinalRelease 0x2984a0, OnAttached 0x2984f0, OnDetached 0x298500, idx 5-8: 0x298510,
// 0x298520, 0x298530, 0x298540 (empty), idx 9 pure}; Win vtable 0x9cff40 {0x65a12a,
// 0x65a12a, 0x5c8021, 0x5c8021, 0x65a12a, 0x65a12a, purecall} (OnAttached, OnDetached, 5-9).
#pragma once
#include "LEGO/Types.h"
#include "Notifier.h"

namespace LEGO {

class BGAutomaticApplication;

class BGAutomaticApplicationObserver : public Observer<BGAutomaticApplication, BGAutomaticApplicationObserver> {
public:
    BGAutomaticApplicationObserver() {}
    virtual ~BGAutomaticApplicationObserver() {}                          // Mac 0x298420 / 0x298450, Win sdd 0x624b31

    // The candidate events changed (BGAutomaticApplication::SelectCandidate). Mac 0x298510
    virtual void OnCandidatesChanged() {}                                  // idx 5
    // The view state was re-evaluated (UpdateView). Mac 0x298520
    virtual void OnViewUpdated() {}                                        // idx 6
    // The step display changed (UpdateView); `atStep`: at a step boundary. Mac 0x298530
    virtual void OnStepChanged(bool /*atStep*/) {}                         // idx 7
    // Not called by the restored code. Mac 0x298540
    virtual void OnUnknown8() {}                                           // idx 8
    // Generation progress, 0..1. Pure.
    virtual void OnProgress(Real32 progress) = 0;                          // idx 9
};

} // namespace LEGO
