// LEGO Digital Designer — restored source
// LEGO::BGApplication — abstract building-guide application used by the BI generators, the
//   producer and BGMode / BGAutomaticMode; only implementation BGAutomaticApplication (TU 364).
//
// RTTI: class LEGO::BGApplication (no bases, no data; Mac vtable 0x74d66c, Win vtable 0xa2bf0c
// with 52 _purecall entries). No virtual destructor: the first two Mac slots (0x348a00 /
// 0x348810) are FilterSubsteps / AnalyzeBrick; BGAutomaticApplication's destructor overrides
// Object's (Mac idx 52/53). NOTE: declared deriving `virtual Object` so a BGApplication* can be
// reference counted (RTTI: BGAutomaticApplication : virtual Object, BGApplication); this
// changes only the RTTI base list, not a slot.
//
// Win slot k = Mac idx k for every slot. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "StorageScene2.h"

namespace LEGO {

class Scene;
class PrimitiveElement;
class BIGraph;
class BIBrick;
class BIBrickPrimitive;
class BIBrickSubstep;
class BICameraChangeFinder;
class BIAnalyzer;
class BIEvent;
class BIGenerator;
class BIProgram;
class BIProducer;
class BGScene;

class BGApplication : public virtual Object {
public:
    typedef BasicArray<RefPointer<BIEvent> > EventList;
    // The events of every substep (key 0 = the top level of the guide).
    typedef BasicMap<BIBrickSubstep*, EventList> SubstepEventMap;

