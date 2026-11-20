// LEGO Digital Designer — restored source
// LEGO::ViewMode — the model viewer mode: the scene against an animated skybox (LDDSkybox, with
//   its background sound), no tool, no grid; commands "changeBackground", "deconstruction" and,
//   when the database knows primitive 3 (the ball), "playBall" (LDD.commands.viewMode); the Lua
//   "viewMode":Create() builds the layout. Enter takes a physics snapshot, Exit puts it back
//   when the ball game moved the bricks.
//   The ball game (PlayBall): six static walls around the model, a gravity solver, and a ball
//   (primitive 3) for every ball shooter (designs 41533 / 43121); the world is stepped from the
//   control manager's update (at most every 5 ms), not while a modal control is up. Physics:
//   CreateBrickPhysics (the headless test physics never moves). NOTE: the shipped
//   program cannot reach the ball game (Developer menu only, db.lif 4.3.11 has no
//   primitive 3).
// LEGO::ChangeBackgroundCommand, LEGO::ViewMode::PlayBallCommand — the two commands of the TU.
//
// TU 115 "ViewMode.cpp" (Mac 0x1cd6b0-0x1d208b; Win 0x622ea4-0x624749, 0x659aa0-0x659c73). It
// also holds template instances: ObserverN<SimpleSoundChangedNotifier, …> (Mac 0x1cff30,
// 0x1d1700-0x1d1890, Win sdd 0x659c73; Attach Mac 0x1ce6c0 = Win 0x6234cf, Detach Mac 0x1cee00 =
// Win 0x623915), NotifierT<SimpleSoundChangedNotifier, …>::RemoveObserver (Mac 0x1d1b90 /
// 0x1d1d80), BasicArray<PhysicsSceneryElement*> (Mac 0x1cf440 / 0x1d1b60, Win 0x623010 / sdd
// 0x623025), BasicArray<PhysicsPlacedConfiguredGeometry> (PushBack Mac 0x1d1370 = Win 0x623402,
// destructors 0x1d1480 / 0x1d1f80, Win 0x6233ed / sdd 0x62347b), BasicArray<SimpleSoundChanged-
// Notifier*> (Mac 0x1d1fb0 / 0x1d1fe0, Win sdd 0x659aa0) and the NotifierT<Command> parts of the
// two commands' destructors (Mac 0x1d1560 / 0x1d1660 / 0x1d1a40 / 0x1d1ac0).
//
// RTTI bases (Mac offsets): BrickMode +0x000, BrickConnectSceneControlObserver +0x280 (subject
//   +0x284), UpdateObserver +0x288 (subject +0x28c), SimpleSoundChangedObserver +0x290 (subjects
//   +0x294), ControlManagerObserver +0x2a4 (subject +0x2a8), members +0x2ac..+0x2e4 below,
//   virtual base Object +0x2e4 (sizeof 0x2ec: AppMain's operator new). Win: observers +0x2a0 /
//   +0x2ac / +0x2b8 / +0x2d0, members = Mac + 0x30 (skybox +0x2dc …), Object +0x31c (new 0x328).
// Virtual slots: Mac vtable 0x72013c (53 entries) = BrickMode's 50 with OnFinalRelease 2 =
//   0x1cfdb0, Enter 45 = 0x1ce0c0, Exit 46 = 0x1cebb0, then the primary entries of the secondary-
//   base overriders: 50 Update 0x1cefe0, 51 OnSoundChanged 0x1d00a0, 52 OnManagerModalControl
//   0x1d0180; BCSC-observer-in 0x720310 (no override), UpdateObserver-in 0x720378 {5: thunk
//   0x1cf0a0}, SimpleSoundChangedObserver-in 0x72039c {5: thunk 0x1d0150}, ControlManager-
//   Observer-in 0x7203c0 {17: thunk 0x1d0260}, Object-in 0x72042c. Win 0x9cfca4 (44: Enter
//   0x623e74, Exit 0x624076), UpdateObserver-in 0x9cfb98 {2: 0x622ea4}, SimpleSound-in 0x9cfb88
//   {2: 0x622fb6}, ControlManagerObserver-in 0x9cfb3c {14: 0x624190}, Object-in 0x9cfb24 {sdd
//   0x623278 -> 0x6234ad, OnFinalRelease 0x62474a -> 0x624136}.
// Class names from RTTI; method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Geometry.h"
#include "Updater.h"
#include "Command.h"
#include "ControlManager.h"
#include "BrickMode.h"
#include "BrickPhysics.h"
#include "SimpleSound.h"
#include "SimpleSoundChangedNotifier.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class EditMode;
class BGAutomaticMode;
class LDDSkybox;
class DeconstructionCommand;
class GenerateBuildingGuideCommand;
class Primitive;
class Render;

