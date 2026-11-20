// Vortex 5.0.0 build 800 "vortexldd41" - VxFrame and VxStepperBase (restored).
// Mac TU 487 (0x453914-0x45740e; static initialiser 0x45739e: <iostream> and the constraint
// pool's element size), Win 0x70dbd0-0x70f890. VxStepperBase::setTimeStep is in VxUniverse.cpp
// (Mac 0x485352), as in the original.
//
// The std::sort / std::find / std::for_each instances of the TU (Mac 0x4563bb-0x45739e,
// 0x45641c, 0x4564bf, 0x45665c, 0x4565de, 0x456aa6-0x4570d3; Win 0x70f270-0x70f5d0, 0x781760)
// are the library code below (boost bind / mem_fn objects in the original).
// NOTE: the five stepper lists are sorted with std::stable_sort: equal priorities keep their
// insertion order, as the original's introsort does for lists of up to 16 steppers (its
// insertion-sort range; LDD has one universe and the controller graph).
#include "Vx/VxFrame.h"
#include "Vx/VxStepperBase.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxVisitor.h"
#include "Vx/VxThreadPool.h"
#include "Vx/VxTimer.h"
#include "Vx/VxMaterial.h"
#include "Vx/VxCollisionPairRequest.h"
#include "Vx/VxSceneGraphInterface.h"
#include "Vx/VxMessage.h"
#include "Me/MeProfile.h"
#include "Me/VinPool.h"
#include "Mdt/MdtTypes.h"
#include "Mdt/MdtConstraint.h"
#include "Vinc/VincIntersect.h"
#include "Vinc/VincModel.h"
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincTriangleMeshContacts.h"
#include "Vinc/VincTriangleMesh.h"
#include "Vinc/VincConvexInteractions.h"
#include "Vinc/VincHeightField.h"
#include "Vinc/VincContactSimplify.h"
#include "Vinc/VincInternal.h"
#include <algorithm>
#include <string.h>
#include <iostream>

