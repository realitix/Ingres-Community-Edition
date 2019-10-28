/*
**Copyright (c) 2004, 2010 Actian Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <bt.h>
#include    <tm.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <sr.h>
#include    <tr.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dmp.h>
#include    <dmxe.h>
#include    <dm2t.h>
#include    <dm1b.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <di.h>
#include    <sxf.h>
#include    <dma.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm1u.h>
#include    <dm2u.h>
#include    <dmftrace.h>
#include    <dmu.h>
#include    <dmpecpn.h>
#include    <st.h>
#include    <dm0pbmcb.h>

/**
**
** Name: DMUMODIFY.C - Functions needed to modify the table structure.
**
** Description:
**      This file contains the functions necessary to modify a table.
**      Common routines used by all DMU functions are not included here, 
**      but are found in DMUCOMMON.C.  This file defines:
**    
**      dmu_modify()       -  Routine to perfrom the normal modify operation.
**
** History:
**      01-sep-85 (jennifer) 
**          Created for Jupiter.
**	15-aug-88 (teg)
**	    Changed hardcoded fill factors to use constants
**	19-sep-89 (rogerk)
**	    Check for fill_factor (default to 100%) when modify to truncate.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**      21-jul-89 (teg)
**          added new param (reltups) to dm2u_modify() to fix bug 6805
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	22-feb-1990 (fred)
**	    Added support for peripheral datatypes and table extensions.
**      10-jan-1991 (bryanp)
**          Added support for Btree index compression.
**	11-oct-1991 (jnash) merged 17-aug-1989 (Derek)
**	    Added DAC success auditing.
**      11-oct-1991 (jnash) merged 27-mar-87 (jennifer)
**          Added code for ORANGE project to test if requesting label is 
**          same as table label.
**	11-oct-1991 (jnash) merged 24-sep-1990 (rogerk)
**	24-sep-1990 (rogerk)
**	    Merged 6.3 changes into 6.5.
**	11-oct-1991 (jnash) merged 14-jun-1991 (Derek)
**	    Added support for new allocation and extend options.
**	17-jan-1992 (bryanp)
**	    Temporary table enhancements: new DMU_TEMP_TABLE characteristic.
**	6-jul-1992 (bryanp)
**	    Prototyping DMF.
**      29-August-1992 (rmuth)
**          Altered default for table_debug, used to be bitmaps and pagetypes
**          now just bitmaps. Reason is that pagetypes scans the whole table
**          which is expensive so make the user request this.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	4-nov-1992 (robf)
**	    When modifying a table, use new SXF message Id.
**	18-nov-92 (robf)
**	    Removed dm0a.h since auditing now handled by SXF
**	    All auditing now thorough dma_write_audit()
**	30-mar-1993 (rmuth)
**	    Add code for "to add_extend" option.
**	7-apr-93 (rickh)
**	    Support new relstat2 bits in modify statements for FIPS
**	    CONSTRAINTS.
**	15-may-1993 (rmuth)
**	    Add support for READONLY tables.
**	1-jun-1993 (robf)
**	    Secure 2.0: Support for relstat2 attributes for row label/audit
**	    attributes preserved.
**      24-Jun-1993 (fred)
**          Added include of dmpecpn.h to pick up dmpe prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <lk.h> to pick up locking system type definitions.
**	    Include <pc.h> before <lk.h> to pick up PID definition
**	26-jul-1993 (rmuth)
**	    Alter READONLY tables to just use a DMU_READONLY entry.
**	8-sep-93 (stephenb)
**	    Change calls to dma_write_audit() to set SXF_A_SUCCESS bit in 
**	    accessmask, the default is fail, which is incorrect here.
**      14-mar-94 (stephenb)
**          Add new parameter to dma_write_audit() to match current prototype.
**      06-jan-95 (nanpr01)
**	    Bug # 66028
**	    Set the bit TCB2_TBL_RECOVERY_DISALLOWED if modify verb is
**	    used on the table.
**      06-jan-95 (forky01)
**	    Bug # 66002
**	    Added support for MODIFY tablename | indexname TO PHYS_INCONSISTENT,
**                     PHYS_CONSISTENT,LOG_INCONSISTENT, LOG_CONSISTENT,
**	               TABLE_RECOVERY_ALLOWED, TABLE_RECOVERY_DISALLOWED
**	08-feb-95 (cohmi01)
**	    For RAW/IO extent names, added rawextnm parameter to dm2u_modify.
**	21-may-96 (kch)
**	    In the function dmu_modify(), we now do not assign the location
**	    name (location) for a session temporary table. This change fixes
**	    bug 75810. 
**	06-mar-1996 (stial01 for bryanp)
**	    Recognize DMU_PAGE_SIZE characteristic; pass page_size argument to
**		dm2u_modify.
**	    Choose default page size from svcb->svcb_page_size.
**      19-apr-1996 (stial01)
**          Validate page_size parameter
**	14-may-1996 (shero03)
**	    Support HI data compression
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	24-Mar-1997 (jenjo02)
** 	    Table priority project:
**	    Get new table priority from DMU_TABLE_PRIORITY or
**	    DMU_TO_TABLE_PRIORITY characteristic, if extant, pass to
**	    dm2u_modify.
**	20-aug-1998 (somsa01)
**	    Added more parameters to dmpe_modify() to help us properly modify
**	    a base table's extension tables.
**	12-mar-1999 (nanpr01)
**	    Get rid of extent names since only one table is supported per
**	    raw location.
**      12-nov-1999 (stial01)
**          Do not allow dm616 (forces key compression) for temp tables
**      12-may-2000 (stial01)
**          modify for retrieve into, pick up the next available page size
**          (another change for SIR 98092)
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      17-Apr-2001 (horda03) Bug 104402
**          Ensure that Foreign Key constraint indices are created with
**          PERSISTENCE. This is different to Primary Key constraints as
**          the index need not be UNIQUE nor have a UNIQUE_SCOPE.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to dma_write_audit() if not C2SECURE.
**      01-feb-2001 (stial01)
**          Updated test for valid page types (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**	05-Apr-2001 (jenjo02)
**	    Added back
**	        dmu->error.err_data = dmu->dmu_tup_cnt;
**	    line dropped by RAW integration.
**	23-Apr-2001 (jenjo02)
**	    That broke raw location index value, don't overlay
**	    dmu->error.err_data if E_DM0189 or E_DM0190.
**      02-jan-2004 (stial01)
**          Added support for ADD_EXTEND with BLOB_EXTEND to add pages to etabs
**          Don't modify etabs if adding pages to base table
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      14-apr-2004 (stial01)
**          E_DM007D_BTREE_BAD_KEY_LENGTH should return klen and max klen
**	    Clean up error handling, use err_data for error info, and
**	    dmu_tup_cnt for additional error data (btree max klen). (SIR 108315)
**      27-aug-2004 (stial01)
**          Remove ifdef code.
**	10-jan-2005 (thaju02)
** 	    Modified params to dm2u_modify for online mod of part tbl.
**      29-nov-2005 (horda03) Bug 48659/INGSRV 2716
**          A TX can now be started with a non-journaled BT record. This
**          leaves the XCB_DELAYBT flag set but sets the XCB_NONJNL_BT
**          flag. Need to write a journaled BT record if the update
**          will be journaled.
**	12-oct-2006 (shust01)
** 	    Modified param to dmpe_modify.  Now passing 0 for page_size, since
** 	    doing a 'modify <base-table> ...with page_size' should not effect
** 	    the extension tables. bug 116847
**	23-Oct-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	21-Nov-2008 (jonj)
**	    SIR 120874: dmxe_? functions converted to DB_ERROR *
**	08-Dec-2008 (jonj)
**	    SIR 120874: dma_? auditing functions converted to DB_ERROR *
**	01-apr-2010 (toumi01) SIR 122403
**	    Add support for column encryption.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/*{
** Name: dmu_modify - Modifies a table's structure.
**
**  INTERNAL DMF call format:      status = dmu_modify(&dmu_cb);
**
**  EXTERNAL call format:          status = dmf_call(DMU_MODIFY_TABLE,&dmu_cb);
**
** Description:
**      The dmu_modify function handles the modification of the 
**      structure.  The table should be owned by the user however,
**      it is presumed that this has been verifed by the caller.  
**      All indices defined on the table are destroyed because the access 
**      information stored in the index is invalid.
**
**      The modify to truncated option is requested by the caller
**      specifying the characteristic DMU_TRUNCATE.  The modify to
**      merge option for BTREE tables is requested by the caller
**      specifying the characteristic DMU_MERGE.  
** 
**      Note: For B1 secure server, this routine must make sure
**      that the security label of the requester is exactly the
**      same as the security label on the table.  If is not 
**      identical, the operation is rejected with the error
**      E_DB0125_DDL_SECURITY_ERROR.
**
** Inputs:
**      dmu_cb 
**          .type                           Must be set to DMU_UTILITY_CB.
**          .length                         Must be at least sizeof(DMU_CB).
**	    .dmu_flags_mask		    Must be zero, or DMU_INTERNAL_REQ
**          .dmu_tran_id                    Must be the current transaction 
**                                          identifier returned from the 
**                                          begin transaction operation.
**          .dmu_tbl_id                     Internal name of table to be 
**                                          modified.
**	    .dmu_action			    What kind of modify to perform
**          .dmu_location.data_address      Pointer to array of locations.
**                                          Each entry in array is of type
**                                          DB_LOC_NAME.  Must be zero if
**                                          no locations specified.
**	    .dmu_location.data_in_size      The size of the location array
**                                          in bytes.
**          .dmu_key_array.ptr_address	    Pointer to array of pointer. Each
**                                          Pointer points to an attribute
**                                          entry of type DMU_KEY_ENTRY.  See
**                                          below for description of entry.
**          .dmu_key_array.ptr_in_count     The number of pointers in the array.
**	    .dmu_key_array.ptr_size	    The size of each element pointed at
**	    .dmu_chars			    Various table characteristics
**
**          <dmu_key_array> entries are of type DMU_KEY_ENTRY and
**          must have following format:
**          key_attr_name                   Name of attribute.
**          key_order                       Must be DMU_ASCENDING or 
**                                          DMU_DESCENDING. DMU_DESCENDING 
**                                          is only legal on a heap structure.
**
** Output:
**      dmu_cb
**	    .dmu_tup_cnt		    Count of tuples following modify.
**					    If no tuple count was obtained then
**					    dmu_tup_cnt is set to -1.  No tuple
**					    count is obtained on the following
**					    modify options:
**						MODIFY TO REORGANIZE
**						MODIFY TO MERGE
**          .error.err_code                 One of the following error numbers.
**                                          E_DM0000_OK                
**                                          E_DM0006_BAD_ATTR_FLAG
**                                          E_DM0007_BAD_ATTR_NAME
**                                          E_DM0009_BAD_ATTR_SIZE
**                                          E_DM0008_BAD_ATTR_PRECISION
**                                          E_DM000A_BAD_ATTR_TYPE
**                                          E_DM000B_BAD_CB_LENGTH
**                                          E_DM000C_BAD_CB_TYPE
**					    E_DM000D_BAD_CHAR_ID
**					    E_DM000E_BAD_CHAR_VALUE
**                                          E_DM0010_BAD_DB_ID
**                                          E_DM001A_BAD_FLAG
**                                          E_DM001C_BAD_KEY_SEQUENCE
**                                          E_DM001D_BAD_LOCATION_NAME
**                                          E_DM001E_DUP_LOCATION_NAME
**                                          E_DM001F_LOCATION_LIST_ERROR
**                                          E_DM002A_BAD_PARAMETER
**                                          E_DM003B_BAD_TRAN_ID
**                                          E_DM0042_DEADLOCK
**                                          E_DM0045_DUPLICATE_KEY
**                                          E_DM004A_INTERNAL_ERROR
**                                          E_DM004B_LOCK_QUOTA_EXCEEDED
**                                          E_DM0054_NONEXISTENT_TABLE
**                                          E_DM005A_CANT_MOD_CORE_STRUCT
**                                          E_DM005D_TABLE_ACCESS_CONFLICT
**                                          E_DM005E_CANT_UPDATE_SYSCAT
**                                          E_DM0064_USER_INTR
**                                          E_DM0065_USER_ABORT
**	    				    E_DM006A_TRAN_ACCESS_CONFLICT
**                                          E_DM0071_LOCATIONS_TOO_MANY
**                                          E_DM0072_NO_LOCATION
**					    E_DM007C_BAD_KEY_DIRECTION
**					    E_DM007D_BTREE_BAD_KEY_LENGTH
**					    E_DM010F_ISAM_BAD_KEY_LENGTH
**					    E_DM0110_COMP_BAD_KEY_LENGTH
**					    E_DM0091_ERROR_MODIFYING_TABLE
**                                          E_DM009F_ILLEGAL_OPERATION
**                                          E_DM00D1_BAD_SYSCAT_MOD
**                                          E_DM0100_DB_INCONSISTENT
**                                          E_DM0103_TUPLE_TOO_WIDE
**                                          E_DM010C_TRAN_ABORTED
**                                          E_DM0125_DDL_SECURITY_ERROR
**
**         .error.err_data                  Set to attribute in error by 
**                                          returning index into attribute list.
**
** History:
**      01-sep-85 (jennifer) 
**          Created for Jupiter.
**      20-nov-87 (jennifer)
**          Added multiple location support.
**	19-sep-99 (rogerk)
**	    Check for fill_factor (default to 100%) when modify to truncate.
**	30-sep-1988 (rogerk)
**	    Added capability to delay writing Begin Transaction record until
**	    first write operation.  If DELAYBT flag is set in the XCB, then
**	    call dmxe to write the BT record.
**	31-mar-1989 (EdHsu)
**	    Added param to dmxe_writebt() to support cluster online backup
**      21-jul-89 (teg)
**          added new param (reltups) to dm2u_modify() to fix bug 6805
**	15-aug-1989 (rogerk)
**	    Added support for Non-SQL Gateway.  Check for illegal modify
**	    of gateway table error.
**	17-jan-1990 (rogerk)
**	    Add support for not writing savepoint records in readonly xacts.
**	    Pass xcb argument to dmxe_writebt routine.
**      15-feb-1990 (teg)
**          drop in check/patch table support (picked up from 6.4).
**	22-feb-1990 (fred)
**	    Added support for peripheral datatypes.  If the table being modified
**	    has extensions, then call dmpe_modify() to modify all the extension
**	    tables as well.
**      10-jan-1991 (bryanp)
**          Added support for Btree index compression. New 'index_compression'
**          argument for dm2u_modify(). If DM616 is set, force index
**          compression to be used.
**      11-oct-1991 (jnash) merged 27-mar-87 (jennifer)
**          Added code for ORANGE project to test if requesting label is 
**          same as table label.
**      11-oct-1991 (jnash) merged 17-aug-1989 (Derek)
**          Added DAC success auditing.
**      11-oct-1991 (jnash) merged 14-jun-1991 (Derek)
**          Added support for new allocation and extend options.
**	17-jan-1992 (bryanp)
**	    Temporary table enhancements: new DMU_TEMP_TABLE characteristic.
**	    New 'temporary' argument to dm2u_modify.
**      29-August-1992 (rmuth)
**          Altered default for table_debug, used to be bitmaps and pagetypes
**          now just bitmaps. Reason is that pagetypes scans the whole table
**          which is expensive so make the user request this.
**	23-oct-92 (teresa)
**	    implement sir 42498 by passing verbose flag down to dm1u_verify.
**	17-Nov-1992 (fred)
**	    Added *truncate* parameter to dmpe_modify().  Since this routine
**	    has already figured out whether this is a modify to truncated or
**	    not, there is no point in duplicating that code in dmpe_modify().
**	    Just pass the knowledge along.
**	21-dec-92 (robf)
**	    Audit a table PATCH operation as a distinct security audit 
**	    item since this could potentially lose data.
**	30-mar-1993 (rmuth)
**	    Add code for "to add_extend" option.
**	7-apr-93 (rickh)
**	    Support new relstat2 bits in modify statements for FIPS
**	    CONSTRAINTS.
**	15-may-1993 (rmuth)
**	    Add support for "readonly" and "noreadonly" options.
**	1-jun-1993 (robf)
**	    Add support for ROW_LABEL and ROW_AUDIT options
**	8-sep-93 (stephenb)
**	    Change calls to dma_write_audit() to set SXF_A_SUCCESS bit in 
**	    accessmask, the default is fail, which is incorrect here.
**      13-oct-1993 (markm)
**          Fix min_pages when using default value.
**	7-jul-94 (robf)
**          Add support for internal request.
**      06-jan-95 (nanpr01)
**	    Bug # 66028
**	    Set the bit TCB2_TBL_RECOVERY_DISALLOWED if modify verb is
**	    used on the table.
**      06-jan-95 (forky01)
**	    Bug # 66002
**	    Added support for MODIFY tablename | indexname TO PHYS_INCONSISTENT,
**                     PHYS_CONSISTENT,LOG_INCONSISTENT, LOG_CONSISTENT,
**	               TABLE_RECOVERY_ALLOWED, TABLE_RECOVERY_DISALLOWED
**	08-feb-95 (cohmi01)
**	    For RAW/IO extent names, added rawextnm parameter to dm2u_modify.
**	21-may-96 (kch)
**	    We now do not assign the location name (location) for a temporary
**	    table, allowing a session temporary declared as 'select ... from
**	    tablename' to be successfully modified during creation in a
**	    database with multiple work locations. In dm2u_modify(), loc_count
**	    is correctly set to t->tcb_table_io.tbio_loc_count and loc_array[]
**	    is correctly filled with the work location names. This then
**	    prevents the error in dm2t_temp_build_tcb() when the extent
**	    information is built for the session temporary table's locations.
**	    This change fixes bug 75810.
**	06-mar-1996 (stial01 for bryanp)
**	    Recognize DMU_PAGE_SIZE characteristic; pass page_size argument to
**		dm2u_modify.
**	    Choose default page size from svcb->svcb_page_size.
**      19-apr-1996 (stial01)
**          Validate page_size parameter
**	17-sep-96 (nick)
**	    Remove compiler warning.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction
**	    access mode> (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_DM006A_TRAN_ACCESS_CONFLICT.
**	24-Mar-1997 (jenjo02)
** 	    Table priority project:
**	    Get new table priority from DMU_TABLE_PRIORITY or
**	    DMU_TO_TABLE_PRIORITY characteristic, if extant, pass to
**	    dm2u_modify.
**	9-apr-98 (inkdo01)
**	    Add support for "to persistence/unique_scope=statement" options
**	    for constraint index with clauses.
**	20-aug-1998 (somsa01)
**	    Added more parameters to dmpe_modify() to help us properly modify
**	    a base table's extension tables.
**      15-mar-1999 (stial01)
**          dmu_modify() additional param to dm2u_modify
**	15-apr-99 (stephenb)
**	    Add code to look for peripheral (coupon) checking when using verify
**      17-Apr-2001 (horda03) Bug 104402
**          Added support for the TCB_NOT_UNIQUE attribute.
**	6-Feb-2004 (schka24)
**	    Get rid of DMU statement count and its limit.
**	28-Feb-2004 (jenjo02)
**	    Added preliminary code for partitioned modifies.
**	12-Mar-2004 (schka24)
**	    Clean up verify flag mess a little bit.
**	2-Apr-2004 (schka24)
**	    Make sure that partition def is one-piece (for logging).
**	30-apr-2004 (thaju02)
**	    Added DMU_PIND_CHAINED.
**	3-Aug-2004 (schka24)
**	    Error code return got lost somehow recently, put it back.
**	11-Nov-2005 (jenjo02)
**	    Replaced dmx_show() with the more robust 
**	    dmxCheckForInterrupt() to standardize external
**	    interrupt handling.
**	20-mar-2006 (toumi01)
**	    Added DMU_NODEPENDENCY_CHECK.
**	13-Apr-2006 (jenjo02)
**	    Add DMU_CLUSTERED, squash insane parm list into (new)
**	    DM2U_MOD_CB structure.
**      05-jul-2006 (stial01)
**          Fix page_size parameter to dmpe_modify.
**	12-oct-2006 (shust01)
** 	    Modified param to dmpe_modify.  Now passing 0 for page_size, since
** 	    doing a 'modify <base-table> ...with page_size' should not effect
** 	    the extension tables. bug 116847
**	26-sep-2007 (kibro01)
**	    Allow modify to nopersistence
**      01-oct-2007 (stial01)
**          Fixed DMU_COMPRESSED to accept char_value DMU_C_HIGH
**	15-sep-2009 (dougi)
**	    Allow "x100ix" storage type for X100 indexes.
**	22-Oct-2009 (kschendel) SIR 122739
**	    Minor rejuggling of how compression, relstat, relstat2 are passed.
**	13-feb-2010 (dougi)
**	    Add DMU_UPD_RELTUPS to update iirelation.reltups (for X100).
**	3-sep-2010 (dougi)
**	    Add support for i8 reltups.
**	12-Oct-2010 (kschendel) SIR 124544
**	    dmu_char_array replaced with DMU_CHARACTERISTICS.
**	    Fix the operation preamble here.
*/

