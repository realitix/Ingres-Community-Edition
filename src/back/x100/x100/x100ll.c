/*
 * Copyright (C) 2009-2011 VectorWise B.V.
 * All Rights Reserved.
 *
 * Maintainer: kschendel (Ingres-ified from giel/michal original)
 * History:
 *	06-Jun-2011 (drivi01)
 *	     Remove a call to close_dblockfile(lockfd) from
 *	     db_kill_if_open, b/c close_dblockfile was already
 *	     called in lock_dblockfile.
 *	     Remove logging.  The memory is being freed 
 *	     incorrectly, LocalFree should be used for strdup
 *	     allocations and free for calloc.  Both types of
 *	     allocations are used in logger_get_config.
 *	     It was decided to remove the logging.
 *	14-Jun-2011 (kschendel)
 *	    Put the above call back, it's needed.  Comments inline.
 *	    Make sure that context/server info is cleared at init time,
 *	    might have been the source of some of the problems.
 *	    (later) Revise lockfile stuff to have a separate unlock,
 *	    and macro-ize the return value since it's a HANDLE on winders.
**	17-Jun-2011 (kschendel) m1930
**	    Add server check-dead entry points; rearrange code a little
**	    so that server cleanup is more centralized.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
 */

#include <compat.h>
#include <iicommon.h>
#include <cs.h>
#include <di.h>
#include <er.h>
#include <ex.h>
#include <lo.h>
#include <nm.h>
#include <x100ll.h>

#if !defined(conf_IVW)
/* stubs for non-IVW builds */
void x100_exit(void)
{
    /* nothing */
}

#else

/* All the rest is IVW only. */

#include <dbdbms.h>
#include <scf.h>
#include <ervwf.h>

/* When running in Ingres, we don't want to include all kinds of profiling
 * code with dependencies.
 */
#undef X100_PROFILE

#include "x100/netbuffer.h"
#include "x100/x100_error_defs.h"

#include <stdlib.h>
#include <stdio.h>	/* for snprintf */
#include <fcntl.h>

#ifdef NT_GENERIC
#define snprintf _snprintf
#endif

#ifndef NT_GENERIC
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

# ifndef NT_GENERIC
# include <netdb.h>
# include <netinet/in.h>
#  include <arpa/inet.h>
# endif

/*
** File: x100ll.c -- Low level X100 interface, servers and connections
**
** Description:
**	This file is the lowest level of the X100 interface, concerning
**	itself with server context, server start/stop, and connections.
**
**	This file lives in a sort of no-mans land between Ingres
**	and X100.  It runs in the Ingres context, but the coding style
**	is X100-ish, and it doesn't obey the Ingres prohibitions against
**	OS calls outside of the CL.  In fact, one could consider this
**	file as in effect being part of the CL.  (Actually making it
**	part of the CL is way too much work for the return, though.)
**
** Edit History:
**	26-May-2011 (kschendel) m1260
**	    Adapt from X100's server_mngmnt.c, which is now a hollow stub.
**	    Move X100 context into Ingres shared memory, mostly so that
**	    the JSP can get at it, and also so that X100 servers can be
**	    cleanly shut down (when the RCP stops).
**	06-Jun-2011 (drivi01) 
**	    Redefine x100_global_ctx as global variable instead of
**	    static and export it in iilibdmfdata on Windows.
**	    Reference it in this file.
**	22-Jun-2011 (drivi01)
**	    On x100 server startup, add some code to set VWLOG
**	    variable if it's not set already, so vectorwise.log
**	    file is created.
*/


#define LOCALHOST "127.0.0.1"

#define DBFARM_QUERY "sys(getconf, [server, dbfarm_path])"
#define DBNAME_QUERY "sys(getconf, [server, dbname])"
#define TERMINATE_QUERY "sys(terminate)"
#define TID_QUERY "sys(get_session_id)"
#define KILL_TID_QUERY "sys(kill_session,['%s'])"

/* FIXME this is disgusting but trying to include file.h leads to
** a long, long fall down the include rabbit hole.  Need to do something
** about that!  move the defn's to a header that doesn't include the
** whole x100 utils world.
*/
#ifdef NT_GENERIC
#define F_TLOCK 2 
#define F_LOCK 1 
#define FILE_LOCK_HANDLE HANDLE
#define FILE_LOCK_BAD ((HANDLE) -2)
#define FILE_LOCK_ALREADYLOCKED ((HANDLE) -1)
#else
#define FILE_LOCK_HANDLE int
#define FILE_LOCK_BAD -2
#define FILE_LOCK_ALREADYLOCKED -1
#endif
FUNC_EXTERN FILE_LOCK_HANDLE file_lock(char *filename, int mode);
FUNC_EXTERN void file_unlock(FILE_LOCK_HANDLE lockfd);
FUNC_EXTERN int write_lockfile(FILE_LOCK_HANDLE lockfd, char *buf, int len);


#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

/* Some stuff in amdb_config.h on the x100 side, which we don't want */

#if defined(NT_GENERIC)
# define DIR_SEP '\\'
#else
# define DIR_SEP '/'
# define closesocket close
# define SOCKET int
#endif

GLOBALREF X100Context *x100_global_ctx;

static const int SERVER_KILL_TIMEOUT = 3000; /* milliseconds */

