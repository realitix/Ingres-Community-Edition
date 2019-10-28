# include	<compat.h>
# include	<gl.h>
# include       <iicommon.h>
# include	<clconfig.h>
# include	<meprivate.h>
# include	<me.h>

/*
 *	Copyright (c) 1983 Actian Corporation
 *
 *	Name:
 *		ME.c
 *
 *	Function:
 *		None
 *
 *	Arguments:
 *		None
 *
 *	Result:
 *		initialize ME global variables
 *
 *	Side Effects:
 *		None
 *
 *	History:
 *		03/83 - (gb) written.
 *
 *		16-Dec-1986 -- (daveb) rworked for layering on malloc.
**	23-may-90 (blaise)
**	    Add <clconfig.h> include to pickup correct ifdefs in <meprivate.h>.
**	08-feb-93 (pearl)
**	    Add #include of me.h.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      19-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
*/

ME_HEAD		MElist;
ME_HEAD		MEfreelist;
i2		ME_pid	     = 0;
bool		MEsetup      = FALSE;
STATUS		MEstatus     = OK;
bool		MEgotadvice  = FALSE;
i4 		MEadvice     = ME_USER_ALLOC;
