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
//
//      (c) Copyright 2013 by cybermind
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
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

#ifndef SPELL_TELEPORT_H
#define SPELL_TELEPORT_H

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include "spells.h"

class Spell_Teleport : public SpellActionType
{
public:
	Spell_Teleport() : SpellActionType(1), UnitType(NULL), TTL(0), RequireCorpse(0) {};
	virtual int Cast(CUnit &caster, const SpellType &spell,
					 CUnit *target, const Vec2i &goalPos);
	virtual void Parse(lua_State *l, int startIndex, int endIndex);

private:
	CUnitType *UnitType;    /// Type of unit to be summoned.
	int TTL;                /// Time to live for summoned unit. 0 means infinite
	int RequireCorpse;      /// Corpse consumed while summoning.
};


//@}

#endif // SPELL_TELEPORT_H
