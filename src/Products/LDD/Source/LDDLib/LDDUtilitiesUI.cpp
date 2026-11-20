// LEGO Digital Designer — restored source
// LDDUtilities (Mac TU 063, Win 0x635e9f-0x639696 + 0x513ae9-0x513c6e) — the application part:
// the LDDUtilities singleton (PLAYSOUND), sounds, application textures, FramedTextControl,
// BrickCountAugmentation, ExtendedTextTooltipAugmentation and small control / tooltip factories.
// See LDDUtilities.h (one object in the original).
#include "LDDUtilities.h"
#include "BrickCountAugmentation.h"
#include "ExtendedTextTooltipAugmentation.h"
#include "StandardTextTooltipAugmentation.h"
#include "FramedTextControl.h"
#include "RenderControl.h"
#include "RenderControlManager.h"
#include "LDDApplication.h"
#include "ObjectRegistry.h"
#include "ObjectFactory.h"
#include "Platform.h"
#include "Directory.h"
#include "Path.h"
#include "Texture.h"
#include "Render.h"
#include "SimpleSound.h"
#include "ToggleControl.h"   // (CreateCheckBox)

#include <typeinfo>

namespace LEGO {

// Mac 0x77ae6c (static initialiser 0x1505a0), Win 0xaef29c
const String kBrickCountAugmentationName("BrickCount");

// ===========================================================================================
// LDDUtilities
// ===========================================================================================

// Mac 0x14ddc0, Win 0x638eb7
LDDUtilities::LDDUtilities()
    : m_playSound(false)
{
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    Attach(preferences.Get());
}

// Mac 0x147b30 (inlined on Win)
LDDUtilities* LDDUtilities::GetInstance()
{
    LDDUtilitiesInstantiator instantiator;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Get(kRegistryID, instantiator);
    if (object.IsNull())
        return 0;
    return dynamic_cast<LDDUtilities*>(object.Get());
}

// Mac 0x14dd10, Win 0x636d60
void LDDUtilities::OnAttached(Preferences* preferences)
{
    preferences->GetValue(String("PLAYSOUND"), m_playSound);
}

// Mac 0x14dc50, Win 0x636d0c
void LDDUtilities::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("PLAYSOUND")))
        GetSubject()->GetValue(key, m_playSound);
}

// Mac 0x14c5b0, Win 0x63769e
RefPointer<SimpleSound> LDDUtilities::CreateSound(const String& fileName)
{
    if (!m_playSound)
        return RefPointer<SimpleSound>();
    Path path;
    path.SetElement(Platform::GetInstance()->GetSystemDirectory(8, true).Get());
    String relative("Sound/");
    if (!path.Append(relative << fileName))
        return RefPointer<SimpleSound>();
    RefPointer<SimpleSound> sound = new SimpleSound;
    if (!sound->Initialize() || !sound->Load(path))
        return RefPointer<SimpleSound>();
    return sound;
}

// Mac 0x14dfc0, Win 0x639302
RefPointer<Object> LDDUtilitiesInstantiator::CreateObject() const
{
    return RefPointer<Object>(new LDDUtilities);
}

// Restoration glue for the LDDSkybox seam (LDDSkybox.h): Mac 0x147b30 + 0x14c5b0 inlined.
RefPointer<SimpleSound> CreateLDDSound(const String& fileName)
{
    return LDDUtilities::GetInstance()->CreateSound(fileName);
}

// ===========================================================================================
// Textures
// ===========================================================================================

// Mac 0x14b7d0, Win 0x637578
Texture* LoadTexture(Directory* directory, const String& path, Render* render)
{
    Texture::LoadArgs2D args;
    args.path.SetElement(directory);
    args.path.Append(path);   // NOTE: the result is ignored (both builds)
    args.create.alphaMode = Texture::kAlphaImage;   // 2
    Object* object = render->GetObjectFactory()->CreateObject(typeid(Texture), args);
    if (object == 0)
        return 0;
    return dynamic_cast<Texture*>(object);
}

