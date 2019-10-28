/* Copyright 2011, Actian Corporation */

#include	<compat.h>
#include	<gl.h>
#include	<me.h>
#include	<st.h>
#include	<cm.h>
#include	<iicommon.h>
#include	<adf.h>
#include	<ulf.h>
#include	<adudate.h>
#include	<adfint.h>
#include	<aduint.h>


/*
** File: adtx100out.c -- X100 conversion and output routines
**
** Description:
**	This file contains ADF-related X100 conversion routines,
**	such as "print constant to x100 algebra".
**
** History:
**	16-Mar-2011 (kschendel) m1541
**	    Extract from opcx100 so qef can use as well.
*/

/* List statics, forward declarations, module variables here */

/* none yet */

/*
** Name: adt_x100_print -- Print constant for X100 algebra
**
** Description:
**	This routine prints a db-data-value in a form suitable for
**	inclusion in X100 algebra.
**
**	Since it's used in a couple different ways, the caller will
**	supply a low level "print string" routine, plus a state
**	control block suitable for said routine.  The caller will
**	also supply an ADF-CB and the DB_DATA_VALUE to print.
**
** Inputs:
**	outstr		Routine to print a null-terminated string to the
**			caller's output text, called as:
**			(*outstr)(outcb, string);
**	outcb		Arbitrary state pointer, passed to outstr routine
**	adfcb		An ADF control block for conversions and ADF calls
**	dbv		The DB_DATA_VALUE to convert/print
**	nulltype	Data type to use for a NULL -- if passed as DB_NODT,
**			dbv's datatype is used.
**
** Outputs:
**	Returns E_DB_OK or error status;
**	errors will (should) have set up error info in the ADF_CB.
**
** History:
**	17-Mar-2011 (kschendel) m1541
**	    Extract from opcx100 so that QEF can use it too, for expanding
**	    actual-parameters into prototype X100 algebra text.
*/

