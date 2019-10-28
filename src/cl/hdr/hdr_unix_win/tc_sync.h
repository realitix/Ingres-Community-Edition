/*
**	Copyright (c) 2010 Actian Corporation
**	All rights reserved.
*/

/**
** Name:	tc_sync.h 
**
** Description:
**	Contains data structures unique to SepSync.
**
** History:
**      06-Sep-2010 (coomi01) b124361
**          Created.
**      21-Jan-2011 (coomi01) b124998
**            Tune to sync to specific listexec controls
**      10-Feb-2011 (coomi01) b125021
**            Add pragma flag
*/
#ifdef SEP_SYNC

#ifndef TC_SYNC_H
#define TC_SYNC_H
#include <me.h>
#include <lo.h>
#include <st.h>
#include <si.h>
#include <tm.h>
#include <pc.h>
#include <tc.h>
#include <clconfig.h>
#include <seek.h>
#include <iicommon.h>
#include <bsi.h>
#include <bsshmio.h>
#define SEPBUFFERSZ  4096
#define SEPHISTORYSZ  256

/*
** A set of defines for sepSyncInfoType.state field
*/
#define SEPSYNC_STATE_INIT       '?'
#define SEPSYNC_STATE_INITIATED  '!'
#define SEPSYNC_STATE_READING    'r'
#define SEPSYNC_STATE_WRITING    'w'
#define SEPSYNC_STATE_WAITING    'c'
#define SEPSYNC_STATE_SUSPENDING 's'
#define SEPSYNC_STATE_SUSPENDED  'S'
#define SEPSYNC_STATE_RESUME     'R'
#define SEPSYNC_STATE_BUSY       'b'
#define SEPSYNC_STATE_CLOSING    'x'
#define SEPSYNC_STATE_CLOSED     'X'

/*
** A set of defines for sepSyncInfoType.cmd field
*/
#define SEPSYNC_CMD_OK        1
#define SEPSYNC_CMD_ATTENTION 2
#define SEPSYNC_CMD_A_REPLIED 3
#define SEPSYNC_CMD_WAITING   4
#define SEPSYNC_CMD_ABORT     5

#define SYNC_DONE   0
#define SYNC_WAIT   1
#define SYNC_NOWAIT 2

/*
** Sep sends state 'info' from child to parent, and vice-versa
** describing the current state of the connection as seen from
** either side of the socket connection
*/ 
typedef struct {
    i4        ident;
    i4          pid;
    i4    readCount;
    i4   writeCount;
    i4        state;
    i4          cmd;
    i4     toBeSent;
} sepSyncInfoType;

/*
**  A buffer type to hold i/o data prior to sep processing 
**  There will be one of these for reading and one for writing.
*/
typedef struct {
    void               *TCHandle; /* Points backwards to parent descriptor */
    i4                     data; /* The i/o handle for read/writing data  */
    i4                     sync; /* The i/o handle for read/writing sync  */

    i4                    count; /* Number of chars in the circular buffer*/
    i4                   getPos; /* Starting Position of the data         */
    i4                   putPos; /* Ending Position of the data           */
#ifdef REMOVE
    char   history[SEPHISTORYSZ]; /* No longer in use                      */
    i4                  pipe[2];  /* No longer in use                      */
#endif
    char  buffer[SEPBUFFERSZ+64]; /* The buffer                            */
} sepDataBufferType;


#define PIPE_READ  0
#define PIPE_WRITE 1
#define SOCK_READ  0
#define SOCK_WRITE 0

/*
** This is for child Data streams
*/
typedef struct _sepIPCBufferType {
    struct _sepIPCBufferType *next;
    i4                   ownerPid;
    sepDataBufferType       reader;
    sepDataBufferType       writer;
} sepSyncBufferType;

typedef struct {
    i4                   abort;
    i4                     pid;
    i4                    data;
    i4                    sync;
    sepSyncInfoType       local;
    sepSyncInfoType      remote;
    sepSyncBufferType  *iobuffs;
} sepSyncDetailsType;


