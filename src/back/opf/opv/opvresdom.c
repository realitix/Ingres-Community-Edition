/*
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <sl.h>
#include    <me.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
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

/* external routine declarations definitions */
#define        OPV_RESDOM      TRUE
#include    <opxlint.h>
#include    <st.h>

/**
**
**  Name: OPVRESDOM.C - file to create result domain
**
**  Description:
**      file to create result domain query node
**
**  History:    
**      1-jul-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	06-Jul-06 (kiria01) b116230
**	    psl_print becomes PSL_RS_PRINT bit in new field psl_rsflags
**	    to allow for extra flags in PST_RSDM_NODE
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
PST_QNODE *opv_resdom(
	OPS_STATE *global,
	PST_QNODE *function,
	DB_DATA_VALUE *datatype);
void opv_norm_resdom(
	OPS_STATE *global,
	PST_QNODE *resdom,
	bool force);

/*{
** Name: opv_resdom	- create a resdom node
**
** Description:
**      The routine will create a resdom node given the query tree fragment 
**      which will produce the attribute and the result datatype.
**
** Inputs:
**      global                          ptr to global state variable
**      function                        query tree to provide result
**      datatype                        datatype of result domain
**
** Outputs:
**	Returns:
**	    ptr to result domain query tree node
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	22-apr-86 (seputis)
**          initial creation
**	12-aug-98 (hayke02)
**	    Initialize the resdom pst_joinid if there is an outer join. This
**	    resdom maybe used in a join (if there is an ifnull in an OJ view).
**	    This prevents an incorrect or out of range ojid being used to
**	    access the outer join descriptor array (opl_ojt[]) in the PST_BOP
**	    case in opb_bfinit(). This change fixes bug 92300.
**	16-july-2008 (dougi)
**	    Invent name for generated RESDOMs (for X100).
**	27-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-selects - Make sure whole resdom init'd
**	25-nov-00 (smeke01) b123321 b92300
**	    Remove above hayke02 change. The change above relied on setting the
**	    pst_joinid field on a PST_QNODE of type PST_RESDOM, but PST_RESDOM
**	    nodes do not have a pst_joinid field.
*/
PST_QNODE *
opv_resdom(
	OPS_STATE          *global,
	PST_QNODE          *function,
	DB_DATA_VALUE      *datatype)
{
    PST_QNODE		*resdom;    /* result domain node being created */
    char		rsname[DB_MAXNAME];

    resdom = (PST_QNODE *) opu_memory(global, (i4) sizeof(PST_QNODE) );
    MEfill((i4) sizeof(PST_QNODE), (u_char)0, (PTR)resdom);
    /* resdom->pst_left = NULL; */
    resdom->pst_right = function;
    resdom->pst_sym.pst_type = PST_RESDOM;
    resdom->pst_sym.pst_len = sizeof(PST_RSDM_NODE);
    STRUCT_ASSIGN_MACRO(*datatype, resdom->pst_sym.pst_dataval);
    /* resdom->pst_sym.pst_dataval.db_data = NULL; */
    resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsflags = PST_RS_PRINT;
    resdom->pst_sym.pst_value.pst_s_rsdm.pst_ntargno =
    resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsno = OPZ_NOATTR;
    /* resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsupdt = FALSE; */
    resdom->pst_sym.pst_value.pst_s_rsdm.pst_ttargtype = PST_ATTNO;

    opv_norm_resdom(global, resdom, TRUE);

    return( resdom );
}

/*{
** Name: opv_norm_resdom	- normalize resdom name
**
** Description:
**      The routine will normalise a resdom name
**
** Inputs:
**      global		ptr to global state variable
**      resdom		query tree to provide result
**	force		skip preset check if needed
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
**	14-Jun-2011 (kiria01) m2106
**	    Created
*/
void
opv_norm_resdom(
	OPS_STATE	*global,
	PST_QNODE	*resdom,
	bool		force)
{
    PST_QNODE *function = resdom->pst_right;
    char *name = (char*)&resdom->pst_sym.pst_value.pst_s_rsdm.pst_rsname;
    if (!force)
    {
	u_i4 i;
	for (i = 0; i < DB_MAXNAME; i++)
	{
	    if (name[i] == EOS)
		break; /* Empty or we've only seen spaces so far */
	    if (name[i] != ' ')
		return; /* Not a space and not forced to must already be set */
	}
    }
    if (function && function->pst_sym.pst_type == PST_VAR)
	MEcopy((char *)&function->pst_sym.pst_value.pst_s_var.pst_atname,
		DB_MAXNAME, name);
    else
    {
	/* Fabricate name if one isn't available. */
	STprintf(name, "TRSDM_%-u", global->ops_trsno++);
	memset(name + STlen(name), ' ', DB_MAXNAME - STlen(name));
    }
}
