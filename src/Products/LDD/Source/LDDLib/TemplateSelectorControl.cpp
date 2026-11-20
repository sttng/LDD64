// LEGO Digital Designer — restored source
// TemplateSelectorControl.cpp — TU 074 (Mac 0x166110-0x1697fb; Win 0x679dc9-0x67ad08):
// TemplateSelectorControl, DeleteGroupAugmentation, DeleteGroupButtonControl. The inline
// StorageSceneControl constructor of this object is in StorageSceneControl.cpp.
// See TemplateSelectorControl.h.
#include "TemplateSelectorControl.h"
#include "StorageSceneControl.h"
#include "StorageScene2.h"
#include "BrickConnectSceneManager.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "Brand.h"
#include "TextLocale.h"
#include "Color.h"
#include "Geometry.h"

namespace LEGO {

// ===========================================================================================
// TemplateSelectorControl
// ===========================================================================================

// Mac 0x166790, Win 0x67aa8f
TemplateSelectorControl::TemplateSelectorControl(ControlManager* manager,
                                                 BrickConnectSceneManager* sceneManager,
                                                 BrickPalette* palette,
                                                 BricksControlItemPressedObserver* observer,
                                                 UInt32 cursor)
    : MatrixLayoutControl(manager), m_sceneManager(sceneManager), m_cursor(cursor)
{
    BrickPaletteObserver::Attach(palette);
    observer->Attach(static_cast<BricksControlItemPressedNotifier*>(this));
    SetEnabled(true);
    SetShown(true);
    SetActive(true);
    SetCellSize(Vector2(128.0f, 128.0f));
    SetSpacing(10.0f);
    Rebuild();
}

// Mac 0x168210 / 0x1684a0 (body 0x167e00), Win sdd 0x67a626 (body 0x67a52a)
TemplateSelectorControl::~TemplateSelectorControl()
{
}

// Mac 0x168790 (thunks 0x1688d0, 0x1688f0, 0x168910, 0x168930, 0x168950), Win 0x67a653
void TemplateSelectorControl::OnFinalRelease()
{
    // ControlObserver: leave the items (no OnDetached)
    for (UInt32 i = 0; i < ControlObserver::m_subjects.GetSize(); ++i)
        static_cast<NotifierT<Control, ControlObserver>*>(ControlObserver::m_subjects[i])
            ->RemoveObserver(static_cast<ControlObserver*>(this));
    ControlObserver::m_subjects.Clear();
    // BricksControlItemPressedNotifier: every observer leaves (last first)
    while (!BricksControlItemPressedNotifier::m_observers.IsEmpty())
        BricksControlItemPressedNotifier::m_observers.Back()->Detach(
            static_cast<BricksControlItemPressedNotifier*>(this));
    // BrickPaletteObserver
    BrickPaletteObserver::Detach();
    // SliderControlObserver: leave the sliders (no OnDetached)
    for (UInt32 i = 0; i < SliderControlObserver::m_subjects.GetSize(); ++i)
        static_cast<NotifierT<SliderControl, SliderControlObserver>*>(SliderControlObserver::m_subjects[i])
            ->RemoveObserver(static_cast<SliderControlObserver*>(this));
    SliderControlObserver::m_subjects.Clear();
    MatrixLayoutControl::OnFinalRelease();
}

// Mac 0x167980, Win 0x67a7f7
StorageSceneControl* TemplateSelectorControl::CreateItem(const StorageScene2* model, Control* parent)
{
    // NOTE: an item whose model cannot be set would be leaked (SetModel always succeeds); fixed:
    // released (low).
    StorageSceneControl* item = new StorageSceneControl(GetManager(), m_sceneManager);
    if (!item->SetModel(const_cast<StorageScene2*>(model))) {
        RefPointer<StorageSceneControl> release = item;
        return 0;
    }
    item->SetSize(GetCellSize());
    item->SetParent(parent, 0);
    item->SetActive(true);
    item->SetEnabled(true);
    item->SetShown(true);
    item->SetClearColor(Color::Clear());
    item->SetBufferOptions(2, 1, Vector2(2.0f, 1.0f));
    item->SetBuffered(true);
    item->SetCursor(m_cursor, true);
    return item;
}

// Mac 0x166cb0, Win 0x67a8d5
StorageSceneControl* TemplateSelectorControl::AddModel(const StorageScene2* model)
{
    BasicArray<RefPointer<const BrickPalette::Brick> > bricks;
    StorageSceneControl* item = 0;
    if (CreatePaletteBricks(const_cast<StorageScene2&>(*model), m_sceneManager->GetDatabase(), bricks, false)
        && !bricks.IsEmpty()) {
        item = CreateItem(model, this);
        // NOTE: a null item is added / observed unchecked (both builds; CreateItem cannot fail).
        AddControl(item);
        ControlObserver::Attach(item);
        m_items[LDDApplication::GetInstance()->GetBrand()].PushBack(RefPointer<StorageSceneControl>(item));
    }
    return item;
}

// Mac 0x166670, Win 0x67a9ac
void TemplateSelectorControl::Rebuild()
{
    RemoveAllControls();
    const Brand brand = LDDApplication::GetInstance()->GetBrand();
    const ItemArray& items = m_items[brand];
    if (!items.IsEmpty()) {
        for (ItemArray::ConstIterator it = items.Begin(); it != items.End(); ++it)
            AddControl(it->Get());
        return;
    }
    BrickPalette* palette = BrickPaletteObserver::GetSubject();
    for (BrickPalette::ModelMap::ConstIterator it = palette->GetModelsBegin(); it != palette->GetModelsEnd(); ++it) {
        const StorageScene2* model = it->first.Get();
        Brand modelBrand;
        if (!LDDUtilities::GetModelBrand(model, modelBrand) || modelBrand != brand)
            continue;
        StorageSceneControl* item = AddModel(model);
        if (item)
            new DeleteGroupAugmentation(item);   // kept by the item (augmentation)
    }
}

// Mac 0x166c00 (thunk 0x166c10)
void TemplateSelectorControl::OnBagAdded(const BrickPalette::Bag*)
{
}

// Mac 0x166c20 (thunk 0x1672a0), Win 0x67ac29
void TemplateSelectorControl::OnModelAdded(StorageScene2* model)
{
    Brand modelBrand;
    if (LDDUtilities::GetModelBrand(model, modelBrand)
        && LDDApplication::GetInstance()->GetBrand() == modelBrand) {
        StorageSceneControl* item = AddModel(model);
        if (item)
            new DeleteGroupAugmentation(item);
    }
    Rebuild();
}

// Mac 0x1672c0 (thunk 0x1673b0), Win 0x67aca7
void TemplateSelectorControl::OnModelRemoved(StorageScene2* model)
{
    ItemArray& items = m_items[LDDApplication::GetInstance()->GetBrand()];
    for (UInt32 i = 0; i < items.GetSize(); ++i) {
        if (items[i]->GetModel() != model)
            continue;
        RemoveControl(items[i].Get());
        items.Erase(i);
        Rebuild();
        return;
    }
}

// Mac 0x1673d0 (thunk 0x1673e0), Win → 0x67a9ac
void TemplateSelectorControl::OnPaletteChanged()
{
    Rebuild();
}

// Mac 0x167400 (thunk 0x167480), Win 0x679dc9
void TemplateSelectorControl::OnAttached(SliderControl* slider)
{
    const Real32 x = slider->GetValue() / 100.0f;
    SetScale(Vector2(x, slider->GetValue() / 100.0f));
}

// Mac 0x167510 (thunk 0x167560), Win 0x679e1b
void TemplateSelectorControl::OnValueChanged(SliderControl*, Real32 value, Real32)
{
    SetScale(Vector2(value / 100.0f, value / 100.0f));
}

// Mac 0x167650
void TemplateSelectorControl::NotifyModelPressed(StorageScene2* model)
{
    BricksControlItemPressedNotifier& notifier = *this;
    BricksControlItemPressedNotifier::Iterator pass(notifier);
    while (BricksControlItemPressedObserver* observer =
               pass.Next(BricksControlItemPressedNotifier::m_observers))
        observer->OnModelPressed(model);
}

// Mac 0x1675b0 (thunk 0x1678d0), Win 0x679edd
void TemplateSelectorControl::OnMouseClicked(Control* control, const Vector2&, UInt32 button)
{
    if (button != 0)
        return;
    StorageSceneControl* item = dynamic_cast<StorageSceneControl*>(control);
    if (!item)
        return;
    RefPointer<StorageScene2> model = item->GetModel();
    if (model)
        NotifyModelPressed(model.Get());
}

// ===========================================================================================
// DeleteGroupButtonControl
// ===========================================================================================

// Mac 0x168e80, Win 0x679fdc
void DeleteGroupButtonControl::OnMouseClicked(const Vector2&, UInt32)
{
    // NOTE: any button; the base handler is not called (both builds).
    LDDApplication* application = LDDApplication::GetInstance();
    const UInt32 answer = application->Alert(3, LocalizedString(String("DELETETEMPLATEDIALOGUE")),
                                             LocalizedString(String("DELETETEMPLATEDIALOGUETITLE")),
                                             LocalizedString(String("YES")),
                                             LocalizedString(String("NO")), String());
    if (answer == 0)
        LDDApplication::GetInstance()->GetBrickPalette()->RemoveModel(
            static_cast<StorageSceneControl*>(GetParent())->GetModel());
}

// ===========================================================================================
// DeleteGroupAugmentation
// ===========================================================================================

// Mac 0x166ea0, Win 0x67a1e0
DeleteGroupAugmentation::DeleteGroupAugmentation(StorageSceneControl* item)
    : m_button(), m_image(), m_item(item)
{
    item->SetImage(String("LDDgraphics01.templatebox"));
    item->SetFrame(Vector3(9.0f, 0.0f, 9.0f), Vector3(9.0f, 0.0f, 9.0f), true);
    item->SetDrawAsFrame(true);
    item->SetSize(Vector2(128.0f, 128.0f));
    item->SetTransparent(true);
    m_button = new DeleteGroupButtonControl(item->GetManager());
    m_button->Create("LDDgraphics01.deletebutton", item, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 1, true);
    m_button->SetAlignment(0x13, 0x13, 3.0f, -3.0f, 0);
    Attach(item, String("DeleteGroupAugmentation"));
    ObserveControl(m_button.Get());
}

// Mac 0x168970 / 0x1689d0 (body 0x169390), Win sdd 0x67a50b
DeleteGroupAugmentation::~DeleteGroupAugmentation()
{
}

// Mac 0x168a40 (thunk 0x168bf0), Win 0x67a40a
void DeleteGroupAugmentation::OnMouseEnter(Control*, const Vector2&)
{
    m_item->SetImage(String("LDDgraphics01.templateboxover"));
}

// Mac 0x168ab0 (thunk 0x168c60), Win 0x67a449
void DeleteGroupAugmentation::OnMouseLeave(Control*, const Vector2&)
{
    m_item->SetImage(String("LDDgraphics01.templatebox"));
}

} // namespace LEGO
