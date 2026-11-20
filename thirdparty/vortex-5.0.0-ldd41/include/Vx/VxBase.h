// Vortex 5.0.0 build 800 "vortexldd41" - VxRefCount, VxBase, VxBaseUserData and the
// subscriber collection template (restored header).
//
// Run-time type scheme (every Vortex class): a static class id (getStaticClassType, an
// out-of-line function returning a constant, e.g. VxBase 0x101 Mac 0x43046c), slot 1
// isOfClassType(id) = exact compare, slot 0 isOfType(id) = id is in the class's list of
// ancestor ids (a lazily built static vector, e.g. VxHinge Mac 0x45a2b4); classes without
// ancestors forward slot 0 to slot 1 (VxBase Mac 0x43048c). The glue tests
// "cg->isOfClassType(VxCompositeCollisionGeometry::getStaticClassType())" (Mac 0x3a8c20).
// Virtual slots (Mac = Itanium, Win = MSVC): 0 isOfType, 1 isOfClassType, 2/3 D1/D0 (Win 2
// scalar deleting dtor), 4 getClassName (pure in VxBase), 5 setName ... (names inferred).
#ifndef VX_VXBASE_H
#define VX_VXBASE_H

#include "Vx/VxPlatform.h"
#include "Vx/VxAllocator.h"
#include "Vx/VxMessage.h"
#include "Vx/VxArray.h"
#include <map>
#include <string>

namespace VxData {
class Container;
}

namespace Vx {

class VxBase;

// --------------------------------------------------------------------------------------
// VxRefCount (Mac vtable 0x75d348, 4 slots; Win 3; class id 0x102 Mac 0x4304aa)
// 8 bytes on both: vptr, refcount. The destructor is inline (expanded in LEGO code, e.g.
// PointGravitationAction dtor Mac 0x3b24f0 / Win 0x52e9a0): it warns unless the count is 0 or
// the "not heap allocated" marker 0x3fffffff.
class VxRefCount {
public:
    static int getStaticClassType();                // Mac 0x4304aa: 0x102 (name inferred)
    virtual bool isOfType(int type) const;          // slot 0 Mac 0x4304ca, Win 0x72a0f0
    virtual bool isOfClassType(int type) const;     // slot 1 Mac 0x4304b4, Win 0x72a220
    VxRefCount() : mRefCount(0) {}
    virtual ~VxRefCount()                           // slot 2/3 Mac 0x43176a/0x4316fc, Win 0x52e8f0
    {
        if (mRefCount != 0 && mRefCount != kStaticRefCount) {
            VxWarning(0, "VxRefCount: deleting still referenced object (refcount = %d)\n", mRefCount);
            VxWarning(0, "VxRefCount: memory corruption is possible.\n");
        }
    }
    void addRef() { ++mRefCount; }
    void release()
    {
        if (--mRefCount < 1)
            delete this;
    }
    int getRefCount() const { return mRefCount; }

    enum { kStaticRefCount = 0x3fffffff }; // (name inferred)

    int mRefCount; // +0x04 (Mac and Win)
};

// --------------------------------------------------------------------------------------
// VxSubscriberCollection<Event, Subscriber, N>: subscribers notified on events.
// 12 bytes on both platforms (from the dtor Mac 0x431cf8/0x431db0, Win 0x72abd0 and
// ~VxBase Mac 0x4312b8): vptr (Mac 2 slots: D1, D0), a lazily allocated array of N per-event
// lists (new VxArray[N]: deleted with delete[]) and a lazily allocated list of subscribers to
// every event (new VxArray: deleted with delete). Notification first copies the list (a
// subscriber may unsubscribe while notified), as ~VxBase does for event 0. Instances:
// <VxBase::Event, VxBase::Subscriber, 2> (VxBase +4), <VxConstraint::Event,
// VxConstraint::Subscriber, 2> (VxConstraint Mac +0x20), <VxEntityConstraintCollectionBase::
// Event, VxEntityConstraintCollection<T>::Subscriber, 9> (VxUniverse / VxAssemblyBase Mac +0x34).
// The add/remove members are not out of line in either binary (names inferred).
template <class Event, class Subscriber, int N>
class VxSubscriberCollection {
public:
    typedef VxArray<Subscriber*> List;
    VxSubscriberCollection() : mEventSubscribers(0), mSubscribers(0) {}
    virtual ~VxSubscriberCollection() // slot 0/1, e.g. <VxBase::Event,...> Mac 0x431db0/0x431cf8, Win 0x72abd0
    {
        delete mSubscribers;
        delete[] mEventSubscribers;
    }
    // subscriber to every event
    void addSubscriber(Subscriber* s)
    {
        if (!mSubscribers)
            mSubscribers = new List;
        addUnique(*mSubscribers, s);
    }
    void addSubscriber(Event e, Subscriber* s)
    {
        if ((int)e < 0 || (int)e >= N)
            return;
        if (!mEventSubscribers)
            mEventSubscribers = new List[N];
        addUnique(mEventSubscribers[e], s);
    }
    bool removeSubscriber(Subscriber* s) { return mSubscribers ? mSubscribers->remove(s) : false; }
    bool removeSubscriber(Event e, Subscriber* s)
    {
        return (mEventSubscribers && (int)e >= 0 && (int)e < N) ? mEventSubscribers[e].remove(s) : false;
    }
    // calls s->notify(e, object) for the subscribers of e, then for the all-event subscribers
    template <class O>
    void notify(Event e, O* object)
    {
        if (mEventSubscribers && (int)e >= 0 && (int)e < N)
            notifyList(mEventSubscribers[e], e, object);
        if (mSubscribers)
            notifyList(*mSubscribers, e, object);
    }
    bool hasSubscribers() const // (name inferred)
    {
        if (mSubscribers && !mSubscribers->empty())
            return true;
        if (mEventSubscribers)
            for (int i = 0; i < N; ++i)
                if (!mEventSubscribers[i].empty())
                    return true;
        return false;
    }

