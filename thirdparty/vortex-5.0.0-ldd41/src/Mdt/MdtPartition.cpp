// Vortex 5.0.0 build 800 "vortexldd41" - partitioning of the enabled bodies into independent
// islands, Kea rows per partition, forces back after the step (restored).
// Mac 0x49035b-0x4920b7 (static initialiser 0x492075 / 0x4920a0; profile section
// "MdtPartition"), Win 0x734030-0x735a10 (/LTCG, the static helpers are inlined differently).
// MdtPartitionWorld: a breadth-first walk from each unvisited dynamic root (a body that is its own
// `self`) collects one partition. With optimize set, constraints reaching too deep (maxDepth) or
// into a full partition are deferred into extra "level" partitions (big islands solved in pieces).
#include "Mdt/MdtWorld.h"
#include "Mdt/MdtBody.h"
#include "Mdt/MdtConstraint.h"
#include "Kea/MdtKea.h"
#include "Me/MeMath.h"
#include "Me/MeProfile.h"
#include "Me/MeIntMap.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

// Mac data 0x779504 (0): the isRoot argument of the first visit of every root (name inferred).
static int MdtPartitionRootFlag = 0;

// Breadth-first work list (a stack-allocated struct on Mac, Win: 4 locals).
struct MdtPartitionStack {
    MdtBody** items;
    int begin;
    int end;
    int capacity;
};

// Mac 0x49035b, Win 0x734030
void MEAPI MdtPartitionOutputDestroy(MdtPartitionOutput* out, MeMemoryAPIStruct* api)
{
    api->destroy(out->constraintsStart);
    api->destroy(out->constraintsSize);
    api->destroy(out->numRows);
    api->destroy(out->maxSortKey);
    api->destroy(out->order);
    api->destroy(out->parent);
    api->destroy(out->numDeferredOf);
    api->destroy(out->bodies);
    api->destroy(out->keaBodies);
    api->destroy(out->bodyMap);
    api->destroy(out->bodiesStart);
    api->destroy(out->bodiesSize);
    api->destroy(out->awake);
    api->destroy(out->touchedBodies);
    api->destroy(out->touchedConstraints);
    if (out->constraints != 0)
        api->destroy(out->constraints);
    if (out->deferred != 0)
        api->destroy(out->deferred);
    api->destroy(out->finished);
    api->destroy(out->stack);
    api->destroy(out->roots);
}

// Mac 0x49042b, Win 0x734100
void MEAPI MdtPartitionOutputInit(MdtPartitionOutput* out)
{
    out->maxPartitions = 0;
    out->maxBodies = 0;
    out->maxConstraints = 0;
    out->constraintsStart = 0;
    out->constraintsSize = 0;
    out->numRows = 0;
    out->order = 0;
    out->parent = 0;
    out->numDeferredOf = 0;
    out->bodies = 0;
    out->keaBodies = 0;
    out->bodyMap = 0;
    out->bodiesStart = 0;
    out->bodiesSize = 0;
    out->awake = 0;
    out->maxSortKey = 0;
    out->constraints = 0;
    out->deferred = 0;
    out->finished = 0;
    out->touchedBodies = 0;
    out->touchedConstraints = 0;
    out->roots = 0;
    out->numRoots = -1;
    out->stack = 0;
}

