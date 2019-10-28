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

/*
** Name: FPCEIL.C - returns the smallest integer >= the argument passed to it.
**
** Description:
**	returns the smallest integer not less than the
**	argument passed to it.
**
**      This file contains the following external routines:
**    
**	FPceil()	   -  Find the ceil of a number.
**
** History:
**	15-apr-92 (rog)
**          Created from mhceil.c.
**	13-aug-2009 (zhayu01) SIR 121210
**		Do not include <errno.h> for Pocket PC and Windows CE
**		because it does not exist there.
*/


/*{
** Name: FPceil() - Ceiling.
**
** Description:
**	Returns the smallest integer not less than the
**	argument passed to it.  The return value, although
**	an integer, is returned as a double.
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
**          Created from MHceil();
*/
STATUS
FPceil(double *x, double *result)
{
	STATUS	status;

	if (!FPdfinite(x))
	{
	    status = FP_NOT_FINITE_ARGUMENT;
	}
	else
	{
	    *result = ceil(*x);
	    status = OK;
	}

	return(status);
}
