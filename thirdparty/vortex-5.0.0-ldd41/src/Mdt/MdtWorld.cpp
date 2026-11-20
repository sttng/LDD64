// Vortex 5.0.0 build 800 "vortexldd41" - MdtWorld (restored).
// Mac 0x4931ac-0x494648 (static initialiser 0x494604 / 0x49462f), Win 0x7323b0-0x733e00
// (/LTCG). __FILE__: Mac "Mdt/src/MdtWorld.cpp", Win
// "c:\work\martinc\vortexldd41\vortex\mdt\src\mdtworld.cpp".
#include "Mdt/MdtWorld.h"
#include "Mdt/MdtBody.h"
#include "Mdt/MdtConstraint.h"
#include "Kea/MdtKea.h"
#include "Me/MeMessage.h"
#include "Me/MeVersion.h"
#include "Me/VinPool.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define MDT_WORLD_FILE "c:\\work\\martinc\\vortexldd41\\vortex\\mdt\\src\\mdtworld.cpp"
#else
#define MDT_WORLD_FILE "Mdt/src/MdtWorld.cpp"
#endif

// Mac 0x4931ac, Win 0x7324c0
MeReal MEAPI MdtWorldGetStepSize(const MdtWorld* w)
{
    return w->stepSize;
}

// Mac 0x4931ba, Win 0x732530
void MEAPI MdtWorldSetGravity(MdtWorld* w, MeReal x, MeReal y, MeReal z)
{
    w->gravity[0] = x;
    w->gravity[1] = y;
    w->gravity[2] = z;
}

// Mac 0x4931e9, Win 0x732560: smallest MdtBody::maxTimeStep of the partition's bodies
MeReal MEAPI MdtWorldGetPartitionMaxTimeStep(MdtPartitionOutput* out, int partition)
{
    const int start = out->bodiesStart[partition];
    const int end = start + out->bodiesSize[partition];
    MeReal m = DBL_MAX;
    for (int k = start; k < end; ++k) {
        const MeReal h = out->bodies[k]->maxTimeStep;
        if (m <= h)
            continue;
        m = h;
    }
    return m;
}

// Mac 0x49325b (static, regparm EAX = out, EDX = partition): 1 = level partition, 2 = has
// deferred constraints, 0 = plain island
static int MdtWorldPartitionClass(const MdtPartitionOutput* out, int partition)
{
    if (out->finished[partition] == 1)
        return 1;
    return (out->numDeferredOf[partition] != 0) ? 2 : 0;
}

// Mac 0x493285, Win 0x7328d0 (qsort over MdtWorkUnit): by level root, class, then most rows first
static int MdtWorldCompareWorkUnits(const void* pa, const void* pb)
{
    const MdtWorkUnit* a = (const MdtWorkUnit*)pa;
    const MdtWorkUnit* b = (const MdtWorkUnit*)pb;
    const MdtPartitionOutput* out = &b->world->partitions;
    const int ra = out->parent[a->partition];
    const int rb = out->parent[b->partition];
    if (ra != rb)
        return ra - rb;
    const int ca = MdtWorldPartitionClass(out, a->partition);
    const int cb = MdtWorldPartitionClass(out, b->partition);
    if (ca == cb)
        return out->numRows[b->partition] - out->numRows[a->partition];
    return cb - ca;
}

// Mac 0x493308, Win 0x732970: level partitions first, then most rows first
static int MdtWorldCompareWorkUnitsSimple(const void* pa, const void* pb)
{
    const MdtWorkUnit* a = (const MdtWorkUnit*)pa;
    const MdtWorkUnit* b = (const MdtWorkUnit*)pb;
    const MdtPartitionOutput* out = &a->world->partitions;
    const int fa = out->finished[a->partition] != 1;
    const int fb = out->finished[b->partition] != 1;
    if (fa == fb)
        return out->numRows[b->partition] - out->numRows[a->partition];
    return fb - fa;
}

// Mac 0x493372, Win 0x732b60
void MEAPI MdtWorldAddConstraint(MdtWorld* w, MdtConstraint* c)
{
    if (c != 0) {
        w->numConstraints++;
        w->sizesChanged++;
        c->world = w;
    }
}

typedef int (*MdtWorkUnitFn)(MdtWorkUnit* u);

// Mac 0x49338c (static, regparm EAX = w, EDX = fn), Win 0x733550
static void MdtWorldForEachWorkUnit(MdtWorld* w, MdtWorkUnitFn fn, int from, int to)
{
    for (int i = from; i < to; ++i)
        fn(&w->workUnits[i]);
}

