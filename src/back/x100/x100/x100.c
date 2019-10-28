/*
 * Copyright (C) 1997-2009 VectorWise.
 * All Rights Reserved.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <compat.h>
#include <iicommon.h>
#include <cs.h>
#include <dbdbms.h>
#include <adf.h>
#include <gca.h>
#include <dmf.h>
#include <qefmain.h>
#include <cui.h>
#include <x100.h>

/* Most of this is under conf_IVW, a bunch of stubs are at the end */
#if defined(conf_IVW)

#include <x100/vwlog.h>

#include <x100client.h>
#include <iitableconv.h>

/*
** File: x100.c -- QEF/X100 interface
**
** Description:
**	This file contains the upper levels of the QEF/X100 interface,
**	the routines that QEF calls to do things.  We'll add any final
**	touches needed to the query and ask x100client to send it off
**	to the Vectorwise server.
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
**	    cancelling.  Make core params a substructure.
**	21-June-2011 (stial01) m2015
**	    Added x100_idxlate()
**	23-Jun-2011 (stial0l)
**          x100_session_begin() JSP x100 session does not have SCF session.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**	29-Jul-2011 (kschendel) SIR 125587
**	    Add x100-vwload-init.
*/

#define VWLOG_FACILITY "X100CLIENT"

#define ishex(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'A' && (c) <= 'F'))
static int
x100_escape_id(char *name, int maxlen, char *output)
{
	int i = 0, j, len;
	static const char hex[] = "0123456789ABCDEF";
	for (len = 0; len < maxlen && name[len]; len++)
		;
	output[i++] = '_';
	while (len > 0 && name[len-1] == ' ') len--;
	for (j = 0; j < len; j++) {
		unsigned int c = (unsigned char)name[j];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9')) {
			output[i++] = c;
		} else if (c == '_' && (j >= len - 2 || !(ishex(name[j+1]) && ishex(name[j+2])))) {
			output[i++] = c;
		} else {
			output[i++] = '_';
			output[i++] = hex[c >> 4];
			output[i++] = hex[c & 15];
		}
	}
	return i;
}

void
x100_print_id(char *name, int namelen, char *output)
{
	int i = x100_escape_id(name, namelen, output);
	output[i] = 0; 
}

void
x100_print_quoted_id(char *name, int namelen, char *output)
{
	int i = 0;
	output[i++] = '\'';
	i += x100_escape_id(name, namelen, output + i);
	output[i++] = '\'';
	output[i] = 0;
}

struct X100QueryText {
	int pos, size;
	char *txt;
};
#define MAX_QUERY_SIZE 4096
static struct X100QueryText *
x100_querytext_init(void)
{
	struct X100QueryText *t = calloc(sizeof(*t),1);
	t->size = MAX_QUERY_SIZE;
	t->pos = 0;
	t->txt = malloc(MAX_QUERY_SIZE);
	return t;
}

static void
x100_querytext_printf(struct X100QueryText *t, char *fmt, ...)
{
	va_list ap;
	while (1) {
		int needed;
		va_start(ap, fmt);
		needed = vsnprintf(t->txt + t->pos, t->size - t->pos, fmt, ap);
		va_end(ap);
		if (needed < 0) needed = t->size;
		else if (t->pos + needed < t->size) {
			t->pos += needed;
			return;
		}
		while (t->pos + needed >= t->size)
			t->size *= 2;
		t->txt = realloc(t->txt, t->size);
	}
}

static void
x100_querytext_add_dbname(struct X100QueryText *t, char *pre, char *dbname, char *post)
{
	char buf[X100_MAXNAME];
	int i, len;
	x100_print_id(dbname, DB_MAXNAME, buf);
	x100_querytext_printf(t, "%s%s%s", pre, buf, post);
}

static void
x100_querytext_add_dbtextstring(struct X100QueryText *t, char *pre, DB_TEXT_STRING *string, char *post)
{
	x100_querytext_printf(t, "%s%.*s%s", pre, string->db_t_count, string->db_t_text, post);
}

static char *
x100_querytext(struct X100QueryText *t)
{
	return t->txt;
}

static void x100_querytext_free(struct X100QueryText *t)
{
	if (t) {
		if (t->txt) free(t->txt);
		free(t);
	}
}


static DB_STATUS
x100_simple_query(void *_x100cb, char *query);

/* Issue start of transaction */
DB_STATUS
x100_start_txn(struct X100SessionState *state)
{
	return (x100_session_start_txn(state));
}

DB_STATUS
x100_savepoint(struct X100SessionState *state, DB_SP_NAME *sp_name)
{
	i4 len;

	len = cui_trmwhite(sizeof(DB_SP_NAME), sp_name->db_sp_name);
	if (state && x100_session_savepoint(state, sp_name->db_sp_name, len) < 0)
		return vw_translate_err(state, x100_session_get_error(state));
	return E_DB_OK;
}

/* If no session state, just return, else commit or rollback */
DB_STATUS
x100_commit(struct X100SessionState *state)
{
	if (state && x100_session_commit(state) < 0)
		return vw_translate_err(state, x100_session_get_error(state));
	return E_DB_OK;
}

/* Null sp_name to rollback entire transaction. */
DB_STATUS
x100_rollback(struct X100SessionState *state, DB_SP_NAME *sp_name)
{
	i4 len;
	int status;

	if (state == NULL)
	    return (E_DB_OK);
	if (sp_name == NULL)
	    status = x100_session_rollback(state, NULL, 0);
	else
	{
	    len = cui_trmwhite(sizeof(DB_SP_NAME), sp_name->db_sp_name);
	    status = x100_session_rollback(state, sp_name->db_sp_name, len);
	}
	if (status < 0)
	    return vw_translate_err(state, x100_session_get_error(state));
	return E_DB_OK;
}

DB_STATUS
x100_query(QEA_X100 *x100cb)
{
	DB_STATUS status = E_DB_OK;
	struct X100QueryState *state = (void*)x100cb->x100_query_handle;
	int cnt = x100cb->x100_rowbuf_count, res;
	
	(*x100cb->x100_resrow_countp) = 0; /* no. rows returned */

	if (!state) {
		if (x100_query_init(&state, x100cb->x100_text, x100cb->x100_resrow_format, 0, &x100cb->core) < 0) {
			status = vw_translate_err(x100cb->core.session, x100_get_error(state));
			(*x100cb->x100_errorp) = x100cb->core.adfcb->adf_errcb.ad_usererr;
			if (state) x100_query_free(state);
			return status;
		}
		x100cb->x100_query_handle = (void*)state;
	}

	res = x100_pop_data(state, &cnt, x100cb->x100_rowbuf_addr);
	if (res < 0) {
		status = vw_translate_err(x100cb->core.session, x100_get_error(state));
		x100_query_free(state);
		x100cb->x100_query_handle = NULL;
		(*x100cb->x100_errorp) = x100_session_get_adfcb(x100cb->core.session)->adf_errcb.ad_usererr;
		return status;
	}
	(*x100cb->x100_resrow_countp) = cnt;
	if (res) return E_DB_OK;
	x100_query_free(state);
	x100cb->x100_query_handle = NULL;
	(*x100cb->x100_errorp) = E_QE0015_NO_MORE_ROWS;
	return E_DB_WARN;
}

