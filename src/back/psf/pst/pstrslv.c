/* 
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <st.h>
#include    <tr.h>
#include    <qu.h>
#include    <bt.h>
#include    <ex.h>
#include    <me.h>
#include    <adf.h>
#include    <adfops.h>
#include    <adudate.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <usererror.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSTRSLV.C - Functions used to resolve a query tree.
**
**  Description:
**      This file contains the functions necessary to resolve a query tree
**	operator node.  This means setting the conversions and the result
**	types and lengths for the node, as well as figuring out which ADF
**	function should be called.
**
**          pst_resolve - Resolve a query tree operator node
**	    pst_union_resolve - Resolve unions
**	    pst_caserslt_resolve - Resolve result expressions of case funcs
**	    pst_caseunion_resolve - Resolve source/comparand exprs of case/union
**	    pst_parm_resolve - Resolve local variables/parms
**	    pst_convlit - Convert literals to compatible type if possible
**
**
**  History:
**      03-oct-85 (jeff)    
**          written
**      13-sep_86 (seputis)
**          added initialization of fidesc ptr
**	4-mar-87 (daved)
**	    added resolution for union nodes
**	23-oct-87 (stec)
**	    General cleanup and fixes to pst_union_resolve.
**	1-feb-88 (stec)
**	    Gene changed data type precedence hierarchy.
**	18-feb-88 (thurston)
**	    Replaced the ty_better() static routine and its uses in pst_opcomp()
**	    and pst_uncomp() with the ty_rank() static routine.  This solves a
**	    bug, and is simpler code.
**	08-jun-88 (stec)
**	    Added pst_parm_resolve procedure.
**	03-nov-88 (stec)
**	    Correct bug found by lint.
**	18-jun-89 (jrb)
**	    Major changes.  Algorithm for doing resolution is now in ADF.  This
**	    routine no longer takes any knowledge about what a datatype looks
**	    like internally nor does it make any assumptions about how a
**	    datatype is resolved.  Removed pst_opcomp, pst_uncomp, and ty_rank
**	    functions.  This file is about half of its original size.
**	12-jul-89 (jrb)
**	    Made E_AD2061_BAD_OP_COUNT map to a user error rather than letting
**	    PSF treat it as an internal error.
**	15-dec-89 (jrb)
**	    Added checks for DB_ALL_TYPE in function instance descriptors (for
**	    outer join project).
**	24-oct-90 (andre)
**	    remove declarations for ty_rank(), pst_opcomp(), pst_uncomp() which
**	    have been previously removed.  Apparently, some compilers do not dig
**	    static declarations for non-existent functions.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	18-feb-93 (walt)
**		Removed (the now incorrect) prototypes for adi_fitab, adi_tycoerce,
**		and adi_tyname now that ADF is prototyped in <adf.h>.
**	23-mar-93 (smc)
**	    Cast parameter of BTtest() to match prototype.
**	29-jun-93 (andre)
**	    #included TR.H
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-mar-94 (andre)
**	    extracted code from pst_union_resolve() into
**	    pst_get_union_resdom_type()
**      1-feb-95 (inkdo01)
**          special case code to allow long byte/varchar union compatibility
**	6-feb-95 (hanch04)
**	    fixed syntax error caused by comment
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-Jan-2003 (hanch04)
**          Back out last change buy zhahu02.  Not compatible with new code.
**	17-jul-2003 (toumi01)
**	    Add to PSF some flattening decisions.
**	31-aug-2004 (sheco02)
**	    X-integrate change 466442 to main.
**	31-aug-2004 (sheco02)
**	    Fixed previous cross change mistake.
**	02-Sep-2004 (bonro01)
**	    Fixed x-integration of change 466442
**	3-Nov-2006 (kschendel)
**	    Remove unused case-source resolver.  Combine parts of case
**	    and union resolving since they do the same thing.
**	    Recognize a standalone NULL when case or union resolving and
**	    just adjust the type (e.g. don't max out decimal prec/scale!).
**	16-Jun-2009 (thich01)
**	    Treat GEOM type the same as LBYTE.
**	20-Aug-2009 (thich01)
**	    Treat all spatial types the same as LBYTE.
**	21-Oct-2010 (kiria01) b124629
**	    Use the macro symbol with ult_check_macro instead of literal.
**	26-Sep-2010 (hweho01)
**	    Add me.h to resolve the reference to MEcopy on Windows.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**	08-Dec-2010 (kiria01) SIR 124690
**	    Moved all the collation ID resolution into the resolvers.
**	    pst_apply_collation_coercion() handles the logic to work out
**	    what needs doing like the special relationship between Unicode
**	    and its case insensitive form. Added pst_mk_similar() to aid
**	    creating coercion nodes. Similar to psl_mk_const_similar but
**	    able to workout the destination size if need be. Also made
**	    adf_scb a parameter instead of deriving from sess_cb as latter
**	    not always present.
**	09-Dec-2010 (kiria01) SIR 124690
**	    Corrected DLL linkages for last change.
**	30-Jan-2011 (kiria01) SIR 124690
**	    Further tightening if collation ID resolution. Restrict
**	    from being set on non-collating types.
**	25-Mar-2011 (kiria01) b125223
**	    Drop adi_fidesc() call & use extended adi_ficoerce_full() that
**	    also addresses dt_family processing which was being missed.
**	08-Jun-2011 (kiria01) b125435
**	    Chain OPERANDS on right hand side.
**       4-Jul-2011 (hanal04) SIR 125458
**          Add DB_ERROR support (__FILE__, __LINE__) to PSF and to user
**          error functions in SCF.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**	19-Aug-2011 (kiria01) b125530
**	    Adjust for adi_castid interface change.
**	17-Oct-2011 (kiria01) b125852
**	    Allow resolver to probe string literals in date contexts
**	    to determine the datatype. Dropped tests for DB_C_CLASS
**	    as there is no practical need to distiguish it from
**	    the DB_CHAR_CLASS & DB_NCHAR_CLASS.
**/

/* TABLE OF CONTENTS */
i4 pst_resolve(
	PSS_SESBLK *sess_cb,
	ADF_CB *adf_scb,
	register PST_QNODE **opnode,
	DB_LANG lang,
	DB_ERROR *error);
static i4 containment_handler(
	EX_ARGS *exargs);
i4 pst_union_resolve(
	PSS_SESBLK *sess_cb,
	ADF_CB *adf_scb,
	PST_QNODE *rootnode,
	DB_ERROR *error);
static i4 pst_caserslt_resolve(
	PSS_SESBLK *sess_cb,
	ADF_CB *adf_scb,
	PST_QNODE *casep,
	DB_ERROR *error);
static i4 pst_caseunion_resolve(
	PSS_SESBLK *sess_cb,
	ADF_CB *adf_scb,
	DB_DATA_VALUE *dv1,
	DB_DATA_VALUE *dv2,
	bool forcenull,
	DB_ERROR *error);
static i4 pst_get_union_resdom_type(
	DB_DT_ID dt,
	DB_DATA_VALUE *resval,
	ADF_CB *adf_scb,
	bool nullable,
	DB_ERROR *error);
i4 pst_parm_resolve(
	PSS_SESBLK *sess_cb,
	PSQ_CB *psq_cb,
	PST_QNODE *resdom);
static i4 pst_handler(
	EX_ARGS *exargs);
bool pst_convlit(
	PSS_SESBLK *sess_cb,
	PSF_MSTREAM *stream,
	DB_DATA_VALUE *targdv,
	PST_QNODE *srcenode);
static DB_STATUS pst_apply_collation_coercion (
	PSS_SESBLK *sess_cb,
	ADF_CB *adf_scb,
	DB_DATA_VALUE *dv1,
	bool need_coercion,
	DB_DATA_VALUE *dvn,
	PST_QNODE **target,
	DB_ERROR *error);
DB_STATUS pst_mk_similar(
	PSS_SESBLK *sess_cb,
	ADF_CB *adf_scb,
	PSF_MSTREAM *mstream,
	DB_DATA_VALUE *templ,
	PST_QNODE **nodep,
	DB_ERROR *err_blk);
bool pst_is_literal_int(
	PST_QNODE *node,
	i8 val);
i4 pst_augment_decimal_cast(
	PSS_SESBLK *sess_cb,
	PSF_MSTREAM *mstream,
	PST_QNODE *node,
	DB_ERROR *err_blk);
i4 pst_resolve_err(
	PSS_SESBLK *cb,
	PST_QNODE *node,
	DB_ERROR *err_blk,
	i4 *flags);

/*
** Counters for constant folding stats - see psfmo.c
*/
i4 Psf_fold = TRUE;
#ifdef PSF_FOLD_DEBUG
i4 Psf_nops = 0;
i4 Psf_nfolded = 0;
i4 Psf_ncoerce = 0;
i4 Psf_nfoldcoerce = 0;
#endif

/* Psf_dt_class - class lookup array keyed on datatype
** defined for PSF_DT_CLASS */
#define _DEFINEEND
#define _DEFINE(ty, v, dc, sc, q, qv, s, sv) DB_##dc##_CLASS,
GLOBALCONSTDEF i4 Psf_dt_class[] = {DB_DT_ID_MACRO 0};
#undef _DEFINE
#undef _DEFINEEND


