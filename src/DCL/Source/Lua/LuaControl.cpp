// LEGO Digital Designer — restored source
// TU 315 "LuaControl.cpp": Mac 0x2bda30-0x2c47b8, Win 0x44b680-0x44f1ff.
#include "LuaControl.h"
#include "LuaBindings.h"
#include "LayoutResources.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "LEGO/StdLib.h"
#include "Color.h"
#include "TextLocale.h"
#include "ControlManager.h"
#include "BufferedAbleRenderControl.h"
#include "RenderControl.h"
#include "TextLayout.h"
#include "Font.h"
#include "BitmapSelectionMask.h"

using namespace LuaPlus;

namespace LEGO {

// Case-insensitive order of the cursor-name map (StrILess on the bytes; a null AString
// compares as ""). Inlined in the map functions Mac 0x2c3800 / 0x2c3960.
template <>
struct StringCaseInsensitiveLess<AString> {
    bool operator()(const AString& a, const AString& b) const
    {
        return StrILess(a.GetBuffer(), b.GetBuffer());
    }
};

// Access to the private UpdateObserver base for the Lua functions below. (restoration helper)
class LuaControlPrivate {
public:
    static void AttachUpdater(LuaControl* object, Updater* updater) { object->UpdateObserver::Attach(updater); }
    static void DetachUpdater(LuaControl* object) { object->UpdateObserver::Detach(); }
};

namespace {

// ---- static data (static initialiser Mac 0x2c4600) ----------------------------------------------

const String kLeft("left");            // Mac 0x77ebf8
const String kTop("top");              // Mac 0x77ec08
const String kCenter("center");        // Mac 0x77ec18
const String kRight("right");          // Mac 0x77ec28
const String kBottom("bottom");        // Mac 0x77ec38
const String kHAnchor("hAnchor");      // Mac 0x77ec48
const String kVAnchor("vAnchor");      // Mac 0x77ec58

// Both directions of the cursor shape names (Mac 0x77ec68: name map +0x00, shape map +0x1c;
// constructor 0x2c2720, destructor 0x2c1f20). (name inferred)
struct CursorShapeNames {
    typedef BasicMap<AString, UInt32, StringCaseInsensitiveLess<AString> > ShapeMap;
    typedef BasicMap<UInt32, AString>                                       NameMap;

    CursorShapeNames()                                                      // Mac 0x2c2720
    {
        static const char* const kNames[] = {
            "none", "pointer", "clickable", "clicked", "grabable", "grabbed", "release", "drop",
            "pickable", "picked", "up", "down", "left", "right", "diagonalRightUpLeftDown",
            "diagonalRightDownLeftUp", "goodJob", "busy"
        };
        for (UInt32 shape = 0; shape < sizeof(kNames) / sizeof(kNames[0]); ++shape)
            Register(shape, AString(kNames[shape]));
    }

    void Register(UInt32 shape, const AString& name)
    {
        m_shapes[name] = shape;
        m_names[shape] = name;
    }

