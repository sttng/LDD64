// LEGO Digital Designer — restored source
// LEGO::StorageSceneControl — a ModelControl that shows a stored model (a StorageScene2): the
//   items of the template palette (TemplateSelectorControl) and the thumbnails of the recent
//   models (RecentModelsControl::RecentModelControl). The model is given directly (SetModel)
//   or as a file (SetPath) that is loaded on demand: on the first draw in the background
//   (LoadOperation on an own OperationQueue, finished in the main thread through the
//   SystemUpdater), or synchronously by GetModel.
// LEGO::LoadStorageSceneOperation — an Operation that reads a model file into a StorageScene2
//   (LoadModel, no error alerts).
// LEGO::StorageSceneControl::LoadOperation — the LoadStorageSceneOperation of a
//   StorageSceneControl: after loading it queues itself as an UpdateObserver of the
//   SystemUpdater, whose Update hands the result to the control.
//
// Home TU 147 "StorageSceneControl.cpp" (Mac 0x209580-0x20af5b; Win 0x692e05-0x693479 +
// 0x67a7d0). The constructor is inline (emitted weak in TU 074, Mac 0x167c70 C2 with VTT; Win
// 0x67a68a).
//
// StorageSceneControl: RTTI si {ModelControl}. Mac layout (constructor 0x167c70):
//   ModelControl +0x000..+0x238, m_database +0x238, m_model +0x23c, m_path +0x240 (name +0x244,
//   directory +0x254), m_operation +0x258, m_queue +0x25c, Object +0x260 (sizeof 0x268).
//   Win (dtor body 0x6931b0, StartLoading 0x6930f7): m_database +0x254, m_model +0x258, m_path
//   +0x25c, m_operation +0x268, m_queue +0x26c, Object +0x270 (new 0x27c).
//   Vtables Mac 0x72ad3c (48 entries: StartLoading 0x20a4a0, OnLoaded 0x20a610) + callback
//   0x72ae4c {0x209e40, 0x20a090, OnLoaded thunk 0x20a7c0, OnNotHandled thunk 0x2093f0}.
// LoadStorageSceneOperation: RTTI si {Operation}. Mac: Operation +0x00..+0x30, m_scene +0x30,
//   m_path +0x34 (name +0x38, directory +0x48), m_success +0x4c, m_database +0x50, Object +0x54.
//   Win: m_scene +0x34 … (Execute 0x6933f1 / Update 0x692fbb: success +0x40, scene +0x30 of the
//   LoadOperation seen from the control). Vtable Mac 0x72af0c {Execute 0x209580, D1 0x20a7e0,
//   D0 0x20a840}.
// LoadOperation: RTTI vmi {LoadStorageSceneOperation, UpdateObserver (+0x54)}; Mac m_control
//   +0x5c, Object +0x60 (new 0x68); Win new 0x64. Vtable Mac 0x72b5d8 {Execute 0x20a190, D1
//   0x20a990, D0 0x20aa00, Update 0x20a1c0}.
// Class names from RTTI; method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Path.h"
#include "OperationManager.h"
#include "Updater.h"
#include "ModelControl.h"

namespace LEGO {

struct StorageScene2;
class BrickDatabase;
class BrickConnectSceneManager;

// ===========================================================================================
// LoadStorageSceneOperation
// ===========================================================================================
class LoadStorageSceneOperation : public Operation {
public:
    // Keeps the path and the database (referenced); no scene yet. Mac 0x20ac80 (C2 with VTT),
    // Win 0x692d79
    LoadStorageSceneOperation(const Path& path, BrickDatabase* database);
    virtual ~LoadStorageSceneOperation();                                   // Mac 0x20a7e0/0x20a840, Win sdd 0x692e05

    // A new StorageScene2 when there is none, then LoadModel(path, scene, database, no
    // alerts) into m_success. Mac 0x209580, Win 0x6933f1
    virtual void Execute();

    StorageScene2* GetScene() const { return m_scene; }
    bool           IsSuccess() const { return m_success; }

protected:
    RefPointer<StorageScene2> m_scene;      // Mac +0x30
    Path                      m_path;       // Mac +0x34
    bool                      m_success;    // Mac +0x4c
    RefPointer<BrickDatabase> m_database;   // Mac +0x50
                                            // Object Mac +0x54
};

// ===========================================================================================
// StorageSceneControl
// ===========================================================================================
class StorageSceneControl : public ModelControl {
public:
    class LoadOperation;

