#ifndef X100_INCLUDED
#define X100_INCLUDED

#include <gca.h>
#include <dmucb.h>
#include <adf.h>
#include <ervwf.h>
#include <x100ll.h>

/*
** File: x100.h -- Definitions for QEF/X100 interface
**
** Description:
**	FIXME need some blah blah blah here!
**
** Edit History:
**	2009-2011 (dougi)
**	    initial versions
**	24-May-2011 (kschendel) m1260
**	    Fairly serious reorganization of the Ingres side of the X100
**	    interface, partly for query kill/cancel, partly to expose
**	    server info to other servers such as the JSP or multiple
**	    DBMS servers.
**	21-June-2011 (stial01) m2015
**	    Added dbxlate to X100_SYSCALL_PARAM
**	28-Jul-2011 (kschendel) SIR 125587
**	    Define things needed for startup of VWLOAD style pseudo-copy.
*/


typedef struct _VW_CORE_PARMS {
    ADF_CB	*adfcb;
    char        *dbname;
    struct X100SessionState *session;
} VW_CORE_PARMS;

#define vw_translate_err(_cb,_X100ErrorBlk) \
	vwTranslateErr(_cb,_X100ErrorBlk)
#ifdef HAS_VARIADIC_MACROS
#define vw_error(_cb,_errcode,...) \
	vwErrorFcn(_cb,_errcode,__FILE__,__LINE__,__VA_ARGS__)

FUNC_EXTERN DB_STATUS vwErrorFcn(
	struct	X100SessionState *session,
	i4	vw_errorCode,
	PTR	filename,
	i4	linenumber,
	i4	pcnt,
	...);
#else

/* Variadic macros not supported */
#define vw_error vw_errorNV

FUNC_EXTERN DB_STATUS vw_errorNV(
	struct X100SessionState	*session,
	i4	x100_errorcode,
	i4	pcnt,
		...);
#endif /* HAS_VARIADIC_MACROS */

FUNC_EXTERN DB_STATUS vwTranslateErr(
	struct X100SessionState	*session,
	void *x100err);

/*
** Name: QEA_X100	- X100 parameter list structure
**
** Description:	This structure contains the parameters that are passed
**	to X100 to execute an X100 query. It defines both input and
**	results of X100 calls.
**
**	The structure is located via the ahd_x100_cb index of the
**	QEF_X100_QRY query plan action.  The action itself is part of
**	the read-only query plan, of course, and can't itself be modified
**	at runtime.
**
** History:
**	27-Aug-2008 (dougi)
**	    Written for preliminary X100 support.
**      19-Nov-2008 (gefei01)
**          Modified X100_BULKLOAD. 
**      25-Jan-2009 (gefei01)
**          X100_BULKLOAD: add col_name for column names.
**      19-Feb-2009 (gefei01)
**          X100_INSERT_PARAM: Make data_address a type of void *.
**	7--may-2009 (dougi)
**	    Add "x100_sessid" to all blocks to allow traceing.
**	8-dec-2009 (dougi)
**	    Add "commit" to pass commit requirements from QEF to X100.
**	4-feb-2010 (michal)
**	    Add "adfcb" requiered for Unicode conversion
**	10-feb-2010 (dougi)
**	    Add "comp_array" for compound column identification.
**	10-Mar-2010 (kiria01)
**	    Pull together parameter blocks for error handling.
**	6-may-2010 (dougi)
**	    Added "close" flag to indicate X100 is done with query.
**	15-sep-2010 (dougi)
**	    Added "pax" flag to X100_CREATE_TABLE_PARAM structure.
**	22-Sep-2010 (kschendel)
**	    Drop "commit" flag, let explicit qet calls do it all now.
**	4-Nov-2010 (kschendel)
**	    Spiff up transaction interface to include savepoints.
**	    Add standard "func-extern" to global declarations.
**      03-feb-2011 (stial01)
**          Change x100_resrow_count i4->i8 (for tuple counts)
**	17-Mar-2011 (kschendel) m1541
**	    Add text-remaining counter.
**	06-Jun-2011 (drivi01) 
**	    Redefine x100_global_ctx as global variable instead of
**	    static and export it in iilibdmfdata on Windows.
**	    Reference it in this file.
*/

