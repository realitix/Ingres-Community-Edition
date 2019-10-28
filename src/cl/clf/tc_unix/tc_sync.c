/*
** Copyright (c) 2010 Actian Corporation
*/

/*
** TC_SYNC.C
**
**     This file contains support, setup and debug, routines for the associated
**     'sepsync' changes in tc.c.  Importantly, the sepsync global variable 
**     sepSyncGlobalVars is defined here and need to be inluded by all executables
**     involved in the sep test regime. Aka, sep itself, and then all clients.
**
** History:
**
**     24-Mar-2010 (coomi01) b124361
**            Created.
**      21-Jan-2011 (coomi01) b124998
**            Tune to sync to specific listexec controls
**      10-Feb-2011 (coomi01) b125021
**            Add pragma flag
**
*/
#ifdef SEP_SYNC

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <compat.h>
#include <gl.h>
#include <clconfig.h>
#include <systypes.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#ifdef xCL_006_FCNTL_H_EXISTS
#include <fcntl.h>
#endif

#include <me.h>
#include <lo.h>
#include <st.h>
#include <si.h>
#include <tm.h>
#include <pc.h>
#include <tc.h>
#include <seek.h>
#include <tc_sync.h>
#include <signal.h>


/* It is IMPORTANT to ensure the global defaults are sensible
** especially 'char *' so if you add anything in cl!hdr!hdr_unix_win!tc_ipc.h 
** be kind enough to follow suite here 
*/
GLOBALDEF SepSyncGlobalType
sepSyncGlobalVars ={0L,          /* pragmaFlags        */
		    0,           /* syncMode           */
		    0,           /* childSyncMode      */
		    0,           /* client direction   */
		    0,           /* inSepTool          */
		    0,           /* aborted subtest    */
                    0,           /* ipcDisable         */

	            0,           /* showLine           */
		    0,           /* showHndl           */

		    0,           /* output Dir         */
		    "unknown",   /* test prefix        */

		    /** Sep Args Struct     **/
		    {
			0,       /* timeOut    */
			FALSE,   /* child  ignores TimeOut */
                        FALSE,   /* parent ignores Timeout */
			FALSE,   /* gdbFlagged */
			FALSE,   /* ipcMode    */
			FALSE,   /* syncMasked */
			FALSE,   /* syncMode   */
			FALSE,   /* ipcLogVerb */
			FALSE,   /* ipcModify  */
			FALSE,   /* ipcXternal */
			{0},     /* xternalOut */
			{0},     /* gdbString  */
			{0},     /* timeString */
			"not set",/* execArgs   */
			{0}      /* execs      */
		    },

		    /** Test Details Struct **/
		    {
		    0,           /* rawline            */
		    0,           /* testLine           */
		    0,           /* command            */
		    "unknown",   /* testName           */
		    NULL,        /* tokens             */
		    0,           /* modify             */
		    NULL,        /* modHndl            */
		    0            /* modState           */
		    },

		    0,           /* gdb                */
		    -1,          /* gdbSoc             */
		    "not set",   /* gdbStr             */
		    0,           /* childNoSync Msg    */

		    0,           /* TCInitDone         */
                    "not set",   /* timings            */

		    3600,        /* childTimeOut       */
		    0,           /* childInputOverRide */
		    0,           /* childOutputOverRide*/
		    0,           /* childSyncOverRide  */

		    NULL,        /* ipcBuffers         */

                    /* sockDriver Init Data Now Follows   */
		    {
			{"unknown"},/* name               */
			{"?"},      /* errors             */
			NULL,       /* socket driver      */
			{0},        /* connection block   */
			{{0},{0}},  /* listeners          */
			{0},        /* bspParms           */
			0,          /* accept (fdesc)     */   
			0,          /* sync (fdesc)       */   
			0,          /* signon (fdesc)     */   
			0,          /* sState             */
			0,          /* port               */
			0,          /* stackUsage         */
			0,          /* stackCount         */
			NULL,       /* stackTop           */
			{0}         /* stack              */
		    } 
};

/*
** Procedure lookSepEnv
**
** Description
**     Looks to see if an environment variable is set.
*/
static bool 
lookSepEnv(char *name, char **env)
{
    *env = NULL;
    NMgtAt(name, env);
    return (NULL != *env && (0 != **env));
}

