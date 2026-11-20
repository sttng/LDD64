// Vortex 5.0.0 build 800 "vortexldd41" - bvbCholRRT: blocked variable bandwidth Cholesky with
// 4x4 blocks (restored). Mac sub-TU 556 0x4a4eaa-0x4a5de0 + kernels
// 0x4a80fb-0x4a99ce, Win 0x7bbff0-0x7bce90 + 0x812910-0x813ef0.
// The linear solver LDD runs (MdtKeaParameters::linearSolver = 1).
// G: mNumBlocks x mNumBlockColumns row-major 4x4 blocks. Lower blocks (J <= I) hold the matrix, then
// the factor L (invDiag = 1 / diagonal); factorize() mirrors L_IJ^T into (J, I) for the solves.
// mMinColumn[I]: first non-zero block column of row I; mMaxColumn[J]: last row mirrored into row J.
// Every floating point expression keeps the association of the Mac SSE2 code.
#include "Kea/LCPMatrix.h"
#include "Kea/MdtKeaConstraints.h"
#include "Kea/MemoryPool.h"

#include <math.h>
#include <string.h>

// ---- kernels --------------------------------------------------------------------------------

// Mac 0x4a8e6a, Win 0x812910 (name inferred)
void bvbFactorize(MeReal* G, int numBlockColumns, int numBlocks, MeReal* invDiag, const int* minColumn)
{
    for (int I = 0; I < numBlocks; ++I) {
        const int minI = minColumn[I];
        for (int J = minI; J <= I; ++J) {
            MeReal* B = G + (I * numBlockColumns + J) * 16;
            int k = minColumn[J];
            if (k < minI)
                k = minI;
            if (k < J) {
                MeReal T[16];
                for (int e = 0; e < 16; ++e)
                    T[e] = 0.0;
                for (; k < J; ++k) {
                    const MeReal* Ik = G + (I * numBlockColumns + k) * 16;
                    const MeReal* Jk = G + (J * numBlockColumns + k) * 16;
                    for (int a = 0; a < 4; ++a)
                        for (int b = 0; b < 4; ++b)
                            T[a * 4 + b] = (((Ik[a * 4 + 0] * Jk[b * 4 + 0] + Ik[a * 4 + 1] * Jk[b * 4 + 1])
                                             + Ik[a * 4 + 2] * Jk[b * 4 + 2])
                                            + Ik[a * 4 + 3] * Jk[b * 4 + 3])
                                           + T[a * 4 + b];
                }
                for (int e = 0; e < 16; ++e)
                    B[e] = B[e] - T[e];
            }
            if (I != J) {
                // B = B L_JJ^-T, row by row
                const MeReal* D = G + J * (numBlockColumns + 1) * 16;
                const MeReal* d = invDiag + J * 4;
                for (int r = 0; r < 4; ++r) {
                    MeReal* b = B + r * 4;
                    const MeReal x0 = b[0] * d[0];
                    const MeReal x1 = (b[1] - x0 * D[4]) * d[1];
                    const MeReal x2 = ((b[2] - x0 * D[8]) - x1 * D[9]) * d[2];
                    const MeReal x3 = (((b[3] - x0 * D[12]) - x1 * D[13]) - x2 * D[14]) * d[3];
                    b[0] = x0;
                    b[1] = x1;
                    b[2] = x2;
                    b[3] = x3;
                }
            }
        }
        // the diagonal block. NOTE: sqrt(fabs(x) + 0) and 1 / (sqrt + 1e-14): no pivot check
        MeReal* D = G + I * (numBlockColumns + 1) * 16;
        MeReal* d = invDiag + I * 4;
        D[0] = sqrt(fabs(D[0]) + 0.0);
        d[0] = 1.0 / (D[0] + 1e-14);
        D[4] = D[4] * d[0];
        D[8] = D[8] * d[0];
        D[12] = D[12] * d[0];
        D[5] = sqrt(fabs(D[5] - D[4] * D[4]) + 0.0);
        d[1] = 1.0 / (D[5] + 1e-14);
        D[9] = (D[9] - D[8] * D[4]) * d[1];
        D[10] = sqrt(fabs((D[10] - D[8] * D[8]) - D[9] * D[9]) + 0.0);
        d[2] = 1.0 / (D[10] + 1e-14);
        D[13] = (D[13] - D[12] * D[4]) * d[1];
        D[14] = ((D[14] - D[12] * D[8]) - D[13] * D[9]) * d[2];
        D[15] = sqrt(fabs(((D[15] - D[12] * D[12]) - D[13] * D[13]) - D[14] * D[14]) + 0.0);
        d[3] = 1.0 / (D[15] + 1e-14);
    }
}

