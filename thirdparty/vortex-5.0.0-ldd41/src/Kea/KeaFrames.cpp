// Vortex 5.0.0 build 800 "vortexldd41" - the Kea frames (work slots) and the MdtKea API
// (restored). Mac sub-TU 556 0x4adfd2-0x4b02dd, Win 0x75e110-0x760700.
// API: Kea/MdtKea.h; classes: Kea/KeaFrames.h.
// Per batch, MdtWorldStep calls MdtKeaFramesSetupSlots, fills each slot (MdtKeaAddPartition), then
// MdtKeaStep: setup, iterate, integrate the ungrouped slots (on the VxFrame pool when threaded).
#include "Kea/KeaFrames.h"
#include "Kea/MdtKeaConstraints.h"
#include "Me/MeMessage.h"
#include "Vx/VxFrame.h"

#include <new>
#include <stddef.h>

// ---- the solver set --------------------------------------------------------------------------

// Mac inlined in 0x4ae43d, Win 0x75faa0
KeaSolverSet::KeaSolverSet()
{
    vbcholMatrix.m14 = 0;
    vbcholMatrix.mPool = 0;
    vbcholMatrix.mFlags = 0;
    numMatrices = 2;
    matrices[0] = &vbcholMatrix;
    matrices[1] = &bvbMatrix;
    numSteppers = 2;
    steppers[0] = &stableStepper;
    steppers[1] = &keaModel;
    numLcps = 1;
    lcps[0] = lcp_simple_hash_table;
    numSolvers = 2;
    solvers[0] = &stdSolver;
    solvers[1] = &itrSolver;
    void* p = MeMemoryAPI.create(sizeof(MemoryPool));
    pool = p != 0 ? new (p) MemoryPool() : 0;
}

// Mac inlined in 0x4ae2c8, Win 0x75fa20
KeaSolverSet::~KeaSolverSet()
{
    if (pool != 0) {
        pool->~MemoryPool();
        if (pool != 0)
            MeMemoryAPI.destroy(pool);
    }
}

// ---- parameters ------------------------------------------------------------------------------

// Mac 0x4ae07a, Win 0x75e460
void MEAPI MdtKeaParametersReset(MdtKeaParameters* p)
{
    p->stepSize = -1.0;
    p->maxIterations = 5;
    p->cpuResource = 0;
    p->stepperModel = 0;
    p->linearSolver = 1;
    p->constraintSolver = 0;
    p->m10 = 0;
    p->itrMaxIterations = 20;
    p->itrTolerance = (double)0.001f;
    p->m44 = 0;
    p->m48 = 0;
    p->lcp.maxIterations = 35;
    p->lcp.warmStart = 0;
    p->lcp.cycleCheckStart = 20;
    p->lcp.tolerance = 0.001;
    p->lcp.useHashTable = 1;
    p->lcp.m14 = 1.01;
    p->lcp.solver = 0;
    p->refineIterations = 1;
    p->refineTolerance = (double)1e-4f;
    p->owned = 0;
}

// Mac 0x4ae14c -> 0x4ae138, Win 0x75e4f0 (parameter in ECX)
void MEAPI MdtKeaParametersInit(MdtKeaParameters* p)
{
    MdtKeaParametersReset(p);
}

// Mac 0x4ae43d, Win 0x75e6a0
void MEAPI MdtKeaParametersCreateSolvers(MdtKeaParameters* p)
{
    void* m = MeMemoryAPI.create(sizeof(KeaSolverSet));
    p->owned = m != 0 ? new (m) KeaSolverSet() : 0;
}

// Mac 0x4ae2c8, Win 0x75eb80
void MEAPI MdtKeaParametersDestroy(MdtKeaParameters* p)
{
    KeaSolverSet* s = (KeaSolverSet*)p->owned;
    if (s != 0) {
        s->~KeaSolverSet();
        MeMemoryAPI.destroy(s);
    }
    p->owned = 0;
}

