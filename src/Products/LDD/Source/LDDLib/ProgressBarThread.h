// LEGO Digital Designer — restored source (header only)
// LEGO::ProgressBarThread — a Thread whose work can be cancelled and reports a percentage for a
//   progress bar: the worker reads the cancel flag, the main thread polls the progress
//   (ModelLoaderHandler, Mac 0x207870-0x2078a0, reads both fields directly).
//
// RTTI (Mac): si, base Thread; no vtable of its own is emitted (every member inline). Win RTTI
//   ProgressBarThread : Thread, vtable 0x9cf18c {sdd 0x612361, Thread's handlers} (the
//   inlined constructor of ModelLoaderProgressBarThread stores it first).
// Layout (both builds): Thread +0x00..+0x70, m_cancelled +0x70, m_progress +0x74; sizeof 0x78.
// Class name from RTTI; member and method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Thread.h"

namespace LEGO {

class ProgressBarThread : public Thread {
public:
    // Inline (Mac 0x53190 inside ModelLoaderProgressBarThread's constructor, TU 021).
    ProgressBarThread() : m_cancelled(false), m_progress(0) {}
    virtual ~ProgressBarThread() {}                                         // Win sdd 0x612361

    // The worker should stop (ModelLoaderHandler::OnLoadCancelled, Mac 0x207870).
    void   Cancel()            { m_cancelled = true; }
    bool   IsCancelled() const { return m_cancelled; }
    // 0..100, written by the worker.
    UInt32 GetProgress() const { return m_progress; }

protected:
    volatile bool   m_cancelled;   // +0x70
    volatile UInt32 m_progress;    // +0x74
};

} // namespace LEGO
