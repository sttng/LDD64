// LEGO Digital Designer — restored source
// LEGO::BGAutomaticApplication — the automatic building-guide application: owns the producer
//   (substep analyzers), the substep filter program and a stack of constructive generators (one
//   per open substep), keeps the events of every substep (key 0 = top level) and drives
//   BGAutomaticMode's navigation. Created by BGAutomaticMode / BuildingGuideGenerator.
//
// Home TU 364 "BGAutomaticApplication.cpp" (Mac 0x345270-0x34c7cf, Win 0x566360-0x56bf6f).
// RTTI: class LEGO::BGAutomaticApplication : virtual Object, BGApplication, SceneObserver,
//   Notifier<BGAutomaticApplicationObserver>.
// Mac layout (sizeof 0x110; names inferred): +0x00 BGApplication vptr, +0x04 SceneObserver
//   (vptr, subject +0x08), +0x0c Notifier (vptr, observers +0x10, iterations +0x20), members
//   +0x30..+0x104 as annotated below, +0x108 Object.
//   Win: the same members 0x0c-0x14 later (VC9 sets / maps are 0x24 bytes): generator +0x64,
//   substeps +0x88, candidates +0xe0, progress +0x10c, cancelled +0x114.
// Mac vtables: 0x74d36c (52 BGApplication slots, D1 0x3459d0, D0 0x345b80, OnFinalRelease
//   0x345d70, SceneObserver overrides 0x34bd30 / 0x34bd50), SceneObserver-in 0x74d45c,
//   Notifier-in 0x74d48c, Object-in 0x74d4b0. Win: 0xa2bfec (52 slots), 0xa2c0c0, 0xa2c0dc.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Geometry.h"
#include "Notifier.h"
#include "Scene.h"
#include "BGApplication.h"
#include "BGAutomaticApplicationObserver.h"

