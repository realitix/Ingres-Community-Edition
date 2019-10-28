/*
**Copyright (c) 2004 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<si.h>
# include	<lo.h>
# include	<me.h>
# include	"silocal.h"

# define SICOPY_BUF_SIZE 16384 /* 16 KB Buffer */

/* SIcopy
**
**	Copy a (possibly binary) file
**
**	Copy file with location pointed to by "in" to 
**      location pointed to by "out."
**
** History
**
**    18-Jun-1997 (easda01)
**        Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/   

STATUS
SIcopy(in, out)
    LOCATION        *in;
    LOCATION        *out;
{
    FILE            *in_fp;
    FILE            *out_fp;
    PTR             buffer;
    i4         bytes_read;
    i4         bytes_written;
    STATUS          status;
	
    if ( (out == NULL) || (in == NULL) )
    {
        status = SI_CANT_OPEN;
    }
    else
    {
        status = SIfopen(in,"r",SI_BIN,1,&in_fp);
        if (status == OK)
        {
	    status = SIfopen(out,"w",SI_BIN,1,&out_fp);
	    if (status == OK)
	    {
	        buffer = (PTR)MEreqmem(0,SICOPY_BUF_SIZE,
				       TRUE,&status);
		if (status == OK)
		{
		    while ( (status = SIread(in_fp,SICOPY_BUF_SIZE,
					     &bytes_read,(PTR)buffer)) == OK )
		    {
		        bytes_written = 0;
		        status = SIwrite(bytes_read,(PTR)buffer,
		                         &bytes_written,out_fp);
			if (status != OK)
			{
			    break;
			}
		    }
		    if (status == ENDFILE)
		    {
			status = OK;
			if (bytes_read > 0)
			{
			    status = SIwrite(bytes_read,(PTR)buffer,
		    		             &bytes_written,out_fp);
			}
		    }
		    MEfree(buffer);
		}
		SIclose(out_fp);
	    }
	    SIclose(in_fp);
	}
    }
    return (status);
}
