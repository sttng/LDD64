// LEGO Digital Designer — restored source
// LEGO::Operation, OperationQueue, OperationManager (+ OperationThread) — work items on a small
// worker pool. TU 270 (file name inferred): Mac 0x2827e0-0x285631 (Obj-C++: on 10.5+ forwards to
// NSOperation via LEGOMacOperation, else the pthread pool), Win 0x539140-0x53a31f.
// Pool model: one static manager (Mac 0x77e758, ctor 0x284b10 from static init 0x285590; Win
//   0xb44050, ctor 0x539140 / dtor 0x539220 via CRT init 0x930bc0) with a recursive lock, a
//   wake-up Event, the pending operations and the workers. A worker runs the first operation
//   whose dependencies are finished (Execute() unlocked); cancelled ones are dropped.
//   OperationQueue::Wait runs ready operations of its queue on the caller, else sleeps (traces
//   "OperationManager::Wait locked." each second). The first queue starts the workers: Win
//   max(1, CPUs - 1), Mac 2; Thread::Start needs a current LEGO::Thread on the caller (tests too).
// Class names from RTTI, OperationManager from the trace string; the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Event.h"
#include "Thread.h"
#include "ThreadHook.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LEGO {

class OperationQueue;
class OperationManager;

// A unit of work. Derived classes implement Execute(). RTTI: vmi, virtual base Object.
// Vtables: Mac 0x705e8c {Execute (pure), D1 0x283010, D0 0x2830d0}, Object-in-Operation 0x705eb0
// {thunks 0x283070 0x283140, Object slots}; Win 0xa2b034 {Execute (purecall)}, 0xa2b03c
// {sdd 0x539b30, Object slots}. Own data 0x30 bytes before a derived class's members (Mac:
// vptr +0; Win: vfptr +0, vbptr +4), Object follows the most derived class.
class Operation : public virtual Object {
public:
    Operation();                                                     // Mac 0x282b00 (C2)  Win 0x539a70
    // The work. Runs on a worker thread (or on a thread inside OperationQueue::Wait), unlocked.
    virtual void Execute() = 0;                                      // slot 0 (name inferred)
    // Leaves the dependency graph (both directions) and wakes the workers.
    virtual ~Operation();                                            // Mac 0x283010/0x2830d0  Win 0x539bf0/0x539b30

    // This operation will not start before `operation` has finished (or was dropped).
    // NOTE: no duplicate check (both builds); call before adding to a queue.
    void AddDependency(Operation* operation);                        // Mac 0x2831c0  Win 0x539b60
    // Marks the operation cancelled and wakes the workers; it is dropped instead of run if it
    // has not started yet. Execute() implementations may poll IsCancelled().
    void Cancel();                                                   // Mac 0x283630  Win 0x539ba0
    bool IsCancelled() const;                                        // Mac 0x2836f0  Win 0x539be0
    // (inlined accessors, names inferred)
    bool IsExecuting() const;                                        // (Mac: -isExecuting on 10.5+)
    bool IsFinished() const { return m_finished; }

private:
    friend class OperationQueue;
    friend class OperationManager;
#if !defined(_WIN32)
    friend struct OperationCocoaAccess;
#endif

    // Removes this operation from the dependency list of every operation that depends on it
    // and forgets them (called with the manager locked when it finished or was dropped).
    void ReleaseDependents();                                        // Win 0x539d30 (Mac inlined, 0x282e10 loops)

    Operation(const Operation&);
    Operation& operator=(const Operation&);

    OperationQueue*        m_queue;          // Mac +0x04  Win +0x08 queue it was added to
    BasicArray<Operation*> m_dependencies;   // Mac +0x08  Win +0x0c unfinished operations it waits for
    BasicArray<Operation*> m_dependents;     // Mac +0x18  Win +0x1c operations waiting for it
    bool                   m_cancelled;      // Mac +0x28  Win +0x2c
    bool                   m_executing;      // Mac +0x29  Win +0x2d
    bool                   m_finished;       // Mac +0x2a  Win +0x2e
#if !defined(_WIN32)
    void*                  m_cocoaOperation; // Mac +0x2c LEGOMacOperation* (Mac OS X >= 10.5, else 0)
#endif
};

// A set of operations that can be waited for. RTTI: si Object. Vtable Mac 0x73e4c4 {D1 0x284300,
// D0 0x284310, Object slots}; Win 0xa2b020 {sdd 0x539e20, Object slots}. Mac sizeof 0x1c,
// Win 0x18.
class OperationQueue : public Object {
public:
    // Starts the worker threads (first queue only).
    OperationQueue();                                                // Mac 0x283730 (C1 0x283e30)  Win 0x539da0
    // Waits for (and helps running) every operation still in the queue.
    virtual ~OperationQueue();                                       // Mac 0x283e40/0x284300/0x284310  Win 0x539e40/0x539e20

