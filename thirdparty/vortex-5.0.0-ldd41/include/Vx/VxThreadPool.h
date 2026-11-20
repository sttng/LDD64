// Vortex 5.0.0 build 800 "vortexldd41" - thread pool (restored).
//
// VxThreadPool is the abstract interface (Win vtable 0x9fa8a0, 12 slots, all pure but the
// dtor); VxFrame makes one lazily (VxFrame +0x100 Mac / +0x118 Win, Mac 0x4539b4:
// VxThreadPool::create(mThreadCount, "VxFrame ThreadPool")) and Kea's threaded step
// (MdtKeaStep, V2.3) queues one work unit per partition and waits.
//  * Mac: VxThreadPoolPthread (TU 530 0x486da2-0x487bee; detached pthreads, a std::deque queue)
//  * Win: VxThreadPoolWin (Win-only, 0x7534e0-0x754370, reached by the ball game: a semaphore,
//    a shutdown event, a critical section and _beginthreadex threads)
// Each platform build compiles its own implementation (src/Vx/VxThreadPoolPthread.cpp for the
// Mac/host branch, src/Vx/VxThreadPoolWin.cpp for Win). Method names are inferred.
//
// Slots (Mac = Itanium: D1, D0 first; Win: one scalar deleting dtor):
//   Mac 2  / Win 1   init(threadCount)          Mac 8  / Win 7   shutdown()
//   Mac 3  / Win 2   addWorkUnits(units, n)     Mac 9  / Win 8   getName()
//   Mac 4  / Win 3   addWorkUnit(unit)          Mac 10 / Win 9   setName(name)
//   Mac 5  / Win 4   waitForCompletion()        Mac 11 / Win 10  getUseAffinity()
//   Mac 6  / Win 5   queueWorkUnit(unit)        Mac 12 / Win 11  setUseAffinity(bool)
//   Mac 7  / Win 6   startQueuedWorkUnits()
#ifndef VX_VXTHREADPOOL_H
#define VX_VXTHREADPOOL_H

#include <string>
#include <deque>
#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace Vx {

// Mac 0x47f5c6, Win 0x75d090 (src/Vx/VxTimer.cpp) (name inferred)
int VxGetNumberOfProcessors();

class VxThreadPool {
public:
    // VxThreadPool::WorkUnit (Win vtable 0x9fb8dc: dtor 0x75f9f0, pure, 0x75e170, pure).
    // MSVC places the overloads in reverse order: Win slot 1 execute(), slot 2 execute(int);
    // Mac slot 2 execute(int) (the Mac worker calls +8 with the thread index), slot 3 execute().
    class WorkUnit {
    public:
        virtual ~WorkUnit() {}
        // Win 0x75e170: runs the unit on worker `threadIndex`; the default ignores the index
        virtual void execute(int threadIndex)
        {
            (void)threadIndex;
            execute();
        }
        virtual void execute() = 0;
        // called for units still queued when the pool shuts down (Mac slot 4, Win slot 3)
        virtual void cancel() = 0;
    };

    // Mac 0x486fa8, Win 0x753740: the platform pool (new), setName(name) if name, init(n)
    static VxThreadPool* create(int threadCount, const char* name);

    virtual ~VxThreadPool() {}                                // Win 0x7534f0
    virtual bool init(int threadCount) = 0;
    virtual bool addWorkUnits(WorkUnit** units, int count) = 0;
    virtual bool addWorkUnit(WorkUnit* unit) = 0;
    virtual void waitForCompletion() = 0;
    virtual bool queueWorkUnit(WorkUnit* unit) = 0;
    virtual bool startQueuedWorkUnits() = 0;
    virtual void shutdown() = 0;
    virtual const char* getName() const = 0;
    virtual void setName(const char* name) = 0;
    virtual bool getUseAffinity() const = 0;
    virtual void setUseAffinity(bool use) = 0;
};

#if defined(_WIN32)