// Mac 0x4904da, Win 0x734160: grow the arrays (never shrinks); api 0 = the stored one
void MEAPI MdtPartitionOutputResize(MdtPartitionOutput* out, int maxPartitions, int maxBodies,
                                    int maxConstraints, MeMemoryAPIStruct* api)
{
    if (api == 0)
        api = out->api;
    else
        out->api = api;
    if (out->maxPartitions < maxPartitions) {
        out->maxPartitions = maxPartitions;
        const size_t n = maxPartitions * sizeof(int);
        out->constraintsStart = (int*)api->resize(out->constraintsStart, n);
        out->finished = (int*)api->resize(out->finished, n);
        out->constraintsSize = (int*)api->resize(out->constraintsSize, n);
        out->numDeferredOf = (int*)api->resize(out->numDeferredOf, n);
        out->bodiesStart = (int*)api->resize(out->bodiesStart, n);
        out->bodiesSize = (int*)api->resize(out->bodiesSize, n);
        out->awake = (int*)api->resize(out->awake, n);
        out->maxSortKey = (int*)api->resize(out->maxSortKey, n);
        out->numRows = (int*)api->resize(out->numRows, n);
        out->order = (int*)api->resize(out->order, n);
        out->parent = (int*)api->resize(out->parent, n);
    }
    if (out->maxConstraints < maxConstraints) {
        out->maxConstraints = maxConstraints;
        const size_t n = maxConstraints * sizeof(MdtConstraint*);
        out->deferred = (MdtConstraint**)api->resize(out->deferred, n);
        out->constraints = (MdtConstraint**)api->resize(out->constraints, n);
        out->touchedConstraints = (MdtConstraint**)api->resize(out->touchedConstraints, n);
    }
    if (out->maxBodies < maxBodies) {
        out->maxBodies = maxBodies;
        out->bodies = (MdtBody**)api->resize(out->bodies, maxBodies * sizeof(MdtBody*));
        out->keaBodies = (MdtKeaBody**)api->resize(out->keaBodies, maxBodies * sizeof(MdtKeaBody*));
        out->bodyMap = (MeIntMapEntry*)api->resize(out->bodyMap, maxBodies * sizeof(MeIntMapEntry));
        out->touchedBodies = (MdtBody**)api->resize(out->touchedBodies, maxBodies * sizeof(MdtBody*));
        out->roots = (MdtBody**)api->resize(out->roots, maxBodies * sizeof(MdtBody*));
        out->stack = (MdtBody**)api->resize(out->stack, maxBodies * sizeof(MdtBody*));
        out->numRoots = -1;
    }
}

// Mac 0x49067e, Win 0x734910
void MEAPI MdtPartitionParamsSetOptimize(MdtPartitionParams* p, int on)
{
    p->optimize = on;
}

// Mac 0x49068b, Win 0x734920
int MEAPI MdtPartitionParamsGetOptimize(const MdtPartitionParams* p)
{
    return p->optimize;
}

// Mac 0x490695, Win 0x735a10
void MEAPI MdtPartitionParamsReset(MdtPartitionParams* p)
{
    p->optimize = 0;
    p->autoDisable = 1;
    p->partitioner = MdtPartitionWorld;
    p->partitionClosed = MdtAutoDisableLastPartition;
    p->defaultMaxDepth = 5;
    p->maxConstraintsPerLevel = 10;
}

// Mac 0x4906d0, Win 0x734930
void MEAPI MdtPartition(MdtBody* enabledBodies, MdtPartitionOutput* out, MdtPartitionParams* p)
{
    p->partitioner(enabledBodies, out, p);
}

// Mac 0x4906ef, Win 0x734960
void MEAPI MdtPartitionParamsSetAutoDisable(MdtPartitionParams* p, int on)
{
    p->autoDisable = on;
    p->partitionClosed = (on != 0) ? MdtAutoDisableLastPartition : 0;
}

// Mac 0x490717 (static, regparm EAX = c, EDX = out), Win 0x7349c0
static void MdtPartitionAddConstraint(MdtConstraint* c, MdtPartitionOutput* out)
{
    c->partitionInfo.state = 1;
    c->partitionInfo.partitionA = out->numPartitions;
    out->constraints[out->numConstraints++] = c;
    out->touchedConstraints[out->numTouchedConstraints++] = c;
    out->constraintsSize[out->numPartitions]++;
    c->bcl.partitionCount++;
}

// Mac 0x49075b, Win 0x734bc0 (qsort)
static int MdtPartitionCompareBodyDepth(const void* a, const void* b)
{
    return (*(MdtBody* const*)a)->partitionInfo.depth - (*(MdtBody* const*)b)->partitionInfo.depth;
}

// Mac 0x490776, Win 0x734be0 (qsort)
static int MdtPartitionCompareConstraintKeys(const void* a, const void* b)
{
    const MdtConstraint* ca = *(MdtConstraint* const*)a;
    const MdtConstraint* cb = *(MdtConstraint* const*)b;
    int d = ca->partitionInfo.sortKey - cb->partitionInfo.sortKey;
    if (d == 0)
        d = ca->partitionInfo.minKey - cb->partitionInfo.minKey;
    return d;
}

