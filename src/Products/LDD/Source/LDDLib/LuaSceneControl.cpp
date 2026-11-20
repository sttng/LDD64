// LEGO Digital Designer — restored source
// TU 278 "LuaSceneControl.cpp": Mac 0x291050-0x291c9b, Win 0x605c47-0x60625f.
#include "LuaSceneControl.h"
#include "LuaBindings.h"
#include "BrickConnectSceneControl.h"
#include "ControlManager.h"
#include "Camera.h"
#include "Geometry.h"
#include "MathUtil.h"

#include <typeinfo>

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:ResetCamera() (name = Lua string)
int LuaSceneControlResetCamera(lua_State* L)                                 // Mac 0x291120, Win 0x605fd6
{
    BrickConnectSceneControl* control = CheckLuaSceneControl(LuaState::CastState(L), 1);
    control->ResetCamera();
    if (!control->GetManager()->IsUpdateLocked())                            // (Win: inline 0x44b400)
        control->Invalidate(false);
    return 0;
}

// self:RotateLeftRight(angle)
int LuaSceneControlRotateLeftRight(lua_State* L)                             // Mac 0x291170, Win 0x605fff
{
    BrickConnectSceneControl* control = CheckLuaSceneControl(LuaState::CastState(L), 1);
    control->RotateCamera((Real32)luaL_checknumber(L, 2));
    return 0;
}

// self:RotateUpDown(angle)
int LuaSceneControlRotateUpDown(lua_State* L)                                // Mac 0x2911c0, Win 0x606038
{
    BrickConnectSceneControl* control = CheckLuaSceneControl(LuaState::CastState(L), 1);
    control->RotateCameraVertical((Real32)luaL_checknumber(L, 2));
    return 0;
}

// self:ZoomInOut(factor): zooms by factor × max(distance(eye, target) / 1000, 0.032).
int LuaSceneControlZoomInOut(lua_State* L)                                   // Mac 0x291210, Win 0x606071
{
    BrickConnectSceneControl* control = CheckLuaSceneControl(LuaState::CastState(L), 1);
    Vector3 eye;
    Matrix3D cameraToWorld;
    cameraToWorld.SetInverse(control->GetCamera()->GetViewMatrix());
    eye = cameraToWorld.GetTranslation();
    Vector3 toTarget = eye - control->GetCameraTarget();
    Real32 root = Sqrt(toTarget.GetLength());
    Real32 step = root * root / 1000.0f;                                     // NOTE: sqrt then square (both builds)
    Real32 scale = 0.032f;
    if (0.032f <= step)
        scale = step;
    Real32 factor = (Real32)luaL_checknumber(L, 2);
    control->ZoomCamera(scale * factor);
    return 0;
}

const luaL_Reg kFunctions[] = {                                             // Mac 0x73f050
    { "ResetCamera", LuaSceneControlResetCamera },
    { "RotateLeftRight", LuaSceneControlRotateLeftRight },
    { "RotateUpDown", LuaSceneControlRotateUpDown },
    { "ZoomInOut", LuaSceneControlZoomInOut },
    { 0, 0 }
};

} // namespace

BrickConnectSceneControl* CheckLuaSceneControl(LuaState* state, int narg)   // Mac 0x291b50, Win 0x605f94
{
    BrickConnectSceneControl* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaSceneControl* luaControl = bound ? dynamic_cast<LuaSceneControl*>(bound) : 0;
        if (luaControl)
            result = luaControl->GetSceneControl();
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(BrickConnectSceneControl).name());
    return result;
}

LuaObject LuaSceneControl::CreateLuaObject(Control* control, LuaState* state) // Mac 0x291310, Win 0x605ee9
{
    RefPointer<LuaSceneControl> object;
    return CreateLuaControlTable<LuaSceneControl>(control, state, "SceneControl", object);
}

bool LuaSceneControl::Attach(Control* control, const String& name)          // Mac 0x291950, Win 0x605ddd
{
    if (!LuaControl::Attach(control, name))
        return false;
    Control* target = GetTargetControl();
    m_sceneControl = target ? dynamic_cast<BrickConnectSceneControl*>(target) : 0;
    return true;
}

void LuaSceneControl::Detach()                                               // Mac 0x291a30
{
    m_sceneControl = 0;
    LuaControl::Detach();
}

void RegisterLuaSceneControl(LuaState* state)                                // Mac 0x291050, Win 0x606146
{
    LuaObject luaClass = CreateLuaClass(state, "SceneControl", "Control");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
