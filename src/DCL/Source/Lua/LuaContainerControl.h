// LEGO Digital Designer — restored source
// LEGO::LuaContainerControl — Lua class "ContainerControl" (base Control): a scroll container.
// Attach adds a plain "content rectangle" Control (margins `contentRect`) that holds the content
// control (<Content> or self:SetContent). Changes go to Lua with only the changed coordinates
// (the other nil): OnContentPositionChanged(x, y), OnContentSizeChanged, OnContentScaleChanged,
// OnSizeDifferenceChanged (content rectangle size minus scaled content size), OnContentChanged().
// Mouse wheel: self:OnContentMouseWheel(axis, delta, type), type 0 over the content, 1 over the
// content rectangle (notifyMouseWheelInContentRect), 2 for a control below the content
// (notifyMouseWheelForContentChildren: observes the ControlManager).
// TU 386 "LuaContainerControl.cpp": Mac 0x373a90-0x375dab, Win 0x461560-0x4629cf.
//
// RTTI vmi {LuaControl +0, ControlManagerObserver +0x4c private}, virtual base Object.
// Mac primary vtable 0x751c7c (27 entries: LuaControl's 24 with D1 0x3746c0, D0 0x374950,
// OnLayoutApplied 0x374e30, OnFinalRelease 0x374c40, Attach 0x375320, Detach 0x3754d0,
// OnRectangleChanged 0x3755a0; Mac-only entries 24-26 of the overriders OnScaleChanged
// 0x3756b0, OnMouseWheel 0x375740, OnManagerMouseWheel 0x375890), ControlManagerObserver-in
// 0x751dcc {…, 0x375950, …}.
// Win 0xa22584 {…, OnLayoutApplied 0x461f60, …}, ControlAugmentation-in 0xa225ac {…, Attach
// 0x4621f0, Detach 0x462330}, ControlObserver-in 0xa225bc {…, OnRectangleChanged 0x4623d0,
// OnScaleChanged 0x462450, …, OnMouseWheel 0x462480, …}, ControlManagerObserver-in 0xa2264c
// {…, OnManagerMouseWheel 0x4624d0, …}, Object-in 0xa22698 {sdd 0x462e60 / 0x461560, …}.
// Layout Mac (ctor 0x373a90): LuaControl +0x00..+0x4c, ControlManagerObserver +0x4c (subject
// +0x50), members below, Object +0x84 (sizeof 0x8c). Names inferred unless Lua / layout strings.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "ControlManager.h"
#include "LuaControl.h"

namespace LEGO {

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
class LuaContainerControl : public LuaControl, private ControlManagerObserver {
public:
    LuaContainerControl();                                                  // Mac 0x373a90
    virtual ~LuaContainerControl();                                         // Mac 0x3746c0/0x374950 (D2 0x3745d0), Win sdd 0x461560

    Control* GetContent() const { return m_content.Get(); }
    // The content rectangle control (created by Attach). (name inferred)
    Control* GetContentRect() const { return m_contentRect.Get(); }

    // Moves `content` into the content rectangle (the previous one leaves the tree), observes
    // it, shows it; OnContentChanged and the position / size / scale callbacks.
    // NOTE: `content` must not be null (AddRef without a check, both builds).
    void SetContent(Control* content);                                      // Mac 0x373bf0, Win 0x4615a0
    void SetNotifyMouseWheelInContentRect(bool notify);                     // (inline: Lua function, layout)
    void SetNotifyMouseWheelForContentChildren(bool notify);                // (inline: Lua function, layout)

    bool m_notifyInContentRect;      // Mac +0x7c
    bool m_notifyForContentChildren; // Mac +0x7d

protected:
    // LuaControl's, then <OnContentChanged> … <OnContentMouseWheel>, contentRect,
    // notifyMouseWheelInContentRect, notifyMouseWheelForContentChildren, <Content>.
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x374e30, Win 0x461f60
    // Leaves the manager, then LuaControl's.
    virtual void OnFinalRelease();                                          // Mac 0x374c40 (thunks 0x374ca0 …)
    // LuaControl's; then the content rectangle control.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x375320 (thunk 0x3754b0), Win 0x4621f0
    // Leaves the manager, the content and the content rectangle, then LuaControl's.
    virtual void Detach();                                                  // Mac 0x3754d0 (thunk 0x375580), Win 0x462330

    // ---- ControlObserver ----
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);                // Mac 0x3755a0 (thunk 0x375620), Win 0x4623d0
    virtual void OnScaleChanged(Control* control, const Vector2& oldScale); // Mac 0x3756b0 (thunk 0x3756f0), Win 0x462450
    virtual void OnMouseWheel(Control* control, UInt32 axis, Real32 delta); // Mac 0x375740 (thunk 0x375850), Win 0x462480

    // ---- ControlManagerObserver: the wheel over a control below the content (type 2) ----
    virtual void OnManagerMouseWheel(UInt32 axis, Real32 delta);           // Mac 0x375890 (thunk 0x375950), Win 0x4624d0

    // OnSizeDifferenceChanged. Mac 0x373de0, Win 0x4626c0
    void UpdateSizeDifference();
    // `stored` = `value`; on a change self:name(x or nil, y or nil). Mac 0x375980, Win 0x462520
    void NotifyChange(const Vector2& value, Vector2& stored, const char* name);

private:
    RefPointer<Control> m_contentRect;     // Mac +0x54
    RefPointer<Control> m_content;         // Mac +0x58
    Vector2             m_sizeDifference;  // Mac +0x5c
    Vector2             m_contentPosition; // Mac +0x64
    Vector2             m_contentSize;     // Mac +0x6c
    Vector2             m_contentScale;    // Mac +0x74 (1, 1)

protected:
    UInt32              m_unused80;        // Mac +0x80 0 (not referenced)
};
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// Registers ContainerControl (base Control) {GetContent … GetNotifyMouseWheelForContentChildren,
// Create (upvalue `manager`)}. Mac 0x373ea0, Win 0x461bf0 (name inferred)
void RegisterLuaContainerControl(LuaPlus::LuaState* state, ControlManager* manager);

} // namespace LEGO
