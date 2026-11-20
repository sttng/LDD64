// Vortex 5.0.0 build 800 "vortexldd41" - MdtKeaAddConstraint (restored; Mac TU
// 556, see MdtKeaConstraints.cpp). In its own object file so that tests with a weak recorder of
// it (test_vortex_bcl) do not pull it in with the other MdtKeaConstraints functions.
#include "Kea/MdtKeaConstraints.h"
#include "Mdt/MdtBclRows.h"
#include "Me/MeIntMap.h"
#include <stddef.h>

// PORT64: the row builders (MdtBcl) write through the prefix view MdtKeaConstraintRows
// (Kea/MdtKeaRows.h): its members must sit where MdtKeaConstraints has them on every target
// (a void* in place of an int moved numRows and the arrays after it on 64-bit). Compile-time
// only; no code.
#define MDTKEA_SAME(a, b) (offsetof(MdtKeaConstraintRows, a) == offsetof(MdtKeaConstraints, b))
typedef char MdtKeaConstraintRowsLayoutCheck[
    (MDTKEA_SAME(xi, error) && MDTKEA_SAME(c, velocity) && MDTKEA_SAME(lo, lo) && MDTKEA_SAME(hi, hi) &&
     MDTKEA_SAME(m10, lambda) && MDTKEA_SAME(epsilon, epsilon) && MDTKEA_SAME(gamma, gamma) &&
     MDTKEA_SAME(constraintType, constraintType) && MDTKEA_SAME(rowFlags, rowFlags) &&
     MDTKEA_SAME(m24, lcpSteps) && MDTKEA_SAME(m28, lcpState) && MDTKEA_SAME(numCoupled, numFrictionRows) &&
     MDTKEA_SAME(m30, m30) && MDTKEA_SAME(coupledRow, frictionNormal) &&
     MDTKEA_SAME(coupledScale, frictionCoefficient) && MDTKEA_SAME(m3c, cohesion) &&
     MDTKEA_SAME(numRows, numRows)) ? 1 : -1];
#undef MDTKEA_SAME

// Mac 0x4a778a, Win 0x791540 (name inferred)
void MEAPI MdtKeaAddConstraint(MdtKeaConstraints* c, const MdtBclRows* rows, MdtKeaBody* const* keaBodies,
                               int numBodies, int numRows, int type, int)
{
    if (numRows <= 0)
        return;
    const int rowStart = c->numRows;
    c->constraintType[c->numBlocks] = type;
    if (numBodies <= 0)
        return;
    int added = 0;
    for (int k = 0; k != numBodies; ++k) {
        MdtKeaBody* body = keaBodies[k];
        if (body == 0)
            continue;
        if (added == 0)
            c->blockFirstEntry[c->numBlocks] = c->numEntries;
        c->entryBody[c->numEntries] = body;
        // NOTE: the body map is keyed by the body address as an int (32-bit in both binaries;
        // PORT64: the whole address on 64-bit, Me/MeIntMap.h)
        int index = MeIntMapLookup(c->bodyMap, (MeIntMapKey)(size_t)body);
        if (index < 0)
            index = 0;
        c->entryBodyIndex[c->numEntries] = index;
        if (c->maxBodyIndex < index)
            c->maxBodyIndex = index;
        c->entryNumRows[c->numEntries] = numRows;
        c->entryRowStart[c->numEntries] = rowStart;
        c->entryJ[c->numEntries] = c->J + c->numJacobianRows * 6;
        c->entryJM[c->numEntries] = c->JM + c->numJacobianRows * 6;
        const int e = c->numEntries;
        MeReal* j = c->entryJ[e];
        const int bodyIndex = c->entryBodyIndex[e];
        c->entryNext[e] = -1;
        if (body != 0) {
            if (c->bodyFirstEntry[bodyIndex] < 0) {
                c->bodyLastEntry[bodyIndex] = e;
                c->bodyFirstEntry[bodyIndex] = e;
            } else {
                c->entryNext[c->bodyLastEntry[bodyIndex]] = e;
                c->bodyLastEntry[bodyIndex] = e;
            }
        }
        ++c->numEntries;
        ++added;
        for (int r = 0; r != numRows; ++r) {
            for (int m = 0; m != 6; ++m)
                j[r * 6 + m] = rows->jacobian[k][r][m];
            ++c->numJacobianRows;
        }
    }
    if (added != 0) {
        c->blockNumEntries[c->numBlocks] = added;
        ++c->numBlocks;
        c->numRows += numRows;
    }
}
