/*
 * Copyright (C) 1997-2009 VectorWise.
 * All Rights Reserved.
 */

#ifndef VMS
#include <sys/types.h>  /* used by socket */
#if defined(NT_GENERIC)
#include<winsock.h>
#else
#include <sys/socket.h> /* used by socket */
#include <unistd.h>     /* used by socket */
#include <netinet/in.h> /* to convert numbers between host byte ordering and network byte ordering */
#include <arpa/inet.h>  /* to convert dotted IP addresses into 32-bit integer, and vice versa */
#include <netdb.h>      /* to convert domain names into IP addresses */
#endif   /* if defined(NT_GENERIC) */
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#endif

#include <compat.h>
#include <iicommon.h>
#include <gca.h>
#include <adf.h>
#include <dbdbms.h>
#include <dmf.h>
#include <nm.h>
#include <cs.h>
#include <x100.h>
#include <x100ll.h>
#include <x100client.h>

#if defined(conf_IVW)

#include <x100/netbuffer.h>
#include <x100/vwlog.h>
#include <x100/tableconv.h>

#include <iitableconv.h>
#include <ulf.h>

/*
** File: x100client.c -- X100 client interface
**
** Description:
**	This file implements the lower level nuts and bolts of
**	the X100 / Vectorwise interface.  It understands how to
**	send queries to X100 and get results back.
**
**	--- need quickie description of the protocol here ---
**
**	FIXME this file lives in a sort of no-mans land between Ingres
**	and X100.  It runs in the Ingres context, but the coding style
**	and many utilities (such as logging) are X100-ish.  It would
**	probably be a good idea to translate at least the logging so
**	that it uses TR and/or ule facilities instead of vwlog.
**
** Edit History:
**	2009-2011 (michal?)
**	    original
**	24-May-2011 (kschendel) m1260
**	    Add this edit history.  Add timeout stuff to allow query
**	    cancelling.  Move a few bits of the interface startup to
**	    x100ll.c which is the lowest level connection stuff.  Move
**	    X100SessionState to be part of the SCF SCB, as the "VW
**	    facility CB", so that X100 can participate in attn handling.
**	08-Jun-2011 (drivi01)
**	    Move x100_x100cb_offset global define to x100lldata file to
**	    properly export it on Windows in iilibdmfdata.dll.
**	    Reference the variable here instead.
**	16-Jun-2011 (kschendel) m1930
**	    Rename check before send;  if previous network error, don't
**	    send;  fix typo in bulkload error on final batch.
**	    For network errors, see if server has died so that we can
**	    reclaim the server entry.  (Particularly if someone has sent a
**	    TERMINATE to the server!)
**      23-Jun-2011 (stial01)
**          x100_session_init() check for DB_NOSESSION, which is a JSP x100
**          session that does not have SCF session.
**      18-Jul-2011 (horda03) b125253
**          Fic VMS build (currently VW not built).
**	1-Aug-2011 (kschendel) SIR 125587
**	    Add connect-if-needed for vwloader protocol.
**	1-Sep-2011 (kschendel) b125689
**	    Fiddle with interrupt (attn) handling to paper over some of the
**	    cracks.  (A full solution needs b125701 fixed as well, to handle
**	    races in cross-thread attns.)  In particular, make sure we send
**	    a kill / interrupt to x100_server if it's chewing on a query,
**	    no matter what Ingres was doing when the attn arrived.
**	    Previously, the kill was only sent if we were actually waiting
**	    on x100-server.
*/


#define VWLOG_FACILITY "X100CLIENT"

#define MAX_ERRMSG_SIZE 1280

/* Timeout waiting for the x100 server to process a query.  This is so
** that we can poll for an interrupt from our session client, or for an
** attn raised by some other session (e.g iimonitor kill).
** The CS cancel-check interval is relatively large, currently 4 seconds,
** so there is little point in making this timeout very small.
** Slightly over one second is fine.
*/
#define X100_CANCELCHECK_TIMEOUT_USEC 1010000

struct column {
	char *name;
	char *type;
	int isconst;
	int isfloat;
	int isfixed;
	int width;
	unsigned char *data;
};


struct X100QueryState {
	struct X100SessionState *session;
	int    nrcolumns;      /* the number of columns the query results will contain */
	int    nrtuples;       /* #tuples each data buffer contain */
	int    tuplepos;       /* position of last tuple sent to Ingers */
	int    tuplesbuf;      /* number of tuples in the buffer */
	struct column *queryres; /* column desctiptors of the query result */
	int    *columnmap;     /* column mapping */
	struct table_converter * conv;
	void  *conv_col;
	void  *conv_row;
	int    row_size;
	int    attr_count;
};

#endif /* conf_IVW */

/* Always define this offset so that the GET_X100_SCB macro always
** works, conf_IVW or not.  It will return NULL for non-IVW builds.
*/

/* Offset within SCF-SCB of the pointer to the x100-session-state CB.
** This is used by the GET_X100_SCB macro.
*/
GLOBALREF i4 x100_x100cb_offset;

