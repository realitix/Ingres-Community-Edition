/*
** Copyright 1983, 2008 Actian Corporation
*/
# include	<compat.h> 
# include	<efndef.h>
# include	<iledef.h>
# include	<iosbdef.h>
# include	<jpidef.h>
# include	<gl.h>
# include	<id.h> 
# include	<starlet.h>

/*{
** Name:  IDgroup
**
** Description:	
**	Set the passed in argument 'gid' to contain the user id
**	of the user who is executing this process.
** Inputs:
**	gid		group identificaton
**
** Outputs:
**	gid		group identification
**
** History:
**		10/5/83 (dd) --	Made for VMS compat lib
**		17-jan-85 (fhc) -- sys$waitfr sys$getjpi for vms 4.0 support
**      16-jul-93 (ed)
**	    added gl.h
**      17-oct-97 (rosga02)
**        added a null item terminator for getjpi() call item list
**        to avoid potential accvio
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	26-oct-2001 (kinte01)
**	   Clean up compiler warnings
**	29-aug-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	24-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
*/

VOID
IDgroup(gid)
GID	*gid;
{
	ILE3 itmlst[2] =
		{{4, JPI$_GRP, (PTR)gid, 0},{0, 0, 0, 0}};
	IOSB iosb;

	sys$getjpiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);
}
