// Vortex 5.0.0 build 800 "vortexldd41" - Vinc framework: VincInit / VincTerm, terminate actions
// and the geometry type registry (restored; Mac sub-TU 557c 0x4bfde8-0x4c0287).
// The registry: seven parallel tables of typeCount = geoTypeMaxCount + 13 entries, allocated zeroed
// by VincInit (sized with sizeof(void*), 4 in both originals, for 64-bit hosts):
//   Mac 0x77ce00 destroy, 0x77ce04 updateAABB, 0x77ce08 getXYAABB, 0x77ce0c getBSphere,
//       0x77ce10 maximumPoint, 0x77ce14 getMassProperties, 0x77ce18 type names
//   Win 0xaf8f54 destroy, 0xaf8f58 updateAABB, 0xaf8f40 getXYAABB, 0xaf8f50 getBSphere,
//       0xaf8f48 maximumPoint, 0xaf8f4c getMassProperties, 0xaf8f44 type names
//   typeCount Mac 0x779544 / Win 0xaf8f5c, highest id + 1 Mac 0x779548 / Win 0xaf8f60.
#include "Vinc/VincIntersect.h"
#include "Me/MeMemory.h"
#include "Me/MeVersion.h"

#include "VincInternal.h"

extern "C" {

// Mac 0x779540, Win 0xaf25bc
VincFramework* VincTheFramework = 0;

} // extern "C"

namespace {

int s_typeCount = 0;                                  // Mac 0x779544
int s_typeEnd = 0;                                    // Mac 0x779548 (highest registered id + 1)
VincGeometryDestroyFn* s_destroyFns = 0;              // Mac 0x77ce00
VincGeometryUpdateAABBFn* s_updateAABBFns = 0;        // Mac 0x77ce04
VincGeometryGetXYAABBFn* s_getXYAABBFns = 0;          // Mac 0x77ce08
VincGeometryGetBSphereFn* s_getBSphereFns = 0;        // Mac 0x77ce0c
VincGeometryMaximumPointFn* s_maximumPointFns = 0;    // Mac 0x77ce10
VincGeometryGetMassPropertiesFn* s_massPropertiesFns = 0; // Mac 0x77ce14
const char** s_typeNames = 0;                         // Mac 0x77ce18

// Mac 0x4bff50 (name inferred). Inlined into VincInit on Win (0x7abcd0 is the out-of-line copy).
void MEAPI VincGeometryRegistryCreate(int geoTypeMaxCount)
{
    s_typeCount = geoTypeMaxCount + VINC_BUILTIN_GEOMETRY_TYPES;
    s_typeEnd = 0;
    s_destroyFns = (VincGeometryDestroyFn*)MeMemoryAPI.createZeroed(s_typeCount * sizeof(void*));
    s_updateAABBFns = (VincGeometryUpdateAABBFn*)MeMemoryAPI.createZeroed(s_typeCount * sizeof(void*));
    s_getXYAABBFns = (VincGeometryGetXYAABBFn*)MeMemoryAPI.createZeroed(s_typeCount * sizeof(void*));
    s_getBSphereFns = (VincGeometryGetBSphereFn*)MeMemoryAPI.createZeroed(s_typeCount * sizeof(void*));
    s_maximumPointFns = (VincGeometryMaximumPointFn*)MeMemoryAPI.createZeroed(s_typeCount * sizeof(void*));
    s_massPropertiesFns = (VincGeometryGetMassPropertiesFn*)MeMemoryAPI.createZeroed(s_typeCount * sizeof(void*));
    s_typeNames = (const char**)MeMemoryAPI.createZeroed(s_typeCount * sizeof(void*));
}

// Mac 0x4c000e (name inferred). NOTE: the table pointers are not cleared.
void MEAPI VincGeometryRegistryDestroy(void)
{
    s_typeCount = 0;
    s_typeEnd = 0;
    MeMemoryAPI.destroy((void*)s_destroyFns);
    MeMemoryAPI.destroy((void*)s_updateAABBFns);
    MeMemoryAPI.destroy((void*)s_getXYAABBFns);
    MeMemoryAPI.destroy((void*)s_getBSphereFns);
    MeMemoryAPI.destroy((void*)s_maximumPointFns);
    MeMemoryAPI.destroy((void*)s_massPropertiesFns);
    MeMemoryAPI.destroy((void*)s_typeNames);
}

} // namespace

