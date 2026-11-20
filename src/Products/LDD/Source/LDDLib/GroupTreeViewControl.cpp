// LEGO Digital Designer — restored source
// GroupTreeViewControl.cpp — TU 266 (Mac 0x27ba80-0x28244b; Win 0x677035-0x679dc8):
// GroupTreeViewControl (+ Branch, MyBrickGroupControl, CollapseGroupButtonControl), the group
// buttons and their commands. See GroupTreeViewControl.h.
#include "GroupTreeViewControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlTool.h"
#include "BrickGroup.h"
#include "Scene.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "TextLocale.h"
#include "Color.h"
#include "Geometry.h"
#include <math.h>

namespace LEGO {

namespace {

// The confirmation of the ungroup button: Alert style 3, YES = 0. (helper of this restoration;
// the original inlines it, Mac 0x27f8c0 / Win 0x677561)
UInt32 AskYesNo(const char* message, const char* title)
{
    return LDDApplication::GetInstance()->Alert(3, LocalizedString(String(message)),
                                                LocalizedString(String(title)),
                                                LocalizedString(String("YES")),
                                                LocalizedString(String("NO")), String());
}

} // namespace

// ===========================================================================================
// The commands
// ===========================================================================================

// Mac 0x280310
AddSubGroupCommand::AddSubGroupCommand(BrickConnectSceneControl* sceneControl, BrickGroupControl* groupControl)
    : Command(false, false), m_groupControl(groupControl)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
}

// Mac 0x2807c0 (thunks 0x2809e0, 0x280b30)
void AddSubGroupCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::Detach();
    Command::OnFinalRelease();
}

// Mac 0x280850, Win 0x678370
void AddSubGroupCommand::Do()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (control)
        control->CreateGroup(m_groupControl->GetGroup(), control->GetSelection());
}

// Mac 0x280890
void AddSubGroupCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    SetEnable(control ? !control->GetSelection().IsEmpty() : false);
}

// Mac 0x2808d0 (thunk 0x280a00)
void AddSubGroupCommand::OnAttached(BrickConnectSceneControl*)
{
    UpdateEnable();
}

// Mac 0x2808e0 (thunk 0x280a20)
void AddSubGroupCommand::OnEvent14(bool selectionEmpty)
{
    SetEnable(!selectionEmpty);
}

// Mac 0x280c50
AddPartsToGroupCommand::AddPartsToGroupCommand(BrickConnectSceneControl* sceneControl, BrickGroupControl* groupControl)
    : Command(false, false), m_groupControl(groupControl)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
}

// Mac 0x281100
void AddPartsToGroupCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::Detach();
    Command::OnFinalRelease();
}

// Mac 0x281190, Win 0x6784da
void AddPartsToGroupCommand::Do()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (control)
        control->AddPartsToGroup(m_groupControl->GetGroup(), control->GetSelection());
}

// Mac 0x2811d0
void AddPartsToGroupCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    SetEnable(control ? !control->GetSelection().IsEmpty() : false);
}

// Mac 0x281210
void AddPartsToGroupCommand::OnAttached(BrickConnectSceneControl*)
{
    UpdateEnable();
}

// Mac 0x281220
void AddPartsToGroupCommand::OnEvent14(bool selectionEmpty)
{
    SetEnable(!selectionEmpty);
}

// Mac 0x281590
MovePartsToParentGroupCommand::MovePartsToParentGroupCommand(BrickConnectSceneControl* sceneControl,
                                                             BrickGroupControl* groupControl)
    : Command(false, false), m_groupControl(groupControl)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
}

// Mac 0x281a40
void MovePartsToParentGroupCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::Detach();
    Command::OnFinalRelease();
}

// Mac 0x281ad0, Win 0x678644
void MovePartsToParentGroupCommand::Do()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (control)
        control->RemovePartsFromGroup(m_groupControl->GetGroup(), control->GetSelection());
}

// Mac 0x281b10
void MovePartsToParentGroupCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    SetEnable(control ? !control->GetSelection().IsEmpty() : false);
}

// Mac 0x281b50
void MovePartsToParentGroupCommand::OnAttached(BrickConnectSceneControl*)
{
    UpdateEnable();
}

// Mac 0x281b60
void MovePartsToParentGroupCommand::OnEvent14(bool selectionEmpty)
{
    SetEnable(!selectionEmpty);
}

// ===========================================================================================
// The buttons
// ===========================================================================================

