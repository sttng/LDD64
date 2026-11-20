// LEGO Digital Designer — restored source
// BrickPaletteControl.cpp — TU 023 (Mac 0x683c0-0x71fbb; Win 0x661614-0x663c9f).
// See BrickPaletteControl.h. The rest of the TU is header template code (observer / notifier
// helpers, BasicArray / BasicMap instances).
#include "BrickPaletteControl.h"
#include "BricksControl.h"
#include "BrickFilter.h"
#include "DescriptionBrickFilter.h"
#include "BrickConnectSceneManager.h"
#include "BrickConnectSceneControl.h"
#include "MainGroupDividerControl.h"
#include "MaterialControl.h"
#include "MaterialPickerAugmentation.h"
#include "StandardTextTooltipAugmentation.h"
#include "BrickCountAugmentation.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "Texture.h"
#include "RenderControlManager.h"
#include "LuaPlus.h"
#include "LuaScriptManager.h"
#include "LuaBrickPaletteControl.h"

namespace LEGO {

namespace {

// The augmentation `name` of `control` as a T (dynamic_cast), or null. (Mac 0x69c60 /
// 0x6bae0 / 0x6bcc0: one template instance per T)
template <class T>
bool FindAugmentation(Control* control, const String& name, RefPointer<T>& result)
{
    RefPointer<ControlAugmentation> augmentation;
    if (!control->GetAugmentation(name, augmentation))
        return false;
    result = dynamic_cast<T*>(augmentation.Get());
    return !result.IsNull();
}

const char* const kTooltipAugmentationName = "tooltip";

} // namespace

// Mac 0x68460, Win 0x662f2b
BrickPaletteControl::BrickPaletteControl(ControlManager* manager,
                                         BrickConnectSceneManager* sceneManager,
                                         PaletteMaterialControl* materials, Texture* texture,
                                         bool showMainGroups, bool flag225)
    : MatrixLayoutControl(manager),
      m_sceneManager(sceneManager),
      m_materials(materials),
      m_showMainGroups(showMainGroups),
      m_flag225(flag225),
      m_dividers(),
      m_items(),
      m_filter(new CompositeBrickFilter),
      m_boxFilter(new CompositeBrickFilter),
      m_descriptionFilter(),
      m_hideMaterials(false),
      m_palette(),
      m_extendedTooltips(false),
      m_showBrickCounts(false),
      m_texture(texture)
{
    m_materials->SetBrickFilter(m_filter);
    m_descriptionFilter = new DescriptionBrickFilter(m_sceneManager->GetDatabase(), String());
    m_filter->SetFilter(m_descriptionFilter);
}

// Mac 0x6d2b0/0x6d700 (body 0x6cbb0), Win sdd 0x663c9f
BrickPaletteControl::~BrickPaletteControl()
{
}

// Mac 0x6fd10, Win 0x663264
void BrickPaletteControl::OnFinalRelease()
{
    Notifier<BrickPaletteControlObserver>::OnFinalRelease();
    SliderControlObserver::OnFinalRelease();
    BrickConnectSceneControlObserver::OnFinalRelease();
    PreferencesObserver::OnFinalRelease();
    ToggleControlObserver::OnFinalRelease();
    PaletteMaterialControlObserver::OnFinalRelease();
    ControlObserver::OnFinalRelease();
    BricksControlItemPressedNotifier::OnFinalRelease();
    MatrixLayoutControl::OnFinalRelease();
}

// Mac 0x68cc0, Win 0x662aa1
bool BrickPaletteControl::Create(const Vector2& itemSize)
{
    PaletteMaterialControlObserver::Attach(m_materials);
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        PreferencesObserver::Attach(preferences);
    }
    SetActive(true);
    SetEnabled(true);
    SetShown(true);
    SetCellSize(itemSize);
    SetMaximumSize(Vector2(100000.0f, 100000.0f));

