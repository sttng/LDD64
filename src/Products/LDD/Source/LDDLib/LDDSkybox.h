// LEGO Digital Designer — restored source
// LEGO::LDDSkybox — the application's Skybox: every skybox switch announces the background
// sound "<skybox name>.mp3" (LDDUtilities' sound folder) to its SimpleSoundChangedObservers
// (ViewMode plays it), and the preference "PLAYSOUND" re-announces the current one.
//
// TU 417 "LDDSkybox.cpp" (Mac 0x395330-0x39607b, Win 0x65b1b5-0x65b51b; LDDLib).
// Created by ViewMode (Mac TU 115 vf44) with the "Graphics/Skyboxes" directory.
//
// Vtables: Mac 0x757bfc {Initialize 0x395410, Draw 0x3977b0, Next 0x3978a0, SetSkybox 0x397be0,
//          D1 0x395840, D0 0x3959f0, LoadTextures 0x395cf0, OnFinalRelease 0x395be0,
//          OnPreferenceChanged 0x395ee0} + SimpleSoundChangedNotifier-in 0x757c2c (offset
//          0x104), PreferencesObserver-in 0x757c44 (0x128), Object-in 0x757c74 (0x134);
//          Win 0x9d6d78 {0x65b49a, 0x5bde70, 0x5bdf70, 0x5be0d0, 0x65b475}, PreferencesObserver
//          0x9d6d68 {.., .., OnPreferenceChanged 0x65b3f3}, Object 0x9d6d54 {sdd 0x65b239 (-> 0x65b453),
//          OnFinalRelease 0x65b514 (-> 0x65b4eb), AddRef, Release}.
// Layout Mac: Skybox +0x00 (to 0x104), SimpleSoundChangedNotifier +0x104, PreferencesObserver
// +0x128 (subject +0x12c), RefPointer<SimpleSound> m_sound +0x130, Object +0x134 (sizeof 0x13c).
// Class name from RTTI; method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Skybox.h"
#include "Preferences.h"
#include "SimpleSound.h"
#include "SimpleSoundChangedNotifier.h"

namespace LEGO {

// LDDUtilities::GetInstance()->CreateSound(fileName), inlined by the original (Mac 0x147b30 +
// 0x14c5b0); the wrapper is restoration glue (LDDUtilitiesUI.cpp; tests define their own): a
// SimpleSound of <system directory 8>/Sound/<fileName>, null while sounds are off (LDDUtilities
// +0x08) or when the path does not resolve. (name inferred)
RefPointer<SimpleSound> CreateLDDSound(const String& fileName);

class LDDSkybox : public Skybox, public SimpleSoundChangedNotifier, public PreferencesObserver {
public:
    // Mac 0x395330 (C1), Win 0x65b241 (thiscall(render, directory, mostDerived)).
    LDDSkybox(Render* render, Directory* directory);
    virtual ~LDDSkybox() {}                                           // Mac 0x395840/0x3959f0 (body 0x395750), Win 0x65b1b5

    // Skybox::Initialize, then observes the Preferences. false when no skybox was built.
    virtual bool Initialize();                                        // 0 Mac 0x395410, Win 0x65b49a
    // Skybox::LoadTextures, and on success the skybox's sound is announced.
    virtual bool LoadTextures(BasicArray<RefPointer<Texture> >& textures,
                              const String& name);                    // 6 Mac 0x395cf0, Win 0x65b475

    SimpleSound* GetSound() const { return m_sound; }                 // (Mac 0x395710: RefPointer copy)

protected:
    // "PLAYSOUND" (any case): announce the current skybox's sound again.
    virtual void OnPreferenceChanged(const String& key);              // Mac 0x395ee0 (thunk 0x395f70), Win 0x65b3f3
    // Leaves the Preferences and every sound observer before the last reference goes.
    virtual void OnFinalRelease();                                    // Mac 0x395be0, Win 0x65b4eb (thunk 0x65b514)

    // m_sound = the sound "<name>.mp3", announced (play = true) to the observers.
    void PlaySound(const String& name);                               // Mac 0x395d30, Win 0x65b34e

    RefPointer<SimpleSound> m_sound;   // Mac +0x130, Win +0x138
};

} // namespace LEGO
