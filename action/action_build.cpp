//   ___________		     _________		      _____  __
//   \_	  _____/______   ____   ____ \_   ___ \____________ _/ ____\/  |_
//    |    __) \_  __ \_/ __ \_/ __ \/    \  \/\_  __ \__  \\   __\\   __\ 
//    |     \   |  | \/\  ___/\  ___/\     \____|  | \// __ \|  |   |  |
//    \___  /   |__|    \___  >\___  >\______  /|__|  (____  /__|   |__|
//	  \/		    \/	   \/	     \/		   \/
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   FreeCraft - A free fantasy real time strategy game engine
//
/**@name action_build.c -	The build building action. */
//
//	(c) Copyright 1998,2000,2001 by Lutz Sammer
//
//	$Id$

//@{

/*----------------------------------------------------------------------------
--      Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "freecraft.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "sound.h"
#include "actions.h"
#include "map.h"
#include "ai.h"
#include "interface.h"
#include "pathfinder.h"

/*----------------------------------------------------------------------------
--      Functions
----------------------------------------------------------------------------*/

/**
**	Unit builds a building.
**
**	@param unit	Unit that builds a building.
*/
global void HandleActionBuild(Unit* unit)
{
    int x;
    int y;
    int n;
    UnitType* type;
    const UnitStats* stats;
    Unit* build;
    Unit* temp;

    if( !unit->SubAction ) {		// first entry
	unit->SubAction=1;
#ifdef NEW_ORDERS
	NewResetPath(unit);
#endif
    }

#ifdef NEW_ORDERS
    type=unit->Orders[0].Type;
#else
    type=unit->Command.Data.Build.BuildThis;
#endif

    switch( DoActionMove(unit) ) {	// reached end-point?
	case PF_UNREACHABLE:
	    //
	    //	Some tries to reach the goal
	    //
	    if( unit->SubAction++<10 ) {
#ifndef NEW_ORDERS
		unit->Command.Action=UnitActionBuild;
#endif
		//	To keep the load low, retry each 1/4 second.
		unit->Wait=FRAMES_PER_SECOND/4+unit->SubAction;
		return;
	    }
	    // FIXME: use general notify/messages
	    if( unit->Player==ThisPlayer ) {
		SetMessage("You cannot reach building place.");
	    } else {
		AiCanNotReach(unit,type);
	    }

#ifdef NEW_ORDERS
	    unit->Orders[0].Action=UnitActionStill;
#endif
	    unit->SubAction=0;
	    if( IsSelected(unit) ) {	// update display for new action
		UpdateButtonPanel();
	    }

	    return;

	case PF_REACHED:
	    DebugLevel3Fn("reached %d,%d\n",unit->X,unit->Y);
	    break;

	default:
	    return;
    }

    //
    //	Building place must be reached!
    //
#ifdef NEW_ORDERS
    x=unit->Orders[0].X;
    y=unit->Orders[0].Y;
#else
    x=unit->Command.Data.Move.DX;
    y=unit->Command.Data.Move.DY;
#endif

    if( type->ShoreBuilding ) {		// correct coordinates.
	++x;
	++y;
    }

    //
    //	Check if the building could be build there.
    //
    if( !CanBuildUnitType(unit,type,x,y) ) {
	//
	//	Some tries to build the building.
	//
	if( unit->SubAction++<10 ) {
#ifndef NEW_ORDERS
	    unit->Command.Action=UnitActionBuild;
#endif
	    //	To keep the load low, retry each 1/4 second.
	    unit->Wait=FRAMES_PER_SECOND/4+unit->SubAction;
	    return;
	}

	// FIXME: use general notify/messages
        if( unit->Player==ThisPlayer ) {
	    SetMessage("You cannot build %s here.", type->Name);
	} else {
	    AiCanNotBuild(unit,type);
	}

#ifdef NEW_ORDERS
	unit->Orders[0].Action=UnitActionStill;
#endif
	unit->SubAction=0;
	if( IsSelected(unit) ) {	// update display for new action
	    UpdateButtonPanel();
	}

	return;
    }

    //
    //	Check if enough resources for the building.
    //
    if( PlayerCheckUnitType(unit->Player,type) ) {
	if( unit->Player!=ThisPlayer ) {
	    AiCanNotBuild(unit,type);
	}
	return;
    }
    PlayerSubUnitType(unit->Player,type);

    build=MakeUnitAndPlace(x,y,type,unit->Player);
    stats=build->Stats;
    // HACK: the building is not ready yet
    build->Player->UnitTypesCount[type->Type]--;
    build->Constructed=1;
    build->HP=0;
#ifdef NEW_ORDERS
    build->Orders[0].Action=UnitActionBuilded;
    build->Data.Builded.Sum=0;  // FIXME: Is it necessary?
    build->Data.Builded.Val=stats->HitPoints;
    n=(stats->Costs[TimeCost]*FRAMES_PER_SECOND/6)/(SpeedBuild*5);
    build->Data.Builded.Add=stats->HitPoints/n;
    build->Data.Builded.Sub=n;
    build->Data.Builded.Cancel=0; // FIXME: Is it necessary?
    build->Data.Builded.Worker=unit;
    DebugLevel3Fn("Build Sum %d, Add %d, Val %d, Sub %d\n"
	,build->Data.Builded.Sum,build->Data.Builded.Add
	,build->Data.Builded.Val,build->Data.Builded.Sub);
#else
    build->Command.Action=UnitActionBuilded;
    build->Command.Data.Builded.Sum=0;  // FIXME: Is it necessary?
    build->Command.Data.Builded.Val=stats->HitPoints;
    n=(stats->Costs[TimeCost]*FRAMES_PER_SECOND/6)/(SpeedBuild*5);
    build->Command.Data.Builded.Add=stats->HitPoints/n;
    build->Command.Data.Builded.Sub=n;
    build->Command.Data.Builded.Cancel=0; // FIXME: Is it necessary?
    build->Command.Data.Builded.Worker=unit;
    DebugLevel3Fn("Build Sum %d, Add %d, Val %d, Sub %d\n"
	,build->Command.Data.Builded.Sum,build->Command.Data.Builded.Add
	,build->Command.Data.Builded.Val,build->Command.Data.Builded.Sub);
#endif
    build->Wait=FRAMES_PER_SECOND/6;

    //
    //	Building oil-platform, must remove oilpatch.
    //
    if( type->GivesOil ) {
        DebugLevel0Fn("Remove oil-patch\n");
	temp=OilPatchOnMap(x,y);
	DebugCheck( !temp );
	unit->Value=temp->Value;	// Let peon hold value while building
	// oil patch should NOT make sound, handled by destroy unit
	DestroyUnit(temp);		// Destroy oil patch
    }

    RemoveUnit(unit);	/* automaticly: CheckUnitToBeDrawn(unit) */

    unit->X=x;
    unit->Y=y;
#ifdef NEW_ORDERS
    unit->Orders[0].Action=UnitActionStill;
#else
    unit->Command.Action=UnitActionStill;
#endif
    unit->SubAction=0;

    CheckUnitToBeDrawn(build);
    MustRedraw|=RedrawMinimap;
}

