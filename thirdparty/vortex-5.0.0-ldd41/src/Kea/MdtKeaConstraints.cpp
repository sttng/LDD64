// Vortex 5.0.0 build 800 "vortexldd41" - Kea constraint rows (restored).
// Mac 0x4a6c3b-0x4a7d2a (TU 556), Win 0x790ca0-0x791eb0. See Kea/MdtKeaConstraints.h.
#include "Kea/MdtKeaConstraints.h"
#include "Me/MeIntMap.h"

#include <string.h>

// Mac 0x4a6c3b, Win 0x790ca0
void MEAPI MdtKeaConstraintsReserve(MdtKeaConstraints* c, int numBlocks, int numBodies, int numRows,
                                    MeMemoryAPIStruct* api)
{
    if (numBlocks < c->blockCapacity)
        numBlocks = c->blockCapacity;
    c->blockCapacity = numBlocks;
    if (numBodies < c->bodyCapacity)
        numBodies = c->bodyCapacity;
    c->bodyCapacity = numBodies;
    if (numRows % 12 != 0)
        numRows = (numRows + 12) - numRows % 12;
    c->rowCapacity = numRows;
    c->numFrictionRows = 0;
    c->numRows = 0;
    c->numBlocks = 0;
    c->numEntries = 0;
    c->numJacobianRows = 0;
    if (c->blockCapacity == 0)
        c->constraintType = 0;
    else
        c->constraintType = (int*)api->resize(c->constraintType, c->blockCapacity * sizeof(int));
    if (c->gamma != 0) {
        api->destroyAligned(c->gamma);
        api->destroyAligned(c->epsilon);
        api->destroyAligned(c->lambda);
        api->destroyAligned(c->lcpState);
        api->destroyAligned(c->hi);
        api->destroyAligned(c->lo);
        api->destroyAligned(c->velocity);
        api->destroyAligned(c->error);
        api->destroyAligned(c->frictionNormal);
        api->destroyAligned(c->frictionCoefficient);
        api->destroyAligned(c->cohesion);
        api->destroyAligned(c->entryNumRows);
        api->destroyAligned(c->entryRowStart);
        api->destroyAligned(c->blockFirstEntry);
        api->destroyAligned(c->blockNumEntries);
        api->destroyAligned(c->bodyFirstEntry);
        api->destroyAligned(c->entryNext);
        api->destroyAligned(c->bodyLastEntry);
        api->destroyAligned(c->entryBody);
        api->destroyAligned(c->entryBodyIndex);
        api->destroyAligned(c->entryForce);
        api->destroyAligned(c->entryJ);
        api->destroyAligned(c->entryJM);
        api->destroyAligned(c->J);
        api->destroyAligned(c->JM);
        api->destroyAligned(c->rhs);
        api->destroyAligned(c->rowFlags);
    }
    if (c->blockCapacity > 0) {
        const int ints = c->rowCapacity * 4;
        const int doubles = c->rowCapacity * 8;
        c->error = (MeReal*)api->createAligned(doubles, 0x40);
        c->velocity = (MeReal*)api->createAligned(doubles, 0x40);
        c->lo = (MeReal*)api->createAligned(doubles, 0x40);
        c->hi = (MeReal*)api->createAligned(doubles, 0x40);
        c->lambda = (MeReal*)api->createAligned(doubles, 0x40);
        c->epsilon = (MeReal*)api->createAligned(doubles, 0x40);
        c->gamma = (MeReal*)api->createAligned(doubles, 0x40);
        c->frictionNormal = (int*)api->createAligned(ints, 0x40);
        c->frictionCoefficient = (MeReal*)api->createAligned(doubles, 0x40);
        c->cohesion = (MeReal*)api->createAligned(doubles, 0x40);
        c->lcpState = (int*)api->createAligned(ints, 0x40);
        c->rowFlags = (int*)api->createAligned(ints, 0x40);
        c->rhs = (MeReal*)api->createAligned(doubles, 0x40);
        c->numBlocks = 0;
        c->numEntries = 0;
        c->numJacobianRows = 0;
        c->entryCapacity = c->blockCapacity * 6;
        c->jacobianCapacity = c->rowCapacity * 6;
        c->entryNumRows = (int*)api->createAligned(c->blockCapacity * 0x18, 0x40);
        c->entryRowStart = (int*)api->createAligned(c->entryCapacity * 4, 0x40);
        c->blockNumEntries = (int*)api->createAligned(c->blockCapacity * 4, 0x40);
        c->blockFirstEntry = (int*)api->createAligned(c->blockCapacity * 4, 0x40);
        c->bodyFirstEntry = (int*)api->createAligned(c->bodyCapacity * 4, 0x40);
        c->bodyLastEntry = (int*)api->createAligned(c->bodyCapacity * 4, 0x40);
        c->entryNext = (int*)api->createAligned(c->entryCapacity * 4, 0x40);
        // PORT64: the three pointer arrays take sizeof(pointer) per entry (4 in both binaries)
        c->entryBody = (MdtKeaBody**)api->createAligned(c->entryCapacity * (int)sizeof(*c->entryBody), 0x40);
        c->entryBodyIndex = (int*)api->createAligned(c->entryCapacity * 4, 0x40);
        c->entryForce = (MeReal*)api->createAligned(c->entryCapacity << 6, 0x40);
        c->entryJ = (MeReal**)api->createAligned(c->entryCapacity * (int)sizeof(*c->entryJ), 0x40);
        c->entryJM = (MeReal**)api->createAligned(c->entryCapacity * (int)sizeof(*c->entryJM), 0x40);
        c->J = (MeReal*)api->createAligned(c->jacobianCapacity * 0x30, 0x40);
        c->JM = (MeReal*)api->createAligned(c->jacobianCapacity * 0x30, 0x40);
    }
}

