//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name ccl_missile.c	-	The missile-type ccl functions. */
//
//      (c) Copyright 2002-2003 by Lutz Sammer
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; version 2 dated June, 1991.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//      $Id$

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "video.h"
#include "tileset.h"
#include "unittype.h"
#include "missile.h"
#include "ccl_sound.h"
#include "ccl.h"

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

#ifdef DEBUG
extern int NoWarningMissileType; /// quiet ident lookup.
#endif

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Parse missile-type.
**
**  @param list  List describing missile-type.
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclDefineMissileType(SCM list)
{
	SCM value;
	char* str;
	MissileType* mtype;
	unsigned i;

	// Slot identifier

	str = gh_scm2newstr(gh_car(list), NULL);
	list = gh_cdr(list);
#ifdef DEBUG
	i = NoWarningMissileType;
	NoWarningMissileType = 1;
#endif
	mtype = MissileTypeByIdent(str);
#ifdef DEBUG
	NoWarningMissileType = i;
#endif
	if (mtype) {
		DebugLevel0Fn("Redefining missile-type `%s'\n" _C_ str);
		free(str);
	} else {
		mtype = NewMissileTypeSlot(str);  // str consumed!
	}

	mtype->NumDirections = 1;
	// Ensure we don't divide by zero.
	mtype->SplashFactor = 100;
	//
	// Parse the arguments, already the new tagged format.
	//
	while (!gh_null_p(list)) {
		value = gh_car(list);
		list = gh_cdr(list);
		if (gh_eq_p(value, gh_symbol2scm("file"))) {
			free(mtype->File);
			mtype->File = gh_scm2newstr(gh_car(list), NULL);
		} else if (gh_eq_p(value, gh_symbol2scm("size"))) {
			value = gh_car(list);
			mtype->Width = gh_scm2int(gh_car(value));
			value = gh_cdr(value);
			mtype->Height = gh_scm2int(gh_car(value));
		} else if (gh_eq_p(value,gh_symbol2scm("frames"))) {
			mtype->SpriteFrames = gh_scm2int(gh_car(list));
		} else if (gh_eq_p(value, gh_symbol2scm("num-directions"))) {
			mtype->NumDirections = gh_scm2int(gh_car(list));
		} else if (gh_eq_p(value, gh_symbol2scm("fired-sound"))) {
			free(mtype->FiredSound.Name);
			mtype->FiredSound.Name = gh_scm2newstr(gh_car(list), NULL);
		} else if (gh_eq_p(value, gh_symbol2scm("impact-sound"))) {
			free(mtype->ImpactSound.Name);
			mtype->ImpactSound.Name = gh_scm2newstr(gh_car(list), NULL);
		} else if (gh_eq_p(value, gh_symbol2scm("class"))) {
			const char* name;

			value = gh_car(list);
			name = get_c_string(value);
			for (i = 0; MissileClassNames[i]; ++i) {
				if (!strcmp(name, MissileClassNames[i])) {
					mtype->Class = i;
					break;
				}
			}
			if (!MissileClassNames[i]) {
				// FIXME: this leaves a half initialized missile-type
				errl("Unsupported class", value);
			}
		} else if (gh_eq_p(value, gh_symbol2scm("num-bounces"))) {
			mtype->NumBounces = gh_scm2int(gh_car(list));
		} else if (gh_eq_p(value, gh_symbol2scm("delay"))) {
			mtype->StartDelay = gh_scm2int(gh_car(list));
		} else if (gh_eq_p(value, gh_symbol2scm("sleep"))) {
			mtype->Sleep = gh_scm2int(gh_car(list));
		} else if (gh_eq_p(value, gh_symbol2scm("speed"))) {
			mtype->Speed = gh_scm2int(gh_car(list));
		} else if (gh_eq_p(value, gh_symbol2scm("draw-level"))) {
			mtype->DrawLevel = gh_scm2int(gh_car(list));
		} else if (gh_eq_p(value, gh_symbol2scm("range"))) {
			mtype->Range = gh_scm2int(gh_car(list));
		} else if (gh_eq_p(value, gh_symbol2scm("impact-missile"))) {
			free(mtype->ImpactName);
			mtype->ImpactName = gh_scm2newstr(gh_car(list), NULL);
		} else if (gh_eq_p(value, gh_symbol2scm("smoke-missile"))) {
			free(mtype->ImpactName);
			mtype->SmokeName = gh_scm2newstr(gh_car(list), NULL);
		} else if (gh_eq_p(value, gh_symbol2scm("can-hit-owner"))) {
			mtype->CanHitOwner = 1;
		} else if (gh_eq_p(value, gh_symbol2scm("friendly-fire"))) {
			mtype->FriendlyFire = 1;
		} else if (gh_eq_p(value, gh_symbol2scm("splash-factor"))) {
			mtype->SplashFactor = gh_scm2int(gh_car(list));;
		} else {
			// FIXME: this leaves a half initialized missile-type
			errl("Unsupported tag", value);
		}
		list = gh_cdr(list);
	}

	return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclDefineMissileType(lua_State* l)
{
	const char* value;
	char* str;
	MissileType* mtype;
	unsigned i;
	int args;
	int j;

	args = lua_gettop(l);
	j = 0;

	// Slot identifier

	str = strdup(LuaToString(l, j + 1));
	++j;
#ifdef DEBUG
	i = NoWarningMissileType;
	NoWarningMissileType = 1;
#endif
	mtype = MissileTypeByIdent(str);
#ifdef DEBUG
	NoWarningMissileType = i;
#endif
	if (mtype) {
		DebugLevel0Fn("Redefining missile-type `%s'\n" _C_ str);
		free(str);
	} else {
		mtype = NewMissileTypeSlot(str);  // str consumed!
	} 

	mtype->NumDirections = 1;
	// Ensure we don't divide by zero.
	mtype->SplashFactor = 100;
	//
	// Parse the arguments, already the new tagged format.
	//
	for (; j < args; ++j) {
		value = LuaToString(l, j + 1);
		++j;
		if (!strcmp(value, "file")) {
			free(mtype->File);
			mtype->File = strdup(LuaToString(l, j + 1));
		} else if (!strcmp(value, "size")) {
			if (!lua_istable(l, j + 1)) {
				lua_pushstring(l, "incorrect argument");
				lua_error(l);
			}
			lua_rawgeti(l, j + 1, 1);
			mtype->Width = LuaToNumber(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, j + 1, 2);
			mtype->Height = LuaToNumber(l, -1);
			lua_pop(l, 1);
		} else if (!strcmp(value,"frames")) {
			mtype->SpriteFrames = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "num-directions")) {
			mtype->NumDirections = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "fired-sound")) {
			free(mtype->FiredSound.Name);
			mtype->FiredSound.Name = strdup(LuaToString(l, j + 1));
		} else if (!strcmp(value, "impact-sound")) {
			free(mtype->ImpactSound.Name);
			mtype->ImpactSound.Name = strdup(LuaToString(l, j + 1));
		} else if (!strcmp(value, "class")) {
			value = LuaToString(l, j + 1);
			for (i = 0; MissileClassNames[i]; ++i) {
				if (!strcmp(value, MissileClassNames[i])) {
					mtype->Class = i;
					break;
				}
			}
			if (!MissileClassNames[i]) {
				// FIXME: this leaves a half initialized missile-type
				lua_pushfstring(l, "Unsupported class: %s", value);
				lua_error(l);
			}
		} else if (!strcmp(value, "num-bounces")) {
			mtype->NumBounces = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "delay")) {
			mtype->StartDelay = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "sleep")) {
			mtype->Sleep = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "speed")) {
			mtype->Speed = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "draw-level")) {
			mtype->DrawLevel = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "range")) {
			mtype->Range = LuaToNumber(l, j + 1);
		} else if (!strcmp(value, "impact-missile")) {
			free(mtype->ImpactName);
			mtype->ImpactName = strdup(LuaToString(l, j + 1));
		} else if (!strcmp(value, "smoke-missile")) {
			free(mtype->ImpactName);
			mtype->SmokeName = strdup(LuaToString(l, j + 1));
		} else if (!strcmp(value, "can-hit-owner")) {
			mtype->CanHitOwner = 1;
		} else if (!strcmp(value, "friendly-fire")) {
			mtype->FriendlyFire = 1;
		} else if (!strcmp(value, "splash-factor")) {
			mtype->SplashFactor = LuaToNumber(l, j + 1);
		} else {
			// FIXME: this leaves a half initialized missile-type
			lua_pushfstring(l, "Unsupported tag: %s", value);
			lua_error(l);
		}
	}

	return 0;
}
#endif