// ===========================================================================================
// ChangeBackgroundCommand — the next skybox (BrickConnectSceneControl::NextSkybox).
// Mac vtable 0x76ec10 {D1 0x1d1910, D0 0x1d1980, OnFinalRelease (NotifierT), Do 0x1d1a00,
// SetEnable, GetEnable, UpdateEnable 0x1d1a20}, Object-in 0x76ec44; Win 0x9cfd94 {Do 0x623591,
// SetEnable, GetEnable, UpdateEnable 0x630465 (folded)}, Object-in 0x9cfd80 {sdd 0x6235a0 ->
// 0x623986}. Layout: Command +0x00, scene control +0x28 (Win +0x2c), Object +0x2c (Mac new 0x34).
// ===========================================================================================
class ChangeBackgroundCommand : public Command {
public:
    // Mac inline in ViewMode::Initialize (0x1cda31), Win 0x62351b (the enable flag is an
    // argument there; ViewMode passes true)
    explicit ChangeBackgroundCommand(BrickConnectSceneControl* sceneControl, bool enabled = true)
        : Command(enabled, false), m_sceneControl(sceneControl) {}
    virtual ~ChangeBackgroundCommand();                                      // Mac 0x1d1910/0x1d1980, Win sdd 0x623986

    // NextSkybox while enabled.
    virtual void Do();                                                       // 3/0 Mac 0x1d1a00, Win 0x623591
    virtual void UpdateEnable();                                             // 6/3 Mac 0x1d1a20 (SetEnable(true))

private:
    BrickConnectSceneControl* m_sceneControl;   // +0x28
};

