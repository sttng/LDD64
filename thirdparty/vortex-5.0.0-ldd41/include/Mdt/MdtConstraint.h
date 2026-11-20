// Vortex 5.0.0 build 800 "vortexldd41" - MdtConstraint, joint types, MdtLimit and MdtContact
// API (restored). Sources: src/Mdt/MdtConstraint.cpp (Mac 0x48cfa2-0x48f407,
// Win 0x72de10-0x72fa70), MdtLimit.cpp, MdtContact.cpp and one file per joint type.
// "Create" functions take the constraint pool (VinPool) - VxFrame's or the world's -, the
// "Create(world)" variants add the constraint to a world. Names inferred (Karma-style) except
// those taken from strings ("MdtConstraintRelaxConstraint",
// "MdtConstraintSetRelaxConstraintStiffness", "MdtConstraintLockConstraint").
#ifndef MDT_MDTCONSTRAINT_H
#define MDT_MDTCONSTRAINT_H

#include "Mdt/MdtTypes.h"
#include "Me/MeCall.h"

// ---- size of the largest constraint type (src/Mdt/MdtConstraintSizes.cpp) -------------------
// Mac 0x4929de, Win 0x752780: maximum of the per-type sizes (Mac table 0x644660).
int MEAPI MdtGetMaxConstraintSize(void);
// Every Mdt source file holds its own copy (one static initialiser at the end of each Mac TU,
// e.g. Mac 0x48cd05 -> 0x78018c; MdtWorldCreate sizes the constraint pool with it).
static const int MdtConstraintMaxSize = MdtGetMaxConstraintSize();

// ---- MdtConstraint (MdtConstraint.cpp) --------------------------------------------------------
MdtConstraint* MEAPI MdtConstraintCreateFromPool(VinPool* pool);      // Mac 0x48dc64, Win 0x72de10
MdtConstraint* MEAPI MdtConstraintCreate(MdtWorld* w);                // Mac 0x48dee4, Win 0x72f380
void MEAPI MdtConstraintReset(MdtConstraint* c);                      // Mac 0x48e463, Win 0x72f850
void MEAPI MdtConstraintResetForces(MdtConstraint* c);                // Mac 0x48df0e, Win 0x72f3b0
void MEAPI MdtConstraintFreeToPool(MdtConstraint* c, VinPool* pool);  // Mac 0x48da72, Win 0x72f680
void MEAPI MdtConstraintDestroy(MdtConstraint* c);                    // Mac 0x48dab8, Win 0x72fa00
int MEAPI MdtConstraintIsEnabled(const MdtConstraint* c);             // Mac 0x48cfa2, Win 0x72e150
void MEAPI MdtConstraintEnable(MdtConstraint* c);                     // Mac 0x48db91, Win 0x72f6c0
void MEAPI MdtConstraintDisable(MdtConstraint* c);                    // Mac 0x48d9b4, Win 0x72e0b0
void MEAPI MdtConstraintWakeBodies(MdtConstraint* c);                 // Mac 0x48dad9, Win 0x72e030
void MEAPI MdtConstraintEnableBodies(MdtConstraint* c);               // Mac 0x48db4d, Win 0x72dff0
MdtConstraint* MEAPI MdtConstraintGetNext(const MdtConstraint* c);    // Mac 0x48d013, Win 0x72e4a0
MdtConstraint* MEAPI MdtConstraintGetNextEnabled(const MdtConstraint* c); // Mac 0x48d01e, Win 0x72e4b0
MdtBody* MEAPI MdtConstraintGetBody(const MdtConstraint* c, int i);   // Mac 0x48cfc1, Win 0x72e190
void MEAPI MdtConstraintSetBodies(MdtConstraint* c, MdtBody* b0, MdtBody* b1); // Mac 0x48d029, Win 0x72e4c0
void MEAPI MdtConstraintSetBodies2(MdtConstraint* c, MdtBody* b0, MdtBody* b1); // Mac 0x48d955, Win 0x72f650
void MEAPI MdtConstraintSetBody(MdtConstraint* c, MdtBody* b, int i);           // Mac 0x48d7dd, Win 0x72f520
void MEAPI MdtConstraintSetBodyAt(MdtConstraint* c, int i, MdtBody* b);         // Mac 0x48d993, Win 0x72f630
void MEAPI MdtConstraintSetUserData(MdtConstraint* c, void* data); /* the VxConstraint */   // Mac 0x48d04b, Win 0x72e4e0
int MEAPI MdtConstraintGetFlags(const MdtConstraint* c);              // Mac 0x48d3b6, Win 0x72f350
void MEAPI MdtConstraintSetFlags(MdtConstraint* c, int flags);        // Mac 0x48d3c4, Win 0x72f360
// constraint forces of the last step (Kea output)
void MEAPI MdtConstraintGetForce(const MdtConstraint* c, int i, MeVector3 f);  // Mac 0x48e050, Win 0x72e1d0
void MEAPI MdtConstraintGetTorque(const MdtConstraint* c, int i, MeVector3 t); // Mac 0x48cfdc, Win 0x72e200
// attachment frames (body i)
void MEAPI MdtConstraintSetAxisAll(MdtConstraint* c, MeReal x, MeReal y, MeReal z);  // Mac 0x48e739, Win 0x72df10
void MEAPI MdtConstraintSetBodyAxis(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z); // Mac 0x48f0f8, Win 0x72e6b0
void MEAPI MdtConstraintSetBodyAxes(MdtConstraint* c, int i, MeReal px, MeReal py, MeReal pz,
                              MeReal sx, MeReal sy, MeReal sz);  // Mac 0x48ee63, Win 0x72e770
