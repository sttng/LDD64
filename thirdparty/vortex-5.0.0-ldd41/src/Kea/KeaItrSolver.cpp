// Vortex 5.0.0 build 800 "vortexldd41" - itr_constraint_solver: block Gauss-Seidel over the
// constraints of a work slot (restored). Mac sub-TU 556 0x4a5e86-0x4a6c3b,
// Win 0x7b9d00-0x7bad50. Selected by MdtKeaParameters::constraintSolver = 1, which LDD never
// sets (its solve, Win 0x7ba8d0, never runs).
// Each block has its own smv_matrix (J_b M^-1 J_b^T); a sweep solves the block LCPs in turn with
// the other blocks' body forces on the right hand side, until the complementarity residual stops
// improving.
#include "Kea/KeaFrames.h"
#include "Kea/MdtKeaConstraints.h"

#include <float.h>
#include <math.h>
#include <string.h>

// Mac 0x4a5f22 -> 0x4a5e86, Win 0x7ba880
itr_constraint_solver::itr_constraint_solver() : mBlockMatrices(0), mBodyForce(0), mW(0), mNumBlocks(0)
{
    mPrototype.m14 = 0;
    mPrototype.mPool = 0;
    mPrototype.mFlags = 0;
}

// Mac 0x4a6abc, Win 0x7b9dd0
void itr_constraint_solver::setup(int n, LCPMatrix* matrix, MdtKeaConstraints* c, MdtKeaParameters* p,
                                  KeaLcpFunction, MemoryPool* pool, MdtKeaBody* const* bodies, int numBodies, int)
{
    mN = n;
    mMatrix = matrix;
    mConstraints = c;
    mParams = p;
    mLcp = KeaSmvLcp;
    mPool = pool;
    mBodies = bodies;
    mNumBodies = numBodies;
    mNumBlocks = c->numBlocks;
    mBodyForce = (MeReal*)pool->allocZeroed(numBodies << 6, 0x10, "body force");
    mW = (MeReal*)mPool->alloc(mN << 3, 0x10, "w");
    mBlockMatrices = (smv_matrix*)mPool->allocZeroed(mNumBlocks * sizeof(smv_matrix), 0x10, "block matrices");
    for (int b = 0; b < mNumBlocks; ++b) {
        const int rows = mConstraints->entryNumRows[mConstraints->blockFirstEntry[b]];
        // NOTE: the prototype (with its vtable) is copied byte by byte
        memcpy((void*)&mBlockMatrices[b], (const void*)&mPrototype, sizeof(smv_matrix));
        mBlockMatrices[b].mPool = mPool;
        mBlockMatrices[b].allocate(rows);
        mBlockMatrices[b].buildBlock(mConstraints, b);
    }
}

// Mac 0x4a5f64, Win 0x7b9f80
void itr_constraint_solver::cleanup()
{
    if (mBodyForce != 0)
        mPool->free(mBodyForce);
    if (mW != 0)
        mPool->free(mW);
    if (mBlockMatrices != 0)
        mPool->free(mBlockMatrices);
}

