//       _________ __                 __                               
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/ 
//  ______________________                           ______________________
//			  T H E   W A R   B E G I N S
//	   Stratagus - A free fantasy real time strategy game engine
//
/**@name botpanel.c	-	The bottom panel. */
//
//	(c) Copyright 1999-2003 by Lutz Sammer, Vladi Belperchinov-Shabanski,
//	                        and Jimmy Salmon
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
//	$Id$

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "stratagus.h"

#include "video.h"
#include "unit.h"
#include "commands.h"
#include "depend.h"
#include "interface.h"
#include "ui.h"
#include "map.h"
#include "font.h"
#include "spells.h"

/*----------------------------------------------------------------------------
--      Defines
----------------------------------------------------------------------------*/

#ifndef NEW_UI
    /// How many different buttons are allowed
#define MAX_BUTTONS	2048
#endif

/*----------------------------------------------------------------------------
--      Variables
----------------------------------------------------------------------------*/

    /// Display the command key in the buttons.
global char ShowCommandKey;

#ifndef NEW_UI
    /// for unit buttons sub-menus etc.
global int CurrentButtonLevel;
    /// All buttons for units
local ButtonAction *UnitButtonTable[MAX_BUTTONS];
    /// buttons in UnitButtonTable
local int NumUnitButtons;
#endif

/*----------------------------------------------------------------------------
--      Functions
----------------------------------------------------------------------------*/

/**
**	Initialize the buttons.
*/
global void InitButtons(void)
{
#ifndef NEW_UI
    int z;

    //
    //	Resolve the icon names.
    //
    for (z = 0; z < NumUnitButtons; z++) {
	UnitButtonTable[z]->Icon.Icon
		= IconByIdent(UnitButtonTable[z]->Icon.Name);
    }
#else
    // FIXME: proabably not necessary
    //CleanButtons();
#endif
}