/* Ingres-ish version of vw_assert, probably generally useful but
** for now just do it here.  Also for now always check, these
** routines aren't massively speed critical.
*/
#define vw_assert(cond) if (!(cond)){ \
  TRdisplay("%@ Assertion \"%s\" failed, line %d of file %s\n",#cond,__LINE__,__FILE__);\
  *(i4 *)0 = 1;\
}

/* A few x100 utilities declared in x100 headers that I don't want
** to drag in (causes include hell).
*/

FUNC_EXTERN int deltree(const char *path);
FUNC_EXTERN char * logger_get_config(void);



/* creates socket and initiate connection with the server
 * @return if succeeded, a positive integer, stream fd
 *         otherwise, a negative integer, indicating error */
static int
setup_connection(int s_addr_, int server_port)
{
	struct sockaddr_in sockaddr;
	SOCKET    sockfd;

	network_init();

	/* indicates which type of address is following, always set to AF_INET */
	sockaddr.sin_family = AF_INET;
	/* in network byte ordering */
	sockaddr.sin_port = htons(server_port);
	/* server address, already in network byte ordering */
	sockaddr.sin_addr.s_addr = s_addr_;

	/* zero the rest of the struct */
	memset(&(sockaddr.sin_zero), '\0', 8);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
		TRdisplay("setup_connection: socket failed\n");
		return sockfd;
	}

	if (connect(sockfd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) < 0) {
		TRdisplay("setup_connection: connect failed\n");
		closesocket(sockfd);
		return -1;
	}
	return sockfd;
}

static int
x100_kill(int pid)
{
	int ret = -1;
#ifdef NT_GENERIC
#define event_name_len 30 /* Local\x100_server_local_65535 */
	HANDLE hProcess, hEvent;
	char eventname[event_name_len];

	snprintf(eventname, event_name_len-1, "x100_server_local_%d", pid);
	eventname[event_name_len-1] = 0;
	hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, eventname);
	if (!hEvent) {
		snprintf(eventname, event_name_len-1, "Global\\x100_server_%d", pid);
		eventname[event_name_len-1] = 0;
		hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, eventname);
		if (hEvent == NULL)
			return -1;
	}
	hProcess = OpenProcess(PROCESS_TERMINATE |
							PROCESS_QUERY_INFORMATION |
							SYNCHRONIZE,
							FALSE, pid);
	if (hProcess == NULL) {
		CloseHandle(hEvent);
		return -1;
	}
	if (SetEvent(hEvent) &&
			WaitForSingleObject(hProcess, SERVER_KILL_TIMEOUT) == WAIT_OBJECT_0)
		ret = 0;
	if (ret < 0) {
		TRdisplay("x100_kill: server with pid %d not responding, terminating.\n", pid);
		ret = TerminateProcess(hProcess, 0) ? 0 : -1;
	}

	CloseHandle(hEvent);
	CloseHandle(hProcess);
	return ret;
}
#else
	int timeout = SERVER_KILL_TIMEOUT * 1000;	/* ii-nap likes usec */
	int waitret = 0;

	ret = kill(pid, SIGTERM);
	while (ret == 0 && timeout > 0 && waitret == 0) {
		II_nap(50000);		/* 0.05 sec */
		timeout -= 50000;
		waitret = waitpid(pid, NULL, WNOHANG);
	}

	if (ret == 0 && timeout <= 0) {
		TRdisplay("%@ x100_kill: server with pid %d not responding, killing.\n", pid);
		ret = kill(pid, SIGKILL);
		(void) waitpid(pid, NULL, 0);
	}
	return ret;
}
#endif


/* Open the lock file for reading */
static int
open_dblockfile(char *dbfarm, char *dbname)
{
	char lockfile[MAX_LOC + DB_DB_MAXNAME + 15];
	int lockfd;

	sprintf(lockfile, "%s%c%s%cCBM%clock",
				dbfarm, DIR_SEP, dbname, DIR_SEP, DIR_SEP);

	lockfd = open(lockfile, O_RDONLY);

	return lockfd;
}


static int
get_pid_from_lockfile(int lockfd)
{
	char buf[50];
	int pid = 0, port = 0, len;

	if (lockfd < 0)
		return -1;

	len = read(lockfd, buf, sizeof(buf)-1);
	buf[len] = '\0';
	sscanf(buf, "%d\n%d\n", &port, &pid);

	return pid;
}

static void
close_dblockfile(int lockfd)
{
	if (lockfd < 0)
		return;

	close(lockfd);
}


/* Lock the x100/database lock-file.  "mode" is either F_LOCK (wait for
** the lock), or F_TLOCK (try to get lock, return immediately if can't).
** Success returns a FILE_LOCK_HANDLE (an fd or a windows HANDLE) which
** will ultimately be passed to file_unlock to unlock the lock.
** Error (bad dbfarm or dbname) returns FILE_LOCK_BAD.
** Lock already locked by someone else returns FILE_LOCK_ALREADYLOCKED.
*/
static FILE_LOCK_HANDLE
lock_dblockfile(char *dbfarm, char *dbname, int mode)
{
	char lockfile[MAX_LOC + DB_DB_MAXNAME + 15];
	FILE_LOCK_HANDLE lockfd;

	sprintf(lockfile, "%s%c%s%cCBM%clock",
				dbfarm, DIR_SEP, dbname, DIR_SEP, DIR_SEP);

	lockfd = file_lock(lockfile, mode);
	return lockfd;
}

