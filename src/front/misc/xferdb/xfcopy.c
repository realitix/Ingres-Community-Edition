/*
** Copyright (c) 2004 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<lo.h>
# include	<st.h>
# include	<er.h>
# include	<si.h>
# include	<adf.h>
# include	<fe.h>
# include	<add.h>
# include	<afe.h>
# include	<ui.h>
# include	<ug.h>
# include	<xf.h>
# include	<spatial.h> /* Needed for SRID_UNDEFINED */
# include	"erxf.h"

/**
** Name:	xfcopy.c - write table-related commands.
**
** Description:
**	This file defines:
**
**	xftables     write create statement, if necessary, and copy statement
**	xfsetauth    write SET AUTHORIZATION statement if new user name
**	xfsetlang    write \quel or \sql directive if the dml changed
**	xfwritehdr   write a comment indicating the type of object unloaded.
**	xfwrite_id   write a name, maybe a delimited identifier.
**	xfread_id    prepare a name, maybe a delimited identifier, for use.
**
** History:
**	13-jul-87 (rdesmond) written.
**	23-sep-88 (marian)	bug 3512
**		Move the location name down into the with clause for the create
**		statement.  Call xfaddloc() to add the additional locations.
**	04-oct-88 (marian)
**		Add support for ingres STAR.  The with clause will have
**		additional information for STAR tables.
**	26-jan-89 (marian)
**		Write FE catalog copy in statement to new cp*.cat file instead
**		of cp*.in file.
**	18-apr-89 (marian)
**		Remove addition information for STAR with clauses.  This will
**		now be handled by a generation of the 'register' statement by
**		STAR and will therefore be unloaded when registered objects are
**		unloaded.
**	18-may-89 (marian)
**		Add support for logical keys.  The create statement must be 
**		modified to add support for logical keys.  The copy statement
**		will not need to be changed, since AFE will handle the new
**		datatype.
**	26-jul-89 (marian)	bug 6325
**		Add the ('NULL') string following the 'with null' string
**		for character strings as well as numerics.  This will allow
**		scripts to be portable between different operating systems.
**	08-jan-90 (marian)	bug 9148
**		Handle UDT columns special.  Don't call the afe routine to do
**		the conversion, use the information stored in the attinfo 
**		structure.  Pass the 'portable' value into xfgetatts() so
**		an error can be generated when the -c flag is used to unload
**		a UDT column.
**	05-mar-1990 (mgw)
**		Changed #include <erxf.h> to #include "erxf.h" since this is
**		a local file and some platforms need to make the destinction.
**	04-may-90 (billc)
**		Major rewrite with bugfixes, etc.  Performance enhancements,
**		simplifications, and partial conversion for handling FIPS
**		namespace.
**	26-apr-91 (billc)
** 		Fix bugs 37189 and 36764.  General improvement of NULL 
**		handling.
**	08-aug-91 (billc)
** 		Fix bug 39047.  Remove ",text" from VMS file spec, COPY command
**		does this automatically.
**	09-sep-91 (billc)
** 		Fix bug 39517 (actually, a cleanup on the side).  Don't look
**		for multiple locations unless we know they're there.
**	17-jan-92 (billc)
** 		Fix bug 40139 - wasn't testing for nullable UDTs.
**	17-nov-93 (robf)
**	        Write security alarms on table if applicable
**      10-jan-1994 (andyw)
**              Fix bug 54209 in writedefault(). Was incorrectly checking
**              that has-default[0] was equal to 'N' and displaying
**              'defaults' changed to 'not default'.
**   16-Nov-1994 (sarjo01) Bug 63058
**              Assure language set to SQL prior to each create table
**              stmt in case it was changed by QUEL define integrity
**   30-Oct-1994 (Bin Li)
**              Fix bug 63319 in unloaddb, modify the xffilobj.sc file to
**              select gateway tables as part of the reloading process. But
**              we don't need to copy the gateway tables in this procedure.
**  05-sep-1995 (abowler)
**		Fix for 70744: handle long varchars in copydb.
**	9-Nov-95 (johna)	
**		Fix bug 72176 - pass the list of tables to xfconstraints
**		for processing (Note: this means that we cannot free the 
**		list before processing the indexes)
**      15-may-1996 (thaju02 & nanpr01)
**         Add local variable firstmodatt. Add 'with page_size' clause if
**         page size is greater than 2k.
**      11-Dec-1998 (hanal04)
**         In writecreate() add 'with priority' clause if the table's cache 
**         priority is not the default value of 0. b94494.
**      26-Apr-1999 (carsu07)
**         Copydb/Unloaddb generates a copy.in script which lists the 
**	   permission statements before th registered tables.  The copy.in
**	   script will fail if any permissions have been set on 
**	   registered tables because it will try to grant the permission 
**	   before registering the table. (Bug 94843)
**	09-July-1999 (thaal01) Typo removed
**  24-Nov-1999 (ashco01)
**     In xftables() call xfcomments() to generate 'COMMENT ON' statements
**     as part of 'TABLES' script.
**  11-May-2000 (fanch01)
**     Bug 101172: PERMISSIONS and CONSTRAINTS were reversed in copy.in
**       generated by unloaddb and copydb.  Moved code from xfcrscript()
**       in xfwrscrp.c to xftables() to correct ordering.
**     Added GLOBALREF With_distrib and With_registrations to accomodate
**       above code movement.
**  15-Aug-2000 (zhahu02)
**      add back row estimate clause to speed reload.
**      The client(s) need the value for subsequent processes that   
**      integrate the sql. (b102333)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      07-jun-2000 (gupsh01)
**         Modified the parameter list for xfaddloc as per the new definition
**         for xfaddloc.
**      18-sep-2000 (hanch04)
**          Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	20-may-2001 (somsa01)
**	    Replace nat and longnat with i4. Also, corrected closing braces
**	    from last cross-integration.
**      07-jun-2001 (gupsh01)
**          Remove the static declarations from writecreate and writecopy as they
**      15-Oct-2001 (hanal04) Bug 105924 INGSRV 1566
**         Modified the parameter list for xfaddloc as per the new definition
**         for xfaddloc.
**          are also used by the genxml program.
**      04-jan-2002 (stial01)
**          Implement -add_drop for -with_permits and -with_integ
**          Don't unload table registrations when output args specified
**      14-jun-2002 (stial01)
**          Test With_comments before calling xfcomments() (b108090)
**      03-oct-2002 (stial01)
**         Create user table ... with nojournaling, if table is journaled, 
**         it will be turned on AFTER the copy (for performance)
**         If fe/be catalog, minpages will always be with MODIFY, not COPY.
**      01-nov-2002 (stial01)
**         XF_INTEG_ONLY used by unloaddb should unload CHECK constraints 
**         as well as integrity constraints.
**	06-nov-2002 (somsa01)
**	    In writecopy(), when copying out with "-c", changed the
**	    "nl= d1" printout to "nl= d0nl", which is more portable across
**	    platforms.
**      09-jan-2003 (stial01)
**         XF_INTEG_ONLY used by unloaddb - process integrities only.
**         Upgradedb no longer drops and recreates check constraints to fix query
**         trees. (It upgrades the underlying system generated rules instead)
**	06-feb-2003 (devjo01) b109544.
**	   Have xfmodify check XF_MODIFY_ALLOCATION flag to see if
**	   "allocation" WITH clause item should be included with MODIFY.
**	24-feb-2003 (devjo01) b109544 part 2
**	   Additional changes for b109544 to accommodate 'ii_sequence_values',
**	   a new catalog which is hash organized.  Since catalogs are
**	   always journaled, bulk copy is never used, and MINPAGES must
**	   be on the MODIFY.  Changes made to always place ALLOCATION &
**	   MIN/MAX_PAGES on the same statement.
**      29-Apr-2003 (hweho01) 
**         Cross-integrated change 455469 by huazh01: 
**         27-Aug-2001 (huazh01)
**           Modify function xftables(). Move the code for unloading a 
**           STAR database to xfwrscrp.c
**           This fixes bug 105610.
**      12-Mar-2004 (vansa02)
**         Issueno: 12849979 Bug:110824 Problem No: INGSRV2498
**         when -relpath is used with copydb, not only copy.in but also copy.out 
**         should generate relative path for the database files.
**         The change (change num: 466196) made by gupsh01 on 15-dec-2003 in order 
**         to convert unicode datatypes to UTF8 datatypes to correctly read and 
**         write from a text file is not updated in the base code.As this change 
**         does not involve definition of any new variables/functions, I have taken
**         the file with change number 466196 to add my changes.
**	23-Apr-2004 (gupsh01)
**	    Added support for convtouni.
**	02-Jun-2004 (vansa02)
**	   Bug:112433
**	   copydb with -no_loc option leaving copy.out with realtive paths for
**	   database files and copydb with -relpath option leaving the copy.out
**	   with absolute paths.This bug is the result of checking for
**	   XF_NO_LOCATIONS in order to determine whether we should be using relative
**	   paths or not. We should have been checking XF_RELATIVE_PATH.
**	30-dec-04 (inkdo01)
**	    Added support of collations. 
**	8-feb-07 (kibro01) b117217
**	   Added TXT_HANDLE ptr to writewith
**       3-Dec-2008 (hanal04) SIR 116958
**          Added -no_rep option to exclude all replicator objects.
**          Add output_flag2 parameter to xffilltable() to allow check.
**      28-jan-2009 (stial01)
**          Use DB_MAXNAME for database objects.
**	16-Jun-2009 (thich01)
**	    Add GEOM type.
**      19-Oct-2009 (gefei01)
**          Support copydb/unloaddb for X100.
**	25-feb-2010 (joea)
**	    Add case for DB_BOO_TYPE in writecopy.
**	21-apr-2010 (toumi01) SIR 122403
**	    Add encryption support.
**	20-Aug-2009 (thich01)
**	    Add all other spatial types.
**	27-Jul-2010 (troal01)
**	    Add srid/geospatial support
**	19-Nov-2010 (kiria01) SIR 124690
**	    Add support for UCS_BASIC collation. Dispense with the hard coded
**	    coll_names using instead the table macro.
**      14-mar-2011 (stial01) m1604
**          writewith() Change value arg to handle i8 row_estimate/num_rows
**      12-Apr-2010 (hanal04) SIR 125255
**          Remove redundant blank lines from ascii COPY statements and
**          the generated data files. Added XF_BLANKLINE to allow the
**          old behaviour to be restored.
**      23-may-2011 (stial01) m1823
**          writecreate() check for vectorwise_row
**      16-Jun-2012 (ashco01/pickr01) b126591.
**          Implement -with_csv/-with_ssv options to generate copy.out/in scripts using
**          text datatype and 'csv' or 'ssv' delimiter to greatly reduce
**          portable format file size and improve export to Vectorwise.
**/
/* # define's */
/* GLOBALDEF's */
/* extern's */