// Win vtable 0x9fa8d4, 0x60 bytes
class VxThreadPoolWin : public VxThreadPool {
public:
    struct ThreadArg { // (name inferred)
        VxThreadPoolWin* pool;
        unsigned int index;
    };
    VxThreadPoolWin();                                         // (inlined in create, Win 0x753740)
    virtual ~VxThreadPoolWin();                                // Win 0x754080 / 0x7538d0 (calls shutdown)
    virtual bool init(int threadCount);                        // Win 0x753a60
    virtual bool addWorkUnits(WorkUnit** units, int count);    // Win 0x753640
    virtual bool addWorkUnit(WorkUnit* unit);                  // Win 0x753c90
    virtual void waitForCompletion();                          // Win 0x753690
    virtual bool queueWorkUnit(WorkUnit* unit);                // Win 0x753cd0
    virtual bool startQueuedWorkUnits();                       // Win 0x753700
    virtual void shutdown();                                   // Win 0x7537b0
    virtual const char* getName() const;                       // Win 0x753e60
    virtual void setName(const char* name);                    // Win 0x753ee0
    virtual bool getUseAffinity() const;                       // Win 0x753e50
    virtual void setUseAffinity(bool use);                     // Win 0x7535d0

    WorkUnit* popWorkUnit();                                   // Win 0x753950 (name inferred)

    ThreadArg* mThreadArgs;        // +0x04
    std::string mName;             // +0x08
    HANDLE* mThreads;              // +0x24
    unsigned int mThreadCount;     // +0x28
    HANDLE mShutdownEvent;         // +0x2c manual reset
    HANDLE mWorkSemaphore;         // +0x30 one count per queued unit
    CRITICAL_SECTION mLock;        // +0x34 queue + pending count
    std::deque<WorkUnit*>* mQueue; // +0x4c
    HANDLE mDoneEvent;             // +0x50 auto reset, set after each unit
    HANDLE mDoneMutex;             // +0x54
    int mPending;                  // +0x58 queued + running units
    bool mUseAffinity;             // +0x5c (= true): thread i runs on CPU i % count
};

#else

// Mac vtable 0x75ee48, 0xdc bytes on Mac
class VxThreadPoolPthread : public VxThreadPool {
public:
    struct ThreadArg { // (name inferred)
        VxThreadPoolPthread* pool;
        unsigned int index;
    };
    VxThreadPoolPthread();                                     // (inlined in create, Mac 0x486fa8)
    virtual ~VxThreadPoolPthread();                            // Mac D1 0x487b88 / D0 0x487b14
    virtual bool init(int threadCount);                        // Mac 0x48724e
    virtual bool addWorkUnits(WorkUnit** units, int count);    // Mac 0x486da2
    virtual bool addWorkUnit(WorkUnit* unit);                  // Mac 0x487016
    virtual void waitForCompletion();                          // Mac 0x486e2c
    virtual bool queueWorkUnit(WorkUnit* unit);                // Mac 0x48768c (= addWorkUnit)
    virtual bool startQueuedWorkUnits();                       // Mac 0x487682 (true)
    virtual void shutdown();                                   // Mac 0x4870b6
    virtual const char* getName() const;                       // Mac 0x4876ba
    virtual void setName(const char* name);                    // Mac 0x4876c6
    virtual bool getUseAffinity() const;                       // Mac 0x4876b0 (false)
    virtual void setUseAffinity(bool use);                     // Mac 0x4876aa (nothing)

    WorkUnit* popWorkUnit();                                   // Mac 0x486dea (name inferred)
    static void* threadMain(void* arg);                        // Mac 0x486e7a (name inferred)

    std::string mName;              // +0x04
    pthread_t* mThreads;            // +0x08
    ThreadArg* mThreadArgs;         // +0x0c
    pthread_attr_t mAttr;           // +0x10
    pthread_mutex_t mQueueMutex;    // +0x38
    pthread_cond_t mQueueCond;      // +0x64 work queued / shutdown / last thread gone
    std::deque<WorkUnit*>* mQueue;  // +0x80
    pthread_cond_t mDoneCond;       // +0x84 a unit finished
    pthread_mutex_t mPendingMutex;  // +0xa0
    unsigned int mThreadCount;      // +0xcc running worker threads
    int mPending;                   // +0xd0 queued + running units
    int mD4;                        // +0xd4
    int mShutdown;                  // +0xd8
};

#endif

} // namespace Vx

#endif
