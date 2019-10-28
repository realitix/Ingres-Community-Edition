/*
**  Copyright (c) 2011 Actian Corporation
*/

/*
**  Name: iimgmtsvc.c
**
**  Description:
**
**	Used by the Windows Service Manager to start <Product Name> Remote Manager 
**	service using iimgmtsvr.exe start and to stop Remote Manager
**	service using iimgmtsvr.exe stop.
**
**	The service runs as a daemon and starts automatically on Windows
**	startup.  It uses LocalSystem account or installation user
**	depending on configurations chosen at install time.
**
**  History:
**	08-22-2011 (drivi01)
**	    Created based on servproc.c code.
**	23-Aug-2011 (drivi01)
**	    Replace sprintf, strcat, strcpy
**	    with CL functions.
**	    Fix a few other minor problems.
**	28-Sep-2011 (drivi01)
**	    Rename discover.exe to iimgmtsvr.exe.
**	    Rename Management Studio discovery service
**	    installer from discversvc.exe to iimgmtinstallsvc.exe.
**	    Rename the service to <Product Name> Remote Manager and
**	    all references of "discovery" to "management".
**	    
*/

/*
**
NEEDLIBS = COMPATLIB 
**
*/
#define _WIN32_WINNT 0x0500
#define xDEBUG 1
#include <compat.h>
#include <iicommon.h>
#include <clusapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsvc.h>
#include <cs.h>
#include <st.h>
#include <me.h>
#include <nm.h>
#include <gl.h>
#include <pc.h>
#include <cv.h>
#include <cm.h>
#include "iimgmtsvc.h"
#include <ingmsg.h>



# define SERVPROC_LOG   "II_IIMGMTSVC_LOG" //define in symbol table to debug

void InitializeGlobals(void);
static void GetRegValue(char *strKey, char *strValue, BOOL CloseKey);
# ifdef xDEBUG
void ServProcTrace( char* fmt, ... );
# define SERVPROCTRACE  ServProcTrace
# else
# define SERVPROCTRACE( x, y )
# endif

TCHAR tchServiceName[255] = "Remote_Manager";

char	*ObjectPrefix;
CRITICAL_SECTION critStopServer;

/*
** Name: CtrlHandlerRoutine
**
** Description:
**      Console control handler function.  This function is called by the
**      OS to signal the following events:
**          CTRL_C_EVENT
**          CTRL_BREAK_EVENT
**          CTRL_CLOSE_EVENT
**          CTRL_LOGOFF_EVENT
**          CTRL_SHUTDOWN_EVENT
**
** Inputs:
**      dwCtrlType      Event from the OS
**
** Outputs:
**      None
**
** Return:
**      TRUE            Event handled in the handler routine
**      FALSE           Event not handled and should be passed to next handler
**
** History:
*/
BOOL
CtrlHandlerRoutine(DWORD dwCtrlType)
{
    DWORD StopType = 0;
    BOOL status = FALSE;
    switch(dwCtrlType)
    {
        case CTRL_CLOSE_EVENT:
            /*
            ** If a true is returned for this event and the service
            ** interacts with the desktop then a dialog to end the
            ** application is displayed.
            */
            status = TRUE;
            break;

        case CTRL_SHUTDOWN_EVENT:
            SERVPROCTRACE( "%s\n", "iimgmtsvc - CtrlHandlerRoutine shutdown" );
            /*
            ** StopType is now a set of bit flags for enabling ingstop
            ** command options.
            */
            status = StopManagement() == OK ? FALSE : TRUE;
            break;
    }
    /*
    ** If the StopManagement was successful return false to indicate
    ** default handler action otherwise return true to indicate
    ** event has been handled.
    */
    return(status);
}

/*
** Name: main
**
** Description:
**	Called when the Ingres service is started by the Service
**	Manager.  Report error in the EventLog if it fails to start.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**	None.
**
** History:
**	05-Oct-2011 (drivi01)
**	   Set environment variable JAVA_HOME to II_MTS_JAVA_HOME
**	   from symbol.tbl.
*/

