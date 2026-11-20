// Vortex 5.0.0 build 800 "vortexldd41" - the part of Kea's constraint-row set that the MdtBcl row
// builders write (restored header; the full struct: Kea/MdtKeaConstraints.h).
// MdtKeaConstraints starts with these per-row arrays and the row count (same offsets on both). A
// row builder fills rows from MdtBclRows (Mdt/MdtBclRows.h) and hands the Jacobian blocks to
// MdtKeaAddConstraint. Offsets from the builders (Mac 0x496e2a, Win 0x736f60 ...),
// MdtKeaConstraintsAllocate (Mac 0x4a7495, Win 0x791180) and MdtKeaAddConstraint (Mac 0x4a778a,
// Win 0x791540). Names inferred.
#ifndef KEA_MDTKEAROWS_H
#define KEA_MDTKEAROWS_H

#include "Kea/MdtKea.h"
#include "Me/MeCall.h"

struct MdtBclRows;

// Row flags (MdtKeaConstraintRows::rowFlags; Bcl writes bits 1-5, names inferred)
enum MdtKeaRowFlags {
    kMdtKeaRowLinear = 0x02,      // translational Jacobian row
    kMdtKeaRowAngular = 0x04,     // rotational Jacobian row (mask 0x06 = the kind)
    kMdtKeaRowBilateral = 0x08,   // equality / limit row (mask 0x18 = the class)
    kMdtKeaRowFriction = 0x10,    // bounded friction / lock row
    kMdtKeaRowRelaxed = 0x20      // epsilon / gamma already set by the builder
};

struct MdtKeaConstraintRows {
    MeReal* xi;             // +0x00 per row: position error (constraint violation)
    MeReal* c;              // +0x04 per row: velocity term (motor speed, restitution ...)
    MeReal* lo;             // +0x08 per row: lower force bound (-DBL_MAX)
    MeReal* hi;             // +0x0c per row: upper force bound (DBL_MAX)
    MeReal* m10;            // +0x10 per row (not written by Bcl)
    MeReal* epsilon;        // +0x14 per row: relaxation (compliance term)
    MeReal* gamma;          // +0x18 per row: relaxation (damping term)
    int* constraintType;    // +0x1c per constraint: MdtBclConstraint::type
    int* rowFlags;          // +0x20 per row: MdtKeaRowFlags
    int m24;                // +0x24 (MdtKeaConstraints::lcpSteps; PORT64: int, as there)
    int* m28;               // +0x28 per row (int)
    int numCoupled;         // +0x2c friction rows coupled to the normal force (contacts)
    int m30;                // +0x30 (MdtKeaConstraints::m30; PORT64: int, as there)
    int* coupledRow;        // +0x34 per row: constraint-relative row index of a coupled row
    MeReal* coupledScale;   // +0x38 per row: friction coefficient of a coupled row
    MeReal* m3c;            // +0x3c per row
    int numRows;            // +0x40 rows added so far
};

// Mac 0x4a778a, Win 0x791540 (stdcall, 7 arguments): appends one constraint - numRows rows of
// `rows` (the Jacobian blocks rows->jacobian[i] of every body i with a Kea body) - and records
// its type. `unused` is always 0. (name inferred)
void MEAPI MdtKeaAddConstraint(MdtKeaConstraints* c, const MdtBclRows* rows,
                               MdtKeaBody* const* keaBodies, int numBodies, int numRows,
                               int type, int unused);

#endif
