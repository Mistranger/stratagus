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
/**@name mainscr.c	-	The main screen. */
//
//	(c) Copyright 1998,2000-2003 by Lutz Sammer, Valery Shchedrin,
//	                             and Jimmy Salmon
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
--		Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "stratagus.h"
#include "video.h"
#include "font.h"
#include "sound_id.h"
#include "unitsound.h"
#include "unittype.h"
#include "player.h"
#include "unit.h"
#include "upgrade.h"
#include "icons.h"
#include "interface.h"
#include "ui.h"
#include "map.h"
#include "trigger.h"

/*----------------------------------------------------------------------------
--		Defines
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--		Functions
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--		Icons
----------------------------------------------------------------------------*/

/**
**		Draw life bar of an unit at x,y.
**
**		Placed under icons on top-panel.
**
**		@param unit		Pointer to unit.
**		@param x		Screen X postion of icon
**		@param y		Screen Y postion of icon
*/
local void UiDrawLifeBar(const Unit* unit, int x, int y)
{
	int f;
#ifdef USE_SDL_SURFACE
	Uint32 color;
#else
	VMemType color;
#endif

	y += unit->Type->Icon.Icon->Height + 7;
	VideoFillRectangleClip(ColorBlack, x, y,
		unit->Type->Icon.Icon->Width + 7, 7);
	if (unit->HP) {
		f = (100 * unit->HP) / unit->Stats->HitPoints;
		if (f > 75) {
			color = ColorDarkGreen;
		} else if (f > 50) {
			color = ColorYellow;
		} else if (f > 25) {
			color = ColorOrange;
		} else {
			color = ColorRed;
		}
		f = (f * (unit->Type->Icon.Icon->Width + 5)) / 100;
		VideoFillRectangleClip(color, x + 1, y + 1, f, 5);
	}
}

/**
**		Draw mana bar of an unit at x,y.
**
**		Placed under icons on top-panel.
**
**		@param unit		Pointer to unit.
**		@param x		Screen X postion of icon
**		@param y		Screen Y postion of icon
*/
local void UiDrawManaBar(const Unit* unit, int x, int y)
{
	int f;

	y += unit->Type->Icon.Icon->Height + 7;
	VideoFillRectangleClip(ColorBlack, x, y + 3,
		unit->Type->Icon.Icon->Width + 7, 4);
	if (unit->HP) {
		// s0m3body: mana bar should represent proportional value of Mana
		// with respect to MaxMana (unit->Type->_MaxMana) for the unit
		f = (100 * unit->Mana) / unit->Type->_MaxMana;
		f = (f * (unit->Type->Icon.Icon->Width + 5)) / 100;
		VideoFillRectangleClip(ColorBlue, x + 1, y + 3 + 1, f, 2);
	}
}

/**
**		Draw completed bar into top-panel.
**
**		@param full		the 100% value
**		@param ready		how much till now completed
*/
local void UiDrawCompleted(int full, int ready)
{
	int f;

	if (!full) {
		return;
	}
	f = (100 * ready) / full;
	f = (f * TheUI.CompletedBarW) / 100;
	VideoFillRectangleClip(TheUI.CompletedBarColor,
		TheUI.CompletedBarX, TheUI.CompletedBarY, f, TheUI.CompletedBarH);
	if (TheUI.CompletedBarText) {
		VideoDrawText(TheUI.CompletedBarTextX, TheUI.CompletedBarTextY,
			TheUI.CompletedBarFont, TheUI.CompletedBarText);
	}
}

/**
**		Draw the stat for an unit in top-panel.
**
**		@param x		Screen X position
**		@param y		Screen Y position
**		@param modified		The modified stat value
**		@param original		The original stat value
*/
local void DrawStats(int x, int y, int modified, int original)
{
	char buf[64];

	if (modified == original) {
		VideoDrawNumber(x, y, GameFont, modified);
	} else {
		sprintf(buf, "%d~<+%d~>", original, modified - original);
		VideoDrawText(x, y, GameFont, buf);
	}
}