// Mac 0x4933cf
int MEAPI MdtWorldGetMaxConstraintsPerLevel(const MdtWorld* w)
{
    return w->partitionParams.maxConstraintsPerLevel;
}

// Mac 0x4933dd
int MEAPI MdtWorldGetKeaM2c(const MdtWorld* w)
{
    return MdtKeaFramesGetM2c(w->frames);
}

// Mac 0x4933f6, Win 0x733630
void MEAPI MdtWorldSetAutoDisable(MdtWorld* w, int on)
{
    MdtPartitionParamsSetAutoDisable(&w->partitionParams, on);
}

// Mac 0x493415, Win 0x7332a0
void MEAPI MdtWorldSetPartitionOptimize(MdtWorld* w, int on)
{
    MdtPartitionParamsSetOptimize(&w->partitionParams, on);
}

// Mac 0x493434, Win 0x732cf0
void MEAPI MdtWorldDestroy(MdtWorld* w)
{
    MeMemoryAPIStruct* api = &MeMemoryAPI;
    MdtPartitionOutputDestroy(&w->partitions, api);
    MdtKeaFramesDestroy(w->frames);
    VinPoolDestroy(&w->bodyPool);
    VinPoolDestroy(&w->constraintNodePool);
    VinPoolDestroy(&w->constraintPool);
    VinPoolDestroy(&w->hookPool);
    VinPoolDestroy(&w->groupingPool);
    if (w->groupings != 0)
        api->destroy(w->groupings);
    if (w->keaParams.owned != 0)
        MdtKeaParametersDestroy(&w->keaParams);
    if (w->workUnits != 0)
        MeMemoryAPI.destroy(w->workUnits);
    MeMemoryAPI.destroy(w);
}

// Mac 0x4934fd, Win 0x732cc0
void MEAPI MdtWorldSetMaxSizes(MdtWorld* w, int maxBodies, int maxConstraints)
{
    w->maxBodies = maxBodies;
    w->maxConstraints = maxConstraints;
    MdtPartitionOutputResize(&w->partitions, 10, maxBodies, maxConstraints, &MeMemoryAPI);
}

// Mac 0x49354e, Win 0x733930: grow the partition buffers after bodies / constraints were added
void MEAPI MdtWorldUpdateSizes(MdtWorld* w)
{
    if (w->sizesChanged != 0) {
        const int numBodies = w->numBodies;
        const int maxBodies = w->maxBodies;
        if (maxBodies < numBodies)
            w->maxBodies = numBodies;
        char changed = maxBodies < numBodies;
        if (w->maxConstraints < w->numConstraints) {
            w->maxConstraints = w->numConstraints;
            ++changed;
        }
        if (changed != 0)
            MdtWorldSetMaxSizes(w, w->maxBodies, w->maxConstraints);
        w->sizesChanged = 0;
    }
}

// Mac 0x4935ac, Win 0x732b90
void MEAPI MdtWorldRemoveConstraint(MdtConstraint* c)
{
    if (c == 0) // NOTE: guard added; both binaries then write c->world through the null pointer
        return;
    if (c->bcl.type != -1) {
        c->world->numConstraints--;
        if (MdtConstraintIsEnabled(c))
            MdtConstraintDisable(c);
    }
    c->world = 0;
}

// Mac 0x4935e8, Win 0x732af0
void MEAPI MdtWorldRemoveBody(MdtBody* b)
{
    if (b->next != 0)
        b->next->prevNext = b->prevNext;
    *b->prevNext = b->next;
    b->next = 0;
    b->prevNext = 0;
    if (MdtBodyIsEnabled(b))
        MdtBodyDisable(b);
    b->world->numBodies--;
    b->world = 0;
}

// Mac 0x493657, Win 0x732a10 (strings)
void MEAPI MdtWorldInsertBody(MdtWorld* w, MdtBody* b)
{
    if (b->world == w) {
        MeInfo(0, "(%s line %d): \n", MeFileNameFromPath(MDT_WORLD_FILE), 0x3f3);
        MeWarning(0, "MdtWorldInsertBody: Body already in world.\n");
    } else if (b->world == 0) {
        b->world = w;
        b->next = w->bodies;
        b->prevNext = &w->bodies;
        if (w->bodies != 0)
            w->bodies->prevNext = &b->next;
        w->bodies = b;
        w->numBodies++;
        w->sizesChanged++;
        if (b->enabledTime != -1 && MdtBodyIsDynamic(b)) {
            b->enabledTime = -1;
            MdtBodyEnable(b);
        }
    } else {
        MeInfo(0, "(%s line %d): \n", MeFileNameFromPath(MDT_WORLD_FILE), 0x3f8);
        MeWarning(0, "MdtWorldInsertBody: Body already in a different world.\n");
    }
}

