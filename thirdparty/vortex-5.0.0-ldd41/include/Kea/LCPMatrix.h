// Vortex 5.0.0 build 800 "vortexldd41" - the LCP matrices of Kea (restored).
//  * LCPMatrix   abstract (Mac vtable 0x75f1c8, Win: none emitted)
//  * vbChol      "variable bandwith cholesky"                     src/Kea/vbChol.cpp (Mac TU 557a)
//  * bvbCholRRT  "block vb chol with tiling using inline functs"  src/Kea/bvbCholRRT.cpp
//  * smv_matrix  "dense, small matrix using home grown lapack"    src/Kea/smv_matrix.cpp
// The matrix is A = J M^-1 J^T + diag(epsilon) of one Kea work slot (built by build()); the LCP
// solvers use it through factorize / solve / multiply / getInverseColumn and factor and solve the
// small principal submatrices of the active rows with factorDense / solveDense (LDL^T in place).
// Slot names inferred from what each implementation does. No virtual destructors.
#ifndef KEA_LCPMATRIX_H
#define KEA_LCPMATRIX_H

#include "Kea/MdtKea.h"

struct MdtKeaConstraints;
class MemoryPool;

// mFlags bits (names inferred)
enum LCPMatrixFlags {
    kLCPMatrixAllocated = 0x02,
    kLCPMatrixFactored = 0x04,
    kLCPMatrixCleared = 0x10
};

class LCPMatrix {
public:
    virtual void allocate(int n) = 0;                                  // slot 0
    virtual void deallocate() = 0;                                     // slot 1
    virtual void clear() = 0;                                          // slot 2
    virtual MeReal set(int i, int j, MeReal value);                    // slot 3 (Mac 0x4af8ee: returns 0)
    virtual MeReal get(int i, int j) = 0;                              // slot 4
    virtual MeReal getFactor(int i, int j) = 0;                        // slot 5
    virtual void build(MdtKeaConstraints* c) = 0;                      // slot 6 (vbChol: "calcJinvMJT")
    virtual void factorize() = 0;                                      // slot 7 (vbChol: "main_factorize")
    virtual void multiply(const MeReal* x, MeReal* y) = 0;             // slot 8: y = A x
    virtual void solve(MeReal* b, int flag) = 0;                       // slot 9: b = A^-1 b
    virtual void getInverseColumn(MeReal* column, int i) = 0;          // slot 10
    virtual void factorDense(MeReal* A, int n) = 0;                    // slot 11: LDL^T, n x n, in place
    virtual void solveDense(const MeReal* A, MeReal* b, int n) = 0;    // slot 12
    virtual MeReal getDiagonal(int i) = 0;                             // slot 13
    virtual const char* getName() = 0;                                 // slot 14

    int mSize;              // +0x04 n
    int* mColumnSolved;     // +0x08 ("column_solved")
    MeReal* mInverse;       // +0x0c inverse column cache ("inverse")
    int mInverseStride;     // +0x10
    int m14;                // +0x14
    MemoryPool* mPool;      // +0x18
    int mFlags;             // +0x1c LCPMatrixFlags
};

// ---- vbChol (0x40 bytes on both) ----------------------------------------------------------------
class vbChol : public LCPMatrix {
public:
    vbChol() {}                                      // Win 0x75e160 (vtable only)
    virtual void allocate(int n);                    // Mac 0x4b0306, Win 0x7b7c80
    virtual void deallocate();                       // Mac 0x4b04f8, Win 0x7b7e20
    virtual void clear();                            // Mac 0x4b0c1e, Win 0x7b8830
    virtual MeReal set(int i, int j, MeReal value);  // Mac 0x4b092c, Win 0x7b8a80
    virtual MeReal get(int i, int j);                // Mac 0x4b0a36, Win 0x7b8b50
    virtual MeReal getFactor(int i, int j);          // Mac 0x4b09ec, Win 0x7b8b10
    virtual void build(MdtKeaConstraints* c);        // Mac 0x4b0f80, Win 0x7b8d50
    virtual void factorize();                        // Mac 0x4b0da6, Win 0x7b8420
    virtual void multiply(const MeReal* x, MeReal* y); // Mac 0x4b0d66, Win 0x7b9060
    virtual void solve(MeReal* b, int flag);         // Mac 0x4b0bda, Win 0x7b9090
    virtual void getInverseColumn(MeReal* column, int i); // Mac 0x4b0c84, Win 0x7b87d0
    virtual void factorDense(MeReal* A, int n);      // Mac 0x4b05be, Win 0x7b7eb0
    virtual void solveDense(const MeReal* A, MeReal* b, int n); // Mac 0x4b0710, Win 0x7b8140
    virtual MeReal getDiagonal(int i);               // Mac 0x4b0912, Win 0x7b8810
    virtual const char* getName();                   // Mac 0x4b02f4, Win 0x7b7c60

