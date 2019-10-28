/*
** Copyright (c) 2010-2011 Actian Corporation. All rights reserved
**
** Description:
**	Utility functions to accompany our main program template.
**
** History:
**    25-Mar-2010 (whiro01)
**	Extracted from "main.cpp" and augmented with new functionality.
**    28-Apr-2010 (whiro01)
**	<intrin.h> is not available with the VS2003.Net compiler, so check
**	compiler version and define it differently if needed.
**    06-Jun-2010 (whiro01)
**	Add a NULL pointer entry at the end of 'argv' array to stop
**	running off the end of the list into uninit memory inside java.c
**    01-Sep-2011 (whiro01)
**	Add functions to convert locale-based strings to UTF-8.
**    02-Sep-2011 (whiro01)
**	Add "dir_exists" (in addition to "file_exists" added last rev).
**    19-Sep-2011 (whiro01)
**	Implement "GetExeName" on OSX.
**    21-Sep-2011 (whiro01)
**	Implement "GetExeName" on Linux.
**    25-Sep-2011 (whiro01)
**	Use NT_GENERIC instead of WINDOWS.
*/

#include <stdlib.h>
#include "util.h"

#if defined(NT_GENERIC) || defined(WIN32)
#if _MSC_VER >= 1500
#include <intrin.h>
#else
#ifdef __cplusplus
extern "C"
#endif
void *_ReturnAddress(void);
#endif
typedef bool (FAR WINAPI *PFNGETMODULEHANDLEEX)(DWORD, LPCTSTR, HMODULE *);
#endif

#if defined(DARWIN) || defined(OSX)
#include <CoreFoundation/CFBundle.h>
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
**	21-Sep-2011 (whiro01)
**	    Implement for Linux.
**
*/
bool
GetExeName(char exename[_MAX_PATH])
{
#if defined(WIN32) || defined(NT_GENERIC)
	DWORD	dwSize = 0;
	HMODULE	hModule;
	HMODULE	hDll = NULL;
	PFNGETMODULEHANDLEEX pfnGetModuleHandleEx = NULL;
#endif
	bool	ret = false;

#if defined(WIN32) || defined(NT_GENERIC)
	LPCTSTR pEnv = (LPCTSTR)_ReturnAddress();

	if ((hDll = LoadLibrary( "kernel32.dll" )) != NULL)
	{
	    pfnGetModuleHandleEx = (PFNGETMODULEHANDLEEX)
			GetProcAddress(hDll, "GetModuleHandleExA");
	}

	if (pfnGetModuleHandleEx != NULL)
	{
	    if ((*pfnGetModuleHandleEx)(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, pEnv, &hModule))
	    {
		dwSize = _MAX_PATH;
		if (GetModuleFileName(hModule, exename, dwSize) != 0)
			ret = true;
	    }
	}

	if (hDll != NULL)
	    FreeLibrary(hDll);
#elif defined(DARWIN) || defined(OSX)
	CFBundleRef bundle = CFBundleGetMainBundle();
	CFURLRef executableURL = CFBundleCopyExecutableURL(bundle);
	CFStringRef executablePath = CFURLCopyFileSystemPath(executableURL, kCFURLPOSIXPathStyle);
	if (CFStringGetFileSystemRepresentation(executablePath, exename, _MAX_PATH))
	    ret = true;
	CFRelease(executablePath);
	CFRelease(executableURL);
#elif defined(LINUX)
	char link[100];
	sprintf(link, "/proc/%d/exe", getpid());
	ssize_t len = readlink(link, exename, _MAX_PATH);
	exename[len] = '\0';
	ret = true;
#else
	#error	Must implement GetExeName for this platform!
#endif

	return ret;
}