/* 
** A typedef for the array of concurrent connections to sep
** Limited to a max depth of 32, which ought to be more than
** enough
*/
#define SEP_SOCKSTACK_SIZE 32
typedef sepSyncDetailsType sepStack[SEP_SOCKSTACK_SIZE+2];

/*
** This anchors our use of SOCKETS
** ~ NB : For convenience, we throw away stack[0] and only 
**        use stack[1] ... stack[stackCount].
** ~ stackTop is another convenience and should normally
**   be set to the address &stack[stackCount]
** ~ stackUsage counts the active children within the
**   stack, and might be less than stackCount.
**
** Note :
**   The existing code style wraps up the bind/listen/accept
**   trio used by TCP/IP stream servers into BS_DRIVER. This,
**   I think, is for DECNet purposes. This complicates matters
**   a little, for we have to call BS_DRIVER support with 
**   BS_PARMS, to implement the trio. So we have sub-structures
**   here just to do that.
*/
typedef struct {
    char                  name[32];   /* Unique string, to ensure genuine clients       */
    char              syserr[1024];   /* Room for driver error messages                 */
    BS_DRIVER            *bsDriver;   /* Indirection to Socket routines for portability */
    BCB                        bcb;   /* Connect block                                  */
    LBCB                   lbcb[2];   /* Listen control blocks                          */
    BS_PARMS              bspParms;   /* Parameter block for the driver                 */
    i4             acceptDataHndl;   /* Fdesc for select/accept-data                   */
    i4             acceptSyncHndl;   /* Fdesc for select/accept-sync                   */
    i4                     signon;   /* Fdesc for select/signon                        */
    i4                     sState;   /* State of signon                                */
    i4                       port;   /* Port to inform client where to connect from    */
    i4                 stackUsage;   /* Count of active children                       */
    i4                 stackCount;   /* Current child to communicate with              */
    sepSyncDetailsType   *stackTop;   /* Working stack entry                            */
    sepStack                 stack;   /* Accept child/grandchild to limited depth       */
} sepSocketsDriverType;

/*
** Details of the current script being processed
** ~ Not only useful for debugging, but also for
**   writing log files. 
*/
typedef struct {
    i4       rawLine; /* Where we got to so far in the .sep file            */
    i4          line; /* Where the actual command sep is processing started */
    i4           cmd; /* Name of the script command                         */
    char       *name; /* Name of the test file                              */
    char    **tokens; /* Token array as seem by procmmd.c                   */
    i4        modify; /* Flag to say we are to write our own sep script     */ 
    FILE    *modHndl; /* File handle to that script                         */
    i4      modState;
} testDetailsType;

/*
** This structure allows us to set defaults for 
** the sep command line args read in septool.c
*/
#define TC_LOCAL_DEBUG_PORT   20000
#define TC_LOCAL_DEBUG_SOCKET "localhost:20000"
#define TC_VALUE_NOT_SET      "not set"
#define TC_ARG_BUFF_SIZE      256

typedef struct {
  i4    timeOut;
  bool  childIgnoreTimeOut;
  bool  parentIgnoreTimeOut;
  bool  gdbFlagged;
  bool  ipcMode;
  bool  syncMasked;
  bool  syncMode;
  bool  ipcLogVerb;
  bool  ipcModify;
  bool  ipcXternal;
  char  xternalOut[TC_ARG_BUFF_SIZE];
  char  gdbString[TC_ARG_BUFF_SIZE];
  char  timeString[TC_ARG_BUFF_SIZE];

#define SEPSYNC_EXECS  200
    char             execArgs[TC_ARG_BUFF_SIZE];
    PTR              execs[SEPSYNC_EXECS];
} sepSyncArgsType;

