# include 	<compat.h>
# include 	<gl.h>
# include 	<lo.h>
# include 	<st.h>


/*
 *Copyright (c) 2004 Actian Corporation
 *
 *	Name: LOcopy
 *
 *	Function: LOcopy copies one location into another.
 *
 *	Arguments: 
 *		oldloc -- the location which is to be copied.
 *		newlocbuf -- buffer to be associated with new location.
 *		newloc -- the destination of the copy.
 *
 *	Result:
 *		newloc will contain copy of oldloc.
 *
 *	Side Effects:
 *		none.
 *
 *	History:
 *		4/4/83 -- (mmm) written
 *
 *
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	20-aug-93 (ed)
**	    add missing includes
**	24-Feb-2011 (kiria01)
**	    Don't copy to self.
*/
VOID
LOcopy(oldloc, newlocbuf, newloc)
register LOCATION	*oldloc, *newloc;
char			newlocbuf[];
{
	if (oldloc->string != newlocbuf)
	    STcopy(oldloc->string, newlocbuf);
	LOfroms(oldloc->desc, newlocbuf, newloc);
}