/**
**	Save all buttons.
*/
global void SaveButtons(CLFile* file)
{
#ifndef NEW_UI
    int i;
    int n;
    char* cp;
#endif

    CLprintf(file,"\n;;; -----------------------------------------\n");
    CLprintf(file,";;; MODULE: buttons $Id$\n\n");

#ifndef NEW_UI
    for( i=0; i<NumUnitButtons; ++i ) {
	CLprintf(file,"(define-button 'pos %d 'level %d 'icon '%s\n",
		UnitButtonTable[i]->Pos,
		UnitButtonTable[i]->Level,
		IdentOfIcon(UnitButtonTable[i]->Icon.Icon));
	CLprintf(file,"  'action ");
	switch( UnitButtonTable[i]->Action ) {
	    case ButtonMove:
		CLprintf(file,"'move"); break;
	    case ButtonStop:
		CLprintf(file,"'stop"); break;
	    case ButtonAttack:
		CLprintf(file,"'attack"); break;
	    case ButtonRepair:
		CLprintf(file,"'repair"); break;
	    case ButtonHarvest:
		CLprintf(file,"'harvest"); break;
	    case ButtonButton:
		CLprintf(file,"'button"); break;
	    case ButtonBuild:
		CLprintf(file,"'build"); break;
	    case ButtonTrain:
		CLprintf(file,"'train-unit"); break;
	    case ButtonPatrol:
		CLprintf(file,"'patrol"); break;
	    case ButtonStandGround:
		CLprintf(file,"'stand-ground"); break;
	    case ButtonAttackGround:
		CLprintf(file,"'attack-ground"); break;
	    case ButtonReturn:
		CLprintf(file,"'return-goods"); break;
	    case ButtonDemolish:
		CLprintf(file,"'demolish"); break;
	    case ButtonSpellCast:
		CLprintf(file,"'cast-spell"); break;
	    case ButtonResearch:
		CLprintf(file,"'research"); break;
	    case ButtonUpgradeTo:
		CLprintf(file,"'upgrade-to"); break;
	    case ButtonUnload:
		CLprintf(file,"'unload"); break;
	    case ButtonCancel:
		CLprintf(file,"'cancel"); break;
	    case ButtonCancelUpgrade:
		CLprintf(file,"'cancel-upgrade"); break;
	    case ButtonCancelTrain:
		CLprintf(file,"'cancel-train-unit"); break;
	    case ButtonCancelBuild:
		CLprintf(file,"'cancel-build"); break;
	}
	if( UnitButtonTable[i]->ValueStr ) {
	    if( isdigit(UnitButtonTable[i]->ValueStr[0]) ) {
		CLprintf(file," 'value %s\n",UnitButtonTable[i]->ValueStr);
	    } else {
		CLprintf(file," 'value '%s\n",UnitButtonTable[i]->ValueStr);
	    }
	} else {
	    CLprintf(file,"\n");
	}
	if( UnitButtonTable[i]->Allowed ) {
	    CLprintf(file,"  'allowed ");
	    if( UnitButtonTable[i]->Allowed == ButtonCheckTrue ) {
		CLprintf(file,"'check-true");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckFalse ) {
		CLprintf(file,"'check-false");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckUpgrade ) {
		CLprintf(file,"'check-upgrade");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckUnitsOr ) {
		CLprintf(file,"'check-units-or");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckUnitsAnd ) {
		CLprintf(file,"'check-units-and");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckNetwork ) {
		CLprintf(file,"'check-network");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckNoNetwork ) {
		CLprintf(file,"'check-no-network");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckNoWork ) {
		CLprintf(file,"'check-no-work");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckNoResearch ) {
		CLprintf(file,"'check-no-research");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckAttack ) {
		CLprintf(file,"'check-attack");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckUpgradeTo ) {
		CLprintf(file,"'check-upgrade-to");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckResearch ) {
		CLprintf(file,"'check-research");
	    } else if( UnitButtonTable[i]->Allowed == ButtonCheckSingleResearch ) {
		CLprintf(file,"'check-single-research");
	    } else {
		DebugLevel0Fn("Unsupported check function %p\n" _C_
			UnitButtonTable[i]->Allowed);
		CLprintf(file,"%p",UnitButtonTable[i]->Allowed);
	    }
	    if( UnitButtonTable[i]->AllowStr ) {
		CLprintf(file," 'allow-arg '(");
		cp=alloca(strlen(UnitButtonTable[i]->AllowStr));
		strcpy(cp,UnitButtonTable[i]->AllowStr);
		cp=strtok(cp,",");
		while( cp ) {
		    CLprintf(file,"%s",cp);
		    cp=strtok(NULL,",");
		    if( cp ) {
			CLprintf(file," ");
		    }
		}
		CLprintf(file,")");
	    }
	    CLprintf(file,"\n");
	}
	CLprintf(file,"  'key \"");
	switch( UnitButtonTable[i]->Key ) {
	    case '\033':
		CLprintf(file,"\\%03o",UnitButtonTable[i]->Key);
		break;
	    default:
		CLprintf(file,"%c",UnitButtonTable[i]->Key);
		break;
	}
	CLprintf(file,"\" 'hint \"%s\"\n",UnitButtonTable[i]->Hint);
	n=CLprintf(file,"  'for-unit '(");
	cp=alloca(strlen(UnitButtonTable[i]->UnitMask));
	strcpy(cp,UnitButtonTable[i]->UnitMask);
	cp=strtok(cp,",");
	while( cp ) {
	    if( n+strlen(cp)>78 ) {
		n=CLprintf(file,"\n    ");
	    }
	    n+=CLprintf(file,"%s",cp);
	    cp=strtok(NULL,",");
	    if( cp ) {
		n+=CLprintf(file," ");
	    }
	}
	CLprintf(file,"))\n\n");
    }
#else

    CLprintf(file,"(set-selection-changed-hook '");
    lprin1CL(SelectionChangedHook,file);
    CLprintf(file,")\n");

    CLprintf(file,"(set-selected-unit-changed-hook '");
    lprin1CL(SelectedUnitChangedHook,file);
    CLprintf(file,")\n");

    CLprintf(file,"(set-choose-target-begin-hook '");
    lprin1CL(ChooseTargetBeginHook,file);
    CLprintf(file,")\n");

    CLprintf(file,"(set-choose-target-finish-hook '");
    lprin1CL(ChooseTargetFinishHook,file);
    CLprintf(file,")\n");

#endif

    CLprintf(file,"(set-show-command-key! %s)\n\n",
	    ShowCommandKey ? "#t" : "#f");
}














/*----------------------------------------------------------------------------
--      Buttons structures
----------------------------------------------------------------------------*/

#ifndef NEW_UI
global ButtonAction* CurrentButtons;	/// Pointer to current buttons
local ButtonAction  _current_buttons[9];	/// FIXME: this is just for test
#else
global ButtonAction CurrentButtons[9];	/// Pointer to current buttons
#endif

#ifdef NEW_UI
local void CleanButton(ButtonAction * ba)
{
    if( !ba->Icon.Name ) {
	return;
    }
    free(ba->Icon.Name);
    CclGcUnprotect(ba->Action);
    memset(ba, 0, sizeof(*ba));
    MustRedraw|=RedrawButtonPanel;
}
#endif

/// FIXME: docu
#ifndef NEW_UI
int AddButton(int pos, int level, const char *icon_ident,
	enum _button_cmd_ action, const char *value, const ButtonCheckFunc func,
	const void *allow, int key, const char *hint, const char *umask)
