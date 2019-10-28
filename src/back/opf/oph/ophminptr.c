/*
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <cs.h>
#include    <sl.h>
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
#define        OPH_MINPTR      TRUE
#include    <opxlint.h>


/**
**
**  Name: OPHMINPTR.C - return min ptr
**
**  Description:
**      Routine to routine the minimum of two ptrs
**
**  History:    
**      1-jun-86 (seputis)    
**          initial creation
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* TABLE OF CONTENTS */
PTR oph_minptr(
	OPS_STATE *global,
	PTR hist1ptr,
	PTR hist2ptr,
	DB_DATA_VALUE *datatype);

/*{
** Name: oph_minptr	- return ptr to minimum of two histogram values
**
** Description:
**      This routine will return a pointer to the minimum of two histogram
**      values
**
**	This routine will call oph_tcompare rather than opu_compare
**	if the attribute type is TEXT. This has to do with the fact that 
**	although the type may be TEXT, the histogram value doesn't 
**	have a count field in  front of it (like most TEXT values do).
**
** Inputs:
**      hist1ptr                        ptr to first histogram value
**	hist2ptr                        ptr to second histogram value
**      datatype                        ptr to datatype of both values
**
** Outputs:
**	Returns:
**	    hist1ptr if (*hist1ptr) <= (*hist2ptr) and hist2ptr otherwise
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-jun-86 (seputis)
**          initial creation
[@history_line@]...
*/
PTR
oph_minptr(
	OPS_STATE          *global,
	PTR                hist1ptr,
	PTR                hist2ptr,
	DB_DATA_VALUE      *datatype)
{
    i4                  result;	    /* negative if hist1ptr <= hist2ptr */

    if (datatype->db_datatype == DB_TXT_TYPE)
    {
	result = oph_tcompare(hist1ptr, hist2ptr, datatype);
    }
    else
    {
	result = opu_compare (global, hist1ptr, hist2ptr, datatype);
    }

    return (result <= 0 ? hist1ptr : hist2ptr);
}
