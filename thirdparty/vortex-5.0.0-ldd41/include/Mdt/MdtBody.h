// Vortex 5.0.0 build 800 "vortexldd41" - MdtBody API (restored).
// Source: src/Mdt/MdtBody.cpp (Mac 0x48adcd-0x48cd47, Win 0x72ffb0-0x7321e0) and the
// frame helpers of src/Mdt/MdtConvert.cpp. Names follow the MathEngine Karma Mdt API where the
// function matches one; all are inferred unless a string names them ("MdtBodyMoveTo").
#ifndef MDT_MDTBODY_H
#define MDT_MDTBODY_H

#include "Mdt/MdtTypes.h"
#include "Me/MeCall.h"

// --- creation ---------------------------------------------------------------------------------
MdtBody* MEAPI MdtBodyCreateFromPool(VinPool* pool);                 // Mac 0x48c91f, Win 0x7321e0
void MEAPI MdtBodyReset(MdtBody* b);                                  // Mac 0x48c20f, Win 0x731fa0
void MEAPI MdtBodyFreeToPool(MdtBody* b, VinPool* pool);              // Mac 0x48b86b, Win 0x72ffb0
void MEAPI MdtBodyGetDefaultAutoDisableParams(MdtAutoDisableParams* p); // Mac 0x48adcd, Win 0x730020

// --- state ------------------------------------------------------------------------------------
MeReal* MEAPI MdtBodyGetTransformPtr(MdtBody* b);                // Mac 0x48ae1b, Win 0x730060
void MEAPI MdtBodySetTransform(MdtBody* b, const MeMatrix4 tm);       // Mac 0x48b83c, Win 0x730620
void MEAPI MdtBodySetCenterOfMassRelativeTransform(MdtBody* b, const MeMatrix4 tm); // Mac 0x48c9d7, Win 0x730bc0
void MEAPI MdtBodyGetQuaternion(const MdtBody* b, MeVector4 q);       // Mac 0x48ae31, Win 0x730110
void MEAPI MdtBodyGetLinearVelocity(const MdtBody* b, MeVector3 v);   // Mac 0x48ae6f, Win 0x730190
void MEAPI MdtBodyGetAngularVelocity(const MdtBody* b, MeVector3 w);  // Mac 0x48b8f1, Win 0x7301d0
MeReal* MEAPI MdtBodyGetLinearVelocityPtr(MdtBody* b);                // Mac 0x48aea0, Win 0x7301c0
MeReal* MEAPI MdtBodyGetAngularVelocityPtr(MdtBody* b);               // Mac 0x48aead, Win 0x730200
void MEAPI MdtBodySetLinearVelocity(MdtBody* b, MeReal x, MeReal y, MeReal z);  // Mac 0x48af0f, Win 0x730890
void MEAPI MdtBodySetAngularVelocity(MdtBody* b, MeReal x, MeReal y, MeReal z); // Mac 0x48af3e, Win 0x7308c0
void MEAPI MdtBodyGetLinearAcceleration(const MdtBody* b, MeVector3 a);  // Mac 0x48b8c0, Win 0x7302a0
void MEAPI MdtBodyGetAngularAcceleration(const MdtBody* b, MeVector3 a); // Mac 0x48b88f, Win 0x7302d0
void MEAPI MdtBodyGetVelocityAtPoint(const MdtBody* b, const MeVector3 p, MeVector3 v);     // Mac 0x48bdf9, Win 0x730210
void MEAPI MdtBodyGetAccelerationAtPoint(const MdtBody* b, const MeVector3 p, MeVector3 a); // Mac 0x48c083, Win 0x730300
void MEAPI MdtBodyResetVelocities(MdtBody* b);                        // Mac 0x48b922, Win 0x730eb0
void MEAPI MdtBodyMoveTo(MdtBody* b, const MeMatrix4 tm, MeReal h);   // Mac 0x48bb7b, Win 0x731c70 (string)

// --- mass properties --------------------------------------------------------------------------
void MEAPI MdtBodySetMass(MdtBody* b, MeReal mass);                   // Mac 0x48af97, Win 0x730a50
void MEAPI MdtBodySetInertiaTensor(MdtBody* b, const MeMatrix3 I);    // Mac 0x48b7af, Win 0x730a80
void MEAPI MdtBodySetSphericalInertia(MdtBody* b, MeReal i);          // Mac 0x48b6dc, Win 0x730b10
MeReal MEAPI MdtBodyGetLinearVelocityDamping(const MdtBody* b);       // Mac 0x48aeba, Win 0x730450
MeReal MEAPI MdtBodyGetAngularVelocityDamping(const MdtBody* b);      // Mac 0x48aec8, Win 0x730460
void MEAPI MdtBodySetLinearVelocityDamping(MdtBody* b, MeReal d);     // Mac 0x48af6d, Win 0x7308f0
void MEAPI MdtBodySetAngularVelocityDamping(MdtBody* b, MeReal d);    // Mac 0x48af82, Win 0x730910
void MEAPI MdtBodySetMaxTimeStep(MdtBody* b, MeReal h);               // Mac 0x48afd6, Win 0x730dc0

// --- forces -----------------------------------------------------------------------------------
// The Kea force of this step (applied by the next MdtWorldStep, then cleared).
void MEAPI MdtBodyAddForce(MdtBody* b, MeReal x, MeReal y, MeReal z);  // Mac 0x48b000, Win 0x731030
void MEAPI MdtBodyAddTorque(MdtBody* b, MeReal x, MeReal y, MeReal z); // Mac 0x48b047, Win 0x7311a0
void MEAPI MdtBodyAddForceAtPosition(MdtBody* b, MeReal fx, MeReal fy, MeReal fz,
                               MeReal px, MeReal py, MeReal pz); // Mac 0x48bfa5, Win 0x731070
