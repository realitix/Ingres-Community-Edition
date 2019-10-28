/*
**      EX Data
**
**  Description
**      File added to hold all GLOBALDEFs in EX facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**	25-jun-1997 (canor01)
**	    Added EX_print_stack.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**
*/
# include    <compat.h>
# include   <iicommon.h>
# include   <cs.h>
# include   <me.h>
# include   <windef.h>

GLOBALDEF CRITICAL_SECTION SemaphoreInitCriticalSection ZERO_FILL;
GLOBALDEF CRITICAL_SECTION ConditionCriticalSection ZERO_FILL;
GLOBALDEF VOID (*Ex_print_stack)() ZERO_FILL; /* NULL or CS_dump_stack */
