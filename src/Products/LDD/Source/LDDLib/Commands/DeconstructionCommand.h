// LEGO Digital Designer — restored source
// LEGO::DeconstructionCommand — the view mode's "deconstruction" (Lua LDD.commands.viewMode.
//   deconstruction): the model's bricks fly apart (FallingBricks, 8 s, all at once, spinning,
//   square-root easing, the scene's own drawing switched off) with the explosion sound; when
//   that ends a second FallingBricks brings them back one brick after the other (6 s, reverse,
//   linear). Enabled while the scene is not empty.
//
// TU 234 "DeconstructionCommand.cpp" (Mac 0x26cb60-0x26e30b; Win 0x65a066-0x65ab2f + the
// constructor 0x65afc0). Also emitted there: FallingBricks' constructor (inline, Mac 0x26cb60),
// FallingBricksObserver / Observer<FallingBricks, …> (Mac 0x26dee0-0x26e230), the FallingBricks
// observer Attach (Mac 0x26ce20), BasicArray<CommandObserver*> instance (0x26d800 = the D2
// destructor with VTT).
//
// RTTI: vmi {Command +0x00, FallingBricksObserver +0x28, PreferencesObserver +0x30,
//   BrickConnectSceneControlObserver +0x38}, virtual base Object +0x50 (Mac sizeof 0x58,
//   ViewMode operator new 0x58). Win: Object +0x60 (operator new 0x6c), scene control +0x50.
// Vtables: Mac 0x737fcc {D1 0x26d9c0, D0 0x26dba0, OnFinalRelease 0x26dde0, Do 0x26d460,
//   SetEnable, GetEnable, UpdateEnable 0x26d420, (primary entries of the secondary-base
//   overriders) OnFinished 0x26d5d0, OnPreferenceChanged 0x26d650, OnAttached 0x26d7a0,
//   OnEvent19 0x26d7d0}; FallingBricksObserver-in 0x738004 {6: thunk 0x26d610},
//   PreferencesObserver-in 0x73802c {5: thunk 0x26d780}, BCSC-observer-in 0x738050 {3: thunk
//   0x26d7b0, 19: thunk 0x26d7e0}, Object-in 0x7380c4. Win 0x9d6c84 {Do 0x65a670, SetEnable,
//   GetEnable, UpdateEnable 0x65a066}, BCSC-observer-in 0x9d6c0c {0: 0x65a08d, 16: 0x65a098},
//   PreferencesObserver-in 0x9d6c60 {2: 0x65a344}, FallingBricksObserver-in 0x9d6c70
//   {3: 0x65a310}, Object-in 0x9d6bf4 {sdd 0x65a7ac -> 0x65ab10, OnFinalRelease 0x65abbc -> 0x65ab2f}.
// Class name from RTTI; the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "Preferences.h"
#include "SimpleSound.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"
#include "FallingBricks.h"

namespace LEGO {

class Render;

class DeconstructionCommand : public Command,
                              public FallingBricksObserver,
                              public PreferencesObserver,
                              public BrickConnectSceneControlObserver {
public:
    // Enabled; two FallingBricks of `sceneControl` / `render` (the first observed), the
    // explosion sound; observes the Preferences and the scene control. Mac 0x26d090, Win 0x65afc0
    DeconstructionCommand(BrickConnectSceneControl* sceneControl, Render* render);
    virtual ~DeconstructionCommand();                                        // Mac 0x26d9c0/0x26dba0, Win sdd 0x65ab10

    // Stops both animations (the first one's OnFinished starts and the stop then ends the
    // second). Called by ViewMode::Exit. Mac 0x26d3f0, Win 0x65a2fc (name inferred)
    void Stop();

    // Stops both, explodes the scene's bricks (one ConceptBrick per brick) and plays the
    // explosion sound at full volume.
    virtual void Do();                                                       // 3/0 Mac 0x26d460, Win 0x65a670
    // Enabled while the scene is not empty.
    virtual void UpdateEnable();                                             // 6/3 Mac 0x26d420, Win 0x65a066

    // ---- accessors (names inferred) ----
    FallingBricks* GetExplosion() const { return m_explosion; }
    FallingBricks* GetAssembly() const { return m_assembly; }
    SimpleSound*   GetSound() const { return m_sound; }

protected:
    // ---- FallingBricksObserver: the explosion ended -> assemble again ----
    virtual void OnFinished(const BasicArray<RefPointer<ConceptBrick> >& conceptBricks,
                            bool reverse);                                   // 7/– Mac 0x26d5d0 (thunk 0x26d610), Win 0x65a310
    // ---- PreferencesObserver: "PLAYSOUND" (any case) loads the sound again ----
    virtual void OnPreferenceChanged(const String& key);                     // 8/– Mac 0x26d650 (thunk 0x26d780), Win 0x65a344
    // ---- BrickConnectSceneControlObserver: UpdateEnable ----
    virtual void OnAttached(BrickConnectSceneControl* control);              // 9/– Mac 0x26d7a0 (thunk 0x26d7b0), Win 0x65a08d
    virtual void OnEvent19();                                                // 10/– Mac 0x26d7d0 (thunk 0x26d7e0), Win 0x65a098
    // The three observers leave their subjects, then the Command's observers. Mac 0x26dde0
    // (thunks 0x26df30-0x26df90), Win 0x65ab2f
    virtual void OnFinalRelease();

    using FallingBricksObserver::OnAttached;
    using FallingBricksObserver::OnDetached;
    using PreferencesObserver::OnAttached;
    using PreferencesObserver::OnDetached;
    using BrickConnectSceneControlObserver::OnDetached;

private:
    BrickConnectSceneControl*  m_sceneControl;   // +0x40 (Win +0x50)
    RefPointer<FallingBricks>  m_explosion;      // +0x44
    RefPointer<FallingBricks>  m_assembly;       // +0x48
    RefPointer<SimpleSound>    m_sound;          // +0x4c "sound_explosion.mp3" (null while sounds are off)
                                                 // Object +0x50
};

} // namespace LEGO
