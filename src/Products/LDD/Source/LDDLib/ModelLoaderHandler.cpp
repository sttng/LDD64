// LEGO Digital Designer — restored source
// ModelLoaderHandler — TU 144 "ModelLoaderHandler.cpp" (Mac 0x207870-0x20797b): OnLoadCancelled
// and the accessors of the load thread's state. Win: every one is inlined into its callers
// (LDDApplication::Update, BrickConnectSceneControl). See ModelLoaderHandler.h.
#include "ModelLoaderHandler.h"
#include "ModelLoaderProgressBarThread.h"

namespace LEGO {

// Mac 0x207870
void ModelLoaderHandler::OnLoadCancelled()
{
    m_thread->Cancel();
}

// Mac 0x207880
bool ModelLoaderHandler::IsCancelled() const
{
    return m_thread->IsCancelled();
}

// Mac 0x207890
bool ModelLoaderHandler::IsLoading() const
{
    return m_thread->IsRunning();
}

// Mac 0x2078a0
UInt32 ModelLoaderHandler::GetProgress() const
{
    return m_thread->GetProgress();
}

// Mac 0x2078b0
bool ModelLoaderHandler::Succeeded() const
{
    return m_thread->Succeeded();
}

// Mac 0x2078c0
UInt32 ModelLoaderHandler::GetMissingBrickCount() const
{
    return m_thread->GetMissingBrickCount();
}

// Mac 0x2078e0
UInt32 ModelLoaderHandler::GetUnplaceableBrickCount() const
{
    return m_thread->GetUnplaceableBrickCount();
}

} // namespace LEGO