extern "C" {

// Mac 0x4bfde8, Win 0x75ce10 (name inferred): LIFO list, run by VincTerm.
void MEAPI VincRegisterTerminateAction(VincTerminateFn fn)
{
    VincTerminateAction* a = (VincTerminateAction*)MeMemoryAPI.create(sizeof(VincTerminateAction));
    a->fn = fn;
    a->next = VincTheFramework->terminateActions;
    VincTheFramework->terminateActions = a;
}

// Mac 0x4bfe24 (Win inlined into the VxFrame teardown)
void MEAPI VincTerm(void)
{
    if (VincTheFramework != 0) {
        VincGeometryRegistryDestroy();
        VincModelPoolDestroy();
        VincPairTableDestroy();
        // NOTE: an action that registers a new action while running would have the new one
        // popped (and freed) instead of itself; kept as in the original.
        while (VincTheFramework->terminateActions != 0) {
            VincTheFramework->terminateActions->fn();
            VincTerminateAction* a = VincTheFramework->terminateActions;
            VincTheFramework->terminateActions = a->next;
            MeMemoryAPI.destroy(a);
        }
        MeMemoryAPI.destroy(VincTheFramework);
        VincTheFramework = 0;
    }
}

// Mac 0x4bfe9e, Win 0x75cd20
void MEAPI VincInit(int geoTypeMaxCount, int modelCount)
{
    VincTheFramework = (VincFramework*)MeMemoryAPI.create(sizeof(VincFramework));
    VincTheFramework->terminateActions = 0;
    VincTheFramework->defaultPadding = (MeReal)0.005f;   // 0x3f747ae140000000 on both
    VincTheFramework->version = MeGetVersionString();
    VincGeometryRegistryCreate(geoTypeMaxCount);
    VincModelPoolInit(modelCount);
    VincRequestInitDefault();
    VincPairTableCreate(geoTypeMaxCount + VINC_BUILTIN_GEOMETRY_TYPES);
    VincSetHelloCallback(0, 0);
    VincTheFramework->initialized = 1;
}

// Mac 0x4bff42, Win 0x7abca0 (name inferred)
void MEAPI VincGeometryInit(VincGeometryID g, int typeId)
{
    g->type = (MeI16)typeId;
}

// Mac 0x4c0091, Win 0x7abe10 (name inferred)
int MEAPI VincGeometryGetTypeCount(void)
{
    return (MeI16)s_typeCount;
}

// Mac 0x4c00a3, Win 0x7abe20 (name inferred)
void MEAPI VincGeometryDestroy(VincGeometryID g)
{
    if (g != 0)
        s_destroyFns[g->type](g);
}

// Mac 0x4c00cc, Win 0x7abe40 (name inferred)
void MEAPI VincGeometryUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 minCorner, MeVector3 maxCorner,
                            MeReal* aabb6)
{
    s_updateAABBFns[g->type](g, tm, minCorner, maxCorner, aabb6);
}

// Mac 0x4c010d, Win 0x7abe70 (name inferred)
void MEAPI VincGeometryGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal* out4)
{
    s_getXYAABBFns[g->type](g, tm, out4);
}

// Mac 0x4c0140, Win 0x7abe90 (name inferred)
void MEAPI VincGeometryGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    s_getBSphereFns[g->type](g, center, radius);
}

// Mac 0x4c0173, Win 0x7abeb0 (name inferred)
void MEAPI VincGeometryMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    s_maximumPointFns[g->type](g, tm, dir, out);
}

