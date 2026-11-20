// LEGO Digital Designer — restored source
// TU 189 "MaterialItemControl.cpp": Mac 0x243c50-0x24570b, Win 0x691c24-0x692488.
// Not code: the Notifier<MaterialItemControlObserver> / NotifierT / BasicArray template instances
// (Mac 0x244660, 0x244d00, 0x245140-0x245690; Notifier.h), construction-vtable thunks
// (0x244840-0x244ee0), Mac 0x245690 = <iostream> + a header static.
#include "MaterialItemControl.h"
#include "RenderControlManager.h"
#include "ControlManager.h"
#include "LDDUtilities.h"
#include "BrickDatabase.h"
#include "TextLocale.h"
#include "Texture.h"
#include "Color.h"

namespace LEGO {

namespace {

// The swatch and its frames in Graphics/Icon_general.png (34 x 34 each). (names inferred)
const Real32 kIconY          = 590.0f;
const Real32 kIconSize       = 34.0f;
const Real32 kSwatchX        = 333.0f;   // the tinted swatch (SetMaterial)
const Real32 kShadeX         = 195.0f;   // the frame child (Initialize)
const Real32 kSelectedX      = 287.0f;   // Draw: selected
const Real32 kHoverX         = 241.0f;   // Draw: mouse over a hoverable item

} // namespace

// Mac 0x243d70 (C1), 0x243c60 (C2)
MaterialItemControl::MaterialItemControl(ControlManager* manager, const BrickDatabase* database)
    : RenderControl(manager), m_material(0), m_icon(), m_database(database), m_frame(),
      m_hover(false), m_selected(false), m_hoverable(false)
{
}

// Mac 0x243ea0, Win 0x691cb7
bool MaterialItemControl::Initialize()
{
    if (!RenderControl::Initialize())
        return false;
    m_icon = LoadApplicationTexture(String("Graphics/Icon_general.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!m_icon)
        return false;
    m_frame = new RenderControl(m_manager);
    m_frame->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_frame->SetMargins(0.0f, 0.0f, 0.0f, 0.0f, 0);
    Rect rect(Vector2(kShadeX, kIconY), Vector2(kIconSize, kIconSize));
    m_frame->SetImage(m_icon.Get(), &rect);
    SetInputFlags(m_inputFlags | 1);
    SetShown(true);
    SetEnabled(true);
    SetActive(true);
    SetTransparent(true);
    return true;
}

// Mac 0x244150, Win 0x691c24: inline in the header.

// Mac 0x244160, Win 0x691f93
void MaterialItemControl::SetMaterial(UInt32 material)
{
    if (m_material == material)
        return;
    m_material = material;
    MaterialColor color;
    if (m_database->GetMaterialColor(material, color)) {
        Rect rect(Vector2(kSwatchX, kIconY), Vector2(kIconSize, kIconSize));
        SetImage(m_icon.Get(), &rect);
        SetColor(color.diffuse);
        String text;
        String key("Material");
        key << material;
        LocalizedString name(key);
        text << material << " - " << name;
        CreateStandardTextTooltip(this, text);
    }
    Notifier<MaterialItemControlObserver>::Iterator it(*this);
    while (MaterialItemControlObserver* observer =
               it.Next(Notifier<MaterialItemControlObserver>::m_observers))
        observer->OnMaterialChanged(m_material);
}

// Mac 0x2445c0, Win 0x691c2b
void MaterialItemControl::SetSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        Redraw();
    }
}

// Mac 0x244610, Win 0x691c43
void MaterialItemControl::SetHoverable(bool hoverable)
{
    m_hoverable = hoverable;
    if (m_status.m_mouseInside) {
        if (hoverable)
            m_hover = true;
        Redraw();
    }
}

// Mac 0x244f80, Win 0x691c69
void MaterialItemControl::OnMouseEnter(const Vector2& position)
{
    if (m_hoverable) {
        m_hover = true;
        Redraw();
    }
    Control::OnMouseEnter(position);
}

// Mac 0x244fc0, Win 0x691c90
void MaterialItemControl::OnMouseLeave(const Vector2& position)
{
    if (m_hoverable) {
        m_hover = false;
        Redraw();
    }
    Control::OnMouseLeave(position);
}

// Mac 0x245000, Win 0x691e2d
void MaterialItemControl::Draw(ControlDrawArgs& args)
{
    RenderControl::Draw(args);
    if (!m_hover && !m_selected)
        return;
    args.Setup(false);
    if (!m_image)
        return;
    Rect textureRect(Vector2(kSelectedX, kIconY), Vector2(kIconSize, kIconSize));
    if (m_hover)
        textureRect.SetPosition(Vector2(kHoverX, kIconY));
    textureRect.Divide(m_image->GetImageSize());
    args.DrawTexture(Rect(m_size), m_image.Get(), textureRect, Color::White(),
                     m_image->GetFormat() != 0);
}

// Mac 0x244c70
void MaterialItemControl::OnFinalRelease()
{
    Notifier<MaterialItemControlObserver>::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

} // namespace LEGO
