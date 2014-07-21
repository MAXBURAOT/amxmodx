/**
* vim: set ts=4 :
* =============================================================================
* SourceMod
* Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*
* As a special exception, AlliedModders LLC gives you permission to link the
* code of this program (as well as its derivative works) to "Half-Life 2," the
* "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
* by the Valve Corporation.  You must obey the GNU General Public License in
* all respects for all other code used.  Additionally, AlliedModders LLC grants
* this exception to all derivative works.  AlliedModders LLC defines further
* exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
* or <http://www.sourcemod.net/license.php>.
*
* Version: $Id$
*/

#include "amxmodx.h"
#include <textparse.h>
#include <am-vector.h>

TextParserHandles<ParseInfo> g_TextParsersHandles;

static cell AMX_NATIVE_CALL SMC_CreateParser(AMX *amx, cell *params)
{
	return static_cast<cell>(g_TextParsersHandles.create());
}

static cell AMX_NATIVE_CALL SMC_SetParseStart(AMX *amx, cell *params)
{
	ParseInfo *p = g_TextParsersHandles.lookup(params[1]);

	if (p == NULL)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid map handle provided (%d)", params[1]);
		return 0;
	}

	int length;
	const char* functionName = get_amxstring(amx, params[2], 0, length);
	int function = registerSPForwardByName(amx, functionName, FP_CELL, FP_DONE);

	if (function == -1)
	{
		LogError(amx, AMX_ERR_NATIVE, "Function is not present (function \"%s\") (plugin \"%s\")", functionName, g_plugins.findPluginFast(amx)->getName());
		return 0;
	}

	p->parse_start = function;

	return 1;
}

static cell AMX_NATIVE_CALL SMC_SetParseEnd(AMX *amx, cell *params)
{
	ParseInfo *p = g_TextParsersHandles.lookup(params[1]);

	if (p == NULL)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid map handle provided (%d)", params[1]);
		return 0;
	}

	int length;
	const char* funcName = get_amxstring(amx, params[2], 0, length);
	int func = registerSPForwardByName(amx, funcName, FP_CELL, FP_DONE);

	if (func == -1)
	{
		LogError(amx, AMX_ERR_NATIVE, "Function is not present (function \"%s\") (plugin \"%s\")", funcName, g_plugins.findPluginFast(amx)->getName());
		return 0;
	}

	p->parse_end = func;

	return 1;
}

static cell AMX_NATIVE_CALL SMC_SetReaders(AMX *amx, cell *params)
{
	ParseInfo *p = g_TextParsersHandles.lookup(params[1]);

	if (p == NULL)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid map handle provided (%d)", params[1]);
		return 0;
	}

	int length;
	const char* NewSectionFuncName = get_amxstring(amx, params[2], 0, length);
	const char* KeyValueFuncName   = get_amxstring(amx, params[3], 1, length);
	const char* EndSectionFuncName = get_amxstring(amx, params[4], 2, length);

	int NewSectionFunc = registerSPForwardByName(amx, NewSectionFuncName, FP_CELL, FP_STRING, FP_DONE);
	if (NewSectionFunc == -1)
	{
		LogError(amx, AMX_ERR_NATIVE, "Function is not present (function \"%s\") (plugin \"%s\")", NewSectionFuncName, g_plugins.findPluginFast(amx)->getName());
		return 0;
	}

	int KeyValueFunc = registerSPForwardByName(amx, KeyValueFuncName, FP_CELL, FP_STRING, FP_STRING, FP_DONE);
	if (KeyValueFunc == -1)
	{
		LogError(amx, AMX_ERR_NATIVE, "Function is not present (function \"%s\") (plugin \"%s\")", KeyValueFuncName, g_plugins.findPluginFast(amx)->getName());
		return 0;
	}

	int EndSectionFunc = registerSPForwardByName(amx, EndSectionFuncName, FP_CELL, FP_DONE);
	if (EndSectionFunc == -1)
	{
		LogError(amx, AMX_ERR_NATIVE, "Function is not present (function \"%s\") (plugin \"%s\")", EndSectionFuncName, g_plugins.findPluginFast(amx)->getName());
		return 0;
	}

	p->new_section = NewSectionFunc;
	p->key_value   = KeyValueFunc;
	p->end_section = EndSectionFunc;

	return 1;
}

static cell AMX_NATIVE_CALL SMC_SetRawLine(AMX *amx, cell *params)
{
	ParseInfo *p = g_TextParsersHandles.lookup(params[1]);

	if (p == NULL)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid map handle provided (%d)", params[1]);
		return 0;
	}

	int length;
	const char* funcName = get_amxstring(amx, params[2], 0, length);

	int func = registerSPForwardByName(amx, funcName, FP_CELL, FP_STRING, FP_CELL, FP_DONE);
	if (func == -1)
	{
		LogError(amx, AMX_ERR_NATIVE, "Function is not present (function \"%s\") (plugin \"%s\")", funcName, g_plugins.findPluginFast(amx)->getName());
		return 0;
	}

	p->raw_line = func;

	return 1;
}

static cell AMX_NATIVE_CALL SMC_ParseFile(AMX *amx, cell *params)
{
	ParseInfo *p = g_TextParsersHandles.lookup(params[1]);

	if (p == NULL)
	{
		LogError(amx, AMX_ERR_NATIVE, "Invalid map handle provided (%d)", params[1]);
		return 0;
	}

	int length;
	const char *file = build_pathname("%s", get_amxstring(amx, params[2], 0, length));

	SMCStates states;
	SMCError p_err = textparsers->ParseFile_SMC(file, p, &states);

	*get_amxaddr(amx, params[3]) = states.line;
	*get_amxaddr(amx, params[4]) = states.col;

	return (cell)p_err;
}

static cell AMX_NATIVE_CALL SMC_GetErrorString(AMX *amx, cell *params)
{
	const char *str = textparsers->GetSMCErrorString((SMCError)params[1]);

	if (!str)
	{
		return 0;
	}

	return set_amxstring(amx, params[2], str, params[3]);
}

static cell AMX_NATIVE_CALL SMC_DestroyParser(AMX *amx, cell *params)
{
	cell *ptr = get_amxaddr(amx, params[1]);

	ParseInfo *p = g_TextParsersHandles.lookup(*ptr);

	if (p == NULL)
	{
		return 0;
	}

	if (g_TextParsersHandles.destroy(*ptr))
	{
		*ptr = 0;
		return 1;
	}

	return 0;
}

AMX_NATIVE_INFO g_TextParserNatives[] =
{
	{ "SMC_CreateParser"  , SMC_CreateParser   },
	{ "SMC_ParseFile"     , SMC_ParseFile      },
	{ "SMC_GetErrorString", SMC_GetErrorString },
	{ "SMC_SetParseStart" , SMC_SetParseStart  },
	{ "SMC_SetParseEnd"   , SMC_SetParseEnd    },
	{ "SMC_SetReaders"    , SMC_SetReaders     },
	{ "SMC_SetRawLine"    , SMC_SetRawLine     },
	{ "SMC_DestroyParser" , SMC_DestroyParser  },
	{ NULL, NULL },
};