#else
global void AddButton(int pos, char *icon_ident, SCM action, int key, char *hint)
#endif
{
#ifndef NEW_UI
    char buf[2048];
#endif
    ButtonAction *ba;

#ifndef NEW_UI
    ba = (ButtonAction *) malloc(sizeof(ButtonAction));
    DebugCheck(!ba);			//FIXME: perhaps should return error?

    ba->Pos = pos;
    ba->Level = level;
    ba->Icon.Name = (char *)icon_ident;
    // FIXME: check if already initited
    //ba->Icon.Icon = IconByIdent(icon_ident);
    ba->Action = action;
    if (value) {
	ba->ValueStr = strdup(value);
	switch (action) {
	case ButtonSpellCast:
	    ba->Value = SpellIdByIdent(value);
	    break;
	case ButtonTrain:
	    ba->Value = UnitTypeIdByIdent(value);
	    break;
	case ButtonResearch:
	    ba->Value = UpgradeIdByIdent(value);
	    break;
	case ButtonUpgradeTo:
	    ba->Value = UnitTypeIdByIdent(value);
	    break;
	case ButtonBuild:
	    ba->Value = UnitTypeIdByIdent(value);
	    break;
	default:
	    ba->Value = atoi(value);
	    break;
	}
    } else {
	ba->ValueStr = NULL;
	ba->Value = 0;
    }

    ba->Allowed = func;
    if (allow) {
	ba->AllowStr = strdup(allow);
    } else {
	ba->AllowStr = NULL;
    }
    ba->Key = key;
    ba->Hint = strdup(hint);
    //FIXME: here should be added costs to the hint
    //FIXME: johns: show should be nice done?
    if (umask[0] == '*') {
	strcpy(buf, umask);
    } else {
	sprintf(buf, ",%s,", umask);
    }
    ba->UnitMask = strdup(buf);
    UnitButtonTable[NumUnitButtons++] = ba;
    // FIXME: check if already initited
    //DebugCheck(ba->Icon.Icon == NoIcon);// just checks, that's why at the end
    return 1;
#else
    if( pos<1 || pos>9 ) {
	DebugLevel0Fn("Bad button positon %d (Icon.Name=%s)\n" _C_ pos _C_ icon_ident);
	// FIXME: better way to kill the program?
	DebugCheck(1);
    }
    ba = CurrentButtons + (pos-1);
    CleanButton(ba);

    // maxy: the caller does not free this pointer
    ba->Icon.Name = icon_ident;
    ba->Icon.Icon = IconByIdent(ba->Icon.Name);
    if( ba->Icon.Icon == NoIcon ) {
	ba->Icon.Icon = IconByIdent(ba->Icon.Name);
	DebugLevel0Fn("Icon not found: Icon.Name = %s\n" _C_ ba->Icon.Name);
	// FIXME: better way to kill the program? or draw a
	// Unknown-Icon and add a hint so the user can test the rest of the ccl?
	DebugCheck(1);
    }

    // maxy: the caller protected this from the GC
    ba->Action = action;

    // maxy: the caller does not free this pointer
    ba->Hint = hint;
    MustRedraw|=RedrawButtonPanel;
    ba->Key = key;
#endif
}

#ifdef NEW_UI
global void RemoveButton(int pos)
{
    if( pos<1 || pos>9 ) {
	DebugLevel0Fn("Bad button positon %d\n" _C_ pos);
	// FIXME: better way to kill the program?
	DebugCheck(1);
    }
    CleanButton(CurrentButtons + (pos-1));
}
#endif

/**
**	Cleanup buttons.
*/
global void CleanButtons(void)
{
#ifndef NEW_UI
    int z;

    //
    //  Free the allocated buttons.
    //
    for (z = 0; z < NumUnitButtons; z++) {
	DebugCheck(!UnitButtonTable[z]);
	if (UnitButtonTable[z]->Icon.Name) {
	    free(UnitButtonTable[z]->Icon.Name);
	}
	if (UnitButtonTable[z]->ValueStr) {
	    free(UnitButtonTable[z]->ValueStr);
	}
	if (UnitButtonTable[z]->AllowStr) {
	    free(UnitButtonTable[z]->AllowStr);
	}
	if (UnitButtonTable[z]->Hint) {
	    free(UnitButtonTable[z]->Hint);
	}
	if (UnitButtonTable[z]->UnitMask) {
	    free(UnitButtonTable[z]->UnitMask);
	}
	free(UnitButtonTable[z]);
    }
    NumUnitButtons = 0;

    CurrentButtonLevel = 0;
    CurrentButtons = NULL;
#else
    int i;
    DebugLevel0Fn("CleanButtons()\n");
    for (i=0; i<9; i++) {
	CleanButton(CurrentButtons + i);
    }
#endif
}

