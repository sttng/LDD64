// Vortex 5.0.0 build 800 "vortexldd41" - the aggregate ("Composite", type 11) geometry: a list of
// element models with their transforms relative to the aggregate, its bounds, mass properties, the
// aggregate-vs-anything intersect test and line segment queries (restored;
// Mac 558e 0x54b015-0x54de3f (the last file of TU 558: it has the TU's ios_base::Init), Win
// 0x759cb0-0x75b830). Model-level functions: VincAggregate.cpp. "Composite" is the registered
// type name; the function names are inferred (declared in VincInternal.h).
#include "Vinc/VincIntersect.h"
#include "Vinc/VincSpace.h"
#include "Me/MeMemory.h"
#include "Me/VinPool.h"

#include "VincInternal.h"

#include <float.h>
#include <math.h>
#include "Me/MeCrtMath.h"
#include <iostream> // the TU's static initialiser (Mac 0x54ddda / 0x54de28, ios_base::Init 0x54b170)

/* Aggregate geometry (0x30 bytes on both) (names inferred) */
struct VincAggregate {
    VincGeometry head;               /* +0x00 type */
    int maxCount;                    /* +0x04 capacity of elements (doubled on demand) */
    int count;                       /* +0x08 */
    VincModelID* elements;           /* +0x0c element models (their relTransform: element in the aggregate) */
    MeReal radius;                   /* +0x10 bounding sphere (VincAggregateGeometryUpdate) */
    MeVector3 center;                /* +0x18 */
};

// The pair filter at VincModel::ma4 (Vx::VxIntersectFilter: slot 2 / Win slot 1 accept(userData1,
// userData2)) (name inferred)
// NOTE: outside the anonymous namespace on purpose. The objects behind ma4 are made elsewhere
// (Vx::VxIntersectFilter subclasses); an abstract class with internal linkage and no subclass
// in this TU lets GCC -O2 devirtualize accept() into __cxa_pure_virtual (host crash in
// VincAggregateIntersect).
class VincPairFilter {
public:
    virtual ~VincPairFilter() {}
    virtual bool accept(void* a, void* b) = 0;
};

