/* Vortex 5.0.0 build 800 "vortexldd41" - registration of the Vinc primitive geometry types
 * (restored). Mac 0x4f8643, 0x4f8a01 (557g); Win 0x75cd00, 0x7b7b10.
 * Kept apart from VincPrimitives.c (the pair-test registrations) so that a user of the types
 * alone does not pull in every pair test. */
#include "Vinc/VincPrimitives.h"

/* Mac 0x4f8a01, Win 0x7b7b10 */
void MEAPI VincSphereBoxPlaneRegisterTypes(void)
{
    VincSphereRegisterType();
    VincBoxRegisterType();
    VincPlaneRegisterType();
}

/* Mac 0x4f8643, Win 0x75cd00 */
void MEAPI VincPrimitivesRegisterTypes(void)
{
    VincSphereBoxPlaneRegisterTypes();
    VincCylinderRegisterType();
    VincConeRegisterType();
    VincCapsuleRegisterType();
    VincTriangleListRegisterType();
}
