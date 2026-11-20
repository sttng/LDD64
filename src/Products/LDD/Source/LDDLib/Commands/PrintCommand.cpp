// LEGO Digital Designer — restored source
// TU 251 "PrintCommand.cpp": Mac 0x274a20-0x2751db, Win 0x609299 / 0x5ca251. The constructor is
// inline (PrintCommand.h). The TU also emits the BasicArray<CommandObserver*> base-destructor
// helper Mac 0x275060 (Notifier.h); Mac 0x275160 = <iostream> + a header static (not code).
#include "PrintCommand.h"
#include "PrintJob.h"
#include "Bitmap.h"
#include "ControlManager.h"

namespace LEGO {

// Mac 0x274cd0 (D1) / 0x274d30 (D0), Win sdd 0x5ca251
PrintCommand::~PrintCommand()
{
}

// Mac 0x274da0 (thunks 0x274f20, 0x275040)
void PrintCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x274a20, Win 0x609299
void PrintCommand::Do()
{
    BrickConnectSceneControl* control = GetSubject();
    if (!control)
        return;
    // The picture side: the smaller side of the window's root control, at most 2048.
    const Vector2& rootSize = m_sceneControl->GetManager()->GetLayer(0)->GetSize();
    Real32 side = rootSize.x;
    if (rootSize.y <= side)
        side = rootSize.y;
    if (2048.0f <= side)
        side = 2048.0f;
    UInt32 pictureSide = (UInt32)(Int64)side;

    RefPointer<PrintJob> job = new PrintJob;
    bool accepted = false;
    bool ok = job->ShowPrintDialog(m_window, accepted);
    if (!accepted || !ok)
        return;
    job->BeginDocument(String("LDD Print Screenshot"));
    IntVector2 page;
    job->GetPageSize(page);
    UInt32 pageSide = (UInt32)page.x < (UInt32)page.y ? (UInt32)page.x : (UInt32)page.y;

    Bitmap bitmap;
    bitmap.m_width = pictureSide & ~0xfu;
    bitmap.m_height = bitmap.m_width;
    bitmap.m_depth = 1;
    bitmap.m_format = kBitmapFormatBGR24;
    bitmap.m_dataSize = 0;
    bitmap.m_pitch = bitmap.m_width * 3;             // Win 0x4f5180 (width, 24 bits per pixel)
    bitmap.m_palette = 0;
    bitmap.m_data = new UInt8[bitmap.m_width * bitmap.m_pitch];
    control->RenderToBitmap(bitmap, 0, true, false);
    job->BeginPage();
    IntRect destination(0, 0, pageSide, pageSide);
    IntRect source(0, 0, bitmap.m_width, bitmap.m_height);
    if (job->DrawBitmap(destination, bitmap, source)) {
        job->EndPage();
        job->EndDocument();
    } else {
        job->AbortDocument();
    }
    delete[] static_cast<UInt8*>(bitmap.m_data);
}

// Mac 0x274c90, Win 0x63120b
void PrintCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = GetSubject();
    SetEnable(control && !control->IsSceneEmpty());
}

// Mac 0x274e30, Win 0x66e988
void PrintCommand::OnSceneControlAttached(BrickConnectSceneControl* /*control*/)
{
    UpdateEnable();
}

// Mac 0x274e40 (thunk 0x274f40), Win 0x631956
void PrintCommand::OnEvent19()
{
    UpdateEnable();
}

} // namespace LEGO
