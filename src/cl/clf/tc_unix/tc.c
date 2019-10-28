/*
**Copyright (c) 2004 Actian Corporation
*/
/*
** TC.C
**
** This file contains Unix versions of the TC CL routines.
** The TC module contains functions to operate on COMM-files, which are
** used for communication between INGRES front ends and the SEP test tool.
**
** On Unix, TCFILES are implemented as plain files. A "writer" process
** process simply appends data to the file. A "reader" process keeps
** track of how many bytes it's read so far; when fstat() reports that there
** are new bytes to be read (i.e. the file's gotten bigger,) the process
** reads. As a performance enhancement, this scheme of polling on fstat()
** will probably change soon.
**
** Also, the current SEP mainline implicitly requires that TCFILEs be able to
** support multiple processes on one end. Synchronization files are used to
** work around this problem. There's a synchronization file for each TCFILE.
** Currently, a "reader" must execute 3 steps: (1) check the synchronization
** file to find the file address of the last bytes read; (2) seek to the first
** unread byte; (3) read; and (4) update the synchronization file. The SEP
** mainline will have this restriction removed in a matter of dayts, at which
** time the significant performance hit from synchronization files will be
** removed.
**
** Routines in this file:
**	TCopen()
**	TCputc()
**	TCflush()
**	TCgetc()
**	TCclose()
**	TCempty()
**
** History:
**	22-may-89 (mca) - Initial version.
**	 1-aug-89 (mca) - Fixed Sun-specific timer sleep problem in TCgetc.
**			  On a heavily-loaded system, signal was occasionally
**			  delivered before sigpause() was called. Made changes
**			  so that this isn't a problem. Also, changed the
**			  length of the sleep.
**	23-Aug-1989 (fredv)
**		Using cl headers instead of sys headers.
**		Removed clsecret.h because include clconfig.h is enough.
**	06-Dec-1989 (fredv)
**		Replaced sun_u42 defines with xCL_030_SETITIMER_EXISTS
**		so that we can use itmer for those systems support this
**		function.
**	12-dec-1989 (mca)
**		Merged tests/sep/sepcl/tcsep.c with this file. Added the
**		IITCmaster bool, which SEP will set to TRUE but which
**		INGRES frontends will leave FALSE. When it's TRUE, TCclose()
**		will unlink sync files. Also, have TCgetc() return TC_EOF
**		if file isn't open for reading; previously, it returned 0,
**		which is a valid character. (Thanks, DaveB.)
**	05-apr-1990 (kathryn) (integrated into ingres63p by rog)
**		Remove references to SETITIMER use II_nap mechanism. 
**	16-apr-90 (kimman)
**		Integrating in the latest changes from sepcl/tcsep.c
**		into tc.c.
**	21-aug-90 (rog)
**		Portability changes so that TCgetc handles -1 correctly.
**	27-aug-90 (rog)
**		Move TC_ESCAPE into this file so that it isn't globally
**		visible.
**	28-aug-1990 (rog)
**		Back out the above two changes because they need to go through
**		the proper review process.
**	08-oct-90 (rog)
**		Escape out-of-band data, e.g., negative numbers -- especially
**		-1, in TCputc() and TCgetc().
**      11-feb-93 (smc)
**              Added time.h to define tm for axp_osf.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	25-may-95 (emmag)
**	    Map II_nap to Sleep() API call on NT.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	06-mar-96 (emmag)
**	    Check for NULL pointer.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to tcdata.c.
**	23-mar-97 (mcgem01)
**	    Desktop platforms are renowned for their tendancy to cache
**	    disk i/o.  Force all writes to disk with a commit.
**	12-jun-1998 (kosma01)
**	    On sgi_us5, replaced the lseeks that use a negative offset to seek
**	    backwards 4 bytes from the end of the syn file with fstat.
**	    Sep was failing by reading the same byte from the comm file.
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-Mar-2001 (wansh01)
**	    On dgi_us5, replaced the lseeks that use a negative offset to seek
**	    backwards 4 bytes from the end of the syn file with fstat.
**	    Sep was failing by reading the same byte from the comm file.
**	23-Sep-2003 (hanje04)
**	    On int_lnx with largefile support lseek must be called with
**        offset of type off_t and not int otherwise we get some crazy
**        file pointer behavior.
**	02-Jun-2005 (hanje04)
**	    MAC OS X also needs lseek to be called with type off_t.
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**      24-Mar-2010 (coomi01) b124361
**          SepSync project to speed SEP up by using Sockets to communicate.
**	23-Nov-2010 (kschendel)
**	    Drop obsolete ports.  Remove windows stuff, this is unix-only.
*/

#include <compat.h>
#include <gl.h>
#include <clconfig.h>
#include <systypes.h>
#include <sys/stat.h>
#ifdef xCL_006_FCNTL_H_EXISTS
#include <fcntl.h>
#endif
#ifdef xCL_007_SYS_FILE_H_EXISTS
#include <sys/file.h>
#endif
#ifdef axp_osf
#include <time.h>
#endif
#include <me.h>
#include <lo.h>
#include <st.h>
#include <si.h>
#include <tm.h>
#include <pc.h>
#include <tc.h>
#include <seek.h>

#ifndef F_OK
#define F_OK 0
#endif

#define     TC_ESCAPE	    '\\'	    /* TC escape character */

#ifdef SEP_SYNC
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <tc_sync.h>
#define max_fd(a,b) ((a) > (b) ? (a) : (b))
#define SOC_NOTSET      -1
#define SOC_CLOSED      -1
#define SOC_ISCLOSED(x) (x < 1)
#define SOC_ISOPEN(x)   (x > 0)
#define SOC_CLOSE(x)    if ( SOC_ISOPEN(x) ) { close(x); x = SOC_NOTSET; }

/*
** Static Function prototypes
*/
static void   serviceIncomingConnection(TCFILE *desc, i4  listen);
static i4     createOutgoingConnection(i4  listener, char *port);
static void   pushStack(TCFILE *desc);
static void   popStack(TCFILE *desc);
static void   lookForDeadPids();
static i4     sepMultiplexRestart(TCFILE *desc, i4  *withTimeout);
static i4     sepMultiplex(TCFILE *desc, i4  withTimeout);
static i4     readSepSyncChar(TCFILE *desc,i4  timeout);
static void   TCinit();
static void   writeSepSync(i4  character, TCFILE *desc);
static STATUS WriteSepSyncChar(i4  character, TCFILE *desc);
static void   FlushsepSync(TCFILE *desc);

static STATUS Orig_TCPutc(i4 achar,TCFILE  *desc);
static i4     Orig_TCGetc(TCFILE  *desc, i4 seconds);
static STATUS Orig_TCOpen(LOCATION *location,char *mode,TCFILE **desc);
static STATUS Orig_TCClose(TCFILE  *desc);
static STATUS Orig_TCFlush(TCFILE  *desc);
static bool   Orig_TCEmpty(TCFILE *desc);

/**********************************************/
/**********************************************/
/**********************************************/

/*
** These are here to assist with GDB style debuging,
** very useful when you need to trace parent/child
** process interactions
*/
volatile int extGdbFlag  = 0;
volatile int waitGdbFlag;
static void
checkGdb(int overRide, int delay)
{
    if (overRide)
    {
	waitGdbFlag = delay;
    }
    else
    {
	waitGdbFlag = extGdbFlag;
    }

    while ( waitGdbFlag )
	sleep(1);
}
/**********************************************/
/**********************************************/
/**********************************************/