GLOBALREF bool With_log_key;	/* determine if logical keys are available */
GLOBALREF bool With_UDT;   /* determine if UDTs are available */
GLOBALREF bool With_sec_audit;
GLOBALREF bool With_distrib;
GLOBALREF bool With_registrations;
GLOBALREF bool With_comments;
GLOBALREF bool  Portable;
GLOBALREF bool identity_columns;
GLOBALREF bool ivw_tabs;
GLOBALREF bool ivw_ixes;

/*
** List of valid collation names (and count thereof).
** The actual definition of these is in iicommon.h.
** Note the array is -1 origin.
*/
static char *coll_names[] = {
#   define _DEFINE(n,v,Ch,Un,t) t,
#   define _DEFINEEND
	DB_COLL_MACRO
#   undef _DEFINEEND
#   undef _DEFINE
};

static		void writeidentity(XF_COLINFO *);
static		bool xfnvarcharblock(XF_TABINFO *);

/*{
** Name:	xftables - write create statements, if necessary,
**			and copy statements.
**
** Description:
**	xftables takes a linked list of XF_TABINFO structs, each one
**	describing a table, and generates the appropriate dml statements.
**
** Inputs:
**	tp		pointer to XF_TABINFO list describing tables.
**
** Outputs:
**	writes to unload/reload scripts.
**
**	Returns:
**		OK	successful
**		FAIL	not successful
**
**	Exceptions:
**		none.
**
** Side Effects:
**	none.
**
** History:
**	13-jul-87 (rdesmond) written.
**	21-sep-88 (marian)	bug 3512
**		Move the location name down into the with clause for the create
**		statement.
**	04-may.90 (billc) use linked list of XF_COLINFOs, so we don't need
**		up-front allocation.  No longer requires a count query.
**	20-nov-1993 (jpk) change order from create-copy-modify to
**	create-modify-copy to support high performance load.
**   16-Nov-1994 (sarjo01) Bug 63058
**              Assure language set to SQL prior to each create table
**              stmt in case it was changed by QUEL define integrity
**      26-Apr-1999 (carsu07)
**         Copydb/Unloaddb generates a copy.in script which lists the
**         permission statements before th registered tables.  
**         Removing the xfpermits() call from here. (Bug 94843)
**  11-May-2000 (fanch01)
**     Bug 101172: PERMISSIONS and CONSTRAINTS were reversed in copy.in
**       generated by unloaddb and copydb.  Moved code from xfcrscript()
**       in xfwrscrp.c to xftables() to correct ordering.
**     Added GLOBALREF With_distrib and With_registrations to accomodate
**       above code movement.
**	16-nov-2000 (gupsh01)
**	    Added new input parameter output_flags for the user 
** 	    requested output statements, eg createonly, modifyonly, etc.
**	06-june-2001 (gupsh01)
**	    Added the check for the xfpermits(), to be executed when 
**	    appropriate flags are set.     
**	13-june-2001 (gupsh01)
**	    modify call to xfmodify to accept output_flags 
**	    in the input parameter list. 
**	06-feb-2003 (devjo01) b109544
**	    Use XF_MODIFY_ALLOCATION to control placement of ALLOCATION
**	    clause.
**	24-feb-2003 (devjo01) b109544 part 2
**	    Use XF_MODIFY_ALLOCATION to control placement of MIN/MAX_PAGES.
**      29-Apr-2003 (hweho01) 
**         Cross-integrated change 455469 by huazh01: 
**         27-Aug-2001 (huazh01)
**         Move the code for unloading STAR database to xfwrscrp.c
**         This fixes bug 105610.
**	06-Aug-2003 (gupsh01)
**	   (bug 110678) Fixed the case where copy statements will be 
**	   created without the fast load statements. In this situation 
**	   we should provide minpages, allocation options with the modify 
**	   statements (if requested) not with the copy statements, to avoid 
**	   error E_US14E4 error. 
**	13-Sep-2004 (schka24)
**	   Constraints are being written before permits again!  Argh!
**	   Put a stop to it once and for all.  (maybe.)
**	07-Dec-2004 (gupsh01)
**	   Modified writecopy to handle the filename parameter. 
**	15-Mar-2005 (thaju02)
**	   Moved up initialization of *list_ptr. (B114094)
**	15-Apr-2005 (thaju02)
**	   Add tlist param to xfindexes.
**	31-Oct-2006 (kiria01) b116944
**	   Increase options passable to xfmodify().
**      13-Jan-2008 (hanal04) Bug 119826
**          Resolve problems in convtouni scripts:
**          1) alter table alter column on a column that is part of an index
**          will fail. Rather than dropping and recreating indexes after
**          the alter statements issue the drop statements before trying
**          to run alter table alter column.
**      17-Mar-2008 (hanal04) SIR 101117
**          Added GroupTabIdx so that index creation can be grouped
**          by table.
**	24-Aug-2009 (wanfr01) B122319
**	    To improve performance, pass NULL to xfindexes if you need
**	    index information for all indexes, or pass in a table if you only
**	    need index information for one index.
**      19-Oct-2009 (gefei01)
**          Support copydb/unloaddb for X100.
**	10-dec-2009 (dougi)
**	    Generalize X100 logic a bit.
**	23-sep-2010 (dougi)
**	    Init ixname_x100 flags, too.
**	29-oct-2010 (dougi)
**	    Ignore ORDER_CCM for VW tables.
**	11-Apr-2011 (kschendel) SIR 125253
**	    Delete join-index column logic, no longer needed.
**      11-May-2011 (hanal04) SIR 125274
**          Pass output_flags2 down into writecreate().
*/