typedef struct _QEA_X100 {
    VW_CORE_PARMS core;
    struct _QEE_DSH *dsh;	/* Needed during parameter substitution */
    char	*x100_text;	/* pointer to X100 query text */
    i4		x100_textlen;	/* length of X100 query text */
    i4		x100_textleft;	/* For parameter substitution */
    GCA_TUP_DESCR 	*x100_resrow_format; /* address of SQLDA for query */
    i4		x100_resrow_size; /* size (in bytes) of result row */
    i4		x100_rowbuf_count; /* count of available result row buffers */
    char	*x100_rowbuf_addr; /* addr of result row buffer array */
    i8		*x100_resrow_countp; /* ptr to count of result rows returned
				** (filled in by X100) */
    char	*x100_query_handle; /* ptr to query handle 
				** (filled in by X100) */
    i4		*x100_errorp;	/* ptr to return error code
				** (filled in by X100) */
    i4		x100_flags;	/* flags */
#define	QEA_X100_QUERYDONE	0x01
    i8		x100_resrow_count; /* count of update result rows */
}	QEA_X100;

typedef struct _X100_CREATE_TABLE_PARAM {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
    DMF_ATTR_ENTRY **attr_entry; /* address of SQLDA for the table */
    i4          attr_count;
    i1          enable_compression; /* 1 - compression enabled, 0 - disabled */
    i1          enable_minmax; /* 1 - minmax enabled, 0 - disabled */
    i1		*comp_array;	/* ptr to compound column array */
    i1		temporary_table; /* 0 - regular table, 1 - temporary table */
    i1		pax;		/* 0 - regular table, 1 - PAX */
} X100_CREATE_TABLE_PARAM;

typedef struct _X100_DROP_TABLE_PARAM {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
} X100_DROP_TABLE_PARAM;

typedef struct _X100_ADD_COLUMN_PARAM {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
    DMF_ATTR_ENTRY *attr_entry; /* address of SQLDA for the column */
    char	*column_name;
    char	*default_value; /* default value as x100 constant expression */
    i1          enable_compression; /* 1 - compression enabled, 0 - disabled */
} X100_ADD_COLUMN_PARAM;

typedef struct _X100_DROP_COLUMN_PARAM {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
    char	*column_name;
} X100_DROP_COLUMN_PARAM;

typedef struct _X100_BULKLOAD {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
    DB_ATT_NAME    *col_name;      /* pointer to column names */
    GCA_TUP_DESCR  *row_format;    /* address of SQLDA for the table */
    /* push phase */
    i8		    row_count;     /* count of rows */
    i8              cur_tups;      /* number of tuples currently being loaded */
    i8		    orig_tups;	   /* number of tuples before copy */
    DM_MDATA	   *row_addr;      /* addr of row array */
    /* state */
    char	   *x100_query_handle; /* ptr to query handle 
				        ** (filled in by X100) */
} X100_BULKLOAD;

typedef struct _X100_INSERT_PARAM {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
    GCA_TUP_DESCR  *row_format;    /* address of SQLDA for the table (no column names) */
    void  	   *data_address;  /* addr of row data */
    /* state */
    char	   *x100_query_handle; /* ptr to query handle 
				        ** (filled in by X100) */
} X100_INSERT_PARAM;



typedef struct _X100_COPYINTO_PARAM {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
    GCA_TUP_DESCR	*row_format; /* address of SQLDA for the table */
    void		*data_address;  /* addr of row data */
    DB_ATT_NAME    *col_name;      /* pointer to column names */
    i8		row_count; /* count of rows allocated in data_address */
    i8		cur_tups; /* number of tuples filled in by pull function */
    /* state */
    char		*x100_query_handle;
} X100_COPYINTO_PARAM;

typedef struct _X100_VWLOAD_PARAM {
    VW_CORE_PARMS core;
    char *tablename;	/* Null terminated table name */
    char *token;	/* Where to put returned secret word */
    i4 port;		/* Returned x100-server connection port */
} X100_VWLOAD_PARAM;

/* Max length of x100 connect token:  "port secret" null terminated.
** Port numbers are usually 5 digits max but allow extra just because.
*/
#define X100_VWLOAD_MAX_SIZE (AUTHPASS_MAX_SIZE + 1 + 10 + 1)

typedef enum {
    X100_KEY_PRIMARY = 0,
    X100_KEY_UNIQUE,
    X100_KEY_FOREIGN
} X100_KEY_TYPE;


typedef char** X100_COLLIST ;