/*
** Procedure:   serviceIncomingConnection
** Description: 
**           To set up a connection request from a child process.
**           This is initiated in typical Berkely fashion
**
**                       bind->listen->accept
**
**           When the client does a connect, a request will arrive
**           on the accept for either the data or sync descriptor. 
**
**           The activity is relatively rare.
**
**           We then go through a minor protocol to ensure details
**           are right, before pushing the child onto our stack.
*/
static void
serviceIncomingConnection(TCFILE *desc, i4  listen)
{
    fd_set                   readMask;
    fd_set                  writeMask;
    i4                         signon;
    i4                          ident;
    i4                           nfds;
    i4                    msgSize = 0;
    char                    msg[1024];
    i4                       childPid;
    i4                            pos;
    i4                          lcbId;
    struct timeval            timeout;
    struct timeval           *timePtr;
    sepSyncDetailsType *wrkingStackPtr;
    i4                         sState;

    /*
    ** Check we are server side 
    */
    if (! sepSyncGlobalVars.inSepTool )
      return;

    /*
    ** Note which service is being invoked */
    if ( listen == sepSyncGlobalVars.socketDriver.acceptDataHndl )
    {
      lcbId = 0;
    }
    else if ( listen == sepSyncGlobalVars.socketDriver.acceptSyncHndl )
    {
      lcbId = 1;
    }
    else
    {
      return;
    }

    signon = SOC_CLOSED;
    sState = 0;
    wrkingStackPtr = (sepSyncDetailsType *)0;

    while (1)
    {
	/*
	** Reset
	*/
	nfds     = 0;
	FD_ZERO(&readMask);    
	FD_ZERO(&writeMask);

	if ( 0 == sState )
	{
	    FD_SET(listen, &readMask);
	    nfds = max_fd(nfds,listen);
	    timePtr = 0;
	}
	else
	{
	    if ( 4 == sState )
	    {
		FD_SET(signon, &writeMask);
		nfds = max_fd(nfds,signon);
	    }
	    else
	    {
	        /* 
		** State == 1,2,3
		*/
		FD_SET(signon, &readMask);
		nfds = max_fd(nfds,signon);
	    }

	    /*
	    ** Must not block in this procedure 
	    */
	    timeout.tv_sec   = 10; 
	    timeout.tv_usec  = 0;
	    timePtr = &timeout;
	}


	if (0 == nfds)
	    return;

	/*****************************************/
	/** Local Multiplex non-blocking SELECT **/
	/*****************************************/
	nfds = select (++nfds, &readMask, &writeMask, NULL, timePtr);
	if ( 0 == nfds) 
	{
	    /** time out **/
	    SOC_CLOSE(signon);
	    return;
	}

	/*
	** If signon channel now active, service it first
	*/
	if ( SOC_ISOPEN(signon) )
	{
	    if ( FD_ISSET(signon, &readMask))
	    {
		/*
		** We want to ensure clients do not inadvertantly connect 
		** to the wrong sep engine, so use a state machine to check it out.
		** ~ Note, this complexity is necessary to drive exchanges through a 
		**   select() and ensures we don't lock up because of external issues.
		*/
		switch ( sState )
		{
		    case 1:
		    {
			/*
			 * The child will send us an identifying message 
			 * ~ First get number of bytes to read
			 */ 
			recv(signon, &msgSize, sizeof(msgSize), 0);
			if (msgSize != strlen(sepSyncGlobalVars.socketDriver.name))
			{
			    close(signon);
			    return;
			}
		    }
		    break;  /** Case 1 **/
			
		    case 2:
		    {
			/*
			 * Now get the message body, 
			 * and compare it with what was expected
			 */
			recv(signon, msg, msgSize, 0);
			msg[msgSize] = 0;
			if (0 != STcompare(sepSyncGlobalVars.socketDriver.name, msg))
			{
			    SOC_CLOSE(signon);
			    return;
			}
		    }
		    break;  /** Case 1 **/
			
		    case 3:
		    {
			/*
			** The child pid now arrives
			*/
			recv(signon, &childPid, sizeof(msgSize), 0);
			
			/*
			** Walk the stack looking for this guy
			** ~ Valid locations are 1..stackCount
			*/
			for (pos=1; pos<=sepSyncGlobalVars.socketDriver.stackCount; pos++)
			{
			    if ( childPid == sepSyncGlobalVars.socketDriver.stack[pos].pid )
			    {
				break;
			    }
			}

			/*
			** Valid positions are 1..SEP_SOCKSTACK_SIZE inclusive
			*/
			if (pos > SEP_SOCKSTACK_SIZE)
			{
			    /*
			    ** Stack overflow 
			    ** ~ Unlikely
			    */
			    SOC_CLOSE(signon);
			    return;
			}

			/*
			** Use a local var for convenience
			** ~ NB pos will be one ahead of stackCount
			*/
			wrkingStackPtr = &sepSyncGlobalVars.socketDriver.stack[pos];

			if (0 == lcbId)
			{
			    /*
			    ** Serving a 'data' connect 
			    */
			    if (childPid == wrkingStackPtr->pid)
			    {
				
				if (SOC_ISOPEN(wrkingStackPtr->data))
				{
				    /*
				    ** Already have a data entry
				    */
				    SOC_CLOSE(signon);
				    return;
				}
			    }

			    /*
			    ** Fill is some details 
			    */
			    wrkingStackPtr->pid               = childPid;
			    wrkingStackPtr->data              = signon;
			    wrkingStackPtr->sync              = SOC_NOTSET;

			} else {

			    /*
			    ** This is a sync channel
			    */
			    if ((childPid != wrkingStackPtr->pid) || SOC_ISOPEN(wrkingStackPtr->sync))
			    {
				/*
				** Already have a sync entry
				*/
				SOC_CLOSE(signon);
				return;
			    }

			    wrkingStackPtr->abort = 0;
			    wrkingStackPtr->sync = signon;			    
			    wrkingStackPtr->remote.pid         = childPid;
			    wrkingStackPtr->remote.readCount   = 0;
			    wrkingStackPtr->remote.writeCount  = 0;
			    wrkingStackPtr->remote.cmd         = SEPSYNC_CMD_OK;
			    wrkingStackPtr->remote.state       = SEPSYNC_STATE_SUSPENDED;
			    wrkingStackPtr->remote.toBeSent    = SYNC_DONE;

			    /*
			    ** There is only one iobuffs structure to provide a single 'stream' of data
			    */
			    wrkingStackPtr->iobuffs           = sepSyncGlobalVars.socketDriver.stack[1].iobuffs;

			    wrkingStackPtr->local.pid         = getpid();
			    wrkingStackPtr->local.readCount   = 0;
			    wrkingStackPtr->local.writeCount  = 0;
			    wrkingStackPtr->local.cmd         = SEPSYNC_CMD_OK;
			    wrkingStackPtr->local.toBeSent    = SYNC_DONE;
			    wrkingStackPtr->local.state       = SEPSYNC_STATE_INITIATED;
			}
		    }
		    break; /** Case 3 **/
		}

		/*
		** Next state 
		*/
		sState += 1;

		/*
		** Restart the multiplex for signon.
		*/
		continue;
	    } 
	    else if ( FD_ISSET(signon, &writeMask) )
	    {	    
		/*
		**  Return our pid
		*/
		msgSize = getpid();
		send(signon, &msgSize, sizeof(msgSize), 0);
	    
		if (1 == lcbId)
		{
		    pushStack(desc);
		}

		/*
		** All done 
		*/
		return;
	    }			
    
	} /** end if signon active **/

	/*
	** Accept any incoming requests on the correct listener control block.
	*/
	if ( SOC_ISOPEN(listen) && FD_ISSET(listen, &readMask) )
	{
	    /*
	    ** Pick up appropriate listener control
	    */
	    sepSyncGlobalVars.socketDriver.bcb.fd = -1;
	    sepSyncGlobalVars.socketDriver.bspParms.lbcb = (PTR) &sepSyncGlobalVars.socketDriver.lbcb[lcbId];
	    sepSyncGlobalVars.socketDriver.bsDriver->accept(&sepSyncGlobalVars.socketDriver.bspParms);

	    /*
	    ** This now gives us a channel to work with
	    */
	    signon = sepSyncGlobalVars.socketDriver.bcb.fd;
	    if ( SOC_ISCLOSED(signon)  )
	    {
		printf("Bad listen address\n");
		return;
	    }

	    /* Check local host only */
	    if (sepSyncGlobalVars.socketDriver.bspParms.is_remote)
	    {
	        SOC_CLOSE(signon);
		return;
	    }

	    /*
	    ** Check room on stack for a data request
	    */
	    if ((0 == lcbId) && (sepSyncGlobalVars.socketDriver.stackCount > (SEP_SOCKSTACK_SIZE)))
	    {
		/*
		**  Not enough room of stack ?
		**  ~ A max depth of 32 is set, unlikely to fail.
		*/
		SOC_CLOSE(signon);
		return;
	    }

	    /*
	    ** Next state 
	    */
	    sState += 1;
	}

    } /** while **/
}

/*
** Procedure: createOutgoingConnection
**
** Description:
**            This is the other side of the conversation to 
**            serviceIncomingConnection() above.
**            Called twice by the child process at startup via TCOpen() -> TCinit()
*/
static i4 
createOutgoingConnection(i4  listener, char *port)
{
    /*
    ** Two known drivers
    */
    GLOBALREF        BS_DRIVER   BS_socktcp;
    GLOBALREF        BS_DRIVER   BS_sockdnet;

    sepDataBufferType         *readPtr;
    sepDataBufferType        *writePtr;
    fd_set                    readMask;
    fd_set                   writeMask;
    i4                            nfds;
    i4                           ident;
    i4                          signon;
    i4                            loop;
    i4                          sepPid;
    struct timeval             timeout;
    i4                         msgSize;
    char                   *socketName;
    sepSyncDetailsType *workingStackPtr;
    sepSocketsDriverType       *driver;

    socketName = NULL;
    NMgtAt(SEPSOCKETNME, &socketName);
    if ( NULL == socketName )
	return SOC_NOTSET;

    /*
    ** Get pointers
    */
    driver = &sepSyncGlobalVars.socketDriver;
    workingStackPtr = driver->stackTop;

    /*
    ** This one will be op system specific
    ** ~ ifdefs perhaps? 
    */
    driver->bsDriver = &BS_socktcp;

    /*
    ** Setup the driver parameter block 
    */
    driver->bspParms.pid = getpid();
    driver->bspParms.is_remote = 0;
    driver->bspParms.lbcb = (PTR) &driver->lbcb[listener];
    driver->bspParms.bcb  = (PTR) &driver->bcb;
    driver->bspParms.syserr  = (CL_ERR_DESC *) &driver->syserr[0];
    driver->bspParms.buf     = &driver->name[0];
    sprintf(driver->bspParms.buf, "localhost;%s", port);

    /*
    ** This initiates the connect to the SEPSYNC remote
    ** ~ The driver stuff is attempting to hide the details so
    **   we may one day use DEC-NET, and run on VMS too
    */
    driver->bsDriver->request(&driver->bspParms);
    driver->bsDriver->connect(&driver->bspParms);
    signon = driver->bcb.fd;

    if (SOC_ISCLOSED(signon))
	return SOC_NOTSET;

    /**
     ** Write our credentials, then read sep pid 
     **/
    for (loop=0;loop<2;loop++)
    {
	/* Reset */
	nfds = 0;
	FD_ZERO(&readMask);    
	FD_ZERO(&writeMask);
	
	if (0 == loop)
	{
	    FD_SET(signon, &writeMask);
	    nfds = max_fd(nfds,signon);
	}
	else
	{
	    FD_SET(signon, &readMask);
	    nfds = max_fd(nfds,signon);
	}

	if ( 0 == nfds )
	    return SOC_NOTSET;

	/*
	** Must not block in this procedure 
	*/
	timeout.tv_sec   = 3600; 
	timeout.tv_usec  = 0;

	/****************************************************************/
	/** Multiplex non-blocking SELECT waiting for permission to go **/
	/****************************************************************/
	nfds = select (++nfds, &readMask, &writeMask, NULL, &timeout);

	if ( -1 == nfds )
	{
	    /** Error **/
	    perror("SEP SOCKET ERROR, Sep Died?? ");
	    return SOC_NOTSET;
	}

	if ( 0 == nfds )
	{
	    /** time out **/
	    return SOC_NOTSET;
	}
	
	if ( FD_ISSET(signon, &readMask) )
	{
	    /*
	    ** Read Sep's pid onto our stack.
	    */
	    read(signon, &sepPid, sizeof(sepPid));
	    
	    /*
	    ** Fill in local (my) sync details
	    */
	    workingStackPtr->local.pid         = getpid();
	    workingStackPtr->local.readCount   = 0;
	    workingStackPtr->local.writeCount  = 0;
	    workingStackPtr->local.cmd         = SEPSYNC_CMD_OK;
	    workingStackPtr->local.state       = SEPSYNC_STATE_INIT;
	    workingStackPtr->local.toBeSent    = SYNC_DONE;
	    workingStackPtr->local.ident       = 0;
	}
	else if ( FD_ISSET(signon, &writeMask)) 
	{
	    /* 
	    ** Say hello to Sep 
	    */
	    msgSize = strlen(socketName);
	    
	    write(signon, &msgSize, sizeof(msgSize));
	    write(signon, socketName, msgSize);
	    
	    msgSize = getpid();
	    write(signon, &msgSize, sizeof(msgSize));
	}
    }

    if (0 == listener)
    {
	workingStackPtr->data = signon;
    }
    else if (1 == listener)
    {
	workingStackPtr->sync = signon;
    }
    return signon;
}

