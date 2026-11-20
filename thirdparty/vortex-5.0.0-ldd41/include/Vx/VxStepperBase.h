// Vortex 5.0.0 build 800 "vortexldd41" - VxStepperBase and the entity/constraint collection
// base of VxUniverse / VxAssemblyBase (restored header).
#ifndef VX_VXSTEPPERBASE_H
#define VX_VXSTEPPERBASE_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"

namespace Vx {

class VxEntity;
class VxConstraint;

// --------------------------------------------------------------------------------------
// VxStepperBase (Mac vtable 0x75dd88, 15 slots; Win 14; class id 0x105 Mac 0x453914)
// Mac 0x34 bytes, Win 0x4c (MSVC sizeof 0x50: rounded to the double alignment). The ctor is
// inline (expanded in VxUniverse's, Mac 0x4848fa). VxFrame (src/Vx/VxFrame.cpp, V4.1) keeps five
// copies of its stepper list, each sorted by one of the five priorities below, and calls slot 8
// on the first, 9 / 10 / 11 on the next three and 12 (+ the timings 13 / 14) on the last.
class VxStepperBase : public VxBase {
public:
    VX_DECLARE_ALLOCATOR();                             // delete Mac 0x453cbe, Win 0x70dbd0 (V4.1)
    static int getStaticClassType();                    // Mac 0x453914: 0x105
    static const VxArray<int>& getStaticClassTypes();   // Mac 0x454afa (name inferred; V1.3 addition)
    VxStepperBase() : mTimeStep(0.02f), mStepperParam0(5), mStepperParam1(5), mStepperParam2(5),
                      mStepperParam3(5), mStepperParam4(5) {}
    virtual bool isOfType(int type) const;              // slot 0 Mac 0x454e82, Win 0x70ef00
    virtual bool isOfClassType(int type) const;         // slot 1 Mac 0x45391e, Win 0x70e050
    // slot 2/3 Mac 0x456834/0x4567fc, Win 0x707660. Inline: the derived destructors expand it
    // (VxControllerGraph Mac 0x41dff8 stores the VxStepperBase vptr and calls ~VxBase).
    virtual ~VxStepperBase() {}
    // slot 4 getClassName pure, slot 5 setName = VxBase's
    virtual void setTimeStep(VxReal dt);                // slot 6 Mac 0x485352 (VxUniverse.cpp), Win 0x707650 (+0x18) (glue: universe->setTimeStep(0) / (1.0)) (name inferred)
    // slot 7 Mac 0x45631c (empty), Win 0x713eb0; VxUniverse Mac 0x482b0a prints its content to
    // the file (stderr for 0) (name inferred)
    virtual void print(const char* fileName);
    virtual void beginStep() = 0;                       // slot 8 (VxUniverse Mac 0x48077c: sets "stepping") (name inferred)
    virtual void stepPhase1() = 0;                      // slot 9 (VxUniverse Mac 0x4814c8: collision) (name inferred)
    virtual void stepDynamics() = 0;                    // slot 10 (VxUniverse Mac 0x481434: MdtWorldStep 0x49376b(world, mTimeStep)) (name inferred)
    virtual void stepPhase3() = 0;                      // slot 11 (VxUniverse Mac 0x4853c2, empty) (name inferred)
    virtual void endStep() = 0;                         // slot 12 (VxUniverse Mac 0x4811da: clears "stepping") (name inferred)
    virtual VxReal getTime1() const;                    // slot 13 Mac 0x456322, Win 0x7fd560 (0.0; VxUniverse: collision ms) (name inferred)
    virtual VxReal getTime2() const;                    // slot 14 Mac 0x45632a, Win 0x7fd560 (0.0; VxUniverse: dynamics ms) (name inferred)

    VxReal getTimeStep() const { return mTimeStep; }
    // the five step priorities as the sort keys VxFrame binds (Mac 0x456332 / 0x45634a /
    // 0x456362 / 0x45637a / 0x456392: (VxReal)param) (V4.1) (names inferred)
    VxReal getStepPriority0() const;
    VxReal getStepPriority1() const;
    VxReal getStepPriority2() const;
    VxReal getStepPriority3() const;
    VxReal getStepPriority4() const;

