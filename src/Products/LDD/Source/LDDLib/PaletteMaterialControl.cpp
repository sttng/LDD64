// LEGO Digital Designer — restored source
// TU 140 "PaletteMaterialControl.cpp": Mac 0x1f72a0-0x1f85eb, Win 0x66b4a9, 0x62bfe2-0x62c210.
// The constructor is inline (EditMode.cpp, Mac 0x18bf60 / Win 0x62ac70) and restored here.
// Not code: the Notifier<PaletteMaterialControlObserver> / NotifierT instances (Mac
// 0x1f77b0-0x1f7c00; Notifier.h), construction-vtable thunks (0x1f7e30-0x1f84d0), Mac 0x1f8570 =
// <iostream> + a header static.
#include "PaletteMaterialControl.h"
#include "BricksControl.h"
#include "LDDApplication.h"
#include "BrickPalette.h"

namespace LEGO {

// Mac 0x18bf60 (inline, EditMode.cpp), Win 0x62ac70
PaletteMaterialControl::PaletteMaterialControl(ControlManager* manager,
                                               BrickConnectSceneManager* sceneManager,
                                               UInt32 rows, UInt32 columns)
    : MaterialControl(manager, sceneManager, rows, columns)
{
}

// Mac 0x1f7ca0 (D1), 0x1f7d60 (D0), Win sdd 0x62c210
PaletteMaterialControl::~PaletteMaterialControl()
{
}

// Mac 0x1f72a0, Win 0x66b4a9
bool PaletteMaterialControl::ShowVariants(const BrickPalette::Brick* brick, const Vector2* camera)
{
    BrickPalette* palette = LDDApplication::GetInstance()->GetBrickPalette();
    BrickArray variants;
    for (BrickPalette::BrickSet::ConstIterator it = palette->GetBricksBegin();
         it != palette->GetBricksEnd(); ++it) {
        if ((*it)->GetDesignID() == brick->GetDesignID())
            variants.PushBack(*it);
        else if (!variants.IsEmpty())
            break;
    }
    return SetBricks(variants, camera, false);
}

// Mac 0x1f7520
void PaletteMaterialControl::OnBricksControlClicked(BricksControl* item)
{
    Notifier<PaletteMaterialControlObserver>::Iterator it(*this);
    while (PaletteMaterialControlObserver* observer =
               it.Next(Notifier<PaletteMaterialControlObserver>::m_observers))
        observer->OnBricksControlClicked(item);
}

// Mac 0x1f7da0
void PaletteMaterialControl::OnFinalRelease()
{
    Notifier<PaletteMaterialControlObserver>::OnFinalRelease();
    MaterialControl::OnFinalRelease();
}

} // namespace LEGO
