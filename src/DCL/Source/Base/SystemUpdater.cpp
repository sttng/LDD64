// LEGO Digital Designer — restored source
// TU "SystemUpdater.cpp": Updater, MainUpdater, SystemUpdater, RunInMainThreadObserver and the
// 'UPDR' / 'updr' instantiators. Mac 0x1ddd0-0x2046b, Win 0x424060-0x424ab0 (+ Updater::Update
// 0x40bd00, RemoveObserver 0x501150). Header templates emitted here: RemoveObserver Mac 0x1f4f0,
// PushBack 0x1dfb0 / Reserve 0x1fb20 (+ dtors, thunks). Static init Mac 0x203f0.
#include "SystemUpdater.h"
#include "Thread.h"
#include "ObjectRegistry.h"
#include <atomic>

namespace LEGO {

// ---------------------------------------------------------------------------
// MainUpdater
// ---------------------------------------------------------------------------

// Mac 0x1ddd0, Win 0x424400
void MainUpdater::ReleaseLater(Object* object)
{
    Thread::AddObject(object);
}

// Mac 0x1e0f0, Win 0x424670
RefPointer<Updater> MainUpdater::GetInstance()
{
    MainUpdaterInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRegistryId, instantiator);
    return RefPointer<Updater>(dynamic_cast<Updater*>(object.Get()));
}

// Mac 0x1f2f0, Win 0x424060 (ReleaseObjects inlined)
void MainUpdater::Update()
{
    Updater::Update();
    Thread::ReleaseObjects();
}

// Mac 0x1e800, Win 0x424550
RefPointer<Object> MainUpdaterInstantiator::CreateObject() const
{
    return RefPointer<Object>(new MainUpdater);
}

// ---------------------------------------------------------------------------
// Updater
// ---------------------------------------------------------------------------

// Mac 0x1e9f0, Win 0x40bd00
void Updater::Update()
{
    Iterator it(*this);
    while (UpdateObserver* observer = it.Next(m_observers))
        observer->Update();
}

// ---------------------------------------------------------------------------
// SystemUpdater
// ---------------------------------------------------------------------------

// Mac 0x1ded0, Win 0x4248d0
RefPointer<Updater> SystemUpdater::GetInstance()
{
    SystemUpdaterInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRegistryId, instantiator);
    return RefPointer<Updater>(dynamic_cast<Updater*>(object.Get()));
}

namespace {
std::atomic<void (*)()> s_wake(0);   // SetWake
} // namespace

void SystemUpdater::SetWake(void (*wake)())
{
    s_wake.store(wake);
}

void SystemUpdater::RunQueued()
{
    RefPointer<Updater> updater = GetInstance();
    SystemUpdater* system = static_cast<SystemUpdater*>(updater.Get());
    if (system == 0)
        return;
    system->m_queueLock.Lock();
    while (!system->m_queue.IsEmpty()) {
        RefPointer<UpdateObserver> observer = system->m_queue[0];
        system->m_queue.Erase(0);
        if (observer->GetSubject() != system)
            observer->Attach(system);
        system->m_queueLock.Unlock();
        observer->Update();
        system->m_queueLock.Lock();
    }
    system->m_queueLock.Unlock();
}

// Mac 0x1dde0, Win 0x424410
void SystemUpdater::Enqueue(UpdateObserver* observer)
{
    RefPointer<Updater> updater = GetInstance();
    SystemUpdater* system = static_cast<SystemUpdater*>(updater.Get());
    system->m_queueLock.Lock();
    const bool wasEmpty = system->m_queue.IsEmpty();
    system->m_queue.PushBack(RefPointer<UpdateObserver>(observer));
    system->m_queueLock.Unlock();
    // a non-empty queue is drained with this observer by the wake-up (or pass) already due
    if (wasEmpty) {
        if (void (*wake)() = s_wake.load())
            wake();
    }
}

// Mac 0x1f950, Win 0x424120
SystemUpdater::SystemUpdater()
{
    m_queue.Reserve(8);                                               // Mac 0x1fb20, Win 0x424a50
}

// Mac 0x1fc90 / 0x1fcf0 (-> D2 0x202b0), Win 0x424330
SystemUpdater::~SystemUpdater()
{
}

// Mac 0x1fd60, Win 0x4241f0
void SystemUpdater::Update()
{
    Updater::Update();
    m_queueLock.Lock();
    while (!m_queue.IsEmpty()) {
        RefPointer<UpdateObserver> observer = m_queue[0];
        m_queue.Erase(0);
        if (observer->GetSubject() != this)
            observer->Attach(this);
        m_queueLock.Unlock();
        observer->Update();
        m_queueLock.Lock();
    }
    m_queueLock.Unlock();
}

// Mac 0x1f8f0, Win 0x4247b0
RefPointer<Object> SystemUpdaterInstantiator::CreateObject() const
{
    return RefPointer<Object>(new SystemUpdater);
}

// ---------------------------------------------------------------------------
// RunInMainThreadObserver
// ---------------------------------------------------------------------------

// Mac 0x1e1f0, Win 0x4249c0
void RunInMainThreadObserver::Run()
{
    if (!Thread::IsMainThread())
        SystemUpdater::Enqueue(this);
    else
        Update();
    m_done.Wait();
}

// Mac 0x1e250, Win 0x424a00
void RunInMainThreadObserver::Update()
{
    Detach();
    m_done.Signal();
}

} // namespace LEGO
