// LEGO Digital Designer — restored source
// LEGO::ButtonControl — a RenderControl with four states (up, over, down, disabled) following
// the mouse and the enabled state; each shows an entry of a state-description table (image +
// rect, colour, transparency, font, text, cursor). A state map (state -> index) lets subclasses
// switch tables (ToggleControl: entries 4-7 while toggled). Image and text may go to other
// RenderControls (m_imageTarget, m_textTarget; null = the button).
// LEGO::NonTransparentSelectionMask — a button's hit-test mask: every point is inside.
// TU 028 "ButtonControl.cpp" (Mac 0xeb1e0-0xee96b; Win 0x516111-0x516b71); also emits inline
// RenderControl and TextLayoutObserver / TextureObserver bodies (RenderControl.h). Directory UI/
// (no __FILE__ evidence).
//
// Layout (Mac / Win): RenderControl +0x000..+0x1ec / +0x208,
//   +0x1ec / +0x208 BasicArray<StateDescription> m_states   (0x40 / 0x34 bytes per entry)
//   +0x1fc / +0x218 m_state, +0x200 / +0x21c m_stateMap[4] {0, 1, 2, 3},
//   +0x210 / +0x22c m_imageTarget, +0x214 / +0x230 m_textTarget, Object +0x218 / +0x238.
//   Mac sizeof 0x220, Win 0x240.
// Virtual slots: Mac vtable 0x70762c (48 entries), Win 0x9d1454 (43 entries = Mac 3..47 without
//   the two Mac-only overrider entries 43/44). Overrides: OnHidden (11/8), OnEnabled (12/9),
//   OnDisabled (13/10), OnMouseEnter (16/13), OnMouseLeave (17/14), OnMousePressed (19/16),
//   OnMouseReleased (20/17), Initialize (38/35). New: Create (45, 46; Win 41, 40 — an overload
//   pair, MSVC reverses its order), UpdateState (47/42).
// Names: classes from RTTI, "up/over/down/disabled" from the image suffixes; the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "Color.h"
#include "BasicArray.h"
#include "BitmapSelectionMask.h"
#include "RenderControl.h"

namespace LEGO {

class Font;
class Texture;

// ===========================================================================================
// NonTransparentSelectionMask — Mac vtable 0x762fd0 {GetSize 0xed2c0, Contains 0xed2e0,
// D1 0xed2f0, D0 0xed310}; Win 0xa289ec {GetSize 0x516111, Contains 0x51611f}, ctor 0x516124.
// Mac sizeof 0xc (vptr + Object), Win 0x10.
// ===========================================================================================
class NonTransparentSelectionMask : public SelectionMask {
public:
    NonTransparentSelectionMask() {}                                  // Win 0x516124 (Mac inlined)
    virtual ~NonTransparentSelectionMask() {}                         // Mac 0xed2f0/0xed310, Win 0x51617f/0x5161c2

    // (0, 0): the mask has no extent of its own.
    virtual Vector2 GetSize() const;                                  // Mac 0xed2c0, Win 0x516111
    // Every point is inside.
    virtual bool Contains(const Vector2& point) const;                // Mac 0xed2e0, Win 0x51611f
};

// ===========================================================================================
// ButtonControl
// ===========================================================================================
class ButtonControl : public RenderControl {
public:
    // The visual states (m_state). (names inferred from the image suffixes)
    enum State {
        STATE_UP       = 0,
        STATE_OVER     = 1,
        STATE_DOWN     = 2,
        STATE_DISABLED = 3
    };

    // One state-table entry (Mac 0x40 bytes, Win 0x34). Copy / assignment are the compiler's (Mac
    // 0xebc50 inlines the member-wise assignment).
    struct StateDescription {
        // No image, empty rectangle, white, transparent, no font, no text, cursor 1.
        StateDescription();                                           // Mac 0xed1e0, Win 0x5161ec
        StateDescription(const StateDescription& other);
        StateDescription& operator=(const StateDescription& other);
        ~StateDescription();                                          // Mac 0xeddc0 (template helper)

        RefPointer<Texture> m_image;        // +0x00 / +0x00 RenderControl::SetImage
        Rect                m_imageRect;    // +0x04 / +0x04 texture rectangle of the image
        Color               m_color;        // +0x14 / +0x14 BufferedAbleRenderControl::SetColor
        bool                m_transparent;  // +0x24 / +0x24 RenderControl::SetTransparent
        RefPointer<Font>    m_font;         // +0x28 / +0x28 RenderControl::SetFont
        String              m_text;         // +0x2c / +0x2c RenderControl::SetText
        UInt32              m_cursor;       // +0x3c / +0x30 Control::SetCursor (1 arrow, 2 hand …)
    };

    explicit ButtonControl(ControlManager* manager);                  // Mac 0xeb560 (C2), Win 0x51631d
    virtual ~ButtonControl();                                         // Mac 0xece40/0xecea0, Win 0x516255 (sdd 0x5162e5)

