// LEGO Digital Designer — restored source
// Home TU 063 "LDDUtilities.cpp"; bodies in LDDUtilitiesUI.cpp (NOT restored here).
// LEGO::ExtendedTextTooltipAugmentation — a StandardTextTooltipAugmentation with a short and an
//   extended text; shows the extended one while LDDApplication::IsExtendedTooltip().
//
// Mac: factory 0x14d140 (new 0x60 bytes + constructor 0x14d1a0), vtable 0x7676f4 {GetControl,
// Attach, Detach, D1 0x14faa0, D0 0x14fb00, GetDelay, CreateTooltip 0x207de0, DestroyTooltip,
// UpdateTooltip 0x207fe0, Mac-only entries 0x14fb70, 0x14fbe0}, LDDApplicationObserver-in
// 0x76772c (OnExtendedTooltipChanged 0x14fce0 thunk), Object-in 0x767770.
// Layout (Mac): StandardTextTooltipAugmentation +0x00..+0x30, LDDApplicationObserver +0x30
// (subject +0x34), String standard text +0x38, String extended text +0x48, Object +0x58.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "StandardTextTooltipAugmentation.h"
#include "LDDApplication.h"

namespace LEGO {

class ExtendedTextTooltipAugmentation : public StandardTextTooltipAugmentation,
                                        public LDDApplicationObserver {
public:
    // Keeps both texts, observes the LDDApplication. Mac 0x14d1a0 (factory 0x14d140)
    ExtendedTextTooltipAugmentation(Control* control, const String& text,
                                    const String& extendedText);
    virtual ~ExtendedTextTooltipAugmentation();                             // Mac 0x14faa0/0x14fb00

    const String& GetStandardText() const { return m_standardText; }
    const String& GetExtendedText() const { return m_extendedText; }

    // Sets both texts and shows the one the application selects. Inlined in the Lua binding
    // (Mac 0x2a6340 / 0x2a6490 / 0x2a6790).
    void SetTexts(const String& text, const String& extendedText)
    {
        m_standardText = text;
        m_extendedText = extendedText;
        if (LDDApplication::GetInstance()->IsExtendedTooltip())
            SetText(m_extendedText);
        else
            SetText(m_standardText);
    }

    // Both texts of `other`, then the shown one. Mac slot 9 0x14fb70, Win slot 7 0x638dbf
    // (name inferred)
    virtual void Assign(const ExtendedTextTooltipAugmentation& other);

protected:
    // LDDApplicationObserver: the shown text follows the application.
    // Mac 0x14fbe0 (primary slot 10; thunk 0x14fce0), Win 0x638deb
    virtual void OnExtendedTooltipChanged(bool enabled);

    String m_standardText;   // Mac +0x38
    String m_extendedText;   // Mac +0x48
};

} // namespace LEGO
