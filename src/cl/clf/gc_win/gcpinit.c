/*
** Copyright (c) 1987, 2010 Actian Corporation
*/

#include <compat.h>
#include <iicommon.h>
#include <er.h>
#include <cs.h>
#include <me.h>
#include <nm.h>
#include <st.h>
#include <qu.h>
#include <pc.h>
#include <lo.h>
#include <pm.h>
#include <tr.h>
#include <gcccl.h>
#include <gc.h>
#include "gclocal.h"
#include "gcclocal.h"

/* Debug definitions */
GLOBALREF i4 GCCL_trace;

# define GCTRACE( n )  if ( GCCL_trace >= n )(void)TRdisplay


/* Global variable definitions */
extern		VOID		GCc_callback_driver();
extern		STATUS		GCc_prot_shutdown();
GLOBALREF 	GCC_PCT		IIGCc_gcc_pct;
GLOBALREF	VOID		(*GCevent_func[])();

GLOBALREF	THREAD_EVENT_LIST IIGCc_proto_threads;
GLOBALREF	CRITICAL_SECTION  GccCompleteQCritSect;

# ifdef MCT
GLOBALREF CS_SEMAPHORE	ME_stream_sem;
# endif /* MCT */

/* Forward and/or External function references */
VOID GCpinit_common();
FUNC_EXTERN STATUS GCloadprotocol (GCC_PCT* pPct);
FUNC_EXTERN STATUS GCunloadprotocol (GCC_PCT* pPct);

/* type_definitions */

/* Definition of static variables and forward static functions.  */


/******************************************************************************
** Name: GCpinit    - Protocol initialization routine
**
** Description:
**      This routine is called during the communication server initialization
**      process.  It is guaranteed to be called before any protocol driver
**      functions are invoked.  It is responsible for allocating and
**      initializing the Protocol Control Table (PCT), and for performing any
**      other initialization which may be required in a specific environment.
**
**      The PCT is described in detail in the header file GCCCL.H.  It has an
**      entry for each supported protocol implementation.  Entries normally,
**      although not necessarily, correspond one-for-one to the collection of
**  protocol handler routines.
**
** Inputs:
**      none
**
** Outputs:
**      pct             Pointer to an allocated and filled-in
**                  Protocol Control Table (PCT).
**  generic_status          System-independent status (output)
**  system_status           Environment-specific status (output)
**
**  Returns:
**      STATUS:
**      OK
**      FAIL
**  Exceptions:
**      none
**
** Side Effects:
**      none
** History:
**	??-???-?? (????)
**	28-oct-93 (edg)
**	   Put in tracing.  Look for II_GCCL_TRACE or !.gcc_trace_level
**	   in PM file and then set static variable GCCL_trace to the
**	   appropriate level.  A Macro, GCTRACE(i4) will call TRdisplay
**	   for tracing.
**	03-nov-93 (edg)
**	   Changed so that it now uses the IIGCc_gcc_pct global to get
**	   the protocols that will be used.
**	19-nov-93 (Edg)
**	   Set the GCC completion function, GCc_callback_driver, into
**	   GCevent_func for GCexec to call when net requests complete.
**      15-jul-95 (emmag)
**          Use a NULL Discretionary Access Control List (DACL) for
**          security, to give implicit access to everyone.
**      12-Sep-95 (fanra01)
**          Extracted data for DLLs on Windows NT.
**	09-may-1996 (canor01)
**	    On shutdown, execute any registered protocol exit routines.
**	28-mar-1997 (canor01)
**	    On shutdown, execute all exit routines, regardless of 
**	    return status from any one of them.
**      19-May-97 (fanra01)
**          Add calls to load and unload protocol driver DLLs.
**      06-aug-1999 (mcgem01)
**          Replace nat and longnat with i4.
**      18-Oct-2002 (fanra01)
**          Bug 106589
**          Change initialization flag to MEreqmem to ensure memory is
**          clean.
**      27-Jan-2004
**          SIR 111718
**          Add GCportaddr function.
**	20-feb-2004 (somsa01)
**	    In GCpaddrlist(), updated to account for multiple listen ports
**	    per protocol.
**  08-Mar-2004 (fanra01)
**      Moved GCpportaddr from this source file.
**      Removed GCpaddrlist
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      28-Aug-2006 (lunbr01) Sir 116548
**          As part of new IPv6 support, initialize pce_dcb to point to
**          the WS driver's THREAD_EVENTS structure, which is really a DCB.
**          Then we won't have to scan for matching THREAD_EVENTS everywhere.   
**      06-Aug-2009 (Bruce Lunsford) Sir 122426
**          Convert GCC completion queue mutex to a critical section
**          to improve performance (less overhead).
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Add support for net drivers like "tcp_ip" as local IPC.
**	    The driver init and exit routines were merged into the WS_DRIVER
**	    structure pointed to by the PCT, rather than maintained as
**	    separate init and exit tables.  Change access logic accordingly.
**	    Add external prototype defns for GCloadprotocol and GCunloadprotocol
**	    to eliminate compiler warnings.
**	    Split out some of the code to new function GCpinit_common()
**	    which is executed even if a full PCT is not needed, such as when
**	    called by gcacl in non-GCx server environment and II_GC_PROT is set.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
******************************************************************************/