/*
** Procedure initStack
**
** Description:
**      Called immediately after each fork() to set up the 
**      stack in anticipation of the first incomming connection.
*/
void
initStack(TCFILE *inPtr, TCFILE *outPtr, i4 childPid)
{
    sepSyncDetailsType *wrkingStackPtr = &sepSyncGlobalVars.socketDriver.stack[0];
    sepSyncGlobalVars.socketDriver.stackCount = 0;

    wrkingStackPtr->pid               = childPid;
    wrkingStackPtr->data              = SOC_NOTSET;
    wrkingStackPtr->sync              = SOC_NOTSET;

    /* Our record */
    wrkingStackPtr->local.pid         = getpid();
    wrkingStackPtr->local.readCount   = 0;
    wrkingStackPtr->local.writeCount  = 0;
    wrkingStackPtr->local.cmd         = SEPSYNC_CMD_OK;
    wrkingStackPtr->local.state       = SEPSYNC_STATE_SUSPENDED;
    wrkingStackPtr->local.toBeSent    = SYNC_DONE;

    /* Child record */
    wrkingStackPtr->remote.pid         = childPid;
    wrkingStackPtr->remote.readCount   = 0;
    wrkingStackPtr->remote.writeCount  = 0;
    wrkingStackPtr->remote.cmd         = SEPSYNC_CMD_OK;
    wrkingStackPtr->remote.state       = SEPSYNC_STATE_SUSPENDED;
    wrkingStackPtr->remote.toBeSent    = SYNC_DONE;

    sepSyncGlobalVars.socketDriver.stackUsage = 1;

    /* 
    ** Make sure buffers are initialised
    */
    inPtr->ipcBuffer->reader.count=0;
    inPtr->ipcBuffer->reader.getPos=0;
    inPtr->ipcBuffer->reader.putPos=0;

    outPtr->ipcBuffer->writer.count=0;
    outPtr->ipcBuffer->writer.getPos=0;
    outPtr->ipcBuffer->writer.putPos=0;
}

/*
** Procedure pushStack
**
** Description
**      Called to add a new connection to sep from a child or grandchild
**      In the case of the latter, the current child must have suspended
**      itself. This happens after the chilkd has sent a sync message to
**      say it is about to fork() a child. In that message wiil be a count
**      of bytes sent to sep. Sep will wait until it has read all those
**      bytes.
*/
static void
pushStack(TCFILE *desc)
{
  i4 count;
  sepSyncDetailsType *wrkingStackPtr;

  count = sepSyncGlobalVars.socketDriver.stackCount;
  wrkingStackPtr = &sepSyncGlobalVars.socketDriver.stack[count];

  /*
  ** Must await all data from current client (that is suspending) first
  */ 
  if (SEPSYNC_STATE_SUSPENDED != wrkingStackPtr->remote.state)
  {
    return;
  }

  /*
  ** Must also wait for attach from new current client 
  */
  count += 1;
  wrkingStackPtr = &sepSyncGlobalVars.socketDriver.stack[count];
  if (SEPSYNC_STATE_INITIATED != wrkingStackPtr->local.state)
  {
    return;
  }

  /*
  ** OK, Start reading/writing to client
  */
  sepSyncGlobalVars.socketDriver.stackCount += 1;

  if (sepSyncGlobalVars.showLine)
      showLine(SHOW_LINE_PUSH, "\n");

  wrkingStackPtr->local.state = SEPSYNC_STATE_WAITING;
  sepSyncGlobalVars.socketDriver.stackTop = wrkingStackPtr;

  desc->ipcBuffer->reader.data = wrkingStackPtr->data;
  desc->ipcBuffer->writer.data = wrkingStackPtr->data;

  desc->ipcBuffer->reader.sync = wrkingStackPtr->sync;
  desc->ipcBuffer->writer.sync = wrkingStackPtr->sync;
}

/*
**  Procedure popStack
**
**  Description
**      The reverse of the above.
*/
static void
popStack(TCFILE *desc)
{
    i4  channel;
    i4  pos;
    sepSyncDetailsType       *wrkingStackPtr;
    sepDataBufferType       *readPtr  = &desc->ipcBuffer->reader;
    sepDataBufferType       *writePtr = &desc->ipcBuffer->writer;
    
    wrkingStackPtr = sepSyncGlobalVars.socketDriver.stackTop;

    SOC_CLOSE( wrkingStackPtr->sync );
    SOC_CLOSE( wrkingStackPtr->data );
    SOC_CLOSE( readPtr->data );
    SOC_CLOSE( writePtr->data );
    

    wrkingStackPtr->remote.state = SEPSYNC_STATE_CLOSED;
    wrkingStackPtr->local.state  = SEPSYNC_STATE_CLOSED;

    wrkingStackPtr->pid             = -1;

    if (sepSyncGlobalVars.showLine)
	if (wrkingStackPtr->abort)
	    showLine(SHOW_LINE_ABORTED, "\n");
	else
	    showLine(SHOW_LINE_POP, "\n");

    /*
    ** Pop the stack ??
    */
    if ( sepSyncGlobalVars.socketDriver.stackCount > 0)
	sepSyncGlobalVars.socketDriver.stackCount--;
    if (sepSyncGlobalVars.socketDriver.stackUsage > 0)
	sepSyncGlobalVars.socketDriver.stackUsage--;
    
    /*
    ** Valid clients are at 1..stackCount
    */
    if ( sepSyncGlobalVars.socketDriver.stackCount )
    {
	wrkingStackPtr = &sepSyncGlobalVars.socketDriver.stack[sepSyncGlobalVars.socketDriver.stackCount];

	sepSyncGlobalVars.socketDriver.stackTop = wrkingStackPtr;
	readPtr->data  = wrkingStackPtr->data;
	writePtr->data = wrkingStackPtr->data;
    }
    else
    {
	sepSyncGlobalVars.socketDriver.stackTop = NULL;
    }
}

/*
** Procedure lookForDeadPids
**
** Description
**     Walk down the stack of currently attached child processes, calling
**     waitpid on each. Adjust stack usage to suit.
*/
static void
lookForDeadPids()
{
    i4  pos;
    i4  pid;
    i4  exitStatus;
    sepSyncDetailsType *wrkingStackPtr;

    for (pos=1; pos<=sepSyncGlobalVars.socketDriver.stackCount; pos++)
    {
	wrkingStackPtr = &sepSyncGlobalVars.socketDriver.stack[pos];

	if (-1 == wrkingStackPtr->pid)
	    continue;

	pid = waitpid(wrkingStackPtr->pid, &exitStatus, WNOHANG);

	if (pid == wrkingStackPtr->pid)
	{
	    /*
	    ** Child has exited
	    */
	    SOC_CLOSE(wrkingStackPtr->data);
	    SOC_CLOSE(wrkingStackPtr->sync);

	    sepSyncGlobalVars.socketDriver.stackUsage--;
	    wrkingStackPtr->remote.state = SEPSYNC_STATE_CLOSED;
	    wrkingStackPtr->pid          = -1;
	}
    }
}