/**
**		Draw the unit info into top-panel.
**
**		@param unit		Pointer to unit.
*/
global void DrawUnitInfo(const Unit* unit)
{
	char buf[64];
	const UnitType* type;
	const UnitStats* stats;
	int i;
	int vpos;
	int x;
	int y;
	Unit* uins;

	type = unit->Type;
	stats = unit->Stats;
#ifdef DEBUG
	if (!type) {
		DebugLevel1Fn(" FIXME: free unit selected\n");
		return;
	}
#endif

	//
	//		Draw icon in upper left corner
	//
	if (TheUI.SingleSelectedText) {
		VideoDrawText(TheUI.SingleSelectedTextX, TheUI.SingleSelectedTextY,
			TheUI.SingleSelectedFont, TheUI.SingleSelectedText);
	}

	// FIXME: allow without button
	if (TheUI.SingleSelectedButton) {
		x = TheUI.SingleSelectedButton->X;
		y = TheUI.SingleSelectedButton->Y;
		DrawUnitIcon(unit->Player, type->Icon.Icon,
			(ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0) ?
				(IconActive | (MouseButtons & LeftButton)) : 0,
			x, y);
		UiDrawLifeBar(unit, x, y);

		if (unit->Player == ThisPlayer) {		// Only for own units.
			if (unit->HP && unit->HP < 10000) {
				sprintf(buf, "%d/%d", unit->HP, stats->HitPoints);
				VideoDrawTextCentered(x + (type->Icon.Icon->Width + 7) / 2,
					y + type->Icon.Icon->Height + 7 + 7 + 3, SmallFont, buf);
			}
		}
	}

	x = TheUI.InfoPanelX;
	y = TheUI.InfoPanelY;

	//
	//		Draw unit name centered, if too long split at space.
	//
	i = VideoTextLength(GameFont, type->Name);
	if (i > 110) {						// didn't fit on line
		const char* s;

		s = strchr(type->Name, ' ');
		DebugCheck(!s);
		i = s-type->Name;
		memcpy(buf, type->Name, i);
		buf[i] = '\0';
		VideoDrawTextCentered(x + 114, y + 8 + 3, GameFont, buf);
		VideoDrawTextCentered(x + 114, y + 8 + 17, GameFont, s + 1);
	} else {
		VideoDrawTextCentered(x + 114, y + 8 + 17, GameFont, type->Name);
	}

	//
	//		Draw unit level.
	//
	if (stats->Level) {
		sprintf(buf, "Level ~<%d~>", stats->Level);
		VideoDrawTextCentered(x + 114, y + 8 + 33, GameFont, buf);
	}

	//
	//		Show How much a resource has left for owner and neutral.
	//
	if (unit->Player == ThisPlayer || unit->Player->Player == PlayerNumNeutral) {
		if (type->GivesResource) {
			sprintf(buf, "%s Left:", DefaultResourceNames[type->GivesResource]);
			VideoDrawText(x + 108 - VideoTextLength(GameFont, buf), y + 8 + 78,
				GameFont, buf);
			if (!unit->Value) {
				VideoDrawText(x + 108, y + 8 + 78, GameFont, "(none)");
			} else {
				VideoDrawNumber(x + 108, y + 8 + 78, GameFont, unit->Value);
			}
			return;
		}
	}

	//
	//		Only for owning player.
	//
#ifndef DEBUG
	if (unit->Player != ThisPlayer) {
		return;
	}
#endif

	//
	//		Draw unit kills and experience.
	//
	if (stats->Level && !(type->Transporter && unit->InsideCount)) {
		sprintf(buf, "XP:~<%d~> Kills:~<%d~>", unit->XP, unit->Kills);
		VideoDrawTextCentered(x + 114, y + 8 + 15 + 33, GameFont, buf);
	}

	//
	//		Show progress for buildings only, if they are selected.
	//
	if (type->Building && NumSelected == 1 && Selected[0] == unit) {
		//
		//		Building under constuction.
		//
		if (unit->Orders[0].Action == UnitActionBuilded) {
			if (unit->Data.Builded.Worker) {
				// FIXME: Position must be configured!
				DrawUnitIcon(unit->Data.Builded.Worker->Player,
					unit->Data.Builded.Worker->Type->Icon.Icon,
					0, x + 107, y + 8 + 70);
			}
			// FIXME: not correct must use build time!!
			UiDrawCompleted(stats->HitPoints, unit->HP);
			return;
		}

		//
		//		Building training units.
		//
		if (unit->Orders[0].Action == UnitActionTrain) {
			if (unit->Data.Train.Count == 1) {
				if (TheUI.SingleTrainingText) {
					VideoDrawText(TheUI.SingleTrainingTextX, TheUI.SingleTrainingTextY,
						TheUI.SingleTrainingFont, TheUI.SingleTrainingText);
				}
				if (TheUI.SingleTrainingButton) {
					DrawUnitIcon(unit->Player, unit->Data.Train.What[0]->Icon.Icon,
						(ButtonAreaUnderCursor == ButtonAreaTraining &&
							ButtonUnderCursor == 0) ?
							(IconActive | (MouseButtons & LeftButton)) : 0,
						TheUI.SingleTrainingButton->X, TheUI.SingleTrainingButton->Y);
				}

				UiDrawCompleted(unit->Data.Train.What[0]->Stats[
					unit->Player->Player].Costs[TimeCost],
					unit->Data.Train.Ticks);
			} else {
				if (TheUI.TrainingText) {
					VideoDrawTextCentered(TheUI.TrainingTextX, TheUI.TrainingTextY,
						TheUI.TrainingFont, TheUI.TrainingText);
				}
				if (TheUI.TrainingButtons) {
					for (i = 0; i < unit->Data.Train.Count &&
							i < TheUI.NumTrainingButtons; ++i) {
						DrawUnitIcon(unit->Player, unit->Data.Train.What[i]->Icon.Icon,
							(ButtonAreaUnderCursor == ButtonAreaTraining &&
								ButtonUnderCursor == i) ?
								(IconActive | (MouseButtons & LeftButton)) : 0,
							TheUI.TrainingButtons[i].X, TheUI.TrainingButtons[i].Y);
					}
				}

				UiDrawCompleted(unit->Data.Train.What[0]->Stats[
					unit->Player->Player].Costs[TimeCost],
					unit->Data.Train.Ticks);
			}
			return;
		}

		//
		//		Building upgrading to better type.
		//
		if (unit->Orders[0].Action == UnitActionUpgradeTo) {
			if (TheUI.UpgradingText) {
				VideoDrawText(TheUI.UpgradingTextX, TheUI.UpgradingTextY,
					TheUI.UpgradingFont, TheUI.UpgradingText);
			}
			if (TheUI.UpgradingButton) {
				DrawUnitIcon(unit->Player, unit->Orders[0].Type->Icon.Icon,
					(ButtonAreaUnderCursor == ButtonAreaUpgrading &&
						ButtonUnderCursor == 0) ?
						(IconActive | (MouseButtons & LeftButton)) : 0,
					TheUI.UpgradingButton->X, TheUI.UpgradingButton->Y);
			}

			UiDrawCompleted(unit->Orders[0].Type->Stats[
				unit->Player->Player].Costs[TimeCost],
				unit->Data.UpgradeTo.Ticks);
			return;
		}

		//
		//		Building research new technologie.
		//
		if (unit->Orders[0].Action == UnitActionResearch) {
			if (TheUI.ResearchingText) {
				VideoDrawText(TheUI.ResearchingTextX, TheUI.ResearchingTextY,
					TheUI.ResearchingFont, TheUI.ResearchingText);
			}
			if (TheUI.ResearchingButton) {
				DrawUnitIcon(unit->Player, unit->Data.Research.Upgrade->Icon.Icon,
					(ButtonAreaUnderCursor == ButtonAreaResearching &&
						ButtonUnderCursor == 0) ?
						(IconActive | (MouseButtons & LeftButton)) : 0,
					TheUI.ResearchingButton->X, TheUI.ResearchingButton->Y);
			}

			UiDrawCompleted(unit->Data.Research.Upgrade->Costs[TimeCost],
				unit->Player->UpgradeTimers.Upgrades[
					unit->Data.Research.Upgrade - Upgrades]);
			return;
		}
	}

	vpos = 77; // Start of resource drawing
	for (i = 1; i < MaxCosts; ++i) {
		if (type->CanStore[i]) {
			if (vpos == 77) {
				VideoDrawText(x + 20, y + 8 + 61, GameFont, "Production");
			}
			sprintf(buf, "%s:", DefaultResourceNames[i]);
			VideoDrawText(x + 78 - VideoTextLength(GameFont, buf),
				y + 8 + vpos, GameFont, buf);
			VideoDrawNumber(x + 78, y + 8 + vpos, GameFont, DefaultIncomes[i]);
			// Incomes have been upgraded
			if (unit->Player->Incomes[i] != DefaultIncomes[i]) {
				sprintf(buf, "~<+%i~>",
					unit->Player->Incomes[i] - DefaultIncomes[i]);
				VideoDrawText(x + 96, y + 8 + vpos, GameFont, buf);
			}
			sprintf(buf, "(%+.1f)", unit->Player->Revenue[i] / 1000.0);
			VideoDrawText(x + 120, y + 8 + vpos, GameFont, buf);
			vpos += 16;
		}
	}
	if (vpos != 77) {
		// We displayed at least one resource
		return;
	}

	if (type->Transporter && unit->InsideCount) {
		if (TheUI.TransportingText) {
			VideoDrawText(TheUI.TransportingTextX, TheUI.TransportingTextY,
				TheUI.TransportingFont, TheUI.TransportingText);
		}
		uins = unit->UnitInside;
		for (i = 0; i < unit->InsideCount; ++i, uins = uins->NextContained) {
			DrawUnitIcon(unit->Player,uins->Type->Icon.Icon,
				(ButtonAreaUnderCursor == ButtonAreaTransporting && ButtonUnderCursor == i) ?
					(IconActive | (MouseButtons & LeftButton)) : 0,
				TheUI.TransportingButtons[i].X, TheUI.TransportingButtons[i].Y);
			UiDrawLifeBar(uins, TheUI.TransportingButtons[i].X, TheUI.TransportingButtons[i].Y);
			if (uins->Type->CanCastSpell && unit->Type->_MaxMana) {
				UiDrawManaBar(uins, TheUI.TransportingButtons[i].X, TheUI.TransportingButtons[i].Y);
			}
			if (ButtonAreaUnderCursor == ButtonAreaTransporting && ButtonUnderCursor == i) {
				SetStatusLine(uins->Type->Name);
			}
		}
		return;
	}

	if (type->Building && !type->CanAttack) {
		if (type->Supply) {				// Supply unit
			VideoDrawText(x + 16, y + 8 + 63, GameFont, "Usage");
			VideoDrawText(x + 58, y + 8 + 78, GameFont, "Supply:");
			VideoDrawNumber(x + 108, y + 8 + 78, GameFont, unit->Player->Supply);
			VideoDrawText(x + 51, y + 8 + 94, GameFont, "Demand:");
			if (unit->Player->Supply < unit->Player->Demand) {
				VideoDrawReverseNumber(x + 108, y + 8 + 94, GameFont,
					unit->Player->Demand);
			} else {
				VideoDrawNumber(x + 108, y + 8 + 94, GameFont,
					unit->Player->Demand);
			}
		}
	} else {
		// FIXME: Level was centered?
		//sprintf(buf, "Level ~<%d~>", stats->Level);
		//VideoDrawText(x + 91, y + 8 + 33, GameFont, buf);

		VideoDrawText(x + 57, y + 8 + 63, GameFont, "Armor:");
		DrawStats(x + 108, y + 8 + 63, stats->Armor, type->_Armor);

		VideoDrawText(x + 47, y + 8 + 78, GameFont, "Damage:");
		if ((i = type->_BasicDamage+type->_PiercingDamage)) {
			// FIXME: this seems not correct
			//				Catapult has 25-80
			//				turtle has 10-50
			//				jugger has 50-130
			//				ship has 2-35
			if (stats->PiercingDamage != type->_PiercingDamage) {
				if (stats->PiercingDamage < 30 && stats->BasicDamage < 30) {
					sprintf(buf, "%d-%d~<+%d+%d~>",
						(stats->PiercingDamage + 1) / 2, i,
						stats->BasicDamage - type->_BasicDamage +
							(int)isqrt(unit->XP / 100) * XpDamage,
						stats->PiercingDamage-type->_PiercingDamage);
				} else {
					sprintf(buf, "%d-%d~<+%d+%d~>",
						(stats->PiercingDamage + stats->BasicDamage - 30) / 2, i,
						stats->BasicDamage - type->_BasicDamage +
							(int)isqrt(unit->XP / 100) * XpDamage,
						stats->PiercingDamage-type->_PiercingDamage);
				}
			} else if (stats->PiercingDamage || stats->BasicDamage < 30) {
				sprintf(buf, "%d-%d", (stats->PiercingDamage + 1) / 2, i);
			} else {
				sprintf(buf, "%d-%d", (stats->BasicDamage - 30) / 2, i);
			}
		} else {
			strcpy(buf, "0");
		}
		VideoDrawText(x + 108, y + 8 + 78, GameFont, buf);

		VideoDrawText(x + 57, y + 8 + 94, GameFont, "Range:");
		DrawStats(x + 108, y + 8 + 94, stats->AttackRange, type->_AttackRange);

		VideoDrawText(x + 64, y + 8 + 110, GameFont, "Sight:");
		DrawStats(x + 108, y + 8 + 110, stats->SightRange, type->_SightRange);

		VideoDrawText(x + 63, y + 8 + 125, GameFont, "Speed:");
		DrawStats(x + 108, y + 8 + 125, stats->Speed, type->_Speed);

		// FIXME: Ugly hack.
		if (unit->Type->Harvester && unit->Value) {
			sprintf(buf, "Carry: %d %s", unit->Value,
				DefaultResourceNames[unit->CurrentResource]);
			VideoDrawText(x + 61, y + 8 + 141, GameFont, buf);
		}
		if ((unit->Type->Harvester) &&
				(unit->Orders->Action == UnitActionResource) &&
				(unit->CurrentResource) &&
				(unit->Type->ResInfo[unit->CurrentResource]) &&
				(unit->SubAction == 60) &&
				(!unit->Type->ResInfo[unit->CurrentResource]->ResourceStep)) {
			sprintf(buf, "%s: %d%%", DefaultResourceNames[unit->CurrentResource],
					100 * (unit->Type->ResInfo[unit->CurrentResource]->WaitAtResource -
						unit->Data.ResWorker.TimeToHarvest) /
						unit->Type->ResInfo[unit->CurrentResource]->WaitAtResource);
			VideoDrawText(x + 61, y + 8 + 141, GameFont, buf);
		}

	}
	//
	//		Unit can cast spell without mana, so only show mana bar for units with mana
	//
	if (type->_MaxMana) {
		if (0) {
			VideoDrawText(x + 59, y + 8 + 140 + 1, GameFont, "Magic:");
			VideoDrawRectangleClip(ColorGray, x + 108, y + 8 + 140, 61, 14);
			VideoDrawRectangleClip(ColorBlack, x + 108 + 1, y + 8 + 140 + 1, 61 - 2, 14 - 2);
			i = (100 * unit->Mana) / unit->Type->_MaxMana;
			i = (i * (61 - 4)) / 100;
			VideoFillRectangleClip(ColorBlue, x + 108 + 2, y + 8 + 140 + 2, i, 14 - 4);

			VideoDrawNumber(x + 128, y + 8 + 140 + 1, GameFont, unit->Mana);
		} else {
			int w;
			w = 140;
			/* fix to display mana bar properly for any maxmana value */
			/* max mana can vary for the unit */
			i = (100 * unit->Mana) / unit->Type->_MaxMana;
			i = (i * w) / 100;
			VideoDrawRectangleClip(ColorGray, x + 16,	 y + 8 + 140,	 w + 4, 16	);
			VideoDrawRectangleClip(ColorBlack,x + 16 + 1, y + 8 + 140 + 1, w + 2, 16 - 2);
			VideoFillRectangleClip(ColorBlue, x + 16 + 2, y + 8 + 140 + 2, i,	 16 - 4);

			VideoDrawNumber(x + 16 + w / 2, y + 8 + 140 + 1, GameFont, unit->Mana);
		}
	}
}