DB_STATUS
x100_query_stop(QEA_X100 *x100cb)
{
	struct X100QueryState *state = (void*)x100cb->x100_query_handle;
	log_debug("query interrupted (state = %p)", state);
	x100_query_free(state);
	x100cb->x100_query_handle = NULL;
	return E_DB_OK;
}

static DB_STATUS
x100_bulkload_or_insert_init(X100_BULKLOAD *x100cb, int is_insert)
{
	struct X100QueryState *state = 0;
	struct X100QueryText *qt = x100_querytext_init();
	int j, first;
	GCA_COL_ATT rescolmem, *rescol = &rescolmem, *irescol;

	log_debug("bulkload init START to table %.*s, rows estimate %ld", DB_TAB_MAXNAME, x100cb->tablename, x100cb->row_count);
	x100_querytext_printf(qt, "NullOp (%s ('", is_insert ? "Insert" : "Append");
	x100_querytext_add_dbname(qt, "", x100cb->tablename, "', BinaryScan ([");

	irescol = x100cb->row_format->gca_col_desc;
	for (j = 0, first = 1; j < x100cb->row_format->gca_l_col_desc; j++) {
		char x100typename[40];
		int x100width;
		irescol = x100_gca_attr_next(irescol, rescol);
		x100width = table_conv_get_x100_type(rescol->gca_attdbv.db_datatype, rescol->gca_attdbv.db_length, rescol->gca_attdbv.db_prec, x100typename);
		if (x100width) {
			if (!first) x100_querytext_printf(qt, ", ");
			if (x100cb->col_name) {
				x100_querytext_add_dbname(qt, "", x100cb->col_name[j].db_att_name, "");
				x100_querytext_printf(qt, " = '%s'", x100typename);
				if (rescol->gca_attdbv.db_datatype < 0)
					x100_querytext_add_dbname(qt, ", ", x100cb->col_name[j].db_att_name, "_null = 'uchr'");
			} else {
				x100_querytext_printf(qt, "attr%d = '%s'", j, x100typename);
				if (rescol->gca_attdbv.db_datatype < 0)
					x100_querytext_printf(qt, ", attr%d_null = 'uchr'", j);
			}
			first = 0;
		} else {
			DB_STATUS status;
			char buf[20];
			sprintf(buf, "%d", rescol->gca_attdbv.db_datatype);
			status = vw_error(x100cb->core.session, E_VW1008_QRY_BAD_DT_ERROR,
					2, buf, x100_querytext(qt));
			x100_querytext_free(qt);
			log_warn("unknown type %d", rescol->gca_attdbv.db_datatype);
			return status;
		}
	}

	x100_querytext_printf(qt, "])))");

	if (x100_query_init(&state, x100_querytext(qt), 0, x100cb->row_format, &x100cb->core)<0) {
		DB_STATUS status;
		status = vw_translate_err(x100cb->core.session, x100_get_error(state));
		if (state) x100_query_free(state);
		x100_querytext_free(qt);
		return status;
	}
	x100_querytext_free(qt);

	x100cb->x100_query_handle = (void*)state;

	log_debug("bulkload init OK");
	return E_DB_OK;
}

DB_STATUS
x100_bulkload_init(X100_BULKLOAD *x100cb)
{
	return x100_bulkload_or_insert_init(x100cb, 0);
}

DB_STATUS
x100_bulkload_push(X100_BULKLOAD *x100cb)
{
	STATUS status;
	struct X100QueryState *state = (void*)x100cb->x100_query_handle;
	if (!state) {
		log_debug("x100_bulkload_push empty state");
		return vw_error(x100cb->core.session, E_VW1010_BKLD_STATE_ERROR, 0);
	}
	if (x100cb->cur_tups) {
		status = x100_push_data(state, x100cb->cur_tups, x100cb->row_addr);
		if (status < 0) {
			status = vw_translate_err(x100cb->core.session, x100_get_error(state));
			log_debug("x100_push_data returned error");
			x100_push_data(state, 0, 0);
			x100_query_free(state);
			x100cb->x100_query_handle = 0;
			return status;
		}
	}
	return E_DB_OK;
}

DB_STATUS
x100_insert(X100_INSERT_PARAM *x100cb)
{
	DB_STATUS status;
	X100_BULKLOAD x100bl;
	DM_MDATA list;
	memset(&x100bl, 0, sizeof(x100bl));
	memset(&list, 0, sizeof(list));
	log_debug("bulkload %p %ld %d %d", x100bl.col_name, x100bl.row_count, x100cb->row_format->gca_l_col_desc, (int) sizeof(x100bl));
	x100bl.core.session = x100cb->core.session;
	x100bl.core.dbname = x100cb->core.dbname;
	x100bl.core.adfcb = x100cb->core.adfcb;
	x100bl.tablename = x100cb->tablename;
	x100bl.row_format = x100cb->row_format;
	if (status = x100_bulkload_or_insert_init(&x100bl, 1))
		return status;
	x100bl.row_count = 1;
	x100bl.cur_tups = 1;
	x100bl.row_addr = &list;
	list.data_address = (void*)x100cb->data_address;
	if (status = x100_bulkload_push(&x100bl))
		return status;
	return x100_bulkload_finish(&x100bl);
}

static DB_STATUS
x100_simple_query(void *_x100cb, char *query)
{
	VW_CORE_PARMS *x100cb = (VW_CORE_PARMS*)_x100cb;
	DB_STATUS status = E_DB_OK;
	struct X100QueryState *state = 0;
	
	if (x100_query_init(&state, query, 0, 0, x100cb)<0) {
		status = vw_translate_err(x100cb->session, x100_session_get_error(x100cb->session));
		if (state) x100_query_free(state);
		return status;
	}
	x100_query_free(state);
	return E_DB_OK;
}

DB_STATUS
x100_bulkload_finish(X100_BULKLOAD *x100cb)
{
	struct X100QueryState *state = (void*)x100cb->x100_query_handle;
	log_debug("bulkload finish START");
	if (!state)
	{
	    return vw_error(x100cb->core.session, E_VW1010_BKLD_STATE_ERROR, 0);
	}
	if (x100_push_data(state, 0, 0) < 0) {
		vw_translate_err(x100cb->core.session, x100_get_error(state));
		log_debug("x100_push_data returned error");
		x100_query_free(state);
		return E_DB_ERROR;
	}
	x100_query_free(state);
	x100cb->x100_query_handle = NULL;
	log_debug("bulkload finish OK");
	return E_DB_OK;
}

