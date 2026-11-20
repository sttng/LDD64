// Vortex 5.0.0 build 800 "vortexldd41" - world groupings (restored).
// Mac 0x48f9e3-0x48fb5e (static initialiser 0x48fb1c / 0x48fb47), Win 0x73e320-0x73e3e0.
// A grouping carries its own Kea and Bcl parameters; bodies select one with MdtBodySetGrouping.
#include "Mdt/MdtWorld.h"
#include "Mdt/MdtConstraint.h"
#include "Me/VinPool.h"

#include <string.h>

// Mac 0x48f9e3, Win 0x73e320: a new grouping with a copy of the world's parameters
MdtGrouping* MEAPI MdtWorldNewGrouping(MdtWorld* w)
{
    if (w->groupingCapacity == w->numGroupings + 1 || w->groupings == 0) {
        int n = w->numGroupings * 2;
        if (n < 10)
            n = 10;
        w->groupingCapacity = n;
        w->groupings = (MdtGrouping**)MeMemoryAPI.resize(w->groupings, n * sizeof(MdtGrouping*));
    }
    MdtGrouping* g = (MdtGrouping*)VinPoolGetStruct(&w->groupingPool);
    g->index = w->numGroupings;
    g->id = w->numGroupings;
    g->m0c = -1;
    memcpy(&g->keaParams, &w->keaParams, sizeof(MdtKeaParameters));
    memcpy(&g->bclParams, &w->bclParams, sizeof(MdtBclSolverParameters));
    g->world = w;
    g->maxConstraints = -1;
    g->maxDepth = -1;
    const int i = w->numGroupings;
    w->groupings[i] = g;
    w->numGroupings = i + 1;
    return g;
}

// Mac 0x48fad8, Win 0x73e3e0: creates groupings up to `index` on demand
MdtGrouping* MEAPI MdtWorldGetGrouping(MdtWorld* w, int index)
{
    if (index < 0)
        return 0;
    while (w->numGroupings < index + 1)
        MdtWorldNewGrouping(w);
    return w->groupings[index];
}
