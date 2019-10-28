/*
**	Copyright (c) 2004 Actian Corporation
*/
#ifndef EX_H_INCLUDED
#define EX_H_INCLUDED

#include    <excl.h>

/**CL_SPEC
** Name:	EX.h	- Define EX function externs
**
** Specification:
**
** Description:
**	Contains EX function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**     26-aug-1993 (huffman)
**          EXdeclare is a macro on AXP VMS machine.  Therefore
**          check to see if it is defined before declaring EXdeclare.
**	21-mar-94 (dkh)
**		Added support for new entry point EXsetclient().
**	15-aug-1997 (canor01)
**	    Complete prototype for EXdeclare().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-Jan-2004 (hanje04)
**          Change functions using old, single argument va_start (in
**          varargs.h), to use newer (ANSI) 2 argument version (in stdargs.h).
**          This is because the older version is no longer implemented in
**          version 3.3.1 of GCC.
**	28-feb-2005 (wanfr01)
**	    Bug 64899, INGSRV87
**	    Added defines for TRY/CATCH
**	24-Nov-2009 (frima01) Bug 122490
**	    Added prototype for EXsetsig to eliminate gcc 4.3 warnings.
**	    clconfig.h needs to be included as well for the return type.
**	15-Dec-2009 (frima01) Bug 122490
**	    Moved prototype for EXsetsig to hdr_unix/excl.h cause it is
**	    Unix specific.
**	11-Nov-2010 (kschendel)
**	    Nested inclusion protection.  Prototype fixes.
**/

#ifndef	EXdeclare
FUNC_EXTERN STATUS EXdeclare(
	    STATUS	    (*handle)(EX_ARGS *args), 
	    EX_CONTEXT	    *context
);
#endif

#ifndef EXdelete
FUNC_EXTERN STATUS    EXdelete(
	    void
);
#endif

FUNC_EXTERN VOID    EXinterrupt(
	    i4		new_state
);

#ifndef EXmatch
FUNC_EXTERN i4  EXmatch(
	EX, 
	...
);
#endif

#ifndef EXmath
FUNC_EXTERN VOID    EXmath(
	    i4		new_state
);
#endif

#ifndef EXsignal
FUNC_EXTERN VOID    EXsignal( EX, i4, ...);
#endif

FUNC_EXTERN void EXdumpInit(void);

FUNC_EXTERN bool    EXsys_report(
	    EX_ARGS	*exargs,
	    char	*buffer
);

# define	EXsetclient	IIEXsetclient
FUNC_EXTERN STATUS EXsetclient(
	i4	client
);

#if defined(axp_osf)
#define IIEXtry 	TRY
#define IIEXcatch(exc)	CATCH(exc)
#define IIEXendtry	ENDTRY
#else
#define IIEXtry
#define IIEXcatch(exc)	if (0)
#define IIEXendtry
#endif

#endif /* EX_H_INCLUDED */