VOID	
main(int argc, char *argv[])
{
	TCHAR	tchFailureMessage[ 256];
	TCHAR	tchJAVA_HOME[MAX_PATH];
	char	*BinaryName, *cptr;
	SERVICE_TABLE_ENTRY	lpServerServiceTable[] = 
	{
	    { tchServiceName, (LPSERVICE_MAIN_FUNCTION) OpenManagementService},
	    { NULL, NULL }
	};
	SERVPROCTRACE( "iimgmtsvc - In main \n");
	GVshobj(&ObjectPrefix);

	InitializeCriticalSection(&critStopServer);

	SetConsoleCtrlHandler( (PHANDLER_ROUTINE)&CtrlHandlerRoutine, TRUE );

	/*
	** Let's get II_SYSTEM from the service executable path which
	** is passed in as an argument via the SCM. This is so that we
	** can run NMgtAt() on II_INSTALLATION.
	*/
	BinaryName = (char *)MEreqmem(0, STlength(argv[0])+1, TRUE, NULL);
	STcopy(argv[0], BinaryName);
	cptr = STstrindex(BinaryName, "\\ingres\\bin\\iimgmtsvc.exe", 0, TRUE);
	*cptr = '\0';
	STcopy(BinaryName, tchII_SYSTEM);
	SetEnvironmentVariable(SYSTEM_LOCATION_VARIABLE, tchII_SYSTEM);

	NMgtAt("II_INSTALLATION", &cptr);

	if (cptr != NULL && *cptr != EOS)
	    STcopy(cptr, tchII_INSTALLATION);
	else
	    GetRegValue("installationcode", tchII_INSTALLATION, FALSE);
	
	/*
	** Let's retrieve II_MTS_JAVA_HOME from symbol.tbl
	** Set environment variable JAVA_HOME to the II_MTS_JAVA_HOME.
	*/
	NMgtAt("II_MTS_JAVA_HOME", &cptr);
	if (cptr != NULL && *cptr !=EOS)
	{
	    STcopy(cptr, tchJAVA_HOME);
	    SetEnvironmentVariable("JAVA_HOME", tchJAVA_HOME);
	    SERVPROCTRACE( "iimgmtsvc - In main JAVA_HOME=%s\n", tchJAVA_HOME);
	}

        STprintf(tchServiceName, "%s_%s", tchServiceName, tchII_INSTALLATION);
	SERVPROCTRACE( "iimgmtsvc - In main tchServiceName=%s\n", tchServiceName);
	/*
	** If StartServiceCtrlDispatcher succeeds, it connects the calling 
	** thread to the service control manager and does not return until all 
	** running services in the process have terminated. 
	*/
	if (!StartServiceCtrlDispatcher( lpServerServiceTable ))
	{
	    LPVOID lpMsgBuf;
	    FormatMessage(
  		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
  		NULL,	                    /* no module handle */
  		GetLastError(),
		MAKELANGID(0, SUBLANG_ENGLISH_US),
  		(LPSTR)&lpMsgBuf,
		0,			    /* minimum size of memory to allocate*/
  		NULL );

	    STprintf( tchFailureMessage, "Failed to start %s Server. %s", 
			SYSTEM_SERVICE_NAME, (LPCTSTR) lpMsgBuf);

	    LocalFree( lpMsgBuf );          /* Free the buffer. */
	    StopService(tchFailureMessage);
	}

	DeleteCriticalSection(&critStopServer);
	return;
}


