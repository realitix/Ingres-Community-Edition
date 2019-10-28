/*
**	Copyright (c) 1983, 2009 Actian Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<ex.h>
#include	<si.h>
#ifndef WCE_PPC
#include	<errno.h>
#include	<direct.h>
#endif

/**
** Name:	logt.c -	Get Location Routine.
**/

bool	iiloChanged = TRUE;

/*{
** Name:	LOgt() -	Get Location.
**
** Description:
**	Get the current directory of the process and return it as a
**	location using the input buffer and location reference.
**
** Input:
**	buf	{char *}  Buffer of at least MAX_LOC bytes used
**				for the location.
**
** Output:
**	loc	{LOCATION *}  The address of the location to contain the path
**				of the current directory.
**
** History:
**	19-may-95 (emmag)
**	    Branched for NT.
**	06-may-1996 (canor01)
**	    Clean up compiler warnings (NULL != EOS).
**      27-may-97 (mcgem01)
**          Clean up compiler warnings.
**  17-aug-2009 (zhayu01) SIR 121210
**      Adopted it for Pocket PC 2003/Windows CE.
*/

STATUS
LOgt ( buf, loc )
char		*buf;
LOCATION	*loc;
{
    ULONG       length = MAX_LOC - 3;
    char        *string;
    string = buf;

    if (_getdcwd(0, buf, MAX_LOC) == EOS)
    {
#ifdef WCE_PPC
         return((STATUS)wceerrno);
#else
         return((STATUS)errno);
#endif /* WCE_PPC */
    }

    return LOfroms( PATH, buf, loc );
}
