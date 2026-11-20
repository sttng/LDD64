// LEGO Digital Designer — restored source
// TU 270 "OperationManager.cpp" (file name inferred), Mac 0x2827e0-0x285631, Win
// 0x539140-0x53a31f. See OperationManager.h. The NSOperation path (OS minor
// >= 5) sits behind __OBJC__; HOST-ONLY builds take the pthread pool (the 10.4 code).
#include "OperationManager.h"
#include "LEGO/String.h"
#include "Trace.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#include <unistd.h>
#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#include "Platform.h"
#endif
#endif

#if defined(__OBJC__) && !defined(_WIN32)
// Mac 0x2827e0-0x282aef. NSOperation subclass wrapping one LEGO::Operation (Mac OS X >= 10.5).
// ivars: LEGO::Operation* +0x0c, BOOL +0x10 (the queue's reference is held). (ivar names inferred)
@interface LEGOMacOperation : NSOperation {
    LEGO::Operation* m_operation;
    BOOL             m_inQueue;
}
- (id)initWithOperation:(LEGO::Operation*)operation;
- (void)addedToQueue;
- (void)removeFromQueue;
@end

@implementation LEGOMacOperation
// Mac 0x2827e0
- (id)initWithOperation:(LEGO::Operation*)operation
{
    self = [super init];
    if (self) {
        m_operation = operation;
        m_inQueue = NO;
    }
    return self;
}
// Mac 0x282830
- (void)dealloc
{
    [super dealloc];
}
// Mac 0x282870: runs Execute() on the NSOperationQueue thread with a LEGO::Thread attached.
- (void)main
{
    m_operation->AddRef();
    LEGO::Operation* operation = m_operation;
    {
        LEGO::Thread thread;
        thread.Attach();
        operation->Execute();
    }
    m_operation->Release();
    [self removeFromQueue];
}
// Mac 0x282940
- (void)cancel
{
    [self removeFromQueue];
    [super cancel];
}
// Mac 0x282990
- (void)addedToQueue
{
    @synchronized(self) {
        m_inQueue = YES;
        m_operation->AddRef();
    }
}
// Mac 0x282a40
- (void)removeFromQueue
{
    @synchronized(self) {
        if (m_inQueue) {
            m_inQueue = NO;
            m_operation->Release();
        }
    }
}
@end
#endif

