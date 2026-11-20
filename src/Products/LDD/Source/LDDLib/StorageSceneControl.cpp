// LEGO Digital Designer — restored source
// StorageSceneControl.cpp — TU 147 (Mac 0x209580-0x20af5b; Win 0x692e05-0x693479, 0x67a7d0)
// plus the inline constructor emitted in TU 074 (Mac 0x167c70, Win 0x67a68a).
// See StorageSceneControl.h.
#include "StorageSceneControl.h"
#include "StorageScene2.h"
#include "BrickDatabase.h"
#include "BrickConnectSceneManager.h"
#include "LDDUtilities.h"
#include "SystemUpdater.h"
#include "Geometry.h"

namespace LEGO {

// ===========================================================================================
// LoadStorageSceneOperation
// ===========================================================================================

// Mac 0x20ac80 (C2), Win 0x692d79
LoadStorageSceneOperation::LoadStorageSceneOperation(const Path& path, BrickDatabase* database)
    : m_scene(), m_path(), m_success(false), m_database(database)
{
    m_path.Assign(path);
}

// Mac 0x20a7e0 / 0x20a840 (body 0x20adb0), Win sdd 0x692e05
LoadStorageSceneOperation::~LoadStorageSceneOperation()
{
}

// Mac 0x209580, Win 0x6933f1
void LoadStorageSceneOperation::Execute()
{
    if (!m_scene)
        m_scene = new StorageScene2();
    m_success = LoadModel(m_path, *m_scene, m_database.Get(), false);
}

// ===========================================================================================
// StorageSceneControl::LoadOperation
// ===========================================================================================

// Mac inlined in 0x20a4a0, Win 0x692e24
StorageSceneControl::LoadOperation::LoadOperation(const Path& path, BrickDatabase* database,
                                                  StorageSceneControl* control)
    : LoadStorageSceneOperation(path, database), m_control(control)
{
}

// Mac 0x20a990 / 0x20aa00, Win 0x692ec0 (sdd 0x692f2a)
StorageSceneControl::LoadOperation::~LoadOperation()
{
}

// Mac 0x20a190, Win 0x693453
void StorageSceneControl::LoadOperation::Execute()
{
    LoadStorageSceneOperation::Execute();
    if (!IsCancelled())
        SystemUpdater::Enqueue(this);
}

// Mac 0x20a1c0 (thunk 0x20a260), Win 0x693030
// NOTE: on the Mac the operation is an NSOperation, and -cancel on one that has finished does
// nothing. GetModel (a click on a recent model while its preview loads) waits for the load,
// cancels and takes the result itself; the Update the load had queued then still ran
// FinishLoading, without an operation: a crash. Fixed: the control detaches the operation when
// it drops it (DropOperation), and a detached one does nothing.
void StorageSceneControl::LoadOperation::Update()
{
    if (!IsCancelled() && m_control != 0)
        m_control->FinishLoading();
}

// ===========================================================================================
// StorageSceneControl
// ===========================================================================================

// Mac 0x167c70 (C2), Win 0x67a68a
StorageSceneControl::StorageSceneControl(ControlManager* manager, BrickConnectSceneManager* sceneManager)
    : ModelControl(manager, sceneManager), m_database(sceneManager->GetDatabase()), m_model(),
      m_path(String(), 0), m_operation(), m_queue()
{
}

// Mac 0x209af0 (D1 0x209d00, D0 0x209f20), Win 0x6931b0 (sdd 0x67a7d0)
StorageSceneControl::~StorageSceneControl()
{
    DropOperation(true);
}

void StorageSceneControl::DropOperation(bool cancel)
{
    if (m_operation) {
        if (cancel)
            m_operation->Cancel();
        m_operation->ForgetControl();
        m_operation = 0;
    }
}

// Win 0x692f49 (Mac inlined)
void StorageSceneControl::ReplaceModel(StorageScene2* model)
{
    DropOperation(true);
    m_model = model;
    ClearModel();
}

// Win 0x692f82 (Mac inlined)
void StorageSceneControl::ReplacePath(const Path& path)
{
    DropOperation(true);
    m_path.Assign(path);
    ClearModel();
}

// Mac 0x209730, Win 0x692ffe
bool StorageSceneControl::SetModel(StorageScene2* model)
{
    EnableModelInput();
    ReplaceModel(model);
    return true;
}

// Mac 0x2097c0, Win 0x693017
bool StorageSceneControl::SetPath(const Path& path)
{
    EnableModelInput();
    ReplacePath(path);
    return true;
}

// Win 0x692fbb (Mac inlined in 0x209830 and 0x20a1c0)
void StorageSceneControl::FinishLoading()
{
    LoadOperation* operation = m_operation.Get();
    if (!operation->IsSuccess()) {
        m_loadFailed = true;
        m_ticket = 0;
    } else {
        m_model = operation->GetScene();
        ClearModel();
    }
    DropOperation(false);
}

// Mac 0x209830, Win 0x693334
StorageScene2* StorageSceneControl::GetModel()
{
    if (!m_model) {
        if (!m_operation) {
            RefPointer<StorageScene2> scene = new StorageScene2();
            if (LoadModel(m_path, *scene, m_database.Get(), false))
                ReplaceModel(scene.Get());
        } else {
            m_queue->Wait();
            m_operation->Cancel();
            FinishLoading();
        }
    }
    return m_model.Get();
}

// Mac 0x20a300, Win 0x69304a
void StorageSceneControl::StartLoadingModel()
{
    LazyLoadData data;
    data.callback = this;
    data.materialSet = 1;
    data.ticket = 0;
    const BasicArray<RefPointer<StorageScene2::Brick> >& bricks = m_model->m_bricks;
    for (UInt32 i = 0; i < bricks.GetSize(); ++i) {
        const StorageScene2::Brick* brick = bricks[i].Get();
        data.AddBrick(brick->m_designID);
        for (UInt32 j = 0; j < brick->m_parts.GetSize(); ++j) {
            const StorageScene2::Part* part = brick->m_parts[j].Get();
            data.AddMaterials(part->m_materials);
            data.AddDecorations(part->m_decorations);
        }
    }
    LazyLoad(data);
}

// Mac 0x20a4a0, Win 0x6930f7
void StorageSceneControl::StartLoading()
{
    if (m_model) {
        StartLoadingModel();
        return;
    }
    m_ticket = 0x6c6f6164;   // 'load': IsLoading() while the file loads
    LoadCachingImage();
    m_operation = new LoadOperation(m_path, m_database.Get(), this);
    if (!m_queue)
        m_queue = new OperationQueue();
    m_queue->Add(m_operation.Get());
}

// Mac 0x20a610 (thunk 0x20a7c0), Win 0x69327a
bool StorageSceneControl::OnLoaded(const LazyLoadData& data)
{
    if (!ModelControl::OnLoaded(data))
        return false;
    m_ticket = 0;
    ClearDraw();
    const BasicArray<RefPointer<StorageScene2::Brick> >& bricks = m_model->m_bricks;
    for (UInt32 i = 0; i < bricks.GetSize(); ++i) {
        const StorageScene2::Brick* brick = bricks[i].Get();
        for (UInt32 j = 0; j < brick->m_parts.GetSize(); ++j)
            if (!m_draw.AddPart(*brick->m_parts[j], 1))
                return false;
    }
    if (m_model->m_camera) {
        const Vector3 direction = Rotate(Vector3(0.0f, 0.0f, 1.0f), m_model->m_camera->m_transformation);
        m_camera.x = direction.GetPitch();
        m_camera.y = direction.GetYaw();
    }
    return true;
}

} // namespace LEGO