/**
**	Unit under Construction
**
**	@param unit	Unit that is builded.
*/
global void HandleActionBuilded(Unit* unit)
{
    Unit* peon;
    UnitType* type;

    type=unit->Type;

    //
    // Check if construction should be canceled...
    //
#ifdef NEW_ORDERS
    if( unit->Data.Builded.Cancel ) {
	// Drop out unit
	peon=unit->Data.Builded.Worker;
	peon->Orders[0].Action=UnitActionStill;
	unit->Data.Builded.Worker=NoUnitP;
#else
    if( unit->Command.Data.Builded.Cancel ) {
	// Drop out unit
	peon=unit->Command.Data.Builded.Worker;
	peon->Command.Action=UnitActionStill;
	unit->Command.Data.Builded.Worker=NoUnitP;
#endif
	peon->Reset=peon->Wait=1;
	peon->SubAction=0;

	unit->Value=peon->Value;	// peon holding value while building
	DropOutOnSide(peon,LookingW,type->TileWidth,type->TileHeight);

	// Player gets back 75% of the original cost for a building.
	PlayerAddCostsFactor(unit->Player,unit->Stats->Costs,
		CancelBuildingCostsFactor);
	// Cancel building
	DestroyUnit(unit);
	return;
    }

    // FIXME: if attacked subtract hit points!!

#ifdef NEW_ORDERS
    unit->Data.Builded.Val-=unit->Data.Builded.Sub;
    if( unit->Data.Builded.Val<0 ) {
	unit->Data.Builded.Val+=unit->Stats->HitPoints;
	unit->HP++;
	unit->Data.Builded.Sum++;
    }
    unit->HP+=unit->Data.Builded.Add;
    unit->Data.Builded.Sum+=unit->Data.Builded.Add;

    //
    //	Check if building ready.
    //
    if( unit->Data.Builded.Sum>=unit->Stats->HitPoints
		|| unit->HP>=unit->Stats->HitPoints ) {
#else
    unit->Command.Data.Builded.Val-=unit->Command.Data.Builded.Sub;
    if( unit->Command.Data.Builded.Val<0 ) {
	unit->Command.Data.Builded.Val+=unit->Stats->HitPoints;
	unit->HP++;
	unit->Command.Data.Builded.Sum++;
    }
    unit->HP+=unit->Command.Data.Builded.Add;
    unit->Command.Data.Builded.Sum+=unit->Command.Data.Builded.Add;

    //
    //	Check if building ready.
    //
    if( unit->Command.Data.Builded.Sum>=unit->Stats->HitPoints
		|| unit->HP>=unit->Stats->HitPoints ) {
#endif
	if( unit->HP>unit->Stats->HitPoints ) {
	    unit->HP=unit->Stats->HitPoints;
	}
#ifdef NEW_ORDERS
	unit->Orders[0].Action=UnitActionStill;
#else
	unit->Command.Action=UnitActionStill;
#endif
	// HACK: the building is ready now
	unit->Player->UnitTypesCount[type->Type]++;
	unit->Constructed=0;
	unit->Frame=0;
	unit->Reset=unit->Wait=1;

#ifdef NEW_ORDERS
	peon=unit->Data.Builded.Worker;
	peon->Orders[0].Action=UnitActionStill;
#else
	peon=unit->Command.Data.Builded.Worker;
	peon->Command.Action=UnitActionStill;
#endif
	peon->SubAction=0;
	peon->Reset=peon->Wait=1;
	DropOutOnSide(peon,LookingW,type->TileWidth,type->TileHeight);

	//
	//	Building oil-platform, must update oil.
	//
	if( type->GivesOil ) {
	    CommandHaulOil(peon,unit,0);	// Let the unit haul oil
	    DebugLevel0Fn("Update oil-platform\n");
#ifdef NEW_ORDERS
	    DebugLevel0Fn(" =%d\n",unit->Data.Resource.Active);
	    unit->Data.Resource.Active=0;
#else
	    DebugLevel0Fn(" =%d\n",unit->Command.Data.Resource.Active);
	    unit->Command.Data.Resource.Active=0;
#endif
	    unit->Value=peon->Value;	// peon was holding value while building
	}

	// FIXME: General message system
	if( unit->Player==ThisPlayer ) {
	    SetMessage2( unit->X, unit->Y, "New %s done", type->Name );
	    PlayUnitSound(peon,VoiceWorkCompleted);
	} else {
	    AiWorkComplete(peon,unit);
	}

	// FIXME: Vladi: this is just a hack to test wall fixing,
	// FIXME:	also not sure if the right place...
	// FIXME: Johns: and now this is also slow
	if ( unit->Type == UnitTypeByIdent("unit-orc-wall")
		    || unit->Type == UnitTypeByIdent("unit-human-wall")) {
	    MapSetWall(unit->X, unit->Y,
		    unit->Type == UnitTypeByIdent("unit-human-wall"));
            RemoveUnit(unit);/* automaticly: CheckUnitToBeDrawn(unit) */
	    UnitLost(unit);
	    ReleaseUnit(unit);
	    return;
        }

	UpdateForNewUnit(unit,0);

	if( IsSelected(unit) ) {
	    UpdateButtonPanel();
	    MustRedraw|=RedrawPanels;
	} else if( unit->Player==ThisPlayer ) {
	    UpdateButtonPanel();
	}
        CheckUnitToBeDrawn(unit);
	return;
    }

    //
    //	Update building states
    //
#ifdef NEW_ORDERS
    if( unit->Data.Builded.Sum*2>=unit->Stats->HitPoints ) {
#else
    if( unit->Command.Data.Builded.Sum*2>=unit->Stats->HitPoints ) {
#endif
        if( (unit->Frame!=1 || unit->Constructed) ) {
	  CheckUnitToBeDrawn(unit);
	}
	unit->Constructed=0;
	unit->Frame=1;
#ifdef NEW_ORDERS
    } else if( unit->Data.Builded.Sum*4>=unit->Stats->HitPoints ) {
#else
    } else if( unit->Command.Data.Builded.Sum*4>=unit->Stats->HitPoints ) {
#endif
        if( unit->Frame!=1 ) {
	  CheckUnitToBeDrawn(unit);
	}
	unit->Frame=1;
    }

    unit->Wait=5;
    if( IsSelected(unit) ) {
        MustRedraw|=RedrawInfoPanel;
    }
}

//@}
