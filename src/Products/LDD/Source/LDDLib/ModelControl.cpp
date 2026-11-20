// LEGO Digital Designer — restored source
// ModelControl.cpp — TU 146 (Mac 0x208090-0x20957b; Win 0x68528a-0x685a7a). See ModelControl.h.
#include "ModelControl.h"
#include "RenderControlManager.h"
#include "Render.h"
#include "RenderQueue.h"
#include "Texture.h"
#include "Image/Bitmap.h"
#include "LDDUtilities.h"

namespace LEGO {

namespace {

// Camera turn per unit of mouse movement (Mac 0x637744 float; Win multiplies by the double
// 0.025). (name inferred)
const Real32 kDragAngleScale = 0.025f;
// Latitude limit: pi/2 - 0.001 (Mac 0x63774c; Win computes it from pi/2 and an epsilon * 100).
const Real32 kMaxLatitude = 1.5697963f;
// sin(fov / 2) of the 45 degree camera, sin((float)pi / 8) rounded to float: Mac constant
// 0x637740 (0x3ec3ef16); Win computes LEGO::Sin(pi * 0.25 * 0.5), the same value.
const Real32 kSinHalfFov = 0.38268346f;
const Real32 kFov = 0.78539819f;   // pi / 4
// Size of a cell of ModelCaching.png (cell 0: loading, cell 1: failed).
const Real32 kCachingCellSize = 64.0f;

RenderControlManager* GetRenderManager(const Control* control)
{
    return static_cast<RenderControlManager*>(control->GetManager());
}

} // namespace

// Mac 0x208090 (no Win partner found; inlined into its callers)
bool ModelControl::RenderToBitmap(Bitmap& bitmap, Texture* target)
{
    Render* render = GetRenderManager(this)->GetRender();
    Texture* previousTarget = render->GetRenderTarget();
    RefPointer<Texture> targetRef(target);

    Control* root = GetRenderManager(this)->GetRootControl();
    if ((Real32)bitmap.m_width > root->GetSize().x || (Real32)bitmap.m_height > root->GetSize().y)
        return false;

    // The meshes are made now (no callback, the palette's material set).
    LazyLoadData data;
    data.materialSet = 1;
    OnLoaded(data);

    render->BeginFrame();
    render->SetRenderTarget(target);
    render->SetViewport(Rect(0.0f, 0.0f, (Real32)bitmap.m_width, (Real32)bitmap.m_height));
    render->DisableScissor();
    render->SetMaterial(0);
    render->SetTexture(0, 0);
    render->EnableDepthTest(true);
    render->EnableLighting(true);
    render->SetCullMode(1);
    render->SetClearColor(Color(1.0f, 1.0f, 1.0f, 0.0f));
    render->Clear(7);
    DrawModel(render);
    render->ReadPixels((UInt8*)bitmap.m_data, bitmap.m_width, bitmap.m_height, 0, 0, target == 0,
                       ((bitmap.m_format & 0x40000000) >> 30) != 0);
    render->SetRenderTarget(previousTarget);
    render->EndFrame();
    if (target == 0)
        GetRenderManager(this)->GetRootControl()->Invalidate(false);
    return true;
}

// Mac 0x208460, Win 0x6854b9
void ModelControl::DrawModel(Render* render)
{
    Sphere sphere(m_draw.m_box);
    Matrix3D matrix;
    render->SetViewMatrix(matrix);

    // Key light: white, from the front top. NOTE: the three lights are built with zero
    // attenuation and spot values (directional lights ignore them).
    Light key;
    key.type = Light::kDirectional;
    key.constantAttenuation = 0.0f;
    key.linearAttenuation = 0.0f;
    key.quadraticAttenuation = 0.0f;
    key.spotCutoff = 0.0f;
    key.spotExponent = 0.0f;
    key.reserved60 = 0;
    key.diffuse = Color::White();
    key.specular = Color(0.7f, 0.7f, 0.7f, 1.0f);
    key.direction = Vector3(0.0f, 1.0f, 2.0f);
    key.direction.Normalize();

    Light fill1;
    fill1.type = Light::kDirectional;
    fill1.constantAttenuation = 0.0f;
    fill1.linearAttenuation = 0.0f;
    fill1.quadraticAttenuation = 0.0f;
    fill1.spotCutoff = 0.0f;
    fill1.spotExponent = 0.0f;
    fill1.reserved60 = 0;
    fill1.diffuse = Color(0.15f, 0.15f, 0.15f, 1.0f);
    fill1.specular = Color(0.15f, 0.15f, 0.15f, 1.0f);
    fill1.direction = Vector3(-1.01f, 1.01f, -1.01f);

    Light fill2;
    fill2.type = Light::kDirectional;
    fill2.constantAttenuation = 0.0f;
    fill2.linearAttenuation = 0.0f;
    fill2.quadraticAttenuation = 0.0f;
    fill2.spotCutoff = 0.0f;
    fill2.spotExponent = 0.0f;
    fill2.reserved60 = 0;
    fill2.diffuse = Color(0.1f, 0.1f, 0.1f, 1.0f);
    fill2.specular = Color(0.1f, 0.1f, 0.1f, 1.0f);
    fill2.direction = Vector3(0.501f, -1.01f, -1.01f);

    render->SetLight(&key, 0);
    render->SetLight(&fill1, 1);
    render->SetLight(&fill2, 2);
    render->SetBlendFunc(2, 3);

    // Camera on the sphere's view cone: distance radius / sin(fov / 2) along the rotated +z.
    matrix.SetEulerAngles(m_camera.x, m_camera.y, 0.0f);
    Vector3 offset = Vector3(0.0f, 0.0f, 1.0f) * (sphere.m_radius / kSinHalfFov);
    Vector3 eye = Rotate(offset, matrix) + sphere.m_center;
    Real32 distance = (eye - sphere.m_center).GetLength();
    matrix.SetPerspectiveFovY(kFov, 1.0f, distance - sphere.m_radius, distance + sphere.m_radius);
    render->SetProjectionMatrix(matrix);
    matrix.SetLookAt(eye, sphere.m_center, Vector3(0.0f, 1.0f, 0.0f));
    render->SetViewMatrix(matrix);

    RenderQueue* queue = render->GetRenderQueue(0);
    m_draw.Draw(queue, matrix);
    queue->Flush(0.02f, true, true);
    queue->Clear();

    render->SetLight(0, 0);
    render->SetLight(0, 1);
    render->SetLight(0, 2);
}

// Mac 0x208be0
void ModelControl::SetCamera(const Vector2& camera)
{
    if (m_camera != camera) {
        m_camera = camera;
        Redraw();
    }
}

// Mac 0x208c40 (inlined on Win)
void ModelControl::ClearModel()
{
    CancelLoading();
    ClearDraw();
    Redraw();
}

// Mac 0x208d40 (inlined on Win)
void ModelControl::CancelLoading()
{
    if (m_ticket != 0) {
        GetSceneManager()->CancelLazyLoad(m_ticket);
        m_ticket = 0;
    }
}

// Mac 0x208d80, Win 0x685413
void ModelControl::LoadCachingImage()
{
    if (m_cachingImage.IsNull())
        m_cachingImage = LoadApplicationTexture(String("Graphics/EditScreen/ModelCaching.png"),
                                                GetRenderManager(this)->GetRender());
}

// Mac 0x208e40, Win 0x68546f
void ModelControl::LazyLoad(const LazyLoadData& data)
{
    CancelLoading();
    m_loadFailed = false;
    m_ticket = GetSceneManager()->RequestLazyLoad(data);
    if (m_draw.GetSubParts().IsEmpty() && m_ticket != 0 && m_cachingImage.IsNull())
        LoadCachingImage();
}

// Mac 0x208ec0, Win 0x6858ef
void ModelControl::EnableModelInput()
{
    SetInputFlags(GetInputFlags() | 1);
    SetActive(true);
    SetEnabled(true);
    SetShown(true);
    SetTransparent(true);
}

// Mac 0x208f40, Win 0x68528a
void ModelControl::OnMouseMove(const Vector2& position)
{
    if (m_dragPosition != position) {
        if (GetPressedButtons() & 2) {
            if (m_dragPosition.x != position.x) {
                m_camera.y += (m_dragPosition.x - position.x) * kDragAngleScale;
                Redraw();
            }
            if (m_dragPosition.y != position.y) {
                m_camera.x += (m_dragPosition.y - position.y) * kDragAngleScale;
                if (m_camera.x < -kMaxLatitude)
                    m_camera.x = -kMaxLatitude;
                else if (m_camera.x > kMaxLatitude)
                    m_camera.x = kMaxLatitude;
                Redraw();
            }
        }
        m_dragPosition = position;
    }
    RenderControl::OnMouseMove(position);
}

// Mac 0x209060, Win 0x685382
void ModelControl::OnMousePressed(const Vector2& position, UInt32 button)
{
    if (button == 1)
        m_dragPosition = position;
    RenderControl::OnMousePressed(position, button);
}

// Mac 0x209090, Win 0x6853ae
void ModelControl::OnMouseEnter(const Vector2& position)
{
    SetCursor(GetCursor(), false);
    RenderControl::OnMouseEnter(position);
}

// Mac 0x2090c0, Win 0x685927
void ModelControl::Draw(ControlDrawArgs& args)
{
    RenderControl::Draw(args);
    Render* render = args.m_render;

    if (m_draw.GetSubParts().IsEmpty() && m_ticket == 0 && !m_loadFailed) {
        m_inStartLoading = true;
        StartLoading();
        m_inStartLoading = false;
    }

    if (!m_draw.GetSubParts().IsEmpty()) {
        args.Setup(true);
        if (!m_buffered)
            render->Clear(6);
        DrawModel(render);
        return;
    }

    if (!m_cachingImage.IsNull() && (m_ticket != 0 || m_loadFailed)) {
        args.Setup(false);
        Rect cell(Vector2(m_loadFailed ? kCachingCellSize : 0.0f, 0.0f),
                  Vector2(kCachingCellSize, kCachingCellSize));
        cell.Divide(m_cachingImage->GetImageSize());
        Rect rect(GetSize());
        Texture* image = m_cachingImage;
        args.DrawTexture(rect, image, cell, Color::White(), m_cachingImage->GetFormat() != 0);
    }
}

// Mac 0x209280 (thunk 0x2092c0), Win 0x6853cd
bool ModelControl::OnLoaded(const LazyLoadData& /*data*/)
{
    if (!m_inStartLoading)
        Redraw();
    return true;
}

// Mac 0x209300 (thunk 0x2093f0), Win 0x685ca8
void ModelControl::OnNotHandled(const LazyLoadData& /*data*/)
{
    m_ticket = 0;
    ClearDraw();
    m_loadFailed = true;
    if (!m_inStartLoading)
        Redraw();
}

} // namespace LEGO
