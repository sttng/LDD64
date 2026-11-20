///////////////////////////////////////////////////////////////////////////////
// This source file is part of the LuaPlus source distribution and is Copyright
// 2001-2005 by Joshua C. Jensen (jjensen@workspacewhiz.com).
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
#include "src/lgc.h"
LUA_EXTERN_C_END
#include "LuaPlus.h"
#include "LuaCall.h"
#include <string.h>
#if defined(_WIN32)
#if defined(_WIN32) && !defined(_XBOX) && !defined(_XBOX_VER) && !defined(_WIN32_WCE)
#include <windows.h>
#elif defined(_XBOX) || defined(_XBOX_VER)
#include <xtl.h>
#endif // WIN32
#undef GetObject
#endif // WIN32
#if defined(__GNUC__)
	#include <new>
#else
	#include <new.h>
#endif

#include <stdlib.h>
#include <wchar.h>
#include <assert.h>

#ifdef _MSC_VER
#pragma warning(disable: 4100)
#endif // _MSC_VER

//-----------------------------------------------------------------------------
LUA_EXTERN_C_BEGIN
#include "src/lualib.h"
#include "src/lstate.h"
#include "src/lualib.h"
#include "src/lfunc.h"
#include "src/lgc.h"
#include "src/lstate.h"
#include "src/lua.h"
#include "src/lauxlib.h"

/* Win 0x5cf360 */
static void *luaplus_alloc (void *ud, void *ptr, size_t osize, size_t nsize, const char* allocName, unsigned int flags) {
  (void)allocName;
  (void)flags;
  (void)osize;
  (void)ud;
  if (nsize == 0) {
    free(ptr);
    return NULL;
  }
  else
    return realloc(ptr, nsize);
}


static lua_Alloc luaHelper_defaultAlloc = luaplus_alloc;
static void* luaHelper_ud = NULL;

NAMESPACE_LUA_BEGIN

/* Win 0x5cf390 */
void lua_getdefaultallocfunction(lua_Alloc* allocFunc, void** ud)
{
	*allocFunc = luaHelper_defaultAlloc;
	*ud = luaHelper_ud;
}


void lua_setdefaultallocfunction(lua_Alloc allocFunc, void* ud)
{
	luaHelper_defaultAlloc = allocFunc ? allocFunc : luaplus_alloc;
	luaHelper_ud = ud;
}

void reallymarkobject (global_State *g, GCObject *o);

#define markvalue(g,o) { checkconsistency(o); \
  if (iscollectable(o) && iswhite(gcvalue(o))) reallymarkobject(g,gcvalue(o)); }

#define markobject(g,t) { if (iswhite(obj2gco(t))) \
		reallymarkobject(g, obj2gco(t)); }

/* Win 0x5cf840: marks the value of every live LuaObject (called by markroot and atomic) */
void LuaPlusGCFunction(void* s)
{
	lua_State* L = (lua_State*)s;
	LuaPlus::LuaState* state = lua_State_To_LuaState(L);
	if (!state)
		return;

    global_State* g = G(L);

	LuaPlus::LuaObject* curObj = (LuaPlus::LuaObject*)G(L)->gchead_next;
	while (curObj != (LuaPlus::LuaObject*)&G(L)->gctail_next)
	{
		markvalue(g, curObj->GetTObject());
		curObj = *(LuaPlus::LuaObject**)curObj;
	}
}

/*
** LuaPlus Build 1100: every lua_State owns a heap-allocated LuaPlus::LuaState object
** (8 bytes: lua_State* m_state, bool m_ownState), allocated with the default allocator
** under the name "LuaState" and kept in lua_State.stateUserData (luai_userstate* hooks,
** luaconf.h). The constructor installs LuaPlusGCFunction.
*/
/* Win 0x5cf3b0 -> 0x5cf4f0 */
void LuaState_UserStateOpen(lua_State* L)
{
	lua_Alloc allocFunc;
	void* ud;
	lua_getdefaultallocfunction(&allocFunc, &ud);
	void* mem = (*allocFunc)(ud, NULL, 0, sizeof(LuaPlus::LuaState), "LuaState", 0);
	if (mem)
		new(mem) LuaPlus::LuaState(L);
}