// Mac 0x49079f (static, regparm): root of a partition in the level forest
static int MdtPartitionFindRoot(const MdtPartitionOutput* out, int i)
{
    int r;
    do {
        r = i;
        i = out->parent[r];
    } while (i < r);
    return r;
}

// Mac 0x4907b4 (static, regparm): point the chain starting at i to root
static void MdtPartitionSetRoot(MdtPartitionOutput* out, int i, int root)
{
    if (i == 0x7fffffff)
        return;
    int* p = &out->parent[i];
    int next = *p;
    while (next != root) {
        *p = root;
        if (next == 0x7fffffff)
            break;
        p = &out->parent[next];
        next = *p;
    }
}

// Mac 0x4907ee, Win 0x734c60
static void MdtPartitionUpdateMaxSortKey(MdtPartitionOutput* out, int partition)
{
    const int start = out->constraintsStart[partition];
    const int end = start + out->constraintsSize[partition];
    for (int k = start; k < end; ++k) {
        MdtConstraint* c = out->constraints[k];
        const int key = c->partitionInfo.minKey;
        for (int i = 0; i < c->bcl.numBodies; ++i) {
            MdtBody* b = c->body[i];
            if (b != 0 && out->maxSortKey[b->keaBody.partitionIndex] < key)
                out->maxSortKey[b->keaBody.partitionIndex] = key;
        }
        if (out->maxSortKey[partition] < key)
            out->maxSortKey[partition] = key;
    }
}

// Mac 0x490884, Win 0x734d00 (qsort)
static int MdtPartitionCompareBodyGrouping(const void* a, const void* b)
{
    return (*(MdtBody* const*)a)->keaBody.grouping - (*(MdtBody* const*)b)->keaBody.grouping;
}

// Mac 0x49089f (static, regparm EAX = out), Win 0x734d20
static void MdtPartitionNew(MdtPartitionOutput* out)
{
    if (out->numPartitions == out->maxPartitions)
        MdtPartitionOutputResize(out, out->maxPartitions * 2, 0, 0, 0);
    const int p = out->numPartitions;
    out->bodiesSize[p] = 0;
    out->awake[p] = 0;
    out->maxSortKey[p] = 0;
    out->constraintsSize[p] = 0;
    out->numDeferredOf[p] = 0;
    out->bodiesStart[p] = out->numBodies;
    out->constraintsStart[p] = out->numConstraints;
    out->parent[p] = 0x7fffffff;
}

// Mac 0x490942 (static, regparm EAX = out, EDX = params, ECX = finished), Win 0x734da0
static void MdtPartitionClose(MdtPartitionOutput* out, MdtPartitionParams* params, int finished)
{
    const int p = out->numPartitions;
    const int deferred = out->numDeferredOf[p];
    out->finished[p] = finished;
    out->numPartitions = p + 1;
    if (params->partitionClosed != 0 && finished == 0 && deferred == 0)
        params->partitionClosed(out);
}

// Mac 0x490993, Win 0x734de0
void MEAPI MdtConstraintResetPartitionInfo(MdtConstraintPartitionInfo* p)
{
    p->state = 0;
    p->partitionB = -1;
    p->partitionA = -1;
    p->sortKey = 0x7fffffff;
    p->minKey = 0;
}

// Mac 0x4909bd, Win 0x734e10
void MEAPI MdtBodyResetPartitionInfo(MdtBodyPartitionInfo* p)
{
    p->state = 0;
    p->m8 = -1;
    p->lastPartition = 0;
}

// Mac 0x4909d9, Win 0x734e30
void MEAPI MdtBodyResetPartitionInfoAll(MdtBody* b)
{
    MdtBodyResetPartitionInfo(&b->partitionInfo);
    for (MdtConstraintNode* n = b->constraints; n != 0; n = n->next)
        MdtConstraintResetPartitionInfo(&n->constraint->partitionInfo);
}

// Mac 0x490a17 (static, regparm), Win 0x734f20
static int MdtPartitionStackPush(MdtPartitionStack* s, MdtBody* b)
{
    if (s->end < s->capacity) {
        s->items[s->end++] = b;
        return 0;
    }
    return -1;
}

// Mac 0x490a3d (static, regparm)
static int MdtPartitionStackSize(const MdtPartitionStack* s)
{
    return s->end - s->begin;
}