/*
** Name: OpenManagementService	
**
** Description:
**	Register the control handling function and start the service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
*/
VOID WINAPI
OpenManagementService(DWORD dwArgc, LPTSTR *lpszArgv)
{
	DWORD		dwWFS;
	DWORD		dwTimeoutms = INFINITE;  /*timeout in milliseconds*/
	DWORD 		dwStatus;


	SERVPROCTRACE( "%s\n", "iimgmtsvc - OpenManagementService start" );
	/* 
	** Register control handling function with the control dispatcher.
	*/
	sshStatusHandle = RegisterServiceCtrlHandler(
					tchServiceName,
					(LPHANDLER_FUNCTION) ManagementCtrlHandler);

	if ( !sshStatusHandle )
		goto cleanup;

	ssServiceStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
	ssServiceStatus.dwServiceSpecificExitCode = 0;
        SERVPROCTRACE( "%s\n", "iimgmtsvc - OpenManagementService service start pending" );
	/* 
	** Update status of SCM to SERVICE_START_PENDING	
	*/
	if (!ReportStatusToSCMgr(
		SERVICE_START_PENDING,
		NO_ERROR,
		2,
		300000))
		goto cleanup;

	/* 
	** Create an event that will handle termination.	
	*/
	hServDoneEvent = CreateEvent( NULL, TRUE, FALSE, NULL);

	if ( hServDoneEvent == NULL )
	    goto cleanup;
        SERVPROCTRACE( "%s\n", "iimgmtsvc - OpenManagementService service StartManagementServer" );	
        dbstatus = StartManagementServer(TRUE);

	if (!dbstatus)
		goto cleanup;
    
	/* 
	** Update status of SCM to SERVICE_RUNNING		
	*/
	if (!ReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR, 0, 0))
	    goto cleanup;

	/* 
	** Wait for terminating event or timeout period.
	*/
	while ((dwWFS = WaitForSingleObject( hServDoneEvent, dwTimeoutms )) == WAIT_TIMEOUT)
	{
	    /* Do not check for servers unless current state is "running". */
	    if (ssServiceStatus.dwCurrentState != SERVICE_RUNNING)
	    	continue;

	    /* If critical section is not available, continue waiting. */
	    if (! TryEnterCriticalSection(&critStopServer))
	    	continue;

	    /* Check to see that all the started servers are still running. */
	    ReportStatusToSCMgr(SERVICE_RUNNING, NO_ERROR, 1, 10000);
            dwStatus = StopManagement();
	    if (dwStatus == OK)
	    {
		LeaveCriticalSection(&critStopServer);
	    	continue;            /* continue waiting for another period */
	    }

	    /*
	    ** At this point, some server has ended or is not responding;
	    ** the Ingres installation should be shut down.
	    */
	    ReportManagementEvents ("Ingres installation failed. Please refer to the error log for further information.", FAIL);
	    
	    if (dwGlobalErr == 0)
	    	dwGlobalErr = dwStatus;
            SERVPROCTRACE( "%s\n", "iimgmtsvc - OpenManagementService shutdown" );
	    ReportStatusToSCMgr(SERVICE_STOP_PENDING, dwGlobalErr, 1, 300000);
            dwStatus = StopManagement();
	    if (dwStatus != OK)
	    { 
		ReportManagementEvents ("Failed to stop Ingres installation. Please refer to the error log for further information.", FAIL);
		ReportStatusToSCMgr (SERVICE_RUNNING, dwGlobalErr, 0, 0);
	    }
	    LeaveCriticalSection(&critStopServer);
	} /* while WaitForSingleObject */

cleanup:
	if (hServDoneEvent != NULL)
	{	
	    CloseHandle(hServDoneEvent);
	    hServDoneEvent = NULL;
	}
	if (sshStatusHandle != 0)
		(VOID)ReportStatusToSCMgr( SERVICE_STOPPED, dwGlobalErr, 0, 0);
	return;
}


/*
** Name: ManagementCtrlHandler
**
** Description:
**     Handles control requests for OpenIngres Service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
*/

VOID WINAPI
ManagementCtrlHandler(DWORD dwCtrlCode)
{
	DWORD	dwState = SERVICE_RUNNING;
	DWORD	nret = 0;
	DWORD	dwThreadId;
	HANDLE  hThread;

	switch(dwCtrlCode) 
	{
	    case SERVICE_CONTROL_PAUSE:
		break;

	    case SERVICE_CONTROL_CONTINUE:
		break;

	    case SERVICE_CONTROL_SHUTDOWN:
	    case SERVICE_CONTROL_STOP:
            SERVPROCTRACE( "iimgmtsvc - ManagementCtrlHandler %s\n",
                (dwCtrlCode == SERVICE_CONTROL_SHUTDOWN) ? "SERVICE_CONTROL_SHUTDOWN" : 
                    "SERVICE_CONTROL_STOP" );

		ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 1, 300000);
		hThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)StopManagement, 0,
					0, &dwThreadId);
		if (hThread == NULL)
		{ ReportManagementEvents ("Failed to stop Ingres installation. Please refer to the error log for further information.", FAIL);
		  ReportStatusToSCMgr (SERVICE_RUNNING, nret, 0, 0);
		}
		else
		    CloseHandle(hThread);
		return;

	    case SERVICE_CONTROL_INTERROGATE:
		break;

	    default:
		break;
	}
	ReportStatusToSCMgr (dwState, NO_ERROR, 0, 0);
}