/*----------------------------------------------------------------------------
--		RESOURCES
----------------------------------------------------------------------------*/

/**
**		Draw the player resource in top line.
*/
global void DrawResources(void)
{
	char tmp[128];
	int i;
	int v;

	if (TheUI.Resource.Graphic) {
		VideoDrawSubClip(TheUI.Resource.Graphic, 0, 0,
			TheUI.Resource.Graphic->Width,
			TheUI.Resource.Graphic->Height,
			TheUI.ResourceX,TheUI.ResourceY);
	}

	for (i = 0; i < MaxCosts; ++i) {
		if (TheUI.Resources[i].Icon.Graphic) {
			VideoDrawSubClip(TheUI.Resources[i].Icon.Graphic, 0,
				TheUI.Resources[i].IconRow * TheUI.Resources[i].IconH,
				TheUI.Resources[i].IconW, TheUI.Resources[i].IconH,
				TheUI.Resources[i].IconX, TheUI.Resources[i].IconY);
		}
		if (TheUI.Resources[i].TextX != -1) {
			v = ThisPlayer->Resources[i];
			VideoDrawNumber(TheUI.Resources[i].TextX,
				TheUI.Resources[i].TextY + (v > 99999) * 3,
				v > 99999 ? SmallFont : GameFont, v);
		}
	}
	if (TheUI.Resources[FoodCost].Icon.Graphic) {
		VideoDrawSubClip(TheUI.Resources[FoodCost].Icon.Graphic, 0,
			TheUI.Resources[FoodCost].IconRow * TheUI.Resources[FoodCost].IconH,
			TheUI.Resources[FoodCost].IconW, TheUI.Resources[FoodCost].IconH,
			TheUI.Resources[FoodCost].IconX, TheUI.Resources[FoodCost].IconY);
	}
	if (TheUI.Resources[FoodCost].TextX != -1) {
		sprintf(tmp, "%d/%d", ThisPlayer->Demand, ThisPlayer->Supply);
		if (ThisPlayer->Supply < ThisPlayer->Demand) {
			VideoDrawReverseText(TheUI.Resources[FoodCost].TextX,
				TheUI.Resources[FoodCost].TextY, GameFont, tmp);
		} else {
			VideoDrawText(TheUI.Resources[FoodCost].TextX,
				TheUI.Resources[FoodCost].TextY, GameFont, tmp);
		}
	}

	if (TheUI.Resources[ScoreCost].Icon.Graphic) {
		VideoDrawSubClip(TheUI.Resources[ScoreCost].Icon.Graphic, 0,
			TheUI.Resources[ScoreCost].IconRow * TheUI.Resources[ScoreCost].IconH,
			TheUI.Resources[ScoreCost].IconW, TheUI.Resources[ScoreCost].IconH,
			TheUI.Resources[ScoreCost].IconX, TheUI.Resources[ScoreCost].IconY);
	}
	if (TheUI.Resources[ScoreCost].TextX != -1) {
		v = ThisPlayer->Score;
		VideoDrawNumber(TheUI.Resources[ScoreCost].TextX,
			TheUI.Resources[ScoreCost].TextY + (v > 99999) * 3,
			v > 99999 ? SmallFont : GameFont, v);
	}
}

