// LEGO Digital Designer — restored source
// LEGO::BrickConnectSceneControlObserver — Observer<BrickConnectSceneControl, …>: the
//   notifications of the scene control (the main 3D view) to its tools, LDDApplication, the
//   palettes, the group tree … (18 handlers).
//
// Home: BrickConnectSceneControl (TU 021; the notifier side is its
// Notifier<BrickConnectSceneControlObserver> base). The original most likely declares the
// observer in BrickConnectSceneControl.h. The default bodies (Mac 0x6efc0-0x6f1c0) are inline.
//
// Mac vtable 0x705b9c (23 entries): D1 0x6efc0, D0 0x6eff0, OnFinalRelease 0x6f040, OnAttached
// 0x6f090, OnDetached 0x6f0a0, handlers 0x6f0b0-0x6f1c0 (all empty, slots 5-22); Object-in
// 0x705c10. Win 0x9c96f4 (20 entries: OnAttached, OnDetached, handlers = Mac slot - 3) with the
// argument byte counts of the ICF'd empty bodies (0x65a12a ret 4, 0x5c801e ret 0x10, 0x6943f1
// ret 8, 0x5c8021 ret) — the only evidence for the signatures below.
// NONE of the handler names survived (no Lua binding forwards them); they are numbered by Mac
// slot. Argument types come from TU 021's calls where noted, else UInt32 per stack word.
// Overriders include LDDApplication slot 13 (bool argument, Mac 0x178c20 via thunk 0x178c50),
// BrickConnectSceneControlTool (OnAttached / OnDetached).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"
#include "BasicArray.h"

namespace LEGO {

class BrickConnectSceneControl;
class Part;
class Brick;
class Matrix3D;
class Render;
class RenderQueue;
struct ControlDrawArgs;
struct StorageScene2;

class BrickConnectSceneControlObserver
    : public Observer<BrickConnectSceneControl, BrickConnectSceneControlObserver> {
public:
    // NOTE: constructor implicit and destructor out of line (BrickConnectSceneControl.cpp; both
    // inline in the original): an inline one instantiates Observer<BrickConnectSceneControl, …>,
    // which needs the full BrickConnectSceneControl (a NotifierT).
    virtual ~BrickConnectSceneControlObserver();                            // Mac 0x6efc0/0x6eff0

    // (Mac slot / Win slot; Win argument words) — names unknown
    // (types from Draw Mac 0x49440) the frame: the tools queue their meshes
    // (transform = identity; the overlay queue; the scene queue)
    virtual void OnEvent05(const Matrix3D&, Render*, RenderQueue*, RenderQueue*) {} // 5/2 4 words Mac 0x6f0b0
    // the queues were flushed
    virtual void OnEvent06(Render*) {}                                      // 6/3   1        Mac 0x6f0c0
    // after the 2D state is set up (ControlDrawArgs::Setup(false))
    virtual void OnEvent07(Render*, ControlDrawArgs&) {}                    // 7/4   2        Mac 0x6f0d0
    virtual void OnEvent08() {}                                             // 8/5   0        Mac 0x6f0e0
    virtual void OnEvent09(UInt32) {}                                       // 9/6   1        Mac 0x6f0f0
    // a model starts loading (OnLoadStarted Mac 0x33e90)
    virtual void OnEvent10(StorageScene2*) {}                               // 10/7  1        Mac 0x6f100
    // the load finished; success (OnLoadFinished Mac 0x346c0)
    virtual void OnEvent11(StorageScene2*, bool) {}                         // 11/8  2        Mac 0x6f110
    virtual void OnEvent12() {}                                             // 12/9  0        Mac 0x6f120
    virtual void OnEvent13(bool) {}                                         // 13/10 1        Mac 0x6f130 (LDDApplication: bool)
    // the selection became empty (true) (ClearSelection Mac 0x36a40, RemoveBricks 0x36040)
    virtual void OnEvent14(bool) {}                                         // 14/11 1        Mac 0x6f140
    // the selection changed (the control's selection array)
    virtual void OnEvent15(const BasicArray<RefPointer<Part> >&) {}         // 15/12 1        Mac 0x6f150
    // the missing-brick counts changed (UpdateMissingBricks Mac 0x3cf20)
    virtual void OnEvent16() {}                                             // 16/13 0        Mac 0x6f160
    // bricks whose palette brick changed (SetPartDescriptions 0x41010)
    virtual void OnEvent17(const BasicArray<RefPointer<Brick> >&) {}        // 17/14 1        Mac 0x6f170
    virtual void OnEvent18() {}                                             // 18/15 0        Mac 0x6f180
    virtual void OnEvent19() {}                                             // 19/16 0        Mac 0x6f190
    virtual void OnEvent20() {}                                             // 20/17 0        Mac 0x6f1a0
    // the tool cursor (SetToolCursor Mac 0x42ac0)
    virtual void OnEvent21(UInt32 /*cursor*/, bool /*flag*/) {}             // 21/18 2        Mac 0x6f1b0
    // bricks were hidden (1: HideBricks 0x3ee00) or shown (0: ShowBricks 0x3f770, ShowAllBricks)
    virtual void OnEvent22(UInt32) {}                                       // 22/19 1        Mac 0x6f1c0
    // fixed (M8, not in the original): a save or export is about to read the scene
    // (BrickConnectSceneControl::ClearToolPreviews): a tool undoes the preview it set on the
    // parts (SurfacePaintBucketTool's hover colour).
    virtual void OnClearPreview() {}
};

} // namespace LEGO