    ShapeMap m_shapes;   // name -> shape
    NameMap  m_names;    // shape -> name
};

CursorShapeNames g_cursorShapeNames;

// ---- argument helpers ------------------------------------------------------------------------

// The object of the LuaControl bound to `object`, or null. (restoration helper)
LuaControl* GetLuaControl(LuaObject& object)
{
    BindableLuaObject* bound = GetBoundObject(object);
    return bound ? dynamic_cast<LuaControl*>(bound) : 0;
}

// The Lua type error of the Check helpers, raised after the LuaObject went out of scope.
const char* BoundTypeName(LuaObject& object)
{
    BindableLuaObject* bound = GetBoundObject(object);
    return bound ? typeid(*bound).name() : 0;
}

// The Updater bound to argument `narg` (e.g. a LuaWindow's Window). Mac 0x2be1b0
Updater* CheckLuaUpdater(LuaState* state, int narg)
{
    Updater* result;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        result = bound ? dynamic_cast<Updater*>(bound) : 0;
        if (!result)
            got = BoundTypeName(object);
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(Updater).name());
    return result;
}

// ---- Lua functions of the class "Control" ------------------------------------------------------

// self:SetParent(parent, after) (nil: no parent / last). Mac 0x2be480, Win 0x44be30
int LuaControlSetParent(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    Control* control = CheckLuaControl(state, 1);
    Control* parent = OptLuaControl(state, 2, 0);
    Control* after = OptLuaControl(state, 3, 0);
    control->SetParent(parent, after);
    return 0;
}

// self:SetVisible([visible = true]). Mac 0x2be4e0, Win 0x44bed0
int LuaControlSetVisible(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    control->SetShown(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:GetVisible() -> visible in the tree (status bit 2). Mac 0x2be540, Win 0x44bf20
int LuaControlGetVisible(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    lua_pushboolean(L, control->GetStatus().m_visible);
    return 1;
}

// self:SetEnable([enabled = true]). Mac 0x2be590, Win 0x44bf60
int LuaControlSetEnable(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    control->SetEnabled(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:SetImage([file [, x, y, width, height]]): no / an empty name removes the image; the
// rectangle defaults to the current image rect. Mac 0x2be5f0, Win 0x44c1f0
int LuaControlSetImage(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    RenderControl* control = CheckLuaRenderControl(state, 1);
    String name = OptLuaString(state, 2, String());
    if (name.IsEmpty()) {
        control->SetImage(0, 0);
        return 0;
    }
    RefPointer<Texture> image = LoadLayoutTexture(name, control->GetManager());
    if (lua_gettop(L) < 3) {
        control->SetImage(image.Get(), 0);
    } else {
        RefPointer<Texture> current;
        Rect rect(0.0f, 0.0f, 0.0f, 0.0f);
        control->GetImage(current, rect);
        Vector2 position((Real32)luaL_optnumber(L, 3, rect.m_min.x),
                         (Real32)luaL_optnumber(L, 4, rect.m_min.y));
        Real32 width = (Real32)luaL_optnumber(L, 5, rect.GetWidth());
        Vector2 size(width, (Real32)luaL_optnumber(L, 6, rect.GetHeight()));
        Rect imageRect(position, size);
        control->SetImage(image.Get(), &imageRect);
    }
    return 0;
}

// self:SetRect([x, y, width, height]) (missing: keep). Mac 0x2be8a0, Win 0x44c3f0
int LuaControlSetRect(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    const Vector2& p = control->GetPosition();
    Vector2 position((Real32)luaL_optnumber(L, 2, p.x), (Real32)luaL_optnumber(L, 3, p.y));
    const Vector2& s = control->GetSize();
    Vector2 size((Real32)luaL_optnumber(L, 4, s.x), (Real32)luaL_optnumber(L, 5, s.y));
    control->SetRectangle(Rect(position, size));
    return 0;
}

// self:SetPosition([x, y]). Mac 0x2be9a0, Win 0x44c4a0
int LuaControlSetPosition(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    const Vector2& p = control->GetPosition();
    control->SetPosition(Vector2((Real32)luaL_optnumber(L, 2, p.x), (Real32)luaL_optnumber(L, 3, p.y)));
    return 0;
}

// self:GetPosition() -> x, y. Mac 0x2bea40, Win 0x44c530
int LuaControlGetPosition(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    lua_pushnumber(L, control->GetPosition().x);
    lua_pushnumber(L, control->GetPosition().y);
    return 2;
}

// self:SetSize([width, height]). Mac 0x2beac0, Win 0x44c590
int LuaControlSetSize(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    const Vector2& s = control->GetSize();
    control->SetSize(Vector2((Real32)luaL_optnumber(L, 2, s.x), (Real32)luaL_optnumber(L, 3, s.y)));
    return 0;
}

// self:GetSize() -> width, height. Mac 0x2beb60, Win 0x44c620
int LuaControlGetSize(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    lua_pushnumber(L, control->GetSize().x);
    lua_pushnumber(L, control->GetSize().y);
    return 2;
}

// self:SetScale([x, y]). Mac 0x2bebe0, Win 0x44c680
int LuaControlSetScale(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    const Vector2& s = control->GetScale();
    control->SetScale(Vector2((Real32)luaL_optnumber(L, 2, s.x), (Real32)luaL_optnumber(L, 3, s.y)));
    return 0;
}

// self:GetScale() -> x, y. Mac 0x2bec80, Win 0x44c6f0
int LuaControlGetScale(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    lua_pushnumber(L, control->GetScale().x);
    lua_pushnumber(L, control->GetScale().y);
    return 2;
}

// self:SetFocus([focus = true]). Mac 0x2bed00, Win 0x44c750
int LuaControlSetFocus(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    Control* control = CheckLuaControl(state, 1);
    control->SetKeyboardFocus(OptLuaBoolean(state, 2, true));
    return 0;
}

// self:SetText([text = ""]). Mac 0x2bed60, Win 0x44c7b0
int LuaControlSetText(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    RenderControl* control = CheckLuaRenderControl(state, 1);
    control->SetText(OptLuaString(state, 2, String()));
    return 0;
}

// self:SetLocText(key): the translation of `key`. Mac 0x2bee20, Win 0x44c850
int LuaControlSetLocText(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    RenderControl* control = CheckLuaRenderControl(state, 1);
    String key = CheckLuaString(state, 2);
    control->SetText(LocalizedString(key));
    return 0;
}

// self:ResizeToFitText([width = 0]): RenderControl slot 41 (Mac +0xa4).
// NOTE: RenderControl.h names slot 41 ResizeHeightToFitText(width) and slot 42 ResizeToFitText();
// the Lua names (here and in ApplyRenderAttributes) are swapped. Mac 0x2beee0, Win 0x44c910
int LuaControlResizeToFitText(lua_State* L)
{
    RenderControl* control = CheckLuaRenderControl(LuaState::CastState(L), 1);
    control->ResizeHeightToFitText((UInt32)(Int64)luaL_optnumber(L, 2, 0));
    return 0;
}

// self:ResizeHeightToFitText(): RenderControl slot 42 (Mac +0xa8). Mac 0x2bef40, Win 0x44c980
int LuaControlResizeHeightToFitText(lua_State* L)
{
    RenderControl* control = CheckLuaRenderControl(LuaState::CastState(L), 1);
    control->ResizeToFitText();
    return 0;
}

// self:Modal([modal = true]): the manager's modal control. Mac 0x2bef80, Win 0x44be80
int LuaControlModal(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    control->GetManager()->SetModalControl(OptLuaBoolean(L, 2, true), control);
    return 0;
}

// self:ConnectUpdateObserver([updater]): self:OnUpdate() on every pass of `updater` (default:
// the control manager). Mac 0x2be090, Win 0x44c140
int LuaControlConnectUpdateObserver(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaControl* self = LuaControl::Check(state, 1);
    Control* control = self->GetTargetControl();
    Updater* updater;
    if (lua_type(L, 2) <= LUA_TNIL)
        updater = control->GetManager();
    else
        updater = CheckLuaUpdater(state, 2);
    LuaControlPrivate::AttachUpdater(self, updater);
    return 0;
}

// self:DisconnectUpdateObserver(). Mac 0x2be280, Win 0x44c1b0
int LuaControlDisconnectUpdateObserver(lua_State* L)
{
    LuaControl* self = LuaControl::Check(LuaState::CastState(L), 1);
    LuaControlPrivate::DetachUpdater(self);
    return 0;
}

// self:FitToChildren([top, left, bottom, right]): a number is the margin of that side, true or
// nothing fits the side without a margin, false keeps it. Mac 0x2befd0, Win 0x44bfc0
int LuaControlFitToChildren(lua_State* L)
{
    Control* control = CheckLuaControl(LuaState::CastState(L), 1);
    Real32 margins[4];
    const Real32* sides[4];
    for (int i = 0; i < 4; ++i) {
        int narg = i + 2;
        if (lua_isnumber(L, narg)) {
            margins[i] = (Real32)lua_tonumber(L, narg);
            sides[i] = &margins[i];
        } else {
            margins[i] = 0.0f;
            sides[i] = OptLuaBoolean(L, narg, true) ? &margins[i] : 0;
        }
    }
    control->FitToChildren(sides[0], sides[1], sides[2], sides[3]);
    return 0;
}

// Control.Create(self, parent): a plain transparent RenderControl of the upvalue's manager
// (Initialize, flags 7 = shown/enabled/active, empty rectangle) bound to `self`.
// Mac 0x2bf140, Win 0x44bc10
int LuaControlCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
    RefPointer<RenderControl> control = new RenderControl(manager);
    control->Initialize();
    control->SetTransparent(true);
    RefPointer<LuaControl> object = new LuaControl;
    LuaObject parent(state, 2);
    LuaObject after(state);
    object->Initialize(self, parent, after, control.Get(), 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x7479bc
    { "SetParent", LuaControlSetParent },
    { "SetVisible", LuaControlSetVisible },
    { "GetVisible", LuaControlGetVisible },
    { "SetEnable", LuaControlSetEnable },
    { "SetImage", LuaControlSetImage },
    { "SetRect", LuaControlSetRect },
    { "SetPosition", LuaControlSetPosition },
    { "GetPosition", LuaControlGetPosition },
    { "SetSize", LuaControlSetSize },
    { "GetSize", LuaControlGetSize },
    { "SetScale", LuaControlSetScale },
    { "GetScale", LuaControlGetScale },
    { "SetFocus", LuaControlSetFocus },
    { "SetText", LuaControlSetText },
    { "SetLocText", LuaControlSetLocText },
    { "ResizeToFitText", LuaControlResizeToFitText },
    { "ResizeHeightToFitText", LuaControlResizeHeightToFitText },
    { "Modal", LuaControlModal },
    { "ConnectUpdateObserver", LuaControlConnectUpdateObserver },
    { "DisconnectUpdateObserver", LuaControlDisconnectUpdateObserver },
    { "FitToChildren", LuaControlFitToChildren },
    { 0, 0 }
};

// ---- anchors ---------------------------------------------------------------------------------

// One anchor value: left/top 0, center 0.5, right/bottom 1, else a number or "#" (keep).
// Mac 0x2c2650, Win 0x44cdc0
bool ParseAnchorFraction(Real32& value, const String& text, UInt32& position)
{
    if (ParseKeyword(kLeft, text, position, ',') || ParseKeyword(kTop, text, position, ',')) {
        value = 0.0f;
        return true;
    }
    if (ParseKeyword(kCenter, text, position, ',')) {
        value = 0.5f;
        return true;
    }
    if (ParseKeyword(kRight, text, position, ',') || ParseKeyword(kBottom, text, position, ',')) {
        value = 1.0f;
        return true;
    }
    return FromStringOptional(value, text, position);
}

// An anchor element: point="own[, reference]", offset="offset[, own[, reference]]",
// dependent="Lua path" (empty: the parent). No attribute switches the anchor off; the first use
// starts from the control's current anchor; a changed anchor without a reference refers to the
// parent. Mac 0x2c2280, Win 0x44ce60 (node in ECX, args in EDX)
void ParseAnchorElement(const XMLDOMNode* node, LuaLayoutArgs& args, Control* control,
                        InternalAnchorData& anchor, bool& set, UInt32 axis, bool nearEdge)
{
    if (node->GetAttributes().IsEmpty()) {
        anchor = InternalAnchorData(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, false);
        set = true;
        return;
    }
    if (!set) {
        anchor = *control->GetAnchor(axis, nearEdge);
        set = true;
    }
    bool changed = false;
    String point;
    if (node->GetAttribute(String("point"), point)) {
        UInt32 position = 0;
        if (ParseAnchorFraction(anchor.m_values[0], point, position)) {
            ++position;
            if (!ParseAnchorFraction(anchor.m_values[2], point, position))
                anchor.m_values[2] = anchor.m_values[0];
            changed = true;
        }
    }
    String offset;
    if (node->GetAttribute(String("offset"), offset)) {
        UInt32 position = 0;
        if (FromStringOptional(anchor.m_values[4], offset, position)) {
            ++position;
            if (FromStringOptional(anchor.m_values[1], offset, position)) {
                ++position;
                FromStringOptional(anchor.m_values[3], offset, position);
            }
            changed = true;
        }
    }
    String dependent;
    if (node->GetAttribute(String("dependent"), dependent)) {
        if (dependent.IsEmpty()) {
            anchor.m_reference = 0;
            anchor.m_flag = true;
        } else {
            Control* reference = 0;
            {
                LuaObject object = LookupLuaObject(*args.m_table, dependent);
                LuaControl* bound = GetLuaControl(object);
                if (bound)
                    reference = bound->GetTargetControl();
            }
            anchor.m_reference = reference;
            anchor.m_flag = false;
        }
    }
    if (changed && !anchor.m_reference && !anchor.m_flag)
        anchor.m_flag = true;
}

// <hAnchor> / <vAnchor> (near anchor; the far one off), <hAnchorA> / <vAnchorA> (near),
// <hAnchorB> / <vAnchorB> (far) of axis `axis` (0 h, 1 v). `anchors` / `set` are the pair of
// that axis. Mac 0x2c0570, Win 0x44d130 (node in ECX, args in EDX)
void ParseAnchorNode(const XMLDOMNode* node, LuaLayoutArgs& args, Control* control,
                     InternalAnchorData* anchors, bool* set, UInt32 axis)
{
    const String& name = node->GetName();
    if (!name.StartsWith(axis ? kVAnchor : kHAnchor))
        return;
    if (name.GetLength() == 7) {
        ParseAnchorElement(node, args, control, anchors[0], set[0], axis, true);
        anchors[1] = InternalAnchorData(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, false);
        set[1] = true;
    } else if (name.GetLength() == 8) {
        if (name.EndsWith('A'))
            ParseAnchorElement(node, args, control, anchors[0], set[0], axis, true);
        else if (name.EndsWith('B'))
            ParseAnchorElement(node, args, control, anchors[1], set[1], axis, false);
    }
}

} // namespace

// ================================================================================================
// LuaControl
// ================================================================================================

LuaControl::LuaControl()                                                    // Mac 0x2bdb50, Win 0x44b680
    : m_control(0), m_bufferedControl(0), m_renderControl(0)
{
}

// The augmentation leaves its control (ControlAugmentation's dtor). Mac 0x2bf480 (base
// 0x2bf390), Win 0x44cc00
LuaControl::~LuaControl()
{
}

RefPointer<LuaControl> LuaControl::FromControl(Control* control)           // Mac 0x2bdc50, Win 0x44b7e0
{
    RefPointer<ControlAugmentation> augmentation;
    control->GetAugmentation(String("lua"), augmentation);
    if (!augmentation)
        return RefPointer<LuaControl>();
    return RefPointer<LuaControl>(static_cast<LuaControl*>(augmentation.Get()));
}

LuaObject LuaControl::GetControlTable(LuaState* state, Control* control)    // Mac 0x2bdd30, Win 0x44b8d0
{
    if (control) {
        RefPointer<LuaControl> object = FromControl(control);
        if (object) {
            LuaObject table = LEGO::GetLuaObject(object.Get());
            if (!table.IsNil())
                return table;
            object->UnbindLuaObject();
            return LuaObject(state);
        }
    }
    return LuaObject(state);
}

LuaObject LuaControl::CreateControlTable(LuaState* state, Control* control) // Mac 0x2bde10, Win 0x44ba40
{
    RefPointer<LuaControl> object;
    return CreateLuaControlTable(control, state, "Control", object);
}

bool LuaControl::Bind(Control* control, LuaObject& table)                  // Mac 0x2bdeb0, Win 0x44bae0
{
    if (!Attach(control, String("lua")))
        return false;
    AttachLuaObject(table, this);
    return true;
}

LuaControl* LuaControl::Check(LuaState* state, int narg)                    // Mac 0x2be2e0
{
    return CheckBoundObject<LuaControl>(state, narg);
}

// Mac 0x2bdf40, Win 0x44bb90
bool LuaControl::Initialize(LuaObject& self, LuaObject& parent, LuaObject& after, Control* control,
                            UInt32 flags, const Rect& rectangle, UInt32 inputFlags, bool hitTestable)
{
    if (!Bind(control, self))
        return false;
    LuaControl* parentObject = GetLuaControl(parent);
    Control* parentControl = parentObject ? parentObject->m_control : 0;
    LuaControl* afterObject = GetLuaControl(after);
    Control* afterControl = afterObject ? afterObject->m_control : 0;
    m_control->Setup(parentControl, afterControl, flags, rectangle, inputFlags, hitTestable);
    return true;
}

// Mac 0x2bf8a0, Win 0x44ccc0
void LuaControl::OnFinalRelease()
{
    UpdateObserver::OnFinalRelease();
    ControlObserver::OnFinalRelease();
    ControlAugmentation::OnFinalRelease();
    LuaLayout::OnFinalRelease();
}

// Mac 0x2bf9a0, Win 0x44cd00
void LuaControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    ApplyControlAttributes(layout, args);
    if (m_bufferedControl)
        ApplyColorAttributes(layout, args);
    if (m_renderControl)
        ApplyRenderAttributes(layout, args);
    DefineCallbacks(layout, args);
    CreateAugmentations(layout, args);
    LuaLayout::OnLayoutApplied(layout, args);
    CreateChildren(String("Children"), layout, args);
}

// Mac 0x2bfa90, Win 0x44d270
void LuaControl::ApplyControlAttributes(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    String parentName;
    if (layout->GetAttribute(String("parent"), parentName)) {
        Control* parent = 0;
        if (!parentName.IsEmpty()) {
            LuaObject object = LookupLuaObject(*args.m_table, parentName);
            LuaControl* bound = GetLuaControl(object);
            if (bound)
                parent = bound->m_control;
        }
        m_control->SetParent(parent, 0);
    }

    Vector2 position;
    {
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("position"), value) && FromString(position, value, at))
            m_control->SetPosition(position);
    }
    Vector2 minimumSize;
    {
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("minSize"), value) && FromString(minimumSize, value, at))
            m_control->SetMinimumSize(minimumSize);
    }
    Vector2 maximumSize;
    {
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("maxSize"), value) && FromString(maximumSize, value, at))
            m_control->SetMaximumSize(maximumSize);
    }
    Vector2 size;
    {
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("size"), value) && FromString(size, value, at)) {
            m_control->SetSize(size);
            // An explicit size stops the render control's fit-to-text sizing (input flag 0x40000000).
            if (m_renderControl)
                m_renderControl->SetInputFlags(m_renderControl->GetInputFlags() & 0xbfffffff);
        }
    }

    if (const XMLDOMNode* anchorsNode = layout->FindChild(String("Anchors"))) {
        InternalAnchorData anchors[4];   // h near, h far, v near, v far
        for (int i = 0; i < 4; ++i)
            anchors[i] = InternalAnchorData(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, false);
        bool set[4] = { false, false, false, false };
        const XMLDOMNode::NodeArray& children = anchorsNode->GetChildren();
        for (UInt32 i = 0; i < children.GetSize(); ++i) {
            ParseAnchorNode(children[i].Get(), args, m_control, &anchors[0], &set[0], 0);
            ParseAnchorNode(children[i].Get(), args, m_control, &anchors[2], &set[2], 1);
        }
        m_control->SetAnchors(set[0] ? &anchors[0] : 0, set[1] ? &anchors[1] : 0,
                              set[2] ? &anchors[2] : 0, set[3] ? &anchors[3] : 0);
    }

    {
        String value;
        bool hit;
        if (layout->GetAttribute(String("hit"), value) && FromString(hit, value))
            m_control->SetHitTestable(hit);
    }
    {
        RefPointer<SelectionMask> mask;
        if (ParseSelectionMask(layout, mask))
            m_control->SetSelectionMask(mask.Get());
    }
    {
        String value;
        bool visible;
        if (layout->GetAttribute(String("visible"), value) && FromString(visible, value))
            m_control->SetShown(visible);
    }
    {
        String value;
        bool enabled;
        if (layout->GetAttribute(String("enable"), value) && FromString(enabled, value))
            m_control->SetEnabled(enabled);
    }
    {
        // cursor="shape[, flag]" ("#" keeps a part).
        String value;
        if (layout->GetAttribute(String("cursor"), value)) {
            UInt32 shape = m_control->GetCursor();
            bool flag = m_control->GetCursorFlag();
            UInt32 at = 0;
            if (ParseCursorShapeOptional(shape, value, at)) {
                ++at;
                FromStringOptional(flag, value, at);
            }
            m_control->SetCursor(shape, flag);
        }
    }
}