/*----------------------------------------------------------------------------
--		MESSAGE
----------------------------------------------------------------------------*/

// FIXME: move messages to console code.

#define MESSAGES_TIMEOUT  (FRAMES_PER_SECOND * 5)		/// Message timeout 5 seconds

local unsigned long   MessagesFrameTimeout;		/// Frame to expire message


#define MESSAGES_MAX  10						/// How many can be displayed

local char Messages[MESSAGES_MAX][128];				/// Array of messages
local int  MessagesCount;						/// Number of messages
local int  MessagesSameCount;						/// Counts same message repeats
local int  MessagesScrollY;						/// Used for smooth scrolling

local char MessagesEvent[MESSAGES_MAX][64];		/// Array of event messages
local int  MessagesEventX[MESSAGES_MAX];		/// X coordinate of event
local int  MessagesEventY[MESSAGES_MAX];		/// Y coordinate of event
local int  MessagesEventCount;						/// Number of event messages
local int  MessagesEventIndex;						/// FIXME: docu


/**
**		Shift messages array by one.
*/
local void ShiftMessages(void)
{
	int z;

	if (MessagesCount) {
		--MessagesCount;
		for (z = 0; z < MessagesCount; ++z) {
			strcpy(Messages[z], Messages[z + 1]);
		}
	}
}

