// LEGO Digital Designer — restored source
// LEGO::StretchableToggleControl — a ToggleControl drawn with a StretchableImage (TU 178) under
// the RenderControl drawing; each state description has its own image position (the grid's
// origin in the texture). Base of LDDPaneToggleControl (TU 288).
// TU 174 "StretchableToggleControl.cpp": Mac 0x232ca0-0x23312b, Win 0x519f26-0x51a04d; the ctor
// is inline.
//
// RTTI (Mac): one public base ToggleControl, virtual base Object. Vtable 0x72e79c (55 entries):
//   ToggleControl's with D1 0x2142b0 / D0 0x214310, OnRectangleChanged 0x232d60 (8), Draw
//   0x232df0 (33), UpdateState 0x232da0 (47), new slot 54 Create 0x232ca0; Object-in at
//   -0x270. (0x72be5c is a construction vtable of a derived class.) Mac layout: ToggleControl
//   +0x000..+0x240, BasicArray<Vector2> +0x240, StretchableImage +0x250, Object +0x270.
//   Win: vtable 0x9d1bec (50 entries: OnRectangleChanged 5 = 0x519f8a, Draw 30 = 0x519fd9,
//   UpdateState 42 = 0x519fac, Create 49 = 0x519f26); StretchableImage at +0x26c;
//   Object-in 0x9d1bb8 at +0x290.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "BasicArray.h"
#include "ToggleControl.h"
#include "StretchableImage.h"

namespace LEGO {

class StretchableToggleControl : public ToggleControl {
public:
    explicit StretchableToggleControl(ControlManager* manager) : ToggleControl(manager) {}
    virtual ~StretchableToggleControl();                                    // Mac 0x2142b0/0x214310

    // ---- overrides (Mac slot / Win slot) ----
    // RenderControl's, then the image's positions follow the new size.
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5 Mac 0x232d60, Win 0x519f8a
    // 2-D render state, the 9-slice image, then RenderControl::Draw.
    virtual void Draw(ControlDrawArgs& args);                               // 33/30 Mac 0x232df0, Win 0x519fd9
    // ButtonControl's, then the image's texture coordinates from the position of the shown
    // state description.
    virtual void UpdateState();                                             // 47/42 Mac 0x232da0, Win 0x519fac

    // ---- new virtual ----
    using ToggleControl::Create;
    // ToggleControl::Initialize, the image (StretchableImage::Create with the manager), one image
    // position (0, 0) per state description. False when either fails.
    virtual bool Create(Texture* texture, const Vector3& columns, const Vector3& rows); // 54/49 Mac 0x232ca0, Win 0x519f26

    // Image position of state description `index` (set by the subclasses; plain member access).
    Vector2&       GetImagePosition(UInt32 index)       { return m_imagePositions[index]; }
    const Vector2& GetImagePosition(UInt32 index) const { return m_imagePositions[index]; }
    UInt32         GetImagePositionCount() const        { return m_imagePositions.GetSize(); }
    StretchableImage& GetImage() { return m_image; }

protected:
    BasicArray<Vector2> m_imagePositions;   // Mac +0x240 one per state description
    StretchableImage    m_image;            // Mac +0x250, Win +0x26c
};

} // namespace LEGO
