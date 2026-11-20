// LEGO Digital Designer — restored source
// TU 196 "DecorationControl.cpp": Mac 0x247d10-0x24a32b, Win 0x697386-0x697b07, 0x6938ab.
// The base constructor (Mac 0x24a330) sits in TU 197 and is restored here.
// Not code: the CallLuaMethod<LuaObject, LuaObject> instance Mac 0x249160 (LuaBindings.h), the
// destructor body 0x24a0e0, construction-vtable thunks (0x249a80-0x24a0c0).
#include "DecorationControl.h"
#include "BricksControl.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickDatabase.h"
#include "PrimitiveElement.h"
#include "PrimitiveElementBrick.h"
#include "Part.h"
#include "LDDPart.h"
#include "RenderControlManager.h"
#include "LDDUtilities.h"
#include "LuaScriptManager.h"
#include "LuaControl.h"
#include "LuaBindings.h"
#include "Updater.h"
#include "StringStream.h"
#include "Texture.h"

namespace LEGO {

namespace {

const Real32 kItemSize    = 64.0f;
const UInt32 kMaxRows     = 6;       // more rows scroll
const Real32 kFrameWidth  = 14.0f;   // frame around the matrix
const Real32 kScrollWidth = 16.0f;   // the scroll bar

} // namespace

// Mac 0x24a330 (C2, TU 197)
DecorationControl::DecorationControl(ControlManager* manager, BrickConnectSceneManager* sceneManager,
                                     UInt32 rows, UInt32 columns)
    : StretchableControl(manager), m_matrix(), m_scroll(), m_rows(rows), m_columns(columns),
      m_itemSize(kItemSize, kItemSize), m_extendedTooltip(false), m_sceneManager(sceneManager)
{
}

// Mac 0x2498a0 (D1, body 0x24a0e0), 0x249900 (D0), Win sdd 0x6938ab
DecorationControl::~DecorationControl()
{
}

// Mac 0x247d10, Win 0x69777e
bool DecorationControl::Create(Control* parent, Control* after)
{
    // The 9-slice frame in LDDgraphics01.png (as MaterialControl's).
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
    SetTransparent(true);
    return true;
}

// Mac 0x2486a0, Win 0x696e74
void DecorationControl::ResetItems()
{
    if (m_matrix)
        MainUpdater::ReleaseLater(m_matrix.Get());
    if (m_scroll) {
        m_scroll->SetParent(0, 0);
        m_scroll = 0;
    }
    m_matrix = new MatrixLayoutControl(m_manager);
    m_matrix->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_matrix->SetCellSize(m_itemSize);

    LuaPlus::LuaObject globals = LuaScriptManager::GetInstance()->GetGlobals();
    LuaPlus::LuaState* state = globals.GetState();
    LuaPlus::LuaObject editMode = globals.GetByName("editMode");
    LuaPlus::LuaObject self = LuaControl::GetControlTable(state, this);
    if (self.IsNil())
        self = LuaControl::CreateControlTable(state, this);
    LuaPlus::LuaObject matrix = LuaControl::CreateControlTable(state, m_matrix.Get());
    if (!CallLuaMethod(editMode, "InjectScrollbarInPopup", false, 1,
                       LuaPlus::LuaObject(self), LuaPlus::LuaObject(matrix)))
        return;
    LuaPlus::LuaObject result(state, -1);
    state->Pop(1);
    BindableLuaObject* bound = GetBoundObject(result);
    LuaControl* scroll = bound ? dynamic_cast<LuaControl*>(bound) : 0;
    m_scroll = scroll ? scroll->GetTargetControl() : 0;
    // NOTE: no null check (both builds): the Lua function must return a control.
    m_scroll->SetMargins(-4.0f, 10.0f, -4.0f, 10.0f, 0);
    m_matrix->SetAxisMargins(0, 0.0f, 0.0f, 0);
}

// Mac 0x248b40, Win 0x697386
bool DecorationControl::HasDecorations(const PrimitiveElement* picked, UInt32 surface) const
{
    const BrickDatabase::DecorationMapping& mapping =
        m_sceneManager->GetDatabase()->GetDecorationMapping();
    return mapping.Find(Pair<UInt32, UInt32>(picked->GetPrimitive()->GetDesignID(), surface))
           != mapping.End();
}

// Mac 0x248bd0, Win 0x6973ee
bool DecorationControl::ShowDecorations(const PrimitiveElement* picked, UInt32 surface)
{
    ResetItems();
    BrickConnectSceneControl* sceneControl = m_sceneManager->GetSceneControl();
    Vector3 direction = sceneControl->GetCameraPosition() - sceneControl->GetCameraTarget();
    Vector2 camera(direction.GetPitch(), direction.GetYaw());

    const UInt32 designID = picked->GetPrimitive()->GetDesignID();
    const BrickDatabase::DecorationMapping& mapping =
        m_sceneManager->GetDatabase()->GetDecorationMapping();
    const Pair<UInt32, UInt32> key(designID, surface);
    const LDDBrick* brick = static_cast<const LDDBrick*>(picked->GetPart()->GetBrick());

    UInt32 count = 1;
    for (BrickDatabase::DecorationMapping::ConstIterator it = mapping.LowerBound(key),
         end = mapping.UpperBound(key); it != end; ++it) {
        RefPointer<PrimitiveElementBrick> decorated = new PrimitiveElementBrick(brick);
        for (UInt32 part = 0; part < decorated->GetBrick()->GetPartCount(); ++part) {
            if (decorated->GetPalettePart(part)->GetDesignID() == designID) {
                decorated->SetDecoration(part, surface, it->second);
                break;
            }
        }
        RefPointer<BricksControl> item = CreateBricksControl(*decorated, camera, m_manager,
            m_sceneManager, static_cast<ControlObserver*>(this), m_matrix.Get());
        m_matrix->AddControl(item.Get());
        ++count;
    }
    // The brick without the decoration.
    RefPointer<PrimitiveElementBrick> plain = new PrimitiveElementBrick(brick);
    for (UInt32 part = 0; part < plain->GetBrick()->GetPartCount(); ++part) {
        if (plain->GetPalettePart(part)->GetDesignID() == designID) {
            plain->RemoveDecoration(part, surface);
            break;
        }
    }
    RefPointer<BricksControl> item = CreateBricksControl(*plain, camera, m_manager, m_sceneManager,
        static_cast<ControlObserver*>(this), m_matrix.Get());
    m_matrix->AddControl(item.Get());

    if (count < 2)
        return false;
    UInt32 rows = count / m_columns + (count % m_columns != 0 ? 1 : 0);
    if (rows > kMaxRows)
        rows = kMaxRows;
    UInt32 columns = count < m_columns ? count : m_columns;
    SetSize(Vector2((Real32)columns * m_itemSize.x + kFrameWidth + kScrollWidth,
                    (Real32)rows * m_itemSize.y + kFrameWidth));
    SetShown(true);
    SetZOrder(0);
    return true;
}

// Mac 0x249270 (thunk 0x249350)
void DecorationControl::OnMouseMove(Control* control, const Vector2&)
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

// Mac 0x249380 (thunk 0x249400)
void DecorationControl::OnMouseClicked(Control* control, const Vector2&, UInt32 button)
{
    if (button != 0)
        return;
    BricksControl* item = dynamic_cast<BricksControl*>(control);
    if (!item)
        return;
    OnBricksControlClicked(item);
    MainUpdater::ReleaseLater(control);
    ResetItems();
    SetShown(false);
}

// Mac 0x249490 (thunk 0x249540)
void DecorationControl::OnManagerMouseMove(const Vector2& position)
{
    if (!m_status.m_visible)
        return;
    Rect rect(m_size);
    Vector2 local(position);
    ManagerToLocal(local);
    if (rect.Contains(local))
        return;
    Control* mouse = m_manager->GetMouseControl();
    if (mouse && mouse->GetManager()->GetMouseControl() == mouse &&
        (mouse->HasMouseCapture() || mouse->GetPressedButtons() != 0) &&
        mouse->IsDescendantOf(this))
        return;
    ResetItems();
    SetShown(false);
}

// Mac 0x249560 (thunk 0x2495d0)
void DecorationControl::OnManagerMouseOverChanged(Control*, Control*)
{
    if (!m_status.m_visible)
        return;
    Rect rect(m_size);
    if (rect.Contains(m_manager->GetMousePosition(this)))
        return;
    ResetItems();
    SetShown(false);
}

// Mac 0x249640 (thunk 0x249680)
void DecorationControl::OnManagerModalControl(bool, Control*)
{
    if (!m_status.m_visible)
        return;
    ResetItems();
    SetShown(false);
}

// Mac 0x2496c0 (thunk 0x2497b0)
void DecorationControl::OnPreferenceChanged(const String& key)
{
    if (!key.EqualsNoCase(String("ShowExtendedBrickToolTip")))
        return;
    String value;
    if (PreferencesObserver::GetSubject()->GetValue(String("ShowExtendedBrickToolTip"), value))
        FromString(m_extendedTooltip, value);
}

// Mac 0x2497d0 (thunk 0x249880)
void DecorationControl::OnAttached(Preferences* preferences)
{
    String value;
    if (preferences->GetValue(String("ShowExtendedBrickToolTip"), value))
        FromString(m_extendedTooltip, value);
}

// Mac 0x249970
void DecorationControl::OnFinalRelease()
{
    PreferencesObserver::OnFinalRelease();
    ControlManagerObserver::OnFinalRelease();
    for (UInt32 i = 0; i < ControlObserver::m_subjects.GetSize(); ++i)
        static_cast<NotifierT<Control, ControlObserver>*>(ControlObserver::m_subjects[i])
            ->RemoveObserver(this);
    ControlObserver::m_subjects.Clear();
    Object::OnFinalRelease();
    StretchableControl::OnFinalRelease();
}

} // namespace LEGO