typedef struct _X100_CREATE_KEY {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
    char           *key_name;
    X100_KEY_TYPE   key_type;
    i4		    key_count;
    X100_COLLIST    columns;
    char           *ref_table;   /* used only for key type X100_KEY_FOREIGN */
    char           *ref_key_name; /* used only for key type X100_KEY_FOREIGN */
} X100_CREATE_KEY;

typedef struct _X100_DROP_KEY {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
    char           *key_name;       /* */
} X100_DROP_KEY;

typedef  void(*x100_join_index_cb)(void*param, char *tablename, char *indexcolumnname, char *fkname);
typedef  void(*x100_sort_order_cb)(void*param, char *tablename, char *sortordername, X100_COLLIST order_columns, int order_count);

typedef struct _X100_CLUSTER_INDEX {
    VW_CORE_PARMS core;
    char	*tablename;	/* pointer to zero terminated table name */
    char           *index_name;       /* */
    X100_COLLIST    order_columns;
    int             order_count;
    x100_join_index_cb create_join_index_callback; /* called every time a join index is created if set */
    x100_sort_order_cb create_sort_order_callback; /* called every time an order is applied to a table if set */
    void           *callback_param;
    int		*enable_joinindex;
} X100_CLUSTER_INDEX;


typedef struct _X100_DATABASE_PARAM {
    VW_CORE_PARMS core;
    /* don't need anything else */
} X100_DATABASE_PARAM;


typedef struct _X100_SYSCALL_PARAM {
    VW_CORE_PARMS core;
    DB_TEXT_STRING	*callname;
    DB_TEXT_STRING	*callparam;
    union {
	DB_TEXT_STRING	text;
	char	storage[1024];
    } result;
    void (*update_count)(struct _X100_SYSCALL_PARAM *x100sc, char table[DB_TAB_MAXNAME], char table_owner[DB_OWN_MAXNAME], i8 new_cnt);
    int (*table_access)(struct _X100_SYSCALL_PARAM *x100sc, char table[DB_TAB_MAXNAME], char table_owner[DB_OWN_MAXNAME]);
    PTR		cb_qef_rcb;	/* ptr to session QEF_RCB for callback */
    u_i4	dbxlate;	/* for case translation of combine params */
} X100_SYSCALL_PARAM;

typedef struct _X100_SESSION_PARAM {
    VW_CORE_PARMS core;
    CS_SID	ing_sessid;
} X100_SESSION_PARAM;

/*
** Name: struct X100SessionState -- X100 session state CB
**
** Description:
**	This is the X100 per-session state control block.  It is allocated
**	as part of the SCF SCB, just like the other major facility CB's
**	(QEF_CB, DML_SCB, etc).  It's done this way so that the X100
**	interface can participate in the standard CSattn / scf attn
**	interrupt and kill processing.  Please note that the existence
**	of X100 session state does NOT imply an actual connection to
**	an X100-server;  the latter is created on demand, while the
**	X100 session state is always there.
**
**	There's no typedef name for this CB, mostly because I'm too lazy
**	to fix all of the numerous structure refs in x100client.c.
**	Feel free to fix this yourself.
**
** Edit History:
**	24-May-2011 (kschendel) m1260
**	    Move here from x100client.
*/

struct X100SessionState
{
    CS_SID	sessid;		/* Ingres session ID */
    ADF_CB	*adfcb;		/* Calling facility ADF CB, probably QEF's */
    char	dbname[DB_DB_MAXNAME+1];  /* Blank-trimmed, null terminated */
    struct X100Connection *conn;  /* The actual connection info if connected */
    struct X100Error *error;	/* Error parameters from x100.  This struct
				** will always be allocated but only x100client
				** cares about it.
				*/
    i4		connection_state;
#define CONNECTION_STATE_NONE    0
#define CONNECTION_STATE_READY   1
#define CONNECTION_STATE_ACK	 2
#define CONNECTION_STATE_RESULT  3
#define CONNECTION_STATE_ERROR   4

    bool	interrupted;	/* TRUE if attn received.  Note that in general
				** this is not really like a signal, since in
				** most cases it can only be set in-thread by
				** doing a poll of the front-end.  In the rare
				** cases that it's set asynchronously by some
				** other thread, such as for a kill-session
				** from iimonitor, it's just to make sure that
				** we see the attn and break out of any wait on
				** the x100 server.
				*/
};

