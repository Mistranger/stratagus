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
/**@name ccl_editor.c	-	Editor CCL functions. */
//
//	(c) Copyright 2002 by Lutz Sammer
//
//	FreeCraft is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published
//	by the Free Software Foundation; only version 2 of the License.
//
//	FreeCraft is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	$Id$

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include "freecraft.h"
#include "editor.h"
#include "ccl.h"

/*----------------------------------------------------------------------------
--	Defines
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

/**
**	Define an editor unit-type list.
**
**	@param list	List of all names.
*/
local SCM CclDefineEditorUnitTypes(SCM list)
{
    int i;
    char** cp;

    if( (cp=EditorUnitTypes) ) {		// Free all old names
	while( *cp ) {
	    free(*cp++);
	}
	free(EditorUnitTypes);
    }

    //
    //	Get new table.
    //
    i=gh_length(list);
    EditorUnitTypes=cp=malloc((i+1)*sizeof(char*));
    while( i-- ) {
	*cp++=gh_scm2newstr(gh_car(list),NULL);
	list=gh_cdr(list);
    }
    *cp=NULL;

    return SCM_UNSPECIFIED;
}


/**
**	Register CCL features for the editor.
*/
global void EditorCclRegister(void)
{
    //gh_new_procedureN("player",CclPlayer);
    //gh_new_procedure0_0("get-this-player",CclGetThisPlayer);
    //gh_new_procedure1_0("set-this-player!",CclSetThisPlayer);
    gh_new_procedureN("define-editor-unittypes",CclDefineEditorUnitTypes);
}

//@}