// Mac 0x147cd0, Win 0x637835
Texture* LoadApplicationTexture(const String& path, Render* render)
{
    RefPointer<Directory> directory = Platform::GetInstance()->GetSystemDirectory(9, true);
    return LoadTexture(directory.Get(), path, render);
}

// ===========================================================================================
// FramedTextControl (inline code emitted in TU 063)
// ===========================================================================================

// Mac 0x147bd0, Win 0x513ae9
FramedTextControl::FramedTextControl(ControlManager* manager)
    : RenderControl(manager)
{
    SetTransparent(true);
    SetTextAlignment(0, 0);
    SetTextOffset(3.0f, 2.0f);
}

// Mac 0x14e020 / 0x14e080, Win 0x513c10 / sdd 0x513c6e
FramedTextControl::~FramedTextControl()
{
}

// Mac 0x14e0f0, Win 0x513bb7
bool FramedTextControl::SetText(const String& text)
{
    bool result = RenderControl::SetText(text);
    if (result) {
        ResizeHeightToFitText(0);
        Vector2 size = m_textLayout->GetSize();
        size.x += 6.0f;
        size.y += 4.0f;
        SetSize(size);
    }
    return result;
}

// ===========================================================================================
// BrickCountAugmentation
// ===========================================================================================

// Mac 0x147d60, Win 0x637ce5
BrickCountAugmentation::BrickCountAugmentation(Control* control)
{
    m_label = new FramedTextControl(control->GetManager());
    m_label->Initialize();
    m_label->Setup(control, 0, 7, Rect(Vector2(2.0f, 2.0f), Vector2()), 0, false);
    m_label->SetFont(LDDApplication::GetInstance()->GetFont(0));
    m_label->SetInputFlags(m_label->GetInputFlags() & ~(UInt32)RenderControl::kInputInitialized);
    m_label->SetShown(false);

    RefPointer<Texture> image = LoadApplicationTexture(
        String("Graphics/EditScreen/EditMode_Final.png"),
        static_cast<RenderControlManager*>(control->GetManager())->GetRender());
    Rect imageRect(374.0f, 27.0f, 381.0f, 34.0f);
    m_label->SetImage(image.Get(), &imageRect);
    m_label->SetFrame(Vector3(2.0f, 3.0f, 2.0f), Vector3(2.0f, 3.0f, 2.0f), true);
    m_label->SetDrawAsFrame(true);
    Attach(control, kBrickCountAugmentationName);
}

// ===========================================================================================
// ExtendedTextTooltipAugmentation
// ===========================================================================================

// Mac 0x14d1a0 (factory 0x14d140)
ExtendedTextTooltipAugmentation::ExtendedTextTooltipAugmentation(Control* control, const String& text,
                                                                 const String& extendedText)
    : StandardTextTooltipAugmentation(control, text),
      m_standardText(text),
      m_extendedText(extendedText)
{
    Observer<LDDApplication, LDDApplicationObserver>::Attach(LDDApplication::GetInstance());
    if (LDDApplication::GetInstance()->IsExtendedTooltip())
        SetText(m_extendedText);
    else
        SetText(m_standardText);
}

// Mac 0x14faa0 / 0x14fb00, Win sdd 0x638e51
ExtendedTextTooltipAugmentation::~ExtendedTextTooltipAugmentation()
{
}

// Mac 0x14fb70, Win 0x638dbf
void ExtendedTextTooltipAugmentation::Assign(const ExtendedTextTooltipAugmentation& other)
{
    m_standardText = other.m_standardText;
    m_extendedText = other.m_extendedText;
    if (LDDApplication::GetInstance()->IsExtendedTooltip())
        SetText(m_extendedText);
    else
        SetText(m_standardText);
}

// Mac 0x14fbe0 (thunk 0x14fce0), Win 0x638deb
void ExtendedTextTooltipAugmentation::OnExtendedTooltipChanged(bool enabled)
{
    SetText(enabled ? m_extendedText : m_standardText);
}

// ===========================================================================================
// Factories
// ===========================================================================================

