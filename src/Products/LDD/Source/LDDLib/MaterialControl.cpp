// LEGO Digital Designer — restored source
// TU 139 "MaterialControl.cpp": Mac 0x1f40d0-0x1f729b, Win 0x66a362-0x66b321.
// Not code: the destructor body with VTT 0x1f5260, construction-vtable thunks (0x1f5510-0x1f5e20,
// 0x1f66c0 …), Mac 0x1f7220 = <iostream> + a header static.
#include "MaterialControl.h"
#include "BricksControl.h"
#include "BrickCountAugmentation.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "RenderControlManager.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "Updater.h"
#include "FramedTextControl.h"
#include "StringStream.h"
#include "Texture.h"

namespace LEGO {

namespace {

const Real32 kItemSize = 64.0f;

// The BrickCountAugmentation of `control` (Control::GetAugmentation + dynamic_cast: the
// GetAugmentation<T> instance Mac 0x6bcc0, Win 0x662527).
RefPointer<BrickCountAugmentation> GetBrickCount(Control* control)
{
    RefPointer<ControlAugmentation> augmentation;
    control->GetAugmentation(kBrickCountAugmentationName, augmentation);
    return RefPointer<BrickCountAugmentation>(
        augmentation ? dynamic_cast<BrickCountAugmentation*>(augmentation.Get()) : 0);
}

} // namespace

// Mac 0x1f40d0 (C2), Win 0x66a71d
MaterialControl::MaterialControl(ControlManager* manager, BrickConnectSceneManager* sceneManager,
                                 UInt32 rows, UInt32 columns)
    : StretchableControl(manager), m_matrix(), m_rows(rows), m_columns(columns),
      m_itemSize(kItemSize, kItemSize), m_more(), m_filter(new CompositeBrickFilter),
      m_sceneManager(sceneManager), m_extendedTooltip(false), m_showCounts(false)
{
}

// Mac 0x1f54b0 (D1, body 0x1f5260), 0x1f5820 (D0), Win sdd 0x66a6fb
MaterialControl::~MaterialControl()
{
}

// Mac 0x1f44c0, Win 0x66ad42
bool MaterialControl::Create(Control* parent, Control* after)
{
    // The 9-slice frame in LDDgraphics01.png (11 px corners, 2 px edges).
    const Rect topLeft    (Vector2(73.0f, 520.0f), Vector2(11.0f, 11.0f));
    const Rect top        (Vector2(84.0f, 520.0f), Vector2( 2.0f, 11.0f));
    const Rect topRight   (Vector2(86.0f, 520.0f), Vector2(11.0f, 11.0f));
    const Rect left       (Vector2(73.0f, 531.0f), Vector2(11.0f,  2.0f));
    const Rect center     (Vector2(84.0f, 531.0f), Vector2( 2.0f,  2.0f));
    const Rect right      (Vector2(86.0f, 531.0f), Vector2(11.0f,  2.0f));
    const Rect bottomLeft (Vector2(73.0f, 533.0f), Vector2(11.0f, 11.0f));
    const Rect bottom     (Vector2(84.0f, 533.0f), Vector2( 2.0f, 11.0f));
    const Rect bottomRight(Vector2(86.0f, 533.0f), Vector2(11.0f, 11.0f));
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image)
        return false;
    if (!StretchableControl::Create(image.Get(), topLeft, top, topRight, left, center, right,
                                    bottomLeft, bottom, bottomRight))
        return false;
    Setup(parent, after, 6, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    ResetItems();
    ControlManagerObserver::Attach(m_manager);
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        PreferencesObserver::Attach(preferences.Get());
    }
    BrickConnectSceneControlObserver::Attach(m_sceneManager->GetSceneControl());
    SetTransparent(true);
    return true;
}

// Mac 0x1f50f0, Win 0x66a362
void MaterialControl::ResetItems()
{
    if (m_matrix)
        MainUpdater::ReleaseLater(m_matrix.Get());
    m_matrix = new MatrixLayoutControl(m_manager);
    m_matrix->Setup(this, 0, 7, Rect(Vector2(2.0f, 3.0f), Vector2(0.0f, 0.0f)), 0, false);
    m_matrix->SetCellSize(m_itemSize);
}

// Mac 0x1f5210
void MaterialControl::SetBrickFilter(CompositeBrickFilter* filter)
{
    m_filter = filter;
}

