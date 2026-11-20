// LEGO Digital Designer — restored source
// TU 105 "ProgressBarControl.cpp" (Mac 0x1b9480-0x1b9b6b).
#include "ProgressBarControl.h"
#include "RenderControlManager.h"
#include "Texture.h"
#include "Color.h"
#include "LEGO/String.h"
#include "LDDUtilities.h"
#include "MathUtil.h"
#include <math.h>

namespace LEGO {

namespace {

// Win 0x4c9630 LEGO::RoundToNearest (inlined on the Mac).
inline Real32 RoundToNearest(Real32 value)
{
    Real32 lower = (Real32)floor((double)value);
    Real32 upper = (Real32)ceil((double)value);
    return value - lower < upper - value ? lower : upper;
}

} // namespace

// Mac 0x1b98d0 (D1, thunks 0x1b99d0 / 0x1b9a30 / 0x1b9a90), 0x1b9990 (D0); Win 0x642779,
// sdd 0x6427fe
ProgressBarControl::~ProgressBarControl()
{
}

// Mac 0x1b9480, Win 0x6876c1
bool ProgressBarControl::Create(Control* parent, Control* after)
{
    if (!RenderControl::Initialize())
        return false;
    Setup(parent, after, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    RefPointer<Texture> image = LoadApplicationTexture(
        String("Graphics/LDDgraphics01.png"), static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image.Get())
        return false;
    m_bar = new RenderControl(m_manager);
    if (!m_bar->Initialize())
        return false;
    m_bar->Setup(this, 0, 7, Rect(Vector2(0.0f, 1.0f), Vector2()), 0, false);
    m_bar->SetColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
    m_bar->SetImage(image, 0);
    SetColor(Color::White());
    Rect frame(Vector2(288.0f, 512.0f), Vector2(272.0f, 27.0f));
    SetImage(image, &frame);
    return true;
}

// Mac 0x1b98a0
void ProgressBarControl::SetValue(Real32 value)
{
    Real32 clamped = m_maximum;
    if (value <= m_maximum)
        clamped = value;
    m_value = clamped;
    UpdateBar();
}

// Mac 0x1b97b0
void ProgressBarControl::UpdateBar()
{
    Real32 width = 0.0f;
    if (m_maximum != 0.0f)
        width = (Real32)(EvalReal(m_value) / EvalReal(m_maximum) * 272.0f);
    Vector2 position(288.0f, 475.0f);
    Vector2 size(RoundToNearest(width), 25.0f);
    Rect rect(position, size);
    m_bar->SetImageRect(&rect);
}

} // namespace LEGO