/*{
** Name: pst_resolve	- Resolve a query tree operator node
**
** Description:
**      The pst_resolve function resolves a query tree operator node.  This
**	means that it sets result type and length for the node.  It also
**	figures out which ADF function should be called for the node.  This
**	operation should be called whenever a new operator node	is constructed. 
**	It will not be necessary to re-resolve an operator node should it get
**	new children; however, one should be sure that the function that the
**	operator represents can take the new children as arguments (it might
**	be impossible to convert the children to the desired types before
**	performing the function).  Any query tree that comes from the parser
**	is guaranteed to have already been resolved.
**
**	This function will work on the following node types.  They MUST have the
**	children required for the particular node type:
**
**	    Node Type	    Left Child	    Right Child
**
**	    PST_UOP	    Yes		    No
**	    PST_BOP	    Yes		    Yes
**	    PST_AOP	    Yes		    No
**	    PST_COP	    No		    No
**	    PST_MOP         Yes             Yes
**
**	NOTE: This function does not set the conversion ids for an operator
**	node.  That it because the operator node could receive new children
**	of different types.  The conversions must be set before the expression
**	is used.
**
**	NOTE: Some operators (cast, for example) set the result type by their
**	very definition in the parser. If the result type determined here is
**	the same as the datatype in the operator node, it is assumed that the 
**	parser already set the data type for the operator and it is retained.
**
** Inputs:
**	sess_cb				session control block. MAY BE NULL.
**	adf_scb				adf session control block
**      opnode				The operator node to resolve
**	lang				query language
**      error                           Error block
**
** Outputs:
**      opnode				The resolved operator node
**	error				Standard error block
**	    .err_code			    What error occurred
**		E_PS0000_OK			Success
**		E_PS0002_INTERNAL_ERROR		Internal inconsistency in PSF
**		2908L				No function found by this name
**						that can take these arguments.
**		2910L				Ambiguous function or operator
**		E_PS0C02_NULL_PTR		opnode = NULL
**		E_PS0C03_BAD_NODE_TYPE		opnode is not an operator node
**		E_PS0C04_BAD_TREE		Something is wrong with the
**						query tree.
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-oct-85 (jeff)
**          written
**      13-sep-86 (seputis)
**          added ptr to ADI_FI_DESC to node to be resolved
**	14-may-87 (stec)
**	    added code to handle COUNT(*) case in SQL (no children).
**	09-jul-87 (stec)
**	    fixed bug resulting in computation of bad result length.
**	12-aug-87 (thurston)
**	    fixed bug that didn't calculate the result length properly for
**	    aggregates that have the nullability of the result different than
**	    that of the input, AND did not need to coerce the input.
**	18-jun-89 (jrb)
**	    Changed this routine to use ADF to do datatype resolution.  Also,
**	    fixed a bug where the function instance id's for the coercions in an
**	    operator node where being set incorrectly (they're not currently
**	    being used anyway).
**	12-jul-89 (jrb)
**	    Made E_AD2061_BAD_OP_COUNT map to a user error rather than letting
**	    PSF treat it as an internal error.
**	18-jul-89 (jrb)
**	    Set precision field for all db_data_values contained in here.
**	25-jul-89 (jrb)
**	    Made sure db_data field was filled in for db_data_value's used in
**	    calclen calls.
**	15-dec-89 (jrb)
**	    Added checks for DB_ALL_TYPE in function instance descriptors.
**	10-aug-93 (Andre)
**	    removed declaration of scf_call()
**	09-Mar-1999 (shero03)
**	    support MOP nodes.
**	7-sep-99 (inkdo01)
**	    Added calls to pst_caserslt_resolve/pst_casesrce_resolve for case 
**	    function resolution.
**	4-july-01 (inkdo01)
**	    Fix error message for wrong number of function parms.
**      09-dec-2005 (huazh01)
**          if the aggregate is under a PST_BOP node, do not 
**          flatten such query too. 
**          b115593, INGSRV3530.
**	16-jul-03 (hayke02)
**	    Call adi_resolve() with varchar_precedence TRUE if either ps202
**	    (PSS_VARCHAR_PRECEDENCE) is set or psf_vch_prec is ON. This change
**	    fixes bug 109132.
**	22-dec-04 (inkdo01)
**	    Add logic to resolve binop's involving explicit collations.
**	2-feb-06 (dougi)
**	    Make cast functions in select list work.
**	8-mar-06 (dougi)
**	    Slight adjustment to avoid problems from ii_iftrue().
**      17-apr-2006 (huazh01)
**          some nodes in a dynamic SQL query might not be resolved in
**          pst_node(), which will leave the node's dbtype as 0. If
**          the dbtype of a child node does not seem to be valid,
**          try to resolve such child node before we continue our work
**          with the invalid dbtype.
**          bug 115963.
**	25-Apr-2006 (kschendel)
**	    Fix to not check args to varargs fns that are in the optional
**	    part of the param list (>= fi's numargs).
**	    Fix old bug in MOP checking, followed wrong link.
**	05-sep-2006 (gupsh01)
**	    Added call to adi_dtfamily_resolve(), if function instance 
**	    selected is generic to the datatype family of the operands.
**	25-oct-2006 (dougi)
**	    Check for null sess_cb before dereferencing it.
**	03-Nov-2007 (kiria01) b119410
**	    When determining the length of parameters ensure that the
**	    datatype is correct if a coercion has been applied.
**	13-Apr-2008 (kiria01) b119410
**	    Apply string-numeric compares using a normalise operator
**	    that we add to the tree at the appropriate place.
**	10-Jul-2008 (kiria01) b119410
**	    Drive the numericnorm insertion based on the NUMERIC
**	    compares instead of BYTE-BYTE as we retain ADF interface
**	    boundaries better as only the dbms (parser) needs the fixup.
**	25-Jun-2008 (kiria01) SIR120473
**	    Move pst_isescape into new u_i2 pst_pat_flags.
**	22-Sep-2008 (kiria01) b120926
**	    Extend numericnorm insertion to detect compressed '=' tree
**	    sometimes used to build IN value lists.
**	12-Oct-2008 (kiria01) SIR121012
**	    Detect and pre-evaluate constant actions applying them back
**	    into the parse tree. This is applied on two levels - coercion
**	    operators on constant operands and on constant functions
**	    of constant operands. The former are very frequent where
**	    often a varchar constant is used to initialise another
**	    datatype.
**	15-Oct-2008 (kiria01) b121061
**	    Increase the number of parameters supported in the calls
**	    to adf_func using the more general vector form.
**	20-Oct-2008 (kiria01) b121098
**	    Properly establish the pst_oplcnvrtid & pst_oprcnvrtid
**	    fields to reliably indicate whether an operand requires
**	    a coercion and if so which one. Well, almost - see comment
**	    below regarding PST_OPERAND and lack of its pst_value.
**	12-Oct-2008 (kiria01) SIR121012
**	    Spread the net wider to catch more constant coercions.
**	    This involves the introduction of ADI_F16384_CHKVARFI and
**	    state information from ADF - .adf_const_const
**	    Also draw in the constants whose matching datatypes would
**	    have excluded them from folding - these could still have
**	    length or precision changes that need applying. This might
**	    look like it will add extra un-needed coercions but we do
**	    this just to literal constants and then these will fold
**	    away.
**	15-Nov-2008 (kiria01) SIR120473
**	    Explicitly handle nil-coercions in the constant folding
**	    where a datatype has minimal support.
**	31-Dec-2008 (kiria01) SIR121012
**	    Leave IN value list constants intact and don't attempt to
**	    fold them.
**	09-Feb-2009 (kiria01) b121546
**	    Propagate the resolved type of the injected numericnorm
**	    node through subselect nodes.
**	06-Mar-2009 (kiria01) b119410
**	    Support efficient folding of string to numeric if string
**	    constant expression offered. This will now allow for key
**	    support in these situations which apparently happen more
**	    often than expected.
**	11-Mar-2009 (kiria01) b121781
**	    Calculate prec & scale instead of using the numeric side
**	    values from a string numeric compare. Otherwise we will
**	    likely loose precision with decimal.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	21-May-2009 (kiria01) b122051
**	    Tidy collID handling
**	24-Aug-2009 (kiria01) b122522
**	    Sanity check before allowing string to int coercion, there
**	    might be a decimal point in the string that default coercion
**	    would be relaxed about.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
**	    Update the parse tree node indication a coercion is no longer
**	    required due to folding.
**	11-Nov-2009 (kiria01) SIR 121883
**	    With PST_INLIST, check for PST_CONST on RHS as the flag
**	    may be applied to PST_SUBSEL to aid servers to distinguish
**	    simple = from IN and <> from NOT IN.
**	27-Nov-2009 (kiria01) b122966
**	    Fully ringfence exceptions in adf_func as they will be
**	    re-raised in context later.
**	08-Feb-2010 (kiria01)
**	    Support functions that in X100 will be constants and can be folded
**	    using the AD_CNST_FOR_X100 flag.
**	10-feb-2010 (dougi)
**	    (Possibly) temporary fix to generate i8 result for count's in
**	    X100.
**	02-May-2010: (kiria01) b123672
**	    Split length check from datatype to avoid the inevitable but
**	    overlooked clash between BYTE(64) and I8 etc!
**      19-May-2010 (hanal04) Bug 123764
**          IFNULL aggregate handling was taking place without checking for
**          and earlier failure in adi_resolve(). This lead to a SIGSEGV
**          instead of the expected error report. Move the IFNULL aggregate
**          down below the error check.
**      7-jun-2010 (stephenb)
**          correct error handling for bad return of adi_dtfamilty_resolve,
**          it may produce various type and operand errors that need to be 
**          returned as user errors. (Bug 123884)
**	14-Jul-2010 (kschendel) b123104
**	    Don't fold true/false generating operators back to constants.
**	3-Aug-2010 (kschendel) b124170
**	    Use joinid from node being resolved when inserting a coercion.
**	4-aug-2010 (stephenb)
**	    ADF calls may also return E_AD2009_NOCOERCION when no coercion
**	    is available, treat as user error. Also fix error handling
**	    for other ADF calls. (Bug 124209)
**	12-Aug-2010 (kschendel) b124234
**	    Don't attempt to constant-fold LONG data.  Even if it works,
**	    the resulting value described by the coupon is unlikely to
**	    last until execution time.
**	09-Dec-2010 (kiria01) SIR 124690
**	    Correct the len calculation in s-n when nullability was
**	    disregarded.
**	16-june-2011 (dougi) m2081
**	    Ignore PST_WINDOW nodes to aid in error detection.
**	16-Jun-2011 (kiria01) b125450
**	    Simplified exception containment.
**	05-Jul-2011 (kiria01) b125511
**	    Allow for folding of <expr> * 1, <expr> / 1, <expr> + 0 and
**	    <expr> - 0. This is performed after resolution so that the
**	    datatype is properly adjusted.
**	15-Jul-2011 (kiria01) b125530
**	    Allocate an unpdatable copy of ADI_FI_DESC structure if FI
**	    has ADI_F1048576_MUTABLE_RETN set so that it might be updated
**	    by a later coercion. Such later coercions applied may be used
**	    to simply effect an altered return type from a function.
**	29-Jul-2011 (kiria01) b125613
**	    Updated pst_resolve so that constant folding can return whole
**	    new nodes if need be instead of relying on passing result back
**	    in-situ.
**	20-Oct-2011 (kiria01) b125878
**	    Allow more type resolution even during prepares. If undefined
**	    types are found, don't just throw an error. Instead allow these
**	    to properly remain for later resolution. Additionally, handle
**	    the case where a cast is applied to parameters and make that
**	    type live.
*/
DB_STATUS
pst_resolve(
	PSS_SESBLK	    *sess_cb,
	ADF_CB		    *adf_scb,
	register PST_QNODE  **opnodep,
	DB_LANG 	    lang,
	DB_ERROR	    *error)
{

    i4			dtbits;
    i4			i;
    DB_STATUS		status;
    DB_DATA_VALUE	ops[ADI_MAX_OPERANDS];
    DB_DATA_VALUE	*pop[ADI_MAX_OPERANDS];
    DB_DATA_VALUE	tempop1;
    ADI_FI_DESC		*best_fidesc;
    ADI_FI_DESC		*base_fidesc;
    ADI_RSLV_BLK	adi_rslv_blk;
    ADI_FI_ID		*pcnvrtid;
    PST_QNODE	        **oprnds[ADI_MAX_OPERANDS];
    i1			const_cand = Psf_fold/*TRUE*/;
    PST_PMSPEC		pmspec = PST_PMNOTUSED;
    DB_COLL_ID		collID = DB_UNSET_COLL;
    bool		vch_prec = (Psf_srvblk->psf_vch_prec ||
			    (sess_cb && ult_check_macro(&sess_cb->pss_trace,
					PSS_VARCHAR_PRECEDENCE, NULL, NULL)));
    PST_QNODE		*opnode = *opnodep;
    bool		prepare_best_try = FALSE;
#ifdef    xDEBUG
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[1];
    PTR			scf_scb;
#endif

    /* Make sure there is a node to resolve */
    if (opnode == (PST_QNODE *) NULL)
    {
	error->err_code = E_PS0C02_NULL_PTR;
	return (E_DB_SEVERE);
    }

    /* set up to call ADF to resolve this node */
    adi_rslv_blk.adi_op_id = opnode->pst_sym.pst_value.pst_s_op.pst_opno;
    adi_rslv_blk.adi_num_dts = 0;
    tempop1 = opnode->pst_sym.pst_dataval;

    switch (opnode->pst_sym.pst_type)
    {
    case PST_CASEOP:
	/* If it's a "case" function, call the special case resolution functions. */
	return pst_caserslt_resolve(sess_cb, adf_scb, opnode, error);

    case PST_BOP:
	if (!opnode->pst_left)
	{
             error->err_code = E_PS0C04_BAD_TREE;
	     return (E_DB_SEVERE);
	}
	/*FALLTHROUGH*/
    case PST_UOP:
	if (!opnode->pst_left)
	{
             error->err_code = E_PS0C04_BAD_TREE;
	     return (E_DB_SEVERE);
	}
	/*FALLTHROUGH*/
    case PST_AOP:
	if (opnode->pst_left)
	{
	    oprnds[adi_rslv_blk.adi_num_dts++] = &opnode->pst_left;
	    if (opnode->pst_right &&
			opnode->pst_right->pst_sym.pst_type != PST_WINDOW)
		oprnds[adi_rslv_blk.adi_num_dts++] = &opnode->pst_right;
	}
	/*FALLTHROUGH*/
    case PST_COP:
	break;
    case PST_MOP:
	{
	    PST_QNODE *opr = opnode;
	    while (opr)
	    {
		oprnds[adi_rslv_blk.adi_num_dts++] = &opr->pst_left;
		opr = opr->pst_right;
	    }
	}
	break;
    default:
	error->err_code = E_PS0C03_BAD_NODE_TYPE;
	return (E_DB_SEVERE);
    }

    for (i = 0; i < adi_rslv_blk.adi_num_dts; i++)
    {	
	PST_QNODE *opr = *oprnds[i];
	adi_rslv_blk.adi_dt[i] = opr->pst_sym.pst_dataval.db_datatype;
        /* b115963 */
        if (adi_rslv_blk.adi_dt[i] == DB_NODT) 
        {
	    if (opr->pst_sym.pst_type != PST_CONST &&
		(status = pst_resolve(sess_cb, adf_scb, oprnds[i], lang, error)))
		return E_DB_SEVERE;
	    opr = *oprnds[i];
	    if ((adi_rslv_blk.adi_dt[i] = opr->pst_sym.pst_dataval.db_datatype) == DB_NODT)
	    {
		/* Still unresolved.
		** If in a prepare context with active param marks then this is
		** acceptible and we can just return success. */
		if (sess_cb->pss_defqry == PSQ_PREPARE &&
			sess_cb->pss_highparm != -1)
		{
		    /* If we find that the operator happens to be an explicit
		    ** coercion or cast then we CAN determine the type and
		    ** may ignore the unknown datatype as irrelevant from the
		    ** parsers point of view but we don't know the operator
		    ** type yet. */
		    prepare_best_try = TRUE;
		    adi_rslv_blk.adi_dt[i] = DB_LTXT_TYPE;
		}
		else
		    goto common_error_return;
	    }
        }
	if (!prepare_best_try &&
	    !DB_COLL_UNSET(opr->pst_sym.pst_dataval))
	{
	    /* Scrape up collID from params if present.
	    ** Once the operator node has been resolved we should know if any
	    ** implicit coercions will need to become explict to enable the
	    ** collation to be applied. However, we will know at this point if
	    ** there is a mismatch apparent. We complain if any two operands
	    ** have collations that differ - except for UNICODE_CASEINSENSITIVE
	    ** with UNICODE where we upgrade UNICODE.
	    ** Soon we will loop through the operands again setting collID
	    ** in place. */
	    if (collID == DB_UNSET_COLL ||
		collID == DB_NOCOLLATION ||
		collID == DB_UNICODE_COLL &&
		    opr->pst_sym.pst_dataval.db_collID == DB_UNICODE_CASEINSENSITIVE_COLL)
	    {
		collID = opr->pst_sym.pst_dataval.db_collID;
	    }
	    else if (collID != opr->pst_sym.pst_dataval.db_collID &&
		    (collID != DB_UNICODE_CASEINSENSITIVE_COLL ||
		    opr->pst_sym.pst_dataval.db_collID != DB_UNICODE_COLL))
	    {
		i4 err_code;
		psf_error(6427L, 0L, PSF_USERERR, &err_code, error, 0);
		return E_DB_ERROR;
	    }
	}
    }

    if (status = adi_resolve(adf_scb, &adi_rslv_blk, vch_prec))
    {
	goto common_error_return;
    }
    else if (prepare_best_try &&
	((~adi_rslv_blk.adi_fidesc->adi_fiflags & ADI_F524288_EXPL_COERCION) ||
	    adi_rslv_blk.adi_fidesc->adi_lenspec.adi_lncompute != ADI_FIXED &&
	    adi_rslv_blk.adi_fidesc->adi_lenspec.adi_lncompute != ADI_LEN_INDIRECT))
    {
	/* We are in a prepare statement and there is nothing further that we can
	** do yet with this operator until the parameter is bound so just return
	** as though everything is fine for now.
	** (Explicit coercions on the other hand can have their type determined.) */
	return E_DB_OK;
    }

    /* Untyped date constant identification by inspection.
    ** Don't attempt this if the operator uses IFNULL resolution or is Quel */
    if ((~adi_rslv_blk.adi_fidesc->adi_fiflags & ADI_F32768_IFNULL) && lang != DB_QUEL) 
    for (i = 0; i < adi_rslv_blk.adi_num_dts; i++)
    {
	PST_QNODE *opr = *oprnds[i];
	bool is_cast = (i == 0 && /* First param of an explicit cast to DATE */
		abs(adi_rslv_blk.adi_fidesc->adi_dtresult) == DB_DTE_TYPE &&
		(adi_rslv_blk.adi_fidesc->adi_fiflags & ADI_F524288_EXPL_COERCION));
	bool is_ingresdate = adi_rslv_blk.adi_op_id == ADI_INGRESDATE_OP;

	if ((is_cast || /* OR expecting a 'date' parameter */
		abs(adi_rslv_blk.adi_fidesc->adi_dt[i]) == DB_DTE_TYPE)
	    && /* AND we have a string constant */
		opr->pst_sym.pst_type == PST_CONST &&
		abs(adi_rslv_blk.adi_dt[i]) < DB_MAX_TYPE &&
		(Psf_dt_class[abs(adi_rslv_blk.adi_dt[i])] == DB_CHAR_CLASS ||
		 Psf_dt_class[abs(adi_rslv_blk.adi_dt[i])] == DB_NCHAR_CLASS))
	{
	    /* If it's an accessible literal */
	    if (opr->pst_sym.pst_dataval.db_data &&
		opr->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER)
	    {
		AD_DTUNION dt;
		bool pure_ansi = TRUE;

		if ((adf_scb->adf_date_type_alias & AD_DATE_TYPE_ALIAS_ANSI) == 0 ||
			is_ingresdate)
		    pure_ansi = FALSE;

		/* Inspect string for type */
		ops[i].db_data = (PTR)&dt;
		if (is_ingresdate)
		{
		    ops[i].db_length = ADF_DTE_LEN;
		    ops[i].db_datatype = DB_DTE_TYPE;
		}
		else
		{
		    ops[i].db_length = sizeof(dt);
		    ops[i].db_datatype = DB_ALL_TYPE;
		}
		ops[i].db_prec = 0;
		ops[i].db_collID = DB_NOCOLLATION;
		if (!ADF_ISNULL_MACRO(&opr->pst_sym.pst_dataval) &&
		    (status = adu_0parsedate(adf_scb, &opr->pst_sym.pst_dataval,
					    &ops[i], pure_ansi, !is_ingresdate)))
		{
		    adf_scb->adf_errcb.ad_errclass = ADF_USER_ERROR;
		    psf_adf_error(&adf_scb->adf_errcb, error, sess_cb);
		    return status;
		}
		else if (ops[i].db_datatype == DB_ALL_TYPE)
		{
		    /* The text is ambiguous so leave as a string & allow
		    ** external cast of coercion to deal with it. */
		    ops[i] = opr->pst_sym.pst_dataval;
		}
		else
		{
		    PST_QNODE *node;
		    PST_CNST_NODE cconst;
		    cconst.pst_tparmtype = PST_USER;
		    cconst.pst_parm_no = 0;
		    cconst.pst_pmspec  = PST_PMNOTUSED;
		    cconst.pst_cqlang = DB_SQL;
		    cconst.pst_origtxt = NULL;
		    /* If the date parse works, we will have established the datatype
		    ** by inspection. If it didn't - we will continue as per usual
		    ** and create an ingresdate.
		    ** If this is a DATE cast, we will be able to replace the whole
		    ** operator with the result. Otherwise, we just replace the parameter.
		    ** Either way we need to get the parsed date into a PST_CONST. */
		    if (E_DB_OK == pst_node(sess_cb, &sess_cb->pss_ostream, NULL, NULL,
				    PST_CONST, (char *)&cconst, sizeof(cconst),
				    ops[i].db_datatype, ops[i].db_prec, ops[i].db_length,
				    (DB_ANYTYPE *)&dt,
				    &node, error, 0))
		    {
			if (is_cast)
			{
			    /* Replace DATE(...) with the typed constant. */
			    *opnodep = node;
			    return E_DB_OK;
			}
			/* Othewise just a parameter - replace it and update adi setup
			** to recall adi_resolve seeing as this might affect resolver.
			** NOTE: All this is happening after we know that there is
			** a potential operator to resolve to but before any dt family
			** processing is performed; Although this extra resolve is in
			** the parameter loop, it will usually only be called once as
			** there are presently no date functions that could initially
			** be resolved to that could be passed more than one string
			** parameter. Compares won't trigger as at least one parameter
			** has to be a date to start with to have a hope of getting here. */
			*oprnds[i] = node;
			adi_rslv_blk.adi_dt[i] = ops[i].db_datatype;
			if (status = adi_resolve(adf_scb, &adi_rslv_blk, vch_prec))
			{
			    goto common_error_return;
			}
		    }
		}
	    }
	    else if (i == 1 && adi_rslv_blk.adi_dt[0] == DB_DTE_TYPE)
	    {
		/* If this is <date>+/-<vch> then as we have hijacked the FI for
		** that combination, dbproc symbols, embedded params etc. might have
		** otherwised matched to the now cloaked definitions. We need to restore
		** these as we cannot otherwise fix these types up as the data is not
		** available and the best we can then do is a runtime conversion as before
		** treating a ingresdate. */
		if (adi_rslv_blk.adi_op_id == ADI_ADD_OP)
		{
		    if (status = adi_fidesc(adf_scb, 1396/*ADFI_1396_DATE_ADD_VCH*/,
					&adi_rslv_blk.adi_fidesc))
			return status;
		    break;
		}
		else if (adi_rslv_blk.adi_op_id == ADI_SUB_OP)
		{
		    if (status = adi_fidesc(adf_scb, 1395/*ADFI_1395_DATE_SUB_VCH*/,
					&adi_rslv_blk.adi_fidesc))
			return status;
		    break;
		}
	    }
	}
    }

    if ((adi_rslv_blk.adi_op_id == ADI_CNTAL_OP ||
		adi_rslv_blk.adi_op_id == ADI_CNT_OP) &&
		sess_cb && (sess_cb->pss_stmt_flags2 & PSS_2_X100TAB_INQ))
    {
	/* "count()" in X100 queries is temporarily changed to produce i8. */
	if (status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
			sizeof(ADI_FI_DESC), &best_fidesc, error))
	    return (status);
	*best_fidesc = *adi_rslv_blk.adi_fidesc;
	best_fidesc->adi_lenspec.adi_fixedsize = 8;
	adi_rslv_blk.adi_fidesc = best_fidesc;
    }

    /*
    ** for flattening we care if we have one or multiple ifnulls with
    ** aggregates, so use two flags as a two-bit counter indicating that
    ** we have none, one, or more than one.
    ** FIXME: This is probably the wrong place to do this as this is just
    ** one node affecting the whole parse tree that could consist of multiple
    ** independant aggregates. This needs further research.
    */
    if (sess_cb && adi_rslv_blk.adi_op_id == ADI_IFNUL_OP)
    {
	if ((opnode->pst_left->pst_sym.pst_type == PST_AGHEAD) ||
             /* b115593, INGSRV3530 
             ** do not flatten the query if the PST_AGHEAD is under
             ** a PST_BOP node.
             */
             (
              (opnode->pst_left->pst_sym.pst_type == PST_BOP) &&
              (
                 (opnode->pst_left->pst_left->pst_sym.pst_type ==
                   PST_AGHEAD) 
                 ||
                 (opnode->pst_left->pst_right->pst_sym.pst_type ==
                   PST_AGHEAD)
              )
             )
           )
	{
	    if (sess_cb->pss_flattening_flags & PSS_IFNULL_AGHEAD)
                sess_cb->pss_flattening_flags |= PSS_IFNULL_AGHEAD_MULTI;
	    sess_cb->pss_flattening_flags |= PSS_IFNULL_AGHEAD;
	}
    }

    /* We now have the "best" fi descriptor - can it be tuned? */
    if (best_fidesc = base_fidesc = adi_rslv_blk.adi_fidesc)
    {
	i2 need_fmly_resolve = 0;
	bool is_mutable = FALSE;

	if (base_fidesc->adi_fiflags & ADI_F1048576_MUTABLE_RETN)
	    is_mutable = TRUE;

	if (status = adi_need_dtfamily_resolve(adf_scb, base_fidesc, 
					    &adi_rslv_blk, 
					    &need_fmly_resolve))
	{
	    error->err_code = E_PS0C05_BAD_ADF_STATUS;
	    return (E_DB_SEVERE);
	}

	if (sess_cb && (need_fmly_resolve == 1 || is_mutable)) 
	{
	    if (status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		    sizeof(ADI_FI_DESC), &best_fidesc, error))
		return status;
 	    if (best_fidesc == NULL)
	    {
    		error->err_code = E_PS0C05_BAD_ADF_STATUS;
		return E_DB_SEVERE;
	    }

	    if (need_fmly_resolve != 1)
	    {
		/* Simply make copy so that it can be updated */
		*best_fidesc = *base_fidesc;
	    }
	    else if (status = adi_dtfamily_resolve(adf_scb, base_fidesc, 
					  best_fidesc, &adi_rslv_blk))
	    {
		goto common_error_return;
	    }
	}
    }

    if (sess_cb &&
	lang == DB_SQL &&
	best_fidesc->adi_fitype == ADI_COMPARISON &&
	adi_rslv_blk.adi_num_dts == 2 &&
	best_fidesc->adi_dt[0] == DB_ALL_TYPE &&
	best_fidesc->adi_dt[1] == DB_ALL_TYPE &&
	(opnode->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) == 0)
    {
	ADI_RSLV_BLK tmp_adi_rslv_blk;
	bool handled = FALSE;
	/*
	** adi_resolve has spotted a string-numeric compare and
	** diverted the FI to one of the NUMERIC comparisons. While
	** this will work it will be inefficient for keyed lookup.
	** In the general case this could be a full join with arbitary
	** expressions. There is a way for this to be handled reasonably
	** but there are some cases that can be done more directly.
	** These relate to the case where we may have a constant and
	** this could be either the the LHS or RHS. If the constant is
	** a number then there is not much we can do as the string expr
	** could have all manner of spacing or justification. However,
	** if we have a string constant then we directly attempt the
	** coercion to the other side. If this works then we can fix up
	** the comparison to the actual type and drop down for constant
	** folding. In all other cases we do the general form.
	**
	** Do we have a string constant?
	*/
	{
	    PST_QNODE **s = &opnode->pst_left;
	    PST_QNODE **n = &opnode->pst_right;
	    PST_QNODE *res_node = NULL;
	    DB_DT_ID n_dt = (*n)->pst_sym.pst_dataval.db_datatype;

	    status = 0;
	    switch (abs((*s)->pst_sym.pst_dataval.db_datatype))
	    {
	    default:
		/* A none s-n - just ignore it */
		break;
	    case DB_INT_TYPE:
	    case DB_DEC_TYPE:
	    case DB_FLT_TYPE:
		/* Swap nodes & fall through */
		s = &opnode->pst_right;
		n = &opnode->pst_left;
		n_dt = (*n)->pst_sym.pst_dataval.db_datatype;
		/*FALLTHROUGH*/
	    case DB_TXT_TYPE:
	    case DB_CHR_TYPE:
	    case DB_CHA_TYPE:
	    case DB_VCH_TYPE:
	    case DB_LVCH_TYPE:
	    case DB_NCHR_TYPE:
	    case DB_NVCHR_TYPE:
	    case DB_LNVCHR_TYPE:
	    case DB_LTXT_TYPE:
		if ((*s)->pst_sym.pst_type == PST_CONST &&
		    (*s)->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER &&
		    (*s)->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0)
		{
		    PST_OP_NODE op;
		    i2 ps = (*n)->pst_sym.pst_dataval.db_prec;
		    i4 len = (*n)->pst_sym.pst_dataval.db_length;

		    if (n_dt < 0)
		    {
			len--;
			n_dt = -n_dt;
		    }
		    /* We do seem to have a string constant so
		    ** lets try to directly coerce to the datatype/size
		    ** of the other side */
		    op.pst_opmeta = PST_NOMETA;
		    op.pst_pat_flags = AD_PAT_DOESNT_APPLY;
		    op.pst_joinid = opnode->pst_sym.pst_value.pst_s_op.pst_joinid;
		    /* The darn i4 casts are because sizeof is technically
		    ** a size_t, which is 64-bit on LP64, and gcc complains. */
		    switch (n_dt)
		    {
		    case DB_INT_TYPE:
			switch (len)
			{
			case (i4)sizeof(i1):
			    op.pst_opno = ADI_I1_OP;
			    break;
			case (i4)sizeof(i2):
			    op.pst_opno = ADI_I2_OP;
			    break;
			case (i4)sizeof(i4):
			    op.pst_opno = ADI_I4_OP;
			    break;
			case (i4)sizeof(i8):
			    op.pst_opno = ADI_I8_OP;
			    break;
			}
			break;
		    case DB_FLT_TYPE:
			switch (len)
			{
			case (i4)sizeof(f4):
			    op.pst_opno = ADI_F4_OP;
			    break;
			case (i4)sizeof(f8):
			    op.pst_opno = ADI_F8_OP;
			    break;
			}
			break;
		    case DB_DEC_TYPE:
			{
			    /* Decimal is a little more awkward */
			    PST_CNST_NODE cconst;
			    cconst.pst_tparmtype = PST_USER;
			    cconst.pst_parm_no = 0;
			    cconst.pst_pmspec  = PST_PMNOTUSED;
			    cconst.pst_cqlang = DB_SQL;
			    cconst.pst_origtxt = (char *) NULL;

			    /* Find out best prec/scale to use */
			    if (status = adu_decprec(adf_scb, &(*s)->pst_sym.pst_dataval, &ps))
				break;
			    status = pst_node(sess_cb, &sess_cb->pss_ostream, (PST_QNODE *)NULL,
				(PST_QNODE *)NULL, PST_CONST, (char *)&cconst,
				sizeof(cconst), DB_INT_TYPE, (i2) 0, (i4)sizeof(ps),
				(DB_ANYTYPE *)&ps, &res_node, error, 0);
			    /* Adjust the length to match the prec */
			    len = DB_PREC_TO_LEN_MACRO(DB_P_DECODE_MACRO(ps));
			    op.pst_opno = ADI_DEC_OP;
			}
			break;
		    default:
			status = -1;
		    }
		    if (!status && n_dt == DB_INT_TYPE)
		    {
			/* Extra check for integer. Normal string coercion to int
			** will stop at the decimal point without error. We use 'is integer'
			** to see if there would be a problem & if not, we go for it. */
			i4 cmp = 0;
			if (!(status = adu_isinteger(adf_scb, &(*s)->pst_sym.pst_dataval, &cmp)) &&
				cmp == 0)
			    status = -1;
		    }
		    if (!status && (*s)->pst_sym.pst_dataval.db_datatype < 0)
		    {
			/* Retain Nullability of the string type in case NULL passed. */
			len++;
			n_dt = -n_dt;
		    }
		    if (!status &&
			!pst_node(sess_cb, &sess_cb->pss_ostream,
			*s, res_node, res_node ? PST_BOP : PST_UOP,
			    (char *)&op, sizeof(op), n_dt, ps, len,
			    (DB_ANYTYPE *)NULL, &res_node, error, PSS_JOINID_PRESET) &&
			res_node &&
			res_node->pst_sym.pst_type == PST_CONST)
		    {
			/* We sucessfully converted the string to the
			** number and now we need to redo the resolve
			** for the new datatype.
			*/
			tmp_adi_rslv_blk = adi_rslv_blk;
			tmp_adi_rslv_blk.adi_dt[0] = tmp_adi_rslv_blk.adi_dt[1] = abs(n_dt);
			if (!adi_resolve(adf_scb, &tmp_adi_rslv_blk, FALSE))
			{
			    /* Resolver happy, use the new fidesc and set the new
			    ** node into the tree */
			    adi_rslv_blk = tmp_adi_rslv_blk;
			    best_fidesc = adi_rslv_blk.adi_fidesc;
			    *s = res_node;
			    handled = TRUE;
			}
		    }
		}
	    }
	}
	/*
	** What we do is swap the NUMERIC compare for a BYTE-BYTE
	** compare and insert calls to numericnorm()
	** Note: IN list processing is presently not optimised in
	** this manner. To do so would require expanding out the
	** compacted list as the short form uses the pst_left field
	** as the constant list pointer and this would conflict with
	** more general expression code.
	** Late breaking news: IN list processing IS now optimised in
	** this manner. The parser determines the situation and adds
	** the numericnorm() calls which could be constant folded and 
	** then mass compared.
	*/
	tmp_adi_rslv_blk = adi_rslv_blk;
	tmp_adi_rslv_blk.adi_dt[0] = tmp_adi_rslv_blk.adi_dt[1] = DB_BYTE_TYPE;
	if (!handled && !adi_resolve(adf_scb, &tmp_adi_rslv_blk, FALSE))
	{
	    PST_OP_NODE op;
	    PST_QNODE **p;
	    PST_QNODE *t;
	    i = 0;
	    /* Use the BYTE-BYTE fidesc we just looked up */
	    adi_rslv_blk = tmp_adi_rslv_blk;
            best_fidesc = adi_rslv_blk.adi_fidesc;

	    op.pst_opno = ADI_NUMNORM_OP;
	    op.pst_opmeta = PST_NOMETA;
	    op.pst_pat_flags = AD_PAT_DOESNT_APPLY;
	    op.pst_joinid = opnode->pst_sym.pst_value.pst_s_op.pst_joinid;

	    /* String numeric compare spotted - slot
	    ** in normalisation functions */
	    p = &opnode->pst_left;
	    do /* once for each sub-tree */
	    {
		PST_QNODE **p2 = p;
		/* We need to find where to stick the normalisation
		** function - usually it will be:
		**	x = y
		** becomes:
		**	n(x) = n(y)
		** However, sometimes the value of interest is buried -
		**	x = (SELECT y FROM ... )
		** must become:
		**	n(x) = (SELECT n(y) FROM ... )
		** Sneeky eh? This means there will be an intervening
		** PST_SUBSEL and PST_RESDOM node to step over. We
		** will allow for stepping over multiple instances
		** in anticipation of select list sub-selects.
		*/
		while ((t = *p) &&
			t->pst_sym.pst_type == PST_SUBSEL &&
			(t = t->pst_left) &&
			t->pst_sym.pst_type == PST_RESDOM)
		{
		    p = &t->pst_right;
		}
		t = *p;
		*p = NULL;
		if (status = pst_node(sess_cb, &sess_cb->pss_ostream,
			t, (PST_QNODE *)NULL, PST_UOP,
			(char *)&op, sizeof(op), DB_NODT, (i2)0, (i4)0,
			(DB_ANYTYPE *)NULL, p, error, PSS_JOINID_PRESET))
		    return (status);
		/* Having injected the node we need to propagete
		** the full resolved datatype to any intermediate
		** SUBSEL/RESDOM nodes */
		while ((t = *p2) &&
			t->pst_sym.pst_type == PST_SUBSEL &&
			(t = t->pst_left) &&
			t->pst_sym.pst_type == PST_RESDOM)
		{
		    /* If subsel here we need to propagate the type up
		    ** from the operator we're about to add.
		    */
		    t->pst_sym.pst_dataval =
			(*p2)->pst_sym.pst_dataval = (*p)->pst_sym.pst_dataval;
		    p2 = &t->pst_right;
		}
		p = &opnode->pst_right;
	    } while (i++ == 0);
	}
    }
    if (status = adi_res_type(adf_scb, best_fidesc,
    		adi_rslv_blk.adi_num_dts, adi_rslv_blk.adi_dt,
		&opnode->pst_sym.pst_dataval.db_datatype))
    {
    	error->err_code = E_PS0C05_BAD_ADF_STATUS;
	return (E_DB_SEVERE);
    }
    opnode->pst_sym.pst_value.pst_s_op.pst_opinst = best_fidesc->adi_finstid;
    opnode->pst_sym.pst_value.pst_s_op.pst_fdesc = best_fidesc;
    if (opnode->pst_sym.pst_dataval.db_datatype < 0 &&
	tempop1.db_datatype > 0)
    {
	tempop1.db_length++;
	tempop1.db_datatype = -tempop1.db_datatype;
    }

    /* Apply collID unless already set */
    switch (abs(opnode->pst_sym.pst_dataval.db_datatype))
    {
    case DB_TXT_TYPE:
    case DB_CHR_TYPE:
    case DB_CHA_TYPE:
    case DB_VCH_TYPE:
    case DB_LVCH_TYPE:
    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_LNVCHR_TYPE:
    case DB_LTXT_TYPE:
    case DB_BOO_TYPE:
	if (DB_COLL_UNSET(opnode->pst_sym.pst_dataval))
	    opnode->pst_sym.pst_dataval.db_collID = collID;
	break;
    default:
	opnode->pst_sym.pst_dataval.db_collID = DB_NOCOLLATION;
    }

    /*
    ** We need to figure out the result length. To do this we must first
    ** get the lengths of the operands after conversion, then we can use
    ** these lengths to figure out the length of the result. As we have to
    ** process each operand, we have an ideal opportunity to try to fold
    ** each as we see them. If all the operands can be folded then the
    ** operator we are resolving might fold too!
    */
    for (i = 0; i < adi_rslv_blk.adi_num_dts; i++)
    {
	PST_QNODE *opr = *oprnds[i];
	DB_DATA_VALUE tempop;
	bool const_coerce = TRUE;
	ADI_FI_ID cnvrtid;
	/* Determine where to store the conversion id. Only space for first 2. */
	if (i == 0)
	    pcnvrtid = &opnode->pst_sym.pst_value.pst_s_op.pst_oplcnvrtid;
	else if (i == 1)
	    pcnvrtid = &opnode->pst_sym.pst_value.pst_s_op.pst_oprcnvrtid;
	else
	    pcnvrtid = &cnvrtid;

	if (adi_rslv_blk.adi_dt[i] == DB_LTXT_TYPE && prepare_best_try)
	{
	    const_cand = FALSE;
	    pop[i] = &ops[i];
	    continue;
	}
	/* Collation ID check. If there is a collation active for this
	** operator, we know that this operand will either have a compatible
	** collation or will be unset. We call pst_apply_collation_coercion to
	** do the check and if necessary provide a collation */
	{
	    DB_DATA_VALUE dv1;
	    dv1.db_datatype = best_fidesc->adi_dt[i];
	    dv1.db_data = NULL;
	    dv1.db_length = 0;
	    dv1.db_prec = 0;
	    dv1.db_collID = collID;
	    if (status = pst_apply_collation_coercion(sess_cb, adf_scb, &dv1, 
			    FALSE, &opr->pst_sym.pst_dataval, oprnds[i], error))
		return status;
	    /* Refresh opr as collID fixup might have added a coercion */
	    opr = *oprnds[i];
	}

	/* Assume no coercion for now */
	ops[i] = tempop = opr->pst_sym.pst_dataval;

	/* If this operand is a constant then we are going to pass this
	** down the coercion path as even if the DTs match the lengths/sizes
	** or precision may still need a coercion that we will otherwise miss
	** and they'll end up added in later by optimiser anyway.
	** Also don't run a coercion that results in a LONG type. */
	if (!Psf_fold ||
		!sess_cb ||
		opr->pst_sym.pst_type != PST_CONST ||
		opr->pst_sym.pst_value.pst_s_cnst.pst_tparmtype != PST_USER ||
		opr->pst_sym.pst_value.pst_s_cnst.pst_parm_no != 0 ||
		best_fidesc->adi_dt[i] != DB_ALL_TYPE && (
		(status = adi_dtinfo(adf_scb, best_fidesc->adi_dt[i], &dtbits)) ||
		(dtbits & AD_PERIPHERAL)))
	    const_coerce = FALSE;

	/* Get coercion id for this child, if needed */
	if (i < best_fidesc->adi_numargs &&
	    best_fidesc->adi_dt[i] != DB_ALL_TYPE &&
	    (best_fidesc->adi_dt[i] != 
		abs(opr->pst_sym.pst_dataval.db_datatype) ||
	    const_coerce &&	/* This to catch elidable constant coercions */
		best_fidesc->adi_dt[i] == base_fidesc->adi_dt[i]))
	{
	    ADI_FI_DESC	fi_desc;

#	    ifdef PSF_FOLD_DEBUG
	    Psf_ncoerce++;
#	    endif
	    status = adi_ficoerce_full(adf_scb, opr->pst_sym.pst_dataval.db_datatype,
			best_fidesc->adi_dt[i], pcnvrtid, &fi_desc);
	    if (status != E_DB_OK)
	    {
		if (!const_coerce ||
		    best_fidesc->adi_dt[i] != base_fidesc->adi_dt[i])
		{
		    goto common_error_return;
		}
		/* Although no coersion operator exists for this we only went
		** down this path to action coersions that matched datatypes
		** but could have length or precision actions required so we
		** ignore the error and saving the NIL coercion FI to
		** use later. */
		*pcnvrtid = ADI_NILCOERCE;
	    }
	    else if (fi_desc.adi_fitype == ADI_AGG_FUNC ||
		    (opnode->pst_sym.pst_value.pst_s_op.pst_flags & PST_INLIST) &&
			opnode->pst_right->pst_sym.pst_type == PST_CONST)
	    {
		/* Don't fold aggregates or IN lists */
		const_cand = FALSE;
		ops[i] = tempop;
	    }
	    else
	    {
		{
		    /* Get length after coercion */
		    DB_DATA_VALUE *opptrs = &ops[i];
		    /* Caller needs to setup the result datatype and the
		    ** coercion will probably have changed it!
		    */
		    tempop.db_datatype = opr->pst_sym.pst_dataval.db_datatype < 0
			? -best_fidesc->adi_dt[i]
			: best_fidesc->adi_dt[i];
		    status = adi_0calclen(adf_scb, &fi_desc.adi_lenspec, 1,
				&opptrs, &tempop);
		}
		if (status != E_DB_OK 
		    && adf_scb->adf_errcb.ad_errcode != E_AD2022_UNKNOWN_LEN)
		{
		    error->err_code = E_PS0C05_BAD_ADF_STATUS;
		    return (E_DB_SEVERE);
		}

		/* Not all coercions are constant! Not at first obvious
		** but due to things like decimal points and currency
		** symbols, some must be done at runtime and can't be
		** optimised :-(
		** So we look for potential constant function invocations
		** based on the VARFI flag being clear (even for coercions)
		** and only when all operands are constant will we fold. */
		if (!const_coerce ||
		    (fi_desc.adi_fiflags & ADI_F1_VARFI) != 0)
		{
		    const_cand = FALSE;
		    ops[i] = tempop;
		}
		else
		{
		    /* Constant fold the coercion.
		    ** Apply the coercion to the constant now and replace
		    ** the constant node. We do this regardless of whether the
		    ** parent can be folded - the coercion can be and that will
		    ** help.
		    */
		    ADF_FN_BLK func_blk;
		    DB_ERROR err_blk;
		    /*
		    ** Set up function block to convert the constant argument.
		    ** Then call ADF to execute it
		    */
		    /* Set up the 1st (and only) parameter */
		    func_blk.adf_dv[1] = opr->pst_sym.pst_dataval;
		    func_blk.adf_dv_n = 1;
		    /* Set up the result value */
		    func_blk.adf_dv[0] = tempop;
		    /* Allocate space for the destination data */
		    status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
					func_blk.adf_dv[0].db_length,
					(PTR *)&func_blk.adf_dv[0].db_data, &err_blk);
		    if (!status)
		    {
			EX_CONTEXT ex_context;

			/* Set up the function id and execute it */
			func_blk.adf_fi_id = *pcnvrtid;
			func_blk.adf_pat_flags = AD_PAT_DOESNT_APPLY;
			func_blk.adf_fi_desc = NULL;
			adf_scb->adf_errcb.ad_errcode = 0;

			/* To control any exceptions, we establish a simple exception
			** handler to contain execution and if fired will merely set
			** the status to reflect a failure in the folding attempt. */
			if (EXdeclare(containment_handler, &ex_context))
			{
			    status = -1;
			}
			else if (fi_desc.adi_fiflags & ADI_F16384_CHKVARFI)
			{
			    u_i1 save = adf_scb->adf_const_const;
			    adf_scb->adf_const_const = 0;
			    status = adf_func(adf_scb, &func_blk);

			    /* For X100, clear any AD_CNST_FOR_X100 bit that
			    ** got set thus implementing the AD_CNST_FOR_X100 */
			    if (sess_cb->pss_stmt_flags2 & PSS_2_X100TAB_INQ)
				adf_scb->adf_const_const &= ~AD_CNST_FOR_X100;
			    /* If any flags set (AD_CNST_DSUPLMT, AD_CNST_USUPLMT or
			    ** AD_CNST_FOR_X100 is set then this was not constant so
			    ** we stop it folding. */
			    if (adf_scb->adf_const_const)
				status = -1;
			    adf_scb->adf_const_const = save;
			}
			else
			    status = adf_func(adf_scb, &func_blk);

			EXdelete();
		    }

		    if (status || adf_scb->adf_errcb.ad_errcode)
		    {
			/* If we got problems trying this, treat the parent
			** as non-constant and continue */
			const_cand = FALSE;
			ops[i] = tempop;
		    }
		    else
		    {
#			ifdef PSF_FOLD_DEBUG
			Psf_nfoldcoerce++;
#			endif
			ops[i] = func_blk.adf_dv[0];
			/* We need to update the parse tree with the
			** new value. It will already be a PST_CONST
			** node but its length will reflect the prior
			** (pre-coerced) value. */
			opr->pst_left = NULL;
			opr->pst_right = NULL;
			opr->pst_sym.pst_len -= opr->pst_sym.pst_dataval.db_length;
			opr->pst_sym.pst_dataval = func_blk.adf_dv[0];
			opr->pst_sym.pst_len += opr->pst_sym.pst_dataval.db_length;
			/* Update the parse tree noting the action done */
			*pcnvrtid = ADI_NILCOERCE;
		    }
		}
	    }
	}
	else
	{
	    /* No coercion needed. This is as determined by datatype as
	    ** above but this could be with a dtfamily fixed up pst_fdesc
	    ** which will not always persist - certainly not through RDF
	    ** if used in a view. As we have saved the coercion FI to
	    ** use later, we now save the fact that no coercion is needed
	    ** for this one even if the datatypes differ! */
	    *pcnvrtid = ADI_NILCOERCE;

	    if (const_cand &&
			!(opr->pst_sym.pst_type == PST_CONST &&
			opr->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER &&
			opr->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0))
		/* Although type matches - the node is not a constant
		** so the parent node can't be either. */
		const_cand = FALSE;
	}

	pop[i] = &ops[i];

	/*
	** (QUEL) If we do end up folding the constant operand(s) into a single
	** constant to replace the operator, we need to fill in a value for
	** the flag pst_pmspec on the replacement constant. The flag can
	** take the values (in logical though not integer order) PST_PMNOTUSED,
	** PST_PMMAYBE, PST_PMUSED. We keep a high-water mark in this order
	** as the values are found in the constant operand(s).
	*/
	if (lang == DB_QUEL && const_cand && opr->pst_sym.pst_type == PST_CONST)
	{
	    PST_PMSPEC cnst_pmspec = opr->pst_sym.pst_value.pst_s_cnst.pst_pmspec;

	    if (cnst_pmspec != PST_PMNOTUSED && cnst_pmspec != pmspec &&
		pmspec != PST_PMUSED)
	    {
		pmspec = cnst_pmspec;
	    }
	}
    }

    /* Now figure out the result length of the operator being resolved */
    status = adi_0calclen(adf_scb, &best_fidesc->adi_lenspec,
			adi_rslv_blk.adi_num_dts, pop,
			&opnode->pst_sym.pst_dataval);

    if (status != E_DB_OK && adf_scb->adf_errcb.ad_errcode != E_AD2022_UNKNOWN_LEN)
    {
	error->err_code = E_PS0C05_BAD_ADF_STATUS;
	return (E_DB_SEVERE);
    }