    void forwardSubstitute(MeReal* b);               // Mac 0x4b0a80, Win 0x7b8b90 (name inferred)
    void backSubstitute(MeReal* b);                  // Mac 0x4b0b1e, Win 0x7b8c60 (name inferred)
    void multiplyLower(const MeReal* x, MeReal* y);  // Mac 0x4b0892, Win 0x7b85d0: y = L x (name inferred)
    void multiplyUpper(const MeReal* x, MeReal* y);  // Mac 0x4b0ce4, Win 0x7b86c0: y = L^T x (name inferred)

    int mN;                 // +0x20
    int m24;                // +0x24
    int mStride;            // +0x28 n rounded up to 4
    int mNumBlocks;         // +0x2c (n + 3) / 4
    MeReal* mInvDiag;       // +0x30 ("Diag")
    MeReal* mG;             // +0x34 lower triangle, rows of mStride ("G")
    MeReal* mX0;            // +0x38 scratch vector ("x0")
    int* mMinColumn;        // +0x3c first non-zero column of each row ("min_col")
};

// ---- bvbCholRRT (Mac 0x4c, Win 0x50 bytes): 4x4 blocks ----------------------------------------
class bvbCholRRT : public LCPMatrix {
public:
    bvbCholRRT();                                    // Mac 0x4a4ef2 -> 0x4a4ebc, Win 0x7bc000
    virtual void allocate(int n);                    // Mac 0x4a5b58, Win 0x7bc060
    virtual void deallocate();                       // Mac 0x4a4f60, Win 0x7bc250
    virtual void clear();                            // Mac 0x4a5ac8, Win 0x7bc2f0
    virtual MeReal set(int i, int j, MeReal value);  // Mac 0x4a59ca, Win 0x7bc3f0
    virtual MeReal get(int i, int j);                // Mac 0x4a5042, Win 0x7bc4c0
    virtual MeReal getFactor(int i, int j);          // Mac 0x4a50d0, Win 0x7bc530
    virtual void build(MdtKeaConstraints* c);        // Mac 0x4a5526, Win 0x7bc640
    virtual void factorize();                        // Mac 0x4a5980, Win 0x7bcdf0
    virtual void multiply(const MeReal* x, MeReal* y); // Mac 0x4a545c, Win 0x7bce30
    virtual void solve(MeReal* b, int flag);         // Mac 0x4a5414, Win 0x7bce60
    virtual void getInverseColumn(MeReal* column, int i); // Mac 0x4a54a4, Win 0x7bc830
    virtual void factorDense(MeReal* A, int n);      // Mac 0x4a5140, Win 0x7bc880
    virtual void solveDense(const MeReal* A, MeReal* b, int n); // Mac 0x4a5292, Win 0x7bcb10
    virtual MeReal getDiagonal(int i);               // Mac 0x4a50f2, Win 0x7bc540
    virtual const char* getName();                   // Mac 0x4a4eaa, Win 0x7bbff0

    void setPaddingDiagonal();                       // Mac 0x4a4f06 (Win inlined in clear) (name inferred)
    void mirrorUpperBlocks();                        // Mac 0x4a5862, Win 0x7bc580 (name inferred)
    void copyToDense(MeReal* out);                   // Win 0x7bc3a0 (Win only, no caller) (name inferred)

