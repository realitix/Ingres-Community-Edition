/*
** Copyright (C) 1996, 2009 Actian Corporation All Rights Reserved.
*/
/*
** Name:    pctask.c
**
** Description:
**      Enumerate the process list.
**
** History:
**      27-Oct-97 (fanra01)
**          Modified to get process list in Windows 95.
**	26-mar-1998 (somsa01)
**	    Needed to initialize dwLimit in GetProcessList() to MAX_TASKS.
**      14-Apr-98 (fanra01)
**          During read of performance key remember the size of memory used.
**          The RegQueryValueEx call corrupts the value when reading
**          HKEY_PERFORMANCE_DATA and returning ERROR_MORE_DATA.
**      10-Jul-98 (fanra01)
**          Reset lStatus after buffer extend otherwise the loop exits too
**          soon.
**      31-aug-1998 (canor01)
**          Remove other CL dependencies so the object can be linked into
**          arbitrary programs without requiring the full CL.
**	01-apr-1999 (somsa01)
**	    Added PCsearch_Pid, which takes a pid and searches the task
**	    list to see if it is there.
**	29-jun-1999 (somsa01)
**	    In GetProcessList(), we were doing a RegCloseKey() on
**	    HKEY_PERFORMANCE_DATA even though we never had this key
**	    specifically opened. This call was what caused this function
**	    to take an enormous amount of time, and also caused stack
**	    corruption. Sometimes (specifically when Oracle and Microsoft
**	    SQL Server were installed on the same machine), this would
**	    eventually lead a process that used PCget_PidFromName() to
**	    give an Access Violation on a printf (or, SIprintf) statement.
**	13-sep-1999 (somsa01)
**	    In GetProcessList(), initialize dwNumTasks to zero so that, in
**	    case of errors, we do not return a bogus value.
**	03-nov-1999 (somsa01)
**	    Put back the RegCloseKey() on HKEY_PERFORMANCE_DATA in
**	    GetProcessList(). Fixes elswhere have properly fixed 97704.
**	28-jun-2001 (somsa01)
**	    For OS Version, use GVosvers().
**	07-aug-2001 (somsa01)
**	    Cleaned up compiler warnings.
**	20-oct-2001 (somsa01)
**	    Added inclusion of systypes.h.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	07-May-2008 (whiro01)
**	    Made more common code routines.
**  19-aug-2009 (zhayu01) SIR 121210
**      Adopted it for Pocket PC/Windows CE.
**	22-Nov-2010 (hweho01)
**	    Add PCget_AllPidsFromName(), it will be called by  
**	    stop_x100server(). 
**	27-Jun-2011 (drivi01)
**	    Add PDirName function to this file.  Add PDirName call
**	    from PCget_PidFromName that will check whether the process
**	    being returned is run in this installation by comparing
**	    II_SYSTEM of the full command.
**	27-Jul-2011 (drivi01)
**	    Changed QueryFullProcessImageName reference to 
**	    pfnQueryFullProcessImageName pointer.
**	31-Aug-2011 (drivi01) b125710
**	    Update pfnQueryFullProcessImageName to a pointer of
**	    type INT_PTR, on 64-bit OS, the 
**	    pfnQueryFullProcessImageName points to an 8-byte
**	    address, therefore the pointer should be typecast
**	    to INT_PTR for comparison and will be 4 bytes
**	    on 32-bit OS and 8 bytes on 64-bit.
*/

# include <stdio.h>
# include <windows.h>
# ifndef WCE_PPC
# include <winperf.h>
# endif
# include <tlhelp32.h>
# include <compat.h>
# include <systypes.h>
# include <psapi.h>
# include <gl.h>
# include <st.h>
# include <cv.h>

//
// manifest constants
//
#define MAX_TASKS           256

#define INITIAL_SIZE        51200
#define EXTEND_SIZE         25600
#define REGKEY_PERF         "software\\microsoft\\windows nt\\currentversion\\perflib"
#define REGSUBKEY_COUNTERS  "Counters"
#define PROCESS_COUNTER     "process"
#define PROCESSID_COUNTER   "id process"
#define UNKNOWN_TASK        "unknown"
#define TITLE_SIZE          64
#define PROCESS_SIZE        16

typedef struct _TASK_LIST {
    DWORD       dwProcessId;
    DWORD       dwInheritedFromProcessId;
    BOOL        flags;
    HANDLE      hwnd;
    CHAR        ProcessName[PROCESS_SIZE];
    CHAR        WindowTitle[TITLE_SIZE];
} TASK_LIST, *PTASK_LIST;