/**
**	Draw bottom panel.
*/
global void DrawButtonPanel(void)
{
    int i;
    int v;
#ifndef NEW_UI
    const UnitStats* stats;
    const ButtonAction* buttons;
    char buf[8];
#else
    //const UnitStats* stats;
    const ButtonAction* ba;
    //char buf[8];
#endif

    //
    //	Draw background
    //
    if (TheUI.ButtonPanel.Graphic) {
	VideoDrawSub(TheUI.ButtonPanel.Graphic,0,0
	    ,TheUI.ButtonPanel.Graphic->Width,TheUI.ButtonPanel.Graphic->Height
	    ,TheUI.ButtonPanelX,TheUI.ButtonPanelY);
    }

#ifndef NEW_UI
    if( !(buttons=CurrentButtons) ) {	// no buttons
	return;
    }
#endif

    // FIXME: this is unneeded DrawUnitIcon does it self
    PlayerPixels(ThisPlayer);		// could only select own units.

#ifndef NEW_UI
    for( i=0; i<TheUI.NumButtonButtons; ++i ) {
	if( buttons[i].Pos!=-1 ) {
	    int j;
	    int action;
#else
    //for( i=0; i<TheUI.NumButtonButtons; ++i ) {
    for( i=0; i<9; ++i ) {
	ba = CurrentButtons + i;
	if( ba->Icon.Icon != NoIcon ) {
	    //int j;
	    //int action;
#endif

	    // cursor is on that button
	    if( ButtonAreaUnderCursor==ButtonAreaButton
		    && ButtonUnderCursor==i ) {
		v=IconActive;
		if( MouseButtons&LeftButton ) {
		    v=IconClicked;
		}
	    } else {
		v=0;
	    }
	    //
	    //	Any better ideas?
	    //	Show the current action state of the unit with the buttons.
	    //
	    //	FIXME: Should show the rally action of buildings.
	    //

	    // NEW_UI:
	    /*  FIXME: maxy: had to disable this feature :(
		should be re-enabled from ccl as a boolean button option,
		together with something like (selected-action-is 'patrol) */

#ifndef NEW_UI
	    action=UnitActionNone;
	    switch( buttons[i].Action ) {
		case ButtonStop:
		    action=UnitActionStill;
		    break;
		case ButtonStandGround:
		    action=UnitActionStandGround;
		    break;
		case ButtonAttack:
		    action=UnitActionAttack;
		    break;
		case ButtonDemolish:
		    action=UnitActionDemolish;
		    break;
		case ButtonAttackGround:
		    action=UnitActionAttackGround;
		    break;
		case ButtonRepair:
		    action=UnitActionRepair;
		    break;
		case ButtonPatrol:
		    action=UnitActionPatrol;
		    break;
		default:
		    break;
	    }
	    if( action!=UnitActionNone ) {
		for( j=0; j<NumSelected; ++j ) {
		    if( Selected[j]->Orders[0].Action!=action ) {
			break;
		    }
		}
		if( j==NumSelected ) {
		    v|=IconSelected;
		}
	    } else {
		switch( buttons[i].Action ) {
		    case ButtonMove:
			for( j=0; j<NumSelected; ++j ) {
			    if( Selected[j]->Orders[0].Action!=UnitActionMove
				    && Selected[j]->Orders[0].Action
					!=UnitActionBuild
				    && Selected[j]->Orders[0].Action
					!=UnitActionFollow ) {
				break;
			    }
			}
			if( j==NumSelected ) {
			    v|=IconSelected;
			}
			break;
		    case ButtonHarvest:
		    case ButtonReturn:
			for( j=0; j<NumSelected; ++j ) {
			    if( Selected[j]->Orders[0].Action!=UnitActionResource
				    && Selected[j]->Orders[0].Action
					!=UnitActionHarvest ) {
				break;
			    }
			}
			if( j==NumSelected ) {
			    v|=IconSelected;
			}
			break;
		    case ButtonSpellCast:
			for( j=0; j<NumSelected; ++j ) {
			    if( Selected[j]->AutoCastSpell!=
				    SpellTypeById(buttons[i].Value) ) {
				break;
			    }
			}
			if( j==NumSelected ) {
			    v|=IconAutoCast;
			}
			break;

		    // FIXME: must handle more actions

		    default:
			break;
		}
	    }
#endif

#ifndef NEW_UI
	    DrawUnitIcon(ThisPlayer,buttons[i].Icon.Icon
		    ,v,TheUI.ButtonButtons[i].X,TheUI.ButtonButtons[i].Y);
#else
	    DrawUnitIcon(ThisPlayer,ba->Icon.Icon
		    ,v,TheUI.ButtonButtons[i].X,TheUI.ButtonButtons[i].Y);
#endif

	    //
	    //	Update status line for this button
	    //
	    if( ButtonAreaUnderCursor==ButtonAreaButton
		    && ButtonUnderCursor==i && KeyState!=KeyStateInput ) {
#ifndef NEW_UI
		SetStatusLine(buttons[i].Hint);
		// FIXME: Draw costs
		v=buttons[i].Value;
		switch( buttons[i].Action ) {
		    case ButtonBuild:
		    case ButtonTrain:
		    case ButtonUpgradeTo:
			// FIXME: store pointer in button table!
			stats=&UnitTypes[v]->Stats[ThisPlayer->Player];
			DebugLevel3("Upgrade to %s %d %d %d %d %d\n"
				_C_ UnitTypes[v].Ident
				_C_ UnitTypes[v].Demand
				_C_ UnitTypes[v]._Costs[GoldCost]
				_C_ UnitTypes[v]._Costs[WoodCost]
				_C_ stats->Costs[GoldCost]
				_C_ stats->Costs[WoodCost]);

			SetCosts(0,UnitTypes[v]->Demand,stats->Costs);

			break;
		    //case ButtonUpgrade:
		    case ButtonResearch:
			SetCosts(0,0,Upgrades[v].Costs);
			break;
		    case ButtonSpellCast:
			SetCosts(SpellTypeById( v )->ManaCost,0,NULL);
			break;

		    default:
			ClearCosts();
			break;
		}
#else
		SetStatusLine(ba->Hint);
#endif
	    }

	    //
	    //	Tutorial show command key in icons
	    //
	    if( ShowCommandKey ) {
		Button* b;
#ifdef NEW_UI
		char buf[4];
#endif

		b=&TheUI.ButtonButtons[i];
#ifndef NEW_UI
		if( CurrentButtons[i].Key==27 ) {
#else
		if( ba->Key==27 ) {
#endif
		    strcpy(buf,"ESC");
		    VideoDrawText(b->X+4+b->Width-VideoTextLength(GameFont,buf),
			b->Y+5+b->Height-VideoTextHeight(GameFont),GameFont,buf);
		} else {
		    // FIXME: real DrawChar would be useful
#ifndef NEW_UI
		    buf[0]=toupper(CurrentButtons[i].Key);
#else
		    buf[0]=toupper(ba->Key);
#endif
		    buf[1]='\0';
		    VideoDrawText(b->X+4+b->Width-VideoTextLength(GameFont,buf),
			b->Y+5+b->Height-VideoTextHeight(GameFont),GameFont,buf);
		}
	    }
	}
    }
}

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

#ifndef NEW_UI
/**
**	Update bottom panel for multiple units.
*/
local void UpdateButtonPanelMultipleUnits(void)
{
    char unit_ident[128];
    int z;
    int i;

    // first clear the table
    for ( z = 0; z < 9; z++ ) {
	_current_buttons[z].Pos = -1;
    }

    i=PlayerRacesIndex(ThisPlayer->Race);
    if( i==-1 ) {
	i=PlayerRaces.Race[PlayerRaces.Count-1];
    }
    sprintf(unit_ident,",%s-group,",PlayerRaces.Name[i]);

    for( z = 0; z < NumUnitButtons; z++ ) {
	if ( UnitButtonTable[z]->Level != CurrentButtonLevel ) {
	    continue;
	}

	// any unit or unit in list
	if ( UnitButtonTable[z]->UnitMask[0] == '*'
		|| strstr( UnitButtonTable[z]->UnitMask, unit_ident ) ) {
	    int allow;

	    allow=0;
	    DebugLevel3("%d: %p\n" _C_ z _C_ UnitButtonTable[z]->Allowed);
	    if ( UnitButtonTable[z]->Allowed ) {
		// there is check function -- call it
		if (UnitButtonTable[z]->Allowed( NULL, UnitButtonTable[z] )) {
		    allow = 1;
		}
	    } else {
		// there is no allow function -- should check dependencies
		// any unit of the group must have this feature
		if ( UnitButtonTable[z]->Action == ButtonAttack ) {
		    for( i=NumSelected; --i; ) {
			if( Selected[i]->Type->CanAttack ) {
			    allow = 1;
			    break;
			}
		    }
		} else if ( UnitButtonTable[z]->Action == ButtonAttackGround ) {
		    for( i=NumSelected; --i; ) {
			if( Selected[i]->Type->GroundAttack ) {
			    allow = 1;
			    break;
			}
		    }
		} else if ( UnitButtonTable[z]->Action == ButtonDemolish ) {
		    for( i=NumSelected; --i; ) {
			if( Selected[i]->Type->Volatile ) {
			    allow = 1;
			    break;
			}
		    }
		} else if ( UnitButtonTable[z]->Action == ButtonCancel ) {
		    allow = 1;
		} else if ( UnitButtonTable[z]->Action == ButtonCancelUpgrade ) {
		    for( i=NumSelected; --i; ) {
			if( Selected[i]->Orders[0].Action==UnitActionUpgradeTo
				|| Selected[i]->Orders[0].Action==UnitActionResearch ) {
			    allow = 1;
			    break;
			}
		    }
		} else if ( UnitButtonTable[z]->Action == ButtonCancelTrain ) {
		    for( i=NumSelected; --i; ) {
			if( Selected[i]->Orders[0].Action==UnitActionTrain ) {
			    allow = 1;
			    break;
			}
		    }
		} else if ( UnitButtonTable[z]->Action == ButtonCancelBuild ) {
		    for( i=NumSelected; --i; ) {
			if( Selected[i]->Orders[0].Action==UnitActionBuilded ) {
			    allow = 1;
			    break;
			}
		    }
		} else {
		    allow = 1;
		}
	    }

	    if (allow) {		// is button allowed after all?
		_current_buttons[UnitButtonTable[z]->Pos-1]
			= (*UnitButtonTable[z]);
	    }
	}
    }

    CurrentButtons = _current_buttons;
    MustRedraw|=RedrawButtonPanel;
}

/**
**	Update bottom panel.
*/
global void UpdateButtonPanel(void)
{
    Unit* unit;
    char unit_ident[128];
    ButtonAction* buttonaction;
    int z;
    int allow;

    DebugLevel3Fn("update buttons\n");

    CurrentButtons=NULL;

    if( !NumSelected ) {		// no unit selected
	// FIXME: need only redraw if same state
	MustRedraw|=RedrawButtonPanel;
	return;
    }

    if( NumSelected>1 ) {		// multiple selected
        for ( allow=z = 1; z < NumSelected; z++ ) {
	    // if current type is equal to first one count it
            if ( Selected[z]->Type == Selected[0]->Type ) {
               allow++;
	    }
	}

	if ( allow != NumSelected ) {
	    // oops we have selected different units types
	    // -- set default buttons and exit
	    UpdateButtonPanelMultipleUnits();
	    return;
	}
	// we have same type units selected
	// -- continue with setting buttons as for the first unit
    }

    unit=Selected[0];
    DebugCheck( (unit==NoUnitP) );

    if( unit->Player!=ThisPlayer ) {	// foreign unit
	return;
    }

    // first clear the table
    for ( z = 0; z < 9; z++ ) {
	_current_buttons[z].Pos = -1;
    }

    //
    //	FIXME: johns: some hacks for cancel buttons
    //
    if( unit->Orders[0].Action==UnitActionBuilded ) {
	// Trick 17 to get the cancel-build button
	strcpy(unit_ident,",cancel-build,");
    } else if( unit->Orders[0].Action==UnitActionUpgradeTo ) {
	// Trick 17 to get the cancel-upgrade button
	strcpy(unit_ident,",cancel-upgrade,");
    } else if( unit->Orders[0].Action==UnitActionResearch ) {
	// Trick 17 to get the cancel-upgrade button
	strcpy(unit_ident,",cancel-upgrade,");
    } else {
	sprintf(unit_ident, ",%s,", unit->Type->Ident);
    }

    for( z = 0; z < NumUnitButtons; z++ ) {
	int pos; //keep position, midified if alt-buttons required
	//FIXME: we have to check and if these unit buttons are available
	//       i.e. if button action is ButtonTrain for example check if
	//        required unit is not restricted etc...

	buttonaction=UnitButtonTable[z];
	pos = buttonaction->Pos;

	// Same level
	if ( buttonaction->Level != CurrentButtonLevel ) {
	    continue;
	}

	if ( pos > 9 ) {	// VLADI: this allows alt-buttons
	    if ( KeyModifiers & ModifierAlt ) {
		// buttons with pos >9 are shown on if ALT is pressed
		pos -= 9;
	    } else {
		continue;
	    }
	}

	// any unit or unit in list
	if ( buttonaction->UnitMask[0] != '*'
		&& !strstr( buttonaction->UnitMask, unit_ident ) ) {
	    continue;
	}

	if ( buttonaction->Allowed ) {
	    // there is check function -- call it
	    allow=buttonaction->Allowed( unit, buttonaction);
	} else {
	    // there is no allow function -- should check dependencies
	    allow=0;
	    switch( buttonaction->Action ) {
	    case ButtonMove:
	    case ButtonStop:
	    case ButtonRepair:
	    case ButtonHarvest:
	    case ButtonButton:
	    case ButtonPatrol:
	    case ButtonStandGround:
	    case ButtonReturn:
		allow = 1;
		break;
	    case ButtonAttack:
		allow=ButtonCheckAttack(unit,buttonaction);
		break;
	    case ButtonAttackGround:
		if( Selected[0]->Type->GroundAttack ) {
		    allow = 1;
		}
		break;
	    case ButtonDemolish:
		if( Selected[0]->Type->Volatile ) {
		    allow = 1;
		}
		break;
	    case ButtonTrain:
		// Check if building queue is enabled
		if( !EnableTrainingQueue
			&& unit->Orders[0].Action==UnitActionTrain ) {
		    break;
		}
		// FALL THROUGH
	    case ButtonUpgradeTo:
	    case ButtonResearch:
	    case ButtonBuild:
		allow = CheckDependByIdent( ThisPlayer,buttonaction->ValueStr);
		if ( allow && !strncmp( buttonaction->ValueStr,
			    "upgrade-", 8 ) ) {
		    allow=UpgradeIdentAllowed( ThisPlayer,
			    buttonaction->ValueStr )=='A';
		}
		break;
	    case ButtonSpellCast:
		allow = CheckDependByIdent( ThisPlayer,buttonaction->ValueStr)
			&& UpgradeIdentAllowed( ThisPlayer,
				buttonaction->ValueStr )=='R';
		break;
	    case ButtonUnload:
		allow = (Selected[0]->Type->Transporter&&Selected[0]->InsideCount);
		break;
	    case ButtonCancel:
		allow = 1;
		break;

	    case ButtonCancelUpgrade:
		allow = unit->Orders[0].Action==UnitActionUpgradeTo
			|| unit->Orders[0].Action==UnitActionResearch;
		break;
	    case ButtonCancelTrain:
		allow = unit->Orders[0].Action==UnitActionTrain;
		break;
	    case ButtonCancelBuild:
		allow = unit->Orders[0].Action==UnitActionBuilded;
		break;

	    default:
		DebugLevel0Fn("Unsupported button-action %d\n" _C_
			buttonaction->Action);
		break;
	    }
	}

	if (allow) {		// is button allowed after all?
	    _current_buttons[pos-1] = (*buttonaction);
	}
    }

    CurrentButtons = _current_buttons;
    MustRedraw|=RedrawButtonPanel;
}

/*
**	Handle bottom button clicked.
*/
global void DoButtonButtonClicked(int button)
{
    int i;
    UnitType* type;
    DebugLevel3Fn("Button clicked %d\n" _C_ button);

    if( !CurrentButtons ) {		// no buttons
	return;
    }
    //
    //	Button not available.
    //
    if( CurrentButtons[button].Pos==-1 ) {
	return;
    }

    PlayGameSound(GameSounds.Click.Sound,MaxSampleVolume);

    //
    //	Handle action on button.
    //

    DebugLevel3Fn("Button clicked %d=%d\n" _C_ button _C_
	    CurrentButtons[button].Action);
    switch( CurrentButtons[button].Action ) {
	case ButtonUnload:
	    //
	    //	Unload on coast, transporter standing, unload all units.
	    //
	    if( NumSelected==1 && Selected[0]->Orders[0].Action==UnitActionStill
		    && CoastOnMap(Selected[0]->X,Selected[0]->Y) ) {
		SendCommandUnload(Selected[0]
			,Selected[0]->X,Selected[0]->Y,NoUnitP
			,!(KeyModifiers&ModifierShift));
		break;
	    }
	case ButtonMove:
	case ButtonPatrol:
	case ButtonHarvest:
	case ButtonAttack:
	case ButtonRepair:
	case ButtonAttackGround:
	case ButtonDemolish:
        case ButtonSpellCast:
	    if( CurrentButtons[button].Action==ButtonSpellCast
		    && (KeyModifiers&ModifierControl) ) {
		int autocast;
		SpellType *spell;
		spell=SpellTypeById(CurrentButtons[button].Value);
		if( !CanAutoCastSpell(spell) ) {
		    PlayGameSound(GameSounds.PlacementError.Sound
			    ,MaxSampleVolume);
		    break;
		}

		autocast=0;
		// If any selected unit doesn't have autocast on turn it on
		// for everyone
		for( i=0; i<NumSelected; ++i ) {
		    if( Selected[i]->AutoCastSpell!=spell) {
			autocast=1;
			break;
		    }
		}
		for( i=0; i<NumSelected; ++i ) {
		    if( !autocast || Selected[i]->AutoCastSpell!=spell ) {
			SendCommandAutoSpellCast(Selected[i]
				,CurrentButtons[button].Value,autocast);
		    }
		}
	    } else {
		CursorState=CursorStateSelect;
		GameCursor=TheUI.YellowHair.Cursor;
		CursorAction=CurrentButtons[button].Action;
		CursorValue=CurrentButtons[button].Value;
		CurrentButtonLevel=9;	// level 9 is cancel-only
		UpdateButtonPanel();
		MustRedraw|=RedrawCursor;
		SetStatusLine("Select Target");
	    }
	    break;
	case ButtonReturn:
	    for( i=0; i<NumSelected; ++i ) {
	        SendCommandReturnGoods(Selected[i],NoUnitP
			,!(KeyModifiers&ModifierShift));
	    }
	    break;
	case ButtonStop:
	    for( i=0; i<NumSelected; ++i ) {
	        SendCommandStopUnit(Selected[i]);
	    }
	    break;
	case ButtonStandGround:
	    for( i=0; i<NumSelected; ++i ) {
	        SendCommandStandGround(Selected[i]
			,!(KeyModifiers&ModifierShift));
	    }
	    break;
	case ButtonButton:
            CurrentButtonLevel=CurrentButtons[button].Value;
            UpdateButtonPanel();
	    break;

	case ButtonCancel:
	case ButtonCancelUpgrade:
	    if ( NumSelected==1 && Selected[0]->Type->Building ) {
		if( Selected[0]->Orders[0].Action==UnitActionUpgradeTo ) {
		    SendCommandCancelUpgradeTo(Selected[0]);
		} else if( Selected[0]->Orders[0].Action==UnitActionResearch ) {
		    SendCommandCancelResearch(Selected[0]);
		}
	    }
	    ClearStatusLine();
	    ClearCosts();
            CurrentButtonLevel = 0;
	    UpdateButtonPanel();
	    GameCursor=TheUI.Point.Cursor;
	    CursorBuilding=NULL;
	    CursorState=CursorStatePoint;
	    MustRedraw|=RedrawCursor;
	    break;

	case ButtonCancelTrain:
	    DebugCheck( Selected[0]->Orders[0].Action!=UnitActionTrain
		    || !Selected[0]->Data.Train.Count );
	    SendCommandCancelTraining(Selected[0],-1,NULL);
	    ClearStatusLine();
	    ClearCosts();
	    break;

	case ButtonCancelBuild:
	    // FIXME: johns is this not sure, only building should have this?
	    if( NumSelected==1 && Selected[0]->Type->Building ) {
		SendCommandCancelBuilding(Selected[0],
		        Selected[0]->Data.Builded.Worker);
	    }
	    ClearStatusLine();
	    ClearCosts();
	    break;

	case ButtonBuild:
	    // FIXME: store pointer in button table!
	    type=UnitTypes[CurrentButtons[button].Value];
	    if( !PlayerCheckUnitType(ThisPlayer,type) ) {
		SetStatusLine("Select Location");
		ClearCosts();
		CursorBuilding=type;
		// FIXME: check is this =9 necessary?
                CurrentButtonLevel=9;	// level 9 is cancel-only
		UpdateButtonPanel();
		MustRedraw|=RedrawCursor;
	    }
	    break;

	case ButtonTrain:
	    // FIXME: store pointer in button table!
	    type=UnitTypes[CurrentButtons[button].Value];
	    // FIXME: Johns: I want to place commands in queue, even if not
	    // FIXME:	enough resources are available.
	    // FIXME: training queue full check is not correct for network.
	    // FIXME: this can be correct written, with a little more code.
	    if( Selected[0]->Orders[0].Action==UnitActionTrain
		    && (Selected[0]->Data.Train.Count==MAX_UNIT_TRAIN
			|| !EnableTrainingQueue) ) {
		NotifyPlayer(Selected[0]->Player,NotifyYellow,Selected[0]->X,
			Selected[0]->Y, "Unit training queue is full" );
	    } else if( PlayerCheckFood(ThisPlayer,type)
			&& !PlayerCheckUnitType(ThisPlayer,type) ) {
		//PlayerSubUnitType(ThisPlayer,type);
		SendCommandTrainUnit(Selected[0],type
			,!(KeyModifiers&ModifierShift));
		ClearStatusLine();
		ClearCosts();
	    }
	    break;

	case ButtonUpgradeTo:
	    // FIXME: store pointer in button table!
	    type=UnitTypes[CurrentButtons[button].Value];
	    if( !PlayerCheckUnitType(ThisPlayer,type) ) {
		DebugLevel3("Upgrade to %s %d %d\n"
			_C_ type->Ident
			_C_ type->_Costs[GoldCost]
			_C_ type->_Costs[WoodCost]);
		//PlayerSubUnitType(ThisPlayer,type);
		SendCommandUpgradeTo(Selected[0],type
			,!(KeyModifiers&ModifierShift));
		ClearStatusLine();
		ClearCosts();
	    }
	    break;
	case ButtonResearch:
	    i=CurrentButtons[button].Value;
	    if( !PlayerCheckCosts(ThisPlayer,Upgrades[i].Costs) ) {
		//PlayerSubCosts(ThisPlayer,Upgrades[i].Costs);
		SendCommandResearch(Selected[0],&Upgrades[i]
			,!(KeyModifiers&ModifierShift));
		ClearStatusLine();
		ClearCosts();
	    }
	    break;
	default:
	    DebugLevel1Fn("Unknown action %d\n"
		    _C_ CurrentButtons[button].Action);
	    break;
    }
}
#endif

#ifdef NEW_UI
global void DoButtonButtonClicked(int pos)
{
    ButtonAction * ba;
    ba = CurrentButtons + pos;
    //
    //	Handle action on button.
    //
    //FIXME DebugLevel3Fn("Button clicked (button hint: %s).", ba->Hint);

    if( !gh_null_p(ba->Action) ) {
	PlayGameSound(GameSounds.Click.Sound,MaxSampleVolume);
	
	/*
	  if( [ccl debugging] ) {         // display executed command
	  gh_display(...);
	  gh_newline();
	  }
	*/
	gh_apply(ba->Action,NIL);
    } else {
	if( ba->Hint ) {
	    DebugLevel0Fn("Missing button action (button hint: %s)." _C_ ba->Hint);
	} else {
	    // FIXME: remove this after testing
	    DebugLevel0Fn("You are clicking on empty space, aren't you ;)");
	}
    }
}
#endif

/**
**	Lookup key for bottom panel buttons.
**
**	@param key	Internal key symbol for pressed key.
**
**	@return		True, if button is handled (consumed).
*/
global int DoButtonPanelKey(int key)
{
    int i;

#ifndef NEW_UI
    if( CurrentButtons ) {		// buttons

	// cade: this is required for action queues SHIFT+M should be `m'
	if ( key >= 'A' && key <= 'Z' ) {
	    key = tolower(key);
	}

	for( i=0; i<9; ++i ) {
	    if( CurrentButtons[i].Pos!=-1 && key==CurrentButtons[i].Key ) {
		DoButtonButtonClicked(i);
		return 1;
	    }
	}
    }
#else
    ButtonAction * ba;

    // cade: this is required for action queues SHIFT+M should be `m'
    if ( key >= 'A' && key <= 'Z' ) {
	key = tolower(key);
    }

    for( i=0; i<9; i++ ) {
	ba = CurrentButtons + i;
	if( key==CurrentButtons[i].Key ) {
	    DoButtonButtonClicked(i);
	    return 1;
	}
    }
#endif
    return 0;
}

//@}