namespace {

#if defined(__i386__) || defined(_M_IX86)
typedef char VincAggregateSizeCheck[sizeof(VincAggregate) == 0x30 ? 1 : -1];
#endif

MeI16 s_typeId = -1;                     // Mac 0x779d3c, Win 0xabd384
const char* s_typeName = "Composite";    // Mac 0x779d40, Win 0xabd388

const MeReal kMaxReal = 1.7976931348623157e308;
const MeReal kNoHit = 1e25;

inline VincAggregate* agg(VincGeometryID g)
{
    return (VincAggregate*)g;
}

inline bool isAggregate(VincGeometryID g)
{
    return g->type == VincAggregateGetTypeId();
}

// Mac 0x54b015 (result in EAX; inlined on Win)
inline void resetCounts(VincIntersectResult* r)
{
    r->contactGroupCount = 0;
    r->contactCount = 0;
}

// Win 0x75b830 (ECX = ab; inlined on Mac): ab = a * b for transforms (row vectors)
void composeTM(MeReal* ab, const MeReal* a, const MeReal* b)
{
    ab[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8];
    ab[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9];
    ab[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10];
    ab[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8];
    ab[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9];
    ab[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10];
    ab[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8];
    ab[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9];
    ab[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10];
    ab[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + b[12];
    ab[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + b[13];
    ab[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + b[14];
}

} // namespace
extern "C" {
// Mac 0x54c2ab, Win 0x759f90: element transforms = relTransform * tm (recursively)
void MEAPI VincAggregateUpdateElementTransforms(VincGeometryID g, const MeReal* tm)
{
    VincAggregate* a = agg(g);
    for (int i = 0; i < a->count; ++i) {
        VincModelID e = a->elements[i];
        composeTM(e->transform, e->relTransform, tm);
        if (VincModelGetGeometry(e)->type == VincAggregateGetTypeId())
            VincAggregateUpdateElementTransforms(VincModelGetGeometry(e), e->transform);
    }
}
}
namespace {

// Mac 0x54b4a1, Win 0x75a0b0
void aabbRecurse(VincGeometryID g, MeReal* tm, MeReal* mn, MeReal* mx, MeReal* aabb6)
{
    VincAggregate* a = agg(g);
    for (int i = 0; i < a->count; ++i) {
        VincModelID e = a->elements[i];
        if (e->state != 0)
            continue;
        VincGeometryID eg = e->geometry;
        if (eg->type == VincAggregateGetTypeId()) {
            aabbRecurse(eg, e->transform, mn, mx, e->aabb);
        } else {
            MeVector3 emn, emx;
            VincGeometryUpdateAABB(eg, (MeReal(*)[4])e->transform, emn, emx, e->aabb);
            for (int k = 0; k < 3; ++k) {
                if (emn[k] < mn[k])
                    mn[k] = emn[k];
                if (mx[k] < emx[k])
                    mx[k] = emx[k];
            }
        }
    }
    (void)tm;
    aabb6[0] = mn[0];
    aabb6[1] = mx[0];
    aabb6[2] = mn[1];
    aabb6[3] = mx[1];
    aabb6[4] = mn[2];
    aabb6[5] = mx[2];
}

// Mac 0x54b3c5, Win 0x75a1a0
void xyRecurse(VincGeometryID g, MeReal* tm, MeReal* out4)
{
    VincAggregate* a = agg(g);
    for (int i = 0; i < a->count; ++i) {
        VincModelID e = a->elements[i];
        if (e->state != 0)
            continue;
        VincGeometryID eg = e->geometry;
        if (eg->type == VincAggregateGetTypeId()) {
            xyRecurse(eg, e->transform, out4);
        } else {
            MeReal b[4];
            VincGeometryGetXYAABB(eg, (MeReal(*)[4])e->transform, b);
            if (b[0] < out4[0])
                out4[0] = b[0];
            if (out4[1] < b[1])
                out4[1] = b[1];
            if (b[2] < out4[2])
                out4[2] = b[2];
            if (out4[3] < b[3])
                out4[3] = b[3];
        }
    }
    (void)tm;
}

// Mac 0x54b5d6, Win 0x759d10: bounding sphere of the elements' spheres (in the aggregate frame)
void updateBSphere(VincGeometryID g)
{
    VincAggregate* a = agg(g);
    if (a->count == 0) {
        a->radius = 0;
        a->center[2] = 0;
        a->center[1] = 0;
        a->center[0] = 0;
    }
    for (int i = 0; i < a->count; ++i) {
        VincModelID e = a->elements[i];
        VincGeometryID eg = e->geometry;
        if (eg->type == VincAggregateGetTypeId())
            updateBSphere(eg);
        MeVector3 c;
        MeReal r;
        VincGeometryGetBSphere(eg, c, &r);
        const MeReal* t = e->relTransform;
        MeReal px = c[0] * t[0] + c[1] * t[4] + c[2] * t[8] + t[12];
        MeReal py = c[0] * t[1] + c[1] * t[5] + c[2] * t[9] + t[13];
        MeReal pz = c[0] * t[2] + c[1] * t[6] + c[2] * t[10] + t[14];
        if (i == 0) {
            a->radius = r;
            a->center[0] = px;
            a->center[1] = py;
            a->center[2] = pz;
            continue;
        }
        MeReal dx = px - a->center[0];
        MeReal dy = py - a->center[1];
        MeReal dz = pz - a->center[2];
        MeReal s = 0;
        s += dx * dx;
        s += dy * dy;
        s += dz * dz;
        MeReal dist = sqrt(s);
        MeReal R = a->radius;
        MeReal big = r <= R ? R : r;
        MeReal small = r <= R ? r : R;
        if (big - (dist + small) <= 0) {
            if (dist >= 1e-14) {
                MeReal nr = (r + dist + R) * 0.5;
                MeReal f = (nr - R) / dist;
                a->center[0] = dx * f + a->center[0];
                a->center[1] = dy * f + a->center[1];
                a->center[2] = dz * f + a->center[2];
                a->radius = nr;
            } else {
                a->radius = r <= R ? R : r;
            }
        } else if (R < r) {
            a->center[0] = px;
            a->center[1] = py;
            a->center[2] = pz;
            a->radius = r;
        }
    }
}

// Win 0x75a3b0 (inlined on Mac): the AABBs (aabb6 layout) overlap
inline bool aabbOverlap(const MeReal* a, const MeReal* b)
{
    for (int k = 0; k < 6; k += 2) {
        if (a[k + 1] < b[k])
            return false;
        if (b[k + 1] < a[k])
            return false;
    }
    return true;
}

// Mac 0x5492dd (a copy of the 558e mass helper): |r|^2 I - r r^T
void parallelAxis(const MeReal* r, MeReal* out9)
{
    MeReal x = r[0], y = r[1], z = r[2];
    MeReal d = x * x + y * y + z * z;
    MeReal o = d * 0.0;
    out9[0] = d - x * x;
    out9[1] = o - x * y;
    out9[2] = o - x * z;
    out9[3] = out9[1];
    out9[4] = d - y * y;
    out9[5] = o - y * z;
    out9[6] = out9[2];
    out9[7] = out9[5];
    out9[8] = d - z * z;
}

void setIdentityTM(MeReal* tm)
{
    tm[0] = 1;
    tm[1] = 0;
    tm[2] = 0;
    tm[3] = 0;
    tm[4] = 0;
    tm[5] = 1;
    tm[6] = 0;
    tm[7] = 0;
    tm[8] = 0;
    tm[9] = 0;
    tm[10] = 1;
    tm[11] = 0;
    tm[12] = 0;
    tm[13] = 0;
    tm[14] = 0;
    tm[15] = 1;
}

// one element's mass properties added into relTM (centre of mass, weighted), inertia (weighted)
// (the loop body of Mac 0x54c666 / 0x54d24d)
void addElementMass(VincModelID e, const MeReal* rel, MeReal* relTM, MeReal* inertia, MeReal* volume, MeReal* totalMass)
{
    MeReal I[9];
    for (int k = 0; k < 9; ++k)
        I[k] = 0;
    MeReal eTM[16], eI[9], eVol, eMass;
    if (!VincModelIsAggregate(e)) {
        VincGeometryGetMassProperties(e->geometry, (MeReal(*)[4])eTM, (MeReal(*)[3])eI, &eVol);
        eMass = e->density * eVol;
    } else {
        VincAggregateGetMassProperties(e->geometry, (MeReal(*)[4])eTM, (MeReal(*)[3])eI, &eVol, &eMass);
    }
    // T = rel * eTM: the element's mass frame in the aggregate
    MeReal T[16];
    composeTM(T, rel, eTM);
    // I' = T^T eI T (rotation part)
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            I[i * 3 + j] = 0;
            MeReal acc = 0;
            for (int k = 0; k < 3; ++k) {
                MeReal d = T[k * 4 + i];
                acc = d * eI[k * 3 + 0] * T[0 * 4 + j] + acc;
                acc = d * eI[k * 3 + 1] * T[1 * 4 + j] + acc;
                acc = d * eI[k * 3 + 2] * T[2 * 4 + j] + acc;
            }
            I[i * 3 + j] = acc;
        }
    MeReal com[3] = {T[12], T[13], T[14]};
    MeReal pa[9];
    parallelAxis(com, pa);
    for (int k = 0; k < 9; ++k)
        I[k] = (I[k] + pa[k]) * eMass;
    for (int k = 0; k < 9; ++k)
        inertia[k] = inertia[k] + I[k];
    relTM[12] = eMass * T[12] + relTM[12];
    relTM[13] = eMass * T[13] + relTM[13];
    relTM[14] = relTM[14] + eMass * T[14];
    *volume = *volume + eVol;
    *totalMass = *totalMass + eMass;
}

// shift the summed inertia to the centre of mass
void finishMass(MeReal* relTM, MeReal* inertia, MeReal totalMass)
{
    if (totalMass > 0) {
        MeReal inv = 1.0 / totalMass;
        relTM[12] = relTM[12] * inv;
        relTM[13] = relTM[13] * inv;
        relTM[14] = relTM[14] * inv;
        for (int k = 0; k < 9; ++k)
            inertia[k] = inertia[k] * inv;
        MeReal com[3] = {relTM[12], relTM[13], relTM[14]};
        MeReal pa[9];
        parallelAxis(com, pa);
        for (int k = 0; k < 9; ++k)
            inertia[k] = inertia[k] + pa[k] * -1.0;
    }
}

} // namespace

extern "C" {

// Mac 0x54b028, Win 0x759cb0
MeI16 MEAPI VincAggregateGetTypeId(void)
{
    return s_typeId;
}

// Mac 0x54b03a, Win 0x759f00
int MEAPI VincAggregateAddElementGeometry(VincGeometryID g, VincModelID element, MeReal* relTM)
{
    VincAggregate* a = agg(g);
    int n = a->maxCount;
    if (a->count == n) {
        a->maxCount = n * 2;
        // the doubled capacity: n * 8 bytes in both binaries (PORT64: n * 2 pointers)
        a->elements = (VincModelID*)MeMemoryAPI.resize(a->elements, n * 2 * (int)sizeof(VincModelID));
    }
    a->elements[a->count] = element;
    a->elements[a->count]->relTransform = relTM;
    a->count++;
    return a->count;
}

// Mac 0x54b0a4, Win 0x759f50
int MEAPI VincAggregateRemoveElementGeometry(VincGeometryID g, int i)
{
    VincAggregate* a = agg(g);
    for (; i < a->count - 1; ++i)
        a->elements[i] = a->elements[i + 1];
    a->count--;
    return a->count;
}

// Mac 0x54b0dd, Win 0x75a020
VincModelID MEAPI VincAggregateGetElement(VincGeometryID g, int i)
{
    return agg(g)->elements[i];
}

// Mac 0x54b0ee, Win 0x75a040
int MEAPI VincAggregateGetElementCount(VincGeometryID g)
{
    return agg(g)->count;
}

// Mac 0x54b0f9, Win 0x75a050 (name inferred)
int MEAPI VincAggregateGetMaxElementCount(VincGeometryID g)
{
    return agg(g)->maxCount;
}

// Win 0x759ff0 (name inferred)
void MEAPI VincAggregateGetElementAndTransform(VincGeometryID g, int i, VincModelID* m, MeReal** relTM)
{
    *m = (VincModelID)agg(g)->elements[i]->geometry; // NOTE: the element's geometry, not the model
    *relTM = agg(g)->elements[i]->relTransform;
}

// Win 0x75a060 (name inferred): all elements out (their parent cleared); returns the array
VincModelID* MEAPI VincAggregateRemoveAllElements(VincGeometryID g)
{
    VincAggregate* a = agg(g);
    for (int i = 0; i < a->count; ++i)
        a->elements[i]->parent = 0;
    a->count = 0;
    return a->elements;
}

// Mac 0x54b104, Win 0x75a090
void MEAPI VincAggregateGeometryDestroy(VincGeometryID g)
{
    MeMemoryAPI.destroy(agg(g)->elements);
    MeMemoryAPI.destroy(g);
}

// Mac 0x54b13c, Win 0x75a310
void MEAPI VincAggregateGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    center[0] = agg(g)->center[0];
    center[1] = agg(g)->center[1];
    center[2] = agg(g)->center[2];
    *radius = agg(g)->radius;
}

// Mac 0x54b1bc, Win 0x75a840: the nearest hit of the elements accepted by filter (0: all);
// result->model = the element, result->m3c = the leaf model hit
int MEAPI VincAggregateLineSegIntersectFiltered(VincModelID m, const MeReal* p0, const MeReal* p1,
                                                VincModelFilterFn filter, void* data, VincLineSegResult* result)
{
    VincModelGetTransformPtr(m);
    VincAggregate* a = agg(m->geometry);
    result->distance = kNoHit;
    for (int i = 0; i < a->count; ++i) {
        VincModelID e = a->elements[i];
        if (e->state != 0 || (filter != 0 && filter(e, data) == 0))
            continue;
        VincLineSegFn fn = VincGeometryGetLineSegmentFn(e->geometry->type);
        VincLineSegResult r;
        r.m3c = 0;
        r.element = -1;
        if (fn != 0 && fn(e, p0, p1, &r) != 0 && r.distance < result->distance) {
            result->distance = r.distance;
            result->model = e;
            result->normal[0] = r.normal[0];
            result->normal[1] = r.normal[1];
            result->normal[2] = r.normal[2];
            result->position[0] = r.position[0];
            result->position[1] = r.position[1];
            result->position[2] = r.position[2];
            result->element = r.element;
            if (!VincModelIsAggregate(e))
                result->m3c = e;
            else
                result->m3c = r.m3c;
        }
    }
    return result->distance < kNoHit;
}

// Mac 0x54b327, Win 0x75a960 (the registered line segment function)
int MEAPI VincAggregateLineSegIntersect(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* result)
{
    return VincAggregateLineSegIntersectFiltered(m, p0, p1, 0, 0, result);
}

// Mac 0x54b191 (inlined on Win) (name inferred)
void MEAPI VincAggregateRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincAggregateGetTypeId(), VincAggregateLineSegIntersect);
}

