// LEGO Digital Designer — restored source
// LEGO::DialControl — a rotary knob: one of four state images (normal, over, pressed, disabled)
// drawn rotated around a centre point; dragging with the left button turns it (angle from the
// control's centre to the mouse, 0 = up, clockwise, [0, 2 pi)); DialControlObservers hear every
// change. Used by HingeTool (TU 056).
// TU 295 "DialControl.cpp" (Mac 0x2a3450-0x2a59db; Win 0x51b310-0x51bf06); DialControlObserver's
// out-of-line members are emitted with HingeTool (Mac 0x127fd0-0x1280b0, Win 0x6803af-0x6829d0).
// Names inferred except RTTI names (DialControl::StateDescription, DialControlObserver).
//
// Layout (Mac): Control +0x000..+0x158, Notifier<DialControlObserver> +0x158 (vptr, observers
//   +0x15c, iterators +0x16c), members below, Object +0x24c (sizeof 0x254, operator new 0x254 in
//   HingeTool 0x11d900). Win: image size +0x194, transform +0x1bc, inverse +0x1fc, state +0x24c.
// Virtual slots: Mac 0x743cdc (36 = Control's) overriding IsInside (3), OnRectangleChanged (8),
//   OnHidden (11), OnEnabled (12), OnDisabled (13), OnMouseMove (18), OnMousePressed (19),
//   OnMouseReleased (20), Draw (33); Win 0xa28d3c (Mac slot - 3).
#pragma once
#include "LEGO/Types.h"
#include "Control.h"
#include "Geometry.h"
#include "Notifier.h"
#include "ObserverN.h"
#include "Texture.h"

namespace LEGO {

class DialControl;

// Mac vtable 0x70c10c {D1 0x127fd0, D0 0x128020, OnFinalRelease 0x127cf0, OnAttached 0x128090,
// OnDetached 0x1280a0, OnAngleChanged 0x1280b0 (empty)}; Win 0x9dbb70 {OnAttached, OnDetached,
// OnAngleChanged 0x6803af}.
class DialControlObserver : public ObserverN<DialControl, DialControlObserver> {
public:
    DialControlObserver() {}
    virtual ~DialControlObserver() {}
    // The dial turned from `oldAngle` to `angle` (radians). (name inferred)
    virtual void OnAngleChanged(DialControl* /*dial*/, Real32 /*angle*/, Real32 /*oldAngle*/) {}
};

class DialControl : public Control, public Notifier<DialControlObserver> {
public:
    // One state's look (0x10 bytes; RTTI BasicArray<DialControl::StateDescription>): the image, the
    // top-left corner of the dial picture in it (the dial's image size) and the cursor shape.
    // Default: no image, (0, 0), cursor 1.
    struct StateDescription {
        StateDescription() : m_cursor(1) {}
        RefPointer<Texture> m_image;    // +0x00
        Vector2             m_offset;   // +0x04
        UInt32              m_cursor;   // +0x0c
    };
    // m_state values (the description used for each is m_stateDescriptionIndex[state]).
    enum State {
        StateNormal   = 0,
        StateOver     = 1,   // mouse over the dial image, or dragging
        StatePressed  = 2,   // just pressed
        StateDisabled = 3
    };

    explicit DialControl(ControlManager* manager);                // Mac 0x2a3450
    virtual ~DialControl();                                       // Mac 0x2a4340/0x2a4480, Win sdd 0x51b310

    // Four default state descriptions (cursors 1, 2, 3, 1), image size and rotation centre in the
    // image; the minimum size becomes the circle the rotated image needs; mouse input on; Setup(no
    // parent, flags 7, empty rectangle). Mac 0x2a3690, Win 0x51b340
    bool Initialize(const Vector2& imageSize, const Vector2& center);
    // Mac 0x2a3980, Win 0x51b510
    void SetStateDescription(UInt32 index, const StateDescription& description);
    Real32 GetAngle() const { return m_angle; }
    // Redraws and notifies the observers when the angle changes. Mac 0x2a3af0, Win 0x51b568
    void SetAngle(Real32 angle);

    // ---- Control overrides (Mac slot / Win slot) ----
    // A selection mask is centred on the control; without one, the rotated image counts.
    virtual bool IsInside(const Vector2& point) const;            // 3/0   Mac 0x2a3f20, Win 0x51b61d
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5 Mac 0x2a4790, Win 0x51b9f9
    virtual void OnHidden();                                      // 11/8  Mac 0x2a4880, Win 0x51bab8
    virtual void OnEnabled();                                     // 12/9  Mac 0x2a48e0, Win 0x51bad5
    virtual void OnDisabled();                                    // 13/10 Mac 0x2a4950, Win 0x51baf4
    virtual void OnMouseMove(const Vector2& position);            // 18/15 Mac 0x2a49a0, Win 0x51bb09
    virtual void OnMousePressed(const Vector2& position, UInt32 button);  // 19/16 Mac 0x2a4b80, Win 0x51bb5d
    virtual void OnMouseReleased(const Vector2& position, UInt32 button); // 20/17 Mac 0x2a4ca0, Win 0x51bb96
    virtual void Draw(ControlDrawArgs& args);                     // 33/30 Mac 0x2a47b0, Win 0x51ba14

protected:
    // Observers of the dial leave first, then Control's part. Mac 0x2a45f0 (thunks 0x2a46b0, 0x2a4710)
    virtual void OnFinalRelease();

private:
    // Image / image rectangle / cursor from the current state. Mac 0x2a3a30, Win 0x51b91c
    void UpdateState();
    // m_state = `state` (on a change: UpdateState + redraw). Win 0x51b8fe (Mac inlined)
    void SetState(UInt32 state);
    // m_transform = translate(-center) * rotate(angle) * translate(floor(size / 2));
    // m_inverseTransform = its inverse. Mac 0x2a3df0, Win 0x51b7e9
    void UpdateTransform();
    // `point` (control coordinates) on the rotated image. Win 0x51b6e5 (Mac inlined)
    bool IsOverImage(const Vector2& point) const;
    // SetAngle from the direction centre -> `point`. Win 0x51b973 (Mac inlined)
    void TrackAngle(const Vector2& point);
    // Invalidate(false) unless the manager draws (inlined everywhere).
    void Redraw();

protected:
    // ---- data (Mac offsets) ----
    RefPointer<Texture>  m_image;                 // +0x17c current state's image
    Vector2              m_imageSize;             // +0x180
    Rect                 m_imageRect;             // +0x188 picture inside m_image (pixels)
    Vector2              m_center;                // +0x198 rotation centre inside the picture
    Real32               m_angle;                 // +0x1a0 0
    bool                 m_flag1a4;               // +0x1a4 false (not used in this TU)
    Matrix3D             m_transform;             // +0x1a8 picture -> control
    Matrix3D             m_inverseTransform;      // +0x1e8 control -> picture
    BasicArray<StateDescription> m_stateDescriptions;  // +0x228
    UInt32               m_state;                 // +0x238 StateNormal
    UInt32               m_stateDescriptionIndex[4];   // +0x23c {0, 1, 2, 3}
                                                  // Object +0x24c
};

} // namespace LEGO
