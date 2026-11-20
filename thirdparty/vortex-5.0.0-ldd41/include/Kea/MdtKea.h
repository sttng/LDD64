// Vortex 5.0.0 build 800 "vortexldd41" - the Kea solver interface Mdt uses
// (restored header; Kea sources: src/Kea).
//
// Mdt hands Kea three things:
//  * MdtKeaBody: the solver part of an MdtBody (MdtBody + 0x140, Mac 0x3ac / Win 0x3b0 bytes;
//    MdtBodyCreateFromPool stores the size in MdtKeaBody::size). Kea integrates it in place.
//  * MdtKeaParameters: the per-step solver settings (world + 0x1b0, copied into every work slot,
//    Mac 0x7c / Win 0x90 bytes; defaults Mac 0x4ae07a / Win 0x75e460).
//  * the constraint rows: per partition, MdtPartition's MdtKeaAddPartition (Mac 0x49178e) runs
//    every constraint's row builder (MdtConstraint::rowBuilder, MdtBcl) into a
//    MdtKeaConstraints set obtained from the Kea frames object (world + 0x100).
// Only what Mdt touches is declared here. All names are inferred (no Kea strings name them)
// unless noted.
#ifndef KEA_MDTKEA_H
#define KEA_MDTKEA_H

#include "Me/MeMemory.h"
#include "Me/MeChunk.h"
#include "Me/MeIntMap.h"
#include "Me/MeCall.h"

typedef double MeReal;

// ---- MdtKeaBody (MdtBody + 0x140) -----------------------------------------------------------
// Offsets: Mac / Win relative to the MdtKeaBody (add 0x140 for the MdtBody offset). Win differs
// from Mac by one 8-byte alignment pad after `flags`.
enum MdtKeaBodyFlags {
    kMdtKeaBodyFlagUseFastSpin = 0x01,     // set by MdtBodySetFastSpinAxis (Win 0x730930)
    kMdtKeaBodyFlagHasConstraints = 0x02,  // body takes part in a constraint (MdtKeaAddPartition)
    kMdtKeaBodyFlagGyroscopic = 0x04,      // Win 0x730a00 / 0x730a30 (name inferred)
    kMdtKeaBodyFlagFrozen = 0x10,          // MdtBodyFreeze / MdtBodyUnfreeze
    kMdtKeaBodyFlagDisabled = 0x20         // MdtBodyDisable (kinematic) / MdtBodyEnable
};

struct MdtKeaBody {
    int index;                // +0x000 (MdtBody +0x140): 100 when created, -1 when freed
    int size;                 // +0x004: sizeof(MdtKeaBody) (Mac 0x3ac, Win 0x3b0)
    MeReal invMass;           // +0x008
    MeReal linearDamping;     // +0x010 (MdtBodyGetLinearVelocityDamping Mac 0x48aeba)
    MeReal angularDamping;    // +0x018
    int flags;                // +0x020 MdtKeaBodyFlags
    MeReal force[4];          // +0x024 / Win +0x028: force applied this step (MdtBodyAddForce)
    MeReal torque[4];         // +0x044 / Win +0x048
    MeReal I[3][4];           // +0x064 / +0x068: inertia tensor (body frame); I[0][3] unused
    MeReal invI[3][4];        // +0x0c4 / +0x0c8
    MeReal Iworld[3][4];      // +0x124 / +0x128: world-frame copies; Iworld[0][3] = 1 / mass
    MeReal invIworld[3][4];   // +0x184 / +0x188; invIworld[0][3] = mass (MdtBodySetMass)
    MeReal vel[4];            // +0x1e4 / +0x1e8 (MdtBody +0x324 / +0x328) linear velocity
    MeReal velrot[4];         // +0x204 / +0x208 (MdtBody +0x344) angular velocity
    MeReal qrot[4];           // +0x224 / +0x228 (MdtBody +0x364) orientation quaternion (w,x,y,z)
    MeReal accel[4];          // +0x244 / +0x248 (MdtBody +0x384) linear acceleration
    MeReal accelrot[4];       // +0x264 / +0x268 (MdtBody +0x3a4) angular acceleration
    MeReal fastSpin[4];       // +0x284 / +0x288 (MdtBody +0x3c4) fast spin axis, default (0,1,0)
    MeReal m2a4[4];           // +0x2a4 / +0x2a8 (MdtBody +0x3e4) cleared by MdtBodyResetForces
    MeReal m2c4[4];           // +0x2c4 / +0x2c8 (MdtBody +0x404) cleared by MdtBodyResetForces
                              //   (VxPart slot 40 reads it "acceleration")
    MeReal comTM[4][4];       // +0x2e4 / +0x2e8 (MdtBody +0x424) world transform of the centre
                              //   of mass (position in row 3)
    MeReal constraintForce[4];  // +0x364 / +0x368 (MdtBody +0x4a4) cleared every step
    MeReal constraintTorque[4]; // +0x384 / +0x388 (MdtBody +0x4c4)
    int partitionIndex;       // +0x3a4 / +0x3a8 (MdtBody +0x4e4): partition of the last step
    int grouping;             // +0x3a8 / +0x3ac (MdtBody +0x4e8): MdtWorld grouping index
                              //   (MdtBodySetGrouping Mac 0x48b374)
};