// Mac 0x54bcf4, Win 0x75a410: the aggregate (model1) against any model2: each element whose AABB
// overlaps model2's (and the models' pair filters accept) is intersected through a temporary pair;
// the contacts are collected from the result's contact pool and simplified.
int MEAPI VincAggregateIntersect(VincModelPair* pair, VincIntersectResult* r)
{
    VincAggregate* a = agg(pair->model1->geometry);
    VinPool* pool = r->contactPool;
    r->pair = pair;
    resetCounts(r);
    r->touch = 0;
    r->normal[2] = 0;
    r->normal[1] = 0;
    r->normal[0] = 0;
    VincModelID other = pair->model2;
    if (other->state != 0)
        return 0;
    if (a->count == 0) {
        resetCounts(r);
        r->touch = 0;
        return r->touch;
    }
    VincContact* buf = (VincContact*)VinPoolGetStruct(pool);
    int remaining = (int)((unsigned)pool->structSize / sizeof(VincContact));
    int n = 0;
    VincIntersectResult tmp;
    tmp.contactPool = r->contactPool;
    for (int i = 0; i < a->count; ++i) {
        VincModelID e = a->elements[i];
        if (e->state != 0 || !aabbOverlap(e->aabb, other->aabb))
            continue;
        VincPairFilter* f = (VincPairFilter*)(size_t)e->ma4;
        if (f != 0 && !f->accept(e->userData, other->userData))
            continue;
        VincPairFilter* of = (VincPairFilter*)(size_t)other->ma4;
        if (of != 0 && of != f && !of->accept(e->userData, other->userData))
            continue;
        tmp.contacts = buf + n;
        tmp.contactMaxCount = remaining;
        resetCounts(&tmp);
        tmp.touch = 0;
        VincModelPair p;
        VincModelPairReset(&p, other, e);
        MeI16 t = VincAggregateGetTypeId();
        if (pair->request->contactMaxCount == 0 ||
            (char)((p.model2->geometry->type == t) + (p.model1->geometry->type == t)) != 0)
            VincHelloWithRequest(&p, pair->request);
        else
            VincHello(&p);
        VincIntersect(&p, &tmp);
        VincGoodbye(&p);
        if (tmp.touch == 0)
            continue;
        r->touch = 1;
        if (p.request->contactMaxCount == 0) {
            resetCounts(r);
            r->touch = tmp.touch;
            VinPoolPutStruct(pool, buf);
            return r->touch;
        }
        if (other != p.model2) {
            for (int k = n; k < n + tmp.contactCount; ++k) {
                VincContact* c = &buf[k];
                c->normal[0] = -c->normal[0];
                c->normal[1] = -c->normal[1];
                c->normal[2] = -c->normal[2];
                MeReal s = c->separation;
                c->position[0] = s * c->normal[0] + c->position[0];
                c->position[1] = s * c->normal[1] + c->position[1];
                c->position[2] = s * c->normal[2] + c->position[2];
                c->dims = (MeI16)(((c->dims & 0xf) << 4) | ((c->dims & 0xf0) >> 4));
            }
            tmp.normal[0] = -tmp.normal[0];
            tmp.normal[1] = -tmp.normal[1];
            tmp.normal[2] = -tmp.normal[2];
        }
        n += tmp.contactCount;
        r->normal[0] = r->normal[0] + tmp.normal[0];
        r->normal[1] = r->normal[1] + tmp.normal[1];
        r->normal[2] = r->normal[2] + tmp.normal[2];
        remaining -= tmp.contactCount;
        if (remaining < 1) {
            pool->m30 = 1;
            break;
        }
    }
    if (r->touch != 0) {
        MeReal x = r->normal[0], y = r->normal[1], z = r->normal[2];
        MeReal len = sqrt(x * x + y * y + z * z);
        if (len != 0) {
            MeReal inv = 1.0 / len;
            r->normal[0] = x * inv;
            r->normal[1] = y * inv;
            r->normal[2] = inv * z;
        }
        r->contactCount = VincContactSimplify(r->normal, buf, n, r);
        if (pair->model1->parent == 0 && pair->model2->parent == 0)
            for (int k = 0; k < r->contactCount; ++k) {
                VincContact* c = &r->contacts[k];
                if (c->model1->m6c != pair->model1->m6c) {
                    VincModelID t1 = c->model1;
                    c->model1 = c->model2;
                    c->model2 = t1;
                    int t2 = c->element1;
                    c->element1 = c->element2;
                    c->element2 = t2;
                }
            }
    }
    VinPoolPutStruct(pool, buf);
    return r->touch;
}

