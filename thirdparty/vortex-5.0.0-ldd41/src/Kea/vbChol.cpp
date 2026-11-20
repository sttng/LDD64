// Vortex 5.0.0 build 800 "vortexldd41" - vbChol: variable bandwidth (skyline) Cholesky
// (restored). Mac sub-TU 557a 0x4b02f4-0x4b11fa, Win 0x7b7c60-0x7b90b0.
// G holds the lower triangle (rows of mStride doubles); row i is stored from mMinColumn[i] on.
// MdtKeaParameters::linearSolver = 0; LDD selects 1 (bvbCholRRT).
#include "Kea/LCPMatrix.h"
#include "Kea/MdtKeaConstraints.h"
#include "Kea/MemoryPool.h"
#include "Me/MeProfile.h"

#include <math.h>
#include <string.h>

// Mac 0x4af8ee: the base class default
MeReal LCPMatrix::set(int, int, MeReal)
{
    return 0;
}

// Mac 0x4b02f4, Win 0x7b7c60
const char* vbChol::getName()
{
    return "variable bandwith cholesky";
}

// Mac 0x4b0306, Win 0x7b7c80
void vbChol::allocate(int n)
{
    m24 = 0;
    mN = n;
    mSize = n;
    mNumBlocks = (n + 3) >> 2;
    mStride = mNumBlocks * 4;
    int stride = n;
    if (n % 12 != 0)
        stride = (n + 12) - n % 12;
    if ((stride & 0xf) != 0)
        stride = (stride + 0x10) - stride % 0x10;
    const unsigned int bytes = stride * stride * 8;
    int cacheBytes;
    if ((bytes & 0x3f) == 0)
        cacheBytes = bytes + 0x40;
    else
        cacheBytes = (bytes & ~0x3fu) + 0x80;
    mG = (MeReal*)mPool->alloc(mStride * mStride * 8, 0x10, "G");
    mInvDiag = (MeReal*)mPool->alloc(mStride * 8 + 0x10, 0x10, "Diag");
    mX0 = (MeReal*)mPool->alloc(mStride * 8 + 0x10, 0x10, "x0");
    mMinColumn = (int*)mPool->alloc(mNumBlocks << 5, 0x10, "min_col");
    mInverse = (MeReal*)mPool->alloc(cacheBytes, 0x10, "solveresultscache");
    mInverseStride = stride;
    mColumnSolved = (int*)mPool->alloc(mStride << 2, 0x10, "column_solved");
    mFlags = kLCPMatrixAllocated;
}

// Mac 0x4b04f8, Win 0x7b7e20
void vbChol::deallocate()
{
    if (mG != 0) {
        mPool->free(mG);
        mG = 0;
    }
    if (mInverse != 0) {
        mPool->free(mInverse);
        mInverse = 0;
    }
    if (mColumnSolved != 0) {
        mPool->free(mColumnSolved);
        mColumnSolved = 0;
    }
    if (mInvDiag != 0) {
        mPool->free(mInvDiag);
        mInvDiag = 0;
    }
    if (mX0 != 0) {
        mPool->free(mX0);
        mX0 = 0;
    }
    if (mMinColumn != 0) {
        mPool->free(mMinColumn);
        mMinColumn = 0;
    }
    mFlags = 0;
}

// Mac 0x4b05be, Win 0x7b7eb0: in place LDL^T of the n x n matrix A (L below the diagonal, D on it)
void vbChol::factorDense(MeReal* A, int n)
{
    if (n <= 0)
        return;
    for (int i = 0;; ++i) {
        for (int k = 0; k < i; ++k)
            A[i * n + i] = A[i * n + i] - A[i * n + k] * A[i * n + k] * A[k * n + k];
        if (i + 1 == n)
            break;
        for (int r = i + 1; r < n; ++r) {
            for (int k = 0; k < i; ++k)
                A[r * n + i] = A[r * n + i] - A[i * n + k] * A[r * n + k] * A[k * n + k];
            A[r * n + i] = A[r * n + i] / A[i * n + i];
        }
    }
}