// sum of row r of block B times the 4-vector x, left to right
static inline MeReal BlockRowDot(const MeReal* B, int r, const MeReal* x)
{
    return ((B[r * 4 + 0] * x[0] + B[r * 4 + 1] * x[1]) + B[r * 4 + 2] * x[2]) + B[r * 4 + 3] * x[3];
}

// Mac 0x4a80fb, Win 0x8139f0 (name inferred)
void bvbSolve(const MeReal* G, int numBlockColumns, int numBlocks, const MeReal* invDiag, const int* minColumn,
              const int* maxColumn, MeReal* b, int /*column*/)
{
    // forward: L y = b
    {
        const MeReal* D = G;
        const MeReal* d = invDiag;
        const MeReal x0 = b[0] * d[0];
        const MeReal x1 = (b[1] - x0 * D[4]) * d[1];
        const MeReal x2 = ((b[2] - x0 * D[8]) - x1 * D[9]) * d[2];
        const MeReal x3 = (((b[3] - x0 * D[12]) - x1 * D[13]) - x2 * D[14]) * d[3];
        b[0] = x0;
        b[1] = x1;
        b[2] = x2;
        b[3] = x3;
    }
    for (int I = 1; I < numBlocks; ++I) {
        MeReal acc0 = 0.0, acc1 = 0.0, acc2 = 0.0, acc3 = 0.0;
        for (int J = minColumn[I]; J < I; ++J) {
            const MeReal* B = G + (I * numBlockColumns + J) * 16;
            const MeReal* x = b + J * 4;
            acc0 = acc0 + BlockRowDot(B, 0, x);
            acc1 = acc1 + BlockRowDot(B, 1, x);
            acc2 = acc2 + BlockRowDot(B, 2, x);
            acc3 = acc3 + BlockRowDot(B, 3, x);
        }
        const MeReal* D = G + I * (numBlockColumns + 1) * 16;
        const MeReal* d = invDiag + I * 4;
        MeReal* y = b + I * 4;
        const MeReal x0 = (y[0] - acc0) * d[0];
        const MeReal x1 = ((y[1] - acc1) - x0 * D[4]) * d[1];
        const MeReal x2 = ((y[2] - acc2) - (x0 * D[8] + x1 * D[9])) * d[2];
        const MeReal x3 = ((y[3] - acc3) - ((x0 * D[12] + x1 * D[13]) + x2 * D[14])) * d[3];
        y[0] = x0;
        y[1] = x1;
        y[2] = x2;
        y[3] = x3;
    }
    // backward: L^T x = y (the upper blocks hold L^T)
    for (int I = numBlocks - 1; I >= 0; --I) {
        MeReal acc0 = 0.0, acc1 = 0.0, acc2 = 0.0, acc3 = 0.0;
        for (int J = I + 1; J <= maxColumn[I]; ++J) {
            const MeReal* B = G + (I * numBlockColumns + J) * 16;
            const MeReal* x = b + J * 4;
            acc0 = acc0 + BlockRowDot(B, 0, x);
            acc1 = acc1 + BlockRowDot(B, 1, x);
            acc2 = acc2 + BlockRowDot(B, 2, x);
            acc3 = acc3 + BlockRowDot(B, 3, x);
        }
        const MeReal* D = G + I * (numBlockColumns + 1) * 16;
        const MeReal* d = invDiag + I * 4;
        MeReal* y = b + I * 4;
        const MeReal x3 = (y[3] - acc3) * d[3];
        const MeReal x2 = ((y[2] - acc2) - x3 * D[14]) * d[2];
        const MeReal x1 = ((y[1] - acc1) - (x2 * D[9] + x3 * D[13])) * d[1];
        const MeReal x0 = d[0] * ((y[0] - acc0) - ((x1 * D[4] + x2 * D[8]) + x3 * D[12]));
        y[0] = x0;
        y[1] = x1;
        y[2] = x2;
        y[3] = x3;
    }
}

