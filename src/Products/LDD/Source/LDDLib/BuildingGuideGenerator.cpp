// LEGO Digital Designer — restored source
// TU 284 "BuildingGuideGenerator.cpp" (Mac 0x296dd0-0x29941b): LEGO::BuildingGuideGenerator,
// LEGO::BuildingGuideGenerateThread (+ the Observer / Notifier template instances of the TU).
#include "BuildingGuideGenerator.h"
#include "BrickConnectSceneControl.h"
#include "ProgressBarDialog.h"
#include "RenderControlManager.h"
#include "TextLocale.h"
#include "BI.h"
#include "BIStorage.h"
#include "BIGenerator.h"
#include "BIProgramGeneric.h"
#include "Scene.h"
#include "BrickPhysics.h"

namespace LEGO {

// Mac 0x296dd0
BuildingGuideGenerator::BuildingGuideGenerator(BrickConnectSceneControl* sceneControl)
    : m_sceneControl(sceneControl), m_startDialog(false), m_finished(false)
{
    RenderControlManager* manager = static_cast<RenderControlManager*>(sceneControl->GetManager());
    m_application = new BGAutomaticApplication(manager->GetRender());
}

// Mac 0x298770 / 0x2987d0 (thunks 0x298920 / 0x298980, 0x298a10 / 0x298a70, 0x298b00 /
// 0x298b60), Win sdd 0x68b644
BuildingGuideGenerator::~BuildingGuideGenerator()
{
}

// Mac 0x298840 (thunks 0x2989f0 / 0x298ae0 / 0x298be0)
void BuildingGuideGenerator::OnFinalRelease()
{
    BGAutomaticApplicationObserver::OnFinalRelease();
    Notifier<BuildingGuideGeneratorObserver>::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
    Object::OnFinalRelease();
}

// Mac 0x296fd0
void BuildingGuideGenerator::ShowDialog()
{
    m_dialog = new ProgressBarDialog(m_sceneControl->GetManager());
    m_dialog->Initialize();
}

// Mac 0x297060, Win 0x68b11c
void BuildingGuideGenerator::Start(bool reset)
{
    if (reset)
        m_sceneControl->GetBuildingInstructions()->Reset();
    BGAutomaticApplication* application = m_application;
    Scene* scene = m_sceneControl->GetScene();
    bool started;
    {
        RefPointer<BIStorage> storage = m_sceneControl->GetBuildingInstructions()->GetStorage();
        started = application->Start(scene, storage);
    }
    if (!started)
        return;
    BGAutomaticApplicationObserver::Attach(m_application);
    m_application->SetProgram(new BIProgramGeneric(true, true, 0.5f, 0.5f, 0.5f));
    {
        RefPointer<Updater> updater = MainUpdater::GetInstance();
        UpdateObserver::Attach(updater);
    }
    m_startDialog = true;
    m_finished = false;
    m_thread = new BuildingGuideGenerateThread(this, m_application);
    m_thread->Start(0);
}

// Mac 0x297700
void BuildingGuideGenerator::StoreGuide()
{
    if (m_application->HasGenerators() && m_application->GetRootGenerator()->HasEvents()) {
        RefPointer<BIStorage> storage = new BIStorage();
        m_application->Finish(storage);
        m_sceneControl->GetBuildingInstructions()->SetStorage(storage);
        m_sceneControl->SetModified(true);
        return;
    }
    m_application->Stop();
}

// Mac 0x297820
void BuildingGuideGenerator::OnThreadFinished(bool success)
{
    if (!success)
        m_application->Stop();
    else
        StoreGuide();
    UpdateObserver::Detach();
    BGAutomaticApplicationObserver::Detach();
    m_finished = true;
    Iterator it(*this);
    while (BuildingGuideGeneratorObserver* observer = it.Next(m_observers))
        observer->OnGenerationFinished(success);
}

// Mac 0x297b20 (thunk 0x297b60), Win 0x68affa
void BuildingGuideGenerator::OnProgress(Real32 progress)
{
    if (m_thread.Get() != 0)
        m_thread->m_progress = (UInt32)(Int32)(progress * 100.0f);
}

// Mac 0x297ba0, Win 0x68b011
void BuildingGuideGenerator::Update()
{
    if (m_startDialog) {
        m_startDialog = false;
        m_dialog->SetProgress(0.0f, 10.0f);
        m_dialog->SetMessage(LocalizedString(String("GENERATINGBUILDINGGUIDE")));
    }
    if (m_dialog->IsCancelled()) {
        m_application->Cancel();
        if (m_thread.Get() == 0)
            return;
        m_thread->Cancel();
    } else if (m_thread.Get() == 0) {
        return;
    }
    if (!m_thread->IsRunning()) {
        m_thread->Finish();
        m_thread = 0;
        m_dialog->EndModal();
    } else {
        m_dialog->SetProgress((Real32)m_thread->GetProgress(), 100.0f);
    }
}

// ---------------------------------------------------------------------------------------------

// Mac 0x297d30 (Thread-in copy 0x297e10), Win 0x68ae90
bool BuildingGuideGenerateThread::Run()
{
    const Vector2& size = m_generator->m_sceneControl->GetSize();
    m_application->SetViewSize(size.x, size.y);
    m_application->GetBrickScene()->GetBrickPhysics()->Unknown3();
    m_application->GetBrickScene()->GetPhysicsScene()->Unknown5(true);
    m_result = m_application->Generate();
    m_application->GetBrickScene()->GetPhysicsScene()->Unknown5(false);
    m_application->GetBrickScene()->GetBrickPhysics()->Unknown4();
    m_application->ApplyEvents();
    return m_result >= 0;
}

// Mac 0x297ee0, Win 0x68b6dc
bool BuildingGuideGenerateThread::Finish()
{
    if (IsCancelled()) {
        m_generator->OnThreadFinished(false);
        return false;
    }
    bool success = m_result != -1;
    m_generator->OnThreadFinished(success);
    return success;
}

} // namespace LEGO