DB_STATUS
x100_create_table(X100_CREATE_TABLE_PARAM *x100cb)
{
	int i, first;
	struct X100QueryText *qt = x100_querytext_init();
	i1 *comp_array = x100cb->pax ? NULL : x100cb->comp_array;

	log_debug("table create <%.*s>", DB_TAB_MAXNAME, x100cb->tablename);

	x100_querytext_add_dbname(qt, "CreateTable('", x100cb->tablename, "', [");
	if (x100cb->pax) x100_querytext_printf(qt, "[");
	for (i = 0, first = 1; i < x100cb->attr_count; i++) {
		int type = x100cb->attr_entry[i]->attr_type;
		int width = x100cb->attr_entry[i]->attr_size;
		int prec = x100cb->attr_entry[i]->attr_precision;
		int null = x100cb->attr_entry[i]->attr_type < 0;
		int prev_group = i > 0 && comp_array ? comp_array[i-1] : 0;
		int group = comp_array ? comp_array[i] : 0;
		int next_group = i + 1 < x100cb->attr_count && comp_array ? comp_array[i+1] : 0;
		char *sname = x100cb->attr_entry[i]->attr_name.db_att_name;
		char x100typename[40];

		if (table_conv_get_x100_type(type, width, prec, x100typename)) {
			if (!first) x100_querytext_printf(qt, ", ");
			if (prev_group != group && group) x100_querytext_printf(qt, " [ ");
			if (null) x100_querytext_printf(qt, "nullable ");
			x100_querytext_add_dbname(qt, "'", sname, "'");
			x100_querytext_printf(qt, " %s", x100typename);
			if (x100cb->enable_compression)
				x100_querytext_printf(qt, " 'AUTO'");
			if (group != next_group && group) x100_querytext_printf(qt, " ]");
			first = 0;
		} else {
			DB_STATUS status;
			char buf[20];
			sprintf(buf, "%d", type);
			status = vw_error(x100cb->core.session, E_VW1008_QRY_BAD_DT_ERROR,
					2, buf, x100_querytext(qt));
			log_error("create_table: unknown type %d", type);
			x100_querytext_free(qt);
			return status;
		}
	}
	if (x100cb->pax) x100_querytext_printf(qt, "]");
	x100_querytext_printf(qt, "]) ['auto_pax'=true");

	if (x100cb->temporary_table)
		x100_querytext_printf(qt, ", 'temporary_table'=true");
	x100_querytext_printf(qt, "]");

	if (x100_simple_query(x100cb, x100_querytext(qt)) == E_DB_ERROR) {
		x100_querytext_free(qt);
		return E_DB_ERROR;
	}
	x100_querytext_free(qt);

	if (!x100cb->enable_minmax) return E_DB_OK;

	qt = x100_querytext_init();

	x100_querytext_add_dbname(qt, "MinMaxIndex('", x100cb->tablename, "', ");
	x100_querytext_add_dbname(qt, "'", x100cb->tablename, "', [");
	for (i = 0, first = 1; i < x100cb->attr_count; i++) {
		char *sname = x100cb->attr_entry[i]->attr_name.db_att_name;
		if (!first) x100_querytext_printf(qt, ", ");
		x100_querytext_add_dbname(qt, "'", sname, "'");
		first = 0;
	}
	x100_querytext_printf(qt, "])");

	if (x100_simple_query(x100cb, x100_querytext(qt)) == E_DB_ERROR) {
		x100_querytext_free(qt);
		return E_DB_ERROR;
	}
	x100_querytext_free(qt);
	return E_DB_OK;
}

DB_STATUS
x100_add_column(X100_ADD_COLUMN_PARAM *x100cb)
{
	struct X100QueryText *qt = x100_querytext_init();
	int type = x100cb->attr_entry->attr_type;
	int width = x100cb->attr_entry->attr_size;
	int prec = x100cb->attr_entry->attr_precision;
	int null = x100cb->attr_entry->attr_type < 0;
	char x100typename[40];

	x100_querytext_add_dbname(qt, "AddColumn('", x100cb->tablename, "', [");

	if (table_conv_get_x100_type(type, width, prec, x100typename)) {
		if (null) x100_querytext_printf(qt, "nullable ");
		x100_querytext_add_dbname(qt, "'", x100cb->column_name, "'");
		x100_querytext_printf(qt, " %s", x100typename);
		if (x100cb->enable_compression)
			x100_querytext_printf(qt, " 'AUTO'");
	} else {
		DB_STATUS status;
		char buf[20];
		sprintf(buf, "%d", type);
		status = vw_error(x100cb->core.session, E_VW1008_QRY_BAD_DT_ERROR,
				2, buf, x100_querytext(qt));
		log_error("add_column: unknown type %d", type);
		x100_querytext_free(qt);
		return status;
	}
//	x100_querytext_printf(qt, "], [%s])", x100cb->default_value);
	x100_querytext_printf(qt, "])");

	if (x100_simple_query(x100cb, x100_querytext(qt)) == E_DB_ERROR) {
		x100_querytext_free(qt);
		return E_DB_ERROR;
	}
	x100_querytext_free(qt);
	return E_DB_OK;
}


DB_STATUS
x100_drop_table(X100_DROP_TABLE_PARAM *x100cb)
{
	struct X100QueryText *qt = x100_querytext_init();
	log_debug("table drop <%.*s>", DB_TAB_MAXNAME, x100cb->tablename);
	log_debug("dbname <%.*s>", DB_DB_MAXNAME, x100cb->core.dbname);
	x100_querytext_add_dbname(qt, "DropTable( '", x100cb->tablename, "', true )");
	if (x100_simple_query(x100cb, x100_querytext(qt)) == E_DB_ERROR) {
		x100_querytext_free(qt);
		return E_DB_ERROR;
	}
	x100_querytext_free(qt);
	return E_DB_OK;
}

DB_STATUS
x100_drop_column(X100_DROP_COLUMN_PARAM *x100cb)
{
	struct X100QueryText *qt = x100_querytext_init();
	x100_querytext_add_dbname(qt, "DropColumn( '", x100cb->tablename, "',");
	x100_querytext_add_dbname(qt, "['",x100cb->column_name,"'])");
	if (x100_simple_query(x100cb, x100_querytext(qt)) == E_DB_ERROR) {
		x100_querytext_free(qt);
		return E_DB_ERROR;
	}
	x100_querytext_free(qt);
	return E_DB_OK;
}

