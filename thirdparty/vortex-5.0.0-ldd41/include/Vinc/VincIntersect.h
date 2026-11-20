/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc framework, requests, model pairs, contacts,
 * intersect results and the error table (restored; src/Vinc/VincFramework.cpp,
 * VincIntersect.cpp, VincModelPair.cpp, VincError.cpp).
 *
 * Structs are laid out naturally (Mac i386: doubles 4-aligned; MSVC: 8-aligned); both columns
 * were checked against the binaries. Names follow the VincErrorTable strings (VincModelPair,
 * VincIntersectResult, VincInit, VincTerm, VincSafeTime, VincSpace, VincFramework); the rest
 * is inferred from the MathEngine Mcd API (marked).
 *
 * Usage for pair tests (V3.3-V3.7):
 *   int MyIntersect(VincModelPair* p, VincIntersectResult* r) {
 *       VincModelID m1 = p->model1, m2 = p->model2;          (m1 has the first registered type)
 *       MeReal* tm1 = VincModelGetTransformPtr(m1);
 *       VincRequest* rq = VincModelPairGetRequestPtr(p);     (rq->contactMaxCount etc.)
 *       ... write r->contacts[0 .. r->contactMaxCount), r->contactCount, r->normal ...
 *       return touch; }
 */
#ifndef VINC_VINCINTERSECT_H
#define VINC_VINCINTERSECT_H

#include "Vinc/VincModel.h"
#include "Me/VinPool.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- request (Mac 0x34, Win 0x38 bytes) ---- */
typedef struct VincRequest {         /*  Mac   Win   default (VincRequestInit) */
    int contactMaxCount;             /* +0x00 +0x00  4  (<= 0x400, VxCollisionPairRequest) */
    int faceNormalsFirst;            /* +0x04 +0x04  0  tri meshes: use the face normal */
    MeReal m08;                      /* +0x08 +0x08  0.1 (name inferred) */
    MeReal m10;                      /* +0x10 +0x10  0.1 (name inferred) */
    int contactGroupMaxCount;        /* +0x18 +0x18  4  (<= 6, VincContactSimplify) */
    MeReal m1c;                      /* +0x1c +0x20  0.5 (name inferred) */
    int m24;                         /* +0x24 +0x28  0  (VincContactSimplify reads it) */
    int m28;                         /* +0x28 +0x2c  0 */
    MeReal m2c;                      /* +0x2c +0x30  0.001 (name inferred) */
} VincRequest;

/* ---- contacts (Mac 0x4c, Win 0x50 bytes) ---- */
typedef struct McdContact {
    MeVector3 position;              /* +0x00 */
    MeVector3 normal;                /* +0x18 from model2 to model1 */
    MeReal separation;               /* +0x30 < 0: penetration */
    MeI16 dims;                      /* +0x38 */
    MeI16 m3a;                       /* +0x3a */
    int element1;                    /* +0x3c (e.g. -1, or the "edge" flag of a tri contact) */
    int element2;                    /* +0x40 triangle / cell index of model2 */
    VincModelID model1;              /* +0x44 for contacts of aggregates: the element models */
    VincModelID model2;              /* +0x48 */
} VincContact;

/* one contact group of VincContactSimplify (Mac 0x24, Win 0x28 bytes) */
typedef struct VincContactGroup {
    int count;                       /* +0x00 */
    MeReal m04;                      /* +0x04 / +0x08 */
    MeVector3 normal;                /* +0x0c / +0x10 */
} VincContactGroup;

#define VINC_MAX_CONTACT_GROUPS 6

struct McdModelPair;

/* one triangle produced by a VincTriangleList generator (0x2c bytes on both; Mac 0x4bc96d reads
 * vertices/normal and copies userId into the contacts' element2) */
struct VincTriangleListTri {
    MeReal* vertices[3];             /* +0x00 */
    MeReal* normal;                  /* +0x0c */
    const float* m10[6];             /* +0x10 per-edge float pointers, 0 when absent (the pair tests
                                        read them as pointers; PORT64: declared as pointers, the
                                        original's ints held them on 32-bit) */
    int userId;                      /* +0x28 */
};
/* the 32-bit stride the original steps through the triangles with */
typedef char VincTriangleListTriSizeCheck[(sizeof(void*) != 4 || sizeof(struct VincTriangleListTri) == 0x2c) ? 1 : -1];

/* ---- intersect result (Mac 0x118, Win 0x140 bytes) ---- */
typedef struct McdIntersectResult {  /*  Mac    Win */
    struct McdModelPair* pair;       /* +0x00  +0x00 */
    VincContact* contacts;           /* +0x04  +0x04 output array (caller provided) */
    int contactMaxCount;             /* +0x08  +0x08 */
    int contactCount;                /* +0x0c  +0x0c */
    int contactGroupCount;           /* +0x10  +0x10 */
    VincContactGroup groups[VINC_MAX_CONTACT_GROUPS]; /* +0x14 +0x18 */
    int touch;                       /* +0xec  +0x108 */
    MeVector3 normal;                /* +0xf0  +0x110 average normal */
    int m108;                        /* +0x108 +0x128 */
    MeReal time;                     /* +0x10c +0x130 time of impact (swept intersection) */
    VinPool* contactPool;            /* +0x114 +0x138 scratch contacts of VincAggregateIntersect (V3.8) */
} VincIntersectResult;

