# include	<compat.h>
# include	<nm.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<lo.h>  
# include	<er.h>  
# include	<pc.h>  
# include	<si.h>  
# include	"monitor.h"

/*
** Copyright (c) 2004 Actian Corporation
**
**  Call Shell (Command Line Interpreter)
**
**	Shell() first tries to call an alternate shell defined
**	by the macro {shell}, and if it fails tries ING_SHELL
**	and finally lets PCcmdline use it's default.
**
**	History:
**	08-jan-90 (sylviap)
**		Added PCcmdline parameters.
**	25-jan-90 (teresal)
**		Changed location buffer (whichshell) size to be MAX_LOC+1.
**      08-mar-90 (sylviap)
**              Added a parameter to PCcmdline call to pass an CL_ERR_DESC.
**	02-aug-91 (kathryn) Bug 38893
**		Reset line drawing characters using saved string
**		IIMOildInitLnDraw, rather than calling ITldopen.
**	05-jun-11 (G Jones) SIR 125412 
**              Allow \sh to run a command and return an error.
*/

extern i2	Peekch;
i4
shell()
{
	char		*shell_str;
	char		*shell_cmd;
	char		whichshell[MAX_LOC+1];
	i4		os_ret;
	LOCATION	*shlocptr = (LOCATION *)NULL;
	LOCATION	shell_loc;
	CL_ERR_DESC	err_code;

	GLOBALREF	char	*IIMOildInitLnDraw;
	FUNC_EXTERN 	char	*macro();
	FUNC_EXTERN 	char	*getfilenm();


	if ((shell_str = macro(ERx("{shell}"))) == NULL)
		NMgtAt(ERx("ING_SHELL"), &shell_str);

	if (shell_str == NULL || *shell_str == NULLCHAR)
	{
		shlocptr = (LOCATION *)NULL;
	}
	else		/* use default shell */
	{
		STcopy(shell_str, whichshell);
		shlocptr = &shell_loc;
		LOfroms(PATH & FILENAME, whichshell, shlocptr);
	}
	shell_cmd=getfilenm();
	os_ret=PCcmdline(shlocptr,
		shell_cmd, 
		PC_WAIT, NULL, &err_code);

	if (Peekch == (i2)'\n')
		Peekch = (i2)0;	/* throw out new line at end of command */

#ifndef FT3270
	/* Re-Initialize IT line drawing - BUG 1702 (ncg) */
	{
	    if (Outisterm && IIMOildInitLnDraw != NULL)
		SIprintf(ERx("%s"), IIMOildInitLnDraw);
	}
#endif
	/* print new prompt to indicate return to INGRES */

	if (os_ret != 0)
	{
		putprintf(ERx("Call to shell returned %d for:\n%s\n"),
			os_ret,shell_cmd);
		return -1;
	}
	return 0;
}