/**
**		Shift messages events array by one.
*/
local void ShiftMessagesEvent(void)
{
	int z;

	if (MessagesEventCount) {
		--MessagesEventCount;
		for (z = 0; z < MessagesEventCount; ++z) {
			MessagesEventX[z] = MessagesEventX[z + 1];
			MessagesEventY[z] = MessagesEventY[z + 1];
			strcpy(MessagesEvent[z], MessagesEvent[z + 1]);
		}
	}
}

/**
**		Update messages
**
**		@todo FIXME: make scroll speed configurable.
*/
global void UpdateMessages(void)
{
	if (!MessagesCount) {
		return;
	}

	// Scroll/remove old message line
	if (MessagesFrameTimeout < FrameCounter) {
		++MessagesScrollY;
		if (MessagesScrollY == VideoTextHeight(GameFont) + 1) {
			MessagesFrameTimeout = FrameCounter + MESSAGES_TIMEOUT - MessagesScrollY;
			MessagesScrollY = 0;
			ShiftMessages();
		}
		MustRedraw |= RedrawMessage;
		// FIXME: for performance the minimal area covered by msg's should be used
		MarkDrawEntireMap();
	}
}

/**
**		Draw message(s).
**
**		@todo FIXME: make message font configurable.
*/
global void DrawMessages(void)
{
	int z;

	// Draw message line(s)
	for (z = 0; z < MessagesCount; ++z) {
		if (z == 0) {
			PushClipping();
			SetClipping(TheUI.MapArea.X + 8, TheUI.MapArea.Y + 8, VideoWidth - 1,
				VideoHeight - 1);
		}
		VideoDrawTextClip(TheUI.MapArea.X + 8,
			TheUI.MapArea.Y + 8 + z * (VideoTextHeight(GameFont) + 1) - MessagesScrollY,
			GameFont, Messages[z]);
		if (z == 0) {
			PopClipping();
		}
	}
	if (MessagesCount < 1) {
		MessagesSameCount = 0;
	}
}