#   ifdef PSF_FOLD_DEBUG
    Psf_nops++;
#   endif
    if (best_fidesc->adi_fiflags & ADI_F524288_EXPL_COERCION)
    {
	ADI_FI_DESC *fd;
	/* Check if the coercion is applied to an operator whose return type
	** is mutable.
	** Presently only geospatial functions benefit from this due to
	** their datatype family. */
	if (opnode->pst_left &&
	    IS_A_PST_OP_NODE(opnode->pst_left->pst_sym.pst_type) &&
	    (fd = opnode->pst_left->pst_sym.pst_value.pst_s_op.pst_fdesc) &&
	    (fd->adi_fiflags & ADI_F1048576_MUTABLE_RETN))
	{
	    /* Fixup the operator's .pst_fdesc to reflect the altered
	    ** return type/adi_lenspec - retain the updated .pst_dataval from
	    ** the coercion and copy up the rest of the mutable operator into
	    ** that of the coercion. The coercion is then gone (overlayed) and
	    ** the original operator dropped. */
	    fd->adi_dtresult = best_fidesc->adi_dtresult;
	    fd->adi_lenspec.adi_lncompute = ADI_FIXED;
	    fd->adi_lenspec.adi_fixedsize =
				opnode->pst_sym.pst_dataval.db_length;
	    fd->adi_lenspec.adi_psfixed = opnode->pst_sym.pst_dataval.db_prec;
	    opnode->pst_sym.pst_value.pst_s_op =
				opnode->pst_left->pst_sym.pst_value.pst_s_op;
	    opnode->pst_sym.pst_type = opnode->pst_left->pst_sym.pst_type;
	    opnode->pst_right = opnode->pst_left->pst_right;
	    opnode->pst_left = opnode->pst_left->pst_left;

	    /* Nothing more to do now so return.*/
	    return E_DB_OK;
	}
	if (prepare_best_try)
	    return E_DB_OK;
    }
    /* Constant folding of opnode:
    ** Assuming the node is a candidate for folding, we will need
    ** to update the parse tree with the new value. It will not be
    ** a PST_CONST node but its length will need to be large enough
    ** to be melded to such. From the checks at the head of this
    ** routine, we know it will be an operator node and so long
    ** as sizeof(PST_CNST_NODE) <= sizeof(PST_OP_NODE) there
    ** will be space to update inplace.*/
    if (!Psf_fold ||
	    /* Operator not marked as 'do not fold' */
	    (best_fidesc->adi_fiflags & ADI_F1_VARFI) != 0 ||
	    /* Don't attempt to constant-fold a LONG datatype.  The resulting
	    ** value (a coupon) doesn't survive through to the query plan,
	    ** since it's in a temporary LONG holding tank, and execution
	    ** time blows up. */
	    adi_dtinfo(adf_scb, best_fidesc->adi_dtresult, &dtbits) != E_DB_OK ||
	    (dtbits & AD_PERIPHERAL) != 0 ||
	    /* Can't fold aggregates */
	    best_fidesc->adi_fitype == ADI_AGG_FUNC ||
	    opnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_IITRUE_OP ||
	    opnode->pst_sym.pst_value.pst_s_op.pst_opno == ADI_IIFALSE_OP)
	return E_DB_OK;

    if (sess_cb &&
	/* All operands must be constant */
	const_cand &&
	/* The const node must be big enough as inplace replace! */
	sizeof(PST_CNST_NODE) <= sizeof(PST_OP_NODE))
    {
	ADF_FN_BLK func_blk;
	DB_ERROR err_blk;
	/*
	** Set up function block to convert the constant argument.
	** Then call ADF to execute it
	*/
	/* Set up the parameters */
	func_blk.adf_dv_n = adi_rslv_blk.adi_num_dts;
	for (i = 0; i < adi_rslv_blk.adi_num_dts; i++)
	    func_blk.adf_dv[i+1] = ops[i];

	/* Set up the result value */
	func_blk.adf_dv[0] = opnode->pst_sym.pst_dataval;
	/* Allocate space for the destination data */
	status = psf_malloc(sess_cb, &sess_cb->pss_ostream, 
			    func_blk.adf_dv[0].db_length,
			    (PTR *)&func_blk.adf_dv[0].db_data, &err_blk);
	if (!status)
	{
	    EX_CONTEXT ex_context;

	    /* Set up the function id etc. and execute it */
	    func_blk.adf_fi_desc = best_fidesc;
	    func_blk.adf_fi_id = opnode->pst_sym.pst_value.pst_s_op.pst_opinst;
	    func_blk.adf_pat_flags = opnode->pst_sym.pst_value.pst_s_op.pst_pat_flags;
	    func_blk.adf_escape = opnode->pst_sym.pst_value.pst_s_op.pst_escape;
	    adf_scb->adf_errcb.ad_errcode = 0;

	    if (EXdeclare(containment_handler, &ex_context))
	    {
		status = -1;
	    }
	    else if (best_fidesc->adi_fiflags & ADI_F16384_CHKVARFI)
	    {
		u_i1 save = adf_scb->adf_const_const;
		adf_scb->adf_const_const = 0;
		status = adf_func(adf_scb, &func_blk);
		/* For X100, clear any AD_CNST_FOR_X100 bit that
		** got set thus implementing the AD_CNST_FOR_X100 */
		if (sess_cb->pss_stmt_flags2 & PSS_2_X100TAB_INQ)
		    adf_scb->adf_const_const &= ~AD_CNST_FOR_X100;
		/* If any flags set (AD_CNST_DSUPLMT or AD_CNST_USUPLMT
		** then this was not constant - stop it folding. */
		if (adf_scb->adf_const_const)
		    status = -1;
		adf_scb->adf_const_const = save;
	    }
	    else
		status = adf_func(adf_scb, &func_blk);

	    EXdelete();
	}

	if (!status && !adf_scb->adf_errcb.ad_errcode)
	{
#	    ifdef PSF_FOLD_DEBUG
	    Psf_nfolded++;
#	    endif
	    /* Update the parse tree with the new value:
	    ** Clear left and right sub-trees */
	    opnode->pst_left = NULL;
	    opnode->pst_right = NULL;
	    /* Copy data descriptor contents */
	    opnode->pst_sym.pst_dataval = func_blk.adf_dv[0];
	    /* Make into a constant node and adjust length */
	    opnode->pst_sym.pst_type = PST_CONST;
	    opnode->pst_sym.pst_len = sizeof(PST_CNST_NODE) + 
			    opnode->pst_sym.pst_dataval.db_length;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype = PST_USER;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_parm_no = 0;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_pmspec = pmspec;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_cqlang = adf_scb->adf_qlang;
	    opnode->pst_sym.pst_value.pst_s_cnst.pst_origtxt = NULL;
	}
    }
    else switch (opnode->pst_sym.pst_value.pst_s_op.pst_opno)
    {
    case ADI_ADD_OP:
    case ADI_SUB_OP:
	/* Check only RHS for constant as should be narmalised */
	if (pst_is_literal_int(opnode->pst_right, 0LL) &&
		(abs(opnode->pst_sym.pst_dataval.db_datatype) == DB_INT_TYPE ||
		 abs(opnode->pst_sym.pst_dataval.db_datatype) == DB_FLT_TYPE ||
		 abs(opnode->pst_sym.pst_dataval.db_datatype) == DB_DEC_TYPE))
	{
	    DB_DATA_VALUE *op = &opnode->pst_sym.pst_dataval;
	    DB_DATA_VALUE *l = &opnode->pst_left->pst_sym.pst_dataval;
	    /* If DBV is same - just transfer - otherwise change bop to coerce */
	    if (l->db_datatype == op->db_datatype &&
		l->db_length == op->db_length &&
		l->db_prec == op->db_prec &&
		opnode->pst_sym.pst_len >= opnode->pst_left->pst_sym.pst_len)
	    {
		*opnodep = opnode->pst_left;
		return status;
	    }
	    else if (opnode->pst_sym.pst_len >= (i4)sizeof(PST_OP_NODE))
	    {
		PST_OP_NODE *op = &opnode->pst_sym.pst_value.pst_s_op;
		i4 dummy_len;
		opnode->pst_sym.pst_type = PST_UOP;
		opnode->pst_right = NULL;
		op->pst_opinst = -1;
		op->pst_fdesc = NULL;
		if ((status = adi_castid(adf_scb, &opnode->pst_sym.pst_dataval,
				&op->pst_opno, &dummy_len)) ||
			op->pst_opno == ADI_DEC_OP &&
			(status = pst_augment_decimal_cast(sess_cb,
					&sess_cb->pss_ostream,
					opnode, error)))
		    return status;
		return pst_resolve(sess_cb, adf_scb, opnodep, lang, error);
	    }	
	}
	break;
    case ADI_MUL_OP:
    case ADI_DIV_OP:
	/* Check only RHS for constant as should be narmalised */
	if (pst_is_literal_int(opnode->pst_right, 1LL) &&
		(abs(opnode->pst_sym.pst_dataval.db_datatype) == DB_INT_TYPE ||
		 abs(opnode->pst_sym.pst_dataval.db_datatype) == DB_FLT_TYPE ||
		 abs(opnode->pst_sym.pst_dataval.db_datatype) == DB_DEC_TYPE))
	{
	    DB_DATA_VALUE *op = &opnode->pst_sym.pst_dataval;
	    DB_DATA_VALUE *l = &opnode->pst_left->pst_sym.pst_dataval;
	    /* If DBV is same - just transfer - otherwise change bop to coerce */
	    if (l->db_datatype == op->db_datatype &&
		l->db_length == op->db_length &&
		l->db_prec == op->db_prec &&
		opnode->pst_sym.pst_len >= opnode->pst_left->pst_sym.pst_len)
	    {
		*opnodep = opnode->pst_left;
		return status;
	    }
	    else if (opnode->pst_sym.pst_len >= (i4)sizeof(PST_OP_NODE))
	    {
		PST_OP_NODE *op = &opnode->pst_sym.pst_value.pst_s_op;
		i4 dummy_len;
		opnode->pst_sym.pst_type = PST_UOP;
		opnode->pst_right = NULL;
		op->pst_opinst = -1;
		op->pst_fdesc = NULL;
		if ((status = adi_castid(adf_scb, &opnode->pst_sym.pst_dataval,
				&op->pst_opno, &dummy_len)) ||
			op->pst_opno == ADI_DEC_OP &&
			(status = pst_augment_decimal_cast(sess_cb,
                                        &sess_cb->pss_ostream,
                                        opnode, error)))
		    return status;
		return pst_resolve(sess_cb, adf_scb, opnodep, lang, error);
	    }	
	}
	break;
    }
    return E_DB_OK;

