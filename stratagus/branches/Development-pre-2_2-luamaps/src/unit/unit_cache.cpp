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
/**@name unit_cache.c - The unit cache. */
//
//      Cache to find units faster from position.
//      Sort of trivial implementation, since most queries are on a single tile.
//      Unit is just inserted in a double linked list for every tile it's on.
//
//      (c) Copyright 2004 by Crestez Leonard
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
//     along with this program; if not, write to the Free Software
//     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//     02111-1307, USA.
//
// $Id$

//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"
#include "unit.h"
#include "unittype.h"
#include "map.h"
#include "editor.h"

/**
**  Insert new unit into cache.
**
**  @param unit  Unit pointer to place in cache.
*/
void UnitCacheInsert(Unit* unit)
{
	int i;
	int j;
	MapField* mf;
	UnitListItem* listitem;

	Assert(!unit->Removed);

	for (i = 0; i < unit->Type->TileHeight; ++i) {
		mf = TheMap.Fields + (i + unit->Y) * TheMap.Info.MapWidth + unit->X;
		listitem = unit->CacheLinks + i * unit->Type->TileWidth;
		for (j = 0; j < unit->Type->TileWidth; ++j) {
			Assert(!listitem->Next && !listitem->Prev);

			listitem->Next = mf->UnitCache;
			if (mf->UnitCache) {
				mf->UnitCache->Prev = listitem;
			}
			mf->UnitCache = listitem;
			++mf;
			++listitem;
		}
	}
}

/**
**  Remove unit from cache.
**
**  @param unit  Unit pointer to remove from cache.
*/
void UnitCacheRemove(Unit* unit)
{
	int i;
	int j;
	MapField* mf;
	UnitListItem* listitem;

	Assert(!unit->Removed);
	for (i = 0; i < unit->Type->TileHeight; ++i) {
		listitem = unit->CacheLinks + i * unit->Type->TileWidth;
		for (j = 0; j < unit->Type->TileWidth; ++j) {
			if (listitem->Next) {
				listitem->Next->Prev = listitem->Prev;
			}
			if (listitem->Prev) {
				listitem->Prev->Next = listitem->Next;
			} else {
				// item is head of the list.
				mf = TheMap.Fields + (i + unit->Y) * TheMap.Info.MapWidth + j + unit->X;
				Assert(mf->UnitCache == listitem);
				mf->UnitCache = listitem->Next;
				Assert(!mf->UnitCache || !mf->UnitCache->Prev);
			}

			listitem->Next = listitem->Prev = NULL;
			++listitem;
		}
	}
}

/**
**  Select units in rectangle range.
**
**  @param x1     Left column of selection rectangle
**  @param y1     Top row of selection rectangle
**  @param x2     Right column of selection rectangle
**  @param y2     Bottom row of selection rectangle
**  @param table  All units in the selection rectangle
**
**  @return       Returns the number of units found
*/
int UnitCacheSelect(int x1, int y1, int x2, int y2, Unit** table)
{
	int i;
	int j;
	int n;
	UnitListItem* listitem;

	// Optimize small searches.
	if (x1 >= x2 - 1 && y1 >= y2 - 1) {
		return UnitCacheOnTile(x1, y1, table);
	}

	//
	//  Reduce to map limits. FIXME: should the caller check?
	//
	if (x1 < 0) {
		x1 = 0;
	}
	if (y1 < 0) {
		y1 = 0;
	}
	if (x2 > TheMap.Info.MapWidth) {
		x2 = TheMap.Info.MapWidth;
	}
	if (y2 > TheMap.Info.MapHeight) {
		y2 = TheMap.Info.MapHeight;
	}

	n = 0;
	for (i = y1; i < y2; ++i) {
		for (j = x1; j < x2; ++j) {
			listitem = TheMap.Fields[i * TheMap.Info.MapWidth + j].UnitCache;
			for (; listitem; listitem = listitem->Next) {
				//
				// To avoid getting a unit in multiple times we use a cache lock.
				// It should only be used in here, unless you somehow want the unit
				// to be out of cache.
				//
				if (!listitem->Unit->CacheLock && !listitem->Unit->Type->Revealer) {
					listitem->Unit->CacheLock = 1;
					Assert(!listitem->Unit->Removed);
					table[n++] = listitem->Unit;
				}
			}
		}
	}

	//
	// Clean the cache locks, restore to original situation.
	//
	for (i = 0; i < n; ++i) {
		table[i]->CacheLock = 0;
	}

	return n;
}

/**
**  Select units on map tile.
**
**  @param x      Map X tile position
**  @param y      Map Y tile position
**  @param table  All units in the selection rectangle
**
**  @return       Returns the number of units found
*/
int UnitCacheOnTile(int x, int y, Unit** table)
{
	UnitListItem* listitem;
	int n;

	//
	// Unlike in UnitCacheSelect, there's no way an unit can show up twice,
	// so there is no need for Cache Locks.
	//
	n = 0;
	listitem = TheMap.Fields[y * TheMap.Info.MapWidth + x].UnitCache;
	for (; listitem; listitem = listitem->Next) {
		if (!listitem->Unit->CacheLock) {
			Assert(!listitem->Unit->Removed);
			table[n++] = listitem->Unit;
		}
	}

	return n;
}

/**
**  Initialize unit-cache.
*/
void InitUnitCache(void)
{
}