// Mac 0x4c01ad, Win 0x7abee0 ("VincGeometryGetMassProperties" in VincErrorTable id 40)
MeI16 MEAPI VincGeometryGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia,
                                    MeReal* volume)
{
    MeI16 status = (MeI16)s_massPropertiesFns[g->type](g, relTM, inertia, volume);
    MeReal s;
    s = (inertia[1][0] + inertia[0][1]) * 0.5;
    inertia[0][1] = s;
    inertia[1][0] = s;
    s = (inertia[2][0] + inertia[0][2]) * 0.5;
    inertia[0][2] = s;
    inertia[2][0] = s;
    s = (inertia[1][2] + inertia[2][1]) * 0.5;
    inertia[2][1] = s;
    inertia[1][2] = s;
    return status;
}

// Mac 0x4c0240, Win 0x7ac0e0 (name inferred)
void MEAPI VincAABBGetMinMax(const MeReal* aabb6, MeVector3 minCorner, MeVector3 maxCorner)
{
    minCorner[0] = aabb6[0];
    maxCorner[0] = aabb6[1];
    minCorner[1] = aabb6[2];
    maxCorner[1] = aabb6[3];
    minCorner[2] = aabb6[4];
    maxCorner[2] = aabb6[5];
}

// Mac 0x4c0287, Win 0x7ac030
int MEAPI VincGeometryRegisterType(int typeId, const char* typeName, VincGeometryDestroyFn destroy,
                             VincGeometryUpdateAABBFn updateAABB, VincGeometryGetXYAABBFn getXYAABB,
                             VincGeometryGetBSphereFn getBSphere,
                             VincGeometryMaximumPointFn maximumPoint,
                             VincGeometryGetMassPropertiesFn getMassProperties)
{
    if (s_typeEnd < typeId + 1)
        s_typeEnd = typeId + 1;
    // error 21 is fatal (MeFatalError + abort): the stores below never run out of range
    if (s_typeCount < s_typeEnd || s_typeCount < typeId)
        VincError(VincErrorTable, 21, " in VincGeometryRegisterType.");
    int i = (MeI16)typeId;
    s_destroyFns[i] = destroy;
    s_updateAABBFns[i] = updateAABB;
    s_getXYAABBFns[i] = getXYAABB;
    s_getBSphereFns[i] = getBSphere;
    s_maximumPointFns[i] = maximumPoint;
    s_massPropertiesFns[i] = getMassProperties;
    s_typeNames[i] = typeName;
    return (MeI16)typeId;
}

// Win 0x7abf40 (name inferred; dead-stripped on the Mac)
int MEAPI VincGeometryTypeIsRegistered(int typeId)
{
    if (-1 < typeId && typeId < s_typeCount && s_destroyFns[typeId] != 0)
        return 1;
    return 0;
}

// Win 0x7abf70 (name inferred; dead-stripped on the Mac)
const char* MEAPI VincGeometryGetTypeName(int typeId)
{
    if (-1 < typeId && typeId < s_typeCount && s_destroyFns[typeId] != 0)
        return s_typeNames[typeId];
    return "UnknownGeometryType";
}

// Win 0x7abfb0 (name inferred; dead-stripped on the Mac)
const char* MEAPI VincGeometryGetTypeNameOf(VincGeometryID g)
{
    int t = g->type;
    if (-1 < t && t < s_typeCount && s_destroyFns[t] != 0)
        return s_typeNames[t];
    return "UnknownGeometryType";
}

// Win 0x7abff0 (name inferred; dead-stripped on the Mac)
int MEAPI VincGeometryIsValid(VincGeometryID g)
{
    if (g != 0) {
        int t = g->type;
        if (-1 < t && t < s_typeCount && s_destroyFns[t] != 0)
            return 1;
    }
    return 0;
}

// (test/introspection helper, not in the binaries) registered functions of one type
void MEAPI VincGeometryGetTypeFunctions(int typeId, void** fns7)
{
    fns7[0] = (void*)s_destroyFns[typeId];
    fns7[1] = (void*)s_updateAABBFns[typeId];
    fns7[2] = (void*)s_getXYAABBFns[typeId];
    fns7[3] = (void*)s_getBSphereFns[typeId];
    fns7[4] = (void*)s_maximumPointFns[typeId];
    fns7[5] = (void*)s_massPropertiesFns[typeId];
    fns7[6] = (void*)s_typeNames[typeId];
}

} // extern "C"
