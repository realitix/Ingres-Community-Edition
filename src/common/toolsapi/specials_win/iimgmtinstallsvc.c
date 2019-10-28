/*
**  Copyright (c) 2011 Ingres Corporation.  All rights reserved.
**
**  Name: iimgmtinstallsvc.c
**
**  Description:
**
**	This program installs Remote Manager service which can be seen 
**	by using 
**		REGEDT32 HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
**
**	USAGE:
**		iimgmtinstallsvc install - to install service as system
**		iimgmtinstallsvc remove - to remove service 
**		iimgmtinstallsvc install -password:<password> - to install as current user
**
**  History:
**	Aug-22-2011 (drivi01)
**	    Created based on opingsvc.c.
**	23-Aug-2011 (drivi01)
**	    Replace sprintf, strcat, strcpy
**	    with CL functions.
**	    Fix a few other minor problems.
**	28-Sep-2011 (drivi01)
**	    Rename discover.exe to iimgmtsvr.exe.
**	    Rename Management Studio discovery service
**	    installer from discversvc.exe to iimgmtinstallsvc.exe.
**	30-Sep-2011 (drivi01)
**	    Update service description to reflect product name.
*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsvc.h>
#include <compat.h>
#include <nm.h>
#include <st.h>
#include <gl.h>
#include <si.h>
#include <er.h>
#include <gl.h>
#include <me.h>
#include <pm.h>

SC_HANDLE	schSCManager;
TCHAR		tchServiceName[ 255 ];
TCHAR		tchOldServiceName[ 255 ];
TCHAR		tchDisplayName[ 255 ];
TCHAR		tchDescription[ 255 ];
TCHAR		tchServiceStartName[ 255 ];
TCHAR		tchServiceInstallName[ 255 ];
CHAR		ServicePassword[255] = "";
CHAR		*ServiceName = "Manager";

void InstallManagementService ();
void RemoveManagementService (VOID);
void PrintUsage		  (char *);
void InitializeGlobals    (char *);


/*
** Name: main
**
** Description:
**      Called to install Remote Manager service for Management Studio.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      None.
**
** History:
**	
*/

VOID
main(int argc, char *argv[])
{
	
    SIprintf( "%s Remote Manager Service Installer. Copyright (c) 2011 ",
		SystemProductName);
    SIprintf("Actian Corporation\n");

    if (argc < 2 || argc > 4) 
    {
	PrintUsage( argv[0] );
    }

    InitializeGlobals( argv[0] );

    if (argc == 3 && stricmp(argv[1], "remove"))
    {
	if ((!stricmp(argv[2], "Manager")))
	{
		STprintf ( tchServiceName, "%s_Service", argv[2] );
		STprintf ( tchDisplayName, "%s Remote %s", SystemProductName, argv[2]);
		STprintf ( tchDescription, "Manages the startup and shutdown of %s Remote Manager service for Management Studio.", SystemProductName);
	}
	else if (STrstrindex(argv[2], "-password:", 0, FALSE))
	    STcopy(STrstrindex(argv[2], ":", 0, FALSE) + 1, ServicePassword);

    }


    if (argc == 2 && !stricmp(argv[1], "remove"))
        RemoveManagementService();
    else if (!stricmp(argv[1], "install"))
	InstallManagementService();
    else
	PrintUsage( argv[0] );
    CloseServiceHandle(schSCManager);
}



/*
** Name: PrintUsage
**
** Description: 
**     Print the usage message.
**
** History:
*/

VOID PrintUsage( char *exe )
{
        SIprintf("usage:\t%s install [ProductName] [client | -password:<ServicePassword>]\n", exe);
        SIprintf("\t\tto install the %s server as a service.\n",
		 SystemProductName);
        SIprintf("\t%s remove <ProductName>\n", exe);
        SIprintf("\t\tto remove the %s server as a service.\n",
		 SystemProductName);
        exit(1);
}


/*
** Name: InstallManagementService.
**
** Description:
**      Called to install Remote Manager Service.
**
** Inputs:
**
** Outputs:
**      None
**
** Returns:
**      None.
**
** History:
*/

