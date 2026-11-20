// LEGO Digital Designer — restored source
// LEGO::BuildingGuideGenerator — generates the building guide of the model of a
//   BrickConnectSceneControl in the background (GenerateBuildingGuideCommand): a
//   BGAutomaticApplication runs Generate() in a BuildingGuideGenerateThread while a
//   ProgressBarDialog ("GENERATINGBUILDINGGUIDE") shows the progress; when the thread is done
//   the guide is stored in the control's BI. Its observers learn the outcome.
// LEGO::BuildingGuideGenerateThread — the worker thread.
// LEGO::BuildingGuideGeneratorObserver — observer of a BuildingGuideGenerator (its destructor
//   is emitted in TU 283).
// Every class name is RTTI; member and method names are inferred.
//
// Home TU 284 "BuildingGuideGenerator.cpp" (Mac 0x296dd0-0x29941b; Win 0x68affa-0x68b715,
// 0x624a51-0x624b31).
// RTTI: BuildingGuideGenerator : UpdateObserver, Notifier<BuildingGuideGeneratorObserver>,
//   BGAutomaticApplicationObserver. Mac layout (sizeof 0x48 + Object): UpdateObserver (vptr,
//   subject +0x04), Notifier +0x08 (observers +0x0c, iterations +0x1c), BGAutomaticApplication-
//   Observer +0x2c (subject +0x30), m_application +0x34, m_sceneControl +0x38, m_thread +0x3c,
//   m_dialog +0x40, m_startDialog +0x44, m_finished +0x45, Object +0x48. Mac vtable 0x74066c
//   {D1 0x298770, D0 0x2987d0, OnFinalRelease 0x298840, OnAttached, OnDetached, Update 0x297ba0,
//   OnProgress 0x297b20}; Win: Update 0x68b011, OnProgress 0x68affa, sdd 0x68b644.
// RTTI: BuildingGuideGenerateThread : Object, ProgressBarThread. Mac layout: Object +0x00,
//   ProgressBarThread +0x08 (cancelled +0x78, progress +0x7c), m_generator +0x80,
//   m_application +0x84, m_result +0x88; sizeof 0x8c. Mac vtable 0x740748 {D1 0x298c00, D0
//   0x298c50, Object slots, Run 0x297d30, Finish 0x297ee0}, Thread-in 0x74076c {.., Run
//   0x297e10}; Win: Run 0x68ae90, Finish 0x68b6dc, sdd 0x68b1a8.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"
#include "Updater.h"
#include "ProgressBarThread.h"
#include "BGAutomaticApplication.h"
#include "BGAutomaticApplicationObserver.h"

namespace LEGO {

class BrickConnectSceneControl;
class ProgressBarDialog;
class BuildingGuideGenerator;
class BuildingGuideGenerateThread;

class BuildingGuideGeneratorObserver : public Observer<BuildingGuideGenerator, BuildingGuideGeneratorObserver> {
public:
    BuildingGuideGeneratorObserver() {}
    virtual ~BuildingGuideGeneratorObserver() {}                          // Mac 0x2963f0 / 0x296420 (TU 283), Win sdd 0x6539de

    // The generation ended; `generated`: a guide was stored. Pure. (Mac idx 5)
    virtual void OnGenerationFinished(bool generated) = 0;
};

class BuildingGuideGenerator : public UpdateObserver, public Notifier<BuildingGuideGeneratorObserver>,
                               public BGAutomaticApplicationObserver {
public:
    // A generator for the model of `sceneControl`: a BGAutomaticApplication on the control's
    // renderer. Mac 0x296dd0
    explicit BuildingGuideGenerator(BrickConnectSceneControl* sceneControl);
    virtual ~BuildingGuideGenerator();                                     // Mac 0x298770 / 0x2987d0, Win sdd 0x68b644

    // Shows the progress dialog. Mac 0x296fd0
    void ShowDialog();
    // Starts the generation (`reset`: the control's guide is reset first). Mac 0x297060,
    // Win 0x68b11c
    void Start(bool reset);
    // Stores the generated guide in the control's BI (when anything was generated), else stops
    // the application. Mac 0x297700
    void StoreGuide();
    // The thread ended: StoreGuide (`success`) or stop, detach, notify the observers. Mac 0x297820
    void OnThreadFinished(bool success);
    BGAutomaticApplication* GetApplication() const { return m_application; }   // Mac 0x297b10
    bool IsFinished() const { return m_finished; }

    // ---- UpdateObserver: shows the dialog once, cancels on request, polls the thread.
    // Mac 0x297ba0, Win 0x68b011
    virtual void Update();
    // ---- BGAutomaticApplicationObserver: the thread's progress in percent. Mac 0x297b20
    // (thunk 0x297b60), Win 0x68affa
    virtual void OnProgress(Real32 progress);

protected:
    // Leaves the application, the observers and the updater. Mac 0x298840 (thunks 0x2989f0,
    // 0x298ae0, 0x298be0)
    virtual void OnFinalRelease();

public:
    RefPointer<BGAutomaticApplication>      m_application;   // +0x34
    BrickConnectSceneControl*               m_sceneControl;  // +0x38
    RefPointer<BuildingGuideGenerateThread> m_thread;        // +0x3c
    RefPointer<ProgressBarDialog>           m_dialog;        // +0x40
    bool                                    m_startDialog;   // +0x44 show the dialog on the next Update
    bool                                    m_finished;      // +0x45
};

class BuildingGuideGenerateThread : public Object, public ProgressBarThread {
public:
    BuildingGuideGenerateThread(BuildingGuideGenerator* generator, BGAutomaticApplication* application)
        : m_generator(generator), m_application(application), m_result(0) {}   // inlined (Mac 0x297060)
    virtual ~BuildingGuideGenerateThread() {}                              // Mac 0x298c00 / 0x298c50, Win sdd 0x68b1a8

    // Generates (view size from the scene control, physics locked meanwhile) and flattens the
    // events; true when Generate succeeded. Mac 0x297d30 (Thread-in copy 0x297e10), Win 0x68ae90
    virtual bool Run();
    // Reports the end to the generator (failure when cancelled); returns the success.
    // Mac 0x297ee0, Win 0x68b6dc
    virtual bool Finish();

    BuildingGuideGenerator* m_generator;     // +0x80
    BGAutomaticApplication* m_application;   // +0x84
    Int32                   m_result;        // +0x88 Generate's result

    friend class BuildingGuideGenerator;
};

} // namespace LEGO
