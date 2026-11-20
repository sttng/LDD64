// LEGO Digital Designer — restored source
// TU 316 "LuaMatrixLayoutControl.cpp": Mac 0x2c47c0-0x2c5c7b, Win 0x464c90-0x465a8f.
#include "LuaMatrixLayoutControl.h"
#include "LuaBindings.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "ControlManager.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

MatrixLayoutControl* CheckMatrix(lua_State* L)
{
    return CheckLuaMatrixLayoutControl(LuaState::CastState(L), 1);
}

// self:GetMatrixSize() -> cell width, height. Mac 0x2c4890
int LuaMatrixGetMatrixSize(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    const Vector2& size = matrix->GetCellSize();
    lua_pushnumber(L, size.x);
    lua_pushnumber(L, size.y);
    return 2;
}

// self:SetMatrixSize([width, height]) (missing: keep). Mac 0x2c4900
int LuaMatrixSetMatrixSize(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    Real32 width = (Real32)luaL_optnumber(L, 2, matrix->GetCellSize().x);
    Real32 height = (Real32)luaL_optnumber(L, 3, matrix->GetCellSize().y);
    matrix->SetCellSize(Vector2(width, height));
    return 0;
}

// self:SetAutoSizeMode([mode = 0]) (0-3; others: 0). Mac 0x2c49a0, Win 0x465170
int LuaMatrixSetAutoSizeMode(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    UInt32 mode = (UInt32)(Int64)luaL_optnumber(L, 2, 0);
    if (mode > 3)
        mode = 0;
    matrix->SetFitMode((MatrixLayoutControl::FitMode)mode);
    return 0;
}

