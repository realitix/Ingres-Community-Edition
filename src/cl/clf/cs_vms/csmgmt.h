/*
** Copyright (c) 1987, 1992 Actian Corporation
*/

/**
** Name:	csmgmt.h	- shared between CSMO files and clients.
**
** Description:
**	Information shared between CSMO related files, like
**	attach/detach function externs and class def array locations.
**
** History:
**	Make _classes GLOBALCONSTREFs for VMS sharability.
**	13-Jul-1995 (dougb)
**		Add additional classes as per Unix version of this file.
**	12-Nov-2010 (kschendel) SIR 124685
**	    Prototype / include fixes.
**/

/* data */

GLOBALCONSTREF MO_CLASS_DEF CS_scb_classes[];
GLOBALCONSTREF MO_CLASS_DEF CS_sem_classes[];
GLOBALCONSTREF MO_CLASS_DEF CS_cnd_classes[];
GLOBALCONSTREF MO_CLASS_DEF CS_mon_classes[];
GLOBALCONSTREF MO_CLASS_DEF CS_int_classes[];

/* utilities */

FUNC_EXTERN STATUS CS_sem_attach( CS_SEMAPHORE *sem );
FUNC_EXTERN STATUS CS_detach_sem( CS_SEMAPHORE *sem );

FUNC_EXTERN void CS_scb_attach( CS_SCB *sem );
FUNC_EXTERN void CS_detach_scb( CS_SCB *sem );

FUNC_EXTERN STATUS CS_get_block( char *instance, SPTREE *tree, PTR *block );
FUNC_EXTERN STATUS CS_nxt_block( char *instance, SPTREE *tree, PTR *block );