/*
** Procedure: initSepArgs
**
** Description:
**
**     Pulls all the sepSync default values prior to 
**     command line examination together into one spot.
**     The command line, in septool.c, can override
**     everything. The purpose is to encapsulate the 
**     bundle, distinguishing between 'standard' sep
**     args, and sep sync args.
*/
void 
initSepSyncArgs()
{
    char *environ;
    i4   loop;

    /*
    ** Set the tollerance level for the child 
    ** ~ Ideally Should be less than our own
    **   so mutal read-deadlock detected first
    ** ~ This is a measure of how quickly sep
    **   ought ot be able to read scripts and 
    **   do its diffs. Five seconds is a long
    **   time in this context.
    */
    sepSyncGlobalVars.args.timeOut             = 5;  
    sepSyncGlobalVars.args.parentIgnoreTimeOut = lookSepEnv("SEP_SYNC_PARENT_NO_TIMEOUT",&environ);
    sepSyncGlobalVars.args.childIgnoreTimeOut  = lookSepEnv("SEP_SYNC_CHILD_NO_TIMEOUT", &environ);

    sepSyncGlobalVars.args.gdbFlagged          = FALSE;                           /* GDB assist required?      */
    sepSyncGlobalVars.args.ipcMode             = FALSE;                           /* ipc mode?                  */

    sepSyncGlobalVars.args.syncMasked          = FALSE;
    sepSyncGlobalVars.args.syncMode            = lookSepEnv("SEP_SYNC",&environ); /* socket mode               */    
    if ( sepSyncGlobalVars.args.syncMode )
    {
	sepSyncGlobalVars.args.syncMasked      = (0 == STncmp(environ,"FALSE",5));
    }

    sepSyncGlobalVars.args.ipcLogVerb          = lookSepEnv("SEP_SYNC_ALOG",&environ); /* Sync Log             */
    sepSyncGlobalVars.args.ipcModify           = lookSepEnv("SEP_SYNC_MOD",&environ);  /* Re-write Sep Files   */

    sepSyncGlobalVars.args.ipcXternal          = lookSepEnv("SEP_SYNC_EXT",&environ);  /* Debug external       */
    if (sepSyncGlobalVars.args.ipcXternal)
	STcopy(environ, &sepSyncGlobalVars.args.xternalOut[0]);
    else
	STcopy(TC_LOCAL_DEBUG_SOCKET,&sepSyncGlobalVars.args.xternalOut[0]);      /* Debug Printf Service      */

    STcopy(TC_VALUE_NOT_SET,&sepSyncGlobalVars.args.gdbString[0]);                   /* GDB assist flags          */
    STcopy(TC_VALUE_NOT_SET,&sepSyncGlobalVars.args.timeString[0]);                  /* Timer flags               */

    sepSyncGlobalVars.args.execArgs[0] = 0;
    for (loop=0;loop<SEPSYNC_EXECS;loop++)
	sepSyncGlobalVars.args.execs[loop] = (PTR)0;

}

/*
** Procedure:  TClistSyncArgs
**
** Description:
**
**    To read through string passed via arg -sync=....
**    Which, if present, dictates which sub-commands are to be run in sync mode
*/
bool
TClistSyncArgs()
{
    i4   loop;
    bool result = FALSE;

    /* This array interprets arg -sync=all  */
    char *all[] = { "abf", 
		    "sql",
		    "qasetusertm",
		    "vision",
		    "vifred",
		    "vigraph",
		    "rbf",
		    "ferun",
                    0 /* Zero must be last */ };

    if ( 0 != sepSyncGlobalVars.args.execArgs[0] )
    {
	result = TRUE;
	loop = 0;

	if ( 0 == STcompare("all", sepSyncGlobalVars.args.execArgs) )
	{
	    while ( 0 != all[loop] )
	    {
		sepSyncGlobalVars.args.execs[loop] = all[loop];
		loop++;
	    }
	    sepSyncGlobalVars.args.execs[loop] = 0;	
	}
	else
	{
	    sepSyncGlobalVars.args.execs[loop] = strtok(sepSyncGlobalVars.args.execArgs, "+");
	    while (0 != sepSyncGlobalVars.args.execs[loop])
	    {
		loop++;
		sepSyncGlobalVars.args.execs[loop] = strtok(0, "+");
	    }
	}

    }

    return result;
}