class ViewMode : public BrickMode,
                 public BrickConnectSceneControlObserver,
                 public UpdateObserver,
                 public SimpleSoundChangedObserver,
                 public ControlManagerObserver {
public:
    // =======================================================================================
    // PlayBallCommand — ViewMode::PlayBall. Mac vtable 0x720550 {D1 0x1d14f0, D0 0x1d15e0,
    // OnFinalRelease, Do 0x1d14d0, SetEnable, GetEnable, UpdateEnable 0x1d14b0}; Win 0x9cfdc4
    // {Do 0x624742, …, UpdateEnable 0x630465}, Object-in 0x9cfdb0 {sdd 0x6238ca -> 0x624723}.
    // Layout: Command +0x00, mode +0x28, Object +0x2c (Mac new 0x34).
    // =======================================================================================
    class PlayBallCommand : public Command {
    public:
        // Mac inline in Initialize (0x1cdc71), Win 0x623854 (enable flag as for ChangeBackground)
        explicit PlayBallCommand(ViewMode* mode, bool enabled = true)
            : Command(enabled, false), m_mode(mode) {}
        virtual ~PlayBallCommand();                                          // Mac 0x1d14f0/0x1d15e0, Win sdd 0x624723

        virtual void Do();                                                   // 3/0 Mac 0x1d14d0, Win 0x624742
        virtual void UpdateEnable();                                         // 6/3 Mac 0x1d14b0 (SetEnable(true))

    private:
        ViewMode* m_mode;   // +0x28 (not referenced)
    };

    // Mac 0x1cd6b0 (C1 with VTT), Win 0x62309c (same arguments as BrickMode)
    ViewMode(ControlManager* manager, Window* window, BrickConnectSceneControl* sceneControl);
    virtual ~ViewMode();                                                     // Mac 0x1cf470/0x1cf8c0 (D2 0x1cf160), Win 0x623280 / sdd 0x6234ad

    // BrickMode::Create below `parent`, then the commands (navigation to `editMode`, the BG
    // mode command of `bgMode`, the upload and building guide commands; NOTE: as in EditMode the
    // BG mode, view navigation and upload commands are made and dropped again, both builds),
    // LDD.commands.viewMode and the Lua "viewMode":Create(). False when a step fails (AppMain:
    // error 0xc "Could not load assets, try restarting LDD"). Mac 0x1cd8b0, Win 0x6239a5
    // (name inferred)
    bool Initialize(EditMode* editMode, BGAutomaticMode* bgMode, Control* parent, Control* after);
    using BrickMode::Initialize;

    // The ball game: the arena and the solver the first time, then a ball for every shooter of
    // the scene. Nothing when the database has no primitive 3. Mac 0x1d0290, Win 0x6241f9
    void PlayBall();
    // Ends the ball game: no more updates, solver stopped, arena and balls removed.
    // Mac 0x1cece0, Win 0x623df6 (+ the scenery removal 0x62338b)
    void StopPlayBall();

    // ---- Mode ----
    // The skybox (created the first time) and its sound, no grid, no tool, wires unselected,
    // physics snapshot, the application's edit and building guide commands; observes the scene
    // control (layout) and the control manager (modal controls); BrickMode::Enter.
    virtual void Enter();                                                    // 45/40 Mac 0x1ce0c0, Win 0x623e74
    // Stops the deconstruction, the sound and the ball game, puts the snapshot back when the
    // ball game ran, leaves what Enter observed; BrickMode::Exit.
    virtual void Exit();                                                     // 46/41 Mac 0x1cebb0, Win 0x624076

    // ---- UpdateObserver (attached while the ball game runs): physics step ----
    virtual void Update();                                                   // 50/– Mac 0x1cefe0 (thunk 0x1cf0a0), Win 0x622ea4
    // ---- SimpleSoundChangedObserver: the skybox's sound changed -> play it ----
    virtual void OnSoundChanged(const RefPointer<SimpleSound>& sound, bool play); // 51/– Mac 0x1d00a0 (thunk 0x1d0150), Win 0x622fb6
    // ---- ControlManagerObserver: no physics while a modal control is up ----
    virtual void OnManagerModalControl(bool modal, Control* control);        // 52/– Mac 0x1d0180 (thunk 0x1d0260), Win 0x624190

    // ---- accessors (names inferred) ----
    LDDSkybox*             GetSkybox() const { return m_skybox; }
    DeconstructionCommand* GetDeconstructionCommand() const { return m_deconstruction; }
    PhysicsSnapshot*       GetPhysicsSnapshot() const { return m_snapshot; }
    bool                   IsPlayingBall() const { return m_playingBall; }
    bool                   GetRestoreSnapshot() const { return m_restoreSnapshot; }
    SimpleSound*           GetSound() const { return m_sound; }
    Real32                 GetField2c4() const { return m_field2c4; }
    const BasicArray<PhysicsSceneryElement*>& GetSceneryElements() const { return m_sceneryElements; }

protected:
    // Exit when current, the snapshot released, the four observers leave, BrickMode's part.
    virtual void OnFinalRelease();                                           // 2/– Mac 0x1cfdb0 (thunks 0x1cff80-0x1d0080), Win 0x624136

private:
    // A ball: a primitive element of `primitive` at `transform` (the scene's BrickConnect) gives
    // the collision geometry of a dynamic scenery element (kept in m_sceneryElements; mass
    // properties of the bone), drawn with the primitive's meshes in `material`. Null when the
    // element or the scenery element cannot be made. Mac 0x1d0d10, Win 0x6235fe (name inferred)
    PhysicsSceneryElement* AddBall(const Primitive* primitive, const Matrix3D& transform, UInt32 material);
    // The renderer of the control manager (RenderControlManager::GetRender, manager slot 38).
    Render* GetRender() const;

    // ---- data (Mac offsets; Win + 0x30) ----
    RefPointer<LDDSkybox>              m_skybox;           // +0x2ac
    RefPointer<DeconstructionCommand>  m_deconstruction;   // +0x2b0
    PhysicsSnapshot*                   m_snapshot;         // +0x2b4 one reference (Enter .. Exit)
    bool                               m_restoreSnapshot;  // +0x2b8 the ball game moved the model
    bool                               m_playingBall;      // +0x2b9
    Int64                              m_lastUpdate;       // +0x2bc ms of the last physics step
    Real32                             m_field2c4;         // +0x2c4 0.01 (not used)
    RefPointer<SimpleSound>            m_sound;            // +0x2c8 the skybox's sound
    RefPointer<Command>                m_editNavigation;   // +0x2cc ModeNavigationCommand(editMode)
    RefPointer<GenerateBuildingGuideCommand> m_buildingGuide; // +0x2d0
    BasicArray<PhysicsSceneryElement*> m_sceneryElements;  // +0x2d4 arena and balls (one reference each)
                                                           // Object +0x2e4
};

} // namespace LEGO