void
xftables(i4 *tcount, i4 output_flags, i4 output_flags2, XF_TABINFO **list_ptr)
{
    XF_TABINFO	*tlist;
    XF_TABINFO	*tp;
    STATUS 	stat;
    i4		apb;	/* ALLOCATION Placement Bit */
    bool	ivwtab;
    char	*ivwstruct;

   /* Read table information into a linked list. */
    tlist = xffilltable(tcount, output_flags2); 

    /* Return table list to caller in case caller cares */
    if ((*list_ptr = tlist) == NULL)
	return;

    xfwritehdr(TABLES_COMMENT);

    /* If CREATE TABLE fails, we will skip the COPY. 
    ** Do not skip if the XF_UNINTURRUPTED 
    ** bit is set.
    */
    if (!(output_flags) || !(output_flags & XF_UNINTURRUPTED))
    xfwrite(Xf_in, ERx("\\nocontinue\n"));

    if (output_flags2 & XF_CONVTOUNI)
    {
      update_tablist(&tlist, output_flags2);
      xfindexes(output_flags, (output_flags2 | XF_NOCREATE), NULL);
    }

    for (tp = tlist; tp != NULL; tp = tp->tab_next)
    {
        if ((STcompare(tp->storage, "x100") == 0) ||
            (STcompare(tp->storage, "x100ix") == 0) ||
            (STcompare(tp->storage, "x100cl") == 0) ||
            (STcompare(tp->storage, "x100_row") == 0) ||
            (STcompare(tp->storage, "vectorwise") == 0) ||
            (STcompare(tp->storage, "vectorwise_row") == 0) ||
            (STcompare(tp->storage, "vectorwise_ix") == 0) ||
            (STcompare(tp->storage, "vectorwise_cl") == 0) ||
            (STcompare(tp->storage, "vw") == 0))
	{
	    tp->is_x100 = TRUE;
	    tp->ixed_x100 = FALSE;
	    tp->ixname_x100 = FALSE;
	    ivwtab = TRUE;
	    ivwstruct = tp->storage;
	}
	else
	{
	    tp->is_x100 = FALSE;
	    ivwtab = FALSE;
	    ivwstruct = NULL;
	}

        xfsetlang(Xf_in, DB_SQL);  /* 11-16-94 */
	/* Did the owner name change? */
	if (output_flags2 & XF_CONVTOUNI)
	  xfsetauth(Xf_in, tp->owner);
	else
	  xfsetauth(Xf_both, tp->owner);

	apb = ( STequal(tp->storage, ERx("heap")) ||
		tp->becat || tp->fecat ||
		((output_flags & (XF_TAB_COPYONLY)) && 
		 !(output_flags & (XF_TAB_CREATEONLY|XF_PRINT_TOTAL)))) ? XF_MODIFY_ALLOCATION : 0;

	/* 
	** Write the CREATE TABLE command.
	** (There are several BE catalogs around: iirole, iiusergroup, etc.
	** We COPY them but we don't CREATE them.)
	*/
	if ((!tp->becat) &&
		(!output_flags || (output_flags & XF_TAB_CREATEONLY )
		|| (output_flags & XF_PRINT_TOTAL )
		|| (output_flags & XF_ORDER_CCM ))) 
	    writecreate(tp, output_flags, output_flags2, ivwstruct);

	if ((!tp->becat) &&
		(output_flags2 && (output_flags2 & XF_CONVTOUNI)))
	    writealtcol(tp, output_flags2);

	/* write the MODIFY command 
	** do not write modify if requested create, copy and then 
	** modify sequence of output statements. 
        ** Do not write modify if CREATE X100 TABLE.
	*/
        if(!ivwtab &&
           (!output_flags || (((output_flags & XF_TAB_MODONLY )
                || (output_flags & XF_PRINT_TOTAL ) 
                || (output_flags2 & XF_CONVTOUNI ))
                && !(output_flags & XF_ORDER_CCM ))))
	    xfmodify(tp, apb | output_flags, output_flags2);
    
	/* write the COPY command */
	/* Bug 63319, if the tables are gateway tables, we don't need to 
	   copy them.            */
        /* Do X100 table COPY at the end. */
	if (!ivwtab &&
            (STbcompare(tp->name, 0, ERx("iigw07_relation"), 0, TRUE) != 0) &&
	    (STbcompare(tp->name, 0, ERx("iigw07_attrbute"), 0, TRUE) != 0) &&
	    (STbcompare(tp->name, 0, ERx("iigw07_index"), 0, TRUE) !=0) &&
	    (!output_flags || (output_flags & XF_PRINT_TOTAL) ||
		 (output_flags & XF_TAB_COPYONLY) || 
		 (output_flags & XF_ORDER_CCM )))
           writecopy(tp, apb | output_flags, output_flags2, NULL, ivwtab);

	if (!ivwtab && 
	    (output_flags && (output_flags & XF_ORDER_CCM )))
	    xfmodify(tp, apb | output_flags, output_flags2);

        /* If requested write index alongside table */
        if ((GroupTabIdx == TRUE) && 
           (!output_flags || ((output_flags & XF_INDEXES_ONLY )
           || (output_flags & XF_PRINT_TOTAL)
           || (output_flags2 & XF_CONVTOUNI ))))
        {
            xfindexes(output_flags, output_flags2, tp);
        }

	if (tp->becat)
	    continue;

	/* alltoall and rettoall are special cases in pre-6.5 Ingres. */
	if (!output_flags || (output_flags & XF_PRINT_TOTAL )) 
	{
	    xfalltoall(tp);

	    /* turn on journalling, if needed. */
	    xfjournal(tp);
	
	    /* write the pre-65 integrity statements. */
	    xfinteg(tp);
	
	    /* write the SAVE UNTIL (!) command */
	    xfsave(tp);
	}
	else
	{
	    if ((output_flags & XF_PERMITS_ONLY)
		&&  (output_flags & XF_DROP_INCLUDE))
	    {
		xfwrite(Xf_in, ERx("drop permit on "));
		xfwrite_id(Xf_in, tp->name);
		xfwrite(Xf_in, ERx(" all"));
		xfwrite(Xf_in, GO_STMT);
	    }

	    if (output_flags & XF_INTEG_ONLY)
	    {
		if ( (output_flags & XF_DROP_INCLUDE) && tp->has_integ[0] != 'N')
		{
		    xfwrite(Xf_in, ERx("drop integrity on "));
		    xfwrite_id(Xf_in, tp->name);
		    xfwrite(Xf_in, ERx(" all"));
		    xfwrite(Xf_in, GO_STMT);
		}
		/* write the pre-65 integrity statements. */
		xfinteg(tp);
	    }
	}

	/* Generate COMMENT ON statement for table */
	if (!output_flags || (output_flags & XF_COMMENT_ONLY )
		|| (output_flags & XF_PRINT_TOTAL ))
	if (With_comments)
	    xfcomments(tp);  
	
    }

    if(output_flags2 & XF_CONVTOUNI)
    {
        output_flags |= XF_TAB_CREATEONLY;
    }

    /* Generate CREATE INDEX statements */
    if ((GroupTabIdx == FALSE) 
        && (!output_flags || ((output_flags & XF_INDEXES_ONLY )
           || (output_flags & XF_PRINT_TOTAL)
           || (output_flags2 & XF_CONVTOUNI )) ))
    xfindexes(output_flags, output_flags2, NULL);

    /* Do not generate constraints yet, we need permits first. */
  
}

