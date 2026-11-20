// LEGO Digital Designer — restored source
// LEGO::ModelLoaderHandler — interface of the object that receives a model while it is loaded
//   in the background: LDDApplication::LoadDocument (Mac 0x1765c0) hands the StorageScene2 to
//   OnLoadStarted; the handler starts a ModelLoaderProgressBarThread (kept at +4) and gets
//   OnLoadFinished when the thread is done. The non-virtual accessors read that thread.
//   The only implementation is BrickConnectSceneControl (its sub-object at Mac +0x1d0).
//
// Home TU 144 "ModelLoaderHandler.cpp" (Mac 0x207870-0x20797b: OnLoadCancelled + the six
// accessors). The destructor / OnFinalRelease are inline
// (emitted with BrickConnectSceneControl, Mac 0x522c0 / 0x522e0 / 0x45060).
// Mac layout: {vptr, m_thread +0x04, virtual base Object +0x08 (Object-in vtable 0x704dfc)}.
// Mac vtable 0x704dcc: OnLoadStarted (pure), OnLoadFinished (pure), OnLoadCancelled 0x207870,
//   D1 0x522c0, D0 0x522e0, OnFinalRelease 0x45060.
// Method names are inferred (LDDApplication::Update reads the progress accessors).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"

namespace LEGO {

struct StorageScene2;
class ModelLoaderProgressBarThread;

class ModelLoaderHandler : public virtual Object {
public:
    ModelLoaderHandler() : m_thread(0) {}
    // A model starts loading into `storage` (BrickConnectSceneControl Mac 0x33e90).
    virtual void OnLoadStarted(StorageScene2* storage) = 0;                 // 0
    // The load thread finished (BrickConnectSceneControl Mac 0x346c0).
    virtual void OnLoadFinished() = 0;                                      // 1
    // Asks the load thread to stop (thread +0x70 = true). Mac 0x207870
    virtual void OnLoadCancelled();                                         // 2
    virtual ~ModelLoaderHandler() {}                                        // Mac 0x522c0/0x522e0

    // ---- the load thread's state (TU 144; field offsets of the thread) ----
    bool   IsCancelled() const;                     // Mac 0x207880 (+0x70)
    bool   IsLoading() const;                       // Mac 0x207890 (+0x5a)
    UInt32 GetProgress() const;                     // Mac 0x2078a0 (+0x74, 0..100)
    bool   Succeeded() const;                       // Mac 0x2078b0 (+0x58)
    UInt32 GetMissingBrickCount() const;            // Mac 0x2078c0 (+0x9c)
    UInt32 GetUnplaceableBrickCount() const;        // Mac 0x2078e0 (+0xa0)

protected:
    // Object::OnFinalRelease only. Mac 0x45060
    virtual void OnFinalRelease() { Object::OnFinalRelease(); }

    ModelLoaderProgressBarThread* m_thread;         // Mac +0x04 (owned: deleted through Thread D0)
};

} // namespace LEGO