    List* mEventSubscribers; // +0x04 new List[N] or 0
    List* mSubscribers;      // +0x08 new List or 0

private:
    static void addUnique(List& l, Subscriber* s)
    {
        for (typename List::iterator i = l.begin(); i != l.end(); ++i)
            if (*i == s)
                return;
        l.push_back(s);
    }
    template <class O>
    static void notifyList(const List& l, Event e, O* object)
    {
        int n = l.size();
        if (n == 0)
            return;
        Subscriber** copy = (Subscriber**)vxAlloc(n * sizeof(Subscriber*));
        for (int i = 0; i < n; ++i)
            copy[i] = l.begin()[i];
        for (int i = 0; i < n; ++i)
            copy[i]->notify(e, object);
        vxFree(copy);
    }
    VxSubscriberCollection(const VxSubscriberCollection&);
    VxSubscriberCollection& operator=(const VxSubscriberCollection&);
};

// --------------------------------------------------------------------------------------
// Run-time type lists. Every class has a static class id and a function-local static
// VxArray<int> holding its own id followed by its parent's list (VxBase Mac 0x430708 = {0x101};
// VxBaseUserData Mac 0x43206a = {0x198, 0x101}); slot 0 isOfType searches the list, slot 1
// isOfClassType compares with the id. In the class (first virtuals = slots 0, 1):
//     VX_DECLARE_CLASS_TYPES();
// in the .cpp:
//     VX_IMPLEMENT_CLASS_TYPES(VxFoo, VxBar, 0x1nn)
// (getStaticClassTypes: name inferred)
#define VX_DECLARE_CLASS_TYPES()                          \
    static int getStaticClassType();                      \
    static const Vx::VxArray<int>& getStaticClassTypes(); \
    virtual bool isOfType(int type) const;                \
    virtual bool isOfClassType(int type) const

#define VX_IMPLEMENT_CLASS_TYPES(Class, Parent, id)                                   \
    int Class::getStaticClassType() { return (id); }                                  \
    bool Class::isOfClassType(int type) const { return getStaticClassType() == type; } \
    const Vx::VxArray<int>& Class::getStaticClassTypes()                              \
    {                                                                                 \
        static Vx::VxArray<int> sTypes;                                               \
        if (sTypes.empty()) {                                                         \
            sTypes.push_back(getStaticClassType());                                   \
            const Vx::VxArray<int>& p = Parent::getStaticClassTypes();                \
            sTypes.insert(sTypes.end(), p.begin(), p.end());                          \
        }                                                                             \
        return sTypes;                                                                \
    }                                                                                 \
    bool Class::isOfType(int type) const                                              \
    {                                                                                 \
        const Vx::VxArray<int>& l = getStaticClassTypes();                            \
        for (const int* i = l.begin(); i != l.end(); ++i)                             \
            if (*i == type)                                                           \
                return true;                                                          \
        return false;                                                                 \
    }

// --------------------------------------------------------------------------------------
// VxBase (Mac vtable 0x75d368, 6 slots; Win 0x9f88c0, 5; class id 0x101)
// Mac 0x18 bytes, Win 0x30. src/Vx/VxBase.cpp (V1.1, Mac TU 458 0x43046c-0x431ec4).
// Constructor Mac 0x430d26: id = counter 0x7791ac++, empty name, registers with
// VxFrame::currentInstance() (VxFrame::addObject, Mac 0x45413e) if a frame exists and then
// notifies the global creation subscriber (Mac 0x7791b0, slot 2, event kEventCreated).
// Destructor: notifies kEventDestroyed, unregisters from the frame, clears the name.
// Names (setName(name, NameMap**)) can be indexed in a multimap name -> object owned by the
// caller (allocated on the first insert, deleted when it becomes empty).
class VxBase {
public:
    enum Event { // (names inferred; 0 sent by the destructor, 1 by the constructor)
        kEventDestroyed = 0,
        kEventCreated = 1
    };
    // VxBase::Subscriber (Mac vtable 0x75e920, 3 slots: D1, D0, notify; Win 2). The dtor is a
    // weak inline function in the original (emitted in VxStateBase's TU, Mac 0x47b590/0x47b566,
    // Win 0x7feb60). (method name inferred)
    class Subscriber {
    public:
        virtual ~Subscriber() {}
        virtual void notify(Event event, VxBase* object) = 0; // slot 2
    };
    typedef std::multimap<std::string, VxBase*> NameMap; // (name inferred)