// Mac 0x2c0750, Win 0x44d980
void LuaControl::ApplyColorAttributes(const XMLDOMNode* layout, LuaLayoutArgs& /*args*/)
{
    {
        String value;
        Real32 alpha;
        if (layout->GetAttribute(String("alpha"), value) && FromString(alpha, value))
            m_bufferedControl->SetOpacity(alpha);
    }
    Color color = m_bufferedControl->GetColor();
    {
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("blendColor"), value) && FromStringOptional(color, value, at))
            m_bufferedControl->SetColor(color);
    }
}

// Mac 0x2c08e0, Win 0x44da90
void LuaControl::ApplyRenderAttributes(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    RenderControl* control = m_renderControl;
    {
        String value;
        bool transparent;
        if (layout->GetAttribute(String("transparent"), value) && FromString(transparent, value))
            control->SetTransparent(transparent);
    }

    RefPointer<Texture> image;
    Rect imageRect(0.0f, 0.0f, 0.0f, 0.0f);
    bool hasImage = false, hasRect = false;
    ParseLayoutImage(layout, image, imageRect, hasImage, hasRect, control->GetManager());
    if (hasImage)
        control->SetImage(image.Get(), &imageRect);
    else if (hasRect)
        control->SetImageRect(&imageRect);

    String frame;
    if (layout->GetAttribute(String("drawImageAsFrame"), frame)) {
        // "left centre right, top centre bottom, drawCenter"
        Vector3 horizontal, vertical;
        bool drawCenter = false;
        UInt32 at = 0;
        FromString(horizontal, frame, at);
        ++at;
        FromString(vertical, frame, at);
        ++at;
        FromString(drawCenter, frame, at);
        control->SetFrame(horizontal, vertical, drawCenter);
        control->SetDrawAsFrame(true);
    }

    RefPointer<Font> font;
    if (ParseLayoutFont(layout, args.m_root, font, control->GetManager()))
        control->SetFont(font.Get());

    String text;
    if (layout->GetAttribute(String("text"), text))
        control->SetText(text);
    if (layout->GetAttribute(String("locText"), text))
        control->SetText(LocalizedString(text));

    {
        String value;
        UInt32 alignment;
        UInt32 at = 0;
        if (layout->GetAttribute(String("hAlignment"), value) &&
            TextLayout::ParseHorizontalAlignment(alignment, value, at))
            control->SetHorizontalAlignment(alignment);
    }
    {
        String value;
        UInt32 alignment;
        UInt32 at = 0;
        if (layout->GetAttribute(String("vAlignment"), value) &&
            TextLayout::ParseVerticalAlignment(alignment, value, at))
            control->SetVerticalAlignment(alignment);
    }
    {
        String value;
        Real32 offset;
        if (layout->GetAttribute(String("hAlignmentOffset"), value) && FromString(offset, value))
            control->SetHorizontalTextOffset(offset);
    }
    {
        String value;
        Real32 offset;
        if (layout->GetAttribute(String("vAlignmentOffset"), value) && FromString(offset, value))
            control->SetVerticalTextOffset(offset);
    }

    UInt32 width;
    {
        RefPointer<TextLayoutRender> textLayout;
        control->GetTextLayout(textLayout, false);
        width = textLayout ? textLayout->GetMaxWidth() : 0;
    }
    {
        // resizeToFitText="[width | #]" -> RenderControl slot 41 (see LuaControlResizeToFitText).
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("resizeToFitText"), value) && FromStringOptional(width, value, at))
            control->ResizeHeightToFitText(width);
    }
    {
        String value;
        if (layout->GetAttribute(String("resizeHeightToFitText"), value))
            control->ResizeToFitText();
    }
}

