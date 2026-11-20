// Vx RTTI classes without a hand-written header yet, generated from the original binaries' class
// information. A class restored in its own header leaves this file. Slot names vfNN are
// placeholders (NN = Mac slot); every slot carries [M:mac addr] [W:win addr]. Win has one scalar
// deleting dtor where Mac has D1+D0. Bases outside this file are forward-declared hand-written
// classes (included below).
#ifndef VX_DETAIL_VXRTTISKELETONS_H
#define VX_DETAIL_VXRTTISKELETONS_H

#include "Vx/VxBase.h"
#include "Vx/VxUniverse.h"
#include "Vx/VxConstraint.h"
#include "Vx/VxIntersect.h"
#include "Vx/VxSubscriberListResponseHandler.h"
#include "Vx/VxStepperBase.h"
#include "Vx/VxGeometry.h"
#include "Vx/VxAssemblyBase.h"
#include "Vx/VxVisitor.h"
#include "Vx/VxStateBase.h"
#include "Vx/VxNamePathBaseInterface.h"
#include "Vx/ConstraintAccessor.h"
#include "Vx/VxFluidState.h"

namespace Vx {

// Win-only class (dead-stripped on Mac): CablePartAttachment
class CablePartAttachment : public Vx::ConstraintAccessor {
public:
    virtual ~CablePartAttachment(); // slot 0 [M:None] [W:0x715db0]
    // slot 1 overridden [M:None] [W:0x781f00]
    // slot 2 overridden [M:None] [W:0x785f80]
};

class GatherAllCollisionSensor : public virtual Vx::VxVisitor {
public:
    virtual void vf00(); // [M:0x485fc8] [W:None]
    virtual void vf01(); // [M:0x485f40] [W:None]
    virtual void vf02(); // [M:0x485364] [W:None]
    virtual void vf03(); // [M:0x48536e] [W:None]
    virtual void vf04(); // [M:0x485378] [W:None]
    virtual void vf05(); // [M:0x485382] [W:None]
    virtual void vf06(); // [M:0x4869ba] [W:None]
    virtual void vf07(); // [M:0x485396] [W:None]
    virtual void vf08(); // [M:0x4853a0] [W:None]
};

class GatherAllConstraints : public virtual Vx::VxVisitor {
public:
    virtual void vf00(); // [M:0x485ba0] [W:None]
    virtual void vf01(); // [M:0x485b2a] [W:None]
    virtual void vf02(); // [M:0x485364] [W:None]
    virtual void vf03(); // [M:0x48536e] [W:None]
    virtual void vf04(); // [M:0x485378] [W:None]
    virtual void vf05(); // [M:0x485382] [W:None]
    virtual void vf06(); // [M:0x48538c] [W:None]
    virtual void vf07(); // [M:0x485e4c] [W:None]
    virtual void vf08(); // [M:0x4853a0] [W:None]
};

class GatherAllConstraintsAndSensors : public Vx::GatherAllConstraints, public Vx::GatherAllCollisionSensor {
public:
    virtual void vf00(); // slot 0 [M:0x48654e] [W:None]
    virtual void vf01(); // slot 1 [M:0x48642c] [W:None]
    virtual void vf02(); // slot 2 [M:0x485364] [W:None]
    virtual void vf03(); // slot 3 [M:0x48536e] [W:None]
    virtual void vf04(); // slot 4 [M:0x485378] [W:None]
    virtual void vf05(); // slot 5 [M:0x485382] [W:None]
    virtual void vf06(); // slot 6 [M:0x45c4c8] [W:None]
    virtual void vf07(); // slot 7 [M:0x485e4c] [W:None]
    virtual void vf08(); // slot 8 [M:0x4853a0] [W:None]
};

class VxAcceptNotificationBase {
public:
};

class VxCableBase {
public:
};

class VxCable : public Vx::VxAssemblyBase, public Vx::VxCableBase {
public:
};

// Win-only class (dead-stripped on Mac): VxCableIntersectSub
class VxCableIntersectSub : public Vx::VxUniverse::IntersectSubscriber {
public:
    virtual void vf00(); // [M:None] [W:0x7f36d0]
    virtual void vf01(); // [M:None] [W:0x7f3490]
    virtual void vf02(); // [M:None] [W:0x7f1710]
};

// Win-only class (dead-stripped on Mac): VxCableState
class VxCableState : public Vx::VxStateBase {
public:
    virtual ~VxCableState(); // slot 0 [M:None] [W:0x7f6280]
    // slot 1 overridden [M:None] [W:0x7f39a0]
    // slot 2 overridden [M:None] [W:0x7f66a0]
    // slot 3 overridden [M:None] [W:0x7f5ad0]
    // slot 4 overridden [M:None] [W:0x7f4c70]
    // slot 5 overridden [M:None] [W:0x7f4cf0]
    // slot 6 overridden [M:None] [W:0x7f4ca0]
    // slot 7 overridden [M:None] [W:0x7f4800]
    // slot 8 overridden [M:None] [W:0x7f3ca0]
    // slot 9 overridden [M:None] [W:0x7f46f0]
};

// Win-only class (dead-stripped on Mac): VxConnectionTemplate
class VxConnectionTemplate {
public:
    virtual void vf00(); // [M:None] [W:0x7fe8b0]
};

// Win-only class (dead-stripped on Mac): VxHeightFieldFluidState
class VxHeightFieldFluidState : public Vx::VxFluidState {
public:
    // slot 0 overridden [M:None] [W:0x74bfe0]
    // slot 1 overridden [M:None] [W:0x7455a0]
    // slot 2 overridden [M:None] [W:0x745670]
};

// Win-only class (dead-stripped on Mac): VxIntersectRules
class VxIntersectRules {
public:
    // slot 0 overridden [M:None] [W:0x744940]
    // slot 2 overridden [M:None] [W:0x744170]
};

// Win-only class (dead-stripped on Mac): VxNameBaseBaseUnknownInterface
class VxNameBaseBaseUnknownInterface : public Vx::VxNamePathBaseInterface {
public:
    virtual ~VxNameBaseBaseUnknownInterface(); // slot 0 [M:None] [W:0x823060]
    // slot 1 overridden [M:None] [W:0x822710]
    // slot 2 overridden [M:None] [W:0x822720]
};

// Win-only class (dead-stripped on Mac): VxPressureSinkageParams
class VxPressureSinkageParams {
public:
    virtual void vf00(); // [M:None] [W:0x801360]
    virtual void vf01(); // [M:None] [W:0x800af0]
    virtual void vf02(); // [M:None] [W:0x7ffd60]
    virtual void vf03(); // [M:None] [W:0x719870]
};

// Win-only class (dead-stripped on Mac): VxPressureSinkageParamsBekker
class VxPressureSinkageParamsBekker : public Vx::VxPressureSinkageParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x801410]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800b10]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801d00]
};