/*
** Procedure: sepMultiplexRestart
**
**  Description:
**
**      This is BY FAR the most important routine in SepSync
**      It multiplexes a bundle of read & write descriptors
**      looking for activity. Initially the there are only
**      two descriptors, for accept data and accept sync.
**      The children dial in, and their descriptors are 
**      added to the mix.
**
**      Bear in mind, we are only prepared to read when we
**      have space, and only prepared to write (which is 
**      even more critical) when we have something to say.
**
**      The restart nature of the routine is to allow us to
**      jump out of nested constructs in an orderly way. The
**      C construct 'break' might return us to an outer loop.
**      A 'return TC_RESTART' break out of the procedure into
**      the wrapper routine below, that then calls us again.
*/
static i4 
sepMultiplexRestart(TCFILE *desc, i4  *withTimeout)
{
    char   direction = desc->direction;
    i4                            nfds;
    i4                       streamCnt;
    i4                         readCnt;
    i4                        writeCnt;
    i4                           ident;
    static i4                syncIdent;
    i4                          signon;
    i4                      listenData;
    i4                      listenSync;
    fd_set                    readMask;
    fd_set                   writeMask;
    i4                       byteCount;
    sepDataBufferType         *readPtr;
    sepDataBufferType        *writePtr;
    i4                        initMode;
    i4                        childPid;
    i4                     msgSize = 0;
    char                     msg[1024];
    i4                             pos;
    i4                       stepCount;
    struct timeval            stepTime;
    struct timeval             timeout;
    struct timeval         *timeoutPtr;
    sepSyncDetailsType *workingStackPtr;
    sepSyncInfoType        *infoRemote;
    sepSyncInfoType         *infoLocal;

    /* 
    ** Will Count the streams 
    */
    readCnt = writeCnt = streamCnt = 0;

    if ( sepSyncGlobalVars.inSepTool )
    {
	/*
	** Only sep listens for incomming connections
	*/
	listenData = sepSyncGlobalVars.socketDriver.acceptDataHndl;
	listenSync = sepSyncGlobalVars.socketDriver.acceptSyncHndl;
    }
    else
    {
	listenData = SOC_NOTSET;
	listenSync = SOC_NOTSET;
	
	workingStackPtr = sepSyncGlobalVars.socketDriver.stackTop;
	if (0 == workingStackPtr)
	    return TC_EOF;

	/*
	** Send our direction changes as a sync
	** ~ unless we are being called to send
	**   some other sort.
	*/
	if ( !workingStackPtr->local.toBeSent &&
	      workingStackPtr->local.state != desc->direction)
	{
	    /*
	    ** Queue a Sync Record
	    */
	    workingStackPtr->local.cmd       = SEPSYNC_CMD_OK;
	    workingStackPtr->local.state     = desc->direction;
	    workingStackPtr->local.ident     = ++syncIdent;
	    workingStackPtr->local.toBeSent  = SYNC_WAIT;
	}
    }

    timeoutPtr = &timeout;	

    if ((sepSyncGlobalVars.inSepTool) && (0 == sepSyncGlobalVars.socketDriver.stackCount))
      {
	/* 
	** Waiting for first connection to establish
	*/
	stepTime.tv_sec   = 10;
	stepTime.tv_usec  = 0;
      }
    else if (*withTimeout)
      {
	stepCount = 5;
	stepTime.tv_sec  = *withTimeout/stepCount;
	stepTime.tv_usec = (*withTimeout % stepCount) * (1000000/stepCount);
      }
    else
      {
	stepTime.tv_sec  = 0;
	stepTime.tv_usec = 500000;
      }

    
    /*
    ** Get down to business 
    */
    while (1) /* until break */
    {
	/* 
	** Reset masks 
	*/
	initMode = 0;
	nfds     = 0;
	FD_ZERO(&readMask);    
	FD_ZERO(&writeMask);

	workingStackPtr = sepSyncGlobalVars.socketDriver.stackTop;
	readPtr    = &desc->ipcBuffer->reader;
	writePtr   = &desc->ipcBuffer->writer;

	/****************************************/
	/**** Initialisation Pre-Select Code  ***/
	/****************************************/

	/*
	** Have we to send a sync record?
	*/
	if (workingStackPtr && workingStackPtr->local.toBeSent)
	{
	    /* Then add to write mask */
	    if ( SOC_ISOPEN(workingStackPtr->sync) )
	    {
		FD_SET(workingStackPtr->sync, &writeMask);
		nfds = max_fd(nfds,workingStackPtr->sync);
		
		streamCnt++;
	    }
	}


	/*
	** Client or Server calling us ? 
	** ~ Each has different send/recieve tasks
	*/
	if ( ! sepSyncGlobalVars.inSepTool )
	{
	    /*
	    ** CHILD: should always read Sync messages
	    */
	    ident = workingStackPtr->sync;

	    if ( SOC_ISOPEN(ident) )
	    {
		FD_SET(ident, &readMask);
		nfds = max_fd(nfds,ident);
		
		streamCnt++;
	    }
	}
	else
	{
	    /*
	    ** SERVER: Always ready to accept comms channel initiations
	    */
	    if ( SOC_ISOPEN(listenData) )
	    {
		FD_SET(listenData, &readMask);
		nfds = max_fd(nfds,listenData);

		streamCnt++;
	    }
	    if ( SOC_ISOPEN(listenSync) )
	    {
	        FD_SET(listenSync, &readMask);
	        nfds = max_fd(nfds,listenSync);

		streamCnt++;
	    }


	    /*
	    ** Now walk ALL sync records
	    */
	    for(pos=1; pos<= sepSyncGlobalVars.socketDriver.stackCount; pos++)
	    {
	      /*
	      ** Ignore any clients who are shutting down
	      */
	      if (SEPSYNC_STATE_CLOSING == 
		  sepSyncGlobalVars.socketDriver.stack[pos].remote.state)
		{
		    continue;
		}

		ident = sepSyncGlobalVars.socketDriver.stack[pos].sync;
		if ( SOC_ISOPEN(ident) )
		{
		    FD_SET(ident, &readMask);
		    nfds = max_fd(nfds,ident);
		    streamCnt++;

		    /* 
		    ** Sometimes want to write child 
		    ** ~ Flagged via local.toBeSent
		    */
		    if (sepSyncGlobalVars.socketDriver.stack[pos].local.toBeSent)
		    {
			FD_SET(ident, &writeMask);
			nfds = max_fd(nfds,ident);
			streamCnt++;
		    }
		}
	    }
	}

	/*
	** Now do normal read/write streams stuff 
	*/
	if ( SOC_ISOPEN(readPtr->data) && (readPtr->count < SEPBUFFERSZ))
	{
	    /*
	    ** Have space for a read 
	    */
	    FD_SET(readPtr->data, &readMask);
	    nfds = max_fd(nfds,readPtr->data);
	    streamCnt++;
	    readCnt++;
	}

	if ( SOC_ISOPEN(writePtr->data) && (writePtr->count > 0))
	{
	    /*
	    ** Write if room available
	    */
	    FD_SET(writePtr->data, &writeMask);
	    nfds = max_fd(nfds,writePtr->data);
	    streamCnt++;
	    writeCnt++;
	}
	
	if ( sepSyncGlobalVars.inSepTool )
	{
	    /*
	    ** We are in sep
	    */
	    if (2 == streamCnt)
	    {
		/*
		** As yet, there are no children connected
		*/
		if (0 == sepSyncGlobalVars.socketDriver.stackUsage)
		{
		    /* 
		    ** And we are waiting for none either 
		    */
		    return 0;
		}
	    }

	    /*
	    ** Re-initialise the timeout struct
	    ** ~ kernel adjusts it in the select call
	    */
	    timeout.tv_sec   =   stepTime.tv_sec;
	    timeout.tv_usec  =   stepTime.tv_usec;

	    /* Look to see if sep has set a timeout */
	    if (*withTimeout)
	    {
		stepCount--;
		if (stepCount < -1)
		{
		    if (sepSyncGlobalVars.showLine)
		    {
			char allowed[128];
			sprintf(allowed, 
				" ?? sep has waited %d secs%s\n", 
				*withTimeout,
				sepSyncGlobalVars.args.parentIgnoreTimeOut ? 
				" (was ignored)" : "");

			showLine(SHOW_LINE_TIMEOUT, allowed);
		    }

		    /* 
		    ** If ignoring timeouts, do a restart 
		    ** ~ Could hang sep! 
		    */
		    if (sepSyncGlobalVars.args.parentIgnoreTimeOut)
		    {
			return TC_RESTART;
		    }
		    else
		    {
			return 0;
		    }
		}
	    }
	}
	else
	{
	    /*
	    ** We are in the client, timeout may be programed via sep env var.
	    */
	    timeout.tv_sec   = sepSyncGlobalVars.childTimeOut; 
	    timeout.tv_usec  = 0;
	}


	/**********************/
	/*****Preparations*****/
	/******Completed*******/
	/**********************/
	/** Multiplex SELECT **/
	/**********************/
	/**********************/
	/**********************/
	nfds = select (++nfds, &readMask, &writeMask, NULL, timeoutPtr);

	/* 
	**   No one listening at all ? 
	*/
	if ( nfds == -1 )
	{
	    return 0;
	}

	/*
	** Timeout 
	*/
	if (0 == nfds)
	{
	    if (sepSyncGlobalVars.inSepTool)
	    {
		lookForDeadPids();

		if (0 == sepSyncGlobalVars.socketDriver.stackCount)
		{
		    /** Stop the multiplex **/
		    return 0;
		}
	       continue;
	    }
	    else
	    {
		/*
		** When *debugging*, there are bound to be timing
		** issues if one is single stepping sep. This flag
		** is here to make the situation workable, the
		** child will NOT send sync messages if set.
		*/
		if (!sepSyncGlobalVars.args.childIgnoreTimeOut)
		{
		    /*
		    ** We are in the client
		    */
		    workingStackPtr->local.cmd       = SEPSYNC_CMD_WAITING;
		    workingStackPtr->local.toBeSent  = SYNC_WAIT;
		}
	    }

	    /*
	    ** Restart the multiplex
	    */
	    return TC_RESTART;

	} /** Timeout end-if **/

	/*
	** Working in Client or Server ?
	*/
	if ( ! sepSyncGlobalVars.inSepTool )
	{
	    /*
	    ** CLIENT, First Looks at Sync messages from SEP.
	    */
	    ident = workingStackPtr->sync;

	    if (SOC_ISOPEN(ident))
	    {
		/* 
		** Need to write our sync ? 
		*/
		if (FD_ISSET(ident, &writeMask))
		{
		    infoLocal = &workingStackPtr->local;
		    infoLocal->ident++;
		    infoLocal->toBeSent  -= 1;
		    byteCount = write( ident,
				       infoLocal,
				       sizeof(sepSyncInfoType));

		    if ( SYNC_DONE == infoLocal->toBeSent )
		      return TC_RESTART;
		    else
		      return 0;
		}
		
		/* 
		** Need to read sep's sync ? 
		*/
		if (FD_ISSET(ident, &readMask))
		{
		    infoRemote  = &workingStackPtr->remote;
		    byteCount   = read( ident,
					infoRemote,
					sizeof(sepSyncInfoType));
		    
		    if (infoRemote->cmd == SEPSYNC_CMD_ABORT)
		    {
			/*
			** Though limited, this is why we do it...
			** ~ Sep is insisting child now quits
			*/
			SOC_CLOSE(workingStackPtr->sync);
			SOC_CLOSE(readPtr->data);
			SOC_CLOSE(writePtr->data);
			
			/*
			** Sep is now assuming we are quitting.
			*/
			return TC_EOF;
		    }
		}
	    }
	}
	else
	{
	    /*
	    ** SERVER, Sep should first deal with sep server connection requests
	    */
	    if (SOC_ISOPEN(listenData) && FD_ISSET(listenData,&readMask))
	    {
	      /*
	      ** Too complicated to do inside this loop
	      */
	      serviceIncomingConnection(desc, listenData);

	      /*
	      ** Restart the multiplex
	      */
	      return TC_RESTART;
	    }
	    else if (SOC_ISOPEN(listenSync) && FD_ISSET(listenSync,&readMask))
	    {
	      serviceIncomingConnection(desc, listenSync);

	      /*
	      ** Restart the multiplex
	      */
	      return TC_RESTART;
	    }

	    /*
	    ** Now look through-out sep client stack looking for sync information
	    */
	    for(pos=1; pos<=sepSyncGlobalVars.socketDriver.stackCount; pos++)
	    {
		if (sepSyncGlobalVars.socketDriver.stack[pos].remote.state == SEPSYNC_STATE_CLOSING)
		    continue;

		ident = sepSyncGlobalVars.socketDriver.stack[pos].sync;

		/*
		** Test to see if sync record needs to be sent
		*/
		infoLocal   = &sepSyncGlobalVars.socketDriver.stack[pos].local;
		infoRemote  = &sepSyncGlobalVars.socketDriver.stack[pos].remote;
		if ( infoLocal->toBeSent && SOC_ISOPEN(ident) && (FD_ISSET(ident, &writeMask)) )
		{
		    infoLocal->ident++;
		    infoLocal->toBeSent  = SYNC_DONE;
		    byteCount = write( ident,
				       infoLocal,
				       sizeof(sepSyncInfoType));

		    if (infoLocal->cmd == SEPSYNC_CMD_ABORT)
		    {
			/* 
			** Make sure we return to previous child, if required 
			*/
			sepSyncGlobalVars.socketDriver.stack[pos].abort = 1;   /* local flag  */
			sepSyncGlobalVars.aborted = 1;                         /* global flag */
			popStack(desc);
			return 0;
		    }
		}

		/*
		** Test to see if sync record has arrived 
		*/
		if ( SOC_ISOPEN(ident) && (FD_ISSET(ident, &readMask))  )
		{
		    byteCount   = read( ident,
					infoRemote,
					sizeof(sepSyncInfoType));

		    if (0 == byteCount)
		    {
			/*
			** Suspiciously like other end has closed
			*/
			lookForDeadPids();

			if (0 == sepSyncGlobalVars.socketDriver.stackUsage)
			{
			    return 0;
			}
		    }

		    /*
		    ** Examin this sync message from client
		    */
		    if (SEPSYNC_CMD_OK != infoRemote->cmd)
		    {
			/*
			** Not OK ?, Something is up!
			*/
			if (SEPSYNC_CMD_WAITING == infoRemote->cmd)
			{
			    /*
			    ** Ths client is in a read-loop, and tired of waiting for sep.
			    ** ~ If we too are in a read loop, we have a permanent hang-up 
			    **   for us to break.
			    */
			    if (SEPSYNC_STATE_READING == direction)
			    {
				infoLocal  = &sepSyncGlobalVars.socketDriver.stack[pos].local;
				infoLocal->cmd      = SEPSYNC_CMD_ABORT;
				infoLocal->toBeSent = SYNC_WAIT;
			    }
			}
			else if (SEPSYNC_CMD_ATTENTION == infoRemote->cmd)
			{
			  if (SEPSYNC_STATE_SUSPENDING == infoRemote->state)
			    {
			      if (infoRemote->writeCount == infoLocal->readCount)
				{
				  infoRemote->state = SEPSYNC_STATE_SUSPENDED;
				  pushStack(desc);	
				}
			    }
			}
		    } 
		    else
		    {
			/*
			** Save current direction, if record belongs to working client entry
			*/
			if (workingStackPtr->pid == infoRemote->pid)
			{
			    sepSyncGlobalVars.clientDirection = infoRemote->state;
			}

		    }

		} /** if (ident read active) **/

	    } /** For loop over client stack for sync i/o **/
	}


	/*
	** Any data to be read ?
	*/
	if (SOC_ISOPEN(readPtr->data) && FD_ISSET(readPtr->data, &readMask))
	{
	    /*
	    ** On closing, wait for all data to be recieved, then return EOF
	    */
	    if (  workingStackPtr && 
		 (workingStackPtr->remote.state == SEPSYNC_STATE_CLOSING) &&
		 (workingStackPtr->local.readCount == workingStackPtr->remote.writeCount)
		)
	    {
		    return TC_EOF;
	    }

	    if (readPtr->putPos < readPtr->getPos)
	    {
		/* 
		** Allow read upto getPos edge
		*/
		byteCount = read(readPtr->data,
				 &readPtr->buffer[readPtr->putPos],
				 readPtr->getPos - readPtr->putPos);
	    }
	    else
	    {
		/* 
		** Allow read upto buffer edge
		*/
		byteCount = read(readPtr->data,
				 &readPtr->buffer[readPtr->putPos],
				 SEPBUFFERSZ - readPtr->putPos);
	    }

	    if ( 0 == byteCount )
	    {
		return TC_EOF;
	    }

	    if ( byteCount  > 0 )
	    {
		workingStackPtr->local.readCount += byteCount;

		readPtr->count  += byteCount;
		readPtr->putPos += byteCount;

		if (SEPBUFFERSZ == readPtr->putPos)
		{
		    readPtr->putPos = 0;
		}

		/* 
		** Always check for delayed input when ready to sync
		*/
		if (SEPSYNC_STATE_SUSPENDING == workingStackPtr->remote.state)
		  {
		    if (workingStackPtr->remote.writeCount == workingStackPtr->local.readCount)
		      {
			infoRemote->state = SEPSYNC_STATE_SUSPENDED;
			pushStack(desc);	
		      }
		  }
	    }
	}

	/*
	** Enough room on device for a write ?
	*/
	if (SOC_ISOPEN(writePtr->data) && FD_ISSET(writePtr->data, &writeMask))
	{
	    if ( writePtr->getPos < writePtr->putPos )
	    {
		byteCount = write(writePtr->data,
				  &writePtr->buffer[writePtr->getPos],
				  writePtr->count);

		if ( byteCount <= 0 )
		{
		  return TC_EOF;
		}

		writePtr->count  -= byteCount; 
		if ( 0 < writePtr->count )
		{
		    writePtr->getPos += byteCount;
		}
		else
		{
		    /*
		    ** Nothing in buffer, so entitled to reset for convenience 
		    */
		    writePtr->putPos = 0;
		    writePtr->getPos = 0;
		    writePtr->count  = 0;
		}
	    }
	    else
	    {
		byteCount = write(writePtr->data,
				  &writePtr->buffer[writePtr->getPos],
				  SEPBUFFERSZ - writePtr->getPos);


		writePtr->count  -= byteCount;
		writePtr->getPos += byteCount;
		if (writePtr->getPos == SEPBUFFERSZ)
		    writePtr->getPos = 0;
	    
	    }
	    workingStackPtr->local.writeCount += byteCount;
	}

	/*
	** Had enough ?
	*/
	if ((direction == 'r') && (readPtr->count > 0))
	{
	    /* 
	    ** Something in the buffer ready for reading 
	    ** ~ We need a sync-count to tell us if partner has a little more waiting ??
	    */
	    return 0;
	}
	else if ((direction == 'w') && (0 == writePtr->count))
	{
	    /*
	    ** Nothing left to write
	    */
	    return 0;
	}

    } /** while (1) **/

    return 0;
}