    // The scene manager's database (referenced), no model, an empty path.
    // Mac 0x167c70 (inline, C2 with VTT), Win 0x67a68a
    StorageSceneControl(ControlManager* manager, BrickConnectSceneManager* sceneManager);
    // Cancels a running load. Mac 0x209d00/0x209f20 (body 0x209af0), Win sdd 0x67a7d0 (body 0x6931b0)
    virtual ~StorageSceneControl();

    // Shows `model` (a pending load is cancelled); the meshes are made again; model input on;
    // true. NOTE: `model` must not be null (AddRef without a check, both builds).
    // Mac 0x209730, Win 0x692ffe (+ 0x692f49)
    bool SetModel(StorageScene2* model);
    // Shows the model file `path` (loaded on the first draw or by GetModel); a pending load is
    // cancelled; model input on; true. Mac 0x2097c0, Win 0x693017 (+ 0x692f82)
    bool SetPath(const Path& path);
    // The model; without one it is loaded now: synchronously when no background load runs,
    // else the load is waited for. Null when the file cannot be loaded. Mac 0x209830, Win 0x693334
    StorageScene2* GetModel();
    const Path& GetPath() const { return m_path; }

    // ---- ModelControl ----
    // With a model: the lazy load of its designs, materials and decorations. Without: the
    // background load of the file starts (the caching image is shown meanwhile).
    // Mac 0x20a4a0, Win 0x6930f7
    virtual void StartLoading();
    // The meshes of every part of the model; the camera follows the model's camera (pitch / yaw
    // of its view direction) when it has one. False when a part cannot be added.
    // Mac 0x20a610 (thunk 0x20a7c0), Win 0x69327a
    virtual bool OnLoaded(const LazyLoadData& data);

protected:
    friend class LoadOperation;

    // LDD64: drops the pending load (cancelled first when `cancel`) and detaches it from this
    // control, so that its queued Update does nothing.
    void DropOperation(bool cancel);

    // The model replaced (pending load cancelled), meshes dropped. Win 0x692f49 (Mac inlined)
    void ReplaceModel(StorageScene2* model);
    // The path replaced (pending load cancelled), meshes dropped. Win 0x692f82 (Mac inlined)
    void ReplacePath(const Path& path);
    // Takes the result of the finished background load: its scene, or the "failed" state;
    // the operation is dropped. Win 0x692fbb (Mac inlined in 0x209830 / 0x20a1c0)
    void FinishLoading();
    // StartLoading with a model. Mac 0x20a300, Win 0x69304a
    void StartLoadingModel();

    // ---- data (Mac offsets / Win offsets) ----
    RefPointer<BrickDatabase>  m_database;    // +0x238 / +0x254 the scene manager's
    RefPointer<StorageScene2>  m_model;       // +0x23c / +0x258
    Path                       m_path;        // +0x240 / +0x25c
    RefPointer<LoadOperation>  m_operation;   // +0x258 / +0x268 background load
    RefPointer<OperationQueue> m_queue;       // +0x25c / +0x26c (created on the first load)
                                              // Object +0x260 / +0x270
};

// ===========================================================================================
// StorageSceneControl::LoadOperation
// ===========================================================================================
class StorageSceneControl::LoadOperation : public LoadStorageSceneOperation, public UpdateObserver {
public:
    // Mac inlined in 0x20a4a0, Win 0x692e24
    LoadOperation(const Path& path, BrickDatabase* database, StorageSceneControl* control);
    virtual ~LoadOperation();                                               // Mac 0x20a990/0x20aa00, Win 0x692ec0 (sdd 0x692f2a)

    // Loads, then (not cancelled) queues itself for the main thread. Mac 0x20a190, Win 0x693453
    virtual void Execute();
    // Main thread: (not cancelled) the control takes the result. Mac 0x20a1c0 (thunk 0x20a260),
    // Win 0x693030
    virtual void Update();
    // LDD64: the control no longer waits for this load (it dropped or finished it).
    void ForgetControl() { m_control = 0; }

protected:
    StorageSceneControl* m_control;   // Mac +0x5c (not referenced)
                                      // Object Mac +0x60
};

} // namespace LEGO