common_error_return:
    if (adf_scb->adf_errcb.ad_errcode == E_AD2062_NO_FUNC_FOUND ||
	    adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION)
	/* Complain if no applicable function found */
	error->err_code = (adi_rslv_blk.adi_num_dts == 1) ? 2907L : 2908L;
    else if (adf_scb->adf_errcb.ad_errcode == E_AD2061_BAD_OP_COUNT)
	/* Complain if parameter count is wrong. */
	error->err_code = 2903L;
    else if (adf_scb->adf_errcb.ad_errcode == E_AD2063_FUNC_AMBIGUOUS)
	/* Complain if ambiguous function found */
	error->err_code = (adi_rslv_blk.adi_num_dts == 1) ? 2909L : 2910L;
    else
	error->err_code = E_PS0C05_BAD_ADF_STATUS;
    return E_DB_ERROR;
}


/*{
** Name: containment_handler	- Exception handler for folding errors
**
** Description:
**      This function is the exception handler that will ensure constant
**	folding remains transparent in the event that a function generates
**	an exception.
**
** Inputs:
**      exargs                          The exception handler args, as defined
**					by the CL spec.
**
** Outputs:
**      None
** Returns:
**	EXDECLARE
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	16-Jun-2011 (kiria01) b125450
**          Created
*/
static STATUS
containment_handler(
	EX_ARGS	*exargs)
{
    return EXDECLARE;
}


