// LEGO Digital Designer — restored source
// LEGO::RecentModelsControl — the list of recently used models ("recentModelsControl" of the
//   edit mode): a MatrixLayoutControl with one RecentModelControl per file of
//   LDDApplication::GetRecentModels (at most six; files that are missing or do not load are
//   removed from the list), updated when the application reports a newly opened / saved model.
// LEGO::RecentModelsControl::RecentModelControl — one entry: a framed box (cell of
//   "Graphics/LDDgraphics01.png", lighter while the mouse is over it) with the file name as text
//   and a StorageSceneControl thumbnail of the model; a click opens the model.
//
// Home TU 396 (Mac 0x37faa0-0x38249c, old guess "RecentModelsControl_RecentModelControl.cpp";
// Win 0x67ad14-0x67ba95).
//
// RecentModelsControl: RTTI vmi {MatrixLayoutControl, LDDApplicationObserver (+0x17c)}. Mac
//   layout (constructor 0x37fb10): MatrixLayoutControl +0x000..+0x17c, LDDApplicationObserver
//   +0x17c (subject +0x180), m_sceneManager +0x184, m_models +0x188, m_texture +0x198, Object
//   +0x19c (sizeof 0x1a4). Win (constructor 0x67b27c): observer +0x190, m_sceneManager +0x19c,
//   m_models +0x1a0, m_texture +0x1b0, Object +0x1b4 (new 0x1c0).
//   Vtables Mac 0x7545bc (38 entries: MatrixLayoutControl's 36, Create 36, the Mac-only
//   overrider entry 37 of OnRecentModelsChanged), observer-in 0x754660, Object-in 0x7546a4.
//   Win: Create is slot 33 (0x67ba02).
// RecentModelControl: RTTI vmi {RenderControl, ControlObserver (+0x1ec)}. Mac layout
//   (constructor 0x380d30): RenderControl +0x000..+0x1ec, ControlObserver +0x1ec (subjects
//   +0x1f0), m_storageControl +0x200, m_path +0x204 (name +0x208, directory +0x218), Object
//   +0x21c (sizeof 0x224). Win (constructor 0x67b05d): ControlObserver +0x208, m_storageControl
//   +0x220, m_path +0x224, Object +0x230 (new 0x23c).
//   Vtable Mac 0x754708 (48 entries: overrides OnFinalRelease 2, OnRectangleChanged 8,
//   OnMouseEnter 16, OnMouseLeave 17, OnMouseClicked 21; Mac-only overrider entries 45-47 of
//   the ControlObserver handlers OnMouseEnter / OnMouseLeave / OnMouseClicked).
// Class names from RTTI; method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Path.h"
#include "Geometry.h"
#include "MatrixLayoutControl.h"
#include "RenderControl.h"
#include "LDDApplication.h"
#include "StorageSceneControl.h"

namespace LEGO {

class BrickConnectSceneControl;
class BrickConnectSceneManager;
class StorageSceneControl;
class Texture;

class RecentModelsControl : public MatrixLayoutControl, public LDDApplicationObserver {
public:
    class RecentModelControl;
    typedef BasicArray<RefPointer<RecentModelControl> > ModelControlArray;

    // The image cells of an entry in "Graphics/LDDgraphics01.png": normal and with the mouse
    // over it (static initialiser Mac 0x3823c0; Win 0xaefea8 / 0xaefe98).
    static const Rect s_imageRectNormal;   // Mac 0x77f318: (15, 124) 19 x 101
    static const Rect s_imageRectHover;    // Mac 0x77f328: (36, 124) 19 x 101

    // Keeps the scene control's scene manager (for the thumbnails). Mac 0x37fb10, Win 0x67b27c
    RecentModelsControl(ControlManager* manager, BrickConnectSceneControl* sceneControl);
    virtual ~RecentModelsControl();                                         // Mac 0x3806d0/0x380810, Win sdd 0x67b412

    // Loads the image, shows the control, builds the entries and observes the application;
    // false without the image. Mac slot 36 (+0x90), Win slot 33 (+0x84). Mac 0x37fc20, Win 0x67ba02
    virtual bool Create();