// Mac 0x490a4a, Win 0x7358a0
static void MdtPartitionReset(MdtPartitionOutput* out, MdtBody* bodies, MdtPartitionParams* params)
{
    out->numTouchedBodies = 0;
    out->numTouchedConstraints = 0;
    out->numPartitions = 0;
    out->numBodies = 0;
    out->numDeferred = 0;
    out->numConstraints = 0;
    if (bodies == 0) {
        out->numRoots = 0;
        return;
    }
    int n = 0;
    MdtBody* b = bodies;
    do {
        if (b->self == b)
            out->roots[n++] = b;
        MdtBodyResetPartitionInfoAll(b);
        b->keaBody.partitionIndex = -1;
        b->partition = -1;
        b = b->nextEnabled;
    } while (b != 0);
    out->numRoots = n;
    if (n > 1 && params->optimize == 1)
        qsort(out->roots, n, sizeof(MdtBody*), MdtPartitionCompareBodyGrouping);
}

// Mac 0x490b17 (static, regparm EAX = out, EDX = b, ECX = depth), Win 0x734e90
static void MdtPartitionAddBody(MdtPartitionOutput* out, MdtBody* b, int depth)
{
    if (!MdtBodyIsEnabled(b))
        MdtBodyEnable(b);
    b->partitionInfo.depth = depth;
    b->partitionInfo.state = 1;
    out->keaBodies[out->numBodies] = &b->keaBody;
    out->bodies[out->numBodies] = b;
    out->numBodies++;
    out->touchedBodies[out->numTouchedBodies++] = b;
    out->bodiesSize[out->numPartitions]++;
    b->partition = out->numPartitions;
    b->keaBody.partitionIndex = out->numPartitions;
}

static MeReal MdtPartitionMagnitudeSqr(const MeReal* v)
{
    MeReal s = v[0] * v[0] + 0.0;
    s = v[1] * v[1] + s;
    s = v[2] * v[2] + s;
    return s;
}