/*{
** Name:	writecreate - write create statement, if necessary.
**
** Description:
**	Write a CREATE TABLE command to the reload script.
**
** Inputs:
**	tp		pointer to XF_TABINFO structure describing object.
**
** Outputs:
**	none.
**
**	Returns:
**		none.
**
** History:
**	04-may-90 (billc) spawned from xfcopy.
**
**	2-sep-93 (robf)
**	   Added handling for secure attributes
**      15-may-1996 (thaju02 & nanpr01)
**         Add local variable firstmodatt. Add 'with page_size' clause if
**         page size is greater than 2k.
**      11-Dec-1998 (hanal04)
**         Add 'with priority' clause if the table's cache priority is not
**         the default value of 0. b94494.
**      07-jun-2000 (gupsh01)
**         Modified the parameter list for xfaddloc as per the new definition
**         for xfaddloc.
**	07-jun-2001 (gupsh01)
**	   Made this function non static, for use by the impxml utility.
**	   The xfaddloc will not be printed for XF_XMLFLAG. 
**      03-oct-2002 (stial01)
**         Create user table ... with nojournaling, if table is journaled,
**         it will be turned on AFTER the copy (for performance)
**      31-Jul-2003 (hanal04) SIR 110651 INGDBA 236
**         Do not generate nojournaling option if user opted for a fully
**         journaled copy.in.
**	7-Jul-2004 (schka24)
**	    Write partition definition for partitioned tables.
**	27-Apr-2006 (gupsh01)
**	   Added \p\g after create table statement. -with_tables does not
**	   write any \p\g statements.
**	17-nov-2008 (dougi)
**	    Added call to writeidentity() to support identity columns.
**	04-Feb-2009 (thaju02)
**	    Always specify "with page_size=..." clause. (B99758)
**      19-Oct-2009 (gefei01)
**          Support copydb/unloaddb for X100.
**	10-dec-2009 (dougi)
**	    Generalize X100 logic a bit.
**	23-Jun-2010 (coomi01) b123927
**	    Where just creating tables, and not copying them, allow
**	    journaling if appropriate.
**	29-Jun-2010 (coomi01) b123927
**	    Adjust test for former code change to operate in
**	    the negative
**	07-Oct-2010 (miket) SIR 122403
**	    Fix xfwrite for MODIFY ENCRYPT PASSPHRASE to avoid extraneous \p\g.
**	14-Dec-2010 (troal01)
**	    Ignore SRID check if _WITH_GEO is not defined.
**      11-May-2011 (hanal04) SIR 125274
**          If user specified -nvarchar temporarily flag the adf_cb with
**          ADF_TYPE_TO_UNICODE to show we want unicode equivalent type names.
**          Also warn the user if they have C or TEXT types in -nvarchar mode.
**          The reload may hit truncation errors.
*/

