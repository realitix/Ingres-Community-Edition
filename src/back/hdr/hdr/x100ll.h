/*
 * Copyright (C) 2009-2011 VectorWise B.V.
 * All Rights Reserved.
 *
 * Maintainer: kschendel
 */

#ifndef _X100LL_H
#define _X100LL_H

#include <lo.h>		/* For MAX_LOC */

/*
** File: x100ll.h -- "Low level" definitions for the X100/Ingres interface.
**
** Description:
**	This header describes structures needed to control the X100
**	interface generally, and X100 servers and connections specifically.
**
**	Adapted from the original server_mngmnt.h from x100.
**
** Edit History:
**	24-May-2011 (kschendel) m1260
**	    Smash-and-grab pass, add some minimal comments.
**	17-Jun-2011 (kschendel) m1930
**	    Add check-dead prototypes.
*/


/*
** Name: X100Server -- Access info for one X100 server
**
** Description:
**	The X100Server structure describes the existence of and
**	access to a running X100 server.  Unlike Ingres servers, there's
**	one X100 server per database.
**
**	X100Server entries aren't found running around loose, they are
**	bound up in an X100Context wrapper.  This was done so that X100
**	server context could be accessible to any server process via
**	shared memory ... see the X100 context.
**
** Edit History:
**	24-May-2011 (kschendel) m1260
**	    Adapt from server_mngmnt.
*/

#define AUTHPASS_MAX_SIZE 80

struct X100Server
{
    ALIGN_RESTRICT toss;		/* Guarantee alignment */
    char	dbfarm[MAX_LOC+1];	/* Path to dbfarm for database */
    char	dbname[DB_DB_MAXNAME+1]; /* Blank-trimmed, null terminated */
    char	authpass[AUTHPASS_MAX_SIZE+1]; /* Secret magic word */
    struct timeval start_time;		/* wall-clock server start time */
    i4		port;			/* Socket port, zero if no server */
    i4		pid;			/* Process ID */
    i4		pfds[2];		/* Pipe FD's */
};

typedef struct X100Server X100Server;

/*
** Name: X100Context -- Global X100 interface context
**
** Description:
**	The X100Context structure is roughly equivalent to the "facility
**	server control block" of a standard Ingres facility.  (e.g. the
**	DM_SVCB, PSF_SRVBLK, etc).  It's not *quite* the same, since there
**	is exactly one X100Context per installation (not per server).
**	That's because the X100Context lives in shared memory;  in
**	the log/lock shared memory, to be precise.
**
**	(and why on earth there?  mainly because the Windows port doesn't
**	use a "system" shared memory segment like unix does, and the
**	other installation processes likely to want to get at X100 server
**	info all have DMF's ... such as the dmfjsp.  Windows DOES have
**	a lock/log shared memory, and I didn't feel like inventing a whole
**	new shared memory segment just for X100.)
**
**	The X100Context is mainly a device for storing and maintaining
**	X100Server entries.
**
** Edit History:
**	24-May-2011 (kschendel) m1260
**	    Adapt from server_mngmnt.
*/

struct X100Context
{
    ALIGN_RESTRICT toss;		/* Guarantee alignment */
    char	dbfarm[MAX_LOC+1];	/* Path to root dbfarm */
    i4		max_servers;		/* Number of server entries total */
    i4		highwater_server;	/* Highest active entry index */
    i4		startport;		/* Add entry index to get server port */
    CS_SEMAPHORE server_mutex;		/* Lock while fooling with entries */
    X100Server	servers[1];		/* Actually sized [max_servers] */
};

typedef struct X100Context X100Context;



typedef struct X100Connection {
	struct X100Server *server;
	int sockfd;
	struct _NetBuffer *netbuf;
	char tid[32];
} X100Connection;


FUNC_EXTERN DB_STATUS x100_init(X100Context *ctx, i4 max_servers, bool);

/* Connect and disconnect to a server, starting if needed */
/* The last parm is really an X100ErrorCode * but that enum is defined
** on the x100 side, and I don't want to drag it in everywhere x100ll is used.
*/
FUNC_EXTERN X100Connection *x100_connect(char *dbname, i4 *error);
FUNC_EXTERN void x100_interrupt_query(X100Connection *x100conn);
FUNC_EXTERN void x100_disconnect(X100Connection *x100conn);

/* Close all servers, called when RCP is exiting */
FUNC_EXTERN void x100_exit(void);

/* Dead process checker(s) */
FUNC_EXTERN void x100_server_check_dead(X100Server *);
FUNC_EXTERN void x100_check_dead(void);

/* Stop server, delete x100 files */
FUNC_EXTERN void x100_destroydb(char *dbname);

/* Called at startup by SCF (scd) to set the x100 "session cb" pointer offset
** for GET_X100_SCB.
*/
FUNC_EXTERN void x100_set_cb_offset(i4 offset);

/* x100_init will register this handler with SCF. */
FUNC_EXTERN STATUS x100_interrupt_handler(i4 eid, void *state);

#endif /* _X100LL_H */