// Mac 0x490b9e (static, regparm EAX = b, EDX = out, ECX = params), Win 0x735140 + 0x734a60:
// add b and its constraints to the current partition, queue the reachable bodies
static void MdtPartitionVisitBody(MdtBody* b, MdtPartitionOutput* out, MdtPartitionParams* params,
                                  MdtPartitionStack* st, int depth, int maxDepth, int grouping,
                                  int isRoot, int maxBodies)
{
    const bool visited = b->partitionInfo.state != 0;
    if (!visited)
        MdtPartitionAddBody(out, b, depth);
    if (isRoot == 1) {
        if (!visited)
            MdtPartitionStackPush(st, b);
        return;
    }
    MdtConstraintNode* n = b->constraints;
    if (n != 0) {
        int handled = 0;
        int total = 0;
        do {
            MdtConstraint* c = n->constraint;
            ++total;
            if (c->ignored != 0) {
                ++handled;
            } else if (c->partitionInfo.state == 2) {
                const int p = out->numPartitions;
                if (c->partitionInfo.partitionA != p && p != c->partitionInfo.partitionB) {
                    if (c->partitionInfo.partitionB == -1)
                        c->partitionInfo.partitionB = p;
                    out->numDeferredOf[out->numPartitions]++;
                }
                ++handled;
            } else if (c->partitionInfo.state == 1) {
                ++handled;
            } else {
                int depthInc = 0;
                int connect = 1;
                bool any = false;
                bool skip = false;
                if (params->optimize != 0) {
                    depthInc = 0;
                    connect = 0;
                    if (c->bcl.numBodies > 0) {
                        int others = 1;
                        for (int i = 0; i < c->bcl.numBodies; ++i) {
                            MdtBody* o = c->body[i];
                            if (o == 0)
                                continue;
                            if (c->bcl.keaBody[i] == 0 || o == b || o->frozen != 0) {
                                int* awake = &out->awake[out->numPartitions];
                                if (o->frozen == 2 && *awake == 0 &&
                                    (MdtPartitionMagnitudeSqr(o->keaBody.vel) > o->autoDisable.velocityThreshold ||
                                     MdtPartitionMagnitudeSqr(o->keaBody.velrot) > o->autoDisable.angularVelocityThreshold))
                                    *awake = 1;
                                continue;
                            }
                            ++others;
                            if (maxBodies > 0 && !(out->bodiesSize[out->numPartitions] < maxBodies)) {
                                any = true;
                                continue;
                            }
                            if (o->keaBody.grouping == grouping) {
                                ++connect;
                                depth = 0;
                                any = true;
                                depthInc = 0;
                                break;
                            }
                            if (depth < maxDepth) {
                                ++connect;
                                any = true;
                                depthInc = 1;
                                continue;
                            }
                            if (o->partitionInfo.state != 1) {
                                any = true;
                                continue;
                            }
                            ++connect;
                            any = true;
                        }
                        if (others == 1) {
                            ++connect;
                            any = true;
                        }
                    } else {
                        ++connect;
                        any = true;
                    }
                    if (MdtConstraintGetFlags(c) != 0)
                        connect = 0;
                    else if (!any)
                        skip = true;
                }
                if (!skip) {
                    ++handled;
                    if (connect < 1) {
                        // defer to a level partition
                        c->partitionInfo.state = 2;
                        out->deferred[out->numDeferred++] = c;
                        out->numDeferredOf[out->numPartitions]++;
                        if (c->partitionInfo.partitionA == -1)
                            c->partitionInfo.partitionA = out->numPartitions;
                        else if (c->partitionInfo.partitionB == -1)
                            c->partitionInfo.partitionB = out->numPartitions;
                        c->partitionInfo.sortKey = b->keaBody.partitionIndex;
                    } else {
                        MdtPartitionAddConstraint(c, out);
                        for (int i = 0; i < c->bcl.numBodies; ++i) {
                            MdtBody* o = c->body[i];
                            if (o != 0 && c->bcl.keaBody[i] != 0 && o != b && o->frozen == 0 &&
                                o->partitionInfo.state == 0) {
                                MdtPartitionAddBody(out, o, depth + depthInc);
                                MdtPartitionStackPush(st, o);
                            }
                        }
                    }
                }
            }
            n = n->next;
        } while (n != 0);
        if (handled != total) {
            if (!visited)
                MdtPartitionStackPush(st, b);
            return;
        }
    }
    b->partitionInfo.state = 2;
    if (visited) {
        // take b off the work list.
        // NOTE: unless b is the first or last entry, both binaries overwrite the FIRST entry
        // with the last one (and drop the last) instead of removing b; kept.
        if (b == st->items[st->begin]) {
            st->begin++;
        } else if (b == st->items[st->end - 1]) {
            st->end--;
        } else if (st->begin < st->end) {
            st->items[st->begin] = st->items[st->end - 1];
            st->end--;
        }
    }
}

