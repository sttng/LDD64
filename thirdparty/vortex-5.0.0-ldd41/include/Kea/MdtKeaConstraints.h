// Vortex 5.0.0 build 800 "vortexldd41" - Kea constraint rows (restored;
// src/Kea/MdtKeaConstraints.cpp, Mac 0x4a6c3b-0x4a7d2a, Win 0x790ca0-0x791eb0).
//
// One per Kea work slot (0xb8 bytes on both: only ints and pointers). MdtKeaAddPartition fills it
// through the MdtBcl row builders: a builder writes its rows at `numRows` (via an MdtBclRows,
// Mdt/MdtBclRows.h), then MdtKeaAddConstraint copies the Jacobian rows and links the constraint
// into the per-body lists. Kea/MdtKeaRows.h has the prefix the builders see and the row flags.
//
// Terminology (names inferred): a "block" is one constraint; an "entry" is one (constraint,
// body) pair of a block (a block has 1..6 entries, one per body that is not the world); a
// "row" is one scalar constraint equation (a block has numRows rows, every entry of the block
// has its own Jacobian rows, 6 doubles each: 3 linear, 3 angular).
#ifndef KEA_MDTKEACONSTRAINTS_H
#define KEA_MDTKEACONSTRAINTS_H

#include "Kea/MdtKea.h"
#include "Me/MeMemory.h"
#include "Me/MeIntMap.h"
#include "Kea/MdtKeaRows.h"

struct MdtKeaConstraints {
    MeReal* error;          // +0x00 per row: position error the row corrects (MdtKeaGetConstraintForces
                            //   reports it as lambda2)
    MeReal* velocity;       // +0x04 per row: desired relative velocity
    MeReal* lo;             // +0x08 per row: lower force bound
    MeReal* hi;             // +0x0c per row: upper force bound
    MeReal* lambda;         // +0x10 per row: the solution (LCP x)
    MeReal* epsilon;        // +0x14 per row: compliance (diagonal of the LCP matrix)
    MeReal* gamma;          // +0x18 per row: error reduction factor
    int* constraintType;    // +0x1c per block: MdtBclConstraint::type of the block
    int* rowFlags;          // +0x20 per row: MdtKeaRowFlags
    int lcpSteps;           // +0x24 LCP pivoting steps accumulated by VindsSolveLcp
    int* lcpState;          // +0x28 per row: LCP index set state (Kea/KeaMath.h kLcp*)
    int numFrictionRows;    // +0x2c rows with Coulomb bounds (set by the contact rows; reset by
                            //   Reserve / Allocate); enables the friction refinement passes
    int m30;                // +0x30
    int* frictionNormal;    // +0x34 per row: row - its normal row for friction rows, else -1
    MeReal* frictionCoefficient; // +0x38 per row: Coulomb mu (Allocate fills it with NaN bytes)
    MeReal* cohesion;       // +0x3c per row
    int numRows;            // +0x40
    int numBlocks;          // +0x44
    int numEntries;         // +0x48
    int numJacobianRows;    // +0x4c (rows over all entries; MdtKeaConstraintsGetNumRows)
    int* entryNumRows;      // +0x50 per entry
    int* entryRowStart;     // +0x54 per entry: first row of its block
    int* blockFirstEntry;   // +0x58 per block
    int* blockNumEntries;   // +0x5c per block
    int* bodyFirstEntry;    // +0x60 per body (-1)
    int* entryNext;         // +0x64 per entry: next entry of the same body (-1)
    int* bodyLastEntry;     // +0x68 per body (-1)
    MdtKeaBody** entryBody; // +0x6c per entry
    int* entryBodyIndex;    // +0x70 per entry: index of the body in the partition (body map)
    MeReal* entryForce;     // +0x74 per entry: 8 doubles: force[4], torque[4] (J^T lambda)
    MeReal** entryJ;        // +0x78 per entry: its Jacobian rows (6 doubles each) in J
    MeReal** entryJM;       // +0x7c per entry: the same rows times M^-1 in JM
    MeReal* J;              // +0x80 Jacobian storage (6 doubles per entry row)
    MeReal* JM;             // +0x84
    int entryCapacity;      // +0x88 (6 per block)
    int jacobianCapacity;   // +0x8c (6 per row)
    int blockCapacity;      // +0x90
    int bodyCapacity;       // +0x94
    int rowCapacity;        // +0x98 (multiple of 12)
    int maxBodyIndex;       // +0x9c
    MeReal* rhs;            // +0xa0 per row: the LCP right hand side b
    int refinePasses;       // +0xa4 friction refinement passes of the last solve (name inferred)
    MeReal refineRatio;     // +0xa8 last friction refinement ratio (name inferred)
    int grouped;            // +0xb0 MdtKeaConstraintsSetGrouped
    MeIntMap* bodyMap;      // +0xb4 MdtKeaConstraintsSetBodyMap: keaBody -> partition index
};

