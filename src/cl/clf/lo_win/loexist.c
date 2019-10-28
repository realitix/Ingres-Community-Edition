/*
** Copyright (c) 1985, 2009 Actian Corporation.
*/

# include	<compat.h>
# include	<gl.h>
# include	<clconfig.h>
# include 	<systypes.h>
# include	<lo.h>
# ifndef WCE_PPC
# include	<io.h>
# endif
# include	"lolocal.h"

/*
** LOexist -- Does location exist?
**
** History:		
**	19-may-95 (emmag)
**	    Branched for NT.
**      27-may-97 (mcgem01)
**          Clean up compiler warnings.
**	20-may-2001 (somsa01 for jenjo02)
**	    Return appropriate error if stat() fails rather than just
**	    "FAIL" so that LO users can distinguish between "does not exist"
**	    and problems with permissions.
**  17-aug-2009 (zhayu01) SIR 121210
**      Adopted it for Pocket PC 2003/Windows CE.
*/

STATUS
LOexist (loc)
register LOCATION	*loc;	/* location of file or directory */
{
    if (access(loc->string, 00))
#ifdef WCE_PPC
	return (LOerrno((i4)wceerrno));
#else
	return (LOerrno((i4)errno));
#endif

    return(OK);
}