// Mac 0x491051, Win 0x735920 (MdtPartitionParams::partitioner)
void MEAPI MdtPartitionWorld(MdtBody* enabledBodies, MdtPartitionOutput* out, MdtPartitionParams* params)
{
    MeProfileStartSectionFn("MdtPartition", 1);
    MdtPartitionReset(out, enabledBodies, params);
    const int numRoots = out->numRoots;
    if (numRoots == 0)
        return; // NOTE: the profile section stays open (both binaries)
    MdtBody** roots = out->roots;
    MdtBody* b;
    if (roots != 0 && (b = roots[0]) != 0) {
        int r = 0;
        for (;;) {
            MdtPartitionNew(out);
            const int grouping = b->keaBody.grouping;
            int firstDeferred = out->numDeferred;
            const MdtGrouping* g = b->world->groupings[grouping];
            const int maxBodies = g->maxConstraints;
            int maxDepth = g->maxDepth;
            MdtPartitionStack st;
            st.items = out->stack;
            st.begin = 0;
            st.end = 0;
            st.capacity = out->maxBodies;
            if (maxDepth < 0)
                maxDepth = params->defaultMaxDepth;
            MdtPartitionVisitBody(b, out, params, &st, 0, maxDepth, grouping, MdtPartitionRootFlag,
                                  maxBodies);
            int size;
            while ((size = MdtPartitionStackSize(&st)) > 0) {
                if (size > 1 && params->optimize == 1)
                    qsort(&st.items[st.begin], size, sizeof(MdtBody*), MdtPartitionCompareBodyDepth);
                MdtBody* next = st.items[st.begin];
                MdtPartitionVisitBody(next, out, params, &st, next->partitionInfo.depth, maxDepth,
                                      grouping, 0, maxBodies);
            }
            // deferred constraints that only touch this partition join it after all
            for (int k = firstDeferred; k < out->numDeferred; ++k) {
                MdtConstraint* c = out->deferred[k];
                if (MdtConstraintGetFlags(c) != 0)
                    continue;
                bool keep = false;
                for (int i = 0; i < c->bcl.numBodies; ++i) {
                    MdtBody* o = c->body[i];
                    if (o != 0 && c->bcl.keaBody[i] != 0 && o->frozen == 0 &&
                        o->keaBody.partitionIndex != out->numPartitions) {
                        keep = true;
                        break;
                    }
                }
                if (keep)
                    continue;
                out->numDeferred--;
                out->numDeferredOf[out->numPartitions]--;
                out->deferred[k] = out->deferred[out->numDeferred];
                --k;
                MdtPartitionAddConstraint(c, out);
            }
            MdtPartitionClose(out, params, 0);
            // next unvisited dynamic enabled root
            bool found = false;
            while (++r < numRoots && (b = roots[r]) != 0) {
                if (b->partitionInfo.state == 0 && b->frozen == 0 && b->enabledTime != -1) {
                    found = true;
                    break;
                }
            }
            if (!found)
                break;
        }
    }
    if (out->numDeferred != 0) {
        int n = out->numDeferred;
        if (n > 0) {
            for (int k = 0; k < out->numDeferred; ++k) {
                MdtConstraint* c = out->deferred[k];
                c->partitionInfo.minKey = 0x7fffffff;
                for (int i = 0; i < c->bcl.numBodies; ++i) {
                    MdtBody* o = c->body[i];
                    if (o != 0) {
                        const int it = o->world->groupings[o->keaBody.grouping]->keaParams.maxIterations;
                        if (it < c->partitionInfo.minKey)
                            c->partitionInfo.minKey = it;
                    }
                }
            }
            n = out->numDeferred;
            if (n > 0) {
                for (int k = 0; k < out->numDeferred; ++k) {
                    MdtConstraint* c = out->deferred[k];
                    int root = MdtPartitionFindRoot(out, c->partitionInfo.sortKey);
                    for (int i = 0; i < c->bcl.numBodies; ++i) {
                        if (c->body[i] != 0) {
                            const int r2 = MdtPartitionFindRoot(out, c->body[i]->keaBody.partitionIndex);
                            if (r2 < root)
                                root = r2;
                        }
                    }
                    for (int i = 0; i < c->bcl.numBodies; ++i) {
                        if (c->body[i] != 0)
                            MdtPartitionSetRoot(out, c->body[i]->keaBody.partitionIndex, root);
                    }
                    c->partitionInfo.sortKey = root;
                }
                for (int k = 0; k < out->numDeferred; ++k) {
                    MdtConstraint* c = out->deferred[k];
                    const int root = MdtPartitionFindRoot(out, c->partitionInfo.sortKey);
                    for (int i = 0; i < c->bcl.numBodies; ++i) {
                        if (c->body[i] != 0)
                            MdtPartitionSetRoot(out, c->body[i]->keaBody.partitionIndex, root);
                    }
                    c->partitionInfo.sortKey = root;
                }
                n = out->numDeferred;
            }
        }
        qsort(out->deferred, n, sizeof(MdtConstraint*), MdtPartitionCompareConstraintKeys);
        int k = 0;
        while (k < out->numDeferred) {
            const int key = out->deferred[k]->partitionInfo.sortKey;
            const int minKey = out->deferred[k]->partitionInfo.minKey;
            MdtPartitionNew(out);
            while (k < out->numDeferred) {
                MdtConstraint* c = out->deferred[k];
                if (c->partitionInfo.sortKey != key || c->partitionInfo.minKey != minKey ||
                    !(out->constraintsSize[out->numPartitions] < params->maxConstraintsPerLevel))
                    break;
                for (int i = 0; i < c->bcl.numBodies; ++i) {
                    MdtBody* o = c->body[i];
                    if (c->bcl.keaBody[i] != 0 && o->frozen == 0 &&
                        o->partitionInfo.lastPartition != out->numPartitions) {
                        o->partitionInfo.lastPartition = out->numPartitions;
                        if (out->maxBodies <= out->numBodies)
                            MdtPartitionOutputResize(out, 0, out->maxBodies * 2, 0, 0);
                        out->keaBodies[out->numBodies] = &o->keaBody;
                        out->bodies[out->numBodies] = o;
                        out->numBodies++;
                        out->bodiesSize[out->numPartitions]++;
                    }
                }
                out->constraints[out->numConstraints++] = c;
                out->touchedConstraints[out->numTouchedConstraints++] = c;
                out->constraintsSize[out->numPartitions]++;
                c->bcl.partitionCount++;
                ++k;
            }
            out->parent[out->numPartitions] = key;
            MdtPartitionUpdateMaxSortKey(out, out->numPartitions);
            MdtPartitionClose(out, params, 1);
        }
    }
    for (int k = 0; k < out->numTouchedConstraints; ++k) {
        out->touchedConstraints[k]->visited = 0;
        MdtConstraintResetPartitionInfo(&out->touchedConstraints[k]->partitionInfo);
    }
    for (int k = 0; k < out->numTouchedBodies; ++k) {
        out->touchedBodies[k]->m514 = 0;
        MdtBodyResetPartitionInfo(&out->touchedBodies[k]->partitionInfo);
    }
    MeProfileEndSectionFn("MdtPartition");
}