// Mac 0x2c11a0, Win 0x44e0d0
void LuaControl::DefineCallbacks(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    DefineLayoutFunction(layout, "OnRectangleChanged", "", args);
    DefineLayoutFunction(layout, "OnShown", "", args);
    DefineLayoutFunction(layout, "OnHidden", "", args);
    DefineLayoutFunction(layout, "OnEnabled", "", args);
    DefineLayoutFunction(layout, "OnDisabled", "", args);
    DefineLayoutFunction(layout, "OnMousePressed", "x,y,button", args);
    DefineLayoutFunction(layout, "OnMouseReleased", "x,y,button", args);
    DefineLayoutFunction(layout, "OnMouseClicked", "x,y,button", args);
    DefineLayoutFunction(layout, "OnChar", "char,modifiers,repeated", args);
    if (DefineLayoutFunction(layout, "OnUpdate", "", args))
        UpdateObserver::Attach(m_control->GetManager());
}

// Mac 0x2c1300, Win 0x44e1d0
void LuaControl::CreateAugmentations(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    CreateChildren(String("Augmentations"), layout, args);
}

void LuaControl::BindLuaObject(LuaObject& /*table*/)                        // Mac 0x2c1380, Win 0x44e240
{
    m_control->AddRef();
}

void LuaControl::UnbindLuaObject()                                          // Mac 0x2c13b0, Win 0x44e260
{
    if (m_control) {
        m_control->Release();
        Detach();
    }
}