// Mac 0x1f5e40, Win 0x66b159
bool MaterialControl::SetBricks(BrickArray bricks, const Vector2* camera, bool withMore)
{
    ResetItems();
    UInt32 count = 0;
    for (BrickArray::Iterator it = bricks.Begin(); it != bricks.End(); ++it) {
        const BrickPalette::Brick* brick = it->Get();
        if (!m_filter->Accept(brick))
            continue;
        RefPointer<BricksControl> item = CreateBricksControl(brick, m_manager, m_sceneManager,
            static_cast<ControlObserver*>(this), m_matrix.Get(), m_extendedTooltip);
        if (camera)
            item->SetCamera(*camera);
        m_matrix->AddControl(item.Get());
        ++count;
    }
    if (withMore) {
        if (!m_more && !CreateMoreButton())
            return false;
        m_matrix->AddControl(m_more.Get());
        ++count;
    }
    if (count == 0)
        return false;
    UInt32 columns = count < m_columns ? count : m_columns;
    UInt32 rows = count / m_columns + (count % m_columns != 0 ? 1 : 0);
    SetSize(Vector2((Real32)columns * m_itemSize.x + 9.0f, (Real32)rows * m_itemSize.y + 14.0f));
    m_matrix->SetSize(m_size);
    SetShown(true);
    SetZOrder(0);
    RefreshCounts();
    return true;
}

// Mac 0x1f61c0, Win 0x66a8e6
bool MaterialControl::CreateMoreButton()
{
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/EditScreen/EditMode_Final.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image)
        return false;
    m_more = new ButtonControl(m_manager);
    if (!m_more->Initialize())
        return false;
    m_more->Setup(0, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_more->SetSize(m_itemSize);
    ButtonControl::StateDescription state;
    state.m_image = image;
    state.m_transparent = false;
    state.m_imageRect.Set(Rect(Vector2(193.0f, 386.0f), Vector2(48.0f, 48.0f)));
    m_more->SetStateDescription(0, state);
    m_more->SetStateDescription(1, state);
    m_more->SetStateDescription(2, state);
    m_more->SetStateDescription(3, state);
    ControlObserver::Attach(m_more.Get());
    return true;
}

// Mac 0x1f65e0 (thunk 0x1f66c0)
void MaterialControl::OnMouseMove(Control* control, const Vector2&)
{
    if (!(control->GetPressedButtons() & 2))
        return;
    BricksControl* dragged = dynamic_cast<BricksControl*>(control);
    if (!dragged)
        return;
    for (MatrixLayoutControl::ControlIterator it = m_matrix->BeginControls();
         it != m_matrix->EndControls(); ++it) {
        if (!*it)
            continue;
        if (BricksControl* item = dynamic_cast<BricksControl*>(it->Get()))
            item->SetCamera(dragged->GetCamera());
    }
}

// Mac 0x1f66f0 (thunk 0x1f6790)
void MaterialControl::OnMouseClicked(Control* control, const Vector2&, UInt32 button)
{
    if (button != 0)
        return;
    if (control == m_more.Get()) {
        OnMoreClicked();
    } else {
        BricksControl* item = dynamic_cast<BricksControl*>(control);
        if (!item)
            return;
        OnBricksControlClicked(item);
    }
    MainUpdater::ReleaseLater(control);
    ResetItems();
    SetShown(false);
}

// Mac 0x1f67c0 (thunk 0x1f6870)
void MaterialControl::OnManagerMouseMove(const Vector2& position)
{
    if (!m_status.m_visible)
        return;
    Rect rect(m_size);
    Vector2 local(position);
    ManagerToLocal(local);
    if (rect.Contains(local))
        return;
    // A control of the pop-up that holds the mouse (capture or a pressed button) keeps it open.
    Control* mouse = m_manager->GetMouseControl();
    if (mouse && mouse->GetManager()->GetMouseControl() == mouse &&
        (mouse->HasMouseCapture() || mouse->GetPressedButtons() != 0) &&
        mouse->IsDescendantOf(this))
        return;
    ResetItems();
    SetShown(false);
}

// Mac 0x1f6890 (thunk 0x1f6900)
void MaterialControl::OnManagerMouseOverChanged(Control*, Control*)
{
    if (!m_status.m_visible)
        return;
    Rect rect(m_size);
    if (rect.Contains(m_manager->GetMousePosition(this)))
        return;
    ResetItems();
    SetShown(false);
}

// Mac 0x1f6970 (thunk 0x1f69b0)
void MaterialControl::OnManagerModalControl(bool, Control*)
{
    if (!m_status.m_visible)
        return;
    ResetItems();
    SetShown(false);
}