/* This stub is called by SCF initialization to set the offset within
** an SCF-SCB of the pointer to the session X100 CB (X100SessionState).
** This offset allows us to retrieve the X100 session CB given the
** CS_SID / session ID of the session, which is in fact just an SCF SCB
** address.  No need for thread-sync here, this is server init.
*/
void
x100_set_cb_offset(i4 offset)
{
    x100_x100cb_offset = offset;
}

/* Most of the rest is under conf_IVW... */

#if defined(conf_IVW)

static void *
x100_calloc(size_t nmemb, size_t size)
{
	void *res = calloc(nmemb, size);
//	log_trace("calloc %p (%d x %d)", res, (int)nmemb, (int)size);
	return res;
}
static void *
x100_malloc(size_t size)
{
	void *res = malloc(size);
//	log_trace("malloc %p (%d)", res, (int)size);
	return res;
}
static void
x100_free(void *ptr)
{
	free(ptr);
//	log_trace("free %p", ptr);
}

static void
free_nonfixed_data(struct X100QueryState *is) {
	int i;
	struct column *queryres = is->queryres;
	for (i = 0; queryres[i].name; i++) if (!queryres[i].isfixed) {
		int j, n = queryres[i].isconst ? 1 : is->nrtuples;
		void **ptrs = (void**)queryres[i].data;
		for (j = 0; j < n; j++) {
			if (ptrs[j]) x100_free(ptrs[j]);
			ptrs[j] = 0;
		}
	}
}

/* Close down a session, disconnecting from the X100 server.
** Stuff attached to the session state block is free'd.
** The session state block itself belongs to the sequencer, and must
** not be free'd;  but it is cleared out.
** (Note that for JSP sessions, the session state is indeed malloc'ed
** here, but JSP sessions don't need nice cleanup because the whole
** process will disappear at end.)
*/
void
x100_session_free(struct X100SessionState *state)
{
	int i;
	if (state) {
		if (state->conn)
			x100_disconnect(state->conn);
		if (state->error) {
			for (i = 0; i < state->error->param_count; i++)
				x100_free(state->error->params[i]);
			x100_free(state->error);
		}
		memset(state, 0, sizeof(struct X100SessionState));
	}
}

static int
x100_skip_output(struct X100SessionState *state, bool timedrecv);

static void
x100_session_query_free(struct X100QueryState *state)
{
	int i;
	if (state) {
		if (state->session &&
		  (state->session->connection_state == CONNECTION_STATE_RESULT
		   || state->session->connection_state == CONNECTION_STATE_ACK))
		{
		    x100_interrupt_query(state->session->conn);
		    x100_skip_output(state->session, FALSE);
		    state->session->connection_state = CONNECTION_STATE_READY;
		}
		if (state->queryres)
		{
		    struct column *queryres = state->queryres;
		    free_nonfixed_data(state);
		    for (i = 0; queryres[i].name; i++)
		    {
			x100_free(queryres[i].name);
			if (queryres[i].type) x100_free(queryres[i].type);
			if (queryres[i].data) x100_free(queryres[i].data);
		    }
		    x100_free(state->queryres);
		}
		if (state->columnmap) x100_free(state->columnmap);
		if (state->conv) table_conv_free(state->conv);
		if (state->conv_col) x100_free(state->conv_col);
		if (state->conv_row) x100_free(state->conv_row);
		x100_free(state);
	}
}

void
x100_query_free(struct X100QueryState *state)
{
	if (state) {
		x100_session_query_free(state);
	}
}

struct X100Error *
x100_get_error(struct X100QueryState *state)
{
	if (state == NULL || state->session == NULL)
	    return NULL;
	return state->session->error;
}

struct X100Error *
x100_session_get_error(struct X100SessionState *state)
{
	if (state == NULL)
	    return NULL;
	return state->error;
}

/* Clear error and interrupt state of the session.  QEF will call this
** from qef-call, at the same point at which QEF's interrupt flags are
** initialized.
*/
void
x100_session_clear_error(struct X100SessionState *state)
{
	int i;
	if (!state) return;
	if (state->error) {
		for (i = 0; i < state->error->param_count; i++)
			x100_free(state->error->params[i]);
	}
	state->error->code = 0;
	state->error->param_count = 0;
	state->interrupted = FALSE;
}

/* Set error state, code params for session error.
** If the error is X100_ERR_NETWORK, which is strictly internally (to
** x100client) generated, we'll set the connection state to "error",
** thus preventing any more comms to x100-server in this session.
*/
static void
x100_session_set_error(struct X100SessionState *state, X100ErrorCode code, char *param)
{
	/* For convenience, if it's X100_ERR_NETWORK, set the connection state
	** to ERROR so that we don't need to do it a bazillion places.
	** Also, if we've managed a real connection, see if maybe the
	** server has gone away (crash or TERMINATE).
	*/
	if (code == X100_ERR_NETWORK) {
		if (state->connection_state != CONNECTION_STATE_ERROR
		  && state->conn && state->conn->server)
			x100_server_check_dead(state->conn->server);
		state->connection_state = CONNECTION_STATE_ERROR;
	}
	else if (state->error->code) {
		return;
	}
	state->error->code = code;
	if (param) {
		state->error->params[0] = strdup(param);
		if (!state->error->params[0]) return;
		state->error->param_count = 1;
	}
}

