/*
** Copyright (c) 2011 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BTintersects - are any bits set in 'p1' also set in 'p2'
**
** Description:
**		Return true if any bits set in 'p1' are also set in 'p2'.
**
** Inputs:
**	size		number of bits to check
**	p1&p2		bitvectors to check for intersection
**
** Outputs:
**	Returns:
**		Return TRUE if p1's set bits intersect with p2's.
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	16-Mar-2011 (kiria01)
**	   Created to avoid the inevitable costly equivalent:
**		r = p2
**		BTand(p1,r,sz)
**		if (BTnext(-1,&r,sz) > 0)
*/

bool
BTintersects (
	const void *p1,
	const void *p2,
	i4 size)
{
    register const char *a = p1;
    register const char *b = p2;
    register i4 n = BT_NUMBYTES_MACRO(size);

    do {
	if((*a & *b) != 0)
	    break;
	a++;
	b++;
    } while (--n);
    return n != 0;
}