void
writecreate(XF_TABINFO	*tp, i4 output_flags, i4 output_flags2, char *ivwstruct)
{
    XF_COLINFO	*ap;
    XF_TABINFO  *x100_tp;
    auto DB_DATA_VALUE	dbdv;
    auto DB_USER_TYPE	att_spec;
    char	tbuf[256];
    bool	got_aud_key=FALSE;
    ADF_CB	*ladfcb;

    if ((tp->fecat) || 
	(output_flags &&
	( output_flags & XF_DROP_INCLUDE )))
    {
        /* FE catalogs must be DROPped before being recreated. */
	xfwrite(Xf_in, ERx("drop table "));
        xfwrite_id(Xf_in, tp->name);
	xfwrite(Xf_in, GO_STMT);
    }

    /* Write CREATE statement */

    xfwrite(Xf_in, ERx("create table "));
    xfwrite_id(Xf_in, tp->name);
    xfwrite(Xf_in, ERx("(\n\t"));
    if (ivwstruct)
	ivw_tabs = TRUE;

    ladfcb = FEadfcb();
    if((output_flags2 & XF_NVARCHAR) && 
       tp->becat == FALSE && tp->fecat == FALSE)
    {
        ladfcb->adf_misc_flags |= ADF_TYPE_TO_UNICODE;
    }
    else
    {
        ladfcb->adf_misc_flags &= ~ADF_TYPE_TO_UNICODE;
    }

    /* Write out the column definitions for this table. */
    for (ap = tp->column_list; ap != NULL; ap = ap->col_next)
    {
        /* Remove the join index tid column on X100 table. This
	** is a bit of a hack, depending on join index columns having
	** the name _j<table name>. We can probably do better. But first,
	** note that table is indexed. */
	if (ivwstruct && ap->key_seq > 0)
	{
	    ivw_ixes = TRUE;
	    tp->ixed_x100 = TRUE;
	}

        if (ivwstruct && ap->column_name[0] == '_' && ap->column_name[1] == 'j' 
	    && STcompare(tp->name, &(ap->column_name[2])) == 0)
	    continue;

	if (ap->col_seq != 1)
	    xfwrite(Xf_in, ERx(",\n\t"));

	xfwrite_id(Xf_in, ap->column_name);
	xfwrite(Xf_in, ERx(" "));

	/*
	** fill in DB_DATA_VALUE for call to
	** afe_tyoutput to get attribute specification
	*/
	dbdv.db_datatype = (DB_DT_ID)ap->adf_type;
	dbdv.db_length = (i4)ap->intern_length;
	dbdv.db_prec = (i2)ap->precision;

	/* Get column specification. */

	/*
	** bug 9148
	** Check to see if this is a UDT.  If it is a UDT,
	** copy the internal datatype.
	**
	** bug 40139
	** make sure we test for nullable UDTs, too!
	*/
	if (IS_UDT(dbdv.db_datatype))
	{
	    xfwrite(Xf_in, ap->col_int_datatype);
	    if (dbdv.db_length != 0)
	    {
		/*
		** A length specifier required for this UDT specification.  
		** WARNING: what about scale?  need new column in iicolumns.
		*/
		STprintf(tbuf, ERx("(%d)"), dbdv.db_length);
	    	xfwrite(Xf_in, tbuf);
	    }
	}
	else
	{
	    /* 
	    ** jup7414 - afe_tyoutput() gives INGRES types, like i2, i4, etc.
	    **  For portability, call IIAFgtm_gtwTypeMap for type specs like
	    **  integer, smallint, etc.
	    */
	    if (IIAFgtm_gtwTypeMap(&dbdv, &att_spec) != OK)
	    {
		/* 
		** However, IIAFgtm_gtwTypeMap doesn't handle MONEY, DATE,
		** DECIMAL, and other good things. 
		*/
		if (afe_tyoutput(ladfcb, &dbdv, &att_spec) != OK)
		    FEafeerr(ladfcb);
	    }

	    /* write attribute spec to input file */
	    xfwrite(Xf_in, att_spec.dbut_name);
	}

#ifdef _WITH_GEO
	if (ap->srid != SRID_UNDEFINED)
	{
		STprintf(tbuf, ERx(" SRID %d "), ap->srid);
		xfwrite(Xf_in, tbuf);
	}
#endif

	/* check nullability in spec */
	if (ap->nulls[0] == 'N')
	    xfwrite(Xf_in, ERx(" not null"));

	/* check for identity columns or defaults and user-defined defaults. */
	if (ap->always_ident[0] == 'Y' || ap->bydefault_ident[0] == 'Y')
	    writeidentity(ap);
	else writedefault(ap);

	/* Check for collate clause. */
	if (ap->collID > DB_NOCOLLATION)
	{
	    xfwrite(Xf_in, ERx(" collate "));
	    xfwrite(Xf_in, coll_names[ap->collID+1]);
	}

	/*
	** If this is a column of type "logical_key", add the 
	** system_maintained information.
	*/
	if (ap->log_key)
	{
	    STprintf(tbuf, ERx(" %s system_maintained"), 
		    (ap->sys_maint[0] == 'Y') ? ERx("with") : ERx("not"));
	    xfwrite(Xf_in, tbuf);
	}

	/*
	** If this column is encrypted add that info.
	*/
	if (ap->column_encrypted[0] == 'Y')
	{
	    xfwrite(Xf_in, ERx(" encrypt"));
	    if (ap->column_encrypt_salt[0] == 'N')
		xfwrite(Xf_in, ERx(" nosalt"));
	}


        if((output_flags2 & XF_NVARCHAR) &&
           (output_flags2 & XF_TRUNC_WARN) == 0)
        {
            switch(dbdv.db_datatype)
            {
                case DB_CHR_TYPE:
                case -DB_CHR_TYPE:
                case DB_TXT_TYPE:
                case -DB_TXT_TYPE:
                    /* C and TEXT target column types are unaffected. The UTF8
                    ** encoding of the data may lead to truncation during a
                    ** reload. Warn the user.
                    */
                    output_flags2 |= XF_TRUNC_WARN;
                    IIUGerr(E_XF0063_Reload_Trunc_Warning, 
                            UG_ERR_ERROR, 0);
            }
        }
    }

    /* Make a lingering ADF_TYPE_TO_UNICODE cannot affect other calls */
    ladfcb->adf_misc_flags &= ~ADF_TYPE_TO_UNICODE;

    /* Add UNIQUE/PRIMARY KEY definitions for X100 tables. */
    if (ivwstruct)
	xfuconsts( tp );

    xfwrite(Xf_in, ERx("\n)\n"));

    /* Add WITH STRUCTURE=X100 clause */
    if (ivwstruct)
    {
        if (STcompare(ivwstruct, "vectorwise_row") == 0
	    || STcompare(ivwstruct, "x100_row") == 0)
	    STprintf(tbuf, ERx("with structure=vectorwise_row"));
	else
	    STprintf(tbuf, ERx("with structure=vectorwise"));;
        xfwrite(Xf_in, tbuf);

        goto done;
    }
    /* always write "duplicates" spec since defaults could be different. */
    STprintf(tbuf, ERx("with %sduplicates"), 
		    (tp->duplicates[0] == 'D') ? ERx("") : ERx("no"));
    xfwrite(Xf_in, tbuf);

#if defined(conf_IVW)
    /* now safe to explicitly add "structure = heap" (since we've passed
    ** VW test).  Don't do this for pure-Ingres builds, since Ingres servers
    ** up through 10.0 don't understand about structure=heap on a create
    ** table statement.  The structure is implied if a pure-ingres build.
    */
    xfwrite(Xf_in, ERx(",\nstructure = heap"));
#endif
    
    /*
    ** create user tables with nojournaling, since defaults could be different
    ** If this table is journaled, "set journaling on table" will be done
    ** after the copy for better performance
    */
    if (!tp->fecat && !tp->becat && !SetJournaling)
    {
	if (((tp->journaled[0] == 'Y' || tp->journaled[0] == 'C')) &&
	    ( output_flags &&
	      !((output_flags & XF_PRINT_TOTAL)  ||
		(output_flags & XF_TAB_COPYONLY) || 
		(output_flags & XF_ORDER_CCM )   ||
		(output_flags & XF_XMLFLAG))
		)
	    )
	{
	    /* 
	    ** Where NO copy is intended on journaled tables, 
	    ** put flag in immediately.
	    */
	    xfwrite(Xf_in, ERx(",\njournaling"));
	}
	else
	{
	    xfwrite(Xf_in, ERx(",\nnojournaling"));
	}
    }

    STprintf(tbuf, ERx(",\npage_size = %d"), tp->pagesize); 
    xfwrite(Xf_in, tbuf);

    /*
    ** bug 3512
    ** The location name(s) should be specified in the
    ** with clause of the create statement.  
    */
    if (!(output_flags) || !(output_flags & XF_NO_LOCATIONS))
    {
    xfwrite(Xf_in, ERx(",\nlocation = ("));
    xfwrite(Xf_in, tp->location);

    /* look for additional locations */
    if (tp->multi_locations[0] == 'Y')
	if (!(output_flags & XF_XMLFLAG))
	    xfaddloc(tp->name, tp->owner, FALSE, NULL);
	else
	    xfwrite(Xf_in, tp->location_list);
    xfwrite(Xf_in, ERx(")"));
    }
    
    /* b94494 - If priority is not default of 0 add the priority clause */
    if(tp->priority != 0)
    {
       STprintf(tbuf, ERx(",\npriority = %d"), tp->priority); 
       xfwrite(Xf_in, tbuf);
    }

    /* If partitioned table, write the partition definition */

    if (tp->partition_dimensions > 0)
    {
	xfwrite(Xf_in, ",\n");
	IIUIpd_PartitionDesc(tp->name, tp->owner, tp->partition_dimensions,
		TRUE, xfwrite, Xf_in);
    }

    /*
    ** Add table-level encryption options.
    */
    if (tp->encrypted_columns[0] == 'Y')
    {
	STprintf(tbuf, ERx(",\nencryption = %s"), tp->encryption_type); 
	xfwrite(Xf_in, tbuf);
	xfwrite(Xf_in, ERx(", passphrase = 'TEMPORARY PASSPHRASE'"));
    }

    /*
    ** Write any secure options
    */

    if(With_sec_audit)
    {
	/*
	** Per-table security audit options
	*/
    	if(tp->row_sec_audit[0]=='Y')
		xfwrite(Xf_in, ERx(",\nsecurity_audit=(table,row)"));
    	else
		xfwrite(Xf_in, ERx(",\nsecurity_audit=(table,norow)"));

    	/*
    	** Look for any security audit key columns
    	*/
    	for (ap = tp->column_list; ap != NULL; ap = ap->col_next)
    	{
		if(ap->audit_key[0]=='Y')
		{
			if(!got_aud_key)
			{
				xfwrite(Xf_in, ERx(",\nsecurity_audit_key=("));
				got_aud_key=TRUE;
			}
			else
				xfwrite(Xf_in, ERx(","));
			xfwrite_id(Xf_in, ap->column_name);
			xfwrite(Xf_in, ERx(" "));
		}
    	}
        if(got_aud_key)
		xfwrite(Xf_in, ERx(")"));
    }
	
done:

    xfwrite(Xf_in, GO_STMT);

    /*
    ** If encryption add a MODIFY ENCRYPT PASSPHRASE placeholder.
    */
    if (tp->encrypted_columns[0] == 'Y')
    {
	xfwrite(Xf_in, ERx("modify "));
	xfwrite_id(Xf_in, tp->name);
	xfwrite(Xf_in, ERx(" encrypt with passphrase = 'TEMPORARY PASSPHRASE'"));
	xfwrite(Xf_in, GO_STMT);
    }

}

