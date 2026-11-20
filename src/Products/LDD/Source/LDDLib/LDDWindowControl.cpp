// LEGO Digital Designer — restored source
// TU 071 "LDDWindowControl.cpp": Mac 0x162010-0x16412b, Win 0x606ebe-0x608090.
// Not code here: the ControlAugmentationControlObserver weak copies (Mac 0x163740-0x163990,
// header-only), BasicArray<Control*> / construction-vtable thunks (0x163a00-0x1640b0),
// 0x1640b0 = <iostream> + a header static.
#include "LDDWindowControl.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "RenderControlManager.h"
#include "ObjectFactory.h"
#include "Texture.h"
#include "VertexBuffer.h"
#include "Color.h"

#include <typeinfo>

namespace LEGO {

namespace {

// The frame: nine quads (left edge, bottom-left corner, bottom edge, bottom-right corner, right
// edge, top-right corner, title bar, top-left corner, client area) of four vertices each.
const UInt32 kFrameVertexCount = 36;

// NOTE: both builds create the buffer for 0x6c (108) vertices and upload 108 positions /
// texture coordinates from arrays of 36 (Mac and Win alike: count 0x6c), reading past the
// arrays; only vertices 0-35 are indexed. The arrays here are 108 long (zero-filled) so the
// upload stays inside them.
const UInt32 kFrameBufferVertexCount = 0x6c;
const UInt32 kFrameIndexCount = 0x36;

// Texture coordinates (pixels of LDDgraphics01.png; divided by the image size in Create).
// Mac table 0x634010 + 0x30 (PIC data), Win immediate stores in 0x607777.
const Real32 kFrameTexCoords[kFrameVertexCount * 2] = {
    36, 250,  41, 250,  41, 308,  36, 308,      // left edge
    36, 308,  41, 308,  41, 317,  36, 317,      // bottom-left corner
    41, 308,  48, 308,  48, 317,  41, 317,      // bottom edge
    48, 308,  57, 308,  57, 317,  48, 317,      // bottom-right corner
    48, 250,  57, 250,  57, 308,  48, 308,      // right edge
    48, 232,  57, 232,  57, 250,  48, 250,      // top-right corner
    41, 232,  48, 232,  48, 250,  41, 250,      // title bar
    36, 232,  41, 232,  41, 250,  36, 250,      // top-left corner
    41, 250,  48, 250,  48, 308,  41, 308       // client area
};

const UInt32 kFrameIndices[kFrameIndexCount] = {
     0,  1,  2,  0,  2,  3,   4,  5,  6,  4,  6,  7,   8,  9, 10,  8, 10, 11,
    12, 13, 14, 12, 14, 15,  16, 17, 18, 16, 18, 19,  20, 21, 22, 20, 22, 23,
    24, 25, 26, 24, 26, 27,  28, 29, 30, 28, 30, 31,  32, 33, 34, 32, 34, 35
};

} // namespace

// Mac 0x13f680 (inline, emitted in TU 060), Win 0x5c8d52
LDDWindowControl::LDDWindowControl(ControlManager* manager)
    : WindowControl(manager), m_minimizeButton(0), m_restoreButton(0)
{
}

// Mac 0x163bb0/0x163c10, Win sdd 0x5c96b7
LDDWindowControl::~LDDWindowControl()
{
}

// Mac 0x162010, Win 0x607777
bool LDDWindowControl::Create(bool titleButtons, bool buttonSounds)
{
    if (!WindowControl::Initialize())
        return false;
    SetInputFlags(GetInputFlags() & ~2u);
    m_titleRect.Set(Rect(10.0f, 0.0f, 10.0f, 16.0f));
    m_titleColor = Color::FromBytes(0, 0, 0, 0xff);
    m_titleShadowColor = Color::FromBytes(0x40, 0x80, 0xff, 0x80);
    m_borderWidth = 5.0f;
    m_marginLeft = 2.0f;
    m_marginTop = 2.0f;
    m_marginRight = 12.0f;
    m_marginBottom = 10.0f;
    m_titleBarHeight = 18.0f;
    m_minimumFrameSize = Vector2(30.0f, 28.0f);
    m_dragHelper.m_minimumSize = m_minimumFrameSize;

    if (!m_manager)
        return false;
    RenderControlManager* manager = dynamic_cast<RenderControlManager*>(m_manager);
    if (!manager)
        return false;
    Render* render = manager->GetRender();
    m_frameTexture = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"), render);
    if (!m_frameTexture)
        return false;
    m_frameOpaque = m_frameTexture->GetFormat() == 0;

    Vector3 positions[kFrameBufferVertexCount];           // (all zero until OnResized)
    for (UInt32 i = 0; i < kFrameBufferVertexCount; ++i)
        positions[i] = Vector3(0.0f, 0.0f, 0.0f);
    Real32 texCoords[kFrameBufferVertexCount * 2];
    for (UInt32 i = 0; i < kFrameBufferVertexCount * 2; ++i)
        texCoords[i] = i < kFrameVertexCount * 2 ? kFrameTexCoords[i] : 0.0f;
    for (UInt32 i = 0; i < kFrameVertexCount * 2; i += 2) {
        texCoords[i] = texCoords[i] / m_frameTexture->GetImageSize().x;
        texCoords[i + 1] = texCoords[i + 1] / m_frameTexture->GetImageSize().y;
    }

    VertexBuffer::CreateArgs args;
    args.vertexCount = kFrameBufferVertexCount;
    args.indexCount = kFrameIndexCount;
    args.frameCount = 0;
    args.dynamic = true;
    args.hasNormals = false;
    args.colorSetCount = 0;
    args.texCoordDimensions.PushBack(2);
    Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    m_frameBuffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    if (!m_frameBuffer)
        return false;
    m_frameBuffer->SetPositions(positions, 0, kFrameBufferVertexCount, 0);
    m_frameBuffer->SetTexCoords(texCoords, 0);
    m_frameBuffer->SetIndices(kFrameIndices, 0, kFrameIndexCount, 0);
    m_frameBuffer->SetPrimitiveType(3);
    if (titleButtons && !CreateTitleButtons(buttonSounds))
        return false;
    return true;
}

// Mac 0x162620, Win 0x60731e
bool LDDWindowControl::CreateTitleButtons(bool buttonSounds)
{
    ButtonControl::StateDescription state;
    state.m_transparent = true;
    state.m_image = m_frameTexture;

    m_minimizeButton = new ButtonControl(m_manager);
    if (!m_minimizeButton->Initialize())
        return false;
    m_minimizeButton->Setup(this, 0, 7, Rect(Vector2(0.0f, 3.0f), Vector2()), 0, false);
    state.m_image = m_frameTexture;
    state.m_imageRect.Set(Rect(Vector2(52.0f, 0.0f), Vector2(13.0f, 13.0f)));
    m_minimizeButton->SetStateDescription(0, state);
    state.m_imageRect.Set(Rect(Vector2(68.0f, 0.0f), Vector2(13.0f, 13.0f)));
    m_minimizeButton->SetStateDescription(1, state);
    state.m_imageRect.Set(Rect(Vector2(84.0f, 0.0f), Vector2(13.0f, 13.0f)));
    m_minimizeButton->SetStateDescription(2, state);
    state.m_imageRect.Set(Rect(Vector2(100.0f, 0.0f), Vector2(13.0f, 13.0f)));
    m_minimizeButton->SetStateDescription(3, state);
    if (buttonSounds)
        LDDApplication::GetInstance()->AddButtonSound(m_minimizeButton);
    ControlObserver::Attach(m_minimizeButton);

    m_restoreButton = new ButtonControl(m_manager);
    if (!m_restoreButton->Initialize())
        return false;
    // flags 6 on both binaries: created hidden (only the minimize button shows at first)
    m_restoreButton->Setup(this, 0, 6, Rect(Vector2(0.0f, 3.0f), Vector2()), 0, false);
    state.m_image = m_frameTexture;
    state.m_imageRect.Set(Rect(Vector2(52.0f, 32.0f), Vector2(13.0f, 13.0f)));
    m_restoreButton->SetStateDescription(0, state);
    state.m_imageRect.Set(Rect(Vector2(68.0f, 32.0f), Vector2(13.0f, 13.0f)));
    m_restoreButton->SetStateDescription(1, state);
    state.m_imageRect.Set(Rect(Vector2(84.0f, 32.0f), Vector2(13.0f, 13.0f)));
    m_restoreButton->SetStateDescription(2, state);
    state.m_imageRect.Set(Rect(Vector2(100.0f, 32.0f), Vector2(13.0f, 13.0f)));
    m_restoreButton->SetStateDescription(3, state);
    if (buttonSounds)
        LDDApplication::GetInstance()->AddButtonSound(m_restoreButton);
    ControlObserver::Attach(m_restoreButton);
    return true;
}

// Mac 0x162e60, Win 0x606df4
void LDDWindowControl::SetContentSize(const Vector2& size)
{
    Real32 width = size.x + m_marginLeft + m_marginRight;
    if (width <= m_minimumFrameSize.x)
        width = m_minimumFrameSize.x;
    Real32 height = size.y + m_marginBottom + m_titleBarHeight;
    if (height <= m_minimumFrameSize.y)
        height = m_minimumFrameSize.y;
    Vector2 minimum(width, height);
    Vector2 current(m_size);
    bool grow = false;
    if (current.x < minimum.x) {
        current.x = minimum.x;
        grow = true;
    }
    if (current.y < minimum.y) {
        current.y = minimum.y;
        grow = true;
    }
    if (grow)
        SetSize(current);
    m_dragHelper.m_minimumSize = minimum;
}

// Mac 0x162f30 (thunk 0x162f60), Win 0x606ebe
void LDDWindowControl::OnMouseClicked(Control* control, const Vector2& /*position*/, UInt32 button)
{
    if (button != 0)
        return;
    if (control == m_minimizeButton)
        Minimize();
    else if (control == m_restoreButton)
        Restore();
}

// Mac 0x162fa0, Win 0x606ef4
void LDDWindowControl::OnResized(const Vector2& /*oldSize*/)
{
    const Real32 w = m_size.x;
    const Real32 h = m_size.y;
    const Real32 title = m_titleBarHeight;
    const Real32 left = m_marginLeft;
    const Real32 right = w - m_marginRight;
    const Real32 bottom = h - m_marginBottom;
    const Vector3 frame[kFrameVertexCount] = {
        // left edge
        Vector3(0.0f, title, 0.0f),     Vector3(left, title, 0.0f),
        Vector3(left, bottom, 0.0f),    Vector3(0.0f, bottom, 0.0f),
        // bottom-left corner
        Vector3(0.0f, bottom, 0.0f),    Vector3(11.0f, bottom, 0.0f),
        Vector3(11.0f, h, 0.0f),        Vector3(0.0f, h, 0.0f),
        // bottom edge
        Vector3(11.0f, bottom, 0.0f),   Vector3(w - 11.0f, bottom, 0.0f),
        Vector3(w - 11.0f, h, 0.0f),    Vector3(11.0f, h, 0.0f),
        // bottom-right corner
        Vector3(w - 11.0f, bottom, 0.0f), Vector3(w, bottom, 0.0f),
        Vector3(w, h, 0.0f),            Vector3(w - 11.0f, h, 0.0f),
        // right edge
        Vector3(right, title, 0.0f),    Vector3(w, title, 0.0f),
        Vector3(w, bottom, 0.0f),       Vector3(right, bottom, 0.0f),
        // top-right corner
        Vector3(w - 12.0f, 0.0f, 0.0f), Vector3(w, 0.0f, 0.0f),
        Vector3(w, title, 0.0f),        Vector3(w - 12.0f, title, 0.0f),
        // title bar
        Vector3(4.0f, 0.0f, 0.0f),      Vector3(w - 12.0f, 0.0f, 0.0f),
        Vector3(w - 12.0f, title, 0.0f), Vector3(4.0f, title, 0.0f),
        // top-left corner
        Vector3(0.0f, 0.0f, 0.0f),      Vector3(4.0f, 0.0f, 0.0f),
        Vector3(4.0f, title, 0.0f),     Vector3(0.0f, title, 0.0f),
        // client area
        Vector3(left, title, 0.0f),     Vector3(right, title, 0.0f),
        Vector3(right, bottom, 0.0f),   Vector3(left, bottom, 0.0f)
    };
    Vector3 positions[kFrameBufferVertexCount];   // (see kFrameBufferVertexCount)
    for (UInt32 i = 0; i < kFrameBufferVertexCount; ++i)
        positions[i] = i < kFrameVertexCount ? frame[i] : Vector3(0.0f, 0.0f, 0.0f);
    m_frameBuffer->SetPositions(positions, 0, kFrameBufferVertexCount, 0);
    if (m_minimizeButton) {
        const Real32 x = w - 8.0f - 16.0f;
        m_minimizeButton->SetPosition(Vector2(x, 3.0f));
        m_restoreButton->SetPosition(Vector2(x, 3.0f));
    }
    Real32 titleRight = w - 23.0f - 16.0f - 16.0f;
    if (titleRight <= m_titleRect.m_min.x)
        titleRight = m_titleRect.m_min.x;
    m_titleRect.m_max.x = titleRight;
}

// Mac 0x163490, Win 0x606cac
void LDDWindowControl::OnWindowActivated()
{
    m_titleShadow = true;
}

// Mac 0x1634a0, Win 0x606cb4
void LDDWindowControl::OnWindowDeactivated()
{
    m_titleShadow = false;
}

// Mac 0x1634b0, Win 0x6072e0
void LDDWindowControl::OnMinimized()
{
    m_minimizeButton->SetShown(false);
    m_restoreButton->SetShown(true);
}

// Mac 0x1634e0, Win 0x6072ff
void LDDWindowControl::OnRestored()
{
    m_minimizeButton->SetShown(true);
    m_restoreButton->SetShown(false);
}

namespace {

// Resize cursor of a drag mode (edge bits); 0 when the mode is no resize. (inlined twice in
// UpdateCursor)
UInt32 ResizeCursor(UInt32 mode)
{
    switch (mode) {
    case 0x04: return 0xc;
    case 0x08: return 10;
    case 0x0c: case 0x30: return 0xf;
    case 0x10: return 0xd;
    case 0x18: case 0x24: return 0xe;
    case 0x20: return 0xb;
    default: return 0;
    }
}

} // namespace

// Mac 0x163510, Win 0x606cbc
void LDDWindowControl::UpdateCursor(const Vector2& position)
{
    if (!m_dragHelper.m_resizable)
        return;
    UInt32 cursor;
    UInt32 mode = m_dragMode;
    if (mode == kDragMove) {
        cursor = 5;
    } else if (mode == kDragNone) {
        mode = HitTest(position);
        if (mode == kDragMove)
            cursor = 4;
        else
            cursor = ResizeCursor(mode);
    } else {
        cursor = ResizeCursor(mode);
    }
    if (cursor == 0)
        cursor = 1;
    SetCursor(cursor, true);
}

} // namespace LEGO