// Win-only class (dead-stripped on Mac): VxPressureSinkageParamsMuskeg
class VxPressureSinkageParamsMuskeg : public Vx::VxPressureSinkageParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x801570]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800b50]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801b20]
};

// Win-only class (dead-stripped on Mac): VxPressureSinkageParamsSnow
class VxPressureSinkageParamsSnow : public Vx::VxPressureSinkageParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x801620]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800b70]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801b50]
};

// Win-only class (dead-stripped on Mac): VxPressureSinkageParamsWong
class VxPressureSinkageParamsWong : public Vx::VxPressureSinkageParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x8014c0]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800b30]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801ae0]
};

// Win-only class (dead-stripped on Mac): VxPrismatic
class VxPrismatic : public Vx::VxConstraint {
public:
    // slot 0 overridden [M:None] [W:0x7237a0]
    // slot 1 overridden [M:None] [W:0x723700]
    virtual ~VxPrismatic(); // slot 2 [M:None] [W:0x723d30]
    // slot 3 overridden [M:None] [W:0x723d20]
    // slot 4 overridden [M:None] [W:0x71fec0]
    // slot 5 overridden [M:None] [W:0x7221a0]
    // slot 6 overridden [M:None] [W:0x71ffb0]
    // slot 7 overridden [M:None] [W:0x71ff80]
    // slot 8 overridden [M:None] [W:0x71ffe0]
    // slot 9 overridden [M:None] [W:0x720100]
    // slot 10 overridden [M:None] [W:0x720080]
    // slot 11 overridden [M:None] [W:0x720150]
    // slot 12 overridden [M:None] [W:0x721a70]
    // slot 13 overridden [M:None] [W:0x721ba0]
    // slot 14 overridden [M:None] [W:0x7217d0]
    // slot 15 overridden [M:None] [W:0x723600]
    // slot 16 overridden [M:None] [W:0x723680]
};

