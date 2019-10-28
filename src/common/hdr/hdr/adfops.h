#ifndef __ADFOPS_H_INCLUDED
#define __ADFOPS_H_INCLUDED
/*
** Copyright (c) 2004,2008 Actian Corporation
*/

/**
** Name: ADFOPS.H - This file contains the defines of the operators
**                  known to ADF
**
** Description:
**      The parser treats operators slightly different than it treats
**      functions.  Because of this, the parser needs to know the
**      operator ids of all of the operators known to the ADF.  These
**      will also be available through the ADF call "adi_opid()", and
**      therefore must have operator ids that are unique from those
**      assigned to functions.  ADF will reserve a range of operator
**      ids for operators.  This range has been decreed to be 1 - 99.
**      Therefore, when defining new functions to work on an ADT, be
**      sure that The operator ids assigned to them lie outside this
**      range.  Operators will be different from functions in that they
**      will be known to the parser by symbolic constants.  No other
**      symbolic constants for operator ids will be know outside the
**      ADF.  Furthermore, only the parser is currently authorized to
**      "see" those constants defined here.
**
** History: $Log-for RCS$
**      03-feb-86 (thurston)
**          Initial creation.
**      03-mar-86 (thurston)
**          Put the  cast on #defines to please lint.  Now this
**	    file is dependent on the ADF.H header file.
**      26-mar-86 (thurston)
**	    Re-defined all of the symbolic constants to be consistent with
**	    the way ADF uses them.  (That is, the operations table is now
**	    in lexical order by operation name, therefore these constants
**	    have been given specific values.)
**	01-oct-86 (thurston)
**	    Added ADI_LIKE_OP and ADI_NLIKE_OP.
**	11-nov-86 (thurston)
**	    Added ADI_HEXIN_OP.
**	07-jan-87 (thurston)
**	    Removed ADI_HEXIN_OP since the parser is now handling hex constants
**	    right in the grammar.
**	09-mar-87 (thurston)
**	    Added ADI_ISNUL_OP, ADI_NONUL_OP, and ADI_CNTAL_OP.
**	08-apr-87 (thurston)
**	    Moved ADI_ANY_OP into this header file from an internal ADF header
**	    file for OPC to use.
**	28-may-87 (thurston)
**	    Added ADI_EXIST_OP.
**	18-jun-87 (thurston)
**	    Added ADI_NXIST_OP.
**	27-apr-88 (thurston)
**	    Added ADI_DEC_OP.
**	15-Feb-89 (andre)
**	    Added ADI_DBMSI_OP (previously removed from adfint.h)
**	03-mar-89 (andre)
**	    Added opcodes for the constant functions (previously removed from
**	    adfint.h)
**	20-may-89 (andre)
**	    moved ADI_CNT_OP from adfint.h so that PSF would see it.
**      03-dec-1992 (stevet)
**          Added ADI_SESSUSER_OP, ADI_INITUSER_OP and ADI_SYSUSER_OP
**          for FIPS functions.
**      10-jan-1993 (stevet)
**          Moved ADI_VARCH_OP, ADI_CHAR_OP, ADI_ASCII_OP and ADI_TEXT_OP
**          from adfint.h to here.
**      14-jun-1993 (stevet)
**          Added new operator ADI_NULJN_OP to handle nullable key join.
**	20-Apr-1999 (shero03)
**	    Add substring(c FROM n [FOR l])
**	6-jul-00 (inkdo01)
**	    Added random operators to make them visible in OPF.
**	18-dec-00 (inkdo01)
**	    Added aggregate operators to make them visible in OPF.
**      16-apr-2001 (stial01)
**          Added support for iitableinfo function
**      7-nov-01 (inkdo01)
**          Copied ADI_TRIM_OP (for ANSI support) and added ADI_POS_OP to make
**          them visible in PSF.
**	27-may-02 (inkdo01)
**	    Dropped random operators. Alternate solution was found that no longer
**	    requires them to be visible outside of ADF.
**      11-nov-2002 (stial01)
**          Added ADI_RESTAB_OP (b109101)
**	17-jul-2003 (toumi01)
**	    Add to PSF some flattening decisions.
**	    to make ADI_IFNUL_OP visible to PSF move #define from
**	    common/adf/hdr/adfint.h to common/hdr/hdr/adfops.h
**	26-nov-2003 (gupsh01)
**	    Added new operators for functions nchar(), nvarchar(), iiucs_to_utf8, 
**	    iiutf8_to_ucs().
**	21-jan-2004 (gupsh01)
**	    Modified operator id for nchar,ncharchar,iiucs_to_utf8,iiutf8_to_ucs
**	    so that they do not conflict with the ANSI char_length() etc.
**	30-june-05 (inkdo01)
**	    Copied "float8" from internal ADF for avg ==> sum/count transform.
**	20-dec-2006 (dougi)
**	    Added date/time registers so they can be used as column defaults.
**	15-jan_2007 (dougi)
**	    Copied ADI+DPART_OP to allow "extract" implementation in parser.
**	07-Mar-2007 (kiria01) b117768
**	    Moved ADI_CNCAT_OP from adfint.h as it is also accessible
**	    as infix operator:  strA || atrB  ==  CONCAT(strA, strB)
**	21-Mar-2007 (kiria01) b117892
**	    Consolidated operator/function code definition into one table macro.
**	02-Apr-2007 b117892
**	    Added main line symbols not present in 9.0.4.
**	2-apr-2007 (dougi)
**	    Added ROUND for round() function.
**	13-apr-2007 (dougi)
**	    Added ceil(), floor(), trunc(), chr(), ltrim() and rtrim() 
**	    functions for various partner interfaces.
**	16-apr-2007 (dougi)
**	    Then add lpad(), rpad() and replace().
**	2-may-2007 (dougi)
**	    And tan(), acos(), asin(), pi() and sign().
**      22-Aug-2007 (kiria01) b118999
**          Added byteextract
**	26-Dec-2007 (kiria01) SIR119658
**	    Added 'is integer/decimal/float' operators.
**      10-sep-2008 (gupsh01,stial01)
**          Added unorm operator
**	27-Oct-2008 (kiria01) SIR120473
**	    Added patcomp
**      17-Dec-2008 (macde01)
**          Added point,x, and y operators.
**	16-jan-2009 (dougi)
**	    Added rowid() operator (for X100 only).
**	28-Feb-2009 (thich01)
**	    Added operators for the spatial types
**	13-Mar-2009 (thich01)
**	     Added WKT and WKB operators to support asText and asBinary
**	12-Mar-2009 (kira01) SIR121788
**	    Added long_nvarchar
**	22-Apr-2009 (Marty Bowes) SIR 121969
**          Add generate_digit() and validate_digit() for the generation
**          and validation of checksum values.
**      01-Aug-2009 (martin bowes) SIR122320
**          Added soundex_dm (Daitch-Mokotoff soundex)
**	25-Aug-2009 (troal01)
**	    Added GEOMNAME and GEOMDIMEN
**	9-sep-2009 (stephenb)
**	    Add last_identity
**      25-sep-2009 (joea)
**          Add ISFALSE, NOFALSE, ISTRUE, NOTRUE, ISUNKN and NOUNKN.
**      23-oct-2009 (joea)
**          Add BOO (boolean).
**	12-Nov-2009 (kschendel) SIR 122882
**	    Add iicmptversion.
**	14-Nov-2009 (kiria01) SIR 122894
**	    Added GREATEST and LEAST
**	07-Dec-2009 (drewsturgiss)
**	    Added NVL and NVL2
**      09-Mar-2010 (thich01)
**          Add union and change geom to perimeter to stay consistent with
**          other spatial operators.
**	18-Mar-2010 (kiria01) b123438
**	    Added SINGLETON aggregate for scalar sub-query support.
**	25-Mar-2010 (toumi01) SIR 122403
**	    Add AES_DECRYPT and AES_ENCRYPT.
**	xx-Apr-2010 (iwawong)
**		Added issimple isempty x y numpoints
**		for the coordinate space.
**	14-Jul-2010 (kschendel) b123104
**	    Add ii_true and ii_false to solve outer join constant folding bug.
**	28-Jul-2010 (kiria01) b124142
**	    Added SINGLECHK
**	15-Jul-2010 (smeke01) b123950
**	    Add operator id ADI_DATEHASH_OP / norm_date_hash.
**	    Also added intervening operator ids from main codeline to keep defines
**	    in step.
**	09-Dec-2010 (kiria01) SIR 124826
**	    Added ASSERT
**      10-Feb-2011 (hanal04)
**          Add the fill ADI_<value>_ID to the comment for each operator.
**          This makes it easier to find the defined values when researching
**          a bug.
**	04-May-2011 (kiria01) SIR 125342
**	    Added InitCap
**	27-may-2011 (dougi) m1961
**	    Added VWDIFF, VWREDIFF, VWROWNUM, VWROWID all internal functions 
**	    used by cross compiler for VW and not defined in adg tables.
**	04-Jun-2011 (kiria01) SIR 125419
**	    Add REPEAT function
**	5-june-2011 (dougi) Code sprint
**	    Add srand & rand functions.
**      07-Jun-2011 (coomi01) b125400
**          Add DPART_DECIMAL function
**	11-Jul-2011 (kiria01) b125530
**	    Slight rearrangement of spatial operators to align with explicit
**	    coercions better.
**	09-Aug-2011 (kiria01) b125634
**	    Added distinct substr() function.
**	26-Aug-2011 (kiria01) SIR 125693
**	    Add prototypes for supporting date_format/time_format, dayofweek,
**	    dayofmonth, dayofyear/doy, unix_timestamp, from_unixtime, last_day
**	    and months_between.
**	01-Nov-2011 (kiria01) b125852
**	    Added distinct INGRESDATE
**/