    LuaPlus::LuaState* state;
    {
        RefPointer<LuaScriptManager> scripts = LuaScriptManager::GetInstance();
        state = scripts->GetLuaState();
    }
    LuaPlus::LuaObject globals = state->GetGlobals();
    LuaPlus::LuaObject ldd = globals.Lookup("LDD");
    RefPointer<LuaBrickPaletteControl> luaControl;
    LuaPlus::LuaObject table =
        CreateLuaControlTable<LuaBrickPaletteControl>(this, state, "BrickPaletteControl", luaControl);
    luaControl->SetBrickDatabase(m_sceneManager->GetDatabase());
    ldd.SetObject("brickPaletteControl", table);
    return true;
}

// Mac 0x69670, Win 0x6632cd
void BrickPaletteControl::Refresh()
{
    GetControlArray().Clear();
    for (DividerMap::Iterator it = m_dividers.Begin(); it != m_dividers.End(); ++it)
        it->second->SetShown(false);

    RefPointer<MaterialPickerAugmentation> picker;   // of the current design's first item
    RefPointer<BricksControl> firstVariant;
    UInt32 lastDesign = 0xffffffff;
    UInt32 lastMainGroup = 0xffffffff;
    bool expanded = false;

    for (ItemArray::Iterator it = m_items.Begin(); it != m_items.End(); ++it) {
        RefPointer<BricksControl> item = *it;
        const BrickPalette::Brick* brick = item->GetPaletteBrick();
        bool accepted = m_filter.IsNull() || m_filter->Accept(brick);
        if (!accepted) {
            item->SetShown(false);
            continue;
        }
        if (m_hideMaterials) {
            if (lastDesign == brick->m_designID) {
                // Another colour of the same design: folded into the first one's picker.
                if (picker.IsNull())
                    picker = new MaterialPickerAugmentation(m_materials, firstVariant, m_texture);
                picker->SetShowVariants(true);
                item->SetShown(false);
                continue;
            }
            picker = 0;
            if (FindAugmentation(item, kMaterialPickerAugmentationName, picker))
                picker->SetShowVariants(false);
            lastDesign = brick->m_designID;
            firstVariant = item;
        }

        UInt32 mainGroup = brick->GetMainGroup();
        if (mainGroup != lastMainGroup && m_showMainGroups) {
            RefPointer<ToggleControl> divider = GetDivider(mainGroup);
            if (divider.IsNull())
                break;
            expanded = divider->IsToggled();
            divider->SetShown(true);
            GetControlArray().PushBack(RefPointer<Control>(divider.Get()));
        }
        if (!expanded && m_showMainGroups) {
            item->SetShown(false);
        } else {
            item->SetShown(true);
            GetControlArray().PushBack(RefPointer<Control>(item.Get()));
        }
        lastMainGroup = mainGroup;
    }
    Layout();
}

// Mac 0x69d70, Win 0x662dc7
ToggleControl* BrickPaletteControl::GetDivider(UInt32 mainGroup)
{
    DividerMap::Iterator found = m_dividers.Find(mainGroup);
    if (found != m_dividers.End())
        return found->second;

    MainGroupDividerControl* divider = new MainGroupDividerControl(GetManager());
    if (!divider->Create(this, mainGroup, m_sceneManager->GetDatabase())) {
        static_cast<Object*>(divider)->Release();
        return 0;
    }
    ToggleControlObserver::Attach(divider);
    LDDApplication::GetInstance()->AddButtonSound(divider);
    m_dividers[mainGroup] = divider;
    return divider;
}

// Mac 0x6a0a0 / 0x6a900 (shared body)
void BrickPaletteControl::SetMainGroupsExpanded(bool expanded)
{
    ToggleControlObserver::DetachAll();
    for (DividerMap::Iterator it = m_dividers.Begin(); it != m_dividers.End(); ++it)
        it->second->SetToggled(expanded);
    for (DividerMap::Iterator it = m_dividers.Begin(); it != m_dividers.End(); ++it)
        ToggleControlObserver::Attach(it->second);
    NotifyMainGroupsExpanded(expanded);
    Refresh();
}

