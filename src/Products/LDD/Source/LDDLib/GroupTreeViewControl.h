// LEGO Digital Designer — restored source
// LEGO::GroupTreeViewControl — the tree of the model's groups ("groupTreeViewControl"): one
//   Branch per BrickGroup (cached): a framed box ("LDDgraphics01.groupbox") with a thumbnail
//   (MyBrickGroupControl), four buttons (ungroup, new sub-group, add to group, move to parent)
//   and a disclosure button that collapses the sub-groups. Boxes stack top-down, 40 units per
//   level, joined by lines; the tree follows the scene's groups. A thumbnail click hands the
//   group's parts to the active tool; a size slider scales the control.
// LEGO::UnGroupButtonControl, LEGO::AddSubGroupButtonControl / AddSubGroupCommand,
//   LEGO::AddPartsToGroupButtonControl / AddPartsToGroupCommand,
//   LEGO::MovePartsToParentGroupButtonControl / MovePartsToParentGroupCommand — the buttons of a
//   branch; the three commands act on the scene control's selection and are enabled while it
//   is not empty.
//
// Home TU 266 "GroupTreeViewControl.cpp" (Mac 0x27ba80-0x28244b; Win 0x677035-0x679dc8).
//
// GroupTreeViewControl: RTTI vmi {RenderControl, SceneObserver (+0x1ec),
//   BricksControlItemPressedNotifier (+0x1f4), ControlObserver (+0x218), SliderControlObserver
//   (+0x22c), BrickConnectSceneControlObserver (+0x240)}. Mac layout (constructor 0x27bb10):
//   m_sceneManager +0x248, m_branches +0x24c, m_branchMap +0x25c (std::map +0x260), m_inLayout
//   +0x278, Object +0x27c (sizeof 0x284). Win: m_inLayout +0x2ac (0x6777b3), new 0x2bc (other
//   Win offsets not recorded).
//   Vtable Mac 0x73beec (50 entries: overrides OnFinalRelease 2, OnRectangleChanged 8, Draw 33,
//   Initialize 38; Mac-only overrider entries 45-49 = ControlObserver::OnMouseClicked,
//   SceneObserver::OnGroupsChanged, SliderControlObserver::OnAttached / OnValueChanged,
//   BrickConnectSceneControlObserver::OnEvent21).
// Branch: RTTI si {Object} (plain base). Mac layout (constructor inlined in 0x27dd10): box +0x08,
//   group control +0x0c, buttons +0x10..+0x20, parent +0x24, children +0x28, collapsed +0x38
//   (new 0x3c). Vtable Mac 0x73c264 {D1 0x27dcc0, D0 0x27dcd0, Object slots}.
// Class names from RTTI; method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "RenderControl.h"
#include "ButtonControl.h"
#include "SliderControl.h"
#include "Command.h"
#include "Scene.h"
#include "BrickConnectSceneControlObserver.h"
#include "BricksControlItemPressedNotifier.h"
#include "CommandButtonControl.h"
#include "BricksControl.h"

