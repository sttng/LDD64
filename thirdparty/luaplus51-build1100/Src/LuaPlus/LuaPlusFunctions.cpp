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
#if defined(_WIN32) && !defined(_XBOX) && !defined(_XBOX_VER)
#include <windows.h>
#elif defined(_XBOX) || defined(_XBOX_VER)
#include <xtl.h>
#endif // WIN32

#if defined(_WIN32)
#ifndef _WIN32_WCE
#include <io.h>
#endif // _WIN32_WCE
#endif // WIN32

#include "LuaLink.h"
LUA_EXTERN_C_BEGIN
#include "src/lobject.h"
LUA_EXTERN_C_END

#include "LuaPlus.h"
LUA_EXTERN_C_BEGIN
#include "src/lua.h"
#include "src/lauxlib.h"
LUA_EXTERN_C_END

#include <stdlib.h>
#include <assert.h>

USING_NAMESPACE_LUA

#ifdef _MSC_VER
#pragma warning( disable: 4100 )
#endif // _MSC_VER

namespace LuaPlus {

int LS_LuaDumpObject( LuaState* state );
int LS_LuaDumpFile( LuaState* state );
int LS_LuaDumpGlobals(LuaState* state);

#if defined(_WIN32)
static unsigned __int64 s_ticksPerSecond;
static float s_secondsPerTick;
#endif

static int LS_GetTickCount( LuaState* state )
{
#if defined(_WIN32)
	if (s_ticksPerSecond == 0)
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&s_ticksPerSecond);
		s_secondsPerTick = 1.0f / (float) (__int64)s_ticksPerSecond;
	}

	unsigned __int64 ticks;
	QueryPerformanceCounter((LARGE_INTEGER*)&ticks);
	state->PushNumber((float)(__int64)ticks * s_secondsPerTick);	// float product (Win 0x5e2b20)
#else
	state->PushNumber(0);
#endif
	return 1;
}


/* Win 0x5e26e0, Mac 0x3e162c (name inferred): package.path as luaopen_package computes it */
static const char* GetLuaPath(lua_State* L)
{
	const char* path;
	lua_getglobal(L, "LUA_PATH");
	path = lua_tostring(L, -1);
	lua_pop(L, 1);
	if (!path)
	{
		path = getenv("LUA_PATH");
		if (!path)
			path = LUA_PATH_DEFAULT;
	}
	return path;
}


/*
** Win 0x5e2750, Mac 0x3e16b4. LuaPlus Build 1100: GetTickCount, plus the "modules" directory
** next to LuaPlus_1100.dll (Win: the executable's directory when linked statically; Mac: an
** empty prefix) for C modules (LUA_SOPATH / LUA_CPATH environment, package.cpath) and Lua
** modules (package.path).
*/
void ScriptFunctionsRegister(LuaState* state)
{
	LuaObject globalsObj = state->GetGlobals();
	globalsObj.Register("GetTickCount",		LS_GetTickCount);

#if defined(_WIN32)
	const char* luaplusdllName = "LuaPlus_1100.dll";
	char modulePath[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(luaplusdllName), modulePath, MAX_PATH);
	char* slashPtr = strrchr(modulePath, '\\');
	slashPtr++;
	*slashPtr = 0;
#else
	char modulePath[300];
	modulePath[0] = 0;
#endif
	const char* luaPath = GetLuaPath(state->GetCState());	// unused (as in the original)
	(void)luaPath;

	/* NOTE (LDD): putenv gets a Lua string that is popped (and collected) straight away (both
	   builds). Windows' _putenv copies it; POSIX putenv keeps the pointer, so on the Mac the
	   environment then points into freed memory, read by the next getenv("LUA_CPATH") in
	   luaopen_package; fixed: setenv, which copies, outside Windows. */
#if !defined(_WIN32)
	state->PushString(modulePath);
	state->PushString("modules/");
	state->Concat(2);
	setenv("LUA_SOPATH", LuaObject(state, -1).GetString(), 1);
	state->Pop();

	state->PushString(modulePath);
	state->PushString("modules/");
	state->PushString("?.dll");
	state->Concat(3);
	setenv("LUA_CPATH", LuaObject(state, -1).GetString(), 1);
	state->Pop();
#else
	state->PushString("LUA_SOPATH=");
	state->PushString(modulePath);
	state->PushString("modules/");
	state->Concat(3);
	putenv((char*)LuaObject(state, -1).GetString());
	state->Pop();

	state->PushString("LUA_CPATH=");
	state->PushString(modulePath);
	state->PushString("modules/");
	state->PushString("?.dll");
	state->Concat(4);
	putenv((char*)LuaObject(state, -1).GetString());
	state->Pop();
#endif

	{
		LuaObject packageObj = state->GetGlobal("package");
		LuaObject pathObj = packageObj["path"];
		state->PushString(pathObj.GetString());
		state->PushString(";");
		state->PushString(modulePath);
		state->PushString("modules/?.lua");
		state->Concat(4);
		packageObj.SetString("path", LuaObject(state, -1).GetString());
		state->Pop();
	}

	{
		LuaObject packageObj = state->GetGlobal("package");
		LuaObject cpathObj = packageObj["cpath"];
		state->PushString(modulePath);
		state->PushString("modules/");
		state->PushString("?.dll");
		state->PushString(";");
		state->PushString(cpathObj.GetString());
		state->Concat(5);
		packageObj.SetString("cpath", LuaObject(state, -1).GetString());
		state->Pop();
	}
}

} // namespace LuaPlus






/*
--[[function io.lines(file, openType)
	if type( file ) == "string" then
		if file:sub( 1, 1 ) == "|" then
			file = io.popen( file:sub( 2 ), openType )
		else
			file = io.open( file, openType )
		end
	end

	return function ()
		return file:read() or (assert(file:close()) and nil), i
	end
end


function DirProcessFiles(path, wildcard, func)
	if type(func) ~= "function" then
		return
	end

	path = path:gsub("/", "\\")
	if path:sub(path:len()) ~= '\\' then
		path = path .. "\\"
	end

	local handle = FileFind.First(path .. wildcard)

	while 1 do
		local fileName = handle:GetFileName()
		local isDirectory = handle:IsDirectory()
		func(path, fileName)
		if not handle:Next() then
			break
		end
	end

	handle:Close()
end


function DirProcessFilesRecursive(path, func, ext)
	if ext then
		ext = ext:lower()
	end

	if type(func) ~= "function" then
		return
	end

	path = path:gsub("/", "\\")
	if path:sub(path:len()) ~= '\\' then
		path = path .. "\\"
	end

	function ReadDirectory(path)
		local handle = FileFind.First(path .. "*.*")

		while handle do
			local fileName = handle:GetFileName()
			local isDirectory = handle:IsDirectory()
			if isDirectory then
				if fileName ~= "." and fileName ~= ".." then
					ReadDirectory(path .. fileName .. "\\")
				end
			else
				if ext then
					local dotPos, endPos, foundExt = fileName:find("%.(%w*)$")
					if foundExt then
						foundExt = foundExt:lower()
						if ext == foundExt then
							func(path, fileName)
						end
					end
				else
					func(path, fileName)
				end
			end
			if not handle:Next() then
				break
			end
		end

		if handle then
			handle:Close()
		end
	end

	ReadDirectory(path)
end
]]
*/