// Mac 0x6a0a0, Win 0x663544
void BrickPaletteControl::CollapseMainGroups()
{
    SetMainGroupsExpanded(false);
}

// Mac 0x6a900, Win 0x663645
void BrickPaletteControl::ExpandMainGroups()
{
    SetMainGroupsExpanded(true);
}

// (inline in the original: Mac 0x6a1ed ff.)
void BrickPaletteControl::NotifyMainGroupsExpanded(bool expanded)
{
    Notifier<BrickPaletteControlObserver>::Iterator it(*this);
    while (BrickPaletteControlObserver* observer =
               it.Next(Notifier<BrickPaletteControlObserver>::m_observers))
        observer->OnMainGroupsExpanded(expanded);
}

// Mac 0x6ac60, Win 0x661f03
bool BrickPaletteControl::AreMainGroupsCollapsed() const
{
    for (DividerMap::ConstIterator it = m_dividers.Begin(); it != m_dividers.End(); ++it)
        if (it->second->IsToggled())
            return false;
    return true;
}

// Mac 0x6acb0, Win 0x663746
bool BrickPaletteControl::SetBrickPalette(BrickPalette* palette)
{
    m_descriptionFilter->ClearCaches();
    m_palette = palette;
    m_items.Clear();
    for (BrickPalette::BrickSet::ConstIterator it = palette->GetBricksBegin();
         it != palette->GetBricksEnd(); ++it)
        AddItem(*it);
    Refresh();
    return true;
}

// Mac 0x6ada0, Win 0x661cc0
BricksControl* BrickPaletteControl::AddItem(const BrickPalette::Brick* brick)
{
    RefPointer<BricksControl> item = CreateBricksControl(
        brick, GetManager(), m_sceneManager, static_cast<ControlObserver*>(this), this,
        m_extendedTooltips);
    if (item.IsNull())
        return 0;
    m_items.PushBack(item);
    return item;
}

// Mac 0x6ae80 (thunk 0x6b1b0), Win 0x6637c2
void BrickPaletteControl::AddFilter(const BrickFilter* filter)
{
    m_filter->SetFilter(filter);
    if (filter->GetType() == BrickFilter::kTypeBox)
        m_boxFilter->SetFilter(filter);
    UpdateBrickCountObserver();
    Refresh();
}

// Mac 0x6b210 (thunk 0x6b260), Win 0x663801
void BrickPaletteControl::RemoveFilterInstance(const BrickFilter* filter)
{
    m_filter->RemoveFilter(filter);
    if (filter->GetType() == BrickFilter::kTypeBox)
        m_boxFilter->RemoveFilter(filter);
    UpdateBrickCountObserver();
    Refresh();
}

// Mac 0x6b2c0 (thunk 0x6b310), Win 0x663840
void BrickPaletteControl::RemoveFilter(const BrickFilter* filter)
{
    m_filter->RemoveFiltersOfType(filter);
    if (filter->GetType() == BrickFilter::kTypeBox)
        m_boxFilter->RemoveFilter(filter);
    UpdateBrickCountObserver();
    Refresh();
}

// Mac 0x6b370 (thunk 0x6b3c0), Win 0x66387f
void BrickPaletteControl::ClearFilter()
{
    m_filter->Clear();
    m_boxFilter->Clear();
    m_filter->SetFilter(m_descriptionFilter);
    UpdateBrickCountObserver();
    Refresh();
}

// Mac 0x6ffc0 (thunk 0x6ffd0), Win 0x66325d
CompositeBrickFilter* BrickPaletteControl::GetFilter() const
{
    return m_filter;
}