// Win-only class (dead-stripped on Mac): VxShearingParams
class VxShearingParams {
public:
    virtual void vf00(); // [M:None] [W:0x8016d0]
    virtual void vf01(); // [M:None] [W:0x800b90]
    virtual void vf02(); // [M:None] [W:0x801c00]
    virtual void vf03(); // [M:None] [W:0x719870]
};

// Win-only class (dead-stripped on Mac): VxShearingParamsExponential
class VxShearingParamsExponential : public Vx::VxShearingParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x801780]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800bb0]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801d30]
};

// Win-only class (dead-stripped on Mac): VxShearingParamsHump
class VxShearingParamsHump : public Vx::VxShearingParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x801830]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800bd0]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801b90]
};

// Win-only class (dead-stripped on Mac): VxShearingParamsWong
class VxShearingParamsWong : public Vx::VxShearingParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x8018e0]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800bf0]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801bc0]
};

// Win-only class (dead-stripped on Mac): VxTriangleBin
class VxTriangleBin {
public:
    virtual void vf00(); // [M:None] [W:0x7ee230]
    virtual void vf01(); // [M:None] [W:0x7ef170]
    virtual void vf02(); // [M:None] [W:0x7f0920]
    virtual void vf03(); // [M:None] [W:0x7ef190]
    virtual void vf04(); // [M:None] [W:0x7ef6b0]
};

// Win-only class (dead-stripped on Mac): VxSubTriBin
class VxSubTriBin : public Vx::VxTriangleBin {
public:
    virtual void vf02(); // slot 2 override [M:None] [W:0x822380]
    virtual void vf05(); // [M:None] [W:0x822370]
    virtual void vf06(); // [M:None] [W:0x7f3490]
};

// Win-only class (dead-stripped on Mac): VxTerrain
class VxTerrain : public Vx::VxGeometry {
public:
    // slot 0 overridden [M:None] [W:0x776ce0]
    // slot 1 overridden [M:None] [W:0x775ea0]
    virtual ~VxTerrain(); // slot 2 [M:None] [W:0x777a80]
    // slot 3 overridden [M:None] [W:0x7779f0]
    // slot 4 overridden [M:None] [W:0x72a6d0]
    // slot 5 overridden [M:None] [W:0x7114d0]
    // slot 6 overridden [M:None] [W:0x775af0]
    // slot 7 overridden [M:None] [W:0x7114e0]
    // slot 8 overridden [M:None] [W:0x7114e0]
    // slot 9 overridden [M:None] [W:0x713e90]
    // slot 10 overridden [M:None] [W:0x713e70]
    // slot 11 overridden [M:None] [W:0x713eb0]
};

// Win-only class (dead-stripped on Mac): VxTerrainFluidState
class VxTerrainFluidState : public Vx::VxFluidState {
public:
    // slot 0 overridden [M:None] [W:0x74bfb0]
    // slot 1 overridden [M:None] [W:0x7453b0]
    // slot 2 overridden [M:None] [W:0x745490]
};

// Win-only class (dead-stripped on Mac): VxTerrainParameters
class VxTerrainParameters {
public:
    virtual void vf00(); // [M:None] [W:0x7776b0]
};