// Win 0x75e1a0 (Win only)
int MEAPI MdtKeaParametersCheck(MdtKeaParameters* p)
{
    const KeaSolverSet* s = (const KeaSolverSet*)p->owned;
    int ok = 1;
    // NOTE: compares with <, so stepperModel == numSteppers passes
    if (p->stepperModel < 0 || s->numSteppers < p->stepperModel) {
        MeWarning(0xc, "MdtKeaStep : no such model number parameters->stepper_model = : %d\n", p->stepperModel);
        ok = 0;
    }
    if (p->lcp.solver < 0 || s->numLcps <= p->lcp.solver) {
        MeWarning(0xc, "MdtKeaStep: no such lcp solver : %d\n", p->lcp.solver);
        ok = 0;
    }
    if (p->cpuResource != 0) {
        MeWarning(0xc, "MdtKeaStep: no such CPU resource : %d\n", p->cpuResource);
        ok = 0;
    }
    if (p->linearSolver < 0 || s->numMatrices <= p->linearSolver) {
        MeWarning(0xc, "MdtKeaStep: no such Linear Solver, see VindLinearSolver enumeration): %d\n",
                  p->linearSolver);
        ok = 0;
    }
    return ok;
}

// Win 0x75e500 (Win only)
void MEAPI MdtKeaParametersSetWorkspace(MdtKeaParameters* p, MeChunk* chunk, int, MeReal h)
{
    p->stepSize = h;
    const int size = p->workspaceSize + p->m64;
    p->workspace = MeChunkGetMem(chunk, size);
    p->workspaceSize = size;
    p->m64 = 0;
}

// Win 0x75e2d0 (Win only)
const char* MEAPI MdtKeaParametersGetLinearSolverName(const MdtKeaParameters* p)
{
    const KeaSolverSet* s = (const KeaSolverSet*)p->owned;
    if (p->linearSolver >= 0 && p->linearSolver < s->numMatrices && p->cpuResource == 0)
        return s->matrices[p->linearSolver]->getName();
    return 0;
}

// Win 0x75e310 (Win only)
const char* MEAPI MdtKeaParametersGetStepperName(const MdtKeaParameters* p)
{
    const KeaSolverSet* s = (const KeaSolverSet*)p->owned;
    if (p->stepperModel >= 0 && p->stepperModel < s->numSteppers)
        return s->steppers[p->stepperModel]->getName();
    return 0;
}

// Win 0x75e6e0 (Win only)
int MEAPI MdtKeaParametersAddStepper(MdtKeaParameters* p, WorldStepper* stepper)
{
    KeaSolverSet* s = (KeaSolverSet*)p->owned;
    s->steppers[s->numSteppers++] = stepper;
    return s->numSteppers;
}

// Win 0x75e710 (Win only)
int MEAPI MdtKeaParametersAddMatrix(MdtKeaParameters* p, LCPMatrix* matrix)
{
    KeaSolverSet* s = (KeaSolverSet*)p->owned;
    s->matrices[s->numMatrices++] = matrix;
    return s->numMatrices;
}

// Mac 0x4ae070, Win 0x75e450
int MEAPI MdtKeaGetThreadingMode(void)
{
    return 0;
}

// ---- slots -----------------------------------------------------------------------------------

// Mac 0x4adfee, Win 0x7b9c70
WorldStepper* KeaSlot::stepper() const
{
    return solvers->steppers[params.stepperModel];
}

// Mac 0x4ae028, Win 0x7b9ca0
LCPMatrix* const* KeaSlot::matrix() const
{
    return &solvers->matrices[params.cpuResource + params.linearSolver];
}

// Mac 0x4ae000, Win 0x7b9c80
constraint_solver* KeaSlot::solver() const
{
    return solvers->solvers[params.constraintSolver];
}

// Mac 0x4ae016, Win 0x7b9c90
KeaLcpFunction KeaSlot::lcp() const
{
    return solvers->lcps[params.lcp.solver];
}

// Mac 0x4ae040, Win 0x7b9cc0
bool KeaSlot::isUngrouped() const
{
    return constraints->grouped == 0;
}

