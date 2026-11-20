// Vortex 5.0.0 build 800 "vortexldd41" - Mdt (dynamics) data structures
// (restored header offsets; full structs).
//
// Mdt is C-style code compiled as C++ (Mac TUs 530 tail-552, one static initialiser per file;
// Win __FILE__ "c:\work\martinc\vortexldd41\vortex\mdt\src\mdtworld.cpp", Mac
// "Mdt/src/MdtWorld.cpp"). The structs below are declared with typed members so both compilers
// reproduce the two layouts: Mac i386 (doubles 4-aligned) and Win MSVC (doubles 8-aligned).
// Offsets in comments: "+0xMAC / Win +0xWIN" (Win only given where it differs).
// Checked by static asserts at the end (Mac with an i386 compiler, Win with the zig/MSVC build)
// and by the restoration's tests. Field names are inferred (Karma/MathEngine conventions) unless a
// string names them.
#ifndef MDT_MDTTYPES_H
#define MDT_MDTTYPES_H

#include <stddef.h>
#include "Me/MeCall.h"
#include "Me/MeMath.h"
#include "Me/VinPool.h"
#include "Kea/MdtKea.h"
#include "Vx/VxPlatform.h"
#include "Vx/VxConstraintFriction.h"
#include "Vx/VxContactMaterial.h"

struct MdtWorld;
struct MdtBody;
struct MdtConstraint;
struct MdtContact;
typedef MdtWorld* MdtWorldID;
typedef MdtBody* MdtBodyID;
typedef MdtConstraint* MdtConstraintID;
typedef MdtConstraint* MdtContactID; // contacts are constraints of type kMdtTypeContact

// MdtConstraint::bcl.type, from the Mdt*Create functions, the Win row-builder switch 0x72f0d0
// (types 0-4, 6-8, 0xa-0xc, 0xe, 0x10-0x15) and the creating Vx classes (names inferred).
enum MdtConstraintType {
    kMdtTypeBallAndSocket = 0x00,   // MdtBSJointCreate Mac 0x48cd73 (VxBallAndSocket)
    kMdtTypeHinge = 0x01,           // MdtHingeCreate Mac 0x48fbab (VxHinge)
    kMdtTypeContact = 0x05,         // MdtContactCreate Mac 0x48f61f
    kMdtTypeRPRO = 0x07,            // MdtRPROJointCreate Mac 0x492368 (VxRPRO)
    kMdtTypeLinear2 = 0x0b,         // MdtLinear2Create Mac 0x490191 (VxLinear2)
    kMdtTypeSpring = 0x0e,          // MdtSpringCreate Mac 0x49291b (VxSpring)
    kMdtTypeConeLimit = 0x0f,       // MdtConeLimitCreate Mac 0x48cee8 (VxConeConstraint)
    kMdtType10 = 0x10,              // Win-only creator 0x7ac3c0 (Mac getter 0x48f98b)
    kMdtTypeWinch = 0x12,           // VxWinch (vxcable license gate), Win row builder 0x75d4c0, creator 0x8529b0
    kMdtTypeCylindrical = 0x13,     // MdtCylindricalCreate Mac 0x48f7d6 (VxCylindrical)
    kMdtType14 = 0x14,              // Win-only creator 0x852ac0 (Mac 0x48f926 / 0x48f93b)
    kMdtTypeAdvanced15 = 0x15,      // vxadvcon license gate, Win row builder 0x73bd20
    kMdtTypeAdvanced16 = 0x16,      // vxadvcon license gate, Win 0x73c920 (creator Win 0x7ac700)
    kMdtTypeAngular1Position3 = 0x18, // MdtAngular1Position3Create Mac 0x48ac9d (VxAngular1Position3)
    kMdtTypeMotor = 0x1a            // MdtMotorCreate Mac 0x490284 (VxMotorConstraint)
};

// ===============================================================================================
// MdtBody (Mac 0x5a4 / Win 0x5b8 bytes = world body pool element)
// ===============================================================================================
// Auto-disable thresholds (defaults: Mac data 0x7794e0, Win 0xabc2f0; Mac 0x24 / Win 0x28).
struct MdtAutoDisableParams {
    MeReal velocityThreshold;            // +0x00 0.02 (compared with |v|^2)
    MeReal angularVelocityThreshold;     // +0x08 0.001
    MeReal accelerationThreshold;        // +0x10 0.002
    MeReal angularAccelerationThreshold; // +0x18 0.002
    int aliveWindow;                     // +0x20 10 steps
};

// Partition bookkeeping of a body (Mac 0x10 bytes; MdtBodyResetPartitionInfo Mac 0x4909bd).
struct MdtBodyPartitionInfo {
    int state;          // +0x0 0 = untouched, 1 = in the current partition, 2 = finished
    int depth;          // +0x4 BFS level the body was reached at
    int m8;             // +0x8 -1
    int lastPartition;  // +0xc level partition that last added this body (MdtPartitionWorld)
};

// contact callback of a body (VxPart's handler): the body and the contact (name inferred)
typedef void (*MdtBodyContactFn)(MdtBody* body, MdtConstraint* contact);

struct MdtConstraintNode; // body's list of constraints (world cNodePool, 0xc bytes)

