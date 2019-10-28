/*
**  Copyright (c) 2011 Actian Corporation
**
**  Name: iimgmtsvc.h
**
**  History:
**	Aug-22-11 (drivi01)
**	    Created based on servproc.h.
**	28-Sep-2011 (drivi01)
**	    Rename Management Studio Discovery service
**	    to Actian Remote Manager service.
**	    Rename all associated functions to eliminate
**	    "discovery".	    
*/

BOOL    ReportStatusToSCMgr (DWORD dwCurrentState,
                             DWORD dwWin32ExitCode,
                             DWORD dwCheckPoint,
                             DWORD dwWaitHint);
VOID    WINAPI OpenManagementService (DWORD dwArgc, LPTSTR *lpszArgv);
int     StartManagementServer (BOOL StartIngres);
VOID    WINAPI ManagementCtrlHandler (DWORD);
VOID    ReportManagementEvents (LPTSTR lpszMsg, STATUS status);
VOID    StopService (LPTSTR lpszMsg);
DWORD WINAPI    StopManagement  ( );

FUNC_EXTERN BOOL	GVosvers(char *OSVersionString);
FUNC_EXTERN VOID	GVshobj(char **shPrefix);

SERVICE_STATUS_HANDLE	sshStatusHandle;
HANDLE                  hServDoneEvent = NULL;
SERVICE_STATUS          ssServiceStatus;
LPSERVICE_STATUS        lpssServiceStatus = &ssServiceStatus;
BOOL                    isstarted;
TCHAR			PostSetupNeeded[6];
PROCESS_INFORMATION     piProcInfo;
PROCESS_INFORMATION     piProcInfo2;
STARTUPINFO             siStartInfo;
STARTUPINFO             siStartInfo2;
SECURITY_ATTRIBUTES	sa;
CHAR                    chMsg[256];
TCHAR   		tchII_SYSTEM[ _MAX_PATH];
TCHAR   		tchII_INSTALLATION[3];
TCHAR   		tchII_TEMPORARY[ _MAX_PATH];
DWORD    		dwGlobalErr;
DWORD    		dwGlobalStatus;
STATUS      		dbstatus;
struct SETUID		setuid;
HANDLE			Setuid_Handle;
CHAR			SetuidShmName[64];
struct SETUID_SHM	*SetuidShmHandle;
struct SETUID_SHM	*SetuidShmPtr;
CHAR			RealuidShmName[64];
struct REALUID_SHM	*RealuidShmHandle;
struct REALUID_SHM	*RealuidShmPtr;
DWORD			BytesRead, BytesWritten;