class VxThreadPool {
public:
};

class VxThreadPoolPthread : public Vx::VxThreadPool {
public:
    virtual void vf00(); // [M:0x487b88] [W:None]
    virtual void vf01(); // [M:0x487b14] [W:None]
    virtual void vf02(); // [M:0x48724e] [W:None]
    virtual void vf03(); // [M:0x486da2] [W:None]
    virtual void vf04(); // [M:0x487016] [W:None]
    virtual void vf05(); // [M:0x486e2c] [W:None]
    virtual void vf06(); // [M:0x48768c] [W:None]
    virtual void vf07(); // [M:0x487682] [W:None]
    virtual void vf08(); // [M:0x4870b6] [W:None]
    virtual void vf09(); // [M:0x4876ba] [W:None]
    virtual void vf10(); // [M:0x4876c6] [W:None]
    virtual void vf11(); // [M:0x4876b0] [W:None]
    virtual void vf12(); // [M:0x4876aa] [W:None]
};

// Win-only class (dead-stripped on Mac): VxThreadPoolWin
class VxThreadPoolWin : public Vx::VxThreadPool {
public:
    virtual void vf00(); // [M:None] [W:0x754080]
    virtual void vf01(); // [M:None] [W:0x753a60]
    virtual void vf02(); // [M:None] [W:0x753640]
    virtual void vf03(); // [M:None] [W:0x753c90]
    virtual void vf04(); // [M:None] [W:0x753690]
    virtual void vf05(); // [M:None] [W:0x753cd0]
    virtual void vf06(); // [M:None] [W:0x753700]
    virtual void vf07(); // [M:None] [W:0x7537b0]
    virtual void vf08(); // [M:None] [W:0x753e60]
    virtual void vf09(); // [M:None] [W:0x753ee0]
    virtual void vf10(); // [M:None] [W:0x753e50]
    virtual void vf11(); // [M:None] [W:0x7535d0]
};

// Win-only class (dead-stripped on Mac): VxTireModelParams
class VxTireModelParams {
public:
    virtual void vf00(); // [M:None] [W:0x7ffda0]
    virtual void vf01(); // [M:None] [W:0x800a50]
    virtual void vf02(); // [M:None] [W:0x801a40]
    virtual void vf03(); // [M:None] [W:0x719870]
};

// Win-only class (dead-stripped on Mac): VxTireModelParamsCompositeSlip
class VxTireModelParamsCompositeSlip : public Vx::VxTireModelParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x801150]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800a90]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801a70]
};

// Win-only class (dead-stripped on Mac): VxTireModelParamsFiala
class VxTireModelParamsFiala : public Vx::VxTireModelParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x801200]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800ab0]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801aa0]
};

// Win-only class (dead-stripped on Mac): VxTireModelParamsMagicFormula
class VxTireModelParamsMagicFormula : public Vx::VxTireModelParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x8010a0]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800a70]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801cd0]
};

// Win-only class (dead-stripped on Mac): VxTireModelParamsSoftTerrain
class VxTireModelParamsSoftTerrain : public Vx::VxTireModelParams {
public:
    virtual void vf00(); // slot 0 override [M:None] [W:0x8012b0]
    virtual void vf01(); // slot 1 override [M:None] [W:0x800ad0]
    virtual void vf02(); // slot 2 override [M:None] [W:0x801ca0]
    virtual void vf03(); // slot 3 override [M:None] [W:0x801c30]
};

// Win-only class (dead-stripped on Mac): VxTriangleSubscriber
class VxTriangleSubscriber {
public:
    virtual void vf00(); // [M:None] [W:0x777900]
    virtual void vf01(); // [M:None] [W:0x752d40]
    virtual void vf02(); // [M:None] [W:0x752d40]
    virtual void vf03(); // [M:None] [W:0x7778e0]
    virtual void vf04(); // [M:None] [W:0x7778f0]
};

} // namespace Vx

#endif
