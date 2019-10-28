/*
** Copyright (c) 2010-2011 Actian Corporation. All rights reserved
**
** Description:
**	Header file for utility functions to accompany our main program template.
**
** History:
**    25-Mar-2010 (whiro01)
**	Extracted from "main.cpp" and augmented with new functionality.
**    16-Jun-2010 (whiro01)
**	Added "UNREFERENCED" macro to quiet compile warnings for unreferenced
**	function parameters.
**    01-Sep-2011 (whiro01)
**	Added more platform-specific definitions; added UTF-8 functions.
**    19-Sep-2011 (whiro01)
**	Implement GetExeName for OSX to find executable path.
**    21-Sep-2011 (whiro01)
**	Define _MAX_PATH for all platforms; add some #includes for non-Windows.
**    22-Aug-2011 (whiro01)
**	Using NT_GENERIC instead of WINDOWS for the platform tests; add
**	OSX in addition to DARWIN.
*/

#ifndef _UTIL_H_INCLUDED
#define _UTIL_H_INCLUDED

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#if defined(NT_GENERIC) || defined(WIN32)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#define FILE_SEPARATOR	'\\'
	#define PATH_SEPARATOR	';'
	#define PATH_VAR	"PATH"
	#define putenv		_putenv
	#define S_IFREG		_S_IFREG
	#define S_IFDIR		_S_IFDIR
#else
	#include <stdio.h>
	#include <unistd.h>
	#define FILE_SEPARATOR	'/'
	#define PATH_SEPARATOR	':'
	#include <dlfcn.h>
	#if defined(LINUX) || defined(UNIX)
		#define PATH_VAR	"LD_LIBRARY_PATH"
	#elif defined(DARWIN) || defined(OSX)
		#define	PATH_VAR	"DYLD_LIBRARY_PATH"
	#else
		#error	Need definitions for this platform!
	#endif
#endif

#define UNREFERENCED(x)	(x)


#ifdef WIN32
#define SINGLEQUOTE	'\''
#define DOUBLEQUOTE	'"'
#define SPACECHAR	' '
#define EOS		'\0'
#define MAX_ARGV	256
#endif
#ifndef _MAX_PATH
#define	_MAX_PATH	1024
#endif


/*
** Name: GetExeName - Get name of current executable.
**
** Description:
**	Queries the system to get the name of the current
**	executable (that is, what is our ".exe" name).
**	Note: relies on being called from the "main" or
**	"WinMain" procedure (instead of from a DLL) in order
**	to function correctly.
**
** Input:
**	None.
**
** Output:
**	exename	- filled in with the fully-qualified path name
**		of the executable we are running as.
**
** Returns:
**	true if successful
**	false if there was some error (not likely)
**
** History:
**	25-Mar-2010 (whiro01)
**	    Created.
**	19-Sep-2011 (whiro01)
**	    Implement for OSX.
**
*/
extern bool GetExeName(char exename[_MAX_PATH]);


#ifdef WIN32
/*
** Name: ParseCommandLineIntoArgv - Parse full command line into argv-style strings.
**
** Description:
**	Takes a full command line (as passed to "WinMain") and parses it into
**	argv-style strings.  Arguments are separated by spaces unless quoted
**	by single or double quotes (must match).
**	Note: embedded quotes are not supported, so be careful!
**
** Input:
**	lpCmdLine	- input command line supplied by caller of "WinMain"
**	argc		- count of arguments already added (executable name usually)
**			before parsing
**	argv		- [0]..[argc-1] already filled in
**
** Output:
**	argv	- parsed arguments are filled in starting from input [argc] to
**		output [argc-1]
**
** Returns:
**	new value for argc (that is, the number of slots in "argv" filled in)
**
** History:
**	22-Feb-2010 (zhayu01)
**	    Created.
**	25-Mar-2010 (whiro01)
**	    Moved to separate module; some minor cleanup.
**
*/
extern int ParseCommandLineIntoArgv(LPSTR lpCmdLine, int argc, char *argv[MAX_ARGV]);

#endif	/* WIN32 */

/*
** Name: wcs_to_utf8 - encode a wide character string to UTF-8
**
** Description:
**	Convert the wide character string, character by character,
**	to UTF-8 and store in "utfstr".  Returns number of bytes
**	stored (excluding the trailing EOS).  If output buffer is
**	NULL, no output is stored and the input length is ignored.
**
**	Note: this only works for characters in the BMP.
**
** Inputs:
**	wstr	- pointer to wide character string
**	len	- length of output buffer (including EOS) in chars
**
** Outputs:
**	utfstr	- pointer to output buffer (can be NULL)
**
** Returns:
**	size_t	- number of bytes written/required in output buffer
**
*/
extern size_t wcs_to_utf8(char *utfstr, wchar_t *wstr, size_t len);

/*
** Name: mbs_to_utf8 - encode a code-page string to UTF-8
**
** Description:
**	Convert the code-page string, character by character,
**	to UTF-8 and store in "utfstr".  Returns number of bytes
**	stored (excluding the trailing EOS).  If output buffer is
**	NULL, no output is stored and the input length is ignored.
**
**	Note: this only works for characters in the BMP.
**
** Inputs:
**	wstr	- pointer to code-page string
**	len	- length of output buffer (including EOS) in chars
**
** Outputs:
**	utfstr	- pointer to output buffer (can be NULL)
**		  (because this routine uses an intermediate buffer,
**		   input and output can be the same location)
**
** Returns:
**	size_t	- number of bytes written/required in output buffer
**
*/
extern size_t mbs_to_utf8(char *utfstr, char *str, size_t len);

/*
** Name: file_exists - determine if a file exists and is a regular
**			file (i.e., not a directory)
**
** Description:
**	Call 'stat' on the given file name (with path).  If the
**	call is successful and the "st_mode" indicates a regular
**	file, return 'true', else 'false'.
**
** Inputs:
**	filename	- name of the file to test existence
**			  (terminated by null byte)
**			  (must be in codepage encoding)
**
** Outputs:
**	None.
**
** Returns:
**	bool	- true if the file exists and is a regular file,
**		  false otherwise
**
*/
extern bool file_exists(const char *filename);

/*
** Name: dir_exists - determine if a directory exists
**
** Description:
**	Call 'stat' on the given path.  If the call is
**	successful and the "st_mode" indicates a directory
**	return 'true', else 'false'.
**
** Inputs:
**	dirname	- name of the directory to test existence
**		  (terminated by null byte)
**		  (must be in codepage encoding)
**
** Outputs:
**	None.
**
** Returns:
**	bool	- true if the path exists and is a directory,
**		  false otherwise
**
*/
extern bool dir_exists(const char *dirname);

#endif	/* _UTIL_H_INCLUDED */
