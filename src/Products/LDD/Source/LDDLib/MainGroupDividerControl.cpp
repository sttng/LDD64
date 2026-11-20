// LEGO Digital Designer — restored source
// TU 134 "MainGroupDividerControl.cpp": Mac 0x1ee090-0x1ef44b, Win 0x691149-0x69181e.
// Not code: Mac 0x1ef3d0 = <iostream> + a header static.
#include "MainGroupDividerControl.h"
#include "BrickDatabase.h"
#include "RenderControlManager.h"
#include "LDDUtilities.h"
#include "Color.h"

namespace LEGO {

// Mac 0x1eed70 (D1) / 0x1eedd0 (D0), Win sdd 0x6625b6
MainGroupDividerControl::~MainGroupDividerControl()
{
}

// Mac 0x1ee090, Win 0x69119a
bool MainGroupDividerControl::Create(Control* parent, const String& mainGroup,
                                     BrickDatabase* database)
{
    if (!ToggleControl::Initialize())
        return false;
    Setup(parent, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 1, false);
    SetInputFlags(GetInputFlags() & ~kInputInitialized);
    SetClearColor(Color(1.0f, 1.0f, 1.0f, 0.0f));

    Render* render = static_cast<RenderControlManager*>(m_manager)->GetRender();
    m_image = LoadTexture(database->GetDirectory(),
                          String("MainGroupDividers/") << mainGroup << ".png", render);
    if (!m_image) {
        m_image = LoadApplicationTexture(String("Graphics/EditScreen/DefaultDivider.png"), render);
        // NOTE: the message is built and dropped (a trace call compiled out in the release builds).
        String message("Maingroup divider graphics missing for maingroup: ");
        message << mainGroup;
        if (!m_image)
            return false;
    }

    RefPointer<Texture> markers =
        LoadApplicationTexture(String("Graphics/EditScreen/EditMode_Final.png"), render);
    if (!markers)
        return false;

    m_expandedMarker = new RenderControl(m_manager);
    if (!m_expandedMarker->Initialize())
        return false;
    m_expandedMarker->Setup(this, 0, 6, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    Rect markerRect(Vector2(405.0f, 205.0f), Vector2(14.0f, 14.0f));
    m_expandedMarker->SetImage(markers, &markerRect);
    m_expandedMarker->SetAlignment(0x33, 0x33, 10.0f, 10.0f, 0);

    m_collapsedMarker = new RenderControl(m_manager);
    if (!m_collapsedMarker->Initialize())
        return false;
    m_collapsedMarker->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    markerRect.Set(Rect(Vector2(405.0f, 223.0f), Vector2(14.0f, 14.0f)));
    m_collapsedMarker->SetImage(markers, &markerRect);
    m_collapsedMarker->SetAlignment(0x33, 0x33, 10.0f, 10.0f, 0);

    // The four quadrants of the 128 x 128 divider image: up, over, down, disabled; the
    // toggled states use the same images.
    StateDescription description;
    description.m_image = m_image;
    description.m_transparent = false;
    static const Real32 kOrigins[4][2] = { { 0.0f, 0.0f }, { 64.0f, 0.0f }, { 0.0f, 64.0f }, { 64.0f, 64.0f } };
    for (UInt32 i = 0; i < 8; ++i) {
        description.m_imageRect.Set(Rect(Vector2(kOrigins[i & 3][0], kOrigins[i & 3][1]),
                                         Vector2(64.0f, 64.0f)));
        SetStateDescription(i, description);
    }
    return true;
}

// Mac 0x1eec50, Win 0x691149
bool MainGroupDividerControl::Create(Control* parent, UInt32 mainGroup, BrickDatabase* database)
{
    String name;
    name << mainGroup;
    return Create(parent, name, database);
}

// Mac 0x1eecd0, Win 0x6917d7
void MainGroupDividerControl::NotifyToggled()
{
    ToggleControl::NotifyToggled();
    m_collapsedMarker->SetShown(false);
    m_expandedMarker->SetShown(true);
}

// Mac 0x1eed20, Win 0x6917fb
void MainGroupDividerControl::NotifyUntoggled()
{
    ToggleControl::NotifyUntoggled();
    m_collapsedMarker->SetShown(true);
    m_expandedMarker->SetShown(false);
}

} // namespace LEGO
