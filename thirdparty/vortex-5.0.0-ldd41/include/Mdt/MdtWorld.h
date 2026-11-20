// Vortex 5.0.0 build 800 "vortexldd41" - MdtWorld, partitioning, groupings and the Bcl solver
// parameters (restored). Sources: MdtWorld.cpp (Mac 0x4931ac-0x494648, Win
// 0x7323b0-0x733e00; __FILE__ "c:\work\martinc\vortexldd41\vortex\mdt\src\mdtworld.cpp"),
// MdtPartition.cpp (Mac 0x49035b-0x4920b7), MdtGrouping.cpp, MdtBcl.cpp. Names inferred except
// "MdtWorldInsertBody".
#ifndef MDT_MDTWORLD_H
#define MDT_MDTWORLD_H

#include "Mdt/MdtTypes.h"
#include "Me/MeCall.h"

// ---- world (MdtWorld.cpp) ---------------------------------------------------------------------
MdtWorld* MEAPI MdtWorldCreate(int maxBodies, int maxConstraints);    // Mac 0x4943d2, Win 0x7337b0
void MEAPI MdtWorldDestroy(MdtWorld* w);                              // Mac 0x493434, Win 0x732cf0
void MEAPI MdtWorldReset(MdtWorld* w);                                // Mac 0x494363, Win 0x7323b0
void MEAPI MdtWorldStep(MdtWorld* w, MeReal h);                       // Mac 0x49376b, Win 0x733990
void MEAPI MdtWorldInsertBody(MdtWorld* w, MdtBody* b);               // Mac 0x493657, Win 0x732a10 (string)
void MEAPI MdtWorldRemoveBody(MdtBody* b);                            // Mac 0x4935e8, Win 0x732af0
void MEAPI MdtWorldAddConstraint(MdtWorld* w, MdtConstraint* c);      // Mac 0x493372, Win 0x732b60
void MEAPI MdtWorldRemoveConstraint(MdtConstraint* c);                // Mac 0x4935ac, Win 0x732b90
void MEAPI MdtWorldSetMaxSizes(MdtWorld* w, int maxBodies, int maxConstraints); // Mac 0x4934fd, Win 0x732cc0
void MEAPI MdtWorldUpdateSizes(MdtWorld* w);                          // Mac 0x49354e, Win 0x733930
MeReal MEAPI MdtWorldGetStepSize(const MdtWorld* w);                  // Mac 0x4931ac, Win 0x7324c0
void MEAPI MdtWorldSetGravity(MdtWorld* w, MeReal x, MeReal y, MeReal z); // Mac 0x4931ba, Win 0x732530
void MEAPI MdtWorldSetAutoDisable(MdtWorld* w, int on);               // Mac 0x4933f6, Win 0x733630
void MEAPI MdtWorldSetPartitionOptimize(MdtWorld* w, int on);         // Mac 0x493415, Win 0x7332a0
int MEAPI MdtWorldGetMaxConstraintsPerLevel(const MdtWorld* w);       // Mac 0x4933cf (name inferred)
int MEAPI MdtWorldGetKeaM2c(const MdtWorld* w);                       // Mac 0x4933dd (name unknown)
MeReal MEAPI MdtWorldGetPartitionMaxTimeStep(MdtPartitionOutput* out, int partition); // Mac 0x4931e9, Win 0x732560
void MEAPI MdtAutoDisableLastPartition(MdtPartitionOutput* out);      // Mac 0x49421e, Win 0x732680

// ---- groupings (MdtGrouping.cpp, Mac 0x48f9e3-0x48fb5e, Win 0x73e320) ------------------------
MdtGrouping* MEAPI MdtWorldNewGrouping(MdtWorld* w);                  // Mac 0x48f9e3, Win 0x73e320
MdtGrouping* MEAPI MdtWorldGetGrouping(MdtWorld* w, int index);       // Mac 0x48fad8, Win 0x73e3e0

// ---- partitioning (MdtPartition.cpp) ---------------------------------------------------------
void MEAPI MdtPartitionOutputInit(MdtPartitionOutput* out);           // Mac 0x49042b, Win 0x734100
void MEAPI MdtPartitionOutputDestroy(MdtPartitionOutput* out, MeMemoryAPIStruct* api); // Mac 0x49035b, Win 0x734030
void MEAPI MdtPartitionOutputResize(MdtPartitionOutput* out, int maxPartitions, int maxBodies,
                              int maxConstraints, MeMemoryAPIStruct* api);       // Mac 0x4904da, Win 0x734160
void MEAPI MdtPartitionParamsReset(MdtPartitionParams* p);            // Mac 0x490695, Win 0x735a10
void MEAPI MdtPartitionParamsSetOptimize(MdtPartitionParams* p, int on); // Mac 0x49067e, Win 0x734910
int MEAPI MdtPartitionParamsGetOptimize(const MdtPartitionParams* p); // Mac 0x49068b, Win 0x734920
void MEAPI MdtPartitionParamsSetAutoDisable(MdtPartitionParams* p, int on); // Mac 0x4906ef, Win 0x734960
void MEAPI MdtPartition(MdtBody* enabledBodies, MdtPartitionOutput* out, MdtPartitionParams* p); // Mac 0x4906d0, Win 0x734930
void MEAPI MdtPartitionWorld(MdtBody* enabledBodies, MdtPartitionOutput* out, MdtPartitionParams* p); // Mac 0x491051, Win 0x735920
// Builds the Kea rows of one partition (bodies: impulses -> forces, gravity; constraints: row
// builders). Returns the number of rows.
int MEAPI MdtKeaAddPartition(MdtPartitionOutput* out, int partition, MeReal h, const MeReal* gravity,
                       const MdtBclSolverParameters* params, MdtKeaConstraints* rows); // Mac 0x49178e, Win 0x734730
// After the step: constraint forces back into the constraints, frictions updated.
void MEAPI MdtPartitionGetForces(MdtKeaConstraints* rows, int partition, MdtPartitionOutput* out); // Mac 0x491b42, Win 0x7343a0

// ---- Bcl solver parameters (MdtWorld.cpp / MdtBcl.cpp) ------------------------------
void MEAPI MdtBclSolverParametersReset(MdtBclSolverParameters* p);    // Mac 0x494311, Win 0x732370
void MEAPI MdtBclSolverParametersSet(MdtBclSolverParameters* p, MeReal epsilon, MeReal epsilon2,
                               MeReal h, MeReal k);             // Mac 0x4946fa, Win 0x73dc50
void MEAPI MdtBclSolverParametersUpdate(MdtBclSolverParameters* p, int grouping, int unused1,
                                  MeReal unused2, MeReal unused3, MeReal h); // Mac 0x494648, Win 0x73dbe0

#endif