typedef struct _TASK_LIST_ENUM {
    PTASK_LIST  tlist;
    DWORD       numtasks;
} TASK_LIST_ENUM, *PTASK_LIST_ENUM;

static DWORD GetProcessList (PTASK_LIST  pTask, DWORD nsize);
static DWORD GetProcessListNT (PTASK_LIST  pTask, DWORD nsize);
static DWORD GetProcessList95 (PTASK_LIST  pTask, DWORD nsize);
static int   Is_w95 = -1;

static OFFSET_TYPE (FAR WINAPI *pfnCreateToolhelp32Snapshot)() = NULL;
static OFFSET_TYPE (FAR WINAPI *pfnProcess32First)() = NULL;
static OFFSET_TYPE (FAR WINAPI *pfnProcess32Next)() = NULL;
static INT_PTR (FAR WINAPI *pfnQueryFullProcessImageName)(HANDLE hProcess, DWORD dwFlags, LPTSTR lpExeName, PDWORD lpdwSize) = -1;

extern BOOL GVosvers(char *OSVersionString);

/*
** Name: PCget_PidFromName
**
** Description:
**      Returns the process id of the specified process
**
** Inputs:
**      pszProcess      name of the process
**
** Outputs:
**      None.
**
** Returns:
**      dwProcessId
**      0               if not found
**
** History:
**      01-Oct-97 (fanra01)
**          Created.
**      28-Oct-97 (fanra01)
**          Add Windows 95 specific calls to get process list.  Load the
**          functions from the DLL otherwise builds will break.
**          Modified string compare.  Since the format of string is different
**          between NT and W95.
**	07-May-2008 (whiro01)
**	    Made more common code routines.
**	22-Nov-2010 (hweho01)
**	    Change to use the buffer that is dynamically allocated during
**	    the runtime, so it is sufficient to hold all process IDs
**	    on the system.
**	27-Jun-2011 (drivi01)
**	    Add PDirName call from PCget_PidFromName that will check 
**	    whether the process being returned is run in this 
**	    installation by comparing II_SYSTEM of the full command.
*/
DWORD
PCget_PidFromName (void *pszProcessName)
{
    DWORD dwNumProc = 0;
    unsigned int i;
    PTASK_LIST tlist = NULL; 
    PTASK_LIST pTask = NULL; 
    bool       NOT_DONE = TRUE;
    DWORD nsize ; 
    char ii_system_buf[MAX_PATH];
    char *ii_system;
    
    nsize = (DWORD) MAX_TASKS ; 

    strlwr(pszProcessName);
    *ii_system_buf = EOS;
    NMgtAt(SYSTEM_LOCATION_VARIABLE, &ii_system);
    if (ii_system && *ii_system)
    {
	STlcopy(ii_system, ii_system_buf, sizeof(ii_system_buf));
	CVlower(ii_system_buf);
    }

    tlist = (PTASK_LIST) calloc( nsize, sizeof(TASK_LIST) ); 

    while( NOT_DONE && tlist != NULL )
    {
      dwNumProc = GetProcessList (tlist, nsize);
      /* if list buffer is not big enough, reallocate and try again */
      if ( dwNumProc >= nsize )
      {
        free(tlist);
        nsize +=  MAX_TASKS ; 
        tlist = (PTASK_LIST) calloc( nsize, sizeof(TASK_LIST) );
      }  
      else
        NOT_DONE = FALSE ;
    }
    if (dwNumProc != 0)
    {
        for (i=0, pTask = tlist ; i < dwNumProc; i++)
        {
            if (strstr(strlwr(pTask->ProcessName), pszProcessName)
                != NULL)
            {
                char path[MAX_PATH];
                if (ii_system_buf && *ii_system_buf && !PDirName(pTask->dwProcessId, path))
                {
                     CVlower(path);
                     if (STstr(path, pszProcessName) && STstr(path, ii_system_buf))
                          return (pTask->dwProcessId);
                }
            }
            pTask++;
        }
    }
    if( tlist != NULL )
      free( tlist ); 
    
    return (0);
}