    VxReal mTimeStep;           // Mac +0x18, Win +0x30 (= (double)0.02f)
    int mStepperParam0;         // Mac +0x20, Win +0x38 (= 5) beginStep order (names inferred)
    int mStepperParam1;         // Mac +0x24, Win +0x3c stepPhase1 order
    int mStepperParam2;         // Mac +0x28, Win +0x40 stepDynamics order
    int mStepperParam3;         // Mac +0x2c, Win +0x44 stepPhase3 order
    int mStepperParam4;         // Mac +0x30, Win +0x48 endStep order
};

// --------------------------------------------------------------------------------------
// VxEntityConstraintCollectionBase / VxEntityConstraintCollection<T>: base of VxUniverse
// (Mac +0x34) and VxAssemblyBase (Mac +0x34): a subscriber collection (12 bytes) whose
// vtable is VxSubscriberCollection<VxEntityConstraintCollectionBase::Event,
// VxEntityConstraintCollection<T>::Subscriber, 9> (Mac 0x75ee18 universe, 0x75d1f0 assembly).
// Win-only RTTI: VxEntityConstraintCollection<VxUniverse>::Subscriber (5 slots), used by
// VxIntersectRules. Events and notification slots from the assembly's add / remove
// functions (Mac 0x429094 / 0x42a4d8 / 0x42b0e0 / 0x427e02), the universe's model
// insertion Mac 0x484e6c / 0x4850e2 and NotificationVisitor<VxUniverse>, Mac TU 453).
class VxEntityConstraintCollectionBase {
public:
    enum Event { // (names inferred; the values are the ones the senders pass)
        kEntityAdded = 0,                 // slot 2
        kEntityRemoving = 1,              // slot 2, before the entity leaves the assembly
        kEntityRemoved = 2,               // slot 2
        kConstraintAdded = 3,             // slot 3
        kConstraintRemoved = 4,           // slot 3
        kCollisionGeometryAdded = 5,      // slot 4 (the universe inserted the model)
        kCollisionGeometryRemoved = 6,    // slot 4
        kAssemblyAdded = 7,               // slot 5
        kAssemblyRemoved = 8,             // slot 5
        kEventNone = 0                    // (V0.2 name)
    };
};

class VxEntity;
class VxConstraint;
class VxCollisionGeometry;
class VxAssemblyBase;

template <class T>
class VxEntityConstraintCollection : public VxEntityConstraintCollectionBase {
public:
    // (slot meanings from the senders; names inferred)
    class Subscriber {
    public:
        virtual ~Subscriber() {}
        virtual void notifyEntity(T* owner, Event e, VxEntity* entity) = 0;                       // slot 2
        virtual void notifyConstraint(T* owner, Event e, VxConstraint* constraint) = 0;           // slot 3
        virtual void notifyCollisionGeometry(T* owner, Event e, VxCollisionGeometry* cg) = 0;     // slot 4
        virtual void notifyAssembly(T* owner, Event e, VxAssemblyBase* assembly) = 0;             // slot 5
    };
    typedef VxSubscriberCollection<VxEntityConstraintCollectionBase::Event, Subscriber, 9> Collection;
    Collection mCollectionSubscribers;

    // the original's notification sequence (inlined at every sender): the subscribers of the
    // event, then the all-event subscribers, each list copied first (V4.1)
    template <class O>
    static void notifyList(typename Collection::List* l, void (Subscriber::*fn)(T*, Event, O*), T* owner, Event e, O* object)
    {
        if (l == 0)
            return;
        int n = l->size();
        if (n == 0)
            return;
        Subscriber** copy = (Subscriber**)vxAlloc(n * sizeof(Subscriber*));
        for (int i = 0; i < n; ++i)
            copy[i] = l->begin()[i];
        for (int i = 0; i < n; ++i)
            (copy[i]->*fn)(owner, e, object);
        vxFree(copy);
    }
    template <class O>
    void notifyCollection(void (Subscriber::*fn)(T*, Event, O*), T* owner, Event e, O* object)
    {
        if (mCollectionSubscribers.mEventSubscribers != 0)
            notifyList(&mCollectionSubscribers.mEventSubscribers[e], fn, owner, e, object);
        notifyList(mCollectionSubscribers.mSubscribers, fn, owner, e, object);
    }
};

} // namespace Vx

#endif
