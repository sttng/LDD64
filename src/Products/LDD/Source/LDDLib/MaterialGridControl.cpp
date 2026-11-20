// LEGO Digital Designer — restored source
// TU 185 "MaterialGridControl.cpp": Mac 0x23cc30-0x23fcdb, Win 0x6640b7-0x664c47.
// The constructor is inline (EditMode.cpp, Mac 0x18cb50 / Win 0x62d415) and restored here.
// Not code: the Notifier<MaterialGridControlObserver> / NotifierT / BasicArray<RefPointer<Control>>
// / BasicMap template instances (Mac 0x23e940-0x23ed90, 0x23f690-0x23fb90; 0x192f40 in TU 085),
// construction-vtable thunks (0x23f030-0x23f670), Mac 0x23fc60 = <iostream> + a header static.
#include "MaterialGridControl.h"
#include "MaterialItemControl.h"
#include "ArrayLayoutControl.h"
#include "MatrixLayoutControl.h"
#include "RenderControlManager.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "BrickPalette.h"
#include "TextLocale.h"
#include "Texture.h"

namespace LEGO {

namespace {

const Real32 kItemSize       = 34.0f;    // MaterialItemControl (the swatch image size)
const Real32 kItemSpacing    = 10.0f;    // between the swatches of a section
const Real32 kSectionSpacing = 30.0f;    // between the sections
const Real32 kHeaderHeight   = 20.0f;
const Real32 kBorder         = 20.0f;    // pop-up frame around the section layout
const UInt32 kHeaderFont     = 0xe;      // LDDApplication::GetFont

} // namespace

// Mac 0x18cb50 (inline, EditMode.cpp), Win 0x62d415
MaterialGridControl::MaterialGridControl(ControlManager* manager, const BrickDatabase* database)
    : LDDPopUpControl(manager), m_layout(), m_items(), m_itemSize(kItemSize, kItemSize),
      m_selected(0), m_controls(), m_database(database)
{
}

// Mac 0x23cc30, Win 0x6642a0
bool MaterialGridControl::Create(Control* parent, Control* after)
{
    if (!LDDPopUpControl::Create(parent, after))
        return false;
    m_layout = new ArrayLayoutControl(m_manager);
    m_layout->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_layout->SetHorizontal(false);
    m_layout->SetResizeMode(ArrayLayoutControl::ResizeBoth);
    m_layout->SetSpacing(kSectionSpacing);
    m_layout->SetLeadingMargin(0.0f);
    m_layout->SetTrailingMargin(0.0f);
    ControlObserver::Attach(m_layout.Get());
    SetEnabled(true);
    SetTransparent(true);
    SetHitTestable(true);
    SetInputFlags(m_inputFlags | 1);
    return true;
}

// Mac 0x23ce20, Win 0x664aa9
void MaterialGridControl::SetMaterialGrid(const MaterialGrid* grid, UInt32 offset)
{
    m_selected = 0;
    m_items.Clear();
    m_layout->RemoveAllControls();
    m_controls.Clear();
    AddSection(grid->m_solidRows, LocalizedString(String("SolidColorHeader")));
    AddSection(grid->m_transparentRows, LocalizedString(String("TransparentColorHeader")));
    AddSection(grid->m_specialRows, LocalizedString(String("MetallicColorHeader")));
    AddSection(grid->m_obsoleteRows, LocalizedString(String("LegacyColorHeader")));
    AddSpacer(offset);
    FitToChildren(true, true, true, true);
}

// Mac 0x23d0e0, Win 0x66457c
void MaterialGridControl::AddSection(const BasicArray<BasicArray<UInt32> >& rows, const String& header)
{
    if (rows.IsEmpty())
        return;
    RefPointer<Control> group = new Control(m_manager);
    group->Setup(m_layout.Get(), 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 1, true);

    RefPointer<RenderControl> label = new RenderControl(m_manager);
    label->Setup(group.Get(), 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    label->SetFont(LDDApplication::GetInstance()->GetFont(kHeaderFont));
    label->SetText(header);
    label->SetTransparent(true);
    label->ResizeHeightToFitText(0);
    label->SetSize(Vector2(label->GetSize().x, kHeaderHeight));

    RefPointer<Texture> texture = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    RefPointer<RenderControl> line = new RenderControl(m_manager);
    line->Setup(group.Get(), 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    // NOTE: marked initialized without RenderControl::Initialize (both builds).
    line->SetInputFlags(line->GetInputFlags() | RenderControl::kInputInitialized);
    Rect lineRect(Vector2(169.0f, 393.0f), Vector2(3.0f, 5.0f));
    line->SetImage(texture.Get(), &lineRect);

    RefPointer<MatrixLayoutControl> matrix = new MatrixLayoutControl(m_manager);
    matrix->Setup(group.Get(), 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 1, true);
    matrix->SetCellSize(m_itemSize);
    matrix->SetSpacing(kItemSpacing);

    UInt32 columns = 0;
    for (UInt32 i = 0; i < rows.GetSize(); ++i)
        if (columns < rows[i].GetSize())
            columns = rows[i].GetSize();
    matrix->SetSize(Vector2((Real32)columns * (kItemSize + kItemSpacing) - kItemSpacing, 0.0f));

    for (UInt32 i = 0; i < rows.GetSize(); ++i) {
        const BasicArray<UInt32>& row = rows[i];
        for (UInt32 j = 0; j < row.GetSize(); ++j)
            AddItem(row[j], matrix.Get());
        for (UInt32 j = row.GetSize(); j < columns; ++j)
            AddItem(0, matrix.Get());
    }

    // The line: vertically centred on the header, from 5 before the header's end to the
    // matrix's right edge; the matrix below the header.
    line->SetAxisAlignment(1, 2, 2, 0.0f, label.Get());
    line->SetAxisAlignments(0, 1, 3, -5.0f, label.Get(), 3, 3, 0.0f, matrix.Get());
    matrix->SetAlignment(0x11, 0x31, 0.0f, 0.0f, label.Get());
    group->SetSize(matrix->GetPosition() + matrix->GetSize());
    m_layout->AddControl(group.Get());

    m_controls.PushBack(group);
    m_controls.PushBack(RefPointer<Control>(label.Get()));
    m_controls.PushBack(RefPointer<Control>(line.Get()));
    m_controls.PushBack(RefPointer<Control>(matrix.Get()));
}

// Mac 0x23dc40, Win 0x664121
void MaterialGridControl::AddSpacer(UInt32 height)
{
    if (height == 0)
        return;
    RefPointer<Control> spacer = new Control(m_manager);
    spacer->Setup(m_layout.Get(), 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    spacer->SetSize(Vector2(0.0f, (Real32)height));
    spacer->SetAxisMargins(0, 0.0f, 0.0f, 0);
    m_controls.PushBack(spacer);
    m_layout->AddControl(spacer.Get());
}

// Mac 0x23dec0, Win 0x66442d
void MaterialGridControl::AddItem(UInt32 material, MatrixLayoutControl* matrix)
{
    RefPointer<Control> control;
    if (material == 0) {
        control = new Control(m_manager);
    } else {
        RefPointer<MaterialItemControl> item = new MaterialItemControl(m_manager, m_database);
        item->Initialize();
        item->SetMaterial(material);
        item->SetHoverable(true);
        LDDApplication::GetInstance()->AddButtonSound(item.Get());
        m_items[material] = item.Get();
        control = item.Get();
    }
    control->SetSize(m_itemSize);
    control->SetParent(matrix, 0);
    ControlObserver::Attach(control.Get());
    control->SetInputFlags(control->GetInputFlags() | 1);
    matrix->AddControl(control.Get());
}

// (inlined in SetSelectedMaterial / OnMouseClicked)
void MaterialGridControl::ClearSelection()
{
    if (m_selected) {
        m_selected->SetSelected(false);
        m_selected = 0;
    }
}

// Mac 0x23ddf0, Win 0x6643aa
void MaterialGridControl::SetSelectedMaterial(UInt32 material)
{
    ItemMap::Iterator it = m_items.Find(material);
    if (it == m_items.End()) {
        ClearSelection();
        return;
    }
    MaterialItemControl* item = it->second;
    ClearSelection();
    if (!item)
        return;
    m_selected = item;
    item->SetSelected(true);
}

MaterialItemControl* MaterialGridControl::GetItem(UInt32 material) const
{
    ItemMap::ConstIterator it = m_items.Find(material);
    return it == m_items.End() ? 0 : it->second;
}

// Mac 0x23e150, Win 0x664414
void MaterialGridControl::OnShown()
{
    ControlManagerObserver::Attach(m_manager);
}

// Mac 0x23e3d0
void MaterialGridControl::OnHidden()
{
    ControlManagerObserver::Detach();
}

// Mac 0x23e430, Win 0x6640b7
void MaterialGridControl::OnRectangleChanged(Control* control, const Vector2&, const Vector2&)
{
    if (control != m_layout.Get())
        return;
    control->SetPosition(Vector2(kBorder, kBorder));
    SetSize(m_layout->GetSize() + Vector2(2.0f * kBorder, 2.0f * kBorder));
}

// Mac 0x23e530, Win 0x664216
void MaterialGridControl::OnMouseClicked(Control* control, const Vector2&, UInt32 button)
{
    if (button != 0)
        return;
    if (MaterialItemControl* item = dynamic_cast<MaterialItemControl*>(control)) {
        ClearSelection();
        m_selected = item;
        item->SetSelected(true);
        Notifier<MaterialGridControlObserver>::Iterator it(*this);
        while (MaterialGridControlObserver* observer =
                   it.Next(Notifier<MaterialGridControlObserver>::m_observers))
            observer->OnMaterialSelected(item->GetMaterial());
    }
    SetShown(false);
}

// Mac 0x23e890 (thunk 0x23e8e0)
void MaterialGridControl::OnManagerMouseDown(const Vector2& position, UInt32)
{
    for (Control* control = m_manager->GetControlAt(position); control; control = control->GetParent())
        if (control == this)
            return;
    SetShown(false);
}

// Mac 0x23ef00
void MaterialGridControl::OnFinalRelease()
{
    Notifier<MaterialGridControlObserver>::OnFinalRelease();
    ControlManagerObserver::OnFinalRelease();
    // The ControlObserver part: leaves every observed control without OnDetached (inlined).
    for (UInt32 i = 0; i < ControlObserver::m_subjects.GetSize(); ++i)
        static_cast<NotifierT<Control, ControlObserver>*>(ControlObserver::m_subjects[i])
            ->RemoveObserver(this);
    ControlObserver::m_subjects.Clear();
    Object::OnFinalRelease();
    LDDPopUpControl::OnFinalRelease();
}

} // namespace LEGO