// Mac 0x6aed0, Win 0x662c10
void BrickPaletteControl::UpdateBrickCountObserver()
{
    if (m_showBrickCounts && !m_filter.IsNull() && m_filter->IsCounting() &&
        !LDDApplication::GetInstance()->IsBrandLDDExtended()) {
        if (BrickConnectSceneControlObserver::GetSubject() != 0) {
            OnEvent16();
            return;
        }
        BrickConnectSceneControl* sceneControl = m_sceneManager->GetSceneControl();
        if (sceneControl == 0)
            return;
        BrickConnectSceneControlObserver::Attach(sceneControl);
        return;
    }
    BrickConnectSceneControlObserver::Detach();
}

// Mac 0x6b420
void BrickPaletteControl::SetHideMaterials(bool hide)
{
    if (hide == m_hideMaterials)
        return;
    m_hideMaterials = hide;
    ResetMaterialPickers(hide);
    UpdateBrickCountObserver();
    Refresh();
}

// Mac 0x6b470
void BrickPaletteControl::ResetMaterialPickers(bool hide)
{
    if (hide)
        return;
    for (ItemArray::Iterator it = m_items.Begin(); it != m_items.End(); ++it) {
        RefPointer<BricksControl> item = *it;
        RefPointer<MaterialPickerAugmentation> picker;
        if (FindAugmentation(item, kMaterialPickerAugmentationName, picker))
            picker->SetShowVariants(false);
    }
}

// Mac 0x6b5a0
void BrickPaletteControl::SetBrickFilterDescription(const String& description)
{
    m_descriptionFilter->SetText(description);
    UpdateBrickCountObserver();
    Refresh();
}

// Mac 0x6b5e0, Win 0x661bd3
void BrickPaletteControl::SelectMaterial(UInt32 material)
{
    Material* renderMaterial = m_sceneManager->GetMaterial(material, 1);
    for (ItemArray::Iterator it = m_items.Begin(); it != m_items.End(); ++it)
        (*it)->SetMaterial(renderMaterial, material);
    if (!static_cast<RenderControlManager*>(GetManager())->IsUpdateLocked())
        Invalidate(false);
}

// Mac 0x6b960, Win 0x6625d8
void BrickPaletteControl::UpdateTooltips()
{
    const BrickDatabase* database = m_sceneManager->GetDatabase();
    for (ItemArray::Iterator it = m_items.Begin(); it != m_items.End(); ++it) {
        BricksControl* item = *it;
        String text = m_extendedTooltips
            ? LDDUtilities::GetBrickDescription(database, item->GetPaletteBrick())
            : database->GetLocalizedName(item->GetPaletteBrick()->m_designID);
        RefPointer<StandardTextTooltipAugmentation> tooltip;
        if (FindAugmentation(item, String(kTooltipAugmentationName), tooltip))
            tooltip->SetText(text);
        else
            CreateStandardTextTooltip(item, text);
    }
}

// Mac 0x6bbf0, Win 0x662709
void BrickPaletteControl::HideBrickCounts()
{
    for (ItemArray::Iterator it = m_items.Begin(); it != m_items.End(); ++it) {
        RefPointer<BrickCountAugmentation> counter;
        if (FindAugmentation(*it, kBrickCountAugmentationName, counter))
            counter->GetLabel()->SetShown(false);
    }
}

// Mac 0x6bdd0 (thunk 0x6be20), Win 0x6616a9
void BrickPaletteControl::OnMousePressed(Control* control, const Vector2& /*position*/, UInt32 button)
{
    if (button == 0)
        NotifyBrickPressed(static_cast<BricksControl*>(control)->GetBrick());
}

// Mac 0x6be50 (thunk 0x6be80), Win 0x6616c9
void BrickPaletteControl::OnBricksControlClicked(BricksControl* item)
{
    NotifyBrickPressed(item->GetBrick());
}

// Mac 0x6beb0 (thunk 0x6c150), Win 0x66390f
void BrickPaletteControl::OnToggled(ToggleControl* /*control*/)
{
    NotifyMainGroupsExpanded(true);
    Refresh();
}