/*{
** Name:	writecopy - write copy statement
**
** Description:
**
** Inputs:
**	tp		pointer to XF_TABINFO structure describing object.
**
** Outputs:
**
**	Returns:
**
** History:
**	04-may-90 (billc) spawned from xfcopy, for easier control.
**      30-may-90 (billc) if portable, make sure VMS output file is 'text'.
**      08-may-91 (billc) Fix 37343, which was a side effect of covering
**		a COPY bug that is now fixed.
**      07-aug-91 (billc) Fix 39047 by undoing 30-may-90 change.  Copy command
**		should handle this automatically on VMS.
**	23-aug-93 (jpk) Rich Muth says emptytable is dead.
**	20-nov-1993 (jpk) add row_estimate clause to support high performance
**		loading.
**	28-aug-1997 (nanpr01)
**		copydb converts long byte to varchar. I should make it
**		long byte. This should be portable across system if 
**		long byte doesnot contain any platform specific binary
**		data. bug 84828.
**    11-feb-1999 (nicph02)
**            minpages/maxpages clauses for HASH tables moved from
**            the 'modify' command to 'copy'. This ensures that a
**            bulk load will occur even if minpages>16 (bug 94487).
**	16-nov-2000 (gupsh01)
**	    added new parameter output_flags for the user requested output
**	    statements.
**      07-jun-2001 (gupsh01)
**         Made this function non static, for use by the impxml utility.
**	13-jun-2001 (gupsh01)
**	    Added check for order_ccm flag in this function. For hash
**	    structure, with [min, max] pages oprins must be specified
**	    with the modify rather that the copy code, if order_ccm is 
**	    set.
**	14-jun-2001 (gupsh01)
**	    If the minpages and maxpages are both not specified then change 
**	    writing the "with row_estimate" statement.
**	06-nov-2002 (somsa01)
**	    When copying out with "-c", changed the "nl= d1" printout
**	    to "nl= d0nl", which is more portable across platforms.
**	24-feb-2003 (devjo01) b109544 part 2
**	   Changes made to always place ALLOCATION & MIN/MAX_PAGES 
**	   together.  Either on COPY (here), or MODIFY.
**	05-mar-2003 (devjo01)
**	   Previous change incorrectly handled XF_ORDER_CCM case.
**	15-dec-2003 (gupsh01)
**	   Added cases for nchar and nvarchar data types, based on new
**	   changes to copy code. format nchar(0) and nvarchar(0) convert
**	   unicode datatypes to UTF8 datatypes, in order to correctly read
**	   and write from a text file.
**	07-dec-2004 (gupsh01)
**	    Add filename parameter in case caller provides a copy filename.
**	11-Mar-2005 (schka24)
**	    copy out long nvarchar as long nvarchar(0), not as c0.
**	29-Sep-2005 (mutma03)
**	    suppress the clause "allocation" for partitioned tables.
**	03-aug-2006 (gupsh01)
**	    Added support for ANSI date/time datatypes.
**      19-Oct-2009 (gefei01)
**          Support copydb/unloaddb for X100.
**	10-dec-2009 (dougi)
**	    Generalize X100 logic a bit.
**      11-May-2011 (hanal04) SIR 125274
**          Use nvarchar instead of varchar and long nvarchar instead of
**          long varchar for copy coercion when XF_NVARCHAR is set. This 
**          generates UTF8 encoded data files.
**      17-May-2011 (hanal04) SIR 125274
**          Do not use nvarchar copy coercion in place of varchar copy 
**          coercion for tables that use varchar columns like byte data.
*/

/*
** Fix bug 37189, sort of.  This string is pretty obscure and unlikely to
** match user data.  But someday, somewhere...
*/
static char *Nullstr = ERx("]^NULL^["); 