namespace Vx {

void vxResetModuleInitialized(); // src/Vx/VxModuleFlag.cpp (Mac 0x445ebe)

namespace {

VxFrame* sInstance = 0;                           // Mac 0x779350, Win 0xaf1384
const int sConstraintSize = MdtGetMaxConstraintSize(); // Mac 0x77fde4 (static initialiser 0x45739e), Win 0xaf1388

// the sort keys (Mac 0x779354-0x779364: member function pointers to the priority getters)
typedef VxReal (VxStepperBase::*PriorityFn)() const;
struct ByPriority {
    explicit ByPriority(PriorityFn f) : mFn(f) {}
    bool operator()(VxStepperBase* a, VxStepperBase* b) const { return (a->*mFn)() < (b->*mFn)(); }
    PriorityFn mFn;
};
void sortList(VxArray<VxStepperBase*>& l, PriorityFn f)
{
    if (l.begin() != l.end())
        std::stable_sort(l.begin(), l.end(), ByPriority(f));
}

// std::find + the inlined erase of the originals (Mac 0x454276)
template <class T>
bool eraseFirst(VxArray<T>& a, const T& x)
{
    return a.remove(x);
}

} // namespace

// =============================================================================================
// VxStepperBase
// =============================================================================================

// Mac 0x453914 (0x105), 0x45391e, 0x454afa, 0x454e82; Win 0x70e050, 0x70ef00 (+ 0x70ee80)
VX_IMPLEMENT_CLASS_TYPES(VxStepperBase, VxBase, 0x105)

// Mac 0x453cbe (delete), Win 0x70dbd0; new: Win 0x70dc00 (Win only)
VX_IMPLEMENT_ALLOCATOR(VxStepperBase)

// Mac 0x45631c, Win 0x713eb0 (shared empty function)
void VxStepperBase::print(const char*)
{
}

// Mac 0x456322 / 0x45632a, Win 0x7fd560 (shared)
VxReal VxStepperBase::getTime1() const
{
    return 0.0;
}

VxReal VxStepperBase::getTime2() const
{
    return 0.0;
}

// Mac 0x456332 / 0x45634a / 0x456362 / 0x45637a / 0x456392 (Win: inlined into the sort)
VxReal VxStepperBase::getStepPriority0() const { return mStepperParam0; }
VxReal VxStepperBase::getStepPriority1() const { return mStepperParam1; }
VxReal VxStepperBase::getStepPriority2() const { return mStepperParam2; }
VxReal VxStepperBase::getStepPriority3() const { return mStepperParam3; }
VxReal VxStepperBase::getStepPriority4() const { return mStepperParam4; }

// =============================================================================================
// VxFrame
// =============================================================================================

// Mac 0x453934 (0x104), 0x45393e, 0x45445c, 0x4547e4; Win 0x70e070, 0x70efb0 (+ 0x70ef30)
VX_IMPLEMENT_CLASS_TYPES(VxFrame, VxBase, 0x104)

// Mac 0x453cd5 (new) / 0x453ca7 (delete), Win: vxAlloc / 0x70dc10
VX_IMPLEMENT_ALLOCATOR(VxFrame)

// Mac 0x453954, Win 0x70dc40
VxFrame* VxFrame::currentInstance()
{
    return sInstance;
}

// Mac 0x453966, Win 0x70dc70
void VxFrame::destroy()
{
    if (sInstance != 0) {
        delete sInstance;
        sInstance = 0;
    }
}

// Mac 0x455430, Win 0x70e7f0
VxFrame* VxFrame::instance()
{
    if (sInstance == 0)
        sInstance = new VxFrame();
    return sInstance;
}

// C2 Mac 0x454ec8 (C1 0x45541c), Win 0x70e190
VxFrame::VxFrame()
    : VxBase(), m18(), m28(), m38(), m48(), m58(), mUniverses(), m78(), mObjects(),
      mBodyPool(0), mConstraintPool(0), mA0(true), mA4(1), mTimeStep(0.05f),
      mMinTimeStep(0.0125f), mMaxTimeStep(0.05f), mTimeStepSmoothing(0.9f), mAutoTimeStep(false),
      mTimer(0), mThreadCount(2), mSceneGraphInterface(0), mMaterialTable(0), mDefaultPairRequest(0),
      mCollisionTime(0), mDynamicsTime(0), mF0(0), mF8(0), mThreadPool(0), m104(false)
{
    VxBase::setName("Frame");
    VxBase::resetIDCounter();
    VxTimer::initialise();
    mThreadCount = VxGetNumberOfProcessors();

    // the collision library: framework, geometry types, the interactions VxFrame itself owns
    VincInit(0x14, 0x14);
    VincPrimitivesRegisterTypes();
    VincTriangleMeshInit();
    VincConvexRegisterType();                   // (Win: VincConvexMeshRegisterType directly)
    VincHeightFieldRegisterType();
    VincTriangleMeshConvexRegisterInteraction();
    VincAggregateRegisterType();
    VincAggregateRegisterInteractions();
    VincTriangleListRegisterLineSegment();

    mDefaultPairRequest = new VxCollisionPairRequest();
    VincRequestInit(mDefaultPairRequest->mRecord);
    VincSetDefaultRequestPtr(mDefaultPairRequest->mRecord);
    mDefaultPairRequest->setFaceNormalsFirst(true);

    // the body and constraint pools of every universe's world (operator new(0x34), zeroed)
    VinPool* bodies = (VinPool*)operator new(sizeof(VinPool));
    memset(bodies, 0, sizeof(VinPool));
    mBodyPool = bodies;
    VinPool* constraints = (VinPool*)operator new(sizeof(VinPool));
    memset(constraints, 0, sizeof(VinPool));
    mConstraintPool = constraints;
    VinPoolInitSimple(bodies, 10, sizeof(MdtBody), 0x10);
    VinPoolInitSimple(constraints, 10, sConstraintSize, 0);

    mA0 = true;
    mAutoTimeStep = false;
    mMinTimeStep = 0.0125f;
    mMaxTimeStep = 0.05f;
    mTimeStepSmoothing = 0.9f;

    mTimer = new VxTimer();
    mTimer->reset();
    mTimer->start();

    mMaterialTable = new VxMaterialTable();
    MeProfileStartFrames(0, 0, 0);
    setCollisionTolerance(0.001f);
}

// D2 Mac 0x45548a (D1 0x4558c2, D0 0x4558d6), Win 0x70e870 (sdd 0x70f860)
VxFrame::~VxFrame()
{
    // the frame owns its steppers (slot 3: the deleting destructor)
    for (VxStepperBase** i = m18.begin(); i != m18.end(); ++i)
        if (*i != 0)
            delete *i;
    m18.mEnd = m18.mBegin;
    m28.mEnd = m28.mBegin;
    m38.mEnd = m38.mBegin;
    m48.mEnd = m48.mBegin;
    m58.mEnd = m58.mBegin;
    m78.mEnd = m78.mBegin;
    mUniverses.mEnd = mUniverses.mBegin;

    delete mMaterialTable;
    delete mDefaultPairRequest;
    sInstance = 0;
    VinPoolDestroy((VinPool*)mConstraintPool);
    VinPoolDestroy((VinPool*)mBodyPool);
    operator delete(mConstraintPool);
    operator delete(mBodyPool);
    vxResetModuleInitialized();
    VincTerm();
    if (mSceneGraphInterface != 0)
        delete (VxSceneGraphInterface*)mSceneGraphInterface;
    delete mTimer;
    // NOTE: Mac 0x47f66e (an empty VxTimer function) is called here
    MeProfileStopFrames();
    if (mThreadPool != 0)
        delete mThreadPool;
    // NOTE: Win then checks in the FLEXnet jobs of the four Vortex features (mdt, mcd, vxadvcon,
    // vxcable) - the license gates are the restored build is the licensed one.
}

// Mac 0x45630a, Win 0x70f5c0
const char* VxFrame::getClassName() const
{
    return "VxFrame";
}

// Mac 0x453996
VxReal VxFrame::getTimeStep() const
{
    return mTimeStep;
}

// Mac 0x4539a4 (Win: inlined, e.g. 0x70e470)
void VxFrame::setAutomaticTimeStep(bool automatic, VxReal minTimeStep, VxReal maxTimeStep, VxReal smoothing)
{
    mAutoTimeStep = automatic;
    mMinTimeStep = minTimeStep;
    mMaxTimeStep = maxTimeStep;
    mTimeStepSmoothing = smoothing;
    if (smoothing < 0.0 || 1.0 < smoothing)
        mTimeStepSmoothing = 0.1f;
    if (automatic)
        mTimeStep = mMinTimeStep;
}

// Mac 0x453b46, Win 0x70e470
void VxFrame::setTimeStep(VxReal dt)
{
    if (!(dt < 0.0)) {
        mTimeStep = dt;
        for (VxStepperBase** i = m18.begin(); i != m18.end(); ++i)
            (*i)->setTimeStep(dt);
    } else {
        setAutomaticTimeStep(true, 0.0125f, 0.05f, 0.9f);
    }
}

// Mac 0x453a24
int VxFrame::getUniverseCount() const
{
    return mUniverses.size();
}

// Mac 0x453a36
VxUniverse* VxFrame::getUniverse(int i) const
{
    if (-1 < i && i < getUniverseCount())
        return mUniverses.begin()[i];
    return 0;
}

// Mac 0x453a6a (Win: Win 0x70e650)
void VxFrame::stepDynamics()
{
    if (mTimeStep != 0.0) {
        for (VxStepperBase** i = m38.begin(); i != m38.end(); ++i)
            (*i)->stepDynamics();
        for (VxStepperBase** i = m48.begin(); i != m48.end(); ++i)
            (*i)->stepPhase3();
    }
}

// Mac 0x453ae0, Win 0x70e500
void VxFrame::stepCollision()
{
    for (VxStepperBase** i = m28.begin(); i != m28.end(); ++i)
        (*i)->stepPhase1();
}

// Mac 0x453b10, Win 0x70e770
bool VxFrame::hasStepper(VxStepperBase* s) const
{
    return std::find(m18.begin(), m18.end(), s) != m18.end();
}

// Mac 0x453c2c, Win 0x70dfe0
void VxFrame::setCollisionTolerance(VxReal tolerance)
{
    VxAssert(0.0 < tolerance, "VxUniverse::setContactBufferSize: newSize must be larger than 0\n");
    VincSetDefaultPadding(&tolerance);
    MeReal distance, cosAngle;
    VincContactSimplifyGetTolerances(&distance, &cosAngle);
    VincContactSimplifySetTolerances(tolerance, cosAngle);
}

// Mac 0x453ce8 (Win: inlined in 0x70e530)
void VxFrame::initializeSceneGraph()
{
    vxSceneGraphInterface(this)->vf11();
    mA0 = false;
}

// Mac 0x453d2a, Win 0x70dd40
VxThreadPool* VxFrame::getThreadPool()
{
    if (mThreadPool == 0)
        mThreadPool = VxThreadPool::create(mThreadCount, "VxFrame ThreadPool");
    return mThreadPool;
}

// Mac 0x453d74, Win 0x70dd70
int VxFrame::accept(VxVisitor* v)
{
    int r = 0;
    for (VxUniverse** i = mUniverses.begin(); i != mUniverses.end(); ++i) {
        r = (*i)->accept(v);
        if (r == 2)
            return 2;
    }
    return r;
}

// Mac 0x453db6, Win 0x70e7a0
bool VxFrame::removeObject(VxBase* object)
{
    if (m104)
        return eraseFirst(mObjects, object);
    return false;
}

// Mac 0x453ec2 (Win only callers inlined it)
void VxFrame::releaseNode(const VxNode& node)
{
    VxNode n = node;
    vxSceneGraphInterface(this)->releaseNode(n);
}

// Mac 0x453f06, Win 0x70ddb0
void VxFrame::updateSceneGraph()
{
    vxSceneGraphInterface(this)->vf09();
}

// Mac 0x453f38, Win 0x70e6f0
void VxFrame::stepEnd()
{
    for (VxStepperBase** i = m58.begin(); i != m58.end(); ++i) {
        VxStepperBase* s = *i;
        s->endStep();
        mCollisionTime = mCollisionTime + s->getTime1();
        mDynamicsTime = mDynamicsTime + s->getTime2();
    }
    vxSceneGraphInterface(this)->vf09();
}

// Mac 0x453fee, Win 0x70e530
void VxFrame::stepStart()
{
    mF8 = 0;
    mF0 = 0;
    mDynamicsTime = 0;
    mCollisionTime = 0;
    if (mA0)
        initializeSceneGraph();
    if (mAutoTimeStep) {
        // NOTE: Win evaluates dt * s + (1 - s) * measured on the x87 stack (one rounding)
        VxReal s = mTimeStepSmoothing;
        VxReal dt = mTimeStep;
        VxReal measured = mTimer->stop();
        mTimeStep = (1.0 - s) * measured + s * dt;
        mTimer->start();
        if (mTimeStep <= mMaxTimeStep) {
            if (mTimeStep < mMinTimeStep)
                mTimeStep = mMinTimeStep;
        } else {
            mTimeStep = mMaxTimeStep;
        }
        setTimeStep(mTimeStep);
    }
    vxSceneGraphInterface(this)->vf08();
    for (VxStepperBase** i = m18.begin(); i != m18.end(); ++i)
        (*i)->beginStep();
}

// Win 0x70eca0 (dead-stripped on the Mac)
void VxFrame::step()
{
    stepStart();
    stepCollision();
    stepDynamics();
    stepEnd();
}

// Mac 0x45413e, Win 0x70efe0
void VxFrame::addObject(VxBase* object)
{
    if (m104)
        mObjects.pushBackUnique(object);
}

// Mac 0x454276, Win 0x70ed20
bool VxFrame::removeStepper(VxStepperBase* s)
{
    // (the original compares the stepper pointer with the universe entries directly; only a
    // universe can be in that list)
    VxUniverse* u = dynamic_cast<VxUniverse*>(s);
    if (u != 0)
        eraseFirst(mUniverses, u);
    eraseFirst(m18, s);
    eraseFirst(m28, s);
    eraseFirst(m38, s);
    eraseFirst(m48, s);
    return eraseFirst(m58, s);
}

// Mac 0x45443e, Win 0x70ee20
bool VxFrame::removeUniverse(VxUniverse* u)
{
    return removeStepper(u);
}

// Mac 0x45482a (Win: in 0x70f5d0)
void VxFrame::sortSteppers()
{
    sortList(m18, &VxStepperBase::getStepPriority0);
    sortList(m28, &VxStepperBase::getStepPriority1);
    sortList(m38, &VxStepperBase::getStepPriority2);
    sortList(m48, &VxStepperBase::getStepPriority3);
    sortList(m58, &VxStepperBase::getStepPriority4);
}

// Mac 0x455d14 (Win: in 0x70f5d0)
void VxFrame::addStepper(VxStepperBase* s)
{
    if (hasStepper(s))
        return;
    s->setTimeStep(getTimeStep());
    m18.push_back(s);
    m28.push_back(s);
    m38.push_back(s);
    m48.push_back(s);
    m58.push_back(s);
    if (s != 0) {
        VxUniverse* u = dynamic_cast<VxUniverse*>(s);
        if (u != 0)
            mUniverses.pushBackUnique(u);
    }
    sortSteppers();
}

// Mac 0x4562f0
void VxFrame::addUniverse(VxUniverse* u)
{
    addStepper(u);
}

} // namespace Vx
