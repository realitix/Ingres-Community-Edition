/*
**    Copyright (c) 1991, 2008 Actian Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include	<er.h>
#include	<gcccl.h>
#include	<gcblsio.h>
#include	<gcblpgb.h>
#include	<cm.h>
#include    	<st.h>
#include    	<me.h>
#include	<nm.h>
#include	<tr.h>
#include	<pm.h>
#include	<cv.h>
#include	<gcatrace.h>

/* BLAST Protocol Generator */

FUNC_EXTERN     VOID    pgini();    	    /* initialise protocol generator */
FUNC_EXTERN     VOID    pglgn();    	    /* logon */
FUNC_EXTERN	VOID	pgcnt();	    /* connect */
FUNC_EXTERN     VOID    pgsnd();    	    /* send */
FUNC_EXTERN     VOID    pgrd();	    	    /* read */
FUNC_EXTERN     VOID    pglgo();    	    /* logout */
FUNC_EXTERN 	int 	pgpnd();    	    /* pend */

/* Serial I/O Driver functions */

FUNC_EXTERN     int     sioinit();  	    /* initialise serial i/o */
FUNC_EXTERN 	VOID	sioreg();    	    /* register callback */
FUNC_EXTERN     VOID    siodone();  	    /* terminate serial i/o */
FUNC_EXTERN 	VOID	siowrite();  	    /* complete serial i/o write */
FUNC_EXTERN 	VOID	sioread();  	    /* complete serial i/o read */
FUNC_EXTERN 	VOID	siotimer(); 	    /* complete timer expiration */

/* Serial communications functions */

FUNC_EXTERN     int     cominit();
FUNC_EXTERN     int     comst();
FUNC_EXTERN     VOID    comdone();

FUNC_EXTERN	int	GCX1_trace_alloc();

/*
** Forward functions
*/
static	VOID	GCblastsm();	    	    /* Async state machine */
static  VOID	GCblastpnd();	    	    /* Async event pend */
static	VOID	GCblastread();	    	    /* Async serial i/o read */
static	VOID	GCblastwrite();	    	    /* Async serial i/o write */

/*
** local variables
*/

/* Per connection control block */

typedef struct _GC_PCB
{
    	GCC_P_PLIST    	*lsn_parm_list;	    /* Listen parameter list */
    	GCC_P_PLIST    	*conn_parm_list;    /* Connection parameter list */
    	GCC_P_PLIST    	*send_parm_list;    /* Send parameter list */
    	GCC_P_PLIST    	*rcv_parm_list;     /* Receive parameter list */
    	GCC_P_PLIST   	*disconn_parm_list; /* Disconnect parameter list */
    	struct
    	{
    	    char    	*buf;	    	    /* current buffer address */
    	    i4    	len;	    	    /* current buffer length */
    	} send, rcv;
    	i4 	    	block_size; 	    /* Connection block size */
	bool		listener;	    /* Listener or Requestor? */
	bool		connected;          /* Connected (obviously...) */
    	bool	    	disconnecting;	    /* Disconnection in progress */
	PG_CB		pgcb;		    /* control block for pg services */
} GC_PCB;

GLOBALDEF i4 GCblast_trace = 0;

# define GCTRACE(n) if( GCblast_trace >= n )(void)TRdisplay

#ifdef  VMS
# define ASYNC_DEF_PORT 	"TT:"
#else
# define ASYNC_DEF_PORT		"<stdout>"
#endif /* VMS */

/* Actions in GCblastsm. */

#define	GC_REG		0	/* open listen port */
#define	GC_LSN		1	/* wait for inbound connection */
#define GC_LSNCMP  	2	/* LISTEN completes */
#define	GC_CONN  	3	/* connect to host */
#define GC_CONNNXT      4	/* make outbound connection */
#define GC_CONNCMP      5       /* CONNECT completes */
#define	GC_SND  	6	/* send TPDU */
#define GC_SNDNXT  	7	/* send next block of TPDU */
#define	GC_SNDCMP	8	/* SEND completes */
#define	GC_RCV		9	/* receive TPDU */
#define	GC_RCVCMP	10	/* RECEIVE completes */
#define	GC_DISCONN	11	/* close a connection */
#define GC_DISCONNCMP   12	/* DISCONNECT completes */


