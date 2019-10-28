/*
**	Copyright (c) 2004 Actian Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<st.h>
#include	<me.h>
#include	<cm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<adf.h>
#include	<afe.h>
# include       <er.h>

/**
** Name:	afopid.c -	Find the operator ID.
**
** Description:
**	This file contains and defines:
**
** 	afe_opid()	find the operator id.
**
** History:
**	Revision 6.0  87/02/06  daver
**	Initial revision.
**
**	07/06/93 (dkh) - Code cleanup: use ADF_MAXNAME instead of DB_MAXNAME.
**	07/06/93 (dkh) - Bracket call to adi_opid() with ADF_ERRMSG_OFF and
**			 ADF_ERRMSG_ON to speed up operator id lookups for
**			 report writer and w4gl.  Speedup is due to the
**			 elimination of a file open to get the "no operator"
**			 error message which is never used.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

DB_STATUS	adi_opid();

/*{
** Name:	afe_opid() -	Find the operator ID.
**
** Description:
**	Finds the operator ID given the name of the operator.
**
** Inputs:
**	cb	{ADF_CB *}  A pointer to the current ADF_CB 
**				control block.
**
**	name	{char *}  A EOS terminated string giving the name of
**				the operator.
**
** Outputs:
**	opid	{ADI_OP_ID *}  Set to the op ID of the operator.
**
** Returns:
**	{STATUS}  OK				If successful.
**
**		  E_AF600A_NAME_TOO_LONG	If the name is too long.
**
**		  returns from: 'adi_opid()'
**
** History:
**	Written	2/6/87	(dpr)
**	10/88 (jhw)  Lowercase name for ADF.
**	17-Dec-2010 (kschendel) SIR 124685
**	    Replace ADI_OP_NAME with char array.  Names of max length
**	    (ie no room for null) are invalid.
*/
STATUS
afe_opid ( cb, name, opid )
ADF_CB		*cb;
char		*name;
ADI_OP_ID	*opid;
{
	register u_i4	len;
	char		opname[ADF_MAXNAME];
	STATUS		stat;

	if ( (len = STlength( name )) >= ADF_MAXNAME )
		return afe_error(cb, E_AF600A_NAME_TOO_LONG, 0);

	STcopy(name, opname);
	CVlower(opname);

	ADF_ERRMSG_OFF(cb);

	stat = adi_opid(cb, opname, opid);

	ADF_ERRMSG_ON(cb);

	return(stat);
}
