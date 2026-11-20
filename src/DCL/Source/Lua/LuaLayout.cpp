// LEGO Digital Designer — restored source
// TU 321 "LuaLayout.cpp": Mac 0x2cb150-0x2cd5cf, Win 0x468330-0x469ee0.
#include "LuaLayout.h"
#include "LuaBindings.h"
#include "XMLDOMNode.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// The Lua half of the Layout class (run by RegisterLuaLayout).
const char kLayoutScript[] =
    "function Layout:New(parent,name,layout)\n"
    "\tlocal new_inst = {}\n"
    "\tsetmetatable( new_inst, self.classMetatable )\n"
    "\tif parent and name then\n"
    "\t\tparent[name] = new_inst\n"
    "\t\tnew_inst.parent = parent\n"
    "\t\tif not parent.destroy then\n"
    "\t\t\tparent.destroy = {}\n"
    "\t\tend\n"
    "\t\tparent.destroy[name] = new_inst\n"
    "\tend\n"
    "\tnew_inst:Create(parent)\n"
    "\tif layout then\n"
    "\t\tnew_inst:Layout(layout)\n"
    "\tend\n"
    "\treturn new_inst\n"
    "end\n"
    "function Layout:NewFromLayout(parent,name,layout)\n"
    "\treturn _G[self:FindLayoutClass(layout)]:New(parent,name,layout)\n"
    "end\n"
    "local oldDestroy = Object.Destroy\n"
    "function Layout:Destroy()\n"
    "\toldDestroy(self)\n"
    "\tif self.destroy then\n"
    "\t\tfor k,v in pairs(self.destroy) do\n"
    "\t\t\tv:Destroy()\n"
    "\t\tend\n"
    "\tend\n"
    "end\n"
    "function Layout:Delete()\n"
    "\tself:Destroy()\n"
    "\tif self.parent and self.parent.destroy then\n"
    "\t\tfor k,v in pairs(self.parent.destroy) do\n"
    "\t\t\tif self == v then\n"
    "\t\t\t\tself.parent[k] = nil\n"
    "\t\t\t\tself.parent.destroy[k] = nil\n"
    "\t\t\t\tself.parent = nil\n"
    "\t\t\t\tbreak\n"
    "\t\t\tend\n"
    "\t\tend\n"
    "\tend\n"
    "end\n";

// Runs "local self, onLoadContext = ...\n<text>" with (*args.m_table, args.m_onLoadContext).
// (name inferred; Mac fastcall ECX = text, EDX = args)
void RunLayoutScript(const String& text, LuaLayoutArgs& args)              // Mac 0x2cc460, Win 0x468650
{
    LuaState* state = args.m_table->GetState();
    AString code("local self, onLoadContext = ...\n");
    code.Append(text.ToUTF8());
    // NOTE: the chunk text doubles as the chunk name (both builds).
    if (state->LoadBuffer(code.c_str(), code.GetLength(), code.c_str())) {
        args.m_table->Push();
        args.m_onLoadContext.Push();
        state->PCall(2, 0);
    }
}

// Splits a `layout` attribute at ',' and trims every name. (helper; inlined in both)
void SplitLayoutNames(const String& list, BasicArray<String>& names)
{
    list.Split((UniChar16)',', names);
    for (UInt32 i = 0; i < names.GetSize(); ++i)
        names[i].Trim();
}

// Lua self:Layout(name): applies the layout `name` to self. (name inferred)
int LuaLayoutLayout(lua_State* L)                                          // Mac 0x2cb280, Win 0x468810
{
    LuaState* state = LuaState::CastState(L);
    String name = CheckLuaString(state, 2);
    if (!name.IsEmpty()) {
        const XMLDOMNode* root = (const XMLDOMNode*)lua_touserdata(L, lua_upvalueindex(1));
        const XMLDOMNode* layout = LuaLayout::FindLayout(name, root, root);
        if (!layout) {
            // NOTE: built and discarded (both builds).
            String message("Layout failed because layout named '");
            message << name << "' does not exist.";
        } else {
            LuaLayout* object = CheckBoundObject<LuaLayout>(state, 1);    // (inline pattern)
            LuaObject self(state, 1);
            object->ApplyLayout(self, layout, root);
        }
    }
    return 0;
}