// Mac 0x1f69f0 (thunk 0x1f6be0)
void MaterialControl::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("ShowExtendedBrickToolTip"))) {
        String value;
        if (PreferencesObserver::GetSubject()->GetValue(String("ShowExtendedBrickToolTip"), value))
            FromString(m_extendedTooltip, value);
        return;
    }
    if (!key.EqualsNoCase(String("SHOWBRICKCOUNTS")))
        return;
    String value;
    if (PreferencesObserver::GetSubject()->GetValue(String("SHOWBRICKCOUNTS"), value))
        FromString(m_showCounts, value);
    RefreshCounts();
}

// Mac 0x1f6c00 (thunk 0x1f6d40)
void MaterialControl::OnAttached(Preferences* preferences)
{
    String value;
    if (preferences->GetValue(String("ShowExtendedBrickToolTip"), value))
        FromString(m_extendedTooltip, value);
    String counts;
    if (preferences->GetValue(String("SHOWBRICKCOUNTS"), counts))
        FromString(m_showCounts, counts);
}

// Win 0x66b10f (inlined on the Mac: SetBricks, OnPreferenceChanged)
void MaterialControl::RefreshCounts()
{
    if (BrickConnectSceneControlObserver::GetSubject()->IsShowingMissingBricks() && m_showCounts &&
        m_filter && m_filter->IsCounting())
        OnEvent16();
    else
        HideCounts();
}

// Mac 0x1f6d60 (thunk 0x1f70d0), Win 0x66aa9a
void MaterialControl::OnEvent16()
{
    BrickConnectSceneControl* sceneControl = BrickConnectSceneControlObserver::GetSubject();
    if (!sceneControl->IsShowingMissingBricks() || !m_showCounts || !m_filter ||
        !m_filter->IsCounting())
        return;
    for (MatrixLayoutControl::ControlIterator it = m_matrix->BeginControls();
         it != m_matrix->EndControls(); ++it) {
        if (!*it)
            continue;
        BricksControl* item = dynamic_cast<BricksControl*>(it->Get());
        if (!item)
            continue;
        const BrickPalette::Brick* brick = item->GetPaletteBrick();
        if (!m_filter->Accept(brick))
            continue;
        // NOTE: the filter's count less the scene's (negative: more in the scene than in the box).
        Int32 missing = (Int32)m_filter->GetCount(brick) - (Int32)sceneControl->GetMissingBrickCount(brick);
        RefPointer<BrickCountAugmentation> badge = GetBrickCount(item);
        if (!badge)
            continue;
        RenderControl* label = badge->GetLabel();
        String text;
        if (missing < 0) {
            Rect rect(384.0f, 27.0f, 391.0f, 34.0f);
            label->SetImageRect(&rect);
            label->SetFont(LDDApplication::GetInstance()->GetFont(0xf));
            text << missing;
        } else {
            Rect rect(374.0f, 27.0f, 381.0f, 34.0f);
            label->SetImageRect(&rect);
            label->SetFont(LDDApplication::GetInstance()->GetFont(0));
            text << missing;
        }
        label->SetText(text);
        label->SetShown(true);
    }
}

// Mac 0x1f7130, Win 0x66acc1
void MaterialControl::HideCounts()
{
    for (MatrixLayoutControl::ControlIterator it = m_matrix->BeginControls();
         it != m_matrix->EndControls(); ++it) {
        RefPointer<BrickCountAugmentation> badge = GetBrickCount(it->Get());
        if (badge)
            badge->GetLabel()->SetShown(false);
    }
}

// Mac 0x1f70f0 / 0x1f7110
void MaterialControl::OnAttached(BrickConnectSceneControl*)
{
}

void MaterialControl::OnDetached(BrickConnectSceneControl*)
{
}

// Mac 0x1f7210
void MaterialControl::OnMoreClicked()
{
}

// Mac 0x1f5c10
void MaterialControl::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    PreferencesObserver::OnFinalRelease();
    ControlManagerObserver::OnFinalRelease();
    // The ControlObserver part: leaves every observed control without OnDetached (inlined).
    for (UInt32 i = 0; i < ControlObserver::m_subjects.GetSize(); ++i)
        static_cast<NotifierT<Control, ControlObserver>*>(ControlObserver::m_subjects[i])
            ->RemoveObserver(this);
    ControlObserver::m_subjects.Clear();
    Object::OnFinalRelease();
    StretchableControl::OnFinalRelease();
}

} // namespace LEGO