/* Action names for tracing. */

static char *gc_names[] = { 
	"REG", "LSN", "LSNCMP", "CONN", "CONNNXT", "CONNCMP",
	"SND", "SNDNXT", "SNDCMP", "RCV", "RCVCMP", "DISCONN", "DISCONNCMP"
} ;

static char async_port[GCC_L_PORT];


/*{
** Name: GCpdrvr    - Protocol driver routine
**
** Description:
**	GCpdrvr is the model for the protocol driver routines which interface
**	to the local system facilities through which access to specific
**	protocols is offered.  In a given environment there may be multiple
**	protocol drivers, depending on the number of separate protocols which
**	are to be supported and the specific implementation of protocol drivers
**	in the environment.  This is an implementation issue, and is dealt with
**	by appropriate construction of the Protocol Control Table (PCT) by the
**	initialization routine GCpinit().
**
**	Protocol driver routines are not called directly by the CL client, but
**	rather are invoked by pointer reference through the Protocol Control
**	Table.  This is therefore a model specification for all protocol
**	drivers, whose names are not actually known outside the CL.  They are
**	known externally only by an identifier in the PCT.
**	
**	All protocol drivers have an indentical interface, through which a set
**	of service functions is offered to clients.  Two parameters are passed
**	by the caller: a function identifier and a parameter list pointer.  The
**	function ID specifies which of the functions in the repertoire is being
**	requested.  The parm list supplies the parameters required for
**	execution of the function.  It contains both input and output
**	parameters.  It has a constant section, and a section containing
**	parameters specific to the requested function.  The interface for all
**	routines is specified here instead of with each routine, as is
**	customary, since it is identical for all.
**
**	It is the responsibility of a single protocol driver to map the service
**	interface for a particular protocol, as provided by a specific
**	environment, to the model interface specified here.  In particular, this
**	includes the management of asynchronous events in the specified way.
**	The protocol driver interface is asynchronous in the sense that when a
**	function is invoked by the client, control is returned prior to
**	completion of the function.  Notification of completion is provided
**	to the client by means of a callback routine provided by the client as a
**	service request parameter.  This routine is to be invoked when the
**	requested service is complete.
**
**	It is to be noted that execution control is guaranteed to pass to the
**	CL routine GCc_exec, and remain there for the duration of server
**	execution.  It is possible for protocol driver service requests to be
**	invoked prior to the time GCc_exec receives control.  It is not
**	required that these service requests complete until control passes to
**	GCc_exec.  This enables the detection and allowance of external
**	asynchronous events to reside in a place to which control is guaranteed
**	to pass subsequent to a service request, if this is appropriate in a
**	particular environment.  This is not required, and if the environment
**	permits, the entire mechanism of dealing with external asynchronous
**	events may be implemented in the protocol driver.  It is also to be
**	noted that when a CL client's callback routine is invoked, it is
**	guaranteed that control will be returned from that invocation when the
**	client's processing is complete, and that the client will not wait or
**	block on some external event.
**
**
** Inputs:
**      Function_code                   Specifies the requested function
**          GCC_OPEN        ||
**	    GCC_CONNECT	    ||
**	    GCC_DISCONNECT  ||
**	    GCC_SEND	    ||
**	    GCC_RECEIVE	    ||
**	    GCC_LISTEN
**      parameter_list                  A pointer to a set of parameters
**					qualifying the requested function.
**					See gcccl.h for description.
**
** Outputs:
**      Contained in the parameter list, described above.
**
**	Returns:
**	    STATUS:
**		OK
**		FAIL
**
** History:
**    06-May-90 (cmorris)
**	Initial coding
**    28-Feb-91 (cmorris)
**  	If protocol generator has indicated a disconnect, don't do
**  	a logoff.
**    11-Jun-91 (cmorris)
**	Make sure we drive outstanding receive/send _before_
**	driving disconnect.
**    09-Jul-91 (cmorris)
**	Fixed handling of disconnects after listen failures.
**    16-Jul-91 (cmorris)
**	Added initial support for outgoing connections.
**    22-Jan-92 (alan)
**	Change atoi to CVan.
**    23-Jan-92 (cmorris)
**  	Added ability to get listen "port" from logical.
**    31-Jan-92 (alan)
**	Changed all entry points from 'GCasync...' to 'GCblast...'
**    31-Jan-92 (cmorris)
**  	Fixed up tracing of async "port" on register.
**    08-Mar-93 (edg)
**	Use PMget for getting aync port.
**    16-Mar-93 (edg)
**	Fixed dumb syntax error introduced in last change.
**	
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**      05-nov-99 (loera01) Bug 99059
**          In GCblastsm(), initialize node_id to NULL.  Failure to do so may 
**          cause an ACCVIO when the GCC examines it and subsequently tries 
**          to copy.
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**      12-dec-2002 (loera01)  SIR 109237
**          In GCblastsm(), set the options field in GCC_P_PLIST to zero 
**          (remote peer). 
**	11-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.
*/
STATUS
GCblast( func_code, parms )
i4		    func_code;
GCC_P_PLIST	    *parms;
{
    /*
    ** Compute initial state based on function code.
    */

    switch( func_code )
    {
    case GCC_OPEN: 	parms->state = GC_REG; break;
    case GCC_LISTEN: 	parms->state = GC_LSN; break;
    case GCC_CONNECT: 	parms->state = GC_CONN; break;
    case GCC_SEND: 	parms->state = GC_SND; break;
    case GCC_RECEIVE: 	parms->state = GC_RCV; break;
    case GCC_DISCONNECT:parms->state = GC_DISCONN; break;
    }

    GCX_TRACER_1_MACRO("GCblast>", parms,
		"function code = %d", func_code);

    GCTRACE(1)("GCblast: %x operation %s\n", parms,
    	gc_names[ parms->state ] );

    parms->generic_status = OK;

    /* initialize system status */
    CL_CLEAR_ERR(&parms->system_status);

    /* Start sm. */

    GCblastsm( parms );
    return OK;
}

