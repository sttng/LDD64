// Vortex 5.0.0 build 800 "vortexldd41" - VxThreadPoolWin (restored; a Win-only
// module, restored here because the ball game reaches it: MdtKeaStep's threaded path).
// Win 0x7534e0-0x754370. Compiled for the Win branch only.
#if defined(_WIN32)

#include "Vx/VxThreadPool.h"
#include <process.h>
#include <stdio.h>

// Win: the CRT's _snprintf (VS2008's CRT has no C99 snprintf; MinGW has both), as Me/MeMessage.c.
#if defined(_WIN32)
#define snprintf _snprintf
#endif

namespace Vx {

// Win 0x753740 (Mac: src/Vx/VxThreadPoolPthread.cpp)
VxThreadPool* VxThreadPool::create(int threadCount, const char* name)
{
    VxThreadPoolWin* pool = new VxThreadPoolWin;
    if (name)
        pool->setName(name);
    pool->init(threadCount);
    return pool;
}

// (inlined in create, Win 0x753740)
VxThreadPoolWin::VxThreadPoolWin()
    : mThreadArgs(0)
    , mThreads(0)
    , mThreadCount(0)
    , mShutdownEvent(0)
    , mWorkSemaphore(0)
    , mQueue(0)
    , mDoneEvent(0)
    , mDoneMutex(0)
    , mPending(0)
    , mUseAffinity(true)
{
}

// Win 0x7538d0 (deleting 0x754080)
VxThreadPoolWin::~VxThreadPoolWin()
{
    shutdown();
}

// Win 0x753520 (name inferred): names a thread for an attached debugger (exception 0x406d1388).
// NOTE (guarded): the original raises it inside __try/__except; without a debugger the
// exception would be unhandled here (no SEH in this compiler), so it is raised only when a
// debugger is present.
static void setThreadName(DWORD threadId, const char* name)
{
    if (!IsDebuggerPresent())
        return;
    struct {
        DWORD type;
        LPCSTR name;
        DWORD threadId;
        DWORD flags;
    } info = { 0x1000, name, threadId, 0 };
    RaiseException(0x406d1388, 0, sizeof(info) / sizeof(ULONG_PTR), (const ULONG_PTR*)&info);
}

// Win 0x7535d0
void VxThreadPoolWin::setUseAffinity(bool use)
{
    if (mUseAffinity == use)
        return;
    unsigned int cpus = (unsigned int)VxGetNumberOfProcessors();
    for (unsigned int i = 0; i < mThreadCount; ++i)
        SetThreadAffinityMask(mThreads[i], use ? (DWORD_PTR)1 << (i % cpus) : 0);
    mUseAffinity = use;
}

// Win 0x753640
bool VxThreadPoolWin::addWorkUnits(WorkUnit** units, int count)
{
    for (int i = 0; i < count; ++i)
        if (!queueWorkUnit(units[i]))
            return false;
    return startQueuedWorkUnits();
}

// Win 0x753690
void VxThreadPoolWin::waitForCompletion()
{
    WaitForSingleObject(mDoneMutex, INFINITE);
    while (mPending > 0) {
        SignalObjectAndWait(mDoneMutex, mDoneEvent, INFINITE, FALSE);
        WaitForSingleObject(mDoneMutex, INFINITE);
    }
    ReleaseMutex(mDoneMutex);
}

// Win 0x753700
bool VxThreadPoolWin::startQueuedWorkUnits()
{
    EnterCriticalSection(&mLock);
    mPending = (int)mQueue->size();
    LeaveCriticalSection(&mLock);
    return ReleaseSemaphore(mWorkSemaphore, mPending, 0) != 0;
}

// Win 0x7537b0
void VxThreadPoolWin::shutdown()
{
    if (mQueue == 0 || !SetEvent(mShutdownEvent))
        return;
    WaitForMultipleObjects(mThreadCount, mThreads, TRUE, INFINITE);
    while (!mQueue->empty()) {
        mQueue->front()->cancel();
        mQueue->pop_front();
    }
    delete mQueue;
    mQueue = 0;
    CloseHandle(mWorkSemaphore);
    CloseHandle(mShutdownEvent);
    DeleteCriticalSection(&mLock);
    CloseHandle(mDoneEvent);
    CloseHandle(mDoneMutex);
    for (unsigned int i = 0; i < mThreadCount; ++i)
        CloseHandle(mThreads[i]);
    delete[] mThreads;
    mThreads = 0;
    delete[] mThreadArgs;
    mThreadArgs = 0;
}

// Win 0x753950
VxThreadPool::WorkUnit* VxThreadPoolWin::popWorkUnit()
{
    EnterCriticalSection(&mLock);
    if (mQueue->empty()) {
        LeaveCriticalSection(&mLock);
        return 0;
    }
    WorkUnit* u = mQueue->front();
    mQueue->pop_front();
    LeaveCriticalSection(&mLock);
    return u;
}

// Win 0x7539f0: worker thread (shutdown event or one semaphore count per unit)
static unsigned __stdcall threadMain(void* p)
{
    VxThreadPoolWin::ThreadArg* arg = (VxThreadPoolWin::ThreadArg*)p;
    VxThreadPoolWin* pool = arg->pool;
    HANDLE handles[2] = { pool->mShutdownEvent, pool->mWorkSemaphore };
    for (;;) {
        DWORD r = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        if (r != WAIT_OBJECT_0 + 1)
            break;
        VxThreadPool::WorkUnit* u = pool->popWorkUnit();
        if (u != 0) {
            u->execute((int)arg->index);
            EnterCriticalSection(&pool->mLock);
            --pool->mPending;
            LeaveCriticalSection(&pool->mLock);
            SetEvent(pool->mDoneEvent);
        }
    }
    return 0;
}

// Win 0x753a60
bool VxThreadPoolWin::init(int threadCount)
{
    delete[] mThreadArgs;
    mThreadArgs = new ThreadArg[threadCount];
    mQueue = new std::deque<WorkUnit*>;
    mWorkSemaphore = CreateSemaphoreA(0, 0, 0x7fffffff, 0);
    mShutdownEvent = CreateEventA(0, TRUE, FALSE, 0);
    mDoneEvent = CreateEventA(0, FALSE, FALSE, 0);
    mDoneMutex = CreateMutexA(0, FALSE, 0);
    mPending = 0;
    InitializeCriticalSection(&mLock);
    mThreads = new HANDLE[threadCount];
    mThreadCount = (unsigned int)threadCount;
    unsigned int cpus = (unsigned int)VxGetNumberOfProcessors();
    for (unsigned int i = 0; i < (unsigned int)threadCount; ++i) {
        mThreadArgs[i].pool = this;
        mThreadArgs[i].index = i;
        unsigned int id = 0;
        mThreads[i] = (HANDLE)_beginthreadex(0, 0, threadMain, &mThreadArgs[i], 0, &id);
        if (mThreads[i] == 0) {
            mThreadCount = i;
            return false;
        }
        if (!mName.empty()) {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s%d", mName.c_str(), (int)i);
            setThreadName(id, buf);
        }
        if (mUseAffinity)
            SetThreadAffinityMask(mThreads[i], (DWORD_PTR)1 << (i % cpus));
    }
    return true;
}

// Win 0x753c90
bool VxThreadPoolWin::addWorkUnit(WorkUnit* unit)
{
    EnterCriticalSection(&mLock);
    mQueue->push_back(unit);
    ++mPending;
    LeaveCriticalSection(&mLock);
    return ReleaseSemaphore(mWorkSemaphore, 1, 0) != 0;
}

// Win 0x753cd0: queued without waking a worker (startQueuedWorkUnits releases them all).
// NOTE: like the original, without taking the lock.
bool VxThreadPoolWin::queueWorkUnit(WorkUnit* unit)
{
    mQueue->push_back(unit);
    return true;
}

// Win 0x753e50
bool VxThreadPoolWin::getUseAffinity() const
{
    return mUseAffinity;
}

// Win 0x753e60
const char* VxThreadPoolWin::getName() const
{
    return mName.c_str();
}

// Win 0x753ee0
void VxThreadPoolWin::setName(const char* name)
{
    mName.assign(name);
}

} // namespace Vx

#endif