/*
** Name: PCsearch_Pid
**
** Description:
**	Searches the machine's process list to see if a particular pid
**	exists for a particular process name.
**
** Inputs:
**	pszProcess	name of the process
**	pid		pid to search for
**
** Outputs:
**	None.
**
** Returns:
**	1	if found
**	0	if not found
**
** History:
**	01-apr-1999 (somsa01)
**	    Created.
**	22-Nov-2010 (hweho01)
**	    Change to use the buffer that is dynamically allocated during
**	    the runtime, so it is sufficient to hold all process IDs
**	    on the system.
*/
DWORD
PCsearch_Pid (void *pszProcessName, DWORD pid)
{
DWORD dwNumProc;
unsigned int i;
PTASK_LIST tlist = NULL; 
PTASK_LIST pTask = NULL; 
bool       NOT_DONE = TRUE;
DWORD nsize ; 
    
    nsize = (DWORD) MAX_TASKS ; 

    strlwr(pszProcessName);

    tlist = (PTASK_LIST) calloc( nsize, sizeof(TASK_LIST) ); 

    while( NOT_DONE && tlist != NULL )
    {
      dwNumProc = GetProcessList (tlist, nsize);
      /* if list buffer is not big enough, reallocate and try again */
      if ( dwNumProc >= nsize )
      {
        free(tlist);
        nsize +=  MAX_TASKS ;
        tlist = (PTASK_LIST) calloc( nsize, sizeof(TASK_LIST) );
      }  
      else
        NOT_DONE = FALSE ;
    }

    if (dwNumProc != 0)
    {
	for (i=0, pTask = tlist ; i < dwNumProc; i++)
	{
	    if ((pTask->dwProcessId == pid) &&
		(strstr(strlwr(pTask->ProcessName), pszProcessName)))
	    {
		return (1);
	    }
            pTask++; 
	}
    }
    if( tlist != NULL )
      free( tlist ); 
    return (0);
}

