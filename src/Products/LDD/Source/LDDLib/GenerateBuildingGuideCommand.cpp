// LEGO Digital Designer — restored source
// TU 283 "GenerateBuildingGuideCommand.cpp" (Mac 0x295230-0x296dcb):
// LEGO::GenerateBuildingGuideCommand (+ the Observer instances of BuildingGuideGenerator / BI and
// BuildingGuideGeneratorObserver's destructor).
#include "GenerateBuildingGuideCommand.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "ControlManager.h"
#include "Application.h"
#include "TextLocale.h"
#include "Scene.h"

namespace LEGO {

// Mac 0x2956f0, Win 0x653d80
GenerateBuildingGuideCommand::GenerateBuildingGuideCommand(BrickConnectSceneControl* sceneControl)
    : m_sceneControl(sceneControl), m_generated(false)
{
    m_generator = new BuildingGuideGenerator(sceneControl);
    m_generator->ShowDialog();
    BuildingGuideGeneratorObserver::Attach(m_generator);           // Mac 0x295230
    BuildingGuideObserver::Attach(sceneControl->GetBuildingInstructions());   // Mac 0x2954a0
}

// Mac 0x295e00 / 0x295fe0 (body 0x295cd0; thunks 0x295e60-0x2961a0), Win sdd 0x653ea7
GenerateBuildingGuideCommand::~GenerateBuildingGuideCommand()
{
}

// Mac 0x296220 (thunks 0x296390 / 0x2963b0 / 0x2963d0), Win 0x653ecb
void GenerateBuildingGuideCommand::OnFinalRelease()
{
    BuildingGuideObserver::OnFinalRelease();
    BuildingGuideGeneratorObserver::OnFinalRelease();
    Notifier<CommandObserver>::OnFinalRelease();
    Object::OnFinalRelease();
}

// Mac 0x295930, Win 0x653a05
void GenerateBuildingGuideCommand::Do()
{
    m_sceneControl->GetSceneManager()->SetActiveTool(0);
    m_generated = false;
    if (m_sceneControl->GetScene()->IsEmpty())
        return;
    m_generator->Start(false);
    SetEnable(false);
    Application::GetInstance()->RunModal(m_sceneControl->GetManager()->GetWindow(), this);
    if (!m_generated && !m_generator->GetApplication()->IsCancelled()) {
        Application::GetInstance()->Alert(2, LocalizedString(String("UNABLETOGENERATEBG")),
                                          LocalizedString(String("UNABLETOGENERATEBGTITLE")),
                                          LocalizedString(String("OK")), String(), String());
    }
    SetEnable(true);
}

// Mac 0x2958e0 (thunk 0x295900)
void GenerateBuildingGuideCommand::ModalUpdate()
{
    m_generator->Update();
}

// Mac 0x295bd0 (thunk 0x295c00)
void GenerateBuildingGuideCommand::OnGenerationFinished(bool generated)
{
    m_generated = generated;
    Application::GetInstance()->StopModal();
}

// Mac 0x295c30 (thunk 0x295c80), Win 0x653675
void GenerateBuildingGuideCommand::OnBuildingGuideChanged(bool hasSteps)
{
    SetEnable(hasSteps ? false : !m_sceneControl->GetScene()->IsEmpty());
}

// Mac 0x295920, Win 0x553620
bool GenerateBuildingGuideCommand::WasGenerated() const
{
    return m_generated;
}

} // namespace LEGO
