/*
** Copyright (c) 2004 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>
#include    <tm.h>
#include    <st.h>

/**
**
**  Name: TMBREAK.C - Time routines.
**
**  Description:
**      This module contains the following TM cl routines.
**
**	    TMbreak	- break char string for date into pieces
**
**
** History:
 * Revision 1.1  88/08/05  13:46:46  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  88/01/06  17:51:52  anton
**      define ctime
**      
**      Revision 1.2  87/11/10  16:03:13  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**/

/*{
** Name: TMbreak	- break char string for date into pieces
**
** Description:
**      Break the string created by TMstr() into component pieces.  This 
**      routine fills in the following fields of the struct (actually 
**      the pointer to the struct) passed to it: the day-of-the-week,
**      month, day, year, hours, minutes, and seconds.
**
** Inputs:
**      tm                              time to break up
**
** Outputs:
**      human                           components of time
**	Returns:
**	    FAIL - if tm is invalid
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**	06-jul-87 (mmm)
**	    initial jupiter unix cl.
**      22-sep-92 (stevet)
**          Call TMstr to get the local time in 'DDD MMM YY HH MM SS YYYY'
**          format and pick up the new timezone support.
*/
STATUS
TMbreak(tm, human)
SYSTIME            *tm;
struct TMhuman     *human;
{
        char    buf[TM_SIZE_STAMP+1];
	char	*cp=buf;

	TMstr(tm, buf);
	STncpy(human->wday, cp, 3);
	human->wday[3] = '\0';
	cp	= cp + 4;
	STncpy(human->month, cp, 3);
	human->month[3] = '\0';
	cp	= cp + 4;
	STncpy(human->day, cp, 2);
	human->day[2] = '\0';
	cp	= cp + 3;
	STncpy(human->hour, cp, 2);
	human->hour[2] = '\0';
	cp	= cp + 3;
	STncpy(human->mins, cp, 2);
	human->mins[2] = '\0';
	cp	= cp + 3;
	STncpy(human->sec, cp, 2);
	human->sec[2] = '\0';
	cp	= cp + 3;
	STncpy(human->year, cp, 4);
	human->year[4] = '\0';

	return(OK);
}
