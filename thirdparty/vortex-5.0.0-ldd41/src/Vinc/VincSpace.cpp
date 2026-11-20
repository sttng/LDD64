// Vortex 5.0.0 build 800 "vortexldd41" - VincSpace: the C API over the CxSpace broad phase
// (restored; Mac 558d 0x541893-0x541df6, Win 0x72cbb0-0x72d580, Win-only line
// segment queries 0x790650-0x7908c0). See include/Vinc/VincSpace.h.
#include "Vinc/VincSpace.h"
#include "Cx/CxSmallSort.h"
#include "Me/MeProfile.h"

#include "VincInternal.h"

namespace {

int s_updateCount; // Win 0xaf1770 (counts VincSpaceUpdateAll calls; never read) (name inferred)

const MeReal kNoHit = 1e25;

inline VincModelPair* pairOf(CxLink* l)
{
    return &((CxPair*)l)->pair;
}

// a model of `space` that is really in it (the line-segment queries)
VincModelID spaceModel(VincSpaceID space, int i)
{
    if (!space->isValid(i))
        return 0;
    VincModelID m = (VincModelID)space->getObject(i);
    if (m == 0 || m->spaceHandle == -1 || (VincSpaceID)m->space != space)
        return 0;
    return m;
}

} // namespace

extern "C" {

// Mac 0x541b99, Win 0x72cbb0
VincSpaceID MEAPI VincSpaceAxisSortCreate(int axisMask, int maxModelCount, int maxPairCount, int pairsDisabled)
{
    CxSpace* s = CxSmallSort::create((unsigned char)axisMask, maxModelCount, maxPairCount, (char)pairsDisabled != 0);
    s->setBoundsFn((CxBoundsFn)VincModelUpdate);
    s->setSphereFn((CxSphereFn)VincModelGetBSphere);
    s->beginChanges();
    return s;
}

// Mac 0x541893, Win 0x72cc00
void MEAPI VincSpaceDestroy(VincSpaceID space)
{
    space->destroy();
}

// Win 0x72cc20
int MEAPI VincSpaceIsChanging(VincSpaceID space)
{
    return space->isChanging();
}

// Mac 0x5418a5, Win 0x72cc40
int MEAPI VincSpaceInsertModel(VincSpaceID space, VincModelID m)
{
    if (m->space != 0)
        return (VincSpaceID)m->space == space;
    if (m->updateTransformFn != 0)
        m->updateTransformFn(m);
    int h = space->insert(m, m->state == 2);
    if (!space->isValid(h))
        return 0;
    m->space = space;
    m->spaceHandle = h;
    return 1;
}

// Mac 0x541923, Win 0x72ccb0
void MEAPI VincSpaceUpdateModel(VincModelID m)
{
    ((VincSpaceID)m->space)->update(m->spaceHandle, 0, false);
}

// Mac 0x54195d, Win 0x72cce0
void MEAPI VincSpaceUpdateModelWithTime(VincModelID m, MeReal dt, int force)
{
    ((VincSpaceID)m->space)->update(m->spaceHandle, dt, force != 0);
}

// Mac 0x541998, Win 0x72cd10
void MEAPI VincSpaceModelSetState2(VincModelID m, int isState2)
{
    ((VincSpaceID)m->space)->setUnsorted(m->spaceHandle, isState2 != 0);
}

// LDD64 (CxSmallSort::takeOut): the handles of models of the first one's space
namespace {
struct ModelHandles {
    int inl[64];
    int* h;
    ModelHandles(VincModelID* models, int count) : h(count <= 64 ? inl : new int[count])
    {
        for (int i = 0; i < count; ++i)
            h[i] = models[i]->spaceHandle;
    }
    ~ModelHandles()
    {
        if (h != inl)
            delete[] h;
    }
};
} // namespace

int MEAPI VincSpaceTakeOutModels(VincModelID* models, int count)
{
    if (count <= 0 || models[0]->space == 0)
        return 0;
    VincSpaceID space = (VincSpaceID)models[0]->space;
    for (int i = 1; i < count; ++i)
        if ((VincSpaceID)models[i]->space != space)
            return 0;
    ModelHandles handles(models, count);
    return ((CxSmallSort*)space)->takeOut(handles.h, count) ? 1 : 0;
}

void MEAPI VincSpacePutBackModels(VincModelID* models, int count)
{
    if (count <= 0 || models[0]->space == 0)
        return;
    ModelHandles handles(models, count);
    ((CxSmallSort*)models[0]->space)->putBack(handles.h, count);
}

void MEAPI VincSpaceDisablePairs(VincModelID m, VincModelID* others, int count)
{
    for (int k = 0; k < count; ++k)
        if (others[k]->space != m->space) {
            for (int i = 0; i < count; ++i)
                VincSpaceDisablePair(m, others[i]);
            return;
        }
    ModelHandles handles(others, count);
    ((CxSmallSort*)m->space)->disablePairs(m->spaceHandle, handles.h, count);
}

int MEAPI VincSpaceGroupNew(VincSpaceID space)
{
    return space != 0 ? ((CxSmallSort*)space)->groupNew() : 0;
}

int MEAPI VincSpaceGroupRecordPartners(VincModelID m, int group, VincModelID* out, int max)
{
    if (m->space == 0)
        return 0;
    CxSmallSort* s = (CxSmallSort*)m->space;
    std::vector<int> handles;
    s->groupRecordPartners(m->spaceHandle, group, handles);
    int n = 0;
    for (size_t i = 0; i < handles.size(); ++i, ++n)
        if (n < max)
            out[n] = (VincModelID)s->getObject(handles[i]);
    return n;
}

void MEAPI VincSpaceGroupJoin(VincModelID m, int group)
{
    if (m->space != 0)
        ((CxSmallSort*)m->space)->groupJoin(m->spaceHandle, group);
}

void MEAPI VincSpaceGroupLeave(VincModelID m)
{
    if (m->space != 0)
        ((CxSmallSort*)m->space)->groupLeave(m->spaceHandle);
}

void MEAPI VincSpaceSetRelocateCrossings(VincSpaceID space, int crossings)
{
    if (space != 0)
        ((CxSmallSort*)space)->relocateCrossings = crossings;
}

// Mac 0x5419c1, Win 0x72cd30
int MEAPI VincSpaceFreezeModel(VincModelID m)
{
    return ((VincSpaceID)m->space)->setActive(m->spaceHandle, false);
}

// Mac 0x5419e9, Win 0x72cd50
int MEAPI VincSpaceUnfreezeModel(VincModelID m)
{
    return ((VincSpaceID)m->space)->setActive(m->spaceHandle, true);
}

// Mac 0x541a11, Win 0x72cd70
int MEAPI VincSpaceModelIsFrozen(VincModelID m)
{
    return !((VincSpaceID)m->space)->isActive(m->spaceHandle);
}

// Win 0x72cda0: the active bit without the handle check
int MEAPI VincSpaceModelIsFrozenNoCheck(VincModelID m)
{
    return !((CxSmallSort*)m->space)->activeBit(m->spaceHandle);
}

// Mac 0x541a34, Win 0x72d2f0
int MEAPI VincSpaceRemoveModel(VincModelID m)
{
    VincSpaceID space = (VincSpaceID)m->space;
    if (space != 0 && space->remove(m->spaceHandle)) {
        m->space = 0;
        m->spaceHandle = -1;
        return 1;
    }
    return 0;
}

// Mac 0x541a78, Win 0x72d330
int MEAPI VincSpaceEnablePair(VincModelID m1, VincModelID m2)
{
    return ((VincSpaceID)m1->space)->setPairEnabled(m1->spaceHandle, m2->spaceHandle, true);
}

// Mac 0x541aaf, Win 0x72d360
int MEAPI VincSpaceDisablePair(VincModelID m1, VincModelID m2)
{
    return ((VincSpaceID)m1->space)->setPairEnabled(m1->spaceHandle, m2->spaceHandle, false);
}

// Win 0x72d390
int MEAPI VincSpaceSetModelPairsEnabled(VincModelID m, int enable)
{
    return ((VincSpaceID)m->space)->setPairEnabled(m->spaceHandle, enable != 0);
}

// Win 0x72d3c0
int MEAPI VincSpaceGetEnabledPairCount(VincSpaceID space)
{
    return space->getEnabledPairCount();
}

// Win 0x72d3e0
int MEAPI VincSpacePairIsEnabled(VincModelID m1, VincModelID m2)
{
    return ((VincSpaceID)m1->space)->isPairEnabled(m1->spaceHandle, m2->spaceHandle);
}

// Win 0x72d410
int MEAPI VincSpacePairIsOverlapping(VincModelID m1, VincModelID m2)
{
    return ((VincSpaceID)m1->space)->isPairOverlapping(m1->spaceHandle, m2->spaceHandle);
}

// Win 0x72d440
VincModelPair* MEAPI VincSpaceGetModelPair(VincModelID m1, VincModelID m2)
{
    return ((VincSpaceID)m1->space)->getPair(m1->spaceHandle, m2->spaceHandle);
}

// Mac 0x541ae6, Win 0x72d470
void MEAPI VincSpaceBeginChanges(VincSpaceID space)
{
    space->beginChanges();
}

// Mac 0x541af9, Win 0x72d490
void MEAPI VincSpaceEndChanges(VincSpaceID space)
{
    space->endChanges();
}

// Mac 0x541b0c, Win 0x72d580
int MEAPI VincSpaceGetModelCount(VincSpaceID space)
{
    int max = space->getMaxObjectCount();
    return max - space->getFreeCount();
}

// Win 0x72ce00
void MEAPI VincSpaceUpdateAll(VincSpaceID space)
{
    ++s_updateCount;
    space->updateAll(0);
}

// Mac 0x541b39, Win 0x72ce20
void MEAPI VincSpacePathUpdate(VincSpaceID space, MeReal dt)
{
    MeProfileStartSection("VincSpacePathUpdate", 0);
    space->updateAll(dt);
    MeProfileEndSection("VincSpacePathUpdate");
}

// Win 0x72ce60
void MEAPI VincSpaceSetAABBFn(VincSpaceID space, void* fn)
{
    space->setBoundsFn((CxBoundsFn)fn);
}

// Win 0x72ce80
void MEAPI VincSpaceSetUserData(VincSpaceID space, void* data)
{
    space->setUserData(data);
}

// Win 0x72cea0
void* MEAPI VincSpaceGetUserData(VincSpaceID space)
{
    return space->getUserData();
}

// Mac 0x541c00, Win 0x72cec0
void MEAPI VincSpacePairIteratorBegin(VincSpaceID space, VincSpacePairIterator* it)
{
    it->goodbye = space->getGoodbyeList()->next;
    it->hello = space->getHelloList()->next;
    it->staying = space->getStayingList()->next;
}

// Mac 0x541c40, Win 0x72d090
int MEAPI VincSpaceGetPairs(VincSpaceID space, VincSpacePairIterator* it, VincModelPairContainer* c)
{
    CxSmallSort* s = (CxSmallSort*)space;
    CxLink* gEnd = space->getGoodbyeList();
    CxLink* g = (CxLink*)it->goodbye;
    while (g != gEnd && c->goodbyeEnd < c->helloFirst) {
        c->array[c->goodbyeEnd++] = pairOf(g);
        g = g->next;
    }
    it->goodbye = g;
    CxLink* sEnd = space->getStayingList();
    CxLink* st = (CxLink*)it->staying;
    while (st != sEnd && c->helloEnd < c->stayingFirst) {
        VincModelPair* p = pairOf(st);
        if (!s->activeBit(p->model1->spaceHandle) && !s->activeBit(p->model2->spaceHandle))
            c->array[--c->stayingFirst] = p;
        else
            c->array[c->helloEnd++] = p;
        st = st->next;
    }
    it->staying = st;
    CxLink* hEnd = space->getHelloList();
    CxLink* h = (CxLink*)it->hello;
    while (h != hEnd && c->goodbyeEnd < c->helloFirst) {
        c->array[--c->helloFirst] = pairOf(h);
        h = h->next;
    }
    it->hello = h;
    return h != hEnd || st != sEnd || g != gEnd;
}

// Mac 0x541df6, Win 0x72cf00
int MEAPI VincSpaceGetActivePairs(VincSpaceID space, VincSpacePairIterator* it, VincModelPairContainer* c)
{
    CxLink* gEnd = space->getGoodbyeList();
    CxLink* g = (CxLink*)it->goodbye;
    while (g != gEnd && c->goodbyeEnd < c->helloFirst) {
        c->array[c->goodbyeEnd++] = pairOf(g);
        g = g->next;
    }
    it->goodbye = g;
    CxLink* sEnd = space->getStayingList();
    CxLink* st = (CxLink*)it->staying;
    while (st != sEnd && c->helloEnd < c->stayingFirst) {
        VincModelPair* p = pairOf(st);
        VincModelID m1 = p->model1, m2 = p->model2;
        if (m1->state == 1 || m2->state == 1)
            c->array[c->helloEnd++] = p;
        else if (((CxSmallSort*)m1->space)->activeBit(m1->spaceHandle) ||
                 ((CxSmallSort*)m2->space)->activeBit(m2->spaceHandle))
            c->array[c->helloEnd++] = p;
        st = st->next;
    }
    it->staying = st;
    CxLink* hEnd = space->getHelloList();
    CxLink* h = (CxLink*)it->hello;
    while (h != hEnd && c->goodbyeEnd < c->helloFirst) {
        c->array[--c->helloFirst] = pairOf(h);
        h = h->next;
    }
    it->hello = h;
    return h != hEnd || st != sEnd || g != gEnd;
}

// Win 0x72d1d0
int MEAPI VincSpaceGetTransitionPairs(VincSpaceID space, VincSpacePairIterator* it, VincModelPairContainer* c)
{
    CxLink* gEnd = space->getGoodbyeList();
    CxLink* g = (CxLink*)it->goodbye;
    // NOTE: at most one goodbye per call (no loop here in the original)
    if (g != gEnd && c->goodbyeEnd < c->helloFirst) {
        c->array[c->goodbyeEnd++] = pairOf(g);
        g = g->next;
    }
    it->goodbye = g;
    CxLink* hEnd = space->getHelloList();
    CxLink* h = (CxLink*)it->hello;
    while (h != hEnd && c->goodbyeEnd < c->helloFirst) {
        c->array[--c->helloFirst] = pairOf(h);
        h = h->next;
    }
    it->hello = h;
    return h != hEnd || g != gEnd;
}

// Win 0x72d4b0
void MEAPI VincSpaceModelIteratorBegin(VincSpaceID, VincSpaceModelIterator* it)
{
    it->handle = 0;
    it->count = 0;
}

// Win 0x72d4d0
int MEAPI VincSpaceGetModel(VincSpaceID space, VincSpaceModelIterator* it, VincModelID* m)
{
    *m = 0;
    int free = space->getFreeCount();
    if (it->count < space->getMaxObjectCount() - free) {
        bool valid = space->isValid(it->handle);
        while (!valid) {
            int h = it->handle;
            if (h >= space->getMaxObjectCount())
                break;
            it->handle = h + 1;
            valid = space->isValid(h + 1);
        }
        int h = it->handle;
        if (h < space->getMaxObjectCount()) {
            *m = (VincModelID)space->getObject(h);
            it->count++;
            it->handle++;
            return 1;
        }
    }
    return 0;
}

// Win 0x790850
int MEAPI VincModelLineSegIntersectFiltered(VincModelID m, const MeReal* p0, const MeReal* p1, VincModelFilterFn filter,
                                            void* data, VincLineSegResult* r)
{
    if (VincModelIsAggregate(m))
        return VincAggregateLineSegIntersectFiltered(m, p0, p1, filter, data, r);
    VincLineSegFn fn = VincGeometryGetLineSegmentFn(m->geometry->type);
    r->m3c = 0;
    if (fn != 0 && fn(m, p0, p1, r) != 0)
        return 1;
    return 0;
}

// Win 0x790650
int VincSpaceLineSegIntersectAllImpl(VincSpaceID space, const MeReal* p0, const MeReal* p1, VincLineSegResult* results,
                                     int max)
{
    int i = 0, hits = 0;
    int n = space->getMaxObjectCount();
    int free = space->getFreeCount();
    int inUse = space->getMaxObjectCount() - free;
    int seen = 0;
    if (max < 1)
        return 0;
    for (;;) {
        if (i >= n || seen >= inUse)
            return hits;
        VincModelID m = spaceModel(space, i);
        if (m != 0) {
            ++seen;
            VincLineSegFn fn = VincGeometryGetLineSegmentFn(m->geometry->type);
            results->m3c = 0;
            if (fn != 0 && fn(m, p0, p1, results) != 0) {
                ++hits;
                results->model = m;
                ++results;
            }
        }
        ++i;
        if (hits >= max)
            return hits;
    }
}

// Win 0x790730
int VincSpaceLineSegIntersectFirstImpl(VincSpaceID space, const MeReal* p0, const MeReal* p1, VincLineSegResult* result)
{
    int n = space->getMaxObjectCount();
    int free = space->getFreeCount();
    int inUse = space->getMaxObjectCount() - free;
    result->distance = kNoHit;
    int seen = 0;
    for (int i = 0; i < n && seen < inUse; ++i) {
        VincModelID m = spaceModel(space, i);
        if (m == 0)
            continue;
        ++seen;
        VincLineSegFn fn = VincGeometryGetLineSegmentFn(m->geometry->type);
        VincLineSegResult r;
        r.m3c = 0;
        r.element = -1;
        if (fn != 0 && fn(m, p0, p1, &r) != 0 && r.distance < result->distance) {
            *result = r;
            result->model = m;
        }
    }
    return result->distance < kNoHit;
}

// Win 0x7908c0
int VincSpaceLineSegIntersectFirstFilteredImpl(VincSpaceID space, const MeReal* p0, const MeReal* p1,
                                               VincModelFilterFn filter, void* data, VincLineSegResult* result)
{
    int n = space->getMaxObjectCount();
    int free = space->getFreeCount();
    int inUse = space->getMaxObjectCount() - free;
    result->distance = kNoHit;
    int seen = 0;
    for (int i = 0; i < n && seen < inUse; ++i) {
        VincModelID m = spaceModel(space, i);
        if (m == 0)
            continue;
        ++seen;
        VincLineSegResult r;
        if (filter(m, data) != 0 && VincModelLineSegIntersectFiltered(m, p0, p1, filter, data, &r) != 0 &&
            r.distance < result->distance) {
            result->distance = r.distance;
            result->model = m;
            result->normal[0] = r.normal[0];
            result->m3c = r.m3c;
            result->normal[1] = r.normal[1];
            result->normal[2] = r.normal[2];
            result->position[0] = r.position[0];
            result->position[1] = r.position[1];
            result->position[2] = r.position[2];
        }
    }
    return result->distance < kNoHit;
}

// Win 0x72d270
int MEAPI VincSpaceLineSegIntersectAll(VincSpaceID space, const MeReal* p0, const MeReal* p1, VincLineSegResult* results,
                                       int max)
{
    return VincSpaceLineSegIntersectAllImpl(space, p0, p1, results, max);
}

// Win 0x72d2a0
int MEAPI VincSpaceLineSegIntersectFirst(VincSpaceID space, const MeReal* p0, const MeReal* p1, VincLineSegResult* result)
{
    return VincSpaceLineSegIntersectFirstImpl(space, p0, p1, result);
}

// Win 0x72d2c0
int MEAPI VincSpaceLineSegIntersectFirstFiltered(VincSpaceID space, const MeReal* p0, const MeReal* p1,
                                                 VincModelFilterFn filter, void* data, VincLineSegResult* result)
{
    return VincSpaceLineSegIntersectFirstFilteredImpl(space, p0, p1, filter, data, result);
}

} // extern "C"
