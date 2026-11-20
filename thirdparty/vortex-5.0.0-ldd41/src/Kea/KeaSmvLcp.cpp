// Vortex 5.0.0 build 800 "vortexldd41" - "smv boxed keller": the LCP of one block of
// itr_constraint_solver (restored). Mac sub-TU 556 0x4ab55c-0x4ad354 (the
// solver inlined in 0x4ab730), Win 0x811570-0x812910 (the solver split into helpers: 0x8117f0
// main, 0x812640 init, 0x811c60 initial states, 0x8127c0 free pivots, 0x811dc0 entering row,
// 0x811ce0 column, 0x811ef0 ratio test, 0x812800 update, 0x8121c0 pivot, 0x811b60 index sets,
// 0x812410 z, 0x811c00 residual, 0x8126b0 output). Not used by LDD (see KeaItrSolver.cpp).
// Boxed LCP w = A x + q, lo <= x <= hi (q = -b, <= 16 rows) by Keller's principal pivoting: M is the
// inverse of the free block, z = M (q + A x_bound); a bound row with wrong-signed z enters the free
// set, the ratio test bounds the step. Struct and field names are inferred.
#include "Kea/KeaMath.h"
#include "Kea/LCPMatrix.h"
#include "Me/MeMessage.h"

#include <float.h>
#include <math.h>
#include <string.h>