bool LuaControl::Attach(Control* control, const String& name)              // Mac 0x2c13f0, Win 0x44e290
{
    if (!ControlAugmentation::Attach(control, name))
        return false;
    m_control = control;
    m_renderControl = dynamic_cast<RenderControl*>(control);
    if (m_renderControl)
        m_bufferedControl = m_renderControl;
    else
        m_bufferedControl = dynamic_cast<BufferedAbleRenderControl*>(control);
    ControlObserver::Attach(control);
    return true;
}

void LuaControl::Detach()                                                   // Mac 0x2c14f0, Win 0x44e2e0
{
    if (m_control) {
        ControlObserver::Detach(m_control);
        UpdateObserver::Detach();
        m_renderControl = 0;
        m_bufferedControl = 0;
        m_control = 0;
    }
    ControlAugmentation::Detach();
}

void LuaControl::OnRectangleChanged(Control* control, const Vector2& /*oldPosition*/,
                                    const Vector2& /*oldSize*/)            // Mac 0x2c1610, Win 0x44e330
{
    if (m_control == control)
        CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnRectangleChanged", false, 0);
}

void LuaControl::OnShown(Control* control)                                  // Mac 0x2c1680, Win 0x44e360
{
    if (m_control == control)
        CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnShown", false, 0);
}