/**
**		Adds message to the stack
**
**		@param msg		Message to add.
*/
local void AddMessage(const char* msg)
{
	char* ptr;
	char* message;
	char* next;

	if (!MessagesCount) {
		MessagesFrameTimeout = FrameCounter + MESSAGES_TIMEOUT;
	}

	if (MessagesCount == MESSAGES_MAX) {
		// Out of space to store messages, can't scroll smoothly
		ShiftMessages();
		MessagesFrameTimeout = FrameCounter + MESSAGES_TIMEOUT;
		MessagesScrollY = 0;
	}

	message = Messages[MessagesCount];
	// Split long message into lines
	if (strlen(msg) >= sizeof(Messages[0])) {
		strncpy(message, msg, sizeof(Messages[0]) - 1);
		ptr = message + sizeof(Messages[0]) - 1;
		*ptr-- = '\0';
		next = ptr + 1;
		while (ptr >= message) {
			if (*ptr == ' ') {
				*ptr = '\0';
				next = ptr + 1;
				break;
			}
			--ptr;
		}
		if (ptr < message) {
			ptr = next - 1;
		}
	} else {
		strcpy(message, msg);
		next = ptr = message + strlen(message);
	}

	// FIXME: 440+(VideoWidth-640) is the wrong value.
	while (VideoTextLength(GameFont, message) >= 440 + (VideoWidth - 640)) {
		while (1) {
			--ptr;
			if (*ptr == ' ') {
				*ptr = '\0';
				next = ptr + 1;
				break;
			} else if (ptr == message) {
				break;
			}
		}
		// No space found, wrap in the middle of a word
		if (ptr == message) {
			ptr = next - 1;
			while (VideoTextLength(GameFont, message) >= 440 + (VideoWidth - 640)) {
				*--ptr = '\0';
			}
			next = ptr + 1;
			break;
		}
	}

	++MessagesCount;

	if (strlen(msg) != (size_t)(ptr - message)) {
		AddMessage(msg + (next - message));
	}
}

/**
**		Check if this message repeats
**
**		@param msg		Message to check.
**		@return				non-zero to skip this message
*/
local int CheckRepeatMessage(const char* msg)
{
	if (MessagesCount < 1) {
		return 0;
	}
	if (!strcmp(msg, Messages[MessagesCount - 1])) {
		++MessagesSameCount;
		return 1;
	}
	if (MessagesSameCount > 0) {
		char temp[128];
		int n;

		n = MessagesSameCount;
		MessagesSameCount = 0;
		// NOTE: vladi: yep it's a tricky one, but should work fine prbably :)
		sprintf(temp, "Last message repeated ~<%d~> times", n + 1);
		AddMessage(temp);
	}
	return 0;
}

/**
**		Set message to display.
**
**		@param fmt		To be displayed in text overlay.
*/
global void SetMessage(const char* fmt, ...)
{
	char temp[512];
	va_list va;

	va_start(va, fmt);
	vsprintf(temp, fmt, va);				// BUG ALERT: buffer overrun
	va_end(va);
	if (CheckRepeatMessage(temp)) {
		return;
	}
	AddMessage(temp);
	MustRedraw |= RedrawMessage;
	// FIXME: for performance the minimal area covered by msg's should be used
	MarkDrawEntireMap();
}

/**
**		Set message to display.
**
**		@param x		Message X map origin.
**		@param y		Message Y map origin.
**		@param fmt		To be displayed in text overlay.
**
**		@note FIXME: vladi: I know this can be just separated func w/o msg but
**				it is handy to stick all in one call, someone?
*/
global void SetMessageEvent(int x, int y, const char* fmt, ...)
{
	char temp[128];
	va_list va;

	va_start(va, fmt);
	vsprintf(temp, fmt, va);
	va_end(va);
	if (CheckRepeatMessage(temp) == 0) {
		AddMessage(temp);
	}

	if (MessagesEventCount == MESSAGES_MAX) {
		ShiftMessagesEvent();
	}

	strcpy(MessagesEvent[MessagesEventCount], temp);
	MessagesEventX[MessagesEventCount] = x;
	MessagesEventY[MessagesEventCount] = y;
	MessagesEventIndex = MessagesEventCount;
	++MessagesEventCount;

	MustRedraw |= RedrawMessage;
	//FIXME: for performance the minimal area covered by msg's should be used
	MarkDrawEntireMap();
}

/**
**		Goto message origin.
*/
global void CenterOnMessage(void)
{
	if (MessagesEventIndex >= MessagesEventCount) {
		MessagesEventIndex = 0;
	}
	if (MessagesEventIndex >= MessagesEventCount) {
		return;
	}
	ViewportCenterViewpoint(TheUI.SelectedViewport,
		MessagesEventX[MessagesEventIndex], MessagesEventY[MessagesEventIndex]);
	SetMessage("~<Event: %s~>", MessagesEvent[MessagesEventIndex]);
	++MessagesEventIndex;
}

/**
**		Cleanup messages.
*/
global void CleanMessages(void)
{
	MessagesCount = 0;
	MessagesSameCount = 0;
	MessagesEventCount = 0;
	MessagesEventIndex = 0;
	MessagesScrollY = 0;
}

/*----------------------------------------------------------------------------
--		STATUS LINE
----------------------------------------------------------------------------*/

#define STATUS_LINE_LEN 256
local char StatusLine[STATUS_LINE_LEN];						/// status line/hints

