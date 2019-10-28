# include	<compat.h>
# include	<ssdef.h>
# include	<starlet.h>
#include <signal.h>
/*
**
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	   and missing external function declarations
**      12-aug-2010 (joea)
**          Use VAXC$ESTABLISH to set up exception handlers.
*/

FUNC_EXTERN STATUS CS_setup();
FUNC_EXTERN VOID CS_eradicate();

int CS_lcexh();


/*
**	A new thread comes here when it first starts up.
*/
cs_jmp_start()
{
	VAXC$ESTABLISH(CS_lcexh);

	/* Startup and run the thread. */
	CS_setup();

	/* The thread is exiting.  Delete it from the server. */
	CS_eradicate();

	/* Shouldn't come back to here.  If we do, pause for someone to check. */
	sys$hiber();
}

/*
**	Local exception handler.
*/
CS_lcexh()
{
	return SS$_NORMAL;		/* same value as SS$_CONTINUE */
}
