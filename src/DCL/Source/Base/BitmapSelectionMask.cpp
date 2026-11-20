// LEGO Digital Designer — restored source
// BitmapSelectionMask (Mac TU 275, 0x28e5f0-0x28ec1f; Win 0x46c5e0, 0x518788-0x5188ab).
// Names inferred.
#include "BitmapSelectionMask.h"

namespace LEGO {

// Win 0x46c5e0 (inlined on Mac)
BitmapSelectionMask::BitmapSelectionMask()
    : m_rect(0.0f, 0.0f, 0.0f, 0.0f), m_value(0), m_inside(true)
{
}

// Mac 0x28e5f0, Win 0x518788
Vector2 BitmapSelectionMask::GetSize() const
{
    return m_rect.GetSize();
}

// Inlined in Contains / Pick (Mac 0x28e630-0x28e6d2, Win 0x5187a8-0x5187f1).
bool BitmapSelectionMask::ToMask(const Vector2& point, Vector2& maskPoint) const
{
    if (!m_rect.IsFlipped()) {
        maskPoint.Set(m_rect.m_min.x + point.x, m_rect.m_min.y + point.y);
        return m_rect.Contains(maskPoint);
    }
    maskPoint.Set(m_rect.m_min.x - point.x, m_rect.m_min.y + point.y);
    Rect flipped = m_rect.GetFlipped();
    return flipped.Contains(maskPoint);
}

// Mac 0x28e620, Win 0x51879f
bool BitmapSelectionMask::Contains(const Vector2& point) const
{
    if (!m_mask.IsNull()) {
        Vector2 maskPoint;
        if (ToMask(point, maskPoint))
            return ((UInt32)m_mask->GetValue(maskPoint) == m_value) == m_inside;
    }
    return !m_inside;
}

// Mac 0x28e710, Win 0x51882c
bool BitmapSelectionMask::Pick(const Vector2& point, bool inside)
{
    if (!m_mask.IsNull()) {
        Vector2 maskPoint;
        if (ToMask(point, maskPoint)) {
            m_value = m_mask->GetValue(maskPoint);
            m_inside = inside;
            return true;
        }
    }
    return false;
}

// Mac 0x28e800, Win 0x5188ab
bool BitmapSelectionMask::SetMask(BitmapMask* mask, const Rect* rect)
{
    if (rect == 0) {
        m_rect.Set(Rect(mask->GetSize()));
    } else {
        Rect normalized(0.0f, 0.0f, 0.0f, 0.0f);
        if (rect->IsFlipped())
            normalized.Set(rect->GetFlipped());
        else
            normalized = *rect;
        if (!Rect(mask->GetSize()).Contains(normalized))
            return false;
        m_rect.Set(*rect);
    }
    m_mask = mask;
    return true;
}

} // namespace LEGO