/* Win 0x5cf3d0 -> 0x5cf560 */
void LuaState_UserStateThread(lua_State* L, lua_State* L1)
{
	lua_Alloc allocFunc;
	void* ud;
	lua_getdefaultallocfunction(&allocFunc, &ud);
	void* mem = (*allocFunc)(ud, NULL, 0, sizeof(LuaPlus::LuaState), "LuaState", 0);
	if (mem)
		new(mem) LuaPlus::LuaState(L, L1);
}


/* Win 0x5cf3f0 -> 0x5cf5d0: frees with the allocator of the lua_State (not the default one) */
void LuaState_UserStateFree(lua_State* L)
{
	LuaPlus::LuaState* state = lua_State_To_LuaState(L);
	if (state)
	{
		void* ud;
		lua_Alloc allocFunc = lua_getallocf(LuaState_to_lua_State(state), &ud);
		state->~LuaState();
		(*allocFunc)(ud, state, sizeof(LuaPlus::LuaState), 0, NULL, 0);
	}
}


NAMESPACE_LUA_END

LUA_EXTERN_C_END

static int FatalError( lua_State* state );

namespace LuaPlus
{

USING_NAMESPACE_LUA

LuaException::LuaException(const char* message)
	: m_message(NULL)
{
	if (message)
	{
		m_message = new char[strlen(message) + 1];
		strcpy(m_message, message);
	}
}


LuaException::LuaException(const LuaException& src)
{
    m_message = new char[strlen(src.m_message) + 1];
    strcpy(m_message, src.m_message);
}


LuaException& LuaException::operator=(const LuaException& src)
{
    delete[] m_message;
    m_message = new char[strlen(src.m_message) + 1];
    strcpy(m_message, src.m_message);
    return *this;
}


LuaException::~LuaException()
{
    delete[] m_message;
}

/* Win 0x5cf740 (Mac 0x3e0e22 area) */
LuaState::LuaState(lua_State* L)
{
	m_state = L;
	m_ownState = false;
	lua_setstateuserdata(m_state, this);
	lua_setusergcfunction(m_state, LuaPlusGCFunction);
}


/* Win 0x5cf790: object of a new thread L1 (parentState unused) */
LuaState::LuaState(lua_State* /*parentState*/, lua_State* L1)
{
	m_state = L1;
	m_ownState = false;
	lua_setstateuserdata(m_state, this);
	lua_setusergcfunction(m_state, LuaPlusGCFunction);
}


/* Win 0x5cf7e0 */
LuaState::~LuaState()
{
	lua_setstateuserdata(m_state, NULL);
	m_ownState = false;
	m_state = NULL;
}


/* Win 0x5cf660, Mac 0x3e0eb6 */
/*static*/ LuaState* LuaState::Create()
{
	lua_State* L = lua_newstate(luaHelper_defaultAlloc, luaHelper_ud);
	LuaState* state = lua_State_To_LuaState(L);
	state->m_ownState = true;
	lua_atpanic(L, FatalError);
	return state;
}


/* Win 0x5cf700 */
/*static*/ LuaState* LuaState::CastState( lua_State* L )
{
	return lua_State_To_LuaState(L);
}


/* Win 0x5cf720: the LuaState object itself is freed by lua_close (luai_userstateclose) */
/*static*/ void LuaState::Destroy( LuaState* state )
{
	if (state->m_ownState)
		lua_close(state->m_state);
}

} // namespace LuaPlus

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4702)
#endif /* _MSC_VER */

/**
**/
/* Win 0x5cf6b0, Mac 0x3e113e */
static int FatalError( lua_State* state )
{
	const char* err = lua_tostring(state, 1);
#if defined(_WIN32)
	if (err)
		OutputDebugStringA(err);
#else // !WIN32
	if (err)
		puts(err);
#endif // WIN32

#ifndef _WIN32_WCE
	luaplus_throw(err);
#endif // _WIN32_WCE

	return -1;
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif /* _MSC_VER */