/*
** Procedure:  TCcompleteSepChildEnviron
**
** Description:
**
**    To adjust the environment after sep has forked()
**    a new process, making ready the child for execution
*/
void
TCcompleteSepChildEnviron(TCFILE *inPtr, TCFILE *outPtr)
{
    i4  fd;
    char envData[64];

    setenv(SEPSOCKETCONN, "TRUE", 1);

    if (sepSyncGlobalVars.args.childIgnoreTimeOut)
	sepSyncGlobalVars.childTimeOut = -1;

    sprintf(envData, "%d", sepSyncGlobalVars.childTimeOut);
    setenv(SEPCLITIME, envData, 1);

    /*
    ** Tell child of any gdb activity
    */
    if ( sepSyncGlobalVars.gdb )
    {
	setenv(SEPDBGMSG, sepSyncGlobalVars.gdbStr, 1);
    }
}

/*
** Procedure:  TCcompleteSepParentEnviron
**
** Description:
**
**    To adjust the environment after sep has forked()
**    a new process, making ready the parent for execution.
**    This includes setting up the stack, and writing the 'Hello'
**    character to the anticipated child.
**
**    The timeout mechanism will falter if the child does not
**    respond in short order. If this happens, the stack count
**    will return 0.
**
**    Take note. We are not capturing stdin at this time. This
**    means that shell scripts, like qasetusertm, which can write
**    error messages to stdout (our stdin) will cause a problem.
**    We could use the pipe mechanism to capture this.
*/
bool
TCcompleteSepParentEnviron(TCFILE *inPtr, TCFILE *outPtr, i4 childPid)
{
    STATUS result;


    /*
    ** Create an initial stack frame for first child to connect upon
    */
    initStack(inPtr,outPtr,childPid);

    TCputc(TC_TIMEDOUT,outPtr);
    TCflush(outPtr);

    return sepSyncGlobalVars.socketDriver.stackCount;
}

/*
** Procedure: TCcreateSepServerSocket
**
** Description:
**         This routine is called by sep once (per script) 
**         to ready itself for the TCP stream services 
**         required by SepSync. We set up two sockets,
**         for data and sync, and prepare them as listeners.
**         Note, listeners have the sole task of allowing 
**         connections to be made. They are, themselves, not
**         the final streams.
**
**         We advertize the presence of the service through
**         environment variables which will be inherited by
**         all children of **this** sep process.
*/
i4
TCcreateSepServerSocket()
{
    GLOBALREF        BS_DRIVER   BS_socktcp;
    GLOBALREF        BS_DRIVER   BS_sockdnet;

    /* 0 ~ We don't care about the TCP port chosen, 
    ** this allows many seps to run at once.
    ** The semi-colon, rather than colon, is required 
    ** by the CL bs-socket interface to delimit host
    ** name from port number.
    */ 
    char             *anonLocalHost = "localhost;0"; 


    /*
    ** Set default positions
    */
    sepSyncGlobalVars.socketDriver.stackTop     = NULL;
    sepSyncGlobalVars.socketDriver.bspParms.buf = anonLocalHost;
    sepSyncGlobalVars.socketDriver.bspParms.pid = getpid();
    sepSyncGlobalVars.socketDriver.bspParms.is_remote = 0;
    sepSyncGlobalVars.socketDriver.bspParms.lbcb = (PTR) &sepSyncGlobalVars.socketDriver.lbcb[0];
    sepSyncGlobalVars.socketDriver.bspParms.bcb  = (PTR) &sepSyncGlobalVars.socketDriver.bcb;
    sepSyncGlobalVars.socketDriver.bspParms.syserr  = (CL_ERR_DESC *) &sepSyncGlobalVars.socketDriver.syserr[0];

    /*
    ** This one will be op system specific
    ** ~ ifdefs perhaps? 
    */
    sepSyncGlobalVars.socketDriver.bsDriver = &BS_socktcp;

    /*
    ** Have to do this right away
    */
    sprintf(sepSyncGlobalVars.socketDriver.name, 
	    "SepSocket:%8.8X", 
	    sepSyncGlobalVars.socketDriver.bspParms.pid);
    setenv(SEPSOCKETNME,  sepSyncGlobalVars.socketDriver.name, 1);

    /* 
    ** Here we set up the primary data listener 
    ** ~ Tells children location of DATA socket
    */
    sepSyncGlobalVars.socketDriver.bsDriver->listen(&sepSyncGlobalVars.socketDriver.bspParms);
    sepSyncGlobalVars.socketDriver.acceptDataHndl =  sepSyncGlobalVars.socketDriver.lbcb[0].fd;
    setenv(SEPSOCKETDATA, sepSyncGlobalVars.socketDriver.bspParms.buf, 1);

    /* 
    ** Here we set up the sync listener 
    ** ~ Tells children location of SYNC socket
    */
    sepSyncGlobalVars.socketDriver.bspParms.buf = anonLocalHost;
    sepSyncGlobalVars.socketDriver.bspParms.lbcb = (PTR) &sepSyncGlobalVars.socketDriver.lbcb[1];
    sepSyncGlobalVars.socketDriver.bsDriver->listen(&sepSyncGlobalVars.socketDriver.bspParms);
    sepSyncGlobalVars.socketDriver.acceptSyncHndl =  sepSyncGlobalVars.socketDriver.lbcb[1].fd;
    setenv(SEPSOCKETSYNC, sepSyncGlobalVars.socketDriver.bspParms.buf, 1);

    /*
    ** done
    */
    return 1;
}


