// Vortex 5.0.0 build 800 "vortexldd41" - the MdtBcl (basic constraint library) entry points
// (restored: MdtBcl.cpp, MdtBclRowsUtil.cpp, MdtBclWinch.cpp = Mac TUs 553 /
// 554 / 555 head, Win 0x7360f0-0x73e300, 0x7921c0-0x7939ff, 0x75d480; internals: Mdt/MdtBclRows.h).
// Names follow Karma's MdtBclAdd<Type> ("MdtBclAddContact" is a string); the others are inferred.
#ifndef MDT_MDTBCL_H
#define MDT_MDTBCL_H

#include "Mdt/MdtTypes.h"
#include "Me/MeCall.h"

// Row builders (MdtConstraint::rowBuilder). Mac: reached through the non-lazy pointers
// 0x702654-0x7026ac; Win: the switch in MdtConstraintUnlockConstraint (0x72f0d0).
int MEAPI MdtBclAddBSJoint(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);    // type 0x00 Mac 0x496e2a, Win 0x736f60
int MEAPI MdtBclAddHinge(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);      // type 0x01 Mac 0x49a903, Win 0x737470
int MEAPI MdtBclAddType02(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x02 Mac 0x49a203, Win 0x7378e0
int MEAPI MdtBclAddType03(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x03 Mac 0x4a0587, Win 0x738930
int MEAPI MdtBclAddType04(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x04 Mac 0x49f824, Win 0x738e40
int MEAPI MdtBclAddContact(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);    // type 0x05 Mac 0x4971c7, Win 0x73dce0 (string)
int MEAPI MdtBclAddType06(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x06 Mac 0x49f28a, Win 0x7395a0
int MEAPI MdtBclAddRPROJoint(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);  // type 0x07 Mac 0x49b20c, Win 0x73b410
int MEAPI MdtBclAddType08(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x08 Mac 0x49ecc3, Win 0x739790
int MEAPI MdtBclAddType0a(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x0a Mac 0x49e3de, Win 0x73a060
int MEAPI MdtBclAddLinear2(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);    // type 0x0b Mac 0x49dfac, Win 0x73a1d0
int MEAPI MdtBclAddType0c(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x0c Mac 0x49db40, Win 0x73a360
int MEAPI MdtBclAddSpring(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x0e Mac 0x49d3fa, Win 0x73a840
int MEAPI MdtBclAddConeLimit(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);  // type 0x0f Mac 0x49cedf, Win 0x73aba0
int MEAPI MdtBclAddType10(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x10 Mac 0x49c852, Win 0x73ada0
int MEAPI MdtBclAddType11(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x11 Mac 0x49c323, Win 0x73b1e0
int MEAPI MdtBclAddWinch(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);      // type 0x12 Mac 0x4a2edf, Win 0x75d4c0 (vxcable gate)
int MEAPI MdtBclAddCylindrical(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p); // type 0x13 Mac 0x499664, Win 0x737c20
int MEAPI MdtBclAddType14(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x14 Mac 0x495d1b, Win 0x738780
int MEAPI MdtBclAddType15(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);     // type 0x15 Mac 0x4982f5, Win 0x73bd20 (vxadvcon gate)
int MEAPI MdtBclAddAngular1Position3(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p); // type 0x18 Mac 0x498bb9, Win 0x7381f0
int MEAPI MdtBclAddMotor(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);      // type 0x1a Mac 0x49e7c2, Win 0x739e10
// MdtConstraintLockConstraint (Win-only 0x72f080) switches any constraint to this builder.
int MEAPI MdtBclAddLock(MdtKeaConstraints* rows, MdtBclConstraint* c, const MdtBclSolverParameters* p);       // Mac 0x497e8e, Win 0x7370b0

// Current coordinate value of a limit, by MdtLimit::kind (MdtConstraintCalculatePosition).
MeReal MEAPI MdtBclGetLinearCoordinate(MdtBclConstraint* c, MdtLimit* l);   // kind 0, Mac 0x495321, Win 0x736c10
MeReal MEAPI MdtBclGetAngularCoordinate(MdtBclConstraint* c, MdtLimit* l);  // kind 1, Mac 0x495068, Win 0x736e30
MeReal MEAPI MdtBclGetDistanceCoordinate(MdtBclConstraint* c, MdtLimit* l); // kind 2, Mac 0x4948aa, Win 0x736f00
// Mac 0x4a3770 (Win 0x75d480 behind the vxcable gate): refreshes the winch state before
// MdtWinchSetLength changes it.
void MEAPI MdtBclWinchUpdate(MdtBclConstraint* c);

#endif
