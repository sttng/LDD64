// LEGO Digital Designer — restored source
// LEGO::GenerateBuildingGuideCommand — the command that generates the building guide of the
//   model (a BuildingGuideGenerator, run modally with a progress dialog). LDDApplication keeps
//   one (+0x168) and runs it when a model without a building guide is saved while "check
//   building guide" is set (Mac 0x1767a0); EditMode creates one too. Enabled while the model
//   is not empty and has no guide yet.
//
// Home TU 283 "GenerateBuildingGuideCommand.cpp" (Mac 0x295230-0x296dcb; Win 0x653675-0x653ecb).
// RTTI: GenerateBuildingGuideCommand : Command, BuildingGuideGeneratorObserver,
//   ModalUpdateReciever, BuildingGuideObserver. Mac layout (sizeof 0x48 + Object): Command
//   +0x00..+0x27, BuildingGuideGeneratorObserver +0x28 (subject +0x2c), ModalUpdateReciever
//   +0x30, BuildingGuideObserver +0x34 (subject +0x38), m_sceneControl +0x3c, m_generator
//   +0x40, m_generated +0x44, Object +0x48. Win sizeof 0x60.
// Mac vtable 0x7402ec {D1 0x295e00, D0 0x295fe0, OnFinalRelease 0x296220, Do 0x295930, Command
//   slots, ModalUpdate 0x2958e0, OnGenerationFinished 0x295bd0, OnBuildingGuideChanged 0x295c30}.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "Window.h"
#include "BI.h"
#include "BuildingGuideGenerator.h"

namespace LEGO {

class BrickConnectSceneControl;

class GenerateBuildingGuideCommand : public Command, public BuildingGuideGeneratorObserver,
                                     public ModalUpdateReciever, public BuildingGuideObserver {
public:
    // Created by EditMode::Initialize (new 0x50 Mac, 0x60 Win). Creates the generator (and its
    // dialog) and observes it and the control's BI.
    // Mac 0x2956f0, Win 0x653d80
    explicit GenerateBuildingGuideCommand(BrickConnectSceneControl* sceneControl);
    virtual ~GenerateBuildingGuideCommand();                               // Mac 0x295e00 / 0x295fe0, Win sdd 0x653ea7

    // Generates the guide (the active tool is reset first); runs the application's modal loop
    // until the generator is done; "UNABLETOGENERATEBG" when nothing was generated and the
    // generation was not cancelled. Nothing for an empty model. Mac 0x295930, Win 0x653a05
    virtual void Do();

    // ---- ModalUpdateReciever: drives the generator (its Update). Mac 0x2958e0 (thunk 0x295900)
    virtual void ModalUpdate();
    // ---- BuildingGuideGeneratorObserver: remembers the outcome and ends the modal loop.
    // Mac 0x295bd0 (thunk 0x295c00)
    virtual void OnGenerationFinished(bool generated);
    // ---- BuildingGuideObserver: enabled when the model has no steps and is not empty.
    // Mac 0x295c30 (thunk 0x295c80), Win 0x653675
    virtual void OnBuildingGuideChanged(bool hasSteps);

    // The last Do() produced a guide (byte +0x44). Mac 0x295920, Win 0x553620
    bool WasGenerated() const;

protected:
    // Leaves the BI, the generator and the command observers. Mac 0x296220 (thunks 0x296390 /
    // 0x2963b0 / 0x2963d0), Win 0x653ecb
    virtual void OnFinalRelease();

public:
    BrickConnectSceneControl*          m_sceneControl;   // +0x3c
    RefPointer<BuildingGuideGenerator> m_generator;      // +0x40
    bool                               m_generated;      // +0x44
};

} // namespace LEGO