DB_STATUS
dmu_modify(DMU_CB    *dmu_cb)
{
    DM_SVCB		*svcb = dmf_svcb;
    DMU_CB		*dmu = dmu_cb;
    DML_XCB		*xcb;
    DML_ODCB		*odcb;
    DM2U_MOD_CB		local_mcb, *mcb = &local_mcb;

    i4			recovery;
    i4			truncate;
    i4			duplicates;
    i4			i,j;
    i4			indicator;
    i4			error, local_error;
    DB_STATUS		status;
    bool                bad_loc;
    i4			blob_add_extend = 0;
    bool                used_default_page_size = TRUE;
    i4			page_size;
    i4			verify_options;
    i4			mask;
    i4			has_extensions = 0;
    DB_OWN_NAME		table_owner;
    DB_TAB_NAME		table_name;
    i4			countvals[2];
    i8			*counti8 = (i8 *)&countvals[0];
    bool		got_action;
    bool		is_table_debug;
    bool		reorg;

    CLRDBERR(&dmu->error);

    /* Any modify should make table recovery disallowed except for the
    ** alter_status options which change logical, physical consistency
    ** and table recovery bit itself
    */
    mcb->mcb_mod_options2 = DM2U_2_TBL_RECOVERY_DEFAULT;

    do
    {
	/*  Check for bad flags. */

	mask = ~(DMU_VGRANT_OK | DMU_INTERNAL_REQ | DMU_RETINTO
		| DMU_PARTITION | DMU_MASTER_OP | DMU_ONLINE_START | DMU_ONLINE_END
		| DMU_NO_PAR_INDEX | DMU_PIND_CHAINED | DMU_NODEPENDENCY_CHECK);
        if ( (dmu->dmu_flags_mask & mask) != 0)
	{
	    SETDBERR(&dmu->error, 0, E_DM001A_BAD_FLAG);
	    break;
	}

	/*  Validate the transaction id. */

	xcb = (DML_XCB *)dmu->dmu_tran_id;
	if (dm0m_check((DM_OBJECT *)xcb, (i4)XCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM003B_BAD_TRAN_ID);
	    break;
	}

	/* Check for external interrupts */
	if ( xcb->xcb_scb_ptr->scb_ui_state )
	    dmxCheckForInterrupt(xcb, &error);

	if ( xcb->xcb_state )
	{
	    if (xcb->xcb_state & XCB_USER_INTR)
	    {
		SETDBERR(&dmu->error, 0, E_DM0065_USER_INTR);
		break;
	    }
	    if (xcb->xcb_state & XCB_FORCE_ABORT)
	    {
		SETDBERR(&dmu->error, 0, E_DM010C_TRAN_ABORTED);
		break;
	    }
	    if (xcb->xcb_state & XCB_ABORT)
	    {
		SETDBERR(&dmu->error, 0, E_DM0064_USER_ABORT);
		break;
	    }	    
	}

	/*  Check the database identifier. */

	odcb = (DML_ODCB *)dmu->dmu_db_id;
	if (dm0m_check((DM_OBJECT *)odcb, (i4)ODCB_CB) != E_DB_OK)
	{
	    SETDBERR(&dmu->error, 0, E_DM0010_BAD_DB_ID);
	    break;
	}

	mcb->mcb_db_lockmode = DM2T_X;

	/*  Check that this is a update transaction on the database 
        **  that can be updated. */
	if (odcb != xcb->xcb_odcb_ptr)
	{
	    SETDBERR(&dmu->error, 0, E_DM005D_TABLE_ACCESS_CONFLICT);
	    break;
	}

	/* Prime the MCB */
	mcb->mcb_dcb = odcb->odcb_dcb_ptr;
	mcb->mcb_xcb = xcb;
	mcb->mcb_tbl_id = &dmu->dmu_tbl_id;
	mcb->mcb_omcb = (DM2U_OMCB*)NULL;
	mcb->mcb_dmu = dmu;
	mcb->mcb_structure = 0;
	mcb->mcb_i_fill = 0;
	mcb->mcb_l_fill = 0;
	mcb->mcb_d_fill = 0;
	mcb->mcb_unique = FALSE;
	mcb->mcb_compressed = TCB_C_NONE;
        mcb->mcb_index_compressed = FALSE;
	mcb->mcb_temporary = FALSE;
	mcb->mcb_merge = FALSE;
	mcb->mcb_clustered = FALSE;
	mcb->mcb_modoptions = 0;
	mcb->mcb_min_pages = 0;
	mcb->mcb_max_pages = 0;
	mcb->mcb_allocation = 0;
	mcb->mcb_extend = 0;
	mcb->mcb_page_type = TCB_PG_INVALID;
	mcb->mcb_page_size = svcb->svcb_page_size;
	mcb->mcb_tup_info = &dmu->dmu_tup_cnt;
	mcb->mcb_reltups = 0;
	mcb->mcb_tab_name = &table_name;
	mcb->mcb_tab_owner = &table_owner;
	mcb->mcb_has_extensions = &has_extensions;
	mcb->mcb_relstat2 = 0;
	mcb->mcb_flagsmask = dmu->dmu_flags_mask;
	mcb->mcb_tbl_pri = 0;
	mcb->mcb_rfp_entry = (DM2U_RFP_ENTRY*)NULL;
	mcb->mcb_new_part_def = (DB_PART_DEF*)dmu->dmu_part_def;
	mcb->mcb_new_partdef_size = dmu->dmu_partdef_size;
	mcb->mcb_verify = 0;

        truncate = 0;
	reorg = FALSE;
	duplicates = -1;
	verify_options = 0;
	got_action = FALSE;

	/* FIXME better messages (in general) */
	/* If there's a partdef it has to be one-piece, else bad param */
	if (dmu->dmu_part_def != NULL
	  && dmu->dmu_part_def->ndims > 0
	  && (dmu->dmu_part_def->part_flags & DB_PARTF_ONEPIECE) == 0)
	{
	    SETDBERR(&dmu->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	/* Disassemble the modify action.
	** FIXME this used to be buried in the characteristics array.
	** It would make much more sense to just carry the action
	** code through, but that will have to wait for another day.
	*/
	got_action = FALSE;
	switch (dmu->dmu_action)
	{
	case DMU_ACT_STORAGE:
	    if (BTtest(DMU_STRUCTURE, dmu->dmu_chars.dmu_indicators))
	    {
		got_action = TRUE;
		mcb->mcb_structure = dmu->dmu_chars.dmu_struct;
	    }
	    break;

	case DMU_ACT_ADDEXTEND:
	    got_action = TRUE;
	    mcb->mcb_mod_options2 |= DM2U_2_ADD_EXTEND;
	    break;

	case DMU_ACT_ENCRYPT:
	    got_action = TRUE;
	    mcb->mcb_mod_options2 |= DM2U_2_ENCRYPT;
	    break;

	case DMU_ACT_LOG_CONSISTENT:
	    if (BTtest(DMU_ACTION_ONOFF, dmu->dmu_chars.dmu_indicators))
	    {
		got_action = TRUE;
		mcb->mcb_mod_options2 &= ~DM2U_2_TBL_RECOVERY_DEFAULT;
		if ( dmu->dmu_chars.dmu_flags & DMU_FLAG_ACTON )
		    mcb->mcb_mod_options2 |= DM2U_2_LOG_CONSISTENT;
		else
		    mcb->mcb_mod_options2 |= DM2U_2_LOG_INCONSISTENT;
	    }
	    break;

	case DMU_ACT_MERGE:
	    got_action = TRUE;
	    mcb->mcb_merge = TRUE;
	    break;

	case DMU_ACT_PERSISTENCE:
	    if (BTtest(DMU_PERSISTS_OVER_MODIFIES, dmu->dmu_chars.dmu_indicators))
	    {
		got_action = TRUE;
		mcb->mcb_mod_options2 |= (dmu->dmu_chars.dmu_flags & DMU_FLAG_PERSISTENCE) ?
			DM2U_2_PERSISTS_OVER_MODIFIES :
			DM2U_2_NOPERSIST_OVER_MODIFIES;
	    }
	    break;

	case DMU_ACT_PHYS_CONSISTENT:
	    if (BTtest(DMU_ACTION_ONOFF, dmu->dmu_chars.dmu_indicators))
	    {
		got_action = TRUE;
		mcb->mcb_mod_options2 &= ~DM2U_2_TBL_RECOVERY_DEFAULT;
		if ( dmu->dmu_chars.dmu_flags & DMU_FLAG_ACTON )
		    mcb->mcb_mod_options2 |= DM2U_2_PHYS_CONSISTENT;
		else
		    mcb->mcb_mod_options2 |= DM2U_2_PHYS_INCONSISTENT;
	    }
	    break;

	case DMU_ACT_PRIORITY:
	    if (BTtest(DMU_TABLE_PRIORITY, dmu->dmu_chars.dmu_indicators))
		got_action = TRUE;
	    /* flag setting when we hit the priority char */
	    break;

	case DMU_ACT_READONLY:
	    if (BTtest(DMU_ACTION_ONOFF, dmu->dmu_chars.dmu_indicators))
	    {
		got_action = TRUE;
		if ( dmu->dmu_chars.dmu_flags & DMU_FLAG_ACTON )
		    mcb->mcb_mod_options2 |= DM2U_2_READONLY;
		else
		    mcb->mcb_mod_options2 |= DM2U_2_NOREADONLY;
	    }
	    break;

	case DMU_ACT_REORG:
	    got_action = TRUE;
	    reorg = TRUE;
	    break;

	case DMU_ACT_TABLE_RECOVERY:
	    if (BTtest(DMU_ACTION_ONOFF, dmu->dmu_chars.dmu_indicators))
	    {
		got_action = TRUE;
		mcb->mcb_mod_options2 &= ~DM2U_2_TBL_RECOVERY_DEFAULT;
		if ( dmu->dmu_chars.dmu_flags & DMU_FLAG_ACTON )
		    mcb->mcb_mod_options2 |= DM2U_2_TBL_RECOVERY_ALLOWED;
		else
		    mcb->mcb_mod_options2 |= DM2U_2_TBL_RECOVERY_DISALLOWED;
	    }
	    break;

	case DMU_ACT_TRUNC:
	    got_action = TRUE;
	    truncate++;
	    break;

	case DMU_ACT_USCOPE:
	    if (BTtest(DMU_STATEMENT_LEVEL_UNIQUE, dmu->dmu_chars.dmu_indicators))
	    {
		got_action = TRUE;
		mcb->mcb_mod_options2 |= DM2U_2_STATEMENT_LEVEL_UNIQUE;
	    }
	    break;

	case DMU_ACT_VERIFY:
	    if (BTtest(DMU_VACTION, dmu->dmu_chars.dmu_indicators))
	    {
		got_action = TRUE;
		mcb->mcb_verify = dmu->dmu_chars.dmu_vaction;
	    }
	    break;

	case DMU_ACT_UPD_RELTUPS:
	    got_action = TRUE;
	    mcb->mcb_mod_options2 |= DM2U_2_UPD_RELTUPS;
	    mcb->mcb_reltups = dmu->dmu_tup_cnt;	/* New value here */
	    break;

	case DMU_ACT_REP_RELTUPS:
	    got_action = TRUE;
	    mcb->mcb_mod_options2 |= DM2U_2_REP_RELTUPS;
	    mcb->mcb_reltups = dmu->dmu_tup_cnt;	/* New value here */
	    break;

	} /* switch */

	if (! got_action)
	{
	    SETDBERR(&dmu->error, 0, E_DM000E_BAD_CHAR_VALUE);
	    break;
	}
	/* Reset this now for output rowcount */
	dmu->dmu_tup_cnt = 0;

	/* Disassemble the characteristics.
	** FIXME probably better to just carry it through, but one step
	** at a time!
	*/
	indicator = -1;
	while ((indicator = BTnext(indicator, dmu->dmu_chars.dmu_indicators, DMU_CHARIND_LAST)) != -1)
	{
	    switch (indicator)
	    {
	    case DMU_ACTION_ONOFF:
	    case DMU_STRUCTURE:
		/* Already picked it up, just skip on */
		continue;

	    case DMU_IFILL:
		mcb->mcb_i_fill = dmu->dmu_chars.dmu_nonleaff;
		if (mcb->mcb_i_fill > 100)
		    mcb->mcb_i_fill = 100;
		continue;

	    case DMU_LEAFFILL:
		mcb->mcb_l_fill = dmu->dmu_chars.dmu_leaff;
		if (mcb->mcb_l_fill > 100)
		    mcb->mcb_l_fill = 100;
		continue;

	    case DMU_DATAFILL:
		mcb->mcb_d_fill = dmu->dmu_chars.dmu_fillfac;
		if (mcb->mcb_d_fill > 100)
		    mcb->mcb_d_fill = 100;
		continue;

	    case DMU_PAGE_SIZE:
		used_default_page_size = FALSE;
		mcb->mcb_page_size = dmu->dmu_chars.dmu_page_size;
		if (mcb->mcb_page_size != 2048   && mcb->mcb_page_size != 4096  &&
		    mcb->mcb_page_size != 8192   && mcb->mcb_page_size != 16384 &&
		    mcb->mcb_page_size != 32768  && mcb->mcb_page_size != 65536)
		{
		    SETDBERR(&dmu->error, indicator, E_DM000E_BAD_CHAR_VALUE);
		    break;
		}
		else if (!dm0p_has_buffers(mcb->mcb_page_size))
		{
		    SETDBERR(&dmu->error, 0, E_DM0157_NO_BMCACHE_BUFFERS);
		    break;
		}		    
		else
		{
		    continue;
		}

	    case DMU_MINPAGES:
		mcb->mcb_min_pages = dmu->dmu_chars.dmu_minpgs;
		continue;

	    case DMU_MAXPAGES:
		mcb->mcb_max_pages = dmu->dmu_chars.dmu_maxpgs;
		continue;

	    case DMU_UNIQUE:
		mcb->mcb_unique = TRUE;
		continue;

	    case DMU_DCOMPRESSION:
		/* Translate DMU_xxx to TCB compression types */
		if (dmu->dmu_chars.dmu_dcompress == DMU_COMP_ON)
		    mcb->mcb_compressed = TCB_C_DEFAULT;
		else if (dmu->dmu_chars.dmu_dcompress == DMU_COMP_HI)
		    mcb->mcb_compressed = TCB_C_HICOMPRESS;
		continue;

            case DMU_KCOMPRESSION:
                mcb->mcb_index_compressed =
			(dmu->dmu_chars.dmu_kcompress != DMU_COMP_OFF);
                continue;

	    case DMU_TEMP_TABLE:
		mcb->mcb_temporary = TRUE;
		continue;

	    case DMU_RECOVERY:
		recovery = (dmu->dmu_chars.dmu_flags & DMU_FLAG_RECOVERY) != 0;
		if (recovery)
		{
		    /* recovery isn't currently supported */
		    SETDBERR(&dmu->error, indicator, E_DM000D_BAD_CHAR_ID);
		    break;
		}
		continue;

	    case DMU_DUPLICATES:
		duplicates = 0;
		if (dmu->dmu_chars.dmu_flags & DMU_FLAG_DUPS)
		    duplicates = 1;
		continue;

	    case DMU_ALLOCATION:
		mcb->mcb_allocation = dmu->dmu_chars.dmu_alloc;
		continue;

	    case DMU_EXTEND:
		mcb->mcb_extend = dmu->dmu_chars.dmu_extend;
		continue;

	    case DMU_VACTION:
		/* Already got it, just skip on */
		continue;

	    case DMU_VOPTION:
		verify_options = dmu->dmu_chars.dmu_voption;
		continue;

	    case DMU_STATEMENT_LEVEL_UNIQUE:
		if (dmu->dmu_chars.dmu_flags & DMU_FLAG_UNIQUE_STMT)
		    mcb->mcb_relstat2 |= TCB_STATEMENT_LEVEL_UNIQUE;
		continue;

	    case DMU_PERSISTS_OVER_MODIFIES:
		if (dmu->dmu_chars.dmu_flags & DMU_FLAG_PERSISTENCE)
		    mcb->mcb_relstat2 |= TCB_PERSISTS_OVER_MODIFIES;
		continue;

	    case DMU_SYSTEM_GENERATED:
		mcb->mcb_relstat2 |= TCB_SYSTEM_GENERATED;
		continue;

	    case DMU_SUPPORTS_CONSTRAINT:
		mcb->mcb_relstat2 |= TCB_SUPPORTS_CONSTRAINT;
		continue;

	    case DMU_NOT_UNIQUE:
		mcb->mcb_relstat2 |= TCB_NOT_UNIQUE;
		continue;

	    case DMU_NOT_DROPPABLE:
		mcb->mcb_relstat2 |= TCB_NOT_DROPPABLE;
		continue;

	    case DMU_ROW_SEC_AUDIT:
		mcb->mcb_relstat2 |= TCB_ROW_AUDIT;
		continue;

	    case DMU_TABLE_PRIORITY:
		mcb->mcb_tbl_pri = dmu->dmu_chars.dmu_cache_priority;
		if (mcb->mcb_tbl_pri < 0 || mcb->mcb_tbl_pri > DB_MAX_TABLEPRI)
		{
		    SETDBERR(&dmu->error, indicator, E_DM000E_BAD_CHAR_VALUE);
		    break;
		}
		/*
		** DMU_TABLE_PRIORITY    is set if priority came from WITH clause.
		** DMU_TO_TABLE_PRIORITY is set if priority came from MODIFY TO clause.
		*/
		if (dmu->dmu_action != DMU_ACT_PRIORITY)
		    mcb->mcb_mod_options2 |= DM2U_2_TABLE_PRIORITY;
		else
		    mcb->mcb_mod_options2 |= DM2U_2_TO_TABLE_PRIORITY;
		continue;

	    case DMU_BLOBEXTEND:
		blob_add_extend = dmu->dmu_chars.dmu_blobextend;
		continue;

	    case DMU_CLUSTERED:
		mcb->mcb_clustered = (dmu->dmu_chars.dmu_flags & DMU_FLAG_CLUSTERED) != 0;
		continue;

	    case DMU_CONCURRENT_UPDATES:
		/* Translate from PSF flag to DMU internal flag */
		if (dmu->dmu_chars.dmu_flags & DMU_FLAG_CONCUR_U)
		    mcb->mcb_flagsmask |= DMU_ONLINE_START;
		continue;

	    default:
		/* Ignore anything else, might be for CREATE, who knows */
		continue;
	    }
	    break;
	}

	/*
	** If no page size specified, set page_size to zero
	** In this case the current page size will be used
	*/
	if (used_default_page_size)
	    mcb->mcb_page_size = 0;

	/* Save a local copy for dmpe_modify, since dm2u_modify can alter mcb */
	page_size = mcb->mcb_page_size;

	if (mcb->mcb_structure == TCB_HEAP)
	{
	    if (mcb->mcb_d_fill == 0)
		mcb->mcb_d_fill = DM_F_HEAP;
	}
	else if (mcb->mcb_structure == TCB_ISAM)
	{
	    if (mcb->mcb_i_fill == 0)
		mcb->mcb_i_fill = DM_FI_ISAM;
	    if (mcb->mcb_d_fill == 0)
	    {
		if (mcb->mcb_compressed != TCB_C_NONE)
		    mcb->mcb_d_fill = DM_F_CISAM;
		else
		    mcb->mcb_d_fill = DM_F_ISAM;
	    }
	}
	else if (mcb->mcb_structure == TCB_HASH)
	{
	    if (mcb->mcb_d_fill == 0)
	    {
		if (mcb->mcb_compressed != TCB_C_NONE)
		    mcb->mcb_d_fill = DM_F_CHASH;
		else
		    mcb->mcb_d_fill = DM_F_HASH;
	    }
	    if (mcb->mcb_min_pages == 0)
	    {
		if (mcb->mcb_compressed != TCB_C_NONE)
		    mcb->mcb_min_pages = 1;
		else
		    mcb->mcb_min_pages = 10;

		/* If user specified max pages, don't set minpages higher */
		if (mcb->mcb_min_pages > mcb->mcb_max_pages && mcb->mcb_max_pages != 0)
		    mcb->mcb_min_pages = mcb->mcb_max_pages;
	    }
	    if (mcb->mcb_max_pages == 0)
		mcb->mcb_max_pages = 8388607;
	}
	else if (mcb->mcb_structure == TCB_BTREE || mcb->mcb_merge)
	{
            if (DMZ_AM_MACRO(16) && !mcb->mcb_temporary)
            {
                /* DM616 -- forces index compression to be used: */
                mcb->mcb_index_compressed = TRUE;
            }

	    if (mcb->mcb_i_fill == 0)
		mcb->mcb_i_fill = DM_FI_BTREE;
	    if (mcb->mcb_l_fill == 0)
		mcb->mcb_l_fill = DM_FL_BTREE;
	    if (mcb->mcb_d_fill == 0)
	    {
		if (mcb->mcb_compressed != TCB_C_NONE)
		    mcb->mcb_d_fill = DM_F_CBTREE;
		else
		    mcb->mcb_d_fill = DM_F_BTREE;
	    }
	}
	else if (truncate)
	{
	    if (mcb->mcb_d_fill == 0)
		mcb->mcb_d_fill = DM_F_HEAP;
	}

	if (mcb->mcb_structure == TCB_HASH && mcb->mcb_min_pages > mcb->mcb_max_pages)
	{
	    SETDBERR(&dmu->error, 0, E_DM000D_BAD_CHAR_ID);
	    break;
	}

	mcb->mcb_kcount = dmu->dmu_key_array.ptr_in_count;
	mcb->mcb_key = (DMU_KEY_ENTRY**) dmu->dmu_key_array.ptr_address;
	if (mcb->mcb_kcount && (mcb->mcb_key == (DMU_KEY_ENTRY**)NULL ||
              dmu->dmu_key_array.ptr_size != sizeof(DMU_KEY_ENTRY)))
	{
	    SETDBERR(&dmu->error, 0, E_DM002A_BAD_PARAMETER);
	    break;
	}

	if (truncate)
	{
	    mcb->mcb_kcount = 0;
	    mcb->mcb_modoptions |= DM2U_TRUNCATE;
	}
	if (duplicates == 1)
	    mcb->mcb_modoptions |= DM2U_DUPLICATES;
	else if (duplicates == 0)
	    mcb->mcb_modoptions |= DM2U_NODUPLICATES;
	/* else duplicates == -1, set neither flag */
	if (reorg)
	    mcb->mcb_modoptions |= DM2U_REORG;

	/* CLUSTERED implies and requires Unique */
	if ( mcb->mcb_clustered && mcb->mcb_structure == TCB_BTREE )
	    mcb->mcb_unique = TRUE;
	else
	    mcb->mcb_clustered = FALSE;


	if (mcb->mcb_verify)
	{
	    if (verify_options == 0)
	    {
		/*  Apply defaults. */

		switch (mcb->mcb_verify)
		{
		case DMU_V_VERIFY:
		    verify_options = DMU_T_LINK | DMU_T_RECORD | DMU_T_ATTRIBUTE;
		    break;

		case DMU_V_REPAIR:
		case DMU_V_DEBUG:
		    verify_options = DMU_T_BITMAP;
		    break;

		case DMU_V_PATCH:
		case DMU_V_FPATCH:
		    break;
		}
	    }
	    /* Shift modifiers into place */
	    mcb->mcb_verify |= (verify_options << DM1U_MODSHIFT);
	}
	is_table_debug = ((mcb->mcb_verify & DM1U_OPMASK) == DM1U_DEBUG);

	/* Check the location names for duplicates, too many. */

	mcb->mcb_location = (DB_LOC_NAME*)NULL;
	mcb->mcb_l_count = 0;
	if (dmu->dmu_location.data_address && 
	    (dmu->dmu_location.data_in_size >= sizeof(DB_LOC_NAME)) &&
	    mcb->mcb_temporary == FALSE)
	{
	    mcb->mcb_location = (DB_LOC_NAME *) dmu->dmu_location.data_address;
	    mcb->mcb_l_count = dmu->dmu_location.data_in_size/sizeof(DB_LOC_NAME);
	    if (mcb->mcb_l_count > DM_LOC_MAX)
	    {
		SETDBERR(&dmu->error, 0, E_DM0071_LOCATIONS_TOO_MANY);
		break;
	    }
	    bad_loc = FALSE;
	    for (i = 0; i < mcb->mcb_l_count; i++)
	    {
		for (j = 0; j < i; j++)
		{
		    /* 
                    ** Compare this location name against other 
                    ** already given, they cannot be the same.
                    */

		    if (MEcmp(mcb->mcb_location[j].db_loc_name,
                              mcb->mcb_location[i].db_loc_name,
			      sizeof(DB_LOC_NAME)) == 0 )
		    {
			SETDBERR(&dmu->error, i, E_DM001E_DUP_LOCATION_NAME);
			bad_loc = TRUE;
			break;
		    }	    	    
		}
		if (bad_loc == TRUE)
		    break;		    
	    }

	    if (bad_loc == TRUE)
		break;
	}
	else
	{
	    /* There must a location list if you are reorganizing
            ** to a different number of locations.
            */
	    if (reorg)
	    {
		if (dmu->dmu_location.data_address &&
					    dmu->dmu_location.data_in_size)
		    SETDBERR(&dmu->error, 0, E_DM001F_LOCATION_LIST_ERROR);
		else
		    SETDBERR(&dmu->error, 0, E_DM0072_NO_LOCATION);
		break;	    
	    }
	}

	mcb->mcb_partitions = (DMU_PHYPART_CHAR*)NULL;
	mcb->mcb_nparts = 0;
	if ( dmu->dmu_ppchar_array.data_address && 
	     dmu->dmu_ppchar_array.data_in_size >= sizeof(DMU_PHYPART_CHAR) )
	{
	    mcb->mcb_partitions = (DMU_PHYPART_CHAR*)dmu->dmu_ppchar_array.data_address;
	    mcb->mcb_nparts = dmu->dmu_ppchar_array.data_in_size
			/ sizeof(DMU_PHYPART_CHAR);
	}
    
	if ((xcb->xcb_x_type & XCB_RONLY) && !is_table_debug)
	{
	    SETDBERR(&dmu->error, 0, E_DM006A_TRAN_ACCESS_CONFLICT);
	    break;
	}

	/*
	** If this is the first write operation for this transaction,
	** then we need to write the begin transaction record.
	*/
	if ((xcb->xcb_flags & XCB_DELAYBT) != 0 && mcb->mcb_temporary == FALSE
	  && !is_table_debug)
	{
	    status = dmxe_writebt(xcb, TRUE, &dmu->error);
	    if (status != E_DB_OK)
	    {
		xcb->xcb_state |= XCB_TRANABORT;
		break;
	    }
	}

        /* Calls the physical layer to process the rest of the modify */

	status = dm2u_modify(mcb, &dmu->error);

	if (status == E_DB_OK && has_extensions)
	{
	    if ((mcb->mcb_mod_options2 & DM2U_2_ADD_EXTEND) && blob_add_extend == 0)
		status = E_DB_OK;
	    else
	    {
		/* FIX ME make modify etabs optional !! */
		/* Add flag to modify top make modify etabs optional */
		/* Add sysmod dbname tablename blob-column-name */
#ifdef xDEBUG
		TRdisplay("Modify etabs for %~t %~t\n",
		    sizeof(DB_TAB_NAME), table_name.db_tab_name,
		    sizeof(DB_OWN_NAME), table_owner.db_own_name);
#endif
		status = dmpe_modify(dmu, odcb->odcb_dcb_ptr, xcb,
			    &dmu->dmu_tbl_id, mcb->mcb_db_lockmode, mcb->mcb_temporary,
			    truncate, (i4)0, blob_add_extend, &dmu->error);
	    }
	}
	  
	    
	/*	Audit successful MODIFY/PATCH of TABLE. */

	if ( status == E_DB_OK && dmf_svcb->svcb_status & SVCB_C2SECURE )
	{
	    i4 msgid;
	    i4 access = SXF_A_SUCCESS;

	    if ((mcb->mcb_verify & DM1U_OPMASK) == DM1U_PATCH ||
		(mcb->mcb_verify & DM1U_OPMASK) == DM1U_FPATCH)
	    {
		access |= SXF_A_ALTER;
		msgid = I_SX271A_TABLE_PATCH;
	    }
	    else 
	    {
		access |= SXF_A_MODIFY;
		msgid = I_SX270F_TABLE_MODIFY;
	    }
	    /*
	    **	Audit success
	    */
	    status = dma_write_audit(
			SXF_E_TABLE,
			access,
			table_name.db_tab_name,	/* Table/view name */
			sizeof(table_name.db_tab_name),	/* Table/view name */
			&table_owner,	/* Table/view owner */
			msgid, 
			FALSE, /* Not force */
			&dmu->error, NULL);
	}

	if (status == E_DB_OK)
	{
	    /* If modify to reorg or merge then return no tuple count info. */
	    if (reorg || (mcb->mcb_merge) || (mcb->mcb_verify != 0))
	    {
		dmu->dmu_tup_cnt = DM_NO_TUPINFO;
	    }
	    return (E_DB_OK);
	}
	else
	{
            if (dmu->error.err_code > E_DM_INTERNAL)
            {
                uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
		SETDBERR(&dmu->error, 0, E_DM0091_ERROR_MODIFYING_TABLE);
            }
	    switch (dmu->error.err_code)
	    {
		case E_DM004B_LOCK_QUOTA_EXCEEDED:
		case E_DM0112_RESOURCE_QUOTA_EXCEED:
		case E_DM0091_ERROR_MODIFYING_TABLE:
		case E_DM009B_ERROR_CHK_PATCH_TABLE:
		case E_DM0045_DUPLICATE_KEY:
		case E_DM0137_GATEWAY_ACCESS_ERROR:
	        case E_DM006A_TRAN_ACCESS_CONFLICT:
		    xcb->xcb_state |= XCB_STMTABORT;
		    break;

		case E_DM0042_DEADLOCK:
		case E_DM004A_INTERNAL_ERROR:
		case E_DM0100_DB_INCONSISTENT:
		    xcb->xcb_state |= XCB_TRANABORT;
		    break;
		case E_DM0065_USER_INTR:
		    xcb->xcb_state |= XCB_USER_INTR;
		    break;
		case E_DM010C_TRAN_ABORTED:
		    xcb->xcb_state |= XCB_FORCE_ABORT;
		    break;
		case E_DM007D_BTREE_BAD_KEY_LENGTH:
		    dmu->dmu_tup_cnt = dmu->dmu_tup_cnt; /* same for now */
		default:
                    break;
	    }
	}
    } while (FALSE);

    if (dmu->error.err_code > E_DM_INTERNAL)
    {
	uleFormat(&dmu->error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char * )NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	SETDBERR(&dmu->error, 0, E_DM0091_ERROR_MODIFYING_TABLE);
    }

    return (E_DB_ERROR);
}