static int MdtWorldGetForcesWork(MdtWorkUnit* u);
static int MdtWorldAddPartitionWork(MdtWorkUnit* u);

// Mac 0x49376b, Win 0x733990: partition, then per batch of work slots: rows, MdtKeaStep, forces
void MEAPI MdtWorldStep(MdtWorld* w, MeReal h)
{
    if (w == 0) // NOTE: guard added; both binaries write w->stepSize through the null pointer
        return;
    if (w->enabledBodies != 0) {
        w->stepSize = h;
        MdtWorldUpdateSizes(w);
        for (MdtConstraint* c = w->enabledConstraints; c != 0; c = c->next)
            c->bcl.partitionCount = 0;
        MdtPartition(w->enabledBodies, &w->partitions, &w->partitionParams);
        for (MdtWorldHook* k = w->hooks; k != 0; k = k->next)
            k->fn(k);
        MdtPartitionOutput* out = &w->partitions;
        int n = out->numPartitions;
        for (int p = 0; p < n; n = out->numPartitions, ++p) {
            const int start = out->constraintsStart[p];
            out->numRows[p] = 0;
            for (int k = 0; k < out->constraintsSize[p]; ++k)
                out->numRows[p] += out->constraints[start + k]->bcl.maxRows;
        }
        int perUnit = w->maxPartitionsPerUnit;
        if (n <= w->maxPartitionsPerUnit)
            perUnit = n;
        if (w->workUnitCapacity < n) {
            int cap = w->workUnitCapacity * 2;
            if (cap <= n)
                cap = n;
            w->workUnitCapacity = cap;
            w->workUnits = (MdtWorkUnit*)MeMemoryAPI.resize(w->workUnits, cap * sizeof(MdtWorkUnit));
        }
        int count = out->numPartitions;
        for (int i = 0; i < count; count = out->numPartitions, ++i) {
            w->workUnits[i].world = w;
            w->workUnits[i].partition = i;
            w->workUnits[i].slot = i;
            out->order[i] = i;
        }
        bool grouped = false;
        if (count >= 2) {
            qsort(w->workUnits, count, sizeof(MdtWorkUnit), MdtWorldCompareWorkUnitsSimple);
            bool byRoot = true;
            if (MdtKeaFramesGetNumThreads(w->frames) >= 2) {
                byRoot = false;
                if (w->maxPartitionsPerUnit < out->numPartitions) {
                    byRoot = true;
                    if ((double)out->numPartitions < 100.0) {
                        const double rows0 = (double)out->numRows[0];
                        if (rows0 < 128.0 || (double)out->numRows[1] * 128.0 < rows0)
                            byRoot = false;
                    }
                }
            }
            if (byRoot)
                qsort(w->workUnits, out->numPartitions, sizeof(MdtWorkUnit), MdtWorldCompareWorkUnits);
            else
                grouped = true;
            for (int i = 0; i < out->numPartitions; ++i) {
                w->workUnits[i].slot = i;
                out->order[i] = w->workUnits[i].partition;
            }
        }
        int total = out->numPartitions;
        if (total > 0) {
            int from = 0;
            int to;
            do {
                to = total;
                if (!grouped) {
                    const int root = out->parent[w->workUnits[from].partition];
                    if (root == 0x7fffffff) {
                        if (from + perUnit <= total)
                            to = from + perUnit;
                    } else {
                        to = from;
                        do {
                            ++to;
                        } while (to < total && out->parent[w->workUnits[to].partition] == root);
                    }
                }
                MdtKeaFramesSetupSlots(w->frames, from, to, out->constraintsSize, out->bodiesSize,
                                       out->numRows, out->keaBodies, out->bodiesStart, out->order);
                if (from > 0 && from < to) {
                    for (int i = from; i != to; ++i)
                        w->workUnits[i].slot -= from;
                }
                MdtWorldForEachWorkUnit(w, MdtWorldAddPartitionWork, from, to);
                MdtKeaStep(w->frames);
                MdtWorldForEachWorkUnit(w, MdtWorldGetForcesWork, from, to);
                total = out->numPartitions;
                from = to;
            } while (to < total);
        }
        for (MdtBody* b = w->enabledBodies; b != 0; b = b->nextEnabled) {
            MdtBodySaveAndClearForces(b);
            if (w->useBodyMaxTimeStep != 0)
                b->maxTimeStep = DBL_MAX;
        }
    }
    w->stepSize = h;
}