void MEAPI MdtConstraintSetBodyAxisRel(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z); // Mac 0x48ec40, Win 0x72e8b0
void MEAPI MdtConstraintSetBodyAxesRel(MdtConstraint* c, int i, MeReal px, MeReal py, MeReal pz,
                                 MeReal sx, MeReal sy, MeReal sz); // Mac 0x48ea29, Win 0x72e980
void MEAPI MdtConstraintGetBodyAxes(const MdtConstraint* c, int i, MeVector3 primary, MeVector3 secondary); // Mac 0x48d607, Win 0x72e390
void MEAPI MdtConstraintGetBodyAxesRel(const MdtConstraint* c, int i, MeVector3 primary, MeVector3 secondary); // Mac 0x48e0b3, Win 0x72e410
void MEAPI MdtConstraintSetPosition(MdtConstraint* c, MeReal x, MeReal y, MeReal z);         // Mac 0x48d6e5, Win 0x72e5f0
void MEAPI MdtConstraintSetBodyPosition(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z); // Mac 0x48d679, Win 0x72e500
void MEAPI MdtConstraintGetBodyPosition(const MdtConstraint* c, int i, MeVector3 p);         // Mac 0x48d7a4, Win 0x72e270
void MEAPI MdtConstraintSetBodyPositionRel(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z); // Mac 0x48e07e, Win 0x72e5c0
void MEAPI MdtConstraintGetBodyPositionRel(const MdtConstraint* c, int i, MeVector3 p);      // Mac 0x48e113, Win 0x72e320
void MEAPI MdtConstraintSetBodyPositionCOM(MdtConstraint* c, int i, MeReal x, MeReal y, MeReal z); // Mac 0x48e358, Win 0x72e550
void MEAPI MdtConstraintGetBodyPositionCOM(const MdtConstraint* c, int i, MeVector3 p);      // Mac 0x48e14a, Win 0x72e2a0
// coordinates
int MEAPI MdtConstraintGetNumLimits(const MdtConstraint* c);          // Mac 0x48d05d, Win 0x72ebc0
MdtLimit* MEAPI MdtConstraintGetLimit(MdtConstraint* c, int i);       // Mac 0x48d06b, Win 0x72ebd0
void MEAPI MdtConstraintSetLimitPosition(MdtConstraint* c, int i, MeReal p); // Mac 0x48d097, Win 0x72ecf0
MeReal MEAPI MdtConstraintGetLimitPosition(MdtConstraint* c, int i);  // Mac 0x48d0ca
MeReal MEAPI MdtConstraintCalculatePosition(MdtConstraint* c, int i); // Mac 0x48d4cf, Win 0x72ec00
MeReal MEAPI MdtConstraintGetPosition(MdtConstraint* c, int i);       // Mac 0x48d59f, Win 0x72f7f0
// rows (relaxation, force limits)
void MEAPI MdtConstraintRelaxConstraint(MdtConstraint* c, int row, MeReal stiffness, MeReal damping,
                                  MeReal m);                    // Mac 0x48d43f, Win 0x72ed70 (string)
