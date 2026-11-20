// Vortex 5.0.0 build 800 "vortexldd41" - Vx::UniverseConnector: the bookkeeping between a
// universe and the assemblies / entities / constraints / collision geometries in it (restored,
// class name from the RTTI of its local DisconnectEntityVisitor, member names
// inferred). Private header of src/Vx/UniverseConnector.cpp (Mac TU 453 0x422e20-0x42405b, Win
// 0x750160-0x750790).
#ifndef VX_UNIVERSECONNECTOR_H
#define VX_UNIVERSECONNECTOR_H

#include "Vx/VxUniverse.h"
#include "Vx/VxVisitor.h"
#include <stdio.h>

namespace Vx {

class VxAssemblyBase;
class VxEntity;
class VxConstraint;
class VxCollisionGeometry;
class VxCollisionSensor;

class UniverseConnector {
public:
    // an assembly enters / leaves universe u: setUniverse, then NotificationVisitor events
    // 7 / 0 / 3 (resp. 8 / 2 / 4) for everything in it
    static void connectAssembly(VxUniverse* u, VxAssemblyBase* a);      // Mac 0x423424, Win 0x750220
    static void disconnectAssembly(VxAssemblyBase* a);                  // Mac 0x423138, Win 0x7502a0 (setMergeRoot(0) first)
    // child added to / removed from parent: setParent, the parent's collection event 7 / 8,
    // the universe connection
    static void addChildAssembly(VxAssemblyBase* parent, VxAssemblyBase* child); // Mac 0x4234be, Win 0x7503d0
    static void removeChildAssembly(VxAssemblyBase* child);                      // Mac 0x4231f0, Win 0x750430
    static void connectEntity(VxAssemblyBase* a, VxEntity* e);          // Mac 0x422f40 (Win: inlined)
    // Mac 0x4230a8, Win 0x750350: entities driven by e (mNodeParent) are released first
    static void disconnectEntity(VxEntity* e);
    static void connectConstraint(VxAssemblyBase* a, VxConstraint* c);  // Mac 0x422ff6, Win 0x750160
    static void disconnectConstraint(VxConstraint* c);                  // Mac 0x422fd4, Win 0x750180
    static void connectCollisionGeometry(VxCollisionSensor* s, VxCollisionGeometry* cg); // Win 0x7501a0 (Win only)
    static void disconnectCollisionGeometry(VxCollisionGeometry* cg);   // Win 0x7501b0 (Win only)
    static void insertBody(VxUniverse* u, MdtBodyID body);              // Mac 0x422f86, Win 0x750200
    static void removeCollisionModel(VxUniverse* u, McdModelID model);  // Mac 0x422fa0, Win 0x7501e0
    static void insertCollisionModel(VxUniverse* u, McdModelID model);  // Mac 0x422fba, Win 0x7501c0
};

} // namespace Vx

// The notifications of the universe's collection while an assembly (with everything in it)
// enters (mode 0) or leaves (mode 1) it (Mac vtable 0x75cf48; slots 4 0x423988 / Win 0x7505e0,
// 5 0x423762 / Win 0x750630, 6 0x423ba8 / Win 0x750540, 7 0x423dd0 / Win 0x750590; dtor Mac
// 0x423738 / 0x423700, Win 0x728d60). Global namespace, as in the RTTI.
template <class T>
class NotificationVisitor : public Vx::VxVisitor {
public:
    typedef typename Vx::VxEntityConstraintCollection<T>::Collection Collection;
    NotificationVisitor(T* owner, Vx::VxEntityConstraintCollection<T>* collection, int mode)
        : mOwner(owner), mCollection(collection), mMode(mode) {}
    virtual ~NotificationVisitor() {}
    virtual int visitAssembly(Vx::VxAssemblyBase* a)
    {
        if (mMode == 0)
            mCollection->notifyCollection(&Vx::VxEntityConstraintCollection<T>::Subscriber::notifyAssembly, mOwner,
                                          Vx::VxEntityConstraintCollectionBase::kAssemblyAdded, a);
        return 0;
    }
    virtual int endVisitAssembly(Vx::VxAssemblyBase* a)
    {
        if (mMode == 1)
            mCollection->notifyCollection(&Vx::VxEntityConstraintCollection<T>::Subscriber::notifyAssembly, mOwner,
                                          Vx::VxEntityConstraintCollectionBase::kAssemblyRemoved, a);
        return 0;
    }
    virtual int visitEntity(Vx::VxEntity* e)
    {
        Vx::VxEntityConstraintCollectionBase::Event ev = mMode == 0 ? Vx::VxEntityConstraintCollectionBase::kEntityAdded
                                                                    : Vx::VxEntityConstraintCollectionBase::kEntityRemoved;
        mCollection->notifyCollection(&Vx::VxEntityConstraintCollection<T>::Subscriber::notifyEntity, mOwner, ev, e);
        return 0;
    }
    virtual int visitConstraint(Vx::VxConstraint* c)
    {
        Vx::VxEntityConstraintCollectionBase::Event ev = mMode == 0 ? Vx::VxEntityConstraintCollectionBase::kConstraintAdded
                                                                    : Vx::VxEntityConstraintCollectionBase::kConstraintRemoved;
        mCollection->notifyCollection(&Vx::VxEntityConstraintCollection<T>::Subscriber::notifyConstraint, mOwner, ev, c);
        return 0;
    }

    T* mOwner;                                          // +0x04
    Vx::VxEntityConstraintCollection<T>* mCollection;   // +0x08 (the owner's collection base)
    int mMode;                                          // +0x0c 0 = entering, 1 = leaving
};

// The print-file helper of TU 453 (Mac 0x422e20-0x422f26) is Vx::VxPrintFile (Vx/VxMaterial.h).

#endif
