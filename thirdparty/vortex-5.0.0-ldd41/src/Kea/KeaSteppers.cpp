// Vortex 5.0.0 build 800 "vortexldd41" - Kea steppers and the plain constraint solver
// (restored). See Kea/KeaFrames.h for the classes.
// One slot's step: setup (momenta, inverse masses, rhs0 = c / h - gamma error / h^2
// - J (momentum / h + force)), resetIterationCount, N x {prepareIteration (rhs = rhs0 - J M^-1
// previous constraint forces), solve, addConstraintForces}, computeAccelerations, integrate,
// cleanup.
#include "Kea/KeaFrames.h"
#include "Kea/MdtKeaConstraints.h"

#include <stddef.h>

// ---- constraint_solver ------------------------------------------------------------------------

// Mac 0x4af8f6, Win 0x75e110
void constraint_solver::setup(int n, LCPMatrix* matrix, MdtKeaConstraints* c, MdtKeaParameters* p,
                              KeaLcpFunction lcp, MemoryPool* pool, MdtKeaBody* const* bodies, int numBodies, int)
{
    mN = n;
    mMatrix = matrix;
    mConstraints = c;
    mParams = p;
    mLcp = lcp;
    mPool = pool;
    mBodies = bodies;
    mNumBodies = numBodies;
}

// Mac 0x4a5de0, Win 0x7b7b20
void std_constraint_solver::setup(int n, LCPMatrix* matrix, MdtKeaConstraints* c, MdtKeaParameters* p,
                                  KeaLcpFunction lcp, MemoryPool* pool, MdtKeaBody* const* bodies, int numBodies,
                                  int)
{
    mN = n;
    mMatrix = matrix;
    mConstraints = c;
    mParams = p;
    mLcp = lcp;
    mPool = pool;
    mBodies = bodies;
    mNumBodies = numBodies;
    matrix->allocate(c->numRows);
    mMatrix->build(mConstraints);
    mMatrix->factorize();
}

// Mac 0x4a5e4e, Win 0x7b7b80
void std_constraint_solver::solve()
{
    VindsSolveLcp(mMatrix, mConstraints, mParams, mLcp, mPool);
}

// Mac 0x4a5e80, Win 0x7076a0
void std_constraint_solver::cleanup()
{
}

// ---- WorldStepper -----------------------------------------------------------------------------

// Mac 0x4b11fa, Win 0x7b90b0
WorldStepper::WorldStepper()
    : m04(0), mRhs0(0), mConstraints(0), mParams(0), mBodies(0), mMatrix(0), mNumBodies(0), mH(1.0),
      mInvH(1.0), mLcp(0), mInvM(0), mPool(0), mSolver(0), mIteration(0)
{
}

// Mac 0x4b127a, Win 0x7b90f0
void WorldStepper::resetIterationCount()
{
    mIteration = 0;
}

// Mac 0x4b12c6, Win 0x7b91a0
void WorldStepper::slot2()
{
}

// Mac 0x4b14b4, Win 0x7b91b0
void WorldStepper::computeAccelerations()
{
    VindsComputeAccelerations(mBodies, mNumBodies, mInvM);
}

// Mac 0x4b1430, Win 0x7b91d0
void WorldStepper::integrate()
{
    VindsUpdateVelocities(mBodies, mNumBodies, &mParams->stepSize, mInvM);
    VindsUpdatePositions(mBodies, mNumBodies, &mParams->stepSize);
    VindsUpdateQuaternions(mBodies, mNumBodies, &mParams->stepSize, 2);
    VindsUpdateOrientationMatrices(mBodies, mNumBodies, &mParams->stepSize);
}

// Mac 0x4b128a, Win 0x7b9160: the whole step of one slot in one go (not used by MdtKeaStep)
void WorldStepper::step()
{
    resetIterationCount();
    solve();
    if (mConstraints->grouped == 0)
        integrate();
    cleanup();
}

// Mac 0x4b136c, Win 0x7b9280
void WorldStepper::prepareIteration()
{
    MdtKeaConstraints* c = mConstraints;
    if (c->numBlocks == 0)
        return;
    KeaCopy(c->numRows, mRhs0, 1, c->rhs, 1);
    if (++mIteration != 1)
        MdtKeaConstraintsAddForces(mConstraints, -1.0);
    c = mConstraints;
    MdtKeaConstraintsMultiplyBodies(-1.0, c, c->entryJM, 0, offsetof(MdtKeaBody, constraintForce), 1.0, c->rhs, 1);
}

// Mac 0x4b133a, Win 0x7b9310
void WorldStepper::addConstraintForces()
{
    if (mConstraints->numBlocks != 0)
        MdtKeaConstraintsAddForces(mConstraints, 1.0);
}

// Mac 0x4b12cc, Win 0x7b9230
void WorldStepper::cleanup()
{
    mSolver->cleanup();
    mPool->free(mInvM);
    mPool->free(mRhs0);
    mPool->reset();
    MdtKeaParameters* p = mParams;
    const int before = p->m64;
    p->m64 = mPool->takeOverflowBytes() + before;
}

// ---- kea_model --------------------------------------------------------------------------------

// Mac 0x4a7d2a, Win 0x7b9550
const char* kea_model::getName()
{
    return "vintage kea model: very stable but without non-inertial forces";
}