void MEAPI MdtConstraintSetRelaxConstraintStiffness(MdtConstraint* c, int row, MeReal k); // Mac 0x48d3d6, Win 0x72ee40 (string)
MeReal MEAPI MdtConstraintGetRelaxConstraintStiffness(const MdtConstraint* c, int row);   // Mac 0x48d154, Win 0x72ef50
void MEAPI MdtConstraintSetRelaxConstraintDamping(MdtConstraint* c, int row, MeReal d);   // Mac 0x48d0e8, Win 0x72eeb0
void MEAPI MdtConstraintEnableRelaxConstraint(MdtConstraint* c, int row);   // Mac 0x48d1b8, Win 0x72f000
void MEAPI MdtConstraintDisableRelaxConstraint(MdtConstraint* c, int row);  // Mac 0x48d1a2, Win 0x72efe0
int MEAPI MdtConstraintIsRelaxConstraintEnabled(const MdtConstraint* c, int row); // Mac 0x48d1ce, Win 0x72f020
void MEAPI MdtConstraintSetForceLimits(MdtConstraint* c, int row, MeReal lo, MeReal hi);   // Mac 0x48d102, Win 0x72eef0
void MEAPI MdtConstraintGetForceLimits(const MdtConstraint* c, int row, MeReal* lo, MeReal* hi); // Mac 0x48d129, Win 0x72ef20
int MEAPI MdtConstraintIsLocked(const MdtConstraint* c);              // Mac 0x48d1e0, Win 0x72f040
void MEAPI MdtConstraintUnlockConstraint(MdtConstraint* c);           // Mac 0x48d201, Win 0x72f0d0
void MEAPI MdtConstraintResetPartitionInfo(MdtConstraintPartitionInfo* p); // Mac 0x490993, Win 0x734de0

// Win-only (Mac dead-stripped, names inferred except LockConstraint)
int MEAPI MdtConstraintLockConstraint(MdtConstraint* c);                       // Win 0x72f080
void MEAPI MdtConstraintSetLimitCurrentPosition(MdtConstraint* c, int i, MeReal p); // Win 0x72eca0
MeReal MEAPI MdtConstraintGetRelaxConstraintDamping(const MdtConstraint* c, int row); // Win 0x72ef80
void MEAPI MdtConstraintSetRelaxConstraintLoss(MdtConstraint* c, int row, MeReal loss); // Win 0x72eed0
MeReal MEAPI MdtConstraintGetRelaxConstraintLoss(const MdtConstraint* c, int row);   // Win 0x72efa0
void MEAPI MdtConstraintGetRelaxConstraint(const MdtConstraint* c, int row, MeReal* stiffness,
                                           MeReal* damping, MeReal* loss);       // Win 0x72edf0
MeReal MEAPI MdtConstraintGetRowForce(const MdtConstraint* c, int row);         // Win 0x72f270
MeReal MEAPI MdtConstraintGetLimitForce(MdtConstraint* c, int i);               // Win 0x72f290
int MEAPI MdtConstraintIsRowAtLowerLimit(const MdtConstraint* c, int row);      // Win 0x72f2f0
int MEAPI MdtConstraintIsRowAtUpperLimit(const MdtConstraint* c, int row);      // Win 0x72f320
void MEAPI MdtConstraintSetAxis(MdtConstraint* c, MeReal x, MeReal y, MeReal z); // Win 0x72e680