static void
read_authpass(char *dbfarm, char *dbname, char *authpass)
{
	char name[MAX_LOC + DB_DB_MAXNAME + 20];
	FILE *f;

	sprintf(name, "%s%c%s%cauthpass", dbfarm, DIR_SEP, dbname, DIR_SEP);
	authpass[0] = 0;
	f = fopen(name, "rb");
	if (f) {
		int len = (int) fread(authpass, 1, AUTHPASS_MAX_SIZE-1, f);
		if (len > 0) authpass[len] = 0;
		fclose(f);
	}
}


/* Kill an X100 server for the given dbfarm/dbname if there appears to be
** one running, as determined by the lockfile.  If all is well we'll get
** the lock and simply return it.  If the lock is already locked that
** implies that an x100 server is running, kill it.
**
** Used in contexts where we don't care about server context entry, either
** we're about to start a new server, or it's system startup.
*/
static void
db_kill_if_open(char *dbfarm, char *dbname)
{
	FILE_LOCK_HANDLE lockfd;

	lockfd = lock_dblockfile(dbfarm, dbname, F_TLOCK);
	if (lockfd == FILE_LOCK_ALREADYLOCKED) {
		int pid;

		/* db is already locked */
		lockfd = open_dblockfile(dbfarm, dbname);
		pid = get_pid_from_lockfile(lockfd);
		close_dblockfile(lockfd);
		lockfd = FILE_LOCK_BAD;

		TRdisplay("db_kill_if_open: %s%c%s already in use\n", dbfarm, DIR_SEP, dbname);
		if (pid > 0) {
			TRdisplay("db_kill_if_open: killing pid %d\n", pid);
			x100_kill(pid);
			/* wait for x100 to exit, i.e., wait for lock */
			lockfd = lock_dblockfile(dbfarm, dbname, F_LOCK);
			if (lockfd == FILE_LOCK_BAD)
				TRdisplay("db_kill_if_open: error waiting for x100 to stop\n");
		}
	}
	file_unlock(lockfd);
}

static char*
copy_env_var(char *env_var)
{
	int name_len = (int) strlen(env_var);
	char *val = getenv(env_var);
	int val_len;
	char *copy;

	if (val == NULL)
	    return NULL;
	val_len = (int) strlen(val);
	copy = calloc(sizeof(char),name_len + val_len + 2);

	sprintf(copy, "%s=%s", env_var, val);
	return copy;
}


/* Stop a server by killing it.
** The "server" arg should point to a stack copy of the server control
** info, because at this point we are NOT holding the server mutex (since
** killing a recalcitrant server might take a while).
*/
static void
server_stop(X100Server *server)
{
	FILE_LOCK_HANDLE lockfd;

	TRdisplay("%@ Stopping X100 server with PID %d (%s%c%s on %d)\n",
			server->pid, server->dbfarm, DIR_SEP, server->dbname, server->port);

	x100_kill(server->pid);
#ifdef NT_GENERIC
	/* wait for x100 to exit, i.e., wait for lock */
	/* Don't bother on unix, believe in waitpid */
	lockfd = lock_dblockfile(server->dbfarm, server->dbname, F_LOCK);
	if (lockfd == FILE_LOCK_BAD)
		TRdisplay("db_kill_if_open: error waiting for x100 to stop\n");
	file_unlock(lockfd);
#endif
}


static void
server_create(X100Server *server, char *dbfarm, char *dbname, int port)
{

	memset(server, 0, sizeof(*server));
	strcpy(server->dbfarm, dbfarm);
	strcpy(server->dbname, dbname);
	server->port = port;
	gettimeofday(&server->start_time, NULL);
	/* authpass is filled in by caller */
}


static void
set_default_vwlog(char *buf, size_t sz, char *ingpath)
{
	snprintf(buf, sz-1,
		"default=file:info,file=timestamp:showlevel:pid:flush,"
		"filename=\"%s%cingres%cfiles%cvectorwise.log\"", ingpath,
		DIR_SEP, DIR_SEP, DIR_SEP);
	buf[sz-1] = 0;
}


