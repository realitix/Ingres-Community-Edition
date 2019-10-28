/*
**	Copyright (c) 1983, 2009 Actian Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#ifndef WCE_PPC
#include	<errno.h>
#endif

/*{
** Name:	LOchange() -	Change location.
**
**	Change the LOCATION of the current process.  
**
**	success:  LOchange changes the location of the current process to loc 
**		  and returns OK.
**	failure:  LOchange does not change location of current process.
**		  LOchange returns  FAIL
**
** Side Effects:
**
** History:
**	19-may-95 (emmag)
**	    Written for NT.
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**  17-aug-2009 (zhayu01) SIR 121210
**      Adopted it for Pocket PC 2003/Windows CE.
*/

STATUS
LOchange( loc )
register LOCATION	*loc;
{
        char            *string;

        string = loc->path;

        if ( loc->fname != NULL && *loc->fname != EOS )
                return LO_NO_SUCH;      /* can't be file name */

        if ( loc->path == NULL )
                return LO_NO_SUCH;      /* must be path */

#ifndef WCE_PPC
        if (!SetCurrentDirectory(loc->string))
                return(FAIL);
#endif

        return OK;
}