namespace LEGO {

namespace {

// Platform::GetInstance()->GetOSVersion().m_minor >= 5 at every call (Mac 0xd7f0 + [+4] > 4).
// HOST-ONLY: plain C++ builds of the Mac branch always take the pool path. (name inferred)
inline bool UseCocoaOperations()
{
#if defined(__OBJC__) && !defined(_WIN32)
    return Platform::GetInstance()->GetOSVersion().m_minor > 4;
#else
    return false;
#endif
}

// std::find + `*it = back; erase(end - 1)` (Mac 0x282e10, Win inlined in 0x539d30 / 0x539bf0).
template <class T>
void RemoveUnordered(BasicArray<T*>& array, T* value)
{
    typename BasicArray<T*>::Iterator it = array.Begin();
    while (it != array.End() && *it != value)
        ++it;
    if (it == array.End())
        return;
    typename BasicArray<T*>::Iterator last = array.End() - 1;
    if (last != it)
        *it = *last;
    array.PopBack();
}

// Index of `operation` in a RefPointer array (std::find), GetSize() when absent.
UInt32 FindOperation(const BasicArray<RefPointer<Operation> >& array, const Operation* operation)
{
    UInt32 i = 0;
    while (i < array.GetSize() && array[i].Get() != operation)
        ++i;
    return i;
}

// The process-wide pool: Mac 0x77e758 (static initialiser 0x285590 -> ctor 0x284b10, atexit
// 0x282af0 -> dtor 0x284cb0), Win 0xb44050 (0x539140 / 0x539220).
OperationManager s_operationManager;

} // namespace

#if !defined(_WIN32)
// Access to the Cocoa object of an operation for the objc calls below. (helper, not original)
struct OperationCocoaAccess {
    static void* Get(const Operation* operation) { return operation->m_cocoaOperation; }
};
#endif

#if defined(__OBJC__) && !defined(_WIN32)
#define LDD_COCOA_OPERATION(operation) ((LEGOMacOperation*)OperationCocoaAccess::Get(operation))
#endif

// ===========================================================================================
// Operation
// ===========================================================================================

// Mac 0x282b00 (C2; the C1 is emitted in each derived class), Win 0x539a70
Operation::Operation()
    : m_queue(0),
      m_cancelled(false),
      m_executing(false),
      m_finished(false)
{
#if !defined(_WIN32)
    m_cocoaOperation = 0;
#if defined(__OBJC__)
    if (UseCocoaOperations())
        m_cocoaOperation = [[LEGOMacOperation alloc] initWithOperation:this];
#endif
#endif
}

// Mac 0x283010 (D1), 0x2830d0 (D0), thunks 0x283070 / 0x283140, body 0x282c20;
// Win 0x539bf0 (body), 0x539b30 (scalar deleting)
Operation::~Operation()
{
    if (!UseCocoaOperations()) {
        OperationManager& manager = OperationManager::GetInstance();
        manager.Lock();
        ReleaseDependents();
        for (BasicArray<Operation*>::Iterator it = m_dependencies.Begin(); it != m_dependencies.End(); ++it)
            RemoveUnordered((*it)->m_dependents, this);
        m_dependencies.Clear();
        bool signal = true;          // unlock, then wake everybody
        manager.Unlock(signal);
    }
#if defined(__OBJC__) && !defined(_WIN32)
    else {
        [LDD_COCOA_OPERATION(this) release];
    }
#endif
}

// Win 0x539d30 (EBX = this); Mac inlined (loops over 0x282e10 in 0x282c20, 0x283f60, 0x285060)
void Operation::ReleaseDependents()
{
    if (m_dependents.IsEmpty())
        return;
    for (BasicArray<Operation*>::Iterator it = m_dependents.Begin(); it != m_dependents.End(); ++it)
        RemoveUnordered((*it)->m_dependencies, this);
    m_dependents.Clear();
}

// Mac 0x2831c0, Win 0x539b60
void Operation::AddDependency(Operation* operation)
{
#if defined(__OBJC__) && !defined(_WIN32)
    if (UseCocoaOperations()) {
        [LDD_COCOA_OPERATION(this) addDependency:LDD_COCOA_OPERATION(operation)];
        return;
    }
#endif
    OperationManager& manager = OperationManager::GetInstance();
    manager.Lock();
    m_dependencies.PushBack(operation);
    operation->m_dependents.PushBack(this);
    bool signal = false;             // plain unlock
    manager.Unlock(signal);
}

// Mac 0x283630, Win 0x539ba0
void Operation::Cancel()
{
#if defined(__OBJC__) && !defined(_WIN32)
    if (UseCocoaOperations()) {
        [LDD_COCOA_OPERATION(this) cancel];
        return;
    }
#endif
    // NOTE: set without the lock (both builds).
    m_cancelled = true;
    OperationManager::GetInstance().SignalAll();
}

// Mac 0x2836f0, Win 0x539be0
bool Operation::IsCancelled() const
{
#if defined(__OBJC__) && !defined(_WIN32)
    if (UseCocoaOperations())
        return [LDD_COCOA_OPERATION(this) isCancelled] != NO;
#endif
    return m_cancelled;
}

// inlined in OperationQueue::Wait (Mac 0x283f60: -isExecuting on 10.5+, else +0x29)
bool Operation::IsExecuting() const
{
#if defined(__OBJC__) && !defined(_WIN32)
    if (UseCocoaOperations())
        return [LDD_COCOA_OPERATION(this) isExecuting] != NO;
#endif
    return m_executing;
}

// ===========================================================================================
// OperationQueue
// ===========================================================================================

// Mac 0x283730 (C2) / 0x283e30 (C1), Win 0x539da0
OperationQueue::OperationQueue()
{
#if !defined(_WIN32)
    m_cocoaQueue = 0;
#if defined(__OBJC__)
    if (UseCocoaOperations()) {
        m_cocoaQueue = [[NSOperationQueue alloc] init];
        return;
    }
#endif
#endif
    OperationManager::GetInstance().StartThreads();
}

// Mac 0x283e40 (body), 0x284300 (D1), 0x284310 (D0); Win 0x539e40, 0x539e20 (scalar deleting)
OperationQueue::~OperationQueue()
{
#if defined(__OBJC__) && !defined(_WIN32)
    if (UseCocoaOperations()) {
        [(NSOperationQueue*)m_cocoaQueue release];
        return;
    }
#endif
    Wait();
}

// Mac 0x284350, Win 0x539eb0
void OperationQueue::Add(Operation* operation)
{
#if defined(__OBJC__) && !defined(_WIN32)
    if (UseCocoaOperations()) {
        // NOTE: on this path the operation is neither in m_operations nor gets m_queue.
        [LDD_COCOA_OPERATION(operation) addedToQueue];
        [(NSOperationQueue*)m_cocoaQueue addOperation:LDD_COCOA_OPERATION(operation)];
        return;
    }
#endif
    OperationManager& manager = OperationManager::GetInstance();
    manager.Lock();
    operation->m_queue = this;
    m_operations.PushBack(RefPointer<Operation>(operation));
    manager.AddOperation(operation);
    bool signal = true;
    manager.Unlock(signal);
}

// Mac 0x284660, Win 0x539f90
void OperationQueue::CancelAll()
{
#if defined(__OBJC__) && !defined(_WIN32)
    if (UseCocoaOperations()) {
        [(NSOperationQueue*)m_cocoaQueue cancelAllOperations];
        return;
    }
#endif
    OperationManager& manager = OperationManager::GetInstance();
    manager.Lock();
    for (UInt32 i = 0; i < m_operations.GetSize(); ++i)
        m_operations[i]->m_cancelled = true;
    bool signal = true;
    manager.Unlock(signal);
}

// Mac 0x283f60, Win 0x53a000
void OperationQueue::Wait()
{
#if defined(__OBJC__) && !defined(_WIN32)
    if (UseCocoaOperations()) {
        [(NSOperationQueue*)m_cocoaQueue waitUntilAllOperationsAreFinished];
        return;
    }
#endif
    OperationManager& manager = OperationManager::GetInstance();
    manager.Lock();
    bool signal = false;
    for (;;) {
        bool progressed = false;
        for (UInt32 i = 0; i < m_operations.GetSize();) {
            Operation* operation = m_operations[i];
            if (!operation->IsExecuting()) {
                if (operation->IsCancelled()) {
                    // dropped without running
                    operation->ReleaseDependents();
                    manager.RemoveOperation(operation);
                    m_operations.Erase(i);
                    progressed = true;
                    signal = true;
                    continue;
                }
                if (operation->m_dependencies.IsEmpty()) {
                    // run it here, unlocked
                    operation->m_executing = true;
                    manager.RemoveOperation(operation);
                    manager.Unlock(signal);
                    // NOTE: deliberate deviation (improvement) on Win: the Mac original catches an exception
                    // from Execute here, the Win original (no try block, Win 0x53a000 / 0x5396f0) lets it escape;
                    // both builds catch it, as the Mac one.
                    try {
                        operation->Execute();
                    } catch (...) {
                    }
                    manager.Lock();
                    operation->m_executing = false;
                    operation->m_finished = true;
                    operation->ReleaseDependents();
                    {
                        RefPointer<Operation> keep(operation);
                        UInt32 index = FindOperation(m_operations, operation);
                        if (index < m_operations.GetSize())
                            m_operations.Erase(index);
                    }
                    progressed = true;
                    signal = true;
                    break;
                }
            }
            ++i;
        }
        if (m_operations.IsEmpty()) {
            manager.Unlock(signal);
            return;
        }
        if (!progressed)
            manager.Wait(signal);
    }
}

// ===========================================================================================
// OperationManager::OperationThread
// ===========================================================================================

// Win 0x539970; Mac inlined in 0x283840 (operator new 0xc0, Thread::Thread, cond/mutex init)
OperationManager::OperationThread::OperationThread(OperationManager* manager)
    : m_manager(manager)
{
}

// Mac 0x284f60 (D1), 0x284fc0 (D0); Win 0x5399e0, 0x539a50 (scalar deleting)
OperationManager::OperationThread::~OperationThread()
{
}

// Mac 0x285040, Win 0x539a40
bool OperationManager::OperationThread::Run()
{
    return m_manager->RunOperations(m_wakeUp);
}

// ===========================================================================================
// OperationManager
// ===========================================================================================

// Mac 0x284b10, Win 0x539140
OperationManager::OperationManager()
    : m_threadHook(ThreadHook::Get()),
      m_quit(false)
{
#if defined(_WIN32)
    InitializeCriticalSection(&m_lock);
#else
    pthread_mutexattr_t attributes;
    pthread_mutexattr_init(&attributes);
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_lock, &attributes);
    pthread_mutexattr_destroy(&attributes);
#endif
}