/*********************************************************
 ***********THE REST  OF THIS FILE ***********************
 *********** IS USED FOR DEBUGING  ***********************
 *************     PURPOSES      *************************
 *********************************************************/


/*
** Procedure TCaddDebugService
**
** Description
**     This is yet more debug capability.
**
**     We open a stream socket to a debug listener/echo, which
**     we can then so 'printf' towards. The listener may be
**     local or remote, and accepts connections from anywhere,
**     displaying messages interleaved if necessary on screen
**     or to a file. 
**     
**     I run such a process in an emacs buffer, so I can
**     browse details as they come in.
*/
void
TCaddDebugService(char *connection)
{
    char host[64];
    char *term;
    char *port;
    i4   portIdent;  

    /*
    ** Make own copy of buffer 
    */
    STcopy(connection,host);

    /*
    ** Properly used arg, -Xhost:port
    **                      ^
    */
    term = strtok(host,":");
    port = strtok(0,"");

    /*
    ** Maybe -Xport
    ** ~ mean use localhost
    */
    if ( !port )
    {
	if (!term)
	{
	    port = "20000";
	}
	else
	{
	    port = term;
	}
	term = "localhost";
    }

    portIdent = -1;
    sscanf(port, "%d", &portIdent);

    /*
    ** Maybe -Xhost
    */
    if ( -1 == portIdent )
    {
	term = port;
	portIdent = TC_LOCAL_DEBUG_PORT;
    }

    struct sockaddr_in      servaddr;
    i4 handle = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(portIdent);
    inet_pton(AF_INET, host, &servaddr.sin_addr);

    if ( -1 != connect(handle,
		       (struct sockaddr *)&servaddr,
		       sizeof(servaddr)) 
	)
    {
	sepSyncGlobalVars.gdbSoc = handle;
	sprintf(host, "SEP (%d) STARTED\n", getpid());
 	write(handle, host, strlen(host));
    }
}

/*
** These routines are to create 'annotations' for the alog files.
** The concept is that we want to know 'precisely' what is communicated
** between excutables, at what time, and where we have got in
** the script/cannons.
**
** Traditional sep hides information with the tilder mechanism as follows 

>>
~
<<

** Sometimes this is hides valuable contexts and hinders understanding
** ensuing error states.
**
** We want output everything, but the prevent total mahem we
** also want to anotate information source details. 
*/
#define MARK_STACK_SIZE 1000

static u_i4 
markStack[MARK_STACK_SIZE];

static i4 
markStackTop=0;

void
showLinePushMark(u_i8 mark)
{
    if (SHOW_LINE_ON & sepSyncGlobalVars.showLine)
    {
	markStack[markStackTop++]  = sepSyncGlobalVars.showLine;
	sepSyncGlobalVars.showLine  = mark|SHOW_LINE_ON;
    }
}

void
showLinePopMark()
{
    if (SHOW_LINE_ON & sepSyncGlobalVars.showLine)
    {
	if (markStackTop > 0)
	{
	    sepSyncGlobalVars.showLine = markStack[--markStackTop];
	}
    }
}