/*
** Name:    GetProcessListNT
**
** Description:
**      Obtains a list of running processes.
**
** Inputs:
**      pTask           array of task list structures.
**
** Outputs:
**      none.
**
** Return:
**      DWORD           dwNumTasks
**      0               if not found
**
** History:
**      01-Oct-97 (fanra01)
**          Created.
**	26-mar-1998 (somsa01)
**	    Needed to initialize dwLimit in GetProcessList() to MAX_TASKS.
**      14-Apr-98 (fanra01)
**          During read of performance key remember the size of memory used.
**          The RegQueryValueEx call corrupts the value when reading
**          HKEY_PERFORMANCE_DATA and returning ERROR_MORE_DATA.
**	29-jun-1999 (somsa01)
**	    We were doing a RegCloseKey() on HKEY_PERFORMANCE_DATA even
**	    though we never had this key specifically opened. This call was
**	    what caused this function to take an enormous amount of time, and
**	    also caused stack corruption. Sometimes (specifically when Oracle
**	    and Microsoft SQL Server were installed on the same machine),
**	    this would eventually lead a process that used
**	    PCget_PidFromName() to give an Access Violation on a printf (or,
**	    SIprintf) statement.
**	13-sep-1999 (somsa01)
**	    Initialize dwNumTasks to zero so that, in case of errors, we do
**	    not return a bogus value.
**	03-nov-1999 (somsa01)
**	    Put back the RegCloseKey() on HKEY_PERFORMANCE_DATA. Fixes
**	    elswhere have properly fixed 97704.
**	22-Nov-2010 (hweho01)
**	    Add 2nd parameter to the routine, so the buffer limit (dwLimit) 
**	    can be specified by the callers.
*/
static DWORD
GetProcessListNT (PTASK_LIST  pTask, DWORD nsize)
{
LANGID                      lid;        /* language id */
LONG                        lStatus;
char                        szSubKey[1024];
char                        szProcessName[MAX_PATH];
HKEY                        hKeyNames = 0;
DWORD                       dwLimit = nsize;
DWORD                       dwType;
DWORD                       dwSize;
DWORD                       dwProcessIdTitle;
DWORD                       dwProcessIdCounter;
DWORD                       dwNumTasks = 0;
#ifndef WCE_PPC
PPERF_DATA_BLOCK            pPerf;
PPERF_OBJECT_TYPE           pObj;
PPERF_INSTANCE_DEFINITION   pInst;
PPERF_COUNTER_BLOCK         pCounter;
PPERF_COUNTER_DEFINITION    pCounterDef;
char                        *pBuf = NULL;
unsigned int                i;
bool                        BUFFER_SIZE_IS_OK = TRUE; 

    lid = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL );
    sprintf( szSubKey, "%s\\%03x", REGKEY_PERF, lid );
    lStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           szSubKey,
                           0,
                           KEY_READ,
                           &hKeyNames
                          );
    if (lStatus == ERROR_SUCCESS)
    {
        /*
        ** get the buffer size for the counter names
        */
        lStatus = RegQueryValueEx( hKeyNames,
                                   REGSUBKEY_COUNTERS,
                                   NULL,
                                   &dwType,
                                   NULL,
                                   &dwSize
                                 );
    }
    if (lStatus == ERROR_SUCCESS)
    {
        if ((pBuf = calloc(1,dwSize)) != NULL)
        {
            /*
            ** read the counter names from the registry
            */
            lStatus = RegQueryValueEx( hKeyNames,
                                       REGSUBKEY_COUNTERS,
                                       NULL,
                                       &dwType,
                                       pBuf,
                                       &dwSize
                                     );
            if (lStatus == ERROR_SUCCESS)
            {
            char* p;

                /*
                ** loop through counter names looking for counters:
                **
                **      1.  "Process"           process name
                **      2.  "ID Process"        process id
                **
                ** the buffer contains multiple null terminated strings and
                ** then finally null terminated at the end.  The strings
                ** are in pairs of counter number and counter name.
                */

                p = pBuf;
                while (*p)
                {
                    /* Skip the counter number for now, but remember where it is */
                    char *pNumber = p;
                    p += (strlen(p) + 1);
                    if (stricmp(p, PROCESS_COUNTER) == 0)
                    {
                        strcpy( szSubKey, pNumber );
                    }
                    else
                    if (stricmp(p, PROCESSID_COUNTER) == 0)
                    {
                        dwProcessIdTitle = atol( pNumber );
                    }
                    /*
                    ** next string
                    */
                    p += (strlen(p) + 1);
                }
            }
            free (pBuf);
            pBuf = NULL;
        }
    }
    if (lStatus == ERROR_SUCCESS)
    {
        DWORD   dwKeySize = INITIAL_SIZE;
        /*
        ** allocate the initial buffer for the performance data
        */
        dwSize = dwKeySize;
        if ((pBuf = calloc(1,dwSize)) != NULL)
        {
            while (lStatus == ERROR_SUCCESS)
            {
                lStatus = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                                           szSubKey,
                                           NULL,
                                           &dwType,
                                           pBuf,
                                           &dwSize
                                         );

                pPerf = (PPERF_DATA_BLOCK) pBuf;

                /*
                ** check for success and valid perf data block signature
                */
                if ((lStatus == ERROR_SUCCESS) &&
                    (dwSize > 0) &&
                    (pPerf)->Signature[0] == (WCHAR)'P' &&
                    (pPerf)->Signature[1] == (WCHAR)'E' &&
                    (pPerf)->Signature[2] == (WCHAR)'R' &&
                    (pPerf)->Signature[3] == (WCHAR)'F' )
                {
                    break;
                }

                /*
                ** if buffer is not big enough, reallocate and try again
                */
                if (lStatus == ERROR_MORE_DATA)
                {
                    free (pBuf);
                    dwKeySize += EXTEND_SIZE;
                    dwSize = dwKeySize;
		    lStatus = ERROR_SUCCESS;	
                    pBuf = calloc(1,dwSize);
                }
            }
            if (lStatus == ERROR_SUCCESS)
            {
                /*
                ** set the perf_object_type pointer
                */
                pObj = (PPERF_OBJECT_TYPE)((OFFSET_TYPE)pPerf +
					   pPerf->HeaderLength);

                /*
                ** loop thru the performance counter definition records looking
                ** for the process id counter and then save its offset
                */
                pCounterDef = (PPERF_COUNTER_DEFINITION)
                                  ((OFFSET_TYPE)pObj + pObj->HeaderLength);
                for (i=0; i<(DWORD)pObj->NumCounters; i++)
                {
                    if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle) {
                        dwProcessIdCounter = pCounterDef->CounterOffset;
                        break;
                    }
                    pCounterDef++;
                }
                
                dwNumTasks = pObj->NumInstances; 
                if ( dwNumTasks >= dwLimit )
                  BUFFER_SIZE_IS_OK = FALSE; 

                pInst = (PPERF_INSTANCE_DEFINITION)
                                  ((OFFSET_TYPE)pObj + pObj->DefinitionLength);

                /*
                ** loop thru the performance instance data extracting each process name
                ** and process id
                */
                for (i=0; i<dwNumTasks && BUFFER_SIZE_IS_OK; i++)
                {
                char* p;
                    /*
                    ** pointer to the process name
                    */
                    p = (LPSTR) ((OFFSET_TYPE)pInst + pInst->NameOffset);

                    /*
                    ** convert it to ascii
                    */
                    lStatus = WideCharToMultiByte( CP_ACP,
                                                   0,
                                                   (LPCWSTR)p,
                                                   -1,
                                                   szProcessName,
                                                   sizeof(szProcessName),
                                                   NULL,
                                                   NULL
                                                 );

                    if (!lStatus)
                    {
            	        /*
                        ** if we cant convert the string then use a default
                        ** value
                        */
                        strcpy( pTask->ProcessName, UNKNOWN_TASK );
                    }
                    else
                    {
                        if (strlen(szProcessName)+4 <=
                                sizeof(pTask->ProcessName))
                        {
                            strcpy( pTask->ProcessName, szProcessName );
                            strcat( pTask->ProcessName, ".exe" );
                        }
                    }

                    /*
                    ** get the process id
                    */
                    pCounter = (PPERF_COUNTER_BLOCK) ((OFFSET_TYPE)pInst +
                                pInst->ByteLength);
                    pTask->flags = 0;
                    pTask->dwProcessId = *((LPDWORD) ((OFFSET_TYPE)pCounter +
						      dwProcessIdCounter));
                    if (pTask->dwProcessId == 0)
                    {
                        pTask->dwProcessId = (DWORD)-2;
                    }

                    /*
                    ** next process
                    */
                    pTask++;
                    pInst = (PPERF_INSTANCE_DEFINITION)
				((OFFSET_TYPE)pCounter + pCounter->ByteLength);
                }
            }
        }
    }
    if (pBuf)
        free(pBuf);
    if (hKeyNames)
        RegCloseKey( hKeyNames );
    RegCloseKey( HKEY_PERFORMANCE_DATA );
