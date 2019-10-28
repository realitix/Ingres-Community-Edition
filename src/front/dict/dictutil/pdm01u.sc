/*
**	Copyright (c) 2004, Actian Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ui.h>
# include	<uigdata.h>

/**
** Name:	main.c -	Main Routine and Entry Point for module upgrade
**
** Description:
**	main		The main program.
**
** History:
**	27-sep-1990 (pete) written.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Updated NEEDLIBS to link dynamic library SHQLIB to replace
**	    its static libraries.
**/


/*{
** Name:	main() -	Main Entry Point for pdm01u upgrade.
**
** Description:
**	Performs upgrade to version 1 of PHYSICAL_DATA_MODELING module.
**	(upgrade from version 1 of METASCHEMA).
**	THIS DOES AN IMMEDIATE EXIT -- does not even connect to database.
**	PDM v1 is not truly an upgrade from Metaschema v1 -- but we have
**	a rule that every upgrade requires an executable.
**
** Side Effects:
**
** History:
**	27-sep-1990 (pete) written.
*/

/*
**	MKMFIN Hints
PROGRAM =	pdm01uin

NEEDLIBS =	DICTUTILLIB SHQLIB COMPATLIB SHEMBEDLIB

UNDEFS =	II_copyright
*/

i4
main(argc, argv)
i4	argc;
char	**argv;
{
	PCexit(OK);
}
