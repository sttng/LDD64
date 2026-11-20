///////////////////////////////////////////////////////////////////////////////
// This source file is part of the LuaPlus source distribution and is Copyright
// 2001-2007 by Joshua C. Jensen (jjensen@workspacewhiz.com).
//
// The latest version may be obtained from http://wwhiz.com/LuaPlus/.
//
// The code presented in this file may be used in any environment it is
// acceptable to use Lua.
///////////////////////////////////////////////////////////////////////////////
#ifndef BUILDING_LUAPLUS
#define BUILDING_LUAPLUS
#endif
#include "LuaLink.h"
LUA_EXTERN_C_BEGIN
#include "src/lobject.h"
LUA_EXTERN_C_END
#include "LuaPlus.h"
#include "LuaCall.h"

//-----------------------------------------------------------------------------
namespace LuaPlus {

LuaCall::LuaCall(LuaObject& functionObj) :
	m_functionObj(functionObj),
	m_numArgs(0)
{
	luaplus_assert(m_functionObj.IsFunction());
	m_state = m_functionObj.GetState();
	m_startResults = m_state->GetTop() + 1;
	m_functionObj.Push();
}

LuaCall& operator<<(LuaCall& call, const LuaArgNil& /*value*/)
{
	call.m_state->PushNil();
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, float value)
{
	call.m_state->PushNumber(value);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, double value)
{
	call.m_state->PushNumber(value);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, int value)
{
	call.m_state->PushInteger(value);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, unsigned int value)
{
	call.m_state->PushInteger(value);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, const char* value)
{
	call.m_state->PushString(value);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, const lua_WChar* value)
{
	call.m_state->PushWString(value);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, lua_CFunction value)
{
	call.m_state->PushCClosure(value, 0);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, int (*value)(LuaState*))
{
	call.m_state->PushCClosure(value, 0);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, bool value)
{
	call.m_state->PushBoolean(value);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, void* value)
{
	call.m_state->PushLightUserData(value);
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, LuaStackObject& value)
{
	value.Push();
	call.m_numArgs++;
	return call;
}

LuaCall& operator<<(LuaCall& call, LuaObject& value)
{
	value.Push();
	call.m_numArgs++;
	return call;
}

/*
** Win 0x5d5cd0, Mac 0x404bba (Build 1100): the call runs through LuaState::PCall(nargs,
** nresults) (Win 0x5cecf0, Mac 0x3e20c6), whose error function hands the message to
** _ERRORREPORTER and returns no value. A failed call does not throw: the function and its
** arguments are gone from the stack and one nil (lua_pushnil, Win 0x5d2350 / Mac 0x3e5531)
** stands in as the result. The returned object is the first result's slot either way.
*/
LuaStackObject LuaCall::operator<<(const LuaRun& run)
{
	int resultsStackPos = m_state->GetTop() - m_numArgs;
	if (!m_state->PCall(m_numArgs, run.m_numResults))
		m_state->PushNil();
	return LuaStackObject(m_state, resultsStackPos);
}

LuaCall& LuaCall::operator=(const LuaCall& src)
{
	m_state = src.m_state;
	m_functionObj = src.m_functionObj;
	m_numArgs = src.m_numArgs;
	m_startResults = src.m_startResults;

	return *this;
}

} // namespace LuaPlus
