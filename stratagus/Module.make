##       _________ __                 __                               
##      /   _____//  |_____________ _/  |______     ____  __ __  ______
##      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
##      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ \ 
##     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
##             \/                  \/          \//_____/            \/ 
##  ______________________                           ______________________
##			  T H E   W A R   B E G I N S
##	   Stratagus - A free fantasy real time strategy game engine
##
##	Module.make	-	Module Makefile (included from Makefile).
##
##	(c) Copyright 2002 by Nehal Mistry
##
##	Stratagus is free software; you can redistribute it and/or modify
##	it under the terms of the GNU General Public License as published
##	by the Free Software Foundation; only version 2 of the License.
##
##	Stratagus is distributed in the hope that it will be useful,
##	but WITHOUT ANY WARRANTY; without even the implied warranty of
##	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##	GNU General Public License for more details.
##

MODULE= src/clone
MSRC=	ccl.c ccl_player.c clone.c construct.c groups.c iolib.c mainloop.c \
	mpq.c player.c pud.c scm.c selection.c spells.c unit.c unit_draw.c \
	unit_find.c unitcache.c ccl_spell.c ccl_helpers.c

SRC+=	$(addprefix $(MODULE)/,$(MSRC))
HDRS+=