VOID
InstallManagementService()
{

	SC_HANDLE	schService;
	LPCTSTR		lpszServiceName;
	LPCTSTR		lpszDisplayName;
	DWORD		fdwDesiredAccess;
	DWORD		fdwServiceType;	
	DWORD		fdwStartType;	
	DWORD		fdwErrorControl; 
	LPCTSTR		lpszBinaryPathName;
	LPCTSTR		lpszLoadOrderGroup;
	LPDWORD		lpdwTagID;
	LPCTSTR		lpzDependencies;
	LPCTSTR		lpszServiceStartName;
	LPCTSTR		lpszPassword;
	TCHAR		tchSysbuf[ _MAX_PATH];
	TCHAR		tchII_SYSTEM[ _MAX_PATH];
	TCHAR		tchII_INSTALLATION[3];
	TCHAR		tchII_TEMPORARY[ _MAX_PATH];
	CHAR		*inst_id;
	CHAR		*temp_loc;
	HKEY		hKey1;
	HKEY		hKey2;
	DWORD		cbData = _MAX_PATH;
	DWORD		dwReserved = 0;
	DWORD		nret;
	DWORD		dwDisposition;
	int		len = 0;
	static char	szUserName[1024] = "";
	HANDLE		hProcess, hAccessToken;
	UCHAR		InfoBuffer[1000], szDomainName[1024], szAccountName[1024];
	PTOKEN_USER	pTokenUser = (PTOKEN_USER)InfoBuffer;
	DWORD		dwInfoBufferSize, dwDomainSize=1024, dwAccountSize=1024;
	SID_NAME_USE	snu;
	DWORD		dwUserNameLen=sizeof(szUserName);
	BOOL 		IsEmbedded = FALSE;
	char    	config_string[256];
	char    	*value, *host;
	SERVICE_DESCRIPTION svcDescription;
	


	/* 
	** Get II_SYSTEM from the environment.
	*/

	GetEnvironmentVariable( SystemLocationVariable, 
				tchII_SYSTEM, _MAX_PATH );
	
        /*
        ** Strip II_SYSTEM of any \'s that may have been tagged onto
        ** the end of it.
        */

        len = strlen (tchII_SYSTEM);

        if (tchII_SYSTEM [len-1] == '\\')
            tchII_SYSTEM [len-1] = '\0';

	/*
	** Get II_INSTALLATION and II_TEMPORARY from the symbol table.
	*/
	NMgtAt("II_INSTALLATION", &inst_id);
	STcopy(inst_id, tchII_INSTALLATION);
	NMgtAt("II_TEMPORARY", &temp_loc);
	STcopy(temp_loc, tchII_TEMPORARY);
	len = STlength (tchII_TEMPORARY);
	if (tchII_TEMPORARY [len-1] == '\\')
	    tchII_TEMPORARY [len-1] = '\0';

	/*
	** Append the installation identifier to the service name.
	*/
	STprintf(tchServiceName, "%s_%s", tchServiceName, tchII_INSTALLATION);
	STprintf(tchDisplayName, "%s [%s]", tchDisplayName,
		 tchII_INSTALLATION);
	if (*ServicePassword == '\0')
	     STcopy("LocalSystem", szUserName);
	else
	{

	/*
	** Get the domain name that this user is logged on to. This could
	** also be the local machine name.
	*/
	hProcess = GetCurrentProcess();
	OpenProcessToken(hProcess,TOKEN_READ,&hAccessToken);
	GetTokenInformation(hAccessToken, TokenUser, InfoBuffer, 1000,
			    &dwInfoBufferSize);
	LookupAccountSid(NULL, pTokenUser->User.Sid, szAccountName,
			 &dwAccountSize, szDomainName, &dwDomainSize, &snu);
	STcopy(szDomainName, szUserName);
	STcat(szUserName, "\\");
	STcat(szUserName, szAccountName);

	}

	/* 
	** Connect to service control manager on the local machine and 
	** open the ServicesActive database.
	*/

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

	
	if ( schSCManager == NULL )
	{
            switch (GetLastError ())
            {
                case ERROR_ACCESS_DENIED:
                    SIprintf ("You must have administrator privileges to "\
                        "install or remove a service.\n");
                    return;

                default:
			SIprintf("%s: Failed to connect to the Service Control ",
				tchServiceInstallName);
			SIprintf("manager.\n\t OpenSCManager (%d)\n", GetLastError());
			return;
            }
	
	}

	nret = RegCreateKeyEx(
			HKEY_CLASSES_ROOT,
			tchServiceName,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS,
			NULL,
			&hKey1,
			&dwDisposition);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf("%s: Cannot open/create Registry Key: %d\n",
					tchServiceInstallName, nret);
	    return;
	}

	nret = RegCreateKeyEx (
		hKey1,
		"shell", 
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,
		NULL,	
		&hKey2,
		&dwDisposition);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf("%s: Cannot open/create Registry Key: %d\n",
					tchServiceInstallName, nret);
	    return;
	}

	cbData = strlen(tchII_SYSTEM) + 1;

	nret = RegSetValueEx(
		hKey2,
		"II_SYSTEM",
		0,
		REG_SZ,
		tchII_SYSTEM,
		cbData);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf( "Cannot create Registry value for %s: %d\n",
		      SystemLocationVariable, nret);
	    return;
	}

	cbData = STlength(tchII_INSTALLATION) + 1;

	nret = RegSetValueEx(
		hKey2,
		"II_INSTALLATION",
		0,
		REG_SZ,
		tchII_INSTALLATION,
		cbData);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf("Cannot create Registry value for II_INSTALLATION: %d\n",
			nret);
	    return;
	}

	cbData = STlength(tchII_TEMPORARY) + 1;

	nret = RegSetValueEx(
		hKey2,
		"II_TEMPORARY",
		0,
		REG_SZ,
		tchII_TEMPORARY,
		cbData);

	if (nret != ERROR_SUCCESS)
	{
	    SIprintf("Cannot create Registry value for %s_TEMPORARY: %d\n",
			SystemVarPrefix, nret);
	    return;
	}

	STprintf(tchSysbuf, "\"%s\\ingres\\bin\\iimgmtsvc.exe\"", tchII_SYSTEM);

	lpszServiceName  = tchServiceName;
	lpszDisplayName  = tchDisplayName;
	fdwDesiredAccess = SERVICE_START | SERVICE_STOP |
				SERVICE_USER_DEFINED_CONTROL | 
				SERVICE_CHANGE_CONFIG;
	fdwServiceType   = SERVICE_WIN32_OWN_PROCESS;
	fdwStartType     = SERVICE_AUTO_START;
	fdwErrorControl  = SERVICE_ERROR_NORMAL;
	lpszBinaryPathName = tchSysbuf;
	lpszLoadOrderGroup = NULL;
	lpdwTagID = NULL;
	lpzDependencies = NULL;
	lpszServiceStartName = szUserName;
	lpszPassword = ServicePassword;



	/* 
	** regedt32 can check this on 
	**	HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services
	*/

	schService = OpenService(schSCManager, 
				lpszServiceName, 
				SERVICE_ALL_ACCESS);

	if (schService != NULL)
        {
	   SIprintf("%s: The service \"%s\" already exists.\n", 
				tchServiceInstallName, lpszServiceName);
	   CloseServiceHandle(schService);
	   return;
	}

	/* 
	** Only create the service if you know that it doesn't already exist.
	*/
	schService = CreateService(     
		schSCManager,
		lpszServiceName,
		lpszDisplayName,
		fdwDesiredAccess,
		fdwServiceType,
		fdwStartType,
		fdwErrorControl,
		lpszBinaryPathName,
		lpszLoadOrderGroup,
		lpdwTagID,
		lpzDependencies,
		lpszServiceStartName,
		lpszPassword);
	
	if ( schService == NULL )
	{
	    SIprintf( "Failed to create the %s Service.\n",
		      lpszServiceName );
	    SIprintf( "\t CreateService (%d)\n", GetLastError() );
	    return;
	}

	svcDescription.lpDescription = tchDescription;
	if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_DESCRIPTION, 
		&svcDescription))
	{
		SIprintf("Failed to update Description of the Ingres Service");
		SIprintf("\n\t Failed with system error (%d)\n", GetLastError());
		return;
	}


        STprintf( tchSysbuf,
            "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s",
            lpszServiceName );
        if ((nret = RegCreateKeyEx( HKEY_LOCAL_MACHINE, tchSysbuf, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey1,
            &dwDisposition )) != ERROR_SUCCESS)
        {
	    SIprintf( "Failed to create the %s key for EventLog messages.\n",
		      lpszServiceName );
	    SIprintf( "\t RegCreateKeyEx (%d)\n", GetLastError() );
	    return;
        }
        if (dwDisposition == REG_CREATED_NEW_KEY)
        {
            STprintf( tchSysbuf, "%s\\ingres\\bin\\iimgmtsvc.exe", tchII_SYSTEM );
            if ((nret = RegSetValueEx( hKey1, TEXT("EventMessageFile"), 0,
                REG_EXPAND_SZ, (CONST BYTE *)tchSysbuf,
                STlength(tchSysbuf) + 1 )) != ERROR_SUCCESS)
            {
                SIprintf( "Failed to set EventMessageFile value.\n" );
                SIprintf( "\t RegSetValueEx (%d)\n", GetLastError() );
                RegCloseKey( hKey1 );
                return;
            }
            else
            {
                DWORD   evt_types = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
                    EVENTLOG_INFORMATION_TYPE;
                if ((nret = RegSetValueEx( hKey1, TEXT("TypesSupported"), 0,
                    REG_DWORD, (CONST BYTE *)&evt_types,
                    sizeof(DWORD) )) != ERROR_SUCCESS)
                {
                    SIprintf( "Failed to set TypesSupported value.\n" );
                    SIprintf( "\t RegSetValueEx (%d)\n", GetLastError() );
                    RegCloseKey( hKey1 );
                    return;
                }
            }

            RegCloseKey( hKey1 );
        }

	SIprintf( "The path to the %s service program ",
		  SystemProductName );
	SIprintf("\"iimgmtsvc.exe\" \nbased upon the Registry is:\n\n");
	SIprintf("\t   %s\n\n",tchSysbuf);
	SIprintf("To view the value stored in the Microsoft Windows NT ");
	SIprintf( "registry\nfor %s, use the Registry Editor ",
		  SystemLocationVariable );
	SIprintf("\"REGEDT32.EXE\" using\n");
	SIprintf("the key \"HKEY_CLASSES_ROOT on Local Machine\" and ");
	SIprintf("the subkeys\n");
	SIprintf("\"%s\" and \"shell\".\n\n", lpszServiceName);
