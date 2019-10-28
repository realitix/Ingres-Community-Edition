/*
** Copyright (c) 2004, 2011 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BTset -	set a bit in a byte stream
**
** Description:
**		set bit 'pos' in 'vector'
**
** Inputs:
**	pos		bit to set in 'vector'
**	vector		byte stream to set bit in
**
** Outputs:
**	vector		result of setting bit 'pos' in 'vector'
**
**	Returns:
**		None
**	Exceptions:
**		None
**
** Side Effects:
**		None
**
** History:
**	29-dec-85 (daveb)
**		Rewrote to use tables, cleaned up conditional compilation.
**	15-jan-86 (greg)
**		Use BT_XXX_MACROs.  Conform to coding standards.
**      25-aug-92 (ed)
**          Changed to use prototypes, and include bt.h
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
**	16-Mar-2011 (kiria01)
**	    Update BT prototypes for void* args and returns.
*/

void *
BTset(
	i4	pos,
	void	*vector)
{
	char *v = vector;
	v[ BT_FULLBYTES_MACRO(pos) ] |= bt_bitmsk[ BT_MOD_MACRO(pos) ];
	return vector;
}
