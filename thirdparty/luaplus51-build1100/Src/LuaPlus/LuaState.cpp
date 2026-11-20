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
LUA_EXTERN_C_END
#include "LuaPlus.h"
#include "LuaState.h"
#include "LuaCall.h"
#include "LuaPlusCD.h"

#if defined(_WIN32) && !defined(_XBOX) && !defined(_XBOX_VER)
#include <windows.h>
#undef GetObject
#undef LoadString
#elif defined(_XBOX) || defined(_XBOX_VER)
#include <xtl.h>
#endif // WIN32

#ifdef _MSC_VER
#pragma warning(disable: 4702)
#endif // _MSC_VER

#include <ctype.h>

LUA_EXTERN_C_BEGIN
#include "src/lstate.h"
#include "src/lvm.h"
#include "src/lgc.h"
LUA_EXTERN_C_END


#ifndef LUAPLUS_ENABLE_INLINES
#include "LuaState.inl"
#endif // LUAPLUS_ENABLE_INLINES

namespace LuaPlus {

LuaObject LuaState::GetGlobals() throw()
{
	return LuaObject( this, gt(LuaState_to_lua_State(this)) );
}

LuaStackObject LuaState::Stack(int index)
{
    return LuaStackObject(this, index);
}

LuaStackObject LuaState::StackTop()
{
    return LuaStackObject(this, GetTop());
}

LuaStackObject LuaState::PushVFString(const char *fmt, va_list argp)
{
	lua_State* L = LuaState_to_lua_State(this);
	lua_lock(L);
	luaC_checkGC(L);
	luaO_pushvfstring(L, fmt, argp);
	lua_unlock(L);
	return LuaStackObject(this, lua_gettop(LuaState_to_lua_State(this)));
}


LuaStackObject LuaState::PushFString(const char *fmt, ...)
{
	lua_State* L = LuaState_to_lua_State(this);
	va_list argp;
	lua_lock(L);
	luaC_checkGC(L);
	va_start(argp, fmt);
	luaO_pushvfstring(L, fmt, argp);
	va_end(argp);
	lua_unlock(L);
	return LuaStackObject(this, lua_gettop(LuaState_to_lua_State(this)));
}






int LuaState::Equal(const LuaObject& o1, const LuaObject& o2)
{
	int i = equalobj(o1.GetCState(), o1.GetTObject(), o2.GetTObject());
	return i;
}


int LuaState::LessThan(const LuaObject& o1, const LuaObject& o2)
{
	int i = luaV_lessthan(o1.GetCState(), o1.GetTObject(), o2.GetTObject());
	return i;
}


LuaObject LuaState::NewUserDataBox(void* u)
{
	LuaObject obj(this);
	obj.AssignUserData(this, u);
	return obj;
}


int LuaState::TypeError(int narg, const char* tname)
{
	return luaL_typerror(LuaState_to_lua_State(this), narg, tname);
}


int LuaState::ArgError(int narg, const char* extramsg)
{
	return luaL_argerror(LuaState_to_lua_State(this), narg, extramsg);
}


const char* LuaState::CheckLString(int numArg, size_t* len)
{
	return luaL_checklstring(LuaState_to_lua_State(this), numArg, len);
}


const char* LuaState::OptLString(int numArg, const char *def, size_t* len)
{
	return luaL_optlstring(LuaState_to_lua_State(this), numArg, def, len);
}


lua_Number LuaState::CheckNumber(int numArg)
{
	return luaL_checknumber(LuaState_to_lua_State(this), numArg);
}


lua_Number LuaState::OptNumber(int nArg, lua_Number def)
{
	return luaL_optnumber(LuaState_to_lua_State(this), nArg, def);
}


lua_Integer LuaState::CheckInteger(int numArg)
{
	return luaL_checkinteger(LuaState_to_lua_State(this), numArg);
}


lua_Integer LuaState::OptInteger(int nArg, lua_Integer def)
{
	return luaL_optinteger(LuaState_to_lua_State(this), nArg, def);
}


void LuaState::ArgCheck(bool condition, int numarg, const char* extramsg)
{
	luaL_argcheck(LuaState_to_lua_State(this), condition, numarg, extramsg);
}


const char* LuaState::CheckString(int numArg)
{
	return luaL_checkstring(LuaState_to_lua_State(this), numArg);
}


const char* LuaState::OptString(int numArg, const char* def)
{
	return luaL_optlstring(LuaState_to_lua_State(this), numArg, def, NULL);
}


int LuaState::CheckInt(int numArg)
{
	return (int)luaL_checkint(LuaState_to_lua_State(this), numArg);
}


long LuaState::CheckLong(int numArg)
{
	return (long)luaL_checklong(LuaState_to_lua_State(this), numArg);
}


int LuaState::OptInt(int numArg, int def)
{
	return (int)luaL_optint(LuaState_to_lua_State(this), numArg, def);
}


long LuaState::OptLong(int numArg, int def)
{
	return (long)luaL_optlong(LuaState_to_lua_State(this), numArg, def);
}


void LuaState::CheckStack(int sz, const char* msg)
{
	luaL_checkstack(LuaState_to_lua_State(this), sz, msg);
}


void LuaState::CheckType(int narg, int t)
{
	luaL_checktype(LuaState_to_lua_State(this), narg, t);
}


void LuaState::CheckAny(int narg)
{
	luaL_checkany(LuaState_to_lua_State(this), narg);
}


LuaStackObject LuaState::NewMetaTable(const char* tname)
{
	luaL_newmetatable(LuaState_to_lua_State(this), tname);
	return LuaStackObject(this, GetTop());
}

	
void* LuaState::CheckUData(int ud, const char* tname)
{
	return luaL_checkudata(LuaState_to_lua_State(this), ud, tname);
}


int LuaState::Where(int lvl)
{
	luaL_where(LuaState_to_lua_State(this), lvl);
	return LuaStackObject(this, GetTop());
}

	
const char* LuaState::GSub(const char *s, const char *p, const char *r)
{
	return luaL_gsub(LuaState_to_lua_State(this), s, p, r);
}


const char* LuaState::FindTable(int idx, const char *fname, int szhint)
{
	return luaL_findtable(LuaState_to_lua_State(this), idx, fname, szhint);
}


LuaObject LuaState::GetLocalByName( int level, const char* name )
{
	lua_State * L = GetCState();
	lua_Debug ar;
	int i;
	const char *localName;
	if (lua_getstack(L, level, &ar) == 0)
		return LuaObject(this);  /* failure: no such level in the stack */
	i = 1;
	while ((localName = lua_getlocal(L, &ar, i++)) != NULL) {
		if (strcmp(name, localName) == 0)
		{
			LuaObject obj(this, -1);
			lua_pop(L, 1);
			return obj;
		}
		lua_pop(L, 1);  /* remove variable value */
	}
	return LuaObject(this);
}


int LuaState::LoadWString(const lua_WChar* str)
{
	return luaL_loadwbuffer(LuaState_to_lua_State(this), str, lua_WChar_len(str), "name");
}

LuaStackObject LuaState::PushCClosure(int (*f)(LuaState*), int n)
{
	unsigned char* buffer = (unsigned char*)lua_newuserdata(LuaState_to_lua_State(this), sizeof(f));
	memcpy(buffer, &f, sizeof(f));
	Insert(-n-1);
	lua_pushcclosure(LuaState_to_lua_State(this), LPCD::LuaStateFunctionDispatcher, n + 1);
	return LuaStackObject(this, lua_gettop(LuaState_to_lua_State(this)));
}

int LuaState::UpValueIndex(int i)
{
	return lua_upvalueindex(i);
}

/* Win 0x5cf0c0 */
bool LuaState::LoadString(const char* str)
{
	return LoadBuffer(str, strlen(str), str);
}


/* Win 0x5cee30 */
bool LuaState::LoadBuffer(const char* buff, size_t size, const char* name)
{
	if (luaL_loadbuffer(LuaState_to_lua_State(this), buff, size, name) == 0)
		return true;
	ReportError();
	lua_settop(LuaState_to_lua_State(this), -2);
	return false;
}


/* Win 0x5cee80 */
bool LuaState::LoadWBuffer(const lua_WChar* buff, size_t size, const char* name)
{
	if (luaL_loadwbuffer(LuaState_to_lua_State(this), buff, size, name) == 0)
		return true;
	ReportError();
	lua_settop(LuaState_to_lua_State(this), -2);
	return false;
}


/* Win 0x5cede0, Mac 0x3e2... */
bool LuaState::DoString(const char *str)
{
	return LoadString(str)  &&  PCall(0, LUA_MULTRET);
}


/* Win 0x5cf340: error function of PCall(nargs, nresults); returns no value, so the error
   object seen by lua_pcall's caller is nil */
static int LuaPlus_ErrorHandler(lua_State* L)
{
	LuaState::CastState(L)->ReportError();
	return 0;
}


/* Win 0x5cecf0 */
bool LuaState::PCall(int nargs, int nresults)
{
	lua_State* L = LuaState_to_lua_State(this);
	lua_checkstack(L, 1);
	int base = lua_gettop(L) - nargs;
	lua_pushcclosure(L, LuaPlus_ErrorHandler, 0);
	lua_insert(L, base);
	bool ok = lua_pcall(L, nargs, nresults, base) == 0;
	if (!ok)
		lua_settop(L, base - 1);
	else
		lua_remove(L, base);
	return ok;
}


/*
** Win 0x5cf230, Mac 0x3e23a2 area: reports the message at stack index 1. A message that
** contains "\x1f" carries a location: the file name between the first pair of quotes and
** the line number after the following ':'; the text after the "\x1f" is the message.
*/
/*
** NOTE (LDD): a non-string error object (error({}), error(nil)) gives a null message that is
** searched; a marker without the quoted file name and ':' line searches from null as well
** (both builds); fixed: such a message is reported as "(error object is not a string)", and a
** marker is only parsed when the quotes and ':' are there.
*/
void LuaState::ReportError()
{
	const char* msg = lua_tostring(LuaState_to_lua_State(this), 1);
	if (msg == NULL)
		msg = "(error object is not a string)";
	const char* marker = strchr(msg, 0x1f);
	char* fileName = NULL;
	int line = 0;
	const char* open = marker ? strchr(msg, '"') : NULL;
	const char* close = open ? strchr(open + 1, '"') : NULL;
	const char* colon = close ? strchr(close + 1, ':') : NULL;
	if (marker && colon)
	{
		const char* start = strchr(msg, '"') + 1;
		const char* end = strchr(start, '"');
		fileName = new char[end - start + 1];
		strncpy(fileName, start, end - start);
		fileName[end - start] = 0;
		line = atoi(strchr(end + 1, ':') + 1);
		msg = marker + 1;
	}
	ReportError(msg, fileName, line);
	delete[] fileName;
}


/* Win 0x5cf200, Mac 0x3e2360 */
void LuaState::ReportError(const char* message, const char* fileName, int line)
{
	lua_pushstring(LuaState_to_lua_State(this), message);
	CallErrorReporter(fileName, line);
}


/* Win 0x5cf160, Mac 0x3e229e: _ERRORREPORTER(message, fileName, line) with the message on the
   stack top; without a reporter the message and the non-function are popped */
void LuaState::CallErrorReporter(const char* fileName, int line)
{
	lua_State* L = LuaState_to_lua_State(this);
	lua_getfield(L, LUA_GLOBALSINDEX, "_ERRORREPORTER");
	if (lua_type(L, -1) == LUA_TFUNCTION)
	{
		lua_insert(L, -2);
		lua_pushstring(L, fileName);
		lua_pushinteger(L, line);
		lua_call(L, 3, 0);
	}
	else
		lua_settop(L, -3);
}

int LuaState::DoString( const char *str, LuaObject& fenvObj )
{
	int status = luaL_loadbuffer(LuaState_to_lua_State(this), str, strlen(str), str);
	if (status == 0)
	{
		fenvObj.Push();
		SetFEnv(-2);
	}
	return aux_do(LuaState_to_lua_State(this), status);
}


LuaObject LuaState::GetGlobal(const char *name)
{
	return GetGlobals()[name];
}


LuaObject LuaState::GetRegistry()
{
	return LuaObject(this, LUA_REGISTRYINDEX);  //{  lua_getregistry(LuaState_to_lua_State(this));
}


int LuaState::DoFile( const char *filename, LuaObject& fenvObj )
{
	int status = luaL_loadfile(LuaState_to_lua_State(this), filename);
	if (status == 0)
	{
		fenvObj.Push();
		SetFEnv(-2);
	}
	return aux_do(LuaState_to_lua_State(this), status);
}


int LuaState::DoBuffer( const char *buff, size_t size, const char *name, LuaObject& fenvObj )
{
	int status = luaL_loadbuffer(LuaState_to_lua_State(this), buff, size, name);
	if (status == 0)
	{
		fenvObj.Push();
		SetFEnv(-2);
	}
	return aux_do(LuaState_to_lua_State(this), status);
}


namespace LuaHelper {

LUAPLUS_API void MergeObjects(LuaObject& mergeTo, LuaObject& mergeFrom, bool replaceDuplicates)
{
	if (mergeTo.GetState() == mergeFrom.GetState())
	{
		for (LuaTableIterator it(mergeFrom); it; ++it)
		{
			LuaObject toNodeKeyObj = mergeTo[it.GetKey()];
			if (it.GetValue().IsTable())
			{
				if (toNodeKeyObj.IsNil()  ||  replaceDuplicates)
				{
					toNodeKeyObj = mergeTo.CreateTable(it.GetKey());
				}
				MergeObjects(toNodeKeyObj, it.GetValue(), replaceDuplicates);
			}
			else if (toNodeKeyObj.IsNil()  ||  replaceDuplicates)
			{
				mergeTo.SetObject(it.GetKey(), it.GetValue());
			}
		}
	}
	else
	{
		for (LuaTableIterator it(mergeFrom); it; ++it)
		{
			LuaObject obj;
			switch (it.GetKey().Type())
			{
				case LUA_TBOOLEAN:	obj.AssignBoolean(mergeTo.GetState(), it.GetKey().GetBoolean());		break;
				case LUA_TNUMBER:	obj.AssignNumber(mergeTo.GetState(), it.GetKey().GetNumber());			break;
				case LUA_TSTRING:	obj.AssignString(mergeTo.GetState(), it.GetKey().GetString());			break;
				case LUA_TWSTRING:	obj.AssignWString(mergeTo.GetState(), it.GetKey().GetWString());		break;
			}

			LuaObject toNodeKeyObj = mergeTo[obj];

			if (it.GetValue().IsTable())
			{
				if (toNodeKeyObj.IsNil()  ||  replaceDuplicates)
				{
					toNodeKeyObj = mergeTo.CreateTable(it.GetKey());
				}
				MergeObjects(toNodeKeyObj, it.GetValue(), replaceDuplicates);
			}
			else if (toNodeKeyObj.IsNil()  ||  replaceDuplicates)
			{
				LuaObject toKeyObj;
				switch (it.GetKey().Type())
				{
					case LUA_TBOOLEAN:	toKeyObj.AssignBoolean(mergeTo.GetState(), it.GetKey().GetBoolean());		break;
					case LUA_TNUMBER:	toKeyObj.AssignNumber(mergeTo.GetState(), it.GetKey().GetNumber());			break;
					case LUA_TSTRING:	toKeyObj.AssignString(mergeTo.GetState(), it.GetKey().GetString());			break;
					case LUA_TWSTRING:	toKeyObj.AssignWString(mergeTo.GetState(), it.GetKey().GetWString());		break;
				}

				switch (it.GetValue().Type())
				{
					case LUA_TBOOLEAN:	mergeTo.SetBoolean(toKeyObj, it.GetValue().GetBoolean());	break;
					case LUA_TNUMBER:	mergeTo.SetNumber(toKeyObj, it.GetValue().GetNumber());		break;
					case LUA_TSTRING:	mergeTo.SetString(toKeyObj, it.GetValue().GetString());		break;
					case LUA_TWSTRING:	mergeTo.SetWString(toKeyObj, it.GetValue().GetWString());	break;
				}
			}
		}
	}
}

} // namespace LuaHelper

} // namespace LuaPlus


namespace LPCD
{
	void Push(lua_State* L, int (*value)(LuaState*))
	{
		LuaState* state = LuaState::CastState(L);
		state->PushCClosure(value, 0);
	}
}
