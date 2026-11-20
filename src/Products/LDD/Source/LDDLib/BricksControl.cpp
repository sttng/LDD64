// LEGO Digital Designer — restored source
// BricksControl.cpp — TU 075 (Mac 0x169800-0x16b6ab; Win 0x685cd0-0x686144, destructors
// 0x63957f / 0x63959b / 0x6790d1), plus the two palette item factories of LDDUtilities (TU 063,
// Mac 0x14a980 / 0x14abe0) that make BricksControls. See BricksControl.h.
#include "BricksControl.h"
#include "RenderControlManager.h"
#include "Texture.h"
#include "BrickGroup.h"
#include "Part.h"
#include "BrickDatabase.h"
#include "LDDUtilities.h"
#include "StandardTextTooltipAugmentation.h"
#include "BrickCountAugmentation.h"

namespace LEGO {

// ===========================================================================================
// BricksControl
// ===========================================================================================

// Mac 0x16ab40/0x16aba0 (body 0x16b4d0), Win 0x6392ee / sdd 0x63959b
BricksControl::~BricksControl()
{
}

// Mac 0x16ac10 (Win ICF with RenderControl's)
void BricksControl::OnFinalRelease()
{
    RenderControl::OnFinalRelease();
}

// Mac 0x169800, Win 0x686301
bool BricksControl::Create(const BrickPalette::Brick* brick)
{
    bool created = AddBrick(brick);
    if (created)
        EnableModelInput();
    return created;
}

// Mac 0x169840, Win 0x686174
bool BricksControl::AddBrick(const BrickPalette::Brick* brick)
{
    const BrickDatabase::Brick* databaseBrick = GetSceneManager()->GetDatabase()->GetBrick(brick->m_designID);
    if (databaseBrick == 0)
        return false;
    m_camera = databaseBrick->m_defaultCamera;
    m_bricks.PushBack(ObjectRefPointer<PrimitiveElementBrick>(
        new PrimitiveElementBrick(brick, databaseBrick->m_defaultOrientation)));
    Rebuild();
    return true;
}

// Mac 0x169950
bool BricksControl::Create(const BrickArray& bricks, const Vector2& camera)
{
    m_camera = camera;
    if (&m_bricks != &bricks)
        m_bricks = bricks;
    Rebuild();
    EnableModelInput();
    return true;
}

// Mac 0x1699c0
bool BricksControl::Create(const PrimitiveElementBrick& brick, const Vector2& camera)
{
    AddBrick(brick, camera);
    EnableModelInput();
    return true;
}

// Mac 0x169a00
bool BricksControl::AddBrick(const PrimitiveElementBrick& brick, const Vector2& camera)
{
    m_camera = camera;
    m_bricks.PushBack(ObjectRefPointer<PrimitiveElementBrick>(new PrimitiveElementBrick(brick)));
    Rebuild();
    return true;
}

// Mac 0x169ae0
bool BricksControl::Create(const PrimitiveElementBrick& brick)
{
    const BrickDatabase::Brick* databaseBrick =
        GetSceneManager()->GetDatabase()->GetBrick(brick.GetBrick()->GetDesignID());
    if (databaseBrick != 0) {
        AddBrick(brick, databaseBrick->m_defaultCamera);
        EnableModelInput();
    }
    return databaseBrick != 0;
}

// Mac 0x169b50, Win 0x685cd0
PrimitiveElementBrick* BricksControl::GetBrick() const
{
    return m_bricks.Begin()->Get();
}

// Mac 0x169b60, Win 0x685cd9
const BrickPalette::Brick* BricksControl::GetPaletteBrick() const
{
    return m_bricks.Begin()->Get()->GetBrick();
}

// Mac 0x169b80, Win 0x685d92
void BricksControl::SetMaterial(Material* material, UInt32 materialID)
{
    for (BrickArray::ConstIterator it = m_bricks.Begin(); it != m_bricks.End(); ++it) {
        it->Get()->SetMaterial(materialID);
        m_draw.SetMaterial(material);
    }
}

// Mac 0x169be0
bool BricksControl::Rebuild()
{
    UpdateFlexBadge();
    CancelLoading();
    ClearDraw();
    Redraw();
    return true;
}

// Mac 0x169cc0
void BricksControl::UpdateFlexBadge()
{
    if (m_bricks.GetSize() == 1 && m_bricks.Begin()->Get()->IsFlexible()) {
        if (!m_flexBadge.IsNull())
            return;
        RefPointer<Texture> image(LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
            static_cast<RenderControlManager*>(GetManager())->GetRender()));
        m_flexBadge = new RenderControl(GetManager());
        m_flexBadge->Initialize();
        m_flexBadge->Setup(this, 0, 6, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
        m_flexBadge->SetAlignment(0x13, 0x13, 0.0f, -20.0f, 0);
        static const Rect s_badgeRect(Vector2(183.0f, 294.0f), Vector2(14.0f, 14.0f));
        m_flexBadge->SetImage(image, &s_badgeRect);
        m_flexBadge->SetShown(true);
    } else if (!m_flexBadge.IsNull()) {
        m_flexBadge = 0;
    }
}

// Mac 0x169fa0, Win 0x685f79
void BricksControl::StartLoading()
{
    if (m_bricks.IsEmpty())
        return;
    LazyLoadData data;
    data.callback = this;
    data.materialSet = m_materialSet;
    for (BrickArray::ConstIterator it = m_bricks.Begin(); it != m_bricks.End(); ++it) {
        const PrimitiveElementBrick* brick = it->Get();
        data.AddBrick(brick->GetBrick()->m_designID);
        for (UInt32 part = 0; part < brick->GetBrick()->m_parts.GetSize(); ++part) {
            data.AddMaterials(brick->GetMaterials(part));
            data.AddDecorations(brick->GetDecorations(part));
        }
    }
    LazyLoad(data);
}

// Mac 0x16a160 (thunk 0x16a2d0), Win 0x6860c7
bool BricksControl::OnLoaded(const LazyLoadData& data)
{
    if (!ModelControl::OnLoaded(data))
        return false;
    m_ticket = 0;
    ClearDraw();
    for (BrickArray::ConstIterator it = m_bricks.Begin(); it != m_bricks.End(); ++it) {
        const PrimitiveElementBrick* brick = it->Get();
        for (UInt32 part = 0; part < brick->GetBrick()->m_parts.GetSize(); ++part) {
            if (!m_draw.AddPart(*brick, part, m_materialSet))
                return false;
        }
    }
    return true;
}

// ===========================================================================================
// BrickGroupControl
// ===========================================================================================

// Mac 0x16b040/0x16b0a0, Win 0x678f05 / sdd 0x6790d1
BrickGroupControl::~BrickGroupControl()
{
}

// Mac 0x16a2f0
bool BrickGroupControl::Create(BrickGroup* group, const Vector2& camera)
{
    m_group = group;
    m_camera = camera;
    CancelLoading();
    ClearDraw();
    Redraw();
    EnableModelInput();
    return true;
}

// Mac 0x16a420, Win 0x686048
void BrickGroupControl::StartLoading()
{
    if (m_group == 0)
        return;
    LazyLoadData data;
    data.callback = this;
    data.materialSet = 1;
    CollectLoadData(m_group, data);
    LazyLoad(data);
}

// Mac 0x16a550, Win 0x685ce5
void BrickGroupControl::CollectLoadData(const BrickGroup* group, LazyLoadData& data)
{
    const BasicArray<RefPointer<Part> >& parts = group->GetParts();
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it) {
        const Part* part = *it;
        data.AddBrick(part->GetBrick()->GetDesignID());
        data.AddMaterials(part->GetMaterials());
        data.AddDecorations(part->GetDecorations());
    }
    const BasicArray<RefPointer<BrickGroup> >& groups = group->GetGroups();
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = groups.Begin(); it != groups.End(); ++it)
        CollectLoadData(*it, data);
}

