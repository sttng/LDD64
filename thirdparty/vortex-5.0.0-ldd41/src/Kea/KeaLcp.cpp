// Vortex 5.0.0 build 800 "vortexldd41" - the Kea LCP solver "lcp_simple_hash_table" and the
// friction bound updates (restored). Mac sub-TU 556 0x4a99ce-0x4aaa0b, Win
// 0x7bad50-0x7bbf10 (Win keeps the workspace and the steps as separate helpers: 0x7bad90 alloc,
// 0x7baed0 free, 0x7baf50 z0, 0x7bb110 tight set, 0x7bb140 mjj, 0x7bb230 qjj / solve / x,
// 0x7bb000 index set update, 0x7bb860 hash, 0x7bbeb0 hash insert).
// Boxed LCP A x = b + w, lo <= x <= hi, by principal pivoting on the tight rows (state != 0):
// x = z0 + A^-1_T q puts them on their bounds, w = A x - b gives the next index set; seen sets are
// hashed (lookup2) and once cycling the update stops at the first change.
// NOTE: Win gates the solve on the Mdt license status (DAT_00af2d60 != 0: x, w zeroed, maxIterations
// 0); the restored code is the licensed path (licensing is V6).
#include "Kea/KeaMath.h"
#include "Kea/LCPMatrix.h"
#include "Kea/MemoryPool.h"
#include "Me/MeMessage.h"

#include <float.h>
#include <math.h>
#include <string.h>

#define KEA_LCP_MIX(a, b, c)                                                                          \
    {                                                                                                 \
        a -= b; a -= c; a ^= (c >> 13);                                                               \
        b -= c; b -= a; b ^= (a << 8);                                                                \
        c -= a; c -= b; c ^= (b >> 13);                                                               \
        a -= b; a -= c; a ^= (c >> 12);                                                               \
        b -= c; b -= a; b ^= (a << 16);                                                               \
        c -= a; c -= b; c ^= (b >> 5);                                                                \
        a -= b; a -= c; a ^= (c >> 3);                                                                \
        b -= c; b -= a; b ^= (a << 10);                                                               \
        c -= a; c -= b; c ^= (b >> 15);                                                               \
    }

// Mac 0x4a99ce (key in EAX, count of ints in EDX, initval 0), Win 0x7bb860 (key in ECX,
// initval in EAX, length on the stack): Bob Jenkins' lookup2 hash
unsigned int KeaLcpHash(const unsigned char* k, unsigned int length, unsigned int initval)
{
    unsigned int a = 0x9e3779b9, b = 0x9e3779b9, c = initval;
    unsigned int len = length;
    while (len >= 12) {
        a += k[0] + ((unsigned int)k[1] << 8) + ((unsigned int)k[2] << 16) + ((unsigned int)k[3] << 24);
        b += k[4] + ((unsigned int)k[5] << 8) + ((unsigned int)k[6] << 16) + ((unsigned int)k[7] << 24);
        c += k[8] + ((unsigned int)k[9] << 8) + ((unsigned int)k[10] << 16) + ((unsigned int)k[11] << 24);
        KEA_LCP_MIX(a, b, c);
        k += 12;
        len -= 12;
    }
    c += length;
    switch (len) {
    case 11: c += (unsigned int)k[10] << 24;
    case 10: c += (unsigned int)k[9] << 16;
    case 9: c += (unsigned int)k[8] << 8;
    case 8: b += (unsigned int)k[7] << 24;
    case 7: b += (unsigned int)k[6] << 16;
    case 6: b += (unsigned int)k[5] << 8;
    case 5: b += k[4];
    case 4: a += (unsigned int)k[3] << 24;
    case 3: a += (unsigned int)k[2] << 16;
    case 2: a += (unsigned int)k[1] << 8;
    case 1: a += k[0];
    default: break;
    }
    KEA_LCP_MIX(a, b, c);
    return c;
}

static inline int RoundUp4(int n)
{
    if ((n & 3) != 0)
        n = (n + 4) - n % 4;
    return n;
}

