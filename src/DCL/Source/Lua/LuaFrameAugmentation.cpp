// LEGO Digital Designer — restored source
// TU 409 "LuaFrameAugmentation.cpp": Mac 0x38dd00-0x38e8ab, Win 0x45b3f0-0x45b93f.
// The Mac static initialiser 0x38e830 is not code.
#include "LuaFrameAugmentation.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "XMLDOMNode.h"
#include "StringStream.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:SetFrame([left [, top [, right [, bottom]]]]). Mac 0x38df60
int LuaFrameAugmentationSetFrame(lua_State* L)
{
    LuaFrameAugmentation* object = LuaFrameAugmentation::Check(LuaState::CastState(L), 1);
    FrameAugmentation* frame = object;
    Real32 left = (Real32)luaL_optnumber(L, 2, frame->GetLeft());
    Real32 top = (Real32)luaL_optnumber(L, 3, frame->GetTop());
    Real32 right = (Real32)luaL_optnumber(L, 4, frame->GetRight());
    Real32 bottom = (Real32)luaL_optnumber(L, 5, frame->GetBottom());
    frame->SetFrame(left, top, right, bottom);
    return 0;
}

// FrameAugmentation.Create(self [, control]). Mac 0x38e030, Win 0x45b4e0
int LuaFrameAugmentationCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    RefPointer<LuaFrameAugmentation> object;
    {
        LuaObject self = GetUnboundSelf(state);
        object = new LuaFrameAugmentation;
        AttachLuaObject(self, object);
    }
    Control* control = OptLuaControl(state, 2, 0);
    if (control)
        object->Attach(control, String("frame"));
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x756fc8
    { "SetFrame", LuaFrameAugmentationSetFrame },
    { 0, 0 }
};

} // namespace

LuaFrameAugmentation* LuaFrameAugmentation::Check(LuaState* state, int narg) // Mac 0x38e760
{
    return CheckBoundObject<LuaFrameAugmentation>(state, narg);
}

void LuaFrameAugmentation::UnbindLuaObject()                                 // Mac 0x38e5e0
{
    Detach();
}

void LuaFrameAugmentation::OnFinalRelease()                                  // Mac 0x38e410, Win 0x45b830
{
    FrameAugmentation::OnFinalRelease();
    LuaControlAugmentation::OnFinalRelease();
}

void LuaFrameAugmentation::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x38e600, Win 0x45b840
{
    LuaLayout::OnLayoutApplied(layout, args);
    Rect frame(GetLeft(), GetTop(), GetRight(), GetBottom());
    bool parsed = false;
    {
        String value;
        if (layout->GetAttribute(String("frame"), value)) {
            UInt32 position = 0;
            parsed = FromStringOptional(frame, value, position);
        }
    }
    if (parsed)
        SetFrame(frame.m_min.x, frame.m_min.y, frame.m_max.x, frame.m_max.y);
}

void RegisterLuaFrameAugmentation(LuaState* state)                           // Mac 0x38deb0, Win 0x45b6e0
{
    LuaObject luaClass = CreateLuaClass(state, "FrameAugmentation", "ControlAugmentation");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("Create", LuaFrameAugmentationCreate, 0);
}

} // namespace LEGO