struct MdtBody {
    MeMatrix4 transform;          // +0x000 user transform (MdtBodySetTransform)
    MeMatrix4 relativeTransform;  // +0x080 body frame relative to the centre of mass frame
    MeReal impulse[4];            // +0x100 MdtBodyAddImpulse: applied as impulse / h by the
    MeReal angularImpulse[4];     // +0x120   next MdtWorldStep (MdtKeaAddPartition)
    MdtKeaBody keaBody;           // +0x140 (Mac 0x3ac / Win 0x3b0 bytes)
    int useRelativeTransform;     // +0x4ec / Win +0x4f0
    int impulseAdded;             // +0x4f0 / +0x4f4 impulse[] / angularImpulse[] are non-zero
    MdtWorld* world;              // +0x4f4 / +0x4f8
    MdtBody* next;                // +0x4f8 / +0x4fc world body list
    MdtBody** prevNext;           // +0x4fc / +0x500
    MdtBody* nextEnabled;         // +0x500 / +0x504 world enabled-body list
    MdtBody** prevNextEnabled;    // +0x504 / +0x508
    void* userData;               // +0x508 / +0x50c (MdtBodySetUserData Mac 0x48aefe)
    const char* name;             // +0x50c / +0x510 (VxPart::setName Mac 0x4680ee)
    int partition;                // +0x510 / +0x514 partition of the last step (-1)
    int m514;                     // +0x514 / +0x518
    MeReal mass;                  // +0x518 / +0x520
    MdtAutoDisableParams autoDisable; // +0x520 / +0x528
    MdtConstraintNode* constraints;   // +0x544 / +0x550
    MdtBodyContactFn m548;        // +0x548 / +0x554 contact callback (Mac 0x48b35a sets both; VxPart
                                  //   Mac 0x4681ca)
    void* m54c;                   // +0x54c / +0x558 its data (VxPart::ContactHandler)
    int enabledTime;              // +0x550 / +0x55c steps since enabled; -1 = disabled
    int frozen;                   // +0x554 / +0x560 bit 0 frozen, bit 1 disabled (kinematic)
    MeReal maxTimeStep;           // +0x558 / +0x568 (DBL_MAX)
    MeReal lastForce[3];          // +0x560 / +0x570 Kea force of the last step
    MeReal lastTorque[3];         // +0x578 / +0x588
    MdtBodyPartitionInfo partitionInfo; // +0x590 / +0x5a0
    MdtBody* self;                // +0x5a0 / +0x5b0 marks a body as a partition root candidate
};

struct MdtConstraintNode {
    MdtConstraint* constraint;    // +0x0
    MdtConstraintNode* next;      // +0x4
    MdtConstraintNode** prevNext; // +0x8
};

// ===============================================================================================
// MdtConstraint
// ===============================================================================================
// Limit of one side of a coordinate (Mac 0x30 bytes; MdtSingleLimitReset Mac 0x48fc8c).
struct MdtSingleLimit {
    MeReal stop;         // +0x00
    MeReal invStiffness; // +0x08 (MdtSingleLimitSetStiffness Mac 0x48fccf stores 1 / |k|)
    MeReal damping;      // +0x10 (Mac 0x48fd3a stores |d|)
    MeReal restitution;  // +0x18
    MeReal m20;          // +0x20
    MeReal m28;          // +0x28 (DBL_MAX)
};

// One controllable coordinate (limits, motor, lock) (0xf8 bytes on both; MdtLimitReset Mac
// 0x48fd9b, Win 0x783870). MdtConstraintGetLimit(c, i) = &limit[i] after the base struct.
enum MdtLimitControl { kMdtLimitFree = 0, kMdtLimitMotorized = 1, kMdtLimitLocked = 2 };
enum MdtLimitKind { kMdtLimitLinear = 0, kMdtLimitAngular = 1, kMdtLimitDistance = 2 };
struct MdtLimit {
    MeReal m00;              // +0x00
    MeReal position;         // +0x08 current position (MdtLimitSetPosition writes +0x08 and +0x40)
    MeReal desiredVelocity;  // +0x10 motor
    MeReal m18;              // +0x18 (Mac 0x48ffa2 / 0x490036)
    MeReal motorInvStiffness;// +0x20 (Mac 0x490041 stores 1 / k)
    MeReal motorDamping;     // +0x28
    MeReal motorMaxForceHi;  // +0x30 (DBL_MAX)
    MeReal motorMaxForceLo;  // +0x38 (-DBL_MAX)
    MeReal previousPosition; // +0x40
    MeReal positionOffset;   // +0x48 added to the measured position
    MeReal m50;              // +0x50
    MdtSingleLimit lower;    // +0x58 stop -DBL_MAX
    MdtSingleLimit upper;    // +0x88 stop DBL_MAX
    MeReal lockPosition;     // +0xb8
    MeReal lockForceHi;      // +0xc0 (DBL_MAX)
    MeReal lockForceLo;      // +0xc8 (-DBL_MAX)
    MeReal lockStiffness;    // +0xd0
    int control;             // +0xd8 MdtLimitControl
    int limitsActive;        // +0xdc
    int kind;                // +0xe0 MdtLimitKind (selects the Bcl position function)
    int index;               // +0xe4 axis index
    int me8;                 // +0xe8 (VxConstraint slot 15 Mac 0x43d27e)
    int mec;                 // +0xec
    int mf0;                 // +0xf0 (Mac 0x490115)
    int mf4;                 // +0xf4
};