// Mac 0x49178e, Win 0x734730 (+ 0x7345e0): impulses -> forces, gravity, Kea rows of the
// partition's constraints (a row builder returning 0 drops the constraint for this step)
int MEAPI MdtKeaAddPartition(MdtPartitionOutput* out, int partition, MeReal h,
                             const MeReal* stepAndGravity, const MdtBclSolverParameters* params,
                             MdtKeaConstraints* rows)
{
    MdtBody** bodies = out->bodies + out->bodiesStart[partition];
    int numConstraints = out->constraintsSize[partition];
    MdtConstraint** constraints = out->constraints + out->constraintsStart[partition];
    const int numBodies = out->bodiesSize[partition];
    MeIntMapEntry* map = out->bodyMap + out->bodiesStart[partition];
    bool fillMap = true;
    if (out->finished[partition] == 0) {
        if (numBodies < 1) {
            fillMap = false;
        } else {
            const MeReal invH = 1.0 / h;
            for (int k = 0; k != numBodies; ++k) {
                MdtBody* b = bodies[k];
                if (b->constraints == 0)
                    b->keaBody.flags &= ~kMdtKeaBodyFlagHasConstraints;
                else
                    b->keaBody.flags |= kMdtKeaBodyFlagHasConstraints;
                const MeReal m = b->mass;
                b->keaBody.force[0] = m * stepAndGravity[1] + b->keaBody.force[0];
                b->keaBody.force[1] = m * stepAndGravity[2] + b->keaBody.force[1];
                b->keaBody.force[2] = b->keaBody.force[2] + m * stepAndGravity[3];
                b->enabledTime++;
                if (b->impulseAdded != 0) {
                    b->keaBody.force[0] = invH * b->impulse[0] + b->keaBody.force[0];
                    b->keaBody.force[1] = invH * b->impulse[1] + b->keaBody.force[1];
                    b->keaBody.force[2] = invH * b->impulse[2] + b->keaBody.force[2];
                    b->keaBody.torque[0] = invH * b->angularImpulse[0] + b->keaBody.torque[0];
                    b->keaBody.torque[1] = invH * b->angularImpulse[1] + b->keaBody.torque[1];
                    b->keaBody.torque[2] = invH * b->angularImpulse[2] + b->keaBody.torque[2];
                    // NOTE: cleared by multiplying with 0 (keeps NaN / infinities)
                    b->impulse[0] = b->impulse[0] * 0.0;
                    b->impulse[1] = b->impulse[1] * 0.0;
                    b->impulse[2] = b->impulse[2] * 0.0;
                    b->angularImpulse[0] = b->angularImpulse[0] * 0.0;
                    b->angularImpulse[1] = b->angularImpulse[1] * 0.0;
                    b->angularImpulse[2] = b->angularImpulse[2] * 0.0;
                    b->impulseAdded = 0;
                }
            }
        }
    } else if (numBodies < 1) {
        fillMap = false;
    }
    if (fillMap) {
        for (int k = 0; k < numBodies; ++k) {
            // NOTE: the key is the Kea body's address as an int (32-bit in both binaries;
            // PORT64: the whole address on 64-bit, Me/MeIntMap.h)
            map[k].key = (MeIntMapKey)(size_t)&bodies[k]->keaBody;
            map[k].value = k;
        }
    }
    MeIntMap bodyMap;
    bodyMap.count = numBodies;
    bodyMap.entries = map;
    MeIntMapSort(&bodyMap);
    MdtKeaConstraintsSetBodyMap(rows, &bodyMap);
    if (numConstraints < 1) {
        MdtKeaConstraintsAllocate(rows, 0, numConstraints, numBodies);
    } else {
        unsigned int numRows = 0;
        for (int k = 0; k != numConstraints; ++k)
            numRows += constraints[k]->bcl.maxRows;
        if ((numRows & 3) != 0)
            numRows = (numRows + 4) - (int)numRows % 4;
        MdtKeaConstraintsAllocate(rows, numRows, numConstraints, numBodies);
        MdtKeaConstraintsBeginAdd(rows);
        int k = 0;
        do {
            MdtConstraint* c = constraints[k];
            c->bcl.partitionCount--;
            if (c->rowBuilder(rows, &c->bcl, params) == 0) {
                constraints[k] = constraints[numConstraints - 1];
                --k;
                --numConstraints;
            }
            ++k;
        } while (k < numConstraints);
        out->constraintsSize[partition] = numConstraints;
        MdtKeaConstraintsEndAdd(rows);
    }
    return MdtKeaConstraintsGetNumRows(rows);
}