#endif /* WCE_PPC */
    return (dwNumTasks);
}

/*
** Name:    GetProcessList95
**
** Description:
**      Obtains a list of running processes in Windows 95.
**
** Inputs:
**      pTask           array of task list structures.
**
** Outputs:
**      none.
**
** Return:
**      DWORD           dwNumTasks
**      0               if not found
**
** History:
**      27-Oct-97 (fanra01)
**          Created.
*/
static DWORD
GetProcessList95 (PTASK_LIST  pTask, DWORD nsize)
{
HANDLE          hProcessSnap = NULL;
BOOL            bRet         = FALSE;
PROCESSENTRY32  pe32         = {0};
DWORD           dwNumTasks   = 0;

    hProcessSnap = (HANDLE)pfnCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == (HANDLE)-1)
        return (dwNumTasks);

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (pfnProcess32First(hProcessSnap, &pe32))
    {
        do
        {
            pTask->flags = 0;
            pTask->dwProcessId = pe32.th32ProcessID;
            strcpy( pTask->ProcessName, pe32.szExeFile );
            if (pTask->dwProcessId == 0)
            {
                pTask->dwProcessId = (DWORD)-2;
            }
            pTask++;
            dwNumTasks += 1;
            if ( dwNumTasks == nsize )
             break; 
        } while (pfnProcess32Next(hProcessSnap, &pe32));
    }
    CloseHandle (hProcessSnap);
    return (dwNumTasks);
}

/*
** Name:    GetProcessList
**
** Description:
**      Wrapper for one of the NT or 95 routines.
**
** Inputs:
**      pTask           pointer to array of task list structures.
**
** Outputs:
**      pTask           array entries filled in
**
** Return:
**      DWORD           dwNumTasks
**      0               if not found
**
** History:
**	07-May-2008 (whiro01)
**	    Made more common code routines.
**	22-Nov-2010 (hweho01)
**	    Add PCget_AllPidsFromName(), it will be called by  
**	    stop_x100server(). 
*/
static DWORD
GetProcessList(PTASK_LIST  pTask, DWORD nsize )
{
DWORD dwNumProc;

    if (Is_w95 == (int)-1)
    {
	char	VersionString[256];

	GVosvers(VersionString);
	Is_w95 = (strstr(VersionString, "Microsoft Windows 9") != NULL) ?
								TRUE : FALSE;
    }
    if (!Is_w95)
    {
	dwNumProc = GetProcessListNT (pTask, nsize);
    }
    else
    {
	HANDLE hKernel32 = LoadLibrary (TEXT("kernel32.dll"));

	if (hKernel32 != NULL)
	{
	    pfnCreateToolhelp32Snapshot=GetProcAddress (hKernel32,
					TEXT("CreateToolhelp32Snapshot"));
	    pfnProcess32First=GetProcAddress (hKernel32,
					TEXT("Process32First"));
	    pfnProcess32Next=GetProcAddress (hKernel32,
					TEXT("Process32Next"));
	    if (pfnCreateToolhelp32Snapshot && pfnProcess32First &&
		pfnProcess32Next)
	    {
		dwNumProc = GetProcessList95 (pTask, nsize);
	    }
	    FreeLibrary (hKernel32);
	    CloseHandle (hKernel32);
	}
    }
    return dwNumProc;
}