namespace {

// The problem (Mac / Win 0xa4 bytes on the stack of KeaSmvLcp)
struct SmvKellerProblem {
    char m00[0x40];
    int n;                  // +0x40
    const MeReal* A;        // +0x44 rows of n (the smv_matrix mA)
    const MeReal* q;        // +0x48 -b
    const MeReal* lo;       // +0x4c
    const MeReal* hi;       // +0x50
    MeReal* x;              // +0x54 solution
    MeReal* w;              // +0x58 w of the bound rows
    int ownState[16];       // +0x5c (used when no state array is passed)
    int* state;             // +0x9c 0 free, 2 at lo, 4 at hi
    int mA0;                // +0xa0
};

// Mac 0x4ab690 (via 0x4ab6e4), Win 0x811700 (+ Mac 0x4ab642 / Win 0x811570)
void ProblemInit(SmvKellerProblem* p, int n, const MeReal* A, const MeReal* q, const MeReal* lo,
                 const MeReal* hi, MeReal* x, MeReal* w, int* state)
{
    p->n = n;
    p->A = A;
    p->q = q;
    p->lo = lo;
    p->hi = hi;
    p->x = x;
    p->w = w;
    p->state = state;
    p->mA0 = 0;
    memset(p->ownState, 0, p->n * 4);
    if (state != 0)
        p->state = state;
    else
        p->state = p->ownState;
}

// The solver (Mac 0x13f4 / Win 0x13f8 + 16 doubles)
struct SmvKeller {
    int m00;                // +0x000
    int n;                  // +0x004
    MeReal epsilon;         // +0x008 (1e-12)
    int freeRows[16];       // +0x010
    int numFree;            // +0x050
    int boundRows[16];      // +0x054
    int numBound;           // +0x094
    int lowerRows[16];      // +0x098
    int numLower;           // +0x0d8
    int upperRows[16];      // +0x0dc
    int numUpper;           // +0x11c
    MeReal column[16];      // +0x120 M A e_pivot
    MeReal tmp[16];         // +0x1a0
    MeReal tmp2[16];        // +0x220
    MeReal M[256];          // +0x2a0
    int direction;          // +0xaa0 entering direction (+1 / -1), then the ratio test case (+-1..4)
    MeReal step;            // +0xaa4 / Win +0xaa8
    MeReal stepPivot;       // its own bound (0 at the pivot's zero crossing)
    MeReal stepRange;       // the box width
    MeReal stepLower;       // a free row reaching lo
    MeReal stepUpper;       // a free row reaching hi
    int pivot;              // Mac +0xacc / Win +0xad0
    int blocking;           // a free row that left the free set (-1: none)
    int lowerRow;
    int upperRow;
    MeReal diagonal;        // Mac +0xadc / Win +0xae0
    MeReal history[32];     // residual norms
    int historyStates[32][16];
    unsigned int numHistory;// Mac +0x13e4 / Win +0x13e8
    int iteration;
    int done;
    SmvKellerProblem* problem; // Mac +0x13f0 / Win +0x13f4
    MeReal z[16];           // Mac +0x13f4 / Win +0x13f8
};

// Mac 0x4ab55c (via 0x4ab620), Win 0x811660
void SolverInit(SmvKeller* s, SmvKellerProblem* p)
{
    s->m00 = 0;
    s->n = p->n;
    s->epsilon = 1e-12;
    s->direction = 0;
    s->step = -1.0;
    s->stepPivot = -1.0;
    s->stepRange = -1.0;
    s->stepLower = -1.0;
    s->stepUpper = -1.0;
    s->pivot = -1;
    s->blocking = -1;
    s->lowerRow = -1;
    s->upperRow = -1;
    s->diagonal = 0;
    s->numHistory = 0;
    s->iteration = 0;
    s->done = 0;
    s->problem = p;
}

// Win 0x811b60: the index sets from the row states
void Classify(SmvKeller* s)
{
    s->numUpper = 0;
    s->numLower = 0;
    s->numBound = 0;
    s->numFree = 0;
    for (int i = 0; i < s->n; ++i) {
        const int state = s->problem->state[i];
        if (state == 0) {
            s->freeRows[s->numFree++] = i;
        } else {
            s->boundRows[s->numBound++] = i;
            if (state == 2)
                s->lowerRows[s->numLower++] = i;
            else
                s->upperRows[s->numUpper++] = i;
        }
    }
}

// Win 0x8121c0: exchanges row p into M; fromA: the column comes from A (p becomes free), else
// from M (p leaves the free set)
void Pivot(SmvKeller* s, int p, int fromA)
{
    const int n = s->n;
    int i, k;
    if (fromA == 0) {
        for (i = 0; i < n; ++i)
            s->tmp[i] = s->M[p + i * n];
    } else {
        const MeReal* A = s->problem->A;
        for (i = 0; i < n; ++i)
            s->tmp2[i] = A[p + i * n];
        for (i = 0; i < n; ++i) {
            MeReal sum = 0.0;
            for (k = 0; k < n; ++k)
                sum = sum - s->M[i * n + k] * s->tmp2[k];
            s->tmp[i] = sum;
        }
    }
    for (k = 0; k < n; ++k)
        s->tmp2[k] = s->M[p * n + k];
    const MeReal inv = 1.0 / s->tmp[p];
    for (i = 0; i < p; ++i)
        for (k = 0; k < n; ++k)
            s->M[i * n + k] = s->M[i * n + k] - inv * s->tmp[i] * s->tmp2[k];
    for (i = p + 1; i < n; ++i)
        for (k = 0; k < n; ++k)
            s->M[i * n + k] = s->M[i * n + k] - inv * s->tmp[i] * s->tmp2[k];
    for (k = 0; k < n; ++k)
        s->M[p * n + k] = s->tmp2[k] * inv;
}

// Win 0x812410: x of the bound rows, then z = M (q + A x)
void ComputeZ(SmvKeller* s)
{
    SmvKellerProblem* p = s->problem;
    const int n = s->n;
    int i, k;
    memset(p->x, 0, n * 8);
    for (i = 0; i < s->numLower; ++i)
        p->x[s->lowerRows[i]] = p->lo[s->lowerRows[i]];
    for (i = 0; i < s->numUpper; ++i)
        p->x[s->upperRows[i]] = p->hi[s->upperRows[i]];
    for (i = 0; i < n; ++i) {
        MeReal sum = p->q[i];
        const MeReal* a = p->A + i * n;
        for (k = 0; k < n; ++k)
            sum = sum + a[k] * p->x[k];
        s->tmp[i] = sum;
    }
    for (i = 0; i < n; ++i) {
        MeReal sum = 0.0;
        for (k = 0; k < n; ++k)
            sum = sum + s->M[i * n + k] * s->tmp[k];
        s->z[i] = sum;
    }
}

// Win 0x811dc0: the bound row whose z has the wrong sign the most
void PickPivot(SmvKeller* s)
{
    s->done = 0;
    s->direction = -1;
    s->pivot = -1;
    int lower = -1;
    MeReal lowerMin = 0.0;
    for (int i = 0; i < s->numLower; ++i) {
        const int r = s->lowerRows[i];
        if (s->z[r] < lowerMin) {
            lower = r;
            lowerMin = s->z[r];
        }
    }
    int upper = -1;
    MeReal upperMax = 0.0;
    for (int i = 0; i < s->numUpper; ++i) {
        const int r = s->upperRows[i];
        if (upperMax < s->z[r]) {
            upperMax = s->z[r];
            upper = r;
        }
    }
    if (lower == -1 && upper == -1) {
        s->done = 1;
        return;
    }
    if (lower == -1 || -upperMax < lowerMin) {
        s->pivot = upper;
        s->direction = -1;
    } else if (upper == -1 || lowerMin <= -upperMax) {
        s->pivot = lower;
        s->direction = 1;
    } else {
        MeFatalError(1, "Internal solver error: smv boxed keller: problem in picking pivot");
    }
}

// Win 0x811ce0: column = M A e_pivot, diagonal = A_pp + A_p,free . column_free
void ComputeColumn(SmvKeller* s)
{
    const SmvKellerProblem* p = s->problem;
    const int n = s->n;
    for (int i = 0; i < n; ++i) {
        const MeReal* a = p->A + s->pivot;
        const MeReal* m = s->M + i * n;
        MeReal sum = 0.0;
        for (int k = 0; k < n; ++k, a += n)
            sum = sum + m[k] * *a;
        s->column[i] = sum;
    }
    s->diagonal = p->A[s->pivot * (n + 1)];
    for (int i = 0; i < s->numFree; ++i) {
        const int f = s->freeRows[i];
        s->diagonal = p->A[s->pivot * s->n + f] * s->column[f] + s->diagonal;
    }
}

// Win 0x811ef0
void RatioTest(SmvKeller* s)
{
    const int direction = s->direction;
    const MeReal start = direction < 1 ? -DBL_MAX : DBL_MAX;
    s->stepUpper = start;
    s->stepLower = start;
    s->upperRow = -1;
    s->stepRange = start;
    s->lowerRow = -1;
    s->stepPivot = start;
    if (0.0 < s->diagonal)
        s->stepPivot = -(s->z[s->pivot] / s->diagonal);
    const SmvKellerProblem* p = s->problem;
    const MeReal range = p->hi[s->pivot] - p->lo[s->pivot];
    s->stepRange = range;
    if (direction < 0)
        s->stepRange = -range;
    for (int i = 0; i < s->numFree; ++i) {
        const int f = s->freeRows[i];
        MeReal c = s->column[f];
        MeReal inv = 1.0 / c;
        if (direction < 0) {
            c = -c;
            inv = -inv;
        }
        if (!(0.0 <= c)) {
            const MeReal t = (p->lo[f] - s->z[f]) * inv;
            if (direction >= 1 && !(s->stepLower <= t)) {
                s->stepLower = t;
                s->lowerRow = f;
            } else if (direction < 0 && t < -s->stepLower) {
                s->lowerRow = f;
                s->stepLower = -t;
            }
        } else if (0.0 < c) {
            const MeReal t = (p->hi[f] - s->z[f]) * inv;
            if (direction >= 1 && !(s->stepUpper <= t)) {
                s->stepUpper = t;
                s->upperRow = f;
            } else if (direction < 0 && t < -s->stepUpper) {
                s->upperRow = f;
                s->stepUpper = -t;
            }
        }
    }
    s->step = s->stepPivot;
    if (direction < 1) {
        s->direction = -1;
        if (s->step < s->stepRange) {
            s->direction = -2;
            s->step = s->stepRange;
        }
        if (s->step < s->stepLower) {
            s->direction = -3;
            s->step = s->stepLower;
        }
        if (s->step < s->stepUpper) {
            s->direction = -4;
            s->step = s->stepUpper;
        }
    } else {
        s->direction = 1;
        if (s->stepRange < s->step) {
            s->direction = 2;
            s->step = s->stepRange;
        }
        if (s->stepLower < s->step) {
            s->direction = 3;
            s->step = s->stepLower;
        }
        if (s->stepUpper < s->step) {
            s->direction = 4;
            s->step = s->stepUpper;
        }
    }
}

// Win 0x812800
void Update(SmvKeller* s)
{
    const int d = s->direction;
    int* state = s->problem->state;
    s->blocking = -1;
    if (d == 1 || d == -1) {
        state[s->pivot] = 0;
        Pivot(s, s->pivot, 1);
    } else if (d == 2 || d == -2) {
        Pivot(s, s->pivot, 0);
        state[s->pivot] = (0 < s->direction) * 2 + 2;
    } else if (d == 3 || d == -3) {
        s->blocking = s->lowerRow;
        Pivot(s, s->lowerRow, 0);
        state[s->blocking] = 2;
    } else if (d == 4 || d == -4) {
        s->blocking = s->upperRow;
        Pivot(s, s->upperRow, 0);
        state[s->blocking] = 4;
    }
}

// Win 0x811c00: the norm of the wrong-signed z of the bound rows
MeReal Residual(const SmvKeller* s)
{
    MeReal sum = 0.0;
    for (int i = 0; i < s->n; ++i) {
        const MeReal v = s->z[i];
        const int state = s->problem->state[i];
        if (state == 2) {
            if (v < 0.0)
                sum = sum + v * v;
        } else if (state == 4 && 0.0 < v) {
            sum = sum + v * v;
        }
    }
    return sqrt(sum);
}

// Win 0x8126b0
void Output(SmvKeller* s)
{
    SmvKellerProblem* p = s->problem;
    int i;
    memset(p->x, 0, s->n * 8);
    memset(p->w, 0, s->n * 8);
    for (i = 0; i < s->numLower; ++i)
        p->x[s->lowerRows[i]] = p->lo[s->lowerRows[i]];
    for (i = 0; i < s->numUpper; ++i)
        p->x[s->upperRows[i]] = p->hi[s->upperRows[i]];
    for (i = 0; i < s->numFree; ++i)
        p->x[s->freeRows[i]] = s->z[s->freeRows[i]];
    for (i = 0; i < s->numBound; ++i)
        p->w[s->boundRows[i]] = s->z[s->boundRows[i]];
}

// solve calls (Mac DAT_007801f0, Win DAT_00afc83c; statistics, never read)
int gSmvKellerSolves = 0;

// Mac 0x4ab730, Win 0x8117f0
int SmvKellerSolve(SmvKeller* s)
{
    ++gSmvKellerSolves;
    // Win 0x812640
    const int n = s->n;
    memset(s->M, 0, n * n * 8);
    for (int i = 0; i < n; ++i)
        s->M[i * (n + 1)] = 1.0;
    s->done = 0;
    s->direction = 0;
    s->iteration = 0;
    s->numHistory = 0;
    // Win 0x811c60
    SmvKellerProblem* p = s->problem;
    for (int i = 0; i < p->n; ++i) {
        int state = 2;
        if (p->lo[i] == -DBL_MAX)
            state = (p->hi[i] != DBL_MAX) << 2;
        p->state[i] = state;
    }
    Classify(s);
    // Win 0x8127c0
    for (int i = 0; i < s->numFree; ++i)
        Pivot(s, s->freeRows[i], 1);
    ComputeZ(s);
    s->iteration = 0;
    while (s->done == 0 && s->iteration < 15) {
        PickPivot(s);
        if (s->done != 0)
            break;
        do {
            ComputeColumn(s);
            RatioTest(s);
            if (s->step == DBL_MAX || s->step == -DBL_MAX) {
                MeFatalError(1, "Internal solver error: smv_boxed_keller: terminating on a ray\n");
                s->done = 1;
                break;
            }
            Update(s);
            Classify(s);
            ComputeZ(s);
            ++s->iteration;
            if (s->numHistory < 32) {
                MeReal r = Residual(s);
                s->done = r < s->epsilon;
                for (int i = 0; i < s->n; ++i)
                    s->historyStates[s->numHistory][i] = s->problem->state[i];
                if (r < 2.220446049250313e-16)
                    r = 2.220446049250313e-16;
                s->history[s->numHistory] = r;
                ++s->numHistory;
            }
        } while (s->done == 0 && s->blocking != -1);
    }
    Output(s);
    return 0;
}

} // namespace

