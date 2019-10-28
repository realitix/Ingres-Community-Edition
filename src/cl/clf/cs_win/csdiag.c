/*
**Copyright (c) 2004 Actian Corporation
*/
#include <compat.h>
#include <systypes.h>
#include <iicommon.h>
#include <ex.h>
#include <pc.h>
#include <cs.h>
#include <csinternal.h>
#include <evset.h>


GLOBALREF CS_SEMAPHORE  Cs_known_list_sem;

/**
**  Name: csdiag - CS diagnostic routines
**
**  Description:
**      This module provides access to diagnostic information from the CS
**      CL group. This is called by the analysis code and makes extensive
**      use of the DIAG routines for locating symbols and providing access
**      to areas of store. This function needs to be ported for each
**	platform.
**
**      The following routines are supplied:-
**
**      CSdiagDumpSched    - Dump scheduler queues
**      CSdiagFindCurrent  - Find SCB of current thread
**
**  History:
**      01-Feb-2010 (horda03) Bug 121811
**          Port to NT from unix.
**	11-Nov-2010 (kschendel) SIR 124685
**	    Delete CS_SYSTEM ref, is in csinternal.
**	    Fix confusion about how output-fcn is called.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
*/

/*{
**  Name: CSdiagDumpSched - Dump scheduler queue
**
**  Description:
**      This routine emulates the "show sessions" code of iimonitor but runs
**      against a core file rather than directly from memory
**      - for each thread a stack trace is also produced
**
**      It works by finding the CS server block which is pointed at by the
**      symbol Cs_srv_block and then dumps each of the priority queues
**      displaying the state of each thread. The DIAGDumpStack routine is
**      called to produce a stack trace for each
**
**  Inputs:
**      VOID (*output)(char *format,...)
**          Routine to call with output
**      VOID (*error)(char *format,...)
**          Routine to call with errors
**
**  Exceptions:
**      Errors in the DIAG routines used result in the DIAGerror() routine
**      being called
**
**  History:
**	06-sep-1996 (canor01)
**	    Call CSget_scb() to correctly get the scb of the current thread
**	    for use with OS Threads.
**      22-Jul-2008 (hanal04) Bug 120532
**          CS_dump_stack() does not take a file name parameter. Also make
**          calls verbose for II_EXCEPTION as function names make life a
**          lot easier than just PC addresses.
**      01-Feb-2010 (horda03)
**          Port to NT. Added exception handler.
**      12-Jan-2011 (horda03) b124923
**          Only attempt to output queries if the server supports them.
*/
STATUS
CSdiag_handler( EX_ARGS *exargs )
{
   return (EXDECLARE);
}

/* First, a helper function.
** The DIAG routines supply a printf-like function.  CS_dump_stack
** expects a TR_OUTPUT_FCN, which is entirely different.
** This helper is a TR_OUTPUT_FCN which calls the DIAG-style output.
** The arg has to be typed PTR, but it's in fact a DIAG_LINK *
** where the CS_dump_stack has stashed the proper DIAG-style routine,
** in the ->output member.
*/

static STATUS diag_output_helper(PTR diaglink, i4 length, char *buffer)
{
    DIAG_LINK *dl = (DIAG_LINK *)diaglink;

    (*dl->output)("%.*s",length,buffer);
    return (OK);
}