void LuaControl::OnHidden(Control* control)                                 // Mac 0x2c16f0, Win 0x44e390
{
    if (m_control == control)
        CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnHidden", false, 0);
}

void LuaControl::OnEnabled(Control* control)                                // Mac 0x2c1760, Win 0x44e3c0
{
    if (m_control == control)
        CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnEnabled", false, 0);
}

void LuaControl::OnDisabled(Control* control)                               // Mac 0x2c17d0, Win 0x44e3f0
{
    if (m_control == control)
        CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnDisabled", false, 0);
}

// self:OnMousePressed(x, y, button) (CallLuaMethod<float, float, UInt32>, Mac 0x2c43c0).
void LuaControl::OnMousePressed(Control* control, const Vector2& position, UInt32 button) // Mac 0x2c1840, Win 0x44e420
{
    if (m_control == control) {
        LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
        CallLuaMethod(self, "OnMousePressed", false, 0, position.x, position.y, (unsigned int)button);
    }
}

void LuaControl::OnMouseReleased(Control* control, const Vector2& position, UInt32 button) // Mac 0x2c19c0, Win 0x44e460
{
    if (m_control == control) {
        LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
        CallLuaMethod(self, "OnMouseReleased", false, 0, position.x, position.y, (unsigned int)button);
    }
}