// Mac 0x4b0710, Win 0x7b8140: solves L D L^T x = b with the factor of factorDense (b in place)
void vbChol::solveDense(const MeReal* A, MeReal* b, int n)
{
    MeReal* x = mX0;
    int i;
    if (n >= 1) {
        for (i = 0; i < n; ++i) {
            MeReal s = 0.0;
            for (int k = 0; k < i; ++k)
                s = s + A[i * n + k] * x[k];
            x[i] = b[i] - s;
        }
        for (i = 0; i != n; ++i)
            x[i] = x[i] / A[i * n + i];
    }
    for (i = n - 1; i >= 0; --i) {
        MeReal s = 0.0;
        for (int k = i + 1; k < n; ++k)
            s = s + A[k * n + i] * b[k];
        b[i] = x[i] - s;
    }
}

// Mac 0x4b0892, Win 0x7b85d0: y = L x
void vbChol::multiplyLower(const MeReal* x, MeReal* y)
{
    for (int i = 0; i < mN; ++i) {
        y[i] = 0;
        for (int k = mMinColumn[i]; k <= i; ++k)
            y[i] = mG[k + i * mStride] * x[k] + y[i];
    }
}

// Mac 0x4b0912, Win 0x7b8810
MeReal vbChol::getDiagonal(int i)
{
    return mG[(mStride + 1) * i];
}

// Mac 0x4b092c, Win 0x7b8a80
MeReal vbChol::set(int i, int j, MeReal value)
{
    if ((mFlags & kLCPMatrixCleared) == 0)
        clear();
    unsigned int row = i, column = j;
    if (i < j) {
        row = j;
        column = i;
    }
    mG[row * mStride + column] = value;
    mG[mStride * column + row] = value;
    unsigned int* minColumn = (unsigned int*)&mMinColumn[row];
    if ((int)column < (int)*minColumn) {
        for (unsigned int k = column + 1; k < *minColumn; ++k) {
            mG[k + row * mStride] = 0;
            mG[mStride * k + row] = 0;
        }
        *minColumn = column;
    }
    return value;
}

// Mac 0x4b09ec, Win 0x7b8b10
MeReal vbChol::getFactor(int i, int j)
{
    int row = i, column = j;
    if (i < j) {
        column = i;
        row = j;
    }
    if (column < mMinColumn[row])
        return 0;
    return mG[row * mStride + column];
}

// Mac 0x4b0a36, Win 0x7b8b50
MeReal vbChol::get(int i, int j)
{
    int row = i, column = j;
    if (i < j) {
        column = i;
        row = j;
    }
    if (column < mMinColumn[row])
        return 0;
    return mG[row * mStride + column];
}

// Mac 0x4b0a80, Win 0x7b8b90: b = L^-1 b
void vbChol::forwardSubstitute(MeReal* b)
{
    b[0] = b[0] * mInvDiag[0];
    for (int i = 1; i < mN; ++i) {
        MeReal s = b[i];
        for (int k = mMinColumn[i]; k < i; ++k)
            s = s - mG[k + i * mStride] * b[k];
        b[i] = mInvDiag[i] * s;
    }
}

// Mac 0x4b0b1e, Win 0x7b8c60: b = L^-T b
void vbChol::backSubstitute(MeReal* b)
{
    for (int i = mSize - 1; i > 0; --i) {
        const MeReal d = b[i] * mInvDiag[i];
        b[i] = d;
        for (int k = mMinColumn[i]; k < i; ++k)
            b[k] = b[k] - d * mG[k + i * mStride];
    }
    b[0] = b[0] * mInvDiag[0];
}

// Mac 0x4b0bda, Win 0x7b9090
void vbChol::solve(MeReal* b, int)
{
    forwardSubstitute(b);
    backSubstitute(b);
}

// Mac 0x4b0c1e, Win 0x7b8830
void vbChol::clear()
{
    if ((mFlags & kLCPMatrixCleared) != 0)
        return;
    memset(mColumnSolved, 0, mStride << 2);
    for (unsigned int i = 0; i < (unsigned int)mSize; ++i) {
        mMinColumn[i] = i;
        mG[(mStride + 1) * i] = 0;
    }
    mFlags |= kLCPMatrixCleared;
}