/*{
** Name: pst_union_resolve	- Resolve a query tree union
**
** Description:
**      The pst_union_resolve function resolves a query tree union.  This
**	means that it sets result type and length for the nodes.  It also
**	figures out which ADF function should be called for the nodes.  This
**	operation should be called whenever a union is added to the select. 
**	Any query tree that comes from the parser is guaranteed to have already
**	been resolved.
**
**	NOTE: This function does not set the conversion ids for an operator
**	node.  That it because the operator node could receive new children
**	of different types.  The conversions must be set before the expression
**	is used.
**
** Inputs:
**	sess_cb				session control block
**					if passed as NULL, dtfamily resolve will
**					be limited for now.
**	adf_scb				ADF cb
**      rootnode			The root node pointing to the union
**      error                           Error block
**
** Outputs:
**      opnode				The resolved operator node
**	error				Standard error block
**	    .err_code			    What error occurred
**		E_PS0000_OK			Success
**		E_PS0002_INTERNAL_ERROR		Internal inconsistency in PSF
**		E_PS0C02_NULL_PTR		rootnode = NULL or rootnode
**						-->pst_union == NULL
**		E_PS0C03_BAD_NODE_TYPE		rootnode is not a root node
**		E_PS0C04_BAD_TREE		Something is wrong with the
**						query tree.
**		2917				Unequal number of resdoms
**						in union.
**		2918				resdom types not compatible
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	4/3/87 (daved)
**	    written
**	23-oct-87 (stec)
**	    Cleanup, do resolution for n subselects, not just two.
**	18-jun-89 (jrb)
**	    This routine now calls ADF to do datatype resolution instead of
**	    taking knowledge of how it should be done.
**	12-jul-89 (jrb)
**	    Made E_AD2061_BAD_OP_COUNT map to a user error rather than letting
**	    PSF treat it as an internal error.
**	01-aug-89 (jrb)
**	    Set precision field for all db_data_values contained in here.  Also,
**	    added rules for doing UNIONs on decimal resdoms.
**	18-aug-89 (jrb)
**	    Fixed problem where error printing code was AVing because it was
**	    using the wrong fi pointer to get the dt name from ADF.
**	30-oct-89 (jrb)
**	    Can't initialize structs on the stack on Unix although it works on
**	    VMS.  Fixed this.
**	15-dec-89 (jrb)
**	    Added checks for DB_ALL_TYPE in function instance descriptors.
**	14-mar-94 (andre)
**	    moved code propagating type information of RESDOM nodes in the first
**	    element of UNION into RESDOM nodes of the subsequent elements
**	15-apr-05 (inkdo01)
**	    Add collation support.
**      09-sep-05 (zhahu02)
**          Updated to be compatible with long nvarchar (INGSRV3411/b115179).
**	15-Oct-2008 (kiria01) b121061
**	    Increase the number of parameters supported in the calls
**	    to adf_func using the more general vector form.
**	19-Jan-2009 (kiria01) b121515
**	    Generate user error if an attempt to UNION NULLs would result in
**	    a typeless NULL.
*/
DB_STATUS
pst_union_resolve(
	PSS_SESBLK  *sess_cb,
	ADF_CB	    *adf_scb,
	PST_QNODE   *rootnode,
	DB_ERROR    *error)
{
    DB_STATUS		status;
    register PST_QNODE	*res1;
    register PST_QNODE	*res2;
    register PST_QNODE	*ss;
    i4		err_code;
    bool		forcenull;
    DB_DATA_VALUE *dv1, *dvn;

    /* Make sure there is a node to resolve */
    if (rootnode == (PST_QNODE *) NULL || 
	rootnode->pst_sym.pst_value.pst_s_root.pst_union.pst_next
						     == (PST_QNODE*) NULL)
    {
	error->err_code = E_PS0C02_NULL_PTR;
	return (E_DB_SEVERE);
    }

    /* Make sure it's a root node */
    if (rootnode->pst_sym.pst_type != PST_ROOT)
    {
	error->err_code = E_PS0C03_BAD_NODE_TYPE;
	return (E_DB_SEVERE);
    }

    /*
    ** In general case, a query expression may have n subselects. We need to
    ** look at data types of the resdoms across subselects (i.e., resdom no. =
    ** const), and determine if they can be coerced into a common datatype, and
    ** if there is more than one, choose the best one. Rules for choosing the
    ** best one are same as in case of other data type coercions. Upon return
    ** the target list of the first subselect shall contain the appropriate
    ** datatype info.  The algorithm is to process two target lists at a time.
    ** In the first run it will be done for the target list of the first and
    ** second subselects, then first and nth subselect, where n = 3, 4, ...
    **
    ** We need to do a scan for collation ID as we need to know the target
    ** collation ID prior to doing the next pass as there might need
    ** to be extra coercions applied. (See similiar in pst_resolve).
    */

    /* traverse each subselect validating its structure against primary */
    for (ss = rootnode->pst_sym.pst_value.pst_s_root.pst_union.pst_next;
		ss; ss = ss->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
    {
	/* walk down list of resdoms for 1st and nth subselect */
	for (res1 = rootnode->pst_left, res2 = ss->pst_left;
		res1 || res2; res1 = res1->pst_left, res2 = res2->pst_left)
	{
	    /* if not both resdoms or tree ends, then error */
	    if (!res1 || !res2 || res1->pst_sym.pst_type != res2->pst_sym.pst_type)
	    {
		i4 zero = 0;
		(VOID) psf_error(2917L, (DB_ERROR *)NULL, PSF_USERERR, 
                    &err_code, error, 1, sizeof (sess_cb->pss_lineno), 
                    sess_cb?&sess_cb->pss_lineno:&zero);
		return (E_DB_ERROR);
	    }

	    /* If we are at a PST_TREE node we have finished this SS */
	    if (res1->pst_sym.pst_type == PST_TREE)
		break;

	    /* Make sure it's a resdom node */
	    if (res1->pst_sym.pst_type != PST_RESDOM)
	    {
		error->err_code = E_PS0C03_BAD_NODE_TYPE;
		return (E_DB_SEVERE);
	    }
	    /* Happy with the structure - propagate collation from expression
	    ** (->pst_right) up to the primary SS resdoms. The primary expressions
	    ** primed the list above. */
	    dv1 = &res1->pst_right->pst_sym.pst_dataval;
	    dvn = &res2->pst_right->pst_sym.pst_dataval;
	    /* Check for select resdoms being constant NULL */
	    forcenull = res1->pst_right->pst_sym.pst_type == PST_CONST &&
			dv1->db_datatype == -DB_LTXT_TYPE &&
			dv1->db_data &&
			(((u_char*)dv1->db_data)[dv1->db_length - 1] & ADF_NVL_BIT) != 0 ||
			res2->pst_right->pst_sym.pst_type == PST_CONST &&
			dvn->db_datatype == -DB_LTXT_TYPE &&
			dvn->db_data &&
			(((u_char*)dvn->db_data)[dvn->db_length - 1] & ADF_NVL_BIT) != 0;

	    if (status = pst_caseunion_resolve(sess_cb, adf_scb,
			&res1->pst_sym.pst_dataval,
			&res2->pst_sym.pst_dataval,
			forcenull, error))
		return (status);
	        
	} /* for each resdom in both analyzed target lists */
    } /* for each subselect in union loop */

    /* Propagate the resdom data type to the subselect being added. */
    for (ss = rootnode; ss;
	 ss = ss->pst_sym.pst_value.pst_s_root.pst_union.pst_next)
    {
	for (res1 = rootnode->pst_left, res2 = ss->pst_left; 
	    res1->pst_sym.pst_type == PST_RESDOM;
	    res1 = res1->pst_left, res2 = res2->pst_left)
	{
	    bool need_coercion;

	    dv1 = &res1->pst_sym.pst_dataval;
	    if (res1 == res2)
	    {
		if (dv1->db_datatype == -DB_LTXT_TYPE)
		{
		    /* After all the processing - untyped NULL remaining is not
		    ** good as we have to have a type */
		    (VOID) psf_error(2183L, (DB_ERROR *)NULL, PSF_USERERR, 
                        &err_code, error, 1,
			sizeof (sess_cb->pss_lineno), &sess_cb->pss_lineno);
		    return (E_DB_ERROR);
		}
	    }
	    else
	    {
		res2->pst_sym.pst_dataval.db_datatype = dv1->db_datatype;
		res2->pst_sym.pst_dataval.db_prec = dv1->db_prec;
		res2->pst_sym.pst_dataval.db_length = dv1->db_length;
		res2->pst_sym.pst_dataval.db_collID = dv1->db_collID;
	    }
	    /* Need to push collID onto elements (including 1) */
	    dvn = &res2->pst_right->pst_sym.pst_dataval;
	    need_coercion = (abs(dv1->db_datatype) != abs(dvn->db_datatype) ||
				dv1->db_length != dvn->db_length ||
				dv1->db_prec != dvn->db_prec);

	    if (status = pst_apply_collation_coercion(sess_cb, adf_scb, dv1,
				need_coercion, dvn, &res2->pst_right, error))
		return status;
	}
    }

    return E_DB_OK;
}

/*{
** Name: pst_caserslt_resolve	- Resolve the set of result expressions
**				in a case function
**
** Description:
**      The pst_caserslt_resolve function resolves the result expressions of
**	a case function. That is, it determines the common data type for
**	all result expressions and sets it into their respective subtrees
**	(and the case node itself).
**
** Inputs:
**	sess_cb				session control block
**					if passed as NULL, dtfamily resolve will
**					be limited for now.
**	adf_scb				ADF cb
**      casep				The case function root node
**      error                           Error block
**
** Outputs:
**      opnode				The resolved operator node
**	error				Standard error block
**	    .err_code			    What error occurred
**		E_PS0000_OK			Success
**		E_PS0002_INTERNAL_ERROR		Internal inconsistency in PSF
**		E_PS0C02_NULL_PTR		rootnode = NULL or rootnode
**						-->pst_union == NULL
**		E_PS0C03_BAD_NODE_TYPE		rootnode is not a root node
**		E_PS0C04_BAD_TREE		Something is wrong with the
**						query tree.
**		2933				Result expressions not coercible
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-sep-99 (inkdo01)
**	    Cloned from pst_union_resolve for case function implementation.
**      23-May-2002 (zhahu02)
**          Modified with the decimal type for case function implementation
**          (b107866/INGSRV1786).
**	07-Jan-2003 (hanch04)
**	    Back out last change.  Not compatible with new code.
**	8-nov-2006 (dougi)
**	    Assign result type to "then" results, too, to assure correct
**	    coercions.
**	13-nov-2006 (dougi)
**	    Oops - that one broke more than it fixed - now removed.
**	22-Mar-07 (wanfr01)
**	    Bug 117979
**	    Need third parameter for adi_resolve
**	3-apr-2007 (dougi)
**	    Permit NULL result from THEN or ELSE.
*/
static DB_STATUS
pst_caserslt_resolve(
	PSS_SESBLK  *sess_cb,
	ADF_CB	    *adf_scb,
	PST_QNODE   *casep,
	DB_ERROR    *error)
{
    DB_STATUS		status;
    register PST_QNODE	*res;
    PST_QNODE		*whlistp, *whop1p, *whop2p;
    DB_DATA_VALUE	*dv1, *dvn;
    bool		forcenull, else_seen;

    /* Make sure there is a node to resolve */
    if (casep == (PST_QNODE *) NULL ||
	(whlistp = casep->pst_left) == (PST_QNODE *)NULL ||
	(whop1p = whlistp->pst_right) == (PST_QNODE *)NULL)
    {
	error->err_code = E_PS0C02_NULL_PTR;
	return (E_DB_SEVERE);
    }

    /* Make sure it's a case function. */
    if (casep->pst_sym.pst_type != PST_CASEOP ||
	whlistp->pst_sym.pst_type != PST_WHLIST ||
	whop1p->pst_sym.pst_type != PST_WHOP)
    {
	error->err_code = E_PS0C03_BAD_NODE_TYPE;
	return (E_DB_SEVERE);
    }

    /* Start by copying first result expression type to "case" node. */
    res = whop1p->pst_right;
    dv1 = &casep->pst_sym.pst_dataval;
    dvn = &res->pst_sym.pst_dataval;
    dv1->db_datatype = dvn->db_datatype;
    dv1->db_prec = dvn->db_prec;
    dv1->db_collID = dvn->db_collID;

    dv1->db_length = dvn->db_length;

    /* Check for stupid "case" with one "when" and no "else". */
    if (whlistp->pst_left == (PST_QNODE *)NULL)
    {
	/* Since there's no else, by definition it has to be nullable! */
	if (dv1->db_datatype > 0)
	{
	    dv1->db_datatype = -dv1->db_datatype;
	    dv1->db_length--;
	}
	return(E_DB_OK);
    }

    /* Check for first result expr being constant NULL */
    forcenull = (res->pst_sym.pst_type == PST_CONST &&
		dvn->db_datatype == -DB_LTXT_TYPE &&
		dvn->db_data != NULL &&
		(((u_char*)dvn->db_data)[dvn->db_length - 1] & ADF_NVL_BIT) != 0);

    /*
    ** The algorithm is simply to resolve the first result expression
    ** (copied to the case node) in turn with each remaining result
    ** expression. This will produce the final result type in the case node.
    */

    /* Walk down when-list to process remaining result exprs. */
    else_seen = FALSE;
    for (whlistp = whlistp->pst_left;
	whlistp && whlistp->pst_sym.pst_type == PST_WHLIST &&
	(whop2p = whlistp->pst_right) && (res = whop2p->pst_right);
	whlistp = whlistp->pst_left) 
    {
	/* Check for next result expr being constant NULL */
	dvn = &res->pst_sym.pst_dataval;
	if (!forcenull &&
		res->pst_sym.pst_type == PST_CONST && 
		dvn->db_datatype == -DB_LTXT_TYPE &&
		dvn->db_data != NULL &&
		(((u_char *)dvn->db_data)[dvn->db_length - 1] & ADF_NVL_BIT) != 0)
	{
	    forcenull = TRUE;
	}

	if (status = pst_caseunion_resolve(sess_cb, adf_scb, dv1, dvn,
			forcenull, error))
	    return (status);

	else_seen = (whop2p->pst_left == NULL);
	forcenull = FALSE;		/* For next trip thru */
    } /* for each "when" in case function */

    /* If no ELSE, make sure we ended up nullable */
    if (!else_seen && casep->pst_sym.pst_dataval.db_datatype > 0)
    {
	casep->pst_sym.pst_dataval.db_datatype = 
			-casep->pst_sym.pst_dataval.db_datatype;
	casep->pst_sym.pst_dataval.db_length++;
    }
    /* Knowing what the results should be - coerce each if needed and
    ** apply collations */
    for (whlistp = casep->pst_left;
	whlistp && whlistp->pst_sym.pst_type == PST_WHLIST &&
	(whop2p = whlistp->pst_right) && (res = whop2p->pst_right);
	whlistp = whlistp->pst_left) 
    {
	bool need_coercion;
	dvn = &res->pst_sym.pst_dataval;
	need_coercion = (abs(dv1->db_datatype) != abs(dvn->db_datatype) ||
			    dv1->db_length != dvn->db_length ||
			    dv1->db_prec != dvn->db_prec);

	if (status = pst_apply_collation_coercion(sess_cb, adf_scb, dv1,
				need_coercion, dvn, &whop2p->pst_right, error))
	    return status;
    } /* for each "when" in case function */
    return    (E_DB_OK);
}

/*
** Name: pst_caseunion_resolve - Resolve types for CASE or UNION
**
** Description:
**
**	Resolve two data values into the "highest" data type,
**	storing the resulting type into the first one.
**
**	If one or the other of the inputs is explicitly the constant
**	NULL, all we need to do is take the other value's type info,
** 	as nullable.  Since there are a few different ways this can
**	occur (e.g. a CASE with no ELSE), we'll let the caller figure
**	it out and flag it.
**
** Inputs:
**	sess_cb			PSS_SESBLK parser session control block
**				if passed as NULL, dtfamily resolve will
**				be limited for now.
**	adf_scb			ADF cb
**	dv1			The best-so-far data value
**	dv2			The other data value to resolve
**	forcenull		TRUE if just make nullable
**	error			Returned error info if errors
**
** Outputs:
**	May update dv1 with new type info
**	Returns E_DB_OK or error
**
** History:
**	3-Nov-2006 (kschendel)
**	    Extract from union and CASE resolvers, in an attempt to fix
**	    up problems with "case when p then decimal() else null".
**	22-Mar-07 (wanfr01)
**	    Bug 117979
**	    Need third parameter for adi_resolve
**	21-Mar-2008 (kiria01) b120144
**	    Apply dtfamily processing to resolve ANSI date coersion.
*/

static DB_STATUS
pst_caseunion_resolve(
	PSS_SESBLK *sess_cb,
	ADF_CB *adf_scb,
	DB_DATA_VALUE *dv1,
	DB_DATA_VALUE *dv2,
	bool forcenull,
	DB_ERROR *error)

{
    ADI_FI_DESC *best_fidesc;
    ADI_FI_DESC *new_fidesc = NULL;
    ADI_RSLV_BLK adi_rslv_blk;
    bool nullable;
    DB_DT_ID temp_dt;
    DB_COLL_ID collID = DB_UNSET_COLL;
    DB_STATUS status;
    i4 err_code;
    i4 dtbits;

    /* Handle unknown as best we can - expect that we can coerce param
    ** to whatever common type is resolved & coerce later. */
    if (dv2->db_datatype == DB_NODT)
	return E_DB_OK;
    if (dv1->db_datatype == DB_NODT)
    {
	dv1->db_datatype = dv2->db_datatype;
	dv1->db_prec = dv2->db_prec;
	dv1->db_length = dv2->db_length;
	dv1->db_collID = dv2->db_collID;
	return E_DB_OK;
    }
    /* Coalesce the collation IDs (if any). If nth is valid,
    ** copy to 1st. Later, 1st will be propagated to the rest. */
    if (!DB_COLL_UNSET(*dv2))
    {
	if (DB_COLL_UNSET(*dv1) ||
	    dv1->db_collID == DB_UNICODE_COLL &&
		dv2->db_collID == DB_UNICODE_CASEINSENSITIVE_COLL)
	{
	    collID = dv2->db_collID;
	}
	else if (dv1->db_collID != dv2->db_collID &&
	    (dv1->db_collID != DB_UNICODE_CASEINSENSITIVE_COLL ||
		dv2->db_collID != DB_UNICODE_COLL))
	{
	    i4 err_code;
	    psf_error(6427L, 0L, PSF_USERERR, &err_code, error, 0);
	    return E_DB_ERROR;
	}
    }
    /* Update primary resdom collID */
    switch (abs(dv1->db_datatype))
    {
    case DB_TXT_TYPE:
    case DB_CHR_TYPE:
    case DB_CHA_TYPE:
    case DB_VCH_TYPE:
    case DB_LVCH_TYPE:
    case DB_NCHR_TYPE:
    case DB_NVCHR_TYPE:
    case DB_LNVCHR_TYPE:
    case DB_LTXT_TYPE:
    case DB_BOO_TYPE:
	if (collID != DB_UNSET_COLL)
	    dv1->db_collID = collID;
	break;
    default:
	dv1->db_collID = DB_NOCOLLATION;
	break;
    }

    /* If we just need to force nullability, do so.  Assume that we'll use
    ** the datatype of the first operand unless it's internal-text, in which
    ** case use the second operand.
    */
    if (forcenull)
    {
	if (abs(dv1->db_datatype) == DB_LTXT_TYPE)
	{
	    dv1->db_datatype = dv2->db_datatype;
	    dv1->db_prec = dv2->db_prec;
	    dv1->db_length = dv2->db_length;
	    dv1->db_collID = dv2->db_collID;
	}
	if (dv1->db_datatype > 0)
	{
	    dv1->db_datatype = -dv1->db_datatype;
	    ++ dv1->db_length;
	}
	return (E_DB_OK);
    }

    /* ask ADF to find a common datatype for the current resdoms */
    adi_rslv_blk.adi_op_id   = ADI_FIND_COMMON;
    adi_rslv_blk.adi_num_dts = 2;
    adi_rslv_blk.adi_dt[0]     = dv1->db_datatype;
    adi_rslv_blk.adi_dt[1]     = dv2->db_datatype;

    status = adi_resolve(adf_scb, &adi_rslv_blk, FALSE);

    if (status != E_DB_OK)
    {
	ADI_DT_NAME leftparm, rightparm;	/* For error reporting */
	i4 zero = 0;
	/* Complain if no applicable function found */
	if (	adf_scb->adf_errcb.ad_errcode == E_AD2062_NO_FUNC_FOUND
	    ||	adf_scb->adf_errcb.ad_errcode == E_AD2061_BAD_OP_COUNT
	    ||	adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION
	    ||  adf_scb->adf_errcb.ad_errcode == E_AD2063_FUNC_AMBIGUOUS)
	{
	    i4 err = 2918;

	    if (adf_scb->adf_errcb.ad_errcode == E_AD2063_FUNC_AMBIGUOUS)
		err = 2919;

	    status = adi_tyname(adf_scb,
				    adi_rslv_blk.adi_dt[0],
				    &leftparm);
	    if (status != E_DB_OK)
	    {
		if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		{
		    (VOID) psf_error(E_PS0C04_BAD_TREE,
			&adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
			&err_code, error, 0);
		}
		else
		{
		    (VOID) psf_error(E_PS0C04_BAD_TREE,
			&adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
			&err_code, error, 0);
		}
		return (E_DB_SEVERE);
	    }
	    status = adi_tyname(adf_scb,
				    adi_rslv_blk.adi_dt[1],
				    &rightparm);
	    if (status != E_DB_OK)
	    {
		if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
		{
		    (VOID) psf_error(E_PS0C04_BAD_TREE,
			&adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
			&err_code, error, 0);
		}
		else
		{
		    (VOID) psf_error(E_PS0C04_BAD_TREE,
			&adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
			&err_code, error, 0);
		}
		return (E_DB_SEVERE);
	    }
	    (VOID) psf_error(err, (DB_ERROR *)NULL, PSF_USERERR, 
		&err_code, error, 3, 
		sizeof (sess_cb->pss_lineno), sess_cb?&sess_cb->pss_lineno:&zero,
		psf_trmwhite(sizeof (ADI_DT_NAME), (char *) &leftparm), 
		&leftparm, 
		psf_trmwhite(sizeof (ADI_DT_NAME), (char *) &rightparm),
		&rightparm);

	    return (E_DB_ERROR);
	}

	error->err_code = E_PS0C05_BAD_ADF_STATUS;
	return (E_DB_ERROR);
    }

    /* we now have the "best" fi descriptor */
    if (best_fidesc = adi_rslv_blk.adi_fidesc)
    {
	i2	need_fmly_resolve = 0;

	status = adi_need_dtfamily_resolve( adf_scb, best_fidesc, 
					    &adi_rslv_blk, 
					    &need_fmly_resolve);
	if (status)
	{
	    error->err_code = E_PS0C05_BAD_ADF_STATUS;
	    return (E_DB_SEVERE);
	}

	if (sess_cb && need_fmly_resolve == 1) 
	{
	    status = psf_malloc(sess_cb, &sess_cb->pss_ostream,
		sizeof(ADI_FI_DESC), &new_fidesc, error);
	    if (status != E_DB_OK)
	    {
		return (status);
	    }
	    if  (new_fidesc == NULL)
	    {
		error->err_code = E_PS0C05_BAD_ADF_STATUS;
		return (E_DB_SEVERE);
	    }

	    status = adi_dtfamily_resolve_union(adf_scb, best_fidesc, 
					    new_fidesc, &adi_rslv_blk);
	    if (status != E_DB_OK)
	    {
		return (status);
	    }

	    best_fidesc = new_fidesc; 
	}
    }

    /*
    ** In some cases coercion per se may not be needed, but
    ** data type may have to be changed to nullable.
    */

    status = adi_res_type(adf_scb, best_fidesc,
		adi_rslv_blk.adi_num_dts, adi_rslv_blk.adi_dt,
		&temp_dt);
		
    nullable = (bool) (temp_dt < 0);

    /* update type info of left node, if needed */
    status = pst_get_union_resdom_type(best_fidesc->adi_dt[0],
		dv1, adf_scb, nullable, error);
    if (DB_FAILURE_MACRO(status))
	return(status);
    
    /* if either type is long datatype, must short circuit the
    ** resolution process here. Since long's have no "=" ops,
    ** adi_resolve (way back there) produced the actual coercion 
    ** fi. Since there's only one operand for coercions, the
    ** rest of this stuff needs to be bypassed */

    if (E_DB_OK == adi_dtinfo(adf_scb, best_fidesc->adi_dt[0], &dtbits) &&
		(dtbits & AD_PERIPHERAL) == 0)
    {                             /* not "long" - keep going */
	DB_DATA_VALUE res_dv;
	
	/* update type info of right node, if needed;
	** do into temp, keep dv2 untouched.
	*/
	STRUCT_ASSIGN_MACRO(*dv2, res_dv);
	status = pst_get_union_resdom_type(best_fidesc->adi_dt[1],
		&res_dv, adf_scb, nullable, error);
	if (DB_FAILURE_MACRO(status))
	    return(status);

       /*
	** For decimal datatype we use the ADI_DECBLEND lenspec to get the
	** result precision, scale, and length; for other types, just
	** take the maximum length.  The resulting precision/scale and
	** length will be placed in dv1 which will end up with the values
	** that everything else will need to be coerced into.
	*/
	if (abs(dv1->db_datatype) == DB_DEC_TYPE)
	{
	    DB_DATA_VALUE	    *dv[2];
	    DB_DATA_VALUE	    tmp;
	    ADI_LENSPEC	    lenspec;

	    tmp.db_datatype = DB_DEC_TYPE;
	    tmp.db_data = NULL;

	    lenspec.adi_lncompute	= ADI_DECBLEND;
	    lenspec.adi_fixedsize	= 0;
	    lenspec.adi_psfixed	= 0;
	    dv[0] = dv1;
	    dv[1] = &res_dv;
	
	    status = adi_0calclen(adf_scb, &lenspec, 2, dv, &tmp);
	    if (status != 0)
	    {
		error->err_code = E_PS0C05_BAD_ADF_STATUS;
		return (E_DB_SEVERE);
	    }

	    dv[0]->db_prec = tmp.db_prec;
	    dv[0]->db_length = tmp.db_length;
	    dv[0]->db_collID = DB_NOCOLLATION;

	    if (nullable)
		dv[0]->db_length++;
	}
	else
	{
	    if (dv1->db_length < res_dv.db_length)
	    {
		dv1->db_length = res_dv.db_length;
	    }
	}
    }

    return (E_DB_OK);
} /* pst_caseunion_resolve */

/*
** Name:    pst_get_union_resdom_type - determine type info of a resdom node
**
** Description:
**	Using result of adi_resolve() and adi_res_type(), determine type info of
**	a resdom node of a UNION select
**
** Input:
**	dt		result of resolving datatypes of left and right RESDOM
**			nodes
**	resval		ptr to DB_DATA_VALUE of RESDOM node whose type info needs
**			to be computed
**	adf_scb		ADF session CB
**	nullable	indicates whether the common datatype should be nullable
**
** Output:
**	error
**	    err_code	filled in if an error occurred
**
** Returns:
**	E_DB_{OK,STATUS}
**
** History:
**	14-mar-94 (andre)
**	    extracted from the body of pst_union_resolve()
**	8-sep-99 (inkdo01)
**	    Changed 2nd parm to ptr to pst_dataval from ptr to node (to make 
**	    case funcs a bit easier to code)
*/
static DB_STATUS
pst_get_union_resdom_type(
			  DB_DT_ID	dt,
			  DB_DATA_VALUE	*resval,
			  ADF_CB	*adf_scb,
			  bool		nullable,
			  DB_ERROR	*error)
{
    DB_STATUS	    status;
    ADI_FI_ID	    convid;
    ADI_FI_DESC	    fi_desc;
    DB_DATA_VALUE   resop;
    DB_DATA_VALUE   *opptrs[ADI_MAX_OPERANDS];

    /* Get coercion id for the resdom, if needed */
    if (dt != abs(resval->db_datatype) && dt != DB_ALL_TYPE)
    {
	ADI_RSLV_BLK tmp_adi_rslv_blk;
	status = adi_ficoerce_full(adf_scb, resval->db_datatype,
	    dt, &convid, &fi_desc);
	if (status != E_DB_OK)
	{
	    error->err_code = E_PS0C05_BAD_ADF_STATUS;
	    return(E_DB_SEVERE);
	}

	resop.db_datatype = (nullable) ? -dt : dt;

	opptrs[0] = resval;
	/* Get length after coercion */
	status = adi_0calclen(adf_scb, &fi_desc.adi_lenspec, 1,
	    opptrs, &resop);
	if (   status != E_DB_OK
	    && adf_scb->adf_errcb.ad_errcode != E_AD2022_UNKNOWN_LEN)
	{
	    error->err_code = E_PS0C05_BAD_ADF_STATUS;
	    return(E_DB_SEVERE);
	}

	/* Remember the result length of the coercion */
	resval->db_prec = resop.db_prec;
	resval->db_length = resop.db_length;
	resval->db_datatype = resop.db_datatype;
    }
    /* Convert from non-nullable to nullable */
    else if (resval->db_datatype > 0 && nullable)
    {
	resval->db_datatype = -resval->db_datatype;
	resval->db_length++;
    }

    return(E_DB_OK);
}

/*{
** Name: pst_parm_resolve   - Resolve a resdom node representing
**			      local var/parm.
**
** Description:
**      The function resolves datatypes for a resdom node representing
**	a local variable or a parameter. The datatype of the resdom node
**	must not actually change, it's the right child that needs adjustment.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	08-jun-88 (stec)
**	    written
**	03-nov-88 (stec)
**	    Correct bug found by lint.
*/
DB_STATUS
pst_parm_resolve(
	PSS_SESBLK  *sess_cb,
	PSQ_CB	    *psq_cb,
	PST_QNODE   *resdom)
{
    ADF_CB	    *adf_scb = (ADF_CB *)sess_cb->pss_adfcb;
    ADI_DT_BITMASK  mask;
    DB_STATUS	    status;
    i4	    err_code;
    DB_DT_ID	    rdt, cdt;
    ADI_DT_NAME	    rdtname, cdtname;
    

    /* Make sure there is a node to resolve */
    if (resdom == (PST_QNODE *) NULL || 
	resdom->pst_right == (PST_QNODE*) NULL
       )
    {
	return (E_DB_OK);
    }

    /* Make sure it's a resdom node */
    if (resdom->pst_sym.pst_type != PST_RESDOM)
    {
	(VOID) psf_error(E_PS0C03_BAD_NODE_TYPE, (DB_ERROR *)NULL,
	    PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	return (E_DB_SEVERE);
    }

    rdt = resdom->pst_sym.pst_dataval.db_datatype;
    cdt = resdom->pst_right->pst_sym.pst_dataval.db_datatype;

    /* See what the right child is coercible into. */
    status = adi_tycoerce(adf_scb, abs(cdt), &mask);

    if (DB_FAILURE_MACRO(status))
    {
	if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	{
	    (VOID) psf_error(E_PS0C05_BAD_ADF_STATUS,
		&adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
		&err_code, &psq_cb->psq_error, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0C05_BAD_ADF_STATUS,
		&adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
		&err_code, &psq_cb->psq_error, 0);
	}
	return(status);
    }

    /* See what the resdom data type is one of the options. */
    if (BTtest(ADI_DT_MAP_MACRO(abs(rdt)), (char *)&mask) == FALSE)
    {
	/* Conversion impossible. */

	status = adi_tyname(adf_scb, rdt, &rdtname);
	if (DB_FAILURE_MACRO(status))
	{
	    if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE,
		    &adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    else
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE,
		    &adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    return (E_DB_SEVERE);
	}

	status = adi_tyname(adf_scb, cdt, &cdtname);
	if (DB_FAILURE_MACRO(status))
	{
	    if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE,
		    &adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    else
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE,
		    &adf_scb->adf_errcb.ad_dberror, PSF_INTERR,
		    &err_code, &psq_cb->psq_error, 0);
	    }
	    return (E_DB_SEVERE);
	}

	(VOID) psf_error(2417L, (DB_ERROR *)NULL, PSF_USERERR, &err_code, 
	    &psq_cb->psq_error, 2,
	    psf_trmwhite(sizeof(ADI_DT_NAME), (char *) &cdtname), &cdtname,
	    psf_trmwhite(sizeof(ADI_DT_NAME), (char *) &rdtname), &rdtname); 

	return (E_DB_ERROR);
    }

    return    (E_DB_OK);
}