// Mac 0x4a5fc0, Win 0x7ba8d0
void itr_constraint_solver::solve()
{
    MdtKeaConstraints* c;
    const MeReal tolerance = mParams->itrTolerance;
    const int maxIterations = mParams->itrMaxIterations;
    mParams->lcp.warmStart = 0;
    KeaCopy(mN, mConstraints->rhs, 1, mW, 1);
    int rowOffset = 0;
    int block = 0;
    int iteration = 0;
    MeReal previous = 1.0;
    MeReal b[16];
    for (;;) {
        for (; block < mNumBlocks; ++block) {
            c = mConstraints;
            const int first = c->blockFirstEntry[block];
            const int rows = c->entryNumRows[first];
            KeaCopy(rows, c->rhs + rowOffset, 1, b, 1);
            c = mConstraints;
            // the right hand side without this block's own forces
            for (int k = 0; k < c->blockNumEntries[block]; ++k) {
                const int e = first + k;
                MeReal* bf = mBodyForce + c->entryBodyIndex[e] * 8;
                const MeReal* ef = c->entryForce + (first + k) * 8;
                bf[0] = bf[0] - ef[0];
                bf[1] = bf[1] - ef[1];
                bf[2] = bf[2] - ef[2];
                bf[4] = bf[4] - ef[4];
                bf[5] = bf[5] - ef[5];
                bf[6] = bf[6] - ef[6];
                const MeReal* jm = mConstraints->entryJM[e];
                for (int r = 0; r < rows; ++r, jm += 6)
                    b[r] = b[r] - ((((((0 + jm[0] * bf[0]) + jm[1] * bf[1]) + jm[2] * bf[2]) + jm[3] * bf[4])
                                    + jm[4] * bf[5])
                                   + jm[5] * bf[6]);
                c = mConstraints;
            }
            MdtKeaParameters* p = mParams;
            MeReal* lambda = c->lambda + rowOffset;
            MeReal* lo = c->lo + rowOffset;
            MeReal* hi = c->hi + rowOffset;
            int* state = c->lcpState + rowOffset;
            smv_matrix* m = &mBlockMatrices[block];
            mLcp(m, lambda, b, lo, hi, state, &p->lcp, 0, 0);
            if (c->numFrictionRows != 0 && c->frictionNormal[rowOffset + 1] != -1) {
                const int warmStart = p->lcp.warmStart;
                KeaUpdateFrictionBounds(m->mSize, lambda, lo, hi, c->frictionNormal + rowOffset,
                                        c->frictionCoefficient + rowOffset, c->cohesion + rowOffset, state);
                p->lcp.warmStart = 1;
                mLcp(m, lambda, b, lo, hi, state, &p->lcp, 0, 0);
                p->lcp.warmStart = warmStart;
            }
            // this block's forces back into the body forces
            c = mConstraints;
            for (int k = 0; k < c->blockNumEntries[block]; ++k) {
                const int e = first + k;
                const int body = c->entryBodyIndex[e];
                MeReal* ef = c->entryForce + e * 8;
                const MeReal* l = c->lambda + rowOffset;
                const MeReal* jj = c->entryJ[e];
                for (int m2 = 0; m2 < 8; ++m2)
                    ef[m2] = 0;
                for (int r = 0; r < rows; ++r, jj += 6) {
                    const MeReal v = l[r];
                    ef[0] = v * jj[0] + ef[0];
                    ef[1] = v * jj[1] + ef[1];
                    ef[2] = v * jj[2] + ef[2];
                    ef[4] = v * jj[3] + ef[4];
                    ef[5] = v * jj[4] + ef[5];
                    ef[6] = ef[6] + v * jj[5];
                }
                MeReal* bf = mBodyForce + body * 8;
                const MeReal* f = mConstraints->entryForce + e * 8;
                bf[0] = bf[0] + f[0];
                bf[1] = bf[1] + f[1];
                bf[2] = bf[2] + f[2];
                bf[4] = bf[4] + f[4];
                bf[5] = bf[5] + f[5];
                bf[6] = bf[6] + f[6];
                c = mConstraints;
            }
            rowOffset += rows;
        }
        mParams->lcp.warmStart = 1;
        // w = A lambda - b, block by block
        // NOTE: each entry of a block overwrites the block's w (only the last entry counts)
        MeReal* w = mW;
        c = mConstraints;
        const MeReal* lambda = c->lambda;
        const int numBlocks = c->numBlocks;
        memset(w, 0, mN << 3);
        if (numBlocks > 0) {
            for (int blk = 0;;) {
                const int first = c->blockFirstEntry[blk];
                const int rows = c->entryNumRows[first];
                for (int k = 0; k < c->blockNumEntries[blk]; ++k) {
                    const int e = first + k;
                    const int body = c->entryBodyIndex[e];
                    if (body == -1)
                        continue;
                    MeReal s0 = 0.0, s1 = 0.0, s2 = 0.0, s3 = 0.0, s4 = 0.0, s5 = 0.0;
                    for (int f = c->bodyFirstEntry[body]; f != -1; f = c->entryNext[f]) {
                        const int n = c->entryNumRows[f];
                        const MeReal* jj = c->entryJ[f];
                        const MeReal* l = lambda + c->entryRowStart[f];
                        for (int r = 0; r < n; ++r, jj += 6) {
                            const MeReal v = l[r];
                            s0 = s0 + v * jj[0];
                            s1 = s1 + v * jj[1];
                            s2 = s2 + v * jj[2];
                            s3 = s3 + v * jj[3];
                            s4 = v * jj[4] + s4;
                            s5 = s5 + v * jj[5];
                        }
                    }
                    const MeReal* jm = c->entryJM[e];
                    for (int r = 0; r < rows; ++r, jm += 6)
                        w[r] = (((((s0 * jm[0] + 0.0) + s1 * jm[1]) + s2 * jm[2]) + s3 * jm[3]) + s4 * jm[4])
                               + s5 * jm[5];
                }
                if (++blk == numBlocks)
                    break;
                w += rows;
            }
        }
        KeaSubtract(mN, mConstraints->rhs, mW);
        // the complementarity residual
        c = mConstraints;
        const MeReal* lo = c->lo;
        const MeReal* hi = c->hi;
        const MeReal* l = c->lambda;
        const MeReal* wv = mW;
        MeReal sum = 0.0;
        for (int i = 0; i < mN; ++i) {
            MeReal v;
            if (lo[i] != -DBL_MAX) {
                if (hi[i] != DBL_MAX) {
                    const MeReal aboveHi = l[i] - hi[i];
                    MeReal m = wv[i];
                    const MeReal aboveLo = l[i] - lo[i];
                    if (aboveLo <= m)
                        m = aboveLo;
                    v = m <= aboveHi ? aboveHi : m;
                } else {
                    v = l[i] - lo[i];
                    if (wv[i] <= v)
                        v = wv[i];
                }
            } else if (hi[i] != DBL_MAX) {
                v = hi[i] - l[i];
                if (wv[i] <= v)
                    v = wv[i];
            } else {
                v = wv[i];
            }
            sum = sum + v * v;
        }
        const MeReal norm = sqrt(sum);
        ++iteration;
        if (maxIterations < iteration || norm < tolerance || fabs((norm - previous) / previous) < tolerance)
            break;
        rowOffset = 0;
        block = 0;
        previous = norm;
    }
    mParams->lcp.warmStart = 0;
}