// Mac 0x4a70a6, Win 0x791060
void MEAPI MdtKeaConstraintsFree(MdtKeaConstraints* c, MeMemoryAPIStruct* api)
{
    if (c->constraintType == 0)
        return;
    api->destroy(c->constraintType);
    api->destroyAligned(c->gamma);
    api->destroyAligned(c->epsilon);
    api->destroyAligned(c->lambda);
    api->destroyAligned(c->lcpState);
    api->destroyAligned(c->rowFlags);
    api->destroyAligned(c->hi);
    api->destroyAligned(c->lo);
    api->destroyAligned(c->velocity);
    api->destroyAligned(c->error);
    api->destroyAligned(c->frictionNormal);
    api->destroyAligned(c->frictionCoefficient);
    api->destroyAligned(c->cohesion);
    api->destroyAligned(c->entryNumRows);
    api->destroyAligned(c->entryRowStart);
    api->destroyAligned(c->blockFirstEntry);
    api->destroyAligned(c->blockNumEntries);
    api->destroyAligned(c->bodyFirstEntry);
    api->destroyAligned(c->entryNext);
    api->destroyAligned(c->bodyLastEntry);
    api->destroyAligned(c->entryBody);
    api->destroyAligned(c->entryBodyIndex);
    api->destroyAligned(c->entryForce);
    api->destroyAligned(c->entryJ);
    api->destroyAligned(c->entryJM);
    api->destroyAligned(c->J);
    api->destroyAligned(c->JM);
    api->destroyAligned(c->rhs);
}

// Mac 0x4a71c7, Win 0x791360
MdtKeaConstraints* MEAPI MdtKeaConstraintsCreate(MeMemoryAPIStruct* api)
{
    return (MdtKeaConstraints*)api->createZeroed(sizeof(MdtKeaConstraints));
}

// Mac 0x4a71dc, Win 0x791500
int MEAPI MdtKeaConstraintsGetNumRows(const MdtKeaConstraints* c)
{
    return c->numJacobianRows;
}

void MEAPI MdtKeaConstraintsEndAdd(MdtKeaConstraints*) {}      // Mac 0x4a71e7, Win 0x791520
void MEAPI MdtKeaConstraintsBeginAdd(MdtKeaConstraints*) {}    // Mac 0x4a71ec, Win 0x791530

// Win 0x791720 (inlined on Mac): JM rows of one entry. Linear part times 1/m (invM[3]),
// angular part times the inverse inertia (row vector times matrix).
static void KeaComputeEntryJM(MeReal* jm, const MeReal* j, int numRows, const MeReal* invM)
{
    int i, k;
    for (k = 0; k < 3; ++k)
        for (i = 0; i < numRows; ++i)
            jm[i * 6 + k] = j[i * 6 + k] * invM[3];
    for (k = 0; k < 3; ++k) {
        for (i = 0; i < numRows; ++i) {
            MeReal* d = &jm[i * 6 + 3 + k];
            *d = 0;
            for (int m = 0; m < 3; ++m)
                *d = j[i * 6 + 3 + m] * invM[m * 4 + k] + *d;
        }
    }
}