/**
**		Draw status line.
*/
global void DrawStatusLine(void)
{
	if (TheUI.StatusLine.Graphic) {
		VideoDrawSubClip(TheUI.StatusLine.Graphic, 0, 0,
			TheUI.StatusLine.Graphic->Width, TheUI.StatusLine.Graphic->Height,
			TheUI.StatusLineX, TheUI.StatusLineY);
	}
	if (StatusLine[0]) {
		PushClipping();
		SetClipping(TheUI.StatusLineTextX, TheUI.StatusLineTextY,
			TheUI.StatusLineX + TheUI.StatusLine.Graphic->Width - 1,
			TheUI.StatusLineY + TheUI.StatusLine.Graphic->Height - 1);
		VideoDrawTextClip(TheUI.StatusLineTextX, TheUI.StatusLineTextY,
			TheUI.StatusLineFont, StatusLine);
		PopClipping();
	}
}

/**
**		Change status line to new text.
**
**		@param status		New status line information.
*/
global void SetStatusLine(char* status)
{
	if (KeyState != KeyStateInput && strcmp(StatusLine, status)) {
		MustRedraw |= RedrawStatusLine;
		strncpy(StatusLine, status, STATUS_LINE_LEN - 1);
		StatusLine[STATUS_LINE_LEN - 1] = '\0';
	}
}

/**
**		Clear status line.
*/
global void ClearStatusLine(void)
{
	if (KeyState != KeyStateInput) {
		SetStatusLine("");
	}
}

/*----------------------------------------------------------------------------
--		COSTS
----------------------------------------------------------------------------*/

local int CostsFood;						/// mana cost to display in status line
local int CostsMana;						/// mana cost to display in status line
local int Costs[MaxCosts];				/// costs to display in status line

/**
**		Draw costs in status line.
*/
global void DrawCosts(void)
{
	int i;
	int x;

	x = TheUI.StatusLineX + 270;
	if (CostsMana) {
		// FIXME: hardcoded image!!!
		VideoDrawSubClip(TheUI.Resources[GoldCost].Icon.Graphic,
			/* 0, TheUI.Resources[GoldCost].IconRow *
				TheUI.Resources[GoldCost].IconH */
			0, 3 * TheUI.Resources[GoldCost].IconH,
			TheUI.Resources[GoldCost].IconW, TheUI.Resources[GoldCost].IconH,
			x, TheUI.StatusLineY + 1);

		VideoDrawNumber(x + 15, TheUI.StatusLineY + 2, GameFont, CostsMana);
		x += 45;
	}

	for (i = 1; i < MaxCosts; ++i) {
		if (Costs[i]) {
			if (TheUI.Resources[i].Icon.Graphic) {
				VideoDrawSubClip(TheUI.Resources[i].Icon.Graphic, 0,
					TheUI.Resources[i].IconRow * TheUI.Resources[i].IconH,
					TheUI.Resources[i].IconW, TheUI.Resources[i].IconH,
					x, TheUI.StatusLineY + 1);
			}
			VideoDrawNumber(x + 15, TheUI.StatusLineY + 2, GameFont,Costs[i]);
			x += 45;
			if (x > VideoWidth - 45) {
				break;
			}
		}
	}

	if (CostsFood) {
		// FIXME: hardcoded image!!!
		VideoDrawSubClip(TheUI.Resources[FoodCost].Icon.Graphic, 0,
			TheUI.Resources[FoodCost].IconRow * TheUI.Resources[FoodCost].IconH,
			TheUI.Resources[FoodCost].IconW, TheUI.Resources[FoodCost].IconH,
			x, TheUI.StatusLineY + 1);
		VideoDrawNumber(x + 15, TheUI.StatusLineY + 2, GameFont, CostsFood);
		x += 45;
	}
}

/**
**		Set costs in status line.
**
**		@param mana		Mana costs.
**		@param food		Food costs.
**		@param costs		Resource costs, NULL pointer if all are zero.
*/
global void SetCosts(int mana, int food, const int* costs)
{
	int i;

	if (CostsMana != mana) {
		CostsMana = mana;
		MustRedraw |= RedrawCosts;
	}

	if (CostsFood != food) {
		CostsFood = food;
		MustRedraw |= RedrawCosts;
	}

	if (costs) {
		for (i = 0; i < MaxCosts; ++i) {
			if (Costs[i] != costs[i]) {
				Costs[i] = costs[i];
				MustRedraw |= RedrawCosts;
			}
		}
	} else {
		for (i = 0; i < MaxCosts; ++i) {
			if (Costs[i]) {
				Costs[i] = 0;
				MustRedraw |= RedrawCosts;
			}
		}
	}
}

/**
**		Clear costs in status line.
*/
global void ClearCosts(void)
{
	SetCosts(0, 0, NULL);
}

/*----------------------------------------------------------------------------
--		INFO PANEL
----------------------------------------------------------------------------*/

/**
**		Draw info panel background.
**
**		@param frame		frame nr. of the info panel background.
*/
local void DrawInfoPanelBackground(unsigned frame)
{
	if (TheUI.InfoPanel.Graphic) {
		VideoDrawSubClip(TheUI.InfoPanel.Graphic, 0,
			TheUI.InfoPanelH * frame,
			TheUI.InfoPanelW, TheUI.InfoPanelH,
			TheUI.InfoPanelX, TheUI.InfoPanelY);
	}
}