void
showClose(u_i8 exitCode)
{
    char fileName[512];

    if ( sepSyncGlobalVars.showHndl > 0 )
    {
	close(sepSyncGlobalVars.showHndl);
    }
    
    if (sepSyncGlobalVars.gdbSoc > 0)
    {
	close(sepSyncGlobalVars.gdbSoc);
    }

    sepSyncGlobalVars.showHndl = -1;
    sepSyncGlobalVars.gdbSoc = -1;
}

void
writeModify(char *data)
{
    static char buffer[1024*1024];
    static i4  index  = 0;
    static char buffer1[1024*1024];
    static i4  index1 = 0;
    i4    length;

    if ( sepSyncGlobalVars.test.modify )
    {
	length = strlen(data);
	switch (sepSyncGlobalVars.test.modState)
	{
	    case 0:
		if (0 != length)
		{
		    memcpy(&buffer[index], data, length);
		    index += length;
		}
		break;

	    case 1:
		if (0 != index1)
		{
		    buffer1[index1] = 0;
		    fprintf(sepSyncGlobalVars.test.modHndl, "%s", &buffer1[0]);
		    index1 = 0;
		}
		if (0 != index)
		{
		    buffer[index] = 0;
		    fprintf(sepSyncGlobalVars.test.modHndl, "%s", &buffer[0]);
		    index = 0;
		}
		if (0 != length)
		    fprintf(sepSyncGlobalVars.test.modHndl, "%s", data);

		sepSyncGlobalVars.test.modState = 2;
		break;

	    case 2:
		if (0 != length)
		{
		    if ((length > 1) && (data[0]=='<' && data[1] == '<' ))
		    {
			if ((length > 16) && 0 == STncmp("<< IF (SEP_SYNC)\n", &data[0],17))
			{
			    sepSyncGlobalVars.test.modState = 4;
			    writeModify(data);			
			}
			else
			{
			    sepSyncGlobalVars.test.modState = 0;
			    writeModify(data);			
			}
		    }
		    else
		    {
			fprintf(sepSyncGlobalVars.test.modHndl, "%s", data);
		    }
		}
		break;

	    case 3:
		if (0 != length)
		    fprintf(sepSyncGlobalVars.test.modHndl, "%s", data);
		break;

	    case 4:
		if (0 != length)
                {
                    memcpy(&buffer1[index1], data, length);
                    index1 += length;
                }

		if ((length > 1) && (data[0]=='>' && data[1] == '>' ))
		{
		    sepSyncGlobalVars.test.modState = 2;
		}
                break;

	    case 5:
		if (0 != length)
		    fprintf(sepSyncGlobalVars.test.modHndl, "%s", data);

		if (0 != index)
		{
		    buffer[index] = 0;
		    fprintf(sepSyncGlobalVars.test.modHndl, "%s", &buffer[0]);
		    index = 0;
		}

		sepSyncGlobalVars.test.modState = 2;
		break;
	}
    }
}

static bool
wholeLine(char **buffer)
{
  static char current[1024];
  static i4  content = 0;

  i4 length = strlen(*buffer);
  if ( (*buffer)[length-1] == '\n' )
    {
      if (0 != content)
	{
	  STcopy(*buffer, &current[content]);
	  *buffer = current;
	  content = 0;
	}
	return TRUE;
    }
  else
    {
	  STcopy(*buffer, &current[content]);
	  content += length;
	  return FALSE;
    }
  return TRUE;
}

static void
printLine(bool quiet, i4 level, char *prefix, i4 disLine, char *buffer)
{
    i4                      loop;
    char             indent[512];
    static char      tstName[64];
    static struct    timeval now;
    static struct  timeval start;
    char             timeBuf[64];
    char           outBuf[65536];

    for (loop=0;loop<level;loop++)
    {
	STcopy("....", &indent[loop*3]);
    }
    indent[loop*3] = 0;

    if (0 != STcompare(tstName, sepSyncGlobalVars.prefix) )
    {
	STcopy(sepSyncGlobalVars.prefix, tstName);
	gettimeofday(&start,0);
	now.tv_sec  = 0;
	now.tv_usec = 0;
    }
    else
    {
	gettimeofday(&now,0);
	now.tv_usec -= start.tv_usec;
	now.tv_sec  -= start.tv_sec;
	if (now.tv_usec < 0)
	{
	    now.tv_usec += 1000000;
	    now.tv_sec  -= 1;
	}
    }
    sprintf(timeBuf, "(%3.3d.%3.3d)", now.tv_sec, now.tv_usec/1000);


    if (quiet)
	sprintf(outBuf, "%s[      @%s%s]%s",  timeBuf,           prefix, &indent[0], buffer);
    else
	sprintf(outBuf, "%s[%-6.6d@%s%s]%s",  timeBuf, disLine,  prefix, &indent[0], buffer);

    if (-1 != sepSyncGlobalVars.showHndl)
    {
	write(sepSyncGlobalVars.showHndl, outBuf, strlen(outBuf));
    }

    if (-1 != sepSyncGlobalVars.gdbSoc)
    {
	write( sepSyncGlobalVars.gdbSoc, sepSyncGlobalVars.prefix, strlen(sepSyncGlobalVars.prefix));
	write( sepSyncGlobalVars.gdbSoc, outBuf, strlen(outBuf));
    }
}