// Partition bookkeeping of a constraint (Mac 0x14 bytes; Mac 0x490993).
struct MdtConstraintPartitionInfo {
    int state;      // +0x00 0 / 1 = in a partition / 2 = deferred to a later level
    int partitionA; // +0x04 -1
    int partitionB; // +0x08 -1
    int sortKey;    // +0x0c 0x7fffffff (partition level the constraint was sorted into)
    int minKey;     // +0x10
};

typedef void (MEAPI* MdtConstraintSetBodiesFn)(MdtConstraint* c, MdtBody* b0, MdtBody* b1);
typedef void (MEAPI* MdtConstraintSetAxisFn)(MdtConstraint* c, MeReal x, MeReal y, MeReal z);

// The part of a constraint Kea and the row builders see (MdtConstraint + 0x1f4 / Win + 0x1f8;
// the row builders get a pointer to `type`). Mac 0x8f4 / Win 0x918 bytes.
struct MdtBclConstraint {
    int type;                     // +0x000 MdtConstraintType (-1 when freed)
    int numBodies;                // +0x004 (2)
    int maxRows;                  // +0x008
    MeReal frame[6][4][4];        // +0x00c / Win +0x010: per body: axis 1, 2, 3, position
                                  //   (body frame; row 3 = attachment position)
    MdtKeaBody* keaBody[6];       // +0x30c / +0x310 (null if the body is disabled)
    const MeMatrix4* comTM[6];    // +0x324 / +0x328 -> body keaBody.comTM
    const MeReal* vel[6];         // +0x33c / +0x340 -> body keaBody.vel (null if frozen)
    const MeReal* qrot[6];        // +0x354 / +0x358 -> body keaBody.qrot
    int relaxActive[16];          // +0x36c / +0x370 per row
    char frictionDirty;           // +0x3ac / +0x3b0
    Vx::VxConstraintFriction friction[6]; // +0x3b0 / +0x3b8 (Mac 0x34 / Win 0x38 each)
    MeReal relaxInvStiffness[16]; // +0x4e8 / +0x508
    MeReal relaxDamping[16];      // +0x568 / +0x588
    MeReal relaxM5e8[16];         // +0x5e8 / +0x608 (third MdtConstraintRelaxConstraint value)
    MeReal forceLo[16];           // +0x668 / +0x688 (-DBL_MAX)
    MeReal forceHi[16];           // +0x6e8 / +0x708 (DBL_MAX)
    int m768[16];                 // +0x768 / +0x788
    int rowStatus[16];            // +0x7a8 / +0x7c8 (Kea output)
    int partitionCount;           // +0x7e8 / +0x808 times added to a partition this step
    MeReal lambda[17];            // +0x7ec / +0x810 (Kea output); [16] at +0x86c / +0x890 is the
                                  // original's m86c (4294967295.0), where MdtKeaGetConstraintForces
                                  // writes its end marker after a 16-row constraint: one array, so
                                  // that store stays in bounds (same layout)
    MeReal lambda2[16];           // +0x874 / +0x898 (Kea output)
};

struct MdtConstraint {
    MdtWorld* world;              // +0x000 (MdtConstraintCreate(world) only)
    MdtConstraint* next;          // +0x004 world enabled-constraint list
    MdtConstraint** prevNext;     // +0x008
    MdtBody* body[6];             // +0x00c
    MdtConstraintNode* node[6];   // +0x024
    MeReal force[6][4];           // +0x03c / Win +0x040 Kea output: force on body i
    MeReal torque[6][4];          // +0x0fc / +0x100
    int visited;                  // +0x1bc / +0x1c0 traversal mark (MdtBodySetCOMRelativeTransform)
    int ignored;                  // +0x1c0 / +0x1c4 not partitioned when set
    MdtBclAddConstraintFn rowBuilder;   // +0x1c4 / +0x1c8 (MdtBcl, switched to the lock builder
                                        //   by MdtConstraintLockConstraint, Win 0x72f080)
    MdtConstraintSetBodiesFn setBodies; // +0x1c8 / +0x1cc (default MdtConstraintSetBodies2)
    MdtConstraintSetAxisFn setAxis;     // +0x1cc / +0x1d0 (default MdtConstraintSetAxisAll)
    int numLimits;                // +0x1d0 / +0x1d4
    void* userData;               // +0x1d4 / +0x1d8 (Mac 0x48d04b; the VxConstraint)
    const char* name;             // +0x1d8 / +0x1dc (VxConstraint::setName)
    MdtConstraintPartitionInfo partitionInfo; // +0x1dc / +0x1e0
    int flags;                    // +0x1f0 / +0x1f4 bit 0: wake the bodies (MdtPartition)
    MdtBclConstraint bcl;         // +0x1f4 / +0x1f8
};                                // Mac 0xae8 / Win 0xb10