// self:SetLeftToRight([leftToRight = true]). Mac 0x2c4a00
int LuaMatrixSetLeftToRight(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    matrix->SetLeftToRight(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:SetTopToBottom([topToBottom = true]). Mac 0x2c4a60
int LuaMatrixSetTopToBottom(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    matrix->SetTopToBottom(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:GetNumberOfControls(). Mac 0x2c4ac0
int LuaMatrixGetNumberOfControls(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    lua_pushnumber(L, (lua_Number)matrix->GetControlCount());
    return 1;
}

// self:AddControl(control). Mac 0x2c4b20
int LuaMatrixAddControl(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    Control* control = CheckLuaControl(LuaState::CastState(L), 2);
    matrix->AddControl(control);
    return 0;
}

// self:RemoveControl(control). Mac 0x2c4b70
int LuaMatrixRemoveControl(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    Control* control = CheckLuaControl(LuaState::CastState(L), 2);
    matrix->RemoveControl(control);
    return 0;
}

// self:RemoveControls(). Mac 0x2c4bc0
int LuaMatrixRemoveControls(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    matrix->RemoveAllControls();
    return 0;
}

// self:GetSpacing(). NOTE: the spacing is pushed but the function returns 0 results (both
// builds), so Lua gets nothing. Mac 0x2c4bf0, Win 0x465260
int LuaMatrixGetSpacing(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    lua_pushnumber(L, matrix->GetSpacing());
    return 0;
}

// self:SetSpacing([spacing = 0]). Mac 0x2c4c30, Win 0x4652a0
int LuaMatrixSetSpacing(lua_State* L)
{
    MatrixLayoutControl* matrix = CheckMatrix(L);
    matrix->SetSpacing((Real32)luaL_optnumber(L, 2, 0));
    return 0;
}

// MatrixControl.Create(self, parent). Mac 0x2c4c80
int LuaMatrixCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
    RefPointer<MatrixLayoutControl> control = new MatrixLayoutControl(manager);
    RefPointer<LuaMatrixLayoutControl> object = new LuaMatrixLayoutControl;
    LuaObject parent(state, 2);
    LuaObject after(state);
    object->Initialize(self, parent, after, control.Get(), 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x747f10
    { "GetMatrixSize", LuaMatrixGetMatrixSize },
    { "SetMatrixSize", LuaMatrixSetMatrixSize },
    { "SetAutoSizeMode", LuaMatrixSetAutoSizeMode },
    { "SetLeftToRight", LuaMatrixSetLeftToRight },
    { "SetTopToBottom", LuaMatrixSetTopToBottom },
    { "GetNumberOfControls", LuaMatrixGetNumberOfControls },
    { "AddControl", LuaMatrixAddControl },
    { "RemoveControl", LuaMatrixRemoveControl },
    { "RemoveControls", LuaMatrixRemoveControls },
    { "GetSpacing", LuaMatrixGetSpacing },
    { "SetSpacing", LuaMatrixSetSpacing },
    { 0, 0 }
};

} // namespace

LuaMatrixLayoutControl::~LuaMatrixLayoutControl()                           // Mac 0x2c5110, Win 0x465540
{
}

LuaObject LuaMatrixLayoutControl::CreateControlTable(LuaState* state, Control* control) // Mac 0x2c4ef0, Win 0x465970
{
    RefPointer<LuaMatrixLayoutControl> object;
    return CreateLuaControlTable(control, state, "MatrixControl", object);
}

// Mac 0x2c5530, Win 0x4655c0
void LuaMatrixLayoutControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    {
        MatrixLayoutControl::FitMode mode;
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("autoSizeMode"), value) && FromString(mode, value, at))
            m_matrix->SetFitMode(mode);
    }
    LuaControl::OnLayoutApplied(layout, args);
    {
        Vector2 size;
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("matrixSize"), value) && FromString(size, value, at))
            m_matrix->SetCellSize(size);
    }
    {
        bool leftToRight;
        String value;
        if (layout->GetAttribute(String("leftToRight"), value) && FromString(leftToRight, value))
            m_matrix->SetLeftToRight(leftToRight);
    }
    {
        bool topToBottom;
        String value;
        if (layout->GetAttribute(String("topToBottom"), value) && FromString(topToBottom, value))
            m_matrix->SetTopToBottom(topToBottom);
    }
    {
        Real32 spacing;
        String value;
        if (layout->GetAttribute(String("spacing"), value) && FromString(spacing, value))
            m_matrix->SetSpacing(spacing);
    }
    // NOTE: a child that is not a Lua control adds a null control (both builds).
    const XMLDOMNode* content = layout->FindChild(String("Content"));
    if (!content)
        return;
    const XMLDOMNode::NodeArray& children = content->GetChildren();
    for (UInt32 i = 0; i < children.GetSize(); ++i) {
        LuaObject table = CreateChild(children[i].Get(), args);
        if (table.IsNil())
            continue;
        Control* control = 0;
        if (BindableLuaObject* bound = GetBoundObject(table))
            if (LuaControl* object = dynamic_cast<LuaControl*>(bound))
                control = object->GetTargetControl();
        MatrixLayoutControl::ControlIterator it = m_matrix->BeginControls();
        while (it != m_matrix->EndControls() && it->Get() != control)
            ++it;
        if (it == m_matrix->EndControls())
            m_matrix->AddControl(control);
    }
}

// Mac 0x2c5a10, Win 0x4658c0
bool LuaMatrixLayoutControl::Attach(Control* control, const String& name)
{
    if (!LuaControl::Attach(control, name))
        return false;
    m_matrix = dynamic_cast<MatrixLayoutControl*>(GetTargetControl());
    return true;
}

void LuaMatrixLayoutControl::Detach()                                       // Mac 0x2c5af0, Win 0x4649d0
{
    m_matrix = 0;
    LuaControl::Detach();
}

MatrixLayoutControl* CheckLuaMatrixLayoutControl(LuaState* state, int narg) // Mac 0x2c5b30, Win 0x465900
{
    MatrixLayoutControl* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaMatrixLayoutControl* lua = bound ? dynamic_cast<LuaMatrixLayoutControl*>(bound) : 0;
        if (lua)
            result = lua->GetMatrix();
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(MatrixLayoutControl).name());
    return result;
}

void RegisterLuaMatrixLayoutControl(LuaState* state, ControlManager* manager) // Mac 0x2c47c0, Win 0x465320
{
    LuaObject luaClass = CreateLuaClass(state, "MatrixControl", "Control");
    luaClass.Register(kFunctions, 0);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register("Create", LuaMatrixCreate, 1);
}

} // namespace LEGO