// Lua self:FindLayoutClass(name) -> class name of the layout (or nothing). (name inferred)
int LuaLayoutFindLayoutClass(lua_State* L)                                 // Mac 0x2cb450, Win 0x468930
{
    LuaState* state = LuaState::CastState(L);
    String name = CheckLuaString(state, 2);
    bool found = false;
    if (name.IsEmpty()) {
        // NOTE: the messages are built and discarded (both builds).
        String message("FindLayoutClass failed because no layout name was provided.");
    } else {
        const XMLDOMNode* root = (const XMLDOMNode*)lua_touserdata(L, lua_upvalueindex(1));
        const XMLDOMNode* layout = LuaLayout::FindLayout(name, root, root);
        if (!layout) {
            String message("FindLayoutClass failed because layout named '");
            message << name << "' does not exist.";
        } else {
            String luaClass;
            found = LuaLayout::FindLayoutClass(layout, root, luaClass);
            if (found) {
                PushLuaString(state, luaClass);
            } else {
                String message("FindLayoutClass failed because layout named '");
                message << name << "' does not specify a class.";
            }
        }
    }
    return found ? 1 : 0;
}

} // namespace

const XMLDOMNode* LuaLayout::FindLayout(const String& name, const XMLDOMNode* scope,
                                        const XMLDOMNode* root)             // Mac 0x2cbd10, Win 0x468330
{
    if (name.IsEmpty())
        return 0;
    BasicArray<String> parts;
    name.Split((UniChar16)'.', parts);
    const String* first = parts.Begin();
    const XMLDOMNode* start = scope ? scope : root;
    if (first->IsEmpty()) {                                                // ".a.b": from the root only
        ++first;
        start = root;
    }
    if (first == parts.End())
        return 0;
    const XMLDOMNode* node = start;
    for (const String* part = first; part != parts.End(); ++part) {
        node = node->FindChild(*part);
        if (!node)
            break;
    }
    if (node)
        return node;
    if (start == root)
        return 0;
    node = root;                                                           // not below the scope: retry from the root
    for (const String* part = first; part != parts.End(); ++part) {
        node = node->FindChild(*part);
        if (!node)
            return 0;
    }
    return node;
}

bool LuaLayout::FindLayoutClass(const XMLDOMNode* layout, const XMLDOMNode* root, String& luaClass) // Mac 0x2cce70, Win 0x468460
{
    layout->GetAttribute(String("class"), luaClass);
    if (!luaClass.IsEmpty())
        return true;
    String inherited;
    if (!layout->GetAttribute(String("layout"), inherited) || inherited.IsEmpty())
        return false;
    BasicArray<String> names;
    SplitLayoutNames(inherited, names);
    for (UInt32 i = 0; i < names.GetSize(); ++i) {
        const XMLDOMNode* base = FindLayout(names[i], layout, root);
        if (!base) {
            // NOTE: built and discarded (both builds).
            String message("Trying to use non-existing inherited layout: ");
            message << names[i];
        } else if (FindLayoutClass(base, root, luaClass)) {
            return true;
        }
    }
    return false;
}

