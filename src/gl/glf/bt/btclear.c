/*
** Copyright (c) 2004, 2011 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BTclear	- clear a bit in a byte stream
**
** Description:
**		clear bit 'pos' in 'vector'
**
** Inputs:
**	pos		bit to clear in 'vector' (LSB is 0)
**	vector		byte stream to clear bit in
**
** Outputs:
**	vector		result clearing bit 'pos' in 'vector'
**
**	Returns:
**		None
**
** History:
**	29-dec-85 (daveb)
**		Rewrote to use tables, cleaned up conditional compilation.
**	15-jan-86 (greg)
**		Use BT_XXX_MACROs.  Conform to coding standards.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      4-oct-92 (ed)
**          add prototypes, and move bt.h to GLHDR
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
**	16-Mar-2011 (kiria01)
**	    Update BT prototypes for void* args and returns.
*/

/*ARGSUSED*/

void *
BTclear(
	i4	pos,
	void	*vector)
{
	register char *v = vector;
	v[ BT_FULLBYTES_MACRO(pos) ] &= bt_clrmsk[ BT_MOD_MACRO(pos) ];
	return vector;
}