/*
**  Procedure:   sepMultiplex
**
**  Descripton:
**        A wrapper for sepMultiplexRestart() above.
**        Allowing us to restart the multiplex from scratch
**        as often as required.
*/
static i4 
sepMultiplex(TCFILE *desc, i4  withTimeout)
{
  i4  result;

  /* 
  ** Put code in a subroutine to allow a return 'restart'
  ** to break out of nested while/if constructs.
  */
  while ( TC_RESTART == (result = sepMultiplexRestart(desc, &withTimeout)));

  return result;
}

/*
** Procedure: readSepSyncChar
**
** Description:
**     We have a TCFILE descriptor, which is from the old SEP stuff.
**     We intend to convert it into the Socket driven stuff.
**     This is part of the glue that ensures we can maintain the 
**     SEP/child interface, and keep the old style code hanging
**      in there
*/
static i4  
readSepSyncChar(TCFILE *desc,i4  timeout)
{
    i4  output = TC_TIMEDOUT;

    sepDataBufferType *readPtr = &(desc->ipcBuffer->reader);

    if ( 0 == readPtr->count )
    {
	/* 
	** Fetch more data
	*/
	if ( TC_EOF == sepMultiplex(desc, timeout) )
	{
	    popStack(desc);
	    return TC_EOF;
	}
    }

    if (readPtr->count)
    {
	/* 
	** Take out of buffer 
	*/
	output = readPtr->buffer[readPtr->getPos++];
	
	readPtr->count--;
	
	if ( readPtr->count <= 0 )
	{
	    /* Reset buffer */
	    readPtr->count  = 0;
	    readPtr->getPos = 0;
	    readPtr->putPos = 0;
	}
	else if ( readPtr->getPos == SEPBUFFERSZ )
	{
	    /* Wrap */
	    readPtr->getPos = 0;
	}
    }

    return(output);
}

/*
** Procedure: WriteSepSyncChar
**
** Description:
**     We have a TCFILE descriptor, which is from the old SEP stuff.
**     We intend to convert it into the Socket driven stuff.
**     This is part of the glue that ensures we can maintain the 
**     SEP/child interface, and keep the old style code hanging
**      in there
*/
static void
writeSepSync(i4  character, TCFILE *desc)
{
    sepDataBufferType *writePtr = &(desc->ipcBuffer->writer);

    /* Check for room */
    if ( writePtr->count == SEPBUFFERSZ )
    {
	/* 
	** MUST flush first 
	** - Somewhat suprising
	*/
	sepMultiplex(desc, 0);
    }

    writePtr->buffer[writePtr->putPos++] = (unsigned char)character;	
    if ( writePtr->putPos == SEPBUFFERSZ )
	writePtr->putPos = 0;

    writePtr->count++;
}

/*
** Procedure
**      WriteSepSyncChar
** Description 
**      A write and flush combined
*/
static STATUS
WriteSepSyncChar(i4  character, TCFILE *desc)
{
    writeSepSync(character, desc);

    /*
    ** Flush, so as to keep child running
    */
    sepMultiplex(desc, 0);

    return(OK);
}

/*
** Procedure FlushsepSync
**
** Description
**     To ensure any waiting data is writen to stream.
*/
static void
FlushsepSync(TCFILE *desc)
{
    /*
    ** Flush, so as to keep child running
    */
    if (desc->ipcBuffer->writer.count) 
	sepMultiplex(desc, 0);
}

/*
** Procedure writeSepSyncString
**
** Description
**     To write a C string to the stream.
*/
static void
writeSepSyncString(unsigned char *string, TCFILE *desc)
{
    while ( *string )
    {
	writeSepSync(*(string++), desc);
    }
	
    /*
    ** Flush, so as to keep child running
    */
    sepMultiplex(desc, 0);
}

