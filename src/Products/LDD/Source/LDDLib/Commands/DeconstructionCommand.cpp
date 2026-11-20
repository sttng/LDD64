// LEGO Digital Designer — restored source
// TU 234 "DeconstructionCommand.cpp": Mac 0x26cb60-0x26e30b, Win 0x65a066-0x65ab2f, 0x65afc0.
#include "DeconstructionCommand.h"

#include <iostream>

#include "LEGO/String.h"
#include "Scene.h"
#include "LDDUtilities.h"
#include "ConceptBrick.h"

namespace LEGO {

// Mac 0x26d090, Win 0x65afc0
DeconstructionCommand::DeconstructionCommand(BrickConnectSceneControl* sceneControl, Render* render)
    : Command(true, false),
      m_sceneControl(sceneControl),
      m_explosion(new FallingBricks(sceneControl, render)),
      m_assembly(new FallingBricks(sceneControl, render)),
      m_sound(0)
{
    FallingBricksObserver::Attach(m_explosion);
    m_sound = LDDUtilities::GetInstance()->CreateSound(String("sound_explosion.mp3"));
    PreferencesObserver::Attach(Preferences::GetInstance());
    BrickConnectSceneControlObserver::Attach(sceneControl);
}

// Mac 0x26d9c0 / 0x26dba0 (+ the D2 0x26d800), Win sdd 0x65ab10
DeconstructionCommand::~DeconstructionCommand()
{
}

// Mac 0x26dde0, Win 0x65ab2f
void DeconstructionCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    PreferencesObserver::OnFinalRelease();
    FallingBricksObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x26d3f0, Win 0x65a2fc
void DeconstructionCommand::Stop()
{
    m_explosion->Stop();
    m_assembly->Stop();
}

// Mac 0x26d460, Win 0x65a670
void DeconstructionCommand::Do()
{
    m_explosion->Stop();
    m_assembly->Stop();
    BasicArray<RefPointer<ConceptBrick> > conceptBricks;
    ConceptBrick::CreateConceptBricks(conceptBricks, m_sceneControl->GetScene());
    m_explosion->Start(conceptBricks, false, 8000, 0.0f, 100.0f, true,
                       FallingBricks::EasingSquareRoot, false, true, 0);
    if (m_sound) {
        m_sound->SetVolume(0xff);
        m_sound->Play();
    }
}

// Mac 0x26d420, Win 0x65a066
void DeconstructionCommand::UpdateEnable()
{
    SetEnable(!m_sceneControl->GetScene()->IsEmpty());
}

// Mac 0x26d5d0 (thunk 0x26d610), Win 0x65a310
void DeconstructionCommand::OnFinished(const BasicArray<RefPointer<ConceptBrick> >& conceptBricks,
                                       bool /*reverse*/)
{
    m_assembly->Start(conceptBricks, true, 6000, 1.5f, 100.0f, true, FallingBricks::EasingLinear,
                      false, true, 0);
}

// Mac 0x26d650 (thunk 0x26d780), Win 0x65a344
void DeconstructionCommand::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("PLAYSOUND")))
        m_sound = LDDUtilities::GetInstance()->CreateSound(String("sound_explosion.mp3"));
}

// Mac 0x26d7a0 (thunk 0x26d7b0), Win 0x65a08d
void DeconstructionCommand::OnAttached(BrickConnectSceneControl* /*control*/)
{
    UpdateEnable();
}

// Mac 0x26d7d0 (thunk 0x26d7e0), Win 0x65a098
void DeconstructionCommand::OnEvent19()
{
    UpdateEnable();
}

} // namespace LEGO