/**
**		Draw info panel.
**
**		Panel:
**				neutral				- neutral or opponent
**				normal				- not 1,3,4
**				magic unit		- magic units
**				construction		- under construction
*/
global void DrawInfoPanel(void)
{
	int i;

	if (NumSelected) {
		if (NumSelected > 1) {
			//
			//  If there are more units selected draw their pictures and a health bar
			//
			PlayerPixels(ThisPlayer);		// can only be own!
			DrawInfoPanelBackground(0);
			for (i = 0; i < (NumSelected > TheUI.NumSelectedButtons ?
					TheUI.NumSelectedButtons : NumSelected); ++i) {
				DrawUnitIcon(ThisPlayer,
					Selected[i]->Type->Icon.Icon,
					(ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == i) ?
						(IconActive | (MouseButtons & LeftButton)) : 0,
					TheUI.SelectedButtons[i].X, TheUI.SelectedButtons[i].Y);
				UiDrawLifeBar(Selected[i],
					TheUI.SelectedButtons[i].X, TheUI.SelectedButtons[i].Y);

				if (ButtonAreaUnderCursor == ButtonAreaSelected &&
						ButtonUnderCursor == i) {
					SetStatusLine(Selected[i]->Type->Name);
				}
			}
			if (NumSelected > TheUI.NumSelectedButtons) {
				char buf[5];
				sprintf(buf, "+%d", NumSelected - TheUI.NumSelectedButtons);
				VideoDrawText(TheUI.MaxSelectedTextX, TheUI.MaxSelectedTextY,
					TheUI.MaxSelectedFont, buf);
			}
			return;
		} else {
			// FIXME: not correct for enemies units
			if (Selected[0]->Player == ThisPlayer) {
				if (Selected[0]->Type->Building &&
						(Selected[0]->Orders[0].Action == UnitActionBuilded ||
							Selected[0]->Orders[0].Action == UnitActionResearch ||
							Selected[0]->Orders[0].Action == UnitActionUpgradeTo ||
							Selected[0]->Orders[0].Action == UnitActionTrain)) {
					i = 3;
				} else if (Selected[0]->Type->_MaxMana) {
					i = 2;
				} else {
					i = 1;
				}
			} else {
				i = 0;
			}
			DrawInfoPanelBackground(i);
			DrawUnitInfo(Selected[0]);
			if (ButtonAreaUnderCursor == ButtonAreaSelected && ButtonUnderCursor == 0) {
				SetStatusLine(Selected[0]->Type->Name);
			}
			return;
		}
	}

	//		Nothing selected

	DrawInfoPanelBackground(0);
	if (UnitUnderCursor && UnitVisibleOnMap(UnitUnderCursor)  &&
		(UnitUnderCursor->Type->Selectable || !GameRunning)) {
			// FIXME: not correct for enemies units
			DrawUnitInfo(UnitUnderCursor);
	} else {
		int x;
		int y;
		char* nc;
		char* rc;
		// FIXME: need some cool ideas for this.

		x = TheUI.InfoPanelX + 16;
		y = TheUI.InfoPanelY + 8;

		VideoDrawText(x, y, GameFont, "Stratagus");
		y += 16;
		VideoDrawText(x, y, GameFont, "Cycle:");
		VideoDrawNumber(x + 48, y, GameFont, GameCycle);
		VideoDrawNumber(x + 110, y, GameFont,
			CYCLES_PER_SECOND * VideoSyncSpeed / 100);
		y += 20;

		GetDefaultTextColors(&nc, &rc);
		for (i = 0; i < PlayerMax - 1; ++i) {
			if (Players[i].Type != PlayerNobody) {
				if (ThisPlayer->Allied & (1 << Players[i].Player)) {
					SetDefaultTextColors(FontGreen, rc);
				} else if (ThisPlayer->Enemy & (1 << Players[i].Player)) {
					SetDefaultTextColors(FontRed, rc);
				} else {
					SetDefaultTextColors(nc, rc);
				}

				VideoDrawNumber(x + 15, y, GameFont, i);

				VideoDrawRectangle(ColorWhite,x, y, 12, 12);
				VideoFillRectangle(Players[i].Color, x + 1, y + 1, 10, 10);

				VideoDrawText(x + 27, y, GameFont,Players[i].Name);
				VideoDrawNumber(x + 117, y, GameFont,Players[i].Score);
				y += 14;
			}
		}
		SetDefaultTextColors(nc, rc);
	}
}

/*----------------------------------------------------------------------------
--		TIMER
----------------------------------------------------------------------------*/

/**
**		Draw the timer
*/
global void DrawTimer(void)
{
	char buf[30];
	int hour;
	int min;
	int sec;

	if (!GameTimer.Init) {
		return;
	}

	sec = GameTimer.Cycles / CYCLES_PER_SECOND % 60;
	min = (GameTimer.Cycles / CYCLES_PER_SECOND / 60) % 60;
	hour = (GameTimer.Cycles / CYCLES_PER_SECOND / 3600);

	if (hour) {
		sprintf(buf, "%d:%02d:%02d", hour, min, sec);
	} else {
		sprintf(buf, "%d:%02d", min, sec);
	}

	// FIXME: make this configurable
	VideoDrawText(TheUI.SelectedViewport->EndX - 70,
		TheUI.SelectedViewport->Y + 15, GameFont, buf);
}

/**
**		Update the timer
*/
global void UpdateTimer(void)
{
	if (GameTimer.Running) {
		if (GameTimer.Increasing) {
			GameTimer.Cycles += GameCycle - GameTimer.LastUpdate;
		} else {
			GameTimer.Cycles -= GameCycle - GameTimer.LastUpdate;
			if (GameTimer.Cycles < 0) {
				GameTimer.Cycles = 0;
			}
		}
		GameTimer.LastUpdate = GameCycle;
		// FIXME: only redraw when the displayed time changes
		MustRedraw |= RedrawTimer;
	}
}

//@}
