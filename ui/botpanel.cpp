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
/**@name botpanel.c - The bottom panel. */
//
//      (c) Copyright 1999-2005 by Lutz Sammer, Vladi Belperchinov-Shabanski,
//                                 and Jimmy Salmon
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
//      $Id$

//@{

/*----------------------------------------------------------------------------
--  Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "stratagus.h"

#include "unit.h"
#include "unittype.h"
#include "upgrade.h"
#include "interface.h"
#include "ui.h"
#include "player.h"
#include "spells.h"
#include "menus.h"
#include "depend.h"
#include "sound.h"
#include "map.h"
#include "commands.h"

/*----------------------------------------------------------------------------
--  Defines
----------------------------------------------------------------------------*/

	/// How many different buttons are allowed
#define MAX_BUTTONS  2048

/*----------------------------------------------------------------------------
--  Variables
----------------------------------------------------------------------------*/

	/// Display the command key in the buttons.
char ShowCommandKey;

	/// for unit buttons sub-menus etc.
int CurrentButtonLevel;
	/// All buttons for units
static ButtonAction *UnitButtonTable[MAX_BUTTONS];
	/// buttons in UnitButtonTable
static int NumUnitButtons;

ButtonAction *CurrentButtons;             /// Pointer to current buttons

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Initialize the buttons.
*/
void InitButtons(void)
{
	// Resolve the icon names.
	for (int z = 0; z < NumUnitButtons; ++z) {
		UnitButtonTable[z]->Icon.Icon =
			IconByIdent(UnitButtonTable[z]->Icon.Name);
	}
	CurrentButtons = NULL;
}

/*----------------------------------------------------------------------------
--  Buttons structures
----------------------------------------------------------------------------*/