// Mac 0x27ec10, Win 0x678717
AddSubGroupButtonControl::AddSubGroupButtonControl(BrickConnectSceneManager* sceneManager,
                                                   ControlManager* manager,
                                                   BrickGroupControl* groupControl)
    : CommandButtonControl(manager), m_ownCommand()
{
    m_ownCommand = new AddSubGroupCommand(sceneManager->GetSceneControl(), groupControl);
    CommandObserver::Attach(m_ownCommand.Get());
}

// Mac 0x280460 / 0x280530
AddSubGroupButtonControl::~AddSubGroupButtonControl()
{
}

// Mac 0x27ed90, Win 0x678824
AddPartsToGroupButtonControl::AddPartsToGroupButtonControl(BrickConnectSceneManager* sceneManager,
                                                           ControlManager* manager,
                                                           BrickGroupControl* groupControl)
    : CommandButtonControl(manager), m_ownCommand()
{
    m_ownCommand = new AddPartsToGroupCommand(sceneManager->GetSceneControl(), groupControl);
    CommandObserver::Attach(m_ownCommand.Get());
}

// Mac 0x280da0 / 0x280e70
AddPartsToGroupButtonControl::~AddPartsToGroupButtonControl()
{
}

// Mac 0x27ef10, Win 0x6789a0
MovePartsToParentGroupButtonControl::MovePartsToParentGroupButtonControl(BrickConnectSceneManager* sceneManager,
                                                                         ControlManager* manager,
                                                                         BrickGroupControl* groupControl)
    : CommandButtonControl(manager), m_ownCommand()
{
    m_ownCommand = new MovePartsToParentGroupCommand(sceneManager->GetSceneControl(), groupControl);
    CommandObserver::Attach(m_ownCommand.Get());
}

// Mac 0x2816e0 / 0x2817b0
MovePartsToParentGroupButtonControl::~MovePartsToParentGroupButtonControl()
{
}

// Mac 0x27eb40
UnGroupButtonControl::UnGroupButtonControl(BrickConnectSceneManager* sceneManager, ControlManager* manager,
                                           BrickGroupControl* groupControl)
    : ButtonControl(manager), m_sceneManager(sceneManager), m_groupControl(groupControl)
{
}

// Mac 0x27fb00 / 0x27fbc0, Win sdd 0x6782a3
UnGroupButtonControl::~UnGroupButtonControl()
{
}

// Mac 0x27f8c0, Win 0x677561
void UnGroupButtonControl::OnMouseClicked(const Vector2&, UInt32)
{
    if (AskYesNo("DOYOUWANTTODELETEGROUP", "DOYOUWANTTODELETEGROUPTITLE") == 0)
        m_sceneManager->GetSceneControl()->UnGroup(m_groupControl->GetGroup());
}

// Mac 0x27f7c0, Win 0x6777e8
void GroupTreeViewControl::CollapseGroupButtonControl::OnMouseClicked(const Vector2&, UInt32)
{
    m_branch->m_collapsed = !m_branch->m_collapsed;
    if (m_branch->m_collapsed)
        m_branch->m_collapse->SetImages("LDDgraphics01.disclosurebutton.open");
    else
        m_branch->m_collapse->SetImages("LDDgraphics01.disclosurebutton.closed");
    m_tree->LayoutTree();
}

// ===========================================================================================
// GroupTreeViewControl
// ===========================================================================================

// Mac 0x27bb10, Win 0x6790ed
GroupTreeViewControl::GroupTreeViewControl(ControlManager* manager, BrickConnectSceneManager* sceneManager)
    : RenderControl(manager), m_sceneManager(sceneManager), m_branches(), m_branchMap(), m_inLayout(false)
{
    BrickConnectSceneControl* sceneControl = sceneManager->GetSceneControl();
    SceneObserver::Attach(sceneControl->GetScene());
    BrickConnectSceneControlObserver::Attach(sceneControl);
    SetEnabled(true);
    SetShown(true);
    SetActive(true);
}

// Mac 0x27c310 / 0x27c6f0 (body 0x27bf70), Win sdd 0x679d3a
GroupTreeViewControl::~GroupTreeViewControl()
{
}