    // Queues `operation` (keeps a reference until it finished or was dropped). (name inferred)
    void Add(Operation* operation);                                  // Mac 0x284350  Win 0x539eb0
    // Cancels every queued operation. (name inferred)
    void CancelAll();                                                // Mac 0x284660  Win 0x539f90
    // Returns when the queue is empty; runs ready operations on the calling thread meanwhile.
    // (name inferred)
    void Wait();                                                     // Mac 0x283f60  Win 0x53a000

    UInt32 GetOperationCount() const { return m_operations.GetSize(); }   // (accessor, name inferred)

private:
    friend class OperationManager;

    BasicArray<RefPointer<Operation> > m_operations;   // +0x08 (Mac vtable 0x774c14, Win BasicArray<RefPointer<Operation>>)
#if !defined(_WIN32)
    void*                              m_cocoaQueue;   // Mac +0x18 NSOperationQueue* (10.5+, else 0)
#endif
};

// The worker pool (one static instance, see the header comment). Not an Object.
// Mac sizeof 0xa0, Win 0x44.
class OperationManager {
public:
    // A worker: waits on its Event, then runs operations until none is ready.
    // RTTI: si Thread. Vtable Mac 0x774be0 {D1 0x284f60, D0 0x284fc0, OnThreadStart 0xeb80,
    // OnThreadEnd 0xeb90, Run 0x285040, OnAttach 0xebb0, OnDetach 0xebc0}; Win vftable
    // {sdd 0x539a50, …, Run 0x539a40}. sizeof Mac 0xc0, Win 0x78.
    class OperationThread : public Thread {
    public:
        explicit OperationThread(OperationManager* manager);         // Win 0x539970 (Mac inlined in 0x283840)
        virtual ~OperationThread();                                  // Mac 0x284f60/0x284fc0  Win 0x5399e0/0x539a50

        void WakeUp() { m_wakeUp.Signal(); }                         // (inlined, name inferred)

    protected:
        virtual bool Run();                                          // Mac 0x285040  Win 0x539a40

    private:
        OperationManager* m_manager;   // +0x70
        Event             m_wakeUp;    // +0x74 (Mac cond +0x74, mutex +0x90, flag +0xbc)
    };

    OperationManager();                                              // Mac 0x284b10  Win 0x539140
    // Stops and deletes the workers (spins until none is running).
    ~OperationManager();                                             // Mac 0x284cb0  Win 0x539220

    // The process-wide instance (a static object in OperationManager.cpp). (name inferred)
    static OperationManager& GetInstance();

    void Lock();                                                     // (inlined: EnterCriticalSection / pthread_mutex_lock)
    // Unlocks; when `signal` is set: clears it and wakes every worker and the manager Event.
    void Unlock(bool& signal);                                       // Mac 0x284930  Win 0x5394a0 (name inferred)
    // Unlock(signal), sleeps on the manager Event (tracing every second), locks again.
    void Wait(bool& signal);                                         // Mac 0x2849d0  Win 0x5394e0 (name inferred)
    // Wakes every worker and the manager Event. (inlined everywhere, name inferred)
    void SignalAll();

    // Creates the workers if there are none. (name inferred)
    void StartThreads();                                             // Mac 0x283840  Win 0x5393e0
    // (lock held) operations waiting to be started. (names inferred)
    void AddOperation(Operation* operation);                         // Win 0x5395b0 (Mac inlined in 0x284350)
    void RemoveOperation(Operation* operation);                      // Mac 0x284750  Win 0x539640
    // Body of every worker: returns true when the manager shuts down. (name inferred)
    bool RunOperations(Event& wakeUp);                               // Mac 0x285060  Win 0x5396f0

    UInt32 GetThreadCount() const { return m_threads.GetSize(); }   // (accessor, name inferred)
    UInt32 GetPendingCount() const { return m_operations.GetSize(); } // (accessor, name inferred)

private:
    OperationManager(const OperationManager&);
    OperationManager& operator=(const OperationManager&);

#if defined(_WIN32)
    CRITICAL_SECTION                   m_lock;        // Win +0x00
#else
    pthread_mutex_t                    m_lock;        // Mac +0x00 (recursive)
#endif
    Event                              m_event;       // Win +0x18 / Mac +0x2c: wakes OperationQueue::Wait
    RefPointer<ThreadHook>             m_threadHook;  // Win +0x1c / Mac +0x78 (kept alive while the pool exists)
    BasicArray<RefPointer<Operation> > m_operations;  // Win +0x20 / Mac +0x7c not started yet
    bool                               m_quit;        // Win +0x30 / Mac +0x8c
    BasicArray<OperationThread*>       m_threads;     // Win +0x34 / Mac +0x90
};

} // namespace LEGO