// Mac inlined in 0x4ae218 / 0x4af3be, Win 0x75e350 / inlined in 0x75e900
void KeaSlot::setupStepper()
{
    iteration = 0;
    stepper()->setup(numBodies, constraints, &params, bodies, lcp(), *matrix(), solver(), solvers->pool);
    stepper()->resetIterationCount();
}

// Mac 0x4ae218, Win 0x75e350
void SetupPartitionWorkUnit::execute(int)
{
    slot->setupStepper();
}

// Mac 0x4ae1e2, Win 0x75e3f0
void UpdatePartitionWorkUnit::execute(int)
{
    slot->stepper()->prepareIteration();
    slot->stepper()->solve();
}

// Mac 0x4af388, Win 0x75e420
void IntegratePartitionWorkUnit::execute(int)
{
    slot->stepper()->integrate();
    slot->stepper()->cleanup();
}

// ---- frames ----------------------------------------------------------------------------------

// Mac 0x4afdca / 0x4afd20, Win 0x760360
VindsConstraintFrames::~VindsConstraintFrames()
{
    for (unsigned int i = 0; i < mChildren.size(); ++i)
        if (mChildren[i] != 0)
            delete mChildren[i];
}

// Mac 0x4af759, Win 0x75f9a0 -> 0x7604b0
InternalConstraintFrames::InternalConstraintFrames(MeMemoryAPIStruct* api)
    : mSlots(0), mCapacity(0), mNumSlots(0), mApi(api), mCallback(0), mCallbackData(0),
      mNumThreads(Vx::VxGetNumberOfProcessors()), mM2c(5), mWin40(0), mStepCount(0), mThreadPool(0)
{
}

// Mac 0x4b0086 / 0x4afe6c, Win 0x760550 (the work unit vectors go with the members)
InternalConstraintFrames::~InternalConstraintFrames()
{
}

// Mac 0x4ae390, Win 0x75ec70
void InternalConstraintFrames::destroySlots()
{
    for (int i = 0; i < mCapacity; ++i) {
        KeaSlot* slot = mSlots[i];
        slot->params.owned = slot->solvers;
        MdtKeaParametersDestroy(&slot->params);
        MdtKeaConstraintsFree(slot->constraints, mApi);
        mApi->destroy(slot->constraints);
        MeChunkTerm(&slot->chunk);
        delete slot;
    }
    mCapacity = 0;
    mApi->destroy(mSlots);
}

// Mac inlined in 0x4ae665, Win 0x75e740 (EBX = frames, ECX = count)
void InternalConstraintFrames::growSlots(int count)
{
    int i = mCapacity;
    if (count <= i)
        return;
    int capacity = i * 2;
    if (capacity <= count)
        capacity = count;
    mCapacity = capacity;
    mSlots = (KeaSlot**)mApi->resize(mSlots, capacity * sizeof(KeaSlot*));
    for (; i < mCapacity; ++i) {
        KeaSlot* slot = new KeaSlot;
        slot->constraints = 0;
        slot->bodies = 0;
        slot->numBodies = 0;
        slot->solvers = 0;
        MdtKeaParametersInit(&slot->params);
        slot->maxIterations = 0;
        slot->iteration = 0;
        slot->setConstraints(MdtKeaConstraintsCreate(mApi));
        MdtKeaParametersReset(&slot->params);
        MdtKeaParametersCreateSolvers(&slot->params);
        slot->setSolvers((KeaSolverSet*)slot->params.owned);
        MeChunkInit(&slot->chunk);
        slot->params.workspaceSize = 10;
        slot->params.m64 = 0;
        MeChunkSetAuto(&slot->chunk, slot->params.workspaceSize, 0x10);
        mSlots[i] = slot;
    }
}

// Mac 0x4ae19a, Win 0x75e670
int InternalConstraintFrames::callback(int phase)
{
    if (mCallback != 0)
        return mCallback(mCallbackData, phase);
    return 0;
}