namespace LEGO {

class BrickConnectSceneManager;
class BrickConnectSceneControl;
class BrickGroup;
class Part;

// ===========================================================================================
// The commands of the group buttons. RTTI vmi {Command, BrickConnectSceneControlObserver
// (+0x28)}. Mac layout: Command +0x00..+0x28, observer +0x28 (subject +0x2c), m_groupControl
// +0x30, Object +0x34 (new 0x3c). Mac vtables: AddSubGroupCommand 0x772bc8, AddPartsToGroup…
// 0x773ab8, MovePartsToParentGroup… 0x774828 (9 entries: Command's 7 with OnFinalRelease, Do,
// UpdateEnable; Mac-only overrider entries 7 / 8 = OnAttached, OnEvent14 of the observer).
// Constructed disabled; enabled while the scene control has a selection.
// ===========================================================================================
class AddSubGroupCommand : public Command, public BrickConnectSceneControlObserver {
public:
    // Mac 0x280310, Win (inlined in 0x678717)
    AddSubGroupCommand(BrickConnectSceneControl* sceneControl, BrickGroupControl* groupControl);
    virtual ~AddSubGroupCommand() {}                                        // Mac 0x2806f0/0x280750, Win sdd 0x678410
    // A new sub-group of the group made of the selection. Mac 0x280850, Win 0x678370
    virtual void Do();
    // Enabled while the selection is not empty. Mac 0x280890
    virtual void UpdateEnable();
    // ---- BrickConnectSceneControlObserver (Mac-only primary entries 7 / 8) ----
    virtual void OnAttached(BrickConnectSceneControl* control);             // Mac 0x2808d0 (thunk 0x280a00): UpdateEnable
    virtual void OnEvent14(bool selectionEmpty);                            // Mac 0x2808e0 (thunk 0x280a20): SetEnable(!empty)
protected:
    using Command::OnFinalRelease;
    // Leaves the scene control, then the command's observers. Mac 0x2807c0 (thunks 0x2809e0, 0x280b30)
    virtual void OnFinalRelease();
    RefPointer<BrickGroupControl> m_groupControl;   // Mac +0x30
};

class AddPartsToGroupCommand : public Command, public BrickConnectSceneControlObserver {
public:
    // Mac 0x280c50, Win (inlined in 0x678824)
    AddPartsToGroupCommand(BrickConnectSceneControl* sceneControl, BrickGroupControl* groupControl);
    virtual ~AddPartsToGroupCommand() {}                                    // Mac 0x281030/0x281090, Win sdd 0x67857a
    // The selection joins the group. Mac 0x281190, Win 0x6784da
    virtual void Do();
    virtual void UpdateEnable();                                            // Mac 0x2811d0
    virtual void OnAttached(BrickConnectSceneControl* control);             // Mac 0x281210
    virtual void OnEvent14(bool selectionEmpty);                            // Mac 0x281220
protected:
    virtual void OnFinalRelease();                                          // Mac 0x281100
    RefPointer<BrickGroupControl> m_groupControl;   // Mac +0x30
};

class MovePartsToParentGroupCommand : public Command, public BrickConnectSceneControlObserver {
public:
    // Mac 0x281590, Win (inlined in 0x6789a0)
    MovePartsToParentGroupCommand(BrickConnectSceneControl* sceneControl, BrickGroupControl* groupControl);
    virtual ~MovePartsToParentGroupCommand() {}                             // Mac 0x281970/0x2819d0, Win sdd 0x6786e4
    // The selection leaves the group (back to its parent). Mac 0x281ad0, Win 0x678644
    virtual void Do();
    virtual void UpdateEnable();                                            // Mac 0x281b10
    virtual void OnAttached(BrickConnectSceneControl* control);             // Mac 0x281b50
    virtual void OnEvent14(bool selectionEmpty);                            // Mac 0x281b60
protected:
    virtual void OnFinalRelease();                                          // Mac 0x281a40
    RefPointer<BrickGroupControl> m_groupControl;   // Mac +0x30
};

// ===========================================================================================
// The command buttons. RTTI si {CommandButtonControl}. Mac layout: CommandButtonControl
// +0x000..+0x224, m_ownCommand +0x224, Object +0x228 (new 0x230). The button observes its own
// command (CommandObserver::Attach).
// ===========================================================================================
class AddSubGroupButtonControl : public CommandButtonControl {
public:
    // Mac 0x27ec10, Win 0x678717
    AddSubGroupButtonControl(BrickConnectSceneManager* sceneManager, ControlManager* manager,
                             BrickGroupControl* groupControl);
    virtual ~AddSubGroupButtonControl();                                    // Mac 0x280460/0x280530
protected:
    RefPointer<AddSubGroupCommand> m_ownCommand;   // Mac +0x224
};

class AddPartsToGroupButtonControl : public CommandButtonControl {
public:
    // Mac 0x27ed90, Win 0x678824
    AddPartsToGroupButtonControl(BrickConnectSceneManager* sceneManager, ControlManager* manager,
                                 BrickGroupControl* groupControl);
    virtual ~AddPartsToGroupButtonControl();                                // Mac 0x280da0/0x280e70
protected:
    RefPointer<AddPartsToGroupCommand> m_ownCommand;   // Mac +0x224
};

class MovePartsToParentGroupButtonControl : public CommandButtonControl {
public:
    // Mac 0x27ef10, Win 0x6789a0
    MovePartsToParentGroupButtonControl(BrickConnectSceneManager* sceneManager, ControlManager* manager,
                                        BrickGroupControl* groupControl);
    virtual ~MovePartsToParentGroupButtonControl();                         // Mac 0x2816e0/0x2817b0
protected:
    RefPointer<MovePartsToParentGroupCommand> m_ownCommand;   // Mac +0x224
};

// ===========================================================================================
// UnGroupButtonControl — RTTI si {ButtonControl}. Mac layout: ButtonControl +0x000..+0x218,
// m_sceneManager +0x218, m_groupControl +0x21c, Object +0x220 (new 0x228).
// ===========================================================================================
class UnGroupButtonControl : public ButtonControl {
public:
    // Mac 0x27eb40
    UnGroupButtonControl(BrickConnectSceneManager* sceneManager, ControlManager* manager,
                         BrickGroupControl* groupControl);
    virtual ~UnGroupButtonControl();                                        // Mac 0x27fb00/0x27fbc0, Win sdd 0x6782a3
    // "DOYOUWANTTODELETEGROUP" (title "…TITLE", YES / NO); YES: the scene control dissolves
    // the group. NOTE: any button, no base call (both builds). Mac 0x27f8c0, Win 0x677561
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);    // 21/18
protected:
    BrickConnectSceneManager*     m_sceneManager;   // Mac +0x218
    RefPointer<BrickGroupControl> m_groupControl;   // Mac +0x21c
};