/*
** Procedure:  TCtransfer
**
** Description:
**
**        We are near the bottom of the stack , so fetch as much
**        as we can by looking into the buffer for sensitive characters
**        that will cause a break out.
** History:
**      10-Feb-2011 (coomi01) b125021
**            Read low level pragma no timeout action
*/
i4
TCtransfer(TCFILE *desc, FILE *outFile, i4 timeout, i4 *prevCh, i4 *result)
{
    i4 chr = TC_TIMEDOUT;
    char transBuffer[SEPBUFFERSZ];
    i4   pos = 0;
    i4   escape = 0;
    i4   sent;
    sepDataBufferType *readPtr = &(desc->ipcBuffer->reader);

    /* Transfer all but these characters, which have special handling, to output file
       TC_EOF        ~ -1
       TC_TIMEDOUT   ~ -2
       TC_EOQ        ~ 28
       TC_EQR        ~ 29
       TC_BOS        ~ 30
       BACKSLASH     ~ '\'
       ESCAPED       > 127
    */
    if (  0 == readPtr->count )
    {
	if (sepSyncGlobalVars.pragmaFlags & SEPSYNC_NOTIMEOUT_1)
	{
	    sepSyncGlobalVars.pragmaFlags &= ~SEPSYNC_NOTIMEOUT_1;
	    *result = TC_TIMEDOUT;
	    return 0;
	}

	/* 
	** Fetch more data
	*/
	if ( TC_EOF == sepMultiplex(desc, timeout) )
	{
	    popStack(desc);
	    *result = TC_EOF;
	    return 1;
	}
    }

    if ( 0 == readPtr->count )
    {
	*result = TC_TIMEDOUT;
	return 0;
    }

    while ( readPtr->count > 0 )
    {
	chr = readPtr->buffer[readPtr->getPos++];
	readPtr->count--;
	if ( SEPBUFFERSZ == readPtr->getPos )
	{
	    /* Wrap */
	    readPtr->getPos = 0;
	}

	if ( (TC_BOS+1) > chr )
	{
	    /* Chars with extra special meaning to sep */
	    if ( 0 > chr || TC_BOS == chr || TC_EOQ == chr )
	    {
		escape = 1;
		break;
	    }

	    if ( chr == TC_EQR )
	    {
		escape = 1;
		break;
	    }
	} 
	else if ( chr > ('\\'-1) )
	{
	    /* Top bit set chars have to be escaped */
	    if ( chr > 0x7F || chr == '\\' )
	    {
		escape = 1;
		break;
	    }
	}

	/* Save current */
	transBuffer[pos++] = chr;
    }

    if ( 0 == readPtr->count )
    {
	/* Reset buffer */
	readPtr->count  = 0;
	readPtr->getPos = 0;
	readPtr->putPos = 0;
    }

    if ( pos )
    {
	/* 
	** We have buffered up some stuff, transfer it out
	*/
	if (sepSyncGlobalVars.showLine)
	{
	    transBuffer[pos] = 0;
	    showLine(SHOW_LINE_SYNC,(PTR)&transBuffer[0]);
	}

	/* Now to the real file */
	SIwrite(pos, (PTR)&transBuffer[0], &sent, outFile);
    }

    *result = chr;
    if ( escape )
    {
	if ( pos )
	    *prevCh = transBuffer[pos-1];
    }
    else
    {
	/*
	** Call again until we get an escape, or time-out 
	*/
	*result = TC_CONTINUE;
    }

    /* 
    ** return number of characters saved 
    */
    return pos;
}

/*
** Procedure: TCinit
**
** Description: 
**
**      Called once by the Sep client to set IPC up.
**      Environment variables are used to configure the client
**      First we look to see if there is any debug assistance to switch on.
**      Then we look to see where Sep is listening on it Data & Sync Ports,
**      calling createOutgoingConnection() to set them up. Note, this is 
**      being done outside of the core multiplex which isn't up and running
**      yet.
*/
static void
TCinit()
{
    char *envString   = NULL;
    char *pipeDetails = NULL;
    char *comma;
    char *envInteger  = NULL;

    sepSyncGlobalVars.TCInitDone = 1;

    /* Clone the debug data */
    sepSyncGlobalVars.gdb = 0;
    NMgtAt(SEPDBGMSG, &envString);

    if ( envString && *envString )
    {
	sepSyncGlobalVars.gdb = 1;
	sepSyncGlobalVars.gdbStr = strdup(envString);
	
	extGdbFlag  = 0;

	/*
	** If you run sep with the switch '-Gchild', 
	** the child will sleep waiting for you to attach
	** with a debugger and set it going again.
	*/
	if ( 0 == STcompare(sepSyncGlobalVars.gdbStr, "child") )
	{
	    extGdbFlag  = 1;
	    checkGdb(0,0);
	}
    }

    sepSyncGlobalVars.childSyncMode = 0;
    sepSyncGlobalVars.childTimeOut  = 10;
    sepSyncGlobalVars.args.childIgnoreTimeOut = FALSE;

    NMgtAt(SEPCLITIME, &envInteger);
    if ( envInteger && *envInteger )
    {
	sepSyncGlobalVars.childTimeOut = atoi(envInteger);
	if ( -1 == sepSyncGlobalVars.childTimeOut )
	{
	    sepSyncGlobalVars.args.childIgnoreTimeOut = TRUE;
	    sepSyncGlobalVars.childTimeOut = 10;
	}
    }

    NMgtAt(SEPSOCKETCONN,&envString);
    if ( envString && *envString )
    {
	/* 
	** Connect to data handler 
	*/
	NMgtAt(SEPSOCKETDATA, &envInteger);
	if ( envInteger && *envInteger )
	{
	    /*
	    ** Fixed stack entry point for **clients**
	    */
	    sepSyncGlobalVars.socketDriver.stackCount = 1;
	    sepSyncGlobalVars.socketDriver.stackTop   = &sepSyncGlobalVars.socketDriver.stack[1];

	    sepSyncGlobalVars.childInputOverRide  = createOutgoingConnection(0, envInteger);
	    sepSyncGlobalVars.childOutputOverRide = sepSyncGlobalVars.childInputOverRide;

	    /* 
	    ** Connect to sync handler 
	    */
	    NMgtAt(SEPSOCKETSYNC, &envInteger);
	    if ( envInteger && *envInteger )
	    {
		sepSyncGlobalVars.childSyncOverRide   = createOutgoingConnection(1, envInteger);
		sepSyncGlobalVars.childSyncMode     = _TCUSESOCK;
	    }
	}

    }
}

/*
** Procedure: TCSyncWait
**
** Description:
**
**     Tell Sep we are about fork a new child
**     This is important. We want sep to read all of our
**     output before it starts to read from our would-be
**     child. Happens that the sync record contains 
**     details of our read/write totals.
*/
void 
TCSyncWait(i4 waiting)
{
    sepSyncDetailsType *wrkingStackPtr;

    if ( sepSyncGlobalVars.childSyncMode )
    {
	wrkingStackPtr = sepSyncGlobalVars.socketDriver.stackTop;
	wrkingStackPtr->local.cmd      = SEPSYNC_CMD_ATTENTION;
	wrkingStackPtr->local.state    = waiting ? SEPSYNC_STATE_SUSPENDING : SEPSYNC_STATE_RESUME;
	wrkingStackPtr->local.toBeSent = SYNC_NOWAIT;
	sepMultiplex(wrkingStackPtr->iobuffs->writer.TCHandle, 0);
	wrkingStackPtr->local.toBeSent = SYNC_DONE;
    }
}

/*
**  Procedure: TCDisconnect
**
**  Description:
**       To shutdown any active streams.
**       Called by top level sep durring the abort process.
*/
void 
TCDisconnect(i4  SEPsonpid)
{
    i4       code;
    i4       loop;
    i4      myPid;
    i4     result;
    i4     status;
    i4  targetPid;

    sepSyncDetailsType *currentStackPtr = sepSyncGlobalVars.socketDriver.stackTop;
    sepSyncDetailsType *wrkingStackPtr;

    myPid = getpid();

    /*
    ** Walk down the stack, telling children to shutdown.
    */
    for (loop=sepSyncGlobalVars.socketDriver.stackCount; loop>=1; loop--)
    {
	wrkingStackPtr = &sepSyncGlobalVars.socketDriver.stack[loop];
	sepSyncGlobalVars.socketDriver.stackTop = wrkingStackPtr;

	targetPid = wrkingStackPtr->pid;

	if (-1 == targetPid || myPid == targetPid )
	    continue;

	/*
	** Test for exit
	*/
	result = waitpid(targetPid, &status, WNOHANG);

	if (targetPid == result) 
	{

	    /* 
	    ** Has Now Quit 
	    */
	    wrkingStackPtr->pid = -1;
	    if ( targetPid == SEPsonpid )
	    {
		/*
		** Nothing to do
		*/
		SEPsonpid = 0;
	    }

	}  else {

	    /* 
	    ** No Change, ie running, Send SyncAbort
	    */
	    wrkingStackPtr->local.cmd      = SEPSYNC_CMD_ABORT;
	    wrkingStackPtr->local.toBeSent = SYNC_NOWAIT;
	    sepMultiplex(wrkingStackPtr->iobuffs->writer.TCHandle, 0);
	    wrkingStackPtr->local.toBeSent = SYNC_DONE;
	} 
    }

    if (0 != SEPsonpid)
    {
	result = waitpid(SEPsonpid, &status, WNOHANG);
    }
}

/*
** Procedure TCclose
**
** Decscription:
**
**         First note, this function is part of the traditional TC interface.
**
**         (a) The definition of the original TCclose is intact and near the
**             bottom of this file.
**
**         (b) Below, we have a #define that renames the original function to  
**             Orig_TCClose(). If Sep is not using sockets, we can call the 
**             original function to do the tradtional job.
**
**         (c) I did it this way in order to be able to compile-out my changes
**             or pick and choose which to use at run time. ie for flexibility.
**
**         TCclose shuts down the interface.
*/
STATUS
TCclose(TCFILE	*desc)
{
    if (desc->_flag & _TCUSESOCK)
    {
	if (desc->_flag & _TCWRT )
	{
	    /** Reader/writer same socket, close once **/
	    if ( SOC_ISOPEN(desc->ipcBuffer->writer.data) )
	    {	    
		/*
		** Should do TCputc(TC_EOF, desc);
		** But send a sync-close instead
		*/
		sepSyncGlobalVars.socketDriver.stackTop->local.cmd         = SEPSYNC_CMD_ATTENTION;
		sepSyncGlobalVars.socketDriver.stackTop->local.state       = SEPSYNC_STATE_CLOSING;
		sepSyncGlobalVars.socketDriver.stackTop->local.toBeSent    = SYNC_NOWAIT;
		sepMultiplex(desc,0);
		
		/*
		** Now let go
		*/
		SOC_CLOSE(desc->ipcBuffer->writer.data);
		SOC_CLOSE(desc->ipcBuffer->writer.sync);

		desc->ipcBuffer->writer.data = SOC_NOTSET;
		desc->ipcBuffer->writer.sync = SOC_NOTSET;
		desc->ipcBuffer->reader.data = SOC_NOTSET;
		desc->ipcBuffer->reader.sync = SOC_NOTSET;
	    }
	}	
	MEfree((PTR)desc);
    }
    else
    {
	return Orig_TCClose(desc);
    }
}