DB_STATUS
x100_create_key(X100_CREATE_KEY *x100cb) {
	char *type;
	int i;
	struct X100QueryText *qt = x100_querytext_init();
	if (x100cb->key_type == X100_KEY_PRIMARY) type = "PrimaryKey";
	else if(x100cb->key_type == X100_KEY_FOREIGN) type = "ForeignKey";
	else type = "UniqueKey";
	log_debug("create %s on <%.*s>", type, DB_TAB_MAXNAME, x100cb->tablename);
	x100_querytext_printf(qt, "%s( ", type);
	x100_querytext_add_dbname(qt, "'", x100cb->key_name, "', ");
	x100_querytext_add_dbname(qt, "'", x100cb->tablename, "', ");
	if (x100cb->key_type == X100_KEY_FOREIGN) {
		x100_querytext_add_dbname(qt, "'", x100cb->ref_key_name, "', ");
		x100_querytext_add_dbname(qt, "'", x100cb->ref_table, "', ");
	}
	for (i = 0; i < x100cb->key_count; i++) {
		x100_querytext_add_dbname(qt, i ? ", '" : "[ '", x100cb->columns[i], "'");
	}
	x100_querytext_printf(qt, " ])");
	if (x100_simple_query(x100cb, x100_querytext(qt)) == E_DB_ERROR) {
		x100_querytext_free(qt);
		return E_DB_ERROR;
	}
	x100_querytext_free(qt);
	return E_DB_OK;
}

DB_STATUS
x100_drop_key(X100_DROP_KEY *x100cb) {
	struct X100QueryText *qt = x100_querytext_init();
	log_debug("drop key %s on <%.*s>", x100cb->key_name, DB_TAB_MAXNAME, x100cb->tablename);
	x100_querytext_printf(qt, "DropKey( ");
	x100_querytext_add_dbname(qt, "'", x100cb->key_name, "', ");
	x100_querytext_add_dbname(qt, "'", x100cb->tablename, "')");
	if (x100_simple_query(x100cb, x100_querytext(qt)) == E_DB_ERROR) {
		x100_querytext_free(qt);
		return E_DB_ERROR;
	}
	x100_querytext_free(qt);
	return E_DB_OK;
}

static void *
x100_expand(void *prev, int size, int nmemb) {
	return realloc(prev, nmemb * size);
}

static char (*x100_list_primary_key(VW_CORE_PARMS *x100cb, char *tablename, int *size))[DB_ATT_MAXNAME] {
	int cl_size = 0, cl_asize = 1, cnt, res;
	char (*cl)[DB_ATT_MAXNAME] = x100_expand(0, sizeof(*cl), cl_asize);
	struct X100QueryText *qt = x100_querytext_init();
	struct X100QueryState *state = 0;
	GCA_TUP_DESCR format[1];
	memset(format, 0, sizeof(format));
	format->gca_l_col_desc = 1;
	format->gca_tsize = DB_ATT_MAXNAME;
	format->gca_col_desc[0].gca_attdbv.db_datatype = X100_CATNAME_TYPE;
	format->gca_col_desc[0].gca_attdbv.db_length = DB_ATT_MAXNAME;
	x100_querytext_add_dbname(qt,
"Project("
	"Sort("
		"HashJoin01("
			"HashJoin01("
				"SysScan( 'gcs' , [ 'cg_ptr', 'cg_column_ptr', 'cg_column_nr' ]), [ cg_ptr ],"
				"Select("
					"HashJoin1("
						"SysScan( 'idxs' , [ 'idx_table_ptr', 'idx_type', 'idx_ptr', 'idx_colgroup_ptr' ]), [ idx_table_ptr ],"
						"SysScan( 'tables', [ 'table_name' , 'table_ptr' ]), [ table_ptr ]"
					"),"
					"&&(==(table_name,'",tablename,"'),==(idx_type, uint('3')))"
				"), [ idx_colgroup_ptr ]"
			"), [ cg_column_ptr ],"
			"SysScan( 'columns' , [ 'column_ptr', 'column_name' ]), [ column_ptr ]"
		"), [ cg_column_nr ]"
	"), [ column_name ]"
")"
	);
	if (x100_query_init(&state, x100_querytext(qt), format, 0, x100cb)<0)
	{
		vw_translate_err(x100cb->session, x100_get_error(state));
		if (state) x100_query_free(state);
		x100_querytext_free(qt);
		return 0;
	}
	x100_querytext_free(qt);
	do {
		if (cl_asize == cl_size) cl = x100_expand(cl, sizeof(*cl), cl_asize*=2);
		cnt = cl_asize - cl_size;
		res = x100_pop_data(state, &cnt, cl + cl_size);
		if (res < 0) {
			vw_translate_err(x100cb->session, x100_get_error(state));
			x100_query_free(state);
			free(cl);
			return 0;
		}
		cl_size += cnt;
	} while (res);
	x100_query_free(state);
	*size = cl_size;
	return cl;
}

struct foreign_key {
	char key[DB_ATT_MAXNAME];
	char column[DB_ATT_MAXNAME];
};

static struct foreign_key *
x100_find_foregin_key_candidates(VW_CORE_PARMS *x100cb, char *tablename, int *size) {
	int cl_size = 0, cl_asize = 1, cnt, res;
	struct foreign_key *cl = x100_expand(0, sizeof(*cl), cl_asize);
	struct X100QueryText *qt = x100_querytext_init();
	struct X100QueryState *state = 0;
	GCA_TUP_DESCR format[2];
	memset(format, 0, sizeof(format));
	format->gca_l_col_desc = 2;
	format->gca_tsize = 2 * DB_ATT_MAXNAME;
	format->gca_col_desc[0].gca_attdbv.db_datatype = X100_CATNAME_TYPE;
	format->gca_col_desc[0].gca_attdbv.db_length = DB_ATT_MAXNAME;
	format->gca_col_desc[1].gca_attdbv.db_datatype = X100_CATNAME_TYPE;
	format->gca_col_desc[1].gca_attdbv.db_length = DB_ATT_MAXNAME;
	x100_querytext_add_dbname(qt,
"Project("
	"Sort("
		"HashJoin1("
			"HashJoin01("
				"SysScan( 'gcs' , [ 'cg_ptr', 'cg_column_ptr', 'cg_column_nr' ]), [ cg_ptr ],"
				"HashJoin1("
					"HashJoin01("
						"Select("
							"SysScan( 'idxs', [ 'idx_type', 'idx_name', 'idx_table_ptr', 'idx_colgroup_ptr' ]),"
							"==(idx_type, sint('4'))"
						"), [ idx_table_ptr ],"
						"Select("
							"SysScan( 'tables', [ 'table_name', 'table_ptr' ]),"
							"==(table_name, '",tablename,"')"
						"), [ table_ptr ]"
					"), [ idx_colgroup_ptr ],"
					"SysScan( 'colgroups', [ 'colgroup_ptr', 'colgroup_num_columns' ]), [ colgroup_ptr ]"
				"), [idx_colgroup_ptr]"
			"), [cg_column_ptr],"
			"SysScan('columns' , [ 'column_ptr', 'column_name' ]), [ column_ptr ]"
		"), [ colgroup_num_columns desc, idx_name, cg_column_nr ]"
	"), [ idx_name, column_name ]"
")"
	);
	if (x100_query_init(&state, x100_querytext(qt), format, 0, x100cb)<0)
	{
		vw_translate_err(x100cb->session, x100_get_error(state));
		if (state) x100_query_free(state);
		x100_querytext_free(qt);
		return 0;
	}
	x100_querytext_free(qt);
	do {
		if (cl_asize == cl_size) cl = x100_expand(cl, sizeof(*cl), cl_asize*=2);
		cnt = cl_asize - cl_size;
		res = x100_pop_data(state, &cnt, cl + cl_size);
		if (res < 0) {
			vw_translate_err(x100cb->session, x100_get_error(state));
			x100_query_free(state);
			free(cl);
			return 0;
		}
		cl_size += cnt;
	} while (res);
	x100_query_free(state);
	*size = cl_size;
	return cl;
}