// Mac 0x4a8701, Win 0x813120 (name inferred)
void bvbMultiply(const MeReal* G, int numBlockColumns, int numBlocks, int n, const int* minColumn,
                 const int* maxColumn, const MeReal* x, MeReal* y)
{
    memcpy(y, x, n * 8);
    // y = L^T y, block rows from the top (the blocks right of the diagonal hold L^T)
    for (int I = 0; I < numBlocks; ++I) {
        MeReal acc0 = 0.0, acc1 = 0.0, acc2 = 0.0, acc3 = 0.0;
        const MeReal* D = G + I * (numBlockColumns + 1) * 16;
        for (int J = I + 1; J <= maxColumn[I]; ++J) {
            const MeReal* B = G + (I * numBlockColumns + J) * 16;
            const MeReal* v = y + J * 4;
            acc0 = acc0 + BlockRowDot(B, 0, v);
            acc1 = acc1 + BlockRowDot(B, 1, v);
            acc2 = acc2 + BlockRowDot(B, 2, v);
            acc3 = acc3 + BlockRowDot(B, 3, v);
        }
        MeReal* v = y + I * 4;
        const MeReal v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];
        v[0] = (((D[0] * v0 + D[4] * v1) + D[8] * v2) + D[12] * v3) + acc0;
        v[1] = ((D[5] * v1 + D[9] * v2) + D[13] * v3) + acc1;
        v[2] = (D[10] * v2 + D[14] * v3) + acc2;
        v[3] = D[15] * v3 + acc3;
    }
    // y = L y, block rows from the bottom
    for (int I = numBlocks - 1; I > 0; --I) {
        MeReal acc0 = 0.0, acc1 = 0.0, acc2 = 0.0, acc3 = 0.0;
        const MeReal* D = G + I * (numBlockColumns + 1) * 16;
        for (int J = minColumn[I]; J < I; ++J) {
            const MeReal* B = G + (I * numBlockColumns + J) * 16;
            const MeReal* v = y + J * 4;
            acc0 = acc0 + BlockRowDot(B, 0, v);
            acc1 = acc1 + BlockRowDot(B, 1, v);
            acc2 = acc2 + BlockRowDot(B, 2, v);
            acc3 = acc3 + BlockRowDot(B, 3, v);
        }
        MeReal* v = y + I * 4;
        const MeReal v0 = v[0], v1 = v[1], v2 = v[2], v3 = v[3];
        v[0] = D[0] * v0 + acc0;
        v[1] = (D[5] * v1 + D[4] * v0) + acc1;
        v[2] = ((D[10] * v2 + D[9] * v1) + D[8] * v0) + acc2;
        v[3] = (((D[15] * v3 + D[14] * v2) + D[13] * v1) + D[12] * v0) + acc3;
    }
    // block 0
    const MeReal v0 = y[0], v1 = y[1], v2 = y[2], v3 = y[3];
    y[0] = G[0] * v0;
    y[1] = v0 * G[4] + v1 * G[5];
    y[2] = (v0 * G[8] + v2 * G[10]) + v1 * G[9];
    y[3] = ((v0 * G[12] + v3 * G[15]) + v1 * G[13]) + v2 * G[14];
}