#if !defined (INGRES4UNI)
	if (STcompare(ServicePassword, "") == 0)
	{
	SIprintf("Before using the service you must set the password for the ");
	SIprintf("\"%s\"\naccount in the Service Startup dialog.\n\n",
		 szUserName);
	}
        SIprintf("WARNING: If you choose to start the %s Database ",
		 SystemProductName);
        SIprintf("Service \nautomatically, you must make certain that the ");
        SIprintf("Password Never \nExpires option is specified in the User ");
        SIprintf("Manager for the \"%s\"\nuser.\n", szUserName);
#endif
	SIprintf("\n%s Service installed ", lpszDisplayName);
	SIprintf("successfully.\n");
	CloseServiceHandle(schService);
}


/*
** Name: RemoveManagementService
**
** Description:
**      Remove Mangement Studio Management service on Windows.
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      None.
**
** History:
*/

VOID
RemoveManagementService()
{
	BOOL    	ret;
	SC_HANDLE	schService, schDepService;
	SC_HANDLE	schSCManager;
	LPTSTR		lpszMachineName; 
	LPCTSTR		lpszServiceName;
	LPCTSTR		lpszOldServiceName;
	LPCTSTR		lpszDisplayName;
	LPTSTR		lpszDatabaseName;
	DWORD		fdwDesiredAccess;
	STATUS		errnum;
	CHAR		*inst_id;
	TCHAR		tchII_INSTALLATION[3];
	TCHAR		tchSysbuf[ _MAX_PATH];
        HKEY            pkey = NULL;
        HKEY            skey = NULL;
	SC_HANDLE	schOldService;
	BOOL		bSame;

	/*
	** Append the installation identifier to the service name.
	*/
	NMgtAt("II_INSTALLATION", &inst_id);
	STcopy(inst_id, tchII_INSTALLATION);
	STprintf(tchServiceName, "%s_%s", tchServiceName, tchII_INSTALLATION);
	STprintf(tchDisplayName, "%s [%s]", tchDisplayName,
		 tchII_INSTALLATION);

	lpszMachineName  = NULL;		/* Local machine	*/
	lpszDatabaseName = NULL;		/* ServicesActive 	*/
	fdwDesiredAccess = SC_MANAGER_CREATE_SERVICE;

	schSCManager = OpenSCManager( lpszMachineName,
				      lpszDatabaseName,
				      fdwDesiredAccess);

	if ( schSCManager == NULL)
	{
	    SIprintf("%s: OpenSCManager (%d)\n", tchServiceInstallName, 
					GetLastError());
	    return;
	}


	lpszServiceName = tchServiceName;
	schService = OpenService(schSCManager, 
				lpszServiceName, 
				SERVICE_ALL_ACCESS);

	if (schService == NULL)
	{
	    errnum = GetLastError();
	    switch (errnum)
	    {
	 	case ERROR_SERVICE_DOES_NOT_EXIST:
		    SIprintf ("The service \"%s\" does not exist.\n", 
					lpszServiceName);
		    break;

		case ERROR_ACCESS_DENIED:
		    SIprintf ("The service control manager does not have ");
		    SIprintf ("access to the service %s.\n", lpszServiceName);
		    break;

		default:
	            SIprintf("Failed to remove service %s.\n",lpszServiceName);
		    break;
	    }

	    CloseServiceHandle(schSCManager);
	    return;
	}

        STprintf( tchSysbuf,
            "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application" );
        errnum = RegOpenKeyEx( HKEY_LOCAL_MACHINE, tchSysbuf, 0, KEY_QUERY_VALUE, &skey );
        if (errnum != ERROR_SUCCESS)
        {
            SIprintf("%s: RegOpenKeyEx (%d)\n", tchServiceInstallName,
                errnum);
        }
        else
        {
            pkey = skey;
            if (RegOpenKeyEx( pkey, tchServiceName, 0, KEY_WRITE, &skey )
                == ERROR_SUCCESS)
            {
                errnum = RegDeleteKey( pkey, tchServiceName );
                if (errnum != ERROR_SUCCESS)
                {
                    SIprintf("%s: RegDeleteKey (%d)\n", tchServiceInstallName,
                        errnum);
                }
            }
        }

	ret = DeleteService(schService);
	CloseServiceHandle(schService);

	lpszDisplayName = tchDisplayName;
	if (ret)
	    SIprintf("%s service removed.\n", lpszDisplayName);
	else
	    SIprintf("DeleteService failed with error code (%d)\n", 
				GetLastError());

	CloseServiceHandle(schSCManager);
}

/*
** Name: InitializeGlobals
**
** Description:
**      Initialize global names
**
** Inputs:
**      None
**
** Outputs:
**      None
**
** Returns:
**      None.
**
** Side effects:
**	global variables are initialized
**
** History:
*/
void
InitializeGlobals( char *exename )
{
	STprintf( tchServiceName, "%s_Service", ServiceName);
	STprintf( tchOldServiceName, "%s_Service", ServiceName);

        STprintf( tchDisplayName, "%s Remote %s", SystemProductName,
		     ServiceName );

	STprintf( tchServiceStartName, ".\\%s", SystemAdminUser );

	STprintf( tchServiceInstallName, exename );
	
	STprintf( tchDescription, "Manages the startup and shutdown of Remote Manager service for Management Studio.");
}
