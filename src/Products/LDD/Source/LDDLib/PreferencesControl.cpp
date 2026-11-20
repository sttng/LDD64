// LEGO Digital Designer — restored source
// TU 133 "PreferencesControl.cpp" (Mac 0x1ea100-0x1ee08b; Win 0x650f28-0x653637): the
// preferences dialog (see PreferencesControl.h). The Windows-only language row (Win 0x651c76,
// 0x6520b0) is at the end of the file. Static initialiser Mac 0x1ee010: <iostream> + a header
// static.
#include "PreferencesControl.h"

#include "PreferenceControlBool.h"
#include "PreferenceControlSlider.h"
#include "LDDButtonControl.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "RenderControlManager.h"
#include "Render.h"
#include "Texture.h"
#include "TextLocale.h"
#include "Geometry.h"
#if defined(_WIN32)
#include "PreferenceControlDropDown.h"
#include "Platform.h"
#include "Directory.h"
#include "Path.h"
#include "BasicMap.h"
#endif
#include "StringStream.h"

#include <iostream>

namespace LEGO {

namespace {
// The label `key` of the language files, `english` when they have none (LDD64: the shipped
// files have no texts for the level-of-detail rows).
String LodLabel(const char* key, const char* english)
{
    String text;
    if (!TextLocale::GetInstance()->Localize(String(key), text))
        text = english;
    return text;
}
} // namespace

// Mac 0x1ea170, Win 0x65187f
PreferencesControl::PreferencesControl(RenderControlManager* manager)
    : ModalControl(manager), m_y(2.0f)
{
}

// Mac 0x1ed030 (D1, body 0x1ecb10 with the VTT) / 0x1ed330 (D0), thunks 0x1ed090-0x1ed620;
// Win sdd 0x653522
PreferencesControl::~PreferencesControl()
{
}

// Mac 0x1ea310, Win 0x652371
bool PreferencesControl::Initialize()
{
    if (!ModalControl::Initialize())
        return false;
    SetTitle(LocalizedString(String("PREFERENCES")));

    AddCheckBox(String("SHOWINFOFIELD"), LocalizedString(String("SHOWINFOFIELD")), GetContent(),
                false, false);
    AddCheckBox(String("SHOWTOOLTIPS"), LocalizedString(String("SHOWTOOLTIPS")), GetContent(),
                false, false);
    if (LDDApplication::GetInstance()->ReturnTrue17a710())
        AddCheckBox(String("PLAYSOUND"), LocalizedString(String("PLAYSOUND")), GetContent(),
                    false, false);
    AddCheckBox(String("SHOWBRICKREORIENTATION"),
                LocalizedString(String("SHOWBRICKREORIENTATION")), GetContent(), false, false);
    AddCheckBox(String("REPEATPASTE"), LocalizedString(String("REPEATPASTE")), GetContent(),
                false, false);
    AddCheckBox(String("SHOWBRICKCOUNTS"), LocalizedString(String("SHOWBRICKCOUNTS")),
                GetContent(), false, false);
    AddCheckBox(String("INVERTEDCAMERAX"), LocalizedString(String("InvertedCameraX")),
                GetContent(), false, false);
    AddCheckBox(String("INVERTEDCAMERAY"), LocalizedString(String("InvertedCameraY")),
                GetContent(), false, false);
    AddSeparator(GetContent());

    // ---- render switches (a change needs a restart) ----
    const bool shaders = static_cast<RenderControlManager*>(m_manager)->GetRender()->SupportsShaders();
    AddCheckBox(String("USESHADERSINSCENE"), LocalizedString(String("USESHADERSINSCENE")),
                GetContent(), !shaders, true);
    m_useShadersInScene = m_rows.Back();
    AddCheckBox(String("USESHADERSINPALETTE"), LocalizedString(String("USESHADERSINPALETTE")),
                GetContent(), !shaders, true);
    m_useShadersInPalette = m_rows.Back();
    {
        bool disabled = true;
        if (shaders) {
            if (LDDApplication::GetInstance()->AreRoundEdgeShadersLoaded()
                && LDDApplication::GetInstance()->UseShadersInScene())
                disabled = !LDDApplication::GetInstance()->UseShadersInPalette();
        }
        AddCheckBox(String("UseRoundEdges"), LocalizedString(String("USEROUNDEDGES")),
                    GetContent(), disabled, true);
    }
    m_useRoundEdges = m_rows.Back();
    {
        const BasicArray<RefPointer<PostProcessingEffect> >& effects =
            LDDApplication::GetInstance()->GetPostProcessingEffects();
        const UInt32 count = effects.GetSize();
        bool disabled;
        if (count == 0)
            disabled = true;
        else
            disabled = !LDDApplication::GetInstance()->IsPostProcessingEnabled();
        // NOTE (both builds): the slider's style argument is the number of effects - 1.
        AddSlider(String("SelectedPostProcessingEffect"),
                  LocalizedString(String("SELECTEDPOSTPROCESSINGEFFECT")), GetContent(), 0.0f,
                  (Real32)count, 1.0f, count - 1, disabled, false);
    }
    m_postProcessing = m_rows.Back();
    // level of detail (LDD64; BrickConnectSceneControl.cpp): applied at once
    AddCheckBox(String("LevelOfDetail"), LodLabel("LEVELOFDETAIL", "Simplify distant bricks (no studs)"),
                GetContent(), false, false);
    AddSlider(String("LevelOfDetailDistance"),
              LodLabel("LEVELOFDETAILDISTANCE", "Distance of simplified bricks"), GetContent(), 100.0f,
              2000.0f, 100.0f, 0, false, false);
    {
        // The slider stores the value it starts with (SliderControl::SetPreferenceName). The
        // default (500, LDDMain.cpp) stays out of preferences.ini, which is then the original's
        // until the distance is changed.
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        Real32 value = 0.0f;
        if (preferences->GetValue(String("LevelOfDetailDistance"), text) && FromString(value, text) &&
            value == 500.0f)
            preferences->RemoveValue(String("LevelOfDetailDistance"));
    }
    AddCheckBox(String("LevelOfDetailBoxes"),
                LodLabel("LEVELOFDETAILBOXES", "Draw bricks beyond twice the distance as boxes"), GetContent(),
                false, false);
    AddSeparator(GetContent());

#if defined(_WIN32)
    // (Windows only) the language of the user interface
    AddDropDown(String("LANGUAGE"), LocalizedString(String("LANGUAGE")), GetContent(),
                GetLanguages(), true);
#endif

    bool qaMode = Preferences::GetInstance()->GetBool(String("qaMode"), false);
    bool developerMode = Preferences::GetInstance()->GetBool(String("developerMode"), false);
    if (qaMode || developerMode) {
        AddCheckBox(String("BRICKTOOLTIP"), LocalizedString(String("BRICKTOOLTIP")),
                    GetContent(), false, false);
        AddCheckBox(String("ShowExtendedBrickToolTip"),
                    LocalizedString(String("ShowExtendedBrickToolTip")), GetContent(), false,
                    false);
    }
    AddSlider(String("CompatibilityModeLevel"), LocalizedString(String("CompatibilityModeLevel")),
              GetContent(), 0.0f, 100.0f, 20.0f, 6, false, true);
    m_y += 5.0f;

    // ---- the restart note: an icon and "PrefChangeNeedsRestart" ----
    m_restartNote = new Control(m_manager);
    m_restartNote->Setup(GetContent(), 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    RefPointer<Texture> image = LoadApplicationTexture(
        String("Graphics/General/Popup Final.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image.Get())
        return false;
    {
        RefPointer<RenderControl> icon = new RenderControl(m_manager);
        icon->Initialize();
        m_controls.PushBack(RefPointer<Control>(icon.Get()));
        icon->Setup(m_restartNote, 0, 7, Rect(Vector2(0.0f, 5.0f), Vector2()), 0, false);
        Rect iconRect(Vector2(233.0f, 217.0f), Vector2(26.0f, 25.0f));
        icon->SetImage(image, &iconRect);

        RefPointer<RenderControl> text = new RenderControl(m_manager);
        text->Initialize();
        m_controls.PushBack(RefPointer<Control>(text.Get()));
        text->Setup(m_restartNote, 0, 7, Rect(Vector2(31.0f, 0.0f), Vector2(200.0f, 0.0f)), 0,
                    false);
        text->SetTransparent(true);
        text->SetFont(LDDApplication::GetInstance()->GetFont(3));
        text->SetText(LocalizedString(String("PrefChangeNeedsRestart")));
        text->ResizeToFitText();
    }
    m_restartNote->FitToChildren(true, true, true, true);
    m_restartNote->SetPosition(Vector2(55.0f, m_y));
    m_restartNote->SetShown(false);
    m_y = m_restartNote->GetSize().y + 2.0f + m_y;

    // ---- the buttons ----
    m_buttonOK = new LDDButtonControl(m_manager);
    if (!m_buttonOK->Create(LocalizedString(String("OK")), GetContent(), Vector2()))
        return false;
    m_buttonOK->SetAlignment(0x33, 0x33, 10.0f, 10.0f, 0);
    ControlObserver::Attach(m_buttonOK.Get());

    m_buttonCancel = new LDDButtonControl(m_manager);
    if (!m_buttonCancel->Create(LocalizedString(String("CANCEL")), GetContent(), Vector2()))
        return false;
    m_buttonCancel->SetAlignment(0x33, 0x33, m_buttonOK->GetSize().x + 20.0f, 10.0f, 0);
    ControlObserver::Attach(m_buttonCancel.Get());

    m_buttonReset = new LDDButtonControl(m_manager);
    if (!m_buttonReset->Create(LocalizedString(String("RESETPREFERENCES")), GetContent(),
                               Vector2()))
        return false;
    m_buttonReset->SetAlignment(0x31, 0x31, -10.0f, 10.0f, 0);
    ControlObserver::Attach(m_buttonReset.Get());

    m_defaultButton = m_buttonOK.Get();
    m_cancelButton = m_buttonCancel.Get();
    GetContent()->SetSize(Vector2(430.0f, m_y + m_buttonOK->GetSize().y + 20.0f));
    return true;
}

// Mac 0x1ec0b0, Win 0x651b68
bool PreferencesControl::AddCheckBox(const String& key, const String& label, Control* parent,
                                     bool disabled, bool needsRestart)
{
    m_y += 8.0f;
    RefPointer<PreferenceControlBool> row =
        new PreferenceControlBool(m_manager, key, label, needsRestart);
    if (!row->Create(parent))
        return false;
    row->SetPosition(Vector2(10.0f, m_y));
    if (disabled)
        row->Enable(false);
    m_rows.PushBack(RefPointer<PreferenceControl>(row.Get()));
    m_y = row->GetSize().y + 8.0f + m_y;
    return true;
}

// Mac 0x1ec270, Win 0x651ed0
bool PreferencesControl::AddSeparator(Control* parent)
{
    m_y += 2.0f;
    RefPointer<Texture> image = LoadApplicationTexture(
        String("Graphics/General/Popup Final.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image.Get())
        return false;
    RefPointer<RenderControl> line = new RenderControl(m_manager);
    if (!line->Initialize())
        return false;
    line->Setup(parent, 0, 7, Rect(Vector2(1.0f, m_y), Vector2(421.0f, 2.0f)), 0, false);
    line->SetInputFlags(line->GetInputFlags() & ~(UInt32)RenderControl::kInputInitialized);
    Rect lineRect(Vector2(135.0f, 201.0f), Vector2(338.0f, 2.0f));
    line->SetImage(image, &lineRect);
    m_controls.PushBack(RefPointer<Control>(line.Get()));
    m_y = line->GetSize().y + 2.0f + m_y;
    return true;
}

// Mac 0x1ec580, Win 0x651d90
bool PreferencesControl::AddSlider(const String& key, const String& label, Control* parent,
                                   Real32 minimum, Real32 maximum, Real32 step, UInt32 style,
                                   bool disabled, bool needsRestart)
{
    m_y += 8.0f;
    RefPointer<PreferenceControlSlider> row = new PreferenceControlSlider(m_manager, needsRestart);
    if (!row->Create(parent, minimum, maximum, step, style, key, label))
        return false;
    row->SetPosition(Vector2(45.0f, m_y));
    row->SetAxisMargins(0, 45.0f, 45.0f, 0);
    if (disabled)
        row->Enable(false);
    m_rows.PushBack(RefPointer<PreferenceControl>(row.Get()));
    m_y = row->GetSize().y + 8.0f + m_y;
    return true;
}

// Mac 0x1ec790, Win 0x6535a5
void PreferencesControl::ShowModal()
{
    for (BasicArray<RefPointer<PreferenceControl> >::Iterator it = m_rows.Begin();
         it != m_rows.End(); ++it)
        (*it)->Remember();
    m_restartNote->SetShown(false);
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        PreferencesObserver::Attach(preferences.Get());
    }
    ModalControl::ShowModal();
}

// Mac 0x1ecac0, Win 0x653544
void PreferencesControl::EndModal()
{
    PreferencesObserver::Detach();
    ModalControl::EndModal();
}

// Mac 0x1ed940, Win 0x651431
void PreferencesControl::Apply()
{
    for (BasicArray<RefPointer<PreferenceControl> >::Iterator it = m_rows.Begin();
         it != m_rows.End(); ++it) {
        if ((*it)->IsChanged())
            (*it)->Store();
    }
    Preferences::GetInstance()->Save();
}

// Mac 0x1eda20, Win 0x65149c
void PreferencesControl::RevertChanges()
{
    for (BasicArray<RefPointer<PreferenceControl> >::Iterator it = m_rows.Begin();
         it != m_rows.End(); ++it) {
        if ((*it)->IsChanged())
            (*it)->Revert();
    }
    Preferences::GetInstance()->Save();
}

// Mac 0x1ed920, Win 0x6515cc
void PreferencesControl::OnAccept()
{
    Apply();
    ModalControl::OnAccept();
}

// Mac 0x1eda00, Win 0x6515dc
void PreferencesControl::OnCancel()
{
    RevertChanges();
    ModalControl::OnCancel();
}

// Mac 0x1ed820 (thunk 0x1ed8a0), Win 0x65158b
void PreferencesControl::OnMouseClicked(Control* control, const Vector2&, UInt32)
{
    if (m_buttonOK.Get() == control) {
        OnAccept();
    } else if (m_buttonCancel.Get() == control) {
        OnCancel();
    } else if (m_buttonReset.Get() == control) {
        for (BasicArray<RefPointer<PreferenceControl> >::Iterator it = m_rows.Begin();
             it != m_rows.End(); ++it)
            (*it)->ResetToDefault();
    }
}

// Mac 0x1edae0 (thunk 0x1edc00), Win 0x651370
void PreferencesControl::OnPreferenceChanged(const String&)
{
    bool needsRestart = false;
    for (BasicArray<RefPointer<PreferenceControl> >::Iterator it = m_rows.Begin();
         it != m_rows.End(); ++it) {
        if ((*it)->IsChanged() && (*it)->NeedsRestart()) {
            needsRestart = true;
            break;
        }
    }
    {
        bool enabled;
        if (!LDDApplication::GetInstance()->AreRoundEdgeShadersLoaded()
            || !m_useShadersInScene->IsOn())
            enabled = false;
        else
            enabled = m_useShadersInPalette->IsOn();
        m_useRoundEdges->Enable(enabled);
    }
    {
        bool enabled;
        if (LDDApplication::GetInstance()->GetPostProcessingEffects().GetSize() == 0)
            enabled = false;
        else
            enabled = m_useShadersInScene->IsOn();
        m_postProcessing->Enable(enabled);
    }
    m_restartNote->SetShown(needsRestart);
}

// Mac 0x1ed6a0
void PreferencesControl::OnFinalRelease()
{
    PreferencesObserver::OnFinalRelease();
    ControlObserver::OnFinalRelease();
    ModalControl::OnFinalRelease();
}

#if defined(_WIN32)
// ---- Windows only: the language row ----------------------------------------------------------

// Win 0x651c76
bool PreferencesControl::AddDropDown(const String& key, const String& label, Control* parent,
                                     BasicArray<Pair<String, String> > items, bool needsRestart)
{
    m_y += 6.0f;
    RefPointer<PreferenceControlDropDown> row =
        new PreferenceControlDropDown(m_manager, key, label, items, needsRestart);
    if (!row->Create(parent))
        return false;
    row->SetPosition(Vector2(25.0f, m_y));
    m_rows.PushBack(RefPointer<PreferenceControl>(row.Get()));
    m_y = row->GetSize().y + 10.0f + m_y;
    return true;
}

// Win 0x6520b0
BasicArray<Pair<String, String> > PreferencesControl::GetLanguages()
{
    BasicArray<Pair<String, String> > languages;
    RefPointer<Directory> directory = Platform::GetInstance()->GetSystemDirectory(9, true);
    if (!directory.Get())
        return languages;
    Path file(String("Languages.loc"), directory.Get());
    String list;
    bool found;
    {
        RefPointer<TextLocale> locale = TextLocale::GetInstance();
        found = locale->Lookup(file, String("Languages"), list);
    }
    if (!found)
        return languages;
    BasicArray<String> codes;
    list.Split((UniChar16)',', codes);
    for (BasicArray<String>::Iterator code = codes.Begin(); code != codes.End(); ++code) {
        String name;
        bool known = false;
        if (directory->DirectoryExists(*code, (RefPointer<Directory>*)0)) {
            RefPointer<TextLocale> locale = TextLocale::GetInstance();
            known = locale->Lookup(file, *code, name);
        }
        if (known)
            languages.PushBack(Pair<String, String>(*code, name));
    }
    return languages;
}
#endif

} // namespace LEGO