// Mac 0x284cb0, Win 0x539220
OperationManager::~OperationManager()
{
    m_quit = true;
    for (;;) {
        SignalAll();
        bool running = false;
        for (UInt32 i = 0; i < m_threads.GetSize(); ++i) {
            if (m_threads[i]->IsRunning()) {
                running = true;
                break;
            }
        }
        if (!running)
            break;
#if defined(_WIN32)
        Sleep(0);
#else
        usleep(1);
#endif
    }
    for (UInt32 i = 0; i < m_threads.GetSize(); ++i)
        delete m_threads[i];
#if defined(_WIN32)
    // (member destructors: thread array, operations, ThreadHook, CloseHandle(event))
    DeleteCriticalSection(&m_lock);
#else
    pthread_mutex_destroy(&m_lock);
#endif
}

OperationManager& OperationManager::GetInstance()
{
    return s_operationManager;
}

void OperationManager::Lock()
{
#if defined(_WIN32)
    EnterCriticalSection(&m_lock);
#else
    pthread_mutex_lock(&m_lock);
#endif
}

// Mac 0x284930, Win 0x5394a0 (EDI = this, EAX = &signal)
void OperationManager::Unlock(bool& signal)
{
#if defined(_WIN32)
    LeaveCriticalSection(&m_lock);
#else
    pthread_mutex_unlock(&m_lock);
#endif
    if (signal) {
        signal = false;
        SignalAll();
    }
}