DB_STATUS
x100_cluster_index(X100_CLUSTER_INDEX *x100cb) {
	struct X100QueryText *qt;
	char (*pk)[DB_ATT_MAXNAME] = 0;
	char **columnlist;
	struct foreign_key *fk;
	char idxname[DB_ATT_MAXNAME], sortname[DB_ATT_MAXNAME];
	int i, j, pk_size, fk_size, list_size = 0, joinkey_end = 0, joinkey_start = 0;
	log_debug("create cluster index %s on <%.*s>", x100cb->index_name, DB_TAB_MAXNAME, x100cb->tablename);
	log_debug("dbname %s", x100_session_get_dbname(x100cb->core.session));
	pk = x100_list_primary_key((VW_CORE_PARMS*)x100cb, x100cb->tablename, &pk_size);
	if (!pk) return vw_error(x100cb->core.session, E_VW1013_PKEY_ERROR, 2,
					x100_session_get_dbname(x100cb->core.session), x100cb->tablename);
	fk = x100_find_foregin_key_candidates((VW_CORE_PARMS*)x100cb, x100cb->tablename, &fk_size);
	if (!fk) {
		/* free(pk); */
		return vw_error(x100cb->core.session, E_VW1014_FKEY_ERROR, 2,
					x100_session_get_dbname(x100cb->core.session), x100cb->tablename);
	}
	// check if a prefix of idexed columns set is a foreign key
	if (x100cb->enable_joinindex) for (j = i = 0; i < fk_size; i++) {
		// Skip to next key candidate if there was no match
		if (j == 0 && i > 0 &&
			!memcmp(fk[i].key, fk[i-1].key, DB_ATT_MAXNAME)) continue;
		// attribute match found
		if (j < x100cb->order_count && !memcmp(fk[i].column, x100cb->order_columns[j], DB_ATT_MAXNAME)) {
			j++;
			// last key attribute matched
			if (i == fk_size - 1 || memcmp(fk[i].key, fk[i+1].key, DB_ATT_MAXNAME)) {
				joinkey_end = i + 1;
				joinkey_start = joinkey_end - j;
				break;
			}
		} else {
			j = 0;
		}
	}
	if (joinkey_end) {
		idxname[0] = sortname[0] = '_';
		idxname[1] = 'j';
		sortname[1] = 's';
		memcpy(idxname + 2, x100cb->tablename, DB_ATT_MAXNAME - 2);
		memcpy(sortname + 2, x100cb->tablename, DB_ATT_MAXNAME - 2);		
		//create join index
		qt = x100_querytext_init();
		x100_querytext_add_dbname(qt, "JIndex( '", idxname, "', ");
		x100_querytext_add_dbname(qt, "'", x100cb->tablename, "', ");
		x100_querytext_add_dbname(qt, "'", fk[joinkey_start].key, "')");
		if (x100_simple_query(x100cb, x100_querytext(qt)) == E_DB_ERROR) {
			free(fk);
			free(pk);
			x100_querytext_free(qt);
			return E_DB_ERROR;
		}
		if (x100cb->create_join_index_callback)
			x100cb->create_join_index_callback(x100cb->callback_param, x100cb->tablename, idxname, fk[joinkey_start].key);
		x100_querytext_free(qt);
	}
	free(fk);
	qt = x100_querytext_init();
	columnlist = malloc(sizeof(char *) * (pk_size + x100cb->order_count));
	x100_querytext_add_dbname(qt, "SparseIndex( '", x100cb->index_name, "', ");
	x100_querytext_add_dbname(qt, "'", x100cb->tablename, "', [ ");
	if (joinkey_end) {
		x100_querytext_add_dbname(qt, "'", idxname, "'");
		columnlist[list_size++] = idxname;
	}
	for (i = joinkey_end - joinkey_start; i < x100cb->order_count; i++) {
		x100_querytext_add_dbname(qt, i?", '":"'", x100cb->order_columns[i], "'");
		columnlist[list_size++] = x100cb->order_columns[i];
	}
	//add pk eliminating duplicate attributes
	for (i = 0; i < pk_size; i++) {
		for (j = 0; j < x100cb->order_count; j++)
			if (!memcmp(pk[i], x100cb->order_columns[j], DB_ATT_MAXNAME))
				break;
		if (j < x100cb->order_count) continue;
		x100_querytext_add_dbname(qt, ", '", pk[i], "'");
		columnlist[list_size++] = pk[i];
	}
	x100_querytext_printf(qt, " ] )");
	if (x100_simple_query(x100cb, x100_querytext(qt)) == E_DB_ERROR) {
		free(columnlist);
		free(pk);
		x100_querytext_free(qt);
		return E_DB_ERROR;
	}
	if (x100cb->create_sort_order_callback)
		x100cb->create_sort_order_callback(x100cb->callback_param, x100cb->tablename, x100cb->index_name, columnlist, x100cb->order_count);
	free(columnlist);
	free(pk);
	x100_querytext_free(qt);
	return E_DB_OK;
}