/*{
** Name: pst_handler	- Exception handler for local coercion errors
**
** Description:
**      This function is the exception handler just for coercion errors
**	locally, since the standard PSF exception handler simply shuts down
**	the session, while our errors here are handleable.
**
** Inputs:
**      exargs                          The exception handler args, as defined
**					by the CL spec.
**
** Outputs:
**      None
** Returns:
**	EXDECLARE
** Exceptions:
**	None
**
** Side Effects:
**	Can set the adf_cb errclass
**
** History:
**	26-sep-07 (kibro01)
**          written
*/
static STATUS
pst_handler(
	EX_ARGS            *exargs)
{
    STATUS		ret_val = EXRESIGNAL;
    PSS_SESBLK		*sess_cb;
    ADF_CB		*adf_cb = NULL;

    if (exargs->exarg_num == EXINTOVF || exargs->exarg_num == EXFLTOVF)
    {
	sess_cb = psf_sesscb();
	if (sess_cb)
	    adf_cb = sess_cb->pss_adfcb;
	/* Note that this is an error which can be reported back */
	if (adf_cb)
	{
	    adf_cb->adf_errcb.ad_errclass = ADF_USER_ERROR;
	    ret_val = EXCONTINUES;
	}
    }
    return(ret_val);
}

/*{
** Name: pst_convlit	- Convert constant literal between string & 
**				numeric
**
** Description:
**      In INSERT/UPDATE statements and binary expressions, if context
**	dictates that type mismatch can be overcome by converting a 
**	constant literal to the target type, we'll give it a whack.
**	This is integral to the Rpath request to support integer literals
**	in character columns and vice versa.
**
** Inputs:
**	sess_cb		- ptr to parser control block
**	stream		- ptr to memory stream for constant allocation
**	targdv		- ptr to target type description
**	srcenode	- ptr to source node
**
** Outputs:
**	Returns:
**	    FALSE			Conversion couldn't be done
**	    TRUE			Conversion was successful
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	13-sep-2006 (dougi)
**	    Written.
**	26-sep-2007 (kibro01) b119191
**	    Added exception so that if one of the coercion functions
**	    throws an exception we catch it here.
*/
bool
pst_convlit(
	PSS_SESBLK  *sess_cb,
	PSF_MSTREAM *stream,
	DB_DATA_VALUE *targdv,
	PST_QNODE   *srcenode)