// Mac 0x27cb60 (thunks 0x27cd00-0x27cde0)
void GroupTreeViewControl::OnFinalRelease()
{
    BrickConnectSceneControlObserver::Detach();
    for (UInt32 i = 0; i < SliderControlObserver::m_subjects.GetSize(); ++i)
        static_cast<NotifierT<SliderControl, SliderControlObserver>*>(SliderControlObserver::m_subjects[i])
            ->RemoveObserver(static_cast<SliderControlObserver*>(this));
    SliderControlObserver::m_subjects.Clear();
    for (UInt32 i = 0; i < ControlObserver::m_subjects.GetSize(); ++i)
        static_cast<NotifierT<Control, ControlObserver>*>(ControlObserver::m_subjects[i])
            ->RemoveObserver(static_cast<ControlObserver*>(this));
    ControlObserver::m_subjects.Clear();
    while (!BricksControlItemPressedNotifier::m_observers.IsEmpty())
        BricksControlItemPressedNotifier::m_observers.Back()->Detach(
            static_cast<BricksControlItemPressedNotifier*>(this));
    SceneObserver::Detach();
    RenderControl::OnFinalRelease();
}

// Mac 0x27bf10, Win 0x6771ff
bool GroupTreeViewControl::Initialize()
{
    if (!RenderControl::Initialize())
        return false;
    SetTransparent(true);
    return true;
}

// (inlined in Mac 0x27ce00, 0x27d2c0, 0x27f7c0)
void GroupTreeViewControl::LayoutTree()
{
    Real32 y = 0.0f;
    m_inLayout = true;
    Layout(m_branches, 0, 0.0f, y, false);
    m_inLayout = false;
}

// Mac 0x27ce00, Win 0x6778b0
void GroupTreeViewControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    if (!m_inLayout)
        LayoutTree();
    RenderControl::OnRectangleChanged(oldPosition, oldSize);
}

// Mac 0x27f090
void GroupTreeViewControl::Layout(const BranchArray& branches, UInt32 depth, Real32 x, Real32& y, bool hidden)
{
    const Real32 childX = x + 40.0f;
    for (BranchArray::ConstIterator it = branches.Begin(); it != branches.End(); ++it) {
        Branch* branch = it->Get();
        if (hidden) {
            branch->m_box->SetShown(false);
            branch->m_box->SetParent(0, 0);
            branch->m_collapse->SetShown(false);
            branch->m_collapse->SetParent(0, 0);
            Layout(branch->m_children, depth + 1, childX, y, true);
            continue;
        }
        branch->m_box->SetShown(true);
        branch->m_box->SetParent(this, this);
        branch->m_collapse->SetShown(!branch->m_children.IsEmpty());
        branch->m_collapse->SetParent(this, this);
        branch->m_box->SetPosition(Vector2(branch->m_collapse->GetSize().x + x, y));
        branch->m_collapse->SetPosition(
            Vector2(x, (branch->m_box->GetSize().y * 0.5f + y) - branch->m_collapse->GetSize().y * 0.5f));
        branch->m_collapse->SetZOrder(0);
        y = branch->m_box->GetSize().y + 2.0f + y;
        Layout(branch->m_children, depth + 1, childX, y, branch->m_collapsed);
    }
    FitToChildren(true, true, true, true);
}

// Mac 0x27ce50, Win 0x677750
void GroupTreeViewControl::Draw(ControlDrawArgs& args)
{
    RenderControl::Draw(args);
    DrawLines(0, m_branches, 0, args);
}

// Mac 0x27ce90, Win 0x677397
void GroupTreeViewControl::DrawLines(const Branch* parent, const BranchArray& branches, UInt32 depth,
                                     ControlDrawArgs& args)
{
    Real32 parentY = 0.0f;
    if (parent) {
        const Control* button = parent->m_collapse.Get();
        parentY = floorf(button->GetSize().y * 0.5f + button->GetPosition().y);
    }
    for (BranchArray::ConstIterator it = branches.Begin(); it != branches.End(); ++it) {
        const Branch* branch = it->Get();
        if (parent) {
            const Control* parentButton = parent->m_collapse.Get();
            const Vector2 from(floorf(parentButton->GetSize().x * 0.5f + parentButton->GetPosition().x), parentY);
            const Control* button = branch->m_collapse.Get();
            Vector2 to(floorf(button->GetSize().x * 0.5f + button->GetPosition().x),
                       floorf(button->GetSize().y * 0.5f + button->GetPosition().y));
            if (branch->m_children.IsEmpty())
                to.x = button->GetSize().x * 0.5f + to.x;
            args.DrawLine(Vector2(from.x, from.y), Vector2(from.x, to.y), Color::Black(), false);
            args.DrawLine(Vector2(from.x, to.y), Vector2(to.x, to.y), Color::Black(), false);
        }
        if (!branch->m_collapsed)
            DrawLines(branch, branch->m_children, depth + 1, args);
    }
}