// ===========================================================================================
// GroupTreeViewControl
// ===========================================================================================
class GroupTreeViewControl : public RenderControl,
                             public SceneObserver,
                             public BricksControlItemPressedNotifier,
                             public ControlObserver,
                             public SliderControlObserver,
                             public BrickConnectSceneControlObserver {
public:
    class Branch;
    class MyBrickGroupControl;
    class CollapseGroupButtonControl;
    typedef BasicArray<RefPointer<Branch> >                     BranchArray;
    typedef BasicMap<const BrickGroup*, RefPointer<Branch> >    BranchMap;

    // Observes the scene and the scene control of `sceneManager`. Mac 0x27bb10, Win 0x6790ed
    GroupTreeViewControl(ControlManager* manager, BrickConnectSceneManager* sceneManager);
    virtual ~GroupTreeViewControl();                                        // Mac 0x27c310/0x27c6f0 (body 0x27bf70), Win sdd 0x679d3a

    const BranchArray& GetBranches() const { return m_branches; }
    const BranchMap&   GetBranchMap() const { return m_branchMap; }

    // ---- RenderControl overrides (Mac slot / Win slot) ----
    // Lays out the branches first (unless the layout itself moved the control).
    // Mac 0x27ce00, Win 0x6778b0
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5
    // Then the connecting lines. Mac 0x27ce50, Win 0x677750
    virtual void Draw(ControlDrawArgs& args);                               // 33/30
    // Transparent. Mac 0x27bf10, Win 0x6771ff
    virtual bool Initialize();                                              // 38/35

    // ---- observer overriders (Mac-only primary entries 45-49) ----
    // A thumbnail clicked (left): the active tool gets the parts of its group.
    // Mac 0x27d140 (thunk 0x27d290), Win 0x677a6f
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button);
    // The branches of removed groups are dropped, the tree rebuilt from the scene's groups
    // (changed groups get new thumbnails) and laid out. Mac 0x27d2c0 (thunk 0x27d7f0), Win 0x679d5c
    virtual void OnGroupsChanged(const BasicArray<RefPointer<BrickGroup> >& added,
                                 const BasicArray<RefPointer<BrickGroup> >& changed,
                                 const BasicArray<RefPointer<BrickGroup> >& moved,
                                 const BasicArray<RefPointer<BrickGroup> >& removed);
    // Scale = value / 100. Mac 0x27d820 (thunk 0x27d8a0), Win 0x677035
    virtual void OnAttached(SliderControl* slider);
    // Mac 0x27d930 (thunk 0x27d980), Win 0x677087
    virtual void OnValueChanged(SliderControl* slider, Real32 value, Real32 oldValue);
    // The tool cursor (1 without `flag`) for every thumbnail. Mac 0x27d9d0 (thunk 0x27da60)
    virtual void OnEvent21(UInt32 cursor, bool flag);

protected:
    friend class CollapseGroupButtonControl;
    using RenderControl::OnMouseClicked;           // (the overloads of the other bases stay visible)
    using ControlObserver::OnRectangleChanged;
    using SliderControlObserver::OnAttached;
    using SceneObserver::OnAttached;
    using ControlObserver::OnAttached;
    using BrickConnectSceneControlObserver::OnAttached;
    using Observer<TextLayout, TextLayoutObserver>::OnAttached;
    using Observer<Texture, TextureObserver>::OnAttached;

    // Leaves the scene control, the sliders, the thumbnails, the item observers and the scene,
    // then RenderControl's. Mac 0x27cb60 (thunks 0x27cd00 …)
    virtual void OnFinalRelease();

    // The branch of `group`: box, thumbnail, buttons, disclosure button (cached in the map);
    // null when a button cannot be created. Mac 0x27dd10, Win 0x6793b4 (name inferred)
    Branch* CreateBranch(BrickGroup* group);
    // Releases a branch CreateBranch could not complete; returns null. (not in the original)
    Branch* DiscardBranch(Branch* branch);
    // `branches` := the branches of the groups [begin, end) below `parent` (made when new),
    // recursively; `count` counts them. A branch whose group is in `changed` or whose subtree
    // changed gets a new thumbnail (ClearModel); true when one did. `added` is not used.
    // Mac 0x27d4d0, Win 0x679bd0 (name inferred)
    bool Rebuild(BranchArray& branches, Branch* parent,
                 BasicArray<RefPointer<BrickGroup> >::ConstIterator begin,
                 BasicArray<RefPointer<BrickGroup> >::ConstIterator end,
                 const BasicArray<RefPointer<BrickGroup> >& added,
                 const BasicArray<RefPointer<BrickGroup> >& changed, UInt32& count);
    // Stacks the boxes of `branches` from `y` down (2 units apart; disclosure button at `x`,
    // box right of it; sub-branches 40 units further right); hidden branches (collapsed parent)
    // leave the tree. Then the control fits its children. Mac 0x27f090 (name inferred)
    void Layout(const BranchArray& branches, UInt32 depth, Real32 x, Real32& y, bool hidden);
    // Layout of the whole tree from the top (m_inLayout set meanwhile). (inlined, name inferred)
    void LayoutTree();
    // The lines from the disclosure button of `parent` to each branch of `branches` (to the
    // branch's own button, or past it when the branch has no children). Mac 0x27ce90,
    // Win 0x677397 (name inferred)
    void DrawLines(const Branch* parent, const BranchArray& branches, UInt32 depth, ControlDrawArgs& args);
    // SetCursor(cursor) on every thumbnail. Mac 0x27da00, Win 0x6778d4 (name inferred)
    void SetBranchCursor(const BranchArray& branches, UInt32 cursor);

    // ---- data (Mac offsets) ----
    BrickConnectSceneManager* m_sceneManager;   // +0x248
    BranchArray               m_branches;       // +0x24c top-level branches (vtable Mac 0x774b8c)
    BranchMap                 m_branchMap;      // +0x25c every branch by group (vtable Mac 0x774ba8)
    bool                      m_inLayout;       // +0x278
                                                // Object +0x27c
};