DB_STATUS
x100_copyinto_init(X100_COPYINTO_PARAM *x100ci)
{
	struct X100QueryText *qt;
	struct X100QueryState *qs = 0;
	int j, first;
	qt = x100_querytext_init();
	x100_querytext_add_dbname(qt,"MScan('",x100ci->tablename,"',[");
	for (j = 0, first = 1; j < x100ci->row_format->gca_l_col_desc; j++) {
		x100_querytext_add_dbname(qt, first?"'":", '", x100ci->col_name[j].db_att_name, "'");
		first = 0;
	}
	x100_querytext_printf(qt,"])");
	if (x100_query_init(&qs, x100_querytext(qt), x100ci->row_format, NULL, &x100ci->core)<0) {
		DB_STATUS status;
		status = vw_translate_err(x100ci->core.session, x100_get_error(qs));
		if (qs) x100_query_free(qs);
		x100_querytext_free(qt);
		return status;
	}
	x100ci->x100_query_handle = (void*)qs;
	x100ci->data_address = 0;
	x100_querytext_free(qt);
	return E_DB_OK;
}

DB_STATUS
x100_copyinto_pull(X100_COPYINTO_PARAM *x100ci)
{
	int res, cnt;
	struct X100QueryState *qs = (struct X100QueryState*)x100ci->x100_query_handle;
	if (!qs) {
		free(x100ci->data_address);
		x100ci->cur_tups = 0;
		return E_DB_OK;
	}
	cnt = x100ci->row_count;
	if (x100ci->data_address) free(x100ci->data_address);
	x100ci->data_address = malloc(cnt * x100ci->row_format->gca_tsize);
	res = x100_pop_data(qs, &cnt, x100ci->data_address);
	if (res < 0) {
		DB_STATUS status;
		status = vw_translate_err(x100ci->core.session, x100_get_error(qs));
		log_error("cannot fetch new data for copy into");
		x100_query_free(qs);
		x100ci->x100_query_handle = NULL;
		return status;
	}
	x100ci->cur_tups = cnt;
	if (!res) {
		x100_query_free(qs);
		x100ci->x100_query_handle = NULL;
	}
	log_debug("copyinto returned %d rows %s",cnt,res?"(more will follow)":"(end of data)");
	return E_DB_OK;
}

/* Terminate a copy-into.  Normally it's self terminating, but if we
** interrupt the copy before it completes, this call will tell the
** x100 side to stop and clean up.
*/
void
x100_copyinto_stop(X100_COPYINTO_PARAM *x100ci)
{
	struct X100QueryState *qs = (struct X100QueryState*)x100ci->x100_query_handle;

	if (qs != NULL) {
		x100_query_free(qs);
		x100ci->x100_query_handle = NULL;
	}
}


/* Start up a Vectorwise fastload, aka VWLOAD.
** The vwloader will make its own connection to x100-server.  What we need
** to do here is to:
** a) ensure that there's an x100 server started, and
** b) tell x100 what table is about to be loaded, and have it return a
**    single use magic word that vwloader will use to authorize its
**    connection to x100-server.
** *NOTE* initially (b) is not done, we'll simply return the server wide
** password.
*/
DB_STATUS
x100_vwload_init(X100_VWLOAD_PARAM *cb)
{
    struct X100SessionState *session = cb->core.session;

    x100_connect_if_needed(session);
    if (session->conn == NULL || session->connection_state == CONNECTION_STATE_ERROR)
	return vw_translate_err(session, x100_session_get_error(session));

    /* **** FIXME this is where the authorizing exchange would go, if
    ** we were to do it.  For now, just dig the port and password out
    ** of the server data block.
    ** Note, the token return area is at least AUTHPASS_MAX_SIZE, no need
    ** to worry about strcpy overrun.  See X100_VWLOAD_MAX_SIZE.
    */
    cb->port = session->conn->server->port;
    strcpy(cb->token, session->conn->server->authpass);
    return E_DB_OK;
} /* x100_vwload_init */


DB_STATUS
x100_create_database(X100_DATABASE_PARAM *x100db)
{
	return E_DB_OK;
}

DB_STATUS
x100_destroy_database(X100_DATABASE_PARAM *x100db)
{
	char trimmed_dbname[DB_DB_MAXNAME+1];
	char *dbname = x100db->core.dbname;
	int i;

	if (!dbname) {
		log_error("no dbname specified for x100 operation");
		return E_DB_ERROR;
	}
	strncpy(trimmed_dbname, dbname, DB_DB_MAXNAME);
	trimmed_dbname[DB_DB_MAXNAME] = 0;
	for (i = strlen(trimmed_dbname) - 1; i >= 0 && trimmed_dbname[i] == ' '; i--)
		trimmed_dbname[i] = 0;
	log_debug("destroydb %s", trimmed_dbname);
	x100_destroydb(trimmed_dbname);
	return E_DB_OK;
}

static void 
x100_idxlate(X100_SYSCALL_PARAM *x100sc, char *id, i4 idlen, bool delim_id)
{
    DB_ERROR		err_blk;
    DB_STATUS		status;
    u_i4		dbxlate = x100sc->dbxlate;
    u_i4		len = idlen;
    char		xbuf[DB_TAB_MAXNAME];
    u_i4		xlen;
    u_i4		ret_mode;

    if (delim_id)
    {
	/*
	** CUI_ID_DLM_U: Translate delimited to upper case
	** CUI_ID_DLM_M: Don't translate delimited (it is mixed case)
	** else translate delimited to lower case (default)
	**
	** CUI_ID_DLM	- Treat the identifier as delimited
	** CUI_ID_NORM  - Treat the identifier as if normalized
	** CUI_ID_STRIP	- Ignore trailing white space in source identifier
	*/
	dbxlate |= (CUI_ID_DLM | CUI_ID_NORM | CUI_ID_STRIP);
	xlen = sizeof(xbuf);
	status = cui_idxlate(id, &len, xbuf, &xlen, dbxlate, &ret_mode, &err_blk);
	if (status == E_DB_OK && xlen <= len)
	{
	    memcpy(id, xbuf, xlen);		
	}
    }
    else
    {
	/*
	** CUI_ID_REG_U: Translate regular identifier to upper case 
	** else translate regular identifier to lower case (default)
	*/
        cui_f_idxlate(id, idlen, dbxlate);
    }
}