static VOID
GCblastsm( parms )
GCC_P_PLIST	*parms;
{
    register	GC_PCB		*pcb = (GC_PCB *)parms->pcb;
    int		rc;
    char    	*trace;
    i4	    	len;
    char    	*inst;
    char    	*sym = "II_GCC%s_ASYNC";
    char    	*val = 0;
    char	pmsym[128];

    GCTRACE(2)("GCblastsm: %x state %s\n", parms, gc_names[ parms->state ] );

    switch( parms->state )
    {
    case GC_REG:
	/*
	** Register before the first listen.
	*/

	NMgtAt( "II_GCCL_TRACE", &trace );

	if( trace && *trace )
	    CVan( trace, &GCblast_trace );

	/* Initialize serial I/O driver */
	if ((rc = sioinit()) != OK)
	{
	    parms->generic_status = GC_OPEN_FAIL;
	    goto complete;
	}	    

        /* 
    	** Open channel - the default is to use the terminal port
    	** but first look to see if symbol has been specified for
    	** async. 
    	*/
	STprintf( pmsym, "!.%.port", parms->pce->pce_pid );

	if ( PMget( pmsym, &val ) != OK )
	{
	    /* use port ID from open paramters */
	    val = parms->function_parms.open.port_id;
	}


	if ((rc = cominit( val )) != OK)
	{
	    siodone();
	    parms->generic_status = GC_OPEN_FAIL;
	    goto complete;
	}

    	/* set up device characteristics */
        if ((rc = comst( parms->function_parms.open.port_id , SIO_B9600,
    	    SIO_PODD)) != OK)
    	{
    	    comdone();
    	    siodone();
    	    parms->generic_status = GC_OPEN_FAIL;
    	    goto complete;
	}

	/* Initialize protocol generator */
	pgini();

	/*
	** Registration completed.
	*/

	/* Note protocol is open and print message to that effect. */
    	if (!(STcompare(val, "")))

	    /* we're using stdout */
	    STcopy(ASYNC_DEF_PORT, async_port);
    	else
    	    STcopy(val, async_port);

    	parms->function_parms.open.lsn_port = async_port;

	GCTRACE(1)("GCblastsm: Async port [%s]\n", 
    	    	   parms->function_parms.open.lsn_port);

	goto complete;

    case GC_LSN:
	/*
	** Extend a listen to accept a connection. In BLAST terminology
	** this translates into doing a "protocol generator logon".
	*/

        /*
        **  Client is always remote.
        */
        parms->options = 0;

	/* Allocate pcb */

	pcb = (GC_PCB *)MEreqmem( 0, sizeof( *pcb ), TRUE, (STATUS *)0 );
	parms->pcb = (PTR)pcb;

	if( !pcb )
	{
	    parms->generic_status = GC_LISTEN_FAIL;
	    goto complete;
	}

    	/* Initialize pcb */
	pcb->listener = TRUE;
	pcb->connected = FALSE;
    	pcb->disconnecting = FALSE;

    	/* Register callbacks with serial i/o driver */
    	sioreg(GCblastread, GCblastwrite, (PTR) pcb);

	/* Logon with pglgn() */
	parms->state = GC_LSNCMP;
    	pcb->lsn_parm_list = parms;
	pglgn(&pcb->pgcb);

	/* enter the protocol generator */
	GCblastpnd(pcb);
	return;

    case GC_LSNCMP:
	/*
	** Completion of listen.
	*/

        /*
        **  Initialize node_id to NULL.  At present in VMS, that's all we do.  
        **  Other platforms use the field.
        */
         parms->function_parms.listen.node_id=NULL;
 
	if (!parms->generic_status)
	    pcb->connected = TRUE;

    	/* Drive completion exit */
    	pcb->lsn_parm_list = NULL;
	goto complete;

    case GC_CONN:
	/* Allocate pcb */

	pcb = (GC_PCB *)MEreqmem( 0, sizeof( *pcb ), TRUE, (STATUS *)0 );
	parms->pcb = (PTR)pcb;

	if( !pcb )
	{
	    parms->generic_status = GC_CONNECT_FAIL;
	    goto complete;
	}

    	/* Initialize pcb */
	pcb->listener = FALSE;
	pcb->connected = FALSE;
    	pcb->disconnecting = FALSE;

    	/* Register callbacks with serial i/o driver */
    	sioreg(GCblastread, GCblastwrite, (PTR) pcb);

	/* Connect to remote host */
     	pcb->pgcb.buf = parms->function_parms.connect.port_id;
	parms->state = GC_CONNNXT;
    	pcb->conn_parm_list = parms;
	pgcnt(&pcb->pgcb);   
        
	/* enter the protocol generator */
	GCblastpnd(pcb);
	return;

    case GC_CONNNXT:
	/* See whether we connected */
        if (parms->generic_status)
	{
	    /* No, there was an error... */
	    pcb->conn_parm_list = NULL;
	    goto complete;
	}

	/* Logon with pglgn() */
	parms->state = GC_CONNCMP;
    	pcb->conn_parm_list = parms;
	pglgn(&pcb->pgcb);

	/* enter the protocol generator */
	GCblastpnd(pcb);
	return;

    case GC_CONNCMP:
	/*
	** Completion of connect.
	*/

	if (!parms->generic_status)
	    pcb->connected = TRUE;

    	/* Drive completion exit */
    	pcb->conn_parm_list = NULL;
	goto complete;

    case GC_SND:
	/*
	** Send TPDU with pgsnd
	*/

	/* set up buffer details by adding length to front */

	pcb->send.buf = parms->buffer_ptr - 2;
	pcb->send.len = parms->buffer_lng + 2;

    	pcb->send.buf[0] = (u_char)(pcb->send.len % 256);
    	pcb->send.buf[1] = (u_char)(pcb->send.len / 256);

	GCTRACE(2)("GCblastsm: %x send size %d\n", parms, parms->buffer_lng );

    	/* send first block */
    	pcb->pgcb.buf = pcb->send.buf;
    	if(pcb->send.len > pcb->block_size)
    	{
    	    /* We have more data to send */
    	    pcb->pgcb.len = pcb->block_size;
    	    pcb->send.buf += pcb->block_size;
    	    pcb->send.len -= pcb->block_size;
    	    parms->state = GC_SNDNXT;
    	}
    	else
    	{
    	    /* everything goes */
    	    pcb->pgcb.len = pcb->send.len;
    	    parms->state = GC_SNDCMP;
	}

    	pcb->send_parm_list = parms;
	pgsnd(&pcb->pgcb);

	/* enter the protocol generator */
	GCblastpnd(pcb);
	return;

    case GC_SNDNXT:
    	/*
    	** Send next block.
    	*/

    	/* Error on send? */
    	if (parms->generic_status)
	{
    	    pcb->send_parm_list = NULL;
    	    goto complete;
	}

    	pcb->pgcb.buf = pcb->send.buf;
    	if(pcb->send.len > pcb->block_size)
    	{
    	    /* We have more data to send */
    	    pcb->pgcb.len = pcb->block_size;
    	    pcb->send.buf += pcb->block_size;
    	    pcb->send.len -= pcb->block_size;
    	    parms->state = GC_SNDNXT;
    	}
    	else
    	{
    	    /* everything goes */
    	    pcb->pgcb.len = pcb->send.len;
    	    parms->state = GC_SNDCMP;
	}

	pgsnd(&pcb->pgcb);

	/* enter the protocol generator */
	GCblastpnd(pcb);
	return;

    case GC_SNDCMP:
	/*
	** Send complete.
	*/

    	pcb->send_parm_list = NULL;
    	goto complete;

    case GC_RCV:
	/*
	** Setup bounds of read area.
	*/
    	pcb->rcv.buf = parms->buffer_ptr - 2;
    	pcb->rcv.len = parms->buffer_lng + 2;

    	/* receive first block */
	pcb->pgcb.buf = pcb->rcv.buf;
	pcb->pgcb.len = pcb->block_size;

	/* Issue read for TPDU with pgrd */

	GCTRACE(2)("GCblastsm: %x read size %d\n", parms, parms->buffer_lng );

	parms->state = GC_RCVCMP;
    	pcb->rcv_parm_list = parms;
        pgrd(&pcb->pgcb);

	/* Pend for protocol generator event */
	GCblastpnd(pcb);
	return;

    case GC_RCVCMP:
	/*
	** Receive complete.
	*/

    	/* Error on read? */
    	if (parms->generic_status)
	{
    	    pcb->rcv_parm_list = NULL;
    	    goto complete;
	}

    	/* Is this first time through? */
    	if (pcb->rcv.buf < parms->buffer_ptr)
    	{
    	    /* Get length of TPDU */
    	    len = ((u_char *)pcb->rcv.buf)[0] +
    	    	  ((u_char *)pcb->rcv.buf)[1] * 256;
	    if( len < 0 || len > parms->buffer_lng )
	    {
	    	parms->generic_status = GC_RECEIVE_FAIL;
       	    	pcb->rcv_parm_list = NULL;
	    	goto complete;
	    }

    	    pcb->rcv.len = len;
    	    parms->buffer_lng = len - 2;
	}

    	/* See how much we read */
    	pcb->rcv.len -= pcb->pgcb.len;
    	pcb->rcv.buf += pcb->pgcb.len;

    	/* More to read? */
    	if(pcb->rcv.len)
    	{
    	    /* post another read */
    	    pcb->pgcb.buf = pcb->rcv.buf;
	    pcb->pgcb.len = pcb->block_size;

	    /* Issue read for next block with pgrd */
            pgrd(&pcb->pgcb);

	    /* Pend for protocol generator event */
	    GCblastpnd(pcb);
	    return;
	}

    	/* No, we're out of here... */
    	pcb->rcv_parm_list = NULL;
        goto complete;

    case GC_DISCONN:
	/*
	** Disconnect with pglgo
	*/

	/* Close connection */
	parms->state = GC_DISCONNCMP;
    	pcb->disconn_parm_list = parms;

    	/* 
    	** If we have received a disconnect indication or we haven't 
	** connected, we do not need to do the protocol generator logoff.
    	*/
    	if (!pcb->disconnecting && pcb->connected)
	{
    	    pcb->pgcb.intr = TRUE;
	    pglgo( &pcb->pgcb );

    	    /* Pend for protocol generator event */
    	    GCblastpnd(pcb);
	}

	/* If we haven't connected, drop thru' */
	if (pcb->connected)
	    return;

    case GC_DISCONNCMP:

    	pcb->disconn_parm_list = NULL;

    	/* Release serial i/o driver */
    	siodone();

    	/* Release communications port */
	comdone();

    	/* Free up pcb */
	MEfree( (PTR)pcb );
	goto complete;

    }

complete:
    /*
    ** Drive completion routine.
    */

    GCTRACE(1)("GCblastsm: %x complete %s status %x\n", parms, 
		gc_names[ parms->state ], parms->generic_status );

    (*parms->compl_exit)(parms->compl_id);
}