DB_STATUS
adt_x100_print(void (*outstr)(void *,const char *), void *outcb,
	ADF_CB *adfcb, DB_DATA_VALUE *dbv, DB_DT_ID nulltype)
{

    ALIGN_RESTRICT	adtwork[(ADF_ADATE_LEN+DB_ALIGN_SZ-1)/DB_ALIGN_SZ];
    ALIGN_RESTRICT	workbuf[(2000+DB_ALIGN_SZ-1)/DB_ALIGN_SZ];
    char		*convbuf;
    char		*convbuf_base;
    char		*localwork = (char *)&workbuf[0];
    register char	*p, *e;
    DB_DATA_VALUE	adtedv;
    DB_DATA_VALUE       dest_dt;
    DB_STATUS           tmcvt_status;
    i4			convlen;
    i4			datatype, origtype;
    i4                  default_width, worst_width;
    i4			outlen;
    STATUS		stat;
    u_i4		trimlen;

    origtype = datatype = abs(dbv->db_datatype);
    convbuf = convbuf_base = localwork;
    convlen = sizeof(workbuf);

    /* Check for type-specific NULL value. */
    if (dbv->db_datatype < 0 &&
	dbv->db_data != NULL &&
	(dbv->db_data[dbv->db_length-1] & ADF_NVL_BIT))
	datatype = DB_LTXT_TYPE;		/* indicate null */

    if (datatype == DB_CHA_TYPE || datatype == DB_VCH_TYPE ||
	datatype == DB_NVCHR_TYPE || datatype == DB_NCHR_TYPE)
    {
	/* For string types convert literal to UTF8 */
	DB_STATUS cvinto_status;
	u_i2 outlen_i2;

	/* Allow for UTF8 blow-up, maybe length, maybe null */
	outlen = dbv->db_length;
	if (datatype == DB_NVCHR_TYPE || datatype == DB_NCHR_TYPE)
	    outlen = dbv->db_length/2;
	outlen = outlen*4 + DB_CNTSIZE + 1;
	if (outlen > convlen)
	{
	    convbuf_base = (char *) MEreqmem(0, outlen, FALSE, &stat);
	    if (stat != OK)
		return (adu_error(adfcb, E_AD9998_INTERNAL_ERROR, 2, 0, "no UTF8 conv memory"));
	    convbuf = convbuf_base;
	    convlen = outlen;
	}
	dest_dt.db_length = outlen;
	dest_dt.db_data = convbuf;
	dest_dt.db_datatype = (dbv->db_datatype > 0) ? DB_UTF8_TYPE : -DB_UTF8_TYPE;
	dest_dt.db_prec = 0;
	if (datatype == DB_CHA_TYPE || datatype == DB_VCH_TYPE)
	{
	    cvinto_status = adc_cvinto( adfcb, dbv, &dest_dt);
	}
	else
	{
	    cvinto_status = adu_nvchr_toutf8( adfcb, dbv, &dest_dt); 
	}
	if (DB_FAILURE_MACRO(cvinto_status))
	{
	    if (convbuf_base != localwork)
		MEfree(convbuf_base);
	    return (cvinto_status);
	}
	I2ASSIGN_MACRO(convbuf[0], outlen_i2);
	convbuf += DB_CNTSIZE;
	outlen = outlen_i2;
    }
    else if (datatype != DB_BOO_TYPE && datatype != DB_LTXT_TYPE)
    {
	DB_STATUS	tmlen_status;

	/* Ingres date literals are unsupported by X100, but 
	** can apparently appear here under some circumstances. 
	** Convert to ANSI date first. */
	if (datatype == DB_DTE_TYPE)
	{
	    DB_STATUS	cvinto_status;

	    adtedv.db_datatype = DB_ADTE_TYPE;
	    adtedv.db_data = (PTR) &adtwork[0];
	    adtedv.db_length = ADF_ADATE_LEN;
	    adtedv.db_prec = 0;
	    adtedv.db_collID = -1;
	    
	    cvinto_status = adc_cvinto(adfcb, dbv, &adtedv);
	    if (DB_FAILURE_MACRO(cvinto_status))
		return (cvinto_status);

	    /* Work with ansidate from here on */
	    dbv = &adtedv;
	}

	tmlen_status = adc_tmlen(adfcb, dbv, &default_width, &worst_width);
	if (DB_FAILURE_MACRO(tmlen_status))
	    return (tmlen_status);

	/* Make sure it fits in localwork, add a little extra just in case,
	** use dynamic memory if necessary.
	*/
	if (worst_width + 1 + 20 > convlen)
	{
	    outlen = worst_width + 1 + 20;
	    convbuf_base = (char *) MEreqmem(0, outlen, FALSE, &stat);
	    if (stat != OK)
		return (adu_error(adfcb, E_AD9998_INTERNAL_ERROR, 2, 0, "no X100 prconst memory"));
	    convbuf = convbuf_base;
	    convlen = outlen;
	}

	dest_dt.db_length = default_width;
	dest_dt.db_data = convbuf_base;
	tmcvt_status= adc_tmcvt( adfcb, dbv, &dest_dt, &outlen); /* use the
				    ** terminal monitor conversion routines
				    ** to display the results */
	if (DB_FAILURE_MACRO(tmcvt_status))
	{
	    /* Try again for full width */
	    dest_dt.db_length = worst_width;
	    tmcvt_status = adc_tmcvt(adfcb, dbv, &dest_dt, &outlen);
	}
	if (DB_FAILURE_MACRO(tmcvt_status))
	{
	    if (convbuf_base != localwork)
		MEfree(convbuf_base);
	    return (tmcvt_status);
	}
    }
    else
	outlen = 0;
    /* Escape the single quotes that tm leaves single. */
    p = convbuf;
    e = p + outlen;
    while (p < e)
    {
	if (*p == '\'')
	{
	    i4 nquot = 1;
	    /* At least 1 */
	    register char *fst = p++;
	    while (p < e)
		if (*p++ == '\'')
		    nquot++;
	    p = e;
	    if (convbuf + outlen + nquot + 1 > convbuf_base + convlen)
		return (adu_error(adfcb, E_AD9998_INTERNAL_ERROR, 2, 0, "quoting overflow"));
	    e += nquot;
	    fst = e;
	    for(;;)
	    {
		*--fst = *--p;
		if (*p == '\'')
		{
		    *--fst = '\\';
		    if (!--nquot)
			break;
		}
	    }
	    break;
	}
	p++;
    }
    *e = 0;	/* null terminate the buffer before printing */

    switch (datatype)
    {	
	case DB_CHR_TYPE:
	case DB_CHA_TYPE:
	case DB_NCHR_TYPE:
	{
	    /* Presently don't trim CHAR/C type constants. This
	    ** runs counter to usual SQL semantics but is thought
	    ** it might aid constant sizing for X100. */
	    (*outstr)(outcb, " char('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_TXT_TYPE:
	case DB_VCH_TYPE:
	case DB_BYTE_TYPE:
	case DB_VBYTE_TYPE:
	case DB_NVCHR_TYPE:
	{
	    (*outstr)(outcb, " str('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_INT_TYPE:
	{
	    while (convbuf[0] == ' ') 
		convbuf = &convbuf[1];	/* skip the leading blanks */
	    trimlen = STtrmwhite(&convbuf[0]);
	    if (dbv->db_length >= 8)
		(*outstr)(outcb, " slng('");
	    else (*outstr)(outcb, " sint('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_FLT_TYPE:
	{
	    while (convbuf[0] == ' ') 
		convbuf = &convbuf[1];	/* skip the leading blanks */
	    trimlen = STtrmwhite(&convbuf[0]);
	    if (dbv->db_length >= 8 && datatype == DB_FLT_TYPE)
		(*outstr)(outcb, " dbl('");
	    else (*outstr)(outcb, " flt('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_DEC_TYPE:
	{
	    while (convbuf[0] == ' ') 
		convbuf = &convbuf[1];	/* skip the leading blanks */
	    trimlen = STtrmwhite(&convbuf[0]);
	    (*outstr)(outcb, " decimal('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_ADTE_TYPE:
	case DB_DTE_TYPE:
	{
	    trimlen = STtrmwhite(&convbuf[0]);
	    (*outstr)(outcb, " date('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_TSWO_TYPE:
	case DB_TSW_TYPE:
	case DB_TSTMP_TYPE:
	{
	    trimlen = STtrmwhite(&convbuf[0]);
	    (*outstr)(outcb, " timestamp('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_TMWO_TYPE:
	case DB_TMW_TYPE:
	case DB_TME_TYPE:
	{
	    trimlen = STtrmwhite(&convbuf[0]);
	    (*outstr)(outcb, " time('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_INDS_TYPE:
	{
	    trimlen = STtrmwhite(&convbuf[0]);
	    (*outstr)(outcb, " intervalds('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_INYM_TYPE:
	{
	    trimlen = STtrmwhite(&convbuf[0]);
	    (*outstr)(outcb, " intervalym('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_MNY_TYPE:
	{
	    trimlen = STtrmwhite(&convbuf[0]);
	    (*outstr)(outcb, " money('");
	    (*outstr)(outcb, &convbuf[0]);
	    (*outstr)(outcb, "')");
	    break;
	}
	case DB_BOO_TYPE:
	{
	    bool res;
	    if (dbv->db_length == sizeof(char))
		res = (bool)*(char*)dbv->db_data;
	    else
		res = *(bool*)dbv->db_data;
	    (*outstr)(outcb, res ? "==(sint('0'),sint('0'))" : "!=(sint('0'),sint('0'))");
	    break;
	}
	case DB_LTXT_TYPE:
	{
	    /* this type is used to represent null literals.
	    ** If caller had a specific null type in mind, use it.
	    ** Otherwise stick with the original type.  (The reason for
	    ** this is that we might have just NULL in some context
	    ** that gives a result-type, but NULL itself is type-ambiguous.)
	    */
	    if (nulltype == DB_NODT)
		nulltype = origtype;
	    switch(nulltype)
	    {
		case DB_CHR_TYPE:
		case DB_CHA_TYPE:
		case DB_TXT_TYPE:
		case DB_VCH_TYPE:
		case DB_NCHR_TYPE:
		case DB_NVCHR_TYPE:
		case DB_BYTE_TYPE:
		case DB_VBYTE_TYPE:
		    (*outstr)(outcb, " null(str(' '))");
		    break;
		case DB_FLT_TYPE:
		    (*outstr)(outcb, " null(dbl('0'))");
		    break;
		case DB_INT_TYPE:
		case DB_DEC_TYPE:
		    (*outstr)(outcb, " null(sint('0'))");
		    break;
		case DB_ADTE_TYPE:
		case DB_DTE_TYPE:
		    (*outstr)(outcb, " null(date('1-1-1'))");
		    break;
		case DB_TSWO_TYPE:
		case DB_TSW_TYPE:
		case DB_TSTMP_TYPE:
		    (*outstr)(outcb, 
				" null(timestamp('1-1-1 0:0:0'))");
		    break;
		case DB_TMWO_TYPE:
		case DB_TMW_TYPE:
		case DB_TME_TYPE:
		    (*outstr)(outcb, " null(time('0:0:0'))");
		    break;
		case DB_INYM_TYPE:
		    (*outstr)(outcb, " null(intervalym('0'))");
		    break;
		case DB_INDS_TYPE:
		    (*outstr)(outcb, " null(intervalds('0 0:0:0'))");
		    break;
		case DB_MNY_TYPE:
		    (*outstr)(outcb, " null(money('0'))");
		    break;
		default:
		    (*outstr)(outcb, " null(uchr('0'))");
		    break;
	    }
	    break;
	}
	default:
	    trimlen = STtrmwhite(&convbuf[0]);
	    (*outstr)(outcb, &convbuf[0]);
    }

    if (convbuf_base != localwork)
	MEfree(convbuf_base);

    return (E_DB_OK);

} /* adt_x100_print */