// Impulses: kept in MdtBody::impulse and turned into force / h by the next step.
void MEAPI MdtBodyAddImpulse(MdtBody* b, MeReal x, MeReal y, MeReal z); // Mac 0x48b08e, Win 0x7311e0
void MEAPI MdtBodyAddImpulseAtPosition(MdtBody* b, MeReal ix, MeReal iy, MeReal iz,
                                 MeReal px, MeReal py, MeReal pz); // Mac 0x48bebd, Win 0x731230
void MEAPI MdtBodyResetForces(MdtBody* b);                            // Mac 0x48b105, Win 0x731bc0
void MEAPI MdtBodySaveAndClearForces(MdtBody* b);                     // Mac 0x48b992, Win 0x730e00

// --- enabling / freezing ----------------------------------------------------------------------
int MEAPI MdtBodyIsEnabled(const MdtBody* b);                         // Mac 0x48afeb, Win 0x730de0
int MEAPI MdtBodyIsDynamic(const MdtBody* b);                         // Mac 0x48b0f0, Win 0x731490
int MEAPI MdtBodyIsFrozen(const MdtBody* b);                          // Mac 0x48b0df, Win 0x731450
void MEAPI MdtBodyEnable(MdtBody* b);                                 // Mac 0x48b1e6, Win 0x731680
void MEAPI MdtBodyEnableNoCallback(MdtBody* b);                       // Mac 0x48b26a, Win 0x731700
void MEAPI MdtBodyWakeUp(MdtBody* b);                                 // Mac 0x48b2d1, Win 0x731770
void MEAPI MdtBodyDisable(MdtBody* b);                                // Mac 0x48b49f, Win 0x7314b0
void MEAPI MdtBodyDisableNoCallback(MdtBody* b);                      // Mac 0x48b3d9, Win 0x7315a0
void MEAPI MdtBodyFreeze(MdtBody* b);                                 // Mac 0x48b6a5, Win 0x731e90
void MEAPI MdtBodyUnfreeze(MdtBody* b);                               // Mac 0x48b65f, Win 0x731ed0
void MEAPI MdtBodySetAnimated(MdtBody* b);                            // Mac 0x48b614, Win 0x731f10
void MEAPI MdtBodySetDynamic(MdtBody* b);                             // Mac 0x48b5da, Win 0x731f60
void MEAPI MdtBodyUpdateConstraints(MdtBody* b);                      // Mac 0x48b57a, Win 0x7313e0
int MEAPI MdtBodyGetEnabledTime(const MdtBody* b);                    // Mac 0x48aed6, Win 0x730560
void MEAPI MdtBodySetEnabledTime(MdtBody* b, int t);                  // Mac 0x48aee4, Win 0x730570
int MEAPI MdtBodyIsMoving(const MdtBody* b);                          // Mac 0x48ba66, Win 0x7312e0
MeReal MEAPI MdtBodyGetAutoDisableVelocityThreshold(const MdtBody* b);            // Mac 0x48b393, Win 0x731ad0
MeReal MEAPI MdtBodyGetAutoDisableAngularVelocityThreshold(const MdtBody* b);     // Mac 0x48b3a1, Win 0x731ae0
MeReal MEAPI MdtBodyGetAutoDisableAccelerationThreshold(const MdtBody* b);        // Mac 0x48b3af, Win 0x731af0
MeReal MEAPI MdtBodyGetAutoDisableAngularAccelerationThreshold(const MdtBody* b); // Mac 0x48b3bd, Win 0x731b00
int MEAPI MdtBodyGetAutoDisableAliveWindow(const MdtBody* b);                     // Mac 0x48b3cb, Win 0x731b10

// --- misc -------------------------------------------------------------------------------------
void* MEAPI MdtBodyGetUserData(const MdtBody* b);                     // Mac 0x48ae23, Win 0x730090
void MEAPI MdtBodySetUserData(MdtBody* b, void* data);                // Mac 0x48aefe, Win 0x730600
void MEAPI MdtBodySetM548(MdtBody* b, MdtBodyContactFn fn, void* data); // Mac 0x48b35a, Win 0x731a60 (name unknown; V4.2: pointer types)
void MEAPI MdtBodySetGrouping(MdtBody* b, int grouping);              // Mac 0x48b374, Win 0x731aa0
int MEAPI MdtBodyGetGrouping(const MdtBody* b);                       // Mac 0x48b385

// --- frames (src/Mdt/MdtConvert.cpp, Mac 0x492a79-0x4930bc, Win 0x778920-0x778ca0) -----------
// Rotate v from body `from`'s COM frame to body `to`'s (null = world).
void MEAPI MdtConvertVector(const MdtBody* from, const MeVector3 v, const MdtBody* to, MeVector3 out);         // Mac 0x492a79, Win 0x778920
void MEAPI MdtConvertPositionVector(const MdtBody* from, const MeVector3 p, const MdtBody* to, MeVector3 out); // Mac 0x492be2, Win 0x778a10
void MEAPI MdtBodyUpdateTransformFromCOM(MdtBody* b);                 // Mac 0x492d77, Win 0x778ba0
void MEAPI MdtBodyUpdateCOMFromTransform(MdtBody* b);                 // Mac 0x492e34, Win 0x778be0

// partition bookkeeping (src/Mdt/MdtPartition.cpp)
void MEAPI MdtBodyResetPartitionInfo(MdtBodyPartitionInfo* p);        // Mac 0x4909bd, Win 0x734e10
void MEAPI MdtBodyResetPartitionInfoAll(MdtBody* b);                  // Mac 0x4909d9, Win 0x734e30

#endif