/*
** Name: StartManagementServer
**
** Description:
**	Runs ingstart.exe which starts all the ingres processes 
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
*/

int
StartManagementServer(BOOL StartManagement)
{
    LPTSTR	tchBuf, tchBuf2;
    CHAR	temp = '\0';
    DWORD	nret;
    DWORD	dwWait, dwExitCode = 0;
    int     	len = 0;
    static CHAR	SetuidPipeName[32];
    char	*cptr;
    SERVPROCTRACE( "%s\n", "iimgmtsvc - StartManagementServer start" );
    SetuidShmHandle = NULL;
    Setuid_Handle = NULL;

    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.lpReserved = NULL;
    siStartInfo.lpDesktop = NULL;
    siStartInfo.lpTitle = NULL;
    siStartInfo.lpReserved2 = NULL;
    siStartInfo.cbReserved2 = 0;
    siStartInfo.dwFlags = 0;

    nret = GetEnvironmentVariable("PATH", &temp, 1);
    if (nret > 0)
    {
	tchBuf = malloc(nret);
	GetEnvironmentVariable("PATH", tchBuf, nret);
	nret += MAX_PATH*4;  /* This is to make room for the add-ons later. */
    }
    else
    {
	nret = MAX_PATH*4;
	tchBuf = malloc(nret);  /* Just make room for the add-ons. */
	GetSystemDirectory(tchBuf, nret);  /* Also add the System path. */
    }
    tchBuf2 = malloc(nret);

    NMgtAt("II_TEMPORARY", &cptr);

    if (cptr != NULL && cptr != EOS)
	STcopy(cptr, tchII_TEMPORARY);
    else
    {
	GetRegValue("II_TEMPORARY", tchII_TEMPORARY, TRUE);

	if (tchII_TEMPORARY[0] == '\0')
	{
	    STprintf(tchII_TEMPORARY, "%s\\%s\\temp", tchII_SYSTEM,
		     SYSTEM_LOCATION_SUBDIRECTORY);
	}
    }
	

    SetEnvironmentVariable("II_TEMPORARY", tchII_TEMPORARY);

    /* 
    ** Strip II_SYSTEM of any \'s that may have been tagged onto 
    ** the end of it.					     
    */

    len = (i4)strlen (tchII_SYSTEM);

    if (tchII_SYSTEM [len-1] == '\\')
	tchII_SYSTEM [len-1] = '\0';

    /* 
    ** Append "%II_SYSTEM%\inges\{bin, utility};" to the beginning of
    ** the path and set the PATH environment variable. This is applicable
    ** only to this process.
    */

    STpolycat(5, tchII_SYSTEM, "\\", SYSTEM_LOCATION_SUBDIRECTORY, "\\", "utility;", tchBuf2);
    STpolycat(5, tchII_SYSTEM, "\\", SYSTEM_LOCATION_SUBDIRECTORY, "\\", "bin;", tchBuf);
    STcat(tchBuf2, tchBuf);

    nret = SetEnvironmentVariable(SYSTEM_LOCATION_VARIABLE, tchII_SYSTEM);
    nret = SetEnvironmentVariable("PATH", tchBuf2);
    free(tchBuf);
    free(tchBuf2);

    /*
    ** Create the shared memory segment needed for running setuid
    ** programs, and the named pipe to communicate with client
    ** programs.
    */
    iimksec(&sa);
    STprintf(SetuidShmName, "%s%sSetuidShm", ObjectPrefix, tchII_INSTALLATION);

    if ((SetuidShmHandle = CreateFileMapping(INVALID_HANDLE_VALUE,
					     &sa,
					     PAGE_READWRITE,
					     0,
					     sizeof(struct SETUID_SHM),
					     SetuidShmName)) == NULL)
    {
	dwGlobalStatus = 0;
	ReportManagementEvents("A shared memory segment which allows the Management service to perform certain tasks was unable to be created. These tasks will be disabled.", FAIL);
    }
    else
    {
	if ((SetuidShmPtr = MapViewOfFile(SetuidShmHandle,
					  FILE_MAP_WRITE | FILE_MAP_READ,
					  0,
					  0,
					  sizeof(struct SETUID_SHM))) == NULL)
	{
	    CloseHandle(SetuidShmHandle);
	    dwGlobalStatus = 0;
	    ReportManagementEvents("A shared memory segment which allows the Managementservice to perform certain tasks was unable to be created. These tasks will be disabled.", FAIL);
	}
	else
	{
	    SetuidShmPtr->pending = FALSE;

	    STprintf(SetuidPipeName, "\\\\.\\PIPE\\MANAGEMENT\\%s\\SETUID",
		     tchII_INSTALLATION);
	    if ((Setuid_Handle = CreateNamedPipe(SetuidPipeName,
						PIPE_ACCESS_DUPLEX,
						0,
						PIPE_UNLIMITED_INSTANCES,
						sizeof(struct SETUID),
						sizeof(struct SETUID),
						0,
						&sa)) == INVALID_HANDLE_VALUE)
	    {
		UnmapViewOfFile(SetuidShmPtr);
		CloseHandle(SetuidShmHandle);
		dwGlobalStatus = 0;
		ReportManagementEvents("A pipe which allows the management service to perform certain tasks was unable to be created. These tasks will be disabled.", FAIL);
	    }
	}
    }

    if (StartManagement)
    {
	char cmdbuf[MAX_PATH*2];
	STprintf(cmdbuf, "%s\\%s\\bin\\iimgmtsvr.exe start", tchII_SYSTEM,
		SYSTEM_LOCATION_SUBDIRECTORY);
        SERVPROCTRACE( "%s %s\n", "iimgmtsvc - cmd", cmdbuf );


	isstarted = CreateProcess(
		NULL,
		cmdbuf,	
		NULL,		
		NULL,		
		TRUE,		
		HIGH_PRIORITY_CLASS,		
		NULL,	
		NULL,		
		&siStartInfo,
		&piProcInfo);

        SERVPROCTRACE( "%s %d procID = %d\n", "iimgmtsvc - StartManagementServer CreateProc result", isstarted,  piProcInfo.hProcess);
	if (!isstarted)
		goto cleanup;

	dwWait = WaitForSingleObject( piProcInfo.hProcess, INFINITE);
    SERVPROCTRACE( "%s\n", "iimgmtsvc - StartManagementServer done waiting");

    GetExitCodeProcess ( piProcInfo.hProcess, &dwExitCode);
    SERVPROCTRACE( "%s Exit Code: %d\n", "iimgmtsvc - StartManagementServer done waiting", dwExitCode);
	CloseHandle(piProcInfo.hProcess);
	CloseHandle(piProcInfo.hThread);


	if (dwExitCode)
	    goto cleanup;
    }
    else
	isstarted = 1;
	
    return isstarted;

cleanup:
    if (dwExitCode)
    {
	ReportManagementEvents ("Failed to start Management service. Please refer to the error log for further information.", FAIL);
    }
	
    if (hServDoneEvent != NULL)
    {	
	CloseHandle(hServDoneEvent);
	hServDoneEvent = NULL;
    }

    if (sshStatusHandle != 0)
	(VOID)ReportStatusToSCMgr(
			SERVICE_STOPPED,
			dwGlobalErr,
			0,
			0);
    return 0;
}



