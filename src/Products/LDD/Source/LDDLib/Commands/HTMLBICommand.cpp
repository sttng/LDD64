// LEGO Digital Designer — restored source
// TU 240 "HTMLBICommand.cpp": Mac 0x270bf0-0x27163b, Win 0x65e6ef-0x65ea78. See HTMLBICommand.h.
// HTMLBIGeneratorObserver / Observer<HTMLBIGenerator, HTMLBIGeneratorObserver> (Mac
// 0x270e60-0x271110, Win 0x65e72c-0x65e892) are the header-inline classes of HTMLBIGenerator.h.
#include "HTMLBICommand.h"
#include "BrickConnectSceneControl.h"
#include "Scene.h"

#include <iostream>

namespace LEGO {

// Mac 0x270bf0, Win 0x65e8bd
HTMLBICommand::HTMLBICommand(BrickConnectSceneControl* sceneControl, RenderControlManager* manager,
                             BrickConnectSceneManager* sceneManager, BGMode* mode, Window* window)
    : Command(true, false),
      m_generator(new HTMLBIGenerator(sceneControl, manager, sceneManager, mode)),
      m_sceneControl(sceneControl), m_window(window)
{
}

// Mac 0x271170 (D1) / 0x2711d0 (D0), base object destructor 0x2714c0; Win sdd 0x65ea36
HTMLBICommand::~HTMLBICommand()
{
}

// Mac 0x271240, Win 0x65ea78
void HTMLBICommand::OnFinalRelease()
{
    HTMLBIGeneratorObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x270e40, Win 0x65e8b1
void HTMLBICommand::Do()
{
    m_generator->Generate(m_window);
}

// Mac 0x270d60, Win 0x65e6ef
void HTMLBICommand::UpdateEnable()
{
    SetEnable(!m_sceneControl->GetScene()->IsEmpty());
}

// Mac 0x270da0, Win 0x65e716
void HTMLBICommand::OnGenerationStarted()
{
    SetEnable(false);
}

// Mac 0x270de0, Win 0x65e721
void HTMLBICommand::OnGenerationFinished()
{
    SetEnable(true);
}

// Mac 0x270e20, Win 0x6943f1
void HTMLBICommand::OnGenerationEvent()
{
}

} // namespace LEGO
