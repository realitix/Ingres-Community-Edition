/*
**Copyright (c) 2011 Actian Corporation
*/

/* 
 * vwsysinfo - Retrieves memory and cpu information of the machine
 *
 * History:
 *		05-May-2011 (drivi01)
 *			Created.
 *
 */


#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define KBYTE 1024
#define MBYTE 1048576
#define	GBYTE 1073741824

void
printUsage()
{
	printf("Usage: vwsysinfo memory [unit] | cpu \r\n\r\n");
	printf("Where Options are:\r\n");
	printf("\tmemory - prints total physical memory on the machine.\r\n");
	printf("           Unit, if not specified prints memory in bytes.\r\n");
	printf("           Otherwise prints memory in unit specified.\r\n");
	printf("           Units can be M for Mb, G for Gb, and K for Kb.\r\n");
	printf("\tcpu - prints number of cpus on the machine\r\n");

}

int
main(int argc, char **argv[])
{
	int bMemory = FALSE;
	int bCpu = FALSE;

	if (stricmp(argv[1], "memory") == 0 && (argc == 2 || argc == 3 ))
		bMemory = TRUE;
	else if ((stricmp(argv[1], "cpu") == 0) && argc == 2)
		bCpu = TRUE;
	else
		printUsage();
	

	
	if (bMemory)
	{
		    MEMORYSTATUSEX memstat;
		    memstat.dwLength = sizeof(memstat);
		    if (GlobalMemoryStatusEx(&memstat))
		    {
				if (argc == 2)
				_tprintf(TEXT("%I64d bytes"),memstat.ullTotalPhys);
				else if (argc == 3)
				{
					double mem = 0;
					char unit[2];
					sprintf(unit, "%c\0", *argv[2]);
					if (*unit == 'K' || *unit == 'k')
						mem = (memstat.ullTotalPhys)/(double)KBYTE;
					else if (*unit == 'M' || *unit == 'm')
						mem = (memstat.ullTotalPhys)/(double)MBYTE;
					else if (*unit == 'G' || *unit == 'g')
						mem = (memstat.ullTotalPhys)/(double)GBYTE;
					if (mem)
						_tprintf("%.0f", mem);
				}
		    }
		    else
				printf("Error: Couldn't retrieve memory due to error %d", GetLastError());
	}

    if (bCpu)
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		if (sysinfo.dwNumberOfProcessors)
			_tprintf(TEXT("%u"), sysinfo.dwNumberOfProcessors);
		else
			printf("Error: Couldn't retrieve number of CPUs due to error %d", GetLastError());		
	} 


	return 0;

}