// ---- MdtLimit (MdtLimit.cpp, Mac 0x48fc8c-0x490123, Win 0x783080-0x783870) --------------------
void MEAPI MdtSingleLimitReset(MdtSingleLimit* l);                    // Mac 0x48fc8c
void MEAPI MdtSingleLimitSetStop(MdtSingleLimit* l, MeReal stop);     // Mac 0x48fcbe
void MEAPI MdtSingleLimitSetStiffness(MdtSingleLimit* l, MeReal k);   // Mac 0x48fccf
void MEAPI MdtSingleLimitSetDamping(MdtSingleLimit* l, MeReal d);     // Mac 0x48fd3a
void MEAPI MdtLimitReset(MdtLimit* l);                                // Mac 0x48fd9b, Win 0x783870
int MEAPI MdtLimitIsActive(const MdtLimit* l);                        // Mac 0x48fd6e
MdtSingleLimit* MEAPI MdtLimitGetLowerLimit(MdtLimit* l);             // Mac 0x48fd83
MdtSingleLimit* MEAPI MdtLimitGetUpperLimit(MdtLimit* l);             // Mac 0x48fd8e
void MEAPI MdtLimitSetLimitsActive(MdtLimit* l, int active);          // Mac 0x48ff43
MeReal MEAPI MdtLimitGetPosition(const MdtLimit* l);                  // Mac 0x48febc, Win 0x783250
int MEAPI MdtLimitIsLocked(const MdtLimit* l);                        // Mac 0x48fed9
int MEAPI MdtLimitIsMotorized(const MdtLimit* l);                     // Mac 0x490100
MeReal MEAPI MdtLimitGetLockPosition(const MdtLimit* l);              // Mac 0x48feee
MeReal MEAPI MdtLimitGetLockMaximumForce(const MdtLimit* l);          // Mac 0x48ff07
MeReal MEAPI MdtLimitGetLockMinimumForce(const MdtLimit* l);          // Mac 0x48ff15
void MEAPI MdtLimitSetLockStiffness(MdtLimit* l, MeReal k);           // Mac 0x48ff23
void MEAPI MdtLimitSetLock(MdtLimit* l, MeReal position, MeReal f1, MeReal f2); // Mac 0x48ff5b
MeReal MEAPI MdtLimitGetDesiredVelocity(const MdtLimit* l);           // Mac 0x48fefc
void MEAPI MdtLimitSetDesiredVelocity(MdtLimit* l, MeReal v);         // Mac 0x48ffb4, Win 0x7835f0
MeReal MEAPI MdtLimitGetMotorMaxForce(const MdtLimit* l);             // Mac 0x48ff38, Win 0x783330
void MEAPI MdtLimitSetMotorMaxForces(MdtLimit* l, MeReal f1, MeReal f2); // Mac 0x48ffc6, Win 0x783600
void MEAPI MdtLimitSetMotor(MdtLimit* l, MeReal v, MeReal f1, MeReal f2); // Mac 0x48fff4
void MEAPI MdtLimitSetM18(MdtLimit* l, MeReal x);                     // Mac 0x48ffa2 (name unknown)
MeReal MEAPI MdtLimitGetM18(const MdtLimit* l);                       // Mac 0x490036 (name unknown)
void MEAPI MdtLimitSetMotorStiffnessDamping(MdtLimit* l, MeReal k, MeReal d); // Mac 0x490041, Win 0x783670
MeReal MEAPI MdtLimitGetMotorStiffness(const MdtLimit* l);            // Mac 0x4900a7
MeReal MEAPI MdtLimitGetMotorDamping(const MdtLimit* l);              // Mac 0x4900f5
int MEAPI MdtLimitGetMf0(const MdtLimit* l);                          // Mac 0x490115 (name unknown)