#if defined(WIN32) || defined(NT_GENERIC)
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
**	06-Jun-2010 (whiro01)
**	    Add a NULL pointer entry at the end of 'argv' array to stop
**	    running off the end of the list into uninit memory inside java.c
**
*/
int
ParseCommandLineIntoArgv(LPSTR lpCmdLine, int argc, char *argv[MAX_ARGV])
{
    char *p;
    char delim_char = EOS;

    /*
    **  Break lpCmdLine into C-style argv[].
    **
    **  Substrings delimited by quotes (single or double) will be treated
    **  as a single token, and the quotes will be removed.
    **
    **  Note.  This is not a sophisticated parser.  It will not deal with
    **  a quote that is embedded in a space-delimited token.
    **
    */
    for (p = lpCmdLine; *p != EOS && argc < MAX_ARGV; p++)
    {
	if (delim_char == EOS)
	{
	    /*
	    **  Skip over white space.
	    */
	    if (*p <= SPACECHAR)
	    {
		continue;
	    }

	    /*
	    **  We are at the beginning of a new token.
	    */
	    switch (*p)
	    {
		case SINGLEQUOTE:
		case DOUBLEQUOTE:
		{
		    delim_char = *p;
		    if (*(p + 1) == EOS)
		    {
			break;
		    }
		    argv[argc++] = p + 1;
		    break;
		}

		default:
		{
		    delim_char = SPACECHAR;
		    argv[argc++] = p;
		    break;
		}
	    }
	}
	else
	{
	    /*
	    **  We are looking for the end of a token.
	    */
	    switch (delim_char)
	    {
		case SPACECHAR:
		{
		    if (*p <= SPACECHAR)
		    {
			*p = EOS;
			delim_char = EOS;
		    }
		    break;
		}

		case SINGLEQUOTE:
		case DOUBLEQUOTE:
		{
		    if (*p == delim_char)
		    {
			*p = EOS;
			delim_char = EOS;
		    }
		    break;
		}
	    }

	}

    }

    argv[argc] = NULL;
    return argc;
}


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
size_t
wcs_to_utf8(char *utfstr, wchar_t *wstr, size_t len)
{
	wchar_t *wptr, wch;
	size_t pos = 0;
	for (wptr = wstr; *wptr != 0; wptr++)
	{
	    wch = *wptr;
	    if (wch <= 0x7F)
	    {
		if (utfstr != NULL)
		{
		    if (pos < len)
			utfstr[pos++] = (char)wch;
		    else
			return pos;
		}
		else
		    pos++;
	    }
	    else if (wch <= 0x7FF)
	    {
		if (utfstr != NULL)
		{
		    if (pos + 1 < len)
		    {
			utfstr[pos++] = (char)(0xC0 | ((wch >> 6) & 0x1F));
			utfstr[pos++] = (char)(0x80 | (wch & 0x3F));
		    }
		    else
			return pos;
		}
		else
		    pos += 2;
	    }
	    else /* if (wch <= 0xFFFF) */
	    {
		if (utfstr != NULL)
		{
		    if (pos + 2 < len)
		    {
			utfstr[pos++] = (char)(0xE0 | ((wch >> 12) & 0x0F));
			utfstr[pos++] = (char)(0x80 | ((wch >> 6) & 0x3F));
			utfstr[pos++] = (char)(0x80 | (wch & 0x3F));
		    }
		    else
			return pos;
		}
		else
		    pos += 3;
	    }
	}

	if (utfstr != NULL && pos < len)
	    utfstr[pos] = '\0';

	return pos;
}

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
size_t
mbs_to_utf8(char *utfstr, char *str, size_t len)
{
	size_t utflen = 0;
	if (str != NULL)
	{
		size_t wlen = mbstowcs( NULL, str, 0 ) + 1;
		wchar_t *wbuf = (wchar_t *)malloc( wlen * sizeof(wchar_t) );
		mbstowcs( wbuf, str, wlen );
		utflen = wcs_to_utf8( NULL, wbuf, 0 );
		if (utfstr != NULL)
		{
			if (utflen <= len)
				wcs_to_utf8( utfstr, wbuf, utflen );
		}
		free(wbuf);
		if (utfstr != NULL && utflen + 1 <= len)
			utfstr[utflen] = '\0';
	}
	return utflen;
}

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
bool
file_exists(const char *filename)
{
	struct stat st;
	if (stat(filename, &st) != 0)
		return false;
	if ((st.st_mode & S_IFREG) != S_IFREG)
		return false;
	return true;
}

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
bool
dir_exists(const char *dirname)
{
	struct stat st;
	if (stat(dirname, &st) != 0)
		return false;
	if ((st.st_mode & S_IFDIR) != S_IFDIR)
		return false;
	return true;
}
