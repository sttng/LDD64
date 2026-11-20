// Vortex 5.0.0 build 800 "vortexldd41" - the geometry pair-interaction table and the per-type
// line segment functions (restored; Mac 557c 0x4c13c3-0x4c165a).
//
// Globals: Mac 0x77ce1c typeCount, 0x77ce20 cells (typeCount^2 * 0x24 bytes), 0x77ce24 line segment
// functions (typeCount); Win 0xaf8f6c typeCount, 0xaf8f70 cells.
// Cell (row = type of model2, column = type of model1): cells[type2 * typeCount + type1].
#include "Vinc/VincIntersect.h"
#include "Me/MeMemory.h"

#include "VincInternal.h"

namespace {
int s_pairTypeCount = 0;               // Mac 0x77ce1c, Win 0xaf8f6c
VincPairTableCell* s_pairCells = 0;    // Mac 0x77ce20, Win 0xaf8f70
VincLineSegFn* s_lineSegFns = 0;       // Mac 0x77ce24
}

extern "C" {

// Mac 0x4c13c3, Win 0x7ac860 (name inferred)
void MEAPI VincInteractionsReset(VincInteractions* i)
{
    i->helloFn = 0;
    i->goodbyeFn = 0;
    i->intersectFn = 0;
    i->sweptIntersectFn = 0;
    i->safeTimeFn = 0;
    i->distanceFn = 0;
    i->distanceBoundedFn = 0;
}

// Mac 0x4c13fb, Win 0x7ac880 (name inferred)
void MEAPI VincInteractionsCopy(const VincInteractions* src, VincInteractions* dst)
{
    dst->helloFn = src->helloFn;
    dst->goodbyeFn = src->goodbyeFn;
    dst->intersectFn = src->intersectFn;
    dst->sweptIntersectFn = src->sweptIntersectFn;
    dst->safeTimeFn = src->safeTimeFn;
    dst->distanceFn = src->distanceFn;
    dst->distanceBoundedFn = src->distanceBoundedFn;
}

// Mac 0x4c142e, Win 0x7ac8c0 (name inferred)
void MEAPI VincPairTableCreate(int typeCount)
{
    s_pairTypeCount = typeCount;
    s_pairCells = (VincPairTableCell*)MeMemoryAPI.create(typeCount * typeCount * sizeof(VincPairTableCell));
    for (int i = 0; i < s_pairTypeCount; ++i) {
        for (int j = 0; j < s_pairTypeCount; ++j) {
            VincPairTableCell* c = &s_pairCells[s_pairTypeCount * i + j];
            c->getFn = 0;
            c->swap = 0;
            VincInteractionsReset(&c->interactions);
        }
    }
    s_lineSegFns = (VincLineSegFn*)MeMemoryAPI.create(s_pairTypeCount * sizeof(VincLineSegFn));
    for (int i = 0; i < s_pairTypeCount; ++i)
        s_lineSegFns[i] = 0;
}

// Mac 0x4c14f9, Win 0x7ac9a0 (name inferred). NOTE: the pointers are not cleared.
void MEAPI VincPairTableDestroy(void)
{
    MeMemoryAPI.destroy(s_pairCells);
    MeMemoryAPI.destroy((void*)s_lineSegFns);
}

// Mac 0x4c1534, Win 0x7ac9c0 (name inferred)
void MEAPI VincGeometryPairRegisterInteractions(int typeA, int typeB, const VincInteractions* i,
                                          VincInteractionsGetFn getFn)
{
    VincPairTableCell* c = &s_pairCells[typeB * s_pairTypeCount + typeA];
    c->swap = 0;
    VincInteractionsCopy(i, &c->interactions);
    c->getFn = getFn;
    if (typeA != typeB) {
        c = &s_pairCells[typeA * s_pairTypeCount + typeB];
        c->swap = 1;
        VincInteractionsCopy(i, &c->interactions);
        c->getFn = getFn;
    }
}

// Mac 0x4c15ce, Win 0x7acb10 (name inferred)
VincInteractions* MEAPI VincPairTableGetInteractions(VincModelPair* pair, int* swap)
{
    VincPairTableCell* c = &s_pairCells[pair->model2->geometry->type * s_pairTypeCount
                                        + pair->model1->geometry->type];
    *swap = c->swap;
    if (c->getFn == 0)
        return &c->interactions;
    return c->getFn(pair);
}

// (not in the binaries; tests / introspection) cell of (model1 type1, model2 type2)
const VincPairTableCell* MEAPI VincPairTableGetCell(int type1, int type2)
{
    if (s_pairCells == 0 || type1 < 0 || type2 < 0 || type1 >= s_pairTypeCount
        || type2 >= s_pairTypeCount)
        return 0;
    return &s_pairCells[type2 * s_pairTypeCount + type1];
}

// Mac 0x4c1627, Win 0x7acb60 (name inferred)
void MEAPI VincGeometryRegisterLineSegmentFn(int typeId, VincLineSegFn fn)
{
    s_lineSegFns[typeId] = fn;
}

// Mac 0x4c1643, Win 0x7acb80 (name inferred)
VincLineSegFn MEAPI VincGeometryGetLineSegmentFn(int typeId)
{
    return s_lineSegFns[typeId];
}

// Mac 0x4c165a, Win 0x775800 (name inferred)
int MEAPI VincLineSegIntersect(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r)
{
    r->model = m;
    VincLineSegFn fn = VincGeometryGetLineSegmentFn(m->geometry->type);
    r->m3c = 0;
    if (fn != 0 && fn(m, p0, p1, r) != 0)
        return 1;
    r->element = -1;
    return 0;
}

} // extern "C"