// Mac 0x4a9e83, Win 0x7bba90 ("lcp_simple_hash_table")
int lcp_simple_hash_table(LCPMatrix* A, MeReal* x, const MeReal* b, const MeReal* lo, const MeReal* hi,
                          int* state, MdtKeaLcpParameters* params, MemoryPool* pool, int)
{
    const int stride = RoundUp4(A->mSize);
    pool->push();
    const unsigned int keyCapacity = params->maxIterations + 2;
    unsigned int* keys = (unsigned int*)pool->alloc(keyCapacity * 4, 0x10, "hash keys");
    const int n0 = A->mSize;
    const int n4 = RoundUp4(n0);
    const int iter4 = RoundUp4(params->maxIterations);
    MeReal* z0 = (MeReal*)pool->alloc(n4 * 8, 0x10, "z0");
    MeReal* zn = (MeReal*)pool->alloc(n4 * 8 * iter4, 0x10, "zn");
    MeReal* w = (MeReal*)pool->alloc(n4 * 8, 0x10, "w");
    int* P = (int*)pool->alloc(n4 << 2, 0x10, "P");
    memset(w, 0, n4 * 8);
    for (int i = 0; i < n0; ++i)
        P[i] = i;

    // z0 = A^-1 b
    {
        const int n = A->mSize;
        const int padded = RoundUp4(n);
        memcpy(x, b, n * 8);
        memset(x + n, 0, (padded - n) * 8);
        A->solve(x, 1);
        memcpy(z0, x, n * 8);
        memset(z0 + n, 0, (padded - n) * 8);
        // NOTE: copies `padded` bytes, not doubles (zn is never read)
        if (zn != 0)
            memcpy(zn, z0, padded);
    }
    if (params->warmStart == 0)
        memset(state, 0, A->mSize << 2);
    {
        const MeReal tol = params->tolerance;
        for (int i = 0; i < A->mSize; ++i)
            if (fabs(lo[i]) < tol && fabs(hi[i]) < tol)
                state[i] = kLcpFixed | kLcpZero;
    }
    const int nq = RoundUp4(A->mSize);
    MeReal* qjj = (MeReal*)pool->alloc(nq * 8, 0x10, "qjj");
    MeReal* mjj = (MeReal*)pool->alloc(nq * nq * 8, 0x10, "mjj");
    int* tight = (int*)pool->alloc(nq * 4, 0x10, "tight");

    keys[0] = KeaLcpHash((const unsigned char*)state, A->mSize * 4, 0);
    if (keyCapacity < 2)
        MeFatalError(0xc, "lcp_simple_hash_table: error in size: current item: %d size = %d\n", 0, keyCapacity);

    int iter = 0;
    int maxIterations = params->maxIterations;
    if (maxIterations > 0) {
        bool cycleCheck = false;   // stop the index set update at the first change
        bool cycleFound = false;
        int cycleCount = 0;
        unsigned int numKeys = 1;
        bool done;
        do {
            // the tight rows
            int n = A->mSize;
            int m = 0;
            if (n > 0) {
                for (int i = 0; i < n; ++i) {
                    const int r = P[i];
                    if (state[r] != 0)
                        tight[m++] = r;
                }
                if (m >= 1) {
                    // mjj = the tight principal submatrix of A^-1 (columns computed on demand)
                    MeReal* row = mjj;
                    for (int a = 0; a < m; ++a, row += m) {
                        const int t = tight[a];
                        MeReal* column = A->mInverse + A->mInverseStride * t;
                        if (++A->mColumnSolved[t] == 1)
                            A->getInverseColumn(column, t);
                        for (int c = 0; c < m; ++c)
                            row[c] = column[tight[c]];
                    }
                    n = A->mSize;
                    for (int a = 0; a < m; ++a) {
                        const int t = tight[a];
                        const MeReal q = -z0[t];
                        qjj[a] = q;
                        if ((state[t] & kLcpAtLower) != 0)
                            qjj[a] = q + lo[t];
                        else if ((state[t] & kLcpAtUpper) != 0)
                            qjj[a] = q + hi[t];
                    }
                } else {
                    n = A->mSize;
                }
            }
            A->factorDense(mjj, m);
            A->solveDense(mjj, qjj, m);
            memcpy(x, z0, n * 8);
            if (m > 0) {
                for (int a = 0; a < m; ++a) {
                    const MeReal* column = A->mInverse + A->mInverseStride * tight[a];
                    const MeReal q = qjj[a];
                    for (int i = 0; i < n; ++i)
                        x[i] = q * column[i] + x[i];
                }
                for (int a = 0; a < m; ++a) {
                    const int t = tight[a];
                    if ((state[t] & kLcpAtLower) != 0)
                        x[t] = lo[t];
                    else if ((state[t] & kLcpAtUpper) != 0)
                        x[t] = hi[t];
                    else if ((state[t] & kLcpZero) != 0)
                        x[t] = 0;
                }
            }
            memcpy(zn + iter * stride, x, stride << 3);
            // w = A x - b
            A->multiply(x, w);
            for (int i = 0; i < A->mSize; ++i) {
                w[i] = w[i] - b[i];
                if ((state[i] & kLcpFixed) != 0)
                    w[i] = 0;
            }
            // the new index set
            const MeReal tol = params->tolerance;
            int changes = 0;
            for (int k = 0; k != n0; ++k) {
                const int i = P[k];
                const int s = state[i];
                int ns = s;
                if ((s & kLcpFixed) == 0) {
                    int keepUpper, lowerSel;
                    const MeReal wi = w[i];
                    if (s == kLcpAtUpper) {
                        keepUpper = (wi < tol) << 2;
                        lowerSel = 0;
                    } else {
                        lowerSel = (s == kLcpAtLower) * 2;
                        keepUpper = 0;
                    }
                    // NOTE: a lower row below lo - tol with -tol < w sums to 4 (upper)
                    ns = (tol + hi[i] < x[i]) * 4 + (x[i] < lo[i] - tol) * 2 + keepUpper
                         + (-tol < wi) * lowerSel;
                }
                changes += (ns != s);
                state[i] = ns;
                if (cycleCheck && changes != 0)
                    break;
            }
            done = changes == 0;
            if (params->useHashTable == 0) {
                if (cycleFound)
                    ++cycleCount;
            } else {
                const unsigned int h = KeaLcpHash((const unsigned char*)state, A->mSize * 4, 0);
                unsigned int stored;
                unsigned int idx = 0;
                bool insert = true;
                if (numKeys != 0) {
                    while (h != keys[idx]) {
                        if (++idx == numKeys)
                            break;
                    }
                    insert = idx == numKeys;
                }
                if (insert) {
                    numKeys = idx + 1;
                    if (keyCapacity <= numKeys)
                        MeFatalError(0xc, "lcp_simple_hash_table: error in size: current item: %d size = %d\n",
                                     idx, keyCapacity);
                    keys[idx] = h;
                    stored = h;
                } else {
                    stored = 0xffffffffu;  // seen before
                }
                if (h == stored) {
                    if (cycleFound)
                        ++cycleCount;
                } else if (cycleCheck) {
                    // NOTE: the second cycle ends the solve with a negated count
                    iter = -iter;
                    done = true;
                    if (cycleFound)
                        ++cycleCount;
                } else {
                    cycleCheck = true;
                    cycleFound = true;
                    numKeys = 0;
                    ++cycleCount;
                }
            }
            if (cycleCount > 5) {
                cycleCheck = false;
                cycleCount = 0;
                cycleFound = false;
                numKeys = 0;
            }
            if (iter > params->cycleCheckStart)
                cycleCheck = true;
            ++iter;
            maxIterations = params->maxIterations;
        } while (iter < maxIterations && !done);
    }
    const int result = iter != maxIterations ? iter : -iter;
    if (z0 != 0)
        pool->free(z0);
    if (zn != 0)
        pool->free(zn);
    if (w != 0)
        pool->free(w);
    if (P != 0)
        pool->free(P);
    if (qjj != 0)
        pool->free(qjj);
    if (mjj != 0)
        pool->free(mjj);
    if (tight != 0)
        pool->free(tight);
    pool->pop();
    if (keys != 0)
        pool->free(keys);
    return result;
}