/* server_start starts a server and fills in its server entry.
** Returns TRUE if it worked and FALSE if it didn't.
** Caller should be holding the server mutex.
*/
#ifndef NT_GENERIC
static bool
server_start(X100Server *server, char *dbfarm, char *dbname, int port)
{
	char buf[300];		/* X100 reply, should be much shorter! */
	char x100bin[MAX_LOC+50];
	int ret, i, count = 0;
	int nullfd;
	int pid;

	char *x100_envp[] = {NULL, NULL, NULL, NULL};
	char *x100_argv[] = {
		"iix100",
		"--dbfarm", NULL,
		"--dbname", NULL,
		"--port", NULL,
		"--set", "server/password_auth=true",
		NULL};
	char *ingpath;
	char *config;
	char *authpass;

	TRdisplay("Starting X100 server for '%s%c%s' on port %d\n",
				dbfarm, DIR_SEP, dbname, port);

	ingpath = getenv("II_SYSTEM");
	vw_assert(ingpath); /* Only for use from within Ingres */

	db_kill_if_open(dbfarm, dbname);

	/* TODO: check port availability */
	server_create(server, dbfarm, dbname, port);

	if (pipe(server->pfds) < 0) {
		TRdisplay("server_start: failed to create pipe\n");
		return FALSE;
	}

	pid = fork();
	if (pid) {
		/* Parent Process */
		server->pid = pid;
		/* Close write in parent process */
		close(server->pfds[1]);
		buf[0] = '\0';

		/* Wait for server to have started */
		ret = read(server->pfds[0], buf, sizeof(buf) - 1);
		if (ret < 0)
			TRdisplay("server_start: error reading from pipe: %s\n",
					strerror(errno));

		buf[ret] = '\0'; /* make null-terminated */
		if (strstr(buf, "#X100SERVER: Ready.") == NULL)
			TRdisplay("server_start: received unexpected message: %s\n", buf);
		close(server->pfds[0]);
		read_authpass(dbfarm, dbname, &server->authpass[0]);
		return TRUE;
	}

	/* Child Process */

	close(server->pfds[0]);
	if ((nullfd = open("/dev/null", O_RDONLY)) >= 0) {
	    dup2(nullfd, 0);
	    close(nullfd);
	} else {
	    close(0);
	}
	dup2(server->pfds[1], 1);
	if ((nullfd = open("/dev/null", O_WRONLY)) >= 0) {
	    dup2(nullfd, 2);
	    close(nullfd);
	} else {
	    close(2);
	}

	/* Close all file descriptors. It would be more efficient to iterate
	 * over /proc/$$/fd, but that's not so portable and more work. Just
	 * checking if this fixes the destroydb problem.
	 */

/* hpux doesn't define getdtablesize when _XOPEN_SOURCE >= 600 */
#if defined(__hpux)
        extern int getdtablesize(void);
#endif

	for (i = getdtablesize(); i > 2; i--)
		if (close(i) == 0)
			count++;

	/* environment */

	config = logger_get_config();
	if (strlen(config)) {
		x100_envp[0] = calloc(sizeof(char), strlen(config) + 7);
		sprintf(x100_envp[0], "VWLOG=%s", config);
	} else {
#define log_string_len 200
		x100_envp[0] = calloc(sizeof(char), log_string_len);
		snprintf(x100_envp[0], log_string_len, "VWLOG=");
		set_default_vwlog(x100_envp[0] + 6, log_string_len - 6, ingpath);
	}
	free(config);
	i = 1;
	x100_envp[i] = copy_env_var("HOME");
	if (x100_envp[i])
	    ++i;
	/* Careful with this one, may not be set at all.
	** It really shouldn't be needed anyway.
	** (OSX uses DYLD_LIBRARY_PATH.  AIX has traditionally used
	** LIBPATH, and HP/UX, SHLIB_PATH.)
	*/
	x100_envp[i] = copy_env_var("LD_LIBRARY_PATH");
	/* Remainder pre-set to null */

	/* command line */
	x100_argv[2] = dbfarm;
	x100_argv[4] = dbname;
	x100_argv[6] = calloc(sizeof(char), 7);
	snprintf(x100_argv[6], 7, "%d", port);

	snprintf(x100bin, sizeof(x100bin)-1, "%s%cingres%cbin%cx100_server", ingpath,
				DIR_SEP, DIR_SEP, DIR_SEP);
	x100bin[sizeof(x100bin)-1] = 0;
	if (execve(x100bin, x100_argv, x100_envp) < 0)
		TRdisplay("server_start: failed to execute server\n");

	/* TODO: free memory */
	exit(0);

	/* We never get here, but icc wants it. */
	return FALSE;
}

#else /* win32 */

static bool
server_start(X100Server *server, char *dbfarm, char *dbname, int port)
{
	char buf[300];
	char x100bin[MAX_LOC+50];
	int ret, count = 0;

	char *x100_envp = NULL;
	char *ingpath;
	char *vwlog = NULL;
	char *authpass;

	HANDLE hRead,hWrite;
	SECURITY_ATTRIBUTES sa;
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	DWORD nBytes;

	TRdisplay("Starting X100 server for '%s%c%s' on port %d\n",
				dbfarm, DIR_SEP, dbname, port);

	ingpath = getenv("II_SYSTEM");
	vw_assert(ingpath); /* Only for use from within Ingres */

	db_kill_if_open(dbfarm, dbname);

	/* TODO: check port availability */
	server_create(server, dbfarm, dbname, port);

	/* Set up the security attributes struct. */
	sa.nLength= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hRead,&hWrite,&sa,0)) {
		TRdisplay("server_start: failed to create pipe\n");
		return FALSE;
	}

	vwlog = getenv("VWLOG");
	if (vwlog == NULL)
	{
		char logpath[MAX_PATH];
		set_default_vwlog(logpath, sizeof(logpath), ingpath);
		SetEnvironmentVariable("VWLOG", TEXT(logpath));
	}

	/* command line */
	snprintf(x100bin, sizeof(x100bin)-1, "\"%s%cingres%cbin%cx100_server\" "
			"--dbfarm \"%s\" --dbname \"%s\" --port %d "
			"--set server/password_auth=true", ingpath, DIR_SEP, DIR_SEP,
			DIR_SEP, dbfarm, dbname, port);
	x100bin[sizeof(x100bin)-1] = 0;

	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = hWrite;
	si.hStdInput  = NULL;
	si.hStdError  = NULL;

	if (!CreateProcess(NULL, x100bin, NULL, NULL, TRUE, 0, x100_envp, NULL, &si, &pi))
		TRdisplay("server_start: failed to execute server\n");

	server->pid = (int) pi.dwProcessId;
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	/* now that we started the client, wait for its succesfull start */
	CloseHandle(hWrite);
	buf[0] = '\0';

	/* Wait for server to have started */
	ret = ReadFile(hRead, buf, sizeof(buf) - 1, &nBytes, NULL);
	if (!ret || !nBytes)
		TRdisplay("server_start: error reading from pipe: %s\n",
					strerror(errno));

	buf[nBytes] = '\0'; /* make null-terminated */
	if (strstr(buf, "#X100SERVER: Ready.") == NULL)
		TRdisplay("server_start: received unexpected message: %s\n", buf);
	CloseHandle(hRead);
	read_authpass(dbfarm, dbname, &server->authpass[0]);
	return TRUE;
}
#endif