// Mac 0x6c170 (thunk 0x6c410), Win 0x66396d
void BrickPaletteControl::OnUntoggled(ToggleControl* /*control*/)
{
    NotifyMainGroupsExpanded(false);
    Refresh();
}

// Mac 0x6c430 (thunk 0x6c650), Win 0x6639cb
void BrickPaletteControl::OnPreferenceChanged(const String& key)
{
    if (key == String("ShowExtendedBrickToolTip")) {
        String value;
        if (PreferencesObserver::GetSubject()->GetValue(String("ShowExtendedBrickToolTip"), value))
            FromString(m_extendedTooltips, value);
        UpdateTooltips();
    } else if (key == String("SORTBYIDONLY")) {
        Refresh();
    } else if (key == String("SHOWBRICKCOUNTS")) {
        String value;
        if (PreferencesObserver::GetSubject()->GetValue(String("SHOWBRICKCOUNTS"), value))
            FromString(m_showBrickCounts, value);
        UpdateBrickCountObserver();
    }
}

// Mac 0x6ca50 (thunk 0x6cb90), Win 0x661a27
void BrickPaletteControl::OnAttached(Preferences* preferences)
{
    String value;
    if (preferences->GetValue(String("ShowExtendedBrickToolTip"), value))
        FromString(m_extendedTooltips, value);
    String counts;
    if (preferences->GetValue(String("SHOWBRICKCOUNTS"), counts))
        FromString(m_showBrickCounts, counts);
}

// Mac 0x6c670 (thunk 0x6c9a0), Win 0x6627f4
void BrickPaletteControl::OnEvent16()
{
    BrickConnectSceneControl* sceneControl = BrickConnectSceneControlObserver::GetSubject();
    for (ItemArray::Iterator it = m_items.Begin(); it != m_items.End(); ++it) {
        RefPointer<BricksControl> item = *it;
        const BrickPalette::Brick* brick = item->GetPaletteBrick();
        if (!m_filter->Accept(brick))
            continue;
        Int32 left = (Int32)m_filter->GetCount(brick);
        Int32 used = (Int32)sceneControl->GetMissingBrickCount(brick);
        RefPointer<BrickCountAugmentation> counter;
        if (!FindAugmentation(item, kBrickCountAugmentationName, counter))
            continue;
        left -= used;
        FramedTextControl* label = counter->GetLabel();
        if (left < 0) {
            Rect frame(384.0f, 27.0f, 391.0f, 34.0f);
            label->SetImageRect(&frame);
            label->SetFont(LDDApplication::GetInstance()->GetFont(15));
            String text;
            text << left;
            label->SetText(text);
        } else {
            Rect frame(374.0f, 27.0f, 381.0f, 34.0f);
            label->SetImageRect(&frame);
            label->SetFont(LDDApplication::GetInstance()->GetFont(0));
            String text;
            text << left;
            label->SetText(text);
        }
        label->SetShown(true);
    }
}

// Mac 0x6c9c0 (thunk 0x6c9e0), Win 0x5c801e
void BrickPaletteControl::OnAttached(BrickConnectSceneControl* control)
{
    control->SetShowMissingBricks(true);
}

// Mac 0x6ca00 (thunk 0x6ca20), Win 0x6629ee
void BrickPaletteControl::OnDetached(BrickConnectSceneControl* control)
{
    control->SetShowMissingBricks(false);
    HideBrickCounts();
}

// Mac 0x6b660 (thunk 0x6b6e0), Win 0x661614
void BrickPaletteControl::OnAttached(SliderControl* slider)
{
    Real32 scale = slider->GetValue() / 100.0f;
    SetScale(Vector2(scale, slider->GetValue() / 100.0f));
}

// Mac 0x6b770 (thunk 0x6b7c0), Win 0x661666
void BrickPaletteControl::OnValueChanged(SliderControl* /*slider*/, Real32 value, Real32 /*oldValue*/)
{
    SetScale(Vector2(value / 100.0f, value / 100.0f));
    Layout();
}

} // namespace LEGO