// ---- constraint types (each starts with MdtConstraint) -------------------------------------
struct MdtBSJoint { MdtConstraint head; int m_ae8; };                      // 0xaec / 0xb18
struct MdtHinge { MdtConstraint head; MdtLimit limit; int m_be0; };        // 0xbe4 / 0xc10
struct MdtCylindrical { MdtConstraint head; MdtLimit limit[2]; };         // 0xcd8 / 0xd00
struct MdtAngular1Position3 { MdtConstraint head; MdtLimit limit[4]; };   // 0xec8 / 0xef0
struct MdtLinear2 { MdtConstraint head; };                                 // 0xae8 / 0xb10
struct MdtMotor { MdtConstraint head; MdtLimit limit; };                  // 0xbe0 / 0xc08
struct MdtSpring { MdtConstraint head; MdtLimit limit; };                 // 0xbe0 / 0xc08
struct MdtType14 { MdtConstraint head; MdtLimit limit; MeReal m_be0; };   // 0xbe8 / 0xc10
struct MdtConeLimit {
    MdtConstraint head;
    MeReal halfAngle;      // +0xae8 (pi)
    MeReal cosHalfAngle;   // +0xaf0 (-1)
    MeReal invStiffness;   // +0xaf8 (0)
    MeReal damping;        // +0xb00 (DBL_MAX)
};                                                                         // 0xb08 / 0xb30
struct MdtRPROJoint {
    MdtConstraint head;
    MeReal q1[4];          // +0xae8 orientation of body 0's frame (1,0,0,0)
    MeReal q2[4];          // +0xb08 orientation of body 1's frame
    int q1Set;             // +0xb28
    int q2Set;             // +0xb2c
    MeReal m_b30[3];       // +0xb30
    MeReal relQ[4];        // +0xb48 relative orientation (MdtRPROJointUpdateRelativeQuaternion)
    MeReal m_b68[3];       // +0xb68
    int m_b80;             // +0xb80
    int m_b84;             // +0xb84
};                                                                         // 0xb88 / 0xbb0
struct MdtContact {
    MdtConstraint head;
    MeReal position[3];    // +0xae8 / Win +0xb10
    MeReal m_b00[3];       // +0xb00 / +0xb28
    MeReal m_b18[3];       // +0xb18 / +0xb40
    MeReal normal[3];      // +0xb30 / +0xb58 (0,1,0); MdtContact's setAxis (Mac 0x48f6d1)
    const MeReal* m_b48;   // +0xb48 / +0xb70 reference transform of m_b00 (MeMatrix4)
    const MeReal* m_b4c;   // +0xb4c / +0xb74 reference transform of m_b18
    MeReal penetration;    // +0xb50 / +0xb78
    MeReal m_b58;          // +0xb58 / +0xb80 (1)
    MeReal direction[3];   // +0xb60 / +0xb88 primary friction direction (1,0,0)
    Vx::VxContactMaterial material; // +0xb78 / +0xba0
    const char* materialName;       // +0xcec / +0xd38 (material.getName())
    MeReal m_cf0;          // +0xcf0 / +0xd40
    MeReal m_cf8;          // +0xcf8 / +0xd48
    int m_d00;             // +0xd00 / +0xd50
    int m_d04;             // +0xd04 / +0xd54 (-1) (Mac 0x48f45a)
    MdtBody* m_d08;        // +0xd08 / +0xd58 model 1's body (VxResponseBody_Body)
    MdtBody* m_d0c;        // +0xd0c / +0xd5c model 2's body
    MdtContact* next;      // +0xd10 / +0xd60 contact list of a contact group
};                                                                         // 0xd14 / 0xd68
// Types with only accessors in the binaries (partial; their sizes come from the size table).
struct MdtType10 { MdtConstraint head; MeReal value; };       // Mac 0x48f98b, Win 0x7ac240
// The same constraint as VxGearRatio's setters see it (Mac 0x4574e2 / 0x45751c; Win setters
// 0x7511d0 ratio, 0x751040 mean lifetime, 0x750ff0 backlash, 0x751110 flag, 0x751190 reference
// positions; names from their messages, the rest inferred). PORT64: VxGearRatio writes through
// these fields instead of byte offsets.
struct MdtGearRatio {
    MdtConstraint head;
    MeReal value;                  // +0xae8 / Win +0xb10 (MdtType10GetValue)
    MeReal m08;                    // +0xaf0 / +0xb18
    MeReal m10;                    // +0xaf8 / +0xb20
    MeReal ratio;                  // +0xb00 / +0xb28 (setRatio)
    MeReal positionOffsetMeanLifetime; // +0xb08 / +0xb30
    MeReal backlash;               // +0xb10 / +0xb38
    MeReal m30;                    // +0xb18 / +0xb40
    MeReal m38;                    // +0xb20 / +0xb48
    MeReal m40;                    // +0xb28 / +0xb50
    int m48;                       // +0xb30 / +0xb58 (bool; Win 0x751110)
    MeReal partReferencePosition[2]; // +0xb34 / +0xb60
};
struct MdtType16 { MdtConstraint head; MeReal value[3]; };    // Mac 0x48f8ca, Win 0x7ac5d0
struct MdtWinch {
    MdtConstraint head;
    MeReal ratio;          // +0xae8 / Win +0xb10
    MeReal m_af0;          // +0xaf0 / +0xb18
    MeReal m_af8;          // +0xaf8 / +0xb20
    MeReal m_b00;          // +0xb00 / +0xb28
    int m_b08;             // +0xb08 / +0xb30
    MeReal offset;         // +0xb0c / +0xb38
    MeReal m_b14;          // +0xb14 / +0xb40 = length / ratio - offset
    MeReal m_b1c;          // +0xb1c / +0xb48
    MeReal length;         // +0xb24 / +0xb50
    // (from the winch row builder Mac 0x4a2edf; names inferred)
    MeReal minLength;      // +0xb2c / +0xb58
    MeReal maxLength;      // +0xb34 / +0xb60
    MeReal m_b3c;          // +0xb3c / +0xb68 (scales the drum's turning into length)
    int part;              // +0xb44 / +0xb70 0 = linear drum coordinate (Jacobian part 0), 3 = angular
    int frame;             // +0xb48 / +0xb74 frame of the drum's counter body
};