/*
** Name: StopManagement
**
** Description:
**      Issue the ingstop command to stop the OpenIngres server.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
*/

DWORD WINAPI
StopManagement()
{
    char	cmdline [MAX_PATH*2];
    i4		checkpoint = 2;
    DWORD	dwExitStatus = 0, dwRet = 0;
    DWORD 	dwCurrentState = SERVICE_STOP_PENDING;
    HANDLE hProcess = NULL;

    EnterCriticalSection(&critStopServer);
    SERVPROCTRACE( "iimgmtsvc - StopManagement entry\n");

	
    STprintf(cmdline, "\"%s\\ingres\\bin\\iimgmtsvr.exe\" stop",
		tchII_SYSTEM);
    SERVPROCTRACE( "iimgmtsvc - StopServer cmd %s\n", cmdline ); 
	
    isstarted = CreateProcess(NULL,
				cmdline,
				NULL,
				NULL,
				TRUE,
				HIGH_PRIORITY_CLASS,
				NULL,
				NULL,
				&siStartInfo,
				&piProcInfo2);

    SERVPROCTRACE( "iimgmtsvc - StopServer CreateProcess %d\n", isstarted );
    /* Report status to service control manager every 2 seconds (2000 ms) */
    while(WaitForSingleObject( piProcInfo2.hProcess, 2000) == WAIT_TIMEOUT)
    {
         ReportStatusToSCMgr( dwCurrentState,
                NO_ERROR,
                checkpoint,
                2000);
    }


    GetExitCodeProcess (piProcInfo2.hProcess, &dwExitStatus);
    SERVPROCTRACE( "iimgmtsvc - StopServer GetExitCodeProcess %d\n", dwExitStatus );
    CloseHandle(piProcInfo2.hProcess);
    CloseHandle(piProcInfo2.hThread);

    if (dwExitStatus)
    {
	ReportManagementEvents ("Failed to stop Management service. Please refer to the error log for further information.", FAIL);
	ReportStatusToSCMgr (SERVICE_RUNNING, dwExitStatus, 0, 0);
	LeaveCriticalSection(&critStopServer);
	return dwExitStatus;
    }

 			        

    /*
    ** Free the setuid shared memory, and close out the setuid pipe
    ** handle.
    */
    SERVPROCTRACE( "iimgmtsvc - StopManagement Terminated\n");
    UnmapViewOfFile(SetuidShmPtr);
    if (SetuidShmHandle)
    	CloseHandle(SetuidShmHandle);
    if (Setuid_Handle)
    	CloseHandle(Setuid_Handle);

    SetEvent(hServDoneEvent);

    LeaveCriticalSection(&critStopServer);
    return(dwExitStatus);
}