void LuaControl::OnMouseClicked(Control* control, const Vector2& position, UInt32 button) // Mac 0x2c1b40, Win 0x44e4a0
{
    if (m_control == control) {
        LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
        CallLuaMethod(self, "OnMouseClicked", false, 0, position.x, position.y, (unsigned int)button);
    }
}

// self:OnChar(char, modifiers, repeated) (helper Mac 0x2c1d60 -> CallLuaMethod<String, UInt32,
// bool> 0x2c44e0).
void LuaControl::OnChar(Control* control, UniChar16 c, UInt32 modifiers, bool repeated) // Mac 0x2c1cc0, Win 0x44e4e0
{
    if (m_control == control) {
        LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
        CallLuaMethod(self, "OnChar", false, 0, String(1, c), (unsigned int)modifiers, repeated);
    }
}

void LuaControl::Update()                                                   // Mac 0x2c1ec0, Win 0x44e560
{
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnUpdate", false, 0);
}

// ================================================================================================
// argument helpers, cursor names, registration
// ================================================================================================

Control* CheckLuaControl(LuaState* state, int narg)                         // Mac 0x2b9670
{
    Control* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        LuaControl* bound = GetLuaControl(object);
        if (bound)
            result = bound->GetTargetControl();
        if (!result)
            got = BoundTypeName(object);
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(Control).name());
    return result;
}

