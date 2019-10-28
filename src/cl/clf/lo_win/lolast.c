/*
** Copyright (c) 1983, 2009 Actian Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include        <clconfig.h>
#include	"lolocal.h"
#include	<lo.h>
#include	<tm.h>		/* to get SYSTIME declaration */
#include	<systypes.h>
#ifndef WCE_PPC
#include	<errno.h>
#include	<sys/stat.h>
#endif

/*LOlast
**
**	Location last modified.
**
**	Get last modification date of a LOCATION.
**
**		History
**			03/09/83 -- (mmm)
**				written
**			02/23/89 -- (daveb, seng)
**				rename LO.h to lolocal.h
**			28-Feb-1989 (fredv)
**				Include <systypes.h>
**	23-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Add "include <clconfig.h>".
**	31-jul-2001 (somsa01)
**	    Use _stati64() if LARGEFILE64 is defined.
**  17-aug-2009 (zhayu01) SIR 121210
**      Adopted it for Pocket PC / Windows CE.
*/

/* static char	*Sccsid = "@(#)LOlast.c	3.1  9/30/83"; */


STATUS
LOlast(loc, last_mod)
register LOCATION	*loc;
register SYSTIME	*last_mod;

{
	STATUS		LOerrno();
#ifdef LARGEFILE64
	struct _stati64	statinfo;	/* the layout of the struct stat as defined in <stat.h> */
#else
#ifdef WCE_PPC
	HANDLE hFile;
	FILETIME laTime;
	SYSTEMTIME systemTime;
#else
	struct stat	statinfo;	/* the layout of the struct stat as defined in <stat.h> */
#endif  /* WCE_PPC */
#endif  /* LARGEFILE64 */
	register i2	os_exitstatus;
	STATUS		status;
	extern int	errno;		/* system supplied variable of type int */


	/* statinfo is a buffer into which information is placed 
	** by stat concerning the file.  
	*/
#ifdef LARGEFILE64
	os_exitstatus = _stati64(loc->string, &statinfo);
#else
#ifdef WCE_PPC
    os_exitstatus = 0;
    if ( (hFile = CreateFile ( loc->string,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL)) == INVALID_HANDLE_VALUE )
		os_exitstatus = 1;
#else
	os_exitstatus = stat(loc->string, &statinfo);
#endif  /* WCE_PPC */
#endif  /* LARGEFILE64 */

	if (os_exitstatus != 0)
	{
		/* error occurred on stat call */
		last_mod->TM_secs = -1;
		last_mod->TM_msecs = -1;

#ifdef WCE_PPC
		status = FAIL;
#else
		status = LOerrno((i4)errno);
#endif /* WCE_PPC */

	}
	else
	{
		/* return time_return location last modified */
#ifdef WCE_PPC
		if (GetFileTime(hFile, NULL, &laTime, NULL))
		{
		     if (FileTimeToSystemTime(&laTime, &systemTime))
		        last_mod->TM_secs = systemTime.wYear;
		}
#else
		last_mod->TM_secs = statinfo.st_mtime;
#endif /* WCE_PPC */
		last_mod->TM_msecs = 0;
		status = OK;

	}

#ifdef WCE_PPC
	CloseHandle(hFile);
#endif

	return(status);
}
