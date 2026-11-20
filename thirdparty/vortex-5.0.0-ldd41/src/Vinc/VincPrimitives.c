/* Vortex 5.0.0 build 800 "vortexldd41" - registration of the Vinc primitive geometry types and
 * of their pair tests (restored). Mac 0x4f85eb-0x4f8a01, 0x4f6b6f (557g);
 * Win 0x75cd00, 0x7b7b10, 0x76a8b0. Called by the Vx::VxFrame constructor (Mac 0x454ec8 /
 * Win 0x70e190: types) and Mac 0x45742f / Win 0x7111d0 (interactions). Names inferred from the
 * Mcd API (McdSphereBoxPlaneRegisterTypes, McdPrimitivesRegisterTypes, ...). */
#include "Vinc/VincPrimitives.h"

/* Mac 0x4f89d1 (Win: inlined) */
void MEAPI VincSphereBoxPlaneRegisterInteractions(void)
{
    VincSphereSphereRegisterInteraction();
    VincBoxBoxRegisterInteraction();
    VincBoxPlaneRegisterInteraction();
    VincBoxSphereRegisterInteraction();
    VincSpherePlaneRegisterInteraction();
    VincBoxRegisterLineSegment();
    VincSphereRegisterLineSegment();
    VincPlaneRegisterLineSegment();
}

/* Mac 0x4f85eb (Win: inlined) */
int MEAPI VincPrimitivesRegisterInteractions(void)
{
    VincSphereBoxPlaneRegisterInteractions();
    VincConePlaneRegisterInteraction();
    VincConeSphereRegisterInteraction();
    VincConeBoxRegisterInteraction();
    VincConeConeRegisterInteraction();
    VincConeRegisterLineSegment();
    VincBoxCylinderRegisterInteraction();
    VincCylinderPlaneRegisterInteraction();
    VincCylinderSphereRegisterInteraction();
    VincCylinderCylinderRegisterInteraction();
    VincCylinderRegisterLineSegment();
    VincConeCylinderRegisterInteraction();
    VincSphereTriangleListRegisterInteraction();
    VincBoxTriangleListRegisterInteraction();
    VincCylinderTriangleListRegisterInteraction();
    return 1;
}

/* Mac 0x4f6b6f, Win 0x76a8b0 */
void MEAPI VincCapsuleRegisterTypeAndInteractions(void)
{
    VincCapsuleRegisterType();
    VincCapsuleRegisterLineSegment();
    VincCapsulePlaneRegisterInteraction();
    VincCapsuleSphereRegisterInteraction();
    VincBoxCapsuleRegisterInteraction();
    VincCylinderCapsuleRegisterInteraction();
    VincCapsuleCapsuleRegisterInteraction();
    VincCapsuleTriangleListRegisterInteraction();
}