/******************************************************************************
**
******************************************************************************/
STATUS
GCpinit(pct, generic_status, system_status)
GCC_PCT     **pct;
STATUS      *generic_status;
CL_ERR_DESC *system_status;
{
	STATUS  status = OK;
	i4 i;

	GCpinit_common();

	GCTRACE(4)("GCpinit: Start ....\n");

	/*
	** If called with a NULL PCT pointer, then the
	** remainder of the logic can be skipped.
	** This is used by gcacl to do minimal initialization.
	*/
	if (pct == NULL)
	    return (status);

	/*
	** Set the GCexec GCC shutdown function.
	*/
	GCevent_func[SHUTDOWN] = GCc_prot_shutdown;

        /*
        **  Load DLL protocol drivers and extend the protocol control table
        */
	GCTRACE(4)("GCpinit: loading protocol DLLs ...\n");
        if ((status = GCloadprotocol (&IIGCc_gcc_pct)) != OK)
        {
            GCTRACE(4)("GCloadprotocol failed status = %d\n", status);
            status = OK;
        }

	/*
	** Count actual number of protocols, they can be terminated teminated
	** by a "" entry.
	*/
	GCTRACE(4)("GCpinit: Getting Protocol entries ...\n");
	for ( i = 0; i < IIGCc_gcc_pct.pct_n_entries; i++ )
	{
	    if ( !IIGCc_gcc_pct.pct_pce[i].pce_pid[0] )
	        break;
	}

	GCTRACE(2)("GCpinit: %d Protocol entries ...\n", i );

	IIGCc_gcc_pct.pct_n_entries = i;

	/*
	** Allocate space for protocol threads in the event list
	*/
	GCTRACE(4)("GCpinit: allocate thread event struct ...\n");
	IIGCc_proto_threads.no_threads = i;
# ifdef MCT
	gen_Psem(&ME_stream_sem);
# endif /* MCT */
	IIGCc_proto_threads.thread = (THREAD_EVENTS *)MEreqmem( 0, 
						sizeof(THREAD_EVENTS) * i,
				     	        TRUE, NULL );
# ifdef MCT
	gen_Vsem(&ME_stream_sem);
# endif /* MCT */
        if ( IIGCc_proto_threads.thread == NULL )
	{
	    DeleteCriticalSection( &GccCompleteQCritSect );
	    status = FAIL;
	    goto end_pinit;
	}

	/*
	** Call any specific protocol driver initialization routines,
	** if any.
	*/
	GCTRACE(4)("GCpinit: Calling protocol init . . .\n");
	for ( i = 0; i < IIGCc_gcc_pct.pct_n_entries; i++ )
	{
	    GCC_PCE *pce = &IIGCc_gcc_pct.pct_pce[i];

	    /*
	    ** Init queues for threads.
	    */
	    QUinit( &IIGCc_proto_threads.thread[i].incoming_head );  
	    QUinit( &IIGCc_proto_threads.thread[i].process_head );  

	    /*
	    ** Assign the thread name.
	    */
	    IIGCc_proto_threads.thread[i].thread_name = 
	    	pce->pce_pid;

	    /*
	    ** Point the PCE's pce_dcb (driver control block) to the
	    ** corresponding thread event entry.  This makes it
	    ** easier to find THREAD_EVENTS entry from parmlist on
	    ** all driver requests.  Can still scan on pce_pid if
	    ** needed (old drivers will continue to do this).
	    */
	    pce->pce_dcb = (PTR)&IIGCc_proto_threads.thread[i];

	    if ( ((WS_DRIVER *)pce->pce_driver)->init_func != NULL )
	        status = (*((WS_DRIVER *)pce->pce_driver)->init_func)( pce );
	    if ( status != OK )
	    {
		DeleteCriticalSection( &GccCompleteQCritSect );
	        goto end_pinit;
	    }
	}
	GCTRACE(4)("GCpinit: Ending . . .\n");

end_pinit:
        *pct = &IIGCc_gcc_pct;
	return (status);
}

