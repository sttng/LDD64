// Vortex 5.0.0 build 800 "vortexldd41" - Kea steppers, constraint solvers, the solver set of a
// work slot and the frames object behind the MdtKea API (restored).
//  * src/Kea/KeaSteppers.cpp  WorldStepper / kea_model / StableStepper (Mac 0x4a7d2a-0x4a80fb,
//                             0x4adcaa-0x4adfd2, 0x4b11fa-0x4b14d8; Win 0x7b90b0-0x7b9760),
//                             constraint_solver / std_constraint_solver (Mac 0x4a5de0-0x4a5e86,
//                             0x4af8f6; Win 0x75e110, 0x7b7b20-0x7b7bc0)
//  * src/Kea/KeaItrSolver.cpp itr_constraint_solver (Mac 0x4a5e86-0x4a6c3b, Win 0x7b9d00-0x7bad50)
//  * src/Kea/KeaFrames.cpp    InternalConstraintFrames, the work units, the MdtKea API (Mac
//                             0x4adfd2-0x4b02dd, Win 0x75e110-0x760700)
// RTTI names: WorldStepper, kea_model, StableStepper, constraint_solver, std_constraint_solver,
// itr_constraint_solver, VindsConstraintFrames, InternalConstraintFrames,
// Setup/Update/IntegratePartitionWorkUnit. Method and field names are inferred.
// No class here has a virtual destructor except the frames and the work units.
#ifndef KEA_KEAFRAMES_H
#define KEA_KEAFRAMES_H

#include "Kea/MdtKea.h"
#include "Kea/KeaMath.h"
#include "Kea/LCPMatrix.h"
#include "Kea/MemoryPool.h"
#include "Vx/VxThreadPool.h"

#include <vector>

struct MdtKeaConstraints;

// ---- constraint solvers ---------------------------------------------------------------------
// Mac vtable 0x75f218 / Win 0x9fb81c (0x24 bytes)
class constraint_solver {
public:
    constraint_solver()                              // (Mac / Win: inlined in the solver set)
        : mN(0), mConstraints(0), mParams(0), mMatrix(0), mLcp(0), mPool(0), mBodies(0), mNumBodies(0) {}
    // slot 0, Mac 0x4af8f6, Win 0x75e110: stores the arguments
    virtual void setup(int n, LCPMatrix* matrix, MdtKeaConstraints* c, MdtKeaParameters* p, KeaLcpFunction lcp,
                       MemoryPool* pool, MdtKeaBody* const* bodies, int numBodies, int unused);
    virtual void solve() = 0;                        // slot 1
    virtual void cleanup() = 0;                      // slot 2

    int mN;                         // +0x04 rows
    MdtKeaConstraints* mConstraints;// +0x08
    MdtKeaParameters* mParams;      // +0x0c
    LCPMatrix* mMatrix;             // +0x10
    KeaLcpFunction mLcp;            // +0x14
    MemoryPool* mPool;              // +0x18
    MdtKeaBody* const* mBodies;     // +0x1c
    int mNumBodies;                 // +0x20
};

// Mac vtable 0x75ef68 / Win 0x9fb82c: one LCP over all rows (VindsSolveLcp)
class std_constraint_solver : public constraint_solver {
public:
    // Mac 0x4a5de0, Win 0x7b7b20: + allocate, build and factorize the matrix
    virtual void setup(int n, LCPMatrix* matrix, MdtKeaConstraints* c, MdtKeaParameters* p, KeaLcpFunction lcp,
                       MemoryPool* pool, MdtKeaBody* const* bodies, int numBodies, int unused);
    virtual void solve();                            // Mac 0x4a5e4e, Win 0x7b7b80
    virtual void cleanup();                          // Mac 0x4a5e80, Win 0x7076a0 (empty)
};

// Mac vtable 0x75ef88 / Win 0xa01084: block Gauss-Seidel over the constraints, each block a
// small LCP on an smv_matrix (MdtKeaParameters::constraintSolver = 1; not used by LDD:
// V0.3 found its solve (Win 0x7ba8d0) never runs)
class itr_constraint_solver : public constraint_solver {
public:
    itr_constraint_solver();                         // Mac 0x4a5f22 -> 0x4a5e86, Win 0x7ba880
    ~itr_constraint_solver() {}                      // Mac 0x4a5f50 -> 0x4a5f36 (smv_matrix part)
    // Mac 0x4a6abc, Win 0x7b9dd0: one smv_matrix per block ("block matrices")
    virtual void setup(int n, LCPMatrix* matrix, MdtKeaConstraints* c, MdtKeaParameters* p, KeaLcpFunction lcp,
                       MemoryPool* pool, MdtKeaBody* const* bodies, int numBodies, int unused);
    virtual void solve();                            // Mac 0x4a5fc0, Win 0x7ba8d0
    virtual void cleanup();                          // Mac 0x4a5f64, Win 0x7b9f80

