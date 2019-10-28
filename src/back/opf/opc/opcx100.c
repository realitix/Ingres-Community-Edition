/*
** Copyright (c) 2008, 2011 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <pc.h>
#include    <cs.h>
#include    <lk.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <gca.h>
#include    <cui.h>
#include    <cut.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <ade.h>
#include    <adp.h>
#include    <adfops.h>
#include    <adudate.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefqeu.h>
#include    <qefmain.h>
#include    <qefexch.h>
#include    <ex.h>
#include    <bt.h>
#include    <me.h>
#include    <st.h>
#include    <x100.h>
#include    <uld.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>
#include    <opckey.h>
#include    <opcd.h>
#define        OPC_SORTS	TRUE
#include    <opxlint.h>
#include    <opclint.h>

/**
**
**  Name: OPCX100.C - Routines to create QEF_X100_QRY action headers.
**
**  Description:
**      This file holds all of the routines that are specific to creating 
**      X100 action headers. That is, it contains the functions that translate
**	a QEP into the equivalent X100 algebra. Many of the functions were
**	cloned from equivalent parse tree node handlers in the Star text
**	regeneration modules, though obviously heavily modified to emit 
**	X100 algebra.
**
**	Externally visable routines:
**          opc_x100ahd_build()	- build a QEF_X100_QRY action header.
**
**	Internal only routines:
**          numerous that will be known as they are coded.
**
**  History:
**      9-july-2008 (dougi)
**	    Written for initial support of X100 engine.
**	24-Apr-2009 (kiria01)
**	    Fixed missing tid on Project for updates & corrected
**	    trim logic to handle single character names. Switch
**	    from TScan to MScan now that it's available
**	05-Aug-2009 (kiria01)
**	    Separate ADF context to allow fixed settings for X100.
**	11-may-2010 (dougi)
**	    Changes to support of "is [not] integer/float/decimal".
**	28-Oct-2010 (kschendel)
**	    Changes for long names trimmed in RDF.
**      03-feb-2011 (stial01)
**          Change x100_resrow_format i4->i8 (for tuple counts)
**	16-Mar-2011 (kschendel) m1541
**	    Changes to support repeated parameterized queries:  the x100
**	    action text becomes a prototype algebra text, and we generate
**	    a list of positions where query actual-parameters need to be
**	    substituted at execution time.
**	    Simplify text list slightly, DD_PACKET is enough, don't need
**	    the qeq-txt-seg header.
**	11-May-2011 (stial01) m1545
**	    Add X100_UNSUPP functions
**	23-May-2011 (stial01) m1545
**	    Added more X100_UNSUPP functions
**	08-Jun-2011 (kiria01) b125435
**	    Chain OPERANDS on right hand side.
**	20-Jun-2011 (michal) m2082
**	    Removed first (fake) parameter to x100 rowid() function.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**	15-Jul-2011 (stial01) m2242
**	    Increase sizeof corrname (for expansion by x100_escape_id)
**/

/*}
** Name: X100_STATE - state of constructing X100 algebra from an Ingres QEP.
**
** Description:
**	Local control block to pass bits and pieces used in the construction
**	of X100 algebra from an Ingres QEP. Simplifies calling sequences.
**
*/
typedef struct _X100_STATE
{
    OPS_STATE	*global;
    OPS_SUBQUERY *subquery;
    OPS_SUBQUERY *saggsubq;
    ADF_CB	adfcb;			/* for calling ADF functions */
    DD_PACKET	*seglist;		/* list of X100 text packets */
    ULD_TSTATE	*textcb;		/* temp text structure */
    QEF_X100_PARAM_INSERT *params;	/* Actual-parameter insert points */
    i4		numparams;		/* How many insert points used */
    i4		maxparams;		/* Total size of insert-point array */
    i4		noCOsaggs;
    DB_STATUS	retstatus;
    DB_ERROR	error;
    i4		querylen;		/* length of query (so far) */
    i4		flag;			/* various flags */
#define X100_DELETE	0x01		/* Delete() statement */
#define	X100_MODIFY	0x02		/* Modify() (update) statement */
#define	X100_APPEND	0x04
#define	X100_DMLUPDEL	(X100_DELETE | X100_MODIFY)
#define	X100_DMLOP	(X100_DELETE | X100_MODIFY | X100_APPEND)
#define	X100_TIDDONE	0x08		/* already emitted "tid" projection */
#define X100_REUSE	0x10		/* query has a REUSE */
					/* maybe set only when agg dist? */
#define X100_UNION	0x20		/* we're under a UNION */
} X100_STATE;

typedef struct _X100_OPNAMES
{
    char	*opname;		/* X100 operator name */
    ADI_OP_ID	opid;			/* Ingres opcode */
    i4		op_flag;		/* operator treatment flag */
#define	X100_ADD_OR_CONCAT	0x00000001	/* + or concat dep on types */
#define X100_ADD_A_NOT		0x00000002	/* wrap not() around op */
#define	X100_IIIFTRUE		0x00000008	/* ii+iftrue(x, y) - only emit
						** y - it'll be ok */
#define X100_NOOP_UOP		0x00000010	/* uop(x) -> just x for unorm */
#define	X100_UNSUPP		0x00000020	/* func/op unsupported in VW */
#define	X100_RANK		0x00000040	/* ranking function that might
						** just have 1 parm */
#define	X100_ROWID		0x00000080	/* rowid with uidx parm */
} X100_OPNAMES;

static const char tid32[] = "_tid__";

static X100_OPNAMES x100_ops[] = {
	"+", ADI_ADD_OP, X100_ADD_OR_CONCAT,
	"extract", ADI_DPART_OP, 0,
	"extract", ADI_DPART_DECIMAL_OP, 0,
	"!=", ADI_NE_OP, 0,
	"==", ADI_EQ_OP, 0,
	"like", ADI_NLIKE_OP, X100_ADD_A_NOT,
#ifndef ADF_STRICT_ANSI_SUBSTRING
	"ingsubstr", ADI_SUBSTRING_OP, 0,
#endif /*ADF_STRICT_ANSI_SUBSTRING*/
	"isnull", ADI_ISNUL_OP, 0,
	"isnull", ADI_NONUL_OP, X100_ADD_A_NOT,
	"dbl", ADI_F8_OP, 0,
	"flt", ADI_F4_OP, 0,
	"slng", ADI_I8_OP, 0,
	"sint", ADI_I4_OP, 0,
	"ssht", ADI_I2_OP, 0,
	"schr", ADI_I1_OP, 0,
	"date", ADI_ADTE_OP, 0,
	"money", ADI_MONEY_OP, 0,
	"hash32", ADI_HASH_OP, 0,
	"str", ADI_VARCH_OP, 0,
	"str", ADI_NVCHAR_OP, 0,
	"intervalds", ADI_INDS_OP, 0,
	"intervalym", ADI_INYM_OP, 0,
	"str", ADI_ASCII_OP, 0,
	"str", ADI_TEXT_OP, 0,
	"str", ADI_CHAR_OP, 0,
	"str", ADI_NCHAR_OP, 0,
	"rtrim", ADI_TRIM_OP, 0,
	"pow", ADI_POW_OP, 0,
	"log", ADI_LOG_OP, 0,
	"minval", ADI_LESSER_OP, 0,
	"maxval", ADI_GREATER_OP, 0,
	"minval_nonull", ADI_LEAST_OP, 0,
	"maxval_nonull", ADI_GREATEST_OP, 0,
	"isinteger", ADI_ISINT_OP, 0,
	"isfloat", ADI_ISFLT_OP, 0,
	"isdecimal", ADI_ISDEC_OP, 0,
	"isinteger", ADI_NOINT_OP, X100_ADD_A_NOT,
	"isfloat", ADI_NOFLT_OP, X100_ADD_A_NOT,
	"isdecimal", ADI_NODEC_OP, X100_ADD_A_NOT,
	"max", ADI_SINGLETON_OP, 0,
	"", ADI_SINGLECHK_OP, 0,
	"", ADI_IFTRUE_OP, X100_IIIFTRUE,
	"", ADI_UNORM_OP, X100_NOOP_UOP,
	"", ADI_ISTRUE_OP, X100_UNSUPP,
	"", ADI_NOTRUE_OP, X100_UNSUPP,
	"", ADI_ISFALSE_OP, X100_UNSUPP,
	"", ADI_NOFALSE_OP, X100_UNSUPP,
	"", ADI_ISUNKN_OP, X100_UNSUPP,
	"", ADI_NOUNKN_OP, X100_UNSUPP,
	"", ADI_BOO_OP, X100_UNSUPP,
	"", ADI_BYTE_OP, X100_UNSUPP,
	"", ADI_VBYTE_OP, X100_UNSUPP,
	"", ADI_LBYTE_OP, X100_UNSUPP,
	"", ADI_LVARCH_OP, X100_UNSUPP,
	"", ADI_LNVCHR_OP, X100_UNSUPP,
	"", ADI_LOGKEY_OP, X100_UNSUPP,
	"", ADI_TABKEY_OP, X100_UNSUPP,
	"", ADI_UUID_TO_CHAR_OP, X100_UNSUPP,
	"", ADI_UUID_FROM_CHAR_OP, X100_UNSUPP,
	"", ADI_UUID_COMPARE_OP, X100_UNSUPP,
	"", ADI_COWGT_OP, X100_UNSUPP,
	"", ADI_IPADDR_OP, X100_UNSUPP,
	"", ADI_UNHEX_OP, X100_UNSUPP,
	"", ADI_RAND_OP, X100_UNSUPP,
	"", ADI_SRAND_OP, X100_UNSUPP,
	"sqlrank", ADI_RANK_OP, X100_RANK,
	"sqldenserank", ADI_DRANK_OP, X100_RANK,
	"sqlrownumber", ADI_ROWNUM_OP, 0,
	"diff", ADI_VWDIFF_OP, 0,
	"rediff", ADI_VWREDIFF_OP, 0,
	"rownum", ADI_VWROWNUM_OP, 0,
	"rowid", ADI_VWROWID_OP, X100_ROWID,
	"sqlntile", ADI_NTILE_OP, 0,
	NULL, -1, 0};

static DB_DT_ID unsuppdts[] = {
	DB_TXT_TYPE,
	DB_CHR_TYPE,
	DB_LOGKEY_TYPE,
	DB_TABKEY_TYPE,
	-1};

static DB_DATA_VALUE	dbdvint8 = {NULL, sizeof(i8), DB_INT_TYPE, 0, -1};
static DB_DATA_VALUE	dbdvflt8 = {NULL, sizeof(f8), DB_FLT_TYPE, 0, -1};
i4 Opf_x100_debug = 0;


/* TABLE OF CONTENTS */
static void opc_x100_textcopy(
	X100_STATE *x100cb,
	QEF_AHD *x100_action);
static void opc_x100_addseg(
	X100_STATE *x100cb,
	const char *stringval,
	i4 stringlen);
static void opc_x100_addparam(
	X100_STATE *x100cb,
	i4 param_no,
	DB_DT_ID nulltype);
static void opc_x100_flush(
	X100_STATE *x100cb);
static void opc_x100_printf(
	X100_STATE *x100cb,
	const char *stringp);
static i4 opc_x100_opinfo(
	ADF_CB *adfcb,
	ADI_OP_ID opid,
	ADI_OPINFO *opinfo,
	i4 *x100_opflag);
static bool opc_x100_unsupptype(
	DB_DT_ID checktype);
static void opc_x100_prconst(
	X100_STATE *x100cb,
	PST_QNODE *constp,
	DB_DATA_VALUE *restype,
	ADI_OP_ID active_op);
static void opc_x100_uop(
	X100_STATE *x100cb,
	OPO_CO *cop,
	PST_QNODE *qnode,
	DB_DATA_VALUE *restype);
static void opc_x100_inlist(
	X100_STATE *x100cb,
	OPO_CO *cop,
	PST_QNODE *qnode,
	PST_QNODE *constp,
	DB_DATA_VALUE *restype,
	bool top);
static void opc_x100_notinlist(
	X100_STATE *x100cb,
	OPO_CO *cop,
	PST_QNODE *qnode,
	PST_QNODE *constp,
	DB_DATA_VALUE *restype,
	bool top);
static void opc_x100_bop(
	X100_STATE *x100cb,
	OPO_CO *cop,
	PST_QNODE *qnode,
	DB_DATA_VALUE *restype);
static void opc_x100_mop(
	X100_STATE *x100cb,
	OPO_CO *cop,
	PST_QNODE *qnode,
	DB_DATA_VALUE *restype);
static void opc_x100_prexpr(
	X100_STATE *x100cb,
	OPO_CO *cop,
	PST_QNODE *qnode,
	DB_DATA_VALUE *restype,
	ADI_FI_ID coerce_fiid);
static void opc_x100_prepcorr(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	OPV_GRV *gvarp,
	OPV_VARS *varp,
	OPZ_ATTS *attrp);
static bool opc_x100_aoptest(
	PST_QNODE *nodep);
static void opc_x100_addVARs(
	OPS_STATE *global,
	PST_QNODE **projlist,
	PST_QNODE *nodep,
	PST_QNODE *prsdmp);
static void opc_x100_bfemit(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	i4 pcnt,
	OPL_IOUTER ojid,
	bool konst,
	bool quals);
static bool opc_x100_FAcheck(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *eqcmap,
	OPV_IVARS varno);
static void opc_x100_reuse_recurse(
	OPS_SUBQUERY *subquery);
static void opc_x100_reuseck(
	OPS_SUBQUERY *subquery,
	OPO_CO *inner);
static void opc_x100_ixcollapse(
	OPS_SUBQUERY *subquery,
	OPE_BMEQCLS *jeqcmap);
static void opc_x100_presdom(
	X100_STATE *x100cb,
	OPO_CO *cop,
	PST_QNODE *qnode,
	bool printname,
	bool reorder,
	bool printall,
	bool aggr);
static void opc_x100_dupselimck(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	bool *deabove);
static void opc_x100_mergenkey(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
static void opc_x100_ifnullc(
	X100_STATE *x100cb,
	DB_DT_ID dtype);
static void opc_x100_fdlist(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	OPO_CO *cop);
static void opc_x100_derckeqc(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPE_BMEQCLS *eqcmap,
	OPE_BMEQCLS *dermapp);
static void opc_x100_colmap(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPO_CO *jcop,
	OPE_BMEQCLS *eqcmap,
	bool joinkeys,
	bool njproj,
	bool draggr);
static void opc_x100_collist(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPE_IEQCLS eqcno,
	bool joinkeys,
	bool njproj);
static bool opc_x100_colemit(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPO_CO *jcop,
	OPE_IEQCLS eqcno,
	bool *not1st,
	bool joinkeys,
	bool njproj,
	bool draggr,
	OPE_BMEQCLS *dermapp);
static bool opc_x100_varsearch(
	PST_QNODE *nodep,
	OPZ_IATTS attrno,
	OPV_IVARS varno);
static void opc_x100_co(
	X100_STATE *x100cb,
	OPO_CO *cop,
	bool undermerge,
	bool undersemi);
static void opc_x100_valid(
	OPS_STATE *global,
	OPV_GRV *grv,
	bool isResult);
static void opc_x100_projaggr(
	OPS_SUBQUERY *subquery,
	X100_STATE *x100cb);
static PST_QNODE * opc_x100_winfunc_rewrite(
	OPS_SUBQUERY *subquery,
	X100_STATE *x100cb,
	PST_QNODE ***oldrsdmpp,
	i4 winix,
	bool noemit);
static void opc_x100_winsort(
	OPS_SUBQUERY *subquery,
	X100_STATE *x100cb,
	i4 winix,
	bool partonly);
static PST_QNODE * opc_x100_winsort_expr(
	OPS_SUBQUERY *subquery);
static bool opc_x100_claggck(
	OPS_SUBQUERY *subquery,
	PST_QNODE *listp);
void opc_x100_derck(
	OPS_SUBQUERY *subquery,
	PST_QNODE *bylistp,
	bool drop_const);
static bool opc_x100_orddeck(
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	bool *dupselim);
static bool opc_x100_aggdistck(
	PST_QNODE *nodep,
	PST_QNODE **distvarpp);
static void opc_x100_bfsaggcheck(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	OPO_CO *cop,
	OPS_SUBQUERY **saggsubq);
static void opc_x100_bfsaggfix(
	X100_STATE *x100cb,
	PST_QNODE **nodep,
	OPS_SUBQUERY **saggsubq,
	bool noCO);
static void opc_x100_bfsaggemit(
	X100_STATE *x100cb,
	OPS_SUBQUERY *saggsubq,
	bool noCO);
static void opc_x100_saggmain(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery,
	PST_QNODE **nodep);
static i4 opc_x100_prsaggemit(
	X100_STATE *x100cb,
	OPS_SUBQUERY *subquery);
static void opc_x100_union_saggck(
	OPS_SUBQUERY *subquery);
static void opc_x100_dmlop(
	OPS_SUBQUERY *subquery,
	X100_STATE *x100cb);
static void opc_x100_varpush(
	PST_QNODE *nodep,
	PST_QNODE *node1p);
static void opc_x100_namepush(
	OPS_SUBQUERY *subquery);
static VOID opc_x100_namepush_vt(
	OPS_SUBQUERY *subquery,
	char *rsname,
	OPZ_IATTS attno);

static void opc_x100_distnames(
	OPS_SUBQUERY *subquery);
static void opc_x100_subquery(
	OPS_SUBQUERY *subquery,
	OPS_SUBQUERY *prevsubq,
	X100_STATE *x100cb,
	char *vtname);
void opc_x100ahd_build(
	OPS_STATE *global,
	QEF_AHD **x100_action);
static ADI_FI_ID opc_x100_adj_coerce(
	X100_STATE *x100cb,
	PST_QNODE *qnode,
	i4 oprd,
	ADI_FI_ID coerce_fiid);
static const char *opc_x100_copy(
	const char *src,
	u_i4 srclen,
	char *dest,
	u_i4 destsize);

/*{
** Name: OPC_X100_TEXTCOPY	- copy accumulated X100 query text to 
**	action header
**
** Description:
**      Allocate string to contain X100 query and copy it into place
**	from the assembled packets.
**
** Inputs:
**      x100cb			Ptr to X100 conversion state structure
**	x100_action		Ptr to X100 action header
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**	14-july-2008 (dougi)
**	    Written for X100 support.
**	17-Mar-2011 (kschendel) m1541
**	    Copy parameter insert-point array too;  eliminate qeq-txt-seg
**	    header, wasn't useful.
*/
static VOID
opc_x100_textcopy(
	X100_STATE	*x100cb,
	QEF_AHD		*x100_action)

{
    OPS_STATE		*global = x100cb->global;
    DD_PACKET		*packetp;
    PTR			workp;
    i4			i;

    /* Allocate memory (if any), then loop over packets, doing the copy. */
    if ((x100_action->qhd_obj.qhd_x100Query.ahd_x100_textlen = i =
		x100cb->querylen) == 0)
	return;

    x100_action->qhd_obj.qhd_x100Query.ahd_x100_text = workp =
	(PTR) opu_qsfmem(global, i+1);

    for (packetp = x100cb->seglist; packetp; 
					packetp = packetp->dd_p3_nxt_p)
     {
	i -= packetp->dd_p1_len;
	if (i < 0)
	    return;		/* actually, an error */

	memcpy(workp, packetp->dd_p2_pkt_p, packetp->dd_p1_len);
	workp += packetp->dd_p1_len;
     }

    *workp = 0x0;		/* null terminate */

    /* If there are actual-parameter insert points, copy them into
    ** QP QSF memory too.
    */
    if (x100cb->numparams > 0)
    {
	i = x100cb->numparams * sizeof(QEF_X100_PARAM_INSERT);
	x100_action->qhd_obj.qhd_x100Query.ahd_x100_params =
	    (QEF_X100_PARAM_INSERT *) opu_qsfmem(global, i);
	memcpy(x100_action->qhd_obj.qhd_x100Query.ahd_x100_params, x100cb->params, i);
	x100_action->qhd_obj.qhd_x100Query.ahd_x100_numparams = x100cb->numparams;
    }

}

/*{
** Name: OPC_X100_ADDSEG	- Add a text segment to a X100 query
**
** Description:
**      Similar to opc_tsegstr, this routine will create a DD_PACKET
**	from a string and add it to a text segment list. 
**
** Inputs:
**      x100cb				X100 compilation state variable
**      stringval                       string to make segment from
**	stringlen			length of the string, or 0
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**	11-july-2008 (dougi)
**	    Cloned from opc_addseg() for X100 support.
**	16-Mar-2011 (kschendel) m1541
**	    Simplify slightly, don't need the qeq-txt-seg header.
*/
static VOID
opc_x100_addseg(
	X100_STATE	*x100cb,
	const char	*stringval,
	i4		stringlen)

{
    OPS_STATE	   	*global = x100cb->global;
    DD_PACKET		**seglist = &x100cb->seglist;
    DD_PACKET		*pktptr, *newpkt;
    i4                  size;

    newpkt = (DD_PACKET *) opu_memory( global, sizeof(DD_PACKET) + stringlen);

    if (*seglist == NULL )
    {
	*seglist = newpkt;
    }
    else
    {
	pktptr = (*seglist);
	while (pktptr->dd_p3_nxt_p != NULL)
	    pktptr = pktptr->dd_p3_nxt_p;
	pktptr->dd_p3_nxt_p = newpkt;
    }

    newpkt->dd_p1_len = stringlen;
    newpkt->dd_p2_pkt_p = (char *) (((PTR) newpkt) + sizeof(DD_PACKET));
    newpkt->dd_p3_nxt_p = NULL;
    newpkt->dd_p4_slot = DD_NIL_SLOT;
    memcpy(newpkt->dd_p2_pkt_p, stringval, stringlen);
    return;
}

/*
** Name: opc_x100_addparam - Add runtime parameter text insert-point
**
** Description:
**	If a runtime parameter is needed (for a cached/repeated parameterized
**	query plan), rather than inserting the parameter value which
**	obviously may change from execution to execution, we'll record
**	where in the X100 algebra text the parameter should be inserted.
**	The parameter is identified by number.
**
**	An array of insert-points was chosen rather than using marker
**	substitution, because x100 algebra text may have arbitrary
**	string constants.  I don't want to try to pick a marker sequence
**	by just hoping that it won't occur in some evil QA person's text
**	string.
**
** Inputs:
**	x100cb			Algebra-generator state structure
**	param_no		The parameter number from the PST_CONST
**	nulltype		Type to use if NULL -- may not be relevant
**				for repeat params, since we should know the
**				type, but be safe/consistent.
**
** Outputs:
**	No return.
**	Generates a new entry into the QEF_X100_PARAM_INSERT array.
**
** History:
**	17-Mar-2011 (kschendel) m1541
**	    Support for parameterized repeated queries.
*/

static void
opc_x100_addparam(X100_STATE *x100cb, i4 param_no, DB_DT_ID nulltype)
{
    QEF_X100_PARAM_INSERT *p, *newp;

    if (x100cb->numparams >= x100cb->maxparams)
    {
	i4 oldsize = x100cb->maxparams;

	/* Need to expand the list */
	p = x100cb->params;
	x100cb->maxparams += x100cb->global->ops_qheader->pst_numparm;
	newp = (QEF_X100_PARAM_INSERT *) opu_memory(x100cb->global,
		x100cb->maxparams * sizeof(QEF_X100_PARAM_INSERT));
	if (p != NULL)
	    memcpy(newp, p, oldsize * sizeof(QEF_X100_PARAM_INSERT));
	x100cb->params = newp;
    }
    p = &x100cb->params[x100cb->numparams++];
    p->offset = x100cb->querylen - 1;	/* Luckily this is always up-to-date */
    p->param_no = param_no;
    p->nulltype = nulltype;
    return;

} /* opc_x100_addparam */

/*{
** Name: opc_x100_flush	- flush characters from temporary into qry text list
**
** Description:
**      Move any remaining characters out of the temp buffer and into
**      the list of query text segments for distributed compiled queries.
**
** Inputs:
**	x100cb				Global state structure
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-july-2008 (dougi)
**	    Cloned from opc_flshb() for X100 support.
**	16-Mar-2011 (kschendel) m1541
**	    It appears we don't need the second arg, drop it.
*/
static VOID
opc_x100_flush(
	X100_STATE	*x100cb)
{
    ULD_TSTATE	*tempbuf = x100cb->textcb;
    /* 
    **  Write strings from the temporary buffer
    **  out into text segments first
    */

    if ( (tempbuf != NULL) && (tempbuf->uld_offset > 0 ))
    {
	opc_x100_addseg( x100cb, &tempbuf->uld_tempbuf[0],
	    tempbuf->uld_offset);

	tempbuf->uld_offset = 0;	    /* reset buffer to be empty */
	tempbuf->uld_remaining = ULD_TSIZE - 1;	/* space for null terminator */
    }

    return;
}

/*{
** Name: OPC_X100_PRINTF	- add string to X100 query buffer.
**
** Description:
**      This routine will move a null terminated string into either the temporary
**	string buffer, or, when that buffer is full, into the list of query text
**	segments.
**	
**
** Inputs:
**	stringp				ptr to the null-terminated string to be added
**	bufstruct			ptr to the temporary buffer structure
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-july-2008 (dougi)
**	    Cloned from opc_prbuf/opcu_printf for X100 support.
*/
static VOID
opc_x100_printf(
	X100_STATE	*x100cb,
	const char	*stringp)
{
    OPS_STATE		*global = x100cb->global;
    i4			len;

    if (stringp[0] && !stringp[1] && x100cb->textcb->uld_remaining)
    {	/* special case single character write */
	x100cb->textcb->uld_tempbuf[x100cb->textcb->uld_offset++]=stringp[0];
	x100cb->textcb->uld_remaining--;
	x100cb->querylen++;
	return;
    }

    len = STlength(stringp);
    x100cb->querylen += len;

    while (len > 0)
    {
	if (x100cb->textcb->uld_remaining >= len)
	{
	    /*  The string will fit completely in the temporary buffer */

	    MEcopy((PTR)stringp, len, 
		(PTR)&x100cb->textcb->uld_tempbuf[x100cb->textcb->uld_offset]);
	    x100cb->textcb->uld_offset += len;
	    x100cb->textcb->uld_remaining -= len;
	    break;
	}
	else
	{   
	    /* The string will only partially fit in the temporary, 
	    ** so create a text segment and put it there. */

	    i4		length_to_move;
	    const char	*source;

	    if (x100cb->textcb->uld_offset <= 0)
	    {
		/* Need to copy directly to allocated buffer since size 
                ** of string is larger than temporary buffer. */
		length_to_move = len;
		source = stringp;
		len = 0;
	    }
	    else
	    {
		/* Move from temporary buffer. */
		length_to_move = x100cb->textcb->uld_offset;
		source = &x100cb->textcb->uld_tempbuf[0];
	    }

	    opc_x100_addseg(x100cb, source, length_to_move);

	    x100cb->textcb->uld_tstring = NULL;
	    x100cb->textcb->uld_offset = 0;	    /* set buffer to be empty */
	    x100cb->textcb->uld_remaining = ULD_TSIZE - 1; /* space for null */
	}
    }
}

/*
** {
** Name: OPC_X100_OPINFO	- call adi_op_info() and check for X100
**	overrides
**
** Description:
**      This routine will call adi_op_info() to retrieve info for a
**	ADF opid. Then it will check a local array for any X100 function
**	name overrides and apply them
**
** Inputs:
**      x100cb                          global state variable
**      constp                          ptr to constant node
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-november-2008 (dougi)
**	    Written.
*/
static DB_STATUS
opc_x100_opinfo(
	ADF_CB		*adfcb,
	ADI_OP_ID	opid,
	ADI_OPINFO	*opinfo,
	i4		*x100_opflag)

{
    DB_STATUS	status;
    i4		i;


    /* Just call ADF, then loop over the local array. */
    /* Note: the status check is deliberately delayed because some 
    ** functions are internal to VW and only the cross compiler knows
    ** about them (diff, rediff, rownum). So the array lookup is done
    ** before status is checked. */
    status = adi_op_info(adfcb, opid, opinfo);

    for (i = 0; x100_ops[i].opid >= 0; i++)
     if (opid == x100_ops[i].opid)
     {
	*x100_opflag = x100_ops[i].op_flag;
	if ((*x100_opflag) & X100_UNSUPP)
	    opx_1perror(E_OP08C8_VWOP_UNSUPP, (char *)&opinfo->adi_opname);

	if (status != E_DB_OK)
	    memset(opinfo, 0, sizeof(ADI_OPINFO));	/* for safety */
	STcopy(x100_ops[i].opname, (char *)&opinfo->adi_opname);
	return(E_DB_OK);
     }

    if (status != E_DB_OK)
	return(status);

    *x100_opflag = 0;
    return(E_DB_OK);
}

/*
** {
** Name: OPC_X100_UNSUPPTYPE	- func to identify unsupported Ingres types
**
** Description:
**      This routine simply loops over a static array of native Ingres
**	data types not supported by IVW. No attempt is then made to coerce
**	to them.`
**
** Inputs:
**      checktype			data type to check
**
** Outputs:
**	Returns:
**	    TRUE	if type is unsupported
**	    FALSE	otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-aug-2010 (dougi)
**	    Written.
*/
static bool
opc_x100_unsupptype(
	DB_DT_ID	checktype)

{
    DB_DT_ID	*unsuppelem = &unsuppdts[0];

    /* Trivially simple - loop over "unsuppdts[]" array looking for match. If
    ** one found, return TRUE, else FALSE. */
    for (;*unsuppelem > 0; unsuppelem++)
     if (checktype == *unsuppelem)
	return(TRUE);

    return(FALSE);

}

/*
** {
** Name: OPC_X100_PRCONST	- convert constant to text
**
** Description:
**      This routine will convert the internal representation of a
**      constant to a text form and insert it into the query text
**      string
**
** Inputs:
**      x100cb                          global state variable
**      constp                          ptr to constant node
**	active_op			Already output cast.
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    places constant text form into query text string
**
** History:
**	11-july-2008 (dougi)
**	    Cloned from opc_prconst() for X100 support.
**	04-Aug-2009 (kiria01)
**	    Make trim datatype specific.
**	05-Aug-2009 (kiria01)
**	    Escape single quotes in data
**	13-Aug-2009 (kiria01)
**	    Provide the BOOL support for constants
**	09-Feb-2010 (kiria01)
**	    Add temporary code for the missing time/timestamp
**	    constants that can appear when COP nodes have been folded.
**	22-apr-2010 (dougi)
**	    Set result string type to -DB_UTF8_TYPE if it's supposed to be
**	    nullable.
**	3-may-2010 (dougi)
**	    In the odd event that an Ingres date constant is presented, convert
**	    it first to ANSI date before generating X100 literal.
**	20-may-2010 (dougi)
**	    Tidy up conversion from Ingres to ANSI date.
**	6-july-2010 (dougi)
**	    Add support of type-specific NULL constants.
**	16-Sep-2010 (kiria01)
**	    Correct NULL bit handling - ADF_NVL_BIT should be checked.
**	27-jan-2011 (dougi)
**	    Slight fix for nulls, then recode constant for null dates, times
**	    & timestamps.
**	02-Mar-2011 (kiria01) m1677
**	    Sometimes the default width is too small - if this proved a problem
**	    use the bigger value.
**	09-Mar-2011 (kiria01) m1677pt2
**	    Use full worst width for result length.
**	17-Mar-2011 (kschendel) m1541
**	    Support for repeat-query parameters.  Extract bulk of constant
**	    printing (to an adt routine) so that QEF can use it too.
*/

/* First, a tiny wrapper for opc_x100_printf to keep C typing happy */
static void
opc_x100_printit(void *x100cb, const char *str)
{
    opc_x100_printf((X100_STATE *) x100cb, str);
}


static VOID
opc_x100_prconst(
	X100_STATE	*x100cb,
	PST_QNODE	*constp,
	DB_DATA_VALUE	*restype,
	ADI_OP_ID	active_op)
{
    OPS_STATE		*global = x100cb->global;
    OPS_SUBQUERY	*subquery = x100cb->subquery;
    OPV_GRV		**opvgrv_p;
    OPV_SEPARM		*separm;
    OPT_NAME		aname;		   
    OPT_NAME		*attrname = &aname;
    OPT_NAME		aliasname;
    DD_CAPS		*cap_ptr;
    i4			atno; 
    i4			vno; 
    i4			length;
    DB_DT_ID		nulltype;
    char		*att_name;
    int			i;
    bool		found = (bool)FALSE;

    for (opvgrv_p = global->ops_rangetab.opv_base->opv_grv, i = 0;
	    i < global->ops_rangetab.opv_gv; i++)
    {
	if (opvgrv_p[i] == (OPV_GRV *)NULL)  
		continue;				/* sparse array */
	if (!(opvgrv_p[i]->opv_gsubselect))
		continue;				/* not a subselect */
	
	for (separm = opvgrv_p[i]->opv_gsubselect->opv_parm;
		separm;
		separm = separm->opv_senext)
	{
	    if (constp == separm->opv_seconst)	/* this is our constant */
	    {
		found = TRUE;
		break;
	    }
	}

	if (found) break;
    }

    if (found)
    {
	/* simulate opc_x100_prvar with separm->opv_sevar */
	vno = separm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_vno;
	atno = separm->opv_sevar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id;
	/* construct alias name from global var num*/
	STprintf((PTR) &aliasname, "t%d", vno );  

	/* copy the attribute name to the output buffer */
	/* adapted from opc_atname */

	if ( sizeof(DB_ATT_NAME) >= sizeof(OPT_NAME))
		length = sizeof(OPT_NAME)-1;
	else length = sizeof(DB_ATT_NAME);

	att_name = opvgrv_p[vno]->opv_relation->rdr_attr[atno]->att_nmstr;
	length = opvgrv_p[vno]->opv_relation->rdr_attr[atno]->att_nmlen;

	{
	    u_i4	    unorm_len = sizeof(OPT_NAME) -1;
	    DB_STATUS	    status;
	    DB_ERROR	    error;

	    /* Unnormalize and delimit column name */
	    status = cui_idunorm( (u_char *)att_name, (u_i4*)&length,
				(u_char *)attrname, &unorm_len, CUI_ID_DLM, 
				&error);

	    if (DB_FAILURE_MACRO(status))
		    opx_verror(E_DB_ERROR, E_OP08A4_DELIMIT_FAIL, error.err_code);

	    attrname->buf[unorm_len]=0;
	}
	opc_x100_printf(x100cb, (char *) attrname );
	return;
    }

    /* Normal constant. */
    nulltype = DB_NODT;
    if (restype != NULL)
	nulltype = abs(restype->db_datatype);
    if (constp->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_RQPARAMNO
      && constp->pst_sym.pst_value.pst_s_cnst.pst_parm_no > 0)
    {	
	/* Repeat parameters are not actually printed, we just remember
	** where they go.
	** Note that SAGG results also look like parameters but they should
	** never get this far -- would be error if they did.
	*/
	if (constp->pst_sym.pst_value.pst_s_cnst.pst_parm_no >
		global->ops_qheader->pst_numparm)
	    opx_1perror(E_OP009D_MISC_INTERNAL_ERROR, "SAGG const reached opx_x100_prconst");
	opc_x100_addparam(x100cb,
		constp->pst_sym.pst_value.pst_s_cnst.pst_parm_no,
		nulltype);
    }
    else
    {
	DB_STATUS status;

	status = adt_x100_print(opc_x100_printit, x100cb, &x100cb->adfcb,
			&constp->pst_sym.pst_dataval, nulltype);
	if (status != E_DB_OK)
	    opx_verror(status, E_OP08C5_BAD_ADC_TMCVT, 
			    x100cb->adfcb.adf_errcb.ad_errcode);
    }
}

/*{
** Name: OPC_X100_UOP	- text generation for uop operator
**
** Description:
**      This routine will generate text for the PST_UOP operator.  Note
**	that since prexpr is highly recursive it is good to have non-recursive
**	routines which process the nodes, so that the stack usage for local
**	variables is not cumulative.
**
** Inputs:
**      x100cb                         text generation state variable
**      qnode                           PST_UOP parse tree node
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    places equivalent text in output buffer pointed at by x100cb
**
** History:
**      11-july-2008 (dougi)
**	    Cloned from opc_uop() for X100 support.
**	14-jan-2010 (dougi)
**	    Add support for "is integer", "is float", "is decimal".
**	15-jan-2010 (dougi)
**	    Add support for "quarter(<expr>)".
**	18-Mar-2011 (kschendel) m1541
**	    Parser-added unorm's aren't folded away for repeat query parameters
**	    where we can't treat the param as a constant.  Just turf out
**	    the unorm and emit the argument.  X100 doesn't understand or
**	    need the unorm anyway.
*/
static VOID
opc_x100_uop(
	X100_STATE	*x100cb,
	OPO_CO		*cop,
	PST_QNODE	*qnode,
	DB_DATA_VALUE	*restype)
{
    OPS_STATE		*global = x100cb->global;
    DB_STATUS	    status;
    ADI_OPINFO	    adi_info;
    i4			x100_opflag = 0, typecode;
    char	    op[10];

    status = opc_x100_opinfo(&x100cb->adfcb, 
	qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_info, &x100_opflag);
    if (DB_FAILURE_MACRO(status))
	opx_verror(status, E_OP08C7_ADI_OPNAME,
	    x100cb->adfcb.adf_errcb.ad_errcode);

    if (x100_opflag & X100_ADD_A_NOT)
	opc_x100_printf(x100cb, "! ( ");	/* wrap "! ()" around BOP */

    if ((x100_opflag & X100_NOOP_UOP) == 0)
    {
	/* All UOPs are prefix for X100. */
	opc_x100_printf(x100cb, (char *)&adi_info.adi_opname);

	opc_x100_printf(x100cb, "(");
    }
    if (x100_opflag & X100_RANK)
    {
	/* This is a ranking function with no partitioning. So it is
	** really a BOP and its 1st parm is "uidx('0')". */
	opc_x100_printf(x100cb, "uidx('0'), ");
    }
    if ((qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_ROWNUM_OP &&
		qnode->pst_left == NULL) || x100_opflag & X100_ROWID)
    {
	/* This is a row_number() function with no ordering. So its only
	** parm is "uidx('0')". */
	/* Or a rowid() function, which also uses uidx(0) as a parameter */
	opc_x100_printf(x100cb, "uidx('0'))");
	return;
    }

    /* The REAL parm of the UOP. */
    opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype,
	opc_x100_adj_coerce(x100cb, qnode, 0,
		qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid));

    if ((x100_opflag & X100_NOOP_UOP) == 0)
	opc_x100_printf(x100cb, ")");

    if (x100_opflag & X100_ADD_A_NOT)
	opc_x100_printf(x100cb, " )");	/* wrap "! ()" around BOP */
}

/*{
** Name: OPC_X100_[NOT]INLIST - text generation for [NOT]IN-list
**
** Description:
**      This routine will generate text for a "="/"!=" PST_BOP operator in which
**	the rhs is a packed list of PST_CONSTs indicating an [NOT]IN-list. It 
**	recursively turns them into a set of nested, comparisons.
**
** Inputs:
**      x100cb                         text generation state variable
**      qnode                           PST_BOP parse tree node
**	constp				ptr to CONST on rhs
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    places equivalent text in output buffer pointed at by x100cb
**
** History:
**	24-nov-2008 (dougi)
**	    Written to handle packed IN-lists in X100 queries.
**	04-Feb-2010 (kiria01)
**	    Add packed NOT IN list support.
*/
static VOID
opc_x100_inlist(
	X100_STATE	*x100cb,
	OPO_CO		*cop,
	PST_QNODE	*qnode,
	PST_QNODE	*constp,
	DB_DATA_VALUE	*restype,
	bool		top)
{


    /* Recurse until rhs CONST nodes are used up. Generate a || for each,
    ** then generate "==" operators on the way back out. */
    if (constp->pst_left)
    {
	opc_x100_printf(x100cb, "|| (");
	opc_x100_inlist(x100cb, cop, qnode, constp->pst_left, restype, FALSE);
    }

    /* Now emit a "==" for each constant. */
    opc_x100_printf(x100cb, "== (");
    opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype,
	opc_x100_adj_coerce(x100cb, qnode, 0,
		qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid));
    opc_x100_printf(x100cb, ",");
    opc_x100_prexpr(x100cb, cop, constp, restype,
	opc_x100_adj_coerce(x100cb, qnode, 1,
		qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid));
    if (constp->pst_left == (PST_QNODE *) NULL)
	opc_x100_printf(x100cb, "), ");		/* first "==" */
    else if (!top)
	opc_x100_printf(x100cb, ")), ");	/* next "=="s close "||", too */
    else opc_x100_printf(x100cb, ")) ");		/* end of ||'s */

}

static VOID
opc_x100_notinlist(
	X100_STATE	*x100cb,
	OPO_CO		*cop,
	PST_QNODE	*qnode,
	PST_QNODE	*constp,
	DB_DATA_VALUE	*restype,
	bool		top)
{


    /* Recurse until rhs CONST nodes are used up. Generate a && for each,
    ** then generate "!=" operators on the way back out. */
    if (constp->pst_left)
    {
	opc_x100_printf(x100cb, "&& (");
	opc_x100_notinlist(x100cb, cop, qnode, constp->pst_left, restype, FALSE);
    }

    /* Now emit a "!=" for each constant. */
    opc_x100_printf(x100cb, "!= (");
    opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype,
	opc_x100_adj_coerce(x100cb, qnode, 0,
		qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid));
    opc_x100_printf(x100cb, ",");
    opc_x100_prexpr(x100cb, cop, constp, restype,
	opc_x100_adj_coerce(x100cb, qnode, 1,
		qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid));
    if (constp->pst_left == (PST_QNODE *) NULL)
	opc_x100_printf(x100cb, "), ");		/* first "!=" */
    else if (!top)
	opc_x100_printf(x100cb, ")), ");	/* next "!="s close "&&", too */
    else opc_x100_printf(x100cb, ")) ");		/* end of &&'s */

}

/*{
** Name: OPC_X100_BOP	- text generation for bop operator
**
** Description:
**      This routine will generate text for the PST_BOP operator.  Note
**	that since prexpr is highly recursive it is good to have non-recursive
**	routines which process the nodes, so that the stack usage for local
**	variables is not cumulative.
**
** Inputs:
**      x100cb                         text generation state variable
**      qnode                           PST_BOP parse tree node
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    places equivalent text in output buffer pointed at by x100cb
**
** History:
**	11-july-2008 (dougi)
**	    Cloned from opc_bop() for X100 support.
**	30-Jul-2009 (kiria01)
**	    Extended LIKE support.
**	3-nov-2009 (dougi)
**	    Make SUBSTR start location 0-origin.
**	04-Feb-2010 (kiria01)
**	    Add packed NOT IN list support.
**	24-Feb-2010 (kiria01)
**	    Correct code generated for the negated pattern operators.
**	26-oct-2010 (dougi)
**	    Add exception handling for "ii_iftrue()" - no func. needed, just
**	    emit right operand.
*/
static VOID
opc_x100_bop(
	X100_STATE	*x100cb,
	OPO_CO		*cop,
	PST_QNODE	*qnode,
	DB_DATA_VALUE	*restype)
{
    OPS_STATE		*global = x100cb->global;
    DB_STATUS		status;
    ADI_OPINFO		adi_info;
    i4			i;
    i4			x100_opflag = 0;

    status = opc_x100_opinfo(&x100cb->adfcb, 
	qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_info, &x100_opflag);
    /* division used to calculate avarage must be marked separately as
    it returns different scale result on decimals */
    if (qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_DIV_OP &&
	(qnode->pst_sym.pst_value.pst_s_op.pst_flags & PST_XFORM_AVG) != 0)
    {
	STcopy("avgdiv", (char *)&adi_info.adi_opname);
    }

    if (DB_FAILURE_MACRO(status))
	opx_verror(status, E_OP08C7_ADI_OPNAME,
	    x100cb->adfcb.adf_errcb.ad_errcode);

    if (x100_opflag & X100_ADD_OR_CONCAT)
    {
	i2	lenspec = qnode->pst_sym.pst_value.pst_s_op.pst_fdesc->
						adi_lenspec.adi_lncompute;

	/* Use of lenspec to determine whether the "+" is for numbers or
	** strings may seem odd, but it involved the fewest comparisons
	** (i.e., it was the fastest). If this offends anyone, they may 
	** change this test to a pure data type check. */
	if (!(lenspec == ADI_FIXED || lenspec == ADI_SHORTER ||
		lenspec == ADI_DECADD))
	    STcopy("concat", (char *)&adi_info.adi_opname); 
    }

    if (x100_opflag & X100_ADD_A_NOT)
	opc_x100_printf(x100cb, "! ( ");	/* wrap "! ()" around BOP */

    /* If the BOP is eq or ne and PST_INLIST is set then the node has a packed
    ** list of PST_CONST node linked on the rhs, it is an IN-list that must be 
    ** decomposed into a series of ||'d ='s. */
    if ((qnode->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) != 0 &&
	qnode->pst_right && qnode->pst_right->pst_sym.pst_type == PST_CONST &&
	    qnode->pst_right->pst_left)
    {
	if (qnode->pst_sym.pst_value.pst_s_op.pst_opno ==
		global->ops_cb->ops_server->opg_eq)
	   opc_x100_inlist(x100cb, cop, qnode, qnode->pst_right,
		&qnode->pst_sym.pst_dataval, TRUE);

	else if (qnode->pst_sym.pst_value.pst_s_op.pst_opno ==
		global->ops_cb->ops_server->opg_ne)
	   opc_x100_notinlist(x100cb, cop, qnode, qnode->pst_right,
		&qnode->pst_sym.pst_dataval, TRUE);
	else
	    opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,"Unexpected IN list pack");
	return;
    }

    if (qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_DEC_OP)
    {
	PST_QNODE	    *newqnode;
	DB_DATA_VALUE   *dataval;
	char		tmp[50];
	u_i2		prec_scale;    
	opc_x100_printf(x100cb, (char *)&adi_info.adi_opname);
	opc_x100_printf(x100cb, "(");
	opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype,
		opc_x100_adj_coerce(x100cb, qnode, 0,
			qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid));
	/* Pull P & S apart and output */
	prec_scale = *(u_i2*)qnode->pst_right->pst_sym.pst_dataval.db_data;
	STprintf(tmp, ",sint('%d'), sint('%d')",
			DB_P_DECODE_MACRO(prec_scale),
			DB_S_DECODE_MACRO(prec_scale));
	opc_x100_printf(x100cb, tmp);
	opc_x100_printf(x100cb, ")");
    }
    else if ((qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_LIKE_OP ||
		qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_NLIKE_OP) &&
		qnode->pst_sym.pst_value.pst_s_op.pst_pat_flags != AD_PAT_DOESNT_APPLY &&
		qnode->pst_sym.pst_value.pst_s_op.pst_pat_flags != AD_PAT_NO_ESCAPE)
    {
	u_i2 flags = qnode->pst_sym.pst_value.pst_s_op.pst_pat_flags;
	u_i4 typ = (flags & AD_PAT_FORM_MASK);
	static const char likes[][16] = {
#define _DEFINE(n,v,s) {s},
#define _DEFINEEND
		AD_PAT_FORMS
#undef _DEFINEEND
#undef _DEFINE
        };
	if (typ >= AD_PAT_FORM_MAX)
	    typ = AD_PAT_FORM_LIKE;
	opc_x100_printf(x100cb, likes[typ>>8]);
	opc_x100_printf(x100cb, "(");
	opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype,
		opc_x100_adj_coerce(x100cb, qnode, 0,
			qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid));
	opc_x100_printf(x100cb, ",");
	opc_x100_prexpr(x100cb, cop, qnode->pst_right, restype,
		opc_x100_adj_coerce(x100cb, qnode, 1,
			qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid));
	if ((flags & AD_PAT_ISESCAPE_MASK) == AD_PAT_HAS_ESCAPE)
	{
	    char esc[5], *e = esc;
	    opc_x100_printf(x100cb, ", str('");
	    *e++ = (char)qnode->pst_sym.pst_value.pst_s_op.pst_escape;
	    if (esc[0] == '\\' || esc[0] == '\'')
		*e++ = '\\';
	    *e = '\0';
	    opc_x100_printf(x100cb, esc);
	    opc_x100_printf(x100cb, "')");
	}
	opc_x100_printf(x100cb, ")");
    }
    else if (x100_opflag & X100_IIIFTRUE)
    {
	/* "ii_iftrue(a, b)" - we only emit "b". */
	opc_x100_prexpr(x100cb, cop, qnode->pst_right, restype,
		opc_x100_adj_coerce(x100cb, qnode, 1,
			qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid));
    }
    else
    {
	opc_x100_printf(x100cb, (char *)&adi_info.adi_opname);
	opc_x100_printf(x100cb, "(");
	opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype,
		opc_x100_adj_coerce(x100cb, qnode, 0,
			qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid));
	opc_x100_printf(x100cb, ",");

	opc_x100_prexpr(x100cb, cop, qnode->pst_right, restype,
		opc_x100_adj_coerce(x100cb, qnode, 1,
			qnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid));

	opc_x100_printf(x100cb, ")");
    }

    if (x100_opflag & X100_ADD_A_NOT)
	opc_x100_printf(x100cb, ") ");	/* close the "! ()" */
}


/*{
** Name: OPC_X100_MOP	- text generation for mop operator
**
** Description:
**      This routine will generate text for the PST_MOP operator.
**	This node represents n-ary operator or function parameters. Note
**	that since prexpr is highly recursive it is good to have non-recursive
**	routines which process the nodes, so that the stack usage for local
**	variables is not cumulative.
**
** Inputs:
**      x100cb                         text generation state variable
**      qnode                           PST_MOP parse tree node
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    places equivalent text in output buffer pointed at by x100cb
**
** History:
**	11-july-2008 (dougi)
**	    Cloned fropm opc_mop() for X100 support.
*/
static VOID
opc_x100_mop(
	X100_STATE	*x100cb,
	OPO_CO		*cop,
	PST_QNODE	*qnode,
	DB_DATA_VALUE	*restype)
{
    OPS_STATE	    *global = x100cb->global;
    DB_STATUS       status;
    ADI_OPINFO      adi_info;
    i4              i = 1;
    i4              opno = qnode->pst_sym.pst_value.pst_s_op.pst_opno;
    i4		    x100_opflag = 0;
    PST_QNODE	    *qnodehead = qnode;

    /* Determine the characteristics of the operator/function */
    status = opc_x100_opinfo(&x100cb->adfcb, opno, &adi_info, &x100_opflag);
    if (DB_FAILURE_MACRO(status))
        opx_verror(status, E_OP08C7_ADI_OPNAME,
                     x100cb->adfcb.adf_errcb.ad_errcode);

    /* For traditional functions emit name */
    if (adi_info.adi_use == ADI_PREFIX || (global->ops_gmask & OPS_X100))
	opc_x100_printf(x100cb, (char *)&adi_info.adi_opname);

    opc_x100_printf(x100cb, "(");

    /* Output initial parameter */
    opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype,
		opc_x100_adj_coerce(x100cb, qnode, 1,
			qnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid));

    while(qnode = qnode->pst_right)
    {
        i++;
        if (adi_info.adi_use == ADI_INFIX)
        {
             /* Surround infix operators with blanks for such as "and" */
            opc_x100_printf(x100cb, " ");
            opc_x100_printf(x100cb, (char *)&adi_info.adi_opname);
            opc_x100_printf(x100cb, " ");

        }
        else
        {
            /* The more usual comma separator */
            opc_x100_printf(x100cb, ",");
        }
        /* Output next parameter - qnode will be PST_OPERAND */
        opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype,
		opc_x100_adj_coerce(x100cb, qnodehead, 1,
			qnodehead->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid));
    }
    opc_x100_printf(x100cb, ")");

    if (adi_info.adi_use == ADI_POSTFIX) 
	opc_x100_printf(x100cb, (char *)&adi_info.adi_opname);
}

/*{
** Name: OPC_X100_PREXPR	- convert a query tree expression for X100
**
** Description:
**      Accepts the following query tree nodes 
**
**	expr :		PST_VAR
**		|	expr BOP expr
**		|	expr Uop
**		|	AOP AGHEAD qual
**			  \
**			   expr
**		|	BYHEAD AGHEAD qual
**			/    \
**		tl_clause     AOP
**				\
**				 expr
**		|	PST_CONST
**
**
** Inputs:
**      x100cb				ptr to X100 text state variable
**      qnode                           query tree node to convert
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-july-2008 (dougi)
**	    Cloned from opcu_prexpr() for X100 support.
**	11-Sep-2009 (kiria01)
**	    Add explicit cast if required.
**	25-sep-2009 (dougi)
**	    Look for QLEND on right of an AND, as well as left. Can happen
**	    with extraction of single column restrictions from complex OR.
**	30-nov-2009 (dougi)
**	    Pay attention to whether columns originate in outer joins.
**	27-jan-2010 (dougi)
**	    Pick up renamed VT columns from reuse sqs.
**	31-mar-2010 (dougi)
**	    Eliminate semi/anti-joins from OJs.
**	22-apr-2010 (dougi)
**	    Look for column names in VT RESDOM list.
**	28-apr-2010 (dougi)
**	    Handle TCONV function attributes in PST_VAR code.
**	26-oct-2010 (dougi)
**	    Little twiddle to emit "tid" (no "__"s) in update/delete.
**	10-nov-2010 (dougi)
**	    Get coerced decimal scale/precision right.
**	23-feb-2011 (dougi)
**	    Only use "tid" for tid of target table of update/delete.
**	2-may-2011 (dougi) m1921
**	    Identify PST_VARs that map to func attrs that require recursion
**	    to prexpr.
**	26-May-2011 (kiria01) m1964
**	    Avoid exiting early when processing for OPZ_FANUM and don't
**	    allow the coercion to be repeated.
*/
static VOID
opc_x100_prexpr(
	X100_STATE	*x100cb,
	OPO_CO		*cop,
	PST_QNODE	*qnode,
	DB_DATA_VALUE	*restype,
	ADI_FI_ID	coerce_fiid)
{
    OPS_SUBQUERY	*subquery = x100cb->subquery, *vsubq;
    OPV_GRV		*gvarp;
    OPV_VARS		*varp;
    PST_QNODE		*rsdmp, *vnodep, *nodep;
    OPZ_ATTS		*attrp;
    OPZ_BMATTS		*attrmap;
    OPZ_IATTS		attno;
    OPZ_FATTS		*fattrp;
    DB_STATUS		status;
    ADI_OPINFO		adi_opinfo;
    DB_ATT_ID		attid;
    OPE_IEQCLS		eqcno;
    OPE_EQCLIST		*eqclp;
    OPO_CO		*wcop;
    char		var_name[X100_MAXNAME];
    char		att_name[X100_MAXNAME];
    i4			x100_opflag = 0;
    bool		invars, tidcol, found;
    ADI_OP_ID		coerce_op = 0;
    OPL_OJTYPE		jtype;
    bool		oj = (cop && cop->opo_sjpr == DB_SJ && 
			    cop->opo_variant.opo_local && 
			    !(cop->opo_variant.opo_local->opo_mask & 
				(OPO_ANTIJOIN | OPO_SEMIJOIN | OPO_SUBQJOIN)) &&
			    ((jtype = cop->opo_variant.opo_local->opo_type)
					== OPL_LEFTJOIN ||
				jtype == OPL_RIGHTJOIN ||
				jtype == OPL_FULLJOIN));

    att_name[0] = EOS;
    if (cop && cop->opo_sjpr >= DB_RSORT)
	wcop = cop->opo_outer;
    else wcop = cop;
    if (coerce_fiid != ADI_NILCOERCE && coerce_fiid && coerce_fiid != ADI_NOFI)
    {
	ADI_FI_DESC *fi_desc;
	ADI_OP_ID opid;
	ADI_OPINFO opinfo;
	i4 x100_flag;
	if (status = adi_fidesc(&x100cb->adfcb, coerce_fiid, &fi_desc))
	{
	    if (DB_FAILURE_MACRO(status))
		opx_verror(status, E_OP08C7_ADI_OPNAME,
			x100cb->adfcb.adf_errcb.ad_errcode);
	}
	/* If we are about to add constant, don't add a cast if the constant
	** matches the required type. */
	else if (qnode->pst_sym.pst_type != PST_CONST ||
		fi_desc->adi_dtresult != qnode->pst_sym.pst_dataval.db_datatype)
	{
	    coerce_op = fi_desc->adi_fiopid;
	    status = opc_x100_opinfo(&x100cb->adfcb, coerce_op, &opinfo, &x100_flag);
	    if (DB_FAILURE_MACRO(status))
		opx_verror(status, E_OP08C7_ADI_OPNAME,
			x100cb->adfcb.adf_errcb.ad_errcode);
	    opc_x100_printf(x100cb, opinfo.adi_opname);
	    opc_x100_printf(x100cb, "(");
	}
    }
    switch (qnode->pst_sym.pst_type)
    {
    case PST_VAR:
	invars = FALSE;
	tidcol = FALSE;
	found = FALSE;
	if (qnode->pst_sym.pst_value.pst_s_var.pst_vno >= 0)
	{
	    if (BTtest(qnode->pst_sym.pst_value.pst_s_var.pst_vno, 
			(char *)wcop->opo_variant.opo_local->opo_x100bmvars))
		invars = TRUE;
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[qnode->
			pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	    eqcno = attrp->opz_equcls;
	    eqclp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];
	    varp = subquery->ops_vars.opv_base->opv_rt[qnode->
			pst_sym.pst_value.pst_s_var.pst_vno];
	    gvarp = varp->opv_grv;
	    vsubq = gvarp->opv_gquery;
	}
	else 
	{
	    eqcno = OPE_NOEQCLS;
	    gvarp = (OPV_GRV *) NULL;
	    varp = (OPV_VARS *) NULL;
	    attrp = (OPZ_ATTS *) NULL;
	    vsubq = (OPS_SUBQUERY *) NULL;
	}

	/* OPE_TID identifies vanilla TID, OPZ_X100TID identifies TID
	** introduced for join index match. */
	if ((eqcno >= 0 && eqclp->ope_eqctype == OPE_TID ||
		(attrp && attrp->opz_mask & OPZ_X100TID)) &&
		vsubq == (OPS_SUBQUERY *) NULL)		/* is this __tid__? */
	    tidcol = TRUE;
	else
	{
	    opc_x100_copy((char*)&qnode->pst_sym.pst_value.pst_s_var.pst_atname,
			sizeof(qnode->pst_sym.pst_value.pst_s_var.pst_atname),
			att_name, sizeof(att_name));
	    if (MEcmp("tid", att_name, sizeof("tid")) == 0)
		STcopy(tid32, att_name);
	}
	    
	/* Find the name of a column actually available in the current node
	** to overlay the default from the PST_VAR (if there is one). If
	** we're in an OJ, it must be the exact column, though. */
	if (eqcno >= 0 && BTcount((char *)wcop->opo_variant.opo_local->
		opo_x100bmvars, subquery->ops_vars.opv_rv) > 0 && 
		!tidcol && !invars)
	{
	    attrmap = &eqclp->ope_attrmap;
	    for (attno = -1; !found && (attno = BTnext(attno, (char *)attrmap, 
				subquery->ops_attrs.opz_av)) >= 0; )
	    {
		attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];
		if (BTtest(attrp->opz_varnm, 
			(char *)wcop->opo_variant.opo_local->opo_x100bmvars))
		{
		    if (oj && attrp->opz_varnm != qnode->pst_sym.pst_value.
					pst_s_var.pst_vno)
			break;			/* must match table for ojs */
		    varp = subquery->ops_vars.opv_base->
					opv_rt[attrp->opz_varnm];
		    gvarp = varp->opv_grv;
		    if (gvarp == (OPV_GRV *)NULL)
			continue;

		    /* Check for function attr. This can happen if a conversion
		    ** (OPZ_TCONV) is required before a comparison is done in
		    ** a PR node. For those, we can safely (I think) replace
		    ** the FA by the VAR on which the conversion is to be 
		    ** done. For all other function attributes, we must
		    ** compute the function and assign its result to the name
		    ** in the RESDOM of the func definition. */
		    if (attrp->opz_attnm.db_att_id == OPZ_FANUM)
		    {
			fattrp = subquery->ops_funcs.opz_fbase->
					opz_fatts[attrp->opz_func_att];
			if (fattrp->opz_type == OPZ_TCONV &&
			    (nodep = fattrp->opz_function) != 
						(PST_QNODE *) NULL &&
			    nodep->pst_sym.pst_type == PST_RESDOM &&
			    (nodep = nodep->pst_right)->pst_sym.pst_type ==
						PST_VAR)
			{
			    opc_x100_copy((char*)&nodep->pst_sym.pst_value.pst_s_var.pst_atname,
					sizeof(nodep->pst_sym.pst_value.pst_s_var.pst_atname),
					att_name, sizeof(att_name));
			    found = TRUE;
			    continue;
			}
			else if (fattrp->opz_function != (PST_QNODE *) NULL)
			{
			    opc_x100_prexpr(x100cb, cop, fattrp->opz_function,
					restype, ADI_NILCOERCE);
			    goto common_return;
			}
		    }

		    /* If var is a virtual table, look for column in its
		    ** RESDOM list. */
		    vsubq = gvarp->opv_gquery;
		    if (vsubq != (OPS_SUBQUERY *) NULL &&
				gvarp->opv_relation == (RDR_INFO *) NULL)
		     for (rsdmp = vsubq->ops_root->pst_left; 
			!found && rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
			rsdmp = rsdmp->pst_left)
		      if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno ==
			attrp->opz_attnm.db_att_id)
		      {
			 opc_x100_copy((char*)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
				sizeof(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
				att_name, sizeof(att_name));
			found = TRUE;
		      }
		    if (gvarp->opv_relation == (RDR_INFO *) NULL)
			continue;

		    opc_x100_copy((char *)
				gvarp->opv_relation->rdr_attr[attrp->opz_attnm.db_att_id]->att_nmstr,
				gvarp->opv_relation->rdr_attr[attrp->opz_attnm.db_att_id]->att_nmlen,
				att_name, sizeof(att_name));
		    found = TRUE;
		}
	    }
	}
	   
	if (vsubq != (OPS_SUBQUERY *) NULL && 
			(vsubq->ops_mask2 & (OPS_REUSE_USED | OPS_X100_RSDREN)))
	{
	    /* Column maps to a reused subquery - it may have been 
	    ** renamed first time around. Look for RESDOM - it'll tell us. */
	    for (rsdmp = vsubq->ops_root->pst_left;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
	     if ((rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
				(PST_RS_REUSEREN | PST_RS_VTRENAMED)) && 
		rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno == 
				attrp->opz_attnm.db_att_id)
	     {
		opc_x100_copy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
			sizeof(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
			att_name, sizeof(att_name));
		break;
	     }
	}

	if (att_name[0] == EOS && /* Test for empty string as blanks trimmed */
		vsubq != (OPS_SUBQUERY *)NULL)
	{
	    /* Blank column name - this might be an expression in
	    ** an aggregate list and requires looking up the RESDOM
	    ** representing its computation in vsubq. */
	    for (rsdmp = vsubq->ops_root->pst_left; 
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
	     if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno ==
		attrp->opz_attnm.db_att_id)
	     {
		if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags 
								& PST_RS_TID)
		    tidcol = TRUE;
	 	else
		    opc_x100_copy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
				sizeof(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
				att_name, sizeof(att_name));
		break;
	     }
	}

	/* This is a __tid__. */
	if (tidcol)
	    STcopy(tid32, att_name);

	/* Prepend correlation name, as appropriate and if column belongs
	** to a table (real or virtual). */
	if (qnode->pst_sym.pst_value.pst_s_var.pst_vno >= 0)
	    opc_x100_prepcorr(x100cb, subquery, gvarp, varp, attrp);

	/* "tid" or if real base table column, get name from VAR node. */
	if (tidcol && (x100cb->flag & X100_DMLUPDEL) && 
	    (subquery->ops_vars.opv_rv <= 1 ||
	     qnode->pst_sym.pst_value.pst_s_var.pst_vno <= 0))
	    STcopy("tid", var_name);
	else x100_print_id(att_name, DB_ATT_MAXNAME, var_name);

	opc_x100_printf(x100cb, var_name);
	break;

    case PST_RESDOM:
	/* Copy name from pst_rsname and print it. */
	x100_print_id((char *)&qnode->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
	    DB_ATT_MAXNAME, var_name);

	opc_x100_printf(x100cb, var_name);
	break;

    case PST_OR:
    {
	opc_x100_printf(x100cb, "|| (");
	opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype, ADI_NILCOERCE);
	opc_x100_printf(x100cb, ", ");
	opc_x100_prexpr(x100cb, cop, qnode->pst_right, restype, ADI_NILCOERCE);
	opc_x100_printf(x100cb, ")");
	break;
    }

    case PST_AND:
    {
	if (!qnode->pst_right ||
	    qnode->pst_right->pst_sym.pst_type == PST_QLEND)
	    opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype, 
			ADI_NILCOERCE); /* degenerate AND
					** can occur on the right side */
	else if ((!qnode->pst_left ||
	    qnode->pst_left->pst_sym.pst_type == PST_QLEND) &&
	    qnode->pst_right != (PST_QNODE *) NULL)
	    opc_x100_prexpr(x100cb, cop, qnode->pst_right, restype, 
			ADI_NILCOERCE); /* degenerate AND
					** can occur on the left side too */
	else
	{
	    opc_x100_printf(x100cb, "&& (");
	    opc_x100_prexpr(x100cb, cop, qnode->pst_left, restype, ADI_NILCOERCE);
	    opc_x100_printf(x100cb, ", ");
	    opc_x100_prexpr(x100cb, cop, qnode->pst_right, restype, ADI_NILCOERCE);
	    opc_x100_printf(x100cb, ")");
	}
	break;
    }

    case PST_MOP:
    {
        opc_x100_mop(x100cb, cop, qnode, restype);
        break;
    }

    case PST_BOP:
    {
	opc_x100_bop(x100cb, cop, qnode, restype);
	break;
    }

    case PST_UOP:
    {
	opc_x100_uop(x100cb, cop, qnode, restype);
	break;
    }

    case PST_COP:
    {
	/* Whilst waiting for boolean support - handle the explicit
	** TRUE / FALSE states that can arise from folding */
	if (qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_IIFALSE_OP)
	{
	    opc_x100_printf(x100cb, "!=(sint('0'),sint('0'))");
	    break;
	}
	else if (qnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_IITRUE_OP)
	{
	    opc_x100_printf(x100cb, "==(sint('0'),sint('0'))");
	    break;
	}
	status = opc_x100_opinfo(&x100cb->adfcb, 
	    qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_opinfo,
	    &x100_opflag);
	if (DB_FAILURE_MACRO(status))
	    opx_verror(status, E_OP08C7_ADI_OPNAME,
		x100cb->adfcb.adf_errcb.ad_errcode);

	opc_x100_printf(x100cb, (char *)&adi_opinfo.adi_opname);

        /* If we have a COP with no parameters add the braces */
        if (qnode->pst_left == NULL && qnode->pst_right == NULL) 
        {
	    opc_x100_printf(x100cb, "()");
        }
	break;
    }

    case PST_CONST:
	opc_x100_prconst(x100cb, qnode, restype, coerce_op);
	break;

    case PST_AOP:
    {
	status = opc_x100_opinfo( &x100cb->adfcb,
		qnode->pst_sym.pst_value.pst_s_op.pst_opno, &adi_opinfo,
		&x100_opflag);
	if (DB_FAILURE_MACRO(status))
		opx_verror(status, E_OP08C7_ADI_OPNAME,
		    x100cb->adfcb.adf_errcb.ad_errcode);

	opc_x100_printf(x100cb, (char *)&adi_opinfo.adi_opname);

	if (qnode->pst_left)
	{   
	    /* Special case hack for count(*) which is returned totally
	    ** in adi_opname */
	    opc_x100_printf( x100cb, "(");

	    /* These will be headed off at the pass in opc_x100_projaggr(). 
	    if (qnode->pst_sym.pst_value.pst_s_op.pst_distinct == 
							PST_DISTINCT)
	    {
		opc_x100_printf( x100cb, " distinct ");
	    } */

	    if (qnode->pst_left->pst_sym.pst_type == PST_CONST
			&&
		(
		    (   qnode->pst_sym.pst_value.pst_s_op.pst_opno 
			==
			x100cb->global->ops_cb->ops_server->opg_count
		    )
		    ||
		    (   qnode->pst_sym.pst_value.pst_s_op.pst_opno 
			==
			x100cb->global->ops_cb->ops_server->opg_scount
		    )
		)
		/* And these will also be headed off.
	        &&
		(   /* Not allowed to have 'count(distinct *)', so
		    ** don't substitute '*' for a PST_CONST if this
		    ** is a distinct-query--use whatever is in
		    ** qnode->pst_left, instead.
		    **
		    qnode->pst_sym.pst_value.pst_s_op.pst_distinct
		    !=
		    PST_DISTINCT
		)
		*/
	       )
		opc_x100_printf( x100cb, " "); /* do not use constants
				    ** in the operand of a count aggregate
				    ** if at all possible (no cols either) */
	    else
		opc_x100_prexpr( x100cb, cop, qnode->pst_left, restype, ADI_NILCOERCE);

	    opc_x100_printf( x100cb, ")");
	    break;
	}
	break;
    }

    case PST_CASEOP:
    {
	/* CASE expressions contain chain of PST_WHLIST nodes, each of
	** which addresses a PST_WHOP. The WHLISTs are the "when" clauses
	** and the WHOPs anchor the predicate and result of the "when".
	** For X100, each WHLIST generates a "ifthenelse()" function that
	** evaluates a predicate and assigns a result. Multiple "when"s 
	** generate nested "ifthenelse()"s.
	**
	** So, given all that, we recurse on the PST_CASEOP node to process
	** the subtree attached to its PST_WHLIST list. */

	opc_x100_prexpr(x100cb, cop, qnode->pst_left, 
					&qnode->pst_sym.pst_dataval, ADI_NILCOERCE);
	break;
    }

    case PST_WHLIST:
    {
	PST_QNODE	*whop = qnode->pst_right;

	opc_x100_printf( x100cb, "ifthenelse( ");
	if (whop->pst_left == (PST_QNODE *)NULL)
	{
	    /* Shouldn't happen. */
	}
	opc_x100_prexpr( x100cb, cop, whop->pst_left, restype, ADI_NILCOERCE);
	opc_x100_printf( x100cb, ", ");
	opc_x100_prexpr( x100cb, cop, whop->pst_right, restype, ADI_NILCOERCE);
	opc_x100_printf( x100cb, ", ");

	/* If the next WHLIST has no predicate, its expr is the FALSE result,
	** and if there is no next WHLIST, the FALSE result is NULL. Otherwise,
	** recurse on WHLIST and emit another "ifthenelse()". */
	if (qnode->pst_left == (PST_QNODE *) NULL)
	    opc_x100_printf( x100cb, "sint'-1' ");
	else if (qnode->pst_left->pst_right->pst_left == (PST_QNODE *) NULL)
	    opc_x100_prexpr( x100cb, cop, qnode->pst_left->pst_right->pst_right,
					restype, ADI_NILCOERCE);
	else
	    opc_x100_prexpr( x100cb, cop, qnode->pst_left, 
					restype, ADI_NILCOERCE);

	opc_x100_printf( x100cb, ")");
	break;
    }

    case PST_QLEND:
    {
	break;				/* nothing to do */
    }

    default:
	opx_error(E_OP0681_UNKNOWN_NODE);
    }


common_return:
    if (coerce_op)
    {
	if (coerce_op == ADI_DEC_OP)
	{
	    char p, s, tmp[50];
	    if (restype && abs(restype->db_datatype) == DB_DEC_TYPE)
	    {
		p = DB_P_DECODE_MACRO(restype->db_prec);
		s = DB_S_DECODE_MACRO(restype->db_prec);
	    }
	    else switch (qnode->pst_sym.pst_dataval.db_datatype*8+
			qnode->pst_sym.pst_dataval.db_length)
	    {
	    case DB_INT_TYPE*8+(i4)sizeof(i1):
	    case-DB_INT_TYPE*8+(i4)sizeof(i1)+1: /*Nullable*/
		p = AD_I1_TO_DEC_PREC; s = AD_I1_TO_DEC_SCALE;
		break;
	    case DB_INT_TYPE*8+(i4)sizeof(i2):
	    case-DB_INT_TYPE*8+(i4)sizeof(i2)+1: /*Nullable*/
		p = AD_I2_TO_DEC_PREC; s = AD_I2_TO_DEC_SCALE;
		break;
	    case DB_INT_TYPE*8+(i4)sizeof(i4):
	    case-DB_INT_TYPE*8+(i4)sizeof(i4)+1: /*Nullable*/
		p = AD_I4_TO_DEC_PREC; s = AD_I4_TO_DEC_SCALE;
		break;
	    case DB_INT_TYPE*8+(i4)sizeof(i8):
	    case-DB_INT_TYPE*8+(i4)sizeof(i8)+1: /*Nullable*/
		p = AD_I8_TO_DEC_PREC; s = AD_I8_TO_DEC_SCALE;
		break;
	    case DB_FLT_TYPE*8+(i4)sizeof(f4):
	    case-DB_FLT_TYPE*8+(i4)sizeof(f4)+1: /*Nullable*/
		p = AD_FP_TO_DEC_PREC; s = AD_FP_TO_DEC_SCALE;
		break;
	    case DB_FLT_TYPE*8+(i4)sizeof(f8):
	    case-DB_FLT_TYPE*8+(i4)sizeof(f8)+1: /*Nullable*/
		p = AD_FP_TO_DEC_PREC; s = AD_FP_TO_DEC_SCALE;
		break;
	    default:
		p = AD_FP_TO_DEC_PREC; s = AD_FP_TO_DEC_SCALE;
                break;
	    }
	    STprintf(tmp, ",sint('%d'), sint('%d')", p, s);
	    opc_x100_printf(x100cb, tmp);
	}
	opc_x100_printf(x100cb, ")");
    }
}

/*{
** Name: OPC_X100_PREPCORR	Prepend column qualification (if any)
**
** Description:
**      Determine if column name qualifier is required. If column ref on
**	base table, do it. If reuse subquery, use reuse name. Otherwise,
**	descend through virtual tables until a base table or reuse is found
**	and prepend accordingly. Or don't do anything!
**
** Inputs:
**	x100cb				ptr to cross copiler state structure
**      subquery			ptr to subquery containing column ref
**	gvarp				ptr to global range table entry 
**	varp				ptr to local range table entry
**	attrp				ptr to joinop range table entry
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    Writes qualifier if there is one.to the text stream (plus ".") 
**
** History:
**	26-june-2009 (dougi)
**	    Written for X100 support.
**	04-Aug-2009 (kiria01)
**	    Don't emit manufactured table names such as 'union'
**	12-Aug-2009 (kiria01)
**	    ... but do output correlated name instead.
**	5-jan-2010 (dougi)
**	    Rewritten to accommodate named virtual tables. That greatly
**	    simplifies the process.
**	2-Dec-2010 (kschendel)
**	    Fix a long-name oops.
*/
static void
opc_x100_prepcorr(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	OPV_GRV		*gvarp,
	OPV_VARS	*varp,
	OPZ_ATTS	*attrp)

{
    OPS_SUBQUERY	*vsubq;
    char		corrname[X100_MAXNAME];
    PST_QNODE		*rsdmp, *vnodep;
    OPZ_ATTS		*attr1;
    OPV_GRV		*gvar1;
    OPV_VARS		*var1;
    bool		found;


    /* Now simply copy name from opv_varname. If there are multiple 
    ** instances of the same table (as can happen in Ingres), use OPV_VARS
    ** to uniqueify them. */
    x100_print_id((char *)&varp->opv_varname, DB_ATT_MAXNAME, corrname);

    if (gvarp->opv_qrt > OPV_NOGVAR &&
	subquery->ops_global->ops_qheader->
		pst_rangetab[gvarp->opv_qrt]->pst_rngvar.db_tab_base != 0 &&
	subquery->ops_global->ops_qheader->
		pst_rangetab[gvarp->opv_qrt]->pst_rgtype == PST_TABLE &&
	varp && gvarp->opv_x100gcnt > 1 && varp->opv_x100cnt > 0)
	STprintf(corrname, "%s%d", corrname, varp->opv_x100cnt);

    opc_x100_printf(x100cb, corrname);
    opc_x100_printf(x100cb, ".");

}

/*{
** Name: OPC_X100_AOPTEST	- Check parse tree fragment for aggs
**
** Description:
**      Recursively descend parse tree fragment looking for PST_AOP nodes.
**
** Inputs:
**      nodep                           ptr to parse tree fragment to search
**
** Outputs:
**	Returns:
**		TRUE	- if parse tree has aggregation operation
**		FALSE	- otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-sept-2008 (dougi)
**	    Written for X100 support.
*/
static bool
opc_x100_aoptest(
	PST_QNODE	*nodep)

{
    /* Trivial function that checks current node for PST_AOP and, if
    ** not PST_AOP, recurses. */

    if (nodep == (PST_QNODE *) NULL)
	return(FALSE);
    else if (nodep->pst_sym.pst_type == PST_AOP)
	return(TRUE);
    else return(opc_x100_aoptest(nodep->pst_left) ||
			opc_x100_aoptest(nodep->pst_right));

}

/*{
** Name: OPC_X100_ADDVARS	- Extract PST_VAR nodes to be added to 
**				introduced Project()s
**
** Description:
**      Recursively descend parse tree fragments from Aggr() RESDOM list
**	looking for PST_VAR nodes to add to pre-projection list.
**
** Inputs:
**	projlist			ptr to projection list to augment
**      nodep                           ptr to parse tree fragment to search
**	prsdmp				ptr to RESDOM that owns nodep
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-sept-2008 (dougi)
**	    Written for X100 support.
**	31-dec-2009 (dougi)
**	    Change 2nd parm to ** to allow for NULL input.
**	12-apr-2010 (dougi)
**	    Remove name copying stuff - that's done elsewhere now.
*/
static VOID
opc_x100_addVARs(
	OPS_STATE	*global,
	PST_QNODE	**projlist,
	PST_QNODE	*nodep,
	PST_QNODE	*prsdmp)

{
    PST_QNODE	*rsdmp, *oldrsdmp, *node1p;
    DB_DATA_VALUE	dummy_dbval;
    bool	tempres = FALSE;

    /* Look for PST_VAR node to copy, otherwise recurse down tree. */
    if (nodep->pst_sym.pst_type != PST_VAR)
    {
	if (nodep->pst_right)
	    opc_x100_addVARs(global, projlist, nodep->pst_right,
							(PST_QNODE *) NULL);
	if (nodep->pst_left)
	    opc_x100_addVARs(global, projlist, nodep->pst_left,
							(PST_QNODE *) NULL);
    }
    else
    {
	if (nodep->pst_sym.pst_value.pst_s_var.pst_vno == -1)
	    tempres = TRUE;
	for (rsdmp = (*projlist), oldrsdmp = (PST_QNODE *) NULL;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		oldrsdmp = rsdmp, rsdmp = rsdmp->pst_left)
	 if ((node1p = rsdmp->pst_right)->pst_sym.pst_type == PST_VAR)
	 {
	     if (nodep->pst_sym.pst_value.pst_s_var.pst_vno ==
		    node1p->pst_sym.pst_value.pst_s_var.pst_vno &&
		nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		    node1p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
		return;			/* already in the list */
	 }
	 else if (tempres && MEcmp((char *)&nodep->pst_sym.pst_value.pst_s_var.
			pst_atname, rsdmp->pst_sym.pst_value.pst_s_rsdm.
			pst_rsname, sizeof(DB_ATT_NAME)) == 0)
	    return;			/* corr. expression already in list */

	/* We arrive here with a PST_VAR to add and with oldrsdmp 
	** addressing the last RESDOM in the list. Make new RESDOM and
	** attach to the list. */
	rsdmp = opv_resdom(global, nodep, &nodep->pst_sym.pst_dataval);

	if (prsdmp)
	    rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags =
			prsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags;
	if (oldrsdmp == (PST_QNODE *) NULL)
	    (*projlist) = rsdmp;
	else oldrsdmp->pst_left = rsdmp;
	rsdmp->pst_left = (PST_QNODE *) NULL;
    }

    return;
}

/*{
** Name: OPC_X100_BFEMIT	- Generate Boolean expressions for Select()
**	operators.
**
** Description:
**
** Inputs:
**      x100cb				ptr to X100 text state variable
**      subquery			ptr to current subquery
**	cop				ptr to current OPO_CO node
**	pcnt				count of BFs to compile
**	ojid				outer join ID of BFs to exclude
**	konst				flag to emit any .opb_bfconstants
**	quals				flag to emit any extra qualifiers
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-sept-2008 (dougi)
**	    Written for X100 support.
**	20-Aug-2009 (kiria01)
**	    Add code to generate the non-BF qualification if needed.
**	11-mar-2011 (dougi)
**	    Add ON clause support.
*/
static VOID
opc_x100_bfemit(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop,
	i4		pcnt,
	OPL_IOUTER	ojid,
	bool		konst,
	bool		quals)

{
    OPS_STATE		*global = subquery->ops_global;
    OPB_BOOLFACT	*bfp, *bfalocal[10], **bfarray;
    i4			i, j, newpcnt;


    /* Allocate (if > 10) and populate array of opb_boolfact ptrs. */
    if (pcnt > 10)
	bfarray = (OPB_BOOLFACT **)opu_memory(global, 
				pcnt * sizeof (OPB_BOOLFACT *));
    else bfarray = &bfalocal[0];

    if (pcnt)
	for (i = -1, j = 0; (i = BTnext(i, 
		(char *)cop->opo_variant.opo_local->
			opo_bmbf, subquery->ops_bfs.opb_bv)) != -1; j++)
	{
	    bfarray[j] = subquery->ops_bfs.opb_base->opb_boolfact[i];

	    /* If no OJ to check, fine. Otherwise, skip those that match -
	    ** they've already been added to ON clause. */
	    if (ojid > OPL_NOOUTER && bfarray[j]->opb_ojid == ojid)
		j--;
	}
    else if (subquery->ops_sqtype == OPS_MAIN && cop == (OPO_CO *) NULL)
    {
	/* Predicate on CO-less MAIN sq - possibly HAVING clause. */
	j = pcnt = subquery->ops_bfs.opb_bv;
	for (i = 0; i < pcnt; i++)
	    bfarray[i] = subquery->ops_bfs.opb_base->opb_boolfact[i];
    }
    else j = 0;

    newpcnt = j;

    /* Sort array by opb_selectivity to apply most restrictive first - 
    ** bubbly, bubbly, bubbly ... */
    for (i = 0; i < newpcnt; i++)
     for (j = i+1; j < newpcnt; j++)
      if (bfarray[i]->opb_selectivity > bfarray[j]->opb_selectivity)
      {
	bfp = bfarray[i];
	bfarray[i] = bfarray[j];
	bfarray[j] = bfp;
      }

    /* With that done, emit the actual expressions.
    ** First - rare case where we have a constant FALSE qualifier - clip the lot. */
    if (konst)
    {
	opc_x100_printf(x100cb, ", ");
	opc_x100_prexpr(x100cb, cop, subquery->ops_bfs.opb_bfconstants, (DB_DATA_VALUE *)NULL, ADI_NILCOERCE);
	opc_x100_printf(x100cb, ") ");
    }
    /* Next emit the (sorted) boolean factors - most restrictive will be innermost */
    for (i = 0; i < newpcnt; i++)
    {
	opc_x100_printf(x100cb, ", ");
	opc_x100_prexpr(x100cb, cop, bfarray[i]->opb_qnode, (DB_DATA_VALUE *)NULL, ADI_NILCOERCE);
	opc_x100_printf(x100cb, ") ");
    }
    /* FIXME? I suspect that the remaining qualifiers might be more optimally interspersed
    ** amongst the boolean factors mirroring the decreasing restrictiveness */
    if (quals)
    {
	opc_x100_printf(x100cb, ", ");
	opc_x100_prexpr(x100cb, cop, subquery->ops_root->pst_right, (DB_DATA_VALUE *)NULL, ADI_NILCOERCE);
	opc_x100_printf(x100cb, ") ");
    }
}

/*{
** Name: OPC_X100_FACHECK	- look for FAs in eqcmap
**
** Description:
**
** Inputs:
**      x100cb				ptr to X100 text state variable
**      subquery			ptr to current subquery
**	eqcmap				ptr to map of EQCs in current node
**	varno				range var no of current table
**
** Outputs:
**	Returns:
**	    TRUE	if there's a function attr in EQC list
**	    FALSE	otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-jan-2009 (dougi)
**	    Written for X100 support.
*/
static bool
opc_x100_FAcheck(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	OPE_BMEQCLS	*eqcmap,
	OPV_IVARS	varno)

{
    OPZ_ATTS	*attrp;
    OPZ_FATTS	*fattrp;
    OPZ_IATTS	attrno;
    OPE_EQCLIST *eqcp;
    OPE_IEQCLS	eqcno;

    for (eqcno = -1; (eqcno = BTnext(eqcno, (char *)eqcmap, 
	    subquery->ops_eclass.ope_ev)) >= 0; )
    {
	eqcp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];

	/* Search for joinop attr in current table. */
	for (attrno = -1; (attrno = BTnext(attrno, (char *)&eqcp->ope_attrmap,
	    subquery->ops_attrs.opz_av)) >= 0; )
	{
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[attrno];
	    if (attrp->opz_varnm != varno)
		continue;
	    if (attrp->opz_func_att == OPZ_NOFUNCATT)
		continue;
	    fattrp = subquery->ops_funcs.opz_fbase->
					opz_fatts[attrp->opz_func_att];

	    /* Following hack test detects "count()" in SAGG, which isn't
 	    ** really what we're looking for. */
	    if (subquery->ops_sqtype == OPS_SAGG &&
		fattrp->opz_function &&
		fattrp->opz_function->pst_sym.pst_type == PST_RESDOM &&
		fattrp->opz_function->pst_right->pst_sym.pst_type == PST_CONST)
		continue;
	    if (attrp->opz_attnm.db_att_id == OPZ_FANUM)
		return(TRUE);
	}
    }

    /* If we get to here, return FALSE (no FAs). */
    return(FALSE);

}

/*{
** Name: OPC_X100_REUSE_RECURSE	- recurse on subquery range table looking
**	for REUSE sqs
**
** Description:	Search for REUSE subqueries n which we can extinguish the
** 	ops_reuse_qual FLAGS.
**
** Inputs:
**      subquery		ptr to current subquery
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    may extinguish OPS_REUSE_QUAL flags of referenced REUSE subqueries
**
** History:
**	8-july-2009 (dougi)
**	    Written for X100 support.
*/
static void
opc_x100_reuse_recurse(
	OPS_SUBQUERY	*subquery)
{
    OPS_SUBQUERY *vsubq;
    OPV_GRV	*grvp;
    i4		i;


    /* First see if this is a REUSE subquery - turn off flag if so. */
    if (subquery->ops_sqtype == OPS_REUSE ||
		(subquery->ops_mask2 & OPS_REUSE_PLANB))
    {
	subquery->ops_mask2 &= ~OPS_REUSE_QUAL;
	return;
    }

    /* If not, loop over its range table and recurse on virtual tables. */
    for (i = 0; i < subquery->ops_vars.opv_prv; i++)
    {
	grvp = subquery->ops_vars.opv_base->opv_rt[i]->opv_grv;
	if (grvp && (vsubq = grvp->opv_gquery) != (OPS_SUBQUERY *) NULL)
	    opc_x100_reuse_recurse(vsubq);
    }
}

/*{
** Name: OPC_X100_REUSECK	- look for REUSE subqueries  & extinguish 
**	OPS_REUSE_QUAL flags
**
** Description:	Search for REUSE subqueries in range of current join. Turn
**	off OPS_REUSE_QUAL flag as needed.
**
** Inputs:
**      subquery		ptr to current subquery
**	inner			ptr to inner CO-node of join being checked
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    may extinguish OPS_REUSE_QUAL flags of referenced REUSE subqueries
**
** History:
**	7-july-2009 (dougi)
**	    Written for X100 support.
*/
static void
opc_x100_reuseck(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*inner)

{
    OPS_SUBQUERY *vsubq;
    OPO_CO	*cop;

    /* If inner addresses a virtual table, run its range table looking for 
    ** a REUSE subquery reference (i.e., another virtual table that is a
    ** REUSE subquery). Turn off the OPS_REUSE_QUAL flags for those guys. */
    if (inner->opo_sjpr == DB_PR)
	cop = inner->opo_outer;
    else cop = inner;

    if (cop->opo_sjpr != DB_ORIG ||
	(vsubq = subquery->ops_vars.opv_base->opv_rt[cop->opo_union.opo_orig]->
		opv_grv->opv_gquery) == (OPS_SUBQUERY *) NULL)
	return;				/* not interesting - just leave */

    /* Call recursion function to analyze vsubq and if it isn't itself
    ** a REUSE, analyze its range table entries to look for one. */
    opc_x100_reuse_recurse(vsubq);

}

/*{
** Name: OPC_X100_IXCOLLAPSE - search for JOINIX EQCs that can be combined
**
** Description:	the joining EQCs may include join indexes that may subsume
**	others (e.g. partsupp->lineitem on l_partkey, l_suppkey, but also
**	indirectly through part and supplier. ).
**
** Inputs:
**      subquery		ptr to current subquery
**	jeqcmap			ptr to EQC bit map of equijoin preds
**
** Outputs:
**	Returns;		
**	Exceptions:
**	    none
**
** Side Effects:
**	    removes subsumed EQCs from EQC map
**
** History:
**	1-may-2009 (dougi)
**	    Written for X100 support.
*/
static VOID
opc_x100_ixcollapse(
	OPS_SUBQUERY	*subquery,
	OPE_BMEQCLS	*jeqcmap)

{
    OPE_IEQCLS		jeqcno1, jeqcno2;
    OPE_EQCLIST		*eqc1p, *eqc2p;

    /* Loop over the join EQCs, looking for a join index. */
    for (jeqcno1 = -1; (jeqcno1 = BTnext(jeqcno1, (char *)jeqcmap, 
			subquery->ops_eclass.ope_ev)) >= 0; )
     if (((eqc1p = subquery->ops_eclass.ope_base->ope_eqclist[jeqcno1])->
		ope_mask & OPE_JOINIX) && eqc1p->ope_joinrep &&
		BTcount((char *)eqc1p->ope_joinrep, subquery->
				ops_eclass.ope_ev) > 0)
     {
	/* We have a multi-column refrel EQC. Look for single column 
	** refrel EQCs subsumed by this one that we can now discard. */
	for (jeqcno2 = -1; (jeqcno2 = BTnext(jeqcno2, (char *)jeqcmap,
			subquery->ops_eclass.ope_ev)) >= 0; )
	 if ((eqc2p = subquery->ops_eclass.ope_base->ope_eqclist[jeqcno2])->
		ope_mask & OPE_JOINIX) 
	 {
	    if (jeqcno1 == jeqcno2 || eqc2p->ope_joinrep == (OPE_BMEQCLS *) NULL)
		continue;
	    if (BTsubset((char *)eqc2p->ope_joinrep, (char *)eqc1p->ope_joinrep,
			subquery->ops_eclass.ope_ev) == TRUE)
		BTclear(jeqcno2, (char *)jeqcmap);
	 }
     }

}

/*{
** Name: OPC_X100_PRESDOM	- generate expression list from RESDOM chain
**
** Description:
**      Process RESDOM chain, producing list of expressions for Project,
**	Aggr BY-list, etc.
**
** Inputs:
**      x100cb				ptr to X100 text state variable
**      qnode                           query tree node to convert
**	printname			TRUE - print "name = " ahead of expr
**					FALSE - don't print name
**	reorder				TRUE - emit result column expressions
**					in pst_rsno sequence
**					FALSE - emit in RESDOM sequence
**	printall			TRUE - emit for all RESDOMs
**					FALSE - only do PST_RS_PRINT RESDOMs
**	aggr				TRUE - prep list for [Ord]Aggr(). Drop
**					duplicate entries
**
** Outputs:
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-aug-2008 (dougi)
**	    Written for X100 support.
**	17-mar-2010 (dougi)
**	    Added aggr parm to eliminate dup columns from aggr list.
**	13-apr-2010 (dougi)
**	    Add code to coerce source to RESDOM type, when needed.
**	14-apr-2010 (dougi)
**	    Add "tid" column to updates only for main subquery.
**	29-apr-2010 (dougi)
**	    Skip tid from RESDOM in UPDATE - it's there for sorting and will
**	    already be in projection because of previous change.
**	13-may-2010 (dougi)
**	    Really skip TIDs in UPDATE. They can be lurking elsewhere in 
**	    RESDOM list, too.
**	21-may-2010 (dougi)
**	    Some derived table induced cases have same column twice in same
**	    RESDOM list - one as non-print. Get rid of the non-print.
**	27-aug-2010 (dougi)
**	    Allow dup entries if their names differ - fixes Mantis 919.
**	25-oct-2010 (dougi)
**	    Don't emit coercions for dupselim aggr calls.
**	9-dec-2010 (dougi)
**	    Force "=" assignments of PST_RS_TIDs (used to exclude them) to 
**	    assure correct "tid" names later on.
**	10-dec-2010 (dougi)
**	    Drop non-aggr dups, if both VAR and RESDOM names are identical.
**	16-Feb-2011 (kiria01)
**	    Make sure rsdmarray is not accessed beyond bounds.
**	17-feb-2011 (dougi)
**	    Better handle renamed expressions in projections.
**	25-feb-2011 (dougi)
**	    Don't rename tid columns in DELETE/MODIFY.
**	11-mar-2011 (dougi)
**	    Just pst_rsname for pseudoTIDs.
**	5-may-2011 (dougi) m1925
**	    Overzealous trimming of DERIVED columns from dups elim.
**	27-may-2011 (dougi) m1961
**	    Skip not-yet-ready window functions.
**	28-May-2011 (kiria01) b125391
**	    Check that the non-printing RESDOM was indeed a valid VAR
**	    (and not a sub-expression mark) before dereferencing 
**	    opz_base via an incorrect attribute number.
**	30-may-2011 (dougi) m1971
**	    Flag constant exprs for DERIVED.
*/

static VOID
opc_x100_presdom(
	X100_STATE	*x100cb,
	OPO_CO		*cop,
	PST_QNODE	*qnode,
	bool		printname,
	bool		reorder,
	bool		printall,
	bool		aggr)

{
    OPS_STATE	*global = x100cb->global;
    OPS_SUBQUERY *subquery = x100cb->subquery;
    PST_QNODE	*rsdmp, **rsdmarray, *rsdm1p, *v1p, *v2p, *nodep;
    OPE_IEQCLS	eqcno;
    OPZ_IATTS	attrno, attrno1;
    OPZ_ATTS	*attrp;
    char	var_name[X100_MAXNAME];
    char	rsname[X100_MAXNAME];
    i4		i, j, k, rscount, namelen, rsflags, strt, end;
    DB_STATUS	status;
    DB_ERROR	error;
    DB_DT_ID	restype, srcetype;
    ADI_FI_ID	cfid;
    bool	skip, gotcha, skiptid;

    if (cop != (OPO_CO *) NULL && cop->opo_sjpr >= DB_RSORT)
	cop = cop->opo_outer;		/* use child of sort */

    /* Count printing RESDOMs so ptr array can be allocated and 
    ** populated. With "reorder", RESDOMs must be reversed (final
    ** projections are chained in reverse). */
    for (rsdmp = qnode, rscount = 0; 
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
     if ((printall || (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
						PST_RS_PRINT)) &&
	!(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
				(PST_RS_X100GB | PST_RS_WINNOEMIT)))
	rscount++;

    if (rscount)
    {
	rsdmarray = (PST_QNODE **)opu_memory(global, 
				rscount * sizeof(PST_QNODE *));
	for (i = 0; i < rscount; i++)
	    rsdmarray[i] = (PST_QNODE *) NULL;
    }
    else 
    {
	opc_x100_printf(x100cb, ", [] ");	/* empty list */
	return;
    }

    /* Now loop over RESDOM chain, populating array in the required
    ** direction. */
    i = (reorder) ? rscount-1 : 0;
    for (rsdmp = qnode; rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
     if ((((rsflags = rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags) & 
		PST_RS_PRINT) || printall) && 
		!(rsflags & (PST_RS_X100GB | PST_RS_WINNOEMIT)))
     {
	PST_QNODE *var;

	if (!(rsflags & PST_RS_PRINT) &&
	    (rsflags & PST_RS_TID &&
	    !BTtest(rsdmp->pst_right->pst_sym.pst_value.pst_s_var.pst_vno,
			(char *)cop->opo_variant.opo_local->opo_x100bmvars)))
	{
	    /* No point in including a non-print derived column - probably a 
	    ** superfluous TID. */
	    continue;
	}

	/* Look for TID columns to skip in UPDATE RESDOM list. */
	if ((x100cb->flag & X100_MODIFY) && !(rsflags & PST_RS_PRINT))
	{
	    continue;				/* skip tid sort column */
	}
	if ((x100cb->flag & X100_MODIFY) &&
		(v1p = rsdmp->pst_right)->pst_sym.pst_type == PST_VAR)
	{
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[v1p->
			pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	    if (attrp->opz_varnm == 0 && attrp->opz_attnm.db_att_id == 0)
	    {
		continue;			/* skip TIDs for UPDATE */
	    }
	}

	{
	    /* Drop duplicates from both Project & Aggr. */
	    skip = FALSE;
	    if (reorder)
	    {
		strt = i+1;
		end = rscount;
	    }
	    else
	    {
		strt = 0;
		end = i;
	    }

	    for (j = strt; j < end && !skip; j++)
	     if ((v1p = rsdmp->pst_right)->pst_sym.pst_type == PST_VAR &&
		(v2p = rsdmarray[j]->pst_right)->pst_sym.pst_type == PST_VAR &&
		v1p->pst_sym.pst_value.pst_s_var.pst_vno ==
			v2p->pst_sym.pst_value.pst_s_var.pst_vno &&
		v1p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
			v2p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id &&
		v1p->pst_sym.pst_value.pst_s_var.pst_vno >= 0 &&
		MEcmp((char *)&v1p->pst_sym.pst_value.pst_s_var.pst_atname,
		    (char *)&v2p->pst_sym.pst_value.pst_s_var.pst_atname,
		    sizeof(DB_ATT_NAME)) == 0 &&
		(aggr || MEcmp((char *)rsdmp->pst_sym.pst_value.pst_s_rsdm.
		    pst_rsname, (char *)&rsdmarray[j]->pst_sym.pst_value.
			pst_s_rsdm.pst_rsname, sizeof(DB_ATT_NAME)) == 0))
		skip = TRUE;

	    if (skip)
	    {
		rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &= 
			~PST_RS_DERIVED;	/* kill this - not relevant */
		continue;			/* already there - skip it */
	    }
	}

	/* Ingres may redundantly include columns for GROUP BY, which can
 	** be in the same EQC as another column in the RESDOM list. This can
	** give X100 indigestion, so we check for the condition here and drop
	** such columns. It would be easier if RESDOM lists were based on EQCs,
	** but they're not. */
	if (!(rsflags & PST_RS_PRINT) &&
	    (var = rsdmp->pst_right) &&
	    var->pst_sym.pst_type == PST_VAR &&
	    var->pst_sym.pst_value.pst_s_var.pst_vno >= 0 &&
	    (attrno = var->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id) >= 0)
	{
	    eqcno = subquery->ops_attrs.opz_base->opz_attnums[attrno]->
								opz_equcls;
	    for (attrno1 = -1, gotcha = FALSE; !gotcha && 
		(attrno1 = BTnext((i4)attrno1, (char *)&subquery->
		    ops_eclass.ope_base->ope_eqclist[eqcno]->ope_attrmap, 
		    subquery->ops_attrs.opz_av)) >= 0; )
	     if (attrno1 == attrno)
		continue;
	     else for (rsdm1p = qnode; !gotcha && rsdm1p && 
			rsdm1p->pst_sym.pst_type == PST_RESDOM; 
			rsdm1p = rsdm1p->pst_left)
	      if (rsdmp == rsdm1p ||
			rsdm1p->pst_right->pst_sym.pst_type != PST_VAR)
		continue;
	      else if (rsdm1p->pst_right->pst_sym.pst_value.pst_s_var.
			pst_atno.db_att_id == attrno1)
		gotcha = TRUE;

	    if (gotcha)
		continue;			/* got a match - skip it */
	}
	

	rsdmarray[i] = rsdmp;
	if (reorder)
	    i--;
	else i++;
     }
    
    /* Loop over RESDOM array, adding expressions to list as well as
    ** names, if requested. */

    opc_x100_printf(x100cb, ", [");

    /* Previous loop may have dropped some entries from array, but remainder 
    ** will still be contiguous. */
    if (reorder)
    {
	rscount = (rscount - i) - 1;
	rsdmarray = &rsdmarray[i+1];
    }
    else rscount = i;

    for (i = 0; i < rscount; i++)
    {
	rsdmp = rsdmarray[i];
	/* If source type differs from result type, add coercion. This 
	** is determined now so name can be added to projection. */
	restype = abs(rsdmp->pst_sym.pst_dataval.db_datatype);
	srcetype = abs(rsdmp->pst_right->pst_sym.pst_dataval.db_datatype);
	if (!aggr && restype != srcetype && restype != 0 && srcetype != 0 && 
	    !opc_x100_unsupptype(restype) && !opc_x100_unsupptype(srcetype))
	{
	    status = adi_ficoerce(&x100cb->adfcb, srcetype, restype, &cfid);
	    if (status != E_DB_OK)
		opx_error(E_OP08C3_BADCOERCE);
	}
	else cfid = ADI_NILCOERCE;

	/* Check for VAR node inserted because of non-VAR appearance in a
	** projection. First projection computes it, 2nd (& subsequent) just
	** reference it by name. */
	nodep = rsdmp->pst_right;
	if (nodep && nodep->pst_sym.pst_type == PST_VAR && 
		nodep->pst_left != (PST_QNODE *) NULL 
		&& nodep->pst_sym.pst_value.pst_s_var.pst_vno == OPV_NOVAR && 
		nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == 
				OPZ_NOATTR)
	{
	    nodep = nodep->pst_left;
	    rsdmp->pst_right->pst_left = (PST_QNODE *) NULL;
	    if (nodep->pst_sym.pst_type == PST_CONST ||
		nodep->pst_sym.pst_type == PST_COP)
		rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |=
				PST_RS_CONST;
	}

	/* Check for tid being projected for DELETE/MODIFY - we don't 
	** rename those. */
	skiptid = FALSE;
	if (printname && (x100cb->flag & X100_DMLUPDEL) &&
		nodep->pst_sym.pst_type == PST_VAR &&
		nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id >= 0)
	{
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[nodep->
			pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	    eqcno = attrp->opz_equcls;

	    if ((eqcno >= 0 && subquery->ops_eclass.ope_base->
			ope_eqclist[eqcno]->ope_eqctype == OPE_TID ||
		(attrp && attrp->opz_mask & OPZ_X100TID)) &&
		(subquery->ops_vars.opv_rv <= 1 ||
		    nodep->pst_sym.pst_value.pst_s_var.pst_vno <= 0))
		skiptid = TRUE;
	}

	if (printname && !skiptid && nodep &&
		(nodep->pst_sym.pst_type != PST_VAR || 
		(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_TID) ||
		cfid != ADI_NILCOERCE ||
		MEcmp((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		    (char *)&nodep->pst_sym.pst_value.pst_s_var.
			pst_atname, sizeof(DB_ATT_NAME)) != 0) ||
		(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
			(PST_RS_RENAMED | PST_RS_PSEUDOTID)))
	{
	    /* These need the name from the RESDOM node - the rest have
 	    ** a VAR node to supply the name. */
	    opc_x100_copy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
			sizeof(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
			rsname, sizeof(rsname));
	    if (rsname[0] != 0 || /* not blank */
		(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
			PST_RS_NOQUAL))
	    {
		x100_print_id((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.
		    pst_rsname, DB_ATT_MAXNAME, var_name);
		opc_x100_printf(x100cb, var_name);
		if (((rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &
					PST_RS_RENAMED) &&
			nodep->pst_sym.pst_type != PST_AOP) ||
		    (nodep->pst_sym.pst_type == PST_CONST &&
		     nodep->pst_sym.pst_value.pst_s_cnst.
					pst_tparmtype == PST_RQPARAMNO &&
		     nodep->pst_sym.pst_value.pst_s_cnst.pst_parm_no >
			global->ops_qheader->pst_numparm) ||
		    (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &
					PST_RS_PSEUDOTID))
		{
		    /* If derived GROUP BY column, append DERIVED. */
		    if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
						PST_RS_DERIVED)
		    {
			opc_x100_printf(x100cb, " DERIVED");
			rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &= 
				~PST_RS_DERIVED; /* use once - turn off */
		    }
		    if (i < rscount-1)
	    		opc_x100_printf(x100cb, ", ");
		    continue;		/* just emit rsname for these */
		}

		opc_x100_printf(x100cb, " = ");
		rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |= 
				(PST_RS_NOQUAL | PST_RS_RENAMED);	
					/* no need to qual & RESDOM renamed */
	    }
	}

	opc_x100_prexpr(x100cb, cop, nodep, &rsdmp->pst_sym.pst_dataval, cfid);

	/* Hacky fix to restore expressions under PST_AOPs in case they 
 	** need to be evaluated again later (as in Q15). */
	if (nodep->pst_sym.pst_type == PST_AOP &&
	    nodep->pst_left != (PST_QNODE *) NULL &&
	    nodep->pst_left->pst_sym.pst_type == PST_VAR &&
	    nodep->pst_left->pst_right != (PST_QNODE *) NULL)
	    nodep->pst_left = nodep->pst_left->pst_right;
					/* copy expr back under PST_AOP */

	/* Another separate hacky fix to push RESDOM names into VAR nodes
 	** whose names may now be out of scope. This is a problem with 
	** TPC H q13. I'm not happy with this fix, but it doesn't seem
	** reparable in the main body of OPF where the problem arises. */
	if (printname && nodep->pst_sym.pst_type == PST_VAR &&
	    MEcmp((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		(char *)&nodep->pst_sym.pst_value.pst_s_var.pst_atname,
		sizeof(DB_ATT_NAME)) != 0)
	    MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
		sizeof(DB_ATT_NAME),
		(char *)&nodep->pst_sym.pst_value.pst_s_var.pst_atname);

	/* If derived GROUP BY column, append DERIVED. */
	if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_DERIVED)
	{
	    opc_x100_printf(x100cb, " DERIVED");
	    rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &= ~PST_RS_DERIVED;
						/* use once - turn off */
	}

	if (i < rscount-1)
	    opc_x100_printf(x100cb, ", ");
    }
    if ((x100cb->flag & X100_MODIFY) && subquery->ops_sqtype == OPS_MAIN)
	opc_x100_printf(x100cb, (i > 0) ? ", tid " : " tid ");
    opc_x100_printf(x100cb, "] ");

}

/*{
** Name: OPC_X100_DUPSELIMCK	- check for need to do dups elimination 
**	in CO-tree
**
** Description:	This function descends a CO-tree (in a subquery) setting (as
**	needed) the opo_dupselim field to indicate duplicates elimination 
**	on that node.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**	cop			Pointer to CO-node used to generate list.
**	deabove			Pointer to flag indicating we're already 
**				under a dupselim node.
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      5-mar-2010 (dougi)
**	    Written.
**	17-sep-2010 (dougi)
**	    Remove RESDOMs from sq unavailable because of semi/anti join.
**	13-dec-2010 (dougi)
**	    Add checks based in uniqueness properties held at node.
**	11-mar-2011 (dougi)
**	    No dups elimination for UNION view result with all ALL options.
**	29-june-2011 (dougi) m2154
**	    One more possible conversion to HashJoin01(). Can't be done
**	    in opjjoinop.c, so must be done here.
*/
static VOID
opc_x100_dupselimck(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop,
	bool		*deabove)

{
    PST_QNODE	*rsdmp, *prsdmp, *nodep;
    OPS_SUBQUERY *vtsubq;
    OPZ_ATTS	*attrp;
    OPZ_IATTS	attno, maxatts = subquery->ops_attrs.opz_av;
    OPV_IVARS	varno;
    OPE_IEQCLS	eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPE_BMEQCLS	eqcmapi, eqcmapd;
    OPV_BMVARS	*ivarmap;
    i4		i;
    bool	matched, antisemi, rev, locde, drop;

    /* Check conditions for dups elimination and set flag if they're met. */
    if (subquery->ops_bestco != cop && cop->opo_psort &&
		cop->opo_ordeqc > OPE_NOEQCLS)
    {
	cop->opo_dupselim = TRUE;
	locde = TRUE;
    }
    else locde = *deabove;

    /* Check for ORIG or PR on top of ORIG - then check for TID in list. No
    ** need to dups elim if TID is in projection. */
    if (cop->opo_sjpr == DB_ORIG)
	varno = cop->opo_union.opo_orig;
    else if (cop->opo_sjpr == DB_PR && cop->opo_outer->opo_sjpr == DB_ORIG)
	varno = cop->opo_outer->opo_union.opo_orig;
    else varno = -1;

    if (varno >= 0)
    {
	if (subquery->ops_vars.opv_base->opv_rt[varno]->opv_grv->opv_relation)
	 for (eqcno = -1; (eqcno = BTnext((i4)eqcno, 
			(char *)&cop->opo_maps->opo_eqcmap, maxeqcls)) >= 0; )
	  for (attno = -1; (attno = BTnext((i4)attno, 
			(char *)&subquery->ops_eclass.ope_base->
			ope_eqclist[eqcno]->ope_attrmap, maxatts)) >= 0; )
	   if ((attrp = subquery->ops_attrs.opz_base->opz_attnums[attno])->
			opz_varnm == varno &&
		attrp->opz_attnm.db_att_id == 0)
	   {
	    cop->opo_dupselim = FALSE;
	    return;
	   }
	if ((vtsubq = subquery->ops_vars.opv_base->opv_rt[varno]->
			opv_grv->opv_gquery) != (OPS_SUBQUERY *) NULL &&
		(vtsubq->ops_sqtype == OPS_VIEW ||
		vtsubq->ops_sqtype == OPS_UNION) &&
		vtsubq->ops_duplicates != OPS_DREMOVE)
	{
	    cop->opo_dupselim = FALSE;
	    return;
	}
    }

    /* Now check for FDs. CO-nodes projecting both sides of a FD don't
    ** need dups elimination. Loop over FDs looking for one in which all
    ** indep columns are projected by CO-node and remaining projected 
    ** columns are in dependent list. */

    /* This started as a test for all CO-nodes (joins included). But FDs
    ** are not enough to subsume dups in a join, though they are in an ORIG
    ** node. So the FD test is only applied to ORIG nodes.
    */

    /* For example, if the independent variables were keys of a table that 
    ** is the root of a referential hierarchy amongst the joined tables,
    ** dups elimination isn't required. In the case of Mantis 373, supplier
    ** is sibling to part and is not the root of a hierarchy. But part is
    ** root in its join - so the part lineitem join doesn't need dups
    ** elimination, but the bigger join does. */
    if (varno >= 0)
     for (i = 0; i < cop->opo_FDcount; i++)
     {
	/* Load the independent EQCs of an FD. */
	if ((eqcno = cop->opo_FDieqcs[i]) < maxeqcls)
	{
	    MEfill(sizeof(eqcmapi), 0, (char *)&eqcmapi);
	    BTset((i4)eqcno, (char *)&eqcmapi);
	}
	else if ((eqcno - maxeqcls) < subquery->ops_msort.opo_sv &&
		subquery->ops_msort.opo_base->opo_stable[eqcno - maxeqcls]->
			opo_bmeqcls != (OPE_BMEQCLS *) NULL)
	    MEcopy((char *)subquery->ops_msort.opo_base->opo_stable[
			eqcno - maxeqcls]->opo_bmeqcls, sizeof(eqcmapi),
			(char *)&eqcmapi);
	else continue;

	/* Load the dependent EQCs of an FD. */
	if ((eqcno = cop->opo_FDdeqcs[i]) < maxeqcls)
	{
	    MEfill(sizeof(eqcmapd), 0, (char *)&eqcmapd);
	    BTset((i4)eqcno, (char *)&eqcmapd);
	}
	else if ((eqcno - maxeqcls) < subquery->ops_msort.opo_sv &&
		subquery->ops_msort.opo_base->opo_stable[eqcno - maxeqcls]->
			opo_bmeqcls != (OPE_BMEQCLS *) NULL)
	    MEcopy((char *)subquery->ops_msort.opo_base->opo_stable[
			eqcno - maxeqcls]->opo_bmeqcls, sizeof(eqcmapd),
			(char *)&eqcmapd);
	else continue;

	/* First assure all independent columns are projected. */
	if (BTsubset((char *)&eqcmapi, (char *)&cop->opo_maps->opo_eqcmap,
		maxeqcls))
	{
	    /* Now assure projected columns are covered by (indep | dep). */
	    BTor((i4)maxeqcls, (char *)&eqcmapi, (char *)&eqcmapd);
	    if (BTsubset((char *)&cop->opo_maps->opo_eqcmap, (char *)&eqcmapd,
		    maxeqcls))
		matched = TRUE;
	    else matched = FALSE;
	}
	else matched = FALSE;

	if (matched)
	{
	    /* FD removes need for dups elimination. */
	    cop->opo_dupselim = FALSE;
	    if (varno >= 0)
		return;				/* if ORIG, no more to do */
	}
     }
	    
    /* Remaining tests are for joins. First recurse, then look for 
    ** interesting things - like anti/semi joins whose inner inputs 
    ** don't require dups elimination. */
    if (cop->opo_outer && varno < 0)
	opc_x100_dupselimck(subquery, cop->opo_outer, &locde);
    if (cop->opo_inner)
	opc_x100_dupselimck(subquery, cop->opo_inner, &locde);

    if (cop->opo_sjpr == DB_SJ && STcasecmp(cop->opo_variant.opo_local->
		opo_x100jt, "HashJoinN") == 0 && cop->opo_inner->opo_dupselim)
    {
	/* We may yet be able to change this to a Join01. More tests may
 	** be required for this bit of code, but it works for all tests
	** so far. */
	cop->opo_variant.opo_local->opo_x100jt = "HashJoin01";

	/* The conversion to HashJoin01() legitimizes propagation of
 	** outer source unique properties. */
	if (cop->opo_ucount == 0)
	{
	    cop->opo_ucount = cop->opo_outer->opo_ucount;
	    cop->opo_ueqcs = cop->opo_outer->opo_ueqcs;
	}
    }

    /* HashJoin01 conversion above may add unique properties. */
    if (cop->opo_sjpr == DB_RSORT && cop->opo_ucount == 0)
    {
	cop->opo_ucount = cop->opo_outer->opo_ucount;
	cop->opo_ueqcs = cop->opo_outer->opo_ueqcs;
    }

    /* See if this node has uniqueness properties that would obviate need
    ** for duplicates elimination. */
    for (i = 0; i < cop->opo_ucount; i++)
     if (((eqcno = cop->opo_ueqcs[i]) < maxeqcls && 
		BTtest(eqcno, (char *)&cop->opo_maps->opo_eqcmap)) ||
	(eqcno >= maxeqcls && BTsubset((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, 
		(char *)&cop->opo_maps->opo_eqcmap, maxeqcls)))
     {
	cop->opo_dupselim = FALSE;
	break;
     }

    if (cop->opo_sjpr == DB_SJ)
    {
	/* Checks for anti/semi-joins. Inners don't need dups elimination. */
	antisemi = rev = FALSE;
	if (cop->opo_variant.opo_local->opo_mask & 
					(OPO_ANTIJOIN | OPO_SEMIJOIN))
	    antisemi = TRUE;
	if (cop->opo_variant.opo_local->opo_mask & OPO_REVJOIN)
	    rev = TRUE;
	if (antisemi)
	{
	    if (rev)
	    {
		cop->opo_outer->opo_dupselim = FALSE;
		if (cop->opo_outer->opo_sjpr == DB_PR)
		    cop->opo_outer->opo_outer->opo_dupselim = FALSE;
		ivarmap = cop->opo_outer->opo_variant.opo_local->opo_bmvars;
	    }
	    else 
	    {
		cop->opo_inner->opo_dupselim = FALSE;
		if (cop->opo_inner->opo_sjpr == DB_PR)
		    cop->opo_inner->opo_outer->opo_dupselim = FALSE;
		ivarmap = cop->opo_inner->opo_variant.opo_local->opo_bmvars;
	    }

	    /* Fiddle the sq's RESDOM list to remove anything dependent on
 	    ** the inner of the anti/semi. */
	    for (rsdmp = subquery->ops_root->pst_left, 
					prsdmp = (PST_QNODE *) NULL;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		prsdmp = (drop) ? prsdmp : rsdmp, rsdmp = rsdmp->pst_left)
	    {
		drop = FALSE;
		if ((nodep = rsdmp->pst_right)->pst_sym.pst_type != PST_VAR)
		    continue;			/* only VARs for now */
		if (rsdmp->pst_sym.pst_value.pst_s_rsdm.
						pst_rsflags & PST_RS_PRINT)
		    continue;			/* only non-prints for now */

		if (!BTtest((i4)nodep->pst_sym.pst_value.pst_s_var.pst_vno,
			(char *)ivarmap))
		    continue;			/* col is in outer side */

		/* Got one - drop it from RESDOM list. */
		if (prsdmp == (PST_QNODE *) NULL)
		    subquery->ops_root->pst_left = rsdmp->pst_left;
		else prsdmp->pst_left = rsdmp->pst_left;
		drop = TRUE;
	    }
	}

	/* Also - if there is a dups elimination above this one in the 
	** CO-tree, we can get rid of the current (I think - this may not
	** be right, but should become evident in testing). */
	if (*deabove)
	    cop->opo_dupselim = FALSE;
    }

}

/*{
** Name: OPC_X100_MERGENKEY	- emit predicates for merge join non keys
**
** Description:	This function emits a sequence of &&'d =='s for non key
**	predicates of a merge join. Just runs through the opo_nkeqcmap.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**	cop			Pointer to CO-node for which we're doing 
**				preds.
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      18-mar-2010 (dougi)
**	    Written.
**	30-june-2010 (dougi)
**	    Oops. Forgot to close the &&'s.
**	4-july-2011 (dougi)
**	    Replace pst_resolve() by opa_resolve().
**	29-Jul-2011 (kiria01) b125613
**	    Updated call interface of opa_resolve reflecting
**	    pst_resolve change.
*/
static VOID
opc_x100_mergenkey(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)

{
    OPS_STATE	*global = subquery->ops_global;
    PST_QNODE	*eqptr, *var1p, *var2p;
    OPO_CO	*outer = cop->opo_outer, *inner = cop->opo_inner;
    OPE_BMEQCLS	*keqcmap = cop->opo_variant.opo_local->opo_nkeqcmap;
    OPV_GRV	*gvarp;
    DB_DATA_VALUE dummy_dbval;
    OPE_IEQCLS	eqcno;
    OPE_EQCLIST	*eqclp;
    OPZ_IATTS	attno;
    OPZ_ATTS	*attrp;
    DB_ATT_ID	dummy_attid;
    DB_STATUS	status;
    DB_ERROR	error;
    i4		pcnt = BTcount((char *)keqcmap, subquery->ops_eclass.ope_ev);
    i4		i;
    bool	in, out;

    /* Print needed no. of &&'s, then loop over EQCs. */
    opc_x100_printf(x100cb, ", ");
    for (i = 0; i < pcnt-1; i++)
	opc_x100_printf(x100cb, "&& (");

    /* Allocate "=" parse tree node and 2 VAR nodes. Then we can call
    ** opc_x100_prexpr() to do the work. */
    dummy_attid.db_att_id = -1;
    eqptr = opv_opnode(global, PST_BOP, global->ops_cb->ops_server->opg_eq,
			OPL_NOOUTER);
    var1p = opv_varnode(global, &dummy_dbval, (OPV_IGVARS)-1, &dummy_attid);
    var2p = opv_varnode(global, &dummy_dbval, (OPV_IGVARS)-1, &dummy_attid);
    eqptr->pst_left = var1p;
    eqptr->pst_right = var2p;

    for (eqcno = -1, i = 0; i < pcnt; i++)
    {
	/* Get next EQC from map. */
	eqcno = BTnext((i4)eqcno, (char *)keqcmap, 
				subquery->ops_eclass.ope_ev);
	eqclp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];

	/* Loop over ope_attrmap finding outer and inner attrs for 
	** "=" operands. */
	for (attno = -1, out = in = FALSE; (!out || !in) &&
		(attno = BTnext((i4)attno, (char *)&eqclp->ope_attrmap,
			subquery->ops_attrs.opz_av)) >= 0; )
	{
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];
	    if (!out && BTtest((i4)attrp->opz_varnm, 
			(char *)outer->opo_variant.opo_local->opo_x100bmvars))
	    {
		out = TRUE;
		var1p->pst_sym.pst_value.pst_s_var.pst_vno = 
					attrp->opz_varnm;
		var1p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
					attrp->opz_attnm.db_att_id;
		STRUCT_ASSIGN_MACRO(attrp->opz_dataval, 
					var1p->pst_sym.pst_dataval);
		if (attrp->opz_attnm.db_att_id > 0)
		{
		    gvarp = subquery->ops_vars.opv_base->
					opv_rt[attrp->opz_varnm]->opv_grv;
		    cui_move(gvarp->opv_relation->rdr_attr[attrp->opz_attnm.db_att_id]->att_nmlen,
		    	gvarp->opv_relation->rdr_attr[attrp->opz_attnm.db_att_id]->att_nmstr,
			' ', DB_ATT_MAXNAME,
			(char *)&var1p->pst_sym.pst_value.pst_s_var.pst_atname);
		}
		else
		    MEmove(STlen(tid32), (char*)tid32, ' ',
			sizeof(var1p->pst_sym.pst_value.pst_s_var.pst_atname),
		    (char *)&var1p->pst_sym.pst_value.pst_s_var.pst_atname);
		
	    }
	    if (!in && BTtest((i4)attrp->opz_varnm, 
			(char *)inner->opo_variant.opo_local->opo_x100bmvars))
	    {
		in = TRUE;
		var2p->pst_sym.pst_value.pst_s_var.pst_vno = 
					attrp->opz_varnm;
		var2p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id =
					attrp->opz_attnm.db_att_id;
		STRUCT_ASSIGN_MACRO(attrp->opz_dataval, 
					var2p->pst_sym.pst_dataval);
		if (attrp->opz_attnm.db_att_id > 0)
		{
		    gvarp = subquery->ops_vars.opv_base->
					opv_rt[attrp->opz_varnm]->opv_grv;
		    cui_move(gvarp->opv_relation->rdr_attr[attrp->opz_attnm.db_att_id]->att_nmlen,
		    	gvarp->opv_relation->rdr_attr[attrp->opz_attnm.db_att_id]->att_nmstr,
			' ', DB_ATT_MAXNAME,
			(char *)&var2p->pst_sym.pst_value.pst_s_var.pst_atname);
		}
		else
		    MEmove(STlen(tid32), (char*)tid32, ' ',
			sizeof(var2p->pst_sym.pst_value.pst_s_var.pst_atname),
			(char *)&var2p->pst_sym.pst_value.pst_s_var.pst_atname);
	    }
	}

	opa_resolve(subquery, &eqptr);

	opc_x100_prexpr(x100cb, cop, eqptr, (DB_DATA_VALUE *)NULL, 
					ADI_NILCOERCE);

	if (i < pcnt - 1)
	    opc_x100_printf(x100cb, ", ");	/* "," if still more */
    }
    /* Close ")"s for &&'s. */
    for (i = 0; i < pcnt-1; i++)
	opc_x100_printf(x100cb, ") ");

    opc_x100_printf(x100cb, ") ");		/* wrap up the Select() */
}

/*{
** Name: OPC_X100_IFNULLC	- emit type dependent constant for "ifnull"
**
** Description:	This function emits the 2nd parm for the "ifnull" used to
**	implement null joins. 
**
** Inputs:
**	dtype			- type of 1st parm of "ifnull"
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      3-sep-2010 (dougi)
**	    Written.
*/
static VOID
opc_x100_ifnullc(
	X100_STATE	*x100cb,
	DB_DT_ID	dtype)

{

    /* Switch to the appropriate data type and emit the value. Note the
    ** "))" at the end to close the "ifnull" function. */
    switch(abs(dtype)) {
	case DB_CHR_TYPE:
	case DB_CHA_TYPE:
	case DB_TXT_TYPE:
	case DB_VCH_TYPE:
	case DB_NCHR_TYPE:
	case DB_NVCHR_TYPE:
       	case DB_BYTE_TYPE:
       	case DB_VBYTE_TYPE:
	    opc_x100_printf(x100cb, ", str(' '))");
	    break;
	case DB_INT_TYPE:
	case DB_FLT_TYPE:
	case DB_DEC_TYPE:
	    opc_x100_printf(x100cb, ", sint('0'))");
	    break;
	case DB_ADTE_TYPE:
	case DB_DTE_TYPE:
	    opc_x100_printf(x100cb, ", date('1-1-1'))");
	    break;
	case DB_INYM_TYPE:
	    opc_x100_printf(x100cb, ", intervalym('0-0'))");
	    break;
	case DB_INDS_TYPE:
	    opc_x100_printf(x100cb, ", intervalds('0 0:0:0'))");
	    break;
	default:
	    opc_x100_printf(x100cb, ", uchr('0'))");
	    break;
     }
}

/*{
** Name: OPC_X100_FDLIST	- emit FD properties
**
** Description:	This function produces a list functional dependencies for
**	the current CO-node as a property of the node used by X100 for
**	various purposes.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**	cop			Pointer to CO-node used to generate list.
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      7-may-2010 (dougi)
**	    Written.
*/
static VOID
opc_x100_fdlist(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop)

{
    OPE_BMEQCLS	imap, dmap;
    OPE_IEQCLS	ieqcls, deqcls, maxeqcls = subquery->ops_eclass.ope_ev;
    OPV_BMVARS	*savemap = cop->opo_variant.opo_local->opo_x100bmvars;
    i4		i;
    bool	firstFD = TRUE;

    if (TRUE || cop->opo_FDcount <= 0) return;

    /* opo_x100bmvars isn't set up when this function is called. opo_bmvars
    ** is a reasonable approximation. */
    cop->opo_variant.opo_local->opo_x100bmvars = cop->opo_variant.
		opo_local->opo_bmvars;

    /* Loop over FDs, adding each in turn to query. */
    for (i = 0; i < cop->opo_FDcount; i++)
    {
	ieqcls = cop->opo_FDieqcs[i];
	if (ieqcls == OPE_NOEQCLS)
	    continue;			/* these sometimes happen */

	deqcls = cop->opo_FDdeqcs[i];
	if (deqcls == OPE_NOEQCLS)
	    continue;

	/* Make bit map of independent/dependent EQCs of current FD, even
	** if single EQclass. */
	MEfill(sizeof(imap), 0, (char *)&imap);
	MEfill(sizeof(dmap), 0, (char *)&dmap);
	if (ieqcls < maxeqcls)
	    BTset((i4)ieqcls, (char *)&imap);
	else MEcopy(subquery->ops_msort.opo_base->opo_stable[ieqcls-maxeqcls]->
			opo_bmeqcls, sizeof(imap), (char *)&imap);
	if (deqcls < maxeqcls)
	    BTset((i4)deqcls, (char *)&dmap);
	else MEcopy(subquery->ops_msort.opo_base->opo_stable[deqcls-maxeqcls]->
			opo_bmeqcls, sizeof(dmap), (char *)&dmap);

	/* Now loop over independent/dependent maps, emitting the stuff. */
	if (firstFD)
	{
	    /* First one - print start stuff. */
	    opc_x100_printf(x100cb, ", 'FD' = ");
	    firstFD = FALSE;
	}
	opc_x100_printf(x100cb, "'(");
	opc_x100_colmap(x100cb, subquery, cop, (OPO_CO *) NULL, &imap, 
							FALSE, FALSE, FALSE);
	opc_x100_printf(x100cb, ") -> (");
	opc_x100_colmap(x100cb, subquery, cop, (OPO_CO *) NULL, &dmap, 
							FALSE, FALSE, FALSE);
	opc_x100_printf(x100cb, ")'");
    }

    cop->opo_variant.opo_local->opo_x100bmvars = savemap;

}

/*{
** Name: OPC_X100_DERCKEQC	- build bit map of "derived" columns
**
** Description:	This function Produces a bit map of EQCs from a dups 
**	elimination list triggered by a CO-node which can be tagged as
**	DERIVED.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**	cop			Pointer to CO-node used to generate list.
**	eqcmap			Map of EQCs to emit.
**	dermapp			Pointer to bit map to contain "derived" EQCs 
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      5-nov-2010 (dougi)
**	    Written - but analogous to opc_x100_derck().
*/
static VOID
opc_x100_derckeqc(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop,
	OPE_BMEQCLS	*eqcmap,
	OPE_BMEQCLS	*dermapp)

{
    OPE_BMEQCLS		ieqcmap, deqcmap, weqcmap;
    OPE_IEQCLS	eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    i4		i;

    MEfill(sizeof(*dermapp), 0, (char *)dermapp);	/* init */

    for (i = 0; i < cop->opo_FDcount; i++)
    {
	/* Load independent EQCs to a map. */
	if ((eqcno = cop->opo_FDieqcs[i]) < maxeqcls)
	{
	    MEfill(sizeof(ieqcmap), 0, (char *)&ieqcmap);
	    BTset (eqcno, (char *)&ieqcmap);
	}
	else if ((eqcno - maxeqcls) < subquery->ops_msort.opo_sv &&
		subquery->ops_msort.opo_base->opo_stable[eqcno - maxeqcls]->
			opo_bmeqcls != (OPE_BMEQCLS *) NULL)
	    MEcopy((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, 
		sizeof(ieqcmap), (char *)&ieqcmap);
	else continue;

	/* They must be a subset of the dups elim column set to be useful. */
	if (!BTsubset((char *)&ieqcmap, (char *)eqcmap, maxeqcls))
	    continue;

	/* Load dependent EQCs to a map. */
	if ((eqcno = cop->opo_FDdeqcs[i]) < maxeqcls)
	{
	    MEfill(sizeof(deqcmap), 0, (char *)&deqcmap);
	    BTset (eqcno, (char *)&deqcmap);
	}
	else if ((eqcno - maxeqcls) < subquery->ops_msort.opo_sv &&
		subquery->ops_msort.opo_base->opo_stable[eqcno - maxeqcls]->
			opo_bmeqcls != (OPE_BMEQCLS *) NULL)
	    MEcopy((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, 
		sizeof(deqcmap), (char *)&deqcmap);
	else continue;

	/* Identify dependent columns in dups elim. column set and roll into
	** accumulated DERIVED set. */
	BTand(BITS_IN(deqcmap), (char *)eqcmap, (char *)&deqcmap);
	BTor(BITS_IN(deqcmap), (char *)&deqcmap, (char *)dermapp);
    }

    /* Last pass is to look for EQCs in the Boolean factor constant list
    **(i.e., they are guaranteed to be constant because of a predicate). */
    if (subquery->ops_bfs.opb_bfeqc)
     for (eqcno = -1; (eqcno = BTnext(eqcno, (char *)subquery->
				ops_bfs.opb_bfeqc, maxeqcls)) >= 0; )
      if (BTtest(eqcno, (char *)eqcmap))
	BTset(eqcno, (char *)dermapp);

}

/*{
** Name: OPC_X100_COLMAP	- drive emission of list of columns
**
** Description:	This function produces a list of qualified columns from
**	an EQC map. It is currently used to produce the key columns for 
**	the inner and outer source of a non-merge join, and to produce the
**	list of columns on which to perform a duplicates removal aggregation.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**	cop			Pointer to CO-node used to generate list.
**	jcop			Pointer to CO-node above cop (for joinkeys)
**	eqcmap			Map of EQCs to emit.
**	joinkeys		Flag - TRUE if calling to build join key list.
**	njproj			Flag - TRUE if calling to project prior to
**				nulljoin.
**	draggr		draggr call (emit list for dups elimination)
**				and from outer join
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      14-jan-2010 (dougi)
**	    Written.
**	2-sept-2010 (dougi)
**	    Separate "collist"/"colmap" from "colemit" to increase flexibility.
**	3-nov-2010 (dougi)
**	    Add draggr parm to handle OJs.
**	5-nov-2010 (dougi)
**	    Add code to set DERIVED on dups elimination aggs on CO nodes.
*/
static VOID
opc_x100_colmap(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop,
	OPO_CO		*jcop,
	OPE_BMEQCLS	*eqcmap,
	bool		joinkeys,
	bool		njproj,
	bool		draggr)
{
    OPE_BMEQCLS dermap, *dermapp;
    OPE_IEQCLS	eqcno;
    bool	not1st;


    /* If this is the null join projection, prepend ", {". */
    if (njproj)
	opc_x100_printf(x100cb, ", [");

    /* For dups elimination calls, determine DERIVED entries. */
    if (draggr)
    {
	opc_x100_derckeqc(subquery, cop, eqcmap, &dermap);
	if (BTcount((char *)&dermap, subquery->ops_eclass.ope_ev))
	    dermapp = &dermap;
	else dermapp = (OPE_BMEQCLS *) NULL;
    }
    else dermapp = (OPE_BMEQCLS *) NULL;

    /* Loop over map, emitting columns as we go. not1st just indicates 
    ** when to prepend a comma into the emitted list. */
    for (eqcno = -1, not1st = FALSE; 
	(eqcno = BTnext(eqcno, (char *)eqcmap, 
				subquery->ops_eclass.ope_ev)) >= 0; )
     opc_x100_colemit(x100cb, subquery, cop, jcop, eqcno, &not1st, joinkeys, 
					njproj, draggr, dermapp);

    /* Close Project with "] )". */
    if (njproj)
	opc_x100_printf(x100cb, "] )");

}

/*{
** Name: OPC_X100_COLLIST	- drive emission of list of columns
**
** Description:	This function produces a list of qualified columns from
**	an EQC number. It is currently used to produce the ordered list of 
**	key columns for the inner and outer source of a merge join.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**	cop			Pointer to CO-node used to generate list.
**	eqcno			Eqcno to emit - could be a multi-EQC list.
**	joinkeys		Flag - TRUE if calling to build join key list.
**	njproj			Flag - TRUE if calling to project prior to
**				nulljoin.
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      14-jan-2010 (dougi)
**	    Written.
**	2-sept-2010 (dougi)
**	    Separate "collist" from "colemit" to increase flexibility.
*/
static VOID
opc_x100_collist(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop,
	OPE_IEQCLS	eqcno,
	bool		joinkeys,
	bool		njproj)
{
    OPO_SORT	*orderp;
    OPE_IEQCLS	maxeqcls = subquery->ops_eclass.ope_ev;
    i4		i;
    bool	not1st;


    /* Determine whether eqcno is single EQC or multi-attr list and setup
    ** loop accordingly. */
 
    if (eqcno < maxeqcls)
	orderp = (OPO_SORT *) NULL;
    else
    {
	orderp = subquery->ops_msort.opo_base->opo_stable[eqcno - maxeqcls];
	eqcno = orderp->opo_eqlist->opo_eqorder[0];
	i = 0;
    }

    /* If this is the null join projection, prepend ", {". */
    if (njproj)
	opc_x100_printf(x100cb, ", [");

    for (not1st = FALSE; ; )
    {
	opc_x100_colemit(x100cb, subquery, cop, (OPO_CO *) NULL, eqcno, 
			&not1st, joinkeys, njproj, FALSE, (OPE_BMEQCLS *) NULL);

	if (orderp == (OPO_SORT *) NULL)
	    return;			/* single EQC - exit now */

	/* Multi-attr - keep going 'til we hit terminator. */
	i++;
	if ((eqcno = orderp->opo_eqlist->opo_eqorder[i]) <= OPE_NOEQCLS)
	    return;
    }

    /* Close Project with "] )". */
    if (njproj)
	opc_x100_printf(x100cb, "] )");
} 


/*{
** Name: OPC_X100_COLEMIT	- emit single column/expr
**
** Description:	This function produces a qualified column from an eqcno.
**	It is currently used to produce the key columns for 
**	the inner and outer source of a join, and to produce the list
**	of columns on which to perform a duplicates removal aggregation.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**	cop			Pointer to CO-node used to generate column.
**	jcop			Pointer to CO-node above (for joinkeys).
**	eqcno			Eqcno to emit.
**	not1st			Ptr to flag indicating when a col has already
**				been added to list (and therefore to prepend
**				subsequent entries with comma).
**	joinkeys		Flag - TRUE if calling to build join key list.
**	njproj			Flag - TRUE if calling to project prior to
**				nulljoin.
**	draggr			draggr call (emit list for dups elimination)
**				and from outer join
**	dermapp			Pointer to list of EQCs for derived columns,
**				if this is a dups elimination list
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      2-sep-2010 (dougi)
**	    Cloned from old opc_x100_collist() to emit one column at a time.
**	3-nov-2010 (dougi)
**	    Add draggr parm to handle dups elim from OJs when cols on either
**	    side of join in same EQC are required.
**	5-nov-2010 (dougi)
**	    Add code to set DERIVED on dups elimination aggs on CO nodes.
**	30-nov-2010 (dougi)
**	    Spiff up OJ logic above.
**	18-feb-2011 (dougi)
**	    Further spiffing of OJ logic.
**	7-mar-2011 (dougi)
**	    Look more precisely for corrent key column for joins in a subq
**	    that has OJs (don't want column frpom outer table when query 
**	    specified column from inner side of OJ).
**	8-mar-2011 (dougi)
**	    More completely qualify IIIFNULL/IIISNULL pseudocols to remove
**	    potential ambiguity.
**	2-may-2011 (dougi) m1921
**	    More careful handling of "names" for results of FAs.
*/
static bool
opc_x100_colemit(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop,
	OPO_CO		*jcop,
	OPE_IEQCLS	eqcno,
	bool		*not1st,
	bool		joinkeys,
	bool		njproj,
	bool		draggr,
	OPE_BMEQCLS	*dermapp)
{
    OPZ_BMATTS	*attrmap, doneatts;
    OPZ_ATTS	*attrp, *attr1p;
    OPZ_FATTS	*fattrp;
    OPV_VARS	*varp = (OPV_VARS *) NULL;
    OPV_GRV	*gvarp;
    OPZ_IATTS	attrno, attr1no;
    OPE_EQCLIST	*eqclp;
    PST_QNODE	*rsdmp, *nodep, *bopp;
    OPV_BMVARS	varmap, *varmapp = &varmap;
    OPL_OUTER	*ojp;
    OPB_BOOLFACT *bfp;
    OPO_CO	*ocop;
    i4		i;
    char	nfuncs[X100_MAXNAME];
    char	var_name[X100_MAXNAME];
    bool	gotcol, gotatleastone = FALSE, gotpref = FALSE, gotoj = FALSE;
    bool	gotojcol = FALSE, derived;


    if (dermapp && BTtest(eqcno, (char *)dermapp))
	derived = TRUE;
    else derived = FALSE;

    eqclp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];
    attrmap = &eqclp->ope_attrmap;

    /* If there are OJs in this OPS_SUBQUERY, build "outers" map so we
    ** project all instances of OJ columns (i.e., those from both sides
    ** of the outer joins). 
    **
    ** Also make sure we emit the right key column for the OJ. */

    MEfill(sizeof(varmap), (u_char)0, (char *)&varmap);
    MEfill(sizeof(doneatts), (u_char)0, (char *)&doneatts);
    attrno = -1;

    /* "ocop" is the other input CO to a join. */
    if (jcop != (OPO_CO *) NULL)
     if (jcop->opo_outer == cop)
	ocop = jcop->opo_inner;
     else ocop = jcop->opo_outer;
    else ocop = (OPO_CO *) NULL;

    if (joinkeys && subquery->ops_oj.opl_lv > 0 && subquery->ops_bfs.opb_bv > 0)
    {
	/* Emitting keys for a subquery with OJs. Make sure we have the
	** correct column. If we find a column here, all the later looping
	** over attrs and RESDOMs is unnecessary. */
	for (i = 0; i < subquery->ops_bfs.opb_bv; i++)
	{
	    /* Look for equijoin BF that matches this eqclass and the 
	    ** current join context. */
	    bfp = subquery->ops_bfs.opb_base->opb_boolfact[i];
	    if (bfp->opb_eqcls == eqcno && 
		(bopp = bfp->opb_qnode)->pst_sym.pst_type == PST_BOP &&
		bopp->pst_sym.pst_value.pst_s_op.pst_opno ==
			subquery->ops_global->ops_cb->ops_server->opg_eq &&
		bopp->pst_left->pst_sym.pst_type == PST_VAR &&
		bopp->pst_right->pst_sym.pst_type == PST_VAR)
	    {
		if (BTtest(bopp->pst_left->pst_sym.pst_value.pst_s_var.pst_vno,
			(char *)cop->opo_variant.opo_local->opo_bmvars) &&
		    BTtest(bopp->pst_right->pst_sym.pst_value.pst_s_var.pst_vno,
			(char *)ocop->opo_variant.opo_local->opo_bmvars))
		{
		    attrno = bopp->pst_left->pst_sym.pst_value.pst_s_var.
				pst_atno.db_att_id;
		    gotcol = gotojcol = gotatleastone = TRUE;
		    break;
		}

		if (BTtest(bopp->pst_left->pst_sym.pst_value.pst_s_var.pst_vno,
			(char *)ocop->opo_variant.opo_local->opo_bmvars) &&
		    BTtest(bopp->pst_right->pst_sym.pst_value.pst_s_var.pst_vno,
			(char *)cop->opo_variant.opo_local->opo_bmvars))
		{
		    attrno = bopp->pst_right->pst_sym.pst_value.pst_s_var.
				pst_atno.db_att_id;
		    gotcol = gotojcol = gotatleastone = TRUE;
		    break;
		}
	    }
	}
    }
    else if (subquery->ops_oj.opl_lv > 0 && draggr && cop->opo_sjpr == DB_SJ)
     for (i = 0; i < subquery->ops_oj.opl_lv; i++)
     {
	/* Build varmap from opl_ivmap's of "real" outer joins (left,
	** right, full, non-semi/anti/subq. */
	ojp = subquery->ops_oj.opl_base->opl_ojt[i];
	if (!(ojp->opl_mask & (OPL_ANTIJOIN | OPL_SEMIJOIN | OPL_SUBQJOIN)) &&
	    (ojp->opl_type == OPL_LEFTJOIN || ojp->opl_type == OPL_RIGHTJOIN ||
		    ojp->opl_type == OPL_FULLJOIN) &&
	    ojp->opl_bvmap)
	{
	    gotoj = TRUE;
	    BTor(BITS_IN(varmap), (char *)ojp->opl_bvmap, (char *)&varmap);
	}
     }
    if (!gotoj)
	varmapp = (OPV_BMVARS *) NULL;

    for ( ; ; )					/* something to break from */
    {
	/* Loop over vars in requested eqc looking for one in supplied
	** subtree. */
	if (gotojcol)
	{
	    attr1no = attrno;
	    attrp = attr1p = subquery->ops_attrs.opz_base->opz_attnums[attrno];
	}
	else for (attrno = -1, attr1p = (OPZ_ATTS *) NULL, gotcol = FALSE;
		(attrno = BTnext(attrno, (char *)attrmap, 
			subquery->ops_attrs.opz_av)) >= 0; )
	  if (BTtest((attrp = subquery->ops_attrs.opz_base->
			opz_attnums[attrno])->opz_varnm,
		(char *)cop->opo_variant.opo_local->opo_x100bmvars))
	  {
	    if (BTtest((i4)attrno, (char *)&doneatts))
		continue;			/* already done this one */

	    if (attr1p == (OPZ_ATTS *) NULL)
	    {
		attr1p = attrp;			/* save the first match */
		attr1no = attrno;
	    }

	    if (varmapp && BTtest(attrp->opz_varnm, (char *)varmapp))
	    {
		/* If this attr is in a var in the OJ map, take it without
		** further consideration. */
		attr1p = attrp;
		attr1no = attrno;
		/* Clear variable this column comes from. */
		if (attrp->opz_varnm >= 0)
		    BTclear(attrp->opz_varnm, (char *)varmapp);
		gotcol = gotatleastone = TRUE;
		break;
	    }
	    else if (varmapp && gotpref)
		continue;

	    if (attrp->opz_func_att >= 0 && (subquery->ops_funcs.opz_fbase->
			opz_fatts[attrp->opz_func_att]->opz_mask & OPZ_OJFA))
		continue;

	    /* The following loop is the trick in this function. Look 
	    ** through RESDOM list of containing subquery for this attr.
	    ** It is preferred column, since it may be used to reference 
	    ** the current EQC from outside the subquery.
	    **
	    ** The loop is executed whether OJs are involved or not. We
	    ** always look for the preferred column. */
	    if (!joinkeys && subquery->ops_root != (PST_QNODE *) NULL)
	     for (rsdmp = subquery->ops_root->pst_left;
			rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM && 
			!gotcol; rsdmp = rsdmp->pst_left)
	      if ((nodep = rsdmp->pst_right) && 
		    opc_x100_varsearch(nodep, attrno, attrp->opz_varnm) &&
		(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_PRINT))
		    gotcol = gotpref = TRUE;
		
	    if (gotcol)
	    {
		attr1p = attrp;
		attr1no = attrno;
		break;
	    }
	  }
	attrp = attr1p;
	attrno = attr1no;
	if (attrno >= 0 && attrp->opz_attnm.db_att_id >= 0)
	    varp = subquery->ops_vars.opv_base->opv_rt[attrp->opz_varnm];

	if (joinkeys && eqclp->ope_nulljoin)
	{
	    /* Emit special funcs to key list to replace original key
	    ** column/expression. */
	    STprintf(&nfuncs[0], "IIISNULLFA_%-u_%-u, IIIFNULLFA_%-u_%-u",
			eqcno, varp->opv_gvar, eqcno, varp->opv_gvar);
	    if (*not1st)
		opc_x100_printf(x100cb, ", ");
	    opc_x100_printf(x100cb, nfuncs);
	    *not1st = TRUE;
	    return(TRUE);
	}

	BTset((i4)attrno, (char *)&doneatts);

	if (varmapp != (OPV_BMVARS *) NULL && !gotcol && gotatleastone)
	    return(TRUE);
	else if (varmapp != (OPV_BMVARS *) NULL && !gotcol && 
		attrp == (OPZ_ATTS *) NULL)
	    return(FALSE);

	/* We can get here with an attr that matches a table in the OJ
 	** var map, an attr that is the "preferred" one for the EQC (ref'ed
	** in the subquery's RESDOM list) or if neither of those conditions
	** apply, the first attr in the EQC (that can happen when the column 
	** is in the key list of as join, but not required higher in the
	** query). */

	if (attrp == (OPZ_ATTS *) NULL && (eqclp->ope_mask & OPE_REFNOJOINIX))
	    return(FALSE);			/* skip bogus EQC */

	if (attrp->opz_func_att >= 0 && (subquery->ops_funcs.opz_fbase->
			opz_fatts[attrp->opz_func_att]->opz_mask & OPZ_OJFA))
	    return(FALSE);			/* OJ specl EQCs don't count */

	if (*not1st)
	    opc_x100_printf(x100cb, ", ");	/* prepend with comma */

	/* attrp now addresses a OPZ_ATTS in our subtree in
	** the same EQC as the current join key. */
	if (attrno >= 0 && attrp->opz_attnm.db_att_id >= 0)
	{
	    /* Copy column name to var_name[] and set 0x0. */
	    gvarp = varp->opv_grv;
	    if (gvarp->opv_gsubselect != (OPV_GSUBSELECT *) NULL ||
		gvarp->opv_gquery != (OPS_SUBQUERY *) NULL)
	    {
		OPS_SUBQUERY	*subq = (gvarp->opv_gquery) ? 
		    gvarp->opv_gquery : gvarp->opv_gsubselect->opv_subquery;

		for (rsdmp = subq->ops_root->pst_left;
			rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
			rsdmp = rsdmp->pst_left)
		 if (rsdmp->pst_sym.pst_value.pst_s_rsdm.
				pst_rsno == attrp->opz_attnm.db_att_id)
		    break;

		x100_print_id((char *)&rsdmp->pst_sym.pst_value.
			pst_s_rsdm.pst_rsname, DB_ATT_MAXNAME, var_name);
	    }
	    else if (attrp->opz_attnm.db_att_id > 0)
		x100_print_id(gvarp->opv_relation->
			rdr_attr[attrp->opz_attnm.db_att_id]->att_nmstr,
			gvarp->opv_relation->
			rdr_attr[attrp->opz_attnm.db_att_id]->att_nmlen,
			var_name);
	    else if (attrp->opz_attnm.db_att_id == 0)
	     if (attrp->opz_varnm == 0 && (x100cb->flag & X100_DMLUPDEL))
		    opc_x100_copy("tid", sizeof("tid"), var_name, sizeof(var_name));
	     else
		opc_x100_copy(X100_TIDNAME, sizeof(X100_TIDNAME),
				var_name, sizeof(var_name));

	    /* Prepend correlation name, if applicable. */
	    opc_x100_prepcorr(x100cb, subquery, gvarp, varp, attrp);
	}
	else if (attrp->opz_attnm.db_att_id == OPZ_FANUM)
	{
	    /* But first, prepend correlation name, if appl. */
	    varp = subquery->ops_vars.opv_base->opv_rt[attrp->opz_varnm];
	    gvarp = varp->opv_grv;
	    fattrp = subquery->ops_funcs.opz_fbase->opz_fatts[
				attrp->opz_func_att];
	    if ((nodep = fattrp->opz_function)->pst_sym.pst_type == 
				PST_RESDOM &&
		fattrp->opz_type != OPZ_TCONV)
	    {
		char	vn1[X100_MAXNAME];
		x100_print_id((char *)&nodep->pst_sym.pst_value.
			pst_s_rsdm.pst_rsname, DB_ATT_MAXNAME, vn1);
		nodep = nodep->pst_right;
		STprintf(var_name, "%s", vn1);
	    }
	    else STprintf(var_name, "FA%d", fattrp->opz_fnum);
	}
	else MEcopy("<col not found>", sizeof("<col not found>"),
		    var_name);

	/* After all that, add the column to the key list. */
	opc_x100_printf(x100cb, var_name);

	/* ... and if this is a null join EQC and we are projecting prior
	** to the join, add the computed funcs. */
	if (njproj && eqclp->ope_nulljoin)
	{
	    STprintf(&nfuncs[0], ", IIISNULLFA_%-u_%-u = isnull(", eqcno,
			varp->opv_gvar);
	    opc_x100_printf(x100cb, nfuncs);
	    if (attrno >= 0 && attrp->opz_attnm.db_att_id >= 0)
		opc_x100_prepcorr(x100cb, subquery, gvarp, varp, attrp);
	    opc_x100_printf(x100cb, var_name);
	    STprintf(&nfuncs[0], "), IIIFNULLFA_%-u_%-u = ifnull(", eqcno,
			varp->opv_gvar);
	    opc_x100_printf(x100cb, nfuncs);
	    if (attrno >= 0 && attrp->opz_attnm.db_att_id >= 0)
		opc_x100_prepcorr(x100cb, subquery, gvarp, varp, attrp);
	    opc_x100_printf(x100cb, var_name);
	    opc_x100_ifnullc(x100cb, attrp->opz_dataval.db_datatype);
	}
	*not1st = TRUE;

	/* And add DERIVED if needed. */
	if (derived)
	    opc_x100_printf(x100cb, " DERIVED");

	if (varmapp == (OPV_BMVARS *) NULL || BTcount((char *)varmapp, 
				subquery->ops_vars.opv_rv) == 0)
	    return(TRUE);
    }
} 


/*{
** Name: OPC_X100_VARSEARCH	- look for PST_VAR matching attrno, varno
**
** Description:	This function searches a parse tree fragment for a PST_VAR
**	node matching the input attrno/varno pair.
**
** Inputs:
**	nodep		ptr to parse tree fragment to be searched
**	attrno		column number to match
**	varno		variable no to match
**
** Outputs:
**	Returns:
**		TRUE	if node matches, else FALSE
**	Exceptions:
**
** Side Effects:
**
** History:
**	16-dec-2010 (dougi)
**	    Written.
*/
static bool
opc_x100_varsearch(
	PST_QNODE	*nodep,
	OPZ_IATTS	attrno,
	OPV_IVARS	varno)

{
    /* Not much to do here - just recurse on nodep looking for VAR. */
    for ( ; ; )
    {
	if (nodep->pst_sym.pst_type == PST_VAR &&
	    nodep->pst_sym.pst_value.pst_s_var.pst_vno == varno &&
	    nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == attrno)
	    return(TRUE);

	if (nodep->pst_right && opc_x100_varsearch(nodep->pst_right, 
				attrno, varno))
	    return(TRUE);

	if (nodep->pst_left)
	    nodep = nodep->pst_left;
	else return(FALSE);
    }
}


/*{
** Name: OPC_X100_CO		- process OPO_CO in X100 query.
**
** Description:	This function processes a CO node from an X100 query. 
**	Depending on the node, various X100 operators will be emitted 
**	(Project, Select, Table/ColumnScan, etc.).
**
**	It recurses, producing source dataflows and emitting expressions
**	as it returns.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**	cop			Pointer to OPO_CO node being processed.
**	undermerge		bool - TRUE if CO is right under merge join
**				(and is therefore known to be ordered)
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      9-july-2008 (dougi)
**	    Written for initial support of X100 engine.
**	13-Aug-2009 (kiria01)
**	    Support .opb_bfconstants if present.
**	20-Aug-2009 (kiria01)
**	    Add code to generate the non-BF qualification if
**	    any and drop the simple tail recursion case.
**	21-Aug-2009 (kiria01)
**	    Reduce effect of previous change which may just be
**	    needed by CartProd.
**	6-nov-2009 (dougi)
**	    Add dups removal support (fixes some nested subquery flattening).
**	8-dec-2009 (dougi)
**	    Fix up some anomalies with FAs and tid refs.
**	21-dec-2009 (dougi)
**	    Refine logic to determine swaps in face of semi joins.
**	4-jan-2010 (dougi)
**	    Refine draggr code to use in-scope names.
**	5-mar-2010 (dougi)
**	    Add sequences of "&&" in front of multiple anti/semi join
**	    restrictions.
**	16-aug-2010 (dougi)
**	    Add code to project funcatts after a join.
**	16-feb-2011 (dougi)
**	    Move SJ bfsaggcheck call after Select() emit, since the predicates
**	    may refer to SAGGs.
**	03-Mar-2011 (kiria01) m1584
**	    If a collation ever gets through don't emit the special attr
**	    associated with it. At the moment this should be avoided due
**	    create table restrictions but as/when collation supported this
**	    will be needed.
**	11-mar-2011 (dougi)
**	    Emit pseudoTID computation when needed.
**	14-Mar-2011 (kiria01)
**	    Handle case where enum phase has interchanged left & right!
**	6-apr-2011 (dougi)
**	    Tighten use of projection list for inner REUSE segment of a join.
**	26-apr-2011 (dougi) m1912
**	    Push FAs in PR eqcmaps to ORIG nodes for evaluation.
**	2-may-2011 (dougi) m1921
**	    More careful handling of "names" for results of FAs.
**	2-may-2011 (dougi)
**	    Avoid project list join parm when nested source already has 
**	    Project().
*/
static VOID
opc_x100_co(
	X100_STATE	*x100cb,
	OPO_CO		*cop,
	bool		undermerge,
	bool		undersemi)
{
    OPS_STATE		*global = x100cb->global;
    OPS_SUBQUERY	*subquery = x100cb->subquery;
    OPS_SUBQUERY	*saggsubq = (OPS_SUBQUERY *) NULL, *vsubq;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    OPV_VARS		*varp;
    OPV_GRV		*gvarp;
    OPV_IVARS		varno;
    OPB_BOOLFACT	*bfp;
    OPE_BMEQCLS		*eqcmap;
    OPE_IEQCLS		dreqc;
    OPZ_BMATTS		*attmap;
    OPZ_ATTS		*attrp;
    OPZ_FATTS		*fattrp;
    DB_ATT_ID		attno;
    PST_QNODE		*rsdmp;
    char		var_name[X100_MAXNAME];
    char		var1_name[X100_MAXNAME];
    char		var_qual[5];
    char		property[40];
    char		*jtype;
    i4			pcnt = 0;
    bool		konst = FALSE, quals = FALSE, draggr = FALSE;
    i4			i, j, k;
    bool		not1st = FALSE, FAs, vtab, dmltid = FALSE;
    bool		topco = (subquery->ops_bestco == cop) ? TRUE : FALSE;
    bool		mscan = TRUE, pseudotids = FALSE;
    PST_QNODE		*save_konst;
    PST_QNODE		*save_qual;

    while (cop->opo_sjpr != DB_PR &&
		cop->opo_sjpr != DB_ORIG &&
		cop->opo_sjpr != DB_SJ)
    {
	if (!(cop = cop->opo_outer))
	    return;
    }
    eqcmap = &cop->opo_maps->opo_eqcmap;

    /* Check for duplicates removal sort on this node. */
    if (cop->opo_dupselim)
    {
	/* CO-node was checked by opc_x100_dupselimck(). */
	opc_x100_printf(x100cb, (undermerge == TRUE) ? "OrdAggr (" : "Aggr (");
	draggr = TRUE;
    }

    if (cop->opo_variant.opo_local->opo_bmbf)
	pcnt = BTcount((char *)cop->opo_variant.opo_local->
					opo_bmbf, subquery->ops_bfs.opb_bv);
    if (save_konst = subquery->ops_bfs.opb_bfconstants)
    {
	bool bval;
	/* Drop redundant ANDs */
	while (subquery->ops_bfs.opb_bfconstants->pst_sym.pst_type == PST_AND &&
	 	!subquery->ops_bfs.opb_bfconstants->pst_right)
	    if (!(subquery->ops_bfs.opb_bfconstants = subquery->ops_bfs.opb_bfconstants->pst_left))
		break;
	/* Avoid emitting a select with a true predicate. */
	if (subquery->ops_bfs.opb_bfconstants->pst_sym.pst_type != PST_QLEND &&
	    (!pst_is_const_bool(subquery->ops_bfs.opb_bfconstants, &bval) || 
				!bval))
	    konst = TRUE;
	subquery->ops_bfs.opb_bfconstants = NULL;
    }
    /* Check for any qualifiers and if present, we will emit these along with the BFs
    ** at this level so to ensure they are not see by nested calls to this routine,
    ** we hide them for the time being.
    ** Note however that when in an anti-join context, the qualifier will be handled
    ** by it and not emitted as a usual qualifier but does need hiding */
    if ((save_qual = subquery->ops_root->pst_right) &&
	subquery->ops_root->pst_right->pst_sym.pst_type != PST_QLEND)
    {
	do
	    subquery->ops_root->pst_right = subquery->ops_root->pst_right->pst_right;
	while(subquery->ops_root->pst_right &&
		subquery->ops_root->pst_right->pst_sym.pst_type != PST_QLEND);
    }
    /* Switch on CO-node, descend and do the right stuff. */
    switch (cop->opo_sjpr) {
      case DB_PR:
	/* OR PR eqcmap into underlying ORIG to assure projection of FAs 
	** that might be required later. */
	if (cop->opo_outer->opo_sjpr == DB_ORIG)
	    BTor(BITS_IN(*eqcmap), (char *)&cop->opo_maps->opo_eqcmap, 
		(char *)&cop->opo_outer->opo_maps->opo_eqcmap);

	/* Add "Select (" for each predicate (if any). */
	for (i = -konst-quals; i < pcnt; i++)
	    opc_x100_printf(x100cb, "Select (");

	/* If query has a SAGG or 2, check predicates for SAGG refs. */
	if (pcnt > 0 && (global->ops_gmask & OPS_SOMESAGGS))
	    opc_x100_bfsaggcheck(x100cb, subquery, cop, &saggsubq);

	/* Finally - emit the dataflow argument to Project/Select. */
	opc_x100_co(x100cb, cop->opo_outer, undermerge, FALSE);
	subquery->ops_bfs.opb_bfconstants = save_konst;
	subquery->ops_root->pst_right = save_qual;

	/* Copy variable bit map. */
	MEcopy((char *)cop->opo_variant.opo_local->opo_bmvars, 
	    sizeof(OPV_BMVARS),
	    (char *)cop->opo_variant.opo_local->opo_x100bmvars);

	/* If there were SAGG refs, emit SAGG materialization and extinguish
	** OPS_REUSE_QUAL flags (if any). */
	if (saggsubq)
	{
	    opc_x100_bfsaggemit(x100cb, saggsubq, FALSE);
	    opc_x100_reuseck(subquery, cop);
	}
	    
	/* Now emit the predicate expressions. */
	if (pcnt || konst || quals)
	{
	    opc_x100_bfemit(x100cb, subquery, cop, pcnt, OPL_NOOUTER, 
				konst, quals);

	    /* And add cardinality property. */
	    STprintf(property, " [ 'est_card' = '%.0f' ", 
						cop->opo_cost.opo_tups);
	    opc_x100_printf(x100cb, property);
	    opc_x100_fdlist(x100cb, subquery, cop);
	    opc_x100_printf(x100cb, "] ");
	}

	if (cop->opo_outer->opo_x100proj)
	    cop->opo_x100proj = TRUE;
	break;

      case DB_ORIG:
	{
	    /* pluck out the variable number & see if it is a table or
	    ** a "subquery". */
	    varno = cop->opo_union.opo_orig;
	    varp = subquery->ops_vars.opv_base->opv_rt[varno];
	    gvarp = varp->opv_grv;
	    varp->opv_x100cnt = gvarp->opv_x100gcnt++;
	    if (gvarp->opv_x100gcnt > 1)
		global->ops_gmask |= OPS_X100DVARS;

	    if (gvarp->opv_gsubselect)
	    {
		vtab = TRUE;
		vsubq = gvarp->opv_gsubselect->opv_subquery;
	    }
	    else if (gvarp->opv_gquery)
	    {
		vtab = TRUE;
		vsubq = gvarp->opv_gquery;
	    }
	    else 
	    {
		vtab = FALSE;
		vsubq = (OPS_SUBQUERY *) NULL;
	    }

	    /* First - if this is a virtual table that requires pseudoTIDs
	    ** or there are FAs, do project to encompass them. */
	    if ((pseudotids = (vtab && (vsubq->ops_mask2 & OPS_X100_TIDVIEW)))
							== TRUE || 
		(FAs = opc_x100_FAcheck(x100cb, subquery, eqcmap, varno)) 
							== TRUE)
		opc_x100_printf(x100cb, "Project (");

	    /* If predicates, one "Select (" for each predicate. */
	    for (i = -konst-quals; i < pcnt; i++)
		opc_x100_printf(x100cb, "Select (");

	    /* If query has a SAGG or 2, check predicates for SAGG refs. */
	    if (pcnt > 0 && (global->ops_gmask & OPS_SOMESAGGS))
		opc_x100_bfsaggcheck(x100cb, subquery, cop, &saggsubq);

	    /* Emit data source - virtual table (subquery) or real table. */
	    if (vtab)
		opc_x100_subquery(vsubq, (OPS_SUBQUERY *) NULL, x100cb, 
						(char *)&varp->opv_varname);
	    else
	    {
		/* Real table, emit MScan() with correlation name (if any). */
		opc_x100_valid(global, gvarp, FALSE);
		opc_x100_printf(x100cb, (mscan) ? "MScan ( " : "TScan ( ");

		if (gvarp->opv_qrt > OPV_NOGVAR)
		{
		    /* Find and print table/correlation name. */
		    x100_print_id((char *)&global->ops_qheader->
			pst_rangetab[gvarp->opv_qrt]->pst_corr_name,
			DB_TAB_MAXNAME, var_name);
		    x100_print_id((char *)&varp->opv_varname,
			DB_ATT_MAXNAME, var_name);
		    if (gvarp->opv_x100gcnt > 1)
			STprintf(var_name, "%s%d", var_name, varp->opv_x100cnt);
		    opc_x100_printf(x100cb, var_name);
		    opc_x100_printf(x100cb, " = ");
		}

		/* Find and print table name that precedes column list. */
		x100_print_quoted_id((char *)&gvarp->opv_relation->
				rdr_rel->tbl_name, DB_TAB_MAXNAME, var_name);
		opc_x100_printf(x100cb, var_name);
		opc_x100_printf(x100cb, ", [ ");

		for (i = -1; (i = BTnext(i, (char *)eqcmap, 
				subquery->ops_eclass.ope_ev)) != -1; )
		{
		    attmap = &subquery->ops_eclass.ope_base->
						ope_eqclist[i]->ope_attrmap;

		    /* Loop over attrs in EQC, looking for one in this table. */
		    for (j = -1; (j = BTnext(j, (char *)attmap, 
				subquery->ops_attrs.opz_av)) != -1; )
		    {
			attrp = subquery->ops_attrs.opz_base->opz_attnums[j];
			if (attrp->opz_varnm != varno ||
				(attrp->opz_mask & OPZ_COLLATT))
			    continue;

			if (attrp->opz_attnm.db_att_id <= 0)
			{
			    /* TID or func att - if fattr, just skip. If TID &
			    ** this is delete or update, assure it's the first
			    ** and emit the right stuff. Otherwise, skip. */
			    if (attrp->opz_func_att >= 0)
				continue;

			    if ((x100cb->flag & X100_DMLUPDEL) &&
				attrp->opz_varnm == 0)
			    {
				dmltid = TRUE;
				continue;
			    }
			    else /* if ((subquery->ops_eclass.ope_base->
				ope_eqclist[i]->ope_mask & OPE_JOINIX) ||
				subquery->ops_duplicates == OPS_DREMOVE) */
				STprintf(var_name, "'" X100_TIDNAME "'");
			    /* else continue; */
			}
			else
			{
			    /* Got the column - look up in table descriptor. */
			    x100_print_quoted_id(gvarp->opv_relation->
				rdr_attr[attrp->opz_attnm.db_att_id]->att_nmstr,
			    	gvarp->opv_relation->
				rdr_attr[attrp->opz_attnm.db_att_id]->att_nmlen,
				var_name);
			}

			if (not1st)
			    opc_x100_printf(x100cb, ", ");

			opc_x100_printf(x100cb, var_name);
			not1st = TRUE;
		    }
		
		}

		if (dmltid || !not1st)
		{
		    /* We saw TID back there, emit it last per X100 rules. */
		    if (not1st)
			opc_x100_printf(x100cb, ", ");

		    opc_x100_printf(x100cb, "tid = '" X100_TIDNAME "'");
		}

		opc_x100_printf(x100cb, "] )");		/* terminate scan op */

		/* If this table has a "cluster ID", adorn with property. */
		opc_x100_printf(x100cb, " [ ");
		if (varp->opv_x100clid > 0)
		{
		    STprintf(property, "'clusterid' = '%d' ", 
							varp->opv_x100clid);
		    opc_x100_printf(x100cb, property);
		}

		{
		    /* Add cardinality property. */
		    STprintf(property, (varp->opv_x100clid > 0) ? 
			", 'est_card' = '%.0f' " : "'est_card' = '%.0f' ",
						cop->opo_cost.opo_tups);
		    opc_x100_printf(x100cb, property);
		    opc_x100_fdlist(x100cb, subquery, cop);
		    opc_x100_printf(x100cb, "] ");
		}
	    }

	    /* Copy variable bit map. */
	    MEcopy((char *)cop->opo_variant.opo_local->opo_bmvars, 
		sizeof(OPV_BMVARS),
		(char *)cop->opo_variant.opo_local->opo_x100bmvars);

	    /* If there were SAGG refs, emit SAGG materialization and extinguish
	    ** OPS_REUSE_QUAL flags (if any). */
	    if (saggsubq)
	    {
		opc_x100_bfsaggemit(x100cb, saggsubq, FALSE);
		opc_x100_reuseck(subquery, cop);
	    }
	    
	    /* Now emit the predicate expressions. */
	    subquery->ops_bfs.opb_bfconstants = save_konst;
	    subquery->ops_root->pst_right = save_qual;
	    if (pcnt || konst || quals)
		opc_x100_bfemit(x100cb, subquery, cop, pcnt, OPL_NOOUTER,
					konst, quals);

	    /* Finally, if there were function attributes or this is a 
	    ** virtual table needing pseudotids, emit the project list. */
	    if (FAs || pseudotids)
	    {
		/* Annoying as this is, we must redo the whole EQC list,
		** projecting columns, as well as FAs. */
		opc_x100_printf(x100cb, ", [ "); /* prepend comma to p-list */
		for (i = -1, not1st = FALSE; (i = BTnext(i, (char *)eqcmap, 
				subquery->ops_eclass.ope_ev)) != -1; )
		{
		    attmap = &subquery->ops_eclass.ope_base->
						ope_eqclist[i]->ope_attrmap;

		    /* Loop over attrs in EQC, looking for one in this table. */
		    for (j = -1; (j = BTnext(j, (char *)attmap, 
				subquery->ops_attrs.opz_av)) != -1; )
		    {
			attrp = subquery->ops_attrs.opz_base->opz_attnums[j];
			if (attrp->opz_varnm != varno)
			    continue;		/* not from this table */

			/* Got the column - look up in table descriptor. */
			if (not1st)
			    opc_x100_printf(x100cb, ", ");
			not1st = TRUE;		/* set it now */

			if (attrp->opz_attnm.db_att_id == OPZ_FANUM)
			{
			    PST_QNODE	*fnodep;
			    /* Got a function attribute, make its name then emit
			    ** its expression. */
			    fattrp = subquery->ops_funcs.opz_fbase->opz_fatts[
					attrp->opz_func_att];
			    if ((fnodep = fattrp->opz_function)->pst_sym.
						pst_type == PST_RESDOM &&
				fattrp->opz_type != OPZ_TCONV)
			    {
				x100_print_id((char *)&fnodep->pst_sym.
				    pst_value.pst_s_rsdm.pst_rsname, 
				    DB_ATT_MAXNAME, var_name);
			    }
			    else STprintf(var_name, "FA%d ", fattrp->opz_fnum);

			    opc_x100_printf(x100cb, var_name);
			    opc_x100_printf(x100cb, " = ");

			    if (fnodep->pst_sym.pst_type == PST_RESDOM)
				fnodep = fnodep->pst_right;
			    opc_x100_prexpr(x100cb, cop, fnodep, 
				(DB_DATA_VALUE *)NULL, ADI_NILCOERCE);
			    continue;		/* jump out */
			}

			/* Prepend correlation name, then column name. */
			if (gvarp->opv_qrt > OPV_NOGVAR && !vtab)
			{
			    x100_print_id((char *)&varp->opv_varname,
					DB_ATT_MAXNAME, var_name);
			    if (gvarp->opv_x100gcnt > 1)
				STprintf(var_name, "%s%d", var_name, 
							varp->opv_x100cnt);
			    opc_x100_printf(x100cb, var_name);
			    opc_x100_printf(x100cb, ".");
			}

			if (vtab)
			{
			    for (rsdmp = vsubq->ops_root->pst_left;
				rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
				rsdmp = rsdmp->pst_left)
			     if (rsdmp->pst_sym.pst_value.pst_s_rsdm.
					pst_rsno == attrp->opz_attnm.db_att_id)
				break;

			    /* Prepend name for later qualification. */
			    opc_x100_prepcorr(x100cb, subquery, gvarp, 
								varp, attrp);
			    x100_print_id((char *)&rsdmp->pst_sym.pst_value.
					pst_s_rsdm.pst_rsname,
					DB_ATT_MAXNAME, var_name);

			    /* If we need pseudotid, emit it here. */
			    if (pseudotids)
			    {
				for (rsdmp = subquery->ops_root->pst_left; 
				    rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM; 
				    rsdmp = rsdmp->pst_left)
				if (rsdmp->pst_sym.pst_value.pst_s_rsdm.
					pst_rsflags & PST_RS_PSEUDOTID)
				{
				    opc_x100_printf(x100cb, var_name);
				    opc_x100_printf(x100cb, ", ");	
				    x100_print_id((char *)&rsdmp->pst_sym.
					pst_value.pst_s_rsdm.pst_rsname, 
					DB_ATT_MAXNAME, var1_name);
				    opc_x100_printf(x100cb, var1_name);
				    opc_x100_printf(x100cb, " = rowid(uidx('0'))");
				    break;
				}
				pseudotids = FALSE;
				continue;
			    }
			}
			else
			{
			    if (attrp->opz_attnm.db_att_id > 0)
				x100_print_id(gvarp->opv_relation->
					rdr_attr[attrp->opz_attnm.db_att_id]->
					att_nmstr,
					gvarp->opv_relation->
					rdr_attr[attrp->opz_attnm.db_att_id]->
					att_nmlen,
					var_name);
			    else if (attrp->opz_varnm == 0 && 
					(x100cb->flag & X100_DMLUPDEL))
				opc_x100_copy("tid", sizeof("tid"), var_name, 
						sizeof(var_name));
			    else opc_x100_copy(X100_TIDNAME, sizeof(X100_TIDNAME), 
						var_name, sizeof(var_name));
			}

			opc_x100_printf(x100cb, var_name);
			not1st = TRUE;
		    }
		}

		opc_x100_printf(x100cb, "])");	/* close the Project() */
		cop->opo_x100proj = TRUE;	/* and flag its presence */
	    }
	}

	break;

      case DB_SJ:
	{
	    OPO_CO	*outer, *inner;
	    OPO_ISORT	jeqc = cop->opo_sjeqc;
	    OPE_IEQCLS	FDeqc;
	    OPE_BMEQCLS	jeqcmap, *nkeqcmap, FAeqcmap;
	    OPZ_IATTS	attrno;
	    OPZ_BMATTS	*attrmap;
	    OPZ_ATTS	*attrp;
	    OPV_GRV	*gvar1;
	    OPV_IVARS	ivar;
	    OPS_SUBQUERY *rsq;
	    OPL_IOUTER	ojid = cop->opo_variant.opo_local->opo_ojid;
	    i4		ival, bix, oncnt = 0;
	    char	tcount[13];
	    bool	hash = FALSE, CP = FALSE, anti = FALSE, semi = FALSE;
	    bool	merge = FALSE, left = FALSE, right = FALSE, full = FALSE;
	    bool	nonkey = FALSE, rev = FALSE, oj = FALSE;

	    /* Flag anti, semi and reverse joins. */
	    if (cop->opo_variant.opo_local->opo_mask & OPO_ANTIJOIN)
		anti = TRUE;
	    if (cop->opo_variant.opo_local->opo_mask & OPO_SEMIJOIN)
		semi = TRUE;
	    if (cop->opo_variant.opo_local->opo_mask & OPO_REVJOIN)
		rev = TRUE;

	    /* Pick up non-key equijoin predicates (if any). */
	    if ((nkeqcmap = cop->opo_variant.opo_local->opo_nkeqcmap) !=
				(OPE_BMEQCLS *) NULL &&
		BTcount((char *)nkeqcmap, subquery->ops_eclass.ope_ev) > 0)
		nonkey = TRUE;
	    else nonkey = FALSE;

	    MEfill(sizeof(jeqcmap), (u_char) 0, (char *)&jeqcmap);
							/* init */
	    if (jeqc >= subquery->ops_eclass.ope_ev)
	    {
		MEcopy((char *)subquery->ops_msort.opo_base->opo_stable[
			jeqc-subquery->ops_eclass.ope_ev]->opo_bmeqcls,
			sizeof(jeqcmap), (char *)&jeqcmap);
	    }
	    else if (jeqc >= 0)
		BTset((i4)jeqc, (char *)&jeqcmap);

	    /* See if there are EQCs in result that aren't in input - this
	    ** means we need to project something. */
	    MEcopy((char *)&cop->opo_outer->opo_maps->opo_eqcmap,
				sizeof(FAeqcmap), (char *)&FAeqcmap);
	    BTor(BITS_IN(FAeqcmap), (char *)&cop->opo_inner->
				opo_maps->opo_eqcmap, (char *)&FAeqcmap);
	    BTnot(BITS_IN(FAeqcmap), (char *)&FAeqcmap);
	    BTand(BITS_IN(FAeqcmap), (char *)&cop->opo_maps->opo_eqcmap,
				(char *)&FAeqcmap);

	    FAs = FALSE;			/* init */
	    if (BTcount((char *)&FAeqcmap, subquery->ops_eclass.ope_ev) > 0)
	    {
		for (i = -1; (i = BTnext(i, (char *)&FAeqcmap, subquery->
			ops_eclass.ope_ev)) != -1 && !FAs; )
		{
		    attmap = &subquery->ops_eclass.ope_base->
						ope_eqclist[i]->ope_attrmap;

		    /* Loop over attrs in EQC, looking for functions. */
		    for (j = -1; (j = BTnext(j, (char *)attmap, 
				subquery->ops_attrs.opz_av)) != -1; )
		    {
			attrp = subquery->ops_attrs.opz_base->opz_attnums[j];
			if (attrp->opz_func_att <= OPZ_NOFUNCATT)
			    continue;		/* not a FA - keep looking */
			if (!(subquery->ops_funcs.opz_fbase->opz_fatts[
				attrp->opz_func_att]->opz_mask & OPZ_OJFA))
			    FAs = TRUE;		/* got one - set flags */
			break;
		    }
		}
	    }
	    if (FAs)				/* emit Project() */
		opc_x100_printf(x100cb, "Project (");

	    outer = cop->opo_outer;		/* default */
	    inner = cop->opo_inner;

	    /* Check for outer joins and set flags. */
	    if (cop->opo_variant.opo_local->opo_ojid != OPL_NOOUTER &&
				!anti && !semi)
		switch (cop->opo_variant.opo_local->opo_type) {
		  case OPL_LEFTJOIN:
		    left = TRUE;
		    break;
		  case OPL_RIGHTJOIN:
		    right = TRUE;
		    break;
		  case OPL_FULLJOIN:
		    full = TRUE;
		    break;
		}

	    switch (cop->opo_variant.opo_local->opo_jtype)
	    {
	      case OPO_SJFSM:
	      case OPO_SJHASH:
		if (MEcmp(cop->opo_variant.opo_local->opo_x100jt, 
				"Merge", 5) == 0)
		merge = TRUE;
		else hash = TRUE;
		break;

	      case OPO_SJCARTPROD:
		CP = quals = TRUE;
		break;
	    }

	    /* Set flag for inequality ON clause predicates (not in
	    ** MergeJoin). */
	    oj = hash && (left || right || full);
	    if (!oj)
		ojid = OPL_NOOUTER;

	    if (oj && pcnt > 0)
	     for (bix = -1; (bix = BTnext(bix, (char *)cop->opo_variant.
			opo_local->opo_bmbf, subquery->ops_bfs.opb_bv)) >= 0;)
	      if (subquery->ops_bfs.opb_base->opb_boolfact[bix]
							->opb_ojid == ojid)
		oncnt++;

	    if (quals && subquery->ops_root->pst_right->
					pst_sym.pst_type == PST_QLEND)
		quals = FALSE;

	    /* If predicates, one "Select (" for each predicate (but only 
 	    ** if not semi/anti joins - extra preds for them go right
	    ** in the join operator). 
	    **
	    ** If merge join with nonkey predicates, also emit a single
	    ** Select(). Later, we'll emit a bunch of &&'d =='s. */
	    for (i = -konst-quals; i < ((!anti && !semi) ? pcnt-oncnt : 0); i++)
		opc_x100_printf(x100cb, "Select (");
	    if (merge && nonkey)
		opc_x100_printf(x100cb, "Select (");

	    /* If query has a SAGG or 2, check predicates for SAGG refs. */
	    if (pcnt > 0 && (global->ops_gmask & OPS_SOMESAGGS))
		opc_x100_bfsaggcheck(x100cb, subquery, cop, &saggsubq);

	    /* Label the join. */
	    opc_x100_printf(x100cb, cop->opo_variant.opo_local->opo_x100jt);
	    opc_x100_printf(x100cb, " ( ");

	    /* If this is a nulljoin, we need to project the join exprs. */
	    if (cop->opo_x100nullj)
		opc_x100_printf(x100cb, "Project( ");

	    /* Dataflow for join outer. */
	    opc_x100_co(x100cb, outer, merge, (semi && (!rev)));
	    if (cop->opo_x100nullj)
	    {
		opc_x100_colmap(x100cb, subquery, outer, (OPO_CO *) NULL, 
			&outer->opo_maps->opo_eqcmap, FALSE, TRUE, FALSE);
		outer->opo_x100proj = TRUE;
	    }

	    opc_x100_printf(x100cb, ", ");	/* parameter separator */

	    if (hash || merge)
	    {
		/* For hash/merge joins, next parm is the outer (inner) 
		** keylist.
		**
		** NOTE: Ingres can generate joins in which opo_sjeqc 
		** doesn't cover all the equijoin predicates (for a variety
		** of reasons, not always correct). nkeqcmap contains EQCs
		** not in opo_sjeqc that are also mapped by equijoin predicates.
		** For now, we take a deep breath and add them to the X100
		** key list. This may not always be the right thing to do
		** (e.g. for merge joins on join indices). So the "nonkey"
		** flag can be used to invoke more elaborate logic in the 
		** future, if needed. */

		if (!merge && nonkey)
		    BTor(BITS_IN(jeqcmap), (char *)nkeqcmap, (char *)&jeqcmap);
		opc_x100_ixcollapse(subquery, &jeqcmap);

		/* Build list of outer key columns (using func). */
		opc_x100_printf(x100cb, "[ ");
		if (merge)
		    opc_x100_collist(x100cb, subquery, outer, jeqc, 
						TRUE, FALSE);
		else opc_x100_colmap(x100cb, subquery, outer, cop, &jeqcmap,
					TRUE, FALSE, FALSE);
		opc_x100_printf(x100cb, " ], ");
	    }

	    /* If this is a nulljoin, we need to project the join exprs. */
	    if (cop->opo_x100nullj)
		opc_x100_printf(x100cb, "Project( ");

	    /* Dataflow for join inner (need "FlowMat()" around inner of 
	    ** Cartprod). */
	    if (CP)
		opc_x100_printf(x100cb, "FlowMat (");

	    opc_x100_co(x100cb, inner, merge, (semi && rev));
	    if (cop->opo_x100nullj)
	    {
		opc_x100_colmap(x100cb, subquery, inner, (OPO_CO *) NULL, 
			&inner->opo_maps->opo_eqcmap, FALSE, TRUE, FALSE);
		inner->opo_x100proj = TRUE;
	    }
	    subquery->ops_bfs.opb_bfconstants = save_konst;
	    subquery->ops_root->pst_right = save_qual;

	    if (CP)
		opc_x100_printf(x100cb, ")");

	    if (hash || merge)
	    {
		/* For hash/merge joins, next parm is the inner keylist,
		** then (for hash only) the OPF inner row estimate. */
		opc_x100_printf(x100cb, " , [ ");	/* delimit parms */
		if (merge)
		    opc_x100_collist(x100cb, subquery, inner, jeqc,
						TRUE, FALSE);
		else opc_x100_colmap(x100cb, subquery, inner, cop, &jeqcmap,
					TRUE, FALSE, FALSE);
		opc_x100_printf(x100cb, " ]");

		/* If inner was a REUSE, we may need to project here, but
		** only if there's no intervening [Ord]Aggr(). */
		ivar = -1;
		if ((x100cb->flag & X100_REUSE) && !(inner->opo_dupselim))
		 if (inner->opo_sjpr == DB_ORIG)
		    ivar = inner->opo_union.opo_orig;
		 else if (inner->opo_outer && inner->opo_outer->opo_sjpr ==
				DB_ORIG && !(inner->opo_outer->opo_dupselim))
		    ivar = inner->opo_outer->opo_union.opo_orig;

		if (ivar >= 0 && subquery->ops_vars.opv_base->
			opv_rt[ivar]->opv_grv && (rsq = subquery->ops_vars.
			opv_base->opv_rt[ivar]->opv_grv->opv_gquery) !=
			(OPS_SUBQUERY *) NULL &&
			rsq->ops_vars.opv_rv == 1 &&
			rsq->ops_vars.opv_base->opv_rt[0]->opv_grv &&
			rsq->ops_vars.opv_base->opv_rt[0]->opv_grv->opv_gquery &&
			rsq->ops_vars.opv_base->opv_rt[0]->opv_grv->opv_gquery->
				ops_sqtype == OPS_REUSE &&
			!inner->opo_x100proj)
		{
		    /* Emit projection list consisting of everything up
		    ** to the first BY-list entry. */
		    opc_x100_printf(x100cb, " [");

		    for (rsdmp = rsq->ops_root->pst_left, not1st = FALSE;
			rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
			rsdmp = rsdmp->pst_left)
		    {
/*
			if (rsdmp == rsq->ops_byexpr)
			    break;
*/
		    
			if (!(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &
						PST_RS_PRINT))
			    continue;
			if (not1st)
			    opc_x100_printf(x100cb, ", ");

			/* Emit virtual table name qualifier, then RSD name. */
			x100_print_id((char *)&subquery->ops_vars.opv_base->
				opv_rt[ivar]->opv_varname,
				DB_ATT_MAXNAME, var_name);
			opc_x100_printf(x100cb, var_name);
			opc_x100_printf(x100cb, ".");
			x100_print_id((char *)&rsdmp->pst_sym.pst_value.
				pst_s_rsdm.pst_rsname,
				DB_ATT_MAXNAME, var_name);
			opc_x100_printf(x100cb, var_name);
			not1st = TRUE;
		    }

		    opc_x100_printf(x100cb, " ]");
		}

		if (pcnt > 0 && (anti || semi || (oj && oncnt > 0)))
		{
		    /* Emit non-key predicates into join. */
		    opc_x100_printf(x100cb, ", ");	/* delimiter */

		    /* Prepend appropriate number of "&& ("s (if any). */
		    for (i = 0; i < ((oj) ? oncnt : pcnt) - 1; i++)
		     opc_x100_printf(x100cb, "&& (");

		    for (bix = -1, i = 0; (bix = BTnext(bix, (char *)cop->
			opo_variant.opo_local->opo_bmbf, 
			subquery->ops_bfs.opb_bv)) != -1; i++)
		    {
			bfp = subquery->ops_bfs.opb_base->opb_boolfact[bix];
			if (oj && bfp->opb_ojid != ojid)
			{
			    i--;
			    continue;
			}

			if (pcnt > 1 && i > 0)
			    opc_x100_printf(x100cb, ", ");

			opc_x100_prexpr(x100cb, cop, bfp->opb_qnode,
			    (DB_DATA_VALUE *)NULL, ADI_NILCOERCE);

			if (!oj && pcnt > 1 || oncnt > 1)
			 if (i == 0)
			    continue;
			 else opc_x100_printf(x100cb, ") ");
		    }
		}

		/* Format inner row count into final parm for hash join. */
		if (hash)
		{
		    if (inner->opo_cost.opo_tups < 100)
			ival = 0;
		    else if (inner->opo_cost.opo_tups > 1000000000)
			ival = 500000000;
		    else ival = (i4)inner->opo_cost.opo_tups;
		    STprintf(tcount, ", %d", ival);
		    opc_x100_printf(x100cb, &tcount[0]);
		}
		/* Finally, outer join type (if applicable). */
		if (!anti && !semi)
		 if (left || right || full)
		    opc_x100_printf(x100cb, (left) ? ", LEFTOUTER" : (right) ?
				", RIGHTOUTER" : ", FULLOUTER");
	    }

	    opc_x100_printf(x100cb, " )");

	    /* Compose variable bit map from inputs. If not anti-/semi-join
 	    ** OR the inner and outer, otherwise, just copy the outer (or inner
	    ** if it is a reversed anti/semi). */
	    if (rev || (right && (anti || semi)))
		MEcopy((char *)inner->opo_variant.opo_local->opo_x100bmvars, 
		    sizeof(OPV_BMVARS),
		    (char *)cop->opo_variant.opo_local->opo_x100bmvars);
	    else MEcopy((char *)outer->opo_variant.opo_local->opo_x100bmvars, 
		    sizeof(OPV_BMVARS),
		    (char *)cop->opo_variant.opo_local->opo_x100bmvars);
	    if (!anti && !semi)
		BTor(BITS_IN(OPV_BMVARS), 
		    (char *)inner->opo_variant.opo_local->opo_x100bmvars,
		    (char *)cop->opo_variant.opo_local->opo_x100bmvars);

	    /* Extinguish OPS_REUSE_QUAL flags, if any. */
	    opc_x100_reuseck(subquery, inner);
	    
	    /* If there were SAGG refs, emit SAGG materialization. */
	    if (saggsubq)
		opc_x100_bfsaggemit(x100cb, saggsubq, FALSE);

	    /* If merge join and non key predicates, finish the Select(). */
	    if (merge && nonkey)
		opc_x100_mergenkey(x100cb, subquery, cop);

	    /* Now emit the predicate expressions for the surrounding 
	    ** Select's. */
	    if (!anti && !semi && pcnt-oncnt || konst || quals)
		opc_x100_bfemit(x100cb, subquery, cop, !anti && !semi ? 
				pcnt-oncnt : 0, ojid, konst, quals);

	    /* And if there are FAs to compute, emit the Project() list. */
	    if (FAs)
	    {
		/* Annoying as this is, we must redo the whole EQC list,
		** projecting columns, as well as FAs. */
		opc_x100_printf(x100cb, ", [ "); /* prepend comma to p-list */
		for (i = -1, not1st = FALSE; (i = BTnext(i, (char *)&cop->
		    opo_maps->opo_eqcmap, subquery->ops_eclass.ope_ev)) != -1; )
		{
		    attmap = &subquery->ops_eclass.ope_base->
						ope_eqclist[i]->ope_attrmap;

		    /* Loop over attrs in EQC, looking for one in this table. */
		    for (j = -1; (j = BTnext(j, (char *)attmap, 
				subquery->ops_attrs.opz_av)) != -1; )
		    {
			attrp = subquery->ops_attrs.opz_base->opz_attnums[j];
			if (BTtest(attrp->opz_varnm, (char *)cop->opo_variant.
				opo_local->opo_bmvars) == FALSE)
			    continue;		/* not from this join */

			/* Got the column - look up in table descriptor. */
			varno = attrp->opz_varnm;
			varp = subquery->ops_vars.opv_base->opv_rt[varno];
			gvarp = varp->opv_grv;
			if (gvarp->opv_gsubselect)
			{
			    vtab = TRUE;
			    vsubq = gvarp->opv_gsubselect->opv_subquery;
			}
			else if (gvarp->opv_gquery)
			{
			    vtab = TRUE;
			    vsubq = gvarp->opv_gquery;
			}
			else 
			{
			    vtab = FALSE;
			    vsubq = (OPS_SUBQUERY *) NULL;
			}

			if (not1st)
			    opc_x100_printf(x100cb, ", ");
			not1st = TRUE;		/* set it now */

			if (attrp->opz_attnm.db_att_id == OPZ_FANUM)
			{
			    PST_QNODE	*fnodep;
			    /* Got a function attribute, make its name then emit
			    ** its expression. */
			    fattrp = subquery->ops_funcs.opz_fbase->opz_fatts[
					attrp->opz_func_att];
			    if ((fnodep = fattrp->opz_function)->pst_sym.
						pst_type == PST_RESDOM &&
				fattrp->opz_type != OPZ_TCONV)
			    {
				x100_print_id((char *)&fnodep->pst_sym.
				    pst_value.pst_s_rsdm.pst_rsname, 
				    DB_ATT_MAXNAME, var_name);
			    }
			    else STprintf(var_name, "FA%d ", fattrp->opz_fnum);

			    opc_x100_printf(x100cb, var_name);
			    opc_x100_printf(x100cb, " = ");

			    if (fnodep->pst_sym.pst_type == PST_RESDOM)
				fnodep = fnodep->pst_right;
			    opc_x100_prexpr(x100cb, cop, fnodep, 
				(DB_DATA_VALUE *)NULL, ADI_NILCOERCE);
			    continue;		/* jump out */
			}

			/* Prepend correlation name, then column name. */
			if (gvarp->opv_qrt > OPV_NOGVAR && !vtab)
			{
			    x100_print_id((char *)&varp->opv_varname,
					DB_ATT_MAXNAME, var_name);
			    if (gvarp->opv_x100gcnt > 1)
				STprintf(var_name, "%s%d", var_name, 
							varp->opv_x100cnt);
			    opc_x100_printf(x100cb, var_name);
			    opc_x100_printf(x100cb, ".");
			}

			if (vsubq)
			{
			    for (rsdmp = vsubq->ops_root->pst_left;
				rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
				rsdmp = rsdmp->pst_left)
			     if (rsdmp->pst_sym.pst_value.pst_s_rsdm.
					pst_rsno == attrp->opz_attnm.db_att_id)
				break;

			    /* Prepend name for later qualification. */
			    opc_x100_prepcorr(x100cb, subquery, gvarp, 
								varp, attrp);
			    x100_print_id((char *)&rsdmp->pst_sym.pst_value.
					pst_s_rsdm.pst_rsname,
					DB_ATT_MAXNAME, var_name);
			}
			else
			{
			    if (attrp->opz_attnm.db_att_id > 0)
				x100_print_id(gvarp->opv_relation->
					rdr_attr[attrp->opz_attnm.db_att_id]->
					att_nmstr,
					gvarp->opv_relation->
					rdr_attr[attrp->opz_attnm.db_att_id]->
					att_nmlen,
					var_name);
			    else
				opc_x100_copy(X100_TIDNAME, sizeof(X100_TIDNAME), 
						var_name, sizeof(var_name));
			}

			opc_x100_printf(x100cb, var_name);
			not1st = TRUE;
		    }
		}
		opc_x100_printf(x100cb, "])");	/* close the Project() */
		cop->opo_x100proj = TRUE;	/* and flag its presence */
	    }

	    {
		/* Add cardinality property & any comments. */
		opc_x100_printf(x100cb, " [");
		STprintf(property, " 'est_card' = '%.0f' ", 
						cop->opo_cost.opo_tups);
		opc_x100_printf(x100cb, property);
		opc_x100_fdlist(x100cb, subquery, cop);
		opc_x100_printf(x100cb, "] ");
	    }

	    break;
	}

      default:
	/* We should not be able to get here */
	opx_1perror(E_OP009D_MISC_INTERNAL_ERROR,"Unexpected opo_sjpr");
	break;
    }

    if (draggr)
    {
	/* Materialize dups elimination column list and complete Aggr(). */
	opc_x100_printf(x100cb, ", [ ");
	opc_x100_colmap(x100cb, subquery, cop, (OPO_CO *) NULL, 
			&cop->opo_maps->opo_eqcmap, FALSE, FALSE, TRUE);
	opc_x100_printf(x100cb, "], [])");
    }

    return;

}

/*
** Name: OPC_X100_VALID - Generate QP valid-list entry for table
**
** Description:
**	Even though an X100 action doesn't use much of the traditional
**	Ingres execution engine, we still need to go through the motions
**	for various things.  One such "motion" is the generation of
**	resource and valid list entries for VectorWise tables.  We
**	still need to open IVW tables at QP validation time to check
**	the validity and timestamps of the tables.  Having a DMF
**	open-table handle for the IVW table also allows us to do
**	some useful catalog-related things such as attach rowcount
**	updates to the TCB, so that DMF can go through all of the
**	usual catalog-updating mechanisms.
**
**	Where we diverge from an Ingres QP action, and the reason for
**	this routine, is that multiple table references don't need
**	to generate multiple valid list entries (and hence multiple
**	table opens).  We only need one valid-list entry per IVW
**	table for the entire query plan, since we aren't actually
**	doing any query execution with the open-table reference.
**
**	If the table hasn't already been referenced somewhere in the
**	query plan, we generate a valid list (and resource list) entry
**	for it.  If we already have a resource list entry for the
**	table, (almost) nothing further needs to happen.
**
** Inputs:
**	global			OPS_STATE optimizer global state
**	grv			Global variable pointer for table
**	isResult		TRUE if table is an update-query result;
**				we'll set the action ahd_dmtix if true
**
** Outputs:
**	None.
**
** History:
**	11-Jan-2011 (kschendel) m1142
**	    Written.
*/

static void
opc_x100_valid(OPS_STATE *global, OPV_GRV *grv, bool isResult)
{

    i4 notused_type;		/* Not used */
    PTR notused_id;		/* Not used */
    QEF_MEM_CONSTTAB *notused_p;
    QEF_RESOURCE *res;		/* Resource entry if any */
    QEF_VALID *ret_vl;		/* New valid-list entry */

    /* See if the table is already registered in the resource list */
    res = opc_get_resource(global, grv, &notused_type, &notused_id, &notused_p);
    if (res != NULL)
    {
	ret_vl = res->qr_resource.qr_tbl.qr_valid;
    }
    else
    {

	/* Not there, create resource and valid list entry.  Don't care
	** about lock-mode or size stuff.
	** X100query actions don't set up the DMR_CB in qee, so don't
	** confuse qeq-valid -- kill the CB index.
	*/
	ret_vl = opc_valid(global, grv, DMT_S, 0, FALSE);
	ret_vl->vl_dmr_cb = -1;
    }

    /* Set the action header DMT_CB index if caller asks nicely */
    if (isResult)
	global->ops_cstate.opc_topahd->qhd_obj.qhd_x100Query.ahd_dmtix = ret_vl->vl_dmf_cb;

} /* opc_x100_valid */

/*{
** Name: OPC_X100_PROJAGGR	- handle Project/Aggr operations.
**
** Description:	This function coordinates the Project and Aggr X100 
**	operations. It adds a Project to Aggr ops when non-aggregate 
**	expressions are involved and it removes common subexpressions 
**	for pre-evaluation.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**      x100cb			ptr to X100 text state variable
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      9-sept-2008 (dougi)
**	    Written for initial support of X100 engine.
**	30-nov-2009 (dougi)
**	    No sorts inside SAGG subqueries.
**	30-dec-2009 (dougi)
**	    Dups elimination is driven by RESDOM list, not EQCs. Add comments
**	    and tidy up.
**	15-mar-2010 (dougi)
**	    Single dups elimination when agg distinct and dups elim are both 
**	    required.
**	22-mar-2010 (dougi)
**	    Remove code that had been disabled for a looong time.
**	13-apr-2010 (dougi)
**	    Adjust a couple of DELETE tests to fix subqueries.
**	20-apr-2010 (dougi)
**	    Fix up confusion between Sort, TopN and Window.
**	29-apr-2010 (dougi)
**	    Explicit print of "tid" as only possible sort column in UPDATE.
**	30-apr-2010 (dougi)
**	    Add check for SAGG required - can happen with SAGGs joined to
**	    non-trivial query (as in Mantis 470).
**	20-may-2010 (dougi)
**	    Smallish changes for dups elimination elimination in nested
**	    derived queries possibly with UNIONs.
**	21-may-2010 (dougi)
**	    Assure copied RESDOMs have print off if source does too.
**	16-dec-2010 (dougi)
**	    Top call to opc_x100_dupselimck() gets "TRUE" since we only
**	    call it when subquery wants dupselim.
**	24-feb-2011 (dougi)
**	    Fine tune placement of "Project"s for distinct but non-agg queries
**	    with common subexpressions in the SELECT list.
**	26-apr-2011 (dougi) m1908
**	    Slight tweak to avoid superfluous Project() in DELETE.
**	5-june-2011 (dougi) r1946
**	    Introduce Peter's optimization to do "Window(Topn(" instead of
**	    "Window(Sort".
**	10-june-2011 (dougi) m2043, 2047
**	    Work around window funcs when aggregates require pre-project.
**	13-june-2011 (dougi) m2045
**	    Support "select distinct .." on query with window funcs.
**	15-june-2011 (dougi) m1953
**	    No Sort()s in OPS_SAGG's.
**	30-june-2011 (dougi) m2198
**	    Re-position projection in "first N" query with window funcs.
*/
static VOID
opc_x100_projaggr(
	OPS_SUBQUERY	*subquery,
	X100_STATE	*x100cb)
{
    OPS_STATE		*global = x100cb->global;
    OPO_CO		*cop = subquery->ops_bestco;
    PST_QNODE		*commexpr = NULL;
    PST_QNODE		*commproj = NULL;
    PST_QNODE		*commproj2 = NULL;
    PST_QNODE		*rsdmp, *rsdm1p, *oldrsdmp, *tempp, *commp1, *firstoffn;
    PST_QNODE		*distvarp = NULL, *distrsdmp = NULL;
    PST_QNODE		*winfp, *wrsdmp, *obrsdmp, **winrsdmpp;
    OPV_GRV		*gvarp = (OPV_GRV *) NULL;
    OPE_IEQCLS		eqcno;
    OPE_EQCLIST		*eqclp;
    OPV_IVARS		varno;
    OPV_BMVARS		*vmap;
    OPZ_IATTS		attno;
    OPZ_ATTS		*attrp;
    i4			i, j, topn = 0, offn = 0;
    char		rsname[X100_MAXNAME];
    bool		agg = (subquery->ops_sqtype == OPS_HFAGG ||
				subquery->ops_sqtype == OPS_RFAGG ||
				subquery->ops_sqtype == OPS_FAGG ||
				subquery->ops_sqtype == OPS_SAGG);
    bool		sort = (cop->opo_sjpr == DB_RSORT &&
				subquery->ops_sqtype != OPS_SAGG);
    bool		aggdist = FALSE, dupselim = FALSE;
    bool		dups_noprint = FALSE, prexpr = FALSE;
    bool		not1st, clagg, orddupselim, cp, dummy_true = TRUE;
    bool		skipproj, extraproj1, extraproj2, found;
    bool		nowsort = FALSE, obexpr = FALSE, noemit = FALSE;
    bool		winfuncs = ((subquery->ops_mask2 & OPS_WINDOW_FUNC)) ?
				TRUE : FALSE;
    bool		distinct = ((subquery->ops_root->pst_sym.pst_value.
			    pst_s_root.pst_mask1 & PST_10DISTINCT)) ?
				TRUE : FALSE;
    bool		CPpart, cjoin;
    char		*fill = "";
    char		firstout[20];
    char		winname[16], partname[16], rowname0[16];
    char                *var_id, *tab_id;


    if (Opf_x100_debug>1)
	opt_pt_entry(global, NULL, subquery->ops_root->pst_left, 
				"Before Transform:", FALSE, TRUE, FALSE);

    /* Start by setting various flags that control the operations emitted and 
    ** the sequence thereof. */

    /* Adorn CO-tree with dups elimination flags (if needed). This is done
    ** here because opc_x100_orddeck() depends on its results. */
    if (subquery->ops_duplicates == OPS_DREMOVE)
	opc_x100_dupselimck(subquery, cop, &dummy_true);

    /* If this is sort, check for no sort equivalence class - if so it is
    ** a duplicates elimination sort - possibly for aggregate distinct, 
    ** possibly as part of subquery flattening. If not dups elimination,
    ** then check for TopN() or ordering Sort(). */
    if ((sort && cop->opo_ordeqc == OPE_NOEQCLS) ||
		!(x100cb->flag & X100_DMLUPDEL) &&
				subquery->ops_duplicates == OPS_DREMOVE)
    {
	/* cop = cop->opo_outer; */
	if (global->ops_qheader->pst_stlist == (PST_QNODE *) NULL)
	    sort = FALSE;
	if (subquery->ops_duplicates == OPS_DREMOVE)
	    orddupselim = opc_x100_orddeck(subquery, cop, &dupselim);
	else orddupselim = FALSE;
    }
    else orddupselim = FALSE;

    /* No sort if only for dups removal atop UNION pieces. */
    if (subquery->ops_duplicates == OPS_DREMOVE && sort &&
	(subquery->ops_sqtype == OPS_VIEW || subquery->ops_sqtype == OPS_UNION))
	sort = FALSE;

    /* Check for clustering on the GROUP BY columns (if aggregate). */
    if (!dupselim && agg)
	clagg = opc_x100_claggck(subquery, subquery->ops_byexpr);
    else clagg = FALSE;

    /* It is possible that a top sort was removed from this plan because
    ** an ordered merge join matched the sort sequence. Such orders are 
    ** at best unreliable for now. So we'll reinstate the sort. */
    if (!sort && subquery->ops_sqtype == OPS_MAIN &&
	cop->opo_ordeqc != OPE_NOEQCLS &&
	global->ops_qheader->pst_stlist != (PST_QNODE *) NULL)
    {
	if (TRUE || !((cop->opo_sjpr == DB_PR || cop->opo_sjpr == DB_ORIG) &&
	    subquery->ops_vars.opv_rv == 1 &&
	    subquery->ops_vars.opv_base->opv_rt[0]->opv_grv->opv_gquery ==
			(OPS_SUBQUERY *) NULL))
	    sort = TRUE;		/* sort flag gets a 2nd chance */
    }

    /* On the other hand, if this isn't MAIN, but immediately precedes it,
    ** we may be able to remove a superfluous sort. */
    if (sort && agg && global->ops_subquery &&
	global->ops_subquery->ops_sqtype == OPS_MAIN &&
	(global->ops_subquery->ops_bestco->opo_sjpr == DB_RSORT ||
	 (global->ops_subquery->ops_bestco->opo_ordeqc != OPE_NOEQCLS &&
	  global->ops_qheader->pst_stlist != (PST_QNODE *) NULL)))
	sort = FALSE;			/* sort will be done in MAIN */

    /* Check for non-printing entries in the pst_stlist. They mean we're
    ** sorting on an expression or something not in the SELECT-list. Either 
    ** way, an extra Project() is required to lose them in the final output. */
    extraproj1 = extraproj2 = skipproj = FALSE;
    if (sort)
    {
	for (tempp = global->ops_qheader->pst_stlist;
		tempp && tempp->pst_sym.pst_type == PST_SORT && !extraproj1;
		tempp = tempp->pst_right)	/* loop over sort cols */
	 for (rsdmp = subquery->ops_root->pst_left, 
					oldrsdmp = subquery->ops_root;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left, oldrsdmp = oldrsdmp->pst_left)	
						/* loop over RESDOMs */
	  if (tempp->pst_sym.pst_value.pst_s_sort.pst_srvno ==
		rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno &&
		!(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
								PST_RS_PRINT))
	  {
	    if (rsdmp->pst_right->pst_sym.pst_type != PST_VAR)
	     for (rsdm1p = subquery->ops_root->pst_left, skipproj = FALSE;
		rsdm1p && rsdm1p->pst_sym.pst_type == PST_RESDOM;
		rsdm1p = rsdm1p->pst_left)	/* loop (again) over RESDOMs */
	     {
		if (rsdmp == rsdm1p)
		    continue;
		if (!opv_ctrees(global, rsdmp->pst_right, rsdm1p->pst_right))
		    continue;

		/* Got a match - reset sort srvno and get rid of 
		** non-printing RESDOM. */
		tempp->pst_sym.pst_value.pst_s_sort.pst_srvno =
		    rsdm1p->pst_sym.pst_value.pst_s_rsdm.pst_rsno;
		oldrsdmp->pst_left = rsdmp->pst_left;
		skipproj = TRUE;
		break;
	     }
	    if (!skipproj)
		extraproj1 = TRUE;
	    break;
	  }
    }

    /* Also set extraproj1 if there are window functions - and munge in the
    ** function transforms to the RESDOM list. */
    if (winfuncs)
    {
	/* Check for expressions in OVER() clause (in order by). They require
	** yet another Project() - this one inside the Sort that's to come. */
	for (i = 0, obexpr = FALSE, nowsort = TRUE; 
		!obexpr && i < subquery->ops_window_cnt; i++)
	 for (rsdmp = (*subquery->ops_winfuncs[i])->pst_right->pst_right->pst_left;
		!obexpr && rsdmp != NULL && 
		    rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
	 {
	    nowsort = FALSE;			/* got a sort */
	    if (rsdmp->pst_right != NULL && 
		    rsdmp->pst_right->pst_sym.pst_type != PST_VAR)
		obexpr = TRUE;
	 }

	if (obexpr)
	    obrsdmp = opc_x100_winsort_expr(subquery);

	/* Now do 1st rewrite. */
	winfp = opc_x100_winfunc_rewrite(subquery, x100cb, &winrsdmpp,
				0, obexpr);
	extraproj1 = TRUE;

	if (Opf_x100_debug>5)
	    opt_pt_entry(global, NULL, subquery->ops_root->pst_left, 
				"After Window Transform:", FALSE, TRUE, FALSE);
    }

    /* Note: "first N" may be specified without "order by". We need to 
    ** use "Window()" operator for those. */
    if (subquery->ops_sqtype == OPS_MAIN && 
	(firstoffn = global->ops_qheader->pst_firstn) != (PST_QNODE *) NULL)
    {
	/* Determine TopN() value (if any). */
	if (firstoffn->pst_sym.pst_dataval.db_length == 2)
	    topn = *(i2 *)firstoffn->pst_sym.pst_dataval.db_data;
	else topn = *(i4 *)firstoffn->pst_sym.pst_dataval.db_data;
    }
    if (subquery->ops_sqtype == OPS_MAIN && 
	(firstoffn = global->ops_qheader->pst_offsetn) != (PST_QNODE *) NULL)
    {
	/* Determine offset value, too (if any). */
	if (firstoffn->pst_sym.pst_dataval.db_length == 2)
	    offn = *(i2 *)firstoffn->pst_sym.pst_dataval.db_data;
	else offn = *(i4 *)firstoffn->pst_sym.pst_dataval.db_data;
    }

    /* Flags are set. Now emit TopN/Sort/Window, if needed, preceded by 
    ** "non-printing RESDOM" removal Project, if needed. 
    ** NOTE: Window() doesn't sort, so an offset on an ordered result must
    ** have both. Because TopN sort is faster than regular Sort() (only sorts
    ** first N), offset + topn generate a Window(Topn()) with a TopN of 
    ** topn+offn. */
    if ((!sort && topn) || offn)
    {
	if (sort && extraproj1)
	    opc_x100_printf(x100cb, "Project (");
					/* project away non printing sorts */
	opc_x100_printf(x100cb, "Window (");
    }

    if (sort)
    {
	if (extraproj1 && offn == 0)
	    opc_x100_printf(x100cb, "Project (");
					/* for sort exprs, etc. */
	opc_x100_printf(x100cb, (topn) ? "TopN (" : "Sort (");
    }

    /* Again - if there are window functions, we need a Sort for the 
    ** partitioning and ordering columns and, if there isn't a sort on
    ** the result, another Project to get rid of the intermediate results.
    ** If there is a sort, "extraproj1" already introduced the extra
    ** Project. */
    if (winfuncs)
    {
	PST_QNODE	**winpp = subquery->ops_winfuncs[0];

	if (!sort)
	    opc_x100_printf(x100cb, "Project (");
				/* project away intermediate results */

	/* If this is a distinct query with window funcs, the dupselim 
	** Aggr() needs to be outside the projection. Otherwise, the dupselim
	** gets merged in a variety of ways. */
	if (distinct && dupselim)
	    opc_x100_printf(x100cb, "Aggr (");

	/* If there are multiple window defs, we need to emit some joins. */
	if (subquery->ops_window_cnt > 1)
	{
	    DB_ATT_ID	attrid;

	    for (i = 1; i < subquery->ops_window_cnt; i++)
		opc_x100_printf(x100cb, "HashJoin01( ");

	    /* Create reuse name to cache original raw input. */
	    STprintf(winname, "IIWINSQNAME%-u", subquery->ops_tsubquery);
	    opc_x100_printf(x100cb, winname);
	    opc_x100_printf(x100cb, " = ");

	    /* And create RESDOM to define row number column used for joins. */
	    attrid.db_att_id = OPZ_NOATTR;
	    wrsdmp = opv_resdom(global, 
		opv_varnode(global, &dbdvint8, OPV_NOVAR, &attrid), &dbdvint8);
	}

	/* Project to compute diff/rediff & window functions. */
	opc_x100_printf(x100cb, "Project (");

	/* Check for presence of window functions requiring partition counts. */
	if (subquery->ops_window_cjoin != NULL && subquery->ops_window_cjoin[0])
	{
	    /* If there's at least one partition column, it's a MergeJoin1.
	    ** Otherwise, it must be CP join. */
	    if ((*winpp)->pst_sym.pst_type == PST_WINDOW &&
		(*winpp)->pst_sym.pst_value.pst_s_win.pst_pcount > 0)
	    {
		opc_x100_printf(x100cb, "MergeJoin1( ");
		CPpart = FALSE;
	    }
	    else
	    {
		opc_x100_printf(x100cb, "CartProd( ");
		CPpart = TRUE;
	    }
	    cjoin = TRUE;

	    /* Create reuse name to cache raw input to count ops. */
	    STprintf(partname, "IIWINCOUNT%-u_%-u", subquery->ops_tsubquery, 0);
	    opc_x100_printf(x100cb, partname);
	    opc_x100_printf(x100cb, " = ");
	}
	else cjoin = FALSE;

	/* Partitioning or ordering columns require Sort(). */
	if (!nowsort)
	    opc_x100_printf(x100cb, "Sort (");

	/* Expressions in order by require Project(). */
	if (obexpr)
	    opc_x100_printf(x100cb, "Project (");
    }
	
    if (agg && subquery->ops_gentry >= 0)
	gvarp = global->ops_rangetab.opv_base->opv_grv[subquery->ops_gentry];

    /* A single Ingres action header can produce numerous combinations of 
    ** X100 Project(), [Ord]Aggr(), Sort(), TopN() and Window() operators.
    ** The sequence is as follows (remembering that the later ops are nested 
    ** within the earlier ones, and that the streams created by each are input
    ** to the preceding ones:
    ** 1. Project() - optional, for discarding non-print RESDOMs that were 
    ** 		required for a Sort(),
    ** 2. Sort(), TopN(), Window(), if ORDER BY/FIRST N is requested,
    ** 3. Project() - optional, for MAIN subquery to produce final result
    **		buffer format. The "stream" layout of all other subqueries 
    **		is not important, hence only MAINs may have this Project(),
    ** 4. [Ord]Aggr() - optional, to compute aggregates,
    ** 5. [Ord]Aggr() - optional, to eliminate duplicates for aggregate
    **		distinct operations (they require 2 [Ord]Aggr()s),
    ** 6. Project() - optional, to compute GROUP BY expressions and expressions
    **		that are aggregate parameters,
    ** 7. Project() - optional, if there are no aggregates to compute, this
    **		is THE Project(),
    ** 8. [Ord]Aggr() - optional, to perform duplicates elimination before 
    **		the rest of the projection/aggregation (when dups elimination
    **		DB_RSORT is atop CO-tree for subquery),
    ** 9. finally, the input stream to the whole sequence of operations.
    **
    ** The next chunk of code determines which of the ops are needed and 
    ** may build lists of RESDOMs representing inputs to each.
    */

    /* If this really is Project/Aggr, emit operation. */
    if (cop->opo_sjpr <= OPO_REFORMATBASE)
    {
	/* Look for common subexpressions in projection list. */
	opc_expropt(subquery, (OPC_NODE *) NULL, 
		&subquery->ops_root->pst_left, &commexpr);
	if (commexpr != (PST_QNODE *) NULL)
	{
	    /* Common subexpressions were found. Do something with 'em. */
	    if (agg)
	    {
		/* Search for non-aggregate common expressions. They need
		** to be Project'ed before the Aggr. */
		for (rsdmp = commexpr, oldrsdmp = (PST_QNODE *) NULL; rsdmp; )
		 if (!opc_x100_aoptest(rsdmp->pst_right))
		 {
		    /* Add non-agg expression to commproj and remove from
		    ** commexpr. */
		    tempp = rsdmp->pst_left;		/* save link */

		    /* Remove from commexpr chain. */
		    if (oldrsdmp == (PST_QNODE *) NULL)
			commexpr = tempp;
		    else oldrsdmp->pst_left = tempp;
		    rsdmp->pst_left = (PST_QNODE *) NULL;

		    /* Append to commproj chain. */
		    if (commproj == (PST_QNODE *) NULL)
		    {
			commproj = commp1 = rsdmp;
		    }
		    else
		    {
			commp1->pst_left = rsdmp;
			commp1 = rsdmp;
		    }
		    rsdmp = tempp;			/* advance rsdmp */
		 }
		 else
		 {
		    oldrsdmp = rsdmp;			/* advance old & curr */
		    rsdmp = rsdmp->pst_left;
		 }
	    }

	    /* If there's common subexpressions, stick them in front of
	    ** the subquery's RESDOM chain. */
	    if (commexpr != (PST_QNODE *) NULL)
	    {
		if (subquery->ops_sqtype == OPS_MAIN ||
		    subquery->ops_sqtype == OPS_VIEW ||
		    subquery->ops_sqtype == OPS_UNION)
		{
		    /* Common subexpressions for the MAIN subquery (or UNION
		    ** subqueries) must be computed before the final project 
		    ** (but after any [Ord]Aggr) to prevent mucking up the 
		    ** layout of the result buffer. */
		    for (rsdmp = commexpr; rsdmp && rsdmp->pst_sym.
			pst_type == PST_RESDOM; rsdmp = rsdmp->pst_left)
		     if (rsdmp->pst_left == (PST_QNODE *) NULL || 
			rsdmp->pst_sym.pst_type != PST_RESDOM)
		     {
			rsdmp->pst_left = subquery->ops_root->pst_left;
			break;
		     }

		    commproj2 = commexpr;
		    extraproj2 = TRUE;
		    if (!dupselim)
			opc_x100_printf(x100cb, "Project( ");
		}
		else
		{
		    /* If not MAIN subquery, project comes first. */
		    for (rsdmp = subquery->ops_root->pst_left, 
					oldrsdmp = (PST_QNODE *) NULL; 
			rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
			oldrsdmp = rsdmp, rsdmp = rsdmp->pst_left);
		    oldrsdmp->pst_left = commexpr;
		}
		commexpr = subquery->ops_root->pst_left;
	    }
	    else commexpr = subquery->ops_root->pst_left;
	}
	else commexpr = subquery->ops_root->pst_left;

	/* commexpr should now address the projection list (Aggr or Project)
	** and commproj, if set, will be the Project list (if any) nested in
	** the Aggr. */

	if (agg)
	{
	    /* There may still be scalar expressions nested in the 
	    ** aggregation operators - not common subexpressions that would
	    ** have been extracted earlier. These must also be removed to
	    ** the "common projection" list and replaced in the aggregation
	    ** lists by corresponding PST_VAR nodes. The same goes for
	    ** expressions outside of aggregations - they are expressions
	    ** that will also appear in the group by list. */
	    for (oldrsdmp = commproj; oldrsdmp && 
			oldrsdmp->pst_left != (PST_QNODE *) NULL; 
			oldrsdmp = oldrsdmp->pst_left);	/* end of commproj */

	    for (rsdmp = commexpr; rsdmp && 
		rsdmp->pst_sym.pst_type == PST_RESDOM; rsdmp = rsdmp->pst_left)
	     if (((tempp = rsdmp->pst_right)->pst_sym.pst_type == PST_AOP &&
			tempp->pst_left != (PST_QNODE *) NULL &&
			tempp->pst_left->pst_sym.pst_type != PST_VAR) ||
		(tempp->pst_sym.pst_type != PST_VAR &&
			tempp->pst_sym.pst_type != PST_AOP))
	     {
		PST_QNODE	**temp1p;
		DB_ATT_ID	attrid;
		/* Got an expression. Make RESDOM to attach it
		** to end of commproj, then a PST_VAR to replace it in 
		** commexpr. */
		if (tempp->pst_sym.pst_type == PST_AOP)
		    temp1p = &tempp->pst_left;
		else 
		{
		    /* Tedious as it looks - the group by expressions we
		    ** are interested in all have blank names. */
		    opc_x100_copy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
				sizeof(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
				rsname, sizeof(rsname));
		    if (rsname[0] == 0)
			temp1p = &rsdmp->pst_right;
		    else continue;
		}
		commp1 = opv_resdom(global, *temp1p, 
					&(*temp1p)->pst_sym.pst_dataval);
		if (commproj == (PST_QNODE *) NULL)
		    commproj = commp1;
		else oldrsdmp->pst_left = commp1;
		oldrsdmp = commp1;
		commp1->pst_left = (PST_QNODE *) NULL;

		attrid.db_att_id = OPZ_NOATTR;
		(*temp1p) = opv_varnode(global, 
			&tempp->pst_sym.pst_dataval, OPV_NOVAR, &attrid);

		/* Copy RESDOM name to VAR name. */
		MEcopy((char *)&commp1->pst_sym.pst_value.pst_s_rsdm.
		    pst_rsname, sizeof(DB_ATT_NAME),
		    (char *)&(*temp1p)->pst_sym.pst_value.pst_s_var.pst_atname);
		/* And to orig. RESDOM if this is GB expr. */
		if (tempp->pst_sym.pst_type != PST_AOP)
		    MEcopy((char *)&commp1->pst_sym.pst_value.pst_s_rsdm.
			pst_rsname, sizeof(DB_ATT_NAME),
			(char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname);

		(*temp1p)->pst_right = commp1->pst_right;
					/* save expr to be restored later */
	     }

	    /* Now check for aggregate distinct operations. There are 
	    ** limits on what combinations are allowed and, if this one 
	    ** passes, we need an extra "Aggr()" to distinctify the input. */
	    if ((aggdist = opc_x100_aggdistck(commexpr, &distvarp)))
	    {
		distrsdmp = opv_resdom(global, distvarp, 
					&distvarp->pst_sym.pst_dataval);
		clagg = FALSE;
	    }

	    /* Aggregate - but also check for projected common exprs. There
	    ** can be up to 3 in a row if the agg is distinct and the aggregate
	    ** input has duplicates removed - dupselim isn't the same as
	    ** aggdist, but there should only be one Aggr(). aggdist takes
	    ** precedence because it will remove more rows (fewer columns). */
	    opc_x100_printf(x100cb, (!aggdist && clagg) ? 
						"OrdAggr (" : "Aggr (");

	    if (aggdist)
		opc_x100_printf(x100cb, "Aggr (");
	    else if (dupselim && (!winfuncs || !distinct))
		opc_x100_printf(x100cb, (orddupselim) ? "OrdAggr (" : "Aggr (");

	    if (commproj != (PST_QNODE *) NULL || (dupselim && (!winfuncs ||
			!distinct)))
		opc_x100_printf(x100cb, "Project (");
	}
	else if (!winfuncs && (!(x100cb->flag & X100_DELETE) || 
			(subquery->ops_sqtype != OPS_MAIN && 
			subquery->ops_sqtype != OPS_VIEW &&
			subquery->ops_sqtype != OPS_UNION)))
	    opc_x100_printf(x100cb, "Project (");
    }
    if (Opf_x100_debug>1 && commproj)
	opt_pt_entry(global, NULL, commproj, "Project before Aggr:", 
							FALSE, TRUE, FALSE);
    if (Opf_x100_debug>1 && commexpr)
	opt_pt_entry(global, NULL, commexpr, "Common aggs: ", 
							FALSE, TRUE, FALSE);

    if (!agg && dupselim && (!winfuncs || !distinct))
    {
	/* Dups elimination of non-aggregates is set up here. If there are 
	** non-printing RESDOMs, an extra Project() is required to include 
	** them - and in either event, an [Ord]Aggr() is required. */
	opc_x100_printf(x100cb, (orddupselim) ? "OrdAggr (" : "Aggr (");
	for (rsdmp = subquery->ops_root->pst_left; rsdmp && 
		rsdmp->pst_sym.pst_type == PST_RESDOM; 
		rsdmp = rsdmp->pst_left)
	{
	    if (!dups_noprint && !(rsdmp->pst_sym.pst_value.pst_s_rsdm.
		pst_rsflags & PST_RS_PRINT))
		dups_noprint = TRUE;
	    if (rsdmp->pst_right && rsdmp->pst_right->pst_sym.pst_type != 
					PST_VAR)
	    {
		PST_QNODE	*tptr;
		DB_ATT_ID	attrid;
		attrid.db_att_id = OPZ_NOATTR;

		if (IS_A_PST_OP_NODE(rsdmp->pst_right->pst_sym.pst_type) &&
		    (rsdmp->pst_right->pst_sym.pst_value.pst_s_op.
						pst_flags & PST_WINDOW_OP))
		{
		    rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |=
						PST_RS_WINNOEMIT;
		    noemit = TRUE;
		    continue;
		}

		prexpr = TRUE;

		tptr = rsdmp->pst_right;
		rsdmp->pst_right = opv_varnode(global, &rsdmp->pst_right->
				pst_sym.pst_dataval, OPV_NOVAR, &attrid);
		MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
		    sizeof(DB_ATT_NAME), 
		    (char *)&rsdmp->pst_right->pst_sym.pst_value.
		    pst_s_var.pst_atname);	/* copy RESDOM name to VAR */
		rsdmp->pst_right->pst_left = tptr;	/* attach expr to
							** VAR node */
	    }
	}
	if (dups_noprint || prexpr)
	    opc_x100_printf(x100cb, "Project (");

    }

    /* Check for SAGGs and emit CartProd()s, as needed. */
    if (subquery->ops_sqtype != OPS_SAGG &&
	subquery->ops_compile.opc_prsaggnext != (OPS_SUBQUERY *) NULL)
    {
	opc_x100_printf(x100cb, "CartProd( ");
	cp = TRUE;
    }
    else cp = FALSE;

    /* Build stream for Project/Aggr. */
    opc_x100_co(x100cb, cop, FALSE, FALSE);

    /* Check for SAGGs and emit SAGG syntax, as needed. */
    if (cp)
    {
	opc_x100_printf(x100cb, ", ");
	opc_x100_prsaggemit(x100cb, subquery);
	opc_x100_printf(x100cb, ", 1) ");
    }

    /* Loop over expressions to project/aggregate. */
    if (cop->opo_sjpr <= OPO_REFORMATBASE &&
	(dupselim || !(x100cb->flag & X100_DELETE) ||
		subquery->ops_sqtype != OPS_MAIN))
    {
	/* If aggregate, check for projected common subexprs to emit,
	** then also the BY-list. */
	if (agg)
	{
	    if (commproj || (dupselim && (!winfuncs || !distinct)))
	    {
		PST_QNODE	**insp;
		/* Loop over Aggr() list, looking for other PST_VARs
		** to Project. Then do the GROUP BY list, too. Project()
		** introduced to pre-compute scalar expressions for Aggr()
		** must include all other columns referenced later. */

		/* First find point of insertion. */
		if (commproj)
		{
		    for (commp1 = commproj; commp1->pst_left && 
			commp1->pst_left->pst_sym.pst_type == PST_RESDOM; 
			commp1 = commp1->pst_left);
		    insp = &commp1->pst_left;
		}
		else insp = &commproj;

		for (rsdmp = commexpr; rsdmp && 
			rsdmp->pst_sym.pst_type == PST_RESDOM; 
			rsdmp = rsdmp->pst_left)
		    opc_x100_addVARs(global, &commproj, rsdmp->pst_right,
			(dupselim && rsdmp->pst_right->
				pst_sym.pst_type == PST_VAR) ?
					rsdmp : (PST_QNODE *) NULL);
		for (rsdmp = subquery->ops_byexpr; rsdmp && 
			rsdmp->pst_sym.pst_type == PST_RESDOM; 
			rsdmp = rsdmp->pst_left)
		 if (rsdmp->pst_right->pst_sym.pst_type == PST_VAR)
		    opc_x100_addVARs(global, &commproj, rsdmp->pst_right,
			(dupselim) ? rsdmp : (PST_QNODE *) NULL);
		 else
		 {
		    DB_ATT_ID	attrid;
		    /* Expression in group by list - Project() must evaluate 
		    ** it first so copy whole thing to commproj and replace
		    ** with fake PST_VAR. */
		    commp1 = opv_resdom(global, rsdmp->pst_right,
					&rsdmp->pst_sym.pst_dataval);
		    if (!(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
						PST_RS_PRINT))
			commp1->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &=
						~PST_RS_PRINT;
		    MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.
			pst_rsname, sizeof(DB_ATT_NAME),
			(char *)&commp1->pst_sym.pst_value.pst_s_rsdm.
			pst_rsname);		/* copy name back again */
		    commp1->pst_left = *insp;
		    *insp = commp1;

		    attrid.db_att_id = OPZ_NOATTR;
		    rsdmp->pst_right = opv_varnode(global, 
			&rsdmp->pst_sym.pst_dataval, OPV_NOVAR, &attrid);
		    /* Copy RESDOM name to VAR name. */
		    MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.
			pst_rsname, sizeof(DB_ATT_NAME),
			(char *)&rsdmp->pst_right->pst_sym.pst_value.pst_s_var.
							pst_atname);
		 }	

		/* Finally, do the introduced Project(). */
		opc_x100_presdom(x100cb, cop, commproj, TRUE, FALSE, 
				(dupselim && !aggdist) ? TRUE : FALSE, FALSE);
		opc_x100_printf(x100cb, ")");
	    }

	    /* Now check for an aggregate distinct. If there is one, we
	    ** now emit the "group by" list - group by columns plus the
	    ** distinct column to make it distinct. And no computations so
	    ** distinct stream flows into "real" Aggr(). Otherwise, check
	    ** for dupselim. aggdist takes precedence because it involves 
	    ** (possibly) fewer columns and will therefore remove more rows. */
	    if (aggdist)
	    {
		distrsdmp->pst_left = subquery->ops_byexpr;
		opc_x100_derck(subquery, distrsdmp, FALSE);
		opc_x100_presdom(x100cb, cop, distrsdmp, FALSE, FALSE, 
								FALSE, TRUE);
		opc_x100_printf(x100cb, ", [ ])");
	    }
	    else if (dupselim && (!winfuncs || !distinct))
	    {
		opc_x100_derck(subquery, commproj, TRUE);
		opc_x100_presdom(x100cb, cop, commproj, TRUE, FALSE, 
								TRUE, TRUE);
		opc_x100_printf(x100cb, ", [ ])");
	    }

	    /* Now the "real" Aggr(). Emit group by list. */
	    opc_x100_derck(subquery, subquery->ops_byexpr, FALSE);
	    opc_x100_presdom(x100cb, cop, subquery->ops_byexpr, TRUE, FALSE, 
								FALSE, TRUE);

	    /* Remove GROUP BY columns (if any) from Aggr() computations. */
	    for (tempp = subquery->ops_byexpr; tempp && 
			tempp->pst_sym.pst_type == PST_RESDOM; 
			tempp = tempp->pst_left)
	     for (rsdmp = commexpr, oldrsdmp = (PST_QNODE *) NULL; rsdmp && 
			rsdmp->pst_sym.pst_type == PST_RESDOM; 
			oldrsdmp = rsdmp, rsdmp = rsdmp->pst_left)
	     {
		/* Search for a match and remove from chain. */
		if (MEcmp((char *)&tempp->pst_sym.pst_value.
			pst_s_rsdm.pst_rsname, 
			(char *)&rsdmp->pst_sym.pst_value.
			pst_s_rsdm.pst_rsname, sizeof(DB_ATT_NAME)) == 0)
		    rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |=
								PST_RS_X100GB;
	     }
	}	/* end of "if (agg)" */
	else if (dupselim && (!winfuncs || !distinct))
	{
	    /* Non-aggregate duplicates elimination. First see if an
	    ** extra Project() for non-printing RESDOMs is required. */
	    if (dups_noprint || prexpr)
	    {
		opc_x100_presdom(x100cb, cop, (extraproj2) ? commproj2 : commexpr, 
						TRUE, FALSE, TRUE, FALSE);
		opc_x100_printf(x100cb, ")");
	    }

	    /* Then emit dupselim column list. */
	    opc_x100_derck(subquery, commexpr, TRUE);
	    opc_x100_presdom(x100cb, cop, commexpr, FALSE, FALSE, TRUE, TRUE);
	    opc_x100_printf(x100cb, ", [ ])");
	}

	/* If extraproj2, this is top subquery and common subexpressions 
	** must be projected first. */
	if (extraproj2)
	{
	    if (!dupselim)
	    {
		opc_x100_presdom(x100cb, cop, commproj2, TRUE, FALSE, 
								TRUE, FALSE);
		opc_x100_printf(x100cb, ")");
	    }

	    /* After one of these, scan final Project list (in commexpr)
	    ** for expressions and use "new" name. */
	    for (rsdmp = commexpr; rsdmp && 
			rsdmp->pst_sym.pst_type == PST_RESDOM;
			rsdmp = rsdmp->pst_left)
	     if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
							PST_RS_RENAMED)
	     {
		/* Make new PST_VAR and copy RESDOM's name to it. */
		DB_ATT_ID	attrid;
		attrid.db_att_id = OPZ_NOATTR;
		rsdmp->pst_right = opv_varnode(global, &rsdmp->pst_right->
				pst_sym.pst_dataval, OPV_NOVAR, &attrid);
		MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
		    sizeof(DB_ATT_NAME), 
		    (char *)&rsdmp->pst_right->pst_sym.pst_value.
		    pst_s_var.pst_atname);	/* copy RESDOM name to VAR */
	     }
	}

	/* If there are window functions, we emit the 1st partitioning/ordering
	** sort list here. */
	if (winfuncs)
	{
	    if (obexpr)
	    {
		/* There are expressions in the sort list. Project them 1st. */
		for (rsdmp = obrsdmp; rsdmp->pst_left != NULL &&
			rsdmp->pst_left->pst_sym.pst_type == PST_RESDOM;
		    rsdmp = rsdmp->pst_left)
		    ;
		rsdmp->pst_left = commexpr;	/* append the rest */
		opc_x100_presdom(x100cb, cop, obrsdmp, TRUE, TRUE, TRUE, FALSE);
		opc_x100_printf(x100cb, ")");	/* finish Project() */
	    }

	    if (!nowsort)
		opc_x100_winsort(subquery, x100cb, 0, FALSE);

	    /* If there's a partition count to be computed, extra stuff
	    ** needs to be added here. */
	    if (cjoin)
	    {
		/* First, emit join columns (if it is MergeJoin1). */
		if (CPpart)
		    opc_x100_printf(x100cb, ", ");
		else
		{
		    opc_x100_winsort(subquery, x100cb, 0, TRUE);
		    opc_x100_printf(x100cb, "], ");
		}

		/* Then the OrdAggr() to do the count. */
		opc_x100_printf(x100cb, (CPpart) ? "Aggr( " : "OrdAggr( ");
		opc_x100_printf(x100cb, partname);

		/* GROUP BY list (if any) for count. */
		if (CPpart)
		    opc_x100_printf(x100cb, ", [ ], ");
		else
		{
		    opc_x100_winsort(subquery, x100cb, 0, TRUE);
		    opc_x100_printf(x100cb, "], ");
		}

		/* Add count computation. */
		opc_x100_printf(x100cb, "[ _IISQCOUNT0 = count(*)])");

		/* Join columns (again, if any) for 2nd MergeJoin1 flow. */
		if (!CPpart)
		{
		    opc_x100_winsort(subquery, x100cb, 0, TRUE);
		    opc_x100_printf(x100cb, "] [_IISQCOUNT0]");
		}
		
		opc_x100_printf(x100cb, ")");	/* and we're done */
	    }

	    /* Find end of commexpr and winfp to splice RESDOM chains. */
	    if (winfp != NULL)
	    {
		for (rsdmp = commexpr; rsdmp->pst_left != NULL &&
		    rsdmp->pst_left->pst_sym.pst_type == PST_RESDOM;
		    rsdmp = rsdmp->pst_left)
		    ;
		for (wrsdmp = winfp; wrsdmp->pst_left != NULL &&
		    wrsdmp->pst_left->pst_sym.pst_type == PST_RESDOM;
		    wrsdmp = wrsdmp->pst_left)
		    ;
		wrsdmp->pst_left = rsdmp->pst_left;
		rsdmp->pst_left = winfp;
	    }

	    /* And finally extinguish the NOEMIT flags from the 1st set
	    ** of window functions. */
	    if (obexpr | noemit)
	     for (i = 0; (rsdmp = winrsdmpp[i]) != NULL; i++)
	      rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &=
				~PST_RS_WINNOEMIT;
	}

	/* Do the main projection for this subquery. */ 
	if (agg || !(x100cb->flag & X100_DELETE))
	    opc_x100_presdom(x100cb, cop, commexpr, TRUE, TRUE, 
					(extraproj1) ? TRUE : FALSE, FALSE);

	/* For subqueries with more than one window definition, loop over
	** 2nd and subsequent - emitting join specs and diff/rediff and
	** functions to process them. */
	STprintf(rowname0, "_IIWINRNUM%-u%-u", subquery->ops_tsubquery, (i4) 0);
	for (i = 1; i < subquery->ops_window_cnt; i++)
	{
	    PST_QNODE	**winpp = subquery->ops_winfuncs[i];
	    char	rowname1[16], rowname2[16], cntname[16];

	    /* Check for need to merge partition counts. */
	    if (subquery->ops_window_cjoin != NULL &&
		subquery->ops_window_cjoin[i])
		cjoin = TRUE;
	    else cjoin = FALSE;

	    /* For each window definition, emit "[IIWINROWNUM]" join keys +
	    ** Project() of part/order cols, Sort() of same and Project()
	    ** of diff/rediff/function values. */
	    STprintf(rowname1, "_IIWINRNUM%-u%-u", 
					subquery->ops_tsubquery, i-1);
	    STprintf(rowname2, "_IIWINRNUM%-u%-u", subquery->ops_tsubquery, i);
	    opc_x100_printf(x100cb, "), [");
	    opc_x100_printf(x100cb, rowname1);
	    opc_x100_printf(x100cb, "], Project( ");

	    if (cjoin)
	    {
		/* Must inject additional code for processing functions
		** requiring partition counts (ntile, percent_rank, ...). */

		/* If there's at least one partition column, it's a MergeJoin1.
		** Otherwise, it must be CP join. */
		if ((*winpp)->pst_sym.pst_type == PST_WINDOW &&
		    (*winpp)->pst_sym.pst_value.pst_s_win.pst_pcount > 0)
		{
		    opc_x100_printf(x100cb, "MergeJoin1( ");
		    CPpart = FALSE;
		}
		else
		{
		    opc_x100_printf(x100cb, "CartProd( ");
		    CPpart = TRUE;
		}

		/* Create reuse name to cache raw input to count ops. */
		STprintf(partname, "IIWINCOUNT%-u_%-u", subquery->
			ops_tsubquery, i);
		opc_x100_printf(x100cb, partname);
		opc_x100_printf(x100cb, " = ");
		STprintf(cntname, "_IISQCOUNT%-u", i);
	    }

	    /* Rest of stuff to compute window functions. */
	    opc_x100_printf(x100cb, "Sort( Project(");
	    opc_x100_printf(x100cb, winname);
	    opc_x100_printf(x100cb, ", [");
	    opc_x100_printf(x100cb, rowname2);
	    opc_x100_printf(x100cb, " = ");
	    opc_x100_printf(x100cb, rowname0);

	    /* Dump sort columns. */
	    for (rsdmp = (winpp[0])->pst_right->pst_right->pst_left, 
			fill = ", ";
		rsdmp != NULL && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
	    {
		opc_x100_printf(x100cb, fill);
		opc_x100_prexpr(x100cb, cop, rsdmp->pst_right, NULL, 
				ADI_NILCOERCE);
	    }
	    opc_x100_printf(x100cb, "])");	/* finish inner Project() */

	    winfp = opc_x100_winfunc_rewrite(subquery, x100cb, &winrsdmpp,
				i, FALSE);
	    opc_x100_winsort(subquery, x100cb, i, FALSE);

	    /* If there's a partition count to be computed, extra stuff
	    ** needs to be added here. */
	    if (cjoin)
	    {
		/* First, emit join columns (if it is MergeJoin1). */
		if (CPpart)
		    opc_x100_printf(x100cb, ", ");
		else
		{
		    opc_x100_winsort(subquery, x100cb, i, TRUE);
		    opc_x100_printf(x100cb, "], ");
		}

		/* Then the OrdAggr() to do the count. */
		opc_x100_printf(x100cb, (CPpart) ? "Aggr( " : "OrdAggr( ");
		opc_x100_printf(x100cb, partname);

		/* GROUP BY list (if any) for count. */
		if (CPpart)
		    opc_x100_printf(x100cb, ", [ ], ");
		else
		{
		    opc_x100_winsort(subquery, x100cb, 0, TRUE);
		    opc_x100_printf(x100cb, "], ");
		}

		/* Add count computation. */
		opc_x100_printf(x100cb, "[ ");
		opc_x100_printf(x100cb, cntname);
		opc_x100_printf(x100cb, " = count(*)])");

		/* Join columns (again, if any) for 2nd MergeJoin1 flow. */
		if (!CPpart)
		{
		    opc_x100_winsort(subquery, x100cb, i, TRUE);
		    opc_x100_printf(x100cb, "] [");
		    opc_x100_printf(x100cb, cntname);
		    opc_x100_printf(x100cb, "]");
		}
		
		opc_x100_printf(x100cb, ")");	/* and we're done */
	    }

	    wrsdmp->pst_left = winfp;		/* prepend ROWNUM column */
	    MEcopy((char *)&rowname2[1], sizeof(rowname1)-1, (char *)&wrsdmp->
			pst_sym.pst_value.pst_s_rsdm.pst_rsname);
	    MEcopy((char *)&rowname2[1], sizeof(rowname1)-1, (char *)&wrsdmp->
			pst_right->pst_sym.pst_value.pst_s_var.pst_atname);
	    opc_x100_presdom(x100cb, cop, wrsdmp, TRUE, TRUE, TRUE, FALSE);
						/* outer Project() */
	    opc_x100_printf(x100cb, "), [");
	    opc_x100_printf(x100cb, rowname2);
	    opc_x100_printf(x100cb, "]");	/* and finish the join */
	}

	if (agg)
	{

	    /* If hash aggr, emit group estimate. */
	    if (!clagg)
	     if (gvarp && gvarp->opv_gcost.opo_tups > 5)
	     {
		/* Format group count estimate into Aggr(). */
		char	tcount[20];

		STprintf(tcount, ", %.0f", gvarp->opv_gcost.opo_tups);
		opc_x100_printf(x100cb, &tcount[0]);
	     }
	     else opc_x100_printf(x100cb, ", 200");	/* hack default */

	    /* Finally, if there is a GROUP BY list, remove PST_RS_X100GB
 	    ** from them now that we're done. */
	    for (rsdmp = subquery->ops_byexpr; rsdmp && 
		rsdmp->pst_sym.pst_type == PST_RESDOM; rsdmp = rsdmp->pst_left)
		rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &= 
							~PST_RS_X100GB;
	}

	if (agg || !(x100cb->flag & X100_DELETE))
	    opc_x100_printf(x100cb, ")");

    }

    /* If this is a DISTINCT query with window functions, the dupselim
    ** Aggr() is completed here. */
    if (winfuncs && distinct && dupselim)
    {
	opc_x100_derck(subquery, commexpr, TRUE);
	opc_x100_presdom(x100cb, cop, commexpr, TRUE, FALSE, FALSE, TRUE);
	opc_x100_printf(x100cb, ", [ ])");
    }

    if (sort)
    {
	/* Final step - emit sort columns and (if necessary) TopN limit. */
	opc_x100_printf(x100cb, ",[");
	fill = "";

	for (tempp = global->ops_qheader->pst_stlist;
		tempp && tempp->pst_sym.pst_type == PST_SORT; 
		tempp = tempp->pst_right)	/* loop over sort cols */
	 for (rsdmp = subquery->ops_root->pst_left;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)	/* loop over RESDOMs */
	  if (tempp->pst_sym.pst_value.pst_s_sort.pst_srvno ==
		rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno)
	  {
	    PST_QNODE	*workp;

	    opc_x100_printf(x100cb, fill);	/* blank or comma */

	    /* Emit column name (qualified or not). */
	    workp = (rsdmp->pst_right && rsdmp->pst_right->
		pst_sym.pst_type == PST_VAR) ? rsdmp->pst_right : rsdmp;
/* Surely this "if" is wrong?? */
	    if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &
			PST_RS_RENAMED && 
		rsdmp->pst_sym.pst_type != PST_VAR)
		workp = rsdmp;			/* use unqualified new name */

	    /* Ugly hack for UPDATEs - can only be "tid". */
	    if ((x100cb->flag & X100_MODIFY) && 
				subquery->ops_sqtype == OPS_MAIN)
		opc_x100_printf(x100cb, "tid");
	    else opc_x100_prexpr(x100cb, cop, workp, (DB_DATA_VALUE *)NULL, 
				ADI_NILCOERCE);

	    if (tempp->pst_sym.pst_value.pst_s_sort.pst_srasc != 1)
		opc_x100_printf(x100cb, " DESC");
	    fill = ", ";
	    break;		/* leave inner loop */
	  }

	if (topn > 0)
	{
	    STprintf(firstout, "], %d)", topn+offn);
	    fill = &firstout[0];
	}
	else fill = "])";

	opc_x100_printf(x100cb, fill);
    }
    if ((!sort && topn > 0) || offn)
    {
	/* If there are window functions with a "first N", the "extraproj1"
	** projection is done here, not after. */
	if (extraproj1 && winfuncs)
	{
	    opc_x100_presdom(x100cb, cop, commexpr, TRUE, TRUE, FALSE, FALSE);
	    opc_x100_printf(x100cb, ")");
	    extraproj1 = FALSE;		/* disable following presdom call */
	}

	/* "first/offset N" with no sort - close out the Window(). */
	if (topn)
	    STprintf(firstout, ", [%d, %d])", offn, topn+offn);
	else STprintf(firstout, ", %d)", offn);
	opc_x100_printf(x100cb, firstout);
    }

    if (extraproj1)		/* this discards non-printing sort columns */
    {
	opc_x100_presdom(x100cb, cop, commexpr, TRUE, TRUE, FALSE, FALSE);
	opc_x100_printf(x100cb, ")");
    }

    return;

}

/*{
** Name: OPC_X100_WINFUNC_REWRITE	- modify RESDOM list to compute
**	window functions
**
** Description:
**	Replace window functions in RESDOM list with VW funcs used to 
**	generate them. opc_x100_presdom() will then produce the appropriate
**	code.
**
** Inputs:
**      subquery			ptr to subquery to check
**      x100cb				ptr to X100 text state variable
**	winix				index into ops_winfuncs array
**
** Outputs:
**	None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-may-2009 (dougi)
**	    Written for window functions.
*/
static PST_QNODE *
opc_x100_winfunc_rewrite(
	OPS_SUBQUERY	*subquery,
	X100_STATE	*x100cb,
	PST_QNODE	***oldrsdmpp,
	i4		winix,
	bool		noemit)
{
    OPS_STATE	*global = subquery->ops_global;
    PST_QNODE	*opp, **oppp, *prsdmp, *drsdmp, *rrsdmp, *rsdmp, *newstuff;
    PST_QNODE	**funcpp, **oldwinp, **newwinp, *dvarp, *redvarp, *wrsdmp;
    PST_QNODE	*nrsdmp, *fvarp, *cvarp, *casep, *whlistp, *ravarp, *rarsdmp;
    PST_QNODE	*rorsdmp, *rovarp;
    DB_ATT_ID	attrid;
    ADI_OP_ID	opno;
    i4		i, j, numpart, numord;
    DB_ERROR	error;
    char	countname[16];
    bool	cfuncs = FALSE;

    /* This function generates a list of RESDOM nodes to compute the 
    ** required intermediate results, etc. to produce the results for 
    ** various ranking functions (rank(), dense_rank() and row_number() for 
    ** now). The RESDOMs are tacked to the end of the existing chain so that
    ** they will be evaluated in the correct sequence with the remainder 
    ** of the subquery results. */
    attrid.db_att_id = OPZ_NOATTR;
    (*oldrsdmpp) = oldwinp = subquery->ops_winfuncs[winix];
    for (i = 0; oldwinp[i] != NULL; i++)
	;						/* count 'em up */
    newwinp = (PST_QNODE **) opu_memory(global, (i+1) * sizeof(PST_QNODE *));
    newwinp[i] = NULL;
    newstuff = prsdmp = ravarp = rovarp = NULL;
    if (subquery->ops_window_cjoin != NULL &&
	    subquery->ops_window_cjoin[winix])
    {
	cvarp = opv_varnode(global, &dbdvint8, OPV_NOVAR, &attrid);
	STprintf(countname, "IISQCOUNT%-u", winix);
	opc_x100_copy(countname, STlength(countname),
	    (char *)&cvarp->pst_sym.pst_value.pst_s_var.pst_atname, DB_MAXNAME);
	cfuncs = TRUE;
    }
    else cvarp = NULL;

    for (i = 0, newstuff = NULL;
	    (wrsdmp = oldwinp[i]) != NULL;
	    i++)
    {
	/* Rewrite current window function - first check for partitioning
	** and/or ordering columns. */
	funcpp = &wrsdmp->pst_right;
	if (!noemit)
	    wrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &= 
				~PST_RS_WINNOEMIT;
	numpart = (*funcpp)->pst_right->pst_sym.pst_value.pst_s_win.pst_pcount;
	numord = (*funcpp)->pst_right->pst_sym.pst_value.pst_s_win.pst_ocount;

	opno = (*funcpp)->pst_sym.pst_value.pst_s_op.pst_opno;
	switch (opno) {

	  case ADI_RANK_OP:
	  case ADI_DRANK_OP:
	  case ADI_ROWNUM_OP:
	  case ADI_PRANK_OP:
	  case ADI_NTILE_OP:
	    /* Shared building of "diff(), rediff()". diff() first. */
	    if (i == 0)
	    {
		/* For first function, generate diff/rediff functions. */
		dvarp = redvarp = NULL;
		if (numpart == 0 && numord == 0)
		    goto skiprediff;		/* must be row_number() */

		/* Make RESDOM for partitioning/ordering diff(). */
		drsdmp = opv_resdom(global, NULL, &dbdvint8);
		drsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = 0;

		/* diff() uses partitioning columns, if there, otherwise
		** ordering columns (must be ranking function). */
		for (rsdmp = (*funcpp)->pst_right->pst_left, j = 0,
			oppp = &drsdmp->pst_right;
		    (numpart > 0 && j < numpart) || 
			(numpart <= 0 && j < numord);
		    rsdmp = rsdmp->pst_left, oppp = &opp->pst_left, j++)
		{
		    opp = (*oppp) = opv_opnode(global, PST_BOP,
				ADI_VWREDIFF_OP, OPL_NOOUTER);
		    STRUCT_ASSIGN_MACRO(dbdvint8, opp->pst_sym.pst_dataval);
		    opp->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid = 
				ADI_NILCOERCE;
		    opp->pst_right = rsdmp->pst_right;
		}
		/* diff() is innermost and is a UOP - so reset. */
		opp->pst_sym.pst_type = PST_UOP;
		opp->pst_left = opp->pst_right;
		opp->pst_right = NULL;
		opp->pst_sym.pst_value.pst_s_op.pst_opno = ADI_VWDIFF_OP;

		/* Make VAR node from it. */
		dvarp = opv_varnode(global, &drsdmp->pst_sym.pst_dataval,
			OPV_NOVAR, &attrid);
		MEcopy((char *)&drsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		    sizeof(DB_ATT_NAME),
		    (char *)&dvarp->pst_sym.pst_value.pst_s_var.pst_atname);

		/* Next do the rediff(). */
		if (numord <= 0 || numpart <= 0)
		{
		    newstuff = prsdmp = drsdmp;
		    goto skiprediff;
		}


		/* Make RESDOM for ordering diff (on top of part. diff). */
		rrsdmp = opv_resdom(global, NULL, &dbdvint8);
		rrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = 0;

		/* rediff() uses partitioning diff combined with ordering 
		** columns. */
		for (oppp = &rrsdmp->pst_right;
		    rsdmp != NULL && rsdmp->pst_sym.pst_type == PST_RESDOM;
		    rsdmp = rsdmp->pst_left, oppp = &opp->pst_left)
		{
		    opp = (*oppp) = opv_opnode(global, PST_BOP,
				ADI_VWREDIFF_OP, OPL_NOOUTER);
		    STRUCT_ASSIGN_MACRO(dbdvint8, opp->pst_sym.pst_dataval);
		    opp->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid = 
				ADI_NILCOERCE;
		    opp->pst_right = rsdmp->pst_right;
		}

		/* Stick partitioning diff as left of last rediff(). */
		opp->pst_left = dvarp;

		/* And diff() follows rediff() on RESDOM chain (Project() goes
		** reverse.). */
		if (newstuff == NULL)
		{
		    newstuff = rrsdmp;
		    rrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno =
			rrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = 0;
		}
		rrsdmp->pst_left = drsdmp;
		drsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno =
		    drsdmp->pst_sym.pst_value.pst_s_rsdm.pst_ntargno = 
			rrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno-1;
		prsdmp = drsdmp;

		/* Make VAR node from it. */
		redvarp = opv_varnode(global, &rrsdmp->pst_sym.pst_dataval,
			OPV_NOVAR, &attrid);
		MEcopy((char *)&rrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		    sizeof(DB_ATT_NAME),
		    (char *)&redvarp->pst_sym.pst_value.pst_s_var.pst_atname);
	    }	/* end of "if (i == 0)" - 1st window func */

skiprediff:
	    /* If this is the first function of the first window, include 
	    ** generation of rowid as join column for other windows. */
	    if (subquery->ops_window_cnt > 1 && winix == 0 && i == 0)
	    {
		char	rowname[16];
		opp = opv_opnode(global, PST_UOP, ADI_VWROWID_OP, OPL_NOOUTER);
		STRUCT_ASSIGN_MACRO(dbdvint8, opp->pst_sym.pst_dataval);
		opp->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid = ADI_NILCOERCE;
		opp->pst_left = (*funcpp)->pst_right->pst_left;
		opv_copytree(global, &opp->pst_left);
		rrsdmp = opv_resdom(global, opp, &dbdvint8);
		rrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = 0;
		STprintf(rowname, "IIWINRNUM%-u%-u", subquery->ops_tsubquery, 
				winix);
		MEmove(STlen(rowname), rowname, ' ',
			sizeof(rrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
		    (char *)&rrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname);
		rrsdmp->pst_left = newstuff;
		newstuff = rrsdmp;
	    }

	    /* Save PST_WINDOW before overlaying. */
	    newwinp[i] = (*funcpp)->pst_right;

	    /* diff() and rediff() have been generated - now generate
	    ** the parse tree fragment to compute the requested function. */
	    if (opno == ADI_RANK_OP && ravarp != NULL)
	    {
		/* rank() function is requested, but it has already been 
		** computed for percent_rank(). */
		opp = ravarp;
	    }
	    else if (opno == ADI_ROWNUM_OP && rovarp != NULL)
	    {
		/* row_number() function is requested, but it has already been 
		** computed for ntile(). */
		opp = rovarp;
	    }
	    else if (opno == ADI_PRANK_OP)
	    {
		/* First see if rank() has already been built. */
		if (ravarp == NULL)
		{
		    opp = opv_opnode(global, PST_BOP, ADI_RANK_OP, OPL_NOOUTER);
		    opp->pst_sym.pst_value.pst_s_op.pst_flags = PST_WINDOW_OP;
		    STRUCT_ASSIGN_MACRO(dbdvint8, opp->pst_sym.pst_dataval);
		    opp->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid =
				ADI_NILCOERCE;
		    opp->pst_left = dvarp;
		    if (numpart > 0)
			opp->pst_right = redvarp;
		    else opp->pst_sym.pst_type = PST_UOP;

		    rarsdmp = opv_resdom(global, opp, &dbdvint8);
		    rarsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = 0;
		    rarsdmp->pst_left = newstuff;
		    newstuff = rarsdmp;
		    ravarp = opv_varnode(global, &dbdvint8, OPV_NOVAR, &attrid);
		    MEcopy((char *)&rarsdmp->pst_sym.pst_value.pst_s_rsdm.
				pst_rsname, sizeof(DB_ATT_NAME),
			(char *)&ravarp->pst_sym.pst_value.pst_s_var.pst_atname);
		}

		/* Build case expression to compute percent_rank(). */
		opp = casep = opv_casenode(global, PST_SEARCHED_CASE);
		casep->pst_left = whlistp = opv_whlistop(global);
		whlistp->pst_right->pst_left = opa_makeop(global, PST_BOP,
		    global->ops_cb->ops_server->opg_le, cvarp, 
		    opv_intconst(global, 1));
		whlistp->pst_right->pst_right = opv_dblconst(global, 0);
		whlistp = whlistp->pst_left = opv_whlistop(global);
		whlistp->pst_right->pst_right = opa_makeop(global, PST_BOP,
		    global->ops_cb->ops_server->opg_div,
			opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
				opg_f8, opa_makeop(global, PST_BOP, 
				    global->ops_cb->ops_server->opg_sub, 
				    ravarp, opv_intconst(global, 1)), NULL),
			opa_makeop(global, PST_UOP, global->ops_cb->ops_server->
				opg_f8, opa_makeop(global, PST_BOP, 
				    global->ops_cb->ops_server->opg_sub, 
				    cvarp, opv_intconst(global, 1)), NULL));
	    }
	    else if (opno == ADI_NTILE_OP)
	    {
		/* First see if row_number() has already been built. If not,
		** do it now. */
		if (rovarp == NULL)
		{
		    opp = opv_opnode(global, PST_UOP, ADI_ROWNUM_OP,
				OPL_NOOUTER);
		    opp->pst_sym.pst_value.pst_s_op.pst_flags = PST_WINDOW_OP;
		    STRUCT_ASSIGN_MACRO(dbdvint8, opp->pst_sym.pst_dataval);
		    opp->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid =
				ADI_NILCOERCE;
		    if (numpart > 0)
			opp->pst_left = dvarp;

		    rorsdmp = opv_resdom(global, opp, &dbdvint8);
		    rorsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = 0;
		    rorsdmp->pst_left = newstuff;
		    newstuff = rorsdmp;
		    rovarp = opv_varnode(global, &dbdvint8, OPV_NOVAR, &attrid);
		    MEcopy((char *)&rorsdmp->pst_sym.pst_value.pst_s_rsdm.
				pst_rsname, sizeof(DB_ATT_NAME),
			(char *)&rovarp->pst_sym.pst_value.pst_s_var.pst_atname);
		}

		/* Now create PST_MOP to define X100 sqlntile() func. */
		opp = opv_opnode(global, PST_MOP, ADI_NTILE_OP, OPL_NOOUTER);
		opp->pst_sym.pst_value.pst_s_op.pst_flags = PST_WINDOW_OP;
		STRUCT_ASSIGN_MACRO(dbdvint8, opp->pst_sym.pst_dataval);
		opp->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid = ADI_NILCOERCE;
		opp->pst_left = rovarp;
		opp->pst_right = opv_operand(global, cvarp);
		opp->pst_right->pst_right = opv_operand(global,
				(*funcpp)->pst_left);
	    }
	    else
	    {
		opp = opv_opnode(global, PST_BOP, opno, OPL_NOOUTER);
		opp->pst_sym.pst_value.pst_s_op.pst_flags = PST_WINDOW_OP;
		STRUCT_ASSIGN_MACRO(dbdvint8, opp->pst_sym.pst_dataval);
		opp->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid = ADI_NILCOERCE;

		/* rank/dense_rank take diff & rediff as parms, row_number
		** just takes diff (and is therefore a PST_UOP). */
		opp->pst_left = dvarp;
		if (numpart > 0 && 
			(opno == ADI_RANK_OP || opno == ADI_DRANK_OP))
		    opp->pst_right = redvarp;
		else opp->pst_sym.pst_type = PST_UOP;

		if (opno == ADI_ROWNUM_OP)
		{
		    if (numpart <= 0)
			opp->pst_left = NULL; /* row_number() & no part */
	
		    if (cfuncs)
		    {
			/* In case ntile(<n>) may yet be requested, create
			** a PST_VAR to represent row_number(). */
			rovarp = opv_varnode(global, &dbdvint8, OPV_NOVAR,
				&attrid);
			/* And a RESDOM to assure rank() is pre-computed. */
			rorsdmp = opv_resdom(global, opp, &dbdvint8);
			MEcopy((char *)&rorsdmp->pst_sym.pst_value.pst_s_rsdm.
				pst_rsname, sizeof(DB_ATT_NAME),
			    (char *)&rovarp->pst_sym.pst_value.pst_s_var.
				pst_atname);
			rorsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = 0;
			rorsdmp->pst_left = newstuff;
			newstuff = rorsdmp;
			opp = rovarp;
		    }
		}

		if (cfuncs && opno == ADI_RANK_OP)
		{
		    /* In case percent_rank() may yet be requested, create
		    ** a PST_VAR to represent rank(). */
		    ravarp = opv_varnode(global, &dbdvint8, OPV_NOVAR, &attrid);
		    /* And a RESDOM to assure rank() is pre-computed. */
		    rarsdmp = opv_resdom(global, opp, &dbdvint8);
		    MEcopy((char *)&rarsdmp->pst_sym.pst_value.pst_s_rsdm.
				pst_rsname, sizeof(DB_ATT_NAME),
			(char *)&ravarp->pst_sym.pst_value.pst_s_var.pst_atname);
		    rarsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = 0;
		    rarsdmp->pst_left = newstuff;
		    newstuff = rarsdmp;
		    opp = ravarp;
		}
	    }
	    

	    /* If there are multiple window defs and this function isn't
	    ** in the 1st, we need to append it to the diff/rediff defs to
	    ** assure the evaluation uses the right order. Add RESDOM to
	    ** attach it to newstuff & replace by VAR on main RESDOM chain. */
	    if (winix == 0)
		(*funcpp) = opp;		/* replace old with new */
	    else
	    {
		nrsdmp = opv_resdom(global, opp, &dbdvint8);
		nrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = 0;
		nrsdmp->pst_left = newstuff;
		newstuff = nrsdmp;
		fvarp = opv_varnode(global, &nrsdmp->pst_sym.pst_dataval,
			OPV_NOVAR, &attrid);
		(*funcpp) = fvarp;

		/* Copy result column name. */
		MEcopy((char *)&wrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		    sizeof(DB_ATT_NAME),
		    (char *)&fvarp->pst_sym.pst_value.pst_s_var.pst_atname);
		MEcopy((char *)&wrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		    sizeof(DB_ATT_NAME),
		    (char *)&nrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname);
	    }
	}
    }

    /* Wrap it up by appending a PST_TREE and returning the fragment. */
    if (prsdmp != NULL)
    {
	prsdmp->pst_left = opv_qlend(global);
	prsdmp->pst_left->pst_sym.pst_type = PST_TREE;
    }
    subquery->ops_winfuncs[winix] = newwinp;
    return(newstuff);
}

/*{
** Name: OPC_X100_WINSORT_EXPR	- build RESDOM chain to add window order by
**	expressions to projected stuff
**
** Description:
**	Construct RESDOM list from expressions and replace them by 
**	corresponding VAR nodes.
**
** Inputs:
**      subquery		ptr to subquery
**
** Outputs:
**	Returns:
**	    ptr to new RESDOM chain
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	31-may-2009 (dougi) m1961
**	    Written for window functions support.
*/
static PST_QNODE *
opc_x100_winsort_expr(
	OPS_SUBQUERY	*subquery)

{
    PST_QNODE	*frsdmp, *nrsdmp, *rsdmp, *expr, *varp;
    OPS_STATE	*global = subquery->ops_global;
    i4		i;
    DB_ATT_ID	attrid;


    /* Loop over all windows looking for expressions in sort list. Allocate
    ** RESDOM to address expression & replace expression in original list
    ** by new PST_VAR node. */
    attrid.db_att_id = OPZ_NOATTR;
    for (i = 0, frsdmp = NULL; i < subquery->ops_window_cnt; i++)
     for (rsdmp = (*subquery->ops_winfuncs[i])->pst_right->pst_right->pst_left;
	    rsdmp != NULL && rsdmp->pst_sym.pst_type == PST_RESDOM;
	    rsdmp = rsdmp->pst_left)
      if ((expr = rsdmp->pst_right)->pst_sym.pst_type != PST_VAR)
      {
	/* Allocate new RESDOM & VAR, copy stuff around. */
	nrsdmp = opv_resdom(global, expr, &expr->pst_sym.pst_dataval);
	if (frsdmp != NULL)
	    nrsdmp->pst_left = frsdmp;
	frsdmp = nrsdmp;

	varp = opv_varnode(global, &expr->pst_sym.pst_dataval, OPV_NOVAR,
			&attrid);
	rsdmp->pst_right = varp;
	MEcopy((char *)&nrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		DB_MAXNAME, (char *)&varp->pst_sym.pst_value.pst_s_var.
			pst_atname);
	MEcopy((char *)&nrsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		DB_MAXNAME, (char *)&rsdmp->pst_sym.pst_value. pst_s_rsdm.
			pst_rsname);
      }

    return(frsdmp);
}

/*{
** Name: OPC_X100_WINSORT	- emit sort list for a window (for window funcs)
**
** Description:
**	Emit list of partitioning/ordering columns for requested window.
**
** Inputs:
**      subquery		ptr to subquery
**      x100cb			ptr to X100 text state variable
**	winix			index of window to process
**	partonly		TRUE - only emit partition cols
**				FALSE - emit partition & sort cols
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	27-may-2009 (dougi) m1961
**	    Written for window functions support.
*/
static void
opc_x100_winsort(
	OPS_SUBQUERY	*subquery,
	X100_STATE	*x100cb,
	i4		winix,
	bool		partonly)
{
    PST_QNODE	**winpp = subquery->ops_winfuncs[winix];
    PST_QNODE	*rsdmp, *sortp;
    OPO_CO	*cop = subquery->ops_bestco;
    char	*fill = "";
    i4		i;

    opc_x100_printf(x100cb, ", [");	/* open sort list */

    /* Partitioning column sublist. */
    for (rsdmp = (winpp[0])->pst_left, i = 0;
	rsdmp != NULL && rsdmp->pst_sym.pst_type == PST_RESDOM &&
	    i < (winpp[0])->pst_sym.pst_value.pst_s_win.pst_pcount;
	rsdmp = rsdmp->pst_left, i++)
    {
	opc_x100_printf(x100cb, fill);
	opc_x100_prexpr(x100cb, cop, rsdmp->pst_right, 
		(DB_DATA_VALUE *) NULL, ADI_NILCOERCE);
	fill = ", ";
    }
    if (partonly)
	return;

    /* ORDER BY sublist - checks sort order, too. */
    for (sortp = (winpp[0])->pst_right;
	rsdmp != NULL && rsdmp->pst_sym.pst_type == PST_RESDOM;
	rsdmp = rsdmp->pst_left, sortp = sortp->pst_left)
    {
	opc_x100_printf(x100cb, fill);
	opc_x100_prexpr(x100cb, cop, rsdmp->pst_right, 
		(DB_DATA_VALUE *) NULL, ADI_NILCOERCE);
	if (sortp->pst_sym.pst_value.pst_s_sort.pst_srasc == FALSE)
	    opc_x100_printf(x100cb, " DESC");
	fill = ", ";
    }
    fill = " ";
    opc_x100_printf(x100cb, "])");	/* close sort list */
}

/*{
** Name: OPC_X100_CLAGGCK	- check for GROUP BY compatible with node 
**	clustering
**
** Description:
**	If subquery has clusterings (ops_bestco), check if GROUP BY is
**	covered by one (permits use of OrdAggr()).
**
** Inputs:
**      subquery			ptr to subquery to check
**
** Outputs:
**	Returns:
**		TRUE	- if GROUP BY is covered by valid clustering
**		FALSE	- otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-may-2009 (dougi)
**	    Written for X100 support.
**	22-mar-2010 (dougi)
**	    Extend check to include EQCs replacing/replaced by clustering EQCs.
*/
static bool
opc_x100_claggck(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	*listp)

{
    OPO_CO	*cop;
    PST_QNODE	*rsdmp;
    OPE_BMEQCLS eqcmap, *eqcmapp;
    OPE_EQCLIST	*eqclp;
    OPE_IEQCLS	eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPZ_ATTS	*attrp;

    i4		i;
    bool	match;

    cop = subquery->ops_bestco;
    if (cop == (OPO_CO *) NULL || cop->opo_clcount <= 0 ||
	listp == (PST_QNODE *) NULL)
	return(FALSE);

    /* Set "derived" group by columns. */
    opc_x100_derck(subquery, listp, FALSE);

    /* Loop over cluster orderings, checking whether one covers the 
    ** GROUP BY clause (only one covering needed). */
    for (i = 0; i < cop->opo_clcount; i++)
    {
	MEfill(sizeof(eqcmap), 0, (char *)&eqcmap);
	eqcno = cop->opo_clusteqcs[i];
	if (eqcno < maxeqcls)
	{
	    BTset(eqcno, (char *)&eqcmap);
	    eqcmapp = &eqcmap;
	}
	else eqcmapp = subquery->ops_msort.opo_base->opo_stable[
			eqcno - maxeqcls]->opo_bmeqcls;

	/* Now loop over GROUP BY list to see if it's covered. */
	for (rsdmp =listp, match = TRUE;
		match && rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM; 
		rsdmp = rsdmp->pst_left)
	{
	    if (rsdmp->pst_right->pst_sym.pst_type != PST_VAR)
		return(FALSE);			/* only works for VARs */
	    if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
				PST_RS_DERIVED)
		continue;			/* skip the DERIVED ones */
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[rsdmp->
		pst_right->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	    if (!BTtest(attrp->opz_equcls, (char *)eqcmapp))
	    {
		/* Try harder - maybe this is a replaced join, so check if 
		** EQC was replaced by or replaces a clustering EQC. */
		eqclp = subquery->ops_eclass.ope_base->ope_eqclist[attrp->
						opz_equcls];
		if ((eqclp->ope_mask & (OPE_JOINIX | OPE_REPJOINIX)) && 
		    (eqclp->ope_joinrep &&
		    BTsubset((char *)eqclp->ope_joinrep, (char *)eqcmapp,
					maxeqcls)) ||
		    (eqclp->ope_repjoin &&
		    BTsubset((char *)eqclp->ope_repjoin, (char *)eqcmapp,
					maxeqcls)))
		    continue;

		match = FALSE;
	    }
	}

	if (match)
	    return(TRUE);			/* ta da!! */
    }

    return(FALSE);				/* if we get here, no match */
}

/*{
** Name: OPC_X100_DERCK	- check for possible DERIVED columns in GROUP BY
**
** Description:
**	Look through FDs of root CO node that might permit some of the
**	GROUP BY columns to be tagged DERIVED (saving processing time).
**
** Inputs:
**      subquery		ptr to subquery to check
**	bylistp			ptr to chain of GROUP BY columns
**	drop_const		TRUE - make constants DERIVED
**
** Outputs:
**	Returns:
**		nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-may-2009 (dougi)
**	    Written for X100 support.
**	21-apr-2010 (dougi)
**	    Add logic to set DERIVED attribute to constant columns.
**	14-june-2010 (dougi)
**	    Fine tune checks for covered independent variables.
**	30-may-2011 (dougi) m1971
**	    Look for constant functions (flagged RESDOMs).
*/
FUNC_EXTERN VOID
opc_x100_derck(
	OPS_SUBQUERY	*subquery,
	PST_QNODE	*bylistp,
	bool		drop_const)

{
    PST_QNODE	*rsdmp, *varp;
    OPO_CO	*cop, *bmcop;
    OPE_BMEQCLS eqcmapi, eqcmapd;
    OPE_EQCLIST	*eqclp;
    OPE_IEQCLS	eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPZ_ATTS	*attrp;
    OPZ_IATTS	attno;

    i4		i;
    bool	match, allderived;

    cop = subquery->ops_bestco;
    if (cop == (OPO_CO *) NULL || bylistp == (PST_QNODE *) NULL)
	return;					/* nothing to check */

    if (cop->opo_sjpr == DB_RSORT)
	bmcop = cop->opo_outer;
    else bmcop = cop;

    /* Loop over FDs. If the independent columns of an FD are covered
    ** by the RESDOM list, any RESDOM in the dependent list can be marked
    ** as DERIVED. */
    for (i = 0; i < cop->opo_FDcount; i++)
    {
	if ((eqcno = cop->opo_FDieqcs[i]) < maxeqcls)
	{
	    MEfill(sizeof(eqcmapi), 0, (char *)&eqcmapi);
	    BTset(eqcno, (char *)&eqcmapi);
	}
	else if ((eqcno - maxeqcls) < subquery->ops_msort.opo_sv &&
		subquery->ops_msort.opo_base->opo_stable[eqcno - maxeqcls]->
			opo_bmeqcls != (OPE_BMEQCLS *) NULL)
	    MEcopy((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, 
		sizeof(eqcmapi), (char *)&eqcmapi);
	else continue;

	/* Loop over independent EQCs. */
	for (eqcno = -1; (eqcno = BTnext(eqcno, (char *)&eqcmapi, 
				maxeqcls)) >= 0; )
	{
	    eqclp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];

	    /* Loop over joinop attrs in EQC map. */
	    for (attno = -1, match = FALSE; !match && (attno = BTnext(attno,
		(char *)&eqclp->ope_attrmap, 
			subquery->ops_attrs.opz_av)) >= 0; )
	    {
		attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];

		/* Now loop over RESDOMs looking for a match. */
		for (rsdmp = bylistp; !match && rsdmp && 
		    rsdmp->pst_sym.pst_type == PST_RESDOM; 
		    rsdmp = rsdmp->pst_left)
	 	 if ((varp = rsdmp->pst_right) && 
		    varp->pst_sym.pst_type == PST_VAR &&
		    varp->pst_sym.pst_value.pst_s_var.pst_vno ==
							attrp->opz_varnm &&
		    varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == 
							attno)
		    match = TRUE;
	    }
	    if (!match)
		break;			/* must match all */
	}

	if (!match)
	    continue;			/* no match with independent EQCs */

	/* If we get here, the current FD is covered by our GROUP BY. Now
	** loop over the dependent EQCs - any RESDOM that matches is 
	** flagged DERIVED. */
	if ((eqcno = cop->opo_FDdeqcs[i]) < maxeqcls)
	{
	    MEfill(sizeof(eqcmapd), 0, (char *)&eqcmapd);
	    BTset(eqcno, (char *)&eqcmapd);
	}
	else if ((eqcno - maxeqcls) < subquery->ops_msort.opo_sv &&
		subquery->ops_msort.opo_base->opo_stable[eqcno - maxeqcls]->
			opo_bmeqcls != (OPE_BMEQCLS *) NULL)
	    MEcopy((char *)subquery->ops_msort.opo_base->
		opo_stable[eqcno-maxeqcls]->opo_bmeqcls, 
		sizeof(eqcmapd), (char *)&eqcmapd);
	else continue;

	/* Loop over dependent EQCs. */
	for (eqcno = -1; (eqcno = BTnext(eqcno, 
			(char *)&eqcmapd, maxeqcls)) >= 0; )
	{
	    if (BTtest(eqcno, (char *)&eqcmapi))
		continue;		/* skip indep cols in dep list */

	    eqclp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];

	    /* Loop over joinop attrs in EQC map. */
	    for (attno = -1, match = FALSE; !match && (attno = BTnext(attno,
		(char *)&eqclp->ope_attrmap, 
			subquery->ops_attrs.opz_av)) >= 0; )
	    {
		attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];

		/* Now loop over RESDOMs looking for a match. */
		for (rsdmp = bylistp; !match && rsdmp && 
		    rsdmp->pst_sym.pst_type == PST_RESDOM; 
		    rsdmp = rsdmp->pst_left)
		 if ((varp = rsdmp->pst_right) && 
		    varp->pst_sym.pst_type == PST_VAR &&
		    varp->pst_sym.pst_value.pst_s_var.pst_vno ==
							attrp->opz_varnm &&
		    varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id == 
							attno)
		 {
		    /* Break loop and flag as DERIVED. */ 
		    match = TRUE;
		    rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |= 
							PST_RS_DERIVED;
		 }
	    }
	}
    }

    /* After all that, check again for grouping columns that are constant
    ** (restricted to a single value by a predicate). Also check for introduced
    ** column - it's probably a sort column not in the select list. These
    ** should all be flagged as DERIVED. */

    for (rsdmp = bylistp, allderived = TRUE; rsdmp && 
	rsdmp->pst_sym.pst_type == PST_RESDOM; rsdmp = rsdmp->pst_left)
     if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_DERIVED)
	continue;			/* already identified as DERIVED */
     else if ((varp = rsdmp->pst_right)->pst_sym.pst_type == PST_VAR &&
	subquery->ops_bfs.opb_bfeqc != NULL &&
	(attno = varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id) >= 0 &&
	(eqcno = subquery->ops_attrs.opz_base->opz_attnums[attno]->
							opz_equcls) >= 0 &&
	BTtest((i4)eqcno, (char *)subquery->ops_bfs.opb_bfeqc))
	rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |= PST_RS_DERIVED;
					/* constant */
     else if (drop_const && (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
				PST_RS_CONST))
	rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |= PST_RS_DERIVED;
					/* introduced sort column */
     else allderived = FALSE;

    /* Must have at least one non-DERIVED column. */
    if (allderived)
	bylistp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &= ~PST_RS_DERIVED;
}

/*{
** Name: OPC_X100_ORDDECK	- check for a duplicates elimination 
**	aggregation that can be done with OrdAggr()
**
** Description:
**	If non-DERIVED columns in a dups elimination are covered by a 
**	clustering, return TRUE.
**
** Inputs:
**      subquery			ptr to subquery to check
**	cop				ptr to CO node being checked
**	dupselim			ptr to bool indicating whether 
**					dupselim is ok
**
** Outputs:
**	Returns:
**		TRUE	- if non-DERIVED columns in aggregation are covered
**			by a clustering
**		FALSE	- otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-june-2009 (dougi)
**	    Written for X100 support.
**	21-dec-2009 (dougi)
**	    Check for dupselim on ORIG node TID. No need to do it then.
**	19-may-2010 (dougi)
**	    Extend TID check to semi/anti joins with single table.
**	19-may-2011 (dougi) m1953
**	    Skip dupselim if this is a SAGG and only input is HFAGG. There
**	    can't be any dups.
*/
static bool
opc_x100_orddeck(
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop,
	bool		*dupselim)

{
    OPE_BMEQCLS		clsmap, eqcmap, FDmap;
    OPE_IEQCLS		eqcno, maxeqcls = subquery->ops_eclass.ope_ev;
    OPE_EQCLIST		*eqclp;
    OPV_IVARS		varno;
    OPZ_IATTS		attno;
    OPZ_ATTS		*attrp;
    OPO_CO		*jcop, *tcop;
    i4		i, j;


    /* Check for SAGG whose only input is a HFAGG. In that case, the 
    ** input is already dups eliminated. */
    if ((subquery->ops_sqtype == OPS_SAGG && subquery->ops_vars.opv_rv == 1 &&
	subquery->ops_vars.opv_base->opv_rt[0]->opv_grv->opv_gquery != NULL &&
	subquery->ops_vars.opv_base->opv_rt[0]->opv_grv->opv_gquery->ops_sqtype
				== OPS_HFAGG) ||
	cop->opo_sjpr == DB_RSORT && cop->opo_ucount > 0)
    {
	*dupselim = FALSE;
	return(FALSE);
    }

    /* If this is an ORIG node or a sort atop an ORIG node, look for
    ** the TID. If there, dups elimination is superfluous. */
    if (cop->opo_sjpr == DB_ORIG)
	varno = cop->opo_union.opo_orig;
    else if (cop->opo_sjpr == DB_RSORT && cop->opo_outer->opo_sjpr == DB_ORIG)
	varno = cop->opo_outer->opo_union.opo_orig;
    else if (cop->opo_sjpr == DB_RSORT && cop->opo_outer->opo_sjpr == DB_PR &&
		cop->opo_outer->opo_outer->opo_sjpr == DB_ORIG)
	varno = cop->opo_outer->opo_outer->opo_union.opo_orig;
    else varno = -1;

    /* Moreover, if this is an SJ or a sort atop an SJ and the SJ is a semi/
    ** anti join in which the preserved side has a single table, we can also
    ** look for a TID. All this might be easier if we maintain unique key 
    ** properties in a CO-node, similar to FDs. Then the presence of a unique
    ** key in the node's EQC map would preclude the need to remove dups. */
    if (((tcop = cop)->opo_sjpr == DB_SJ || cop->opo_sjpr == DB_RSORT &&
	(tcop = cop->opo_outer)->opo_sjpr == DB_SJ) &&
	(tcop->opo_variant.opo_local->opo_mask & (OPO_ANTIJOIN | OPO_SEMIJOIN)))
    {
	/* Determine preserved side and check for single table. */
	if ((jcop = tcop)->opo_variant.opo_local->opo_mask & OPO_REVJOIN)
	    tcop = tcop->opo_inner;
	else tcop = tcop->opo_outer;

	MEcopy((char *)&jcop->opo_maps->opo_eqcmap, sizeof(eqcmap),
					(char *)&eqcmap);
	if (jcop->opo_sjeqc != OPE_NOEQCLS)
	{
	    /* Add join EQC(s) to test for coverage of join input. */
	    if (jcop->opo_sjeqc < maxeqcls)
		BTset((i4)jcop->opo_sjeqc, (char *)&eqcmap);
	    else BTor((i4)maxeqcls, (char *)subquery->ops_msort.opo_base->
		opo_stable[jcop->opo_sjeqc-maxeqcls]->opo_bmeqcls, 
		(char *)&eqcmap);
	}
	
	if (tcop->opo_dupselim && BTsubset((char *)&tcop->opo_maps->opo_eqcmap, 
		(char *)&jcop->opo_maps->opo_eqcmap, (i4)maxeqcls))
	{
	    /* Dups already removed. */
	    *dupselim = FALSE;
	    return(FALSE);
	}

	if (BTcount((char *)tcop->opo_variant.opo_local->opo_bmvars,
		subquery->ops_vars.opv_rv) == 1)
	    varno = BTnext(-1, (char *)tcop->opo_variant.opo_local->opo_bmvars,
		subquery->ops_vars.opv_rv);
	else varno = -1;
    }

    if (varno >= 0)
     for (eqcno = -1; (eqcno = BTnext((i4)eqcno, (char *)&cop->opo_maps->
		opo_eqcmap, subquery->ops_eclass.ope_ev)) >= 0; )
     {
	eqclp = subquery->ops_eclass.ope_base->ope_eqclist[eqcno];

	for (attno = -1; (attno = BTnext(attno, (char *)&eqclp->ope_attrmap, 
			subquery->ops_attrs.opz_av)) >= 0; )
	{
	    attrp = subquery->ops_attrs.opz_base->opz_attnums[attno];
	    if (attrp->opz_attnm.db_att_id == 0 && attrp->opz_varnm == varno)
	    {
		*dupselim = FALSE;
		return(FALSE);
	    }
	}
     }
    *dupselim = TRUE;

    /* Loop over clusterings. Check each against the attrs in aggregation.
    ** Then for each, look at each FD and remove any DERIVED EQCs. Then
    ** check again (if not already successful). */
    for (i = 0; i < cop->opo_clcount; i++)
    {
	eqcno = cop->opo_clusteqcs[i];
	if (eqcno < maxeqcls)
	{
	    MEfill(sizeof(clsmap), 0, (char *)&clsmap);
	    BTset(eqcno, (char *)&clsmap);
	}
	else MEcopy((char *)subquery->ops_msort.opo_base->opo_stable[eqcno -
		maxeqcls]->opo_bmeqcls, sizeof(clsmap), (char *)&clsmap);

	if (BTsubset((char *)&cop->opo_maps->opo_eqcmap, (char *)&clsmap,
			sizeof(clsmap)))
	    return(TRUE);			/* no need for FDs */

	/* Now loop over FDs. */
	for (j = 0; j < cop->opo_FDcount; j++)
	{
	    eqcno = cop->opo_FDieqcs[j];
	    if (eqcno < maxeqcls)
	    {
		MEfill(sizeof(FDmap), 0, (char *)&FDmap);
		BTset(eqcno, (char *)&FDmap);
	    }
	    else MEcopy((char *)subquery->ops_msort.opo_base->opo_stable[eqcno -
		maxeqcls]->opo_bmeqcls, sizeof(FDmap), (char *)&FDmap);
	    if (!BTsubset((char *)&FDmap, (char *)&cop->opo_maps->opo_eqcmap,
			sizeof(FDmap)))
		continue;			/* indep not in CO result */

	    eqcno = cop->opo_FDdeqcs[j];
	    if (eqcno < maxeqcls)
	    {
		MEfill(sizeof(FDmap), 0, (char *)&FDmap);
		BTset(eqcno, (char *)&FDmap);
	    }
	    else MEcopy((char *)subquery->ops_msort.opo_base->opo_stable[eqcno -
		maxeqcls]->opo_bmeqcls, sizeof(FDmap), (char *)&FDmap);

	    MEcopy((char *)&cop->opo_maps->opo_eqcmap, sizeof(eqcmap),
			(char *)&eqcmap);
	    BTnot(maxeqcls, (char *)&FDmap);
	    BTand(BITS_IN(FDmap), (char *)&FDmap, (char *)&eqcmap);
	    if (BTsubset((char *)&eqcmap, (char *)&clsmap, sizeof(clsmap)))
		return(TRUE);			/* non-DERIVEDs covered */
	}
    }

    return(FALSE);				/* failed if we get to here */
}

/*{
** Name: OPC_X100_AGGDISTCK	- check for aggregate distincts in projection
**
** Description:
**      Iterate over aggregate RESDOM list looking for aggregate distinct
**	operations - and validate those that are found.
**
** Inputs:
**      nodep                           ptr to parse tree fragment to search
**
** Outputs:
**	distvarpp			ptr to ptr to RESDOM anchoring 
**					aggregate distinct PST_VAR (if there)
**	Returns:
**		TRUE	- if parse tree has aggregate distinct operation
**		FALSE	- otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-feb-2009 (dougi)
**	    Written for X100 support.
**	26-Feb-2011 (kiria01) m1619
**	    Reworked to take explicit note of .pst_distinct and
**	    hence allow ANY, MIN and MAX not to upset DISTINCT
**	    choice.
*/
static bool
opc_x100_aggdistck(
	PST_QNODE	*nodep,
	PST_QNODE	**distvarpp)

{

    PST_QNODE	*rsdmp, *distvar = NULL;
    bool	aggdist = FALSE, aggnondist = FALSE;

    /* Loop over RESDOMs looking at the PST_AOPs of the aggregate
    ** operations. Any aggregate distinct must be on a PST_VAR and if
    ** there is more than one aggregate distinct, it must be on the same
    ** PST_VAR. And if there are aggregate distincts, there can be no other
    ** (non distinct) aggregates. Failure of any of those conditions results
    ** in an error (for now - Marcin suggests using "reuse" to enable more
    ** complete functionality).
    ** FIXME: Two concerns with the above: 1) Rather than a VAR, it could
    ** be a general expression. 2) The AOP might might be embedded in an
    ** expression. */

    for (rsdmp = nodep; rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM; 
				rsdmp = rsdmp->pst_left)
    {
	PST_QNODE *aop = rsdmp->pst_right, *var;
	if (aop->pst_sym.pst_type == PST_AOP)
	{
	    switch (aop->pst_sym.pst_value.pst_s_op.pst_distinct)
	    {
	    case PST_DISTINCT:
		aggdist = TRUE;
		break;
	    case PST_NDISTINCT:
		aggnondist = TRUE;
		break;
	    default:
		continue;
	    }
	    if (aggdist)
	    {
		/* Check for errors. */
		PST_QNODE *var = aop->pst_left;
		if (aggnondist ||
		    var->pst_sym.pst_type != PST_VAR ||
		    (distvar && (
			distvar->pst_sym.pst_value.pst_s_var.pst_vno !=
				var->pst_sym.pst_value.pst_s_var.pst_vno ||
			distvar->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id !=
				var->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)))
		{
		    /* One of the conditions was violated - issue message. */
		    opx_error(E_OP08C1_BADAGGDIST);
		}
		distvar = var;		/* save 1st VAR for later checks */
	    }
	}
    }

    *distvarpp = distvar;			/* return PST_VAR ptr or NULL */
    return(aggdist);
}

/*{
** Name: OPC_X100_BFSAGGCHECK	search Boolean factors for current CO node
**	for SAGG references
**
** Description:	For each Boolean factor in the bit map of this CO node, call
**	opc_x100_bfsaggfix to search it for SAGG refs and do what's necessary.
**
** Inputs:
**      x100cb				ptr to X100 text state variable
**	subquery			ptr to current subquery
**	cop				ptr to current CO node
**      saggsubq			ptr to ptr to SAGG subquery chain
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	8-dec-2008 (dougi)
**	    Written for X100 support.
**	11-dec-2009 (dougi)
**	    Fix to support noCO MAIN subqueries.
*/
static VOID
opc_x100_bfsaggcheck(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	OPO_CO		*cop,
	OPS_SUBQUERY	**saggsubq)

{

    i4		i;

    /* Loop over bitmap, calling opc_x100_bfsaggfix() with each BF. */
    *saggsubq = (OPS_SUBQUERY *) NULL;		/* init to NULL */
    if (cop != (OPO_CO *) NULL)
     for (i = -1; (i = BTnext(i, (char *)cop->opo_variant.opo_local->opo_bmbf,
			subquery->ops_bfs.opb_bv)) != -1; )
	opc_x100_bfsaggfix(x100cb, &subquery->ops_bfs.opb_base->
				opb_boolfact[i]->opb_qnode, saggsubq, FALSE);
    else if (subquery->ops_sqtype == OPS_MAIN)
    {
	x100cb->noCOsaggs = 0;
	for (i = 0; i < subquery->ops_bfs.opb_bv; i++)
	    opc_x100_bfsaggfix(x100cb, &subquery->ops_bfs.opb_base->
				opb_boolfact[i]->opb_qnode, saggsubq, TRUE);

	/* And if there's a bfconstants, do it, too. */
	if (subquery->ops_bfs.opb_bfconstants != (PST_QNODE *) NULL)
	    opc_x100_bfsaggfix(x100cb, &subquery->ops_bfs.opb_bfconstants,
						saggsubq, TRUE);
    }

}

/*{
** Name: OPC_X100_BFSAGGFIX	Search and log SAGG references in Boolean expr
**
** Description:	Locate PST_CONST nodes that refer to simple aggregate results
**	and replace by PST_VAR nodes with name set to result column name.
**
** Inputs:
**      x100cb				ptr to X100 text state variable
**	nodep				ptr to ptr to projection parse tree 
**					node being processed
**      saggsubq			ptr to ptr to SAGG subquery list
**	noCO				TRUE - main subquery w. no CO-tree
**					FALSE - otherwise
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-oct-2008 (dougi)
**	    Written for X100 support.
**	20-june-2011 (dougi) m2123
**	    Fix to handle multiple SAGGs in noCO MAIN.
*/
static VOID
opc_x100_bfsaggfix(
	X100_STATE	*x100cb,
	PST_QNODE	**nodep,
	OPS_SUBQUERY	**saggsubq,
	bool		noCO)

{
    OPS_STATE	*global = x100cb->global;
    PST_QNODE	*rsdmp;
    DB_ATT_ID	attrid;
    OPS_SUBQUERY *subqp, *tsq;
    DB_ERROR	error;

    /* Look through parse tree for PST_CONST nodes representing SAGG
    ** results. */
    switch((*nodep)->pst_sym.pst_type) {
      case PST_CONST:
	/* SAGG result looks like a constant repeat-parameter, but it has
	** an "invalid" param number.
	*/
	if ((*nodep)->pst_sym.pst_value.pst_s_cnst.pst_tparmtype != 
								PST_RQPARAMNO
		|| (*nodep)->pst_sym.pst_value.pst_s_cnst.pst_parm_no <= global->ops_qheader->pst_numparm)
	    break;

	/* Loop over subqueries looking for SAGG that matches this CONST. 
	** For each SAGG, loop over its parse tree looking for matching
	** RESDOM. Then replace original CONST node with fake VAR with same 
	** name as RESDOM. */
	for (subqp = global->ops_subquery; subqp; subqp = subqp->ops_next)
	 if (subqp->ops_sqtype == OPS_SAGG)
	  for (rsdmp = subqp->ops_root->pst_left;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
	   if (rsdmp->pst_right && 
			rsdmp->pst_right->pst_sym.pst_type == PST_AOP &&
		rsdmp->pst_right->pst_sym.pst_value.pst_s_op.pst_opparm ==
			(*nodep)->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
	   {
	    /* Replace CONST with VAR node and add SAGG subquery to 
	    ** current chain (for CartProd(, , 1) generation to come). */
	    attrid.db_att_id = OPZ_NOATTR;
	    (*nodep) = opv_varnode(global, &(*nodep)->pst_sym.pst_dataval, 
							OPV_NOVAR, &attrid);
	    MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
		sizeof(DB_ATT_NAME), (char *)&(*nodep)->pst_sym.pst_value.
		pst_s_var.pst_atname);		/* copy RESDOM name to VAR */

	    /* It is conceivable that 2 aggs are computed by the same SAGG
	    ** subquery (as in TPC H Q22). So before we add this one to the 
	    ** chain, see if it is already there. */
	    for (tsq = (*saggsubq); tsq; tsq = tsq->ops_compile.opc_bfsaggnext)
	     if (tsq == subqp)
		return;				/* already here - just leave */

	    if (*saggsubq == (OPS_SUBQUERY *) NULL)
		*saggsubq = subqp;
	    else
	    {
		subqp->ops_compile.opc_bfsaggnext = *saggsubq;
		*saggsubq = subqp;
	    }

	    /* Emit "CartProd" operator. */
	    if (!noCO || x100cb->noCOsaggs >= 1)
		opc_x100_printf(x100cb, "CartProd( ");
	    x100cb->noCOsaggs++;
	    return;
	   }

	/* If we get this far, we didn't find the SAGG subquery and that 
	** is an error. */
	opx_verror(E_DB_ERROR, E_OP08C0_NOSAGG, error.err_code);
	break;

      default:
	if ((*nodep)->pst_left)
	    opc_x100_bfsaggfix(x100cb, &(*nodep)->pst_left, saggsubq, noCO);
	if ((*nodep)->pst_right)
	    opc_x100_bfsaggfix(x100cb, &(*nodep)->pst_right, saggsubq, noCO);
	break;
    }

}

/*{
** Name: OPC_X100_BFSAGGEMIT	Emit syntax to materialize SAGGs
**
** Description:	Complete syntax generation for CartProd(, , 1) operators
**	by materializing SAGG expression.
**
** Inputs:
**      x100cb				ptr to X100 text state variable
**      saggsubq			ptr to SAGG subquery list
**	noCO				TRUE - main subquery w. no CO-tree
**					FALSE - otherwise
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	9-dec-2008 (dougi)
**	    Written for X100 support.
**	20-june-2011 (dougi) m2123
**	    Fix to handle multiple SAGGs in noCO MAIN.
*/
static VOID
opc_x100_bfsaggemit(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*saggsubq,
	bool		noCO)

{
    i4		i;

    /* Simply run the ops_bfsaggnext chain of subqueries, materializing
    ** each and closing its "CartProd(" operator. */

    for (i = 0; saggsubq != (OPS_SUBQUERY *) NULL; 
		saggsubq = saggsubq->ops_compile.opc_bfsaggnext, i++)
    {
	if (i > 0 || !noCO)
	    opc_x100_printf(x100cb, ", ");
	if (!noCO)
	    saggsubq->ops_mask2 |= OPS_X100_BFSAGG;
	opc_x100_subquery(saggsubq, (OPS_SUBQUERY *) NULL, x100cb, 
							(char *)NULL);
	if (i > 0 || !noCO)
	    opc_x100_printf(x100cb, ", 1)");
    }

}

/*{
** Name: OPC_X100_SAGGMAIN	Fix up SAGGs in simple aggregate query
**
** Description:	Search a subquery for references to SAGGs (PST_CONST nodes
**	that look like repeat-query params with a too-large pst_parm_no)
**	and replace with PST_VARs with SAGG RESDOM's names.
**
** Inputs:
**      x100cb				ptr to X100 text state variable
**      subquery			ptr to subquery being searched for
**					SAGG refs
**	nodep				ptr to ptr to MAIN parse tree node
**					being analyzed
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-dec-2008 (dougi)
**	    Written for X100 support.
*/
static VOID
opc_x100_saggmain(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery,
	PST_QNODE	**nodep)

{
    OPS_SUBQUERY	*saggsubq, *worksq;
    PST_QNODE		*rsdmp;
    DB_ATT_ID		attrid;
    bool		found;

    /* Descend left/right side of MAIN sq looking for the telltale PST_CONSTs.
    ** Because this is a CO-less MAIN, there can't be anything on the
    ** right of the root. */
    if ((*nodep)->pst_sym.pst_type == PST_CONST)
    {
	if ((*nodep)->pst_sym.pst_value.pst_s_cnst.pst_tparmtype != PST_RQPARAMNO
	  || (*nodep)->pst_sym.pst_value.pst_s_cnst.pst_parm_no <= x100cb->global->ops_qheader->pst_numparm)
	    return;		/* Not a SAGG result */

	/* This is a SAGG result - loop over SAGG sq's & their RESDOMs looking
	** for a match. Then make a VAR to replace the CONST and give it 
	** the RESDOM's name. */
	for (saggsubq = x100cb->global->ops_subquery; saggsubq;
			saggsubq = saggsubq->ops_next)
	{
	    if (saggsubq->ops_sqtype != OPS_SAGG)
		continue;

	    for (rsdmp = saggsubq->ops_root->pst_left, found = FALSE;
		!found && rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
	     if (rsdmp->pst_right && 
			rsdmp->pst_right->pst_sym.pst_type == PST_AOP &&
		rsdmp->pst_right->pst_sym.pst_value.pst_s_op.pst_opparm ==
			(*nodep)->pst_sym.pst_value.pst_s_cnst.pst_parm_no)
	     {
		/* Replace the CONST with a VAR with same name as RESDOM. */
		attrid.db_att_id = OPZ_NOATTR;
		(*nodep) = opv_varnode(x100cb->global, &(*nodep)->pst_sym.pst_dataval,
		    OPV_NOVAR, &attrid);
		MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
		    sizeof(DB_ATT_NAME), (char *)&(*nodep)->pst_sym.pst_value.
					pst_s_var.pst_atname);

		/* First see if subq is already in list - if not, add it. */
		if (subquery->ops_compile.opc_prsaggnext !=
					(OPS_SUBQUERY *)NULL)
		    for (worksq = subquery->ops_compile.opc_prsaggnext;
				worksq != (OPS_SUBQUERY *)NULL;
				worksq = worksq->ops_compile.opc_prsaggnext)
		     if (worksq == saggsubq)
		     {
			found = TRUE;
			break;
		     }

		if (found)
		    break;		/* again */

		saggsubq->ops_compile.opc_prsaggnext = subquery->
						ops_compile.opc_prsaggnext;
		subquery->ops_compile.opc_prsaggnext = saggsubq;

		/* Patch x100cb, too. */
		if (x100cb->saggsubq == saggsubq)
		 if (saggsubq->ops_next && 
				saggsubq->ops_next->ops_sqtype == OPS_SAGG)
		    x100cb->saggsubq = saggsubq->ops_next;
		 else x100cb->saggsubq = (OPS_SUBQUERY *) NULL;

		found = TRUE;
		break;			/* leave loop */
	     }
	}

	return;
    }
    else				/* anything but PST_CONST */
    {
	/* Recurse. */
	if ((*nodep)->pst_left)
	    opc_x100_saggmain(x100cb, subquery, &(*nodep)->pst_left);
	if ((*nodep)->pst_right)
	    opc_x100_saggmain(x100cb, subquery, &(*nodep)->pst_right);

	return;
    }
}

/*{
** Name: OPC_X100_PRSAGGEMIT	Emit syntax to materialize SAGGs
**
** Description:	Complete syntax generation for CartProd(, , 1) operators
**	by materializing SAGG expression.
**
** Inputs:
**      x100cb				ptr to X100 text state variable
**      subquery			ptr to main subquery
**
** Outputs:
**	Returns:
**	    count of SAGGs to terminate with ")"s.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	28-aug-2009 (dougi)
**	    Written for X100 support.
**	29-oct-2010 (dougi)
**	    Deal with UNIONs with the samne SAGG more than once.
**	6-july-2011 (dougi) m2213
**	    Slight adjustments for nested SAGGs.
*/
static i4
opc_x100_prsaggemit(
	X100_STATE	*x100cb,
	OPS_SUBQUERY	*subquery)

{
    OPS_SUBQUERY *saggsubq;
    i4		i;

    /* First, run the ops_prsaggnext chain of subqueries, counting them as
    ** we go and emitting a "CartProd(" operator for each one in excess
    ** of 1. */

    for (i = 0, saggsubq = subquery->ops_compile.opc_prsaggnext; 
		saggsubq != (OPS_SUBQUERY *) NULL; 
		saggsubq = saggsubq->ops_compile.opc_prsaggnext, i++)
    {
	if (saggsubq->ops_compile.opc_prsaggnext != (OPS_SUBQUERY *)NULL)
	    opc_x100_printf(x100cb, "CartProd( ");
	if ((subquery->ops_sqtype == OPS_VIEW || 
			subquery->ops_sqtype == OPS_UNION) &&
	    saggsubq->ops_mask2 & OPS_REUSE_PLANB &&
	    saggsubq->ops_mask2 & OPS_REUSE_USED)
	    opc_x100_printf(x100cb, (char *)&saggsubq->ops_reusetab);
	else opc_x100_subquery(saggsubq, (OPS_SUBQUERY *) NULL, x100cb, 
								(char *)NULL);
	if (saggsubq->ops_sqtype == OPS_SAGG && saggsubq->ops_bestco == NULL)
	    i--;			/* decrement count sent to caller */
	else if (saggsubq->ops_compile.opc_prsaggnext != (OPS_SUBQUERY *)NULL)
	    opc_x100_printf(x100cb, ", ");
    }

    return(i);

}

/*{
** Name: OPC_X100_UNION_SAGGCK	- check for multiple unions mappoing to 
**	same SAGG subqueries.
**
** Description:	Run through SAGG chains and union chains looking for SAGG
**	subqueries referenced in more than one select. They must be treated
**	as "reuse" subqueries.
**
** Inputs:
**      subquery			ptr to first union subquery
**
** Outputs:
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-apr-2010 (dougi)
**	    Written to handle union's with simple aggregates.
*/
static VOID
opc_x100_union_saggck(
	OPS_SUBQUERY	*subquery)

{
    OPS_SUBQUERY	*usq1, *usq2, *ssq1, *ssq2;
    char		sqname[DB_MAXNAME];
    i2			sqcount = 0;

    /* Nested loops looking for sq's that appear on the opc_prsaggnext chain
    ** of more than one sq on the ops_union chain. */
    for (usq1 = subquery; usq1 != (OPS_SUBQUERY *) NULL; 
					usq1 = usq1->ops_union)
     if (usq1->ops_sqtype != OPS_VIEW && usq1->ops_sqtype != OPS_UNION)
	    continue;
     else for (ssq1 = usq1->ops_compile.opc_prsaggnext;
		ssq1 != (OPS_SUBQUERY *) NULL; 
		ssq1 = ssq1->ops_compile.opc_prsaggnext)
     {
	for (usq2 = usq1->ops_union; usq2 != (OPS_SUBQUERY *) NULL;
					usq2 = usq2->ops_union)
	 if (usq2->ops_sqtype != OPS_VIEW && usq2->ops_sqtype != OPS_UNION)
	    continue;
	else for (ssq2 = usq2->ops_compile.opc_prsaggnext;
		ssq2 != (OPS_SUBQUERY *) NULL; 
		ssq2 = ssq2->ops_compile.opc_prsaggnext)
	 if (ssq1 == ssq2 && !(ssq1->ops_mask2 & OPS_REUSE_PLANB))
	 {
	    /* Got one. Compose reuse name and flag it. */
	    STprintf((char *)&sqname, "IISAGGREUSE%-u", sqcount++);
	    MEcopy((char *)&sqname, DB_MAXNAME, (char *)&ssq1->ops_reusetab);
	    ssq1->ops_mask2 |= OPS_REUSE_PLANB;
	 }
     }

}

/*{
** Name: OPC_X100_DMLOP	- process DML (update/delete) X100 query
**
** Description:	This function coordinates the construction of a X100
**	Delete() or Modify() statement, including the building of the
**	stream expression that identifies the affected rows.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				input data stream.
**	x100cb			Pointer to cross compiler state structure.
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      3-march-2009 (dougi)
**	    Written for initial support of X100 engine.
**	13-may-2010 (dougi)
**	    Fix ", " printing for UPDATE.
**	19-aug-2010 (dougi)
**	    Prepend "Ivw_sess_temp_" to temp table names.
**	17-sep-2010 (dougi)
**	    Small change to support DGTT AS for VW temp tables.
**	29-Oct-2010 (kschendel)
**	    Update above to look in RDF rather than pst-restab.
**	4-Jan-2011 (kschendel)
**	    Fill in result table DMT CB index in action header from valid entry.
**	11-Jan-2011 (kschendel)
**	    Fine-tune the above.
*/
static VOID
opc_x100_dmlop(
	OPS_SUBQUERY	*subquery,
	X100_STATE	*x100cb)

{
    OPS_STATE		*global = x100cb->global;
    OPV_GRV		*grv;
    PST_QNODE		*rsdmp;
    QEF_VALID		*ret_vl;
    char		var_name[X100_MAXNAME];
    i2			i;
    bool		notfirst;

    /* Start by emitting DML operator and target table name. Note, by
    ** convention, an Aggr() to compute "count(*)" is wrapped around the 
    ** DML operation to return the row count. */
    opc_x100_printf(x100cb, (x100cb->flag & X100_DELETE) ? "Aggr( Delete ('" :
		(x100cb->flag & X100_MODIFY) ? "Aggr( Modify ('" :
		(x100cb->flag & X100_APPEND) ? "Aggr( Append ('" : "Oops ('");

    grv = global->ops_rangetab.opv_base->opv_grv[subquery->ops_result];
    if (grv && grv->opv_relation && grv->opv_relation->rdr_rel
      && grv->opv_relation->rdr_rel->tbl_temporary)
    {
	/* Temp table - prepend "Ivw_sess_temp_". */
	MEcopy("_Ivw_sess_temp_", sizeof("_Ivw_sess_temp_"), (char *)&var_name);
	x100_print_id((char *)&global->ops_qheader->pst_restab.pst_resname,
		DB_TAB_MAXNAME, &var_name[14]);
    }
    else
    {
	x100_print_id((char *)&global->ops_qheader->pst_restab.pst_resname,
		DB_TAB_MAXNAME, var_name);
    }
    opc_x100_printf(x100cb, var_name);
    opc_x100_printf(x100cb, "', ");

    /* Now generate the source data flow (to identify affected rows). */
    opc_x100_subquery(subquery, (OPS_SUBQUERY *)NULL, x100cb, (char *) NULL);

    /* For Modify(), we now emit the assignments. Values were projected 
    ** as part of dataflow construction. Here we just emit the list of 
    ** assigned columns. */
    if (x100cb->flag & X100_MODIFY)
    {
	opc_x100_printf(x100cb, ", [");
	
	/* Silly loop to emit "x = x" for each updated column. */
	for (rsdmp = subquery->ops_root->pst_left, notfirst = FALSE;
	    rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
	    rsdmp = rsdmp->pst_left)
	{
	    if (!(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags 
				& PST_RS_PRINT))
		continue;			/* only those marked PRINT */

	    x100_print_id((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		DB_ATT_MAXNAME, var_name);
	    if (notfirst)
		opc_x100_printf(x100cb, ", ");
	    opc_x100_printf(x100cb, var_name);
	    opc_x100_printf(x100cb, " = ");
	    opc_x100_printf(x100cb, var_name);
	    notfirst = TRUE;
	}
	opc_x100_printf(x100cb, "]");
    }

    /* Finally, emit the tid "column" (for UPDATE/DELETE), then count(). */
    if (!(x100cb->flag & X100_APPEND))
        opc_x100_printf(x100cb, ", tid");
    opc_x100_printf(x100cb, "), [], [row_count = count()])");

    /* Emit result table valid list entry if needed.  Also, ask to
    ** have the action header ahd_dmtix fixed up with the result
    ** table's DMT_CB index.
    */
    opc_x100_valid(global, grv, TRUE);
}

/*{
** Name: OPC_X100_VARPUSH - searches and changes other VARs after namepush
**
** Description:	This function seeaches for matching VARs in same subquery
**	to one already renamed by opc_x100_namepush() to make sq consistent.
**
** Inputs:
**	nodep			Pointer to VAR just changed
**	node1p			Pointer to subtree being analyzed
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**	some PST_VAR nodes are renamed
**
** History:
**      26-jan-2010 (dougi)
**	    Written to resolve name space ambiguities.
*/
static VOID
opc_x100_varpush(
	PST_QNODE	*nodep,
	PST_QNODE	*node1p)

{

    /* Simply analyze the subquery's parse tree for other instances of
    ** same VAR node as one already changed - and copy the name. */

    for ( ; node1p; node1p = node1p->pst_left)
     if (nodep != node1p && node1p->pst_sym.pst_type == PST_VAR && 
	    nodep->pst_sym.pst_value.pst_s_var.pst_vno ==
		node1p->pst_sym.pst_value.pst_s_var.pst_vno &&
	    nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		node1p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	MEcopy((char *)&nodep->pst_sym.pst_value.pst_s_var.pst_atname.
					db_att_name, sizeof(DB_ATT_NAME),
	    (char *)&node1p->pst_sym.pst_value.pst_s_var.pst_atname.db_att_name);
     else if (node1p->pst_right)
	opc_x100_varpush(nodep, node1p->pst_right);

}

/*{
** Name: OPC_X100_NAMEPUSH - push pst_rsname's from AS clauses through VTs
**
** Description:	This function looks for renamed RESDOMs and pushes the new
**	name (assigned by AS clause) to underlying VT PST_RESDOM and
**	corresponding PST_VAR nodes (if any).
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure 
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**	some PST_RESDOM and PST_VAR nodes are renamed
**
** History:
**      25-jan-2010 (dougi)
**	    Written to resolve name space ambiguities.
**	14-may-2010 (dougi)
**	    Fine tuning - RESDOM's that map to a virtual table and on to a 
**	    reuse subquery need to be left.
**	22-june-2010 (dougi)
**	    Add code to force unique column rename values in MAIN sq.
**	4-mar-2011 (dougi)
**	    Add call to pursue REUSE refs into subqueries not on ops_next
**	    chain.
*/
static VOID
opc_x100_namepush(
	OPS_SUBQUERY	*subquery)

{
    OPS_SUBQUERY *vtsubq;
    PST_QNODE	*rsdmp, *rsdm1p, *nodep, *node1p;
    OPV_VARS	*varp, *var1p;
    OPZ_IATTS	attno;
    i4		namelen;
    char	rsname[X100_MAXNAME];


    /* Search for RESDOM nodes and push their names to underlying virtual
    ** table RESDOMs. */
    for (rsdmp = subquery->ops_root->pst_left;
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
    {
	if (!(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_PRINT))
	    continue;
	if ((nodep = rsdmp->pst_right)->pst_sym.pst_type != PST_VAR)
	    continue;
	if (nodep->pst_sym.pst_value.pst_s_var.pst_vno < 0 ||
	    nodep->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id < 0)
	    continue;
	opc_x100_copy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		sizeof(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
		rsname, sizeof(rsname));
	if (rsname[0] == 0) /* Test empty string as blanks trimmed */
	    continue;

	/* We have a RESDOM of potential interest. Does the PST_VAR map 
	** to a virtual table?? */
	if ((varp = subquery->ops_vars.opv_base->opv_rt[nodep->
		pst_sym.pst_value.pst_s_var.pst_vno])->opv_grv->opv_relation)
	    continue;				/* not a virtual table */
	if ((vtsubq = varp->opv_grv->opv_gquery) == (OPS_SUBQUERY *) NULL)
	    continue;

	if (subquery->ops_sqtype == OPS_MAIN)
	{
	    /* Renamed names can be the same as some other column and that 
	    ** causes problems for VectorWise. So, coarse a change as this 
	    ** may be (not explicitly checking for such conflicts), we 
	    ** automatically apply a name distinctifier here. 
	    **
	    ** And we only do it for the MAIN subquery. */
	    namelen = STlen(rsname);

	    STprintf(rsname+namelen, "_%-u",
		    rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno);
	    				/* add pst_rsno to distinctify */
	    MEmove(STlen(rsname), rsname, ' ',
			sizeof(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
		    (char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname);
	}

	/* Get pst_rsno of mapping virtual table RESDOM. */
	attno = subquery->ops_attrs.opz_base->opz_attnums[nodep->
		pst_sym.pst_value.pst_s_var.pst_atno.db_att_id]->
		opz_attnm.db_att_id;
	for (rsdm1p = vtsubq->ops_root->pst_left;
		rsdm1p && rsdm1p->pst_sym.pst_type == PST_RESDOM;
		rsdm1p = rsdm1p->pst_left)
	 if (rsdm1p->pst_sym.pst_value.pst_s_rsdm.pst_rsno == attno)
	 {
	    if ((vtsubq->ops_sqtype == OPS_REUSE || 
			(vtsubq->ops_mask2 & OPS_REUSE_PLANB)) &&
		subquery->ops_sqtype == OPS_VIEW)
		break;				/* special case we skip */

	    if ((vtsubq->ops_sqtype == OPS_REUSE || 
			(vtsubq->ops_mask2 & OPS_REUSE_PLANB)) &&
		(rsdm1p->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
				PST_RS_REUSEREN))
	    {
		MEcopy((char *)&rsdm1p->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		    sizeof(DB_ATT_NAME), (char *)&rsdmp->pst_sym.pst_value.
		    pst_s_rsdm.pst_rsname);	/* copy from inner RESDOM */
		MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		    sizeof(DB_ATT_NAME), (char *)&nodep->pst_sym.pst_value.
		    pst_s_var.pst_atname);	/* copy to outer VAR, too */
	    }
	    else if ((node1p = rsdm1p->pst_right)->pst_sym.pst_type == PST_VAR)
	    {
		/* Do another hack check. */
		var1p = vtsubq->ops_vars.opv_base->opv_rt[node1p->pst_sym.
				pst_value.pst_s_var.pst_vno];
		if (var1p->opv_grv->opv_gquery != (OPS_SUBQUERY *) NULL &&
			var1p->opv_grv->opv_gquery->ops_sqtype == OPS_REUSE)
		    break;
	    }
	    else 
	    {
		MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		    sizeof(DB_ATT_NAME), (char *)&rsdm1p->pst_sym.pst_value.
		    pst_s_rsdm.pst_rsname);	/* copy to inner RESDOM */
		MEcopy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		    sizeof(DB_ATT_NAME), (char *)&nodep->pst_sym.pst_value.
		    pst_s_var.pst_atname);	/* copy to outer VAR, too */

		if (vtsubq->ops_sqtype == OPS_REUSE || 
			(vtsubq->ops_mask2 & OPS_REUSE_PLANB))
		    rsdm1p->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |=
				PST_RS_REUSEREN;
	    }
	    opc_x100_varpush(nodep, subquery->ops_root);
						/* adjust other VARs in sq */
	    break;
	 }

	/* If the column identified for renaming was in a REUSE sq, 
	** look for other references to it and change them, too. */
	if (vtsubq->ops_sqtype == OPS_REUSE || 
		(vtsubq->ops_mask2 & OPS_REUSE_PLANB))
	    opc_x100_namepush_vt(vtsubq, rsdmp->pst_sym.pst_value.pst_s_rsdm.
			pst_rsname, attno);

    }
}

/*{
** Name: OPC_X100_NAMEPUSH_VT - search for other references to pushed name
**
** Description:	If a name has been reset in a REUSE or PLANB subquery, look
**	for other refs to the name and change them, too.
**
** Inputs:
**	subquery		Pointer to REUSE subquery
**	rsname			Pointer to new name
**	attno			DMF attr no of column being searched
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      3-mar-2011 (dougi)
**	    Written.
*/
static VOID
opc_x100_namepush_vt(
	OPS_SUBQUERY	*subquery,
	char	*rsname,
	OPZ_IATTS attno)

{
    OPS_STATE	*global = subquery->ops_global;
    OPS_SUBQUERY *vtsubq, *subq;
    PST_QNODE	*rsdmp, *varp;
    OPZ_ATTS	*attrp;
    i4		i;
    bool	found;

    /* Loop over all subqueries (except current), looking for ones that 
    ** have virtual tables mapped to current. */
    for (subq = global->ops_subquery; subq; subq = subq->ops_onext)
    {
	if (subq == subquery)
	    continue;			/* skip REUSE subquery */

	for (i = 0, found = FALSE; i < subq->ops_vars.opv_prv; i++)
	 if ((vtsubq = subq->ops_vars.opv_base->opv_rt[i]->opv_grv->
			opv_gquery) == subquery)
	 {
	    found = TRUE;
	    break;
	 }

	if (!found)
	    continue;

	/* This subquery has a reference to the REUSE. Check its RESDOMs
	** for match to renamed column and copy. */
	for (rsdmp = subq->ops_root->pst_left; 
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM; 
		rsdmp = rsdmp->pst_left)
	{
	    if ((varp = rsdmp->pst_right) && varp->pst_sym.pst_type != PST_VAR)
		continue;
	    if (varp->pst_sym.pst_value.pst_s_var.pst_vno != i)
		continue;

	    attrp = subq->ops_attrs.opz_base->opz_attnums[varp->
			pst_sym.pst_value.pst_s_var.pst_atno.db_att_id];
	    if (attrp->opz_attnm.db_att_id != attno)
		continue;

	    if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags &
							PST_RS_REUSEREN)
		continue;

	    MEmove(STlen(rsname), rsname, ' ', 
		sizeof(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname), 
		(char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname);
	    rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |= PST_RS_REUSEREN; 
	}
    }
}

/*{
** Name: OPC_X100_DISTNAMES - assure distinct pst_rsname values
**
** Description:	This function examines the subquery RESDOM chain, looking
**	for duplicate pst_rsname values. Any that are found are made 
**	distinct.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure 
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      13-jan-2010 (dougi)
**	    Written to resolve name space ambiguities.
**	13-apr-2010 (dougi)
**	    Remove bypass for identical VARs - to allow "select sno, sno ...".
**	7-july-2011 (dougi) m2203
**	    Reinstate removal of identical VAR when one is non-printing.
*/
static VOID
opc_x100_distnames(
	OPS_SUBQUERY	*subquery)

{
    PST_QNODE	*rsdmp, *rsdm1p, **rsdmpp, **rsdm1pp, *varp, *var1p;
    char	rsname[X100_MAXNAME];
    i4		namelen;
    bool	drop;

    /* Simply execute nested loops to compare RESDOM entries against
    ** one another. If dup is found, distinctify the 2nd using its
    ** pst_rsno value. */
    if (subquery->ops_root == (PST_QNODE *) NULL ||
	subquery->ops_root->pst_left == (PST_QNODE *) NULL)
	return;

    for (rsdmp = subquery->ops_root->pst_left, 
			rsdmpp = &subquery->ops_root->pst_left; 
	rsdmp != (PST_QNODE *) NULL && rsdmp->pst_sym.pst_type == PST_RESDOM;
	rsdmp = rsdmp->pst_left)
    {
	for (rsdm1p = rsdmp->pst_left, rsdm1pp = &rsdmp->pst_left;
	    rsdm1p != (PST_QNODE *) NULL && 
			rsdm1p->pst_sym.pst_type == PST_RESDOM;
	    rsdm1p = rsdm1p->pst_left)
	{
	    /* Before anything else, if the RESDOM's both address the same 
	    ** PST_VAR, drop one of them. */
	    drop = FALSE;
	    if ((varp = rsdmp->pst_right)->pst_sym.pst_type == PST_VAR &&
		(var1p = rsdm1p->pst_right)->pst_sym.pst_type == PST_VAR &&
		varp->pst_sym.pst_value.pst_s_var.pst_vno ==
		    var1p->pst_sym.pst_value.pst_s_var.pst_vno &&
		varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
		    var1p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
	    {
		if (!(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
				PST_RS_PRINT))
		{
		    drop = TRUE;
		    (*rsdmpp) = rsdmp->pst_left;
		    break;
		}
		else if (!(rsdm1p->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & 
				PST_RS_PRINT))
		{
		    drop = TRUE;
		    (*rsdm1pp) = rsdm1p->pst_left;
		    continue;
		}
	    }

	    /* Got a pair of RESDOMs - compare their names. */
	    opc_x100_copy((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		sizeof(rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname),
		rsname, sizeof(rsname));
	    if (rsname[0] != 0 &&
		MEcmp((char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname,
		( char *)&rsdm1p->pst_sym.pst_value.pst_s_rsdm.pst_rsname, 
				sizeof(DB_ATT_NAME)) == 0)
	    {
		/* Next verify that the RHS expressions are distinct - more 
		** specifically, if they are both the same PST_VAR, skip
		** the distinctification. Parse trees apparently can have 
		** this condition. 
		**
		** This is a bit delicate, though, and we only use this logic 
		** for non-MAIN subqueries. We may later need to find another
		** solution to the problem that this solves. */

		if (subquery->ops_sqtype != OPS_MAIN && 
		    (varp = rsdmp->pst_right) && 
		    varp->pst_sym.pst_type == PST_VAR &&
		    (var1p = rsdm1p->pst_right) && 
		    var1p->pst_sym.pst_type == PST_VAR &&
		    varp->pst_sym.pst_value.pst_s_var.pst_vno ==
			var1p->pst_sym.pst_value.pst_s_var.pst_vno &&
		    varp->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id ==
			var1p->pst_sym.pst_value.pst_s_var.pst_atno.db_att_id)
		    continue;

		/* Names are equal and non-blank. Copy one and distinctify. */
		namelen = STlen(rsname);

		STprintf(rsname + namelen, "_%-u",
		    rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsno);
	    				/* add pst_rsno to distinctify */
		MEmove(STlen(rsname), rsname, ' ', sizeof(DB_ATT_NAME),
		    (char *)&rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsname);
		rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags |= 
							PST_RS_VTRENAMED;
		subquery->ops_mask2 |= OPS_X100_RSDREN;
	    }
	    if (!drop)
		rsdm1pp = &rsdm1p->pst_left;	/* advance splice point */
	}
	if (!drop)
	    rsdmpp = &rsdmp->pst_left;	/* advance splice point */
    }

}

/*{
** Name: OPC_X100_SUBQUERY	- process OPS_SUBQUERY in X100 query.
**
** Description:	This function processes a OPS_SUBQUERY for an X100 query.
**	Special stuff happens for aggregate, view or union subqueries.
**
** Inputs:
**	subquery		Pointer to OPS_SUBQUERY structure defining 
**				current subquery.
**	prevsubq		Pointer to previous subquery, if current
**				is OPS_MAIN and has no CO-tree.
**	x100_action		Pointer to action header for X100 query.
**	vtname			SQ virtual table name (or NULL).
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      9-july-2008 (dougi)
**	    Written for initial support of X100 engine.
**	28-aug-2009 (dougi)
**	    Changes to support multiple SAGGs from the same noCO query.
**	11-dec-2009 (dougi)
**	    Add Select() for noCO MAIN.
**	8-jan-2010 (dougi)
**	    Support As() operator for virtual tables.
**	13-jan-2010 (dougi)
**	    Add function call to distinctify pst_rsname values.
**	25-jan-2010 (dougi)
**	    Add function to push outer RESDOM names into inner VARs.
**	1-apr-2010 (dougi)
**	    Fix HAVING clauses in CO-less OPS_MAINs.
**	19-apr-2010 (dougi)
**	    Significant changes required for UNION queries with simple
**	    aggregates.
**	22-apr-2010 (dougi)
**	    Add code to prevent a SAGG subquery from being compiled twice.
**	30-apr-2010 (dougi)
**	    Look for SAGGs even when subquery has CO. They can happen when
**	    there are simple aggregate derived tables in a query.
**	30-aug-2010 (dougi)
**	    Add code to use "reuse" for multiply-occurring refs to same SAGG.
**	19-oct-2010 (dougi)
**	    Handle UNION queries with FROM-less SELECTs.
**	2-Dec-2010 (kschendel)
**	    Fix a long-name oops.
**	20-june-2011 (dougi) m2123
**	    Fix to handle multiple SAGGs in noCO MAIN.
**	6-july-2011 (dougi) m2213
**	    Fix for nested SAGGs.
*/
static VOID
opc_x100_subquery(
	OPS_SUBQUERY	*subquery,
	OPS_SUBQUERY	*prevsubq,
	X100_STATE	*x100cb,
	char		*vtname)
{
    OPS_STATE		*global = x100cb->global;
    OPS_SUBQUERY	*oldsubq = x100cb->subquery;
    OPS_SUBQUERY	*sq, *saggsubq = (OPS_SUBQUERY *) NULL;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    PST_QNODE		*rsdmp;
    OPO_CO		*cop = subquery->ops_bestco;
    i4			i, saggcount = 1;
    char		asname[32];
    bool		noCO = (subquery->ops_bestco == (OPO_CO *) NULL);
    bool		unionsq = FALSE;


    /* Don't recompile a SAGG that's already been compiled. */
    if (subquery->ops_sqtype == OPS_SAGG)
    {
	STprintf(asname, "IISAGG_%-u", subquery->ops_tsubquery);
	if (subquery->ops_mask2 & OPS_X100_SAGGDONE)
	{
	    /* SAGG already emitted - if BF, print stream name. Then return. */
	    if (subquery->ops_mask2 & OPS_X100_BFSAGG)
		opc_x100_printf(x100cb, asname);
	    return;
	}
	else if (subquery->ops_mask2 & OPS_X100_BFSAGG)
	{
	    /* First time for this SAGG - emit " = " after name, then
	    ** subquery itself. */
	    opc_x100_printf(x100cb, asname);
	    opc_x100_printf(x100cb, " = ");
	}
	subquery->ops_mask2 |= OPS_X100_SAGGDONE;
    }

    /* Before anything else, call functions to push names from AS clauses
    ** to where the expressions are generated and to check for duplicate 
    ** names in subquery RESDOM list, distinctifying any that are found. 
    ** The syntax interface with X100 demands that our names be distinct 
    ** within a given scope. 
    **
    ** An annoying extenuating factor is that we must only do this for the
    ** first "select" of a set of UNIONs. */
    if (!((x100cb->flag & X100_UNION) && (subquery->ops_sqtype == OPS_VIEW ||
			subquery->ops_sqtype == OPS_UNION)))
	opc_x100_namepush(subquery);
    opc_x100_distnames(subquery);

    if (subquery->ops_sqtype != OPS_MAIN && vtname != (char *)NULL)
    {
	opc_x100_printf(x100cb, "As (");
	x100_print_id(vtname, DB_TAB_MAXNAME, asname);
    }

    /* If this is the MAIN subquery and there is no CO tree, emit a 
    ** Project(), then do the previous subquery, and finally, emit the
    ** projection list. Otherwise, just do the normal thing. */
    if (noCO)
    {
	if (x100cb->saggsubq && (subquery->ops_sqtype == OPS_MAIN ||
	    subquery->ops_sqtype == OPS_VIEW || 
		subquery->ops_sqtype == OPS_UNION))
	{
	    /* This is a vanilla simple aggregate query (though possibly in
	    ** a UNION query). Call func to look for RESDOMs in the MAIN or
	    ** UNION sqs addressing PST_CONSTS representing aggregate results 
	    ** and replace the CONSTs with PST_VARs with the RESDOM's name. */
	    for (sq = subquery;
		sq != (OPS_SUBQUERY *) NULL; sq = sq->ops_union)
		opc_x100_saggmain(x100cb, sq, &sq->ops_root->pst_left);
	    if (subquery->ops_sqtype == OPS_VIEW || 
				subquery->ops_sqtype == OPS_UNION)
		opc_x100_union_saggck(subquery);
	}

	/* Check for union. */
	if (subquery->ops_sqtype == OPS_VIEW && 
			subquery->ops_union != (OPS_SUBQUERY *) NULL)
	{
	    unionsq = TRUE;
	    opc_x100_printf(x100cb, "Union (");
	    x100cb->flag |= X100_UNION;
	}

	/* A noCO SAGG needs an Aggr(), not a Project(). */
	if (subquery->ops_sqtype == OPS_SAGG)
	    opc_x100_printf(x100cb, "Aggr (");
	else opc_x100_printf(x100cb, "Project (");

	/* If there are predicates on the MAIN parse tree, we need to emit
	** Select()s out here. Resolve SAGG CONSTs while we're there.*/
	if (subquery->ops_bfs.opb_bv > 0 || 
	    subquery->ops_bfs.opb_bfconstants != (PST_QNODE *) NULL)
	{
	    for (i = 0; i < subquery->ops_bfs.opb_bv + 
		(subquery->ops_bfs.opb_bfconstants != (PST_QNODE *) NULL) ? 
							1 : 0; i++)
	     opc_x100_printf(x100cb, "Select (");
	    if (global->ops_gmask & OPS_SOMESAGGS)
	    {
		/* Look for and emit SAGG source for HAVING preds. */
		opc_x100_bfsaggcheck(x100cb, subquery, 
					(OPO_CO *) NULL, &saggsubq);
		if (saggsubq != (OPS_SUBQUERY *) NULL)
		    opc_x100_bfsaggemit(x100cb, saggsubq, TRUE);
	    }
	}

	/* Another spot to possibly emit SAGG subquery. */
	if (x100cb->saggsubq && subquery->ops_sqtype != OPS_MAIN)
	    opc_x100_saggmain(x100cb, subquery, &subquery->ops_root->pst_left);

	x100cb->subquery = prevsubq;
	if (subquery->ops_compile.opc_prsaggnext != (OPS_SUBQUERY *)NULL)
	{
	    /* Generate X100 for SAGGs in select-list, then close
	    ** Cartprod()s (if any). */
	    saggcount = opc_x100_prsaggemit(x100cb, subquery);
	    for (; saggcount > 1; saggcount--)
		opc_x100_printf(x100cb, ") ");
	}
	else if (saggsubq == (OPS_SUBQUERY *) NULL)
	{
	    if (subquery->ops_sqtype == OPS_VIEW || 
			subquery->ops_sqtype == OPS_UNION)
		opc_x100_printf(x100cb, "Array([ __from_less = 1])");
	    else opc_x100_printf(x100cb, ", ");
	}

	/* Emit the predicates (if any). */
	if (subquery->ops_bfs.opb_bv > 0 || 
	    subquery->ops_bfs.opb_bfconstants != (PST_QNODE *) NULL)
	    opc_x100_bfemit(x100cb, subquery, (OPO_CO *) NULL, 0, OPL_NOOUTER,
		(subquery->ops_bfs.opb_bfconstants != (PST_QNODE *) NULL) ? 
			TRUE : FALSE, FALSE);

	/* And emit the projections - if SAGG, prepend empty grouping list. */
	if (subquery->ops_sqtype == OPS_SAGG)
	    opc_x100_printf(x100cb, ", []");	/* SAGG noCO - another aggr */
	opc_x100_presdom(x100cb, cop, subquery->ops_root->pst_left, 
						TRUE, TRUE, FALSE, FALSE);
	opc_x100_printf(x100cb, ")");

	if (unionsq)
	{
	    /* Comma separation, next subquery in union, then closing paren. */
	    opc_x100_printf(x100cb, ", ");
	    opc_x100_subquery(subquery->ops_union, prevsubq, x100cb,
								(char *) NULL);
	    opc_x100_printf(x100cb, ")");
	}
	if (subquery->ops_sqtype == OPS_VIEW &&
		subquery->ops_union == (OPS_SUBQUERY *) NULL)
	    x100cb->flag &= ~X100_UNION;		/* turn off flag */
    }
    else 
    {
	if (subquery->ops_mask2 & OPS_REUSE_USED)
	{
	    char	asop[DB_TAB_MAXNAME+1];
	    /* Reference to already materialized REUSE subquery - just 
	    ** emit sq name, close virtual table As() operator and 
	    ** we're done. */
	    STprintf(asop, " %s", &subquery->ops_reusetab.db_tab_name[0]);
	    opc_x100_printf(x100cb, asop);
	    x100cb->flag |= X100_REUSE;

	    /* (So far) SAGG reuse subqueries only happen in UNION queries
	    ** and we don't emit the AS name. Just proceed to the operation
	    ** for which the reuse is being used. */
	    if (subquery->ops_sqtype == OPS_SAGG)
		return;

	    opc_x100_printf(x100cb, ", ");
	    opc_x100_printf(x100cb, asname);
	    opc_x100_printf(x100cb, ")");
	    return;
	}
 
	else if (subquery->ops_sqtype == OPS_REUSE ||
		(subquery->ops_mask2 & OPS_REUSE_PLANB))
	{
	    /* First reference to REUSE subquery. Materialize and assign
	    ** to reuse name. */
	    opc_x100_printf(x100cb, (char *)&subquery->ops_reusetab);
	    opc_x100_printf(x100cb, " = ");
	}

	if (x100cb->saggsubq && (subquery->ops_sqtype == OPS_MAIN ||
	    ((subquery->ops_sqtype == OPS_VIEW || 
		subquery->ops_sqtype == OPS_UNION) && 
		!(x100cb->flag & X100_UNION))))
	{
	    /* This is a vanilla simple aggregate query (though possibly in
	    ** a UNION query). Call func to look for RESDOMs in the MAIN or
	    ** UNION sqs addressing PST_CONSTS representing aggregate results 
	    ** and replace the CONSTs with PST_VARs with the RESDOM's name. */
	    for (sq = subquery;
		sq != (OPS_SUBQUERY *) NULL; sq = sq->ops_union)
		opc_x100_saggmain(x100cb, sq, &sq->ops_root->pst_left);
	    if (subquery->ops_sqtype == OPS_VIEW || 
				subquery->ops_sqtype == OPS_UNION)
		opc_x100_union_saggck(subquery);
	}

	x100cb->subquery = subquery;

	/* Check for union. */
	if (subquery->ops_sqtype == OPS_VIEW && 
			subquery->ops_union != (OPS_SUBQUERY *) NULL)
	{
	    unionsq = TRUE;
	    opc_x100_printf(x100cb, "Union (");
	    x100cb->flag |= X100_UNION;
	}

	opc_x100_projaggr(subquery, x100cb);

	if (subquery->ops_sqtype == OPS_REUSE ||
		(subquery->ops_mask2 & OPS_REUSE_PLANB))
	    subquery->ops_mask2 |= OPS_REUSE_USED;

	if (unionsq)
	{
	    /* Comma separation, next subquery in union, then closing paren. */
	    opc_x100_printf(x100cb, ", ");
	    opc_x100_subquery(subquery->ops_union, prevsubq, x100cb,
								(char *) NULL);
	    opc_x100_printf(x100cb, ")");
	}

	if (subquery->ops_sqtype == OPS_VIEW &&
		subquery->ops_union == (OPS_SUBQUERY *) NULL)
	    x100cb->flag &= ~X100_UNION;		/* turn off flag */
    }

    if (subquery->ops_sqtype != OPS_MAIN && vtname != (char *)NULL)
    {
	/* Close out "As()" operator with name. */
	opc_x100_printf(x100cb, ", ");
	opc_x100_printf(x100cb, asname);
	opc_x100_printf(x100cb, ")");
    }

    x100cb->subquery = oldsubq;
    return;

}

/*{
** Name: OPC_X100AHD_BUILD	- build a X100 action header.
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	Exceptions:
**
** Side Effects:
**
** History:
**      9-july-2008 (dougi)
**	    Written for initial support of X100 engine.
**	26-apr-2010 (dougi)
**	    Fiddle ADF float settings to get better precision.
**	22-Sep-2010 (kschendel)
**	    Drop autocommit flag, not needed now that X100 side has txn support.
**	2-Dec-2010 (kschendel)
**	    Also drop the CTAS recovery flag and tabname, same reason.
**	4-Jan-2011 (kschendel)
**	    Add result table DMT CB index for DML queries.
*/
FUNC_EXTERN VOID
opc_x100ahd_build(
	OPS_STATE	*global,
	QEF_AHD		**x100_action)
{
    OPS_SUBQUERY	*subqry, *prevsq;
    QEF_QP_CB		*qp = global->ops_cstate.opc_qp;
    QEF_AHD		*ahd;
    OPC_PST_STATEMENT	*opc_pst;
    PST_QNODE		*rsdmp;
    ULD_TSTATE		textcb;
    X100_STATE		x100cb;
    i4			rowsz;

    *x100_action = (QEF_AHD *) NULL;
    x100cb.saggsubq = (OPS_SUBQUERY *) NULL;
    x100cb.error.err_code = E_UL0000_OK;
    x100cb.error.err_data = 0;
    x100cb.retstatus = E_DB_OK;


    *x100_action = ahd = (QEF_AHD *) opu_qsfmem(global, sizeof (QEF_AHD)-
			sizeof((*x100_action)->qhd_obj) + sizeof(QEF_X100_QRY));

    if (global->ops_cstate.opc_topahd == NULL)		/* Top of QEP */
	global->ops_cstate.opc_topahd = ahd;
    if (global->ops_cstate.opc_firstahd == NULL)	/* First of QP */
	global->ops_cstate.opc_firstahd = ahd;
    opc_pst = (OPC_PST_STATEMENT *)global->ops_statement->pst_opf;
    if (opc_pst->opc_stmtahd == NULL)
	opc_pst->opc_stmtahd = ahd;

    /* Fill in the action boiler plate stuff */
    ahd->ahd_next = ahd->ahd_prev = ahd;
    ahd->ahd_length = sizeof(QEF_AHD) -
			    sizeof(ahd->qhd_obj) + sizeof(QEF_X100_QRY);
    ahd->ahd_type = QEAHD_CB;
    ahd->ahd_ascii_id = QEAHD_ASCII_ID;
    ahd->ahd_valid = NULL;
    ahd->ahd_atype = QEA_X100_QRY;
    ahd->ahd_mkey = (QEN_ADF *)NULL;
    ahd->qhd_obj.qhd_x100Query.ahd_dmtix = -1;

    qp->qp_ahd_cnt += 1;

    /* Init X100-specific structure of action header. */
    ahd->qhd_obj.qhd_x100Query.ahd_x100_text = (PTR) NULL;
    ahd->qhd_obj.qhd_x100Query.ahd_x100_textlen = 0;
    ahd->qhd_obj.qhd_x100Query.ahd_x100_flag = 0;
    ahd->qhd_obj.qhd_x100Query.ahd_x100_params = NULL;
    ahd->qhd_obj.qhd_x100Query.ahd_x100_numparams = 0;
    opc_ptcb(global, &ahd->qhd_obj.qhd_x100Query.ahd_x100_cb, 0);

    textcb.uld_offset = 0;
    textcb.uld_remaining = ULD_TSIZE;
    textcb.uld_tstring = (ULD_TSTRING **) NULL;

    x100cb.global = global;
    /* Copy defaults from adfcb and override for X100 */
    x100cb.adfcb = *global->ops_adfcb;
    x100cb.adfcb.adf_outarg.ad_c0width = 0;	/* min field width for char/c */
    x100cb.adfcb.adf_outarg.ad_t0width = 0;	/* min field width for varchar/text */
    x100cb.adfcb.adf_outarg.ad_i1width = 0;	/* min field width for i1 */
    x100cb.adfcb.adf_outarg.ad_i2width = 0;	/* min field width for i2 */
    x100cb.adfcb.adf_outarg.ad_i4width = 0;	/* min field width for i4 */
    x100cb.adfcb.adf_outarg.ad_i8width = 0;	/* min field width for i8 */
    x100cb.adfcb.adf_outarg.ad_f4width = 14;/* min field width for f4 */
    x100cb.adfcb.adf_outarg.ad_f4prec = 6;	/* prec for f4 */
    x100cb.adfcb.adf_outarg.ad_f8width = 24;/* min field width for f8 */
    x100cb.adfcb.adf_outarg.ad_f8prec = 17;	/* prec for f8 */
    x100cb.adfcb.adf_outarg.ad_f4style = 'g'; /* e f g or n */
    x100cb.adfcb.adf_outarg.ad_f8style = 'g'; /* e f g or n */
    x100cb.textcb = &textcb;
    x100cb.seglist = (DD_PACKET *) NULL;
    x100cb.querylen = 0;
    x100cb.flag = 0;
    x100cb.params = NULL;
    x100cb.numparams = x100cb.maxparams = 0;
    x100cb.noCOsaggs = 0;

    if (Opf_x100_debug)
	opt_pt_entry(global, global->ops_qheader, global->ops_subquery->
			ops_root, "Before X100: ", TRUE, TRUE, FALSE);

    /* Loop over subqueries looking for MAIN. */
    for (subqry = global->ops_subquery, prevsq = (OPS_SUBQUERY *) NULL; 
	subqry && subqry->ops_sqtype != OPS_MAIN;
	prevsq = subqry, subqry = subqry->ops_next)
     if (subqry->ops_sqtype == OPS_SAGG)
     {
	if (x100cb.saggsubq == (OPS_SUBQUERY *) NULL)
	{
	    char	sqname[DB_MAXNAME];

	    x100cb.saggsubq = subqry;		/* remember first SAGG */
	}
	subqry->ops_compile.opc_bfsaggnext =
		subqry->ops_compile.opc_prsaggnext = (OPS_SUBQUERY *) NULL;
					/* NULL saggnexts while we're here */
     }

    /* Check for DML op. */
    if (subqry->ops_mode == PSQ_DELETE)
    {
	x100cb.flag |= X100_DELETE;
	ahd->qhd_obj.qhd_x100Query.ahd_x100_flag |= AHD_X100_DELETE;
    }
    else if (subqry->ops_mode == PSQ_REPLACE)
    {
	x100cb.flag |= X100_MODIFY;
	ahd->qhd_obj.qhd_x100Query.ahd_x100_flag |= AHD_X100_UPDATE;
    }
    else if (subqry->ops_mode == PSQ_APPEND ||
		subqry->ops_mode == PSQ_RETINTO ||
		subqry->ops_mode == PSQ_X100_DGTT_AS)
    {
	x100cb.flag |= X100_APPEND;
	ahd->qhd_obj.qhd_x100Query.ahd_x100_flag |= AHD_X100_APPEND;
    }

    /* subqry points to OPS_MAIN subquery. Go straight to query gen or 
    ** stop off at DML generator, first. */
    if (x100cb.flag & X100_DMLOP)
	opc_x100_dmlop(subqry, &x100cb);
    else opc_x100_subquery(subqry, prevsq, &x100cb, (char *) NULL);

    /* Check for goofy queries that have "0 = 1" pred and return no rows. */
    if (subqry->ops_sqtype == OPS_MAIN && subqry->ops_bfs.opb_qfalse)
	ahd->qhd_obj.qhd_x100Query.ahd_x100_flag |= AHD_X100_NOROWS;

    if (global->ops_cb->ops_alter.ops_parallel && 
	    global->ops_cb->ops_alter.ops_pq_dop > 0)
    {
	char	text[36];

	/* Add "max_parallelism_level" annotation. */
	STprintf(text, " ['max_parallelism_level' = %d]", 
	    global->ops_cb->ops_alter.ops_pq_dop);
	opc_x100_printf(&x100cb, text);
    }
    /* Move rest of temp data into linked list. */
    opc_x100_flush(&x100cb); 

    opc_x100_textcopy(&x100cb, *x100_action);	/* copy query text to action */

    /* Build result row descriptor. */
    if (x100cb.flag & X100_DMLOP)
    {
	/* DMLs have a single i4 result column - the count of affected rows. */
	GCA_TUP_DESCR	*rrfmt;

	/* Allocate the result row descriptor. */
	rrfmt = (GCA_TUP_DESCR *) opu_qsfmem(global, sizeof(GCA_TUP_DESCR));
       
        /* Set up result row format */
        rrfmt->gca_tsize = sizeof(i4);
        rrfmt->gca_result_modifier = 0;
        rrfmt->gca_id_tdscr = 0;
        rrfmt->gca_l_col_desc = 1;

        rrfmt->gca_col_desc[0].gca_attdbv.db_data = 0;
        rrfmt->gca_col_desc[0].gca_attdbv.db_length = sizeof(i8);
        rrfmt->gca_col_desc[0].gca_attdbv.db_datatype = DB_INT_TYPE;
        rrfmt->gca_col_desc[0].gca_attdbv.db_prec = 0;
        rrfmt->gca_col_desc[0].gca_l_attname = 0;

	ahd->qhd_obj.qhd_x100Query.ahd_resrow_format = (PTR)rrfmt;
	ahd->qhd_obj.qhd_x100Query.ahd_resrow_size = sizeof(i4);
    }
    else
    {
	/* The whole result row. */
	ulc_bld_descriptor(subqry->ops_root, 
	    GCA_NAMES_MASK, &global->ops_qsfcb,
	    (GCA_TD_DATA **)&ahd->qhd_obj.qhd_x100Query.ahd_resrow_format, 0);
	for (rsdmp = subqry->ops_root->pst_left, rowsz = 0; 
		rsdmp && rsdmp->pst_sym.pst_type == PST_RESDOM;
		rsdmp = rsdmp->pst_left)
	 if (rsdmp->pst_sym.pst_value.pst_s_rsdm.pst_rsflags & PST_RS_PRINT)
	    rowsz += rsdmp->pst_sym.pst_dataval.db_length;
	ahd->qhd_obj.qhd_x100Query.ahd_resrow_size = rowsz;
    }

    if (global->ops_cb->ops_check &&
	            (opt_strace(global->ops_cb, OPT_F079_X100_SYNTAX_DUMP)))
    {
	char        temp[OPT_PBLEN + 1];

	if (global->ops_cstate.opc_prbuf == NULL)
	    global->ops_cstate.opc_prbuf = temp;

	TRformat(opt_scc, (i4*)NULL, global->ops_cstate.opc_prbuf,
                OPT_PBLEN, "VectorWise query text: %.#s\n\n",
                ahd->qhd_obj.qhd_x100Query.ahd_x100_textlen,
                ahd->qhd_obj.qhd_x100Query.ahd_x100_text);
	global->ops_cstate.opc_prbuf = NULL;		/* reset */
    }
 
    return;
}


/*{
** Name: opc_x100_adj_coerce	- adjust coercion operator from implicit to explicit
**
** Description:
**	With the parse tree annotated with the required conversions we could
**	do with these being translated into explicit form to derive the name.
**
** Inputs:
**	x100cb		X100_STATE state block.
**	qnode		Operator node
**	oprd		operand number that this corresponds to.
**	coerce_fiid	The coersion fiid that is in the tree. Usually
**			this will be ADI_NILCOERCE (-3) and nothing needs
**			doing.
**
** Outputs:
**	Returns:	coerce_fiid or adjusted form
**	Exceptions:
**
** Side Effects:
**
** History:
**	13-Sep-2009 (kiria01)
**	    Written for implicit coercion fixups.
**	19-Aug-2011 (kiria01) b125530
**	    Adjust for adi_castid interface change.
*/
static ADI_FI_ID
opc_x100_adj_coerce(
	X100_STATE	*x100cb,
	PST_QNODE	*qnode,
	i4		oprd,
	ADI_FI_ID	coerce_fiid)
{
    if (coerce_fiid != ADI_NILCOERCE)
    {
	ADI_FI_DESC *fdesc = qnode->pst_sym.pst_value.pst_s_op.pst_fdesc;
	DB_DT_ID dt;
	if (fdesc)
	{
	    /* With fdesc, we should have an indication as to the resolved
	    ** input datatypes (including dtfamily processed) so that we
	    ** know what to convert into. What we need is the function
	    ** that will convert typea to typeb - usually this will be
	    ** typeb:=typeb(typea). We would usually get this with the
	    ** adi_ficoerce function but this only gives us the implicit
	    ** form and we could do with the named form. */
	    if (oprd == 0)
		dt = abs(qnode->pst_left->pst_sym.pst_dataval.db_datatype);
	    else if (qnode->pst_right &&
		qnode->pst_right->pst_sym.pst_type == PST_OPERAND)
	    {
		/* We have a MOP context */
		PST_QNODE *p = qnode;
		i4 i;
		for (i = 0; i < oprd; i++)
		    p = p->pst_right;
		dt = abs(p->pst_left->pst_sym.pst_dataval.db_datatype);
	    }
	    else
		dt = abs(qnode->pst_right->pst_sym.pst_dataval.db_datatype);
	    if (dt == fdesc->adi_dt[oprd])
		/* Datatype actually match - accept no coercion needed */
		coerce_fiid = ADI_NILCOERCE;
	    {
		DB_DATA_VALUE tmp;
		ADI_OP_ID opid = ADI_NOOP;
		i4 par;
		tmp.db_data = NULL;
		tmp.db_length = sizeof(i8);
		tmp.db_datatype = fdesc->adi_dt[oprd];
		tmp.db_prec = 0;
		if (adi_castid(&x100cb->adfcb, &tmp, &opid, &par))
		{
		    ADI_RSLV_BLK adi_rslv_blk;
		    adi_rslv_blk.adi_op_id = opid;
		    adi_rslv_blk.adi_num_dts = 1;
		    adi_rslv_blk.adi_dt[0] = fdesc->adi_dt[oprd];
		    if (!adi_resolve(&x100cb->adfcb, &adi_rslv_blk, FALSE))
			coerce_fiid = adi_rslv_blk.adi_fidesc->adi_finstid;
		}
	    }
	}
    }
    return coerce_fiid;
}

static const char *
opc_x100_copy(
	const char *src,
	u_i4 srclen,
	char *dest,
	u_i4 destsize)
{
    register u_i4 i = destsize;
    if (srclen >= i)
	srclen = i - 1;
    else
	i = srclen;
    MEcopy(src, srclen, dest);
    while (i && dest[i - 1] == ' ')
	i--;
    dest[i] = EOS;
    return dest;
}