static void
x100_query_set_error(struct X100QueryState *state, X100ErrorCode code, char *param)
{
	x100_session_set_error(state->session, code, param);
}

static void
x100_session_set_server_error(struct X100SessionState *state)
{
	NetBuffer *netbuf = state->conn->netbuf;
	int numparams, i;
	if (!state || !state->conn) return;
	if (state->error->code) return;
	state->error->code = (X100ErrorCode)NetBufferGetInt(netbuf);
	numparams = NetBufferGetInt(netbuf);
	log_debug("server returned error %d", state->error->code);
	if (numparams > MAX_X100_ERROR_PARAMS) numparams = MAX_X100_ERROR_PARAMS;
	for (i = 0; i < numparams; i++) {
		state->error->params[i] = x100_malloc( NetBufferGetLen(netbuf) + 1 );
		assert(state->error->params[i]);
		NetBufferGetStr(netbuf, state->error->params[i]);
		log_debug(" error param %d = %s", i + 1, state->error->params[i]);
	}
	state->error->param_count = numparams;
	state->connection_state = CONNECTION_STATE_READY;
}


/* Do a receive with timeout and cancel check from the other end.
** If an interrupt occurs, send the X100 end an interrupt, and
** wait (without timeout) for X100 to notice.  X100 might send back:
** - an error (probably X100_ERR_KILL, but if not we'll force it) which
**   is the normal result when it notices an interrupt;
** - A normal reply with rows, which implies that the buffer was filled
**   and formatted before it noticed the interrupt.  Just toss and keep
**   waiting for more;
** - A normal end-of-query (zeros) which implies that the other end
**   finished the query before noticing the interrupt.  Fudge up an
**   X100_ERR_KILL state and return.
** Fortunately x100-skip-output will do all the right things, we just
** need to stuff the interrupt error when it's finished.
** Returns +n = ok, 0 or -1 = error, -2 = interrupt handled
**
** Note, this mechanism can still delay quite a bit if x100 takes a long
** time to recognize an interrupt, or if it goes off into the weeds
** entirely.  Ideally we'd give up after some period of time, but that
** is going to throw the netbuffer protocol out of sync and there's no
** good way (at present) to re-sync it.  Something to FIXME eventually.
*/
static int
x100_timed_recv(struct X100SessionState *state, NetBuffer *netbuf)
{
	int res;

	for (;;) {
	    res = NetBufferTimedRecv(netbuf, X100_CANCELCHECK_TIMEOUT_USEC);
	    if (res != -2)
		return (res);
	    CScancelCheck(state->sessid);
	    if (state->interrupted) {
		x100_interrupt_query(state->conn);
		x100_skip_output(state, FALSE);
		/* Note that this clears "interrupted"... */
		x100_session_clear_error(state);
		x100_session_set_error(state, X100_ERR_KILL, NULL);
		return (-2);
	    }
	}
}


/* Before sending a query, check for interrupt already set, no point
** in even sending in that case.
*/
static int
x100_check_sendok(struct X100SessionState *sstate)
{
	if (sstate->connection_state == CONNECTION_STATE_ERROR) {
	    x100_session_set_error(sstate, X100_ERR_NETWORK, NULL);
	    return -1;
	}
	if (sstate->interrupted) {
	    x100_session_set_error(sstate, X100_ERR_KILL, NULL);
	    return -1;
	}
	return 0;
}


/* Read and toss the result from a query. */
static int
x100_skip_output(struct X100SessionState *state, bool timedrecv) {
	NetBuffer *netbuf = state->conn->netbuf;
	int res;

	for (;;) {
		if (timedrecv) {
			res = x100_timed_recv(state, netbuf);
			if (res == -2)
				return -1;	/* error already set */
		} else {
			res = NetBufferRecv(netbuf);
		}
		if (res <= 0)
			break;

		res = NetBufferGetInt(netbuf);
		log_debug("result is %d", res);
		if (res) {
			x100_session_set_server_error(state);
			return -1;
		}
		if (state->connection_state == CONNECTION_STATE_ACK) {
			state->connection_state = CONNECTION_STATE_RESULT;
			continue;
		}
		res = NetBufferGetInt(netbuf);
		log_debug("Ignored %d tuples", res);
		if (!res) {
			state->connection_state = CONNECTION_STATE_READY;
			return 0;
		}
	}
	x100_session_set_error(state, X100_ERR_NETWORK, NULL);
	return -1;
}