    // ---- Control overrides (Mac slot / Win slot): the state machine ----
    // Up when enabled.
    virtual void OnHidden();                                          // 11/8  Mac 0xec0d0, Win 0x5168e5
    // Over when the mouse is inside, else up.
    virtual void OnEnabled();                                         // 12/9  Mac 0xec110, Win 0x516900
    virtual void OnDisabled();                                        // 13/10 Mac 0xec160, Win 0x51691d
    // Down when the left button is held on the button (the press started here), else over
    // (enabled only).
    virtual void OnMouseEnter(const Vector2& position);               // 16/13 Mac 0xec190, Win 0x516930
    // Enabled: up. NOTE: original quirk: a disabled button does not notify its observers.
    virtual void OnMouseLeave(const Vector2& position);               // 17/14 Mac 0xec1f0, Win 0x516960
    virtual void OnMousePressed(const Vector2& position, UInt32 button);  // 19/16 Mac 0xec230, Win 0x516982
    virtual void OnMouseReleased(const Vector2& position, UInt32 button); // 20/17 Mac 0xec270, Win 0x5169a7

    // ---- RenderControl override ----
    // At least four state descriptions (cursors 1, 2, 3, 1), RenderControl::Initialize, the
    // NonTransparentSelectionMask, mouse input, transparent.
    virtual bool Initialize();                                        // 38/35 Mac 0xeb650, Win 0x516421

    // ---- new virtuals ----
    // Initialize, then Control::Setup with the same arguments. (name inferred)
    virtual bool Create(Control* parent, Control* after, UInt32 flags, const Rect& rectangle,
                        UInt32 inputFlags, bool hitTestable);         // 45/41 Mac 0xeb8f0, Win 0x5164e7
    // Create, then SetImages(imageBaseName).
    virtual bool Create(const char* imageBaseName, Control* parent, Control* after, UInt32 flags,
                        const Rect& rectangle, UInt32 inputFlags,
                        bool hitTestable);                            // 46/40 Mac 0xeb920, Win 0x516517
    // Shows the current state's description (image, colour, transparency on the image target; font,
    // text on the text target; the cursor). NOTE: original quirk: without a font in the description
    // and on the text target, text and cursor are left as they are.
    virtual void UpdateState();                                       // 47/42 Mac 0xebf70, Win 0x516805

    // ---- non-virtual ----
    // Images "<name>.up", ".over", ".down", ".disabled" from the ImageLoader into descriptions
    // 0-3 (image + rectangle), then UpdateState. (name inferred)
    void SetImages(const char* imageBaseName);                        // Mac 0xeb990, Win 0x5165da
    // Replaces description `index`; UpdateState when the current state shows it.
    void SetStateDescription(UInt32 index, const StateDescription& description); // Mac 0xebc50, Win 0x51654c (+ 0x516588)
    StateDescription& GetStateDescription(UInt32 index);              // Mac 0xebd70 (Win inlined)
    // Copies the table into `descriptions`.
    void GetStateDescriptions(BasicArray<StateDescription>& descriptions) const; // Mac 0xebd90 (Win 0x516ac0 inner)
    // Replaces the table, then UpdateState.
    void SetStateDescriptions(const BasicArray<StateDescription>& descriptions); // Mac 0xebde0
    UInt32 GetStateDescriptionCount() const { return m_states.GetSize(); }
    // The same text / font in every description, then UpdateState (names inferred; not the
    // RenderControl virtuals SetText / SetFont).
    // NOTE: original quirk: SetFontForAllStates expects a font (AddRef without a null check).
    void SetTextForAllStates(const String& text);                     // Mac 0xebe30, Win 0x51675a
    void SetFontForAllStates(Font* font);                             // Mac 0xebe80, Win 0x516790
    // Which description state `state` shows; UpdateState when it is the current state.
    void SetStateMapping(UInt32 state, UInt32 index);                 // Mac 0xebf00, Win 0x5167c6
    UInt32 GetStateMapping(UInt32 state) const;                       // Mac 0xebf40 (Win inlined)
    UInt32 GetState() const;                                          // Mac 0xebf60 (Win inlined)
    // Other controls that show the image / the text (null: the button). (names inferred; inline in
    // the original, used by subclasses)
    void SetImageTarget(RenderControl* target) { m_imageTarget = target; }
    void SetTextTarget(RenderControl* target) { m_textTarget = target; }
    RenderControl* GetImageTarget() const { return m_imageTarget; }
    RenderControl* GetTextTarget() const { return m_textTarget; }

protected:
    // Changes the state and calls UpdateState when it differs. Win 0x5167f0 (Mac inlined)
    void SetState(UInt32 state);
    // At least `count` descriptions (inlined in Initialize, Mac 0xeb650 / 0x1a5df0).
    void EnsureStateDescriptions(UInt32 count);

    // ---- data (Mac offset / Win offset) ----
    BasicArray<StateDescription> m_states;          // +0x1ec / +0x208
    UInt32                       m_state;           // +0x1fc / +0x218 (STATE_DISABLED initially)
    UInt32                       m_stateMap[4];     // +0x200 / +0x21c state -> description
    RefPointer<RenderControl>    m_imageTarget;     // +0x210 / +0x22c
    RefPointer<RenderControl>    m_textTarget;      // +0x214 / +0x230
                                                    // Object +0x218 / +0x238
};

} // namespace LEGO