// Mac 0x493c2b, Win inlined in 0x733990 (-> 0x7343a0)
static int MdtWorldGetForcesWork(MdtWorkUnit* u)
{
    MdtWorld* w = u->world;
    const int partition = u->partition;
    MdtKeaConstraints* rows = MdtKeaFramesGetConstraints(w->frames, u->slot);
    MdtPartitionGetForces(rows, partition, &w->partitions);
    return 0;
}

// Mac 0x493c77, Win 0x733300: the Kea rows and parameters of one work slot
static int MdtWorldAddPartitionWork(MdtWorkUnit* u)
{
    MdtKeaParameters previous;
    MdtKeaParametersInit(&previous);
    const int partition = u->partition;
    const int slot = u->slot;
    MdtWorld* w = u->world;
    MdtKeaFrames* frames = w->frames;
    MdtPartitionOutput* out = &w->partitions;
    int iterations = out->maxSortKey[partition];
    if (w->keaParams.maxIterations < iterations)
        iterations = w->keaParams.maxIterations;
    MdtKeaFramesSetSlotMaxIterations(frames, slot, iterations);
    MdtKeaConstraints* rows = MdtKeaFramesGetConstraints(frames, slot);
    MeReal h;
    int grouping;
    const MdtKeaBody* first = out->keaBodies[out->bodiesStart[partition]];
    if (out->finished[partition] == 0) {
        MdtKeaConstraintsSetGrouped(rows, 0);
        h = w->stepSize;
        grouping = first->grouping;
    } else {
        MdtKeaConstraintsSetGrouped(rows, 1);
        h = w->stepSize;
        grouping = w->groupings[first->grouping]->id;
    }
    if (w->useBodyMaxTimeStep != 0) {
        MeReal m = h;
        if (MdtWorldGetPartitionMaxTimeStep(out, partition) <= h)
            m = MdtWorldGetPartitionMaxTimeStep(out, partition);
        if (m != 0.0)
            h = m;
    }
    int numRows;
    MeChunk* chunk;
    MdtKeaParameters* kp;
    void* m0c;   // the slot's solver set (an int in both binaries; PORT64: a pointer)
    const MdtKeaParameters* source;
    if (grouping == -1) {
        MdtBclSolverParametersUpdate(&w->bclParams, -1, 0, 0, 0, h);
        numRows = MdtKeaAddPartition(out, partition, h, &w->stepSize, &w->bclParams, rows);
        chunk = MdtKeaFramesGetSlotChunk(frames, slot);
        kp = MdtKeaFramesGetSlotParameters(frames, slot);
        m0c = MdtKeaFramesGetSlotM0c(frames, slot);
        memcpy(&previous, kp, sizeof(MdtKeaParameters));
        source = &w->keaParams;
    } else {
        if (w->partitionParams.optimize == 0) {
            MdtBclSolverParametersUpdate(&w->bclParams, grouping, 0, 0, 0, h);
            numRows = MdtKeaAddPartition(out, partition, h, &w->stepSize, &w->bclParams, rows);
        } else {
            MdtBclSolverParameters* gp = &w->groupings[grouping]->bclParams;
            MdtBclSolverParametersUpdate(gp, grouping, 0, 0, 0, h);
            numRows = MdtKeaAddPartition(out, partition, h, &w->stepSize, gp, rows);
        }
        chunk = MdtKeaFramesGetSlotChunk(frames, slot);
        kp = MdtKeaFramesGetSlotParameters(frames, slot);
        m0c = MdtKeaFramesGetSlotM0c(frames, slot);
        memcpy(&previous, kp, sizeof(MdtKeaParameters));
        source = &w->groupings[grouping]->keaParams;
    }
    memcpy(kp, source, sizeof(MdtKeaParameters));
    kp->owned = m0c;
    kp->workspaceSize = previous.workspaceSize;
    kp->m64 = previous.m64;
    kp->stepSize = h;
    kp->m64 = 0;
    int size = numRows * 0x80;
    if (size < previous.workspaceSize + previous.m64)
        size = previous.workspaceSize + previous.m64;
    kp->workspace = MeChunkGetMem(chunk, size);
    kp->workspaceSize = size;
    return 0;
}