#define NEXTCH ((c = i < p->db_t_count ? p->db_t_text[i++] : 256) < 256)
#define READ_ID(id) \
			delim_id = FALSE; \
			id_pos = 0; \
			if (c == '"') for (delim_id=TRUE;NEXTCH;) { \
				if (c == '"') { \
					NEXTCH; \
					break; \
				} \
				if (id_pos < DB_TAB_MAXNAME) id[id_pos++] = c; \
			} else do { \
				if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')|| (c >= '0' && c <= '9') || c == '_')) break; \
				if (id_pos < DB_TAB_MAXNAME) id[id_pos++] = c; \
			} while (NEXTCH); \
			if (id_pos == 0) continue; \
			while (id_pos < DB_TAB_MAXNAME) id[id_pos++] = ' ';\
			x100_idxlate(x100sc, id, 256, delim_id);
#define READ_WHITESPACE \
			do { \
				if (c != ' ' && c != '\t') break; \
			} while (NEXTCH);

static int
x100_querytext_add_checkpoint_param(struct X100QueryText *qt, struct X100QueryText *cqt, X100_SYSCALL_PARAM *x100sc)
{
	int i = 0, first = 1, num = 0, c;
	DB_TEXT_STRING *p = x100sc->callparam;
	bool delim_id;

	x100_querytext_printf(qt, ",['");
	NEXTCH;
	for (;c < 256;) {
		READ_WHITESPACE
		if (c == '"' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') {
			int accessres = 0;
			int id_pos;
			char id1[DB_TAB_MAXNAME+1], id2[DB_TAB_MAXNAME+1], *id, *qual;
			READ_ID(id1)
			READ_WHITESPACE
			if (c == '.') {
				NEXTCH;
				READ_WHITESPACE
				READ_ID(id2)
				qual = id1;
				id = id2;
			} else {
				qual = NULL;
				id = id1;
			}
			x100_querytext_add_dbname(qt, "", id, "");
			if (x100sc->table_access) accessres = x100sc->table_access(x100sc, id, qual);
			if (accessres < 0) {
				int j = DB_TAB_MAXNAME;
				do id[j] = 0; while(j && id[--j] == ' ');
				vw_error(x100sc->core.session, accessres == -1 ? E_VW1084_NO_ACCESS_TO_TABLE : E_VW1041_TABLE_NOT_EXISTS, 1, id);
				return -1;
			}
			if (first) {
				x100_querytext_add_dbname(cqt, "Union(Project(Aggr(MScan('",id,"',[]),[],[c=count()])");
				x100_querytext_add_dbname(cqt, ",[c,t='",id,"']),");
				num++;
				first = 0;
			}
		} else if (c == ',') {
			first = 1;
			x100_querytext_printf(qt, "','");
			NEXTCH;
		} else {
			x100_querytext_printf(qt, "%c", c);
			NEXTCH;
		}
	}
	x100_querytext_printf(qt, "']");
	x100_querytext_printf(cqt, "Project(Aggr(Array([x=0]),[x],[c=count()]),[c,t=''])");
	for(i=0;i<num;i++) x100_querytext_printf(cqt, ")");
	return num;
}

static void
sample_update_count(X100_SYSCALL_PARAM *x100sc, char table[DB_TAB_MAXNAME], char table_owner[DB_OWN_MAXNAME], i8 new_cnt)
{
	log_debug("new table size %.*s = %ld", DB_TAB_MAXNAME, table, new_cnt);
}

DB_STATUS
x100_syscall(X100_SYSCALL_PARAM *x100sc)
{
	DB_STATUS status;
	struct X100QueryText *qt, *cqt = 0;
	struct X100QueryState *qs = 0;
	int res, is_combine;
	GCA_TUP_DESCR format[2];
	memset(format, 0, sizeof(format));
	memset(&x100sc->result, 0, sizeof(x100sc->result));
	format->gca_l_col_desc = 1;
	format->gca_tsize = sizeof(x100sc->result);
	format->gca_col_desc[0].gca_attdbv.db_datatype = DB_VCH_TYPE;
	format->gca_col_desc[0].gca_attdbv.db_length = sizeof(x100sc->result);
	qt = x100_querytext_init();
	is_combine = (x100sc->callname->db_t_count == 7 && !memcmp(x100sc->callname->db_t_text, "combine", 7))
	          || (x100sc->callname->db_t_count == 10 && !memcmp(x100sc->callname->db_t_text, "checkpoint", 10));
	if (is_combine)
		x100_querytext_printf(qt,"Sys(checkpoint");
	else
		x100_querytext_add_dbtextstring(qt,"Sys(",x100sc->callname,"");
	if (x100sc->callparam) {
		if (is_combine) {
			cqt = x100_querytext_init();
			res = x100_querytext_add_checkpoint_param(qt, cqt, x100sc);
			if (res < 0) {
				x100_querytext_free(qt);
				if (cqt) x100_querytext_free(cqt);
				return E_DB_ERROR;
			}
		} else x100_querytext_add_dbtextstring(qt,",[",x100sc->callparam,"]");
	}
	x100_querytext_printf(qt, ")");
	if (x100_query_init(&qs, x100_querytext(qt), format, NULL, &x100sc->core)<0) {
		status = vw_translate_err(x100sc->core.session, x100_get_error(qs));
		if (qs) x100_query_free(qs);
		x100_querytext_free(qt);
		if (cqt) x100_querytext_free(cqt);
		return status;
	}
	x100_querytext_free(qt);
	if (!qs) {
		if (cqt) x100_querytext_free(cqt);
		return 0;
	}
	do {
		int cnt = 1;
		res = x100_pop_data(qs, &cnt, x100sc->result.storage);
		if (res < 0) {
    			status = vw_translate_err(x100sc->core.session, x100_get_error(qs));
			x100_query_free(qs);
			if (cqt) x100_querytext_free(cqt);
			return status;
		}
	} while (res);
	x100_query_free(qs);
	log_debug("x100_syscall OK");
	if (!cqt) return E_DB_OK;
	qs = 0;
	format->gca_l_col_desc = 2;
	format->gca_tsize = 8 + DB_TAB_MAXNAME;
	format->gca_col_desc[0].gca_attdbv.db_datatype = DB_INT_TYPE;
	format->gca_col_desc[0].gca_attdbv.db_length = 8;
	format->gca_col_desc[1].gca_attdbv.db_datatype = X100_CATNAME_TYPE;
	format->gca_col_desc[1].gca_attdbv.db_length = DB_TAB_MAXNAME;
	if (x100_query_init(&qs, x100_querytext(cqt), format, NULL, &x100sc->core)<0) {
		status = vw_translate_err(x100sc->core.session, x100_get_error(qs));
		if (qs) x100_query_free(qs);
		x100_querytext_free(cqt);
		return status;
	}
	x100_querytext_free(cqt);
	if (!qs) return E_DB_OK;
	do {
		int cnt = 1;
		i8 dat[(8 + DB_TAB_MAXNAME+7)/8];
		res = x100_pop_data(qs, &cnt, dat);
		if (res < 0) {
    			status = vw_translate_err(x100sc->core.session, x100_get_error(qs));
			x100_query_free(qs);
			return status;
		}
		if (res) (x100sc->update_count ? x100sc->update_count : sample_update_count)(x100sc, (char*)(dat+1), NULL, *(i8*)dat);
	} while (res);
	x100_query_free(qs);
	return E_DB_OK;
}

DB_STATUS
x100_session_begin(X100_SESSION_PARAM *x100cb)
{
	DB_STATUS status;
	if (x100_session_init(&x100cb->core.session, x100cb->ing_sessid, x100cb->core.dbname, x100cb->core.adfcb) < 0) {
		status = E_DB_ERROR;
		if (x100cb->ing_sessid != DB_NOSESSION && x100cb->core.session){
			status = vw_translate_err(x100cb->core.session, x100_session_get_error(x100cb->core.session));
			x100_session_free(x100cb->core.session);
		}
		return status;
	}
	log_debug("session_start: %s", x100_session_get_dbname(x100cb->core.session));
	return E_DB_OK;
}


/* The interrupt handler for X100.  Although we're specifically interested
** in ATTN, REMOVE, and KILL events, there's no harm in handling everything,
** just like QEF does.
** Basically, the reason for this handler is to indicate to the rest of
** x100client that if it's waiting for the X100 server to do something,
** it should stop waiting, send the X100 server an interrupt message,
** and bubble an error return back up to the calling facility.  The only
** times we can be in an X100 server wait is if a query is running, which
** in turn means that we'll never see stuff like RCVR (force-abort) events,
** or CTHREAD (cut thread) events, and if we do, they are irrelevant to
** the X100 interface (and the interrupt notification will be cleared when
** the next x100 query starts because it's irrelevant.)
*/
STATUS
x100_interrupt_handler(i4 eid, void *cb)
{
    struct X100SessionState *state = (struct X100SessionState *) cb;

    if (state)
	state->interrupted = TRUE;
    return OK;
}


DB_STATUS
x100_session_end(X100_SESSION_PARAM *x100cb)
{
	if (x100cb && x100cb->core.session) {
		log_debug("session_close: %s", x100_session_get_dbname(x100cb->core.session));
		x100_session_free(x100cb->core.session);
	} else {
		log_debug("session_close: tried to end a NULL session.");
	}
	return E_DB_OK;
}

DB_STATUS
x100_supported_dt(i4 dt)
{
    i4	attr_type;

    /*
    ** Check for unsupported datatype
    ** See table_conv_get_x100_type() for supported/mapped datatypes
    ** x100_type = table_conv_get_x100_type(attr_type, 
    **    attr->attr_size, attr->attr_precision, NULL);
    */

    attr_type = abs(dt);
    switch (attr_type)
    {
	case DB_BOO_TYPE: /* boolean */
	case DB_BYTE_TYPE: /* byte */
	case DB_VBYTE_TYPE: /* byte varying,varbyte,binary varying, varbinary */
	case DB_LBYTE_TYPE: /* long byte */
	case DB_LVCH_TYPE: /* long varchar */
	case DB_LNVCHR_TYPE: /* long nvarchar */
	case DB_LOGKEY_TYPE: /* object_key */
	case DB_TABKEY_TYPE: /* table_key */
	case DB_DTE_TYPE:  /* ingresdate */
	case DB_TMW_TYPE: /* time with time zone */
	case DB_TME_TYPE: /* time with local time zone */
	case DB_TSW_TYPE: /* timestamp with time zone */
	case DB_TSTMP_TYPE: /* timestamp with local time zone */
	{
	    return (E_DB_ERROR);
	}

	default:
	    break;
    }

    return (E_DB_OK);
}

#else
/* Stubs for non-IVW builds */

DB_STATUS x100_create_database(X100_DATABASE_PARAM *x){return (E_DB_OK);}
DB_STATUS x100_destroy_database(X100_DATABASE_PARAM *x){return (E_DB_OK);}

DB_STATUS x100_query(QEA_X100 *x100cb){return (E_DB_FATAL);}
DB_STATUS x100_query_stop(QEA_X100 *x100cb){return (E_DB_OK);}

DB_STATUS x100_create_table(X100_CREATE_TABLE_PARAM *x){return (E_DB_FATAL);}
DB_STATUS x100_drop_table(X100_DROP_TABLE_PARAM *x){return (E_DB_FATAL);}
DB_STATUS x100_add_column(X100_ADD_COLUMN_PARAM *x){return (E_DB_FATAL);}
DB_STATUS x100_drop_column(X100_DROP_COLUMN_PARAM *x){return (E_DB_FATAL);}
DB_STATUS x100_create_key(X100_CREATE_KEY *x){return (E_DB_FATAL);}
DB_STATUS x100_drop_key(X100_DROP_KEY *x){return (E_DB_FATAL);}
DB_STATUS x100_cluster_index(X100_CLUSTER_INDEX *x){return (E_DB_FATAL);}

DB_STATUS x100_session_begin(X100_SESSION_PARAM *x){return (E_DB_OK);}
STATUS x100_interrupt_handler(i4 eid, void *cb){return (OK);}
DB_STATUS x100_session_end(X100_SESSION_PARAM *x){return (E_DB_OK);}

DB_STATUS x100_start_txn(struct X100SessionState *state){return (E_DB_OK);}
DB_STATUS x100_savepoint(struct X100SessionState *state, DB_SP_NAME *savepoint){return (E_DB_OK);}
DB_STATUS x100_commit(struct X100SessionState *state){return (E_DB_OK);}
DB_STATUS x100_rollback(struct X100SessionState *state, DB_SP_NAME *savepoint){return (E_DB_OK);}

ADF_CB *x100_session_get_adfcb(struct X100SessionState *state){return NULL;}

void x100_print_id(char *name, int namelen, char *output){}
void x100_print_quoted_id(char *name, int namelen, char *output){}

DB_STATUS x100_bulkload_init(X100_BULKLOAD *x){return (E_DB_FATAL);}
DB_STATUS x100_bulkload_push(X100_BULKLOAD *x){return (E_DB_FATAL);}
DB_STATUS x100_bulkload_finish(X100_BULKLOAD *x){return (E_DB_FATAL);}
DB_STATUS x100_insert(X100_INSERT_PARAM *x){return (E_DB_FATAL);}

DB_STATUS x100_copyinto_init(X100_COPYINTO_PARAM *x){return (E_DB_FATAL);}
DB_STATUS x100_copyinto_pull(X100_COPYINTO_PARAM *x){return (E_DB_FATAL);}
void x100_copyinto_stop(X100_COPYINTO_PARAM *x100ci){}
DB_STATUS x100_vwload_init(X100_VWLOAD_PARAM *x){return (E_DB_FATAL);}

DB_STATUS x100_syscall(X100_SYSCALL_PARAM *x){return (E_DB_FATAL);}
DB_STATUS x100_supported_dt(i4 dt){ return (E_DB_OK);}

#endif /* conf_IVW */
