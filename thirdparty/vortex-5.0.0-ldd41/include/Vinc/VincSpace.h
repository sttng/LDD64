/* Vortex 5.0.0 build 800 "vortexldd41" - VincSpace: the broad phase as seen by the Vx layer (a C
 * API over the CxSpace object, Cx/CxSpace.h) (restored; src/Vinc/VincSpace.cpp,
 * Mac 558d 0x541893-0x541df6, Win 0x72cbb0-0x72d580 + Win-only line-segment queries 0x790650-
 * 0x7908c0).
 *
 * A model is in a space when VincModel::space != 0; VincModel::spaceHandle is its CxSpace handle.
 * Names follow the MathEngine McdSpace API that Vinc continues ("VincSpaceBeginChanges",
 * "VincSpaceEndChanges", "VincSpaceIsChanging", "VincSpacePathUpdate", "VincAxisSortCreate" and
 * "VincSpaceID" are strings of the binaries); the others are inferred. All MEAPI (stdcall on Win)
 * except the cdecl line-segment workers. Functions marked Win-only were dead-stripped on Mac.
 *
 * Use (Vx::VxUniverse):
 *     space = VincSpaceAxisSortCreate(7, maxModels, maxPairs, 0);   (inside a change period)
 *     VincSpaceInsertModel(space, m);  VincSpaceUpdateModel(m) ...
 *     VincSpaceEndChanges(space);
 *     VincSpacePairIteratorBegin(space, &it);
 *     while (more) { VincModelPairContainerReset(c); more = VincSpaceGetPairs(space, &it, c); ... }
 *     VincSpaceBeginChanges(space);  VincSpacePathUpdate(space, dt) ...
 */
#ifndef VINC_VINCSPACE_H
#define VINC_VINCSPACE_H

#include "Vinc/VincIntersect.h"

