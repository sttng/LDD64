// LEGO Digital Designer — restored source
// LEGO::RenderControlManager — the ControlManager of a window drawn with a Render (the main
// window's OpenGLRender), and the window's ModalUpdateReciever (redraws during modal loops). Its
// object factory forwards Texture / VertexBuffer / Font / TextLayoutRender creation to the
// render's (ProxyFactory<T>); it keeps the quad and line vertex buffers of ControlDrawArgs.
// RenderControlManager::RenderRoot — layer 0: a buffered root that draws to the screen.
// Home TU 065 "RenderControl.cpp" (Mac 0x1523d0-0x15811b; Win 0x50ceae-0x50fa32, GetRender
// 0x51cc2a), bodies there. Created at start-up (Mac 0x13a1c0: new + ctor 0x152ca0). Directory UI/
// (guess).
//
// Mac layout (ctor 0x152ca0): ControlManager +0x00..+0xd4, ModalUpdateReciever vptr +0xd4
//   (stored into the window's m_modalUpdateReciever, Window +0xb0), RefPointer<Render> +0xd8,
//   quad +0xdc, lines +0xe0, a fourth reference +0xe4 (never set; released by the dtor), Object
//   +0xe8 (Object-in vtable 0x70de90).
// Win (ctor 0x50d798): ModalUpdateReciever +0xec, render +0xf0, quad +0xf4, lines +0xf8, Object
//   +0xfc (no fourth reference).
// Virtual slots: Mac 0x70dd4c (40 entries) = ControlManager's 38 + GetRender (38) + the
//   ModalUpdate overrider (39, Mac-only primary entry). Win 0xa2819c (35 entries):
//   GetRender = Win slot 34. Overrides: OnPaint (Mac 18 / Win 16), Initialize (Mac 26 /
//   Win 23), Update (Mac 27; Win in the UpdateObserver vtable), CreateLayer (Mac 28 / Win 24).
// GetRootControl and IsUpdateLocked (for BrickConnectSceneManager TU 022 and
// BrickConnectSceneControl's render path 021e) are inline field reads (ControlManager +0x60 /
// +0xd0) in the original, out of line here so the test seams can define them.
// Class name from RTTI; method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ControlManager.h"

namespace LEGO {

class Render;
class Control;
class VertexBuffer;

class RenderControlManager : public ControlManager, public ModalUpdateReciever {
public:
    class RenderRoot;   // RenderControl.cpp

    // Mac 0x152ca0 (C2 with VTT), Win 0x50d798: the window and its render (referenced); the
    // manager becomes the window's ModalUpdateReciever.
    RenderControlManager(Window* window, Render* render);
    virtual ~RenderControlManager();                                        // Mac 0x156a50/0x156ab0, Win 0x50cec4 (sdd 0x50d05e)

    // ---- ControlManager overrides ----
    // Invalidates the root and draws the control tree with the render (BeginFrame, Draw,
    // EndFrame, Present) when the root is invalid. Mac 0x156b20 (→ 0x153640), Win 0x50ceae (→ 0x50dd05)
    virtual void OnPaint(UInt32 arg);                                       // 18/16
    // ControlManager::Initialize, the four proxy factories, the quad and line buffers.
    virtual bool Initialize();                                              // 26/23 Mac 0x152db0, Win 0x50d895
    // ControlManager::Update, then asks the window to repaint when the root is invalid.
    virtual void Update();                                                  // 27/–  Mac 0x153480, Win 0x50dcb5 (UpdateObserver vtable)
    // Layer 0: RenderRoot; Mac: MacWindowResizeRootControl for layers 1 and 2 (Win: Control).
    virtual void CreateLayer(UInt32 layer);                                 // 28/24 Mac 0x153810, Win 0x50de35

    // ---- new ----
    virtual Render* GetRender() const;                                      // 38/34 Mac 0x156b40, Win 0x51cc2a
    // While a modal loop runs: ask the window to repaint when the root is invalid.
    virtual void ModalUpdate();                                             // 39/–  Mac 0x1535a0 (thunk 0x1535f0), Win 0x50dcde

    // (for BrickConnectSceneControl's render path, Mac 021e / Win 0x615a54)
    // The window's root control (ControlManager layer 0: field Mac +0x60 / Win +0x70, read inline).
    // (name inferred)
    Control* GetRootControl() const;
    // Set while the manager suspends redrawing (ControlManager byte Mac +0xd0 / Win +0xe8, read
    // inline by BrickConnectSceneControl Mac 0x39df0 / Win 0x44b400). (name inferred)
    bool IsUpdateLocked() const;

    VertexBuffer* GetQuadBuffer() const { return m_quad.Get(); }
    VertexBuffer* GetLineBuffer() const { return m_lines.Get(); }

protected:
    // Draws the tree when the root is invalid and has a size. Mac 0x153640, Win 0x50dd05
    void Paint();
    // The root is invalid and has a size. Win 0x50cf68 (inlined on Mac)
    bool NeedsPaint() const;

    // ---- data (Mac offsets) ----
    RefPointer<Render>       m_render;      // +0xd8
    RefPointer<VertexBuffer> m_quad;        // +0xdc
    RefPointer<VertexBuffer> m_lines;       // +0xe0
#if !defined(_WIN32)
    RefPointer<Object>       m_valueE4;     // +0xe4 (Mac only; never set)
#endif
                                            // Object +0xe8
};

} // namespace LEGO