// ---- class ------------------------------------------------------------------------------------

// Mac 0x4a4ef2 -> 0x4a4ebc, Win 0x7bc000
bvbCholRRT::bvbCholRRT()
{
    m14 = 0;
    mPool = 0;
    mFlags = 0;
    mBlockSize = 4;
}

// Mac 0x4a4eaa, Win 0x7bbff0
const char* bvbCholRRT::getName()
{
    return "block vb chol with tiling using inline functs";
}

// Mac 0x4a4f06 (Win: inlined in clear): 1 on the diagonal of the padding rows
void bvbCholRRT::setPaddingDiagonal()
{
    int i = (mBlockSize - mPaddedSize) + mSize;
    if (i >= mBlockSize)
        return;
    MeReal* last = mG + (mNumBlockColumns + 1) * mBlockElements * (mNumBlocks - 1);
    for (; i < mBlockSize; ++i)
        last[i * 5] = 1.0;
}

// Mac 0x4a4f60, Win 0x7bc250
void bvbCholRRT::deallocate()
{
    if (mColumnSolved != 0) {
        mPool->free(mColumnSolved);
        mColumnSolved = 0;
    }
    if (mG != 0) {
        mPool->free(mG);
        mG = 0;
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
    if (mMaxColumn != 0) {
        mPool->free(mMaxColumn);
        mMaxColumn = 0;
    }
    if (mInverse != 0) {
        mPool->free(mInverse);
        mInverse = 0;
    }
    mFlags = 0;
}

// Mac 0x4a5042, Win 0x7bc4c0
MeReal bvbCholRRT::get(int i, int j)
{
    int row = i, column = j;
    if (i < j) {
        row = j;
        column = i;
    }
    const int I = row / 4, J = column / 4;
    // NOTE: strict compare: elements in the first non-zero block column read as 0
    if (!(mMinColumn[I] < J))
        return 0;
    return mG[((I * mNumBlockColumns + J) * 4 + row % 4) * 4 + column % 4];
}

// Mac 0x4a50d0, Win 0x7bc530: forwards to get()
MeReal bvbCholRRT::getFactor(int i, int j)
{
    return get(i, j);
}

// Mac 0x4a50f2, Win 0x7bc540
MeReal bvbCholRRT::getDiagonal(int i)
{
    return mG[(mNumBlockColumns + 1) * 16 * (i / 4) + (i % 4) * 5];
}

// Mac 0x4a5140, Win 0x7bc880: as vbChol::factorDense
void bvbCholRRT::factorDense(MeReal* A, int n)
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

// Mac 0x4a5292, Win 0x7bcb10: as vbChol::solveDense (scratch mX0)
void bvbCholRRT::solveDense(const MeReal* A, MeReal* b, int n)
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

// Mac 0x4a5414, Win 0x7bce60
void bvbCholRRT::solve(MeReal* b, int)
{
    bvbSolve(mG, mNumBlockColumns, mNumBlocks, mInvDiag, mMinColumn, mMaxColumn, b, 0);
}

// Mac 0x4a545c, Win 0x7bce30
void bvbCholRRT::multiply(const MeReal* x, MeReal* y)
{
    bvbMultiply(mG, mNumBlockColumns, mNumBlocks, mPaddedSize, mMinColumn, mMaxColumn, x, y);
}

// Mac 0x4a54a4, Win 0x7bc830
void bvbCholRRT::getInverseColumn(MeReal* column, int i)
{
    memset(column, 0, mPaddedSize << 3);
    column[i] = 1.0;
    bvbSolve(mG, mNumBlockColumns, mNumBlocks, mInvDiag, mMinColumn, mMaxColumn, column, i);
}

// Mac 0x4a5526, Win 0x7bc640 (+ 0x7bbf10 per entry pair): A = J M^-1 J^T + diag(epsilon)
void bvbCholRRT::build(MdtKeaConstraints* c)
{
    clear();
    const int diagStride = (mNumBlockColumns + 1) * mBlockElements;
    unsigned int i;
    for (i = 0; i < (unsigned int)mSize; ++i)
        mG[(i % mBlockSize) * (mBlockSize + 1) + (i / mBlockSize) * diagStride] = c->epsilon[i];
    for (; i < (unsigned int)mPaddedSize; ++i)
        mG[(i % mBlockSize) * (mBlockSize + 1) + (i / mBlockSize) * diagStride] = 1.0;
    for (int e = 0; e < c->numEntries; ++e) {
        int j = c->bodyFirstEntry[c->entryBodyIndex[e]];
        if (j > e || j == -1)
            continue;
        for (int pass = 0;; ++pass) {
            const int rowStart = c->entryRowStart[e];
            const int rows = c->entryNumRows[e];
            const int columnStart = c->entryRowStart[j];
            const int columns = c->entryNumRows[j];
            const int bs = mBlockSize;
            const int blockColumn = columnStart / bs;
            if (pass == 0) {
                for (int r = 0; r < rows; ++r) {
                    const int blockRow = (r + rowStart) / mBlockSize;
                    int* minColumn = &mMinColumn[blockRow];
                    const int old = *minColumn;
                    if (blockColumn < old) {
                        *minColumn = blockColumn;
                        for (int k = 0; k < old - blockColumn; ++k)
                            memset(&mG[(blockRow * mNumBlockColumns + blockColumn + k) * 0x10], 0, 0x80);
                    }
                }
            }
            // Win 0x7bbf10 (name inferred: per element accumulate of the rows of the two entries)
            const MeReal* jm = c->entryJM[e];
            const MeReal* jj = c->entryJ[j];
            MeReal* base = mG + (blockColumn + (rowStart / bs) * mNumBlockColumns) * mBlockElements;
            for (int r = 0; r < rows; ++r, jm += 6) {
                const int rr = r + rowStart % bs;
                const MeReal* jc = jj;
                for (int col = 0; col < columns; ++col, jc += 6) {
                    const int cc = col + columnStart % bs;
                    MeReal* g = base + (cc / bs + (rr / bs) * mNumBlockColumns) * bs * bs + (rr % bs) * bs + cc % bs;
                    for (int k = 0; k < 6; ++k)
                        *g = jm[k] * jc[k] + *g;
                }
            }
            j = c->entryNext[j];
            if (e < j || j == -1)
                break;
        }
    }
}

// Mac 0x4a5862, Win 0x7bc580: copies L_IJ^T into block (J, I), zeroing skipped blocks
void bvbCholRRT::mirrorUpperBlocks()
{
    for (int I = 1; I < mNumBlocks; ++I) {
        for (int J = mMinColumn[I]; J < I; ++J) {
            const int old = mMaxColumn[J];
            if (old < I) {
                mMaxColumn[J] = I;
                const int count = (I - old) - 1;
                if (count > 0)
                    memset(&mG[(mNumBlockColumns * J + 1 + old) * 0x10], 0, count * 0x80);
            }
            MeReal* upper = mG + (mNumBlockColumns * J + I) * 0x10;
            const MeReal* lower = mG + (mNumBlockColumns * I + J) * 0x10;
            for (int r = 0; r < 4; ++r) {
                upper[r * 4 + 0] = lower[0 + r];
                upper[r * 4 + 1] = lower[4 + r];
                upper[r * 4 + 2] = lower[8 + r];
                upper[r * 4 + 3] = lower[12 + r];
            }
        }
    }
}

// Mac 0x4a5980, Win 0x7bcdf0
void bvbCholRRT::factorize()
{
    if ((mFlags & kLCPMatrixFactored) == 0) {
        bvbFactorize(mG, mNumBlockColumns, mNumBlocks, mInvDiag, mMinColumn);
        mFlags |= kLCPMatrixFactored;
    }
    // NOTE: mirrors again even when the factor was already there
    mirrorUpperBlocks();
}

// Mac 0x4a59ca, Win 0x7bc3f0: stores the lower element only; zeros are not stored
MeReal bvbCholRRT::set(int i, int j, MeReal value)
{
    if (value != 0.0) {
        int row = i, column = j;
        if (i < j) {
            row = j;
            column = i;
        }
        const int I = row / 4, J = column / 4;
        int* minColumn = &mMinColumn[I];
        const int old = *minColumn;
        if (J < old) {
            *minColumn = J;
            for (int k = 0; k < old - J; ++k)
                memset(&mG[(I * mNumBlockColumns + J + k) * 0x10], 0, 0x80);
        }
        mG[((I * mNumBlockColumns + J) * 4 + row % 4) * 4 + column % 4] = value;
    }
    return value;
}

// Mac 0x4a5ac8, Win 0x7bc2f0
void bvbCholRRT::clear()
{
    if ((mFlags & kLCPMatrixCleared) != 0)
        return;
    memset(mColumnSolved, 0, mPaddedSize << 2);
    for (int i = 0; i < mNumBlocks; ++i) {
        mMinColumn[i] = i;
        mMaxColumn[i] = i;
        memset(&mG[mBlockElements * (mNumBlockColumns + 1) * i], 0, 0x80);
    }
    setPaddingDiagonal();
    mFlags |= kLCPMatrixCleared;
}

// Mac 0x4a5b58, Win 0x7bc060
void bvbCholRRT::allocate(int n)
{
    mBlockElements = mBlockSize * mBlockSize;
    const unsigned int blocks = (unsigned int)ceil((double)n / (double)mBlockSize);
    mNumBlocks = blocks;
    const unsigned int padded = blocks * mBlockSize;
    mPaddedSize = padded;
    m24 = 0;
    mSize = n;
    mNumBlockColumns = blocks;
    int stride = padded;
    if ((int)padded % 12 != 0)
        stride = (padded + 12) - (int)padded % 12;
    if ((stride & 0xf) != 0)
        stride = (stride + 0x10) - stride % 0x10;
    unsigned int gBytes = padded * padded * 8;
    if ((gBytes & 0xf) != 0)
        gBytes = (gBytes & ~0xfu) + 0x10;
    const unsigned int cache = stride * stride * 8;
    unsigned int cacheBytes;
    if ((cache & 0x3f) == 0)
        cacheBytes = cache + 0x40;
    else
        cacheBytes = (cache & ~0x3fu) + 0x80;
    unsigned int vBytes = padded * 8;
    if ((vBytes & 0xf) != 0)
        vBytes = (vBytes & ~0xfu) + 0x10;
    mG = (MeReal*)mPool->alloc(gBytes, 0x10, "G");
    mInvDiag = (MeReal*)mPool->alloc(vBytes, 0x10, "inv_d");
    mMinColumn = (int*)mPool->alloc(mNumBlocks << 2, 0x10, "min_col");
    mMaxColumn = (int*)mPool->alloc(mNumBlocks << 2, 0x10, "max_col");
    mX0 = (MeReal*)mPool->alloc(vBytes, 0x10, "x0");
    mInverse = (MeReal*)mPool->alloc(cacheBytes, 0x10, "inverse");
    mInverseStride = stride;
    mColumnSolved = (int*)mPool->alloc(mPaddedSize << 2, 0x10, "column_solved");
    mFlags = kLCPMatrixAllocated;
}

// Win 0x7bc3a0 (Win only, no caller): the matrix as n x n through get()
void bvbCholRRT::copyToDense(MeReal* out)
{
    for (int i = 0; i < mSize; ++i)
        for (int j = 0; j < mSize; ++j)
            out[mSize * i + j] = get(i, j);
}