/*
** Name: ReportManagementEvents
**
** Description:
**      Write events to the event log.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** History:
**
*/

VOID
ReportManagementEvents(LPTSTR lpszMsg, STATUS status)
{
	HANDLE	hEventSource;
	LPTSTR	lpszStrings[2];
	CHAR	chMsg[256];

	dwGlobalErr = (status == FAIL) ? GetLastError() : 0;

	hEventSource = RegisterEventSource( NULL, tchServiceName );

	STprintf( chMsg, "%s error: %d %d", 
		 SYSTEM_SERVICE_NAME, dwGlobalErr, dwGlobalStatus );

	lpszStrings[0] = chMsg;
	lpszStrings[1] = lpszMsg;

	if (hEventSource != NULL) 
	{
            if (status == FAIL)
            {
                ReportEvent(
                        hEventSource,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        E_ING_FAILED,
                        NULL,
                        2,
                        0,
                        lpszStrings,
                        NULL);
            }
            else
            {
                ReportEvent(
                        hEventSource,
                        EVENTLOG_INFORMATION_TYPE,
                        0,
                        E_ING_SUCCEDED,
                        NULL,
                        1,
                        0,
                        &lpszStrings[1],
                        NULL);
            }
            DeregisterEventSource(hEventSource);
        }
	return;
}


/*
** Name: StopService
**
** Description:
**      Stop the service.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
*/

VOID
StopService(LPTSTR lpszMsg)
{
	HANDLE	hEventSource;
	LPTSTR	lpszStrings[2];

	dwGlobalErr = GetLastError();

	hEventSource = RegisterEventSource( NULL, tchServiceName );

	STprintf(chMsg, "%s error: %d", SYSTEM_SERVICE_NAME, dwGlobalErr);
	lpszStrings[0] = chMsg;
	lpszStrings[1] = lpszMsg;

	if (hEventSource != NULL) 
	{
		ReportEvent(hEventSource, 
			EVENTLOG_ERROR_TYPE,
			0,
			0,
			NULL,
			2,
			0,
			lpszStrings,
			NULL);
		(VOID) DeregisterEventSource(hEventSource);
	}
	SetEvent(hServDoneEvent);
}


/*
** Name: ReportStatusToSCMgr
**
** Description:
**     Report the status to the service control manager.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      OK
**      !OK
**
** History:
*/

BOOL
ReportStatusToSCMgr(DWORD dwCurrentState,
		DWORD dwWin32ExitCode,
		DWORD dwCheckPoint,
		DWORD dwWaitHint)

