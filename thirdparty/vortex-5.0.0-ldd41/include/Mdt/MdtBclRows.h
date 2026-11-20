// Vortex 5.0.0 build 800 "vortexldd41" - MdtBcl (basic constraint library) internals: the row
// view a builder works on, the contact material data it reads and the shared row helpers
// (restored header). Sources:
//   src/Mdt/MdtBcl.cpp       Mac TU 553 (0x494648-0x4a0e56): solver parameters, coordinate
//                            functions, MdtBclEndConstraint, every row builder (Win
//                            0x7360f0-0x73e300, 0x73dbe0-0x73dc50)
//   src/Mdt/MdtBclRowsUtil.cpp  Mac TU 554 (0x4a0e56-0x4a2e04): row / limit helpers (Win
//                            0x7921c0-0x7939ff)
//   src/Mdt/MdtBclWinch.cpp  Mac TU 555 head (0x4a2e04-0x4a391c): winch builder (vxcable gate)
// File names inferred (each Mac file has the MdtConstraint.h static initialiser). All names in
// this header are inferred except MdtBclAddContact (string).
#ifndef MDT_MDTBCLROWS_H
#define MDT_MDTBCLROWS_H

#include "Mdt/MdtTypes.h"
#include "Kea/MdtKeaRows.h"
#include "Me/MeCall.h"

// One Jacobian row of one body: linear part [0..2], angular part [3..5].
typedef MeReal MdtBclJacobianRow[6];

// Pointers to the first row of the constraint being built (Mac/Win 0x20 bytes on the builder's
// stack; Kea reads `jacobian` and `flags`).
struct MdtBclRows {
    MeReal* xi;                   // +0x00 = MdtKeaConstraintRows::xi + numRows
    MeReal* c;                    // +0x04
    MeReal* lo;                   // +0x08
    MeReal* hi;                   // +0x0c
    MeReal* epsilon;              // +0x10
    MeReal* gamma;                // +0x14
    MdtBclJacobianRow** jacobian; // +0x18 per body: 16 rows (the builder's zeroed stack buffer)
    int* flags;                   // +0x1c = MdtKeaConstraintRows::rowFlags + numRows
};

// Maximum rows of one constraint (MdtBclConstraint arrays) and of bodies.
enum { kMdtBclMaxRows = 16, kMdtBclMaxBodies = 6 };

// The VxContactMaterial members the contact builder reads (Mac material +0x18, Win +0x30; the
// class itself: Vx/VxContactMaterial.h).
// Axis index: 0 / 1 = the two linear friction directions, 2 = spin about the normal, 3 / 4 =
// rolling; friction model 6 = none.
struct MdtBclContactMaterialData {
    int m00;                      // +0x00 (material Mac +0x18 / Win +0x30)
    unsigned int flags;           // +0x04 bit 2: compliance (soft contact)
    MeReal restitution;           // +0x08
    MeReal restitutionThreshold;  // +0x10
    MeReal compliance;            // +0x18
    MeReal damping;               // +0x20
    MeReal adhesiveForce;         // +0x28
    int frictionModel[5];         // +0x30 0 = box (fixed force), 1 = scaled box (coupled), 6 = none
    MeReal boxFrictionForce[5];   // +0x44 / Win +0x48
    MeReal slip[5];               // +0x6c / +0x70
    MeReal slideVelocity[5];      // +0x94 / +0x98
    MeReal frictionCoefficient[5];// +0xbc / +0xc0
    MeReal scale[5];              // +0xe4 / +0xe8 per axis multiplier of both limits
    char m10c[2];                 // +0x10c / +0x110
    char sticky;                  // +0x10e / +0x112 keep the contact anchored (MdtContact m_cf0..)
};

enum { kMdtBclFrictionModelBox = 0, kMdtBclFrictionModelScaledBox = 1,
       kMdtBclFrictionModelNone = 6 };

// ---- MdtBcl.cpp (TU 553) ----------------------------------------------------------------------
// Mac 0x494a7c, Win 0x736970: default relaxation of the rows the builder left unrelaxed,
// clamping by the constraint's relaxation / force limits, velocity terms of animated bodies, then
// MdtKeaAddConstraint. Returns numRows (0 when the constraint is still queued in a later
// partition).
int MEAPI MdtBclEndConstraint(MdtKeaConstraints* rows, MdtBclConstraint* c, MdtBclRows* r,
                              int numRows, const MdtBclSolverParameters* p);
// Mac 0x494e98, Win 0x7372f0: contact position in the frames of the two reference transforms.
void MEAPI MdtBclContactUpdateLocalPositions(MdtBclConstraint* c);
// Mac 0x4954e6, Win 0x73d720: contact Jacobian (normal, the two friction directions, spin and
// rolling rows); dirs receives normal, d1, d2 (may be 0).
void MEAPI MdtBclContactJacobian(MdtBclConstraint* c, MdtBclJacobianRow** jac, MeReal* dirs);
// Mac 0x49617c, Win 0x7364a0: three angular rows fixing the relative orientation of the two
// attachment frames (quaternion error); returns 3.
int MEAPI MdtBclAddOrientationRows(MdtBclRows* r, MdtBclJacobianRow** jac, int row,
                                   const MeReal* const* vel, const int* useFrame,
                                   const MeReal* frameQ, const MeReal* const* qrot,
                                   const MeReal* qRef, const MeReal* wRef, MeReal* qOut);

