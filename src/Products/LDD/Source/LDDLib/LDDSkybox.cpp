// LEGO Digital Designer — restored source
// LDDSkybox.cpp (Mac TU 417, Win 0x65b1b5-0x65b514; LDDLib). See LDDSkybox.h.
#include "LDDSkybox.h"

namespace LEGO {

// Mac 0x395330, Win 0x65b241
LDDSkybox::LDDSkybox(Render* render, Directory* directory)
    : Skybox(render, directory)
{
}

// Mac 0x395410, Win 0x65b49a
bool LDDSkybox::Initialize()
{
    if (!Skybox::Initialize())
        return false;
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    Attach(preferences);
    return true;
}

// Mac 0x395cf0, Win 0x65b475
bool LDDSkybox::LoadTextures(BasicArray<RefPointer<Texture> >& textures, const String& name)
{
    bool loaded = Skybox::LoadTextures(textures, name);
    if (loaded)
        PlaySound(name);
    return loaded;
}

// Mac 0x395d30, Win 0x65b34e
void LDDSkybox::PlaySound(const String& name)
{
    String fileName(name);
    fileName << ".mp3";
    m_sound = CreateLDDSound(String(fileName));
    RefPointer<SimpleSound> sound = m_sound;
    NotifySoundChanged(sound, true);
}

// Mac 0x395ee0, Win 0x65b3f3
void LDDSkybox::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("PLAYSOUND")))
        PlaySound(m_names[m_current]);
}

// Mac 0x395be0, Win 0x65b4eb: the three inherited OnFinalRelease bodies, inlined in this order.
void LDDSkybox::OnFinalRelease()
{
    PreferencesObserver::OnFinalRelease();        // leave the Preferences
    SimpleSoundChangedNotifier::OnFinalRelease(); // every sound observer leaves
    Skybox::OnFinalRelease();
}

} // namespace LEGO