// Mac 0x27d140 (thunk 0x27d290), Win 0x677a6f
void GroupTreeViewControl::OnMouseClicked(Control* control, const Vector2&, UInt32 button)
{
    if (button != 0)
        return;
    MyBrickGroupControl* groupControl = dynamic_cast<MyBrickGroupControl*>(control);
    if (!groupControl || !m_sceneManager->GetSceneControl())
        return;
    BasicArray<RefPointer<Part> > parts;
    groupControl->GetGroup()->GetAllParts(parts);
    // NOTE: the active tool is not checked for null (both builds).
    m_sceneManager->GetActiveTool()->OnToolEvent24(parts);
}

// Mac 0x27d9d0 (thunk 0x27da60)
void GroupTreeViewControl::OnEvent21(UInt32 cursor, bool flag)
{
    if (!flag)
        cursor = 1;
    SetBranchCursor(m_branches, cursor);
}

// Mac 0x27da00, Win 0x6778d4
void GroupTreeViewControl::SetBranchCursor(const BranchArray& branches, UInt32 cursor)
{
    for (BranchArray::ConstIterator it = branches.Begin(); it != branches.End(); ++it) {
        (*it)->m_groupControl->SetCursor(cursor, true);
        SetBranchCursor((*it)->m_children, cursor);
    }
}

// Mac 0x27d820 (thunk 0x27d8a0), Win 0x677035
void GroupTreeViewControl::OnAttached(SliderControl* slider)
{
    const Real32 x = slider->GetValue() / 100.0f;
    SetScale(Vector2(x, slider->GetValue() / 100.0f));
}

// Mac 0x27d930 (thunk 0x27d980), Win 0x677087
void GroupTreeViewControl::OnValueChanged(SliderControl*, Real32 value, Real32)
{
    SetScale(Vector2(value / 100.0f, value / 100.0f));
}

// Mac 0x27d2c0 (thunk 0x27d7f0), Win 0x679d5c
void GroupTreeViewControl::OnGroupsChanged(const BasicArray<RefPointer<BrickGroup> >& added,
                                           const BasicArray<RefPointer<BrickGroup> >& changed,
                                           const BasicArray<RefPointer<BrickGroup> >&,
                                           const BasicArray<RefPointer<BrickGroup> >& removed)
{
    for (BasicArray<RefPointer<BrickGroup> >::ConstIterator it = removed.Begin(); it != removed.End(); ++it)
        m_branchMap.Erase(it->Get());
    UInt32 count = 0;
    const BasicArray<RefPointer<BrickGroup> >& groups = SceneObserver::GetSubject()->GetGroups();
    Rebuild(m_branches, 0, groups.Begin(), groups.End(), added, changed, count);
    LayoutTree();
}

// Mac 0x27d4d0, Win 0x679bd0
bool GroupTreeViewControl::Rebuild(BranchArray& branches, Branch* parent,
                                   BasicArray<RefPointer<BrickGroup> >::ConstIterator begin,
                                   BasicArray<RefPointer<BrickGroup> >::ConstIterator end,
                                   const BasicArray<RefPointer<BrickGroup> >& added,
                                   const BasicArray<RefPointer<BrickGroup> >& changed, UInt32& count)
{
    branches.Clear();
    bool result = false;
    for (; begin != end; ++begin) {
        BrickGroup* group = begin->Get();
        Branch* branch;
        bool existing = false;
        bool inChanged = false;
        BranchMap::Iterator found = m_branchMap.Find(group);
        if (found == m_branchMap.End()) {
            // NOTE: a null branch (a button failed) is dereferenced (both builds).
            branch = CreateBranch(group);
        } else {
            branch = found->second.Get();
            existing = true;
            for (BasicArray<RefPointer<BrickGroup> >::ConstIterator c = changed.Begin(); c != changed.End(); ++c)
                if (c->Get() == group) {
                    inChanged = true;
                    break;
                }
        }
        branch->m_parent = parent;
        branches.PushBack(RefPointer<Branch>(branch));
        ++count;
        const bool subtreeChanged = Rebuild(branch->m_children, branch, group->GetGroups().Begin(),
                                            group->GetGroups().End(), added, changed, count);
        const bool update = subtreeChanged ? existing : inChanged;
        if (update) {
            branch->m_groupControl->ClearModel();
            result = true;
        }
    }
    return result;
}

