/*
** Copyright (c) 2004, 2011 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BTtest - is the specified bit in a byte stream set.
**
** Description:
**		test bit 'pos' in 'vector'
**
** Inputs:
**	pos		bit to test in 'vector'
**	vector		byte stream to test bit of
**
** Outputs:
**	Returns:
**		TRUE if bit 'pos' is set in 'vector'
**	Exceptions:
**		None
**
** Side Effects:
**		None
**
** History:
**	29-dec-85 (daveb)
**		Rewrote to use tables, cleaned up conditional compilation.
**	16-jan-86 (greg)
**		Use BT_XXX_MACROs.  Avoid `?' operator.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
**	16-Mar-2011 (kiria01)
**	    Update BT prototypes for void* args and returns.
*/

bool
BTtest(
	i4		pos,
	const void	*vector)
{
	register i4	p = pos;
	register i4	s = BT_MOD_MACRO(p);
	register const u_char *v = vector;
	return (v[ BT_FULLBYTES_MACRO(p) ] >> s) & 01;
}

