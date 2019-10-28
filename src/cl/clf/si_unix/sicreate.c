/*
**Copyright (c) 2004 Actian Corporation
*/
# include <compat.h>
# include <gl.h>
# include <si.h>
# include <lo.h>

# include <fcntl.h>
# include <sys/stat.h>

# include "silocal.h"
/*
** Name: SIcreate
**
** Description:
**      Creates the file whose name is specified by lo. If the file already
**      exists, the function returns SI_CANT_OPEN.
**
** Inputs:
**      lo              ptr to a location containing a path and filename
**
** Outputs:
**      none
**
** Returns:
**      OK if the file was created
**
** History:
**      12-dec-1996 (wilan06)
**          Created
**      03-jun-1999 (hanch04)
**          Added LARGEFILE64 for files > 2gig
**      05-may-2010 (coomi01) b123677
**          Distinguish failure to open due to O_EXCL from other errors 
*/
STATUS
SIcreate (LOCATION* lo)
{
    STATUS      status = OK;
    int         handle;
    char*       pathName = NULL;

    LOtos (lo, &pathName);

#ifdef LARGEFILE64
    handle = open64 (pathName, O_CREAT | O_EXCL, S_IREAD | S_IWRITE);
#else
    handle = open (pathName, O_CREAT | O_EXCL, S_IREAD | S_IWRITE);
#endif /* LARGEFILE64 */


    if (handle != -1)
    {
        close (handle);
    }
    else if (errno != EEXIST)
    {
        status = SI_CANT_OPEN_EACCES;
    }
    else
    {
        status = SI_CANT_OPEN_EEXIST;
    }
    return status;
}