// Mac 0x4a71f1, Win 0x791880
void MEAPI MdtKeaConstraintsComputeJM(MdtKeaConstraints* c, const MeReal* invM)
{
    for (int e = 0; e != c->numEntries; ++e) {
        // NOTE Mac masks the byte offset with ~0xf, Win divides by 0x30: the same offset
        c->entryJM[e] = c->JM + (c->entryJ[e] - c->J);
        KeaComputeEntryJM(c->entryJM[e], c->entryJ[e], c->entryNumRows[e], invM + c->entryBodyIndex[e] * 12);
    }
}

// Win 0x791900 (inlined on Mac): one entry's rows times its body vector x (4 linear, 4 angular)
static void KeaMultiplyEntry(MeReal alpha, MeReal beta, const MeReal* rows, int numRows, const MeReal* x,
                             MeReal* y)
{
    for (int i = 0; i < numRows; ++i) {
        for (int k = 0; k < 3; ++k)
            y[i] = beta * y[i] + alpha * (rows[i * 6 + k] * x[k] + rows[i * 6 + 3 + k] * x[4 + k]);
    }
}

// Mac 0x4a734e, Win 0x791970
void MEAPI MdtKeaConstraintsMultiplyBodies(MeReal alpha, const MdtKeaConstraints* c,
                                           MeReal* const* entryRows, int, int bodyField,
                                           MeReal beta, MeReal* y, int)
{
    int e = 0;
    for (int b = 0; b != c->numBlocks; ++b) {
        for (int k = 0; k < c->blockNumEntries[b]; ++k, ++e) {
            const MeReal* x = (const MeReal*)((const char*)c->entryBody[e] + bodyField);
            KeaMultiplyEntry(alpha, beta, entryRows[e], c->entryNumRows[e], x, y);
        }
        if (b + 1 == c->numBlocks)
            break;
        y += c->entryNumRows[e - 1];
    }
}

// Win 0x791a00 (Win only): one entry's rows transposed times x into its body vector
static void KeaMultiplyEntryTransposed(MeReal alpha, const MeReal* rows, int numRows, const MeReal* x,
                                       MeReal beta, MeReal* out)
{
    for (int k = 0; k < 3; ++k) {
        for (int i = 0; i < numRows; ++i) {
            out[k] = out[k] * beta + rows[i * 6 + k] * alpha * x[i];
            out[4 + k] = out[4 + k] * beta + rows[i * 6 + 3 + k] * alpha * x[i];
        }
    }
}

// Win 0x791c80 (Win only, no caller)
void MdtKeaConstraintsMultiplyTransposed(MeReal alpha, const MdtKeaConstraints* c,
                                         MeReal* const* entryRows, const MeReal* x, int, MeReal beta,
                                         int, int bodyField)
{
    int e = 0;
    for (int b = 0; b != c->numBlocks; ++b) {
        for (int k = 0; k < c->blockNumEntries[b]; ++k, ++e) {
            MeReal* out = (MeReal*)((char*)c->entryBody[e] + bodyField);
            KeaMultiplyEntryTransposed(alpha, entryRows[e], c->entryNumRows[e], x, beta, out);
        }
        x += c->entryNumRows[e - 1];
    }
}

// Mac 0x4a7473, Win 0x791e70
void MEAPI MdtKeaConstraintsSetBodyMap(MdtKeaConstraints* c, MeIntMap* map)
{
    c->bodyMap = map;
}

// Mac 0x4a7484, Win 0x791e90
void MEAPI MdtKeaConstraintsSetGrouped(MdtKeaConstraints* c, int grouped)
{
    c->grouped = grouped;
}

// Mac 0x4a7495, Win 0x791180
void MEAPI MdtKeaConstraintsAllocate(MdtKeaConstraints* c, int rows, int numConstraints, int numBodies)
{
    c->refineRatio = 0;
    c->maxBodyIndex = 0;
    c->numFrictionRows = 0;
    c->refinePasses = 0;
    c->numRows = 0;
    c->numBlocks = 0;
    c->numEntries = 0;
    c->numJacobianRows = 0;
    if (numConstraints > 0) {
        const size_t doubles = rows * 8;
        const size_t ints = rows * 4;
        memset(c->error, 0, doubles);
        memset(c->velocity, 0, doubles);
        memset(c->lo, 0, doubles);
        memset(c->hi, 0, doubles);
        memset(c->lambda, 0, doubles);
        memset(c->epsilon, 0, doubles);
        memset(c->gamma, 0, doubles);
        memset(c->frictionNormal, 0xff, ints);
        memset(c->frictionCoefficient, 0xff, doubles);
        memset(c->cohesion, 0, doubles);
        memset(c->lcpState, 0, ints);
        memset(c->rowFlags, 0, ints);
        const size_t entries = numConstraints * 0x18;
        memset(c->entryNumRows, 0, entries);
        memset(c->entryRowStart, 0, entries);
        memset(c->blockFirstEntry, 0, numConstraints * 4);
        memset(c->blockNumEntries, 0, numConstraints * 4);
        memset(c->bodyFirstEntry, -1, numBodies * 4);
        memset(c->bodyLastEntry, -1, numBodies * 4);
        memset(c->entryNext, -1, entries);
        // PORT64: the pointer arrays clear 6 pointers per constraint (0x18 bytes in both binaries)
        const size_t pointerEntries = numConstraints * 6 * sizeof(void*);
        memset(c->entryBody, 0, pointerEntries);
        memset(c->entryBodyIndex, 0, entries);
        memset(c->entryForce, 0, numConstraints * 0x180);
        memset(c->entryJ, 0, pointerEntries);
        memset(c->J, 0, rows * 0xc0);
        memset(c->rhs, 0, doubles);
    }
}

