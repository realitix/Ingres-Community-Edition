
/*
**  Name: msiversupdate.c
**
**  Description:
**	Update MSI database with correct Ingres version.
**	
**  History:
**	10-Jan-2008 (drivi01)
**	    Created.
**	25-Feb-2011 (drivi01)
**	    For VectorWise Documentation msi, update some of
**	    the version handling to automate the procedure a bit.
**	    This module will update installation path, shortcut
**          name and internal version number of the msi with
**          a short version of the version.  For example:
**          for release 1.5.0, the version of the module 
**          will be 1.5, the path will be 
**	    %INGRESCORPFOLDER%\\VectorWise Documentation 1.5,
**	    and shortcuts will be Start->Programs->Ingres->
**	    VectorWise Documentation 1.5.
**	    The product name will be hardcoded to make sure that
**	    with every new version product code changes.
*/

#include <windows.h>
#include <winbase.h>
#include <msiquery.h>
#include <compat.h>
#include <gv.h>

GLOBALREF ING_VERSION ii_ver;

i4 
main(i4 argc, char *argv[])
{
     int ret_val=0;
     MSIHANDLE hDatabase;
     char vers[12];
	 char short_vers[12];
     char view[1024];

	 DWORD dw;
	 dw = GetFileAttributes(argv[1]);
	
     if ((argc < 2) || (GetFileAttributes(argv[1])==-1))
     {
		 printf("Usage:\nmsiversupdate <full path to MSI file>\n");
		 ret_val=1;
     }
     if (argc == 2 && GetFileAttributes(argv[1]) != -1)
     {
		 MsiOpenDatabase(argv[1], MSIDBOPEN_DIRECT, &hDatabase);
		 if (!hDatabase)
			 ret_val=1;

		 sprintf(vers, "%d.%d.%d", ii_ver.major, ii_ver.minor, ii_ver.genlevel);
		 sprintf(short_vers, "%d.%d", ii_ver.major, ii_ver.minor);
	

		 /*
		 ** Automatically update version in the MSI module
		 **
		 */
		 if (hDatabase && strstr(argv[1], ".msm"))
		 {
			 sprintf(view, "UPDATE ModuleSignature SET Version='%s'", vers);	
			 if (!ViewExecute(hDatabase, view))
				 ret_val=1;
		 }
		if (hDatabase && strstr(argv[1], ".msi"))
		{
			 sprintf(view, "UPDATE Property SET Value='%s' WHERE Property='ProductVersion'", vers);
			 if (!ViewExecute(hDatabase, view))
				 ret_val=1;
			 if(MsiDatabaseCommit(hDatabase)!=ERROR_SUCCESS)
				 ret_val=1;
			 if (strstr(argv[1], "Ingres.msi") || strstr(argv[1], "IngresII.msi"))
			 {
				 sprintf(view, "UPDATE ModuleSignature SET Version='%s' WHERE Version='9.1.0'", vers);
				 if (!ViewExecute(hDatabase, view))
					 ret_val=1;
				 if(MsiDatabaseCommit(hDatabase)!=ERROR_SUCCESS)
					 ret_val=1;
			 }
			 if (strstr(argv[1], "Documentation") && strstr(argv[1], "VectorWise"))
			 {
				 char szCol[MAX_PATH];
				 char *ptr = NULL;
				 char *sCol = &szCol;
				 sprintf(view, "UPDATE Property SET Value='%s' WHERE Property='ProductVersion'", short_vers);
				 if (!ViewExecute(hDatabase, view))
					 ret_val=1;
				 if(MsiDatabaseCommit(hDatabase)!=ERROR_SUCCESS)
					 ret_val=1;
				 //update path/shortcut
				 sprintf(view, "UPDATE Directory SET DefaultDir='VectorWise Documentation %s' WHERE DefaultDir='VECTOR~1|VectorWise Documentation'", short_vers);
				 if (!ViewExecute(hDatabase, view))
					 ret_val = 1;
				 if(MsiDatabaseCommit(hDatabase)!=ERROR_SUCCESS)
					 ret_val=1;
				
			 }
		}

	 }
     else
     {
		 printf("Wrong number of arguments or MSI file is inaccessible.\n");
		 ret_val=1;
     }

    if (ret_val)
		printf("System function failed with error (%d).\n", GetLastError());
    if(MsiDatabaseCommit(hDatabase)!=ERROR_SUCCESS)
		ret_val=1;
    if(MsiCloseHandle(hDatabase)!=ERROR_SUCCESS)
		ret_val=1;

	return ret_val;
}

BOOL 
ViewExecute(MSIHANDLE hDatabase, char *szQuery)
{
	MSIHANDLE hView;
	BOOL retVal = 1;
	DWORD dwError = 0;
	
	if((dwError = MsiDatabaseOpenView(hDatabase, szQuery, &hView))!=ERROR_SUCCESS)
		retVal = 0;
	if(retVal && (dwError = MsiViewExecute(hView, 0))!=ERROR_SUCCESS)
		retVal = 0;
	if(retVal && MsiCloseHandle(hView)!=ERROR_SUCCESS)
		retVal = 0;

CLEANUP:
	if (hView)
		if (MsiViewClose(hView) == ERROR_SUCCESS)
			MsiCloseHandle(hView);
	
	return retVal;
}

BOOL
ViewExecuteAndFetchCol(MSIHANDLE hDatabase, char *szQuery, int nCol, char **szCol)
{
	MSIHANDLE hView = 0, hRec = 0;
	char *sCol = *szCol;
	DWORD dwSize, dwError;
	BOOL retVal = 1;

	if (MsiDatabaseOpenView(hDatabase, szQuery, &hView)!=ERROR_SUCCESS)
		retVal = 0;	
	if (retVal && MsiViewExecute(hView, 0)!=ERROR_SUCCESS)
		retVal = 0;	
	if (retVal && MsiViewFetch(hView, &hRec) != ERROR_SUCCESS)
		retVal = 0;	
	if (retVal && MsiRecordGetString(hRec, nCol, "", &dwSize) == ERROR_MORE_DATA)
	{
		dwSize+=1;
		if (MsiRecordGetString(hRec, nCol, (LPTSTR)sCol, &dwSize) != ERROR_SUCCESS)
			retVal = 0;	
	}


CLEANUP:
	dwError = GetLastError();
	if (hRec)
		MsiCloseHandle(hRec);
	if (hView)
		if (MsiViewClose(hView) == ERROR_SUCCESS)
			MsiCloseHandle(hView);

	return retVal;
}
