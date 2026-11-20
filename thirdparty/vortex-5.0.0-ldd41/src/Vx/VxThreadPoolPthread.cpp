// Vortex 5.0.0 build 800 "vortexldd41" - VxThreadPoolPthread (restored).
// Mac TU 530 0x486da2-0x487bee (file guess Vx/VxThreadPoolPthread.cpp). The std::deque code of
// the TU (Mac 0x4876fe-0x487a8c) is generated from <deque>. Compiled for the Mac branch only.
#if !defined(_WIN32)

#include "Vx/VxThreadPool.h"
#include <string.h>

namespace Vx {

// Mac 0x486fa8 (Win: src/Vx/VxThreadPoolWin.cpp)
VxThreadPool* VxThreadPool::create(int threadCount, const char* name)
{
    VxThreadPoolPthread* pool = new VxThreadPoolPthread;
    if (name)
        pool->setName(name);
    pool->init(threadCount);
    return pool;
}

// (inlined in create, Mac 0x486fa8: only the name and the argument array are initialised;
// the rest is zeroed here)
VxThreadPoolPthread::VxThreadPoolPthread()
    : mThreads(0)
    , mThreadArgs(0)
    , mQueue(0)
    , mThreadCount(0)
    , mPending(0)
    , mD4(0)
    , mShutdown(0)
{
}

// D2 Mac 0x487b88, D0 0x487b14: only the name. NOTE: unlike the Win pool, the destructor does
// not shut the threads down (the owner calls shutdown()).
VxThreadPoolPthread::~VxThreadPoolPthread()
{
}

// Mac 0x486da2
bool VxThreadPoolPthread::addWorkUnits(WorkUnit** units, int count)
{
    for (int i = 0; i < count; ++i)
        if (!addWorkUnit(units[i]))
            return false;
    return true;
}

// Mac 0x486dea (queue mutex held)
VxThreadPool::WorkUnit* VxThreadPoolPthread::popWorkUnit()
{
    if (mQueue->empty())
        return 0;
    WorkUnit* u = mQueue->front();
    mQueue->pop_front();
    return u;
}

// Mac 0x486e2c
void VxThreadPoolPthread::waitForCompletion()
{
    pthread_mutex_lock(&mPendingMutex);
    while (mPending > 0)
        pthread_cond_wait(&mDoneCond, &mPendingMutex);
    pthread_mutex_unlock(&mPendingMutex);
}

// Mac 0x486e7a: worker thread
void* VxThreadPoolPthread::threadMain(void* p)
{
    ThreadArg* arg = (ThreadArg*)p;
    VxThreadPoolPthread* pool = arg->pool;
    if (pthread_mutex_lock(&pool->mQueueMutex) != 0)
        return 0;
    for (;;) {
        while (!pool->mQueue->empty() || pool->mShutdown != 0) {
            WorkUnit* u = pool->popWorkUnit();
            if (u == 0) {
                if (pool->mShutdown != 0) {
                    if (--pool->mThreadCount == 0)
                        pthread_cond_broadcast(&pool->mQueueCond);
                    pthread_mutex_unlock(&pool->mQueueMutex);
                    return 0;
                }
            } else {
                if (pthread_mutex_unlock(&pool->mQueueMutex) != 0)
                    return 0;
                u->execute((int)arg->index);
                pthread_mutex_lock(&pool->mPendingMutex);
                --pool->mPending;
                pthread_cond_signal(&pool->mDoneCond);
                pthread_mutex_unlock(&pool->mPendingMutex);
                if (pthread_mutex_lock(&pool->mQueueMutex) != 0)
                    return 0;
            }
        }
        if (pthread_cond_wait(&pool->mQueueCond, &pool->mQueueMutex) != 0)
            break;
    }
    pthread_mutex_unlock(&pool->mQueueMutex);
    return 0;
}

// Mac 0x487016
bool VxThreadPoolPthread::addWorkUnit(WorkUnit* unit)
{
    pthread_mutex_lock(&mQueueMutex);
    mQueue->push_back(unit);
    pthread_mutex_lock(&mPendingMutex);
    ++mPending;
    pthread_mutex_unlock(&mPendingMutex);
    int r = pthread_cond_signal(&mQueueCond);
    pthread_mutex_unlock(&mQueueMutex);
    return r == 0;
}

// Mac 0x4870b6.
// NOTE (guarded UB): the original sets the shutdown flag, wakes the workers only when units
// are pending, waits on the done condition without holding its mutex and then destroys the
// mutexes / conditions while the detached workers may still wait on them (glibc's
// pthread_cond_destroy blocks on waiters). Here every worker is woken, the remaining queue is
// worked off as the original workers do, and the objects are destroyed only after the last
// worker has left (it broadcasts the queue condition, as in the original).
void VxThreadPoolPthread::shutdown()
{
    if (mQueue == 0)
        return;
    if (pthread_mutex_lock(&mQueueMutex) != 0)
        return;
    mShutdown = 1;
    pthread_cond_broadcast(&mQueueCond);
    while (mThreadCount > 0)
        pthread_cond_wait(&mQueueCond, &mQueueMutex);
    pthread_mutex_unlock(&mQueueMutex);
    // units nobody ran (none once the workers have drained the queue)
    while (!mQueue->empty()) {
        mQueue->front()->cancel();
        mQueue->pop_front();
    }
    delete mQueue;
    mQueue = 0;
    pthread_mutex_destroy(&mQueueMutex);
    pthread_cond_destroy(&mQueueCond);
    pthread_attr_destroy(&mAttr);
    pthread_mutex_destroy(&mPendingMutex);
    pthread_cond_destroy(&mDoneCond);
    delete[] mThreads;
    mThreads = 0;
    delete[] mThreadArgs;
    mThreadArgs = 0;
}

// Mac 0x48724e.
// NOTE (guarded): the original counts every thread it asked for; a thread that could not be
// created is not counted here (shutdown waits for the count to reach 0).
bool VxThreadPoolPthread::init(int threadCount)
{
    delete[] mThreadArgs;
    mThreadArgs = new ThreadArg[threadCount];
    mQueue = new std::deque<WorkUnit*>;
    mThreadCount = (unsigned int)threadCount;
    mPending = 0;
    mShutdown = 0;
    mThreads = new pthread_t[threadCount];
    if (pthread_attr_init(&mAttr) != 0)
        return false;
    if (pthread_attr_setdetachstate(&mAttr, PTHREAD_CREATE_DETACHED) != 0) {
        pthread_attr_destroy(&mAttr);
        return false;
    }
    if (pthread_mutex_init(&mQueueMutex, 0) != 0) {
        pthread_attr_destroy(&mAttr);
        return false;
    }
    if (pthread_cond_init(&mQueueCond, 0) != 0) {
        pthread_mutex_destroy(&mQueueMutex);
        pthread_attr_destroy(&mAttr);
        return false;
    }
    pthread_cond_init(&mDoneCond, 0);
    pthread_mutex_init(&mPendingMutex, 0);
    unsigned int created = 0;
    pthread_mutex_lock(&mQueueMutex);
    for (unsigned int i = 0; i < (unsigned int)threadCount; ++i) {
        mThreadArgs[i].pool = this;
        mThreadArgs[i].index = i;
        if (pthread_create(&mThreads[i], &mAttr, threadMain, &mThreadArgs[i]) == 0)
            ++created;
    }
    mThreadCount = created;
    pthread_mutex_unlock(&mQueueMutex);
    return true;
}

// Mac 0x487682
bool VxThreadPoolPthread::startQueuedWorkUnits()
{
    return true;
}

// Mac 0x48768c
bool VxThreadPoolPthread::queueWorkUnit(WorkUnit* unit)
{
    return addWorkUnit(unit);
}

// Mac 0x4876aa
void VxThreadPoolPthread::setUseAffinity(bool)
{
}

// Mac 0x4876b0
bool VxThreadPoolPthread::getUseAffinity() const
{
    return false;
}

// Mac 0x4876ba
const char* VxThreadPoolPthread::getName() const
{
    return mName.c_str();
}

// Mac 0x4876c6
void VxThreadPoolPthread::setName(const char* name)
{
    mName.assign(name, strlen(name));
}

} // namespace Vx

#endif
