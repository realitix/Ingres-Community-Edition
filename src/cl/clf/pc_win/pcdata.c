/*
** Copyright (c) 1999 Actian Corporation
*/

/*
**  pcdata.c -  PC Data
**
**  Description
**      File added to hold all GLOBALDEFs in PC facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**
**  History:
**      09-nov-1999 (somsa01)
**          Created.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
*/
#include    <compat.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <qu.h>

GLOBALDEF QUEUE		Pidq;
GLOBALDEF bool		Pidq_init = FALSE;
GLOBALDEF CS_SYNCH	Pidq_mutex ZERO_FILL;