/**
**  Define missile type mapping from original number to internal symbol
**
**  @param list  List of all names.
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclDefineMissileTypeWcNames(SCM list)
{
	int i;
	char** cp;

	if ((cp = MissileTypeWcNames)) {  // Free all old names
		while (*cp) {
			free(*cp++);
		}
		free(MissileTypeWcNames);
	}

	//
	// Get new table.
	//
	i = gh_length(list);
	MissileTypeWcNames = cp = malloc((i + 1) * sizeof(char*));
	while (i--) {
		*cp++ = gh_scm2newstr(gh_car(list), NULL);
		list = gh_cdr(list);
	}
	*cp = NULL;

	return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclDefineMissileTypeWcNames(lua_State* l)
{
	int i;
	int j;
	char** cp;

	if ((cp = MissileTypeWcNames)) {  // Free all old names
		while (*cp) {
			free(*cp++);
		}
		free(MissileTypeWcNames);
	}

	//
	// Get new table.
	//
	i = lua_gettop(l);
	MissileTypeWcNames = cp = malloc((i + 1) * sizeof(char*));
	if (!cp) {
		fprintf(stderr, "out of memory.\n");
		ExitFatal(-1);
	}

	for (j = 0; j < i; ++j) {
		*cp++ = strdup(LuaToString(l, j + 1));
	}
	*cp = NULL;

	return 0;
}
#endif

/**
**  Create a missile.
**
**  @param list  List of all names.
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclMissile(SCM list)
{
	SCM value;
	char* str;
	MissileType* type;
	int x;
	int y;
	int dx;
	int dy;
	int sx;
	int sy;
	Missile* missile;

	DebugLevel0Fn("FIXME: not finished\n");

	missile = NULL;
	type = NULL;
	x = dx = y = dy = sx = sy = -1;

	while (!gh_null_p(list)) {
		value = gh_car(list);
		list = gh_cdr(list);

		if (gh_eq_p(value, gh_symbol2scm("type"))) {
			value = gh_car(list);
			list = gh_cdr(list);
			str = gh_scm2newstr(value, NULL);
			type = MissileTypeByIdent(str);
			free(str);
		} else if (gh_eq_p(value, gh_symbol2scm("pos"))) {
			SCM sublist;

			sublist = gh_car(list);
			list = gh_cdr(list);
			x = gh_scm2int(gh_car(sublist));
			y = gh_scm2int(gh_cadr(sublist));
		} else if (gh_eq_p(value, gh_symbol2scm("origin-pos"))) {
			SCM sublist;

			sublist = gh_car(list);
			list = gh_cdr(list);
			sx = gh_scm2int(gh_car(sublist));
			sy = gh_scm2int(gh_cadr(sublist));
		} else if (gh_eq_p(value, gh_symbol2scm("goal"))) {
			SCM sublist;

			sublist = gh_car(list);
			list = gh_cdr(list);
			dx = gh_scm2int(gh_car(sublist));
			dy = gh_scm2int(gh_cadr(sublist));
		} else if (gh_eq_p(value, gh_symbol2scm("local"))) {
			DebugCheck(!type);
			missile = MakeLocalMissile(type, x, y, dx, dy);
			// we need to reinitialize position parameters - that's because of
			// the way InitMissile() (called from MakeLocalMissile()) computes
			// them - it works for creating a missile during a game but breaks
			// loading the missile from a file.
			missile->X = x;
			missile->Y = y;
			missile->SourceX = sx;
			missile->SourceY = sy;
			missile->DX = dx;
			missile->DY = dy;
			missile->Local = 1;
		} else if (gh_eq_p(value, gh_symbol2scm("global"))) {
			DebugCheck(!type);
			missile = MakeMissile(type, x, y, dx, dy);
			missile->X = x;
			missile->Y = y;
			missile->SourceX = sx;
			missile->SourceY = sy;
			missile->DX = dx;
			missile->DY = dy;
			missile->Local = 0;
		} else if (gh_eq_p(value, gh_symbol2scm("frame"))) {
			DebugCheck(!missile);
			missile->SpriteFrame = gh_scm2int(gh_car(list));
			list = gh_cdr(list);
		} else if (gh_eq_p(value, gh_symbol2scm("state"))) {
			DebugCheck(!missile);
			missile->State = gh_scm2int(gh_car(list));
			list = gh_cdr(list);
		} else if (gh_eq_p(value, gh_symbol2scm("anim-wait"))) {
			DebugCheck(!missile);
			missile->AnimWait = gh_scm2int(gh_car(list));
			list = gh_cdr(list);
		} else if (gh_eq_p(value, gh_symbol2scm("wait"))) {
			DebugCheck(!missile);
			missile->Wait = gh_scm2int(gh_car(list));
			list = gh_cdr(list);
		} else if (gh_eq_p(value, gh_symbol2scm("delay"))) {
			DebugCheck(!missile);
			missile->Delay = gh_scm2int(gh_car(list));
			list = gh_cdr(list);
		} else if (gh_eq_p(value, gh_symbol2scm("source"))) {
			DebugCheck(!missile);
			value = gh_car(list);
			list = gh_cdr(list);
			str = gh_scm2newstr(value, NULL);
			missile->SourceUnit = UnitSlots[strtol(str + 1, 0, 16)];
			free(str);
			++missile->SourceUnit->Refs;
		} else if (gh_eq_p(value, gh_symbol2scm("target"))) {
			DebugCheck(!missile);
			value = gh_car(list);
			list = gh_cdr(list);
			str = gh_scm2newstr(value, NULL);
			missile->TargetUnit = UnitSlots[strtol(str + 1, 0, 16)];
			free(str);
			missile->TargetUnit->Refs++;
		} else if (gh_eq_p(value, gh_symbol2scm("damage"))) {
			DebugCheck(!missile);
			missile->Damage = gh_scm2int(gh_car(list));
			list = gh_cdr(list);
		} else if (gh_eq_p(value, gh_symbol2scm("ttl"))) {
			DebugCheck(!missile);
			missile->TTL = gh_scm2int(gh_car(list));
			list = gh_cdr(list);
		} else if (gh_eq_p(value, gh_symbol2scm("hidden"))) {
			DebugCheck(!missile);
			missile->Hidden = 1;
			list = gh_cdr(list);
		} else if (gh_eq_p(value, gh_symbol2scm("step"))) {
			SCM sublist;

			DebugCheck(!missile);
			sublist = gh_car(list);
			list = gh_cdr(list);
			missile->CurrentStep = gh_scm2int(gh_car(sublist));
			missile->TotalStep = gh_scm2int(gh_cadr(sublist));
		}
	}
	return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclMissile(lua_State* l)
{
	return 0;
}
#endif

/**
**  Define burning building missiles.
**
**  @param list  FIXME: docu.
*/
#if defined(USE_GUILE) || defined(USE_SIOD)
local SCM CclDefineBurningBuilding(SCM list)
{
	SCM value;
	SCM sublist;
	BurningBuildingFrame** frame;
	BurningBuildingFrame* ptr;
	BurningBuildingFrame* next;
	char* str;

	ptr = BurningBuildingFrames;
	while (ptr) {
		next = ptr->Next;
		free(ptr);
		ptr = next;
	}
	BurningBuildingFrames = NULL;

	frame = &BurningBuildingFrames;

	while (!gh_null_p(list)) {
		sublist = gh_car(list);
		list = gh_cdr(list);

		*frame = calloc(1, sizeof(BurningBuildingFrame));
		while (!gh_null_p(sublist)) {
			value = gh_car(sublist);
			sublist = gh_cdr(sublist);

			if (gh_eq_p(value, gh_symbol2scm("percent"))) {
				value = gh_car(sublist);
				sublist = gh_cdr(sublist);
				(*frame)->Percent = gh_scm2int(value);
			} else if (gh_eq_p(value, gh_symbol2scm("missile"))) {
				value = gh_car(sublist);
				sublist = gh_cdr(sublist);
				str = gh_scm2newstr(value, NULL);
				(*frame)->Missile = MissileTypeByIdent(str);
				free(str);
			}
		}
		frame = &((*frame)->Next);
	}
	return SCM_UNSPECIFIED;
}
#elif defined(USE_LUA)
local int CclDefineBurningBuilding(lua_State* l)
{
	const char* value;
	BurningBuildingFrame** frame;
	BurningBuildingFrame* ptr;
	BurningBuildingFrame* next;
	int args;
	int j;
	int subargs;
	int k;

	ptr = BurningBuildingFrames;
	while (ptr) {
		next = ptr->Next;
		free(ptr);
		ptr = next;
	}
	BurningBuildingFrames = NULL;

	frame = &BurningBuildingFrames;

	args = lua_gettop(l);
	for (j = 0; j < args; ++j) {
		if (!lua_istable(l, j + 1)) {
			lua_pushstring(l, "incorrect argument");
			lua_error(l);
		}

		*frame = calloc(1, sizeof(BurningBuildingFrame));
		subargs = luaL_getn(l, j + 1);
		for (k = 0; k < subargs; ++k) {
			lua_rawgeti(l, j + 1, k + 1);
			value = LuaToString(l, -1);
			lua_pop(l, 1);
			++k;

			if (!strcmp(value, "percent")) {
				lua_rawgeti(l, j + 1, k + 1);
				(*frame)->Percent = LuaToNumber(l, -1);
				lua_pop(l, 1);
			} else if (!strcmp(value, "missile")) {
				lua_rawgeti(l, j + 1, k + 1);
				(*frame)->Missile = MissileTypeByIdent(LuaToString(l, -1));
				lua_pop(l, 1);
			}
		}
		frame = &((*frame)->Next);
	}
	return 0;
}
#endif

/**
**  Register CCL features for missile-type.
*/
global void MissileCclRegister(void)
{
#if defined(USE_GUILE) || defined(USE_SIOD)
	gh_new_procedureN("define-missiletype-wc-names",
		CclDefineMissileTypeWcNames);
	gh_new_procedureN("define-missile-type", CclDefineMissileType);
	gh_new_procedureN("missile", CclMissile);
	gh_new_procedureN("define-burning-building", CclDefineBurningBuilding);
#elif defined(USE_LUA)
	lua_register(Lua, "DefineMissileTypeWcNames",
		CclDefineMissileTypeWcNames);
	lua_register(Lua, "DefineMissileType", CclDefineMissileType);
	lua_register(Lua, "Missile", CclMissile);
	lua_register(Lua, "DefineBurningBuilding", CclDefineBurningBuilding);
#endif
}

//@}