void
writecopy(XF_TABINFO *tp, i4 output_flags, i4 output_flags2, char *cpfile,
	bool ivwtab)
{
    auto bool		asciitype;
    char		buf[1024];
    char		*filename;
    auto bool		firstmodatt=TRUE;
    XF_TABINFO          *x100_tp;
    bool		nvarchar_copy = FALSE;

    if (Portable &&
       (STbcompare(tp->name, 0, ERx("ii_encoded_forms"), 0, TRUE) ==0 ))
    {
	/*
	**  bug 20967 - don't copy "ii_encoded_forms" if the user specified -c.
	**  ii_encoded_forms is in binary format and not portable.
	*/
	return;
    }

    /* write copy statement */
    xfwrite(Xf_both, ERx("copy "));
    xfwrite_id(Xf_both, tp->name);

    if (Portable)
    {
	XF_COLINFO	*ap;

	/*
	** User entered -c flag on command line, so we want COPY to generate
	** files of printable characters.  
	*/

        if((output_flags2 & XF_NVARCHAR) && (xfnvarcharblock(tp) == FALSE))
        {
            nvarchar_copy = TRUE;
        }

	xfwrite(Xf_both, ERx("(\n\t"));

	for (ap = tp->column_list; ap != NULL; ap = ap->col_next)
	{
	    auto DB_DATA_VALUE	dbdv1;
	    auto DB_DATA_VALUE	dbdv2;

            /* Remove the join index tid column on X100 table. This
	    ** is a bit of a hack, depending on join index columns having
	    ** the name _j<table name>. We can probably do better. */
            if (ivwtab && ap->column_name[0] == '_' && 
		ap->column_name[1] == 'j' &&
		STcompare(tp->name, &(ap->column_name[2])) == 0)
		continue;

	    if (ap->col_seq > 1)
		xfwrite(Xf_both, ERx(",\n\t"));

	    xfwrite_id(Xf_both, ap->column_name);
	    xfwrite(Xf_both, ERx("= "));

	    switch(ap->adf_type)
	    {
            case DB_BOO_TYPE:
            case -DB_BOO_TYPE:
	    case DB_DTE_TYPE:
	    case -DB_DTE_TYPE:
	    case DB_ADTE_TYPE:
	    case -DB_ADTE_TYPE:
	    case DB_TMWO_TYPE:
	    case -DB_TMWO_TYPE:
	    case DB_TMW_TYPE:
	    case -DB_TMW_TYPE:
	    case DB_TME_TYPE:
	    case -DB_TME_TYPE:
	    case DB_TSWO_TYPE:
	    case -DB_TSWO_TYPE:
	    case DB_TSW_TYPE:
	    case -DB_TSW_TYPE:
	    case DB_TSTMP_TYPE:
	    case -DB_TSTMP_TYPE:
	    case DB_INYM_TYPE:
	    case -DB_INYM_TYPE:
	    case DB_INDS_TYPE:
	    case -DB_INDS_TYPE:
	    case DB_MNY_TYPE:
	    case -DB_MNY_TYPE:
		/* money and date have 0 length, so varchar() won't work */
                if (output_flags2 & XF_CSV || output_flags2 & XF_SSV)
                    xfwrite(Xf_both, ERx("text(0)"));
                else
		    xfwrite(Xf_both, ERx("c0"));
		break;

	    case DB_LOGKEY_TYPE:
	    case -DB_LOGKEY_TYPE:
	    case DB_TABKEY_TYPE:
	    case -DB_TABKEY_TYPE:
		/*
		** bug 36764: logical keys must use varchar(), but 
		** afe_cancoerce doesn't think logical keys are coercible
		** to varchar.
		*/
		if (ap->sys_maint[0] == 'Y')
		    xfwrite(Xf_both, ERx("d0"));
		else
                    if (output_flags2 & XF_CSV || output_flags2 & XF_SSV)
                        xfwrite(Xf_both, ERx("text(0)"));
                    else
		        xfwrite(Xf_both, ERx("varchar(0)")); 
		break;

           case DB_LVCH_TYPE:
           case -DB_LVCH_TYPE:
               /*
               ** bug 70744: handle long varchars
               */
               if(nvarchar_copy)
               {
                   xfwrite(Xf_both, ERx("long nvarchar(0)"));
               }
               else
               {
                   xfwrite(Xf_both, ERx("long varchar(0)"));
               }
               break;

           case DB_LBYTE_TYPE:
           case -DB_LBYTE_TYPE:
               /*
               ** bug 84828 : handle long varchars
               */
               xfwrite(Xf_both, ERx("long byte(0)"));
               break;

           case DB_GEOM_TYPE:
               xfwrite(Xf_both, ERx("geom(0)"));
               break;

           case DB_POINT_TYPE:
               xfwrite(Xf_both, ERx("point(0)"));
               break;

           case DB_MPOINT_TYPE:
               xfwrite(Xf_both, ERx("multipoint(0)"));
               break;

           case DB_LINE_TYPE:
               xfwrite(Xf_both, ERx("line(0)"));
               break;

           case DB_MLINE_TYPE:
               xfwrite(Xf_both, ERx("multiline(0)"));
               break;

           case DB_POLY_TYPE:
               xfwrite(Xf_both, ERx("polygon(0)"));
               break;

           case DB_MPOLY_TYPE:
               xfwrite(Xf_both, ERx("multipolygon(0)"));
               break;

           case DB_GEOMC_TYPE:
               xfwrite(Xf_both, ERx("geomcollection(0)"));
               break;

	   case DB_NCHR_TYPE:
           case -DB_NCHR_TYPE:
               if (output_flags2 & XF_CSV || output_flags2 & XF_SSV)
                   xfwrite(Xf_both, ERx("text(0)"));
               else
                   xfwrite(Xf_both, ERx("nchar(0)"));
               break;

	   case DB_NVCHR_TYPE:
           case -DB_NVCHR_TYPE:
               if (output_flags2 & XF_CSV || output_flags2 & XF_SSV)
                   xfwrite(Xf_both, ERx("text(0)"));
               else
                   xfwrite(Xf_both, ERx("nvarchar(0)"));
               break;

	    case DB_LNVCHR_TYPE:
	    case -DB_LNVCHR_TYPE:
               xfwrite(Xf_both, ERx("long nvarchar(0)"));
               break;


	    default:
		/* determine whether datatype is coercible to char type. */
		dbdv1.db_datatype = DB_CHR_TYPE;
		dbdv1.db_length = (i4)ap->intern_length;
		dbdv2.db_datatype = (DB_DT_ID)ap->adf_type;
		dbdv2.db_length = (i4)ap->intern_length;

		(void) afe_cancoerce(FEadfcb(), &dbdv1, &dbdv2, &asciitype);
		if (asciitype)
                {
                    if(nvarchar_copy)
                    {
                        switch(ap->adf_type)
                        {
                            case DB_CHR_TYPE:
                            case -DB_CHR_TYPE:
                            case DB_TXT_TYPE:
                            case -DB_TXT_TYPE:
                            case DB_CHA_TYPE:
                            case -DB_CHA_TYPE:
                            case DB_VCH_TYPE:
                            case -DB_VCH_TYPE:
                            if ( !(output_flags2 & XF_CSV) && !(output_flags2 & XF_SSV))
                                xfwrite(Xf_both, ERx("n"));
                        }

                    }
                    if ( output_flags2 & XF_CSV  || output_flags2 & XF_SSV)
                        xfwrite(Xf_both, ERx("text(0)"));
                    else
                        xfwrite(Xf_both, ERx("varchar(0)"));
                }
		else
                    if ( output_flags2 & XF_CSV  || output_flags2 & XF_SSV)
                        xfwrite(Xf_both, ERx("text(0)"));
                    else
		        xfwrite(Xf_both, ERx("c0"));
		break;
	    }
            /* if -with_csv or ssv, use comma, except for long datatypes */
            if (  (output_flags2 & XF_CSV || output_flags2 & XF_SSV)
                && ( ap->adf_type != DB_LNVCHR_TYPE)
                && ( ap->adf_type != DB_LVCH_TYPE)
                && ( ap->adf_type != -DB_LNVCHR_TYPE)
                && ( ap->adf_type != -DB_LVCH_TYPE) ) 
                /* csv format does not support 'long' datatypes */
                if (output_flags2 & XF_CSV) xfwrite(Xf_both, ERx("csv"));
                else xfwrite(Xf_both, ERx("ssv"));
            else
              xfwrite(Xf_both, ap->col_next != NULL ? ERx("tab") : ERx("nl"));

	    /*
	    ** bug 6325
	    ** Handle nulls the same for character and non-character datatypes. 
	    */
	    if (ap->nulls[0] == 'Y')
	    {
		STprintf(buf, ERx(" with null('%s')"), Nullstr);
		xfwrite(Xf_both, buf);
	    }
	}
        if(output_flags2 & XF_BLANKLINE)
        {
            xfwrite(Xf_both, ERx(",\n\tnl= d0nl)\n"));
        }
        else if ( output_flags2 & XF_CSV || output_flags2 & XF_SSV)
            xfwrite(Xf_both, ERx("\n\t)\n"));
        else
        {
	    xfwrite(Xf_both, ERx(")\n"));
        }
    }
    else
    {
	/* Simple case: binary COPY. */
	xfwrite(Xf_both, ERx(" () "));
    }

    /* Generate a name for the COPY file for this table. */
    if (cpfile)
      filename = cpfile;
    else
    {
      filename = xfnewfname(tp->name, tp->owner);
      if (filename == NULL)
      {
	IIUGerr(E_XF0001_Cannot_generate_file, UG_ERR_ERROR, 0);
	return;
      }
    }

    if (output_flags & XF_RELATIVE_PATH)
    STprintf(buf, ERx("from '%s'"), filename);
    else
    STprintf(buf, ERx("from '%s'"), xfaddpath(filename, XF_FROMFILE));
    xfwrite(Xf_in, buf);

    if (!(output_flags & XF_XMLFLAG))
    {
      if (output_flags & XF_RELATIVE_PATH)
      STprintf(buf, ERx("into '%s'"), filename);
      else 
      STprintf(buf, ERx("into '%s'"), xfaddpath(filename, XF_INTOFILE));
      xfwrite(Xf_out, buf);
    }

    /*
    ** Only add minpages, maxpages, and/or allocation to COPY "WITH"
    ** clause, if not heap or a catalog asked (not XF_MODIFY_ALLOCATION),
    ** and the COPY statement will appear after the MODIFY. (No "-order_ccm",
    ** or "-with_table" plus "-with_data" and "-with_modify" missing.)
    */
    if (!(output_flags & (XF_MODIFY_ALLOCATION|XF_ORDER_CCM)) && 
	!(XF_TAB_CREATEONLY ==
	  (output_flags & (XF_TAB_CREATEONLY|XF_TAB_MODONLY)))) 
    {
	/* add minpages & maxpages for hash storage */
	if (STequal(tp->storage, ERx("hash")))
	{

	   if (tp->minpages != 0)
	     writewith(ERx("minpages"), (i8)tp->minpages, &firstmodatt, Xf_in);
	   if (tp->maxpages != 0 && tp->maxpages != XF_MAXPAGES)
	     writewith(ERx("maxpages"), (i8)tp->maxpages, &firstmodatt, Xf_in);
	}

	if ((tp->allocation_size > 0) && (tp->partition_dimensions == 0) &&
	    !(tp->is_x100))
	{
	    /* Put allocation clause on COPY statement except for partitioned
	       tables and X100 tables
	     */
	    writewith(ERx("allocation"), (i8)tp->allocation_size, &firstmodatt,
		Xf_in);
	}
    }

    /* add back row estimate clause to speed reload, */
    /* and for clients' subsequent processes to integrate sql (issue 9172832) */
    writewith(ERx("row_estimate"), tp->num_rows, &firstmodatt, Xf_in);

    xfwrite(Xf_both, GO_STMT);

    return;
}