/* ---- safe time result ---- */
typedef struct McdSafeTimeResult {
    struct McdModelPair* pair;       /* +0x00 */
    MeReal time;                     /* +0x04 / +0x08 */
} VincSafeTimeResult;

/* ---- distance result (Win-only users; Win +0x08 / +0x10) (names inferred) ---- */
typedef struct McdDistanceResult {
    struct McdModelPair* pair;       /* +0x00 */
    MeReal distance;                 /* +0x04 / +0x08 DBL_MAX: no distance function */
    MeReal m10;                      /* +0x0c / +0x10 DBL_MAX: no distance function */
    /* (V3.8, from the composite distance Win 0x75a9a0, which copies them per model) */
    MeVector3 point1;                /* Win +0x18 on model1 */
    MeVector3 point2;                /* Win +0x30 on model2 */
    int element1;                    /* Win +0x48 (-1) */
    int element2;                    /* Win +0x4c (-1) */
} VincDistanceResult;

/* ---- line segment result (Mac 0x48, Win 0x50 bytes) ---- */
typedef struct McdLineSegResult {
    VincModelID model;               /* +0x00 */
    MeVector3 position;              /* +0x04 / +0x08 */
    MeVector3 normal;                /* +0x1c / +0x20 */
    MeReal distance;                 /* +0x34 / +0x38 */
    struct McdModel* m3c;            /* +0x3c / +0x40 cleared by VincLineSegIntersect; the leaf model
                                        an aggregate hit (an int holding the pointer in both binaries;
                                        PORT64: the pointer itself, the same 4 bytes on 32-bit) */
    int element;                     /* +0x40 / +0x44 hit element (triangle user id), -1 when nothing was hit;
                                        in: a triangle list re-tests it first (V3.6) */
    int firstHit;                    /* +0x44 / +0x48 > 0: stop at the first triangle hit (V3.6, Mac 0x4f52ad) */
} VincLineSegResult;

/* ---- model pair (0x30 bytes on both) ---- */
typedef struct McdModelPair {
    VincModelID model1;              /* +0x00 */
    VincModelID model2;              /* +0x04 */
    VincRequest* request;            /* +0x08 */
    int m0c;                         /* +0x0c */
    void* pairData;                  /* +0x10 per-pair cache (hello/goodbye of the pair test) */
    VincIntersectFn intersectFn;     /* +0x14 */
    VincSweptIntersectFn sweptIntersectFn; /* +0x18 */
    VincSafeTimeFn safeTimeFn;       /* +0x1c */
    VincGoodbyeFn goodbyeFn;         /* +0x20 */
    ptrdiff_t m24;                   /* +0x24 Vx: the pair's Mdt contact list (VxResponseBody_Body;
                                        pointer-sized for 64-bit hosts) */
    unsigned int flags;              /* +0x28 bit 0: VincModelPairSetFlag0 */
    int pairKey;                     /* +0x2c type2 * 13 + type1, or 0xaa (types 9 / 12 involved) */
} VincModelPair;

/* ---- model pair container (0x24 bytes) (names inferred) ---- */
typedef struct VincModelPairContainer {
    VincModelPair** array;           /* +0x00 */
    int size;                        /* +0x04 */
    int goodbyeFirst;                /* +0x08 reset 0 */
    int goodbyeEnd;                  /* +0x0c reset 0  (VincGoodbyeEach: [0, goodbyeEnd)) */
    int helloFirst;                  /* +0x10 reset middle */
    int middle;                      /* +0x14 size / 2 */
    int helloEnd;                    /* +0x18 reset middle */
    int stayingFirst;                /* +0x1c reset size */
    int stayingEnd;                  /* +0x20 reset size */
} VincModelPairContainer;

/* ---- framework (Mac 0x48, Win 0x50 bytes) ---- */
typedef void (MEAPI *VincTerminateFn)(void);
typedef struct VincTerminateAction {
    VincTerminateFn fn;
    struct VincTerminateAction* next;
} VincTerminateAction;

typedef struct VincFramework {
    int initialized;                 /* +0x00 */
    VincTerminateAction* terminateActions; /* +0x04 LIFO */
    VinPool modelPool;               /* +0x08 (0x34 bytes) */
    MeReal defaultPadding;           /* +0x3c / +0x40  (MeReal)0.005f */
    const char* version;             /* +0x44 / +0x48 MeGetVersionString() */
} VincFramework;

typedef void (MEAPI *VincHelloCallback)(VincModelPair* pair, void* data);