{
	BOOL fResult;

	EnterCriticalSection(&critStopServer);
	if (dwCurrentState == SERVICE_START_PENDING) 
	    lpssServiceStatus->dwControlsAccepted = 0;
	else
	    lpssServiceStatus->dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;

	ssServiceStatus.dwCurrentState  = dwCurrentState;
	ssServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
	ssServiceStatus.dwCheckPoint    = dwCheckPoint;
	ssServiceStatus.dwWaitHint      = dwWaitHint;

	if (!(fResult = SetServiceStatus( sshStatusHandle, lpssServiceStatus)))
	{
	    StopService("SetServiceStatus");
	}
	
	LeaveCriticalSection(&critStopServer);
	return fResult;
}

static void
GetRegValue(char *strKey, char *strValue, BOOL CloseKey)
{
    HKEY		hKey;
    static HKEY		SoftwareKey = NULL;
    int			ret_code = 0, i;
    TCHAR		KeyName[MAX_PATH + 1];
    TCHAR		tchValue[MAX_PATH];
    unsigned long	dwSize;
    DWORD		dwType;

    if (!SoftwareKey)
    {
	/*
	** We have to figure out the right registry software key that
	** matches the install path.
	*/
	if(!RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		     "Software\\IngresCorporation\\Ingres",
		     0, KEY_ALL_ACCESS, &hKey))
	{
		for (i = 0; ret_code != ERROR_NO_MORE_ITEMS; i++)
		{
			ret_code = RegEnumKey(hKey, i, KeyName, sizeof(KeyName));
			if (!ret_code)
			{
				RegOpenKeyEx(hKey, KeyName, 0, KEY_ALL_ACCESS, &SoftwareKey);
				dwSize = sizeof(tchValue);
				RegQueryValueEx(SoftwareKey, SYSTEM_LOCATION_VARIABLE,
						NULL, &dwType, (unsigned char *)&tchValue,
						&dwSize);

				if (!strcmp(tchValue, tchII_SYSTEM))
					break;
				else
					RegCloseKey(SoftwareKey);
			}
		}
	}
	RegCloseKey(hKey);
    }

    /*
    ** Now that we know where to look, let's get the value that we need
    ** from the registry.
    */
    dwSize = sizeof(tchValue);
    if (RegQueryValueEx(SoftwareKey, strKey, NULL, &dwType,
			(unsigned char *)&tchValue, &dwSize) == ERROR_SUCCESS)
    {
	STcopy(tchValue, strValue);
    }
    else
	STcopy("", strValue);

    if (CloseKey)
    {
	RegCloseKey(SoftwareKey);
	SoftwareKey = NULL;
    }
}


# ifdef xDEBUG
static char logfilename[MAX_PATH + 1] ZERO_FILL;    /* path to log file */
static bool loginit = FALSE;                        /* initialization flag */
/*
** Name: ServProcTrace
**
** Description:
**      Function to log messages to a file.
**      Used during shutdown processing when the eventlog service is shutdown
**      before us.
**
** Inputs:
**      fmt         Formating character string.  See printf.
**      ...         variable argument list
**
** Outputs:
**      None
**
** Return:
**      None
**
** History:
**      27-Jan-2003 (fanra01)
**          Created.
*/
void
ServProcTrace( char* fmt, ... )
{
    char    lbuf[MAX_PATH];
    char    *log = NULL;
    FILE* fd = NULL;
    va_list p;

    if (loginit == FALSE)
    {
        NMgtAt( SERVPROC_LOG, &log );
        if (log && *log)
        {
            STcopy( log, logfilename );
            loginit = TRUE;
        }
    }
    /*
    ** If initialized and able to open file then write a log message.
    ** Flush the message and close the file just in case OS decides to
    ** terminate this process.
    */
    if ((loginit == TRUE) && ((fd=fopen( logfilename, "a+")) != NULL))
    {
        va_start( p, fmt );
	vfprintf(fd, "Logging:", p);
        STprintf(lbuf, "%08d %s", GetCurrentThreadId(), fmt);
        vfprintf( fd, lbuf, p );
        fflush( fd );
        fclose( fd );
    } 
    return;
}
# endif