static int
x100_simple_query(struct X100SessionState *state, char *query)
{
	NetBuffer *netbuf = state->conn->netbuf;
	int res;

	log_debug("got simple query: [%s]", query);
	NetBufferClear(netbuf);
	NetBufferPutInt(netbuf, 1);
	NetBufferPutStr(netbuf, query);
	NetBufferPutStr(netbuf, state->conn->server->authpass);
	if (x100_check_sendok(state) < 0)
		return -1;
	if (NetBufferSend(netbuf) < 0) {
		x100_session_set_error(state, X100_ERR_NETWORK, 0);
		log_error("network error while sending query");
		return -1;
	} else {
		state->connection_state = CONNECTION_STATE_ACK;
		res = x100_timed_recv(state, netbuf);
		if (res == -2)
			return -1;	/* Interrupt error set up */
		if (res <= 0) {
			x100_session_set_error(state, X100_ERR_NETWORK, 0);
			log_error("network error while reading ack");
			return -1;
		} else {
			int err = NetBufferGetInt(netbuf);
			if (err) {
				x100_session_set_server_error(state);
				log_error("query not accepted");
				return -1;
			}
			state->connection_state = CONNECTION_STATE_RESULT;
			if (x100_skip_output(state, TRUE) < 0) return -1;
		}
	}
	return 0;
}

int
x100_session_savepoint(struct X100SessionState *state, char *sp_name, int len)
{
	int status;
	char query_string[DB_MAXNAME+sizeof("Savepoint")+2+2+1];

	if (state->conn == NULL)
	    return 0;
	strcpy(query_string,"Savepoint('");
	/* The savepoint name may not be null terminated.
	** The magic 11 is strlen("Savepoint('")
	*/
	memcpy(&query_string[11], sp_name, len);
	strcpy(&query_string[11+len], "')" );
	status = x100_simple_query(state, query_string);
	return status;
}


int
x100_session_start_txn(struct X100SessionState *state)
{
	int status;

	if (state->conn == NULL)
	    return 0;
	status = x100_simple_query(state, "/*Transaction start*/ Array([a=0])");
	return status;
}

/* If no connection, just return, else commit (or rollback). */
int
x100_session_commit(struct X100SessionState *state)
{
	int status;

	if (state->conn == NULL)
	    return 0;
	status = x100_simple_query(state, "commit");
	return status;
}


/* Issue a rollback / Abort to the X100 server, sending the savepoint
** name if any.
*/
/* **** FIXME!  Hack!
** At present (125701 not fixed), if an interrupt arrives that isn't a
** simple "end retrieve" interrupt, and if the session thread isn't
** actually waiting on x100_server to send something, the sequencer
** goes directly into its query cleanup code and the first thing it
** does is a QET_ABORT.  If an X100 query is pending, we need to kill
** it off first, or we'll get direly out of sync with the X100 server.
*/
int
x100_session_rollback(struct X100SessionState *state, char *sp_name, int len)
{
	int status;
	char query_string[DB_MAXNAME+5+2+2+1];

	if (state->conn == NULL)
	    return 0;
	/* **** FIXME! Hack for sequencer / qef foolishness. */
	if (state->connection_state == CONNECTION_STATE_ACK
	  || state->connection_state == CONNECTION_STATE_RESULT)
	{
	    x100_interrupt_query(state->conn);
	    x100_skip_output(state, FALSE);
	    x100_session_clear_error(state);
	}
	if (sp_name == NULL)
	    status = x100_simple_query(state, "abort");
	else
	{
	    strcpy(query_string,"abort('");
	    /* The savepoint name may not be null terminated.
	    ** The magic 7 is strlen("abort('")
	    */
	    memcpy(&query_string[7], sp_name, len);
	    strcpy(&query_string[7+len], "')" );
	    status = x100_simple_query(state, query_string);
	}
	return status;
}

struct type_properties {
	char *name;
	int width;
	int isfloat;
} type_prop[] = {
	"schr", 1, 0,
	"uchr", 1, 0,
	"ssht", 2, 0,
	"usht", 2, 0,
	"sint", 4, 0,
	"uint", 4, 0,
	"slng", 8, 0,
	"ulng", 8, 0,
	"flt",  4, 1,
	"dbl",  8, 1,
	"s128", 16, 0,
	"uidx", sizeof(size_t), 0, //not sure if correct
	"str", -1, 0,
};

static void
x100_query_parse_format(struct X100QueryState *state)
{
	NetBuffer *netbuf = state->session->conn->netbuf;
	int i;
	struct column *queryres;

	queryres = state->queryres = x100_calloc(sizeof(struct column), state->nrcolumns + 1);
	assert(queryres);
	for (i=0; i<state->nrcolumns; i++) {
		queryres[i].name = x100_malloc( NetBufferGetLen(netbuf) + 1 );
		assert(queryres[i].name);
		NetBufferGetStr(netbuf, queryres[i].name);
	}
	for (i=0; i<state->nrcolumns; i++) {
		int j;
		char stype[40];
		queryres[i].isconst = NetBufferGetInt(netbuf);
		queryres[i].type = x100_malloc( NetBufferGetLen(netbuf) + 1 );
		assert(queryres[i].type);
		NetBufferGetStr(netbuf, queryres[i].type);
		NetBufferGetStr(netbuf, stype);
		if (!*queryres[i].type) {
			x100_free(queryres[i].type);
			queryres[i].type = x100_malloc(strlen(stype)+1);
			strcpy(queryres[i].type, stype);
		}
		for (j=0; j < sizeof(type_prop)/sizeof(*type_prop); j++)
			if(!strcmp(type_prop[j].name,stype)) {
				queryres[i].isfixed = type_prop[j].width > 0;
				queryres[i].width = type_prop[j].width > 0 ? type_prop[j].width : sizeof(void*);
				queryres[i].isfloat = type_prop[j].isfloat;
			}
		if (!queryres[i].width) {
			log_error("Unknown type %s/%s returned by query\n",queryres[i].type,stype);
			queryres[i].width = 8;
		}
		queryres[i].data = x100_calloc(queryres[i].width *
			(queryres[i].isconst ? 1 : state->nrtuples), 1);
		assert(queryres[i].data);
	}
}