// Win 0x75fbd0 (Mac inlined in 0x4af3be / 0x4ae89a): the number of ungrouped slots (they come
// first); 0 when the callback skips the solve (every slot is cleaned up then)
int InternalConstraintFrames::numSolvableSlots()
{
    if (callback(0) != 0) {
        for (unsigned int i = 0; i < mNumSlots; ++i)
            mSlots[i]->stepper()->cleanup();
        return 0;
    }
    int n = 0;
    for (unsigned int i = 0; i < mNumSlots; ++i)
        if (mSlots[i]->isUngrouped())
            ++n;
    return n;
}

// Mac 0x4ae160, Win 0x75e640: the largest iteration count of the first `count` slots (>= 1)
int InternalConstraintFrames::maxIterations(int count) const
{
    int n = 1;
    for (int i = 0; i < count; ++i)
        if (n < mSlots[i]->maxIterations)
            n = mSlots[i]->maxIterations;
    return n;
}

static void IterateSlot(KeaSlot* slot)
{
    if (slot->iteration < slot->maxIterations) {
        slot->stepper()->prepareIteration();
        slot->stepper()->solve();
        slot->stepper()->addConstraintForces();
        ++slot->iteration;
    }
}

// Mac 0x4af3be, Win 0x75e900
void InternalConstraintFrames::stepSerial()
{
    if (mNumSlots == 0)
        return;
    const unsigned int solvable = numSolvableSlots();
    if (solvable == 0)
        return;
    unsigned int i;
    for (i = 0; i < mNumSlots; ++i)
        mSlots[i]->setupStepper();
    for (int pass = maxIterations(solvable); pass != 0; --pass) {
        for (i = solvable; i < mNumSlots; ++i)
            IterateSlot(mSlots[i]);
        for (i = 0; i < solvable; ++i)
            IterateSlot(mSlots[i]);
    }
    for (i = 0; i < solvable; ++i)
        mSlots[i]->stepper()->computeAccelerations();
    const int skipIntegration = callback(1);
    for (i = 0; i < solvable; ++i) {
        if (skipIntegration == 0)
            mSlots[i]->stepper()->integrate();
        mSlots[i]->stepper()->cleanup();
    }
    for (; i < mNumSlots; ++i)
        mSlots[i]->stepper()->cleanup();
}

