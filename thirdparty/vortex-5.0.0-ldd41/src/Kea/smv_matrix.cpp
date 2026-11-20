// Vortex 5.0.0 build 800 "vortexldd41" - smv_matrix: the dense matrix of one constraint block
// of itr_constraint_solver (restored). Mac sub-TU 556 0x4ad354-0x4adcaa, Win
// 0x810a20-0x8116e0. mA holds the whole symmetric matrix (rows of mN), mFactor the LDL^T factor.
#include "Kea/LCPMatrix.h"
#include "Kea/MdtKeaConstraints.h"

#include <string.h>

// Mac 0x4ad354, Win 0x810a20
const char* smv_matrix::getName()
{
    return "dense, small matrix using home grown lapack";
}

// Mac 0x4ad366, Win 0x810af0: stores at [min(i, j)][max(i, j)]
MeReal smv_matrix::set(int i, int j, MeReal value)
{
    int index;
    if (j < i)
        index = j * mN + i;
    else
        index = i * mN + j;
    mA[index] = value;
    return value;
}

// Mac 0x4ad394, Win 0x810b30
MeReal smv_matrix::get(int i, int j)
{
    int index;
    if (j < i)
        index = j * mN + i;
    else
        index = i * mN + j;
    return mA[index];
}

// Mac 0x4ad3be, Win 0x810b60
MeReal smv_matrix::getFactor(int i, int j)
{
    int index;
    if (j < i)
        index = j * mN + i;
    else
        index = i * mN + j;
    return mFactor[index];
}

// Mac 0x4ad3ec, Win 0x810ba0
MeReal smv_matrix::getDiagonal(int i)
{
    return mFactor[(mN + 1) * i];
}

// Mac 0x4ad406, Win 0x810cb0
void smv_matrix::buildBlock(const MdtKeaConstraints* c, int block)
{
    int e = c->blockFirstEntry[block];
    const int rowStart = c->entryRowStart[e];
    for (unsigned int i = 0; i < (unsigned int)mSize; ++i)
        mA[(mN + 1) * i] = c->epsilon[rowStart + i];
    for (int k = 0; k < c->blockNumEntries[block]; ++k, ++e) {
        const int n = mSize;
        const MeReal* jj = c->entryJ[e];
        const MeReal* jm = c->entryJM[e];
        const int stride = mN;
        for (int r = 0; r < n; ++r, jm += 6) {
            MeReal* a = &mA[r * stride];
            const MeReal* jc = jj;
            for (int col = 0; col < n; ++col, ++a, jc += 6)
                for (int m = 0; m < 6; ++m)
                    *a = jm[m] * jc[m] + *a;
        }
    }
    // NOTE: mirrors with the row stride mSize (the accumulation above uses mN; they are equal)
    for (int i = 0; i < mSize; ++i)
        for (int j = i + 1; j < mSize; ++j)
            mA[mSize * i + j] = mA[mSize * j + i];
}

// Mac 0x4ad586, Win 0x810d90
void smv_matrix::factorize()
{
    const int n = mSize;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j <= i; ++j)
            mFactor[j + i * mSize] = mA[j + i * mSize];
    factorDense(mFactor, mSize);
}

// Mac 0x4ad5e8, Win 0x810e40
void smv_matrix::solve(MeReal* b, int)
{
    solveDense(mFactor, b, mSize);
}

// Mac 0x4ad614, Win 0x810e60: y = A x
void smv_matrix::multiply(const MeReal* x, MeReal* y)
{
    for (int i = 0; i < mSize; ++i) {
        const MeReal* a = &mA[i * mSize];
        MeReal s = 0.0;
        for (int j = 0; j < mSize; ++j)
            s = s + a[j] * x[j];
        y[i] = s;
    }
}

// Mac 0x4ad68e, Win 0x810f50: as vbChol::factorDense
void smv_matrix::factorDense(MeReal* A, int n)
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

// Mac 0x4ad7e0, Win 0x8111e0: as vbChol::solveDense with a stack scratch vector
void smv_matrix::solveDense(const MeReal* A, MeReal* b, int n)
{
    MeReal x[18];
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

// Mac 0x4ad9c6, Win 0x8114d0
void smv_matrix::deallocate()
{
    mFlags = 0;
}

// Mac 0x4ad9d6, Win 0x8114e0
void smv_matrix::clear()
{
    if ((mFlags & kLCPMatrixCleared) == 0) {
        memset(mColumnSolved, 0, mSize << 2);
        mFlags |= kLCPMatrixCleared;
    }
}

// Mac 0x4ada0e, Win 0x810f10
void smv_matrix::getInverseColumn(MeReal* column, int i)
{
    memset(column, 0, mSize << 3);
    column[i] = 1.0;
    solve(column, 0);
}

// Mac 0x4ada6e, Win 0x810a30
void smv_matrix::allocate(int n)
{
    m24 = 0;
    mSize = n;
    mN = n;
    mInverse = mInverseCache;
    mInverseStride = n;
    mColumnSolved = mColumnSolvedStore;
    memset(mA, 0, n * n * 8);
    memset(mColumnSolved, 0, mSize << 2);
    mFlags = kLCPMatrixAllocated;
}

// Mac 0x4adae4, Win 0x810bc0
void smv_matrix::build(MdtKeaConstraints* c)
{
    clear();
    for (unsigned int i = 0; i < (unsigned int)mSize; ++i)
        mA[(mSize + 1) * i] = c->epsilon[i];
    for (int e = 0; e < c->numEntries; ++e) {
        for (int j = c->bodyFirstEntry[c->entryBodyIndex[e]]; j <= e && j != -1; j = c->entryNext[j]) {
            const int rows = c->entryNumRows[e];
            const int columns = c->entryNumRows[j];
            const MeReal* jj = c->entryJ[j];
            const MeReal* jm = c->entryJM[e];
            const int stride = mN;
            MeReal* row = &mA[stride * c->entryRowStart[e] + c->entryRowStart[j]];
            for (int r = 0; r < rows; ++r, jm += 6, row += stride) {
                MeReal* a = row;
                const MeReal* jc = jj;
                for (int col = 0; col < columns; ++col, ++a, jc += 6)
                    for (int m = 0; m < 6; ++m)
                        *a = jm[m] * jc[m] + *a;
            }
        }
    }
    for (int i = 0; i < mSize; ++i)
        for (int j = i + 1; j < mSize; ++j)
            mA[mSize * i + j] = mA[mSize * j + i];
}

// Win 0x810aa0 (Win only, no caller)
void smv_matrix::copyToDense(MeReal* out)
{
    for (int i = 0; i < mSize; ++i)
        for (int j = 0; j < mSize; ++j)
            out[mSize * i + j] = get(i, j);
}
