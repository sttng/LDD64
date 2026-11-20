// Vortex 5.0.0 build 800 "vortexldd41" - VxControllerGraph (restored header).
//
// The process-wide controller scheduler: a VxStepperBase registered with the VxFrame (by the
// first VxControllerBase constructor) whose step (slot 8) runs every controller of every
// assembly once, in dependency order:
//  * rebuild (when invalidated): a VxControllerGraphVisitor collects the embedded composite
//    of every assembly of every universe of the frame, recursively their child controllers
//    and all their connections. A controller depends on the source controller of each
//    connection into it (except connections created as "not a dependency", VxConnection
//    mNotDependency) and on its parent composite; a composite is followed by its children and
//    by the destination controller of each connection out of it.
//  * step: Kahn's topological order starting from the controllers without predecessors (in
//    collection order, first-in first-out), then VxControllerBase::execute for each one.
// Any change of controllers / connections invalidates the graph.
// Mac TU 447 "VxControllerGraph.cpp" (0x41d6c0-0x42051f), Win 0x7fb450-0x7fd570.
#ifndef VX_VXCONTROLLERGRAPH_H
#define VX_VXCONTROLLERGRAPH_H

#include "Vx/VxStepperBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxVisitor.h"
#include <map>

namespace Vx {

class VxControllerBase;
class VxCompositeController;
class VxConnection;
class VxControllerGraph;

// --------------------------------------------------------------------------------------
// VxControllerGraphVisitor (Mac vtable 0x75cc68, 9 slots; Win 0x7fc340 / 0x7fb8c0)
class VxControllerGraphVisitor : public VxVisitor {
public:
    explicit VxControllerGraphVisitor(VxControllerGraph* graph) : mGraph(graph) {} // Mac 0x41d7ae / 0x41d7ce
    virtual ~VxControllerGraphVisitor();                    // Mac D1 0x41d8e0 (-> 0x41d8b6) / D0 0x41d87e, Win 0x7fc340
    virtual int visitAssembly(VxAssemblyBase* a);           // slot 4 Mac 0x41e81e, Win 0x7fb8c0

    // collects the composite, its connections and (recursively) its children
    // (Mac 0x41e4ea, Win 0x7fb810) (name inferred)
    void addComposite(VxCompositeController* c);

    VxControllerGraph* mGraph; // +0x04
};

// --------------------------------------------------------------------------------------
// Listener of the graph's step (no RTTI, no implementation linked) (all names inferred).
// Mac slots: 0/1 dtor, 2-6 below (offsets 8 / 0xc / 0x10 / 0x14 / 0x18 of Mac 0x41d6ec-0x41d78a).
class VxControllerGraphListener {
public:
    virtual ~VxControllerGraphListener() {}
    virtual void beginStep(VxArray<VxControllerBase*>* order) = 0;   // slot 2 (Mac 0x41d6ec)
    virtual int skipStep() = 0;                                      // slot 3 (Mac 0x41d710): non-zero skips the step
    virtual void endStep() = 0;                                      // slot 4 (Mac 0x41d73a)
    virtual int skipController(VxControllerBase* c) = 0;             // slot 5 (Mac 0x41d758): non-zero skips c
    virtual void controllerExecuted(VxControllerBase* c) = 0;        // slot 6 (Mac 0x41d78a)
};

// --------------------------------------------------------------------------------------
// VxControllerGraph (Mac vtable 0x75cca8, 15 slots; Win 14; class id 0x181 Mac 0x41d6c0)
// Mac 0xb4 bytes (new(0xb4) Mac 0x41da84).
class VxControllerGraph : public VxStepperBase {
public:
    typedef VxArray<VxControllerBase*> ControllerList;
    typedef std::map<VxControllerBase*, ControllerList> ControllerMap;
    typedef std::map<VxControllerBase*, int> CountMap;

    VX_DECLARE_ALLOCATOR();                              // new Mac 0x41d86a, delete Mac 0x41d853
    VX_DECLARE_CLASS_TYPES();                            // slots 0/1: Mac 0x41de66 / 0x41d6ca, Win 0x7fb6c0 / 0x7fb5c0; types Mac 0x41dade
    VxControllerGraph();                                 // Mac C2 0x41d948 / C1 0x41da70
    virtual ~VxControllerGraph();                        // slot 2/3 Mac 0x41e1be (-> 0x41dff8) / 0x41e1d2, Win 0x7fd570
    virtual const char* getClassName() const;            // slot 4 Mac 0x41faf6, Win 0x7fd550
    // slots 6/7: VxStepperBase's
    virtual void beginStep();                            // slot 8 Mac 0x41f5ba, Win 0x7fbee0: runs the controllers
    virtual void stepPhase1();                           // slot 9 Mac 0x41fb08 (empty)
    virtual void stepDynamics();                         // slot 10 Mac 0x41fb0e (empty)
    virtual void stepPhase3();                           // slot 11 Mac 0x41fb14 (empty)
    virtual void endStep();                              // slot 12 Mac 0x41fb1a (empty)
    virtual VxReal getTime1() const;                     // slot 13 Mac 0x41fb20, Win 0x7fd560 (0)
    virtual VxReal getTime2() const;                     // slot 14 Mac 0x41fb28, Win 0x7fd560 (0)

    // the singleton, created on first use (Mac 0x41da84, Win 0x7fc160) (name inferred)
    static VxControllerGraph* instance();
    // the singleton or 0 (Mac: the non-lazy pointer to 0x779194 read inline) (name inferred)
    static VxControllerGraph* currentInstance() { return sInstance; }
    // marks the order as stale (Mac 0x41d6e0, Win 0x7fb4d0) (name inferred)
    void invalidate() { mValid = false; }
    // invalidate the singleton if it exists (the idiom after every change of a composite) (name inferred)
    static void invalidateInstance()
    {
        if (sInstance != 0)
            instance()->invalidate();
    }
    // registers the singleton with the VxFrame as a stepper unless already there
    // (Mac 0x41d8f4, Win 0x7fb450) (name inferred)
    void registerWithFrame();
    // collects the controllers and builds the dependency maps (Mac 0x41e846, Win 0x7fb9b0) (name inferred)
    void rebuild();
    // the connections of mConnections whose source / destination controller is c
    // (Mac 0x41deac / 0x41e39e) (names inferred)
    VxArray<VxConnection*> getConnectionsFrom(VxControllerBase* c) const;
    VxArray<VxConnection*> getConnectionsTo(VxControllerBase* c) const;

    void setListener(VxControllerGraphListener* l) { mListener = l; } // (name inferred)
    bool isValid() const { return mValid; }

    static VxControllerGraph* sInstance;                 // Mac 0x779194, Win 0xafa24c

    bool mValid;                       // Mac +0x34 (name inferred)
    bool mRegistered;                  // Mac +0x35 (set when added to the frame) (name inferred)
    ControllerList mControllers;       // Mac +0x38 (collection order)
    VxArray<VxConnection*> mConnections; // Mac +0x48
    ControllerMap mPredecessors;       // Mac +0x58
    ControllerMap mSuccessors;         // Mac +0x70
    CountMap mPredecessorCount;        // Mac +0x88
    ControllerList mRoots;             // Mac +0xa0 (controllers without predecessors)
    VxControllerGraphListener* mListener; // Mac +0xb0
};

} // namespace Vx

#endif