    // Removes from `bricks` the substeps the substep filters reject (BISubstepFilterProgram::
    // Apply); returns the number removed.
    virtual UInt32 FilterSubsteps(BasicSet<BIBrick*>& bricks) = 0;         // idx 0: Mac 0x348a00, Win 0x5677b0
    // Runs the producer over the primitives of `brick` and registers the proposed substeps
    // in the repository; false when cancelled.
    virtual bool AnalyzeBrick(BIBrick* brick) = 0;                         // idx 1: Mac 0x348810, Win 0x567640
    // Sets the program (+0x4c) and re-applies it to the current generator.
    virtual void SetProgram(BIProgram* program) = 0;                       // idx 2: Mac 0x348a20, Win 0x568000
    // A new constructive generator as the current one.
    virtual void CreateGenerator() = 0;                                    // idx 3: Mac 0x346880, Win 0x566fb0
    // The generator of the top level (first of the generator stack).
    virtual BIGenerator* GetRootGenerator() = 0;                           // idx 4: Mac 0x346470, Win 0x566c40
    // The top-level events of the guide (the substep map's entry 0).
    virtual const EventList& GetEvents() = 0;                              // idx 5: Mac 0x346480, Win 0x566c70
    // The events of every substep (+0x9c).
    virtual const SubstepEventMap& GetSubstepEvents() = 0;                 // idx 6: Mac 0x3464c0, Win 0x566cb0
    // The generator stack is not empty / holds more than the top level.
    virtual bool HasGenerators() = 0;                                      // idx 7: Mac 0x346450, Win 0x566c10
    virtual bool IsInSubstep() = 0;                                        // idx 8: Mac 0x3464d0, Win 0x566cd0
    // Shows every free brick of the repository in the current scene.
    virtual void ShowFreeBricks() = 0;                                     // idx 9: Mac 0x3464f0, Win 0x566d00
    // Rebuilds the scene of every generator from its bricks before the current position.
    virtual void RestoreScenes() = 0;                                      // idx 10: Mac 0x3465c0, Win 0x566db0
    // The current generator's event at `offset` (BIGenerator::GetEvent); null without
    // generator.
    virtual BIEvent* GetEvent(Int32 offset) = 0;                           // idx 11: Mac 0x348ba0, Win 0x5680f0
    // One part forward / back (`substeps`: into substeps); the event of the part, null at the
    // end; `camera` (may be null) receives the camera of a step passed.
    virtual BIEvent* NextPart(bool substeps, StorageScene2::Camera** camera) = 0;   // idx 12: Mac 0x348bd0, Win 0x568560
    virtual BIEvent* PrevPart(bool substeps, StorageScene2::Camera** camera) = 0;   // idx 13: Mac 0x348d40, Win 0x568140
    // One step forward / back: the events of the step; false when there is none.
    virtual bool NextStep(EventList& events, StorageScene2::Camera** camera) = 0;   // idx 14: Mac 0x348fb0, Win 0x568810
    virtual bool PrevStep(EventList& events, StorageScene2::Camera** camera) = 0;   // idx 15: Mac 0x349220, Win 0x568330
    // Inserts a jumper event (BIEventJumper, value 2) at the current position. Always false.
    virtual bool InsertJumper() = 0;                                       // idx 16: Mac 0x349480, Win 0x568a70
    // Enters the substep of the next event (a new generator for it).
    virtual bool EnterSubstep() = 0;                                       // idx 17: Mac 0x3495b0, Win 0x568ca0
    // Leaves the current substep: its bricks become one event of the parent generator, which
    // is returned (null when the substep was empty or the parent could not take it); `camera`
    // as NextPart.
    virtual BIEvent* ExitSubstep(StorageScene2::Camera** camera) = 0;      // idx 18: Mac 0x3496c0, Win 0x568e50
    // To the first / last event (the generator's Rewind / FastForward).
    virtual bool Rewind() = 0;                                             // idx 19: Mac 0x349cf0, Win 0x5692d0
    virtual bool FastForward() = 0;                                        // idx 20: Mac 0x349d10, Win 0x569300
    // Moves to event `position` of the current generator: the events passed going back are
    // appended to `back`, those passed going forward to `forward`.
    virtual void MoveTo(Int32 position, EventList& back, EventList& forward) = 0;   // idx 21: Mac 0x349d40, Win 0x569340
    // Takes the next event out of the current generator (`checkNext`: only when it is a brick
    // event the generator can take; `release`: its substep becomes removed in the repository).
    virtual RefPointer<BIEvent> TakeEvent(bool checkNext, bool release) = 0;   // idx 22: Mac 0x349f30, Win 0x569560
    // BIGenerator::Truncate of the current generator; true.
    virtual bool Truncate() = 0;                                           // idx 23: Mac 0x34a0a0, Win 0x569780
    // BIGenerator::ToggleBrick of the current generator.
    virtual bool ToggleBrick(BIBrick* brick, BasicArray<Pair<bool, RefPointer<BIEvent> > >& changes) = 0;   // idx 24: Mac 0x34a0c0, Win 0x5697b0
    // The camera change finder, created on first use (+0x60).
    virtual BICameraChangeFinder* GetCameraChangeFinder() = 0;            // idx 25: Mac 0x34b0f0, Win 0x56a8a0
    // The analyzers whose substeps may be placed (+0x30).
    virtual void ClearEnabledGroups() = 0;                                 // idx 26: Mac 0x34c560, Win 0x566360
    virtual void EnableGroup(const BIAnalyzer* group) = 0;                 // idx 27: Mac 0x34c5a0, Win 0x566380
    // Bricks of this group (BIBrick::GetGroup) may be placed.
    virtual bool IsGroupEnabled(const BIAnalyzer* group) = 0;              // idx 28: Mac 0x34c5d0, Win 0x5663b0
    // The current generator's scene (null without generator).
    virtual BGScene* GetScene() = 0;                                       // idx 29: Mac 0x34b290, Win 0x56aa20
    // The scene of the model (+0x50).
    virtual Scene* GetBrickScene() = 0;                                    // idx 30: Mac 0x34b2c0, Win 0x56aa60
    // The program (+0x4c) / the producer (+0x5c).
    virtual BIProgram* GetProgram() = 0;                                   // idx 31: Mac 0x34b2d0, Win 0x56aa80
    virtual BIProducer* GetProducer() = 0;                                 // idx 32: Mac 0x34c630, Win 0x5663f0
    // The candidate events (+0xcc).
    virtual EventList& GetCandidates() = 0;                                // idx 33: Mac 0x34a9f0, Win 0x569f70
    // The candidate brick to place for `brick`: the candidate with the fewest primitives that
    // `brick` intersects.
    virtual BIBrick* SelectCandidate(BIBrick* brick) = 0;                  // idx 34: Mac 0x34aa00, Win 0x569f90
    // At the first / last part.
    virtual bool IsAtStart() = 0;                                          // idx 35: Mac 0x34b030, Win 0x56a540
    virtual bool IsAtEnd() = 0;                                            // idx 36: Mac 0x34b070, Win 0x56a5a0
    // The current generator's position (BIGenerator::GetCurrent) / complete count
    // (BIGenerator::GetCompleteCount); 0 without generator.
    virtual UInt32 GetCurrentStep() = 0;                                   // idx 37: Mac 0x34af90, Win 0x56a430
    virtual UInt32 GetStepCount() = 0;                                     // idx 38: Mac 0x34afc0, Win 0x56a480
    // The current generator (+0x58).
    virtual BIGenerator* GetGenerator() = 0;                               // idx 39: Mac 0x34aff0, Win 0x56a4d0
    // The substeps being worked on, outermost first (+0x7c; BIGenerator reads
    // BIBrick::m_primitives of the last one).
    virtual BasicArray<BIBrick*>& GetSubsteps() = 0;                       // idx 40: Mac 0x34c640, Win 0x566410
    // The primitives already placed (`placed`) or still to place.
    virtual void GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives, bool placed) = 0;   // idx 41: Mac 0x34b2e0, Win 0x56aaa0
    // The generation was cancelled (+0x100) / sets the flag.
    virtual bool IsCancelled() = 0;                                        // idx 42: Mac 0x34b0b0, Win 0x56a840
    virtual void Cancel() = 0;                                             // idx 43: Mac 0x34b0c0, Win 0x56a860
    // The progress by one (a primitive placed) and the observers notified.
    virtual void AdvanceProgress() = 0;                                    // idx 44: Mac 0x34a430, Win 0x569d40
    // The progress by one analyzer step (+0x104) and the observers notified.
    virtual void StepProgress() = 0;                                       // idx 45: Mac 0x34a6f0, Win 0x569e30
    // Remembers / restores the progress.
    virtual void SaveProgress() = 0;                                       // idx 46: Mac 0x34a9b0, Win 0x569f30
    virtual void RestoreProgress() = 0;                                    // idx 47: Mac 0x34a9d0, Win 0x569f50
    // The world transform of a primitive element of the model (identity when unknown).
    virtual Matrix3D GetElementTransform(const PrimitiveElement* element) = 0;   // idx 48: Mac 0x349ee0, Win 0x5694e0
    // The index of the current step (+0xe4).
    virtual void UpdateCurrentStep() = 0;                                  // idx 49: Mac 0x34c020, Win 0x56b5e0
    // One of the 8 corner directions (±1, ±1, ±1) used for the camera positions.
    virtual Vector3 GetCornerDirection(UInt32 corner) = 0;                 // idx 50: Mac 0x34a0e0, Win 0x5697f0
    // Re-evaluates the guide state and notifies the observers (`stepChanged`: the step
    // display too).
    virtual void UpdateView(bool stepChanged) = 0;                         // idx 51: Mac 0x34b700, Win 0x56afe0
};

} // namespace LEGO