/*
** Name: PCget_AllPidsFromName
**
** Description:
**      Returns all process ids that have the specified process name
**
** Input parameters:
**      pszProcessName  process name used to search the process ids
**      pPidBuf         pointer to buffer that will contain the process ids
**
** Output :
**      process ids filled in the buffer 
**
** Returns:
**      count           the number of process ids. 
**
** History:
**	17-Nov-2010 (hweho01)
**	   Initial version.
*/
DWORD
PCget_AllPidsFromName(void *pszProcessName, DWORD *pPidBuf, DWORD PidBuf_size)
{
    DWORD        dwNumProc, nsize;
    unsigned int i;
    PTASK_LIST   tlist;
    PTASK_LIST   pTask;
    bool         NOT_DONE = TRUE;
    DWORD        count = 0; 	

    nsize = (DWORD)MAX_TASKS;	

    strlwr(pszProcessName);

    tlist = (PTASK_LIST) calloc( nsize, sizeof(TASK_LIST) ); 
    while ( NOT_DONE && tlist != NULL ) 
    {
       dwNumProc = GetProcessList (tlist, nsize);
       /* if list buffer is not big enough, reallocate and try again */
       if ( dwNumProc >= nsize )
       {
        free( tlist );
        nsize +=  MAX_TASKS ;
        tlist = (PTASK_LIST)calloc( nsize, sizeof(TASK_LIST) );
       }
       else
         NOT_DONE = FALSE ;
    }

    if (dwNumProc != 0)
    {
        for ( i=0,pTask = tlist; 
              ( i < dwNumProc && count < PidBuf_size ); i++ )
        {
          if (strstr(strlwr(pTask->ProcessName), pszProcessName)
                != NULL)
            {
             count++;
             *pPidBuf = pTask->dwProcessId; 
             pPidBuf++;
            }
          pTask++; 
        }
    }

  if( tlist != NULL )
    free( tlist ); 

  return count; 
}
/*
** Name: PDirName
**
** Description:
**	Retrieves the path of the process given the process id.
**
** Inputs:
**	PID	Process Id
**
** Outputs:
**	char *	The path to the process with the PID specified in inputs.
**
** Returns:
**	Status 0 SUCCESS 
**         system error code for FAILURE
**
** History:
**	24-Mar-2009 (drivi01)
**	    Added.
*/
DWORD 
PDirName(DWORD pid, char *path){
HANDLE Handle;
char buffer[MAX_PATH];
DWORD dwSize=0, dwRet=0;
char *unknown="Unknown";
int ret_val=1;
HANDLE	hDll = NULL;

	*path='\0';
	dwSize = sizeof(buffer);
	if ((INT_PTR)pfnQueryFullProcessImageName < 0 && ((hDll = LoadLibrary( TEXT("kernel32.dll"))) != NULL))
	{
		pfnQueryFullProcessImageName = (BOOL (FAR WINAPI *)(HANDLE hProcess,
						DWORD dwFlags,
						LPTSTR lpExeName,
						PDWORD lpdwSize))
						GetProcAddress(hDll, TEXT("QueryFullProcessImageNameA"));
	}
	/* This will execute on Vista version of Windows and above */
	if ((INT_PTR)pfnQueryFullProcessImageName > 0)
	{
		Handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
		if (Handle != 0)
			dwRet = (*pfnQueryFullProcessImageName)(Handle, 0, buffer, &dwSize); 
	}
	else /* This will execute on XP, Win 2003 and earlier versions */
	{
		Handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if (Handle != 0) 
			dwRet = GetModuleFileNameEx(Handle, 0,buffer, &dwSize);
	}
	if (Handle != NULL)
		CloseHandle(Handle);
	if (dwRet != 0)
	{
		STcopy(buffer, path);
		ret_val=0;
	}
	else
		ret_val = GetLastError();
	return ret_val;
}