// Mac 0x4ad202, Win 0x811970
int KeaSmvLcp(LCPMatrix* matrix, MeReal* x, const MeReal* b, const MeReal* lo, const MeReal* hi, int* state,
              MdtKeaLcpParameters*, MemoryPool*, int)
{
    smv_matrix* A = static_cast<smv_matrix*>(matrix);
    MeReal q[18];
    MeReal w[18];
    int n = A->mSize;
    if (n > 0) {
        int equal = 0;
        for (int i = 0; i < A->mSize; ++i) {
            q[i] = -b[i];
            equal += lo[i] == hi[i];
        }
        n = A->mSize;
        if (equal != 0) {
            // NOTE: a block with a fixed row (lo == hi) is "solved" on its first row only
            if (n < 1)
                return 0;
            const MeReal v = -q[0] / A->mA[0];
            x[0] = v;
            MeReal clamped = lo[0];
            if (lo[0] <= v) {
                clamped = hi[0];
                if (v <= hi[0])
                    clamped = v;
            }
            x[0] = clamped;
            return 1;
        }
    }
    SmvKellerProblem problem;
    ProblemInit(&problem, n, A->mA, q, lo, hi, x, w, state);
    SmvKeller solver;
    SolverInit(&solver, &problem);
    return SmvKellerSolve(&solver);
}
