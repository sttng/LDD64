// LEGO Digital Designer — restored source
// LEGO::ModelControl — a RenderControl that draws a small 3D model (a BrickDraw: the meshes of
//   one or more bricks) with its own camera: the camera angles follow a mouse drag, the meshes
//   are made lazily (BrickConnectSceneManager::RequestLazyLoad) and a "caching" image is shown
//   while they load (cell 0) or when loading failed (cell 1). Base of BricksControl (palette /
//   material / decoration items), BrickGroupControl (group tree) and StorageSceneControl
//   (templates).
//
// Home TU 146 "ModelControl.cpp" (Mac 0x208090-0x20957b; Win 0x68528a-0x685a7a). The
// constructor is inline (emitted weak in LDDUtilities, Mac 0x14e680 C2 with VTT; Win inlined
// into the derived constructors), the destructor is inline too (see below); IsInside /
// StartLoading are emitted in TU 075 (Mac 0x16a850 / 0x16a860) as weak copies of
// these inline members.
//
// RTTI vmi {RenderControl, BrickConnectSceneManager::LazyLoadCallback (+0x1ec / Win +0x208)},
//   virtual Object. Mac layout (constructor 0x14e680): RenderControl +0x000..+0x1ec,
//   LazyLoadCallback +0x1ec, members below, Object +0x238 (sizeof 0x240). Win: members at
//   Mac + 0x1c (BrickDraw +0x20c, caching image +0x238, camera +0x23c, drag position +0x244,
//   ticket +0x24c, flags +0x250 / +0x251), Object +0x258.
// Virtual slots: Mac 0x72a69c (48 entries): RenderControl's 0..44, overriding IsInside (3),
//   OnMouseEnter (16), OnMouseMove (18), OnMousePressed (19), Draw (33); new 45 StartLoading,
//   46 / 47 = the LazyLoadCallback overriders OnLoaded / OnNotHandled (thunks in the callback
//   vtable at -0x1ec). Win: primary 0x9d3bcc (41 entries, Win slot = Mac slot - 3 for 3..42,
//   40 = StartLoading 0x5c8021), callback vftable 0x9d3c74 {sdd thunk 0x639086, OnLoaded
//   0x6853cd, OnNotHandled 0x685ca8}.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "RenderControl.h"
#include "BrickDraw.h"
#include "BrickConnectSceneManager.h"

namespace LEGO {

class Render;
class Bitmap;
class Texture;

class ModelControl : public RenderControl, public BrickConnectSceneManager::LazyLoadCallback {
public:
    typedef BrickConnectSceneManager::LazyLoadData LazyLoadData;

    // The default camera (latitude, longitude) of a palette brick (BrickDatabase <DefaultCamera>
    // default, -0.800467 / 0.765663).
    static Vector2 GetDefaultCamera() { return Vector2(-0.80046701f, 0.76566267f); }

    // Mac 0x14e680 (inline, C2 with VTT), Win inlined
    ModelControl(ControlManager* manager, BrickConnectSceneManager* sceneManager)
        : RenderControl(manager), m_draw(sceneManager), m_cachingImage(), m_camera(GetDefaultCamera()),
          m_dragPosition(), m_ticket(0), m_loadFailed(false), m_inStartLoading(false)
    {
    }
    // Cancels a pending load. Inline: D2 emitted in TemplateSelectorControl.cpp (Mac 0x169450),
    // D1/D0 in BricksControl.cpp (Mac 0x16a780/0x16a7e0); Win 0x639149 (sdd 0x63957f)
    virtual ~ModelControl() { CancelLoading(); }