// Mac 0x4b0c84, Win 0x7b87d0
void vbChol::getInverseColumn(MeReal* column, int i)
{
    memset(column, 0, mStride << 3);
    column[i] = 1.0;
    solve(column, 1);
}

// Mac 0x4b0ce4, Win 0x7b86c0: y = L^T x
void vbChol::multiplyUpper(const MeReal* x, MeReal* y)
{
    memset(y, 0, mN << 3);
    for (int i = 0; i < mN; ++i)
        for (int k = mMinColumn[i]; k <= i; ++k)
            y[k] = x[i] * mG[k + i * mStride] + y[k];
}

// Mac 0x4b0d66, Win 0x7b9060
void vbChol::multiply(const MeReal* x, MeReal* y)
{
    MeReal* t = mX0;
    multiplyUpper(x, t);
    multiplyLower(t, y);
}

// Mac 0x4b0da6, Win 0x7b8420
void vbChol::factorize()
{
    MeProfileStartSection("main_factorize", 0);
    if ((mFlags & kLCPMatrixFactored) == 0) {
        mInvDiag[0] = sqrt(mG[0]);
        MeReal d = 0;
        for (int i = 0; i < mN; ++i) {
            mInvDiag[i] = 0;
            const int first = mMinColumn[i];
            for (int j = first; j <= i; ++j) {
                MeReal* g = &mG[j + i * mStride];
                d = *g;
                if (j != 0) {
                    int k = mMinColumn[j];
                    if (k < first)
                        k = first;
                    if (j != k && k < j) {
                        for (; k != j; ++k)
                            d = d - mG[k + i * mStride] * mG[k + mStride * j];
                    }
                }
                // NOTE: also runs for j == i with mInvDiag[i] = 0 (overwritten below)
                *g = d * mInvDiag[j];
            }
            if (!(0.0 < d))
                d = (double)1e-12f;
            mG[(mStride + 1) * i] = sqrt(d);
            mInvDiag[i] = 1.0 / mG[(mStride + 1) * i];
        }
        mFlags |= kLCPMatrixFactored;
    }
    MeProfileEndSection("main_factorize");
}

// Mac 0x4b0f80, Win 0x7b8d50 (+ 0x7b7ba0 per block)
void vbChol::build(MdtKeaConstraints* c)
{
    MeProfileStartSection("calcJinvMJT", 0);
    clear();
    for (unsigned int i = 0; i < (unsigned int)mSize; ++i) {
        unsigned int count = mStride - i;
        if (count > 6)
            count = 6;
        memset(&mG[(mStride + 1) * i], 0, count << 3);
        mG[(mStride + 1) * i] = c->epsilon[i];
        mMinColumn[i] = i;
    }
    for (int e = 0; e < c->numEntries; ++e) {
        int j = c->bodyFirstEntry[c->entryBodyIndex[e]];
        if (j > e || j == -1)
            continue;
        for (int pass = 0;; ++pass) {
            const int row0 = c->entryRowStart[e];
            const int rows = c->entryNumRows[e];
            const unsigned int column0 = c->entryRowStart[j];
            const int columns = c->entryNumRows[j];
            if (pass == 0 && rows > 0) {
                for (int r = 0; r != rows; ++r) {
                    const unsigned int minColumn = mMinColumn[row0 + r];
                    if (column0 < minColumn) {
                        memset(&mG[(row0 + r) * mStride + column0], 0, (minColumn - column0) * 8);
                        mMinColumn[row0 + r] = column0;
                    }
                }
            }
            const MeReal* jj = c->entryJ[j];
            const MeReal* jm = c->entryJM[e];
            MeReal* g = &mG[row0 * mStride + column0];
            for (int r = 0; r < rows; ++r, jm += 6, g += mStride) {
                const MeReal* jc = jj;
                for (int col = 0; col < columns; ++col, jc += 6)
                    for (int k = 0; k < 6; ++k)
                        g[col] = jm[k] * jc[k] + g[col];
            }
            j = c->entryNext[j];
            if (e < j || j == -1)
                break;
        }
    }
    MeProfileEndSection("calcJinvMJT");
}
