// LEGO Digital Designer — restored source
// RecentModelsControl.cpp — TU 396 (Mac 0x37faa0-0x38249c, old guess
// "RecentModelsControl_RecentModelControl.cpp"; Win 0x67ad14-0x67ba95). See RecentModelsControl.h.
#include "RecentModelsControl.h"
#include "StorageSceneControl.h"
#include "StorageScene2.h"
#include "BrickConnectSceneControl.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "RenderControlManager.h"
#include "TextLayout.h"
#include "Color.h"
#include "LEGO/String.h"
#include "LEGO/StdLib.h"

namespace LEGO {

// Static initialiser Mac 0x3823c0 (Win: initialised data 0xaefea8 / 0xaefe98)
const Rect RecentModelsControl::s_imageRectNormal(Vector2(15.0f, 124.0f), 19.0f, 101.0f);
const Rect RecentModelsControl::s_imageRectHover(Vector2(36.0f, 124.0f), 19.0f, 101.0f);

// ===========================================================================================
// RecentModelsControl
// ===========================================================================================

// Mac 0x37fb10, Win 0x67b27c
RecentModelsControl::RecentModelsControl(ControlManager* manager, BrickConnectSceneControl* sceneControl)
    : MatrixLayoutControl(manager), m_sceneManager(sceneControl->GetSceneManager()), m_models(),
      m_texture()
{
}

// Mac 0x3806d0 / 0x380810 (body 0x380540), Win sdd 0x67b412
RecentModelsControl::~RecentModelsControl()
{
}

// Mac 0x380980 (thunks 0x3809f0, 0x380a60), Win 0x67b434
void RecentModelsControl::OnFinalRelease()
{
    LDDApplicationObserver::Detach();
    MatrixLayoutControl::OnFinalRelease();
}

// Mac 0x37fc20, Win 0x67ba02
bool RecentModelsControl::Create()
{
    m_texture = LoadApplicationTexture(String("Graphics/LDDgraphics01.png"),
                                       static_cast<RenderControlManager*>(GetManager())->GetRender());
    if (!m_texture)
        return false;
    SetEnabled(true);
    SetShown(true);
    SetActive(true);
    Rebuild();
    LDDApplicationObserver::Attach(LDDApplication::GetInstance());
    return true;
}

// Mac 0x37ffb0, Win 0x67b7ed
void RecentModelsControl::Rebuild()
{
    const BasicArray<String>& recent = LDDApplication::GetInstance()->GetRecentModels();
    RemoveAllControls();
    BasicArray<String> missing;
    UInt32 count = 0;
    for (BasicArray<String>::ConstIterator it = recent.Begin(); it != recent.End() && count < 6;
         ++it, ++count) {
        // NOTE: a StorageScene2 is made and dropped again for every entry (both builds).
        RefPointer<StorageScene2> unused = new StorageScene2();
        Path path(String(), 0);
        if (!path.Append(*it)) {
            // NOTE: the messages are built and dropped (a trace compiled out; both builds).
            String message("Could not find folder that should contain the file: ");
            message << *it;
            missing.PushBack(*it);
        } else if (!path.FileExists()) {
            String message("Could not find recent file: ");
            message << *it;
            missing.PushBack(*it);
        } else {
            // NOTE: an entry that cannot be created is leaked (never referenced), both builds;
            // fixed: released (low).
            RecentModelControl* control = new RecentModelControl(GetManager());
            RefPointer<RecentModelControl> holder = control;
            if (!control->Create(this, path)) {
                if (control->GetParent())
                    control->SetParent(0);
                String message("Could not Create StorageSceneControl from file: ");
                message << *it;
                missing.PushBack(*it);
            } else {
                m_models.PushBack(RefPointer<RecentModelControl>(control));
                AddControl(control);
            }
        }
    }
    LDDApplication::GetInstance()->RemoveRecentModels(missing);
}

// Mac 0x380ad0 (thunk 0x3812c0), Win 0x67b6c7
void RecentModelsControl::OnRecentModelsChanged(const Path& path)
{
    // NOTE: an entry that cannot be created is leaked (never referenced), both builds; fixed:
    // released (low).
    RecentModelControl* control = new RecentModelControl(GetManager());
    RefPointer<RecentModelControl> holder = control;
    if (!control->Create(this, path)) {
        if (control->GetParent())
            control->SetParent(0);
        return;
    }
    UInt32 index = 0;
    while (index < m_models.GetSize() && !(m_models[index]->GetPath() == path))
        ++index;
    if (index < m_models.GetSize())
        m_models.Erase(index);
    if (m_models.GetSize() > 5)
        m_models.PopBack();
    m_models.Insert(m_models.Begin(), 1, RefPointer<RecentModelControl>(control));
    RemoveAllControls();
    for (ModelControlArray::ConstIterator it = m_models.Begin(); it != m_models.End(); ++it)
        AddControl(it->Get());
}

// ===========================================================================================
// RecentModelsControl::RecentModelControl
// ===========================================================================================

// Mac 0x380d30, Win 0x67b05d
RecentModelsControl::RecentModelControl::RecentModelControl(ControlManager* manager)
    : RenderControl(manager), ControlObserver(), m_storageControl(), m_path(String(), 0)
{
}

// Mac 0x381820 / 0x381a40 (body 0x3816b0), Win sdd 0x67b3da
RecentModelsControl::RecentModelControl::~RecentModelControl()
{
}

// Mac 0x381cb0 (thunks 0x381d20, 0x381d90, 0x381e00, 0x381e70), Win 0x67b448
void RecentModelsControl::RecentModelControl::OnFinalRelease()
{
    for (UInt32 i = 0; i < m_subjects.GetSize(); ++i)
        static_cast<NotifierT<Control, ControlObserver>*>(m_subjects[i])->RemoveObserver(this);
    m_subjects.Clear();
    RenderControl::OnFinalRelease();
}

// Mac 0x380ec0, Win 0x67b46c
bool RecentModelsControl::RecentModelControl::Create(RecentModelsControl* owner, const Path& path)
{
    if (!RenderControl::Initialize())
        return false;
    SetParent(owner, 0);
    SetActive(true);
    SetEnabled(true);
    SetShown(true);
    m_path.Assign(path);
    m_storageControl = new StorageSceneControl(GetManager(), owner->GetSceneManager());
    if (!m_storageControl->SetPath(path))
        return false;
    StorageSceneControl* thumbnail = m_storageControl.Get();
    thumbnail->SetParent(this, 0);
    thumbnail->SetActive(true);
    thumbnail->SetEnabled(true);
    thumbnail->SetShown(true);
    thumbnail->SetMargins(0.0f, 0.0f, 0.0f, 20.0f, 0);
    thumbnail->SetInputFlags(thumbnail->GetInputFlags() | 1);
    thumbnail->SetCursor(2, true);
    thumbnail->SetClearColor(Color(1.0f, 1.0f, 1.0f, 0.0f));
    thumbnail->SetBufferOptions(2, 1, Vector2(2.0f, 1.0f));
    thumbnail->SetBuffered(true);
    ControlObserver::Attach(thumbnail);
    SetInputFlags(GetInputFlags() | 1);
    SetCursor(2, true);
    SetImage(owner->GetTexture(), &s_imageRectNormal);
    SetFrame(Vector3(9.0f, 0.0f, 9.0f), Vector3(9.0f, 0.0f, 9.0f), true);
    SetDrawAsFrame(true);
    SetFont(LDDApplication::GetInstance()->GetFont(0));
    SetTextAlignment(2, 1);
    SetVerticalTextOffset(-3.0f);
    m_textLayout->SetTruncate(true);
    SetText(path.GetPath(false));
    return true;
}

// Mac 0x381420, Win 0x67ae12
void RecentModelsControl::RecentModelControl::OnRectangleChanged(const Vector2& oldPosition,
                                                                 const Vector2& oldSize)
{
    if (m_textLayout) {
        const Real32 width = GetSize().x - 6.0f;
        if (width > 0.0f && Abs(width) > 1e-05f)
            m_textLayout->SetMaxWidth((UInt32)(Int32)floorf(GetSize().x) - 6);
    }
    RenderControl::OnRectangleChanged(oldPosition, oldSize);
}

// Mac 0x3814a0, Win 0x67ad14
void RecentModelsControl::RecentModelControl::OnMouseEnter(const Vector2& position)
{
    SetImageRect(&s_imageRectHover);
    RenderControl::OnMouseEnter(position);
}

// Mac 0x3814d0, Win 0x67ad30
void RecentModelsControl::RecentModelControl::OnMouseLeave(const Vector2& position)
{
    SetImageRect(&s_imageRectNormal);
    RenderControl::OnMouseLeave(position);
}

// (inlined in Mac 0x381500 / 0x381610, Win 0x67ae90 / 0x67aed4)
void RecentModelsControl::RecentModelControl::OpenModel()
{
    StorageScene2* model = m_storageControl->GetModel();
    if (model)
        LDDApplication::GetInstance()->OpenDocument(m_path, model);
}

// Mac 0x381500, Win 0x67ae90
void RecentModelsControl::RecentModelControl::OnMouseClicked(const Vector2& position, UInt32 button)
{
    if (button == 0)
        OpenModel();
    RenderControl::OnMouseClicked(position, button);
}

// Mac 0x381550 (thunk 0x381580), Win 0x67ad4c
void RecentModelsControl::RecentModelControl::OnMouseEnter(Control*, const Vector2&)
{
    SetImageRect(&s_imageRectHover);
}

// Mac 0x3815b0 (thunk 0x3815e0), Win 0x67ad5f
void RecentModelsControl::RecentModelControl::OnMouseLeave(Control*, const Vector2&)
{
    SetImageRect(&s_imageRectNormal);
}

// Mac 0x381610 (thunk FUN 0x381660), Win 0x67aed4
void RecentModelsControl::RecentModelControl::OnMouseClicked(Control*, const Vector2&, UInt32 button)
{
    if (button == 0)
        OpenModel();
}

} // namespace LEGO