    int mNumBlockColumns;   // +0x20 blocks per block row
    int m24;                // +0x24
    int mBlockSize;         // +0x28 4
    int mBlockElements;     // +0x2c 16
    int mNumBlocks;         // +0x30
    int mPaddedSize;        // +0x34 blocks * 4
    MeReal* mG;             // +0x38 blocks of 16 doubles ("G")
    MeReal* mInvDiag;       // +0x3c ("inv_d")
    MeReal* mX0;            // +0x40 ("x0")
    int* mMinColumn;        // +0x44 per block row ("min_col")
    int* mMaxColumn;        // +0x48 per block column ("max_col")
};

// The block kernels of bvbCholRRT (static in bvbCholRRT.cpp in the original; names inferred):
// Mac 0x4a8e6a, Win 0x812910: blocked Cholesky (G = L, diagonal blocks inverted into invDiag)
void bvbFactorize(MeReal* G, int numBlockColumns, int numBlocks, MeReal* invDiag, const int* minColumn);
// Mac 0x4a80fb, Win 0x8139f0: b = (L L^T)^-1 b in place; `column` (getInverseColumn passes the
// unit column, solve 0) is not read
void bvbSolve(const MeReal* G, int numBlockColumns, int numBlocks, const MeReal* invDiag, const int* minColumn,
              const int* maxColumn, MeReal* b, int column);
// Mac 0x4a8701, Win 0x813120: y = L L^T x
void bvbMultiply(const MeReal* G, int numBlockColumns, int numBlocks, int n, const int* minColumn,
                 const int* maxColumn, const MeReal* x, MeReal* y);

// ---- smv_matrix (0x1868 bytes on both): dense, at most 16 rows ---------------------------------
class smv_matrix : public LCPMatrix {
public:
    smv_matrix() {}
    virtual void allocate(int n);                    // Mac 0x4ada6e, Win 0x810a30
    virtual void deallocate();                       // Mac 0x4ad9c6, Win 0x8114d0
    virtual void clear();                            // Mac 0x4ad9d6, Win 0x8114e0
    virtual MeReal set(int i, int j, MeReal value);  // Mac 0x4ad366, Win 0x810af0
    virtual MeReal get(int i, int j);                // Mac 0x4ad394, Win 0x810b30
    virtual MeReal getFactor(int i, int j);          // Mac 0x4ad3be, Win 0x810b60
    virtual void build(MdtKeaConstraints* c);        // Mac 0x4adae4, Win 0x810bc0
    virtual void factorize();                        // Mac 0x4ad586, Win 0x810d90
    virtual void multiply(const MeReal* x, MeReal* y); // Mac 0x4ad614, Win 0x810e60
    virtual void solve(MeReal* b, int flag);         // Mac 0x4ad5e8, Win 0x810e40
    virtual void getInverseColumn(MeReal* column, int i); // Mac 0x4ada0e, Win 0x810f10
    virtual void factorDense(MeReal* A, int n);      // Mac 0x4ad68e, Win 0x810f50
    virtual void solveDense(const MeReal* A, MeReal* b, int n); // Mac 0x4ad7e0, Win 0x8111e0
    virtual MeReal getDiagonal(int i);               // Mac 0x4ad3ec, Win 0x810ba0
    virtual const char* getName();                   // Mac 0x4ad354, Win 0x810a20

    // Mac 0x4ad406, Win 0x810cb0: builds the matrix of one block (constraint) of the iterative
    // solver: J_b M^-1 J_b^T of the rows of block `block` (name inferred)
    void buildBlock(const MdtKeaConstraints* c, int block);
    void copyToDense(MeReal* out);                   // Win 0x810aa0 (Win only, no caller) (name inferred)

    int mN;                 // +0x20
    int m24;                // +0x24
    MeReal mA[256];         // +0x28 (16 x 16, rows of mN)
    MeReal mFactor[256];    // +0x828
    MeReal mInverseCache[256]; // +0x1028 (mInverse points here)
    int mColumnSolvedStore[16]; // +0x1828 (mColumnSolved points here)
};

#endif