static int
query_next(struct X100QueryState *state)
{
	int i, j, nrtuples, res;
	struct column *queryres = state->queryres;
	NetBuffer *netbuf = state->session->conn->netbuf;

	res = x100_timed_recv(state->session, netbuf);
	if (res == 0) {
		x100_query_set_error(state, X100_ERR_NETWORK, 0);
		log_debug("server closed connection unexpectedly");
		return -1;
	}
	if (res == -2)
		return -1;	/* interrupt, all set up */
	if (res < 0) {
		x100_query_set_error(state, X100_ERR_NETWORK, 0);
		log_error("network error\n");
		return -1;
	}
	res = NetBufferGetInt(netbuf);
	if (res != 0) {
		x100_session_set_server_error(state->session);
		log_debug("runtime query execution error: %d", state->session->error->code);
		return -1;
	}
	nrtuples = NetBufferGetInt(netbuf);
	if (nrtuples == 0) {
		log_debug("no more data\n");
		return 0;
	}
	free_nonfixed_data(state);
	for (i = 0; queryres[i].name; i++) {
		if (queryres[i].isfixed) {
			int n = queryres[i].isconst ? 1 : nrtuples;
			void *dst = queryres[i].data;
			NetBufferGet(netbuf, dst, n*queryres[i].width);
		} else {
			int j, n = queryres[i].isconst ? 1 : nrtuples;
			void **ptrs = (void**)queryres[i].data;
			for (j = 0; j < n; j++) {
				int len = NetBufferGetLen(netbuf);
				ptrs[j] = x100_malloc(len);
				NetBufferGetVar(netbuf, ptrs[j]);
			}
		}
	}
	return nrtuples;
}


/* Initialize an X100 "session":  set up the X100 session state block and
** return its address.  The X100 session state block is usually allocated
** by the sequencer (as one of the session's facility CB's), but if this
** is a JSP session there's no sequencer and we must allocate one.
**
** Please note that there is as yet no x100_server connection.  At
** present, the x100_server connection waits for first use, although that
** may change (to allow better transaction management).
*/
int
x100_session_init(struct X100SessionState **pstate, CS_SID csid, char *dbname, ADF_CB *adf) {
	char trimmed_dbname[DB_DB_MAXNAME+1];
	struct X100SessionState *state;
	int res;
	int i;

	if (pstate == NULL) return -1;

	if (csid == DB_NOSESSION)
	{
	    /* this is a JSP session, so we haven't gone through the
	    ** sequencer, so there is no X100SessionState structure.
	    ** Allocate one here.
	    */
	    state = x100_calloc(sizeof(struct X100SessionState), 1);
	    if (!state) {
		    log_error("failed to allocate space for internal buffer\n");
		    return -1;
	    }
	}
	else
	{
	    /* this is an SCF session */
	    state = GET_X100_SCB(csid);
	}

	*pstate = state;
	state->sessid = csid;
	state->adfcb = adf;
	state->conn = NULL;
	state->error = x100_malloc(sizeof(struct X100Error));
	if (state->error == NULL)
		return -1;
	memset(state->error, 0, sizeof(struct X100Error));
	state->connection_state = CONNECTION_STATE_NONE;
	state->interrupted = FALSE;
	strncpy(trimmed_dbname, dbname, DB_DB_MAXNAME);
	trimmed_dbname[DB_DB_MAXNAME] = 0;
	for (i = strlen(trimmed_dbname) - 1; i >= 0 && trimmed_dbname[i] == ' '; i--)
		trimmed_dbname[i] = 0;
	strcpy(state->dbname, trimmed_dbname);
	return 0;
}

GCA_COL_ATT *
x100_gca_attr_next(GCA_COL_ATT *rescol, GCA_COL_ATT *dst) {
	memcpy(dst, rescol, sizeof(GCA_COL_ATT));
	if (dst->gca_l_attname) {
		rescol = (GCA_COL_ATT *)&rescol->gca_attname[dst->gca_l_attname];
	} else rescol++;
	return rescol;
}