/******************************************************************************
** Name: GCpinit_common - Protocol initialization routine common code
**
** Description:
**      This routine is called by GCpinit() during comm server initialization
**	and/or by gcacl initialization if II_GC_PROT was used to select a
**	network driver for the local IPC.  This routine will return
**	immediately if it has previously been called.  This ensures that
**	it doesn't reinitialize objects that are already active and thus
**	can be called by anyone in any order.
**
** Inputs:
**      none
**
** Outputs:
**      Initialized global objects required for network drivers
**
** Returns:
**      none
**
** Exceptions:
**      none
**
** Side Effects:
**      none
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Extracted from GCpinit() function.
******************************************************************************/
VOID
GCpinit_common()
{
	char *trace;
	/*
	** Make sure logic is only done once (1st time in).
	*/
	static bool GCpinit_common_called = FALSE;
	if (GCpinit_common_called == TRUE)
	    return;
	GCpinit_common_called = TRUE;

	NMgtAt( "II_GCCL_TRACE", &trace );
	if ( !(trace && *trace) && PMget( "!.gccl_trace_level", &trace ) != OK )
	{
	    GCCL_trace = 0;
	}
	else
	{
	    GCCL_trace = atoi( trace );
	}

	GCTRACE(4)("GCpinit_common: Start ....\n");

	/*
	** Set the GCexec/GCsync GCC callback function.
	*/
	GCevent_func[GCC_COMPLETE] = GCc_callback_driver;

	/*
	** Create the Mutex for the GCC completion Q.
	*/
	GCTRACE(4)("GCpinit: Initializing Completion Q Critical Section ...\n");
	InitializeCriticalSection( &GccCompleteQCritSect );

	/*
	** Init completed queue
	*/
	QUinit( &IIGCc_proto_threads.completed_head );
}


/******************************************************************************
** Name: GCc_prot_shutdown - Protocol shutdown routine
**
** Description:
**      This routine is set as the GCevent_func() for the SHUTDOWN event by
**	GCinitiate (GCA CL initialization).  The SHUTDOWN event is signalled
**	with SetEvent() by GCshut() when a shutdown of GCC has been requested
**	and (unless forced) no connections exist.  When the event is signalled,
**      this routine is called by GCexec() or any other function that waits
**	on the SHUTDOWN event.
**	This routine calls the exit function for each protocol driver in the
**	PCT.
**
** Inputs:
**      none
**
** Outputs:
**      Initialized global objects required for network drivers
**
** Returns:
**      STATUS:
**	    OK
**	    status of (first) failing protocol driver routine
**
** Exceptions:
**      none
**
** Side Effects:
**      All protocol drivers inactivated and resources freed.
**
** History:
**	13-Apr-2010 (Bruce Lunsford) Sir 122679
**	    Added function header documentation comment box.
**	    The driver init and exit routines were merged into the WS_DRIVER
**	    structure pointed to by the PCT, rather than maintained as
**	    separate init and exit tables.  Change access to the exit 
**	    routine accordingly.
**	    Only check status when exit function called; otherwise could
**	    erroneously think an error occurred if uninit'd status has garbage.
**	    Add some tracing.
******************************************************************************/
STATUS
GCc_prot_shutdown()
{
	int i;
	STATUS status;
	STATUS ret = OK;

	GCTRACE(4)("GCc_prot_shutdown: Start .... \n");

        for ( i = 0; i < IIGCc_gcc_pct.pct_n_entries; i++ )
        {
	    GCC_PCE *pce = &IIGCc_gcc_pct.pct_pce[i];
	    if ( ((WS_DRIVER *)pce->pce_driver)->exit_func != NULL )
	    {
		GCTRACE(4)("GCc_prot_shutdown: Calling Protocol %s exit function\n", pce->pce_pid);
	        status = (*((WS_DRIVER *)pce->pce_driver)->exit_func)( pce );
        	if ( status != OK )
		{
		    GCTRACE(1)("GCc_prot_shutdown: Protocol %s exit function returned error status %d\n", pce->pce_pid, status);
		    ret = status;
		}
	    }
        }
        GCunloadprotocol (&IIGCc_gcc_pct);
	return ( ret );
}