// Mac 0x16a5f0 (thunk 0x16a760), Win 0x686144
bool BrickGroupControl::OnLoaded(const LazyLoadData& data)
{
    if (!ModelControl::OnLoaded(data))
        return false;
    m_ticket = 0;
    ClearDraw();
    AddGroup(m_group);
    return true;
}

// Mac 0x16a6f0, Win 0x685d4a
void BrickGroupControl::AddGroup(const BrickGroup* group)
{
    const BasicArray<RefPointer<Part> >& parts = group->GetParts();
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it)
        m_draw.AddPart(it->Get(), 1);
    const BasicArray<RefPointer<BrickGroup> >& groups = group->GetGroups();
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = groups.Begin(); it != groups.End(); ++it)
        AddGroup(*it);
}

// ===========================================================================================
// LDDUtilities palette item factories (TU 063)
// ===========================================================================================

// Mac 0x14abe0, Win 0x639445
BricksControl* CreateBricksControl(const BrickPalette::Brick* brick, ControlManager* manager,
                                   BrickConnectSceneManager* sceneManager, ControlObserver* observer,
                                   Control* parent, bool fullDescription)
{
    BricksControl* control = new BricksControl(manager, sceneManager, 1);
    if (control->Create(brick) && control->SetParent(parent, 0)) {
        control->SetCursor(100, true);
        observer->Attach(control);
        String text = fullDescription
            ? LDDUtilities::GetBrickDescription(sceneManager->GetDatabase(), brick)
            : sceneManager->GetDatabase()->GetLocalizedName(brick->m_designID);
        new StandardTextTooltipAugmentation(control, text);
        new BrickCountAugmentation(control);
        return control;
    }
    static_cast<Object*>(control)->Release();
    return 0;
}

// Mac 0x14a980
BricksControl* CreateBricksControl(const PrimitiveElementBrick& brick, const Vector2& camera,
                                   ControlManager* manager, BrickConnectSceneManager* sceneManager,
                                   ControlObserver* observer, Control* parent)
{
    BricksControl* control = new BricksControl(manager, sceneManager, 1);
    if (control->Create(brick, camera) && control->SetParent(parent, 0)) {
        control->SetCursor(100, true);
        observer->Attach(control);
        String text = sceneManager->GetDatabase()->GetLocalizedName(brick.GetBrick()->m_designID);
        new StandardTextTooltipAugmentation(control, text);
        new BrickCountAugmentation(control);
        return control;
    }
    static_cast<Object*>(control)->Release();
    return 0;
}

} // namespace LEGO