/* ---- error table (Mac 0x779560, Win 0xac9f30; 43 entries, id 42 ends the scan) ---- */
typedef struct VincErrorEntry {
    MeI16 id;                        /* +0x00 */
    MeI16 type;                      /* +0x02 0 fatal (abort), 1 warning, 2 info */
    MeI16 count;                     /* +0x04 > 0: report (never decremented: all 5) */
    MeI16 m06;
    const char* message;             /* +0x08 */
} VincErrorEntry;
extern VincErrorEntry VincErrorTable[43];
/* Mac 0x4c16b7, Win 0x7feea0: "Vinc: " + message(id) + extra + "\n" to MeWarning / MeInfo /
 * MeFatalError (+ abort) (name inferred) */
void VincError(VincErrorEntry* table, int id, const char* extra);

/* ---- framework (VincFramework.cpp) ---- */
extern VincFramework* VincTheFramework;      /* Mac 0x779540, Win 0xaf25bc (name inferred) */
void MEAPI VincInit(int geoTypeMaxCount, int modelCount);  /* Mac 0x4bfe9e, Win 0x75cd20 */
void MEAPI VincTerm(void);                                  /* Mac 0x4bfe24 */
void MEAPI VincRegisterTerminateAction(VincTerminateFn fn); /* Mac 0x4bfde8, Win 0x75ce10 */

/* ---- requests and pairs (VincModelPair.cpp) ---- */
void MEAPI VincRequestInit(VincRequest* r);                 /* Mac 0x4c248d, Win 0x754ec0 */
void MEAPI VincSetDefaultRequestPtr(VincRequest* r);        /* Mac 0x4c2512, Win 0x754f70 */
void MEAPI VincRequestCopy(const VincRequest* src, VincRequest* dst); /* Mac 0x4c2526 */
void MEAPI VincModelPairReset(VincModelPair* p, VincModelID m1, VincModelID m2); /* Mac 0x4c2575 */
void MEAPI VincModelPairGetModels(VincModelPair* p, VincModelID* m1, VincModelID* m2); /* Mac 0x4c261d */
VincRequest* MEAPI VincModelPairGetRequestPtr(VincModelPair* p);       /* Mac 0x4c2634 */
void MEAPI VincModelPairSetFlag0(VincModelPair* p, int on);  /* Mac 0x4c263f */
unsigned int MEAPI VincModelPairGetFlag0(VincModelPair* p);  /* Mac 0x4c2657 */
void MEAPI VincModelPairContainerReset(VincModelPairContainer* c);     /* Mac 0x4c2665 */
void MEAPI VincModelPairContainerInit(VincModelPairContainer* c, VincModelPair** array, int size); /* Mac 0x4c268d */
VincModelPairContainer* MEAPI VincModelPairContainerCreate(int size); /* Mac 0x4c26be */
void MEAPI VincModelPairContainerDestroy(VincModelPairContainer* c);  /* Mac 0x4c2715 */

/* ---- hello / goodbye / intersect (VincIntersect.cpp) ---- */
void MEAPI VincGoodbyeEach(VincModelPairContainer* c);                 /* Mac 0x4bfdb8 */
void MEAPI VincGoodbye(VincModelPair* p);                              /* Mac 0x4c035a */
int MEAPI VincSafeTime(VincModelPair* p, MeReal maxTime, VincSafeTimeResult* r); /* Mac 0x4c0371, Win 0x754d30 */
void MEAPI VincSetHelloCallback(VincHelloCallback fn, void* data);     /* Mac 0x4c03ce, Win 0x754d90 */
void MEAPI VincHelloWithRequest(VincModelPair* p, VincRequest* r);     /* Mac 0x4c03eb, Win 0x754260 */
void MEAPI VincHello(VincModelPair* p);                                /* Mac 0x4c0445 */
int MEAPI VincIntersect(VincModelPair* p, VincIntersectResult* r);     /* Mac 0x4c04ba */
int MEAPI VincIntersectWithTime(VincModelPair* p, VincIntersectResult* r, MeReal dt); /* Mac 0x4c10cb */
int MEAPI VincIntersectSafeTime(VincModelPair* p, VincIntersectResult* r, MeReal dt); /* Mac 0x4c11f6 */
/* Win 0x754c50: intersect at the poses after time t (name inferred; dead-stripped on the Mac) */
int MEAPI VincIntersectAtTime(VincModelPair* p, VincIntersectResult* r, MeReal t);
/* Win 0x754dd0: {0, DBL_MAX, 0} (name inferred) */
void MEAPI VincDistanceBoundsInit(MeReal* bounds3);
/* Win 0x754df0 / 0x754e50 (names inferred; dead-stripped on the Mac) */
int MEAPI VincDistanceBounded(VincModelPair* p, const MeReal* bounds3, VincDistanceResult* r);
int MEAPI VincDistance(VincModelPair* p, VincDistanceResult* r);
/* line segment against one model (Mac 0x4c165a, Win 0x775800) */
int MEAPI VincLineSegIntersect(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r);

#ifdef __cplusplus
}
#endif

#endif