// Mac 0x4a71c7, Win 0x791360: a zeroed MdtKeaConstraints from api->createZeroed (name inferred)
MdtKeaConstraints* MEAPI MdtKeaConstraintsCreate(MeMemoryAPIStruct* api);
// Mac 0x4a6c3b, Win 0x790ca0: grows the arrays for (at least) these sizes (name inferred)
void MEAPI MdtKeaConstraintsReserve(MdtKeaConstraints* c, int numBlocks, int numBodies, int numRows,
                                    MeMemoryAPIStruct* api);
// Mac 0x4a70a6, Win 0x791060 (name inferred)
void MEAPI MdtKeaConstraintsFree(MdtKeaConstraints* c, MeMemoryAPIStruct* api);
// Mac 0x4a778a, Win 0x791540: MdtKeaAddConstraint, see Kea/MdtKeaRows.h
// Mac 0x4a71f1, Win 0x791880 (+ 0x791720 per entry): JM = J * M^-1 (invM: 12 doubles per body,
// Kea/KeaMath.h) (name inferred)
void MEAPI MdtKeaConstraintsComputeJM(MdtKeaConstraints* c, const MeReal* invM);
// Mac 0x4a734e, Win 0x791970 (+ 0x791900 per entry): y = beta y + alpha J x, x = the body vector
// at byte offset `bodyField` of every entry's MdtKeaBody (4 doubles linear, 4 angular) (name inferred)
void MEAPI MdtKeaConstraintsMultiplyBodies(MeReal alpha, const MdtKeaConstraints* c,
                                           MeReal* const* entryRows, int unused, int bodyField,
                                           MeReal beta, MeReal* y, int incy);
// Win 0x791c80 (+ 0x791a00 per entry), Win only (no caller in either build) (name inferred):
// the transposed product, body vector at byte offset bodyField of every entry's body =
// beta * itself + alpha rows^T x (x advances by the rows of each block)
void MdtKeaConstraintsMultiplyTransposed(MeReal alpha, const MdtKeaConstraints* c,
                                         MeReal* const* entryRows, const MeReal* x, int unused,
                                         MeReal beta, int unused2, int bodyField);
// Mac 0x4a7b25, Win 0x791d10 (+ 0x791b60 per entry): entryForce = J^T lambda per entry and
// body constraintForce / constraintTorque += scale * entryForce (name inferred)
void MEAPI MdtKeaConstraintsAddForces(MdtKeaConstraints* c, MeReal scale);
// Win only accessors (no callers): Win 0x791e50 refineRatio, 0x791e60 refinePasses,
// 0x791eb0 grouped (names inferred)
MeReal MEAPI MdtKeaConstraintsGetRefineRatio(const MdtKeaConstraints* c);
int MEAPI MdtKeaConstraintsGetRefinePasses(const MdtKeaConstraints* c);
int MEAPI MdtKeaConstraintsGetGrouped(const MdtKeaConstraints* c);

#endif
