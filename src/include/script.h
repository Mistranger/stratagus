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
/**@name ccl.h		-	The clone configuration language headerfile. */
//
//	(c) Copyright 1998-2002 by Lutz Sammer
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

#ifndef __CCL_H__
#define __CCL_H__

//@{

/*----------------------------------------------------------------------------
--	Includes
----------------------------------------------------------------------------*/

#include <string.h>
#include "siod.h"

/*----------------------------------------------------------------------------
--	Macros
----------------------------------------------------------------------------*/

//
//	Macros for compatibility with guile high level interface.
//

#define SCM LISP
#define SCM_UNSPECIFIED NIL
#define gh_null_p(lisp) NULLP(lisp)

#define gh_eq_p(lisp1,lisp2)	EQ(lisp1,lisp2)

#define gh_list_p(lisp)		CONSP(lisp)
#define gh_car(lisp)		car(lisp)
#define gh_cdr(lisp)		cdr(lisp)
#define gh_caar(lisp)		caar(lisp)
#define gh_cadr(lisp)		cadr(lisp)
#define gh_cddr(lisp)		cddr(lisp)
#define gh_length(lisp)		nlength(lisp)

#define gh_exact_p(lisp)	TYPEP(lisp,tc_flonum)
#define gh_scm2int(lisp)	(long)FLONM(lisp)
#define gh_int2scm(num)		flocons(num)

#define gh_string_p(lisp)	TYPEP(lisp,tc_string)
#define	gh_scm2newstr(lisp,str) strdup(get_c_string(lisp))
#define	gh_str02scm(str) strcons(strlen(str),str)

#define	gh_vector_p(lisp)	\
	(TYPE(lisp)>=tc_string && TYPE(lisp)<=tc_byte_array)
#define	gh_vector_length(lisp)	nlength(lisp)
#define	gh_vector_ref(lisp,n)	aref1(lisp,n)

#define gh_boolean_p(lisp)	(EQ(lisp,sym_t) || NULLP(lisp))
#define gh_scm2bool(lisp)	(NNULLP(lisp))
#define gh_bool2scm(n)		((n) ? SCM_BOOL_T : SCM_BOOL_F)

#define gh_symbol_p(lisp)	SYMBOLP(lisp)
#define gh_symbol2scm(str)	cintern(str)

#define gh_define(str,val)	setvar(rintern((str)),(val),NIL)

#define gh_display(lisp)	lprin1f(lisp,stdout)
#define gh_newline()		fprintf(stdout,"\n")

#define gh_eval_file(str)	vload(str,0,0)

#define gh_apply(proc,args)	lapply(proc,args)
#define gh_eval(proc,env)	leval(proc,env)

#define gh_new_procedure0_0	init_subr_0
#define gh_new_procedure1_0	init_subr_1
#define gh_new_procedure2_0	init_subr_2
#define gh_new_procedure3_0	init_subr_3
#define gh_new_procedure4_0	init_subr_4
#define gh_new_procedure5_0	init_subr_5
#define gh_new_procedureN	init_lsubr

#define SCM_BOOL_T	sym_t
#define SCM_BOOL_F	NIL

extern LISP sym_t;

//extern SCM CclEachSecond;		/// Scheme function called each second

/*----------------------------------------------------------------------------
--	Variables
----------------------------------------------------------------------------*/

extern char* CclStartFile;		/// CCL start file
extern int CclInConfigFile;		/// True while config file parsing

/*----------------------------------------------------------------------------
--	Functions
----------------------------------------------------------------------------*/

extern void CclGcProtect(SCM obj);	/// Protect scm object for GC
extern void InitCcl(void);		/// Initialise ccl
extern void LoadCcl(void);		/// Load ccl config file
extern void SaveCcl(FILE* file);	/// Save CCL module
extern void SavePreferences(void);	/// Save user preferences
extern void CclCommand(const char*);	/// Execute a ccl command
extern void CclFree(void*);		/// Save free
extern void CleanCclCredits();		/// Free Ccl Credits Memory

//@}

#endif	// !__CCL_H__