// Mac 0x14ab90, Win 0x636a8a
StandardTextTooltipAugmentation* CreateStandardTextTooltip(Control* control, const String& text)
{
    return new StandardTextTooltipAugmentation(control, text);
}

// Mac 0x14d140
ExtendedTextTooltipAugmentation* CreateExtendedTextTooltip(Control* control, const String& text,
                                                           const String& extendedText)
{
    return new ExtendedTextTooltipAugmentation(control, text, extendedText);
}

// Mac 0x14b910, Win 0x6375cf
RenderControl* CreateTextControl(Control* parent, ControlManager* manager, Font* font,
                                 const String& text)
{
    RenderControl* control = new RenderControl(manager);
    // a failed control is released, and taken out of `parent` (leaked in the original; low).
    // Only then: the control is returned unreferenced, as in the original (the caller's
    // RefPointer takes the first reference), so a reference held here over a successful
    // creation would free it on return.
    if (!control->Initialize()) {
        RefPointer<RenderControl> release = control;
        return 0;
    }
    control->Setup(parent, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    control->SetTransparent(true);
    control->SetText(text);
    control->SetFont(font);
    if (!control->ResizeHeightToFitText(0)) {
        RefPointer<RenderControl> release = control;
        control->SetParent(0);
        return 0;
    }
    return control;
}

// Mac 0x14c810, Win 0x637884
bool CreateCheckBox(ControlManager* manager, Control* parent, const Vector2& position,
                    const String& text, Real32 labelOffset, RefPointer<ToggleControl>& toggle,
                    RefPointer<RenderControl>& box, RefPointer<RenderControl>& label)
{
    toggle = new ToggleControl(manager);
    if (toggle->Initialize()) {
        toggle->Setup(parent, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        box = new RenderControl(manager);
        if (box->Initialize()) {
            box->Setup(toggle, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
            toggle->SetImageTarget(box);
            label = new RenderControl(manager);
            if (label->Initialize()) {
                label->Setup(toggle, 0, 7, Rect(Vector2(labelOffset, 0.0f), Vector2()), 0, false);
                label->SetText(text);
                label->SetFont(LDDApplication::GetInstance()->GetFont(3));
                label->ResizeHeightToFitText(0);
                label->SetTransparent(true);
                toggle->SetTextTarget(label);
                RefPointer<Texture> image;
                {
                    RefPointer<Directory> directory = Platform::GetInstance()->GetSystemDirectory(9, true);
                    image = LoadTexture(directory.Get(), String("Graphics/General/Popup Final.png"),
                        static_cast<RenderControlManager*>(manager)->GetRender());
                }
                if (image) {
                    ButtonControl::StateDescription state;
                    state.m_image = image;
                    state.m_transparent = false;
                    state.m_text = text;
                    state.m_font = LDDApplication::GetInstance()->GetFont(3);
                    state.m_imageRect.Set(Rect(0.0f, 182.0f, 13.0f, 195.0f));
                    toggle->SetStateDescription(0, state);
                    toggle->SetStateDescription(1, state);
                    state.m_imageRect.Set(Rect(26.0f, 182.0f, 39.0f, 195.0f));
                    toggle->SetStateDescription(2, state);
                    toggle->SetStateDescription(3, state);
                    state.m_imageRect.Set(Rect(13.0f, 182.0f, 26.0f, 195.0f));
                    toggle->SetStateDescription(4, state);
                    toggle->SetStateDescription(5, state);
                    state.m_imageRect.Set(Rect(39.0f, 182.0f, 51.0f, 195.0f));
                    toggle->SetStateDescription(6, state);
                    toggle->SetStateDescription(7, state);
                    toggle->FitToChildren(true, true, true, true);
                    toggle->SetPosition(position);
                    return true;
                }
                toggle = 0;
                box = 0;
                label = 0;
                return false;
            }
            toggle = 0;
            box = 0;
        } else {
            toggle = 0;
        }
    }
    return false;
}

} // namespace LEGO