static int
x100_query_init_input(struct X100QueryState *state, GCA_TUP_DESCR *informat)
{
	int i, j, k, row_size;
	struct column *queryres = state->queryres;
	GCA_COL_ATT rescolmem, *rescol = &rescolmem, *irescol;
	irescol = informat->gca_col_desc;
	row_size = 0;
	for (j = 0; j < informat->gca_l_col_desc; j++) {
		irescol = x100_gca_attr_next(irescol, rescol);
		row_size += rescol->gca_attdbv.db_length;
	}
	state->row_size = row_size;
	state->attr_count = informat->gca_l_col_desc;
	state->columnmap = x100_calloc(informat->gca_l_col_desc * 2, sizeof(*(state->columnmap)));
	state->conv = table_conv_init(informat->gca_l_col_desc * 2,row_size);
	state->conv_col = x100_calloc(informat->gca_l_col_desc * 2, sizeof(struct table_conv_column));

	irescol = informat->gca_col_desc;
	for (k = j = 0; j < informat->gca_l_col_desc; j++) {
		int type, width, mapped_j = -1, null_j = -1;
		irescol = x100_gca_attr_next(irescol, rescol);
		type = rescol->gca_attdbv.db_datatype;
		width = rescol->gca_attdbv.db_length - (type < 0);
		if (queryres[k].name && !queryres[k].name[0] && !strcmp("bool",queryres[k].type)) {
    			log_debug("match for null marker of column %d(%s) = %d, type %s",j, queryres[k].name, k, queryres[k].type);
			null_j = k++;
		}
		if (queryres[k].name) {
			log_debug("match for column %d(%s) = %d, type %d/%s (ingres/x100) width %d/%d\n",j, queryres[k].name, k, rescol->gca_attdbv.db_datatype, queryres[k].type,  rescol->gca_attdbv.db_length, queryres[k].width);
			mapped_j = k++;
		}
		if (mapped_j >= 0) {
			if (table_conv_add_typed_column(state->conv, &rescol->gca_attdbv, queryres[mapped_j].type, queryres[mapped_j].width, queryres[mapped_j].isfloat, state->session->adfcb) < 0) {
				x100_query_set_error(state, X100_ERR_NO_TYPE_CONVERSION, 0);
				return -1;
			}
			((struct table_conv_column*)state->conv_col)[j*2].column_const = queryres[mapped_j].isconst;
		} else {
			log_warn("no match for column %d = type %d\n",j, rescol->gca_attdbv.db_datatype);
			table_conv_add_column(state->conv, 0, width, 0, &convert_clear, NULL);
		}
		//Null marker column (always present, width 0 in NON NULL columns)
		table_conv_add_null_column(state->conv, null_j >= 0, type < 0);
		if (null_j >= 0) {
			((struct table_conv_column*)state->conv_col)[j*2+1].column_const = queryres[null_j].isconst;
		}
		state->columnmap[j * 2] = mapped_j;
		state->columnmap[j * 2 + 1] = null_j;
	}
	state->tuplepos    = state->tuplesbuf = 0; //buffer is empty initially
	return 0;
}

static int
x100_query_init_output(struct X100QueryState *state, GCA_TUP_DESCR *outformat)
{
	int i, j, k;
	struct column *queryres = state->queryres;
	struct table_converter *conv = 0;
	GCA_COL_ATT rescolmem, *rescol = &rescolmem, *irescol;

	state->queryres = x100_calloc(sizeof(struct column), outformat->gca_l_col_desc * 2 + 1);
	state->conv_col = x100_calloc(sizeof(void*), outformat->gca_l_col_desc * 2);
	state->conv = conv = table_conv_init(outformat->gca_l_col_desc * 2, 0);

	irescol = outformat->gca_col_desc;
	for (k = j = 0; j < outformat->gca_l_col_desc; j++) {
		char x100typename[40];
		int x100width, fixed;
		irescol = x100_gca_attr_next(irescol, rescol);
		x100width = table_conv_get_x100_type(rescol->gca_attdbv.db_datatype, rescol->gca_attdbv.db_length, rescol->gca_attdbv.db_prec, x100typename);
		fixed = x100width >= 0;
		if (x100width) {
			state->queryres[k].isfixed = fixed;
			state->queryres[k].width = fixed ? x100width : sizeof(void*);
			state->queryres[k].name = strdup("unnamed");
			vw_assert(state->queryres[k].name);
			state->queryres[k].data = x100_calloc(state->queryres[k].width, state->nrtuples);
			if (!fixed) {
				for (i = 0; i < state->nrtuples; i++)
					((void**)state->queryres[k].data)[i] = x100_calloc(-x100width + 1, 1);
			}
			if (table_conv_add_typed_column(conv, &rescol->gca_attdbv, x100typename, x100width > 0 ? x100width : sizeof(void*), 0, state->session->adfcb) < 0) {
				x100_query_set_error(state, X100_ERR_NO_TYPE_CONVERSION, 0);
				return -1;
			}
			k++;
			if (rescol->gca_attdbv.db_datatype < 0) {
				state->queryres[k].isfixed = 1;
				state->queryres[k].width = 1;
				state->queryres[k].name = strdup("unnamednull");
				vw_assert(state->queryres[k].name);
				state->queryres[k].data = x100_calloc(1, state->nrtuples);
				table_conv_add_null_column(conv, 1, 1);
				k++;
			}
		}
	}
	state->conv_row = x100_calloc(sizeof(void*), state->nrtuples);
	state->tuplepos = state->tuplesbuf = 0; //buffer is empty initially
	return 0;
}