// Mac 0x54b35f, Win 0x75aba0
int MEAPI VincAggregateRegisterInteractions(void)
{
    VincInteractions i;
    VincInteractionsReset(&i);
    i.intersectFn = VincAggregateIntersect;
    for (int t = 0; t < VincGeometryGetTypeCount(); ++t)
        VincGeometryPairRegisterInteractions(VincAggregateGetTypeId(), t, &i, 0);
    VincAggregateRegisterLineSegment();
    return 1;
}

// Win 0x7acac0 (name inferred). NOTE: a pair table function (next to VincInteractions.cpp's code,
// Win 0x7acac0), defined here through VincPairTableGetCell.
void MEAPI VincGeometryPairRegisterDistanceBoundedFn(int typeA, int typeB, VincDistanceBoundedFn fn)
{
    ((VincPairTableCell*)VincPairTableGetCell(typeA, typeB))->interactions.distanceBoundedFn = fn;
    if (typeA != typeB)
        ((VincPairTableCell*)VincPairTableGetCell(typeB, typeA))->interactions.distanceBoundedFn = fn;
}

// Win 0x75a9a0 (name inferred): the nearest element of the aggregate (pair->model1) to model2;
// result keeps the nearest distance / points (in the pair's model order) and the smallest second
// distance (m10) of the farther elements
int MEAPI VincAggregateDistanceBounded(VincModelPair* pair, const MeReal* bounds3, VincDistanceResult* result)
{
    VincModelPair copy = *pair;
    VincAggregate* a = agg(copy.model1->geometry);
    int i;
    for (i = 0; i < a->count; ++i) {
        VincModelID e = a->elements[i];
        if (e->state != 0)
            continue;
        VincModelPair p;
        VincModelPairReset(&p, e, copy.model2);
        VincHello(&p);
        VincDistanceResult d;
        d.distance = DBL_MAX;
        d.m10 = DBL_MAX;
        result->element2 = -1; // NOTE: cleared before every element (lost unless set again below)
        d.element1 = -1;
        int found = VincDistanceBounded(&p, bounds3, &d);
        VincGoodbye(&p);
        if (found == 0)
            continue;
        if (d.distance < result->distance) {
            if (p.model1 == e) {
                result->pair->model1 = d.pair->model1;
                result->pair->model2 = d.pair->model2;
                for (int k = 0; k < 3; ++k) {
                    result->point1[k] = d.point1[k];
                    result->point2[k] = d.point2[k];
                }
                result->element1 = d.element1;
                result->element2 = d.element2;
            } else {
                result->pair->model1 = d.pair->model2;
                result->pair->model2 = d.pair->model1;
                for (int k = 0; k < 3; ++k) {
                    result->point1[k] = d.point2[k];
                    result->point2[k] = d.point1[k];
                }
                result->element1 = d.element2;
                result->element2 = d.element1;
            }
            result->distance = d.distance;
            result->m10 = d.m10;
        } else if (d.m10 < result->m10) {
            result->m10 = d.m10;
        }
    }
    return i; // (the loop counter is what the original leaves in EAX; VincDistanceBounded ignores it)
}

