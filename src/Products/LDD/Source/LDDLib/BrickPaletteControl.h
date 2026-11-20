// LEGO Digital Designer — restored source
// LEGO::BrickPaletteControl — the edit mode's brick palette: one BricksControl item per palette
//   brick (sort order), laid out by MatrixLayoutControl, filtered by a composite brick filter
//   (bag / material / text search). Main-group dividers fold the groups; colour variants can fold
//   behind a MaterialPickerAugmentation ("hide materials"); with SHOWBRICKCOUNTS and a counting
//   filter items show the bricks left. Pressing an item notifies the
//   BricksControlItemPressedObservers. Lua: "LDD.brickPaletteControl" (LuaBrickPaletteControl).
// LEGO::BrickPaletteControlObserver — told when the main groups were expanded / collapsed
//   (Collapse/ExpandMaingroupsCommand).
//
// Home TU 023 "BrickPaletteControl.cpp" (Mac 0x683c0-0x71fbb; Win 0x661614-0x663c9f). The
// palette's item factory is LDDUtilities' CreateBricksControl (BricksControl.h).
//
// RTTI vmi (Mac offset / Win offset): MatrixLayoutControl (+0), FilterControlManager (+0x17c /
//   +0x190), BricksControlItemPressedNotifier (+0x180 / +0x200), ControlObserver (+0x1a4 /
//   +0x194), PaletteMaterialControlObserver (+0x1b8 / +0x1ac), ToggleControlObserver (+0x1c0 /
//   +0x1b8), PreferencesObserver (+0x1d4 / +0x1d0), BrickConnectSceneControlObserver (+0x1dc /
//   +0x1dc), SliderControlObserver (+0x1e4 / +0x1e8), Notifier<BrickPaletteControlObserver>
//   (+0x1f8 / +0x224); Object Mac +0x270 / Win +0x2a8. Mac sizeof 0x278, Win 0x2b0.
// Virtual slots: Mac 0x7051ac (52 entries): Control's 0..35 (OnFinalRelease 2 = 0x6fd10, D1/D0
//   0x6d2b0/0x6d700, OnRectangleChanged 8 = MatrixLayoutControl's), then the overriders of the
//   secondary bases in declaration order, 36..51 (see the class). Win: the base vftables
//   (FilterControlManager 0x9d795c {0x6637c2, 0x663840, 0x663801, 0x66387f, 0x66325d}, …).
// Names inferred except the Lua ones (SetBrickFilterDescription, ClearFilter) and the
// preference keys.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Notifier.h"
#include "ObserverN.h"
#include "MatrixLayoutControl.h"
#include "ToggleControl.h"
#include "SliderControl.h"
#include "Preferences.h"
#include "FilterControlManager.h"
#include "BricksControlItemPressedNotifier.h"
#include "BrickConnectSceneControlObserver.h"
#include "PaletteMaterialControl.h"
#include "BrickPalette.h"
#include "BrickFilter.h"
#include "DescriptionBrickFilter.h"
#include "BrickConnectSceneManager.h"
#include "Texture.h"

