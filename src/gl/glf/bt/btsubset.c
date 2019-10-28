/*
** Copyright (c) 2004, 2011 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BTsubset - are all bits set in 'sub' also set in 'set'
**
** Description:
**		Return true if bits set in 'subset' are a subset of the bits
**		set in 'set'.
**
** Inputs:
**	size		number of bits to check
**	sub		vector to check against set
**	set		vector to be checked against
**
** Outputs:
**	Returns:
**		Return TRUE if sub's set bits are a subset of set's.
**	Exceptions:
**		None
**
** Side Effects:
**		None
**
** History:
**	01-14-86 (greg)
**		Rewrote to avoid function calls.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**      25-aug-92 (ed)
**          Changed to use prototypes, and include bt.h
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
**	16-Mar-2011 (kiria01)
**	    Update BT prototypes for void* args and returns.
*/

bool
BTsubset (
	const void	*sub,
	const void	*set,
	i4	size)
{
	register const char *v = sub;
	register const char *s = set;
	register i4	n	= BT_NUMBYTES_MACRO(size);


	for ( ; n-- ; s++, v++ )
		if( ( *v & *s ) != *v )
			return(FALSE);

	return(TRUE);
}