static void
log_query(CS_SID sessid, char *query)
{
        void *f;
	if (!(ult_always_trace() & SC930_TRACE)) return;
	f = ult_open_tracefile((void *)sessid);
	if (!f) return;
	ult_print_tracefile(f,SC930_LTYPE_VECTORWISE_QRY,query);
	ult_close_tracefile(f);
}


/* Given a SessionState, connect to an x100 server if we don't already have
** one.  Set error status if needed;  if connected, but in ERROR state,
** also set the error status.
*/
void
x100_connect_if_needed(struct X100SessionState *session)
{
	X100ErrorCode conn_error = X100_OK;

	if (session->conn == NULL) {
		log_debug("connecting to <%s>", session->dbname);
		session->conn = x100_connect(session->dbname, (i4 *) &conn_error);
		session->connection_state = session->conn ? CONNECTION_STATE_READY : CONNECTION_STATE_ERROR;
	}
	if (session->conn == NULL) {
		log_error("failed to create x100 connection");
		x100_session_set_error(session, conn_error, 0);
	} else if (session->connection_state == CONNECTION_STATE_ERROR) {
		x100_session_set_error(session, X100_ERR_NETWORK, 0);
	}
}


static int
x100_session_query_init(struct X100QueryState **pstate,
	struct X100SessionState *sstate, char *querytext,
	GCA_TUP_DESCR *informat, GCA_TUP_DESCR *outformat) {

	NetBuffer *netbuf;
	struct X100QueryState *state = 0;
	int res;
	struct table_conv_column *conv_col;

	if (!(state = x100_calloc(sizeof(struct X100QueryState), 1))) {
		log_error("failed to allocate space for internal buffer\n");
		return -1;
	}
	*pstate = state;
	log_debug("got query: %s", querytext);
	if (sstate->sessid) log_query(sstate->sessid, querytext);
	if (!querytext) return -1;
	// sending and receiving tuples at the same time not supported
	// because of some ugly state variables reuse
	if (informat && outformat) return -1;

	x100_connect_if_needed(sstate);
	if (!sstate->conn)
		return -1;

	netbuf = sstate->conn->netbuf;
	state->session = sstate;
	NetBufferClear(netbuf);
	NetBufferPutInt(netbuf, 1);
	NetBufferPutStr(netbuf, querytext);
	NetBufferPutStr(netbuf, sstate->conn->server->authpass);
	if (x100_check_sendok(sstate) < 0)
		return -1;
	if (NetBufferSend(netbuf) < 0) {
		log_error("network error while sending query");
		x100_query_set_error(state, X100_ERR_NETWORK, 0);
		return -1;
	}
	sstate->connection_state = CONNECTION_STATE_ACK;

	/* Read the ack saying that the query was parsed OK (or not) */
	res = x100_timed_recv(sstate, netbuf);
	if (res == -2)
		return -1;	/* Error all set up */
	if (res <= 0) {
		log_error("network error while reading ack");
		x100_query_set_error(state, X100_ERR_NETWORK, 0);
		return -1;
	}
	res = NetBufferGetInt(netbuf);
 	if (res) {
		x100_session_set_server_error(state->session);
		log_error("query not accepted by the x100 server: %d\n", state->session->error->code);
		return -1;
	} else {
		state->nrcolumns = NetBufferGetInt(netbuf);
		state->nrtuples = NetBufferGetInt(netbuf);
	}
	log_debug("tuples per buf = %d", state->nrtuples);
	sstate->connection_state = CONNECTION_STATE_RESULT;

	if (informat) {
		x100_query_parse_format(state);
		if (x100_query_init_input(state, informat) < 0) {
			return -1;
		}
	}
	if (outformat) {
		if (x100_query_init_output(state, outformat) < 0) {
			return -1;
		}
	}
	if (!informat && !outformat) {
		if (x100_skip_output(sstate, TRUE) < 0) return -1;
	}
	return 0;
}

int
x100_query_init(struct X100QueryState **pstate, char *querytext,
	GCA_TUP_DESCR *informat, GCA_TUP_DESCR *outformat,
	VW_CORE_PARMS *x100cb)
{
	if (!x100cb->session) {
		log_warn("out of session query: %s", querytext);
		return -1;
	}
	return x100_session_query_init(pstate, x100cb->session, querytext, informat, outformat);
}


