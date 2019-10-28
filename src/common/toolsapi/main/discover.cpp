/*
** Copyright (c) 2009-2011 Actian Corporation. All rights reserved
**
** Description:
**	Main utility program which can be built as either a console
**	application or a Windows application (depending on platform
**	and the define of "BUILD_AS_WINMAIN".  This wrapper simply
**	invokes the "mainprog" method with argc, argv, which in turn
**	is responsible for invoking the "execute" method with proper
**	arguments to invoke the JavaVM.
**
** History:
**    07-Dec-2009 (whiro01)
**	Initial prototype version.
**    22-Feb-2010 (zhayu01)
**	Use WinMain() for Windows.
**    25-Mar-2010 (whiro01)
**	Move the new code out to a separate module to avoid
**	duplication.
**    16-Jun-2010 (whiro01)
**	Added "UNREFERENCED" macro to quiet compile warnings for unreferenced
**	function parameters.
**    11-Nov-2010 (whiro01)
**	Cloned for "makebranch.exe".
**    31-Aug-2011 (whiro01)
**	Rework to call "mainprog" with the arguments, which in turn
**	will call "execute" to invoke the JavaVM.
**    02-Sep-2011 (whiro01)
**	Rework again to make this the generic program and define the
**	jar file name and its checksum externally.
**    19-Sep-2011 (whiro01)
**	Use "GetExeName" to get program path to look for .jar file.
**    24-Sep-2011 (whiro01)
**	On Linux (at least) "fork" in the case of BUILD_AS_WINMAIN
**	and exit the main right away, letting the child continue.
**    26-Sep-2011 (whiro01)
**	Branched "main.cpp" to "discover.cpp" and define JAR_FILE here.
**    29-Sep-2011 (whiro01)
**	Rename "discover.jar" to "iimgmtsvr.jar" and include its checksum
**	in here via #include (instead of relying on command line
**	definition (which is harder to do in Jam).
*/

#include "execute.h"
#include "util.h"

#ifdef INCLUDE_SPLASH
#include "splash.h"
#endif

#define QUOTE_(x)	#x
#define QUOTE(x)	QUOTE_(x)

#define	JAR_FILE	iimgmtsvr.jar
#include "iimgmtsvr.checksum"


#if defined(WIN32) && defined(BUILD_AS_WINMAIN)
int WINAPI
WinMain
(
    HINSTANCE	hInst,
    HINSTANCE	hPrevInst,
    LPSTR	lpCmdLine,
    int		nCmdShow
)
{
	int argc = 0;
	char *argv[MAX_ARGV];

	UNREFERENCED(hInst);
	UNREFERENCED(hPrevInst);
	UNREFERENCED(nCmdShow);

	// Dummy value for now, the code below
	// will get the full path and substitute that
	argv[0] = "main";
	argc++;

	argc = ParseCommandLineIntoArgv(lpCmdLine, argc, argv);

#else
int
main(int argc, char **argv)
{
#endif
	char exename[_MAX_PATH];
	if (GetExeName(exename))
	    argv[0] = exename;

#if defined(LINUX) && defined(BUILD_AS_WINMAIN)
	pid_t ret = fork();
	// If we are the parent and "fork" didn't succeed (-1)
	// or we are the child (0) then just continue executing.
	// Else we are the parent and the "fork" succeeded (pid)
	// so just exit right away.
	if (ret != -1 && ret != 0)
	    return 0;
#endif

#ifdef INCLUDE_SPLASH
	return execute(argv[0], QUOTE(JAR_FILE), QUOTE(CHECKSUM), image, sizeof(image), argc, argv);
#else
	return execute(argv[0], QUOTE(JAR_FILE), QUOTE(CHECKSUM), NULL, 0, argc, argv);
#endif
}
