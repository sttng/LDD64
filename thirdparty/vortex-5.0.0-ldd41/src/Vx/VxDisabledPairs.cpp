// Vortex 5.0.0 build 800 "vortexldd41" - the disabled-pair lists of universes and assemblies
// (restored). Mac TU 452 0x422bb8-0x422e20 (static initialiser 0x422dbb:
// <iostream> only), Win 0x72d620-0x72d730. Class / file names inferred.
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxUniverse.h"
#include <iostream>

namespace Vx {

// Mac 0x422c32, Win 0x72d620
void VxDisabledPairs::apply(VxUniverse* u)
{
    if (u == 0)
        return;
    for (AssemblyPair* i = mAssemblyPairs.begin(); i != mAssemblyPairs.end(); ++i)
        i->a0->disablePairIntersect(i->a1);
    for (GeometryAssembly* i = mGeometryAssemblyPairs.begin(); i != mGeometryAssemblyPairs.end(); ++i)
        i->assembly->disablePairIntersect(i->cg);
    for (GeometryPair* i = mGeometryPairs.begin(); i != mGeometryPairs.end(); ++i)
        u->disablePairIntersect(i->cg0, i->cg1);
}

} // namespace Vx