/*
** Procedure TCopen
**
** Decscription:
**
**         First note, this function is part of the traditional TC interface.
**
**         (a) The definition of the original TCopen is intact and near the
**             bottom of this file.
**
**         (b) Below, we have a #define that renames the original function to  
**             Orig_TCOpen(). If Sep is not using sockets, we can call the 
**             original function to do the tradtional job.
**
**         TCopen starts up the interface.
*/
STATUS
TCopen(LOCATION *location, char *mode, TCFILE **desc)
{
    STATUS result = OK;
    i4     flag;
    i4     pid;
    sepSyncBufferType *ipcBuffer;

    /* 
    ** Look to see if child needs initialisation.
    */
    if ( ! (sepSyncGlobalVars.inSepTool || sepSyncGlobalVars.TCInitDone) )
    {
	/*
	** Called by child only 
	** ~ This is where the child will decide to start 
	**   behaving differently when in traditional or sync mode.
	*/
	TCinit();
    }

    /*
    ** Parent uses 'syncMode', and child uses 'childSyncMode'
    ** to flag the sync mode is in use.
    */
    if ( sepSyncGlobalVars.syncMode  || sepSyncGlobalVars.childSyncMode )
    {
  	if (*mode == 'r')
	{
	    flag = _TCREAD;
	}
	else if (*mode == 'w')
	{
	    flag = _TCWRT;
	}
	else
	{
	    return(TC_BAD_SYNTAX);
	}

	/* 
	** Allocate basic TC block 
	*/
	*desc = (TCFILE *) IIMEreqmem(0, sizeof(TCFILE), TRUE, &result);
	if (result != OK)
	    return(FAIL);

	pid = getpid();
	ipcBuffer = sepSyncGlobalVars.ipcBuffers;

	while ( ipcBuffer && (ipcBuffer->ownerPid != pid))
	{
	    ipcBuffer = ipcBuffer->next;
	}

	if (!ipcBuffer)
	{
	    /* Allocate substructure */
	    ipcBuffer = (sepSyncBufferType *) IIMEreqmem(0, 
							sizeof(sepSyncBufferType), 
							TRUE, 
							&result);

	    ipcBuffer->ownerPid = pid;
	    ipcBuffer->next     = sepSyncGlobalVars.ipcBuffers;
	    sepSyncGlobalVars.ipcBuffers = ipcBuffer;
	    sepSyncGlobalVars.socketDriver.stack[0].iobuffs = ipcBuffer;
	    sepSyncGlobalVars.socketDriver.stack[1].iobuffs = ipcBuffer;
	}

	(*desc)->ipcBuffer = ipcBuffer; 
	(*desc)->direction = *mode;
	(*desc)->connected = 0;

	/* 
	** Create a pipe 
	** ~ Or utilize an override, aka:-
	**   The child process does not create pipes, 
	**   it is passed the pipe ends via the environment. 
	*/
	if (sepSyncGlobalVars.childInputOverRide && (flag & _TCREAD))
	{
	    /* 
	    ** IPC was created by parent (sep) we only need to attach 
	    */
	    ipcBuffer->reader.TCHandle        = *desc;
	    ipcBuffer->reader.data            = sepSyncGlobalVars.childInputOverRide;
	    ipcBuffer->reader.sync            = sepSyncGlobalVars.childSyncOverRide;
	    sepSyncGlobalVars.childInputOverRide  = 0;

	    flag = flag | sepSyncGlobalVars.childSyncMode;
	}
	else if (sepSyncGlobalVars.childOutputOverRide && (flag & _TCWRT))
	{
	    /* 
	    ** IPC was created by parent (sep) we only need to attach 
	    */
	    ipcBuffer->writer.TCHandle            = *desc;
	    ipcBuffer->writer.data                = sepSyncGlobalVars.childOutputOverRide;
	    ipcBuffer->writer.sync                = sepSyncGlobalVars.childSyncOverRide;
	    sepSyncGlobalVars.childOutputOverRide = 0;

	    flag = flag | sepSyncGlobalVars.childSyncMode;
	}
	else if (flag & _TCREAD)
	{
	    /*
	    ** This is done in the parent sep process
	    */
	    ipcBuffer->reader.TCHandle = *desc;
	    flag = flag | sepSyncGlobalVars.syncMode;
	}
	else if (flag & _TCWRT)
	{
	    /*
	    ** This is done in the parent sep process
	    */
 	    ipcBuffer->writer.TCHandle = *desc;


	    flag = flag | sepSyncGlobalVars.syncMode;
	}

	/*
	** Set Flags 
	*/
	(*desc)->_flag = flag | _TCOPEN;

	return result;
    }
    else
    {
	result = Orig_TCOpen(location, mode, desc);
	if ( result == OK)
	{
	    (*desc)->ipcBuffer = NULL;
	}
	return result;
    }
}

/*
** Procedure TCflush
**
** Decscription:
**
**         First note, this function is part of the traditional TC interface.
**
**         Looks to see if this is a ipc streams version of the interface is active,
**         then calls the SepSync version, or the traditional version accordingly.
*/
STATUS
TCflush(TCFILE *desc)
{
    if ( (desc->_flag & _TCUSEIPC) )
    {
	FlushsepSync(desc);
	return OK;
    }
    else
    {
	return Orig_TCFlush(desc);
    }
}

STATUS
TCputc(i4 achar,TCFILE *desc)
{
    i4   startOfCmd;
    char buff[200];
    i4  i;

    startOfCmd = 0;
    if (TC_TIMEDOUT == achar)
    {
	startOfCmd = 1;
	achar = TC_EOQ;
    }

    /*
    ** Detect Ipc Stuff 
    */
    if ( (desc->_flag & _TCUSEIPC) && 
	 (desc->_flag & _TCWRT)) 
    {
	return WriteSepSyncChar(achar, desc);
    }
    else
    {
	return ( Orig_TCPutc(achar, desc) );
    }
}

/*
** Procedure TCempty
**
** Decscription:
**
**         First note, this function is part of the traditional TC interface.
**
**         Looks to see if this is a ipc streams version of the interface is active,
**         then tests for , or the traditional version accordingly.
*/
bool
TCempty(TCFILE *desc)
{
    /* If this is a write (not a read) descriptor, 
    ** do as original ... always return true
    */
    if (!(desc->_flag & _TCREAD))
	return(TRUE);

    if (desc->_flag & _TCUSESOCK)
    {
	return desc->ipcBuffer->reader.count != 0;
    }
    else
    {
	return Orig_TCEmpty(desc);
    }
}

i4
TCgetc(TCFILE *desc,i4 seconds)
{
    i4 result;
    char buff[200];
    i4  i;

    /*
    ** Detect Sync Stuff 
    */
    if ( (desc->_flag & _TCUSEIPC) && 
	 (desc->_flag & _TCREAD)) 
    {
	result = readSepSyncChar(desc, seconds);
    }
    else
    {
	result = Orig_TCGetc(desc,seconds);
    }

    return ( result );
}


/*
** When activated, ie SEP_SYNC defined, the traditional 
** routines i/o below are renamed. This enables us to 
** compile SEP_SYNC in or out.
*/
#define SEP_SYNC_HIDES static

#define TCputc  Orig_TCPutc
#define TCgetc  Orig_TCGetc
#define TCopen  Orig_TCOpen
#define TCclose Orig_TCClose
#define TCflush Orig_TCFlush
#define TCempty Orig_TCEmpty
#else /** SEP_SYNC */

#define SEP_SYNC_HIDES /* no it doesn't */

#endif /** SEP_SYNC */

/*
** Now follows the original coding, completely intact
** except for the SEP_SYNC_HIDES macro, which make these
** routines static when SEP_SYNC is defined. 
*/


/*
**
**  NAME
**
**      TCopen()
**
**  FUNCTIONAL DESCRIPTION:
**
**      Opens a COMM-file.
**
**  FORMAL PARAMETERS:
**
**      location        - location of the file
**      mode            - access mode of the file, possible values are
**                        "w" - open file for writing
**                        "r" - open file for reading 
**      desc            - value to get file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**      	OK              the file was properly opened,
**      	TC_BAD_SYNTAX   illegal parameter was passed
**      	FAIL            file couldn't be opened
**
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**	22-may-89 (mca)		Initial version.
**      30-Aug-95 (fanra01)
**          Added NT_GENERIC sections for file creation and open in binary mode.
**          File types are important on the desktop platforms.  Using text
**          mode does not guarentee the number of bytes written to the file
**          because of carriage return insertion.
*/


SEP_SYNC_HIDES STATUS
TCopen(location, mode, desc)
LOCATION	 *location;
char		 *mode;
TCFILE		**desc;
{
	char	    *filename;
	i4 	     fileflags;
	STATUS	     status;

	if ( (*mode != 'r') && (*mode != 'w') )
	{
		return(TC_BAD_SYNTAX);
	}

	LOtos(location,&filename);

	if (filename == NULL || *filename == '\0')
		return(FAIL);

	/* allocate space for file structure */

	*desc = (TCFILE *) MEreqmem(0, sizeof(**desc), TRUE, &status);
	if (status != OK)
		return(FAIL);

	STcopy(filename, (*desc)->_fname);

	(*desc)->_flag = _TCOPEN;
	(*desc)->_size = 0;

	/* Test for existence of the file. If we end up being the one who
	   created it, we'll have to unlink it later.
	*/
	if (access(filename, F_OK) != 0)
		(*desc)->_flag |= _TCCREAT;

	/* Open the file. */
	fileflags = (*mode == 'r' ? O_RDONLY : O_WRONLY) | O_CREAT;
	(*desc)->_id = open(filename, fileflags, 0666);
	if ((*desc)->_id < 0)
	{
		MEfree( (PTR) *desc);
		return(FAIL);
	}

	/* Initialize the buffer pointers. */
	(*desc)->_pos = (*desc)->_buf;

	if (*mode == 'r')
	{
		struct stat st;
		i4 ZERO = 0;
		char *dot;

		/* Create synchronization file. */
		STcopy(filename, (*desc)->_sync_fname);
		if ( (dot = STrchr( (*desc)->_sync_fname, '.') ) != NULL)
			STcopy(".syn", dot);
		else
			STcat( (*desc)->_sync_fname, ".syn");
		(*desc)->_sync_id = open((*desc)->_sync_fname, O_RDWR|O_CREAT,
		  0666);
		if ((*desc)->_sync_id < 0)
		{
			MEfree( (PTR) *desc);
			return(FAIL);
		}
		/* Initialiaze synch. file. */
		fstat( (*desc)->_sync_id, &st);
		if (st.st_size == 0)
		{
			write( (*desc)->_sync_id, &ZERO, sizeof(ZERO) );
		}

		(*desc)->_flag |= _TCREAD;
		(*desc)->_end = (*desc)->_pos;
	}
	else  
	{
		/* Since we may not be the first writer to attach to this
		   TCFILE, we have to expliticly go to the end.
		*/
		lseek( (*desc)->_id, 0, L_XTND);
		(*desc)->_flag |= _TCWRT;
		(*desc)->_end = (*desc)->_buf + TCBUFSIZE;
	}

	return(OK);
}  /* TCopen */