// ---- MdtKeaParameters (world + 0x1b0, Mac 0x7c / Win 0x90) ----------------------------------
// The LCP settings are an embedded struct: its double makes it 8-aligned on Win, hence Win's
// extra word at +0x14. Defaults: MdtKeaParametersReset.
struct MdtKeaLcpParameters {  // Mac +0x14 / Win +0x18 of MdtKeaParameters (Mac 0x24 / Win 0x28)
    int solver;               // +0x00 index into the LCP functions ("MdtKeaStep: no such lcp solver")
    int maxIterations;        // +0x04 (35) principal pivoting steps (name inferred)
    int cycleCheckStart;      // +0x08 (20) after this many steps cycling is assumed (name inferred)
    MeReal tolerance;         // +0x0c / Win +0x10 (0.001) bound / complementarity tolerance (name inferred)
    MeReal m14;               // +0x14 / Win +0x18 (1.01) not read by the LCPs of this build
    int useHashTable;         // +0x1c / Win +0x20 (1) hash the index sets to detect cycles (name inferred)
    int warmStart;            // +0x20 / Win +0x24 (0) keep the previous row states (name inferred)
};

struct MdtKeaParameters {
    MeReal stepSize;          // +0x00 (default -1; MdtWorldStep writes the partition's h)
    int maxIterations;        // +0x08 (5) (name inferred)
    int stepperModel;         // +0x0c ("MdtKeaStep : no such model number ... stepper_model")
    int m10;                  // +0x10
    MdtKeaLcpParameters lcp;  // +0x14 / Win +0x18
    int refineIterations;     // +0x38 / +0x40 (1) friction refinement passes (name inferred)
    MeReal refineTolerance;   // +0x3c / +0x48 ((double)1e-4f) (name inferred)
    int m44;                  // +0x44 / +0x50 (0)
    int m48;                  // +0x48 / +0x54 (0)
    int itrMaxIterations;     // +0x4c / +0x58 (20) itr_constraint_solver sweeps (name inferred)
    MeReal itrTolerance;      // +0x50 / +0x60 ((double)0.001f) (name inferred)
    void* workspace;          // +0x58 / +0x68 MeChunkGetMem result (MdtWorld step work unit)
    int m5c;                  // +0x5c / +0x6c
    int workspaceSize;        // +0x60 / +0x70
    int m64;                  // +0x64 / +0x74
    int cpuResource;          // +0x68 / +0x78 ("MdtKeaStep: no such CPU resource")
    int linearSolver;         // +0x6c / +0x7c (1) ("no such Linear Solver, see VindLinearSolver")
    int constraintSolver;     // +0x70 / +0x80 (0 = std, 1 = itr) (name inferred)
    void* owned;              // +0x74 / +0x84 the work slot's KeaSolverSet (Kea/KeaFrames.h), made by
                              //   MdtKeaParametersCreateSolvers, freed by MdtKeaParametersDestroy;
                              //   MdtWorld copies it from MdtKeaFramesGetSlotM0c
    int m78;                  // +0x78 / +0x88
};

// Kea objects, opaque to Mdt (defined in Kea/KeaFrames.h, Kea/MdtKeaConstraints.h).
struct MdtKeaFrames;        // world + 0x100: InternalConstraintFrames (RTTI class, Mac 0x4af759)
struct MdtKeaConstraints;   // one per work slot: the rows of one partition

// Kea's view of one constraint (MdtConstraint + 0x1f4 / Win + 0x1f8): MdtBclConstraint, see
// Mdt/MdtTypes.h. A row builder returns 0 when it added no rows (constraint dropped).
struct MdtBclConstraint;
struct MdtBclSolverParameters;
typedef int (MEAPI* MdtBclAddConstraintFn)(MdtKeaConstraints* rows, MdtBclConstraint* c,
                                     const MdtBclSolverParameters* params);