/* Send a syscall, answer (if any) returned in "buf".
** Buf can be null if we don't care about the answer.
*/
static bool
send_syscall(NetBuffer *netbuf, char *authpass, char *query, char *buf, int bufsize)
{
	int err;

	NetBufferClear(netbuf);
	NetBufferPutInt(netbuf, 1);
	NetBufferPutStr(netbuf, query);
	NetBufferPutStr(netbuf, authpass);

	err = NetBufferSend(netbuf);
	if (err < 0) {
		TRdisplay("send_syscall: failed to send query\n");
		return FALSE;
	}

	err = NetBufferRecv(netbuf);
	if (err <= 0) {
		TRdisplay("send_syscall: failed to receive ack\n");
		return FALSE;
	}

	err = NetBufferGetInt(netbuf);
	if (err) { /* not accepted */
		char errmsg[200];
		NetBufferGetStr(netbuf, errmsg);
		TRdisplay("send_syscall: query not accepted: %s\n", errmsg);
		return FALSE;
	}

	NetBufferClear(netbuf); /* We already know what to expect */
	err = NetBufferRecv(netbuf);
	if (err <= 0) {
		TRdisplay("send_syscall: error receiving response\n");
		return FALSE;
	}

	err = NetBufferGetInt(netbuf);
	if (err) {
		err = NetBufferGetInt(netbuf);
		TRdisplay("send_syscall: returned error: %d\n", err);
		return FALSE;
	}

	err = NetBufferGetInt(netbuf);
	if (err != 1 ) { /* we expect one row only */
		TRdisplay("send_syscall: unexpected number of rows\n");
		return FALSE;
	}

	err = NetBufferGetLen(netbuf); /* We don't care */
	if (err > BUFSIZ) {
		TRdisplay("send_syscall: response too long\n");
		return FALSE;
	}


	if (buf) {
		if (NetBufferGetLen(netbuf) + 1 > bufsize)
		    return FALSE;
		NetBufferGetStr(netbuf, buf);
	}

	/* We already have our response, but we need to receive
	 * the end of result set message.
	 */
	err = NetBufferRecv(netbuf);
	if (err <= 0) {
		TRdisplay("send_syscall: error receiving response\n");
		return FALSE;
	}

	err = NetBufferGetInt(netbuf);
	if (err) {
		err = NetBufferGetInt(netbuf);
		TRdisplay("send_syscall: returned error: %d\n", err);
		return FALSE;
	}

	err = NetBufferGetInt(netbuf);
	if (err != 0) { /* we expect one row only */
		TRdisplay("send_syscall: unexpected number of rows\n");
		return FALSE;
	}
	return TRUE;
}

static bool
send_syscall_expect(NetBuffer *netbuf, char *authpass, char *query, const char *expected)
{
	char buf[500];
	if (!send_syscall(netbuf, authpass, query, buf, sizeof(buf)))
		return FALSE;
	if (strcmp(buf, expected) != 0) {
		TRdisplay("send_syscall: received %s instead of %s\n",
					buf, expected);
		return FALSE;
	}
	return TRUE;
}

static bool
server_check(X100Server *server)
{
	int sockfd;
	NetBuffer netbuf;
	bool ok = TRUE;
	struct stat lockstat;
	int fd, pid = 0;

	sockfd = setup_connection(inet_addr(LOCALHOST), server->port);
	if (sockfd < 0) {
		return FALSE;
	}

	NetBufferInit(&netbuf, sockfd);

	if (!send_syscall_expect(&netbuf, server->authpass, DBFARM_QUERY, server->dbfarm) ||
		!send_syscall_expect(&netbuf, server->authpass, DBNAME_QUERY, server->dbname)) {
		TRdisplay("server_check: server at port %d not for %s%c%s, terminating\n",
				server->port, server->dbfarm, DIR_SEP, server->dbname);
		send_syscall_expect(&netbuf, server->authpass, TERMINATE_QUERY, "OK");
		ok = FALSE;
	}

	NetBufferClose(&netbuf);
	if (!ok)
		return FALSE;

	/* Server seems to be for real, make sure it's the one we started,
	** also correct the pid if server entry seems wrong.
	*/
	fd = open_dblockfile(server->dbfarm, server->dbname);
	if (fd >= 0) {
		fstat(fd, &lockstat);
		pid = get_pid_from_lockfile(fd);
		close_dblockfile(fd);
		if (pid > 0 && server->pid != pid) {
			TRdisplay("server_check: expected pid %d, got %d, updating\n",
					server->pid, pid);
			server->pid = pid;
		}

		if (server->start_time.tv_sec > lockstat.st_mtime && pid) {
			TRdisplay("server_check: old server, killing pid %d\n", pid);
			return FALSE;
		}
	}
	return TRUE;
}