// Mac 0x4a9c1b, Win 0x7bb3f0
int KeaUpdateFrictionBounds(int n, const MeReal* lambda, MeReal* lo, MeReal* hi, const int* normalOffset,
                            const MeReal* mu, const MeReal* cohesion, int* state)
{
    int count = 0;
    for (int i = 0; i < n; ++i) {
        const int offset = normalOffset[i];
        if (offset == -1)
            continue;
        MeReal normal = lambda[i - offset];
        MeReal coefficient = mu[i];
        MeReal bound = cohesion[i];
        if (!(fabs(normal) <= DBL_MAX)) {
            MeWarning(0xc, "Overriding a normal force that was infinite.\n");
            normal = 0.0;
        }
        if (!(fabs(coefficient) <= DBL_MAX)) {
            MeWarning(0xc, "Overriding an friction coefficient that was infinite\n");
            coefficient = 0.0;
        }
        if (!(fabs(bound) <= DBL_MAX)) {
            MeWarning(0xc, "Overriding a cohesion coefficient that was infinite\n");
            bound = 0.0;
        }
        state[i] = 0;
        ++count;
        if (normal > 0.0 && coefficient != 0.0) {
            bound = coefficient * normal + bound;
            hi[i] = bound;
            lo[i] = -bound;
        } else if (normal < 0.0 && coefficient != 0.0) {
            bound = bound - coefficient * normal;
            hi[i] = bound;
            lo[i] = -bound;
        } else if (normal == 0.0 || coefficient == 0.0) {
            hi[i] = bound;
            lo[i] = -bound;
        }
    }
    return count;
}

// Win 0x7bb590 (Win only)
int KeaClampFrictionBounds(int n, const MeReal* lambda, MeReal* lo, MeReal* hi, const int* normalOffset,
                           const MeReal* mu, int* state)
{
    if (n <= 0)
        return 0;
    int count = 0;
    for (int i = 0; i < n; ++i) {
        if (normalOffset[i] == -1)
            continue;
        ++count;
        const MeReal normal = lambda[i - normalOffset[i]];
        const MeReal coefficient = mu[i];
        state[i] = 0;
        if (fabs(normal) <= 0.1) {
            if (normal == 0.0 || coefficient == 0.0) {
                hi[i] = 0;
                lo[i] = 0;
            }
        } else {
            hi[i] = DBL_MAX;
            lo[i] = -DBL_MAX;
        }
    }
    return count;
}

// Win 0x7bb670 (Win only)
int KeaScaleFrictionBounds(int n, MeReal* lambda, void*, void*, const int* normalOffset, const MeReal* mu)
{
    int count = 0;
    for (int i = 0; i < n; ++i) {
        if (normalOffset[i] == -1 || mu[i] == DBL_MAX)
            continue;
        ++count;
        const MeReal bound = lambda[i - normalOffset[i]] * mu[i];
        if (-bound <= lambda[i]) {
            if (bound < lambda[i])
                lambda[i] = bound;
        } else {
            lambda[i] = -bound;
        }
    }
    return count;
}