// ---- frames (Mac TU 556) --------------------------------------------------------------------
MdtKeaFrames* MEAPI MdtKeaFramesCreate(MeMemoryAPIStruct* api);               // Mac 0x4af759, Win 0x75f9a0
void MEAPI MdtKeaFramesDestroy(MdtKeaFrames* f);                               // Mac 0x4ae418, Win 0x75ed00
int MEAPI MdtKeaFramesGetNumThreads(const MdtKeaFrames* f);                    // Mac 0x4ae058 (+0x28), Win 0x75e240 (+0x34)
int MEAPI MdtKeaFramesGetM2c(const MdtKeaFrames* f);                           // Mac 0x4ae064 (+0x2c)
// Mac 0x4ae665, Win 0x75e840 (stdcall; its slot-growing helper Win 0x75e740 takes EBX = frames,
// ECX = count): (re)builds the work slots [from, to) for the partitions (counts and index tables
// of MdtPartitionOutput, by partition / work slot).
void MEAPI MdtKeaFramesSetupSlots(MdtKeaFrames* f, int from, int to, const int* constraintsSize,
                                  const int* bodiesSize, const int* numRows,
                                  MdtKeaBody* const* keaBodies, const int* bodiesStart,
                                  const int* order);
MdtKeaConstraints* MEAPI MdtKeaFramesGetConstraints(MdtKeaFrames* f, int slot); // Mac 0x4ae652, Win 0x75e580
void MEAPI MdtKeaFramesSetSlotMaxIterations(MdtKeaFrames* f, int slot, int n);  // Mac 0x4ae637, Win 0x75e5e0 (+0xac = n + 1)
MeChunk* MEAPI MdtKeaFramesGetSlotChunk(MdtKeaFrames* f, int slot);             // Mac 0x4ae621, Win 0x75e600
MdtKeaParameters* MEAPI MdtKeaFramesGetSlotParameters(MdtKeaFrames* f, int slot); // Mac 0x4ae886, Win 0x75e620
void* MEAPI MdtKeaFramesGetSlotM0c(MdtKeaFrames* f, int slot);                  // Mac 0x4ae871, Win 0x75e5c0
                              // (returns an int in both binaries; PORT64: the pointer itself)
void MEAPI MdtKeaStep(MdtKeaFrames* f);                                         // Mac 0x4af6de, Win 0x75f270 (name: string)
// NOTE (Win): the worker count (frames +0x34) is the CPU count (GetSystemInfo, Win 0x75d090);
// with > 0 workers and more than one slot MdtKeaStep runs the slots on the VxFrame thread pool
// (Win 0x75ed30 -> 0x70dd40), else serially (Win 0x75e900).
int MEAPI MdtKeaGetThreadingMode(void);                                         // Mac 0x4ae070, Win 0x75e450 (returns 0)

// ---- parameters ----------------------------------------------------------------------------
void MEAPI MdtKeaParametersReset(MdtKeaParameters* p);                          // Mac 0x4ae07a, Win 0x75e460
void MEAPI MdtKeaParametersInit(MdtKeaParameters* p);                           // Mac 0x4ae14c, Win 0x75e4f0
void MEAPI MdtKeaParametersDestroy(MdtKeaParameters* p);                        // Mac 0x4ae2c8, Win 0x75eb80
int MEAPI MdtKeaParametersCheck(MdtKeaParameters* p);                           // Win 0x75e1a0 (Win-only callers)

// ---- constraint rows (Mac TU 556 0x4a71dc..) -------------------------------------------------
void MEAPI MdtKeaConstraintsSetBodyMap(MdtKeaConstraints* c, MeIntMap* map);   // Mac 0x4a7473 (+0xb4), Win 0x791e70
void MEAPI MdtKeaConstraintsSetGrouped(MdtKeaConstraints* c, int grouped);     // Mac 0x4a7484 (+0xb0), Win 0x791e90
void MEAPI MdtKeaConstraintsAllocate(MdtKeaConstraints* c, int rows, int numConstraints,
                               int numBodies);                           // Mac 0x4a7495, Win 0x791180
void MEAPI MdtKeaConstraintsBeginAdd(MdtKeaConstraints* c);                    // Mac 0x4a71ec (empty), Win 0x791530
void MEAPI MdtKeaConstraintsEndAdd(MdtKeaConstraints* c);                      // Mac 0x4a71e7 (empty), Win 0x791520
int MEAPI MdtKeaConstraintsGetNumRows(const MdtKeaConstraints* c);             // Mac 0x4a71dc (+0x4c), Win 0x791500
// Mac 0x4a798b, Win 0x791380: copies the solved forces of constraint `index` back into the
// constraint (forces / torques per body, row lambdas, row status).
void MEAPI MdtKeaGetConstraintForces(MdtKeaConstraints* c, int index, MdtKeaBody* const* keaBodies,
                               int numBodies, MeReal (*force)[4], MeReal (*torque)[4],
                               MeReal* lambda, MeReal* lambda2, int* rowStatus);

#endif