// Win 0x75ab50 (name inferred)
int MEAPI VincAggregateRegisterDistance(void)
{
    for (int t = 0; t < VincGeometryGetTypeCount(); ++t)
        VincGeometryPairRegisterDistanceBoundedFn(VincAggregateGetTypeId(), t, VincAggregateDistanceBounded);
    return 1;
}

// Mac 0x54b8a8, Win 0x75ab90
void MEAPI VincAggregateGeometryUpdate(VincGeometryID g)
{
    updateBSphere(g);
}

// Mac 0x54b8bb, Win 0x759cc0
VincGeometryID MEAPI VincAggregateGeometryCreate(int maxElements)
{
    VincAggregate* a = (VincAggregate*)MeMemoryAPI.create(sizeof(VincAggregate));
    a->maxCount = maxElements;
    a->count = 0;
    a->elements = (VincModelID*)MeMemoryAPI.create(maxElements * sizeof(VincModelID));
    a->radius = 0;
    a->center[2] = 0;
    a->center[1] = 0;
    a->center[0] = 0;
    VincGeometryInit(&a->head, VincAggregateGetTypeId());
    return &a->head;
}

// Mac 0x54b9a2, Win 0x75ac10
void MEAPI VincAggregateMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    VincAggregate* a = agg(g);
    if (a->count == 0) {
        out[0] = kMaxReal;
        out[1] = kMaxReal;
        out[2] = kMaxReal;
        return;
    }
    out[0] = -kMaxReal;
    out[1] = -kMaxReal;
    out[2] = -kMaxReal;
    for (int i = 0; i < a->count; ++i) {
        VincModelID e = a->elements[i];
        if (e->state != 0)
            continue;
        MeReal t[16];
        composeTM(t, e->relTransform, &tm[0][0]);
        MeVector3 p;
        VincGeometryMaximumPoint(e->geometry, (MeReal(*)[4])t, dir, p);
        if (out[0] < p[0])
            out[0] = p[0];
        if (out[1] < p[1])
            out[1] = p[1];
        if (out[2] < p[2])
            out[2] = p[2];
    }
}