int
x100_pop_data(struct X100QueryState *state, int *prowbuf_count, void *rowbuf_addr)
{
	struct table_conv_column *conv_col = state->conv_col;
	struct column *queryres = state->queryres;
	void *bufptr;
	int i, rowbuf_count = *prowbuf_count;	

	log_debug("max accepted tuples = %d\n",rowbuf_count);
	for (i = 0; i < rowbuf_count; ) {
		int j, todo;
		X100ErrorCode err;
		if (state->tuplepos == state->tuplesbuf) {
			state->tuplesbuf = query_next(state);
			log_debug("read %d new tuples\n",state->tuplesbuf);
			if (state->tuplesbuf < 0) return -1;
			if (state->tuplesbuf == 0) {
				state->session->connection_state = CONNECTION_STATE_READY;
				log_debug("finished sending query result\n");
				*prowbuf_count = i;
				return 0;
			}
			state->tuplepos = 0;
		}
		bufptr = (char*)rowbuf_addr + state->row_size * i;
		todo = state->tuplesbuf - state->tuplepos;
		if (todo + i > rowbuf_count) todo = rowbuf_count - i;

		log_debug("processing %d tuples at offset %d\n",todo,i);

		for (j = 0; j < state->attr_count * 2; j++) {
			int mapped_j = state->columnmap[j];
			int pos = mapped_j < 0 || queryres[mapped_j].isconst ? 0 : state->tuplepos;
			void *src = mapped_j < 0 ? 0 : queryres[mapped_j].data + queryres[mapped_j].width * pos;
			conv_col[j].column_data = src;
		}

		if ((err = table_conv_export(state->conv, conv_col, bufptr, todo)) != X100_OK) {
			log_error("conversion error in the received tuples");
			x100_query_set_error(state, err, 0);
			*prowbuf_count = i;
			return -1;
		}

		state->tuplepos+=todo;
		i+=todo;
	}

	log_debug("finished but more rows to send\n");
	*prowbuf_count = i;
	return 1;
}

static int
x100_bulkload_send(struct X100QueryState *state, struct column *src, int nrtuples)
{
	int i, j;
	NetBuffer *netbuf = state->session->conn->netbuf;
	log_trace("sending to server %d tuples", nrtuples);
	NetBufferClear(netbuf);
	NetBufferPutInt(netbuf, 2);
	NetBufferPutInt(netbuf, nrtuples);
	if (nrtuples > 0) for (i = 0; src[i].name; i++) {
		if (!src[i].isfixed) {
			for (j = 0; j < nrtuples; j++) {
				void **obj = (void**)src[i].data;
				int objlen;
				obj += j;
				objlen = strlen(*obj);
				NetBufferPutVar(netbuf, *obj, objlen);
			}
		} else {
			NetBufferPut(netbuf, src[i].data, nrtuples * src[i].width);
		}
	}
	if (x100_check_sendok(state->session))
		return -1;
	if (NetBufferSend(netbuf) < 0) {
		x100_query_set_error(state, X100_ERR_NETWORK, 0);
		log_error("sending data error");
		return -1;
	}
	return 0;
}

static X100ErrorCode
x100_convert_rows(struct X100QueryState *state, int startpos, int nrtuples)
{
	X100ErrorCode err;
	int i;
	if (!nrtuples) return X100_OK;
	for (i = 0; state->queryres[i].name; i++) {
		((void**)state->conv_col)[i] = (char *)state->queryres[i].data + startpos * state->queryres[i].width;
	}
	err = table_conv_import(state->conv, state->conv_row, state->conv_col, nrtuples);
	if (err) log_error("tuple data conversion error");
	return err;
}

int
x100_push_data(struct X100QueryState *state, int row_count, DM_MDATA* list)
{
	void *data;
	void **rows = state->conv_row;
	int startpos = state->tuplepos;
	int convpos = 0;
	int i;
	X100ErrorCode err;
	if (!row_count) {
		if (state->tuplepos)
			if (x100_bulkload_send(state, state->queryres, state->tuplepos) < 0) return -1;
		if (x100_bulkload_send(state, state->queryres, 0) < 0) return -1;
		if (x100_skip_output(state->session, TRUE) < 0) return -1;
		return 0;
	}
	for (i = 0; i < row_count && list && list->data_address; list = list->data_next, i++) {
		rows[convpos++] = list->data_address;
		state->tuplepos++;
		if(state->tuplepos == state->nrtuples) {
			int res;
			err = x100_convert_rows(state, startpos, convpos);
			if (err) {
				state->tuplepos = 0;
				return -1;
			}
			res = x100_bulkload_send(state, state->queryres, state->tuplepos);
			state->tuplepos = startpos = convpos = 0;
			if (res < 0) return -1;
		}
	}
	err = x100_convert_rows(state, startpos, convpos);
	if (err) {
		x100_query_set_error(state, err, 0);
		state->tuplepos = 0;
		return -1;
	}
	return row_count;
}

CS_SID
x100_session_get_sessid(struct X100SessionState *state)
{
	return state->sessid;
}

ADF_CB *
x100_session_get_adfcb(struct X100SessionState *state)
{
	return state->adfcb;
}

char *
x100_session_get_dbname(struct X100SessionState *state)
{
	return state->dbname;
}

#endif /* conf_IVW */
