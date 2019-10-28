/*
**  Copyright (C) 2005-2006 Actian Corporation. All Rights Reserved.		       
*/

/*
**    Source   : sqlerr.cpp: implementation
**    Project  : Meta data library 
**    Author   : Schalk Philippe(schph01)
**    Purpose  : Store SQL error in temporary file name.
**
** History:
**
** 16-Jul-2004 (schph01)
**    Created
** 04-Nov-2010 (drivi01) 
**    Port the solution to x64. Clean up the warnings.
**    Clean up datatypes. Port function calls to x64.
**/

#include "stdafx.h"
#include "mgterr.h"
#include "ingobdml.h"

static TCHAR tcErrorFileName[_MAX_PATH]= _T("");
static TCHAR tcGlobalErrorFileName[_MAX_PATH]= _T("");

BOOL INGRESII_llInitGlobalErrorLogFile()
{
	TCHAR* penv = _tgetenv(_T("II_SYSTEM"));
	if (!penv || *penv == _T('\0'))
		return FALSE;

	// build full file name with path
	lstrcpy(tcGlobalErrorFileName, penv);
#if defined (MAINWIN)
	lstrcat(tcGlobalErrorFileName, _T("/"));
	lstrcat(tcGlobalErrorFileName, _T("ingres"));
	lstrcat(tcGlobalErrorFileName, _T("/files/errvdba.log"));
#else
	lstrcat(tcGlobalErrorFileName, _T("\\"));
	lstrcat(tcGlobalErrorFileName, _T("ingres"));
	lstrcat(tcGlobalErrorFileName, _T("\\files\\errvdba.log"));
#endif

	return TRUE;
}

void INGRESII_llSetErrorLogFile(LPTSTR pFileName)
{
	if ( pFileName && *pFileName != _T('\0'))
		lstrcpy(tcErrorFileName,pFileName);
	else
		tcErrorFileName[0]=_T('\0');

	if ( tcGlobalErrorFileName && *tcGlobalErrorFileName == _T('\0'))
	{
		if (!INGRESII_llInitGlobalErrorLogFile())
			tcGlobalErrorFileName[0]=_T('\0');
	}
}
LPTSTR INGRESII_llGetErrorLogFile()
{
	return tcErrorFileName;
}



void INGRESII_llWriteInFile(LPTSTR FileName, LPTSTR SqlTextErr,LPTSTR SqlStatement,int iErr)
{
	char      buf[256];
	char      buf2[256];
	HANDLE     hFile;
	DWORD	  dwSize;
	char      *p, *p2;
	char	  *crnl = "\r\n";

	//open a file for writing with shared access
	if (FileName[0])
		hFile = CreateFile(FileName, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile) 
	{
		if (SetFilePointer(hFile, 0, NULL, FILE_END) != INVALID_SET_FILE_POINTER)
		{
		// statement that produced the error
		_tcscpy(buf, "Erroneous statement:");
		WriteFile(hFile, buf, lstrlen(buf), &dwSize, NULL);
		WriteFile(hFile, crnl, 2, &dwSize, NULL);
		WriteFile(hFile, SqlStatement, lstrlen(SqlStatement), &dwSize, NULL);
		WriteFile(hFile, crnl, 2, &dwSize, NULL);

		// Sql error code
		_tcscpy(buf, "Sql error code: %ld");
		wsprintf(buf2, buf, iErr);
		WriteFile(hFile, buf2, lstrlen(buf2), &dwSize, NULL);
		WriteFile(hFile, crnl, 2, &dwSize, NULL);

		// Sql error text line after line and time after time
		_tcscpy(buf, "Sql error text:");
		WriteFile(hFile, buf, lstrlen(buf), &dwSize, NULL);
		WriteFile(hFile, crnl, 2, &dwSize, NULL);
		p = SqlTextErr;
		while (p) {
			p2 = strchr(p, '\n');
			if (p2) {
				WriteFile(hFile, p, (DWORD)(p2-p), &dwSize, NULL);		
				WriteFile(hFile, crnl, 2, &dwSize, NULL);
				p = ++p2;
			}
			else {
				WriteFile(hFile, p, lstrlen(p), &dwSize, NULL);		
				WriteFile(hFile, crnl, 2, &dwSize, NULL);
				break;
			}
		}

		// separator for next statement
		memset(buf, '-', 70);
		buf[70] = '\0';
		WriteFile(hFile, buf, lstrlen(buf), &dwSize, NULL);		
		WriteFile(hFile, crnl, 2, &dwSize, NULL);
		}
		CloseHandle(hFile);
	}
}

void INGRESII_ManageErrorInLogFiles( LPTSTR SqlTextErr,LPTSTR SqlStatement,int iErr)
{
	INGRESII_llWriteInFile( tcErrorFileName,SqlTextErr,SqlStatement,iErr);
	INGRESII_llWriteInFile( tcGlobalErrorFileName,SqlTextErr,SqlStatement,iErr);
}