    smv_matrix mPrototype;          // +0x24 (copied into every block matrix)
    smv_matrix* mBlockMatrices;     // +0x188c ("block matrices")
    MeReal* mBodyForce;             // +0x1890 ("body force", 8 doubles per body)
    MeReal* mW;                     // +0x1894 ("w")
    int mNumBlocks;                 // +0x1898
};

// ---- steppers -------------------------------------------------------------------------------
// Mac vtable 0x75f2a8 / Win 0xa00cec (Mac 0x44 bytes; Win has one more word at +4)
class WorldStepper {
public:
    WorldStepper();                                  // Mac 0x4b11fa, Win 0x7b90b0
    virtual void resetIterationCount();              // slot 0, Mac 0x4b127a, Win 0x7b90f0
    // slot 1: bodies -> momenta / inverse masses, the right hand side, solver->setup
    virtual void setup(int numBodies, MdtKeaConstraints* c, MdtKeaParameters* p, MdtKeaBody* const* bodies,
                       KeaLcpFunction lcp, LCPMatrix* matrix, constraint_solver* solver, MemoryPool* pool) = 0;
    virtual void slot2();                            // slot 2, Mac 0x4b12c6, Win 0x7b91a0 (empty)
    virtual void solve() = 0;                        // slot 3
    virtual void computeAccelerations();             // slot 4, Mac 0x4b14b4, Win 0x7b91b0
    virtual void integrate();                        // slot 5, Mac 0x4b1430, Win 0x7b91d0
    virtual void step();                             // slot 6, Mac 0x4b128a, Win 0x7b9160
    virtual void prepareIteration();                 // slot 7, Mac 0x4b136c, Win 0x7b9280
    virtual void addConstraintForces();              // slot 8, Mac 0x4b133a, Win 0x7b9310
    virtual void cleanup();                          // slot 9, Mac 0x4b12cc, Win 0x7b9230
    virtual const char* getName() = 0;               // slot 10

    int m04;                        // +0x04
    MeReal* mRhs0;                  // +0x08 ("rhs0")
    MdtKeaConstraints* mConstraints;// +0x0c
    MdtKeaParameters* mParams;      // +0x10 (its stepSize is the h of the Vinds calls)
    MdtKeaBody* const* mBodies;     // +0x14
    LCPMatrix* mMatrix;             // +0x18
    int mNumBodies;                 // +0x1c
    MeReal mH;                      // +0x20
    MeReal mInvH;                   // +0x28
    KeaLcpFunction mLcp;            // +0x30
    MeReal* mInvM;                  // +0x34 VindsComputeInverseMass*
    MemoryPool* mPool;              // +0x38
    constraint_solver* mSolver;     // +0x3c
    int mIteration;                 // +0x40
};

// Mac vtable 0x75efc8 / Win 0x9fb8ac
class kea_model : public WorldStepper {
public:
    virtual void setup(int numBodies, MdtKeaConstraints* c, MdtKeaParameters* p, MdtKeaBody* const* bodies,
                       KeaLcpFunction lcp, LCPMatrix* matrix, constraint_solver* solver, MemoryPool* pool);
                                                     // Mac 0x4a7e2e, Win 0x7b9560
    virtual void solve();                            // Mac 0x4a7d3c, Win 0x7b9720
    virtual const char* getName();                   // Mac 0x4a7d2a, Win 0x7b9550
};

// Mac vtable 0x75f0c8 / Win 0x9fb87c (the default: MdtKeaParameters::stepperModel = 0)
class StableStepper : public WorldStepper {
public:
    virtual void setup(int numBodies, MdtKeaConstraints* c, MdtKeaParameters* p, MdtKeaBody* const* bodies,
                       KeaLcpFunction lcp, LCPMatrix* matrix, constraint_solver* solver, MemoryPool* pool);
                                                     // Mac 0x4adcdc, Win 0x7b9340
    virtual void solve();                            // Mac 0x4adcbc, Win 0x7b9530
    virtual const char* getName();                   // Mac 0x4adcaa, Win 0x7b9330
};

// ---- the solver set of a work slot (MdtKeaParameters::owned) ---------------------------------
// Mac 0x1a34 / Win 0x1a58 bytes, MeMemoryAPI.create; made by MdtKeaParametersCreateSolvers
// (Mac 0x4ae43d, Win 0x75e6a0 / inlined in 0x75e740 -> ctor 0x75faa0), freed by
// MdtKeaParametersDestroy. (name inferred)
struct KeaSolverSet {
    KeaSolverSet();                                  // Win 0x75faa0
    ~KeaSolverSet();                                 // Win 0x75fa20