// Win-only (names inferred)
MeReal MEAPI MdtSingleLimitGetStop(const MdtSingleLimit* l);          // Win 0x7830b0
MeReal MEAPI MdtSingleLimitGetStiffness(const MdtSingleLimit* l);     // Win 0x7830c0
MeReal MEAPI MdtSingleLimitGetDamping(const MdtSingleLimit* l);       // Win 0x7830f0
MeReal MEAPI MdtSingleLimitGetRestitution(const MdtSingleLimit* l);   // Win 0x783100
void MEAPI MdtSingleLimitSetRestitution(MdtSingleLimit* l, MeReal r); // Win 0x7831b0
MeReal MEAPI MdtLimitGetM50(const MdtLimit* l);                       // Win 0x783290
MeReal MEAPI MdtLimitGetLockStiffness(const MdtLimit* l);             // Win 0x783320
int MEAPI MdtLimitIsAngular(const MdtLimit* l);                       // Win 0x7837d0
void MEAPI MdtLimitSetMf0(MdtLimit* l, int v);                        // Win 0x783820

// ---- joint types (one file each) -------------------------------------------------------------
MdtConstraint* MEAPI MdtBSJointCreate(VinPool* pool);                 // Mac 0x48cd73
void MEAPI MdtBSJointReset(MdtConstraint* c);                         // Mac 0x48cd47, Win 0x78db40
MdtConstraint* MEAPI MdtHingeCreate(VinPool* pool);                   // Mac 0x48fbab, Win 0x775110
void MEAPI MdtHingeReset(MdtConstraint* c);                           // Mac 0x48fb5e, Win 0x7750d0
MdtConstraint* MEAPI MdtConeLimitCreate(VinPool* pool);               // Mac 0x48cee8, Win 0x783ee0
void MEAPI MdtConeLimitReset(MdtConstraint* c);                       // Mac 0x48ce63, Win 0x783dc0
void MEAPI MdtConeLimitSetConeHalfAngle(MdtConstraint* c, MeReal a);  // Mac 0x48ce36, Win 0x783e10
MdtConstraint* MEAPI MdtCylindricalCreate(VinPool* pool);             // Mac 0x48f7d6, Win 0x783d00
void MEAPI MdtCylindricalReset(MdtConstraint* c);                     // Mac 0x48f771, Win 0x783c90
MdtConstraint* MEAPI MdtLinear2Create(VinPool* pool);                 // Mac 0x490191
void MEAPI MdtLinear2Reset(MdtConstraint* c);                         // Mac 0x490165, Win 0x775000
MdtConstraint* MEAPI MdtMotorCreate(VinPool* pool);                   // Mac 0x490284
void MEAPI MdtMotorReset(MdtConstraint* c);                           // Mac 0x49024a, Win 0x783b80
MdtConstraint* MEAPI MdtAngular1Position3Create(VinPool* pool);       // Mac 0x48ac9d, Win 0x78da50
void MEAPI MdtAngular1Position3Reset(MdtConstraint* c);               // Mac 0x48ac0d, Win 0x78d9b0
MdtConstraint* MEAPI MdtSpringCreate(VinPool* pool);                  // Mac 0x49291b
void MEAPI MdtSpringReset(MdtConstraint* c);                          // Mac 0x4928c4, Win 0x782230
void MEAPI MdtSpringSetNaturalLength(MdtConstraint* c, MeReal l);     // Mac 0x49288b, Win 0x782280
MeReal MEAPI MdtSpringGetNaturalLength(const MdtConstraint* c);       // Mac 0x4928a9, Win 0x7822a0
void MEAPI MdtSpringSetStiffness(MdtConstraint* c, MeReal k);        // Win 0x7822c0 (Win only, V4.3)
void MEAPI MdtSpringSetDamping(MdtConstraint* c, MeReal d);          // Win 0x782320 (Win only, V4.3)
MdtConstraint* MEAPI MdtRPROJointCreate(VinPool* pool);               // Mac 0x492368, Win 0x774f70
void MEAPI MdtRPROJointReset(MdtConstraint* c);                       // Mac 0x4922ac, Win 0x774920
void MEAPI MdtRPROJointGetQuaternion(const MdtConstraint* c, int which, MeVector4 q); // Mac 0x4920f9
void MEAPI MdtRPROJointSetQuaternion(MdtConstraint* c, MeReal q0, MeReal q1, MeReal q2, MeReal q3,
                               int which);                      // Mac 0x4923df