/*
** Name: GCblastpnd - pend for async event
**
** Description:
**	Pends (by calling pgpnd) for protocol generator
**  	event to complete. Upon recepit of a pg event,
**  	it reenters the async state machine. Has the side-effect 
**      of driving a previously posted protocol generator event.
**  	NOTE: This function never blocks for i/o; if no
**  	event can be completed without i/o, no event
**  	is returned by pgpnd and the async state machine is not
**  	reenetered.
**
** Inputs:
**	pcb - protocol control block
**
** Returns:
**	nothing
**
** History
**	28-Jan-91 (cmorris)
**	    Initial coding.
**  	13-Jul-92 (cmorris)
**  	    A disconnect can also time out.
*/
static VOID
GCblastpnd( pcb )
GC_PCB	*pcb;
{
    int	event;	    	    	/* protocol generator event */
    int val;	    	    	/* value associated with event */
    GCC_P_PLIST *parm_list; 	/* parameter list */
    bool retry;	    	    	/* retry pgpnd */

    /* Pend for protocol generator event */
    event = pgpnd(&pcb->pgcb);

    GCTRACE(2)("GCblastpnd: %x protocol generator event %d\n", pcb, event);

    /* switch on the type of event and process appropriately */
    switch (event)
    { 
    case PG_BLOCKED:

        /*
    	** No pg event can be completed without completion of i/o event.
        ** We will be re-called when an i/o event completes and the appropriate
    	** callback has been driven. Right now do nothing and return.
        */
    	break;

    case PG_SND:

        /* A send has completed. Reenter the protocol machine */
    	parm_list = pcb->send_parm_list;
        GCblastsm(parm_list);

    	/*
	** Check for piggybacked event (a single pg block may both
	** contain an ack (which completes a send) and data (which
	** can complete a receive). Additionally, sends that do not
	** fill the send window need driving a second time to
	** actually "send" them.
        */ 
   	GCblastpnd(pcb);
    	break;

    case PG_RD:

        /* A read has completed. Reenter the protocol machine */
    	parm_list = pcb->rcv_parm_list;
        GCblastsm(parm_list);
    	break;

    case PG_UISND:
    case PG_UIRD:

        /* Not used by protocol driver. Just ignore */
    	break;

    case PG_LOGON:

    	/* Logon has completed - stash away block size */
    	pcb->block_size = pcb->pgcb.len;
	if (pcb->listener)
    	    parm_list = pcb->lsn_parm_list;
	else
	    parm_list = pcb->conn_parm_list;
    	GCblastsm(parm_list);
    	break;

    case PG_CONN:

    	/* Connect has completed */
        parm_list = pcb->conn_parm_list;
    	GCblastsm(parm_list);
    	break;

    case PG_DISC:

        /* 
    	** Disconnect received. Drive outstanding receive/sends with
	** an error, and then drive the disconnect completion.
	** NOTE: parm_list's in the pcb are nulled upon event 
    	** completion within calls to GCblastsm.
    	*/

    	pcb->disconnecting = TRUE;

    	/* See if send operation in progress */
        parm_list = pcb->send_parm_list;
    	if (parm_list)
        {
    	    /* Set up error and renter state machine with it */

    	    parm_list->generic_status = GC_SEND_FAIL;
            GCblastsm(parm_list);
    	}

        /* See if receive operation in progress */
    	parm_list = pcb->rcv_parm_list;
        if (parm_list)
       	{
    	    /* Set up error and reenter state machine with it */
            parm_list->generic_status = GC_RECEIVE_FAIL;
    	    GCblastsm(parm_list);
        }

        /* See if disconnect issued */
    	parm_list = pcb->disconn_parm_list;
    	if (parm_list)
            GCblastsm(parm_list);

    	break;

    case PG_TIMO:
    	parm_list = pcb->lsn_parm_list;
        if (parm_list)
    	{
    	    /* We are doing a listen so complete it with error */
      	    parm_list->generic_status = pcb->pgcb.status;
    	    STRUCT_ASSIGN_MACRO(pcb->pgcb.syserr, parm_list->system_status);
    	    GCblastsm(parm_list);
    	}

        /* See if disconnect issued */
    	parm_list = pcb->disconn_parm_list;
    	if (parm_list)
            GCblastsm(parm_list);

    	break;

    case PG_ERROR:

        /* 
    	** Disconnect received. Drive outstanding receive/sends with
	** an error, and then drive the disconnect completion.
	** NOTE: parm_list's in the pcb are nulled upon event 
    	** completion within calls to GCblastsm.
    	*/

    	pcb->disconnecting = TRUE;

    	parm_list = pcb->lsn_parm_list;
        if (parm_list)
    	{
    	    /* We are still doing a listen so complete it with error */
      	    parm_list->generic_status = pcb->pgcb.status;
    	    STRUCT_ASSIGN_MACRO(pcb->pgcb.syserr, parm_list->system_status);
    	    GCblastsm(parm_list);
    	    break;
    	}

    	/* See if send operation in progress */
        parm_list = pcb->send_parm_list;
    	if (parm_list)
        {
    	    /* Set up error and renter state machine with it */

    	    parm_list->generic_status = GC_SEND_FAIL;
            GCblastsm(parm_list);
    	}

        /* See if receive operation in progress */
    	parm_list = pcb->rcv_parm_list;
        if (parm_list)
       	{
    	    /* Set up error and reenter state machine with it */
            parm_list->generic_status = GC_RECEIVE_FAIL;
    	    GCblastsm(parm_list);
        }

        /* See if disconnect issued */
    	parm_list = pcb->disconn_parm_list;
    	if (parm_list)
            GCblastsm(parm_list);

    	break;

    default:
    	break;
    }    
}