// The failure exit of CreateBranch (not in the original; see there).
GroupTreeViewControl::Branch* GroupTreeViewControl::DiscardBranch(Branch* branch)
{
    RefPointer<Branch> release = branch;
    if (!branch->m_groupControl.IsNull())
        ControlObserver::Detach(branch->m_groupControl.Get());
    if (!branch->m_box.IsNull())
        branch->m_box->SetParent(0);
    return 0;
}

// Mac 0x27dd10, Win 0x6793b4
GroupTreeViewControl::Branch* GroupTreeViewControl::CreateBranch(BrickGroup* group)
{
    // NOTE: on failure the branch and its controls are leaked (never referenced), both builds;
    // fixed: the box leaves this control, the thumbnail is no longer observed and the branch is
    // released (low).
    Branch* branch = new Branch();
    branch->m_box = new RenderControl(GetManager());
    branch->m_box->Initialize();
    branch->m_box->Setup(this, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    branch->m_box->SetImage(String("LDDgraphics01.groupbox"));

    branch->m_groupControl = new MyBrickGroupControl(GetManager(), m_sceneManager, branch);
    MyBrickGroupControl* thumbnail = branch->m_groupControl.Get();
    thumbnail->Create(group, ModelControl::GetDefaultCamera());
    thumbnail->Setup(branch->m_box.Get(), 0, 7, Rect(Vector2(58.0f, 12.0f), Vector2(99.0f, 98.0f)), 0, false);
    thumbnail->SetInputFlags(thumbnail->GetInputFlags() & ~(UInt32)RenderControl::kInputInitialized);
    thumbnail->SetClearColor(Color::Clear());
    thumbnail->SetBufferOptions(2, 1, Vector2(1.0f, 1.0f));
    thumbnail->SetBuffered(true);
    ControlObserver::Attach(thumbnail);

    branch->m_unGroup = new UnGroupButtonControl(m_sceneManager, GetManager(), thumbnail);
    if (!branch->m_unGroup->Create("LDDgraphics01.deletebutton", branch->m_box.Get(), 0, 7,
                                   Rect(Vector2(153.0f, 5.0f), Vector2()), 1, true))
        return DiscardBranch(branch);

    branch->m_addSubGroup = new AddSubGroupButtonControl(m_sceneManager, GetManager(), thumbnail);
    if (!branch->m_addSubGroup->Create("IconGeneral.createsubgroupbutton", branch->m_box.Get(), 0, 7,
                                       Rect(Vector2(12.0f, 85.0f), Vector2()), 1, true))
        return DiscardBranch(branch);
    CreateExtendedTextTooltip(branch->m_addSubGroup.Get(), LocalizedString(String("CREATESUBGROUPTOOLTIP")),
                              LocalizedString(String("CREATESUBGROUPTOOLTIPEXTENDED")));

    branch->m_addParts = new AddPartsToGroupButtonControl(m_sceneManager, GetManager(), thumbnail);
    if (!branch->m_addParts->Create("IconGeneral.addbricksbutton", branch->m_box.Get(), 0, 7,
                                    Rect(Vector2(7.0f, 3.0f), Vector2()), 1, true))
        return DiscardBranch(branch);
    CreateExtendedTextTooltip(branch->m_addParts.Get(), LocalizedString(String("ADDTOGROUPTOOLTIP")),
                              LocalizedString(String("ADDTOGROUPTOOLTIPEXTENDED")));

    branch->m_moveParts = new MovePartsToParentGroupButtonControl(m_sceneManager, GetManager(), thumbnail);
    if (!branch->m_moveParts->Create("IconGeneral.removebricksbutton", branch->m_box.Get(), 0, 7,
                                     Rect(Vector2(7.0f, 44.0f), Vector2()), 1, true))
        return DiscardBranch(branch);
    CreateExtendedTextTooltip(branch->m_moveParts.Get(), LocalizedString(String("REMOVEFROMGROUPTOOLTIP")),
                              LocalizedString(String("REMOVEFROMGROUPTOOLTIPEXTENDED")));

    branch->m_collapse = new CollapseGroupButtonControl(GetManager(), this, branch);
    if (!branch->m_collapse->Create("LDDgraphics01.disclosurebutton.closed", this, 0, 7,
                                    Rect(Vector2(), Vector2()), 1, true))
        return DiscardBranch(branch);

    m_branchMap[group] = branch;
    return branch;
}

} // namespace LEGO
