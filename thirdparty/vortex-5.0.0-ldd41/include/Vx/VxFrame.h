// Vortex 5.0.0 build 800 "vortexldd41" - VxFrame (restored header:
// VxFrame.cpp = Mac TU 487 0x453914-0x45740e, Win 0x70dbd0-0x70f890; VxFrameRegisterInteractions.cpp
// = Mac TU 488).
//
// The process-wide singleton that owns the steppers (universes, the controller graph), the
// registered objects, the Mdt body/constraint pools, the material table, the default collision
// pair request and the step timing. LDD drives the step in pieces (VortexPhysicsScene::
// StepPhysics, Mac TU 435 0x3af290): stepStart, [stepCollision], stepDynamics, stepEnd - i.e.
// the body of the frame's step (names inferred). Every stepper of the frame is stepped: all
// universes of the process.
//
// The steppers are kept in five lists, each sorted (ascending, std::sort) by one of the five
// VxStepperBase priorities: list 0 gets beginStep (slot 8) and setTimeStep, list 1 stepPhase1
// (slot 9), list 2 stepDynamics (10), list 3 stepPhase3 (11), list 4 endStep (12) + the timings.
#ifndef VX_VXFRAME_H
#define VX_VXFRAME_H

#include "Vx/VxBase.h"
#include "Vx/VxArray.h"
#include "Vx/VxIntersect.h"

namespace Vx {

class VxUniverse;
class VxStepperBase;
class VxVisitor;
class VxThreadPool;
class VxTimer;
class VxMaterialTable;
class VxNode;

// VxFrame (Mac vtable 0x75dd68, 6 slots; Win 5; class id 0x104 Mac 0x453934)
// Mac 0x108 bytes (instance(): new(0x108), Mac 0x455430), Win 0x120 (computed from the Mac
// layout with MSVC alignment; the pool offset +0xb4 is confirmed by Win VxHinge::VxHinge 0x7183a0,
// the thread count +0xe8 / pool +0x118 by Win 0x70dd40, timer +0xe4 / material table +0xf0 /
// pair request +0xf4 by the ctor Win 0x70e190).
class VxFrame : public VxBase {
public:
    VX_DECLARE_ALLOCATOR();                             // new Mac 0x453cd5, delete Mac 0x453ca7
    VX_DECLARE_CLASS_TYPES();                           // Mac 0x453934 (0x104), 0x45445c, slot 0 0x4547e4 / Win 0x70efb0, slot 1 0x45393e / Win 0x70e070
    static VxFrame* instance();                         // Mac 0x455430, Win 0x70e7f0 (creates on first use)
    static VxFrame* currentInstance();                  // Mac 0x453954, Win 0x70dc40: the instance or 0 (name inferred)
    static void destroy();                              // Mac 0x453966, Win 0x70dc70: delete the instance (slot 3) (name inferred)

    VxFrame();                                          // C1 Mac 0x45541c -> C2 0x454ec8, Win 0x70e190 (name "Frame")
    virtual ~VxFrame();                                 // slot 2/3 Mac 0x4558c2 (-> D2 0x45548a)/0x4558d6, Win 0x70f860 (-> 0x70e870)
    virtual const char* getClassName() const;           // slot 4 Mac 0x45630a, Win 0x70f5c0 "VxFrame"

    // --- steppers / universes (names inferred) ---
    void addUniverse(VxUniverse* u);                    // Mac 0x4562f0 -> addStepper
    bool hasStepper(VxStepperBase* s) const;            // Mac 0x453b10, Win 0x70e770 (list 0)
    // Mac 0x455d14: unless present: s->setTimeStep(mTimeStep), appended to the five lists (and
    // to the universes when it is one), then the lists are sorted
    void addStepper(VxStepperBase* s);
    // Mac 0x454276, Win 0x70ed20: out of the universes and the five lists; true when it was in
    // the last list
    bool removeStepper(VxStepperBase* s);
    bool removeUniverse(VxUniverse* u);                 // Mac 0x45443e, Win 0x70ee20 -> removeStepper
    int getUniverseCount() const;                       // Mac 0x453a24
    VxUniverse* getUniverse(int i) const;               // Mac 0x453a36 (0 when out of range)
    void sortSteppers();                                // Mac 0x45482a (std::sort of the five lists by priority)
    // Mac 0x453d74, Win 0x70dd70: every universe accepts v until one returns 2
    int accept(VxVisitor* v);

    // --- registered objects (VxBase ctor / dtor, while m104 is set) (declarations) ---
    void addObject(VxBase* object);                     // Mac 0x45413e, Win 0x70efe0 (unique)
    bool removeObject(VxBase* object);                  // Mac 0x453db6, Win 0x70e7a0

    // Mac 0x45742f, Win 0x7111d0 (Mac TU 488): the Vinc pair-test registrations (12 calls).
    // Static, with the frame as an unused stack argument (cdecl on Win: the glue's
    // VortexBrickPhysics ctor Win 0x51d769 pushes VxFrame::instance() and pops it itself).
    // Not called by Vortex itself: without it no pair test is registered and nothing collides.
    static void registerCollisionTypes(VxFrame* frame);

    // --- step pieces (names inferred) ---
    // Mac 0x453fee, Win 0x70e530: timings reset, the scene-graph interface set up on the first
    // step, the automatic time step (measured, smoothed, clamped), scene-graph slot 8, beginStep
    void stepStart();
    void stepCollision();                               // Mac 0x453ae0, Win 0x70e500: stepPhase1 of list 1
    void stepDynamics();                                // Mac 0x453a6a: stepDynamics / stepPhase3 (lists 2 / 3) when mTimeStep != 0
    void stepEnd();                                     // Mac 0x453f38: endStep + timings (list 4), scene-graph slot 9
    // Win 0x70eca0 (Win only): stepStart, stepCollision, stepDynamics, stepEnd
    void step();

