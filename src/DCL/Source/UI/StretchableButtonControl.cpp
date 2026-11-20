// LEGO Digital Designer — restored source
// TU 120 "StretchableButtonControl.cpp".
#include "StretchableButtonControl.h"
#include "TextLayout.h"
#include "Font.h"

namespace LEGO {

// Mac 0x1d7c30 / 0x1d7c90, Win sdd 0x638cab
StretchableButtonControl::~StretchableButtonControl()
{
}

// Mac 0x1d7d00
void StretchableButtonControl::OnFinalRelease()
{
    ButtonControl::OnFinalRelease();
}

// Mac 0x1d67e0, Win 0x51a2a3
bool StretchableButtonControl::Create(const Rect& topLeft, const Rect& top, const Rect& topRight,
                                      const Rect& left, const Rect& center, const Rect& right,
                                      const Rect& bottomLeft, const Rect& bottom,
                                      const Rect& bottomRight, UInt32 textMarginY,
                                      UInt32 textMarginX)
{
    bool ok = ButtonControl::Initialize();
    if (ok) {
        SetInputFlags(m_inputFlags & ~(UInt32)kInputInitialized);
        m_topLeft.Set(topLeft);
        m_top.Set(top);
        m_topRight.Set(topRight);
        m_left.Set(left);
        m_center.Set(center);
        m_right.Set(right);
        m_bottomLeft.Set(bottomLeft);
        m_bottom.Set(bottom);
        m_bottomRight.Set(bottomRight);
        m_textMarginY = textMarginY;
        m_textMarginX = textMarginX;
    }
    return ok;
}

// Mac 0x1d68f0, Win 0x51a360
void StretchableButtonControl::Draw(ControlDrawArgs& args)
{
    args.Setup(false);
    Color color = m_color;
    if (!m_buffered) {
        if (color.a == 0.0f)
            return;
    } else {
        color = Color(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (!m_image) {
        if (!m_transparent)
            args.DrawRect(Rect(m_size), color, color.a != 1.0f);
    } else {
        Vector2 imageSize = m_image->GetImageSize();
        const Rect& image = m_imageRect;
        const Real32 l = image.m_min.x, t = image.m_min.y, r = image.m_max.x, b = image.m_max.y;

        // Source parts in image pixels.
        Rect srcTopLeft(image.m_min, m_topLeft.GetSize());
        Rect srcTopRight(Vector2(r - m_topRight.GetWidth(), t), m_topRight.GetSize());
        Rect srcBottomLeft(Vector2(l, b - m_bottomLeft.GetHeight()), m_bottomLeft.GetSize());
        Rect srcBottomRight(Vector2(r - m_bottomRight.GetWidth(), b - m_bottomRight.GetHeight()),
                            m_bottomRight.GetSize());
        Rect srcLeft(Vector2(l, t + m_topLeft.GetHeight()),
                     Vector2(m_topLeft.GetWidth(),
                             (image.GetHeight() - m_topLeft.GetHeight()) - m_bottomLeft.GetHeight()));
        Rect srcRight(Vector2(r - m_topRight.GetWidth(), t + m_topRight.GetHeight()),
                      Vector2(m_topRight.GetWidth(),
                              (image.GetHeight() - m_topRight.GetHeight()) - m_bottomRight.GetHeight()));
        // NOTE: original quirk: the top-right width twice, the top-left height (top and bottom).
        Rect srcTop(Vector2(l + m_topLeft.GetWidth(), t),
                    Vector2((image.GetWidth() - m_topRight.GetWidth()) - m_topRight.GetWidth(),
                            m_topLeft.GetHeight()));
        Rect srcBottom(Vector2(l + m_bottomLeft.GetWidth(), b - m_bottomLeft.GetHeight()),
                       Vector2((image.GetWidth() - m_topRight.GetWidth()) - m_topRight.GetWidth(),
                               m_topLeft.GetHeight()));
        // NOTE: original quirk: the height subtracts the top-right height, not the bottom-left one.
        Rect srcCenter(Vector2(l + m_topLeft.GetWidth(), t + m_topLeft.GetHeight()),
                       Vector2((image.GetWidth() - m_topLeft.GetWidth()) - m_topRight.GetWidth(),
                               (image.GetHeight() - m_topLeft.GetHeight()) - m_topRight.GetHeight()));

        const Real32 w = m_size.x, h = m_size.y;
        // Destination parts in control units, drawn in this order.
        const Rect* sources[9] = {
            &srcTopLeft, &srcTopRight, &srcBottomLeft, &srcBottomRight, &srcLeft, &srcRight,
            &srcTop, &srcBottom, &srcCenter
        };
        const Rect targets[9] = {
            Rect(0.0f, 0.0f, m_topLeft.GetWidth(), m_topLeft.GetHeight()),
            Rect(w - m_topRight.GetWidth(), 0.0f, w, m_topRight.GetHeight()),
            Rect(0.0f, h - m_bottomLeft.GetHeight(), m_bottomLeft.GetWidth(), h),
            Rect(w - m_bottomRight.GetWidth(), h - m_bottomRight.GetHeight(), w, h),
            // NOTE: original quirk: the left edge uses the top-right corner's size.
            Rect(0.0f, m_topRight.GetHeight(), m_topRight.GetWidth(), h - m_bottomLeft.GetHeight()),
            Rect(w - m_topRight.GetWidth(), m_topRight.GetHeight(), w, h - m_bottomRight.GetHeight()),
            Rect(m_topRight.GetWidth(), 0.0f, w - m_topRight.GetWidth(), m_topRight.GetHeight()),
            Rect(m_bottomLeft.GetWidth(), h - m_bottomLeft.GetHeight(), w - m_bottomRight.GetWidth(), h),
            Rect(m_topLeft.GetWidth(), m_topLeft.GetHeight(), w - m_topRight.GetWidth(),
                 h - m_bottomRight.GetHeight())
        };
        for (UInt32 i = 0; i < 9; ++i) {
            Rect source(*sources[i]);
            source.Divide(imageSize);
            bool blend = true;
            if (color.a == 1.0f)
                blend = m_image->GetFormat() != 0;
            args.DrawTexture(targets[i], m_image.Get(), source, color, blend);
        }
    }

    if (m_textLayout) {
        Vector2 position(m_textOffset);
        m_textLayout->AlignInArea(m_size, position);
        Rect textRect(position, m_textLayout->GetSize());
        Rect visible((args.m_clip.m_min.x - args.m_offset.x) / args.m_scale.x,
                     (args.m_clip.m_min.y - args.m_offset.y) / args.m_scale.y,
                     (args.m_clip.m_max.x - args.m_offset.x) / args.m_scale.x,
                     (args.m_clip.m_max.y - args.m_offset.y) / args.m_scale.y);
        textRect.Intersect(visible);
        if (textRect.IsValid()) {
            Texture* texture = m_textLayout->GetTexture();
            if (texture) {
                RefPointer<Font> font;
                m_textLayout->GetFont(font);
                const Color& fontColor = font->GetColor();
                color.r = fontColor.r * color.r;
                color.g = fontColor.g * color.g;
                color.b = fontColor.b * color.b;
                color.a = fontColor.a * color.a;
                Rect target(position, m_textLayout->GetSize());
                Rect texCoords = m_textLayout->GetTexCoords();
                args.DrawTexture(target, texture, texCoords, color, true);
            }
        }
    }
    m_status.m_invalid = 0;
}

// Mac 0x1d7b40, Win 0x51b0e7 (replaces RenderControl's)
bool StretchableButtonControl::ResizeHeightToFitText(UInt32 width)
{
    Vector2 size;
    if (m_textLayout) {
        m_textLayout->SetMaxWidth(width);
        m_textLayout->SetTruncate(true);
        size = m_textLayout->GetSize();
        Real32 textWidth = (Real32)width;
        if (size.x <= textWidth)
            textWidth = size.x;
        Real32 marginX = (Real32)m_textMarginX;
        size.x = marginX + marginX + textWidth;
        size.y = (Real32)(m_textMarginY * 2) + size.y;
    }
    return SetSize(size);
}

} // namespace LEGO
