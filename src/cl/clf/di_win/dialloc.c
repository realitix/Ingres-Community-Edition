/******************************************************************************
**
** Copyright (c) 1987, 1998 Actian Corporation
**
******************************************************************************/

#include   <compat.h>
#include   <iicommon.h>
#include   <cs.h>
#include   <di.h>
#include   "dilru.h"

extern STATUS DI_sense(DI_IO *, i4 *, CL_SYS_ERR *);


/******************************************************************************
**
** Name: DIalloc - Allocates a page to a direct access file.
**
** Description:
**      The DIalloc routine is used to add pages to a direct
**      access file.  This routine can add more than one page
**      at a time by accepting a count of the number of pages to add.
**
**      The end of file and allocated are not updated on disk until a DIflush
**      call is issued.  This insures that pages are not considered valid
**      until after they are formatted.  The allocation can be ignored if
**      the file is closed or the system crashes before the DIflush.
**
** Inputs:
**      f                Pointer to the DI file
**                       context needed to do I/O.
**      n                The number of pages to allocate.
**
** Outputs:
**      page             Pointer to variable used to
**                       return the page number of the
**                       first page allocated.
**      err_code         Pointer to a variable used
**                       to return operating system
**                       errors.
**    Returns:
**        OK
**        DI_BADEXTEND      Can't allocate disk space
**        DI_BADFILE        Bad file context.
**        DI_EXCEED_LIMIT   Too many open files.
**    Exceptions:
**        none
**
** Side Effects:
**        none
** History:
**	09-feb-1996 (canor01)
**	    Get exclusive semaphore on DI_IO before updating it in DI_sense
**	08-dec-1997 (canor01)
**	    Implement LRU for open files (initial copy from Unix).
**	28-jan-1998 (canor01)
**	    Optimize LRU--only call DIlru_open if file has been closed.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	13-Nov-2009 (kschendel) SIR 122757
**	    Make io-sem a SYNCH.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**
******************************************************************************/
STATUS
DIalloc(DI_IO      *f,
        i4         n,
        i4         *page,
        CL_SYS_ERR *err_code)
{
    STATUS status = OK;

    CLEAR_ERR(err_code);

    /*
     * Check file control block pointer, return if bad.
     */

    if (f->io_type != DI_IO_ASCII_ID)
    	return (DI_BADFILE);

    CS_synch_lock( &f->io_sem );

    /* get file descriptor for this file */
    do
    {
	if ( f->io_nt_fh == INVALID_HANDLE_VALUE )
        status = DIlru_open( f, FALSE, err_code);
        if ( status != OK )
            break;
 
        status = DI_sense( f, page, err_code );
 
	if ( status != OK )
	    break;

	*page = (i4) (f->io_system_eof + 1);
	f->io_system_eof += n;
 
    } while (FALSE);
 
    CS_synch_unlock( &f->io_sem );
 
    return( status );

}