    // --- time step (names inferred) ---
    VxReal getTimeStep() const;                         // Mac 0x453996 (out of line)
    // Mac 0x453b46: a negative dt selects the automatic time step (1/80 .. 1/20, smoothing 0.9);
    // otherwise mTimeStep = dt and every stepper of list 0 gets it
    void setTimeStep(VxReal dt);
    // Mac 0x4539a4: smoothing outside [0, 1] -> 0.1; with `automatic` the step restarts at min
    void setAutomaticTimeStep(bool automatic, VxReal minTimeStep, VxReal maxTimeStep, VxReal smoothing);
    bool isAutomaticTimeStep() const { return mAutoTimeStep; }

    // Mac 0x453c2c, Win 0x70dfe0: the default Vinc model padding and the first contact
    // simplification tolerance (the ctor passes (double)0.001f). NOTE: its assert text is
    // "VxUniverse::setContactBufferSize: newSize must be larger than 0" (the original's) (name inferred)
    void setCollisionTolerance(VxReal tolerance);

    // Addition for Kea's threaded MdtKeaStep, Win 0x75ed30): the frame's pool, made on first
    // use (VxThreadPool::create(mThreadCount, "VxFrame ThreadPool"))
    VxThreadPool* getThreadPool();                      // Mac 0x453d2a, Win 0x70dd40 (name inferred)
    int getThreadCount() const { return mThreadCount; }
    // (glue: the harness sets the count before the first step; the pool is made from it)
    void setThreadCount(int n) { mThreadCount = n; }

    // --- scene-graph interface (names inferred) ---
    void initializeSceneGraph();                        // Mac 0x453ce8: slot 11, mA0 = false
    void releaseNode(const VxNode& node);               // Mac 0x453ec2: slot 7
    void updateSceneGraph();                            // Mac 0x453f06, Win 0x70ddb0: slot 9

    VxCollisionPairRequest* getDefaultCollisionPairRequest() const { return mDefaultPairRequest; }
    VxMaterialTable* getMaterialTable() const { return mMaterialTable; } // (VxFrame +0xd8)
    void setRegisterNewObjects(bool b) { m104 = b; }    // glue: frame->m104 = false (Mac TU 422 0x39e7c0) (name inferred)

    VxArray<VxStepperBase*> m18;      // Mac +0x18, Win +0x30 list 0 (priority 0): beginStep, setTimeStep
    VxArray<VxStepperBase*> m28;      // Mac +0x28, Win +0x40 list 1: stepPhase1 (slot 9)
    VxArray<VxStepperBase*> m38;      // Mac +0x38, Win +0x50 list 2: stepDynamics (slot 10)
    VxArray<VxStepperBase*> m48;      // Mac +0x48, Win +0x60 list 3: stepPhase3 (slot 11)
    VxArray<VxStepperBase*> m58;      // Mac +0x58, Win +0x70 list 4: endStep, getTime1 / getTime2
    VxArray<VxUniverse*> mUniverses;  // Mac +0x68, Win +0x80 (accept Mac 0x453d74)
    VxArray<void*> m78;               // Mac +0x78, Win +0x90 (only emptied by the dtor)
    VxArray<VxBase*> mObjects;        // Mac +0x88, Win +0xa0 (addObject) (name inferred)
    void* mBodyPool;                  // Mac +0x98, Win +0xb0 (VinPool of 10 x 0x5a4 / Win 0x5b8: MdtBody) (name inferred)
    void* mConstraintPool;            // Mac +0x9c, Win +0xb4 (MdtConstraint pool, MdtGetMaxConstraintSize) (name inferred)
    bool mA0;                         // Mac +0xa0, Win +0xb8 (= true: the scene graph is set up by the first stepStart) (name inferred)
    int mA4;                          // Mac +0xa4, Win +0xbc (= 1, not read)
    VxReal mTimeStep;                 // Mac +0xa8, Win +0xc0 (= (double)0.05f)
    VxReal mMinTimeStep;              // Mac +0xb0, Win +0xc8 (= (double)0.0125f) (name inferred)
    VxReal mMaxTimeStep;              // Mac +0xb8, Win +0xd0 (= (double)0.05f) (name inferred)
    VxReal mTimeStepSmoothing;        // Mac +0xc0, Win +0xd8 (= (double)0.9f) (name inferred)
    bool mAutoTimeStep;               // Mac +0xc8, Win +0xe0 (name inferred)
    VxTimer* mTimer;                  // Mac +0xcc, Win +0xe4 (new(4) Mac / new(0x20) Win)
    int mThreadCount;                 // Mac +0xd0, Win +0xe8 (VxGetNumberOfProcessors) (name inferred)
    void* mSceneGraphInterface;       // Mac +0xd4, Win +0xec (VxSceneGraphInterface*, lazy) (name inferred)
    VxMaterialTable* mMaterialTable;  // Mac +0xd8, Win +0xf0 (new(0x38))
    VxCollisionPairRequest* mDefaultPairRequest; // Mac +0xdc, Win +0xf4
    VxReal mCollisionTime;            // Mac +0xe0, Win +0xf8 (sum of the steppers' getTime1) (name inferred)
    VxReal mDynamicsTime;             // Mac +0xe8, Win +0x100 (sum of getTime2) (name inferred)
    VxReal mF0;                       // Mac +0xf0, Win +0x108 (reset by stepStart)
    VxReal mF8;                       // Mac +0xf8, Win +0x110 (reset by stepStart)
    VxThreadPool* mThreadPool;        // Mac +0x100, Win +0x118
    bool m104;                        // Mac +0x104, Win +0x11c (register new objects; ctor: false)
};

} // namespace Vx

#endif