    // ---- Control overrides (Mac slot / Win slot) ----
    // The whole rectangle is "inside". Mac 0x16a850, Win 0x6545fc (ICF)
    virtual bool IsInside(const Vector2& /*point*/) const { return true; }    // 3/0
    // Re-applies the control's cursor. Mac 0x209090, Win 0x6853ae
    virtual void OnMouseEnter(const Vector2& position);                     // 16/13
    // With the left button down: the camera turns with the drag (0.025 per unit; latitude
    // clamped to +-(pi/2 - 0.001)). Mac 0x208f40, Win 0x68528a
    virtual void OnMouseMove(const Vector2& position);                      // 18/15
    // Left button: the drag starts at `position`. Mac 0x209060, Win 0x685382
    virtual void OnMousePressed(const Vector2& position, UInt32 button);    // 19/16
    // The model (meshes made: DrawModel in a 3D viewport), else the caching image while loading
    // / after a failure; starts the loading on the first draw. Mac 0x2090c0, Win 0x685927
    virtual void Draw(ControlDrawArgs& args);                               // 33/30

    // ---- new virtuals ----
    // Starts loading the meshes (builds the LazyLoadData, LazyLoad). Empty here.
    // Mac 0x16a860, Win 0x5c8021 (ICF)
    virtual void StartLoading() {}                                          // 45/40

    // ---- LazyLoadCallback ----
    // Redraw (unless called from StartLoading); true. Derived classes build the BrickDraw.
    // Mac 0x209280 (thunk 0x2092c0), Win 0x6853cd
    virtual bool OnLoaded(const LazyLoadData& data);                        // 46
    // Loading failed: no ticket, no meshes, the "failed" image. Mac 0x209300 (thunk 0x2093f0),
    // Win 0x685ca8
    virtual void OnNotHandled(const LazyLoadData& data);                    // 47

    // ---- non-virtual ----
    // Renders the model (meshes made synchronously through OnLoaded) at bitmap size into
    // `target` (null: the screen's back buffer, then the root is redrawn) and reads the pixels
    // into `bitmap`; false when the bitmap is larger than the root control.
    // Mac 0x208090 (callers: HTML building instructions 0x1c5ae0, ExportBOM 0x3ba5b0)
    bool RenderToBitmap(Bitmap& bitmap, Texture* target);
    // The camera angles (latitude, longitude); redraw when they change. Mac 0x208be0
    void SetCamera(const Vector2& camera);
    const Vector2& GetCamera() const { return m_camera; }
    // Cancels the load and drops the meshes; redraw. Mac 0x208c40
    void ClearModel();
    // Cancels a pending lazy load. Mac 0x208d40 (inlined on Win)
    void CancelLoading();
    // Requests the lazy load of `data` (with this as callback); loads the caching image when a
    // load is pending and nothing is drawn yet. Mac 0x208e40, Win 0x68546f
    void LazyLoad(const LazyLoadData& data);
    // Mouse input, active, enabled, shown, transparent. Mac 0x208ec0, Win 0x6858ef
    void EnableModelInput();

    BrickDraw&       GetBrickDraw()       { return m_draw; }
    const BrickDraw& GetBrickDraw() const { return m_draw; }
    BrickConnectSceneManager* GetSceneManager() const { return m_draw.m_sceneManager; }
    bool IsLoading() const { return m_ticket != 0; }

protected:
    // "Graphics/EditScreen/ModelCaching.png" (once). Mac 0x208d80, Win 0x685413
    void LoadCachingImage();
    // Draws the BrickDraw with three lights and a perspective camera at m_camera around the
    // bounding sphere of the model. Mac 0x208460, Win 0x6854b9
    void DrawModel(Render* render);
    // Drops the meshes (no redraw). (inline in the original: Mac 0x16a189 …)
    void ClearDraw() { m_draw.Clear(); }

    // ---- data (Mac offsets / Win offsets) ----
    BrickDraw            m_draw;            // +0x1f0 / +0x20c
    RefPointer<Texture>  m_cachingImage;    // +0x21c / +0x238
    Vector2              m_camera;          // +0x220 / +0x23c (latitude, longitude)
    Vector2              m_dragPosition;    // +0x228 / +0x244
    UInt32               m_ticket;          // +0x230 / +0x24c RequestLazyLoad ticket
    bool                 m_loadFailed;      // +0x234 / +0x250
    bool                 m_inStartLoading;  // +0x235 / +0x251
                                            // Object +0x238 / +0x258
};

} // namespace LEGO