    VX_DECLARE_ALLOCATOR();                         // new Mac 0x43057f, delete Mac 0x430568 / Win 0x72a0a0
    static int getStaticClassType();                // Mac 0x43046c: 0x101
    static const VxArray<int>& getStaticClassTypes(); // Mac 0x430708, Win 0x72a310: {0x101}

    VxBase();                                       // Mac 0x430d26, Win 0x72a3b0
    virtual bool isOfType(int type) const;          // slot 0 Mac 0x43048c, Win 0x72a0d0 (= isOfClassType)
    virtual bool isOfClassType(int type) const;     // slot 1 Mac 0x430476, Win 0x72a200
    virtual ~VxBase();                              // slot 2/3 Mac 0x4316b0 (-> D2 0x4312b8)/0x430eb6, Win 0x72b800 (D2 0x72a6f0)
    virtual const char* getClassName() const = 0;   // slot 4 (returns "VxFrame", "VxHinge", ...)
    virtual void setName(const char* name);         // slot 5 Mac 0x430d04 -> setName(name, 0), Win 0x72a6d0
                                                    // (glue: rootMechanism->setName("rootMechanism"),
                                                    //  joint->setName("toolJoint"))

    // Mac 0x4306b8, Win 0x72a240: the name; an empty name is first replaced by
    // "default_<getClassName()>_<id>" (setDefaultName) (name inferred)
    const char* getName() const;
    // Mac 0x4308c4, Win 0x72a520: sets the name; with a map, the object's entry under the old
    // name is removed and one under the new name added (name inferred)
    void setName(const char* name, NameMap** nameMap);
    // Mac 0x430592, Win 0x72a110: setName("<prefix>_<className>_<id>") through slot 5; id < 0 =
    // getID(); at most 255 characters (name inferred)
    void setDefaultName(const char* prefix, const char* className, int id);
    // Mac 0x4307f0, Win 0x72a450: the first object of that name ("VxBase::findFromName found %d
    // object named '%s'." when there are several) or 0
    static VxBase* findFromName(const char* name, NameMap* nameMap);
    // Mac 0x431a74, Win 0x72ace0: all objects of that name, in map order (name inferred)
    static VxArray<VxBase*> findAllFromName(NameMap* nameMap, const char* name);
    // Mac 0x4304e8, Win 0x72a100: the next object gets id 0 (name inferred)
    static void resetIDCounter();
    int getID() const { return mID; }
    const std::string& getNameString() const { return mName; } // (inline accessor, no binary counterpart)

    static int sIDCounter;                  // Mac 0x7791ac, Win 0xaf1710 (name inferred)
    static Subscriber* sCreationSubscriber; // Mac 0x7791b0, Win 0xaf1714 (never set by LDD) (name inferred)

    VxSubscriberCollection<Event, Subscriber, 2> mSubscribers; // Mac +0x04, Win +0x04
    int mID;                                                     // Mac +0x10, Win +0x10 (name inferred)
    std::string mName;                                           // Mac +0x14, Win +0x14
};

// --------------------------------------------------------------------------------------
// VxBaseUserData (Mac vtable 0x75d3c8, 6 slots; class id 0x198 Mac 0x431ec4)
// Mac 0x20 bytes, Win 0x38. Ctor Mac 0x431f06. The glue stores its LEGO object in
// mUserData directly (Mac "*(cg + 0x1c) = this", Win "+0x34").
class VxBaseUserData : public VxBase {
public:
    static int getStaticClassType();                // Mac 0x431ec4: 0x198
    static const VxArray<int>& getStaticClassTypes(); // Mac 0x43206a: {0x198, 0x101}
    VxBaseUserData();                               // Mac 0x431f06, Win 0x763050
    virtual bool isOfType(int type) const;          // slot 0 Mac 0x4323f2, Win 0x763180
    virtual bool isOfClassType(int type) const;     // slot 1 Mac 0x431ece, Win 0x7630e0
    virtual ~VxBaseUserData();                      // slot 2/3 Mac 0x432056 (-> D2 0x432002)/0x431fa6,
                                                    // Win 0x7634b0 (D2 0x763070) (deletes mUserDataFields
                                                    // through its virtual deleting destructor)
    void* getUserData() const { return mUserData; }
    void setUserData(void* p) { mUserData = p; }

    // named user-data fields ("VxBaseUserData::setUserData: user data '%s' has an invalid type,
    // it must be a basic type.", Win strings) (name inferred)
    VxData::Container* mUserDataFields; // Mac +0x18, Win +0x30
    void* mUserData;                    // Mac +0x1c, Win +0x34
};

} // namespace Vx

#endif