void
showLine(u_i8 what, char *buffer)
{
    static i4      level = 0;
    static i4     lineNo = 0;
    static i4   canon_id = 1;
    static i4      soFar = 0;
    static char  tstName[64];
    static struct timeval start;
    
    char localBuf[1024];
    char outBuf[65536];
    char indent[512];
    char format[512];
    char timeBuf[64];
    char *unknown = "UNKNOWN  ";
    char *prefix  = unknown;
    i4 disLine;
    i4 quiet = 0;
    i4 loop;
    static struct timeval now;

    disLine = sepSyncGlobalVars.test.rawLine;

    if (SHOW_LINE_ON & sepSyncGlobalVars.showLine)
    {
	if (sepSyncGlobalVars.showHndl == 0)
	{
	    if (sepSyncGlobalVars.outDir)
	    {
		sprintf(outBuf, "%s/%s.alog", sepSyncGlobalVars.outDir, sepSyncGlobalVars.prefix);
	    }
	    else
	    {
		sprintf(outBuf, "%s.alog", sepSyncGlobalVars.prefix);
	    }

	    sepSyncGlobalVars.showHndl = open(outBuf, O_CREAT|O_WRONLY|O_TRUNC, 0666);
	}

	if (-1 == sepSyncGlobalVars.showHndl)
	    return;

	if (what)
	{
	    if (SHOW_LINE_RSLT == what )
	    {
		quiet   = 1; /* Set to zero if want to show result lines independently numbered */
		disLine = lineNo++;
		prefix  = "RESULT   ";
	    }
	    else if (SHOW_LINE_RFILE == what)
	    {
		quiet   = 1;
		disLine = lineNo++;
		prefix  = "RFILE    ";
	    }
	    else if (SHOW_LINE_SYNC == what)
	    {
                /* 
		** Have to break gigantic buffer into single lines 
		*/
		char *input   = &buffer[0];
		while (*input)
		{
		    localBuf[soFar++] = *input;
		    if (*input == '\n')
		    {
			localBuf[soFar] = 0;
			printLine(1, level, "SYNC     ", disLine, &localBuf[0]);
			soFar   = 0;
		    }
		    input++;
		}
		return;
	    }
	    else if (SHOW_LINE_CMD == what)
	    {
		canon_id = 1;
		prefix = "COMMAND  ";
	    }
	    else  if (SHOW_LINE_COMMENT == what)
	    {
		prefix = "COMMENT  ";
	    }
	    else  if (SHOW_LINE_CANON == what)
	    {
		prefix = "CANON    ";
	    }
	    else  if (SHOW_LINE_FILL  == what)
	    {
		prefix = "FILL     ";
	    }
	    else  if (SHOW_LINE_IGNRD == what)
	    {
		quiet  = 1;
		prefix = "IGNORED  ";
	    }
	    else  if (SHOW_LINE_PASS  == what)
	    {
		quiet = 1;
		prefix = "PASSED   ";
	    }
	    else  if (SHOW_LINE_FAIL  == what)
	    {
		quiet = 1;
		prefix = "FAILED   ";
	    }
	    else  if (SHOW_LINE_PUSH  == what)
	    {
		quiet = 1;
	        sprintf(format, "PUSH-%2.2d  ", sepSyncGlobalVars.socketDriver.stackCount);
		prefix = format;
	    }
	    else  if (SHOW_LINE_POP  == what)
	    {
		quiet = 1;
	        sprintf(format, "POP-%2.2d   ", sepSyncGlobalVars.socketDriver.stackCount);
		prefix = format;
	    }
	    else  if (SHOW_LINE_ABORTED  == what)
	    {
		quiet = 1;
	        sprintf(format, "ABORT-%2.2d ", sepSyncGlobalVars.socketDriver.stackCount);
		prefix = format;
	    }
	    else  if (SHOW_LINE_TIMEOUT  == what)
	    {
		quiet = 1;
		prefix = "TIMEOUT  ";
	    }
	}
	else
	{
	    if (SHOW_LINE_RSLT & sepSyncGlobalVars.showLine)
	    {
		quiet   = 1; 
		disLine = lineNo++;
		prefix  = "RESULT   ";
	    }
	    else if (SHOW_LINE_CMD & sepSyncGlobalVars.showLine)
	    {
		canon_id = 1;
		prefix = "COMMAND  ";
	    }
	    else  if (SHOW_LINE_COMMENT & sepSyncGlobalVars.showLine)
	    {
		prefix = "COMMENT  ";
	    }
	    else  if (SHOW_LINE_CANON & sepSyncGlobalVars.showLine)
	    {
		sprintf(format, "CANON-%2.2d ", canon_id);
		prefix = "CANON    ";
		prefix = format;
		if ( 0 == STncmp(buffer,">>",2) )
		{
		    prefix = "CANON-E  ";
		    canon_id++;
		}
	    }
	    else  if (SHOW_LINE_FILL  & sepSyncGlobalVars.showLine)
	    {
		prefix = "FILL     ";
	    }
	}

	if ( unknown == prefix )
	{
	    if ( 0 == STncmp(buffer,"<<",2) )
		prefix = "CANON-S  ";
	    else if (  0 == STncmp(buffer,"? ",2) )
		prefix = "COMMAND  ";
	    else if (  0 == STncmp(buffer,"/*",2) )
		prefix = "COMMENT  ";
	    else if (  0 == STncmp(buffer,"^^",2) )
	    {
		canon_id = 1;
		prefix = "KEYINSEQ ";
	    }
	    else if ( 0 == STncmp(buffer,".if",3) )
	    {
		prefix  = "CONDCODE ";
		level += 1;
	    }
	    else if ( 0 == STncmp(buffer,".endif",6) )
	    {
		prefix  = "CONDEND  ";
		level -= 1;
	    }
	    
	}

	for (loop=0;loop<level;loop++)
	{
	    STcopy("....", &indent[loop*3]);
	}
	indent[loop*3] = 0;

	if (0 != STcompare(tstName, sepSyncGlobalVars.prefix) )
	  {
	      STcopy(sepSyncGlobalVars.prefix, tstName);
	    gettimeofday(&start,0);
	    now.tv_sec  = 0;
	    now.tv_usec = 0;
	  }
	else
	  {
	    gettimeofday(&now,0);
	    now.tv_usec -= start.tv_usec;
	    now.tv_sec  -= start.tv_sec;
	    if (now.tv_usec < 0)
	      {
		now.tv_usec += 1000000;
		now.tv_sec  -= 1;
	      }
	  }
	sprintf(timeBuf, "(%3.3d.%3.3d)", now.tv_sec, now.tv_usec/1000);


	if (quiet)
	  sprintf(outBuf, "%s[      @%s%s]%s",  timeBuf,           prefix, &indent[0], buffer);
	else
	  sprintf(outBuf, "%s[%-6.6d@%s%s]%s",  timeBuf, disLine,  prefix, &indent[0], buffer);

	if (-1 != sepSyncGlobalVars.showHndl)
	{
	    write(sepSyncGlobalVars.showHndl, outBuf, strlen(outBuf));
	}

	if (-1 != sepSyncGlobalVars.gdbSoc)
	{
	    write( sepSyncGlobalVars.gdbSoc, sepSyncGlobalVars.prefix, strlen(sepSyncGlobalVars.prefix));
	    write( sepSyncGlobalVars.gdbSoc, outBuf, strlen(outBuf));
	}

	if (soFar)
	{
	    localBuf[soFar]  = '\n';
	    localBuf[soFar+1]= 0;
	    printLine(1,level,"SYNC     ",disLine,&localBuf[0]);
	}
	soFar = 0;

	// printLine(quiet,level,prefix,disLine,buffer);
    }
}
#endif /* SEP_SYNC */