// ---- GroupTreeViewControl::MyBrickGroupControl — the thumbnail of a branch. RTTI si
// {BrickGroupControl}; Mac m_branch +0x23c, Object +0x240 (new 0x248). Vtable Mac 0x73c284.
class GroupTreeViewControl::MyBrickGroupControl : public BrickGroupControl {
public:
    // (inlined in Mac 0x27dd10)
    MyBrickGroupControl(ControlManager* manager, BrickConnectSceneManager* sceneManager, Branch* branch)
        : BrickGroupControl(manager, sceneManager), m_branch(branch) {}
    virtual ~MyBrickGroupControl() {}                                       // Mac 0x27f330/0x27f550, Win sdd 0x679d1e
    Branch* GetBranch() const { return m_branch; }
protected:
    Branch* m_branch;   // Mac +0x23c (not referenced)
};

// ---- GroupTreeViewControl::CollapseGroupButtonControl — the disclosure button of a branch.
// RTTI si {ButtonControl}; Mac m_tree +0x218, m_branch +0x21c, Object +0x220 (new 0x228).
class GroupTreeViewControl::CollapseGroupButtonControl : public ButtonControl {
public:
    // (inlined in Mac 0x27dd10)
    CollapseGroupButtonControl(ControlManager* manager, GroupTreeViewControl* tree, Branch* branch)
        : ButtonControl(manager), m_tree(tree), m_branch(branch) {}
    virtual ~CollapseGroupButtonControl() {}                                // Mac 0x27ff70/0x27ffd0, Win sdd 0x678aad
    // Toggles the branch (image ".disclosurebutton.open" when collapsed, ".closed" when not)
    // and lays the tree out again. NOTE: any button, no base call. Mac 0x27f7c0, Win 0x6777e8
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);    // 21/18
protected:
    GroupTreeViewControl* m_tree;     // Mac +0x218
    Branch*               m_branch;   // Mac +0x21c
};

// ---- GroupTreeViewControl::Branch ----
class GroupTreeViewControl::Branch : public Object {
public:
    Branch() : m_parent(0), m_collapsed(false) {}                           // (inlined in Mac 0x27dd10)
    virtual ~Branch() {}                                                    // Mac 0x27dcc0/0x27dcd0 (body 0x27da90), Win 0x6779a5 / sdd 0x677ae2

    RefPointer<RenderControl>                       m_box;            // +0x08 "LDDgraphics01.groupbox"
    RefPointer<MyBrickGroupControl>                 m_groupControl;   // +0x0c thumbnail
    RefPointer<UnGroupButtonControl>                m_unGroup;        // +0x10
    RefPointer<AddSubGroupButtonControl>            m_addSubGroup;    // +0x14
    RefPointer<AddPartsToGroupButtonControl>        m_addParts;       // +0x18
    RefPointer<MovePartsToParentGroupButtonControl> m_moveParts;      // +0x1c
    RefPointer<CollapseGroupButtonControl>          m_collapse;       // +0x20 disclosure button
    Branch*                                         m_parent;         // +0x24 (not referenced)
    BranchArray                                     m_children;       // +0x28
    bool                                            m_collapsed;      // +0x38
};

} // namespace LEGO