/* 
** Using a global struct here to collect flags etc. under one roof.
** ~ BE CAREFUL TO INTIALISE DEFAULT VALUES in tc_ipc.c to avoid
**   SegVs and other hangups.
*/
typedef struct {
#define SEPSYNC_NOTIMEOUT   0x01L
#define SEPSYNC_NOTIMEOUT_1 0x02L
    u_i8       pragmaFlags;
    i4            syncMode;
    i4       childSyncMode; 
    u_char clientDirection;
    i4           inSepTool;
    i4             aborted;
    i4          ipcDisable;

#define SHOW_LINE_OFF      (0)
#define SHOW_LINE_ON       (1<<1)
#define SHOW_LINE_RSLT     (1<<2)
#define SHOW_LINE_CMD      (1<<3)
#define SHOW_LINE_CANON    (1<<4)
#define SHOW_LINE_COMMENT  (1<<5)
#define SHOW_LINE_FILL     (1<<6)
#define SHOW_LINE_RFILE    (1<<7)
#define SHOW_LINE_PASS     (1<<8)
#define SHOW_LINE_FAIL     (1<<9)
#define SHOW_LINE_IGNRD    (1<<10)
#define SHOW_LINE_PUSH     (1<<11)
#define SHOW_LINE_POP      (1<<12)
#define SHOW_LINE_ABORTED  (1<<13)
#define SHOW_LINE_SYNC     (1<<14)
#define SHOW_LINE_TIMEOUT  (1<<15)

    u_i8          showLine;
    i4            showHndl;
    char           *outDir;
    char           *prefix;

    sepSyncArgsType   args;
    testDetailsType   test;
    i4                 gdb;
    i4              gdbSoc;
    char           *gdbStr;
    i4         childNoSync;

    i4          TCInitDone;
    char          *timings;

    i4        childTimeOut;
    i4  childInputOverRide;
    i4 childOutputOverRide;
    i4   childSyncOverRide;

    sepSyncBufferType        *ipcBuffers;
    sepSocketsDriverType    socketDriver;

} SepSyncGlobalType;

GLOBALREF SepSyncGlobalType sepSyncGlobalVars;
GLOBALREF TCFILE *SEPtcinptr;
GLOBALREF TCFILE *SEPtcoutptr;

/*
** Define environment variables used by Sep to indicate presence
** to child(ren) 
*/
#define SEPDBGMSG     "USE_SEP_GDB"
#define SEPSOCKETNME  "USE_SEP_NAME"      /** For Verification           **/
#define SEPSOCKETCONN "USE_SEP_SOCKETS"   /** Environ flag, set to TRUE  **/
#define SEPSOCKETDATA "USE_SEP_DATA"      /** Environ value for DataPort **/
#define SEPSOCKETSYNC "USE_SEP_SYNC"      /** Environ value for SyncPort **/
#define SEPCLITIME    "USE_SEP_TIME"



/*
** Function Prototypes from tc_sync.c
** ==================================
*/
extern void initSepSyncArgs(void);
extern bool TClistSyncArgs();
extern void TCcompleteSepChildEnviron(TCFILE *inPtr, TCFILE *outPtr);
extern bool TCcompleteSepParentEnviron(TCFILE *inPtr, TCFILE *outPtr, i4 childPid);
extern i4   TCcreateSepServerSocket(void);
extern void TCaddDebugService(char *connection);
extern void showLinePushMark(u_i8 mark);
extern void showLinePopMark(void);
extern void showClose(u_i8 exitCode);
extern void writeModify(char *data);
extern void showLine(u_i8 what, char *buffer);

/*
** Function Prototypes from tc.c
** ==================================
*/
extern void initStack(TCFILE *inPtr, TCFILE *outPtr, i4 childPid);
extern i4   TCtransfer(TCFILE *desc, FILE *outFile, i4 timeout, i4 *prevCh, i4 *result);
extern void TCSyncWait(i4 waiting);
extern void TCDisconnect(i4  SEPsonpid);


#endif /** TC_SYNC_H **/
#endif /** SEP_SYNC **/