// Mac 0x54c530, Win 0x75a2d0. NOTE: starts from {-DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX}, so the
// result is always the whole plane
void MEAPI VincAggregateGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal* out4)
{
    out4[0] = -kMaxReal;
    out4[1] = kMaxReal;
    out4[2] = -kMaxReal;
    out4[3] = kMaxReal;
    VincAggregateUpdateElementTransforms(g, &tm[0][0]);
    xyRecurse(g, &tm[0][0], out4);
}

// Mac 0x54c59b, Win 0x75a260
void MEAPI VincAggregateUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 minCorner, MeVector3 maxCorner, MeReal* aabb6)
{
    minCorner[0] = kMaxReal;
    minCorner[1] = kMaxReal;
    minCorner[2] = kMaxReal;
    maxCorner[0] = -kMaxReal;
    maxCorner[1] = -kMaxReal;
    maxCorner[2] = -kMaxReal;
    VincAggregateUpdateElementTransforms(g, &tm[0][0]);
    aabbRecurse(g, &tm[0][0], minCorner, maxCorner, aabb6);
    if (maxCorner[0] < minCorner[0]) {
        maxCorner[0] = kMaxReal;
        maxCorner[1] = kMaxReal;
        maxCorner[2] = kMaxReal;
    }
}

// Mac 0x54c666, Win 0x75ad10: relTM = centre of mass frame, inertia about it (mass weighted),
// volume, mass; an empty aggregate is a solid sphere of the bounding radius (mass = volume)
MeI16 MEAPI VincAggregateGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume,
                                           MeReal* mass)
{
    VincAggregate* a = agg(g);
    MeReal* tm = &relTM[0][0];
    MeReal* I = &inertia[0][0];
    MeReal total;
    if (a->count == 0) {
        MeReal r2 = a->radius * a->radius;
        for (int k = 0; k < 9; ++k)
            I[k] = 0;
        MeReal d = r2 * 0.4;
        I[8] = d;
        I[4] = d;
        I[0] = d;
        setIdentityTM(tm);
        total = a->radius * 4.1887902047863905 * r2;
        *volume = total;
    } else {
        for (int k = 0; k < 9; ++k)
            I[k] = 0;
        setIdentityTM(tm);
        *volume = 0;
        total = 0;
        for (int i = 0; i < a->count; ++i) {
            VincModelID e = a->elements[i];
            addElementMass(e, e->relTransform, tm, I, volume, &total);
        }
        finishMass(tm, I, total);
    }
    *mass = total;
    return 1;
}

