// LEGO Digital Designer — restored source
// LEGO::PasteCommand — pastes the clip board into the click-stick tool ("paste" in
//   LDD.commands.editMode, created by EditMode::Initialize, TU 085): the tool becomes the scene
//   manager's active tool and picks the clip board's element bricks up. Enabled while the clip
//   board has bricks and a brick palette is observed (it follows the scene manager's scene
//   control's palette like NewCommand).
//
// TU 249 "PasteCommand.cpp": Mac 0x273380-0x27454b, Win 0x66e587-0x66e7e0. The TU also emits
// Observer<BricksClipBoard, BricksClipBoardObserver>::Attach (Mac 0x273380, Notifier.h), the
// BricksClipBoardObserver / Observer<BricksClipBoard> vtable bodies (Mac 0x2739e0-0x273c90,
// BricksClipBoard.h) and the BasicArray<CommandObserver*> base-destructor helper Mac 0x274380;
// Mac 0x2744d0 = <iostream> + a header static (not code).
// RTTI: vmi {Command +0, BricksClipBoardObserver (Mac +0x28), BrickPaletteObserver (Mac +0x30),
//   BrickConnectSceneManagerObserver (Mac +0x38)}, virtual base Object. Mac vtable 0x739a9c {D1
//   0x273cf0, D0 0x273d50, OnFinalRelease 0x273dc0, Do 0x273760, SetEnable, GetEnable,
//   UpdateEnable 0x2737b0, the primary entries 0x273ec0-0x273f00 (clip board OnAttached /
//   OnDetached / OnClipBoardChanged, palette OnAttached / OnDetached: all UpdateEnable),
//   OnAttached(scene manager) 0x2737f0, OnDetached 0x273870, OnSceneControlChanged 0x273910};
//   clip-board-in 0x739ae4 {3-5: 0x274000 / 0x274020 / 0x274040}, palette-in 0x739b08 {3:
//   0x274150, 4: 0x274170}, scene-manager-in 0x739b3c {3: 0x273830, 4: 0x2738c0, 5: 0x273970}.
//   Win 0x9d86f8 {Do 0x66e5ef, …, UpdateEnable 0x66e587}; clip-board-in 0x9d86e8 {0-2: 0x631d53};
//   palette-in 0x9d86c8 {0, 1: 0x66e70e}; scene-manager-in 0x9d86b4 {0x676911, 0x66e61e,
//   0x66e804}.
// Layout (Mac): Command +0x00..+0x28, clip board observer +0x28 (subject +0x2c), palette observer
//   +0x30 (subject +0x34), scene-manager observer +0x38 (subject +0x3c), click-stick tool +0x40
//   (referenced), Object +0x44 (sizeof 0x4c). Win: Object +0x58.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BricksClipBoard.h"
#include "BrickPalette.h"
#include "BrickConnectSceneManager.h"
#include "ClickStickTool.h"

namespace LEGO {

class BrickConnectSceneControl;

class PasteCommand : public Command,
                     private BricksClipBoardObserver,
                     private BrickPaletteObserver,
                     private BrickConnectSceneManagerObserver {
public:
    // Disabled; references `clickStickTool`, observes `clipBoard` and `sceneManager` (each
    // attachment re-evaluates the enable state). Mac 0x2735d0, Win 0x66e629
    PasteCommand(BrickConnectSceneManager* sceneManager, ClickStickTool* clickStickTool,
                 BricksClipBoard* clipBoard);
    virtual ~PasteCommand();                                                // Mac 0x273cf0/0x273d50, Win sdd 0x66e7c1

    // While enabled: the click-stick tool becomes the active tool and picks the clip board's
    // bricks up (flags 0, 0, 1, 0).
    virtual void Do();                                                      // 3/0 Mac 0x273760, Win 0x66e5ef
    // Enabled while a clip board and a palette are observed and the clip board has bricks.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x2737b0, Win 0x66e587

protected:
    // ---- BricksClipBoardObserver / BrickPaletteObserver: each one UpdateEnable ----
    virtual void OnAttached(BricksClipBoard* clipBoard);                    // 7  Mac 0x273ec0 (thunk 0x274000), Win 0x631d53
    virtual void OnDetached(BricksClipBoard* clipBoard);                    // 8  Mac 0x273ed0 (thunk 0x274020), Win 0x631d53
    virtual void OnClipBoardChanged(bool empty);                            // 9  Mac 0x273ee0 (thunk 0x274040), Win 0x631d53
    virtual void OnAttached(BrickPalette* palette);                         // 10 Mac 0x273ef0 (thunk 0x274150), Win 0x66e70e
    virtual void OnDetached(BrickPalette* palette);                         // 11 Mac 0x273f00 (thunk 0x274170), Win 0x66e70e
    // ---- BrickConnectSceneManagerObserver: follow the scene control's palette ----
    virtual void OnAttached(BrickConnectSceneManager* sceneManager);        // 12 Mac 0x2737f0 (thunk 0x273830), Win 0x676911
    virtual void OnDetached(BrickConnectSceneManager* sceneManager);        // 13 Mac 0x273870 (thunk 0x2738c0), Win 0x66e61e
    virtual void OnSceneControlChanged(BrickConnectSceneControl* previous,
                                       BrickConnectSceneControl* current);  // 14 Mac 0x273910 (thunk 0x273970), Win 0x66e804
    // The three observer parts (scene manager, palette, clip board), then NotifierT<Command>'s.
    // Mac 0x273dc0, Win 0x66e7e0
    virtual void OnFinalRelease();

    RefPointer<ClickStickTool> m_clickStickTool;   // Mac +0x40
};

} // namespace LEGO