// Mac 0x4a7e2e, Win 0x7b9560
void kea_model::setup(int numBodies, MdtKeaConstraints* c, MdtKeaParameters* p, MdtKeaBody* const* bodies,
                      KeaLcpFunction lcp, LCPMatrix* matrix, constraint_solver* solver, MemoryPool* pool)
{
    mPool = pool;
    mConstraints = c;
    mParams = p;
    mBodies = bodies;
    mNumBodies = numBodies;
    mH = p->stepSize;
    mInvH = 1.0 / mH;
    mMatrix = matrix;
    mLcp = lcp;
    matrix->mPool = pool;
    mSolver = solver;
    mRhs0 = 0;
    mPool->setBuffer((char*)mParams->workspace, mParams->workspaceSize);
    VindsComputeWorldInertia(mBodies, mNumBodies, true);
    VindsComputeMomenta(mBodies, mNumBodies);
    mInvM = VindsComputeInverseMass(&mParams->stepSize, mBodies, mNumBodies, 1, mPool);
    if (mConstraints->numBlocks == 0)
        return;
    const int n = mConstraints->numRows;
    mRhs0 = (MeReal*)mPool->allocZeroed(n * 8, 0x10, "rhs0");
    MdtKeaConstraintsComputeJM(mConstraints, mInvM);
    KeaCopy(n, mConstraints->velocity, 1, mRhs0, 1);
    KeaScale(n, mInvH, mRhs0, 1);
    KeaMultiplyAdd(n, mInvH * -mInvH, mConstraints->gamma, 1, mConstraints->error, 1, 1.0, mRhs0, 1);
    MdtKeaConstraintsMultiplyBodies(-mInvH, mConstraints, mConstraints->entryJM, 0, offsetof(MdtKeaBody, m2a4),
                                    1.0, mRhs0, 1);
    MdtKeaConstraintsMultiplyBodies(-1.0, mConstraints, mConstraints->entryJM, 0, offsetof(MdtKeaBody, force),
                                    1.0, mRhs0, 1);
    mSolver->setup(n, mMatrix, mConstraints, mParams, mLcp, mPool, mBodies, mNumBodies, 0);
}

// Mac 0x4a7d3c, Win 0x7b9720
void kea_model::solve()
{
    MdtKeaConstraints* c = mConstraints;
    if (c->numBlocks == 0)
        return;
    KeaCopy(c->numRows, mRhs0, 1, c->rhs, 1);
    if (++mIteration != 1) {
        MdtKeaConstraintsAddForces(mConstraints, -1.0);
        c = mConstraints;
        MdtKeaConstraintsMultiplyBodies(-1.0, c, c->entryJM, 0, offsetof(MdtKeaBody, constraintForce), 1.0,
                                        c->rhs, 1);
    }
    mSolver->solve();
    MdtKeaConstraintsAddForces(mConstraints, 1.0);
}

// ---- StableStepper ----------------------------------------------------------------------------

// Mac 0x4adcaa, Win 0x7b9330
const char* StableStepper::getName()
{
    return "stable rotational model: very stable with non-inertial forces";
}

// Mac 0x4adcdc, Win 0x7b9340
void StableStepper::setup(int numBodies, MdtKeaConstraints* c, MdtKeaParameters* p, MdtKeaBody* const* bodies,
                          KeaLcpFunction lcp, LCPMatrix* matrix, constraint_solver* solver, MemoryPool* pool)
{
    mPool = pool;
    mConstraints = c;
    mParams = p;
    mBodies = bodies;
    mNumBodies = numBodies;
    mH = p->stepSize;
    mInvH = 1.0 / mH;
    mMatrix = matrix;
    mLcp = lcp;
    matrix->mPool = pool;
    mSolver = solver;
    // NOTE: unlike kea_model, rhs0 is not reset here (it stays from the previous step when the
    // slot has no constraints; cleanup's pool->free is a no-op)
    mPool->setBuffer((char*)mParams->workspace, mParams->workspaceSize);
    VindsComputeWorldInertia(mBodies, mNumBodies, false);
    VindsComputeMomenta(mBodies, mNumBodies);
    mInvM = VindsComputeInverseMassStable(mBodies, mNumBodies, &mParams->stepSize, 1.0, mPool);
    VindsAddGyroscopicTorques(mBodies, mNumBodies, 1.0);
    if (mConstraints->numBlocks == 0)
        return;
    const int n = mConstraints->numRows;
    const MeReal invH = mInvH;
    mRhs0 = (MeReal*)mPool->allocZeroed(n * 8, 0x10, "rhs0");
    MdtKeaConstraintsComputeJM(mConstraints, mInvM);
    KeaCopy(n, mConstraints->velocity, 1, mRhs0, 1);
    KeaScale(n, mInvH, mRhs0, 1);
    KeaMultiplyAdd(n, -(invH * invH), mConstraints->gamma, 1, mConstraints->error, 1, 1.0, mRhs0, 1);
    MdtKeaConstraintsMultiplyBodies(-mInvH, mConstraints, mConstraints->entryJM, 0, offsetof(MdtKeaBody, m2a4),
                                    1.0, mRhs0, 1);
    MdtKeaConstraintsMultiplyBodies(-1.0, mConstraints, mConstraints->entryJM, 0, offsetof(MdtKeaBody, force),
                                    1.0, mRhs0, 1);
    mSolver->setup(n, mMatrix, mConstraints, mParams, mLcp, mPool, mBodies, mNumBodies, 0);
}

// Mac 0x4adcbc, Win 0x7b9530
void StableStepper::solve()
{
    if (mConstraints->numBlocks != 0)
        mSolver->solve();
}