// Mac 0x54d21d, Win 0x75b770 (the registered mass properties function) (name inferred)
int MEAPI VincAggregateGetMassPropertiesFn(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    MeReal mass;
    return VincAggregateGetMassProperties(g, relTM, inertia, volume, &mass);
}

// Mac 0x54d24d, Win 0x75b240: the same for a list of models (relTransform 0: identity), the inertia
// symmetrised at the end (name inferred)
MeI16 MEAPI VincModelsGetMassProperties(VincModelID* models, int count, MeMatrix4 relTM, MeMatrix3 inertia,
                                        MeReal* volume, MeReal* mass)
{
    MeReal* tm = &relTM[0][0];
    MeReal* I = &inertia[0][0];
    MeReal total = 0;
    if (count != 0) {
        MeReal identity[16];
        setIdentityTM(identity);
        for (int k = 0; k < 9; ++k)
            I[k] = 0;
        setIdentityTM(tm);
        *volume = 0;
        for (int i = 0; i < count; ++i) {
            VincModelID e = models[i];
            const MeReal* rel = e->relTransform != 0 ? e->relTransform : identity;
            addElementMass(e, rel, tm, I, volume, &total);
        }
        finishMass(tm, I, total);
    }
    // NOTE: count 0 leaves relTM / inertia / volume alone but still symmetrises the inertia
    MeReal s = (I[3] + I[1]) * 0.5;
    I[1] = s;
    I[3] = s;
    s = (I[6] + I[2]) * 0.5;
    I[2] = s;
    I[6] = s;
    s = (I[5] + I[7]) * 0.5;
    I[7] = s;
    I[5] = s;
    *mass = total;
    return 1;
}

// Mac 0x54b936, Win 0x75b7a0
void MEAPI VincAggregateRegisterType(void)
{
    s_typeId = (MeI16)VincGeometryRegisterType(0xb, s_typeName, VincAggregateGeometryDestroy, VincAggregateUpdateAABB,
                                               VincAggregateGetXYAABB, VincAggregateGetBSphere,
                                               VincAggregateMaximumPoint, VincAggregateGetMassPropertiesFn);
}

} // extern "C"