void LuaLayout::ApplyLayoutNode(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2cb620, Win 0x468bb0
{
    String scope;
    if (layout->GetAttribute(String("scope"), scope) && !scope.IsEmpty())
        args.m_scopes.PushBack(LuaLayoutArgs::ScopeEntry(scope.ToUTF8(), args.m_table));
    for (UInt32 i = 0; i < args.m_scopes.GetSize(); ++i) {
        const LuaLayoutArgs::ScopeEntry& entry = args.m_scopes[i];
        args.m_table->Set(entry.first.c_str(), *entry.second);
    }

    String inherited;
    if (layout->GetAttribute(String("layout"), inherited) && !inherited.IsEmpty()) {
        BasicArray<String> names;
        SplitLayoutNames(inherited, names);
        for (UInt32 i = 0; i < names.GetSize(); ++i) {
            const XMLDOMNode* base = FindLayout(names[i], layout, args.m_root);
            if (!base) {
                // NOTE: built and discarded (both builds).
                String message("Trying to use non-existing inherited layout: ");
                message << names[i];
            } else {
                ApplyLayoutNode(base, args);
            }
        }
    }

    args.m_layout->OnLayoutApplied(layout, args);

    if (!scope.IsEmpty())
        args.m_scopes.PopBack();

    if (args.m_callOnLoad) {
        const XMLDOMNode* postLoad = layout->FindChild(String("OnPostLoad"));
        if (postLoad)
            args.m_postLoads.PushBack(LuaLayoutArgs::PostLoadEntry(LuaObject(*args.m_table), postLoad));
    }
}

void LuaLayout::Apply(LuaObject* table, const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2cbfc0, Win 0x469030
{
    bool callOnLoad;
    if (m_firstLayout) {
        args.m_layouts.PushBack(this);
        m_firstLayout = false;
        callOnLoad = true;
    } else {
        LuaLayout** it = args.m_layouts.Begin();
        while (it != args.m_layouts.End() && *it != this)
            ++it;
        callOnLoad = it != args.m_layouts.End();
    }
    bool savedCallOnLoad = args.m_callOnLoad;
    args.m_callOnLoad = callOnLoad;
    LuaObject* savedTable = args.m_table;
    args.m_table = table;
    LuaLayout* savedLayout = args.m_layout;
    args.m_layout = this;
    ApplyLayoutNode(layout, args);
    args.m_layout = savedLayout;
    args.m_table = savedTable;
    args.m_callOnLoad = savedCallOnLoad;
}

void LuaLayout::ApplyLayout(LuaObject& table, const XMLDOMNode* layout, const XMLDOMNode* root) // Mac 0x2cc300, Win 0x4690b0
{
    LuaLayoutArgs args(root);
    args.m_onLoadContext.AssignNewTable(table.GetState(), 0, 0);
    Apply(&table, layout, args);
    for (LuaLayoutArgs::PostLoadEntry* entry = args.m_postLoads.Begin(); entry != args.m_postLoads.End(); ++entry) {
        String text = entry->second->GetText();
        args.m_table = &entry->first;
        RunLayoutScript(text, args);
    }
}

void LuaLayout::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2cc720, Win 0x4693d0
{
    if (!args.m_callOnLoad)
        return;
    const XMLDOMNode* onLoad = layout->FindChild(String("OnLoad"));
    if (onLoad)
        RunLayoutScript(onLoad->GetText(), args);
}

void LuaLayout::CreateChildren(const String& containerName, const XMLDOMNode* node, LuaLayoutArgs& args) // Mac 0x2cc7d0, Win 0x469480
{
    const XMLDOMNode* container = node->FindChild(containerName);
    if (!container)
        return;
    const XMLDOMNode::NodeArray& children = container->GetChildren();
    for (UInt32 i = 0; i < children.GetSize(); ++i)
        CreateChild(children[i], args);
}

LuaObject LuaLayout::CreateChild(const XMLDOMNode* node, LuaLayoutArgs& args) // Mac 0x2cc830, Win 0x4694d0
{
    const String& name = node->GetName();
    LuaState* state = args.m_table->GetState();
    LuaObject existing = args.m_table->GetByName(name.ToUTF8().c_str());
    if (!existing.IsNil()) {
        // Already created (e.g. by an inherited layout): lay it out again.
        BindableLuaObject* bound = GetBoundObject(existing);
        LuaLayout* layout = bound ? dynamic_cast<LuaLayout*>(bound) : 0;
        if (layout)
            layout->Apply(&existing, node, args);
        return existing;
    }

    // NOTE: the failure messages below are built and discarded (both builds).
    String luaClass;
    if (!FindLayoutClass(node, args.m_root, luaClass)) {
        String message("Layout child ");
        message << name << " not have a class specified on first layout.";
        return LuaObject(state);
    }
    LuaObject classTable = state->GetGlobal(luaClass.ToUTF8().c_str());
    if (!classTable.IsTable()) {
        String message("Layout child ");
        message << name << " of type " << luaClass << " cannot be created because the class does not exist.";
        return LuaObject(state);
    }
    LuaObject newFunction = classTable.GetByName("New");
    if (!newFunction.IsFunction()) {
        String message("Layout child ");
        message << name << " of type " << luaClass << " cannot be created because the class does not have a New() function.";
        return LuaObject(state);
    }
    {
        LuaCall call(newFunction);
        call << classTable << *args.m_table << name << LuaRun(1);       // Class:New(parent, name)
    }
    LuaObject child(state, -1);
    state->Pop(1);
    if (!child.IsTable()) {
        String message("Layout child ");
        message << name << " of type " << luaClass << " cannot be created because the class New() function returned garbage.";
        return LuaObject(state);
    }
    BindableLuaObject* bound = GetBoundObject(child);
    LuaLayout* layout = bound ? dynamic_cast<LuaLayout*>(bound) : 0;
    if (layout)
        layout->Apply(&child, node, args);
    return child;
}

bool LuaLayout::DefineLayoutFunction(const XMLDOMNode* node, const char* name, const char* params,
                                     LuaLayoutArgs& args)                   // Mac 0x2cd120, Win 0x469960
{
    const XMLDOMNode* child = node->FindChild(String(name));
    if (!child)
        return false;
    return DefineLuaMethod(*args.m_table, name, params, child->GetText());
}

void RegisterLuaLayout(LuaState* state, XMLDOMNode* layouts)              // Mac 0x2cb190, Win 0x468af0
{
    LuaObject luaClass = CreateLuaClass(state, "Layout", "Object");
    lua_pushlightuserdata(state->GetCState(), layouts);
    luaClass.Register("Layout", LuaLayoutLayout, 1);
    lua_pushlightuserdata(state->GetCState(), layouts);
    luaClass.Register("FindLayoutClass", LuaLayoutFindLayoutClass, 1);
    state->DoString(kLayoutScript);
}

} // namespace LEGO