{
    ADF_CB	*adf_cb = sess_cb->pss_adfcb;
    DB_STATUS	status;
    DB_ERROR	errblk;
    DB_DATA_VALUE convdv;
    char	*convptr;
    DB_DT_ID	targtype, srcetype;
    EX_CONTEXT	ex_context;


    /* Verify that source node is a PST_CONST literal. */
    if (srcenode->pst_sym.pst_type != PST_CONST ||
	srcenode->pst_sym.pst_value.pst_s_cnst.pst_tparmtype != PST_USER)
	return(FALSE);

    /* We have a literal - now see if it is coercible. Verify the 
    ** permissable combinations. */
    srcetype = abs(srcenode->pst_sym.pst_dataval.db_datatype);
    targtype = abs(targdv->db_datatype);

    switch (targtype) {
      case DB_INT_TYPE:
      case DB_FLT_TYPE:
      case DB_DEC_TYPE:
	if (srcetype != DB_CHA_TYPE && srcetype != DB_VCH_TYPE)
	    return(FALSE);
	break;

      case DB_CHA_TYPE:
      case DB_VCH_TYPE:
	if (srcetype != DB_INT_TYPE && srcetype != DB_FLT_TYPE &&
		srcetype != DB_DEC_TYPE)
	    return(FALSE);
	break;

      default:	/* All the rest won't work. */
	return(FALSE);
    }

    /* Passed the easy tests - allocate space for the converted literal. */
    status = psf_malloc(sess_cb, stream, targdv->db_length, 
	&convptr, &errblk);
    if (status != E_DB_OK)
	return(FALSE);

    STRUCT_ASSIGN_MACRO(*targdv, convdv);
    if (convdv.db_datatype < 0)
    {
	/* Convert to not null so's not to offend the coercion functions. */
	convdv.db_datatype = -convdv.db_datatype;
	convdv.db_length--;
    }

    convdv.db_data = convptr;

    if (EXdeclare(pst_handler, &ex_context) != OK)
    {
	EXdelete();
	return (FALSE);
    }

    /* Use switch to direct coercion to correct function. */
    switch(targtype) {
      case DB_INT_TYPE:
	status = adu_1int_coerce(adf_cb, &srcenode->pst_sym.pst_dataval,
						&convdv);
	break;

      case DB_FLT_TYPE:
	status = adu_1flt_coerce(adf_cb, &srcenode->pst_sym.pst_dataval,
						&convdv);
	break;

      case DB_DEC_TYPE:
	status = adu_1dec_coerce(adf_cb, &srcenode->pst_sym.pst_dataval,
						&convdv);
	break;

      case DB_CHA_TYPE:
      case DB_VCH_TYPE:
	status = adu_ascii(adf_cb, &srcenode->pst_sym.pst_dataval,
						&convdv);
	break;

      default:	/* Shouldn't get here - but just in case */
	EXdelete();
	return(FALSE);
    }

    EXdelete();

    if (adf_cb->adf_errcb.ad_errclass == ADF_USER_ERROR)
    {
	/* We found an overflow or similar failure through an exception, so 
	** unset the error class and return FALSE (kibro01) b119191
	*/
	adf_cb->adf_errcb.ad_errclass = 0;
	return(FALSE);
    }

    /* If coercion failed, just return FALSE. */
    if (status != E_DB_OK)
	return(FALSE);

    /* Copy the resulting DB_DATA_VALUE and return with success. */
    STRUCT_ASSIGN_MACRO(convdv, srcenode->pst_sym.pst_dataval);
    return(TRUE);		/* Ta da! */

}

/*{
** Name: pst_apply_collation_coercion - Apply collation and coercion to node.
**
** Description:
**	pst_resolve, pst_union_resolve and pst_caserslt_resolve each need
**	to resolve datatypes an apply collation merging. This routine applies
**	the collation rules and if needed, can introduce the coercion that
**	might be needed to carry the collation id. For example, UNICODE
**	collation on LHS cannot be applied to a CHAR. In this case the
**	CHAR needs coercing to correspond to a better type that can take the
**	UNICODE collation.
**
** Inputs:
**	sess_cb		Pointer to parser session contorl block
**	datatype	Datatype to resolve to.
**	collID		Collation ID to resolve to
**	need_coercion	Boolean to potentially apply coercion even if
**			not needed for collation id.
**	dvn		Data value that may need updating
** Outputs:
**	dvn		Data value that may need updating
**	target		Address of pointer to update should the parse
**			tree need updating for coercion.
**	error		Error block
**
**	Returns:
**	    status from pst_node or E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    May allocate coercion node
**
** History:
**	07-Dec-2010 (kiria01) SIR 124690
**	    Added to consolidate logic in one place 
*/
static DB_STATUS
pst_apply_collation_coercion (
	PSS_SESBLK *sess_cb,
	ADF_CB *adf_scb,
	DB_DATA_VALUE *dv1,
	bool need_coercion,
	DB_DATA_VALUE *dvn,
	PST_QNODE **target,
	DB_ERROR *error)
{
    DB_STATUS status = E_DB_OK;

    if (!DB_COLL_UNSET(*dv1))
    {
	i4 valid_fl = Psf_valid_coll[dv1->db_collID];
	enum DB_DC_ID_enum dc_got = PSF_DT_CLASS(dvn->db_datatype);
	enum DB_DC_ID_enum dc_need = PSF_DT_CLASS(abs(dv1->db_datatype));
	/* This part is awkward. We need to set the operand
	** to match collID if a string type but collID might not
	** be valid for this datatype. However, there is likely
	** to be an implicit coersion that could be made explict
	** to carry the collID. */
	if (dc_got == DB_CHAR_CLASS &&
		    !CMischarset_utf8() && (valid_fl & F_NON_UNICODE) ||
	    (dc_got == DB_CHAR_CLASS && CMischarset_utf8() ||
	     dc_got == DB_NCHAR_CLASS) && (valid_fl & F_UNICODE))
	{
	    /* Directly applicable to the operand - do it */
	    dvn->db_collID = dv1->db_collID;
	}
	else if (dc_need == DB_CHAR_CLASS &&
			!CMischarset_utf8() && (valid_fl & F_NON_UNICODE) ||
		(dc_need == DB_CHAR_CLASS &&
			CMischarset_utf8() ||
		 dc_need == DB_NCHAR_CLASS) && (valid_fl & F_UNICODE))
	{
	    need_coercion = TRUE;
	}
    }
    if (need_coercion)
    {
	/* We need the explict coercion operator for the implicit that
	** we would be using */
	status = pst_mk_similar(sess_cb, adf_scb, &sess_cb->pss_ostream,
					dv1, target, error);
    }
    return status;
}


/*
** Name:	pst_mk_similar
**
** Description:	This routine will cast a node into a different form.
**
** Input:
**	sess_cb			PSF session CB
**	adf_scd			ADF DB
**	templ			Addresss of template DBV to match
**				If length specified as 0, calclen will
**				be called to fill in the detail.
**	nodep			Address of pointer to the parse tree
**	
** Output:
**	nodep			Address of pointer to the parse tree
**				may be updated if sucessful change.
**	error			may be filled in if an error occurs
**
** Side effects:
**	will allocate memory
**
** Returns:
**	E_DB_OK, E_DB_ERROR
**
** History:
**	19-Nov-2010 (kiria01) SIR 124690
**	    Added to apply implicit coercions as explicit.
**	04-Jan-2011(hweho01)
**	    Correct the pointer array size of pop, change from 0 to 1. 
**	11-Jul-2011 (kiria01) b125530
**	    If we find that we are adding a cast to an expression that has
**	    already got the same cast operator but unresolved then replace
**	    old with new.
**	28-Jul-2011 (kiria01) b125530 
**	    When string truncation is set to be signalled we need to use
**	    operators that will raise the error if needed. All the usual
**	    length qualified string coercions are set to silently truncate.
**	    Operators have been added that allow for the necessary control.
**	19-Aug-2011 (kiria01) b125530
**	    Adjust for adi_castid interface change that simplifies the
**	    building of the second parameter if needed.
*/