static int x100_find_server(X100Context *x100ctx, char *dbname)
{
	X100Server *server = NULL;
	int i = 0;

	while (i <= x100ctx->highwater_server) {
		server = &x100ctx->servers[i];
		if (server->port > 0
		  && strcmp(server->dbname, dbname) == 0)
			break;
		++i;
	}

	if (i <= x100ctx->highwater_server)
		return i;
	return -1;
}

static X100Server *x100_new_server(X100Context *x100ctx, char *dbname,
		X100ErrorCode *error)
{
	int i, port;

	for (i = 0; i <= x100ctx->highwater_server; i++)
		if (x100ctx->servers[i].port == 0)
			break;
	if (i >= x100ctx->max_servers) {
		TRdisplay("x100_connect: maximum number of %d servers reached\n",
					x100ctx->max_servers);
		*error = X100_ERR_SERVER_LIMIT;
		return NULL;
	}

	if (i > x100ctx->highwater_server)
		x100ctx->highwater_server = i;
	port = x100ctx->startport + i;
	if (!server_start(&x100ctx->servers[i], x100ctx->dbfarm, dbname, port)) {
		memset(&x100ctx->servers[i], 0, sizeof(X100Server));
		TRdisplay("x100_connect: failed to start server\n");
		*error = X100_ERR_SERVER_START;
		return NULL;
	}

	return &x100ctx->servers[i];
}

/* Stop server indexed by i in the servers array.  Call holding the
** server mutex, but we'll release it before actually stopping the
** server (since killing it might take a while if x100 is not cooperating).
*/
static void x100_stop_by_idx(X100Context *x100ctx, int i)
{
	X100Server *server;
	X100Server server_copy;

	vw_assert(i < x100ctx->max_servers && i >= 0);

	server = &x100ctx->servers[i];
	server_copy = *server;

	memset(server, 0, sizeof(X100Server));
	if (i == x100ctx->highwater_server)
	{
	    while (--i >= 0)
	    {
		if (x100ctx->servers[i].port > 0)
		    break;
	    }
	    x100ctx->highwater_server = i;
	}
	CSv_semaphore(&x100ctx->server_mutex);

	server_stop(&server_copy);
}


/* DIlistdir calls a routine to do its actual work, which in this case
** is to kill any running x100-servers for the directories listed in
** the dbfarm.
*/
static STATUS
x100_init_process_dir(X100Context *ctx, char *dirname, i4 dirlen,
	CL_ERR_DESC *err_code)
{
	char name[256];

	if (dirlen > sizeof(name)-1)
		return OK;		/* no point looking */
	STlcopy(dirname, &name[0], dirlen);
	db_kill_if_open(ctx->dbfarm, name);
	return OK;
}