/*{
** Name:	writeidentity - write AS IDENTITY clause
**
** Description:
**
** Inputs:
**	cp		pointer to XF_COLINFO structure describing object.
**
** Outputs:
**
**	Returns:
**
** History:
**	16-nov-2008 (dougi)
**	    Written for identity column support.
*/

void
writeidentity(XF_COLINFO *cp)
{
    char	*ident_name = &cp->default_value[14];

    if (cp->always_ident[0] == 'Y')
	xfwrite(Xf_in, ERx(" generated always as "));
    else xfwrite(Xf_in, ERx(" generated by default as "));

    /* Hack sequence name out of default_value 
    ** ("next value for <sequence name>"). */
    xfwrite(Xf_in, ident_name);

    xfwrite(Xf_in, ERx(" identity"));

    identity_columns = TRUE;		/* triggers generation of ALTER */
}

/*{
** Name:	writedefault - write DEFAULT clause.
**
** Description:
**
** Inputs:
**	cp		pointer to XF_COLINFO structure describing object.
**
** Outputs:
**
**	Returns:
*/

void
writedefault(XF_COLINFO *cp)
{
    if (!With_65_catalogs)
    {
	/* pre-6.5 defaults. */
	if (cp->defaults[0] == 'Y')
	    xfwrite(Xf_in, ERx(" with default"));
	else if (cp->nulls[0] == 'N')
	    xfwrite(Xf_in, ERx(" not default"));
    }
    else if (cp->default_value == NULL || cp->default_value[0] == EOS)
    {
	if (cp->has_default[0] == 'N')
	    xfwrite(Xf_in, ERx(" not default"));
    }
    else
    {
	xfwrite(Xf_in, ERx(" default "));
	xfwrite(Xf_in, cp->default_value);
    }
}

/*{
** Name:	xfsetauth - check if user id should be reset.
**
** Description:
**	Checks to see if given user id is the current effective user id. 
**	If not, then write out a SET USER command.
**
** Inputs:
**	tfd -	handle of the file to write to.
**	owner -	name of an object owner.
**
** Outputs:
**	none.
**
** History:
**	30-feb-92 (billc) first written.
*/

static char	Last_owner[DB_MAXNAME + 1] = ERx("");

void
xfsetauth(TXT_HANDLE *tfd, char *owner)
{
    if (Owner[0] == EOS && owner != NULL &&
	(STbcompare(Last_owner, 0, owner, 0, TRUE) != 0))
    {
	/* 
	** Change effective user id.  
	*/

	/* SET SESSION in SQL only */
	xfsetlang(tfd, DB_SQL);

	xfwrite(tfd, ERx("set session authorization "));

	xfwrite_id(tfd, owner);

	xfwrite(tfd, GO_STMT);

	STlcopy(owner, Last_owner, (i4) sizeof(Last_owner) - 1);
    }
}

/*{
** Name:	xfsetlang - set language if it's changed.
**
** Description:
**	Checks to see if given language is the current language.
**	If not, write out a \quel or \sql command.
**	xfsetlang can write either directly to the reload file or, if
**	specified, one of our text buffers.
**
** Inputs:
**	tfd 	- text handle to write to.
**	dml	- language specifier.
**
** Outputs:
**	writes to the reload script.
**
** History:
**	30-feb-92 (billc) first written.
*/

static DB_LANG	Last_lang = DB_NOLANG;

void
xfsetlang(TXT_HANDLE *tfd, DB_LANG dml)
{
    if (dml != Last_lang)
    {
	xfwrite(tfd, (dml == DB_SQL ? ERx("\\sql\n") : ERx("\\quel\n")));
	Last_lang = dml;
    }
}

/*{
** Name:	xfwritehdr - write a comment that announces the next section.
**
** Description:
**	Write out a comment that denotes the next set of things being 
**	handled by the script.  Always writes to Xf_in.
**
** Inputs:
**	msg	id of the message to write.
**
** History:
**	30-feb-92 (billc) first written.
*/

void
xfwritehdr(ER_MSGID msg)
{
    xfwrite(Xf_in, ERx("\n\t/* "));
    xfwrite(Xf_in, ERget(msg));
    xfwrite(Xf_in, ERx(" */\n"));
}

/*{
** Name:	xfwrite_id - write a name, which might be a delimited id. 
**
** Description:
**	Write out a name, which might be a delimited identifier.
**	If it is a delimited identifier, we got it from the database,
**	so it is normalized.  This routine denormalizes -- that is,
**	escapes embedded quotes and such.
**
**	This routine should be used for user names, schema names, table names,
**	column names, etc.
**
**	We call this routine several times on table names, so perhaps
**	we should cache a copy of the output inside the XF_TABINFO struct.
**
** Inputs:
**	tfd	the file to write to.
**	name	the name to write.
**
** History:
**	30-feb-92 (billc) first written.
*/

void
xfwrite_id(TXT_HANDLE	*tfd, char	*name)
{
    char	nbuf[((2 * DB_MAXNAME) + 2 + 1)];

    if (IIUGdlm_ChkdlmBEobject(name, NULL, TRUE))
    {
	IIUGrqd_Requote_dlm(name, nbuf);
	xfwrite(tfd, nbuf);
    }
    else
    {
	xfwrite(tfd, name);
    }
}

/*{
** Name:	xfread_id - prepare a name, which might be a delimited id.
**
** Description:
**	Prepare a name, which might have trailing whitespace, for 
**	internal use as an id.
**	This should be used for table names, column names, etc.
**
** Inputs:
**	name	the name to prepare.  Note that this routine will crash if
**		the name's buffer is less than 2 bytes in length.
**
** History:
**	30-feb-92 (billc) first written.
*/

void
xfread_id(char	*name)
{
    if (STtrmwhite(name) == 0)
    {
	/* Handle the case of delimited ids that are a single blank. */
	name[0] = ' ';
	name[1] = EOS;
    }
}

/*{
** Name:        xfnvarcharblock - Check to see whether a table not use
**                                nvarchar copy coercion.
**
** Description:
**      A number of system catalogs have varchars that are used as byte
**      storage. These tables must not have their character columns
**      transliterated to UTF8 when XF_NVARCHAR is set.
**
** Inputs:
**      tp      pointer to XF_TABINFO structure describing object.
**
** History:
**      17-May-2011 (hanal04) SIR 125274
*/

static
bool
xfnvarcharblock(XF_TABINFO *tp)
{
    bool	block = FALSE;

    if(tp->becat || tp->fecat)
    {
        if((STncasecmp(tp->name, "ii_encoded_forms", 
                  sizeof("ii_encoded_forms")) == 0) ||
           (STncasecmp(tp->name, "iitree",
                  sizeof("iitree")) == 0) ||
           (STncasecmp(tp->name, "iiddb_tree",
                  sizeof("iiddb_tree")) == 0) ||
           (STncasecmp(tp->name, "iidd_ddb_tree",
                  sizeof("iidd_ddb_tree")) == 0) ||
           (STncasecmp(tp->name, "ii_stored_bitmaps",
                  sizeof("ii_stored_bitmaps")) == 0) ||
           (STncasecmp(tp->name, "ii_encoded_forms",
                  sizeof("ii_encoded_forms")) == 0) ||
           (STncasecmp(tp->name, "iihistogram",
                  sizeof("iihistogram")) == 0) ||
           (STncasecmp(tp->name, "ii_srcobj_encoded",
                  sizeof("ii_srcobj_encoded")) == 0) ||
           (STncasecmp(tp->name, "ii_bitmap_objects",
                  sizeof("ii_bitmap_objects")) == 0) ||
           (STncasecmp(tp->name, "ii_stored_strings",
                  sizeof("ii_stored_strings")) == 0)) 
        {
            block = TRUE;
        }
    }

    return(block);
}