/**
**  FIXME: docu
*/
int AddButton(int pos, int level, const char *icon_ident,
	enum _button_cmd_ action, const char *value, const ButtonCheckFunc func,
	const char *allow, int key, const char *hint, const char *umask)
{
	char buf[2048];
	ButtonAction* ba;

	ba = (ButtonAction *)malloc(sizeof(ButtonAction));
	Assert(ba);

	ba->Pos = pos;
	ba->Level = level;
	ba->Icon.Name = (char*)icon_ident;
	// FIXME: check if already initited
	//ba->Icon.Icon = IconByIdent(icon_ident);
	ba->Action = action;
	if (value) {
		ba->ValueStr = strdup(value);
		switch (action) {
			case ButtonSpellCast:
				ba->Value = SpellTypeByIdent(value)->Slot;
#ifdef DEBUG
				if (ba->Value < 0) {
					DebugPrint("Spell %s does not exist?\n" _C_ value);
					Assert(ba->Value >= 0);
				}
#endif
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
	// FIXME: here should be added costs to the hint
	// FIXME: johns: show should be nice done?
	if (umask[0] == '*') {
		strcpy(buf, umask);
	} else {
		sprintf(buf, ",%s,", umask);
	}
	ba->UnitMask = strdup(buf);
	UnitButtonTable[NumUnitButtons++] = ba;
	// FIXME: check if already initited
	//Assert(ba->Icon.Icon != NULL);// just checks, that's why at the end
	return 1;
}


/**
**  Cleanup buttons.
*/
void CleanButtons(void)
{
	// Free the allocated buttons.
	for (int z = 0; z < NumUnitButtons; ++z) {
		Assert(UnitButtonTable[z]);
		free(UnitButtonTable[z]->Icon.Name);
		free(UnitButtonTable[z]->ValueStr);
		free(UnitButtonTable[z]->AllowStr);
		free(UnitButtonTable[z]->Hint);
		free(UnitButtonTable[z]->UnitMask);
		free(UnitButtonTable[z]);
	}
	NumUnitButtons = 0;

	CurrentButtonLevel = 0;
	free(CurrentButtons);
	CurrentButtons = NULL;
}

/**
**  Return Status of button.
**
**  @param button  button to check status
**
**  @return status of button
**  @return Icon(Active | Selected | Clicked | AutoCast | Disabled).
**
**  @todo FIXME : add IconDisabled when needed.
**  @todo FIXME : Should show the rally action for training unit ? (NewOrder)
*/
static int GetButtonStatus(const ButtonAction* button)
{
	int res;
	int action;
	int i;

	Assert(button);
	Assert(NumSelected);

	res = 0;
	// cursor is on that button
	if (ButtonAreaUnderCursor == ButtonAreaButton && ButtonUnderCursor == button->Pos - 1) {
		res |= IconActive;
		if (MouseButtons & LeftButton) {
			// Overwrite IconActive.
			res = IconClicked;
		}
	}

	action = UnitActionNone;
	switch (button->Action) {
		case ButtonStop:
			action = UnitActionStill;
			break;
		case ButtonStandGround:
			action = UnitActionStandGround;
			break;
		case ButtonAttack:
			action = UnitActionAttack;
			break;
		case ButtonAttackGround:
			action = UnitActionAttackGround;
			break;
		case ButtonPatrol:
			action = UnitActionPatrol;
			break;
		case ButtonHarvest:
		case ButtonReturn:
			action = UnitActionResource;
			break;
		default:
			break;
	}
	// Simple case.
	if (action != UnitActionNone) {
		for (i = 0; i < NumSelected; ++i) {
			if (Selected[i]->Orders[0].Action != action) {
				break;
			}
		}
		if (i == NumSelected) {
			res |= IconSelected;
		}
		return res;
	}
	// other cases : manage AutoCast and different possible action.
	switch (button->Action) {
		case ButtonMove:
			for (i = 0; i < NumSelected; ++i) {
				if (Selected[i]->Orders[0].Action != UnitActionMove &&
						Selected[i]->Orders[0].Action != UnitActionBuild &&
						Selected[i]->Orders[0].Action != UnitActionFollow) {
					break;
				}
			}
			if (i == NumSelected) {
				res |= IconSelected;
			}
			break;
		case ButtonSpellCast:
			// FIXME : and IconSelected ?

			// Autocast
			for (i = 0; i < NumSelected; ++i) {
				Assert(Selected[i]->AutoCastSpell);
				if (Selected[i]->AutoCastSpell[button->Value] != 1) {
					break;
				}
			}
			if (i == NumSelected) {
				res |= IconAutoCast;
			}
			break;
		case ButtonRepair:
			for (i = 0; i < NumSelected; ++i) {
				if (Selected[i]->Orders[0].Action != UnitActionRepair) {
					break;
				}
			}
			if (i == NumSelected) {
				res |= IconSelected;
			}
			// Auto repair
			for (i = 0; i < NumSelected; ++i) {
				if (Selected[i]->AutoRepair != 1) {
					break;
				}
			}
			if (i == NumSelected) {
				res |= IconAutoCast;
			}
			break;
		// FIXME: must handle more actions
		default:
			break;
	}
	return res;
}

/**
**  Draw button panel.
**
**  Draw all action buttons.
*/
void CButtonPanel::Draw(void)
{
	Player *player;
	const ButtonAction* buttons;
	char buf[8];

	//
	//  Draw background
	//
	if (TheUI.ButtonPanel.G) {
		TheUI.ButtonPanel.G->DrawSubClip(0, 0,
			TheUI.ButtonPanel.G->Width, TheUI.ButtonPanel.G->Height,
			TheUI.ButtonPanel.X, TheUI.ButtonPanel.Y);
	}

	// No buttons
	if (!(buttons = CurrentButtons)) {
		return;
	}

	Assert(NumSelected > 0);
	player = Selected[0]->Player;

	//
	//  Draw all buttons.
	//
	for (int i = 0; i < TheUI.ButtonPanel.NumButtons; ++i) {
		if (buttons[i].Pos == -1) {
			continue;
		}
		Assert(buttons[i].Pos == i + 1);
		//
		//  Tutorial show command key in icons
		//
		if (ShowCommandKey) {
			if (CurrentButtons[i].Key == 27) {
				strcpy(buf, "ESC");
			} else {
				buf[0] = toupper(CurrentButtons[i].Key);
				buf[1] = '\0';
			}
		} else {
			buf[0] = '\0';
		}

		//
		// Draw main Icon.
		//
		DrawUnitIcon(player, TheUI.ButtonPanel.Buttons[i].Style, buttons[i].Icon.Icon,
			GetButtonStatus(&buttons[i]),
			TheUI.ButtonPanel.Buttons[i].X, TheUI.ButtonPanel.Buttons[i].Y, buf);

		//
		//  Update status line for this button
		//
		if (ButtonAreaUnderCursor == ButtonAreaButton &&
				ButtonUnderCursor == i && KeyState != KeyStateInput) {
			UpdateStatusLineForButton(&buttons[i]);
		}
	}
}

/**
**  Update the status line with hints from the button
**
**  @param button  Button
*/
void UpdateStatusLineForButton(const ButtonAction *button)
{
	int v;  // button->Value.
	const UnitStats *stats;

	Assert(button);
	SetStatusLine(button->Hint);

	v = button->Value;
	switch (button->Action) {
		case ButtonBuild:
		case ButtonTrain:
		case ButtonUpgradeTo:
			// FIXME: store pointer in button table!
			stats = &UnitTypes[v]->Stats[ThisPlayer->Index];
			SetCosts(0, UnitTypes[v]->Demand, stats->Costs);
			break;
		case ButtonResearch:
			SetCosts(0, 0, AllUpgrades[v]->Costs);
			break;
		case ButtonSpellCast:
			SetCosts(SpellTypeTable[v]->ManaCost, 0, NULL);
			break;
		default:
			ClearCosts();
			break;
	}
}

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  tell if the button is allowed for the unit.
**
**  @param unit         unit which checks for allow.
**  @param buttonaction button to check if it is allowed.
**
**  @return 1 if button is allowed, 0 else.
**
**  @todo FIXME : better check. (dependancy, resource, ...)
**  @todo FIXME : make difference with impossible and not yet researched.
*/
static int IsButtonAllowed(const Unit *unit, const ButtonAction *buttonaction)
{
	int res;

	Assert(unit);
	Assert(buttonaction);

	if (buttonaction->Allowed) {
		return buttonaction->Allowed(unit, buttonaction);
	}

	res = 0;
	// FIXME: we have to check and if these unit buttons are available
	//    i.e. if button action is ButtonTrain for example check if
	// required unit is not restricted etc...
	switch (buttonaction->Action) {
		case ButtonStop:
		case ButtonStandGround:
		case ButtonButton:
		case ButtonMove:
			res = 1;
			break;
		case ButtonRepair:
			res = unit->Type->RepairRange > 0;
			break;
		case ButtonPatrol:
			res = CanMove(unit);
			break;
		case ButtonHarvest:
			if (!unit->CurrentResource ||
					!(unit->ResourcesHeld > 0 && !unit->Type->ResInfo[unit->CurrentResource]->LoseResources) ||
					(unit->ResourcesHeld != unit->Type->ResInfo[unit->CurrentResource]->ResourceCapacity &&
						unit->Type->ResInfo[unit->CurrentResource]->LoseResources)) {
				res = 1;
			}
			break;
		case ButtonReturn:
			if (!(!unit->CurrentResource ||
					!(unit->ResourcesHeld > 0 && !unit->Type->ResInfo[unit->CurrentResource]->LoseResources) ||
					(unit->ResourcesHeld != unit->Type->ResInfo[unit->CurrentResource]->ResourceCapacity &&
						unit->Type->ResInfo[unit->CurrentResource]->LoseResources))) {
				res = 1;
			}
			break;
		case ButtonAttack:
			res = ButtonCheckAttack(unit, buttonaction);
			break;
		case ButtonAttackGround:
			if (unit->Type->GroundAttack) {
				res = 1;
			}
			break;
		case ButtonTrain:
			// Check if building queue is enabled
			if (!EnableTrainingQueue &&
					unit->Orders[0].Action == UnitActionTrain) {
				break;
			}
			// FALL THROUGH
		case ButtonUpgradeTo:
		case ButtonResearch:
		case ButtonBuild:
			res = CheckDependByIdent(unit->Player, buttonaction->ValueStr);
			if (res && !strncmp(buttonaction->ValueStr, "upgrade-", 8)) {
				res = UpgradeIdentAllowed(unit->Player, buttonaction->ValueStr) == 'A';
			}
			break;
		case ButtonSpellCast:
			res = SpellIsAvailable(unit->Player, buttonaction->Value);
			break;
		case ButtonUnload:
			res = (Selected[0]->Type->CanTransport && Selected[0]->BoardCount);
			break;
		case ButtonCancel:
			res = 1;
			break;
		case ButtonCancelUpgrade:
			res = unit->Orders[0].Action == UnitActionUpgradeTo ||
				unit->Orders[0].Action == UnitActionResearch;
			break;
		case ButtonCancelTrain:
			res = unit->Orders[0].Action == UnitActionTrain;
			break;
		case ButtonCancelBuild:
			res = unit->Orders[0].Action == UnitActionBuilt;
			break;
	}
#if 0
	// there is a additional check function -- call it
	if (res && buttonaction->Disabled) {
		return buttonaction->Disabled(unit, buttonaction);
	}
#endif
	return res;
}

/**
**  Update bottom panel for multiple units.
**
**  @return array of TheUI.ButtonPanel.NumButtons buttons to show.
**
**  @todo FIXME : make UpdateButtonPanelMultipleUnits more configurable.
**  @todo show all possible buttons or just same button...
*/
static ButtonAction *UpdateButtonPanelMultipleUnits(void)
{
	char unit_ident[128];
	int z;
	int i;
	ButtonAction *res;
	int allow;         // button is available for at least 1 unit.

	res = (ButtonAction*)calloc(TheUI.ButtonPanel.NumButtons, sizeof (*res));
	for (z = 0; z < TheUI.ButtonPanel.NumButtons; ++z) {
		res[z].Pos = -1;
	}

	sprintf(unit_ident,	",%s-group,",
			PlayerRaces.Name[ThisPlayer->Race]);

	for (z = 0; z < NumUnitButtons; ++z) {
		if (UnitButtonTable[z]->Level != CurrentButtonLevel) {
			continue;
		}

		// any unit or unit in list
		if (UnitButtonTable[z]->UnitMask[0] != '*' &&
				!strstr(UnitButtonTable[z]->UnitMask, unit_ident)) {
			continue;
		}
		allow = 1;
		for (i = 0; i < NumSelected; i++) {
			if (!IsButtonAllowed(Selected[i], UnitButtonTable[z])) {
				allow = 0;
				break;
			}
		}
		Assert(1 <= UnitButtonTable[z]->Pos);
		Assert(UnitButtonTable[z]->Pos <= TheUI.ButtonPanel.NumButtons);

		// is button allowed after all?
		if (allow) {
			// OverWrite, So take last valid button.
			res[UnitButtonTable[z]->Pos - 1] = *UnitButtonTable[z];
		}
	}
	return res;
}

/**
**  Update bottom panel for single unit.
**  or unit group with the same type.
**
**  @param unit  unit which has actions shown with buttons.
**
**  @return array of TheUI.ButtonPanel.NumButtons buttons to show.
**
**  @todo FIXME : Remove Hack for cancel button.
*/
static ButtonAction *UpdateButtonPanelSingleUnit(const Unit *unit)
{
	int allow;
	char unit_ident[128];
	ButtonAction *buttonaction;
	ButtonAction *res;
	int z;

	Assert(unit);

	res = (ButtonAction*)calloc(TheUI.ButtonPanel.NumButtons, sizeof (*res));
	for (z = 0; z < TheUI.ButtonPanel.NumButtons; ++z) {
		res[z].Pos = -1;
	}

	//
	//  FIXME: johns: some hacks for cancel buttons
	//
	if (unit->Orders[0].Action == UnitActionBuilt) {
		// Trick 17 to get the cancel-build button
		strcpy(unit_ident, ",cancel-build,");
	} else if (unit->Orders[0].Action == UnitActionUpgradeTo) {
		// Trick 17 to get the cancel-upgrade button
		strcpy(unit_ident, ",cancel-upgrade,");
	} else if (unit->Orders[0].Action == UnitActionResearch) {
		// Trick 17 to get the cancel-upgrade button
		strcpy(unit_ident, ",cancel-upgrade,");
	} else {
		sprintf(unit_ident, ",%s,", unit->Type->Ident);
	}

	for (z = 0; z < NumUnitButtons; ++z) {
		int pos; // keep position, modified if alt-buttons required

		buttonaction = UnitButtonTable[z];
		Assert(0 < buttonaction->Pos && buttonaction->Pos <= TheUI.ButtonPanel.NumButtons);

		// Same level
		if (buttonaction->Level != CurrentButtonLevel) {
			continue;
		}

		// any unit or unit in list
		if (buttonaction->UnitMask[0] != '*' &&
				!strstr(buttonaction->UnitMask, unit_ident)) {
			continue;
		}
		allow = IsButtonAllowed(unit, buttonaction);

		pos = buttonaction->Pos;

		// is button allowed after all?
		if (allow) {
			// OverWrite, So take last valid button.
			res[pos - 1] = *buttonaction;
		}
	}
	return res;
}

/**
**  Update button panel.
**
**  @internal Affect CurrentButtons with buttons to show.
*/
void CButtonPanel::Update(void)
{
	Unit *unit;
	int sameType;   // 1 if all selected units are same type, 0 else.

	// Default is no button.
	free(CurrentButtons);
	CurrentButtons = NULL;

	if (!NumSelected) {
		return;
	}

	unit = Selected[0];
	// foreign unit
	if (unit->Player != ThisPlayer &&
			!PlayersTeamed(ThisPlayer->Index, unit->Player->Index)) {
		return;
	}

	sameType = 1;
	// multiple selected
	for (int i = 1; i < NumSelected; ++i) {
		if (Selected[i]->Type != unit->Type) {
			sameType = 0;
			break;
		}
	}

	// We have selected different units types
	if (sameType == 0) {
		CurrentButtons = UpdateButtonPanelMultipleUnits();
	} else {
		// We have same type units selected
		// -- continue with setting buttons as for the first unit
		CurrentButtons = UpdateButtonPanelSingleUnit(unit);
	}
}

/**
**  Handle bottom button clicked.
**
**  @param button  Button that was clicked.
*/
void CButtonPanel::DoClicked(int button)
{
	int i;
	UnitType *type;

	Assert(0 <= button && button < TheUI.ButtonPanel.NumButtons);
	// no buttons
	if (!CurrentButtons) {
		return;
	}
	//
	//  Button not available.
	//  or Not Teamed
	//
	if (CurrentButtons[button].Pos == -1 ||
			!PlayersTeamed(ThisPlayer->Index, Selected[0]->Player->Index)) {
		return;
	}

	PlayGameSound(GameSounds.Click.Sound, MaxSampleVolume);

	//
	//  Handle action on button.
	//
	switch (CurrentButtons[button].Action) {
		case ButtonUnload:
			//
			//  Unload on coast, transporter standing, unload all units right now.
			//  That or a bunker.
			//
			if ((NumSelected == 1 && Selected[0]->Orders[0].Action == UnitActionStill &&
					CoastOnMap(Selected[0]->X, Selected[0]->Y)) || !CanMove(Selected[0])) {
				SendCommandUnload(Selected[0],
					Selected[0]->X, Selected[0]->Y, NoUnitP,
					!(KeyModifiers & ModifierShift));
				break;
			}
			CursorState = CursorStateSelect;
			GameCursor = TheUI.YellowHair.Cursor;
			CursorAction = CurrentButtons[button].Action;
			CursorValue = CurrentButtons[button].Value;
			CurrentButtonLevel = 9; // level 9 is cancel-only
			TheUI.ButtonPanel.Update();
			SetStatusLine("Select Target");
			break;
		case ButtonSpellCast:
			if (KeyModifiers & ModifierControl) {
				int autocast;
				int spellId;

				spellId = CurrentButtons[button].Value;
				if (!CanAutoCastSpell(SpellTypeTable[spellId])) {
					PlayGameSound(GameSounds.PlacementError.Sound,
						MaxSampleVolume);
					break;
				}

				autocast = 0;
				// If any selected unit doesn't have autocast on turn it on
				// for everyone
				for (i = 0; i < NumSelected; ++i) {
					if (Selected[i]->AutoCastSpell[spellId] == 0) {
						autocast = 1;
						break;
					}
				}
				for (i = 0; i < NumSelected; ++i) {
					if (Selected[i]->AutoCastSpell[spellId] != autocast) {
						SendCommandAutoSpellCast(Selected[i],
							spellId, autocast);
					}
				}
				break;
			}
			// Follow Next -> Select target.
		case ButtonRepair:
			if (KeyModifiers & ModifierControl) {
				unsigned autorepair;

				autorepair = 0;
				// If any selected unit doesn't have autocast on turn it on
				// for everyone
				for (i = 0; i < NumSelected; ++i) {
					if (Selected[i]->AutoRepair == 0) {
						autorepair = 1;
						break;
					}
				}
				for (i = 0; i < NumSelected; ++i) {
					if (Selected[i]->AutoRepair != autorepair) {
						SendCommandAutoRepair(Selected[i], autorepair);
					}
				}
				break;
			}
			// Follow Next -> Select target.
		case ButtonMove:
		case ButtonPatrol:
		case ButtonHarvest:
		case ButtonAttack:
		case ButtonAttackGround:
			// Select target.
			CursorState = CursorStateSelect;
			GameCursor = TheUI.YellowHair.Cursor;
			CursorAction = CurrentButtons[button].Action;
			CursorValue = CurrentButtons[button].Value;
			CurrentButtonLevel = 9; // level 9 is cancel-only
			TheUI.ButtonPanel.Update();
			SetStatusLine("Select Target");
			break;
		case ButtonReturn:
			for (i = 0; i < NumSelected; ++i) {
				SendCommandReturnGoods(Selected[i], NoUnitP,
					!(KeyModifiers & ModifierShift));
			}
			break;
		case ButtonStop:
			for (i = 0; i < NumSelected; ++i) {
				SendCommandStopUnit(Selected[i]);
			}
			break;
		case ButtonStandGround:
			for (i = 0; i < NumSelected; ++i) {
				SendCommandStandGround(Selected[i],
					!(KeyModifiers & ModifierShift));
			}
			break;
		case ButtonButton:
			CurrentButtonLevel = CurrentButtons[button].Value;
			TheUI.ButtonPanel.Update();
			break;

		case ButtonCancel:
		case ButtonCancelUpgrade:
			if (NumSelected == 1) {
				if (Selected[0]->Orders[0].Action == UnitActionUpgradeTo) {
					SendCommandCancelUpgradeTo(Selected[0]);
				} else if (Selected[0]->Orders[0].Action == UnitActionResearch) {
					SendCommandCancelResearch(Selected[0]);
				}
			}
			ClearStatusLine();
			ClearCosts();
			CurrentButtonLevel = 0;
			TheUI.ButtonPanel.Update();
			GameCursor = TheUI.Point.Cursor;
			CursorBuilding = NULL;
			CursorState = CursorStatePoint;
			break;

		case ButtonCancelTrain:
			Assert(Selected[0]->Orders[0].Action == UnitActionTrain);
			SendCommandCancelTraining(Selected[0], -1, NULL);
			ClearStatusLine();
			ClearCosts();
			break;

		case ButtonCancelBuild:
			// FIXME: johns is this not sure, only building should have this?
			Assert(Selected[0]->Orders[0].Action == UnitActionBuilt);
			if (NumSelected == 1) {
				SendCommandDismiss(Selected[0]);
			}
			ClearStatusLine();
			ClearCosts();
			break;

		case ButtonBuild:
			// FIXME: store pointer in button table!
			type = UnitTypes[CurrentButtons[button].Value];
			if (!PlayerCheckUnitType(Selected[0]->Player, type)) {
				SetStatusLine("Select Location");
				ClearCosts();
				CursorBuilding = type;
				// FIXME: check is this =9 necessary?
				CurrentButtonLevel = 9; // level 9 is cancel-only
				TheUI.ButtonPanel.Update();
			}
			break;

		case ButtonTrain:
			// FIXME: store pointer in button table!
			type = UnitTypes[CurrentButtons[button].Value];
			// FIXME: Johns: I want to place commands in queue, even if not
			// FIXME:        enough resources are available.
			// FIXME: training queue full check is not correct for network.
			// FIXME: this can be correct written, with a little more code.
			if (Selected[0]->Orders[0].Action == UnitActionTrain &&
					!EnableTrainingQueue) {
				NotifyPlayer(Selected[0]->Player, NotifyYellow, Selected[0]->X,
					Selected[0]->Y, "Unit training queue is full");
			} else if (PlayerCheckLimits(Selected[0]->Player, type) >= 0 &&
					!PlayerCheckUnitType(Selected[0]->Player, type)) {
				//PlayerSubUnitType(player,type);
				SendCommandTrainUnit(Selected[0], type,
					!(KeyModifiers & ModifierShift));
				ClearStatusLine();
				ClearCosts();
			}
			break;

		case ButtonUpgradeTo:
			// FIXME: store pointer in button table!
			type = UnitTypes[CurrentButtons[button].Value];
			if (!PlayerCheckUnitType(Selected[0]->Player, type)) {
				//PlayerSubUnitType(player,type);
				SendCommandUpgradeTo(Selected[0],type,
					!(KeyModifiers & ModifierShift));
				ClearStatusLine();
				ClearCosts();
			}
			break;
		case ButtonResearch:
			i = CurrentButtons[button].Value;
			if (!PlayerCheckCosts(Selected[0]->Player, AllUpgrades[i]->Costs)) {
				//PlayerSubCosts(player,Upgrades[i].Costs);
				SendCommandResearch(Selected[0], AllUpgrades[i],
					!(KeyModifiers & ModifierShift));
				ClearStatusLine();
				ClearCosts();
			}
			break;
	}
}


/**
**  Lookup key for bottom panel buttons.
**
**  @param key  Internal key symbol for pressed key.
**
**  @return     True, if button is handled (consumed).
*/
int CButtonPanel::DoKey(int key)
{
	if (CurrentButtons) {
		// This is required for action queues SHIFT+M should be `m'
		if (isascii(key) && isupper(key)) {
			key = tolower(key);
		}

		for (int i = 0; i < TheUI.ButtonPanel.NumButtons; ++i) {
			if (CurrentButtons[i].Pos != -1 && key == CurrentButtons[i].Key) {
				TheUI.ButtonPanel.DoClicked(i);
				return 1;
			}
		}
	}
	return 0;
}

//@}
