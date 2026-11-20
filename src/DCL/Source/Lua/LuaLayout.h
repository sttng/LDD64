// LEGO Digital Designer — restored source
// LEGO::LuaLayout — a BindableLuaObject built from XML layouts (Lua class "Layout", base Object):
// base of LuaControl and of AdapterBindableLuaObjectBase<Window, LuaLayout>.
// TU 321 "LuaLayout.cpp" (Mac 0x2cb150-0x2cd5cf; Win 0x468330-0x469ee0); directory: see
// BindableLuaObject.h.
//
// Layouts are the children of the script manager's XMLDOM (one element per layout, from
// Scripts/*.xml). ApplyLayout of a node to a bound table:
//   1. `scope="name"` makes the table reachable as `name` from every node applied below it;
//   2. `layout="a, b.c"` applies the named layouts first ("b.c" = child c of layout b, searched
//      below the current node, then from the root; ".x" = root only);
//   3. OnLayoutApplied(node, args) (overridden by the controls); the base runs <OnLoad> once per
//      object and ApplyLayout call;
//   4. <OnPostLoad> texts run after the whole tree was applied.
// Children: CreateChild (class from `class=` or an inherited layout, _G[class]:New(parent,
// name)). OnLoad / OnPostLoad run as "local self, onLoadContext = ...\n<text>", onLoadContext
// being one table per ApplyLayout call.
//
// RTTI si class (base BindableLuaObject). Mac vtable 0x73f3cc {D1 0x2cc650, D0 0x2cc690,
// BindLuaObject 0x291a70, UnbindLuaObject = pure, OnLayoutApplied 0x2cc720}; layout
// BindableLuaObject +0x00..+0x08, bool +0x08, Object +0x0c (LuaControl has its
// ControlAugmentation base at +0x0c). Win vtable 0xa233b4 {0x40be80, _purecall, 0x4693d0},
// Object-in 0xa233c4 {sdd 0x4687e0, …}; Win: vfptr, vbptr +4, state +8, bool +0xc, Object +0x10.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "BindableLuaObject.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BasicString.h"
#include "LEGO/String.h"
#include "LuaPlus.h"

namespace LEGO {

class XMLDOMNode;
class LuaLayout;

// State of one ApplyLayout call (a stack object of LuaLayout::ApplyLayout, passed by reference
// through the recursion and to OnLayoutApplied). Mac sizeof 0x58. (name inferred)
struct LuaLayoutArgs {
    explicit LuaLayoutArgs(const XMLDOMNode* root) : m_root(root), m_table(0), m_callOnLoad(false), m_layout(0) {}

    typedef Pair<AString, LuaPlus::LuaObject*>                ScopeEntry;
    typedef Pair<LuaPlus::LuaObject, const XMLDOMNode*>       PostLoadEntry;

    const XMLDOMNode*          m_root;            // +0x00 the layouts (script manager XMLDOM)
    LuaPlus::LuaObject*        m_table;           // +0x04 table the current node is applied to
    LuaPlus::LuaObject         m_onLoadContext;   // +0x08 shared `onLoadContext` table
    BasicArray<LuaLayout*>     m_layouts;         // +0x20 objects laid out for the first time
    BasicArray<ScopeEntry>     m_scopes;          // +0x30 active `scope` names
    BasicArray<PostLoadEntry>  m_postLoads;       // +0x40 <OnPostLoad> nodes and their tables
    bool                       m_callOnLoad;      // +0x50 run <OnLoad> for the current object
    LuaLayout*                 m_layout;          // +0x54 object of the current node

private:
    LuaLayoutArgs(const LuaLayoutArgs&);
    LuaLayoutArgs& operator=(const LuaLayoutArgs&);
};

class LuaLayout : public BindableLuaObject {
public:
    LuaLayout() : m_firstLayout(true) {}                                   // Mac 0x2cb150 (C2), Win 0x468760
    virtual ~LuaLayout() {}                                                 // Mac 0x2cc650/0x2cc690, Win sdd 0x4687e0

    // Applies one layout node to the object's table (step 3 above). The base runs the node's
    // <OnLoad> child when args.m_callOnLoad is set. (name inferred)
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac slot 4 0x2cc720, Win slot 2 0x4693d0

    // Applies `layout` (a node below `root`) to `table` (this object's table), then runs the
    // collected <OnPostLoad> texts. (name inferred)
    void ApplyLayout(LuaPlus::LuaObject& table, const XMLDOMNode* layout,
                     const XMLDOMNode* root);                                 // Mac 0x2cc300, Win 0x4690b0

    // Applies `layout` to `table` with this object as args.m_layout. (name inferred)
    void Apply(LuaPlus::LuaObject* table, const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x2cbfc0, Win 0x469030
    // Creates (or re-lays out) the child of args.m_table named like `node`; nil on failure.
    // NOTE: `this` is not used (both builds). (name inferred)
    LuaPlus::LuaObject CreateChild(const XMLDOMNode* node, LuaLayoutArgs& args); // Mac 0x2cc830, Win 0x4694d0
    // CreateChild for every child of node's child `containerName` (e.g. "Children"). (name inferred)
    void CreateChildren(const String& containerName, const XMLDOMNode* node, LuaLayoutArgs& args); // Mac 0x2cc7d0, Win 0x469480
    // Defines args.m_table:name(params) from the text of node's child `name` (OnMouseClicked …).
    // False when there is no such child. (name inferred)
    bool DefineLayoutFunction(const XMLDOMNode* node, const char* name, const char* params,
                              LuaLayoutArgs& args);                           // Mac 0x2cd120, Win 0x469960

    // Steps 1, 2, 3 and 4 for one node (recursive through `layout=`). (name inferred)
    static void ApplyLayoutNode(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x2cb620, Win 0x468bb0
    // Layout `name` ("a.b", ".a") searched from `scope`, then from `root`. (name inferred)
    static const XMLDOMNode* FindLayout(const String& name, const XMLDOMNode* scope,
                                        const XMLDOMNode* root);              // Mac 0x2cbd10, Win 0x468330
    // `class` of a layout node or of the first inherited layout that has one. (name inferred)
    static bool FindLayoutClass(const XMLDOMNode* layout, const XMLDOMNode* root, String& luaClass); // Mac 0x2cce70, Win 0x468460

protected:
    bool m_firstLayout;   // Mac +0x08, Win +0x0c: not laid out yet (cleared by the first Apply)
};

// Registers Layout {Layout, FindLayoutClass (upvalue `layouts`), New, NewFromLayout, Destroy,
// Delete}. (name inferred)
void RegisterLuaLayout(LuaPlus::LuaState* state, XMLDOMNode* layouts);    // Mac 0x2cb190, Win 0x468af0

} // namespace LEGO
