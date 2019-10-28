/*
**Copyright (c) 2004 Actian Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include       <me.h>
# include       <te.h>

/*
**  TEvalid
**	return validity of terminal name
**
**  Arguments
**	ttyname  --  terminal name to check;  this should be the name only,
**			no path should be on the front
**
**  Returns TRUE if the given terminal name is in a valid format,
**	begins with the prefix "tty" (ttyh3, ttyi4, ...), "co" (console)
**	or "cu" (call unix port).
**  FALSE otherwise
**
** History:
**	29-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Include <me.h> before calling MEcmp.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	04-Nov-2010 (miket) SIR 124685
**	    Prototype cleanup.
*/

bool
TEvalid(ttyname)
char	*ttyname;
{
	return(!MEcmp(ttyname, "tty", 3) || !MEcmp(ttyname, "co", 2)
					 || !MEcmp(ttyname, "cu", 2));
}