/*
**++
**  NAME
**
**      TCputc()
**
**  FUNCTIONAL DESCRIPTION:
**
**      Writes a character to a COMM-file. 
**      Data written to a COMM-file is sequenced in order of arrival.
**      Data will be appended to a buffer, after buffer limit is reached
**	data will be flushed to the file. 
**      Client code may make no assumptions about when and whether the TCputc
**      routine will block. An implementation is free to make TCputc block on
**      the second TCputc call if the previous character has not been read. 
**
**  FORMAL PARAMETERS:
**
**      achar           - char to be written to COMM-file
**      desc            - file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**       OK             char was properly appended to file,
**
**       FAIL           char could not be appended to file.
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**	22-may-89 (mca)		Initial version.
*/

SEP_SYNC_HIDES STATUS
TCputc(achar,desc)
i4	achar;
TCFILE	*desc;
{
	if (!(desc->_flag & _TCWRT))
	    return(FAIL);

	if (desc->_pos >= desc->_end)
	{
	    TCflush(desc);
	}

	if ((achar != (i4) ((uchar) achar)) || (achar == TC_ESCAPE))
	{
		*desc->_pos++ = TC_ESCAPE;

		if (desc->_pos >= desc->_end)
		{
		    TCflush(desc);
		}
	}
	*desc->_pos++ = (uchar) achar;

	return(OK);
} /* TCputc */



/*
**++
**  NAME
**
**      TCflush()
**
**  FUNCTIONAL DESCRIPTION:
**
**      sends write buffer to given COMM-file. 
**
**  FORMAL PARAMETERS:
**
**      desc            - file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**       OK             data in write buffer was flushed,
**
**       FAIL           flush could not be done.
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**	22-may-89 (mca)		Initial version.
*/

SEP_SYNC_HIDES STATUS
TCflush(desc)
TCFILE	*desc;
{
	i4 bytesToWrite;

	if (!desc || !(desc->_flag & _TCWRT))
		return(FAIL);

	bytesToWrite = desc->_pos - desc->_buf;
	if (bytesToWrite > 0)
	{
		/* Since we might be sharing this TCFILE with another writer,
		   we have to make sure we're really at the end of the file.
		*/
		lseek(desc->_id, 0, L_XTND);
		if (write(desc->_id, desc->_buf, bytesToWrite) != bytesToWrite)
			return(FAIL);
	}

	desc->_pos = desc->_buf;
	return(OK);
}  /* TCflush */



/*
**++
**  NAME
**
**      TCgetc()
**
**  FUNCTIONAL DESCRIPTION:
**
**      Reads a character from a COMM-file. If the COMM-file is empty,
**      it will wait until at least one byte of data is available.
**      If the process writing to the file closes it, TCgetc will return
**      TC_EOF.
**      The TCgetc routine supports a timeout feature. If a given number of 
**      seconds has passed while the TCgetc routine is waiting for data to
**      be available, the TC_TIMEDOUT code will be returned by TCgetc.
**      If the given number of seconds is < 0, TCgetc will wait until data
**      is available.
**
**  FORMAL PARAMETERS:
**
**      desc            - file pointer
**      seconds         - number of seconds to wait for data
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**       next character available in the COMM-file, or
**
**       TC_EOF if process writing to the file closes it, or
**
**       TC_TIMEDOUT if TCgetc has been waiting for a given number of seconds
**                   ans still no data is available
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**	22-may-89 (mca)		Initial version.
*/
 
static bool doescape = TRUE;

SEP_SYNC_HIDES i4
TCgetc(desc,seconds)
TCFILE	*desc;
i4	seconds;
{
	i4	toreturn;


	if (!(desc->_flag & _TCREAD))
		return(TC_EOF);

	if (desc->_pos < desc->_end)
		toreturn = *desc->_pos++;
	else
	{
		i4  bytesToRead, ret_val;
		i4 start_time, now_time, cur_size;
		struct stat statbuf;

		/* Retrieve the last i4  written to the synch file. This
		   tells the file address of the last byte read from the
		   TCFILE.
		*/
#if defined(sgi_us5) || defined(LNX) || \
    defined(a64_sol) || defined(OSX)
{
		off_t  real_offset;
		real_offset = (-1 * (off_t)sizeof(i4));
		lseek(desc->_sync_id, real_offset, L_XTND);
}
#else
		lseek(desc->_sync_id, sizeof(i4)*-1, L_XTND);
#endif
		read(desc->_sync_id, &cur_size, sizeof(i4) );
		/* If we aren't the most recent reader, seek to the first
		   unread byte.
		*/
		if (cur_size != desc->_size)
		{
			desc->_size = cur_size;
			lseek(desc->_id, (off_t)cur_size, L_SET);
		}
		start_time = TMsecs();
		while (1)
		{
			if (fstat(desc->_id, &statbuf) < 0)
				return(TC_EOF);
			if (statbuf.st_size > desc->_size)
				break;
			now_time = TMsecs();
			if ( (seconds > 0)
			  && ( (now_time - start_time) > seconds) )
				return(TC_TIMEDOUT);
			II_nap(1000);
		}
		bytesToRead = statbuf.st_size - desc->_size;
		if (bytesToRead > TCBUFSIZE)
			bytesToRead = TCBUFSIZE;
		if ((ret_val = read(desc->_id, desc->_buf, bytesToRead)) <= 0)
			return(TC_EOF);
		if (ret_val != bytesToRead)
			SIprintf("TCgetc: tried to read %d bytes, but read() returned %d.\n",
			  bytesToRead, ret_val);
		desc->_end = desc->_buf + ret_val;
		desc->_pos = desc->_buf + 1;
		desc->_size += ret_val;
		/* Update the synch file. */
		write(desc->_sync_id, &desc->_size, sizeof(i4) );
		toreturn = *desc->_buf;
	}
	if (doescape && toreturn == TC_ESCAPE)
	{
	    doescape = FALSE;
	    switch(toreturn = TCgetc(desc, 0))
	    {
		case TC_ESCAPE:
		    break;
		default:
		    toreturn = -1 - ( (i4)((uchar)-1) - ((uchar)toreturn));
		    break;
	    }
	    doescape = TRUE;
	}
	return(toreturn);
}  /* TCgetc */



/*
**++
**  NAME
**
**      TCclose()
**
**  FUNCTIONAL DESCRIPTION:
**
**      closes a COMM-file.
**
**  FORMAL PARAMETERS:
**
**      desc            - file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**      OK      file was properly closed,
**
**      FAIL    couldn't close file.
**
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**	22-may-89 (mca)		Initial version.
**	05-sep-95 (allst01/smiba01)
**		MUST check for NULL ptr
*/

/* This global will always be FALSE in INGRES frontends. In SEP, it will
** immediately be set to TRUE at startup.
*/
GLOBALREF bool IITCmaster;

SEP_SYNC_HIDES STATUS
TCclose(desc)
TCFILE	*desc;
{
	if (!desc || !(desc->_flag & _TCOPEN))
		return(FAIL);


	if (desc->_flag & _TCWRT)
	{
		TCputc(TC_EOF, desc);
		TCflush(desc);
		if (IITCmaster)
		{
		    char *dot;

		    STcopy(desc->_fname, desc->_sync_fname);
		    if ( (dot = STrchr(desc->_sync_fname, '.') ) == NULL)
			STcat(desc->_sync_fname, ".syn");
		    else
			STcopy(".syn", dot);
		    unlink(desc->_sync_fname);
		}
	}
	close(desc->_id);

	if (desc->_flag & _TCREAD)
	{
		close(desc->_sync_id);
		if (desc->_flag & _TCCREAT)
			unlink(desc->_sync_fname);
	}

	if (desc->_flag & _TCCREAT)
		unlink(desc->_fname);

	MEfree((PTR)desc);

	return(OK);
}



/*
**++
**  NAME
**
**      TCempty()
**
**  FUNCTIONAL DESCRIPTION:
**
**      Check if there is unread data in a COMM-file.
**
**  FORMAL PARAMETERS:
**
**      desc            - file pointer
**
**  IMPLICIT INPUTS:
**
**      none
**
**  IMPLICIT OUTPUTS:
**
**      none
**
**  FUNCTION VALUE:
**
**      The function returns:
**
**      TRUE    there is no unread data in the COMM-file,
**
**      FALSE   there is unread data in the file.
**
**
**  SIDE EFFECTS:
**
**      none
**
**  History:
**	22-may-89 (mca)		Initial version.
*/

SEP_SYNC_HIDES bool
TCempty(desc)
TCFILE	*desc;
{
	if (!(desc->_flag & _TCREAD))
		return(TRUE);

	if (desc->_pos < desc->_end)
		return(FALSE);
	else
	{
		struct stat statbuf;

		if (fstat(desc->_id, &statbuf) < 0)
		{
			desc->_pos = desc->_buf;
			*desc->_pos = TC_EOF;
			desc->_end = desc->_pos + 1;
			return(FALSE);
		}
		/* If it appears that there are new bytes to read in the
		   TCFILE, check the snch file to make sure someone else
		   hasn't already read them.
		*/
		if (statbuf.st_size > desc->_size)
		{
#if defined(sgi_us5) || defined(a64_sol) || \
    defined(LNX) || defined(OSX)
{
			off_t  real_offset;
			real_offset = (-1 * (off_t)sizeof(i4));
			lseek(desc->_sync_id, real_offset, L_XTND);
}
#else
			lseek(desc->_sync_id, sizeof(i4)*-1, L_XTND);
#endif
			read(desc->_sync_id, &desc->_size, sizeof(i4) );
		}
		return(statbuf.st_size == desc->_size);
	}
}