static void
CSdiagDumpSched(output,error)
VOID (*output)();
VOID (*error)();
{
    CS_SCB 	*scb;
    char 	*what;
    EX_CONTEXT  excontext;


    CSp_semaphore( TRUE, &Cs_known_list_sem );

    if (EXdeclare( CSdiag_handler, &excontext) )
    {
       (*output)("EXception in CSdiagDumpSched" );
    }
    else
    {
       scb = Cs_srv_block.cs_known_list;
       scb = scb->cs_next;

       while (scb != Cs_srv_block.cs_known_list)
       {
           switch (scb->cs_state)
           {
               case CS_FREE:
                   (*output)("0x%p %t CS_FREE\n", scb->cs_self, sizeof(scb->cs_username), scb->cs_username);
                   break;
               case CS_COMPUTABLE:
                   (*output)("0x%p %t CS_COMPUTABLE\n",scb->cs_self,sizeof(scb->cs_username), scb->cs_username);
                   break;
               case CS_STACK_WAIT:
                   (*output)("0x%p %t CS_STACK_WAIT\n",scb->cs_self,sizeof(scb->cs_username), scb->cs_username);
                   break;
               case CS_UWAIT:
                   (*output)("0x%p %t CS_UWAIT\n",scb->cs_self, sizeof(scb->cs_username), scb->cs_username);
                   break;
               case CS_EVENT_WAIT:
                   if (scb->cs_memory & CS_DIO_MASK)
                       what="DIO";
                   else if (scb->cs_memory & CS_BIO_MASK)
                       what="BIO";
                   else if (scb->cs_memory & CS_LOCK_MASK)
                       what="LOCK";
                   else
                       what="LOG";
                   (*output)("0x%p %t CS_EVENT_WAIT(%s)\n",scb->cs_self, sizeof(scb->cs_username), 
                             scb->cs_username,what);
                   break;
               case CS_MUTEX:
                   (*output)("0x%p %t CS_MUTEX(%x)\n",scb->cs_self,
                             sizeof(scb->cs_username), scb->cs_username,scb->cs_memory);
                   break;
               case CS_CNDWAIT:
                   (*output)("0x%p %t CS_CNDWAIT(%x)\n",scb->cs_self,
                             sizeof(scb->cs_username), scb->cs_username,scb->cs_memory);
                   break;
               default:
                   (*output)("0x%p %t CS_????\n",scb->cs_self,
                             sizeof(scb->cs_username), scb->cs_username);
                   break;
           }

           /* Dump the stack frame for this thread - watch out for the current */
           /* thread the registers in the scb shouldn't be used for this       */

           (*output)
	   ("-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  \n");

	   /* read symbols from the object file */

	    {
		DIAG_LINK diag_link;
		CS_SCB *current_scb;
		CS_SCB *dump_scb;

		diag_link.output = output;
		dump_scb = NULL;
		if (Cs_srv_block.cs_state != CS_INITIALIZING)
		{
		    CSget_scb( &current_scb );
		    if (current_scb != scb)
			dump_scb = scb;
		}
		CS_dump_stack(dump_scb,0,(PTR)&diag_link,diag_output_helper,TRUE);
	    }

           /* Only display query where they could exist */
           if (Cs_srv_block.cs_state != CS_INITIALIZING)
           {
              (*output)
	         ("-  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  \n");
	      /* Print out query for each session */
	      {
	          DIAG_LINK diag_link;
	          diag_link.type = SCF_CURR_QUERY;
	          diag_link.output = output;
	          diag_link.error = error;
	          diag_link.filename = NULL;
	          diag_link.scd = scb;
	          (*Cs_srv_block.cs_diag)((VOID *)&diag_link);
	      }
           }
           (*output)
	   ("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

           scb=scb->cs_next;
       }
    }

    EXdelete();
    CSv_semaphore( &Cs_known_list_sem );
}
/**
**  Name: CSdiag_server_link - CS diagnostic routines
**
**  Description:
**	This function is a link between ex and scs. It was written
**	for compile reasons. cs cannot be called directly from
**	ex so we define a global in EX and assign the address of this
**	function to it. This is done in Ex_print_stack as well.
**
**      The following routines are supplied:-
** Inputs:
**	type 		function type to call
**	output		Output function writes to file.
**	error		Error function writes to file. 
**	filename	If we need to overload TRdisplay in
**			generic function we use this filename.
**
** Returns:
**      0    -- Process does not supports user queries
**      1    -- Process supports user queries.
**
**  History:
**      13-Mch-1996 (prida01)
**	    Created.	
**	06-sep-1996 (canor01)
**	    Replace use of Cs_srv_block.cs_current with call to CSget_scb()
**	    for use with OS Threads.
**      12-Jan-2011 (horda03) b124923
**          Only use cs_diag if the server supports queries.
**          Changed return type.
*/
i4
CSdiag_server_link(type,output,error,filename)
i4  	type;
VOID	(*output)();
VOID	(*error)();
PTR	filename;
{
   DIAG_LINK	diag_link;
   i4           ret = (Cs_srv_block.cs_state == CS_INITIALIZING) ? 0 : 1;

   diag_link.output = output;
   if (type == DIAG_DUMP_STACKS)
   {
       (*output)
          ("======================= STACK DUMP ==========================\n");
       CS_dump_stack(NULL, NULL, (PTR) &diag_link, diag_output_helper, FALSE);
       (*output)
          ("====================== SCHEDULER QUEUE ======================\n");
       CSdiagDumpSched(error,output);
   }
   else if (ret)
   {
      diag_link.type = type;
      diag_link.error = error;
      diag_link.filename = filename;
      CSget_scb( (CS_SCB **)&diag_link.scd );
      (*Cs_srv_block.cs_diag)((VOID *)&diag_link);
   }

   return ret;
}