// Mac 0x4a798b, Win 0x791380
void MEAPI MdtKeaGetConstraintForces(MdtKeaConstraints* c, int index, MdtKeaBody* const* keaBodies,
                                     int numBodies, MeReal (*force)[4], MeReal (*torque)[4],
                                     MeReal* lambda, MeReal* lambda2, int* rowStatus)
{
    const int first = c->blockFirstEntry[index];
    int found = 0;
    for (int k = 0; k < numBodies; ++k) {
        const int e = found + first;
        if (keaBodies[k] == c->entryBody[e]) {
            memcpy(force[k], &c->entryForce[e * 8], 4 * sizeof(MeReal));
            memcpy(torque[k], &c->entryForce[e * 8 + 4], 4 * sizeof(MeReal));
            ++found;
        } else {
            memset(force[k], 0, 4 * sizeof(MeReal));
            memset(torque[k], 0, 4 * sizeof(MeReal));
        }
    }
    int i = 0;
    const int start = c->entryRowStart[first];
    for (; i < c->entryNumRows[first]; ++i) {
        const int r = start + i;
        if (c->lambda[r] < c->hi[r]) {
            if (c->lambda[r] <= c->lo[r])
                rowStatus[i] |= 5;
        } else {
            rowStatus[i] |= 3;
        }
        lambda[i] = c->lambda[r];
        lambda2[i] = c->error[r];
    }
    lambda[i] = 4294967295.0;   // end marker
}

// Win 0x791b60 (inlined on Mac): J^T lambda of one entry (force[0..2], torque at [4..6])
static void KeaEntryForce(MeReal* f, const MeReal* j, int numRows, const MeReal* lambda)
{
    for (int k = 0; k < 3; ++k) {
        for (int i = 0; i < numRows; ++i) {
            f[k] = j[i * 6 + k] * lambda[i] + f[k];
            f[4 + k] = j[i * 6 + 3 + k] * lambda[i] + f[4 + k];
        }
    }
}

// Mac 0x4a7b25, Win 0x791d10
void MEAPI MdtKeaConstraintsAddForces(MdtKeaConstraints* c, MeReal scale)
{
    const MeReal* lambda = c->lambda;
    int e = 0;
    for (int b = 0; b != c->numBlocks; ++b) {
        for (int k = 0; k < c->blockNumEntries[b]; ++k, ++e) {
            MdtKeaBody* body = c->entryBody[e];
            MeReal* f = &c->entryForce[e * 8];
            memset(f, 0, 8 * sizeof(MeReal));
            KeaEntryForce(f, c->entryJ[e], c->entryNumRows[e], lambda);
            body->constraintForce[0] = scale * f[0] + body->constraintForce[0];
            body->constraintForce[1] = scale * f[1] + body->constraintForce[1];
            body->constraintForce[2] = scale * f[2] + body->constraintForce[2];
            body->constraintTorque[0] = scale * f[4] + body->constraintTorque[0];
            body->constraintTorque[1] = scale * f[5] + body->constraintTorque[1];
            body->constraintTorque[2] = scale * f[6] + body->constraintTorque[2];
        }
        if (b + 1 == c->numBlocks)
            break;
        lambda += c->entryNumRows[e - 1];
    }
}

// Win 0x791e50 / 0x791e60 / 0x791eb0 (Win only)
MeReal MEAPI MdtKeaConstraintsGetRefineRatio(const MdtKeaConstraints* c) { return c->refineRatio; }
int MEAPI MdtKeaConstraintsGetRefinePasses(const MdtKeaConstraints* c) { return c->refinePasses; }
int MEAPI MdtKeaConstraintsGetGrouped(const MdtKeaConstraints* c) { return c->grouped; }