// ---- MdtBclRowsUtil.cpp (TU 554) --------------------------------------------------------------
int MEAPI MdtBclContactMaterialGetFrictionModel(const MdtBclContactMaterialData* m, int axis); // Mac 0x4a0e56, Win 0x73e300
void MEAPI MdtBclAddLimitBounceRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row,
                                   MdtLimit* l);                         // Mac 0x4a0e6f, Win 0x7921c0
int MEAPI MdtBclLimitUpdateOvershoot(MdtLimit* l);                       // Mac 0x4a101f, Win 0x7926a0
MeReal MEAPI MdtBclLinearVelocity(const MdtBclConstraint* c, const MeReal* axis,
                                  const MeReal* arms);                   // Mac 0x4a10d9, Win 0x792820
MeReal MEAPI MdtBclAngularVelocity(const MdtBclConstraint* c, const MeReal* axis); // Mac 0x4a11f4, Win 0x792990
int MEAPI MdtBclLimitIsActive(MdtLimit* l);                              // Mac 0x4a12cd, Win 0x792a80
int MEAPI MdtBclAddAngularAxisRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row,
                                  const MeReal* axis, int unused);       // Mac 0x4a1368, Win 0x7934c0
void MEAPI MdtBclLinearLimitUpdate(const MdtBclConstraint* c, const MdtBclSolverParameters* p,
                                   MdtLimit* l, MeReal (*frames)[4][4], const MeReal* arms,
                                   const MeReal* d);                     // Mac 0x4a1423, Win 0x792920 (+ 0x792740)
MeReal MEAPI MdtBclUnwrapAngle(MeReal previous, const MeReal* a, const MeReal* x,
                               const MeReal* y);                         // Mac 0x4a15a3, Win 0x792d60
void MEAPI MdtBclAngleFromAxes(const MdtBclConstraint* c, MdtLimit* l, const MeReal* a,
                               const MeReal* x, const MeReal* y);        // Mac 0x4a1693, Win 0x792c90
void MEAPI MdtBclAngleFromFrames(const MdtBclConstraint* c, MdtLimit* l,
                                 MeReal (*frames)[4][4]);                // Mac 0x4a178c, Win 0x792b80
void MEAPI MdtBclFrictionRowBounds(const MdtBclConstraint* c, Vx::VxConstraintFriction* f,
                                   MdtBclRows* r, const MdtBclSolverParameters* p, int row); // Mac 0x4a18d9, Win 0x792e10
void MEAPI MdtBclAddLockedLimitRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row,
                                   const MdtLimit* l);                   // Mac 0x4a19a8, Win 0x7925e0
void MEAPI MdtBclAddSoftRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row,
                            MeReal invStiffness, MeReal damping, MeReal xi, MeReal lo,
                            MeReal hi);                                  // Mac 0x4a1abb, Win 0x792550
void MEAPI MdtBclAddMotorRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row,
                             const MdtLimit* l);                         // Mac 0x4a1b7b, Win 0x7924b0
int MEAPI MdtBclAddLimitRow(MdtBclRows* r, const MdtBclSolverParameters* p, int row,
                            MdtLimit* l);                                // Mac 0x4a1c5b, Win 0x792b10
int MEAPI MdtBclAddAngularRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row, const MeReal* a,
                              const MeReal* b, int setError);            // Mac 0x4a1ce1, Win 0x792ea0
int MEAPI MdtBclAddLinearRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row, const MeReal* d,
                             const MeReal* axis, const MeReal* arms, int body,
                             int setError);                              // Mac 0x4a1e41, Win 0x792f90
int MEAPI MdtBclAddPositionRows(MdtBclRows* r, MdtBclJacobianRow** jac, int row,
                                MeReal (*frames)[4][4], const MeReal* arms, int body,
                                int setError);                           // Mac 0x4a2082, Win 0x7936f0
int MEAPI MdtBclAddFrictionRows(MdtBclConstraint* c, MdtBclRows* r, MdtBclJacobianRow** jac,
                                int row, MeReal (*frames)[4][4], const MeReal* arms,
                                const MdtBclSolverParameters* p);        // Mac 0x4a23d4, Win 0x793540
void MEAPI MdtBclAngularLimitUpdate(const MdtBclConstraint* c, const MdtBclSolverParameters* p,
                                    MdtLimit* l, MeReal (*frames)[4][4]); // Mac 0x4a25ab, Win 0x792a00 (+ 0x792740)
int MEAPI MdtBclAddAngleRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row, int body,
                            const MeReal* x, const MeReal* y, const MeReal* arms,
                            const MeReal* d, int setError);              // Mac 0x4a26f6, Win 0x7932a0
int MEAPI MdtBclAddDistanceRow(MdtBclRows* r, MdtBclJacobianRow** jac, int row, const MeReal* d,
                               const MeReal* arms, MeReal length, int setError); // Mac 0x4a2b56, Win 0x7930f0

#endif