// Mac 0x491b42, Win 0x7343a0: after MdtKeaStep - body transforms, constraint forces, and the
// force limits of proportional frictions (coefficient * |force perpendicular to the axis|)
void MEAPI MdtPartitionGetForces(MdtKeaConstraints* rows, int partition, MdtPartitionOutput* out)
{
    MdtBody** bodies = out->bodies + out->bodiesStart[partition];
    MdtConstraint** constraints = out->constraints + out->constraintsStart[partition];
    for (int k = 0; k < out->bodiesSize[partition]; ++k)
        MdtBodyUpdateTransformFromCOM(bodies[k]);
    for (int k = 0; k < out->constraintsSize[partition]; ++k) {
        MdtConstraint* c = constraints[k];
        MdtKeaGetConstraintForces(rows, k, c->bcl.keaBody, c->bcl.numBodies, c->force, c->torque,
                                  c->bcl.lambda, c->bcl.lambda2, c->bcl.rowStatus);
        c->flags &= ~1;
        if (c->bcl.frictionDirty == 0)
            continue;
        MeMatrix4 frames[6];
        for (int i = 0; i < c->bcl.numBodies; ++i) {
            if (c->bcl.comTM[i] == 0)
                memcpy(frames[i], c->bcl.frame[i], sizeof(MeMatrix4));
            else
                MeMatrix4TMCompoundRotation(frames[i], c->bcl.frame[i], *c->bcl.comTM[i]);
        }
        const MeReal* F = c->force[c->bcl.keaBody[0] == 0 ? 1 : 0];
        char dirty = 0;
        for (int pass = 0; pass != 2; ++pass) {
            for (int j = 0; j != 3; ++j) {
                Vx::VxConstraintFriction& f = c->bcl.friction[pass * 3 + j];
                if (f.mProportional && f.mEnabled) {
                    const MeReal* a = frames[f.mAxis][j];
                    const MeReal d = -(a[0] * F[0] + a[1] * F[1] + a[2] * F[2]);
                    const MeReal vx = F[0] + a[0] * d;
                    const MeReal vy = F[1] + a[1] * d;
                    const MeReal vz = F[2] + a[2] * d;
                    f.mMaxForce = f.mCoefficient * sqrt(vz * vz + (vy * vy + (vx * vx + 0.0)));
                    dirty = 1;
                }
            }
        }
        c->bcl.frictionDirty = dirty;
    }
}