void MEAPI MdtRPROJointSetRelativeQuaternion(MdtConstraint* c, const MeVector4 q); // Mac 0x49213c
void MEAPI MdtRPROJointUpdateRelativeQuaternion(MdtConstraint* c);    // Mac 0x492436
void MEAPI MdtRPROJointSetAngularStrength(MdtConstraint* c, MeReal x, MeReal y, MeReal z); // Mac 0x49217a, Win 0x774cf0
void MEAPI MdtRPROJointSetLinearStrength(MdtConstraint* c, MeReal x, MeReal y, MeReal z);  // Mac 0x492213, Win 0x774d90
MeReal MEAPI MdtType10GetValue(const MdtConstraint* c);               // Mac 0x48f98b, Win 0x7ac240 (name unknown)
void MEAPI MdtType14SetValue(MdtConstraint* c, MeReal v);             // Mac 0x48f926, Win 0x852a90 (name unknown)
MeReal MEAPI MdtType14GetValue(const MdtConstraint* c);               // Mac 0x48f93b, Win 0x852ab0 (name unknown)
MeReal MEAPI MdtType16GetValue(const MdtConstraint* c, int i);        // Mac 0x48f8ca, Win 0x7ac5d0 (name unknown)
MeReal MEAPI MdtWinchGetLength(const MdtConstraint* c);               // Mac 0x4930bc, Win 0x852800
MeReal MEAPI MdtWinchSetLength(MdtConstraint* c, MeReal l);           // Mac 0x4930d2, Win 0x852790

// ---- MdtContact (MdtContact.cpp, Mac 0x48f407-0x48f771, Win 0x7845c0-0x7848f0) ---------------
MdtConstraint* MEAPI MdtContactCreate(MdtWorld* w);                   // Mac 0x48f61f
void MEAPI MdtContactReset(MdtConstraint* c);                         // Mac 0x48f4c6, Win 0x7845f0
void MEAPI MdtContactDestroy(MdtConstraint* c);                       // Mac 0x48f5f9, Win 0x7845c0
MeReal MEAPI MdtContactGetPenetration(const MdtConstraint* c);        // Mac 0x48f407, Win 0x784740
void MEAPI MdtContactSetPenetration(MdtConstraint* c, MeReal p);      // Mac 0x48f430, Win 0x784810
void MEAPI MdtContactSetM_b58(MdtConstraint* c, MeReal x);            // Mac 0x48f445, Win 0x784830 (name unknown)
void MEAPI MdtContactSetM_d04(MdtConstraint* c, int x);               // Mac 0x48f45a (name unknown)
Vx::VxContactMaterial* MEAPI MdtContactGetContactMaterial(MdtConstraint* c); // Mac 0x48f415, Win 0x784790
void MEAPI MdtContactSetContactMaterial(MdtConstraint* c, const Vx::VxContactMaterial* m); // Mac 0x48f48c, Win 0x784850
MdtConstraint* MEAPI MdtContactGetNext(const MdtConstraint* c);       // Mac 0x48f422, Win 0x7847a0
void MEAPI MdtContactSetWakeBodies(MdtConstraint* c, int wake);       // Mac 0x48f46b
void MEAPI MdtContactSetNormal(MdtConstraint* c, MeReal x, MeReal y, MeReal z);   // Mac 0x48f6d1, Win 0x7847e0
void MEAPI MdtContactSetPosition(MdtConstraint* c, MeReal x, MeReal y, MeReal z); // Mac 0x48f700, Win 0x7847b0

#endif