/* Startup initialization, called from LGK (!) after the X100 context area
** is allocated.  The server isn't fully up yet so there is no danger
** of any thread conflict.
** The "already-inited" flag indicates that LGK was already set up, and
** all we need to do here is remember the x100 context address
** and register the attention handler.
** If already-inited is TRUE then max_servers is not passed in and is zero!
*/
DB_STATUS
x100_init(X100Context *ctx, int max_servers, bool already_inited)
{
	DB_STATUS scf_status;
	SCF_CB scf_cb;

	x100_global_ctx = ctx;		/* Save for everyone */
	if (!already_inited) {
	    char *system_dir = NULL;
	    char *installation = NULL;
	    CL_ERR_DESC err_code;
	    DI_IO dummy;		/* unused arg to DIlistdir */
	    i4 err;
	    i4 x, y;

	    /* X100Context includes one X100Server, clear the rest too */
	    memset(ctx, 0, sizeof(*ctx) + (max_servers-1) * sizeof(X100Server));
	    CSw_semaphore(&ctx->server_mutex, CS_SEM_MULTI, "x100ctx mutex");

	    /* Get the root dbfarm */
	    NMgtIngAt("II_VWDATA", &system_dir);
	    if (!system_dir || !*system_dir)
		NMgtIngAt("II_DATABASE", &system_dir);
	    vw_assert (system_dir && *system_dir);
	    snprintf(&ctx->dbfarm[0], sizeof(ctx->dbfarm)-1,
		"%s%cingres%cdata%cvectorwise", system_dir, DIR_SEP, DIR_SEP, DIR_SEP);
	    ctx->dbfarm[sizeof(ctx->dbfarm)-1] = 0;
	    NMgtAt("II_INSTALLATION", &installation);
	    vw_assert (installation && strlen(installation) >= 2);

	    /* Installation ID is supposed to be letters or numbers, squish to
	    ** 6 bits each, leaving 3 bits for a server count.  This could in
	    ** theory lead to trouble if someone is running multiple installations
	    ** with sequential installation ID's and lots of databases, but
	    ** oh well.  The whole business of assigning a port without checking
	    ** it is wrong (see server_start) and needs fixed.  Maybe the
	    ** x100 server should return the port number in its startup reply
	    ** to stdout.  (and maybe only if authpass is on the command line?)
	    */
	    x = installation[0];
	    y = installation[1];
	    if (x >= '0' && x <= '9')
		x = x - '0';
	    else if (x >= 'A' && x <= 'Z')
		x = x - 'A' + 10;
	    else if (x >= 'a' && x <= 'z')
		x = x - 'a' + 36;
	    else
		x = 63;
	    if (y >= '0' && y <= '9')
		y = y - '0';
	    else if (y >= 'A' && y <= 'Z')
		y = y - 'A' + 10;
	    else if (y >= 'a' && y <= 'z')
		y = y - 'a' + 36;
	    else
		y = 63;
	    ctx->startport = (1 << 15) + (x << 9) + (y << 3);

	    ctx->max_servers = max_servers;
	    ctx->highwater_server = -1;

	    /* traverse directories in dbfarm and kill existing servers */
	    (void) DIlistdir(&dummy, ctx->dbfarm, strlen(ctx->dbfarm),
			x100_init_process_dir, (PTR) ctx, &err_code);
	} /* !already_inited */

	/* Register the "VW facility" interrupt handler with SCF.
	** It will be called for various attention conditions.
	*/
	scf_cb.scf_length = sizeof(scf_cb);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_VWF_ID;
	scf_cb.scf_ptr_union.scf_afcn = x100_interrupt_handler;
	scf_cb.scf_session = DB_NOSESSION;
	scf_cb.scf_nbr_union.scf_amask = SCS_ALWAYS_MASK;
	scf_status = scf_call(SCS_DECLARE, &scf_cb);

	return scf_status;
}


/* Make a new connection, returning the connection struct.
** As a special hack, if "errptr" is null, we assume it's a special
** transient connection (e.g. for sending an interrupt) and don't
** bother asking for our own thread-id.
*/
X100Connection *
x100_connect(char *call_dbname, i4 *errptr)
{
	char dbname[DB_DB_MAXNAME+1];
	X100Context *x100ctx = x100_global_ctx;
	X100ErrorCode *error = (X100ErrorCode *) errptr;
	X100ErrorCode errtoss;
	X100Server *server = NULL;
	X100Connection *x100conn;
	int i;
	int s_addr_;

	if (error == NULL)
	    error = &errtoss;

	vw_assert(call_dbname);
	/* Make a local copy because the caller might be using the
	** server dbname to pass in!  which we'll wipe if the server
	** is toast and gets killed/restarted.
	*/
	strcpy(dbname, call_dbname);

	/* Find a valid server.  If we find a junk, bogus, or dead one,
	** try to kill it and/or clean it up, then try again.
	*/
	do
	{
	    server = NULL;
	    CSp_semaphore(TRUE, &x100ctx->server_mutex);

	    i = x100_find_server(x100ctx, dbname);
	    if (i < 0)
		break;
	    server = &x100ctx->servers[i];
	    if (server_check(server))
		break;
	    x100_stop_by_idx(x100ctx, i);	/* Releases the mutex */
	} while (1);

	if (server == NULL)
		server = x100_new_server(x100ctx, dbname, error);

	/* Don't need the server mutex now either way.  If error,
	** *error has already been set.
	*/
	CSv_semaphore(&x100ctx->server_mutex);

	if (server == NULL) {
		return NULL;
	}

	x100conn = malloc(sizeof(*x100conn));
	memset(x100conn, 0, sizeof(*x100conn));
	if (!x100conn) {
		TRdisplay("x100_connect: failed to allocate memory\n");
		*error = X100_ERR_CONNECTION;
		return NULL;
	}

	x100conn->sockfd = -1;
	x100conn->server = server;

	s_addr_ = inet_addr(LOCALHOST);

	x100conn->sockfd = setup_connection(s_addr_, server->port);
	if (x100conn->sockfd < 0) {
		TRdisplay("x100_connect: failed to connect to server\n");
		x100_disconnect(x100conn);
		*error = X100_ERR_CONNECTION;
		return NULL;
	}

	x100conn->netbuf = malloc(sizeof(NetBuffer));
	NetBufferInit(x100conn->netbuf, x100conn->sockfd);
	/* Don't bother getting thread-id if transient connection */
	if (errptr != NULL)
	    (void) send_syscall(x100conn->netbuf, server->authpass, TID_QUERY, x100conn->tid, sizeof(x100conn->tid));

	return x100conn;
}

void
x100_disconnect(X100Connection *x100conn)
{
	vw_assert(x100conn);

	if (x100conn->netbuf != NULL) {
		NetBufferClose(x100conn->netbuf);
		free(x100conn->netbuf);
	}

	free(x100conn);
}