namespace LEGO {

class Render;
class BISubstepFilterProgram;
class BIStorage;

class BGAutomaticApplication : public BGApplication, public SceneObserver,
                               public Notifier<BGAutomaticApplicationObserver> {
public:
    // `render`: the renderer the camera change finder draws with (RenderControlManager::
    // GetRender of the caller). Mac 0x3452e0, Win 0x566470
    explicit BGAutomaticApplication(Render* render);
    virtual ~BGAutomaticApplication();                                     // Mac 0x3459d0 / 0x345b80 (base 0x345540), Win 0x5667a0 / sdd 0x56e0a0

    // ---- BGApplication (see BGApplication.h for the slot comments) ----------------------------
    virtual UInt32 FilterSubsteps(BasicSet<BIBrick*>& bricks);             // Mac 0x348a00, Win 0x5677b0
    virtual bool AnalyzeBrick(BIBrick* brick);                             // Mac 0x348810, Win 0x567640
    virtual void SetProgram(BIProgram* program);                           // Mac 0x348a20, Win 0x568000
    virtual void CreateGenerator();                                        // Mac 0x346880, Win 0x566fb0
    virtual BIGenerator* GetRootGenerator();                               // Mac 0x346470, Win 0x566c40
    virtual const EventList& GetEvents();                                  // Mac 0x346480, Win 0x566c70
    virtual const SubstepEventMap& GetSubstepEvents();                     // Mac 0x3464c0, Win 0x566cb0
    virtual bool HasGenerators();                                          // Mac 0x346450, Win 0x566c10
    virtual bool IsInSubstep();                                            // Mac 0x3464d0, Win 0x566cd0
    virtual void ShowFreeBricks();                                         // Mac 0x3464f0, Win 0x566d00
    virtual void RestoreScenes();                                          // Mac 0x3465c0, Win 0x566db0
    virtual BIEvent* GetEvent(Int32 offset);                               // Mac 0x348ba0, Win 0x5680f0
    virtual BIEvent* NextPart(bool substeps, StorageScene2::Camera** camera);   // Mac 0x348bd0, Win 0x568560
    virtual BIEvent* PrevPart(bool substeps, StorageScene2::Camera** camera);   // Mac 0x348d40, Win 0x568140
    virtual bool NextStep(EventList& events, StorageScene2::Camera** camera);   // Mac 0x348fb0, Win 0x568810
    virtual bool PrevStep(EventList& events, StorageScene2::Camera** camera);   // Mac 0x349220, Win 0x568330
    virtual bool InsertJumper();                                           // Mac 0x349480, Win 0x568a70
    virtual bool EnterSubstep();                                           // Mac 0x3495b0, Win 0x568ca0
    virtual BIEvent* ExitSubstep(StorageScene2::Camera** camera);          // Mac 0x3496c0, Win 0x568e50
    virtual bool Rewind();                                                 // Mac 0x349cf0, Win 0x5692d0
    virtual bool FastForward();                                            // Mac 0x349d10, Win 0x569300
    virtual void MoveTo(Int32 position, EventList& back, EventList& forward);   // Mac 0x349d40, Win 0x569340
    virtual RefPointer<BIEvent> TakeEvent(bool checkNext, bool release);   // Mac 0x349f30, Win 0x569560
    virtual bool Truncate();                                               // Mac 0x34a0a0, Win 0x569780
    virtual bool ToggleBrick(BIBrick* brick, BasicArray<Pair<bool, RefPointer<BIEvent> > >& changes);   // Mac 0x34a0c0, Win 0x5697b0
    virtual BICameraChangeFinder* GetCameraChangeFinder();                // Mac 0x34b0f0, Win 0x56a8a0
    virtual void ClearEnabledGroups();                                     // Mac 0x34c560, Win 0x566360
    virtual void EnableGroup(const BIAnalyzer* group);                     // Mac 0x34c5a0, Win 0x566380
    virtual bool IsGroupEnabled(const BIAnalyzer* group);                  // Mac 0x34c5d0, Win 0x5663b0
    virtual BGScene* GetScene();                                           // Mac 0x34b290, Win 0x56aa20
    virtual Scene* GetBrickScene();                                        // Mac 0x34b2c0, Win 0x56aa60
    virtual BIProgram* GetProgram();                                       // Mac 0x34b2d0, Win 0x56aa80
    virtual BIProducer* GetProducer();                                     // Mac 0x34c630, Win 0x5663f0
    virtual EventList& GetCandidates();                                    // Mac 0x34a9f0, Win 0x569f70
    virtual BIBrick* SelectCandidate(BIBrick* brick);                      // Mac 0x34aa00, Win 0x569f90
    virtual bool IsAtStart();                                              // Mac 0x34b030, Win 0x56a540
    virtual bool IsAtEnd();                                                // Mac 0x34b070, Win 0x56a5a0
    virtual UInt32 GetCurrentStep();                                       // Mac 0x34af90, Win 0x56a430
    virtual UInt32 GetStepCount();                                         // Mac 0x34afc0, Win 0x56a480
    virtual BIGenerator* GetGenerator();                                   // Mac 0x34aff0, Win 0x56a4d0
    virtual BasicArray<BIBrick*>& GetSubsteps();                           // Mac 0x34c640, Win 0x566410
    virtual void GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives, bool placed);   // Mac 0x34b2e0, Win 0x56aaa0
    virtual bool IsCancelled();                                            // Mac 0x34b0b0, Win 0x56a840
    virtual void Cancel();                                                 // Mac 0x34b0c0, Win 0x56a860
    virtual void AdvanceProgress();                                        // Mac 0x34a430, Win 0x569d40
    virtual void StepProgress();                                           // Mac 0x34a6f0, Win 0x569e30
    virtual void SaveProgress();                                           // Mac 0x34a9b0, Win 0x569f30
    virtual void RestoreProgress();                                        // Mac 0x34a9d0, Win 0x569f50
    virtual Matrix3D GetElementTransform(const PrimitiveElement* element); // Mac 0x349ee0, Win 0x5694e0
    virtual void UpdateCurrentStep();                                      // Mac 0x34c020, Win 0x56b5e0
    virtual Vector3 GetCornerDirection(UInt32 corner);                     // Mac 0x34a0e0, Win 0x5697f0
    virtual void UpdateView(bool stepChanged);                             // Mac 0x34b700, Win 0x56afe0

    // ---- SceneObserver ------------------------------------------------------------------------
    // The scene stepped: the current part has to be re-evaluated (m_sceneChanged). Mac 0x34bd30
    // (thunk 0x34bd40)
    virtual void OnSceneNotification5();
    // Empty. Mac 0x34bd50 (thunk 0x34bd60)
    virtual void OnStructureChanged();

    // ---- non-virtual (names inferred) -----------------------------------------------------------
    // Prepares the generation for `scene`: producer, filter program, the BIBrickRepository
    // and BI element graph rebuilt from the scene, observes the scene, takes the existing
    // events of `storage` (may be a guide loaded with the model). Always true. Mac 0x345ee0,
    // Win 0x566990
    bool Start(Scene* scene, BIStorage* storage);
    // Leaves every substep and writes the top-level and substep events into `storage` (cleared
    // when nothing was generated); then Stop. Always true. Mac 0x346790
    bool Finish(BIStorage* storage);
    // Stops observing the scene and drops the generators, producer, program and events.
    // Always true. Mac 0x3462e0
    bool Stop();
    // Generates the guide: <0 failure / cancelled. Mac 0x346900
    Int32 Generate();
    // The generator stack is deeper / shallower than the marked level (BGAutomaticMode's
    // substep buttons). Mac 0x349510 / 0x349550
    bool IsBelowMarkedLevel();
    bool IsAboveMarkedLevel();
    // Marks the current generator level. Mac 0x349590
    void MarkLevel();
    // The generator one level up (null at the top level). Mac 0x34b000
    BIGenerator* GetParentGenerator();
    // The current step index / the number of steps (UpdateCurrentStep / UpdateView). Mac
    // 0x34bd70 / 0x34c0c0
    UInt32 GetStepIndex() const { return m_currentStep; }
    UInt32 GetStepTotal() const { return m_stepCount; }
    // Flattens the top-level events (with the substeps') and gives them to the current
    // generator. Mac 0x34a390, Win 0x569c60
    void ApplyEvents();
    // The size the camera change finder renders its views for. Mac 0x34b0d0
    void SetViewSize(Real32 width, Real32 height);
    UInt32 GetGeneratorCount() const { return m_generators.GetSize(); }
    UInt32 GetModelCount() const { return m_modelCount; }