// Mac 0x4ae89a, Win 0x75ed30
void InternalConstraintFrames::stepThreaded()
{
    if (mNumSlots == 0)
        return;
    const unsigned int solvable = numSolvableSlots();
    if (solvable == 0)
        return;
    if (mThreadPool == 0)
        mThreadPool = Vx::VxFrame::currentInstance()->getThreadPool();
    unsigned int i;
    // setup: the ungrouped slots on the pool, the grouped ones here
    if (mSetupUnits.size() < solvable)
        mSetupUnits.resize(solvable);
    for (i = 0; i < solvable; ++i) {
        SetupPartitionWorkUnit& u = mSetupUnits[i];
        u.index = i;
        u.slot = mSlots[i];
        mThreadPool->queueWorkUnit(&u);
    }
    mThreadPool->startQueuedWorkUnits();
    mThreadPool->waitForCompletion();
    for (i = solvable; i < mNumSlots; ++i)
        mSlots[i]->setupStepper();
    unsigned int units = mNumSlots - solvable;
    if (units < solvable)
        units = solvable;
    if (mUpdateUnits.size() < units)
        mUpdateUnits.resize(units);
    for (int pass = maxIterations(solvable); pass != 0; --pass) {
        if (solvable < mNumSlots) {
            for (i = solvable; i < mNumSlots; ++i) {
                KeaSlot* slot = mSlots[i];
                if (slot->iteration < slot->maxIterations) {
                    UpdatePartitionWorkUnit& u = mUpdateUnits[i - solvable];
                    u.index = i;
                    u.slot = slot;
                    mThreadPool->queueWorkUnit(&u);
                }
            }
            mThreadPool->startQueuedWorkUnits();
            mThreadPool->waitForCompletion();
            for (i = solvable; i < mNumSlots; ++i) {
                KeaSlot* slot = mSlots[i];
                if (slot->iteration < slot->maxIterations) {
                    slot->stepper()->addConstraintForces();
                    ++slot->iteration;
                }
            }
        }
        for (i = 0; i < solvable; ++i) {
            KeaSlot* slot = mSlots[i];
            if (slot->iteration < slot->maxIterations) {
                UpdatePartitionWorkUnit& u = mUpdateUnits[i];
                u.index = i;
                u.slot = slot;
                mThreadPool->queueWorkUnit(&u);
            }
        }
        mThreadPool->startQueuedWorkUnits();
        mThreadPool->waitForCompletion();
        for (i = 0; i < solvable; ++i) {
            KeaSlot* slot = mSlots[i];
            if (slot->iteration < slot->maxIterations) {
                slot->stepper()->addConstraintForces();
                ++slot->iteration;
            }
        }
    }
    for (i = 0; i < solvable; ++i)
        mSlots[i]->stepper()->computeAccelerations();
    // NOTE: when the callback skips the integration the ungrouped slots are not cleaned up
    // (the serial step does clean them up)
    if (callback(1) == 0) {
        if (mIntegrateUnits.size() < solvable)
            mIntegrateUnits.resize(solvable);
        for (i = 0; i < solvable; ++i) {
            IntegratePartitionWorkUnit& u = mIntegrateUnits[i];
            u.index = i;
            u.slot = mSlots[i];
            mThreadPool->queueWorkUnit(&u);
        }
        mThreadPool->startQueuedWorkUnits();
        mThreadPool->waitForCompletion();
    }
    for (i = solvable; i < mNumSlots; ++i)
        mSlots[i]->stepper()->cleanup();
}

static inline InternalConstraintFrames* Frames(MdtKeaFrames* f)
{
    return reinterpret_cast<InternalConstraintFrames*>(f);
}
static inline const InternalConstraintFrames* Frames(const MdtKeaFrames* f)
{
    return reinterpret_cast<const InternalConstraintFrames*>(f);
}

// Mac 0x4af759, Win 0x75f9a0
MdtKeaFrames* MEAPI MdtKeaFramesCreate(MeMemoryAPIStruct* api)
{
    return reinterpret_cast<MdtKeaFrames*>(new InternalConstraintFrames(api));
}

// Mac 0x4ae418, Win 0x75ed00
void MEAPI MdtKeaFramesDestroy(MdtKeaFrames* f)
{
    if (f == 0)
        return;
    Frames(f)->destroySlots();
    delete Frames(f);
}

// Mac 0x4ae058, Win 0x75e240
int MEAPI MdtKeaFramesGetNumThreads(const MdtKeaFrames* f)
{
    return Frames(f)->mNumThreads;
}

// Win 0x75e250 (Win only): returns the old count
int MEAPI MdtKeaFramesSetNumThreads(MdtKeaFrames* f, int n)
{
    const int old = Frames(f)->mNumThreads;
    Frames(f)->mNumThreads = n;
    return old;
}

// Mac 0x4ae064, Win 0x75e270
int MEAPI MdtKeaFramesGetM2c(const MdtKeaFrames* f)
{
    return Frames(f)->mM2c;
}

// Win 0x75e280 (Win only)
int MEAPI MdtKeaFramesSetM2c(MdtKeaFrames* f, int n)
{
    const int old = Frames(f)->mM2c;
    Frames(f)->mM2c = n;
    return old;
}

// Win 0x75e2a0 (Win only)
MeReal MEAPI MdtKeaFramesGetWin40(const MdtKeaFrames* f)
{
    return Frames(f)->mWin40;
}

// Win 0x75e2b0 (Win only)
MeReal MEAPI MdtKeaFramesSetWin40(MdtKeaFrames* f, MeReal v)
{
    const MeReal old = Frames(f)->mWin40;
    Frames(f)->mWin40 = v;
    return old;
}