/*
**  Defines of other constants.
*/

/*: ADF Operation IDs
**      These are symbolic constants to use for the ADF operation codes,
**      otherwise known as op-id's.
**	The table macro serves to keep the definitions in one place, ensures
**	that they are kept contiguous and provides a means of extracting the
**	symbol name for logging.
**
**	DANGER AND WARNING
**
**	adgfitab.roc is sorted (by hand == human) based upon these operator ids.
**	Changing these operator ids requires that adgfitab.roc be resorted, and
**	that those changes be carried back to adgoptab.roc as well.
**
**	Best Advice:	Don't change the operator id's
*/
/*: PSF and OPF Visible Op IDs
**      These are the symbolic constants defined for the operator ids of the
**      operators known to the ADF.  The parser and optimizer are currently
**      the only facilities authorized to use these symbolic constants outside
**	of ADF.
*/


#define ADI_OPS_MACRO \
_DEFINE(ILLEGAL,        -1  /* sentinel value; not a legal op               */)\
_DEFINE(NE,              0  /* !=                      ADI_NE_OP            */)\
_DEFINE(MUL,             1  /* *                       ADI_MUL_OP           */)\
_DEFINE(POW,             2  /* **                      ADI_POW_OP           */)\
_DEFINE(ADD,             3  /* +                       ADI_ADD_OP           */)\
_DEFINE(SUB,             4  /* -                       ADI_SUB_OP           */)\
_DEFINE(DIV,             5  /* /                       ADI_DIV_OP           */)\
_DEFINE(LT,              6  /* <                       ADI_LT_OP            */)\
_DEFINE(LE,              7  /* <=                      ADI_LE_OP            */)\
_DEFINE(EQ,              8  /* =                       ADI_EQ_OP            */)\
_DEFINE(GT,              9  /* >                       ADI_GT_OP            */)\
_DEFINE(GE,             10  /* >=                      ADI_GE_OP            */)\
_DEFINE(BNTIM,          11  /* _bintim                 ADI_BNTIM_OP         */)\
_DEFINE(BIOC,           12  /* _bio_cnt                ADI_BIOC_OP          */)\
_DEFINE(CHRD,           13  /* _cache_read             ADI_CHRD_OP          */)\
_DEFINE(CHREQ,          14  /* _cache_req              ADI_CHREQ_OP         */)\
_DEFINE(CHRRD,          15  /* _cache_rread            ADI_CHRRD_OP         */)\
_DEFINE(CHSIZ,          16  /* _cache_size             ADI_CHSIZ_OP         */)\
_DEFINE(CHUSD,          17  /* _cache_used             ADI_CHUSD_OP         */)\
_DEFINE(CHWR,           18  /* _cache_write            ADI_CHWR_OP          */)\
_DEFINE(CPUMS,          19  /* _cpu_ms                 ADI_CPUMS_OP         */)\
_DEFINE(BDATE,          20  /* _date                   ADI_BDATE_OP         */)\
_DEFINE(DIOC,           21  /* _dio_cnt                ADI_DIOC_OP          */)\
_DEFINE(ETSEC,          22  /* _et_sec                 ADI_ETSEC_OP         */)\
_DEFINE(PFLTC,          23  /* _pfault_cnt             ADI_PFLTC_OP         */)\
_DEFINE(PHPAG,          24  /* _phys_page              ADI_PHPAG_OP         */)\
_DEFINE(BTIME,          25  /* _time                   ADI_BTIME_OP         */)\
_DEFINE(VERSN,          26  /* _version                ADI_VERSN_OP         */)\
_DEFINE(WSPAG,          27  /* _ws_page                ADI_WSPAG_OP         */)\
_DEFINE(ABS,            28  /* abs                     ADI_ABS_OP           */)\
_DEFINE(ANY,            29  /* any                     ADI_ANY_OP           */)\
_DEFINE(ASCII,          30  /* ascii or c              ADI_ASCII_OP         */)\
_DEFINE(ATAN,           31  /* atan                    ADI_ATAN_OP          */)\
_DEFINE(AVG,            32  /* avg                     ADI_AVG_OP           */)\
_DEFINE(DBMSI,          33  /* dbmsinfo                ADI_DBMSI_OP         */)\
_DEFINE(CHAR,           34  /* char                    ADI_CHAR_OP          */)\
_DEFINE(CNCAT,          35  /* concat                  ADI_CNCAT_OP         */)\
_DEFINE(TYNAM,          36  /* typename                ADI_TYNAM_OP         */)\
_DEFINE(COS,            37  /* cos                     ADI_COS_OP           */)\
_DEFINE(CNT,            38  /* count                   ADI_CNT_OP           */)\
_DEFINE(CNTAL,          39  /* count*                  ADI_CNTAL_OP         */)\
_DEFINE(DATE,           40  /* date                    ADI_DATE_OP          */)\
_DEFINE(DPART,          41  /* date_part               ADI_DPART_OP         */)\
_DEFINE(DTRUN,          42  /* date_trunc              ADI_DTRUN_OP         */)\
_DEFINE(DBA,            43  /* dba                     ADI_DBA_OP           */)\
_DEFINE(DOW,            44  /* dow                     ADI_DOW_OP           */)\
_DEFINE(ISNUL,          45  /* is null                 ADI_ISNUL_OP         */)\
_DEFINE(NONUL,          46  /* is not null             ADI_NONUL_OP         */)\
_DEFINE(EXP,            47  /* exp                     ADI_EXP_OP           */)\
_DEFINE(F4,             48  /* float4                  ADI_F4_OP            */)\
_DEFINE(F8,             49  /* float8                  ADI_F8_OP            */)\
_DEFINE(I1,             50  /* int1                    ADI_I1_OP            */)\
_DEFINE(I2,             51  /* int2                    ADI_I2_OP            */)\
_DEFINE(I4,             52  /* int4                    ADI_I4_OP            */)\
_DEFINE(INTVL,          53  /* interval                ADI_INTVL_OP         */)\
_DEFINE(LEFT,           54  /* left                    ADI_LEFT_OP          */)\
_DEFINE(LEN,            55  /* length                  ADI_LEN_OP           */)\
_DEFINE(LOC,            56  /* locate                  ADI_LOC_OP           */)\
_DEFINE(LOG,            57  /* log                     ADI_LOG_OP           */)\
_DEFINE(LOWER,          58  /* lowercase               ADI_LOWER_OP         */)\
_DEFINE(MAX,            59  /* max                     ADI_MAX_OP           */)\
_DEFINE(MIN,            60  /* min                     ADI_MIN_OP           */)\
_DEFINE(MOD,            61  /* mod                     ADI_MOD_OP           */)\
_DEFINE(MONEY,          62  /* money                   ADI_MONEY_OP         */)\
_DEFINE(PAD,            63  /* pad                     ADI_PAD_OP           */)\
_DEFINE(RIGHT,          64  /* right                   ADI_RIGHT_OP         */)\
_DEFINE(SHIFT,          65  /* shift                   ADI_SHIFT_OP         */)\
_DEFINE(SIN,            66  /* sin                     ADI_SIN_OP           */)\
_DEFINE(SIZE,           67  /* size                    ADI_SIZE_OP          */)\
_DEFINE(SQRT,           68  /* sqrt                    ADI_SQRT_OP          */)\
_DEFINE(SQZ,            69  /* squeeze                 ADI_SQZ_OP           */)\
_DEFINE(SUM,            70  /* sum                     ADI_SUM_OP           */)\
_DEFINE(EXIST,          71  /* exists                  ADI_EXIST_OP         */)\
_DEFINE(TEXT,           72  /* text or vchar           ADI_TEXT_OP          */)\
_DEFINE(TRIM,           73  /* trim                    ADI_TRIM_OP          */)\
_DEFINE(PLUS,           74  /* u+                      ADI_PLUS_OP          */)\
_DEFINE(MINUS,          75  /* u-                      ADI_MINUS_OP         */)\
_DEFINE(UPPER,          76  /* uppercase               ADI_UPPER_OP         */)\
_DEFINE(USRLN,          77  /* iiuserlen               ADI_USRLN_OP         */)\
_DEFINE(USRNM,          78  /* username                ADI_USRNM_OP         */)\
_DEFINE(VARCH,          79  /* varchar                 ADI_VARCH_OP         */)\
_DEFINE(LIKE,           80  /* like                    ADI_LIKE_OP          */)\
_DEFINE(NLIKE,          81  /* not like                ADI_NLIKE_OP         */)\
_DEFINE(HEX,            82  /* hex                     ADI_HEX_OP           */)\
_DEFINE(IFNUL,          83  /* ifnull                  ADI_IFNUL_OP         */)\
_DEFINE(STRUC,          84  /* iistructure             ADI_STRUC_OP         */)\
_DEFINE(DGMT,           85  /* date_gmt                ADI_DGMT_OP          */)\
_DEFINE(CHA12,          86  /* iichar12                ADI_CHA12_OP         */)\
_DEFINE(CHEXT,          87  /* charextract             ADI_CHEXT_OP         */)\
_DEFINE(NTRMT,          88  /* ii_notrm_txt            ADI_NTRMT_OP         */)\
_DEFINE(NTRMV,          89  /* ii_notrm_vch            ADI_NTRMV_OP         */)\
_DEFINE(NXIST,          90  /* not exists              ADI_NXIST_OP         */)\
_DEFINE(XYZZY,          91  /* xyzzy                   ADI_XYZZY_OP         */)\
_DEFINE(HXINT,          92  /* iihexint                ADI_HXINT_OP         */)\
_DEFINE(TB2DI,          93  /* ii_tabid_di             ADI_TB2DI_OP         */)\
_DEFINE(DI2TB,          94  /* ii_di_tabid             ADI_DI2TB_OP         */)\
_DEFINE(DVDSC,          95  /* ii_dv_desc              ADI_DVDSC_OP         */)\
_DEFINE(DEC,            96  /* decimal                 ADI_DEC_OP           */)\
_DEFINE(ETYPE,          97  /* ii_ext_type             ADI_ETYPE_OP         */)\
_DEFINE(ELENGTH,        98  /* ii_ext_length           ADI_ELENGTH_OP       */)\
_DEFINE(LOGKEY,         99  /* object_key              ADI_LOGKEY_OP        */)\
_DEFINE(TABKEY,        100  /* table_key               ADI_TABKEY_OP        */)\
_DEFINE(101,           101  /*   Unused                                   */)\
_DEFINE(IFTRUE,        102  /* ii_iftrue               ADI_IFTRUE_OP        */)\
_DEFINE(RESTAB,        103  /* resolve_table           ADI_RESTAB_OP        */)\
_DEFINE(GMTSTAMP,      104  /* gmt_timestamp           ADI_GMTSTAMP_OP      */)\
_DEFINE(CPNDMP,        105  /* ii_cpn_dump             ADI_CPNDMP_OP        */)\
_DEFINE(BIT,           106  /* bit                     ADI_BIT_OP           */)\
_DEFINE(VBIT,          107  /* varbit                  ADI_VBIT_OP          */)\
_DEFINE(ROWCNT,        108  /* ii_row_count            ADI_ROWCNT_OP        */)\
_DEFINE(SESSUSER,      109  /* session_user            ADI_SESSUSER_OP      */)\
_DEFINE(SYSUSER,       110  /* system_user             ADI_SYSUSER_OP       */)\
_DEFINE(INITUSER,      111  /* initial_user            ADI_INITUSER_OP      */)\
_DEFINE(ALLOCPG,       112  /* iitotal_allocated_pages ADI_ALLOCPG_OP       */)\
_DEFINE(OVERPG,        113  /* iitotal_overflow_pages  ADI_OVERPG_OP        */)\
_DEFINE(BYTE,          114  /* byte                    ADI_BYTE_OP          */)\
_DEFINE(VBYTE,         115  /* varbyte                 ADI_VBYTE_OP         */)\
_DEFINE(LOLK,          116  /* ii_lolk(large obj)      ADI_LOLK_OP          */)\
_DEFINE(NULJN,         117  /* a = a or null=null      ADI_NULJN_OP         */)\
_DEFINE(PTYPE,         118  /* ii_permit_type(int)     ADI_PTYPE_OP         */)\
_DEFINE(SOUNDEX,       119  /* soundex(char)           ADI_SOUNDEX_OP       */)\
_DEFINE(BDATE4,        120  /* _date4(int)             ADI_BDATE4_OP        */)\
_DEFINE(INTEXT,        121  /* intextract              ADI_INTEXT_OP        */)\
_DEFINE(ISDST,         122  /* isdst()                 ADI_ISDST_OP         */)\
_DEFINE(CURDATE,       123  /* CURRENT_DATE            ADI_CURDATE_OP       */)\
_DEFINE(CURTIME,       124  /* CURRENT_TIME            ADI_CURTIME_OP       */)\
_DEFINE(CURTMSTMP,     125  /* CURRENT_TIMESTAMP       ADI_CURTMSTMP_OP     */)\
_DEFINE(LCLTIME,       126  /* LOCAL_TIME              ADI_LCLTIME_OP       */)\
_DEFINE(LCLTMSTMP,     127  /* LOCAL_TIMESTAMP         ADI_LCLTMSTMP_OP     */)\
_DEFINE(SESSION_PRIV,  128  /* session_priv            ADI_SESSION_PRIV_OP  */)\
_DEFINE(IITBLSTAT,     129  /* iitblstat               ADI_IITBLSTAT_OP     */)\
_DEFINE(130,           130  /*   Unused                                     */)\
_DEFINE(LVARCH,        131  /* long_varchar            ADI_LVARCH_OP        */)\
_DEFINE(LBYTE,         132  /* long_byte               ADI_LBYTE_OP         */)\
_DEFINE(BIT_ADD,       133  /* bit_add                 ADI_BIT_ADD_OP       */)\
_DEFINE(BIT_AND,       134  /* bit_and                 ADI_BIT_AND_OP       */)\
_DEFINE(BIT_NOT,       135  /* bit_not                 ADI_BIT_NOT_OP       */)\
_DEFINE(BIT_OR,        136  /* bit_or                  ADI_BIT_OR_OP        */)\
_DEFINE(BIT_XOR,       137  /* bit_xor                 ADI_BIT_XOR_OP       */)\
_DEFINE(IPADDR,        138  /* ii_ipaddr(c)            ADI_IPADDR_OP        */)\
_DEFINE(HASH,          139  /* hash(all)               ADI_HASH_OP          */)\
_DEFINE(RANDOMF,       140  /* randomf                 ADI_RANDOMF_OP       */)\
_DEFINE(RANDOM,        141  /* random                  ADI_RANDOM_OP        */)\
_DEFINE(UUID_CREATE,   142  /* uuid_create()           ADI_UUID_CREATE_OP   */)\
_DEFINE(UUID_TO_CHAR,  143  /* uuid_to_char(u)         ADI_UUID_TO_CHAR_OP  */)\
_DEFINE(UUID_FROM_CHAR,144  /* uuid_from_char(c)       ADI_UUID_FROM_CHAR_OP*/)\
_DEFINE(UUID_COMPARE,  145  /* uuid_compare(u,u)       ADI_UUID_COMPARE_OP  */)\
_DEFINE(SUBSTRING,     146  /* substring               ADI_SUBSTRING_OP     */)\
_DEFINE(UNHEX,         147  /* unhex                   ADI_UNHEX_OP         */)\
_DEFINE(CORR,          148  /* corr()                  ADI_CORR_OP          */)\
_DEFINE(COVAR_POP,     149  /* covar_pop()             ADI_COVAR_POP_OP     */)\
_DEFINE(COVAR_SAMP,    150  /* covar_samp()            ADI_COVAR_SAMP_OP    */)\
_DEFINE(REGR_AVGX,     151  /* regr_avgx()             ADI_REGR_AVGX_OP     */)\
_DEFINE(REGR_AVGY,     152  /* regr_avgy()             ADI_REGR_AVGY_OP     */)\
_DEFINE(REGR_COUNT,    153  /* regr_count()            ADI_REGR_COUNT_OP    */)\
_DEFINE(REGR_INTERCEPT,154  /* regr_intercept()        ADI_REGR_INTERCEPT_OP*/)\
_DEFINE(REGR_R2,       155  /* regr_r2()               ADI_REGR_R2_OP       */)\
_DEFINE(REGR_SLOPE,    156  /* regr_slope()            ADI_REGR_SLOPE_OP    */)\
_DEFINE(REGR_SXX,      157  /* regr_sxx()              ADI_REGR_SXX_OP      */)\
_DEFINE(REGR_SXY,      158  /* regr_sxy()              ADI_REGR_SXY_OP      */)\
_DEFINE(REGR_SYY,      159  /* regr_syy()              ADI_REGR_SYY_OP      */)\
_DEFINE(STDDEV_POP,    160  /* stddev_pop()            ADI_STDDEV_POP_OP    */)\
_DEFINE(STDDEV_SAMP,   161  /* stddev_samp()           ADI_STDDEV_SAMP_OP   */)\
_DEFINE(VAR_POP,       162  /* var_pop()               ADI_VAR_POP_OP       */)\
_DEFINE(VAR_SAMP,      163  /* var_samp()              ADI_VAR_SAMP_OP      */)\
_DEFINE(TABLEINFO,     164  /* iitableinfo()           ADI_TABLEINFO_OP     */)\
_DEFINE(POS,           165  /* ANSI position()         ADI_POS_OP           */)\
_DEFINE(ATRIM,         166  /* ANSI trim()             ADI_ATRIM_OP         */)\
_DEFINE(CHLEN,         167  /* ANSI char_length()      ADI_CHLEN_OP         */)\
_DEFINE(OCLEN,         168  /* ANSI octet_length()     ADI_OCLEN_OP         */)\
_DEFINE(BLEN,          169  /* ANSI bit_length()       ADI_BLEN_OP          */)\
_DEFINE(NCHAR,         170  /* nchar()                 ADI_NCHAR_OP         */)\
_DEFINE(NVCHAR,        171  /* nvarchar()              ADI_NVCHAR_OP        */)\
_DEFINE(UTF16TOUTF8,   172  /* ii_utf16_to_utf8()      ADI_UTF16TOUTF8_OP   */)\
_DEFINE(UTF8TOUTF16,   173  /* ii_utf8_to_utf16()      ADI_UTF8TOUTF16_OP   */)\
_DEFINE(I8,            174  /* int8                    ADI_I8_OP            */)\
_DEFINE(COWGT,         175  /* collation_weight()      ADI_COWGT_OP         */)\
_DEFINE(ADTE,          176  /* ansidate()              ADI_ADTE_OP          */)\
_DEFINE(TMWO,          177  /* time_wo_tz()            ADI_TMWO_OP          */)\
_DEFINE(TMW,           178  /* time_with_tz()          ADI_TMW_OP           */)\
_DEFINE(TME,           179  /* time_local()            ADI_TME_OP           */)\
_DEFINE(TSWO,          180  /* timestamp_wo_tz()       ADI_TSWO_OP          */)\
_DEFINE(TSW,           181  /* timestamp_with_tz()     ADI_TSW_OP           */)\
_DEFINE(TSTMP,         182  /* timestamp_local()       ADI_TSTMP_OP         */)\
_DEFINE(INDS,          183  /* interval_dtos()         ADI_INDS_OP          */)\
_DEFINE(INYM,          184  /* interval_ytom()         ADI_INYM_OP          */)\
_DEFINE(YPART,         185  /* year()                  ADI_YPART_OP         */)\
_DEFINE(QPART,         186  /* quarter()               ADI_QPART_OP         */)\
_DEFINE(MPART,         187  /* month()                 ADI_MPART_OP         */)\
_DEFINE(WPART,         188  /* week()                  ADI_WPART_OP         */)\
_DEFINE(WIPART,        189  /* week_iso()              ADI_WIPART_OP        */)\
_DEFINE(DYPART,        190  /* day()                   ADI_DYPART_OP        */)\
_DEFINE(HPART,         191  /* hour()                  ADI_HPART_OP         */)\
_DEFINE(MIPART,        192  /* minute()                ADI_MIPART_OP        */)\
_DEFINE(SPART,         193  /* second()                ADI_SPART_OP         */)\
_DEFINE(MSPART,        194  /* microsecond()           ADI_MSPART_OP        */)\
_DEFINE(NPART,         195  /* nanosecond()            ADI_NPART_OP         */)\
_DEFINE(ROUND,         196  /* round()                 ADI_ROUND_OP         */)\
_DEFINE(TRUNC,         197  /* trunc[ate]()            ADI_TRUNC_OP         */)\
_DEFINE(CEIL,          198  /* ceil[ing]()             ADI_CEIL_OP          */)\
_DEFINE(FLOOR,         199  /* floor()                 ADI_FLOOR_OP         */)\
_DEFINE(CHR,           200  /* chr()                   ADI_CHR_OP           */)\
_DEFINE(LTRIM,         201  /* ltrim()                 ADI_LTRIM_OP         */)\
_DEFINE(LPAD,          202  /* lpad()                  ADI_LPAD_OP          */)\
_DEFINE(RPAD,          203  /* rpad()                  ADI_RPAD_OP          */)\
_DEFINE(REPL,          204  /* replace()               ADI_REPL_OP          */)\
_DEFINE(ACOS,          205  /* acos()                  ADI_ACOS_OP          */)\
_DEFINE(ASIN,          206  /* asin()                  ADI_ASIN_OP          */)\
_DEFINE(TAN,           207  /* tan()                   ADI_TAN_OP           */)\
_DEFINE(PI,            208  /* pi()                    ADI_PI_OP            */)\
_DEFINE(SIGN,          209  /* sign()                  ADI_SIGN_OP          */)\
_DEFINE(ATAN2,         210  /* atan2()                 ADI_ATAN2_OP         */)\
_DEFINE(BYTEXT,        211  /* byteextract()           ADI_BYTEXT_OP        */)\
_DEFINE(ISINT,         212  /* is integer              ADI_ISINT_OP         */)\
_DEFINE(NOINT,         213  /* is not integer          ADI_NOINT_OP         */)\
_DEFINE(ISDEC,         214  /* is decimal              ADI_ISDEC_OP         */)\
_DEFINE(NODEC,         215  /* is not decimal          ADI_NODEC_OP         */)\
_DEFINE(ISFLT,         216  /* is float                ADI_ISFLT_OP         */)\
_DEFINE(NOFLT,         217  /* is not float            ADI_NOFLT_OP         */)\
_DEFINE(NUMNORM,       218  /* numeric norm            ADI_NUMNORM_OP       */)\
_DEFINE(UNORM,         219  /* unorm                   ADI_UNORM_OP         */)\
_DEFINE(PATCOMP,       220  /* patcomp                 ADI_PATCOMP_OP       */)\
_DEFINE(LNVCHR,        221  /* long_nvarchar           ADI_LNVCHR_OP        */)\
_DEFINE(ISFALSE,       222  /* is false                ADI_ISFALSE_OP       */)\
_DEFINE(NOFALSE,       223  /* is not false            ADI_NOFALSE_OP       */)\
_DEFINE(ISTRUE,        224  /* is true                 ADI_ISTRUE_OP        */)\
_DEFINE(NOTRUE,        225  /* is not true             ADI_NOTRUE_OP        */)\
_DEFINE(ISUNKN,        226  /* is unknown              ADI_ISUNKN_OP        */)\
_DEFINE(NOUNKN,        227  /* is not unknown          ADI_NOUNKN_OP        */)\
_DEFINE(LASTID,	       228  /* last_identity	       ADI_LASTID_OP        */)\
_DEFINE(BOO,           229  /* boolean                 ADI_BOO_OP           */)\
_DEFINE(CMPTVER,       230  /* iicmptversion           ADI_CMPTVER_OP       */)\
_DEFINE(GREATEST,      231  /* greatest                ADI_GREATEST_OP      */)\
_DEFINE(LEAST,         232  /* least                   ADI_LEAST_OP         */)\
_DEFINE(GREATER,       233  /* greater                 ADI_GREATER_OP       */)\
_DEFINE(LESSER,        234  /* lesser                  ADI_LESSER_OP        */)\
_DEFINE(NVL2,          235  /* NVL2                    ADI_NVL2_OP          */)\
_DEFINE(GENERATEDIGIT, 236  /* generate_digit()        ADI_GENERATEDIGIT_OP */)\
_DEFINE(VALIDATEDIGIT, 237  /* validate_digit()        ADI_VALIDATEDIGIT_OP */)\
_DEFINE(SINGLETON,     238  /* singleton               ADI_SINGLETON_OP     */)\
_DEFINE(AES_DECRYPT,   239  /* aes_decrypt             ADI_AES_DECRYPT_OP   */)\
_DEFINE(AES_ENCRYPT,   240  /* aes_encrypt             ADI_AES_ENCRYPT_OP   */)\
_DEFINE(SOUNDEX_DM,    241  /* soundex_dm              ADI_SOUNDEX_DM_OP    */)\
_DEFINE(IIFALSE,       242  /* ii_false                ADI_IIFALSE_OP       */)\
_DEFINE(IITRUE,        243  /* ii_true                 ADI_IITRUE_OP        */)\
_DEFINE(SINGLECHK,     244  /* singlechk               ADI_SINGLECHK_OP     */)\
_DEFINE(DATEHASH,      245  /* norm_date_hash          ADI_DATEHASH_OP      */)\
_DEFINE(POINT,         246  /* point()                 ADI_POINT_OP         */)\
_DEFINE(X,             247  /* x(point)                ADI_X_OP             */)\
_DEFINE(Y,             248  /* y(point)                ADI_Y_OP             */)\
_DEFINE(LINE,          249  /* line()                  ADI_LINE_OP          */)\
_DEFINE(POLY,          250  /* poly()                  ADI_POLY_OP          */)\
_DEFINE(MPOINT,        251  /* mpoint()                ADI_MPOINT_OP        */)\
_DEFINE(MLINE,         252  /* mline()                 ADI_MLINE_OP         */)\
_DEFINE(MPOLY,         253  /* mpoly()                 ADI_MPOLY_OP         */)\
_DEFINE(GEOM,          254  /* geom()                  ADI_GEOM_OP          */)\
_DEFINE(TEXTR_GEOM,    255  /* asTextRound(geom)       ADI_TEXTR_GEOM_OP    */)\
_DEFINE(NBR,           256  /* Spatial nbr op          ADI_NBR_OP           */)\
_DEFINE(HILBERT,       257  /* Spatial hilbert op      ADI_HILBERT_OP       */)\
_DEFINE(OVERLAPS,      258  /* overlaps(geom, geom     ADI_OVERLAPS_OP      */)\
_DEFINE(INSIDE,        259  /* inside(geom, geom)      ADI_INSIDE_OP        */)\
_DEFINE(PERIMETER,     260  /* perimeter(geom)         ADI_PERIMETER_OP     */)\
_DEFINE(GEOMNAME,      261  /* iigeomname()            ADI_GEOMNAME_OP      */)\
_DEFINE(GEOMDIMEN,     262  /* iigeomdimensions()      ADI_GEOMDIMEN_OP     */)\
_DEFINE(DIMENSION,     263  /* dimension(geom)         ADI_DIMENSION_OP     */)\
_DEFINE(GEOMETRY_TYPE, 264  /* geometrytype(geom)      ADI_GEOMETRY_TYPE_OP */)\
_DEFINE(BOUNDARY,      265  /* boundary(geom)          ADI_BOUNDARY_OP      */)\
_DEFINE(ENVELOPE,      266  /* envelope(geom)          ADI_ENVELOPE_OP      */)\
_DEFINE(EQUALS,	       267  /* equals(geom, geom)      ADI_EQUALS_OP        */)\
_DEFINE(DISJOINT,      268  /* disjoint(geom, geom)    ADI_DISJOINT_OP      */)\
_DEFINE(INTERSECTS,    269  /* intersects(geom, geom)  ADI_INTERSECTS_OP    */)\
_DEFINE(TOUCHES,       270  /* touches(geom, geom)     ADI_TOUCHES_OP       */)\
_DEFINE(CROSSES,       271  /* crosses(geom, geom)     ADI_CROSSES_OP       */)\
_DEFINE(WITHIN,	       272  /* within(geom, geom)      ADI_WITHIN_OP        */)\
_DEFINE(CONTAINS,      273  /* contains(geom, geom)    ADI_CONTAINS_OP      */)\
_DEFINE(RELATE,        274  /* relate(geom,geom,char)  ADI_RELATE_OP        */)\
_DEFINE(DISTANCE,      275  /* distance(geom, geom)    ADI_DISTANCE_OP      */)\
_DEFINE(INTERSECTION,  276  /* intersection(geom,geom) ADI_INTERSECTION_OP  */)\
_DEFINE(DIFFERENCE,    277  /* difference(geom, geom)  ADI_DIFFERENCE_OP    */)\
_DEFINE(SYMDIFF,       278  /* symdifference(geom,geom) ADI_SYMDIFF_OP      */)\
_DEFINE(BUFFER,	       279  /* buffer()                ADI_BUFFER_OP        */)\
_DEFINE(CONVEXHULL,    280  /* convexhull(geom)        ADI_CONVEXHULL_OP    */)\
_DEFINE(POINTN,	       281  /* pointn(linestring)      ADI_POINTN_OP        */)\
_DEFINE(STARTPOINT,    282  /* startpoint(curve)       ADI_STARTPOINT_OP    */)\
_DEFINE(ENDPOINT,      283  /* endpoint(curve)         ADI_ENDPOINT_OP      */)\
_DEFINE(ISCLOSED,      284  /* isclosed(curve)         ADI_ISCLOSED_OP      */)\
_DEFINE(ISRING,	       285  /* isring(curve)           ADI_ISRING_OP        */)\
_DEFINE(STLENGTH,      286  /* st_length(curve)        ADI_STLENGTH_OP      */)\
_DEFINE(CENTROID,      287  /* centroid(surface)       ADI_CENTROID_OP      */)\
_DEFINE(PNTONSURFACE,  288  /* pointonsurface(surface) ADI_PNTONSURFACE_OP  */)\
_DEFINE(AREA,  	       289  /* area(surface)           ADI_AREA_OP          */)\
_DEFINE(EXTERIORRING,  290  /* exteriorring(polygon)   ADI_EXTERIORRING_OP  */)\
_DEFINE(NINTERIORRING, 291  /* numinteriorring(polygon) ADI_NINTERIORRING_OP*/)\
_DEFINE(INTERIORRINGN, 292  /* interiorringn(polygon,i) ADI_INTERIORRINGN_OP*/)\
_DEFINE(NUMGEOMETRIES, 293  /* numgeometries(geomcoll) ADI_NUMGEOMETRIES_OP */)\
_DEFINE(GEOMETRYN,     294  /* geometryn(geomcoll)     ADI_GEOMETRYN_OP     */)\
_DEFINE(ISEMPTY,       295  /* isempty(geom)           ADI_ISEMPTY_OP       */)\
_DEFINE(ISSIMPLE,      296  /* issimple(geom)          ADI_ISSIMPLE_OP      */)\
_DEFINE(UNION,         297  /* union(geom, geom)       ADI_UNION_OP         */)\
_DEFINE(SRID,          298  /* srid(geom)              ADI_SRID_OP          */)\
_DEFINE(NUMPOINTS,     299  /* numpoints(linestring)   ADI_NUMPOINTS_OP     */)\
_DEFINE(TRANSFORM,     300  /* transform(geom)         ADI_TRANSFORM_OP     */)\
_DEFINE(GEOMRAW,       301  /* AsTextRaw(geom)         ADI_GEOMRAW_OP       */)\
_DEFINE(GEOMC,         302  /* GeomColl operators      ADI_GEOMC_OP         */)\
_DEFINE(ASSERT,        303  /* ASSERT                  ADI_ASSERT_OP        */)\
_DEFINE(INITCAP,       304  /* InitCap                 ADI_INITCAP_OP       */)\
_DEFINE(REPEAT,        305  /* REPEAT                  ADI_REPEAT_OP        */)\
_DEFINE(RANK,	       306  /* rank		       ADI_RANK_OP	    */)\
_DEFINE(DRANK,	       307  /* dense_rank   	       ADI_DRANK_OP	    */)\
_DEFINE(ROWNUM,	       308  /* row_number   	       ADI_ROWNUM_OP	    */)\
_DEFINE(PRANK,	       309  /* percent_rank   	       ADI_PRANK_OP	    */)\
_DEFINE(CDIST,	       310  /* cume_dist   	       ADI_CDIST_OP	    */)\
_DEFINE(VWDIFF,	       311  /* VW internal - diff()    ADI_VWDIFF_OP	    */)\
_DEFINE(VWREDIFF,      312  /* VW internal - rediff()  ADI_VWREDIFF_OP	    */)\
_DEFINE(VWROWNUM,      313  /* VW internal - rownum()  ADI_VWROWNUM_OP	    */)\
_DEFINE(VWROWID,       314  /* VW internal - rowid()   ADI_VWROWID_OP	    */)\
_DEFINE(SRAND,	       315  /* SRAND		       ADI_SRAND_OP	    */)\
_DEFINE(RAND,	       316  /* RAND		       ADI_RAND_OP	    */)\
_DEFINE(DPART_DECIMAL, 317  /* date_part_decimal       ADI_DPART_DECIMAL_OP */)\
_DEFINE(NTILE,	       318  /* ntile(int)  	       ADI_NTILE_OP	    */)\
_DEFINE(SUBSTR,	       319  /* substr(s,i [,i])        ADI_SUBSTR_OP	    */)\
_DEFINE(DATEFMT,       320  /* date_format(date,str)   ADI_DATEFMT_OP       */)\
_DEFINE(DAYOFWK,       321  /* dayofweek               ADI_DAYOFWK_OP       */)\
_DEFINE(DAYOFMTH,      322  /* dayofmonth              ADI_DAYOFMTH_OP      */)\
_DEFINE(DAYOFYR,       323  /* dayofyear/doy           ADI_DAYOFYR_OP       */)\
_DEFINE(UNXTMSTMP,     324  /* unix_timestamp          ADI_UNXTMSTMP_OP     */)\
_DEFINE(FROMUNXTM,     325  /* from_unixtime           ADI_FROMUNXTM_OP     */)\
_DEFINE(LASTDAY,       326  /* last_day                ADI_LASTDAY_OP       */)\
_DEFINE(MTHSBTWN,      327  /* months_between          ADI_MTHSBTWN_OP      */)\
_DEFINE(MLSPART,       328  /* millisecond             ADI_MLSPART_OP       */)\
_DEFINE(INGRESDATE,    329  /* ingresdate              ADI_INGRESDATE_OP    */)\
_DEFINEEND


#define _DEFINEEND
/*
** Define ADI_OP_ID_enum
**
** Note the two synonymns for ADI_LVARCH_OP & ADI_LBYTE_OP
*/
#define _DEFINE(n, v) ADI_##n##_OP=v,
enum ADI_OP_ID_enum {ADI_OPS_MACRO /*, */
			ADI_LONG_VARCHAR=ADI_LVARCH_OP,
			ADI_LONG_BYTE=ADI_LBYTE_OP};
#undef  _DEFINE

/*
** Ensure elements are contiguous
*/
#define _DEFINE(n, v) == v && v+1
#if !(-1 ADI_OPS_MACRO >0)
# error "ADI_OPS_MACRO is not contiguous"
#endif
#undef  _DEFINE
#undef  _DEFINEEND

#endif /*__ADFOPS_H_INCLUDED*/