    // ---- helpers (TU-internal on Mac; names inferred) -----------------------------------------
    // A primitive is pushed on the substep stack; a substep with stored events is regenerated
    // from them, another one generated. Mac 0x347040
    Int32 GenerateBrick(BIBrick* brick);
    // A new generator for `brick` (`depth` < m_analyzeDepth: with analysis) pushed on the
    // stacks; popped again on failure. Mac 0x347530
    Int32 GenerateSubstep(BIBrick* brick, UInt32 depth);
    // Generates the substeps (recursively) of the events stored for `substep`. Mac 0x347930
    void GenerateSubsteps(BIBrickSubstep* substep, UInt32 depth);
    // A new generator for the substep of `events` (its primitives, or with `fromRepository`
    // the free primitives) created from the events; progress 0.33 / 0.66 / 1.0. Mac 0x347c70
    Int32 Regenerate(const EventList& events, bool fromRepository);
    // Merges consecutive step events and expands the events of the substeps. Mac 0x34a1d0,
    // Win 0x5699c0
    void Flatten(EventList& events);
    // The number of steps in `events` (with the substeps) up to the current generator's
    // events at `position`; `found` when reached. Mac 0x34bd80
    UInt32 CountSteps(const EventList& events, UInt32 position, bool& found);
    // OnProgress(progress / total) for every observer (inlined everywhere).
    void NotifyProgress(Real32 progress);

    BasicSet<const BIAnalyzer*>                        m_enabledGroups;      // +0x30
    RefPointer<BIProgram>                              m_program;            // +0x4c
    Scene*                                             m_scene;              // +0x50
    Render*                                            m_render;             // +0x54
    RefPointer<BIGenerator>                            m_generator;          // +0x58
    RefPointer<BIProducer>                             m_producer;           // +0x5c
    RefPointer<BICameraChangeFinder>                   m_cameraChangeFinder; // +0x60
    Real32                                             m_viewWidth;          // +0x64
    Real32                                             m_viewHeight;         // +0x68
    BasicArray<Pair<PrimitiveElement*, Matrix3D> >     m_elementTransforms;  // +0x6c
    BasicArray<BIBrick*>                               m_substeps;           // +0x7c
    BasicArray<RefPointer<BIGenerator> >               m_generators;         // +0x8c
    SubstepEventMap                                    m_substepEvents;      // +0x9c
    BIBrick*                                           m_currentSubstep;     // +0xb8 (never read here)
    bool                                               m_sceneChanged;       // +0xbc
    RefPointer<BISubstepFilterProgram>                 m_filterProgram;      // +0xc0
    // Passed to BIGenerator::Next / Previous(Object*) after a scene change; never set in this
    // TU (and both generators ignore them).
    RefPointer<Object>                                 m_nextArgument;       // +0xc4
    RefPointer<Object>                                 m_previousArgument;   // +0xc8
    EventList                                          m_candidates;         // +0xcc
    bool                                               m_useGeneratorCandidates;   // +0xdc (never set here)
    UInt32                                             m_stepCount;          // +0xe0
    UInt32                                             m_currentStep;        // +0xe4
    UInt32                                             m_modelCount;         // +0xe8 hinged elements of the scene
    UInt32                                             m_markedLevel;        // +0xec
    UInt32                                             m_analyzeDepth;       // +0xf0 (1 for one model, else 2)
    UInt32                                             m_progressTotal;      // +0xf4
    UInt32                                             m_progress;           // +0xf8
    UInt32                                             m_savedProgress;      // +0xfc
    bool                                               m_cancelled;          // +0x100
    UInt32                                             m_progressStep;       // +0x104

protected:
    // Detaches the observers and leaves the scene. Mac 0x345d70 (thunks 0x345e80 / 0x345ea0 /
    // 0x345ec0)
    virtual void OnFinalRelease();
};

} // namespace LEGO