/*
** Name: GCblastread - Async serial i/o read completion handler
**
** Description:
**	Handles the completion of serial i/o read.
**
** Inputs:
**	pcb - protocol control block
**  	error - indicates whether timeout has occurred.
**
** Returns:
**	nothing
**
** History
**	28-Jan-91 (cmorris)
**	    Initial coding.
**	09-Jan-92 (alan)
**	    For VMS, do not issue the sioread.
*/
static VOID
GCblastread( pcb, error )
GC_PCB	*pcb;
int error;
{
#ifndef VMS
    /* Complete serial i/o read */
    sioread(error);
#endif /* VMS */
    
    /* Pend for protocol generator event */
    GCblastpnd(pcb);
}

/*
** Name: GCblastwrite - Async serial i/o write completion handler
**
** Description:
**	Handles the completion of serial i/o write.
**
** Inputs:
**	pcb - protocol control block
**  	error - indicates whether timeout has occurred
**
** Returns:
**	nothing
**
** History
**	28-Jan-91 (cmorris)
**	    Initial coding.
**	09-Jan-92 (alan)
**	    For VMS, do not issue the siowrite.
*/
static VOID
GCblastwrite( pcb, error )
GC_PCB	*pcb;
int error;
{

#ifndef VMS
    /* Complete serial i/o write */
    siowrite(error);
#endif /* VMS */
    
    /* Pend for protocol generator event */
    GCblastpnd(pcb);
}