DB_STATUS
pst_mk_similar(
	PSS_SESBLK	*sess_cb,
	ADF_CB		*adf_scb,
	PSF_MSTREAM	*mstream,
	DB_DATA_VALUE	*templ,
	PST_QNODE	**nodep,
	DB_ERROR	*err_blk)
{
    DB_STATUS status = E_DB_OK;
    DB_DATA_VALUE cdv = (*nodep)->pst_sym.pst_dataval;
    DB_DATA_VALUE ndv = *templ;
    PST_OP_NODE op;
    PST_QNODE *res_node = NULL;

    /* For now if sess_cb is null (not in parser) - make this a no-op */
    if (!sess_cb)
	return status;

    if (ndv.db_length == 0)
    {
 	ADI_FI_ID cnvrtid;
	ADI_FI_DESC fdesc;
	DB_DATA_VALUE *pop[1];
	pop[0] = &cdv;
	/* We need the explicit coercion operator for the implicit that
	** we would be using and determine the unconstrained length as we have
	** not been forced into a full CAST but just datatype & collID */
	if (status = adi_ficoerce_full(adf_scb, cdv.db_datatype, ndv.db_datatype,
			&cnvrtid, &fdesc))
	{
	    if (adf_scb->adf_errcb.ad_errcode == E_AD2009_NOCOERCION)
		status = E_DB_OK;
	    return status;
	}
	if (status = adi_0calclen(adf_scb, &fdesc.adi_lenspec, 1, pop, &ndv))
            return status;
    }
    if (ndv.db_datatype < 0)
    {
	ndv.db_datatype = -ndv.db_datatype;
	ndv.db_length--;
    }
    if (cdv.db_datatype < 0)
    {
	cdv.db_datatype = -cdv.db_datatype;
	cdv.db_length--;
    }
    if (ndv.db_datatype == cdv.db_datatype &&
	ndv.db_length == cdv.db_length &&
	ndv.db_prec == cdv.db_prec)
    {
	if (ndv.db_collID > DB_NOCOLLATION)
	    (*nodep)->pst_sym.pst_dataval.db_collID = ndv.db_collID;
	return status;
    }
    op.pst_opmeta = PST_NOMETA;
    op.pst_pat_flags = AD_PAT_DOESNT_APPLY;
    /* The darn i4 casts are because sizeof is technically
    ** a size_t, which is 64-bit on LP64, and gcc complains. */
    switch (ndv.db_datatype)
    {
    case DB_INT_TYPE:
	switch (ndv.db_length)
	{
	case (i4)sizeof(i1):
	    op.pst_opno = ADI_I1_OP;
	    break;
	case (i4)sizeof(i2):
	    op.pst_opno = ADI_I2_OP;
	    break;
	case (i4)sizeof(i4):
	    op.pst_opno = ADI_I4_OP;
	    break;
	case (i4)sizeof(i8):
	    op.pst_opno = ADI_I8_OP;
	    break;
	}
	break;
    case DB_FLT_TYPE:
	switch (ndv.db_length)
	{
	case (i4)sizeof(f4):
	    op.pst_opno = ADI_F4_OP;
	    break;
	case (i4)sizeof(f8):
	    op.pst_opno = ADI_F8_OP;
	    break;
	}
	break;
    case DB_DEC_TYPE:
	{
	    /* Decimal is a little more awkward */
	    PST_CNST_NODE cconst;
	    cconst.pst_tparmtype = PST_USER;
	    cconst.pst_parm_no = 0;
	    cconst.pst_pmspec  = PST_PMNOTUSED;
	    cconst.pst_cqlang = DB_SQL;
	    cconst.pst_origtxt = NULL;

	    /* Find out best prec/scale to use if we have the option.
	    ** If there is no choice, we will cast directly anyway */
	    if (ndv.db_prec == 0)
	    {
		if (status = adu_decprec(sess_cb->pss_adfcb, &cdv, &ndv.db_prec))
		    break;
	    }
	    if (status = pst_node(sess_cb, mstream, NULL, NULL,
			    PST_CONST, (char *)&cconst, sizeof(cconst),
			    DB_INT_TYPE, 0, sizeof(ndv.db_prec),
			    (DB_ANYTYPE *)&ndv.db_prec, &res_node, err_blk, 0))
		return status;
	    op.pst_opno = ADI_DEC_OP;
	}
	break;
    default:
	if (ndv.db_datatype == cdv.db_datatype &&
	    ndv.db_prec == cdv.db_prec)
	{
	    (*nodep)->pst_sym.pst_dataval.db_collID = ndv.db_collID;
	    return status;
	}
	else
	{
	    i4 len;

	    if (status = adi_castid(adf_scb, &ndv, &op.pst_opno, &len))
		return status;
	    if (len != 0)
	    {
		PST_CNST_NODE cconst;
		cconst.pst_tparmtype = PST_USER;
		cconst.pst_parm_no = 0;
		cconst.pst_pmspec  = PST_PMNOTUSED;
		cconst.pst_cqlang = DB_SQL;
		cconst.pst_origtxt = (char *) NULL;
		if (status = pst_node(sess_cb, mstream, NULL, NULL,
			    PST_CONST, (char *)&cconst, sizeof(cconst),
			    DB_INT_TYPE, 0, sizeof(len), (DB_ANYTYPE *)&len,
			    &res_node, err_blk, 0))
		    return status;
		if (sess_cb->pss_adfcb->adf_strtrunc_opt != ADF_IGN_STRTRUNC)
		{
		    /* We may need a signalling conversion at this point so add
		    ** appropriate bool
		    ** FIXME: This ought to be effected by a runtime test to avoid
		    ** variations in QP caching due to ADF settings. This could be
		    ** simply achieved with a session enquiry function that could be
		    ** folded away if needed. */
		    PST_QNODE *p3;
		    DB_ANYTYPE val;
		    switch (op.pst_opno)
		    {
		    case ADI_NVCHAR_OP:
		    case ADI_VARCH_OP:
		    case ADI_TEXT_OP:
		    case ADI_VBYTE_OP:
		    case ADI_NCHAR_OP:
		    case ADI_ASCII_OP:
		    case ADI_CHAR_OP:
		    case ADI_BYTE_OP:
			val.db_booltype = (sess_cb->pss_adfcb->adf_strtrunc_opt ==
						    ADF_ERR_STRTRUNC);
			if (status = pst_node(sess_cb, mstream, NULL, NULL,
					    PST_CONST, (PTR)&cconst, sizeof(cconst),
					    DB_BOO_TYPE, 0, sizeof(val.db_booltype), &val,
					    &p3, err_blk, 0))
			    return status;
    			/* Add the extra operand. */
			if (status = pst_node(sess_cb, mstream, res_node, p3,
					    PST_OPERAND, NULL, 0, DB_NODT, 0, 0, NULL,
					    &res_node, err_blk, 0))
			    return status;
		    }
		}
	    }
	}
    }
    if (templ->db_datatype < 0)
    {
	/* Make result nullable if we were requested */
	ndv.db_datatype = -ndv.db_datatype;
	ndv.db_length++;
    }

    /* We know that we are adding an explicit cast so if there is
    ** another identical operator already there, drop it now & use
    ** newly created one as it will be matched to destination unless
    ** it's passing an extra parameter. */
    if (!status && IS_A_PST_OP_NODE((*nodep)->pst_sym.pst_type))
    {
	if (op.pst_opno == (*nodep)->pst_sym.pst_value.pst_s_op.pst_opno)
	{
	    if ((*nodep)->pst_right == NULL)
		*nodep = (*nodep)->pst_left;
	    else if (res_node == NULL)
		return status;
	}
    }

    if (status = pst_node(sess_cb, mstream, *nodep, res_node,
		    res_node ? PST_BOP : PST_UOP,
		    (char *)&op, sizeof(op), ndv.db_datatype, ndv.db_prec, ndv.db_length,
		    (DB_ANYTYPE *)NULL, nodep, err_blk, PSS_JOINID_STD))
	return status;
    if (ndv.db_collID > DB_NOCOLLATION)
	(*nodep)->pst_sym.pst_dataval.db_collID = ndv.db_collID;
    return status;
}


/*
** Name:	pst_is_literal_int
**
** Description:	This routine will determine if a given node has the
**		specified constant value.
**
** Input:
**	node			Address of pointer to the parse tree
**	val			Value to be checked for. Usually 0 or 1
**	
** Output:
**	none
**
** Side effects:
**	none
**
** Returns:
**	boolean			TRUE if value is constant and matches
**
** History:
**	05-Jul-2011 (kiria01) b125511
**	    Added to allow for simple optimisations in pst_resolve.
*/

bool
pst_is_literal_int(
	PST_QNODE	*node,
	i8		val)
{
    if (node &&
	node->pst_sym.pst_dataval.db_data &&
	node->pst_sym.pst_type == PST_CONST &&
	node->pst_sym.pst_value.pst_s_cnst.pst_tparmtype == PST_USER &&
	node->pst_sym.pst_value.pst_s_cnst.pst_parm_no == 0)
    {
	switch (node->pst_sym.pst_dataval.db_datatype)
	{
	case DB_INT_TYPE:
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(i4))
		return *(i4*)node->pst_sym.pst_dataval.db_data == val;
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(i2))
		return *(i2*)node->pst_sym.pst_dataval.db_data == val;
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(i8))
		return *(i8*)node->pst_sym.pst_dataval.db_data == val;
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(i1))
		return *(i1*)node->pst_sym.pst_dataval.db_data == val;
	    break;
	case -DB_INT_TYPE:
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(i4)+1)
		return *(i4*)node->pst_sym.pst_dataval.db_data == val;
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(i2)+1)
		return *(i2*)node->pst_sym.pst_dataval.db_data == val;
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(i8)+1)
		return *(i8*)node->pst_sym.pst_dataval.db_data == val;
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(i1)+1)
		return *(i1*)node->pst_sym.pst_dataval.db_data == val;
	    break;
	case DB_FLT_TYPE:
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(f4))
		return *(f4*)node->pst_sym.pst_dataval.db_data == (f4)val;
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(f8))
		return *(f8*)node->pst_sym.pst_dataval.db_data == (f8)val;
	    break;
	case -DB_FLT_TYPE:
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(f4)+1)
		return *(f4*)node->pst_sym.pst_dataval.db_data == (f4)val;
	    if (node->pst_sym.pst_dataval.db_length == (i4)sizeof(f8)+1)
		return *(f8*)node->pst_sym.pst_dataval.db_data == (f8)val;
	    break;
	case DB_DEC_TYPE:
	case -DB_DEC_TYPE:
	    {
		f8 res;
		if (!CVpkf((PTR)node->pst_sym.pst_dataval.db_data,
			(i4)DB_P_DECODE_MACRO(node->pst_sym.pst_dataval.db_prec),
			(i4)DB_S_DECODE_MACRO(node->pst_sym.pst_dataval.db_prec),
			&res))
		    return res == (f8)val;
	    }
	}
    }
    return FALSE;
}

/*
** Name: pst_augment_decimal_cast - add correct parameters to cast for decimal
**
** Description:	The decimal datatype cast requires that the scale and precision
**		be passed correctly to effect the required coercion.
**
** Inputs:
**	sess_cb					Session CB	
**	mstream					Stream to allocate from
**	node					Pointer to the node
**	  .pst_sym.pst_dataval.db_prec		Must be set to expected P&S
**	  .pst_sym.pst_type			Must be PST_UOP
**	  .pst_sym.pst_value.pst_s_op.pst_opno	Should be ADI_DEC_OP
**	err_blk					Place to put error code
**
** Outputs:
**	node
**	  .pst_right				Updated with new node
**	  .pst_sym.pst_type			Updated to PST_BOP
**	err_blk					Udated if error
**
** Side Effects:
**	Allocates memory out of the stream.
**
** History:
**	05-Jul-2011 (kiria01) b125511
**	    Created
*/

DB_STATUS
pst_augment_decimal_cast(
	PSS_SESBLK	*sess_cb,
	PSF_MSTREAM	*mstream,
	PST_QNODE	*node,
	DB_ERROR	*err_blk)
{
    if (node->pst_sym.pst_type == PST_UOP &&
	node->pst_sym.pst_value.pst_s_op.pst_opno == ADI_DEC_OP)
    {
	PST_CNST_NODE cconst;
	cconst.pst_tparmtype = PST_USER;
	cconst.pst_parm_no = 0;
	cconst.pst_pmspec  = PST_PMNOTUSED;
	cconst.pst_cqlang = DB_SQL;
	cconst.pst_origtxt = NULL;

	node->pst_sym.pst_type = PST_BOP;
	return pst_node(sess_cb, mstream, NULL, NULL,
			PST_CONST, (char *)&cconst, sizeof(cconst),
			DB_INT_TYPE, 0, sizeof(node->pst_sym.pst_dataval.db_prec),
			(DB_ANYTYPE *)&node->pst_sym.pst_dataval.db_prec,
			&node->pst_right, err_blk, 0);
    }
    return E_DB_OK;
}

/*{
** Name: pst_resolve_err	- Handle resolver errors
**
** Description:
**      This function provides common error handling for resolver errors
**
** Inputs:
**	sess_cb
**	node				Pointer to the node
**	err_blk				Place to put error code
**	flags				Pointer to mask to convey special info
**					as to pst_node(). Only the following
**					bits are relevant here:
**		    PSS_NOTYPERES_ERR  - Don't report type resolution errors.
**		    PSS_REF_TYPERES    - if error, report special error 
**
** Outputs:
**      err_blk
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Minor error or user error
**	    E_DB_FATAL			Catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	20-Oct-2011 (kiria01) b125878
**	    Allow more type resolution even during prepares.
*/
DB_STATUS
pst_resolve_err(
	PSS_SESBLK	*cb,
	PST_QNODE	*node,
	DB_ERROR	*err_blk,
	i4		*flags)
{
    i4 err_code;
    ADF_CB *adf_scb = cb->pss_adfcb;

    /*
    ** If error indicates that there was no applicable function or
    ** an ambiguous function was found, and the user requested that
    ** such errors not be shown to the user, just return; otherwise
    ** display error to the user.
    */

    if (((*flags) & PSS_NOTYPERES_ERR) && (
	err_blk->err_code == 2903 ||
	err_blk->err_code == 2907 ||
	err_blk->err_code == 2908 ||
	err_blk->err_code == 2909 ||
	err_blk->err_code == 2910))
    {
	/*
	** Turn off PSS_NOTYPERES_ERR bit to indicate that an error
	** of the typem expected by the user has been encountered
	** and return E_DB_ERROR
	*/
	*flags ^= PSS_NOTYPERES_ERR;
	err_blk->err_code = 0L;	/* reset error code */
    }
    else if (((*flags) & PSS_REF_TYPERES) && (
		err_blk->err_code == 2903 ||
		err_blk->err_code == 2907 ||
		err_blk->err_code == 2908 ||
		err_blk->err_code == 2909 ||
		err_blk->err_code == 2910))
    {
	PST_QNODE *l = node->pst_left;
	PST_QNODE *r = node->pst_right;
	/*
	** If error indicates that there was no applicable function,
	** & the user wants a special REFERENTIAL CONSTRAINT error,
	** display that special error to the user.
	*/
	psf_error(E_PS0485_REF_TYPE_RES, NULL, PSF_USERERR, &err_code, err_blk, 2,
		psf_trmwhite(sizeof(DB_ATT_NAME),
			(char *)&l->pst_sym.pst_value.pst_s_var.pst_atname),
			&l->pst_sym.pst_value.pst_s_var.pst_atname,
		psf_trmwhite(sizeof(DB_ATT_NAME),
			(char *)&r->pst_sym.pst_value.pst_s_var.pst_atname),
			&r->pst_sym.pst_value.pst_s_var.pst_atname);
    }
    else if (err_blk->err_code == E_PS0001_USER_ERROR && (
	node->pst_sym.pst_type == PST_CASEOP ||
	node->pst_sym.pst_type == PST_WHOP || 
	node->pst_sym.pst_type == PST_WHLIST ||
	adf_scb->adf_errcb.ad_errcode == E_AD5052_DATEVALID ||
	adf_scb->adf_errcb.ad_errcode == E_AD5058_DATEBADCHAR ||
	adf_scb->adf_errcb.ad_errcode == E_AD5053_DATEYEAR ||
	adf_scb->adf_errcb.ad_errcode == E_AD5054_DATEMONTH ||
	adf_scb->adf_errcb.ad_errcode == E_AD5055_DATEDAY ||
	adf_scb->adf_errcb.ad_errcode == E_AD5056_DATETIME ||
	adf_scb->adf_errcb.ad_errcode == E_AD5059_DATEAMPM ||
	adf_scb->adf_errcb.ad_errcode == E_AD505A_DATEYROVFLO ||
	adf_scb->adf_errcb.ad_errcode == E_AD505E_NOABSDATES ||
	adf_scb->adf_errcb.ad_errcode == E_AD5067_ANSIDATE_4DIGYR))
    {
	;/* case-specific error or date parsing */
    }
    else
    {
	if (err_blk->err_code == E_PS0C05_BAD_ADF_STATUS)
	{
	    switch (node->pst_sym.pst_value.pst_s_op.pst_opno)
	    {
	    case ADI_GREATEST_OP:
	    case ADI_LEAST_OP:
	    case ADI_LESSER_OP:
	    case ADI_GREATER_OP:
		err_blk->err_code = 2938;
	    }
	}
	return pst_rserror(cb->pss_lineno, node, err_blk);
    }

    return E_DB_ERROR;
}