/* We can get a session's X100SessionState from its CS_SID session ID
** directly, since the SID is really just the SCF SCB.
*/
GLOBALREF i4 x100_x100cb_offset;	/* is constant for any given server */
GLOBALREF X100Context *x100_global_ctx;
#define GET_X100_SCB(sid) \
        ((CS_SID)sid == CS_ADDER_ID ? (struct X100SessionState *)NULL : \
	*(struct X100SessionState**)((char *)sid + x100_x100cb_offset))


FUNC_EXTERN DB_STATUS x100_create_database(X100_DATABASE_PARAM *);
FUNC_EXTERN DB_STATUS x100_destroy_database(X100_DATABASE_PARAM *);

FUNC_EXTERN DB_STATUS x100_query(QEA_X100 *x100cb);
FUNC_EXTERN DB_STATUS x100_query_stop(QEA_X100 *x100cb);

FUNC_EXTERN DB_STATUS x100_create_table(X100_CREATE_TABLE_PARAM *);
FUNC_EXTERN DB_STATUS x100_drop_table(X100_DROP_TABLE_PARAM *);
FUNC_EXTERN DB_STATUS x100_add_column(X100_ADD_COLUMN_PARAM *);
FUNC_EXTERN DB_STATUS x100_drop_column(X100_DROP_COLUMN_PARAM *);
FUNC_EXTERN DB_STATUS x100_create_key(X100_CREATE_KEY *); /* create key constraint */
FUNC_EXTERN DB_STATUS x100_drop_key(X100_DROP_KEY *); /* destroys key constraint or index */
FUNC_EXTERN DB_STATUS x100_cluster_index(X100_CLUSTER_INDEX *); /* creates cluster tree strarting at given table */

FUNC_EXTERN DB_STATUS x100_session_begin(X100_SESSION_PARAM *);
FUNC_EXTERN DB_STATUS x100_session_end(X100_SESSION_PARAM *);

FUNC_EXTERN DB_STATUS x100_start_txn(struct X100SessionState *state);
FUNC_EXTERN DB_STATUS x100_savepoint(struct X100SessionState *state, DB_SP_NAME *savepoint);
FUNC_EXTERN DB_STATUS x100_commit(struct X100SessionState *state);
FUNC_EXTERN DB_STATUS x100_rollback(struct X100SessionState *state, DB_SP_NAME *savepoint);

FUNC_EXTERN void x100_no_autocommit(void);

/* A couple things defined in x100client.c that are needed elsewhere */
FUNC_EXTERN void x100_session_clear_error(struct X100SessionState *state);
FUNC_EXTERN ADF_CB *x100_session_get_adfcb(struct X100SessionState *state);

#define X100_TIDNAME "__tid__"

/* escaped SQL name may be 3 times plus 1 char longer,
   plus two quotes and the final null */
#define X100_MAXNAME (DB_MAXNAME * 3 + 4)

FUNC_EXTERN void x100_print_id(char *name, int namelen, char *output);
FUNC_EXTERN void x100_print_quoted_id(char *name, int namelen, char *output);

/*
** The following three functions should be used following the 
** standard iterator interface:
** - a single call to x100_bulkload_init(), which will start the 
**   process and create X100-specific state
** - a series of calls to x100_bulkload_push(), that iteratively
**   feed more data into X100
** - a single call to x100_bulkload_finish(), that will commit the save
**   and free the resources.
*/

FUNC_EXTERN DB_STATUS x100_bulkload_init(X100_BULKLOAD *);
FUNC_EXTERN DB_STATUS x100_bulkload_push(X100_BULKLOAD *);
FUNC_EXTERN DB_STATUS x100_bulkload_finish(X100_BULKLOAD *);
FUNC_EXTERN DB_STATUS x100_insert(X100_INSERT_PARAM *);

FUNC_EXTERN DB_STATUS x100_copyinto_init(X100_COPYINTO_PARAM *);
FUNC_EXTERN DB_STATUS x100_copyinto_pull(X100_COPYINTO_PARAM *);
FUNC_EXTERN void x100_copyinto_stop(X100_COPYINTO_PARAM *);

FUNC_EXTERN DB_STATUS x100_vwload_init(X100_VWLOAD_PARAM *);

FUNC_EXTERN DB_STATUS x100_syscall(X100_SYSCALL_PARAM *);
FUNC_EXTERN DB_STATUS x100_supported_dt(i4 dt);

#endif
