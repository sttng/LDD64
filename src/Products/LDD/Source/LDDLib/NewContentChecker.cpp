// LEGO Digital Designer — restored source
// TU 226 "NewContentChecker.cpp" (Mac 0x269c10-0x26bafb, Win 0x693c2d-0x69480f):
// NewContentChecker (+ instantiator) and NewContentCallerDelegator. See NewContentChecker.h.
// Template instances emitted here (restored through the shared templates): NotifierT /
// Notifier of NewContentChecker (Mac 0x26a610-0x26ab40, 0x26b380), their BasicArray
// destructors (0x26af80-0x26b010), the observer removal 0x26b690.
#include "NewContentChecker.h"

#include "Application.h"
#include "PaletteNames.h"
#include "Updater.h"
#include "File.h"

#include <iostream>

namespace LEGO {

// ===========================================================================================
// NewContentCallerDelegator
// ===========================================================================================

// Mac 0x26b110, Win 0x693f0c
void NewContentCallerDelegator::OnResponse(UInt32 size)
{
    NewContentCaller::OnResponse(size);
    if (HasError()) {
        OnFailed();
        return;
    }
    m_handler->OnNewContentSucceeded(size, m_collections);
}

// Mac 0x26b160, Win 0x693f40
void NewContentCallerDelegator::OnProgress(UInt32 received, UInt32 total)
{
    NewContentCaller::OnProgress(received, total);
    m_handler->OnNewContentProgress(received, total);
}

// Mac 0x26b1a0, Win 0x693f58
void NewContentCallerDelegator::OnFailed()
{
    NewContentCaller::OnFailed();
    m_handler->OnNewContentFailed();
}

// ===========================================================================================
// NewContentChecker
// ===========================================================================================

// Mac 0x269c10
NewContentChecker* NewContentChecker::GetInstance()
{
    NewContentCheckerInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRegistryId, instantiator);
    if (object.IsNull())
        return 0;
    return dynamic_cast<NewContentChecker*>(object.Get());
}

// Mac 0x26a090, Win 0x6940ca
void NewContentChecker::Notify(UInt32 status)
{
    Iterator it(*this);
    while (NewContentCheckerObserver* observer = it.Next(m_observers))
        observer->OnNewContentChecked(status);
}

// Mac 0x269cb0
void NewContentChecker::Check(const Version& brickVersion)
{
    m_brickVersion = brickVersion;
    if (m_caller.Get())
        MainUpdater::ReleaseLater(m_caller.Get());
    m_caller = new NewContentCallerDelegator(this);
    m_caller->Call();
}

// Mac 0x269e20 (thunk 0x26a310)
void NewContentChecker::OnNewContentSucceeded(UInt32, const String& collections)
{
    Version version;
    if (Application::GetInstance()->GetVersion(version)
        && version < m_caller->GetLatestVersion()) {
        Notify(NewContentCheckerObserver::kNewApplication);
        return;
    }
    // NOTE (both builds): the collections are split into a list nobody reads.
    PaletteNames names;
    BasicArray<String> parts;
    collections.Split((UniChar16)',', parts);

    if (m_brickVersionCaller.Get())
        MainUpdater::ReleaseLater(m_brickVersionCaller.Get());
    m_brickVersionCaller = new BrickVersionDownloaderCaller(this);
    m_brickVersionCaller->Start();
}

// Mac 0x26a330 (thunk 0x26a350)
void NewContentChecker::OnNewContentFailed()
{
    Notify(NewContentCheckerObserver::kFailed);
}

// Mac 0x26a370 (thunk 0x26a4c0)
void NewContentChecker::OnBrickVersionsDownloaded(UInt32, const Path& file)
{
    BrickVersionLoaderXML loader;
    BrickVersionMap versions;
    RefPointer<File> opened = file.OpenFile(OpenMode(true, false, true, false), false);
    if (opened.IsNull())
        return;
    bool fullUpdate = false;
    if (!loader.Load(opened.Get(), versions, fullUpdate, m_brickVersion))
        return;
    if (versions.GetSize() == 0)
        Notify(NewContentCheckerObserver::kNoNewContent);
    else
        Notify(NewContentCheckerObserver::kNewBricks);
}

// Mac 0x26a4e0
void NewContentChecker::OnBrickVersionsFailed()
{
    Notify(NewContentCheckerObserver::kFailed);
}

// Mac 0x26acb0
void NewContentChecker::OnFinalRelease()
{
    NotifierT<NewContentChecker, NewContentCheckerObserver>::OnFinalRelease();
}

// Mac 0x26ae90, Win 0x693c2d (?)
RefPointer<Object> NewContentCheckerInstantiator::CreateObject() const
{
    return RefPointer<Object>(new NewContentChecker());
}

} // namespace LEGO