namespace LEGO {

class BrickPaletteControl;
class BrickConnectSceneManager;
class BrickConnectSceneControl;
class PaletteMaterialControl;
class BricksControl;
class BrickFilter;
class CompositeBrickFilter;
class DescriptionBrickFilter;
class MaterialPickerAugmentation;
class Texture;

// Header inline (vtable emitted with the commands: Win D0 0x66dd8c).
class BrickPaletteControlObserver
    : public Observer<BrickPaletteControl, BrickPaletteControlObserver> {
public:
    BrickPaletteControlObserver() {}
    virtual ~BrickPaletteControlObserver() {}
    // The main groups were expanded (true: a divider toggled on / ExpandMainGroups) or
    // collapsed. Mac slot 5, Win slot 2. (name inferred)
    virtual void OnMainGroupsExpanded(bool /*expanded*/) {}
};

class BrickPaletteControl : public MatrixLayoutControl, public FilterControlManager,
                            public BricksControlItemPressedNotifier, public ControlObserver,
                            public PaletteMaterialControlObserver, public ToggleControlObserver,
                            public PreferencesObserver, public BrickConnectSceneControlObserver,
                            public SliderControlObserver,
                            public Notifier<BrickPaletteControlObserver> {
public:
    typedef BasicArray<RefPointer<BricksControl> >               ItemArray;
    typedef BasicMap<UInt32, RefPointer<ToggleControl>, Less<UInt32> > DividerMap;

    // `showMainGroups`: main-group dividers; `flag225` is stored only. The filter: the
    // material control gets the first composite filter, which holds the description filter.
    // Mac 0x68460, Win 0x662f2b (sizeof Mac 0x278, Win 0x2b0)
    BrickPaletteControl(ControlManager* manager, BrickConnectSceneManager* sceneManager,
                        PaletteMaterialControl* materials, Texture* texture, bool showMainGroups,
                        bool flag225);
    virtual ~BrickPaletteControl();                                         // Mac 0x6d2b0/0x6d700 (body 0x6cbb0), Win sdd 0x663c9f

    // Observes the material control and the Preferences, shows the control with `itemSize`
    // cells and registers its Lua table "LDD.brickPaletteControl". Always true.
    // Mac 0x68cc0, Win 0x662aa1
    bool Create(const Vector2& itemSize);
    // Rebuilds the items: one per brick of `palette` (current brick set); then Refresh.
    // Mac 0x6acb0, Win 0x663746
    bool SetBrickPalette(BrickPalette* palette);
    // Hides (true) / shows the colour variants. Mac 0x6b420
    void SetHideMaterials(bool hide);
    bool GetHideMaterials() const { return m_hideMaterials; }
    // LDD Extended: every item gets `material`. Mac 0x6b5e0, Win 0x661bd3
    void SelectMaterial(UInt32 material);
    // Filters the bricks by a description text (Lua "SetBrickFilterDescription"). Mac 0x6b5a0
    void SetBrickFilterDescription(const String& description);
    // Collapse / expand every main group; notifies OnMainGroupsExpanded. Mac 0x6a0a0 / 0x6a900,
    // Win 0x663544 / 0x663645
    void CollapseMainGroups();
    void ExpandMainGroups();
    // No main group expanded. Mac 0x6ac60, Win 0x661f03
    bool AreMainGroupsCollapsed() const;

    // The palette shown (Mac +0x264, Win +0x298; read inline by LuaBrickPaletteControl
    // GetNumberOfBags 0x37d150).
    BrickPalette* GetBrickPalette() const { return m_palette; }
    // The material picker given to the constructor (Mac +0x220, Win +0x24c).
    // NOTE: the member +0x258 that EditMode passes to MaterialTool::Create (Mac 0x186423) is
    // the second composite filter (GetMaterialFilter below), not this control.
    PaletteMaterialControl* GetPaletteMaterialControl() const { return m_materials; }
    // The composite filter of the bag (box) filters (Mac +0x258, Win +0x28c).
    CompositeBrickFilter* GetMaterialFilter() const { return m_boxFilter; }
    const ItemArray& GetItems() const { return m_items; }
    const DividerMap& GetDividers() const { return m_dividers; }

    // ---- FilterControlManager (Mac primary slots 36-40; Win via +0x190 slots 0, 2, 1, 3, 4) ----
    // Adds `filter` (replacing a filter of its type); a box filter also goes to the box
    // filters. Mac 0x6ae80 (thunk 0x6b1b0), Win 0x6637c2
    virtual void AddFilter(const BrickFilter* filter);                      // 36
    // Removes exactly `filter`. Mac 0x6b210 (thunk 0x6b260), Win 0x663801
    virtual void RemoveFilterInstance(const BrickFilter* filter);           // 37
    // Removes the filters of the type of `filter` (a box filter: exactly it from the box
    // filters). Mac 0x6b2c0 (thunk 0x6b310), Win 0x663840
    virtual void RemoveFilter(const BrickFilter* filter);                   // 38
    // Clears both composites; the description filter stays. Lua "ClearFilter".
    // Mac 0x6b370 (thunk 0x6b3c0), Win 0x66387f
    virtual void ClearFilter();                                             // 39
    // Mac 0x6ffc0 (thunk 0x6ffd0), Win 0x66325d
    virtual CompositeBrickFilter* GetFilter() const;                        // 40

    // ---- ControlObserver: an item pressed with button 0 -> NotifyBrickPressed ----
    // Mac 0x6bdd0 (thunk 0x6be20), Win 0x6616a9
    virtual void OnMousePressed(Control* control, const Vector2& position, UInt32 button); // 41
    // ---- PaletteMaterialControlObserver: a variant chosen in the material picker ----
    // Mac 0x6be50 (thunk 0x6be80), Win 0x6616c9
    virtual void OnBricksControlClicked(BricksControl* item);              // 42
    // ---- ToggleControlObserver: a divider toggled -> notify, Refresh ----
    // Mac 0x6beb0 / 0x6c170 (thunks 0x6c150 / 0x6c410), Win 0x66390f / 0x66396d
    virtual void OnToggled(ToggleControl* control);                         // 43
    virtual void OnUntoggled(ToggleControl* control);                       // 44
    // ---- PreferencesObserver: ShowExtendedBrickToolTip -> tooltips, SORTBYIDONLY -> Refresh,
    // SHOWBRICKCOUNTS -> the counting observer. Mac 0x6c430 (thunk 0x6c650), Win 0x6639cb
    virtual void OnPreferenceChanged(const String& key);                    // 45
    // ---- BrickConnectSceneControlObserver: the scene changed -> the brick counts ----
    // Mac 0x6c670 (thunk 0x6c9a0), Win 0x6627f4
    virtual void OnEvent16();                                               // 46

protected:
    // The scene control shows missing bricks while observed. Mac 0x6c9c0 (thunk 0x6c9e0) /
    // 0x6ca00 (thunk 0x6ca20; also hides the counts), Win 0x5c801e (ICF) / 0x6629ee
    virtual void OnAttached(BrickConnectSceneControl* control);             // 47
    virtual void OnDetached(BrickConnectSceneControl* control);             // 48
    // Reads ShowExtendedBrickToolTip / SHOWBRICKCOUNTS. Mac 0x6ca50 (thunk 0x6cb90), Win 0x661a27
    virtual void OnAttached(Preferences* preferences);                      // 49
    // Scale = the slider's value / 100. Mac 0x6b660 (thunk 0x6b6e0), Win 0x661614
    virtual void OnAttached(SliderControl* slider);                         // 50
    // Scale = value / 100, then Layout. Mac 0x6b770 (thunk 0x6b7c0), Win 0x661666
    virtual void OnValueChanged(SliderControl* slider, Real32 value, Real32 oldValue); // 51

    // Detaches every observer base, then Control's. Mac 0x6fd10, Win 0x663264
    virtual void OnFinalRelease();

    // (the other bases' handlers of the same names stay visible)
    using MatrixLayoutControl::OnMousePressed;
    using ControlObserver::OnAttached;
    using ControlObserver::OnDetached;
    using PaletteMaterialControlObserver::OnAttached;
    using PaletteMaterialControlObserver::OnDetached;
    using ToggleControlObserver::OnAttached;
    using ToggleControlObserver::OnDetached;
    using PreferencesObserver::OnDetached;
    using SliderControlObserver::OnDetached;

    // Rebuilds the layout from the items: filter, colour variants, main-group dividers.
    // Mac 0x69670, Win 0x6632cd
    void Refresh();
    // The divider of `mainGroup` (made on first use; null when it cannot be made).
    // Mac 0x69d70, Win 0x662dc7
    ToggleControl* GetDivider(UInt32 mainGroup);
    // A new item for `brick` (appended). Mac 0x6ada0, Win 0x661cc0
    BricksControl* AddItem(const BrickPalette::Brick* brick);
    // Attaches / detaches the scene-control observer as the count display needs it
    // (SHOWBRICKCOUNTS, a counting filter, not LDD Extended). Mac 0x6aed0, Win 0x662c10
    void UpdateBrickCountObserver();
    // !hide: every item's material picker loses its variants. Mac 0x6b470
    void ResetMaterialPickers(bool hide);
    // Tooltips of every item (name / full description). Mac 0x6b960, Win 0x6625d8
    void UpdateTooltips();
    // Hides every item's count label. Mac 0x6bbf0, Win 0x662709
    void HideBrickCounts();
    // Sets every divider's toggle state (observer off), then notifies. (shared body of
    // Mac 0x6a0a0 / 0x6a900)
    void SetMainGroupsExpanded(bool expanded);
    // Notifies OnMainGroupsExpanded. (inline in the original)
    void NotifyMainGroupsExpanded(bool expanded);

    // ---- data (Mac offsets / Win offsets) ----
    RefPointer<BrickConnectSceneManager> m_sceneManager;     // +0x21c / +0x248
    RefPointer<PaletteMaterialControl>   m_materials;        // +0x220 / +0x24c
    bool                                 m_showMainGroups;   // +0x224 / +0x250
    bool                                 m_flag225;          // +0x225 / +0x251 (not read)
    DividerMap                           m_dividers;         // +0x228 / +0x254 main group -> divider
    ItemArray                            m_items;            // +0x244 / +0x278
    RefPointer<CompositeBrickFilter>     m_filter;           // +0x254 / +0x288
    RefPointer<CompositeBrickFilter>     m_boxFilter;        // +0x258 / +0x28c
    RefPointer<DescriptionBrickFilter>   m_descriptionFilter;// +0x25c / +0x290
    bool                                 m_hideMaterials;    // +0x260 / +0x294
    RefPointer<BrickPalette>             m_palette;          // +0x264 / +0x298
    bool                                 m_extendedTooltips; // +0x268 / +0x29c ShowExtendedBrickToolTip
    bool                                 m_showBrickCounts;  // +0x269 / +0x29d SHOWBRICKCOUNTS
    RefPointer<Texture>                  m_texture;          // +0x26c / +0x2a0
                                                             // Object +0x270 / +0x2a8
};

} // namespace LEGO