Control* OptLuaControl(LuaState* state, int narg, Control* def)             // Mac 0x2c20c0
{
    Control* result = def;
    const char* got = 0;
    bool error = false;
    {
        LuaObject object(state, narg);
        if (!object.IsNil()) {
            LuaControl* bound = GetLuaControl(object);
            result = bound ? bound->GetTargetControl() : 0;
            if (!result) {
                error = true;
                got = BoundTypeName(object);
            }
        }
    }
    if (error)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(Control).name());
    return result;
}

RenderControl* CheckLuaRenderControl(LuaState* state, int narg)             // Mac 0x2c21b0
{
    RenderControl* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        LuaControl* bound = GetLuaControl(object);
        if (bound)
            result = bound->GetRenderControl();
        if (!result)
            got = BoundTypeName(object);
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(RenderControl).name());
    return result;
}

void RegisterCursorShapeName(UInt32 shape, const AString& name)            // Mac 0x2c1fb0, Win 0x44ec40
{
    g_cursorShapeNames.Register(shape, name);
}

bool ParseCursorShape(UInt32& shape, const String& text, UInt32& position) // Mac 0x2c2010
{
    for (CursorShapeNames::ShapeMap::Iterator it = g_cursorShapeNames.m_shapes.Begin();
         it != g_cursorShapeNames.m_shapes.End(); ++it) {
        if (ParseKeyword(String(it->first), text, position, ',')) {
            shape = it->second;
            return true;
        }
    }
    return false;
}

void RegisterLuaControl(LuaState* state, ControlManager* manager)          // Mac 0x2be3b0, Win 0x44c9b0
{
    LuaObject luaClass = CreateLuaClass(state, "Control", "Layout");
    luaClass.Register(kFunctions, 0);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register("Create", LuaControlCreate, 1);
}

} // namespace LEGO