// ===============================================================================================
// MdtWorld (Mac 0x324 / Win 0x338)
// ===============================================================================================
// Bcl solver parameters (world + 0x22c / Win + 0x240, also in every grouping; 0x88 bytes).
// Defaults Mac 0x494311 / Win 0x732370: MdtBclSolverParametersSet(p, 1e-10, 1e-10, 1/60f, 5).
struct MdtBclSolverParameters {
    MeReal stepSize;    // +0x00
    MeReal epsilon;     // +0x08
    MeReal epsilon2;    // +0x10
    MeReal gamma;       // +0x18 = h * k / epsilon
    MeReal gamma2;      // +0x20 = h * k / epsilon2
    MeReal m28;         // +0x28 = epsilon / h
    MeReal m30;         // +0x30 = epsilon2 / h
    MeReal invH;        // +0x38
    MeReal invH2;       // +0x40
    MeReal m48;         // +0x48
    MeReal m50;         // +0x50
    MeReal m58;         // +0x58
    MeReal m60;         // +0x60
    MeReal m68;         // +0x68
    MeReal m70;         // +0x70
    MeReal m78;         // +0x78
    MeReal m80;         // +0x80
};

// The partitioner's output (world + 0x10c, 0x7c bytes on both). "Partitions" are the
// independent islands of bodies connected by enabled constraints; each gets one Kea work slot.
struct MdtPartitionOutput {
    MdtConstraint** constraints;      // +0x00 all partitioned constraints, by partition
    MdtConstraint** deferred;         // +0x04 constraints that reach a later level
    int numDeferred;                  // +0x08
    int* constraintsStart;            // +0x0c per partition
    int* constraintsSize;             // +0x10 per partition
    int* numRows;                     // +0x14 per partition: sum of the constraints' maxRows
    int* order;                       // +0x18 per work slot: partition (MdtWorldStep)
    int* parent;                      // +0x1c per partition: level/union-find link (0x7fffffff)
    int* numDeferredOf;               // +0x20 per partition
    MdtBody** bodies;                 // +0x24 all partitioned bodies, by partition
    MdtKeaBody** keaBodies;           // +0x28
    MeIntMapEntry* bodyMap;           // +0x2c (keaBody, index) pairs for MeIntMapSort
    int* bodiesStart;                 // +0x30 per partition
    int* finished;                    // +0x34 per partition
    int* bodiesSize;                  // +0x38 per partition
    int* awake;                       // +0x3c per partition: a frozen neighbour is still moving
    int* maxSortKey;                  // +0x40 per partition
    int numPartitions;                // +0x44
    int maxPartitions;                // +0x48
    int numBodies;                    // +0x4c
    int numConstraints;               // +0x50
    int maxBodies;                    // +0x54
    MdtBody** touchedBodies;          // +0x58 bodies whose partition info must be reset
    int numTouchedBodies;             // +0x5c
    MdtConstraint** touchedConstraints; // +0x60
    int numTouchedConstraints;        // +0x64
    int maxConstraints;               // +0x68
    MdtBody** roots;                  // +0x6c enabled bodies that are their own root
    int numRoots;                     // +0x70
    MdtBody** stack;                  // +0x74 BFS work list
    MeMemoryAPIStruct* api;           // +0x78
};

struct MdtPartitionParams;
// Mac 0x491051 (MdtPartitionWorld), called through MdtPartitionParams::partitioner
typedef void (MEAPI* MdtPartitionFn)(MdtBody* enabledBodies, MdtPartitionOutput* out,
                               MdtPartitionParams* params);
// Mac 0x49421e (MdtAutoDisableLastPartition), called for every closed partition
typedef void (MEAPI* MdtPartitionCallbackFn)(MdtPartitionOutput* out);

struct MdtPartitionParams {      // world + 0x2b4 / Win + 0x2c8 (Mac 0x490695 sets the defaults)
    int optimize;                 // +0x00 0: every constraint joins the partition (sort off)
    int autoDisable;              // +0x04 1
    int maxConstraintsPerLevel;   // +0x08 10
    MdtPartitionFn partitioner;   // +0x0c MdtPartitionWorld
    MdtPartitionCallbackFn partitionClosed; // +0x10 MdtAutoDisableLastPartition when autoDisable
    int defaultMaxDepth;          // +0x14 5
};

