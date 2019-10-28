/*
**Copyright (c) 2004, 2009 Actian Corporation
*/

# include	<compat.h>		/* header file for compatibility lib */
# include	<systypes.h>
#ifndef WCE_PPC
# include	<errno.h>		/* header file for error checking */
#endif
# include	<fp.h>

# include	"fplocal.h"

/**
** Name: FPATAN.C - Find the arctangent of x.  
**
** Description:
** 	Find the arctangent of x.  Returned result is in radians.
**
**      This file contains the following external routines:
**    
**	FPatan()	   -  Find the arctangent of x.
**
** History:
**	16-apr-1992 (rog)
**	    Created from mhatan.c.
**	13-aug-2009 (zhayu01) SIR 121210
**		Do not include <errno.h> for Pocket PC and Windows CE
**		because it does not exist there.
**/


/*{
** Name: FPatan() - Arctangent.
**
** Description:
**	Find the arctangent of x.
**	Returned result is in radians.
**
** Inputs:
**      x                               Number to find the arctangent of.
**
** Outputs:
**      result				Point to the result
**
** Returns:
**	OK				If operation succeeded
**
**	FP_NOT_FINITE_ARGUMENT		When x is not a finite number
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      16-apr-1992 (rog)
**          Created from MHatan();
*/
STATUS
FPatan(double *x, double *result)
{
	STATUS	status;

	if (!FPdfinite(x))
	{
	    status = FP_NOT_FINITE_ARGUMENT;
	}
	else
	{
	    *result = atan(*x);
	    status = OK;
	}

	return(status);
}