// Mac 0x49421e, Win 0x732680 (MdtPartitionParams::partitionClosed): a closed island whose bodies
// are all at rest is disabled and dropped from the step
void MEAPI MdtAutoDisableLastPartition(MdtPartitionOutput* out)
{
    const int p = out->numPartitions - 1;
    if (out->awake[p] != 0)
        return;
    for (int k = out->bodiesStart[p]; k < out->bodiesStart[p] + out->bodiesSize[p]; ++k) {
        if (MdtBodyIsMoving(out->bodies[k]))
            return;
    }
    for (int k = out->bodiesStart[p]; k < out->bodiesStart[p] + out->bodiesSize[p]; ++k) {
        MdtBody* b = out->bodies[k];
        if (b->partition == p) {
            MdtBodyDisable(b);
            MdtBodySaveAndClearForces(b);
            MdtBodyResetVelocities(b);
        }
    }
    out->numPartitions--;
    out->numBodies -= out->bodiesSize[p];
    out->numConstraints -= out->constraintsSize[p];
}

// Mac 0x494311, Win 0x732370: 1/60 is passed as a float (0x3f91111120000000)
void MEAPI MdtBclSolverParametersReset(MdtBclSolverParameters* p)
{
    MdtBclSolverParametersSet(p, 1e-10, 1e-10, (double)(1.0f / 60.0f), 5.0);
}

// Mac 0x494363, Win 0x7323b0
void MEAPI MdtWorldReset(MdtWorld* w)
{
    w->useBodyMaxTimeStep = 0;
    w->m1a8 = 0;
    w->gravity[2] = 0;
    w->gravity[1] = 0;
    w->gravity[0] = 0;
    MdtPartitionParamsReset(&w->partitionParams);
    MdtKeaParametersReset(&w->keaParams);
    MdtBclSolverParametersReset(&w->bclParams);
}

// Mac 0x4943d2, Win 0x7337b0
MdtWorld* MEAPI MdtWorldCreate(int maxBodies, int maxConstraints)
{
    MeMemoryAPIStruct* api = &MeMemoryAPI;
    MdtWorld* w = (MdtWorld*)MeMemoryAPI.createZeroed(sizeof(MdtWorld));
    if (w != 0) {
        w->frames = MdtKeaFramesCreate(api);
        w->workUnits = 0;
        w->workUnitCapacity = 0;
        int t = MdtKeaGetThreadingMode();
        if (t == 2)
            t = 0;
        w->threadingMode = t;
        w->numBodies = 0;
        w->numEnabledBodies = 0;
        w->bodies = 0;
        w->enabledBodies = 0;
        w->maxBodies = maxBodies;
        VinPoolInitSimple(&w->bodyPool, maxBodies, sizeof(MdtBody), 0x10);
        w->maxPartitionsPerUnit = 0x7fffffff;
        w->maxConstraints = maxConstraints;
        w->numEnabledConstraints = 0;
        w->numConstraints = 0;
        w->enabledConstraints = 0;
        VinPoolInitSimple(&w->constraintPool, w->maxConstraints, MdtConstraintMaxSize, 0);
        VinPoolSetName(&w->constraintPool, "world constraintPool");
        VinPoolInitSimple(&w->constraintNodePool, w->maxConstraints * 6, sizeof(MdtConstraintNode), 0);
        VinPoolSetName(&w->constraintNodePool, "world cNodePool");
        VinPoolInitSimple(&w->hookPool, 10, sizeof(MdtWorldHook), 0);
        w->m00c = 0;
        w->hooks = 0;
        MdtPartitionOutputInit(&w->partitions);
        MdtWorldSetMaxSizes(w, maxBodies, maxConstraints);
        w->bodyDisabledCallback = 0;
        w->bodyEnabledCallback = 0;
        w->stepSize = 1.0 / 60.0;
        MdtWorldReset(w);
        w->useBodyMaxTimeStep = 0;
        w->version = MeGetVersionString();
        w->groupings = 0;
        w->numGroupings = 0;
        w->groupingCapacity = 0;
        VinPoolInitSimple(&w->groupingPool, 10, sizeof(MdtGrouping), 0);
        VinPoolSetName(&w->groupingPool, "Groupings Pool");
    }
    return w;
}