void OperationManager::SignalAll()
{
    for (UInt32 i = 0; i < m_threads.GetSize(); ++i)
        m_threads[i]->WakeUp();
    m_event.Signal();
}

// Mac 0x2849d0, Win 0x5394e0
void OperationManager::Wait(bool& signal)
{
    Unlock(signal);
    while (!m_event.Wait(1000))
        Trace(String("OperationManager::Wait locked."), true);
    Lock();
}

// Mac 0x283840 (two threads, unrolled), Win 0x5393e0 (GetSystemInfo)
void OperationManager::StartThreads()
{
    if (!m_threads.IsEmpty())
        return;
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    Int32 count = (Int32)info.dwNumberOfProcessors - 1;
    if (count < 1)
        count = 1;
#else
    const Int32 count = 2;
#endif
    for (Int32 i = 0; i < count; ++i) {
        OperationThread* thread = new OperationThread(this);
        m_threads.PushBack(thread);
        thread->Start(0);
    }
}

// Win 0x5395b0; Mac inlined in 0x284350 (0x284520 = PushBack)
void OperationManager::AddOperation(Operation* operation)
{
    m_operations.PushBack(RefPointer<Operation>(operation));
}

// Mac 0x284750 (+ erase 0x284850), Win 0x539640
void OperationManager::RemoveOperation(Operation* operation)
{
    RefPointer<Operation> keep(operation);
    UInt32 index = FindOperation(m_operations, operation);
    if (index < m_operations.GetSize())
        m_operations.Erase(index);
}

// Mac 0x285060 (manager, &thread->m_wakeUp), Win 0x5396f0 (ECX = manager)
bool OperationManager::RunOperations(Event& wakeUp)
{
    for (;;) {
        wakeUp.Wait();
        if (m_quit)
            return true;
        Lock();
        bool signal = false;
        for (;;) {
            bool again = false;
            UInt32 i = 0;
            while (i < m_operations.GetSize()) {
                Operation* operation = m_operations[i];
                if (operation->IsCancelled()) {
                    // dropped: out of the pool and out of its queue
                    operation->ReleaseDependents();
                    m_operations.Erase(i);
                    OperationQueue* queue = operation->m_queue;
                    {
                        RefPointer<Operation> keep(operation);
                        UInt32 index = FindOperation(queue->m_operations, operation);
                        if (index < queue->m_operations.GetSize())
                            queue->m_operations.Erase(index);
                    }
                    signal = true;
                    again = true;
                    continue;
                }
                if (operation->m_dependencies.IsEmpty()) {
                    operation->m_executing = true;
                    m_operations.Erase(i);          // the queue still holds a reference
                    Unlock(signal);
                    // NOTE: deliberate deviation (improvement) on Win: the Mac original catches an exception
                    // from Execute here, the Win original (no try block, Win 0x53a000 / 0x5396f0) lets it escape;
                    // both builds catch it, as the Mac one.
                    try {
                        operation->Execute();
                    } catch (...) {
                    }
                    Lock();
                    operation->m_executing = false;
                    operation->m_finished = true;
                    operation->ReleaseDependents();
                    OperationQueue* queue = operation->m_queue;
                    {
                        RefPointer<Operation> keep(operation);
                        UInt32 index = FindOperation(queue->m_operations, operation);
                        if (index < queue->m_operations.GetSize())
                            queue->m_operations.Erase(index);
                    }
                    signal = true;
                    again = true;
                    break;                           // rescan from the start
                }
                ++i;
            }
            if (!again || m_operations.IsEmpty())
                break;
        }
        Unlock(signal);
#if defined(_WIN32)
        // NOTE: Win only (0x5396f0 inlines Thread::ReleaseObjects); the Mac loop keeps them.
        Thread::ReleaseObjects();
#endif
    }
}

} // namespace LEGO
