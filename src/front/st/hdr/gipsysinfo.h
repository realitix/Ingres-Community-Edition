/*
** Copyright 2011 Actian Corporation. All rights reserved.
*/

/*
** Name: gipsysconf.h
**
** Description:
**	Header file for system parameter management module gipsysconf.c
**
**	10-Feb-2011 (hanje04)
**	    Mantis 1303, 1348 & 1434
**	    Created.
*/

/* Public protypes */
STATUS gipInitSysInfo(bool update);
STATUS gipGetSysPMem(SIZE_TYPE *syspmemkb);
STATUS gipGetSysFMem(SIZE_TYPE *sysfmemkb);
STATUS gipGetSysCpu(i4 *cpus);
STATUS gipGetSysShmmax(SIZE_TYPE *shmmax);
STATUS gipSetSysShmmax(SIZE_TYPE shmmax);
STATUS gipCheckMemReq(void);
