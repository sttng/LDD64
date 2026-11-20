// LEGO Digital Designer — restored source
// TU 303 "InfoBoxControl.cpp": Mac 0x2aff40-0x2b149b, Win 0x6769c6-0x677034.
// Not code: Mac 0x2b1420 = <iostream> + a header static.
#include "InfoBoxControl.h"
#include "BrickConnectSceneControl.h"
#include "BrickDatabase.h"
#include "Brick.h"
#include "Part.h"
#include "Scene.h"
#include "TextLocale.h"
#include "BasicMap.h"

namespace LEGO {

// Mac 0x2aff40, Win 0x6769c6
InfoBoxControl::InfoBoxControl(ControlManager* manager, BrickDatabase* database)
    : RenderControl(manager), m_database(database), m_flag(false)
{
}

// Mac 0x2b03c0 (D1) / 0x2b05e0 (D0), Win sdd 0x676ab3
InfoBoxControl::~InfoBoxControl()
{
}

// Mac 0x2b0850 (thunks 0x2b08c0 … 0x2b0a10)
void InfoBoxControl::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

// Mac 0x2b0040, Win 0x676ad5
bool InfoBoxControl::Create(BrickConnectSceneControl* sceneControl)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
    m_transparent = true;
    return true;
}

// Mac 0x2b0a90 (thunk 0x2b13a0), Win 0x677011
void InfoBoxControl::OnEvent15(const BasicArray<RefPointer<Part> >& selection)
{
    UpdateInfo(selection);
}

// Mac 0x2b13c0 (thunk 0x2b13f0), Win 0x67701c
void InfoBoxControl::OnEvent19()
{
    UpdateInfo(BrickConnectSceneControlObserver::GetSubject()->GetSelection());
}

// Mac 0x2b0aa0, Win 0x676b18
void InfoBoxControl::UpdateInfo(const BasicArray<RefPointer<Part> >& selection)
{
    BasicArray<RefPointer<Brick> > bricks;
    Scene::GetBricks(selection, bricks);

    if (bricks.GetSize() == 0) {
        String count;
        count << BrickConnectSceneControlObserver::GetSubject()->GetScene()->GetBricks().GetSize();
        LocalizedString text(String("TOTAL BRICKS INFO"));
        text.ReplaceAll(String("%s"), count);
        SetText(text);
    } else if (bricks.GetSize() == 1) {
        // NOTE: the brick of the first selected part, not bricks[0] (both builds).
        Brick* brick = selection[0]->GetBrick();
        String id;
        id << brick->GetDesignID();
        LocalizedString idText(String("SELECTED BRICK ID INFO"));
        idText.ReplaceAll(String("%s"), id);

        String name = BrickDatabase::GetLocalizedName(*m_database->GetBrick(brick->GetDesignID()));
        LocalizedString nameText(String("SELECTED BRICK NAME INFO"));
        nameText.ReplaceAll(String("%s"), name);

        // The materials of all the brick's parts, ascending, as "<id> - <name>, …".
        BasicSet<UInt32> materialIDs;
        const BasicArray<RefPointer<Part> >& parts = brick->GetParts();
        for (UInt32 p = 0; p < parts.GetSize(); ++p) {
            Part* part = parts[p];
            for (UInt32 i = 0; i < part->GetMaterials().GetSize(); ++i)
                materialIDs.Insert(part->GetMaterial(i));
        }
        String materials;
        for (BasicSet<UInt32>::ConstIterator it = materialIDs.Begin(); it != materialIDs.End(); ++it) {
            String key("Material");
            LocalizedString materialName(key << *it);
            if (!materials.IsEmpty())
                materials << ", ";
            materials << *it << " - " << materialName;
        }
        LocalizedString colorText(String("SELECTED BRICK COLOR INFO"));
        colorText.ReplaceAll(String("%s"), materials);

        String text;
        text << idText << " " << nameText << " " << colorText;
        SetText(text);
    } else {
        String count;
        count << bricks.GetSize();
        LocalizedString text(String("SELECTED BRICKS INFO"));
        text.ReplaceAll(String("%s"), count);
        SetText(text);
    }
    ResizeHeightToFitText(0);
}

} // namespace LEGO
