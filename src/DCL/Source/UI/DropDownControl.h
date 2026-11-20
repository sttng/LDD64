// LEGO Digital Designer — restored source
// LEGO::DropDownControl — a drop-down chooser: a label (the choice's text), an arrow button and a
// pop-up of item controls in columns of 8. Label / button toggle the pop-up; an item click
// selects (DropDownControlObserver::OnItemSelected) and closes it, as does a click elsewhere.
// Used by LDDPostProcessor's debug UI (TU 373) and the LDD dialogs.
// TU 169 "DropDownControl.cpp" (Mac 0x229100-0x22b96b; Win 0x658907-0x659103, 0x68b8b2-0x68c280)
// and TU 157 (Mac 0x220600-0x22180b: ctor, DropDownControlObserver / ObserverN template
// instances). Names inferred except RTTI names.
//
// Layout (Mac): RenderControl +0x000..+0x1ec, Notifier<DropDownControlObserver> +0x1ec
//   (observers +0x1f0, iterators +0x200), ControlObserver +0x210 (subjects +0x214),
//   ControlManagerObserver +0x224 (subject +0x228), members below, Object +0x258 (sizeof 0x260).
// Virtual slots: Mac 0x72dd2c (48 = RenderControl's 45 + Create 45 + the primary entries 46/47
//   of the ControlObserver::OnMousePressed / ControlManagerObserver::OnManagerMouseDown
//   overriders, thunks 0x22a520 / 0x22a6d0); Win 0x9d69a4 (41 = 40 + Create 40).
#pragma once
#include "LEGO/Types.h"
#include "RenderControl.h"
#include "ControlManager.h"
#include "Notifier.h"
#include "ObserverN.h"

namespace LEGO {

class DropDownControl;
class ButtonControl;
class String;

// Mac vtable 0x74ff9c {D1 0x220eb0, D0 0x220f00, OnFinalRelease 0x220f70, OnAttached 0x220fd0,
// OnDetached 0x220fe0, OnItemSelected 0x220ff0 (empty)}; Win 0x9d5f88 {…, 0x6943f1}.
class DropDownControlObserver : public ObserverN<DropDownControl, DropDownControlObserver> {
public:
    DropDownControlObserver() {}
    virtual ~DropDownControlObserver() {}                         // Mac 0x220eb0/0x220f00, Win sdd 0x65156c
    // `item` (index `index` of the list) was chosen. (name inferred)
    virtual void OnItemSelected(Control* /*item*/, UInt32 /*index*/) {}
};

class DropDownControl : public RenderControl,
                        public Notifier<DropDownControlObserver>,
                        public ControlObserver,
                        public ControlManagerObserver {
public:
    // `notifyAlways`: OnItemSelected also when the selected item is chosen again.
    // Mac 0x220600 (C1)
    DropDownControl(ControlManager* manager, bool notifyAlways);
    virtual ~DropDownControl();                                    // Mac 0x22abc0/0x22ac20 (body 0x22b3f0), Win sdd 0x659103

    // Builds label, button and pop-up from "Graphics/LDDgraphics01.png" below `parent` at
    // `position` (size = label + button). Mac 0x229100, Win 0x68bc73 (slot 45 / 40)
    virtual bool Create(Control* parent, Control* after, const Vector2& position);

    // Appends an item control to the pop-up (laid out again). Mac 0x22a0a0
    void AddItem(Control* item);
    // The label's text. Mac 0x22a160
    bool SetText(const String& text);
    // Puts the pop-up below another parent (e.g. a top layer). Mac 0x22a190, Win 0x68b8b2
    void SetPopupParent(Control* parent);
    Int32 GetSelectedIndex() const { return m_selected; }
    // (additive; Windows: PreferenceControlDropDown) Appends item controls to the
    // pop-up, laid out once. Win 0x68bbb8 (not emitted on Mac). (name inferred)
    void AddItems(const BasicArray<RefPointer<Control> >& items)
    {
        for (UInt32 i = 0; i < items.GetSize(); ++i) {
            m_items.PushBack(items[i]);
            items[i]->SetParent(m_popup, 0);
            ControlObserver::Attach(items[i].Get());
        }
        LayoutItems();
    }
    // (additive) Selects item `index` and tells the observers (OnItemSelected),
    // also when it was selected already. Win 0x68b9e9 (not emitted on Mac). (name inferred)
    void SelectItem(UInt32 index)
    {
        m_selected = (Int32)index;
        NotifierT<DropDownControl, DropDownControlObserver>::Iterator it(*this);
        while (DropDownControlObserver* observer =
                   it.Next(NotifierT<DropDownControl, DropDownControlObserver>::m_observers))
            observer->OnItemSelected(m_items[m_selected].Get(), (UInt32)m_selected);
    }
    UInt32 GetItemCount() const { return m_items.GetSize(); }

    // ---- secondary-base handlers (Mac primary slots 46 / 47) ----
    // A click on an item selects it; on the label / button / pop-up toggles the pop-up.
    // Mac 0x22a1c0 (thunk 0x22a520), Win 0x68ba5a
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button);
    // A press in the window outside the drop-down and its items closes the pop-up.
    // Mac 0x22a540 (thunk 0x22a6d0), Win 0x68bb22
    virtual void OnManagerMouseDown(const Vector2& position, UInt32 button);
    using RenderControl::OnMousePressed;

protected:
    // Mac 0x22ac90
    virtual void OnFinalRelease();
    // Columns of 8 items of the largest item size, 5 units in; the pop-up fits them + 5.
    // Mac 0x229eb0
    void LayoutItems();

    // ---- data (Mac offsets) ----
    RefPointer<RenderControl>        m_label;          // +0x22c
    RefPointer<ButtonControl>        m_button;         // +0x230
    RefPointer<RenderControl>        m_popup;          // +0x234
    BasicArray<RefPointer<Control> > m_items;          // +0x238
    Int32                            m_selected;       // +0x248 -1
    Vector2                          m_vector24c;      // +0x24c (0, 0) (not used in these TUs)
    bool                             m_notifyAlways;   // +0x254
                                                       // Object +0x258
};

} // namespace LEGO
