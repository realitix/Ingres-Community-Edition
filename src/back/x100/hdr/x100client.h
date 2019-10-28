#if defined(conf_IVW) && !defined(X100CLIENT_INCLUDED)
#define X100CLIENT_INCLUDED
#include <x100/x100_error_defs.h>

/*
** Name: x100client.h -- Local definitions for x100 client interface.
**
** Description:
**	Put things here that are local to the x100 / x100client interface
**	and not needed outside of the x100 subdirectory.
**
** Edit History:
**	michal, created
**
**	3-Aug-2011 (kschendel) SIR 125587
**	    Add this edit history
*/

/* Prone to failure, but at least better than nothing at all */
#ifndef DIR_SEP
# ifdef WIN32
#  define DIR_SEP '\\'
# else
#  define DIR_SEP '/'
# endif
#endif

#define MAX_X100_ERROR_PARAMS 3
struct X100Error {
	X100ErrorCode code;
	int param_count;
	char *params[MAX_X100_ERROR_PARAMS];
};
struct X100QueryState;
struct X100SessionState;
struct X100Error *x100_get_error(struct X100QueryState *state);

GCA_COL_ATT *x100_gca_attr_next(GCA_COL_ATT *rescol, GCA_COL_ATT *dst);

int x100_session_init(struct X100SessionState **state, CS_SID csid, char *dbname, ADF_CB *adf);
void x100_session_free(struct X100SessionState *state);
struct X100Error *x100_session_get_error(struct X100SessionState *state);
CS_SID x100_session_get_sessid(struct X100SessionState *state);
char *x100_session_get_dbname(struct X100SessionState *state);
int x100_session_start_txn(struct X100SessionState *state);
int x100_session_commit(struct X100SessionState *state);
int x100_session_rollback(struct X100SessionState *state, char *sp_name, int len);
int x100_session_savepoint(struct X100SessionState *state, char *sp_name, int len);

int x100_query_init(struct X100QueryState **state, char *querytext, GCA_TUP_DESCR *informat, GCA_TUP_DESCR *outformat, VW_CORE_PARMS *x100cb);
int x100_pop_data(struct X100QueryState *state, int *rowbuf_count, void *rowbuf_addr);
int x100_push_data(struct X100QueryState *state, int row_count, DM_MDATA* list);
int x100_query_commit(struct X100QueryState *state);
void x100_query_free(struct X100QueryState *state);
void x100_connect_if_needed(struct X100SessionState *session);
#endif