// Mac 0x4ae665, Win 0x75e840
void MEAPI MdtKeaFramesSetupSlots(MdtKeaFrames* frames, int from, int to, const int* constraintsSize,
                                  const int* bodiesSize, const int* numRows, MdtKeaBody* const* keaBodies,
                                  const int* bodiesStart, const int* order)
{
    InternalConstraintFrames* f = Frames(frames);
    f->growSlots(to - from);
    f->mNumSlots = to - from;
    for (int i = from; i < to; ++i) {
        const int p = order[i];
        KeaSlot* slot = f->mSlots[i - from];
        MdtKeaConstraints* c = slot->constraints;
        if (c->bodyCapacity < bodiesSize[p] || c->blockCapacity < constraintsSize[p] || c->rowCapacity < numRows[p])
            MdtKeaConstraintsReserve(c, constraintsSize[p], bodiesSize[p], numRows[p], f->mApi);
        slot->numBodies = bodiesSize[p];
        slot->bodies = keaBodies + bodiesStart[p];
    }
}

// Win 0x75e550 (Win only)
void MEAPI MdtKeaFramesSetSlotBodies(MdtKeaFrames* f, int slot, MdtKeaBody* const* bodies, int n)
{
    Frames(f)->mSlots[slot]->bodies = bodies;
    Frames(f)->mSlots[slot]->numBodies = n;
}

// Mac 0x4ae652, Win 0x75e580
MdtKeaConstraints* MEAPI MdtKeaFramesGetConstraints(MdtKeaFrames* f, int slot)
{
    return Frames(f)->mSlots[slot]->constraints;
}

// Win 0x75e5a0 (Win only)
int MEAPI MdtKeaFramesGetCapacity(const MdtKeaFrames* f)
{
    return Frames(f)->mCapacity;
}

// Win 0x75e5b0 (Win only)
int MEAPI MdtKeaFramesGetNumSlots(const MdtKeaFrames* f)
{
    return Frames(f)->mNumSlots;
}

// Mac 0x4ae871, Win 0x75e5c0
void* MEAPI MdtKeaFramesGetSlotM0c(MdtKeaFrames* f, int slot)
{
    // NOTE: the solver set pointer, returned as an int in both binaries (PORT64: as a pointer,
    // the same 4 bytes in eax on 32-bit)
    return Frames(f)->mSlots[slot]->solvers;
}

// Mac 0x4ae637, Win 0x75e5e0
void MEAPI MdtKeaFramesSetSlotMaxIterations(MdtKeaFrames* f, int slot, int n)
{
    Frames(f)->mSlots[slot]->maxIterations = n + 1;
}

// Mac 0x4ae621, Win 0x75e600
MeChunk* MEAPI MdtKeaFramesGetSlotChunk(MdtKeaFrames* f, int slot)
{
    return &Frames(f)->mSlots[slot]->chunk;
}

// Mac 0x4ae886, Win 0x75e620
MdtKeaParameters* MEAPI MdtKeaFramesGetSlotParameters(MdtKeaFrames* f, int slot)
{
    return &Frames(f)->mSlots[slot]->params;
}

// Mac 0x4af6de, Win 0x75f270 ("MdtKeaStep")
// NOTE: the Win build first checks the FLEXlm license of the "Vortex Toolkit" feature (every
// 0x2b3 steps once licensed); the restored code is the licensed path (V0.3; licensing is V6).
void MEAPI MdtKeaStep(MdtKeaFrames* frames)
{
    InternalConstraintFrames* f = Frames(frames);
    if (f->mStepCount++ == 0) {
        if (f->mNumThreads < 1)
            MeInfo(0xc, "MdtKeaStep: Not using threaded version.\n");
        else
            MeInfo(0xc, "MdtKeaStep: Using threaded version.\n");
    }
    if (f->mNumThreads > 0 && f->mNumSlots > 1)
        f->stepThreaded();
    else
        f->stepSerial();
}