/* Send X100 an interrupt for the given session.  We do this with a
** syscall sent via a temporary connection.
*/
void
x100_interrupt_query(X100Connection *x100conn)
{
	X100ErrorCode error;
	X100Connection *c2;
	X100Context *x100ctx = x100_global_ctx;
	char query[100];
	int i;

	vw_assert(x100conn);

	CSp_semaphore(TRUE, &x100ctx->server_mutex);
	i = x100_find_server(x100ctx, x100conn->server->dbname);
	CSv_semaphore(&x100ctx->server_mutex);
	if (i < 0) return;	/* Ignore bogus request */
	c2 = x100_connect(x100conn->server->dbname, NULL);
	if (!c2) return;
	snprintf(query, sizeof(query)-1, KILL_TID_QUERY, x100conn->tid);
	query[sizeof(query)-1] = 0;
	(void) send_syscall(c2->netbuf, c2->server->authpass, query, NULL, 0);
	x100_disconnect(c2);
}

static void
x100_stop(X100Context *x100ctx, char *dbname)
{
	int i;

	vw_assert(dbname);

	CSp_semaphore(TRUE, &x100ctx->server_mutex);
	i = x100_find_server(x100ctx, dbname);
	if (i >= 0)
	    x100_stop_by_idx(x100ctx, i);
	else
	    CSv_semaphore(&x100ctx->server_mutex);
}

void
x100_exit(void)
{
	int i;
	X100Context *x100ctx = x100_global_ctx;

	if (x100ctx == NULL)
	    return;		/* I guess we never got started */

	for (i = 0; i < x100ctx->max_servers; i++)
	{
	    if (x100ctx->servers[i].port)
	    {
		/* Keep stop-by-idx happy by getting the mutex;  don't really
		** need it since this is shutdown and we're the last man out
		*/
		CSp_semaphore(TRUE, &x100ctx->server_mutex);
		x100_stop_by_idx(x100ctx, i);
	    }
	}
	x100ctx->highwater_server = -1;
}

void
x100_destroydb(char *dbname) {
	char path[MAX_LOC + DB_DB_MAXNAME + 2];
	int err = 0;

	x100_stop(x100_global_ctx, dbname);

	sprintf(path, "%s%c%s", x100_global_ctx->dbfarm, DIR_SEP, dbname);
	/* FIXME stop using deltree, use DIlistdir as above, can probably
	** get rid of deltree after that.
	*/
	err = deltree(path);
	if (err < 0)
		TRdisplay("%@ x100_destroydb: Error deleting %s: %s\n", path, strerror(-err));

}


/* Check whether an x100 server appears to be alive at all (as opposed to
** being usable, which is what server_check() does.)  If the x100 server
** appears to be dead, reap the dead process (on unix) and clean up the
** server entry so that future connections can start a new one.
** Call holding the server mutex.
*/
static void
x100_local_check_dead(X100Context *x100ctx, X100Server *server)
{
    int idx, ret;

    if (server->port <= 0)
	return;			/* Someone else got here first */

#ifndef NT_GENERIC
    /* Wait (nowait) before checking for process existence, it appears
    ** that a kill with 0 signal succeeds even if the process is a zombie.
    ** The wait will reap zombies and then we'll get a better indication.
    */
    (void) waitpid(server->pid, NULL, WNOHANG);  /* reap any defunct process */
    ret = kill(server->pid, 0);
    if (ret == 0 || errno != ESRCH)
	return;			/* Looks like it's still there */

    /* Cleanup below */
#else
    /* Windows could use PCis_alive but that routine does a full enumeration.
    ** seems overkill;  we ought to be able to get process info for x100
    ** server since we spawned it.
    */
    {
	HANDLE hProcess;
	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, server->pid);
	if (hProcess != NULL)
	{
	    CloseHandle(hProcess);
	    return;			/* Looks like it's still there */
	}
    }
#endif

    /* Doesn't seem to be there any more, clean up the server entry */

    idx = server - &x100ctx->servers[0];
    memset(server, 0, sizeof(X100Server));
    if (idx == x100ctx->highwater_server)
    {
	while (--idx >= 0)
	{
	    if (x100ctx->servers[idx].port > 0)
		break;
	}
	x100ctx->highwater_server = idx;
    }

} /* x100_local_check_dead */


void
x100_server_check_dead(X100Server *server)
{
    X100Context *x100ctx = x100_global_ctx;

    CSp_semaphore(TRUE, &x100ctx->server_mutex);
    x100_local_check_dead(x100ctx, server);
    CSv_semaphore(&x100ctx->server_mutex);
} /* x100_server_check_dead */



/* The dead process detector thread calls this periodically to see if
** the x100 servers look OK
*/
void
x100_check_dead(void)
{
	X100Context *x100ctx = x100_global_ctx;
	X100Server *server;

	if (x100ctx == NULL)
	    return;		/* Wildly unlikely, maybe impossible */

	CSp_semaphore(TRUE, &x100ctx->server_mutex);
	for (server = &x100ctx->servers[x100ctx->highwater_server];
	     server >= &x100ctx->servers[0];
	     --server)
	{
	    if (server->port > 0)
		x100_local_check_dead(x100ctx, server);
	}
	CSv_semaphore(&x100ctx->server_mutex);
} /* x100_check_dead */


#endif /* conf_IVW */