    MemoryPool* pool;               // [0x00] MeMemoryAPI.create(0x2c)
    LCPMatrix* matrices[6];         // [0x01] by cpuResource + linearSolver: 0 vbChol, 1 bvbCholRRT
    WorldStepper* steppers[5];      // [0x07] by stepperModel: 0 StableStepper, 1 kea_model
    KeaLcpFunction lcps[3];         // [0x0c] by lcp.solver: 0 lcp_simple_hash_table
    constraint_solver* solvers[5];  // [0x0f] by constraintSolver: 0 std, 1 itr
    vbChol vbcholMatrix;            // [0x14]
    bvbCholRRT bvbMatrix;           // [0x24]
    StableStepper stableStepper;    // [0x37] / Win [0x38]
    kea_model keaModel;             // [0x48] / Win [0x4c]
    std_constraint_solver stdSolver;// [0x59] / Win [0x60]
    itr_constraint_solver itrSolver;// [0x62] / Win [0x6a]
    int numMatrices;                // [0x689] (2)
    int numSteppers;                // [0x68a] (2)
    int numLcps;                    // [0x68b] (1)
    int numSolvers;                 // [0x68c] (2)
};

// One work slot (Mac 0xb4 / Win 0xc8 bytes, operator new). (name inferred)
struct KeaSlot {
    MdtKeaConstraints* constraints; // +0x00
    MdtKeaBody* const* bodies;      // +0x04
    int numBodies;                  // +0x08
    KeaSolverSet* solvers;          // +0x0c (= params.owned)
    MdtKeaParameters params;        // +0x10
    MeChunk chunk;                  // Mac +0x8c / Win +0xa0 (the MemoryPool buffer)
    int maxIterations;              // Mac +0xac / Win +0xc0: iterations + 1
    int iteration;                  // Mac +0xb0 / Win +0xc4

    void setConstraints(MdtKeaConstraints* c) { constraints = c; } // Mac 0x4adfd2, Win 0x7b9c50
    void setSolvers(KeaSolverSet* s) { solvers = s; } // Mac 0x4adfe0, Win 0x7b9c60
    WorldStepper* stepper() const;                   // Mac 0x4adfee, Win 0x7b9c70
    LCPMatrix* const* matrix() const;                // Mac 0x4ae028, Win 0x7b9ca0
    constraint_solver* solver() const;               // Mac 0x4ae000, Win 0x7b9c80
    KeaLcpFunction lcp() const;                      // Mac 0x4ae016, Win 0x7b9c90
    bool isUngrouped() const;                        // Mac 0x4ae040, Win 0x7b9cc0
    void setupStepper();                             // the SetupPartitionWorkUnit body (name inferred)
};

// ---- work units (VxThreadPool::WorkUnit, 12 bytes) -------------------------------------------
class SetupPartitionWorkUnit : public Vx::VxThreadPool::WorkUnit {
public:
    SetupPartitionWorkUnit() : index(-1), slot(0) {}
    virtual void execute(int);                       // Mac 0x4ae218, Win 0x75e350
    virtual void execute() {}                        // Mac 0x4af92e / Win 0x7076a0
    virtual void cancel() {}                         // Mac 0x4af934 / Win 0x7076a0
    int index;
    KeaSlot* slot;
};
class UpdatePartitionWorkUnit : public Vx::VxThreadPool::WorkUnit {
public:
    UpdatePartitionWorkUnit() : index(-1), slot(0) {}
    virtual void execute(int);                       // Mac 0x4ae1e2, Win 0x75e3f0
    virtual void execute() {}
    virtual void cancel() {}
    int index;
    KeaSlot* slot;
};
class IntegratePartitionWorkUnit : public Vx::VxThreadPool::WorkUnit {
public:
    IntegratePartitionWorkUnit() : index(-1), slot(0) {}
    virtual void execute(int);                       // Mac 0x4af388, Win 0x75e420
    virtual void execute() {}
    virtual void cancel() {}
    int index;
    KeaSlot* slot;
};

// ---- frames (MdtKeaFrames is an InternalConstraintFrames) -------------------------------------
// Mac vtable 0x75f1b8 / Win 0x9fb934
class VindsConstraintFrames {
public:
    VindsConstraintFrames() {}                       // Win 0x7604b0 (inlined)
    virtual ~VindsConstraintFrames();                // Mac 0x4afdca / 0x4afd20, Win 0x760360
    std::vector<VindsConstraintFrames*> mChildren;   // +0x04 (deleted with the frames; always empty)
};