    BrickConnectSceneManager* GetSceneManager() const { return m_sceneManager; }
    Texture*                  GetTexture() const { return m_texture.Get(); }
    const ModelControlArray&  GetModelControls() const { return m_models; }

    // LDDApplicationObserver: a new entry for `path` goes first (an existing one for the same
    // file is removed, at most six stay). Nothing when the entry cannot be created.
    // Mac 0x380ad0 (Mac-only primary entry 37; thunk 0x3812c0), Win 0x67b6c7
    virtual void OnRecentModelsChanged(const Path& path);

protected:
    // Leaves the application, then the Control part. Mac 0x380980 (thunks 0x3809f0, 0x380a60),
    // Win 0x67b434
    virtual void OnFinalRelease();

    // The entries of the application's recent models (at most six); the files that are
    // missing or cannot be shown are removed from the application's list.
    // Mac 0x37ffb0, Win 0x67b7ed (name inferred)
    void Rebuild();

    // ---- data (Mac offsets / Win offsets) ----
    BrickConnectSceneManager* m_sceneManager;   // +0x184 / +0x19c
    ModelControlArray         m_models;         // +0x188 / +0x1a0 (vtable Mac 0x778740)
    RefPointer<Texture>       m_texture;        // +0x198 / +0x1b0 "Graphics/LDDgraphics01.png"
                                                // Object +0x19c / +0x1b4
};

class RecentModelsControl::RecentModelControl : public RenderControl, public ControlObserver {
public:
    // Mac 0x380d30, Win 0x67b05d
    explicit RecentModelControl(ControlManager* manager);
    virtual ~RecentModelControl();                                          // Mac 0x381820/0x381a40, Win sdd 0x67b3da

    // Initializes the entry below `owner` for the model file `path`: the thumbnail (a
    // StorageSceneControl of the file, 20 units of bottom margin, hand cursor, buffered at
    // twice the resolution), the framed image, the file name as text (font 0, centred, 3 units
    // up). False when initialization fails. Mac 0x380ec0, Win 0x67b46c
    bool Create(RecentModelsControl* owner, const Path& path);

    const Path&          GetPath() const { return m_path; }
    StorageSceneControl* GetStorageControl() const { return m_storageControl.Get(); }

    // ---- Control / RenderControl overrides (Mac slot / Win slot) ----
    // The text wraps at the width minus 6 (when positive). Mac 0x381420, Win 0x67ae12
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5
    // The hover / normal image cell. Mac 0x3814a0 / 0x3814d0, Win 0x67ad14 / 0x67ad30
    virtual void OnMouseEnter(const Vector2& position);                     // 16/13
    virtual void OnMouseLeave(const Vector2& position);                     // 17/14
    // Left button: the model is opened (LDDApplication::OpenDocument with the loaded model).
    // Mac 0x381500, Win 0x67ae90
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);    // 21/18

    // ---- ControlObserver (the thumbnail): the same (Mac-only primary entries 45-47) ----
    virtual void OnMouseEnter(Control* control, const Vector2& position);   // Mac 0x381550 (thunk 0x381580), Win 0x67ad4c
    virtual void OnMouseLeave(Control* control, const Vector2& position);   // Mac 0x3815b0 (thunk 0x3815e0), Win 0x67ad5f
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // Mac 0x381610 (FUN 0x381660), Win 0x67aed4

protected:
    using ControlObserver::OnRectangleChanged;   // (the observer handlers stay visible)
    // Leaves the observed controls (no OnDetached), then RenderControl's. Mac 0x381cb0
    // (thunks 0x381d20 …), Win 0x67b448
    virtual void OnFinalRelease();

    // Opens the model of the thumbnail. (inlined in both handlers; name inferred)
    void OpenModel();

    // ---- data (Mac offsets / Win offsets) ----
    RefPointer<StorageSceneControl> m_storageControl;   // +0x200 / +0x220
    Path                            m_path;             // +0x204 / +0x224
                                                        // Object +0x21c / +0x230
};

} // namespace LEGO
