// Vortex 5.0.0 build 800 "vortexldd41" - Kea numerics: BLAS-like vector helpers, the Vinds body
// integrator and the LCP solvers (restored). All names inferred unless a
// string names them; the Win build keeps several of the static helpers that GCC inlined on Mac
// (their Win addresses are given where they exist).
//  * src/Kea/KeaBlas.cpp      Mac 0x4a3933-0x4a3cc2, Win 0x80e520-0x80ee30
//  * src/Kea/Vinds.cpp        Mac 0x4a3d0d-0x4a47e9, 0x4aaa0b-0x4ab11a, Win 0x80ee30-0x810950
//  * src/Kea/KeaLcp.cpp       Mac 0x4a99ce-0x4aaa0b ("lcp_simple_hash_table"), Win 0x7bad50-0x7bbf10
//  * src/Kea/KeaSmvLcp.cpp    Mac 0x4ab55c-0x4ad354 (smv boxed Keller), Win 0x811570-0x812910 (sans 0x810xxx smv_matrix)
#ifndef KEA_KEAMATH_H
#define KEA_KEAMATH_H

#include "Kea/MdtKea.h"

struct MdtKeaConstraints;
class LCPMatrix;
class MemoryPool;

// ---- vector helpers (strided, BLAS style) -------------------------------------------------------
// Mac 0x4a3933, Win 0x80e520: y = x
void KeaCopy(int n, const MeReal* x, int incx, MeReal* y, int incy);
// Mac 0x4a396d, Win 0x80e640: x = alpha x (alpha == 0 stores zeros)
void KeaScale(int n, MeReal alpha, MeReal* x, int incx);
// Mac 0x4a39d9, Win 0x80e6f0: y = alpha x + y (alpha == 1: y = y + x)
void KeaAxpy(int n, MeReal alpha, const MeReal* x, int incx, MeReal* y, int incy);
// Mac 0x4a3a62, Win 0x80e840: y = y - x (contiguous)
void KeaSubtract(int n, const MeReal* x, MeReal* y);
// Mac 0x4a3a92, Win 0x80e8c0: z = alpha x y + beta z, element by element
void KeaMultiplyAdd(int n, MeReal alpha, const MeReal* x, int incx, const MeReal* y, int incy,
                    MeReal beta, MeReal* z, int incz);
// Mac 0x4a3cc2, Win 0x80e5c0: Euclidean norm
MeReal KeaNorm2(int n, const MeReal* x, int incx);

// ---- LCP ------------------------------------------------------------------------------------------
// Row states (MdtKeaConstraints::lcpState) (names inferred)
enum KeaLcpState {
    kLcpFree = 0,       // unbounded / strictly inside the bounds
    kLcpAtLower = 2,    // clamped to lo
    kLcpAtUpper = 4,    // clamped to hi
    kLcpFixed = 0x10,   // |lo| and |hi| below the tolerance: the row is held at 0
    kLcpZero = 0x20     // lambda forced to 0
};
// The LCP functions of the solver set (KeaSolverSet::lcp). x (solution), b (right hand side),
// lo / hi (bounds), state (KeaLcpState per row), returns the step count (negative when the
// iteration limit was reached).
typedef int (*KeaLcpFunction)(LCPMatrix* A, MeReal* x, const MeReal* b, const MeReal* lo,
                              const MeReal* hi, int* state, MdtKeaLcpParameters* params,
                              MemoryPool* pool, int unused);
// Mac 0x4a9e83 ("lcp_simple_hash_table"), Win 0x7bba90: boxed principal pivoting (Keller)
// with a hash table of the visited index sets to detect cycles
int lcp_simple_hash_table(LCPMatrix* A, MeReal* x, const MeReal* b, const MeReal* lo, const MeReal* hi,
                          int* state, MdtKeaLcpParameters* params, MemoryPool* pool, int unused);
// Mac 0x4ad202 (-> boxed Keller 0x4ab730), Win 0x811970: the LCP of one itr_constraint_solver
// block (an smv_matrix; pool unused). "smv boxed keller" (name inferred)
int KeaSmvLcp(LCPMatrix* A, MeReal* x, const MeReal* b, const MeReal* lo, const MeReal* hi, int* state,
              MdtKeaLcpParameters* params, MemoryPool* pool, int unused);