// Groupings: per-grouping solver settings (world "Groupings Pool", Mac 0x11c / Win 0x130).
struct MdtGrouping {
    MdtWorld* world;              // +0x00
    int index;                    // +0x04
    int id;                       // +0x08
    int m0c;                      // +0x0c -1
    int maxDepth;                 // +0x10 -1 = MdtPartitionParams::defaultMaxDepth
    int maxConstraints;           // +0x14 -1
    MdtKeaParameters keaParams;   // +0x18 copy of the world's
    MdtBclSolverParameters bclParams; // +0x94 / Win +0xa8
};

struct MdtWorkUnit {              // world + 0x104: one per partition, 0xc bytes
    MdtWorld* world;
    int partition;
    int slot;                     // Kea work slot
};

struct MdtWorldHook {             // world + 0x10 list (pool +0xcc, 0x3c bytes) (name inferred)
    int m00[5];
    MdtWorldHook* next;           // +0x14
    int m18[2];
    void (MEAPI* fn)(MdtWorldHook* h);  // +0x20 called at the start of every step
    int m24[6];
};

struct MdtWorld {
    MdtBody* bodies;              // +0x000 all bodies (MdtBody::next)
    MdtConstraint* enabledConstraints; // +0x004 (MdtConstraint::next)
    MdtBody* enabledBodies;       // +0x008 (MdtBody::nextEnabled)
    int m00c;                     // +0x00c
    MdtWorldHook* hooks;          // +0x010
    int threadingMode;            // +0x014 MdtKeaGetThreadingMode(), 2 -> 0
    int maxBodies;                // +0x018
    int numBodies;                // +0x01c
    int numEnabledBodies;         // +0x020
    VinPool bodyPool;             // +0x024 (element 0x5a4 / Win 0x5b8, 16-aligned)
    int maxConstraints;           // +0x058
    int numEnabledConstraints;    // +0x05c
    int numConstraints;           // +0x060
    VinPool constraintPool;       // +0x064 "world constraintPool"
    VinPool constraintNodePool;   // +0x098 "world cNodePool"
    VinPool hookPool;             // +0x0cc (10 x 0x3c)
    MdtKeaFrames* frames;         // +0x100
    MdtWorkUnit* workUnits;       // +0x104
    int workUnitCapacity;         // +0x108
    MdtPartitionOutput partitions;  // +0x10c
    MeReal stepSize;              // +0x188 (1/60)
    MeReal gravity[3];            // +0x190
    MeReal m1a8;                  // +0x1a8
    MdtKeaParameters keaParams;   // +0x1b0
    MdtBclSolverParameters bclParams; // +0x22c / Win +0x240
    MdtPartitionParams partitionParams; // +0x2b4 / +0x2c8
    void (MEAPI* bodyEnabledCallback)(MdtBody* b);  // +0x2cc / +0x2e0 called by MdtBodyEnable
    void (MEAPI* bodyDisabledCallback)(MdtBody* b); // +0x2d0 / +0x2e4 called by MdtBodyDisable
    const char* version;          // +0x2d4 / +0x2e8 MeGetVersionString()
    int useBodyMaxTimeStep;       // +0x2d8 / +0x2ec cap h by MdtBody::maxTimeStep
    int sizesChanged;             // +0x2dc / +0x2f0 constraints/bodies created since the last step
    VinPool groupingPool;         // +0x2e0 / +0x2f4 "Groupings Pool"
    MdtGrouping** groupings;      // +0x314 / +0x328
    int numGroupings;             // +0x318 / +0x32c
    int groupingCapacity;         // +0x31c / +0x330
    int maxPartitionsPerUnit;     // +0x320 / +0x334 (0x7fffffff)
};

