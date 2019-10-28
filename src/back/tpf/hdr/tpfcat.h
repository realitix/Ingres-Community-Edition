/*
**Copyright (c) 2004 Actian Corporation
*/

/**
** Name: TPFCAT.H - CDB catalog structures for TPF.
**
** Description:
**      This file contains all the catalog structures dedicated to TPF's use
**
** 	    TPC_D1_DXLOG	    - Catalog structure IIDD_DDB_DXLOG
** 	    TPC_D2_DXLDBS	    - Catalog structure IIDD_DDB_DXLDBS
** 	    TPC_L1_DBCAPS	    - Catalog structure IIDBCAPABILITIES
**	    TPC_I1_STARCDBS	    - Catalog structure IISTAR_CDBS
**
** History: $Log-for RCS$
**      12-oct-89 (carl)
**          created
**      07-jul-90 (carl)
**	    added new fields d2_8_ldb_lxid1, d2_9_ldb_lxid2, d2_10_ldb_lxname 
**	    and d2_11_ldb_lxflags to TPC_D2_DXLDBS
**      28-oct-90 (carl)
**	    replaced defines for TPC_D1_DXLOG.d1_5_dx_state with more meaningful
**	    prefix to avoid confusion
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/*}
** Name: TPC_D1_DXLOG - Catalog IIDD_DDB_DXLOG
**
**	An entry is inserted into the catalog only when the DX is in
**	one of 3 states: SECURE, COMMIT, ABORT
**
** Description:
**      This structure matches the schema of IIDD_DDB_DXLOG and is used for
**	accessing the catalog.
**      
** History:
**      12-oct-89 (carl)
**          created
**      28-oct-90 (carl)
**	    replaced defines for d1_5_dx_state with more meaningful prefix
**	    to avoid confusion
*/

typedef struct 
{
    i4	    d1_1_dx_id1;	    /* part 1 of DX id */
    i4	    d1_2_dx_id2;	    /* part 2 of DX id */
    DB_DB_STR  d1_3_dx_name;	    /* the DX name */
    i4	    d1_4_dx_flags;	    /* for future use */
    i4	    d1_5_dx_state;	    /* DX state */

#define LOG_STATE0_UNKNOWN  0	    /* unknown state, place holder */
#define LOG_STATE1_SECURE   1	    /* DX was last in secure state */
#define LOG_STATE2_COMMIT   2       /* DX was last in commit state */
#define LOG_STATE3_ABORT    3       /* DX was last in abort state */
/*
#define TPC_0DX_UNKNOWN	    0	    ** unknown state, place holder **
#define TPC_1DX_SECURE	    1	    ** DX was last in secure state **
#define TPC_2DX_COMMIT	    2       ** DX was last in commit state **
#define TPC_3DX_ABORT	    3       ** DX was last in abort state **
*/

    DB_DATE_STR d1_6_dx_create;	    /* time DX entry was created/recorded */
    DB_DATE_STR d1_7_dx_modify;	    /* time DX entry was last updated, 
				     * e.g., when recovery was attempted */
    i4	    d1_8_dx_starid1;	    /* part 1 of STAR id */
    i4	    d1_9_dx_starid2;	    /* part 2 of STAR id */
    DB_NODE_STR d1_10_dx_ddb_node;  /* DDB's node name */
    DB_DB_STR d1_11_dx_ddb_name;    /* DDB's name */
    DB_TYPE_STR d1_12_dx_ddb_dbms;  /* DDB's dbms name */
    i4	    d1_13_dx_ddb_id;	    /* assigned id of the LDB */

#define	TPC_01_CC_DXLOG_13	13  /* number of columns in catalog */

}   TPC_D1_DXLOG;


/*}
** Name: TPC_D2_DXLDBS - Catalog IIDD_DDB_DXLDBS.
**
**	For each update LDB involved in a DX, an entry will be inserted into
**	this catalog recording its identity.
**
** Description:
**      This structure matches the schema of IIDD_DDB_DXLDBS and is used for
**	accessing the catalog.
**      
** History:
**      12-oct-89 (carl)
**          created
**      07-jul-90 (carl)
**	    added new fields d2_8_ldb_lxid1, d2_9_ldb_lxid2, d2_10_ldb_lxname 
**	    and d2_11_ldb_lxflags
*/

typedef struct _TPC_D2_DXLDBS
{
    i4		    d2_1_ldb_dxid1;	/* part 1 of DX id */
    i4		    d2_2_ldb_dxid2;	/* part 2 of DX id */
    DB_NODE_STR     d2_3_ldb_node;	/* node name */
    char            d2_4_ldb_name[DD_256_MAXDBNAME + 1];/* database name */
    DB_TYPE_STR     d2_5_ldb_dbms;	/* dbms name */
    i4		    d2_6_ldb_id;	/* assigned id of the LDB */
    i4		    d2_7_ldb_lxstate;	/* state of the LX on the LDB */
    i4		    d2_8_ldb_lxid1;	/* part 1 of LX id */
    i4		    d2_9_ldb_lxid2;	/* part 2 of LX id */
    DB_DB_STR       d2_10_ldb_lxname;   /* the LX's name */
    i4		    d2_11_ldb_lxflags;  /* for future use */

#define	TPC_LX_02FLAG_2PC	0x0002
				/* ON if the LDB supports the 2PC protocol,
				** OFF otherwise meaning that this is a 1PC 
				** site */

#define	TPC_02_CC_DXLDBS_11	11	/* number of columns in catalog */
}   TPC_D2_DXLDBS;


/*}
** Name: TPC_L1_DBCAPS - Catalog IIDBCAPABILITIES.
**
** Description:
**      This structure matches the schema of IIDBCAPABILITIES and is used for
**	accessing the catalog.
**      
** History:
**      12-oct-89 (carl)
**          created
*/

typedef struct _TPC_L1_DBCAPS
{
    DB_CAP_STR		l1_1_cap_cap;	/* capability */
    DB_CAPVAL_STR	l1_2_cap_val;	/* value of above */

#define	TPC_03_CC_DBCAPS_2	2	/* number of columns in catalog */
}   TPC_L1_DBCAPS;


/*}
** Name: TPC_I1_STARCDBS - Catalog IISTAR_CDBS.
**
** Description:
**      This structure matches the schema of IISTAR_CDBS and is used for
**	accessing the catalog.
**      
** History:
**      12-oct-89 (carl)
**          created
*/

typedef struct _TPC_I1_STARCDBS
{
    DB_DB_STR        i1_1_ddb_name;	/* DDB name */
    DB_OWN_STR       i1_2_ddb_owner;	/* DDB owner name */
    DB_DB_STR        i1_3_cdb_name;	/* CDB name */
    DB_OWN_STR       i1_4_cdb_owner;	/* CDB owner name */
    DB_NODE_STR      i1_5_cdb_node;	/* CDB node name */
    DB_TYPE_STR      i1_6_cdb_dbms;	/* DBMS for CDB */
    DB_TYPE_STR      i1_7_scheme_desc;	/* centralized, etc */
    DB_DATE_STR      i1_8_create_date;	/* DDB creation date */
    DB_TYPE_STR      i1_9_original;	/* "Y" or "N" */
					/* DB_TYPE_STR big enough for */
					/* iistar_cdbs.original char(8) */
    i4		     i1_10_cdb_id;	/* id of CDB */
    i4		     i1_11_cdb_cap;	/* encoded CDB caps */

#define	TPC_04_CC_STARCDBS_11	11	/* number of columns in catalog */

}   TPC_I1_STARCDBS;