// Mac vtable 0x75f190 / Win 0x9fb93c (Mac 0x70 / Win 0x80 bytes)
class InternalConstraintFrames : public VindsConstraintFrames {
public:
    explicit InternalConstraintFrames(MeMemoryAPIStruct* api); // Mac 0x4af759, Win 0x75f9a0 -> 0x7604b0
    virtual ~InternalConstraintFrames();             // Mac 0x4b0086 / 0x4afe6c, Win 0x760550

    void destroySlots();                             // Mac 0x4ae390, Win 0x75ec70
    void growSlots(int count);                       // Mac inlined in 0x4ae665, Win 0x75e740
    int numSolvableSlots();                          // Win 0x75fbd0 (Mac inlined in 0x4af3be)
    int maxIterations(int count) const;              // Mac 0x4ae160, Win 0x75e640
    int callback(int phase);                         // Mac 0x4ae19a, Win 0x75e670
    void stepSerial();                               // Mac 0x4af3be, Win 0x75e900
    void stepThreaded();                             // Mac 0x4ae89a, Win 0x75ed30

    KeaSlot** mSlots;               // Mac +0x10 / Win +0x1c
    int mCapacity;                  // Mac +0x14 / Win +0x20
    unsigned int mNumSlots;         // Mac +0x18 / Win +0x24
    MeMemoryAPIStruct* mApi;        // Mac +0x1c / Win +0x28
    int (*mCallback)(void* data, int phase); // Mac +0x20 / Win +0x2c: nonzero = skip (phase 0:
                                    //   the whole solve, 1: the integration) (never set in LDD)
    void* mCallbackData;            // Mac +0x24 / Win +0x30
    int mNumThreads;                // Mac +0x28 / Win +0x34 (Vx::VxGetNumberOfProcessors)
    int mM2c;                       // Mac +0x2c / Win +0x38 (5)
    MeReal mWin40;                  // Win +0x40 (Win-only accessors 0x75e2a0 / 0x75e2b0)
    int mStepCount;                 // Mac +0x38 / Win +0x48
    Vx::VxThreadPool* mThreadPool;  // Mac +0x3c / Win +0x4c
    std::vector<SetupPartitionWorkUnit> mSetupUnits;         // Mac +0x40 / Win +0x50
    std::vector<UpdatePartitionWorkUnit> mUpdateUnits;       // Mac +0x50 / Win +0x60
    std::vector<IntegratePartitionWorkUnit> mIntegrateUnits; // Mac +0x60 / Win +0x70
};

// ---- the rest of the API (Win-only or not used by Mdt; names inferred) -----------------------
// Mac 0x4ae43d, Win 0x75e6a0: params->owned = a new KeaSolverSet
void MEAPI MdtKeaParametersCreateSolvers(MdtKeaParameters* p);
// Win 0x75e500: stepSize = h, workspace = MeChunkGetMem(chunk, workspaceSize + m64)
void MEAPI MdtKeaParametersSetWorkspace(MdtKeaParameters* p, MeChunk* chunk, int unused, MeReal h);
const char* MEAPI MdtKeaParametersGetLinearSolverName(const MdtKeaParameters* p); // Win 0x75e2d0
const char* MEAPI MdtKeaParametersGetStepperName(const MdtKeaParameters* p);      // Win 0x75e310
int MEAPI MdtKeaParametersAddStepper(MdtKeaParameters* p, WorldStepper* s);       // Win 0x75e6e0
int MEAPI MdtKeaParametersAddMatrix(MdtKeaParameters* p, LCPMatrix* m);           // Win 0x75e710
int MEAPI MdtKeaFramesSetNumThreads(MdtKeaFrames* f, int n);                      // Win 0x75e250
int MEAPI MdtKeaFramesSetM2c(MdtKeaFrames* f, int n);                             // Win 0x75e280
MeReal MEAPI MdtKeaFramesGetWin40(const MdtKeaFrames* f);                         // Win 0x75e2a0
MeReal MEAPI MdtKeaFramesSetWin40(MdtKeaFrames* f, MeReal v);                     // Win 0x75e2b0
void MEAPI MdtKeaFramesSetSlotBodies(MdtKeaFrames* f, int slot, MdtKeaBody* const* bodies, int n); // Win 0x75e550
int MEAPI MdtKeaFramesGetCapacity(const MdtKeaFrames* f);                         // Win 0x75e5a0
int MEAPI MdtKeaFramesGetNumSlots(const MdtKeaFrames* f);                         // Win 0x75e5b0

#endif
