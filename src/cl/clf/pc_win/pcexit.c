/******************************************************************************
**
**	Copyright (c) 1983, 2011 Actian Corporation
**
******************************************************************************/

#include	<compat.h>
#include	<pc.h>
#include	<wn.h>
#include	<cv.h>

# define        MAXFUNCS    32

#ifdef xde
#define	STATICDEF       static
#define CLEAR_ERR(s) \
    { \
        (s)->intern = ((u_i4) 0); \
        (s)->callid = ((u_i4) 0); \
        (s)->errnum  = ((nat)  0); \
    }
#endif

STATICDEF	i4      PCexiting = 0;
STATICDEF	bool    bPChelpCalled = FALSE;
STATICDEF	VOID    (*PCexitfuncs[MAXFUNCS])() = {0};
STATICDEF	VOID    (**PClastfunc)() = NULL;
STATICDEF	char	szProgram[_MAX_PATH+10] = "INGRES";
STATICDEF	bool    isOpenROAD_PM;
STATICDEF	i4      isOpenROADCount = 0;
GLOBALREF       i4      clf_dll_module;  /* Identifies when executing inside a DLL */


GLOBALREF	HWND	hWndStdout;		/* valerier, A25 */
/******************************************************************************
**	Name:
**		PCexit.c
**
**	Function:
**		PCexit
**
**	Arguments:
**		i4	status;
**
**	Result:
**		Program exit.
**
**		Status is value returned to shell or command interpreter.
**
**	Side Effects:
**		None
**
**
**	History:
**	 ?	Initial version
**	17-dec-1996 (donc)
**	   Melded the OpenINGRES PCexit with the OpenROAD version. Created
**	   a version which allows the former, character-based, behavior to 
**	   co-exist with a Windows/GUI behavior. 
**	25-may-97 (mcgem01)
**	   Cleaned up compiler warnings.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**	8-dec-1999 (crido01) Bug 99701
**	   Add isOpenROADCount static.  It keeps track of how many times
**	   OpenROAD calls PCpinit.  If PCPinit is called more than once, do
**	   not show popup message via PCexit_or when shutting down.   
**	09-Apr-2002 (yeema01) Bug 107276, 105960
**	   Provided support for Trace Window.
**	   Added the #define in order to build this file on Unix with Mainwin.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**  19-aug-2009 (zhayu01) SIR 121210
**      Adopted it for Pocket PC/Windows CE.
**	19-Nov-2010 (bonro01) Bug 124685
**	    Update PCatexit return type to VOID to match new prototype.
**      07-Jan-2011 (horda03) b124907
**          Only use atexit() if the PCatexit function isn't in a DLL. If
**          atexit used and the DLL is unloaded before the process terminates
**          at process termination a crash will occur because the onexit handling
**          tries to execute a function which isn't installed.
**          If in a DLL use the OR (now renamed to dll) method of keeping our own 
**          list of the functions to execute on termination. Use our own DLL entry 
**          routine to handle load/unload of the DLL executing the PCatexit functions 
**          when DLL being unloaded.
**          Modified PCexit_dll (nee PCexit_or) to invoke PC_doexitfuncs so that if i
**          the function is also invoked due to DLL unload the exit functions are only 
**          executed once.
**
******************************************************************************/
VOID
PCexit(i4  status)
{
    VOID PCexit_dll();

    if (!isOpenROAD_PM)
        exit(status);
    else
	PCexit_dll( status );
}

/******************************************************************************
**
**
**
******************************************************************************/

/*
**  Name:   PC_doexitfuncs - Execute stored exit functions
**
**  History:
**       07-Jan-2011 (horda03) b124907
**            Created.
*/
void
PC_doexitfuncs()
{
   VOID   (**f)() = PClastfunc;
   MEMORY_BASIC_INFORMATION info;

   if (f)
   {
      while(f-- != PCexitfuncs)
      {
         /* Ignore exit functions if they've been run already. */

         if (*f)
         {
            if ( ! (VirtualQuery( (LPCVOID) **f, &info, sizeof(info))) )
            {
               /* Don't expect this but unloading DLL so ignore */

               continue;
            }

            /* MEM_COMMIT state indicates the memory is accessible, I'm
            ** assuming all other states (currently 2) mean the memory isn't
            ** defined and would cause the access violation if executed.
            */
            if (info.State == MEM_COMMIT)
            {
               (**f)();
            }

            *f = 0;
         }
      }
   }
}

/*
**  Name:   PCatexit - set exit function in the function table
**
**  History:
**      15-mar-89 (puree)
**          Rollover from 6.1 DOS version
&&      07-Jan-2011 (horda03) b124907
**          Only use atexit if not in a DLL. Execution of the
**          exit function will occur as part of the DLL unload.
**      31-Jan-2011 (horda03)
**          PCatexit is a VOID function.
*/
VOID
PCatexit( func )
VOID (*func)();
{
    VOID PCatexit_dll();

    if (!isOpenROAD_PM && !clf_dll_module)
	atexit((VOID (*)(VOID)) func);
    else
	PCatexit_dll( func );
}

VOID
PCexit_dll(i4 status)
{
	register VOID   (**f)() = PClastfunc;

	/* 
	** A19 - put in popup if exiting with bad status
	*/
	/* valerier, A25 - Get rid of message box for console apps. */
	if (isOpenROAD_PM && (isOpenROADCount < 2 ))
	{
	   if (hWndStdout && status && (status != OK_TERMINATE))
	   {
		GLOBALREF HWND hWndDummy;

		ShowWindow( hWndStdout, SW_SHOWNORMAL );
		strcat(szProgram, " exiting");
		MessageBox(hWndDummy, szProgram, NULL, MB_APPLMODAL);
	   }
#ifndef WCE_PPC
	   if (bPChelpCalled)
		WinHelp(NULL, (LPCTSTR)NULL, HELP_QUIT, (DWORD)NULL);
#endif
        }

	if (!PCexiting && f)
	{
		/* avoid recursive calls */
		PCexiting = 1;

		/* execute all exit functions */
                PC_doexitfuncs();
	}

	if (isOpenROAD_PM && (isOpenROADCount < 2 ))
	{
	   SetCursor(LoadCursor(NULL, IDC_ARROW));
	   if (hWndStdout)
		DestroyWindow(hWndStdout);	// This avoids a bug in Win32s version 1.2
	}
	exit(status);
}

VOID
PCatexit_dll(
VOID    (*func)())
{
    if (!PClastfunc)
		PClastfunc = PCexitfuncs;
    if (PClastfunc < &PCexitfuncs[MAXFUNCS])
		*PClastfunc++ = func;
}

VOID
PCsaveModuleName(
char *	szPath)
{
	char *	p;
	
	for (p = szPath + strlen(szPath); --p > szPath; )
	{
		if ((*p == '\\') || (*p == ':'))
		{
			++p;
			break;
		}
	}
	strcpy(szProgram, p);
	CVupper(szProgram);
}

VOID
PChelpCalled(
i4  	command, 
BOOL	status)
{
#ifndef WCE_PPC
	if (status)
		bPChelpCalled = (command != HELP_QUIT) ? TRUE : FALSE;
#endif
}

VOID 
PCpinit()
{
	isOpenROAD_PM = TRUE;
	isOpenROADCount++;
}
bool
PCgetisOpenROAD_PM()
{
	return isOpenROAD_PM;
}