#ifdef __cplusplus
class CxSpace;
extern "C" {
#else
typedef struct CxSpace CxSpace;
#endif

typedef CxSpace* VincSpaceID;

/* cursors into the hello / staying / goodbye lists (CxLink*) (Mac 0x541c00) */
typedef struct VincSpacePairIterator {
    void* hello;                     /* +0x00 */
    void* staying;                   /* +0x04 */
    void* goodbye;                   /* +0x08 */
} VincSpacePairIterator;

/* Win 0x72d4b0 / 0x72d4d0 */
typedef struct VincSpaceModelIterator {
    int handle;                      /* +0x00 next handle to look at */
    int count;                       /* +0x04 models returned so far */
} VincSpaceModelIterator;

/* filter of the filtered line-segment queries (stdcall on Win): nonzero = test the model */
typedef int (MEAPI *VincModelFilterFn)(VincModelID model, void* data);

/* Mac 0x541b99, Win 0x72cbb0: CxSmallSort::create + bounds fn VincModelUpdate, sphere fn
 * VincModelGetBSphere, then VincSpaceBeginChanges */
VincSpaceID MEAPI VincSpaceAxisSortCreate(int axisMask, int maxModelCount, int maxPairCount, int pairsDisabled);
void MEAPI VincSpaceDestroy(VincSpaceID space);                        /* Mac 0x541893, Win 0x72cc00 */
int MEAPI VincSpaceIsChanging(VincSpaceID space);                      /* Win 0x72cc20 */
int MEAPI VincSpaceInsertModel(VincSpaceID space, VincModelID m);      /* Mac 0x5418a5, Win 0x72cc40 */
int MEAPI VincSpaceRemoveModel(VincModelID m);                         /* Mac 0x541a34, Win 0x72d2f0 */
void MEAPI VincSpaceUpdateModel(VincModelID m);                        /* Mac 0x541923, Win 0x72ccb0 */
void MEAPI VincSpaceUpdateModelWithTime(VincModelID m, MeReal dt, int force); /* Mac 0x54195d, Win 0x72cce0 */
/* state 2 (VincModelSetState): out of the axis lists (1) / back in (0) (name from V3.2) */
void MEAPI VincSpaceModelSetState2(VincModelID m, int isState2);       /* Mac 0x541998, Win 0x72cd10 */
/* LDD64: `count` models of one space out of its axis lists (1), or nothing done (0: the caller
 * updates them as usual), for a jump; then back in at their new bounds (CxSmallSort::takeOut) */
int MEAPI VincSpaceTakeOutModels(VincModelID* models, int count);
void MEAPI VincSpacePutBackModels(VincModelID* models, int count);
/* LDD64: update moves a model whose ends would pass at least `crossings` endpoints by taking it
 * out and sorting it in again (CxSmallSort::relocateCrossings; 0x7fffffff: the walk, the default) */
void MEAPI VincSpaceSetRelocateCrossings(VincSpaceID space, int crossings);
/* LDD64: VincSpaceDisablePair(m, others[k]) for k = 0 .. count - 1 (CxSmallSort::disablePairs) */
void MEAPI VincSpaceDisablePairs(VincModelID m, VincModelID* others, int count);
/* LDD64: pair groups (CxSmallSort::groupNew ...): a new group of `space` (0: none possible); the
 * members of `group` m has a record with (at most `max`, their count returned); m joins `group`
 * (its pairs with the members disabled); m leaves its group, its pairs keeping their states */
int MEAPI VincSpaceGroupNew(VincSpaceID space);
int MEAPI VincSpaceGroupRecordPartners(VincModelID m, int group, VincModelID* out, int max);
void MEAPI VincSpaceGroupJoin(VincModelID m, int group);
void MEAPI VincSpaceGroupLeave(VincModelID m);
int MEAPI VincSpaceFreezeModel(VincModelID m);                         /* Mac 0x5419c1, Win 0x72cd30 */
int MEAPI VincSpaceUnfreezeModel(VincModelID m);                       /* Mac 0x5419e9, Win 0x72cd50 */
int MEAPI VincSpaceModelIsFrozen(VincModelID m);                       /* Mac 0x541a11, Win 0x72cd70 */
int MEAPI VincSpaceModelIsFrozenNoCheck(VincModelID m);                /* Win 0x72cda0 */
void MEAPI VincSpaceUpdateAll(VincSpaceID space);                      /* Win 0x72ce00 (dt 0) */
void MEAPI VincSpacePathUpdate(VincSpaceID space, MeReal dt);          /* Mac 0x541b39, Win 0x72ce20 */
void MEAPI VincSpaceSetAABBFn(VincSpaceID space, void* fn);            /* Win 0x72ce60 */
void MEAPI VincSpaceSetUserData(VincSpaceID space, void* data);        /* Win 0x72ce80 */
void* MEAPI VincSpaceGetUserData(VincSpaceID space);                   /* Win 0x72cea0 */
void MEAPI VincSpacePairIteratorBegin(VincSpaceID space, VincSpacePairIterator* it); /* Mac 0x541c00, Win 0x72cec0 */
/* fill c with goodbyes [goodbyeFirst, goodbyeEnd), hellos [helloFirst, middle), active stayings
 * [middle, helloEnd) and frozen stayings [stayingFirst, size); nonzero while pairs are left */
int MEAPI VincSpaceGetPairs(VincSpaceID space, VincSpacePairIterator* it, VincModelPairContainer* c); /* Mac 0x541c40, Win 0x72d090 */
/* the same, stayings only when a model is in state 1 or not frozen */
int MEAPI VincSpaceGetActivePairs(VincSpaceID space, VincSpacePairIterator* it, VincModelPairContainer* c); /* Mac 0x541df6, Win 0x72cf00 */
/* one goodbye and the hellos (Win 0x72d1d0) */
int MEAPI VincSpaceGetTransitionPairs(VincSpaceID space, VincSpacePairIterator* it, VincModelPairContainer* c);
int MEAPI VincSpaceEnablePair(VincModelID m1, VincModelID m2);         /* Mac 0x541a78, Win 0x72d330 */
int MEAPI VincSpaceDisablePair(VincModelID m1, VincModelID m2);        /* Mac 0x541aaf, Win 0x72d360 */
int MEAPI VincSpaceSetModelPairsEnabled(VincModelID m, int enable);    /* Win 0x72d390 */
int MEAPI VincSpaceGetEnabledPairCount(VincSpaceID space);             /* Win 0x72d3c0 */
int MEAPI VincSpacePairIsEnabled(VincModelID m1, VincModelID m2);      /* Win 0x72d3e0 */
int MEAPI VincSpacePairIsOverlapping(VincModelID m1, VincModelID m2);  /* Win 0x72d410 */
VincModelPair* MEAPI VincSpaceGetModelPair(VincModelID m1, VincModelID m2); /* Win 0x72d440 */
void MEAPI VincSpaceBeginChanges(VincSpaceID space);                   /* Mac 0x541ae6, Win 0x72d470 */
void MEAPI VincSpaceEndChanges(VincSpaceID space);                     /* Mac 0x541af9, Win 0x72d490 */
void MEAPI VincSpaceModelIteratorBegin(VincSpaceID space, VincSpaceModelIterator* it); /* Win 0x72d4b0 */
int MEAPI VincSpaceGetModel(VincSpaceID space, VincSpaceModelIterator* it, VincModelID* m); /* Win 0x72d4d0 */
int MEAPI VincSpaceGetModelCount(VincSpaceID space);                   /* Mac 0x541b0c, Win 0x72d580 */

/* Win-only line-segment queries over the models of a space. results: an array of `max`
 * VincLineSegResults (model = the hit model); first: the nearest hit; returns the count / 1 on a
 * hit (distance < 1e25). The cdecl workers are called by the MEAPI wrappers. */
int MEAPI VincSpaceLineSegIntersectAll(VincSpaceID space, const MeReal* p0, const MeReal* p1,
                                       VincLineSegResult* results, int max);          /* Win 0x72d270 */
int MEAPI VincSpaceLineSegIntersectFirst(VincSpaceID space, const MeReal* p0, const MeReal* p1,
                                         VincLineSegResult* result);                  /* Win 0x72d2a0 */
int MEAPI VincSpaceLineSegIntersectFirstFiltered(VincSpaceID space, const MeReal* p0, const MeReal* p1,
                                                 VincModelFilterFn filter, void* data,
                                                 VincLineSegResult* result);          /* Win 0x72d2c0 */
int VincSpaceLineSegIntersectAllImpl(VincSpaceID space, const MeReal* p0, const MeReal* p1,
                                     VincLineSegResult* results, int max);            /* Win 0x790650 */
int VincSpaceLineSegIntersectFirstImpl(VincSpaceID space, const MeReal* p0, const MeReal* p1,
                                       VincLineSegResult* result);                    /* Win 0x790730 */
int VincSpaceLineSegIntersectFirstFilteredImpl(VincSpaceID space, const MeReal* p0, const MeReal* p1,
                                               VincModelFilterFn filter, void* data,
                                               VincLineSegResult* result);            /* Win 0x7908c0 */
/* one model (aggregates: their elements, filtered) (Win 0x790850) */
int MEAPI VincModelLineSegIntersectFiltered(VincModelID m, const MeReal* p0, const MeReal* p1,
                                            VincModelFilterFn filter, void* data, VincLineSegResult* r);

#ifdef __cplusplus
}
#endif

#endif