// Mac 0x4a99ce, Win 0x7bb860: Bob Jenkins' lookup2 hash of n bytes (key, length, initval)
unsigned int KeaLcpHash(const unsigned char* key, unsigned int length, unsigned int initval);
// Mac 0x4a9c1b, Win 0x7bb3f0: Coulomb bounds of the friction rows from the normal force
// (row i is a friction row of normal row i - normalOffset[i]); returns the rows changed
int KeaUpdateFrictionBounds(int n, const MeReal* lambda, MeReal* lo, MeReal* hi, const int* normalOffset,
                            const MeReal* mu, const MeReal* cohesion, int* state);
// Win 0x7bb590 (Win only, no callers): friction rows whose normal force exceeds 0.1 become
// unbounded, rows with no normal force or mu == 0 are fixed at 0; returns the friction rows
int KeaClampFrictionBounds(int n, const MeReal* lambda, MeReal* lo, MeReal* hi, const int* normalOffset,
                           const MeReal* mu, int* state);
// Win 0x7bb670 (Win only, no callers): clamps the friction forces themselves into
// [-mu lambda_normal, mu lambda_normal] (mu == DBL_MAX: unbounded); returns the rows checked
int KeaScaleFrictionBounds(int n, MeReal* lambda, void* unused1, void* unused2, const int* normalOffset,
                           const MeReal* mu);

// ---- Vinds: the body side of a Kea step -----------------------------------------------------------
// Mac 0x4a3d0d, Win 0x80ee30: solves the LCP of the slot (+ friction refinement passes); returns
// the pivoting steps
int VindsSolveLcp(LCPMatrix* A, MdtKeaConstraints* c, MdtKeaParameters* p, KeaLcpFunction lcp,
                  MemoryPool* pool);
// Mac 0x4a400b, Win 0x80fae0: com += h vel; comTM[3][3] = 1
void VindsUpdatePositions(MdtKeaBody* const* bodies, int n, const MeReal* h);
// Mac 0x4a409c, Win 0x810340: comTM rotation from qrot
void VindsUpdateOrientationMatrices(MdtKeaBody* const* bodies, int n, const MeReal* h);
// Mac 0x4a41e9, Win 0x810410: torque += scale (Iw x w) (gyroscopic term)
void VindsAddGyroscopicTorques(MdtKeaBody* const* bodies, int n, MeReal scale);
// Mac 0x4a42a8, Win 0x810670: momenta m2a4 = mass v, m2c4 = I w
void VindsComputeMomenta(MdtKeaBody* const* bodies, int n);
// Mac 0x4a4333, Win 0x810870 (+ 0x810750): world inertia tensors (I and invI) from comTM
void VindsComputeWorldInertia(MdtKeaBody* const* bodies, int n, bool spherical);
// Mac 0x4a4663, Win 0x80fc90: velocities from momenta + h accelerations
void VindsUpdateVelocities(MdtKeaBody* const* bodies, int n, const MeReal* h, const MeReal* invM);
// Mac 0x4a47e9, Win 0x80ff00 ("Vinds:: body_update_quaternion"): method 1 = linear (q += h/2 G w),
// 2 = finite rotation, 3 = implicit midpoint
void VindsUpdateQuaternions(MdtKeaBody* const* bodies, int n, const MeReal* h, int method);
// Mac 0x4aaa0b, Win 0x80f0d0: per body 12 doubles: invI (3 rows of 4, [3] = 1/mass) of the
// damped body (+ h damping), allocated in the pool ("inv M")
MeReal* VindsComputeInverseMass(const MeReal* h, MdtKeaBody* const* bodies, int n, int spherical,
                                MemoryPool* pool);
// Mac 0x4aacea, Win 0x80f660 (+ 0x80f3c0): the same with the stable rotational model's
// gyroscopic correction (scale * h^2 * Iw S(w) ...)
MeReal* VindsComputeInverseMassStable(MdtKeaBody* const* bodies, int n, const MeReal* h, MeReal scale,
                                      MemoryPool* pool);
// Mac 0x4aabc8, Win 0x80f500: accelerations from forces (+ constraint forces)
void VindsComputeAccelerations(MdtKeaBody* const* bodies, int n, const MeReal* invM);
// Win-only helpers without callers (Win 0x80f040 = 0x80f8d0, 0x80f900, 0x80f070 = 0x80f930,
// 0x810370, 0x8103c0)
void VindsVector3AddScaled(MeReal* y, const MeReal* x, MeReal s);
void VindsVector3ScaleAdd(MeReal* out, const MeReal* a, const MeReal* x, MeReal s);
void VindsVector3Scale(MeReal* out, const MeReal* x, MeReal s);
void VindsGatherBodyVectors(MeReal* out, MdtKeaBody* const* bodies, int n, int field, int count);
void VindsScatterBodyVectors(MdtKeaBody* const* bodies, int n, const MeReal* in, int field, int count);

#endif
