#include	  <compat.h>
#include    <gl.h>
#include	  <lo.h>
# include 	  <er.h>  
#include	  "lolocal.h"

/*LOgtfile
**
**	LOgtfile gets fname field from oldloc and stores it in newloc.
**
**		Copyright (c) 1983 Actian Corporation
**		History
**			03/09/83 -- (mmm)
**				written
**			09/13/83 -- VMS CL (dd)
**			10/89 -- Clean up (Mike S)
**      16-jul-93 (ed)
**	    added gl.h
**      18-Jul-2011 (horda03)
**          VMS LOCATION now uses same field names as Unix.
*/

/* static char	*Sccsid = "@(#)LOgtfile.c	1.3  4/21/83"; */

STATUS
LOgtfile(oldloc, newloc)
register LOCATION	*oldloc;
register LOCATION	*newloc;

{
 	return (LOfstfile(oldloc->fname, newloc));
}