// ===============================================================================================
// Layout checks (numbers read from the binaries)
// ===============================================================================================
#define MDT_OFFSET_ASSERT(T, m, mac, win) VX_STATIC_ASSERT(offsetof(T, m) == MDT_PICK(mac, win))
// The checks describe the two original 32-bit builds only (PORT64: not 64-bit Windows, where
// _WIN32 is defined too).
#if defined(_WIN32) && !defined(_WIN64)
#define MDT_PICK(mac, win) (win)
#define MDT_LAYOUT_CHECK 1
#elif defined(__i386__)
#define MDT_PICK(mac, win) (mac)
#define MDT_LAYOUT_CHECK 1
#endif
#if defined(MDT_LAYOUT_CHECK)
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof" // classes with a vtable member (frictions)
#endif
MDT_OFFSET_ASSERT(MdtBody, keaBody, 0x140, 0x140);
MDT_OFFSET_ASSERT(MdtBody, useRelativeTransform, 0x4ec, 0x4f0);
MDT_OFFSET_ASSERT(MdtBody, impulseAdded, 0x4f0, 0x4f4);
MDT_OFFSET_ASSERT(MdtBody, world, 0x4f4, 0x4f8);
MDT_OFFSET_ASSERT(MdtBody, nextEnabled, 0x500, 0x504);
MDT_OFFSET_ASSERT(MdtBody, userData, 0x508, 0x50c);
MDT_OFFSET_ASSERT(MdtBody, partition, 0x510, 0x514);
MDT_OFFSET_ASSERT(MdtBody, mass, 0x518, 0x520);
MDT_OFFSET_ASSERT(MdtBody, autoDisable, 0x520, 0x528);
MDT_OFFSET_ASSERT(MdtBody, constraints, 0x544, 0x550);
MDT_OFFSET_ASSERT(MdtBody, m548, 0x548, 0x554);
MDT_OFFSET_ASSERT(MdtBody, enabledTime, 0x550, 0x55c);
MDT_OFFSET_ASSERT(MdtBody, frozen, 0x554, 0x560);
MDT_OFFSET_ASSERT(MdtBody, maxTimeStep, 0x558, 0x568);
MDT_OFFSET_ASSERT(MdtBody, lastForce, 0x560, 0x570);
MDT_OFFSET_ASSERT(MdtBody, lastTorque, 0x578, 0x588);
MDT_OFFSET_ASSERT(MdtBody, partitionInfo, 0x590, 0x5a0);
MDT_OFFSET_ASSERT(MdtBody, self, 0x5a0, 0x5b0);
VX_STATIC_ASSERT(sizeof(MdtBody) == MDT_PICK(0x5a4, 0x5b8));
VX_STATIC_ASSERT(sizeof(MdtKeaBody) == MDT_PICK(0x3ac, 0x3b0));
VX_STATIC_ASSERT(offsetof(MdtKeaBody, force) == MDT_PICK(0x24, 0x28));
VX_STATIC_ASSERT(offsetof(MdtKeaBody, I) == MDT_PICK(0x64, 0x68));
VX_STATIC_ASSERT(offsetof(MdtKeaBody, vel) == MDT_PICK(0x1e4, 0x1e8));
VX_STATIC_ASSERT(offsetof(MdtKeaBody, comTM) == MDT_PICK(0x2e4, 0x2e8));
VX_STATIC_ASSERT(offsetof(MdtKeaBody, grouping) == MDT_PICK(0x3a8, 0x3ac));
VX_STATIC_ASSERT(sizeof(MdtKeaParameters) == MDT_PICK(0x7c, 0x90));
VX_STATIC_ASSERT(offsetof(MdtKeaParameters, workspace) == MDT_PICK(0x58, 0x68));
VX_STATIC_ASSERT(offsetof(MdtKeaParameters, owned) == MDT_PICK(0x74, 0x84));
VX_STATIC_ASSERT(offsetof(MdtKeaParameters, lcp) == MDT_PICK(0x14, 0x18));
VX_STATIC_ASSERT(offsetof(MdtKeaParameters, refineTolerance) == MDT_PICK(0x3c, 0x48));
VX_STATIC_ASSERT(offsetof(MdtKeaParameters, itrTolerance) == MDT_PICK(0x50, 0x60));
VX_STATIC_ASSERT(offsetof(MdtKeaParameters, constraintSolver) == MDT_PICK(0x70, 0x80));
VX_STATIC_ASSERT(sizeof(Vx::VxConstraintFriction) == MDT_PICK(0x34, 0x38));
VX_STATIC_ASSERT(offsetof(Vx::VxConstraintFriction, mEnabled) == MDT_PICK(0x04, 0x08));
VX_STATIC_ASSERT(offsetof(Vx::VxConstraintFriction, mAxis) == MDT_PICK(0x08, 0x0c));
VX_STATIC_ASSERT(offsetof(Vx::VxConstraintFriction, mMaxForce) == MDT_PICK(0x0c, 0x10));
VX_STATIC_ASSERT(offsetof(Vx::VxConstraintFriction, mDirty) == MDT_PICK(0x30, 0x34));
VX_STATIC_ASSERT(sizeof(MdtLimit) == 0xf8);
MDT_OFFSET_ASSERT(MdtConstraint, force, 0x3c, 0x40);
MDT_OFFSET_ASSERT(MdtConstraint, torque, 0xfc, 0x100);
MDT_OFFSET_ASSERT(MdtConstraint, visited, 0x1bc, 0x1c0);
MDT_OFFSET_ASSERT(MdtConstraint, rowBuilder, 0x1c4, 0x1c8);
MDT_OFFSET_ASSERT(MdtConstraint, numLimits, 0x1d0, 0x1d4);
MDT_OFFSET_ASSERT(MdtConstraint, name, 0x1d8, 0x1dc);
MDT_OFFSET_ASSERT(MdtConstraint, partitionInfo, 0x1dc, 0x1e0);
MDT_OFFSET_ASSERT(MdtConstraint, flags, 0x1f0, 0x1f4);
MDT_OFFSET_ASSERT(MdtConstraint, bcl, 0x1f4, 0x1f8);
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, maxRows) == MDT_PICK(0x1fc, 0x200));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, frame) == MDT_PICK(0x200, 0x208));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, keaBody) == MDT_PICK(0x500, 0x508));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, qrot) == MDT_PICK(0x548, 0x550));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, relaxActive) == MDT_PICK(0x560, 0x568));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, frictionDirty) == MDT_PICK(0x5a0, 0x5a8));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, friction) == MDT_PICK(0x5a4, 0x5b0));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, relaxInvStiffness) == MDT_PICK(0x6dc, 0x700));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, forceLo) == MDT_PICK(0x85c, 0x880));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, rowStatus) == MDT_PICK(0x99c, 0x9c0));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, partitionCount) == MDT_PICK(0x9dc, 0xa00));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, lambda) == MDT_PICK(0x9e0, 0xa08));
VX_STATIC_ASSERT(offsetof(MdtConstraint, bcl) + offsetof(MdtBclConstraint, lambda2) == MDT_PICK(0xa68, 0xa90));
VX_STATIC_ASSERT(sizeof(MdtConstraint) == MDT_PICK(0xae8, 0xb10));
VX_STATIC_ASSERT(sizeof(MdtBSJoint) == MDT_PICK(0xaec, 0xb18));
VX_STATIC_ASSERT(sizeof(MdtHinge) == MDT_PICK(0xbe4, 0xc10));
VX_STATIC_ASSERT(sizeof(MdtCylindrical) == MDT_PICK(0xcd8, 0xd00));
VX_STATIC_ASSERT(sizeof(MdtAngular1Position3) == MDT_PICK(0xec8, 0xef0));
VX_STATIC_ASSERT(sizeof(MdtMotor) == MDT_PICK(0xbe0, 0xc08));
VX_STATIC_ASSERT(sizeof(MdtType14) == MDT_PICK(0xbe8, 0xc10));
VX_STATIC_ASSERT(sizeof(MdtConeLimit) == MDT_PICK(0xb08, 0xb30));
VX_STATIC_ASSERT(sizeof(MdtRPROJoint) == MDT_PICK(0xb88, 0xbb0));
VX_STATIC_ASSERT(offsetof(MdtRPROJoint, relQ) == MDT_PICK(0xb48, 0xb70));
VX_STATIC_ASSERT(offsetof(MdtContact, normal) == MDT_PICK(0xb30, 0xb58));
VX_STATIC_ASSERT(offsetof(MdtContact, penetration) == MDT_PICK(0xb50, 0xb78));
VX_STATIC_ASSERT(offsetof(MdtContact, material) == MDT_PICK(0xb78, 0xba0));
// The real VxContactMaterial holds a std::string (VxBase): its size follows the build's
// std::string (Mac libstdc++ 4 bytes, MSVC 2008 28); the members after it move by the difference.
// (Win builds pad the material data to the MSVC offset: delta 0, see VxMaterial.h VxLayoutPad.)
#if defined(_WIN32)
#define MDT_STRING_DELTA 0
#else
#define MDT_STRING_DELTA ((int)sizeof(std::string) - 4)
#endif
VX_STATIC_ASSERT(sizeof(Vx::VxContactMaterial) == MDT_PICK(0x174, 0x198) + MDT_STRING_DELTA);
VX_STATIC_ASSERT((int)offsetof(MdtContact, materialName) == MDT_PICK(0xcec, 0xd38) + MDT_STRING_DELTA);
VX_STATIC_ASSERT((int)offsetof(MdtContact, next) == MDT_PICK(0xd10, 0xd60) + MDT_STRING_DELTA);
VX_STATIC_ASSERT((int)sizeof(MdtContact) == MDT_PICK(0xd14, 0xd68) + MDT_STRING_DELTA);
VX_STATIC_ASSERT(offsetof(MdtWinch, offset) == MDT_PICK(0xb0c, 0xb38));
VX_STATIC_ASSERT(offsetof(MdtWinch, length) == MDT_PICK(0xb24, 0xb50));
VX_STATIC_ASSERT(offsetof(MdtWinch, part) == MDT_PICK(0xb44, 0xb70));
VX_STATIC_ASSERT(offsetof(MdtWinch, frame) == MDT_PICK(0xb48, 0xb74));
VX_STATIC_ASSERT(sizeof(MdtPartitionOutput) == 0x7c);
VX_STATIC_ASSERT(sizeof(MdtPartitionParams) == 0x18);
VX_STATIC_ASSERT(sizeof(MdtBclSolverParameters) == 0x88);
VX_STATIC_ASSERT(sizeof(MdtGrouping) == MDT_PICK(0x11c, 0x130));
VX_STATIC_ASSERT(sizeof(MdtWorldHook) == 0x3c);
MDT_OFFSET_ASSERT(MdtWorld, bodyPool, 0x24, 0x24);
MDT_OFFSET_ASSERT(MdtWorld, constraintPool, 0x64, 0x64);
MDT_OFFSET_ASSERT(MdtWorld, frames, 0x100, 0x100);
MDT_OFFSET_ASSERT(MdtWorld, partitions, 0x10c, 0x10c);
MDT_OFFSET_ASSERT(MdtWorld, stepSize, 0x188, 0x188);
MDT_OFFSET_ASSERT(MdtWorld, keaParams, 0x1b0, 0x1b0);
MDT_OFFSET_ASSERT(MdtWorld, bclParams, 0x22c, 0x240);
MDT_OFFSET_ASSERT(MdtWorld, partitionParams, 0x2b4, 0x2c8);
MDT_OFFSET_ASSERT(MdtWorld, useBodyMaxTimeStep, 0x2d8, 0x2ec);
MDT_OFFSET_ASSERT(MdtWorld, sizesChanged, 0x2dc, 0x2f0);
MDT_OFFSET_ASSERT(MdtWorld, groupingPool, 0x2e0, 0x2f4);
MDT_OFFSET_ASSERT(MdtWorld, groupings, 0x314, 0x328);
VX_STATIC_ASSERT(sizeof(MdtWorld) == MDT_PICK(0x324, 0x338));
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif

#endif
