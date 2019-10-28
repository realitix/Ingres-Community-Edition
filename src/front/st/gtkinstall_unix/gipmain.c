/*
** Copyright 2006, 2011 Actian Corporation. All rights reserved.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

# include <compat.h>
# include <unistd.h>
# include <math.h>
# include <libgen.h>
# if defined( xCL_GTK_EXISTS )

# include <gl.h>
# include <clconfig.h>
# include <ex.h>
# include <me.h>
# include <nm.h>
# include <pc.h>
# include <st.h>
# include <si.h>
# include <erclf.h>

# include <locale.h>
# include <gtk/gtk.h>

# include <gipinterf.h>
# include <gipsup.h>
# include <gipcb.h>
# include <gip.h>
# include <gipdata.h>
# include <giputil.h>
# include <gipsysinfo.h>
# include <gipmsg.h>
# include "gipmain.h"

/*
** Name: gipmain.c
**
** Description:
**
**      Main module for the Linux GUI installer main() program.
**
** History:
**
**	09-Oct-2006 (hanje04)
**	    SIR 116877
**          Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add check to make sure program runs as root.
**	    Use EXIT_STATUS macro to determine outcome of install.
**	    Load Charset and Timezone dialogs during setup instead
**	    of at launch time.
**	    Add init_gu_train_icons to make sure "train" images are
**	    set correctly.
**	26-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add ability to turn on tracing by setting II_GIP_TRACE
**	06-Nov-2006 (hanje04)
**	    SIR 116877
**	    Make sure code only get built on platforms that support it
**	21-Nov-2006 (hanje04)
**	    BUG 117167 
**	    SD 112193
**	    Make sure package check boxes are set correctly when only
**	    one instance is detected. Also make sure instance are loaded
**	    when only "modify" instances have been detected.
**	27-Nov-2006 (hanje04)
**	    Temporarily move inclusion of RPM headers outside of # ifdef
**	     xCL_HAS_RPM to prevent conflict with defn of u_char in
**	    sys/types.h with that in systypes.h. This really needs to be 	**	    addressed is systypes.h but it can't be changed so close to
**	    beta.
**	    The MANIFEST file prevents it from being built where it shouldn't
**	    anyway.
**	28-Nov-2006 (hanje04)
**	    Remove "realloc()" from code as it was causing problems when memory
**	    areas were moved under the covers. Shouldn't have been using it
**	    anyway.
**	09-Jan-2007 (hanje04)
**	    Bug 117445
**	    SD 113106
**	    Update error handler inst_proc_exit_handler() so that the installer
**	    process is not marked as failed if the handler has been invoked 
**	    for reasons other that the process exiting. (e.g. receiving a 
**	    non-fatal signal.)
**	02-Feb-2007 (hanje04)
**	    SIR 116911
**	    Move the following functions here from front!st!gtkinstall_unix_win
**	    giputil.c as they cause compilations problems when building
**	    iipackman on Window.
**
**		get_rename_cmd_len()
**		add_pkglst_to_rmv_command_line()
**		get_pkg_list_size()
**		calc_cmd_len()
**		generate_command_line()
**		add_pkglst_to_inst_command_line()
**
**	    Replace PATH_MAX with MAX_LOC.
**	27-Feb-2007 (hanje04)
**	    BUG 117330
**	    Don't add doc package with others to the command line. It
**	    can't be relocated and needs to be installed separately.
**	13-Mar-2007 (hanje04)
**	    BUG 117902
**	    Add missing version to remove command line for non renamed
**	    packages.
**	15-Mar-2007 (hanje04)
**	    BUG 117926
**	    SD 116393
**	    Use list of packages that are installed when removing old
**	    package info on upgrade. When adding them to the command line
**	    search backwards for '-' when looking for the rename suffix
**	    so we don't get caught out by ca-ingres packages.
**	27-Mar-2007 (hanje04)
**	    BUG 118008
**	    SD 116528
**	    Always display instance selection pane even when there is only
**	    one instance. Otherwise, users are not given the option not to
**	    upgrade their user databases.
**	18-Apr-2007 (hanje04)
**	    Bug 118087
**	    SD 115787
**	    Add progress bar to installation screen to indicate progress.
**	    Only re-write the output every 100 pulses otherwise we get
**	    flickering and far more CPU use than is really neccisary.
**	20-Apr-2007 (hanje04)
**	    Bug 118090
**	    SD 116383
**	    Use newly added database and log locations from instance
**	    structure to remove ALL locations if requested by the user.
**	    Update cleanup routine to use the instance tag to free the
**	    memory as each instance may now have multiple memory chunks
**	    associated with it. 
**      12-dec-2007 (huazh01)
**          Make sure the 'back' button is disabled on the initial screen.
**          This prevents installer crashes with SEGV if user does click
**          the 'back' button on the initial screen. 
**          This fixes bug 119603. 
**      03-jan-2008 (huazh01)
**          don't attempt to install the documentation package if the
**          check box for 'documentation package' is not pressed in, 
**          which means the documentation RPM might not be there.
**          This fixes bug 119687.
**	12-Aug-2008 (hanje04)
**	    BUG 120782, SD 130225
**	    Use "selected_instance" structure to check renamed status instead
**	    of calling is_renamed() again. Was causing problems with UTF8
**	    instances although this has also been addressed. Just a more
**	    efficient check.
**	13-Oct-2008 (hanje04)
**	    BUG 121048
**	    Add remove_temp_files() to remove temporary files and directories
**	    created by installation process.
**	30-Mar-2009 (hanje04)
**	    SD 133435, Bug 119603
**	    Make sure 'back button' is dissabled for BOTH startup screens not
**	    just upgrade mode.
**	06-Aug-2009 (hanje04)
**	    BUG 122571
**	    Add argument checking for file passed in from new Python layer.
**	    Entry point for importing info is now gipImportPkgInfo()
**	    Improve error handling for importing saveset and package info.
**	24-Nov-2009 (frima01)
**	    BUG 122571
**	    Add include of unistd.h, libgen.h, clconfig.h and nm.h plus
**	    static keyword to do_install to eliminate gcc 4.3 warnings.
**	15-Jan-2010 (hanje04)
**	    SIR 123296
**	    Config is now run post install from RC scripts. Update installation
**	    commands appropriately.
**	18-May-2010 (hanje04)
**	    SIR 123791
**	    Update installer for Vectorwise support.
**	     - Add new IVW_INSTALL "mode" and VW specific initialization.
**	     - Add init_ivw_cfg() to handle most of this
**	     - Add get_sys_mem() funtion to get the system memory for
**	       determining memory dependent defaults
**	25-May-2010 (hanje04)
**	    SIR 123791
**	    Vectorwise installs should default to VW instead of II
**	26-May-2010 (hanje04)
**	    SIR 123791
**	    Set pkgs_to_install=TYPICAL_VECTORWISE when initializing for
**	    IVW_INSTALL
**	01-Jun-2010 (hanje04)
**	    Mantis 741
**	    Set prodname for VW installs to use in summary
**	17-Jun-2010 (hanje04)
**	    BUG 123942
**	    Ensure temporary rename dir is always created when rename is
**	    triggered. Was previously only being created for upgrade mode.
**	24-Jun-2010 (hanje04)
**	    BUG 124020
**	    Use instance ID of the install we're modifying when generating
**	    stop/star/config command line, not the default instID.
**	24-Jun-2010 (hanje04)
**	    BUG 124019
**	    Need to add INGCONFIG_CMD to command line length to stop buffer
**	    overrun when generating command line.
**	    Also add gipAppendCmdLine() to manage allocated buffer space and
**	    use it in place of STcat().
**	13-Jul-2010 (hanje04)
**	    BUG 124081
**	    For installs and modifies, save the response file off to 
**	    /var/lib/ingres/XX so that we can find it for subsequent
**	    installations and not inadvertantly overwrite config
**	01-Jul-2010 (hanje04)
**	    BUG 124256
**	    Stop miss reporting errors whilst cleaning up temporary
**	    files. Also don't give up first error, process the whole list.
**	01-Jul-2010 (hanje04)
**	    BUG 123942
**	    If only renamed instance exist, UM_RENAME won't be set so
**	    test for UM_TRUE also when creating temp rename location.
**	04-Oct-2010 (hanje04)
**	    BUG 124537
**	    Improve error reporting in install_control_thread().
**	    If an error occurs generating a response file, generating the 
**	    command line or spawning the installation process. Popup an
**	    error box to the user and abort the installation.
**	    Consolidate out/err display variables too.
**	22-Nov-2010 (hanje04)
**	    Add new Max Parallelism parameter in IVW 1.5
**	    and remove Max DB size (no longer valid).
**	    Add GIPsetUgAction() to display correct train and text for various
**	    upgrade/modify modes.
**	    Also generally clean up GUI and improve usability
**	    - Reduce packages to DBMS, NET and core.
**	    - Remove Tx log config page
**	    - Move II_VWDATA to DB location page
**	    - Remove package selection and summary from basic install
**	09-Dec-2010 (hanje04)
**	    Maintis 1263
**	    Dialogs aren't correctly being initialized when one or more
**	    instance of the same version are found. For IVW we should
**	    default to new install for this case as Modify is no longer
**	    valid and defaulting to Remove is wrong.
**	    Also fix log display, make sure correct location is used.
**	14-Dec-2010 (hanje04)
**	    For IVW use II_CONFIG_TYPE=IVW as default.
**	    Add get & set_sys_shmmax to check and update kernel.shmmax if
** 	    if needed. Really needs to be a check based on amount required,
**	    hard coded value will have to do for now as there's no easy
**	    way to check between installation and start up.
**	26-Jan-2011 (hanje04)
**	    Mantis 1350
**	    Define GIP_INT_DIV_MACRO to use decimal rounding when
**	    dividing up memory an converting between Kb->Mb->Gb.
**	    Also raise the point at which we convert to Gb to 4Gb to
**	    keep higher granularity for smaller ammounts of memory.
**	01-Feb-2011 (hanje04)
**	    Move all errors and other user messages to front!st!hdr gipmsg.h
**	    Add native language support by way of gettext and locale libraries.
**	10-Feb-2011 (hanje04)
**	    Mantis 1303, 1348 & 1434
**	    Add sanity checking and upper limits to VW config parameters
**	    in GUI. Move all functions relating to /proc values to new
**	    gipsysinfo.c module so values can be better managed:
**	
**		get_sys_mem()
**		get_sys_shmmax()
**		get_num_cpu()
**		set_sys_shmmax()
**
**	    Replaced with calls to new gipGetSysxx() in new modules.
**	16-Feb-2011 (hanje04)
**	    Mantis 1514
**	    Correct call to check kernel.shmmax and re-get values
**	    just before install.
**	22-Feb-2011 (hanje04)
**	    Mantis 1448
**	    Hook up "install documentation" logic for vectorwise installs.
**	25-Mar-2011 (hanje04)
**	    Mantis 1806
**	    Don't blindly overwrite the installation ID for IVW. It might have
**	    been tweaked by gipImportPkgInfo()
**	18-May-2011 (hanje04)
**	    Add support for DEBs packaging:
**		- Limit to 1 install (no rename)
**		- II_SYSTEM fixed at /opt/Ingres/PRODNAME
**		- Remove APT source list once we're done
**	    Move installation related functions to separate (new) files as 
**	    this one has become a huge catchall.
**	    gipinstutil():
**		- install_control_thread()
**		- do_install()
**		- generate_command_line()
**		- calc_cmd_len()
**		- gipAppendCmdLine()
**	    giprpminst():
**		- get_pkg_list_size()
**		- add_pkglst_to_*_command_line() (now add_rpmlst...())
**		- get_rename_cmd_len() (now get_rpm_rename...())
**	    Replace STprintf with snprintf in an attempt to reduce the risk
**	    of buffer overrun where appropriate
**	25-May-2011 (hanje04)
**	    Make sure instID is set for new installs for DEBs builds
**	01-Jul-2011 (hanje04)
**	    Mantis 2020
**	    Grey out II_SYSTEM entry box for DEBs, was un-greyed by
**	    last change.
**	27-Jul-2011 (hanje04)
**	    Handle GUI differences between Ingres and VectorWise after
**	    merge from codev.
**	01-Sep-2011 (hanje04)
**	    Add set_prod_name() to set product name dependent strings in 
**	    GUI on start up.
**	    
*/

# define RF_VARIABLE "export II_RESPONSE_FILE"
# define GIP_INT_DIV_MACRO(intnum, intdiv) round((double)intnum/intdiv)

/* local prototypes */
static STATUS remove_temp_files( void );
static STATUS create_temp_rename_loc( void );
static void inst_proc_exit_handler( int sig );
static void init_ug_train_icons( UGMODE state );
static STATUS init_ivw_cfg();
static STATUS set_prod_name();

/* static variables */
static size_t	cmdlen = 0;

/* GUI object names */
static const char *ivwdata_objs[] = {
	"ivwdata_image1",
	"ivwdata_box",
	"ii_vwdata",
	"BrowseIVWButton",
	"ivwdata_browse_box",
	"ivwdata_image2",
	NULL
	};

/* entry point to gipxml.c */
STATUS
gipImportPkgInfo( LOCATION *xmlfileloc, UGMODE *state, i4 *count );

int
main (int argc, char *argv[])
{
    char	exedir[MAX_LOC];
    char	pixmapdir[MAX_LOC];
    char	langdir[MAX_LOC];
    char	pkginfofile[MAX_LOC];
    char	tmpbuf[MAX_LOC];
    char	licfile[MAX_LOC];
    char	*known_instance_loc=NULL;
    char	*binptr=NULL;
    char	*tracevar;
    LOCATION	pkginfoloc;
    LOCATION	licfileloc;
    GtkListStore prog_list_store;
    GtkTextBuffer *lic_text_buff;
    GtkWidget	*widget_ptr; 
    i4		i=0;
    STATUS	rc;
	
    /* init threads */	
    g_thread_init(NULL);
    gdk_threads_init();

    /* initialize GTK */
    gtk_set_locale ();
    gtk_init (&argc, &argv);

    /* turn on tracing? */
    NMgtAt( "II_GIP_TRACE", &tracevar );
    if ( tracevar && *tracevar )
        debug_trace = 1 ;

    /* installer the signal handler for SIGCHLD */
    EXsetsig( SIGCHLD, inst_proc_exit_handler );

    /* set defaults */
    userid = (char *)defaultuser;
    groupid = (char *)defaultuser;

    /*
    ** Save set should have the following format:
    **
    **	rootdir
    **	   |
    **	   |-- bin (exedir)
    **	   |
    **	   |-- locale
    **	   |
    **	   |-- pixmaps
    **	   |
    **	   |-- rpm
    **	   |
    **	   ~
    **	   |
    **	ingres_install
    */

    /*
    ** To find the root of the install tree relative to the executable,
    ** copy the location locally so we can manipulate the string.
    **
    */
    STcopy( argv[0], tmpbuf );
    dirname( tmpbuf );

    /* make sure we have an absolute path */
    if ( *argv[0] != '/' )
    {
	STcat( getcwd(exedir, MAX_LOC), "/" );
	STcat( exedir, tmpbuf );
    }
    else
	strcpy( exedir, tmpbuf );

    /* Strip off 'bin' directory */
    binptr = STrstrindex( exedir, "bin", 0, FALSE );
    if ( binptr != NULL )
    {
	/* truncate exedir at /bin */
	binptr--;
	*binptr = '\0';
    }
		
    DBG_PRINT( "exedir = %s\n", exedir );
    if ( strlen(exedir) + strlen("/pixmaps") > MAX_LOC )
    {
	SIprintf(ERROR_PATH_TOO_LONG);
	exit(2);
    }
    /* store root directory */
    STcopy( exedir, new_pkgs_info.file_loc );

    snprintf( pixmapdir, MAX_LOC, "%s%s", exedir, "/pixmaps" );
    add_pixmap_directory( pixmapdir );

    /* international language support */
    snprintf( langdir, MAX_LOC, "%s%s", exedir, "/locale" );

    /*
    ** Set the current local to default and bind "inginstgui" domain
    ** to saveset directory
    */
    setlocale(LC_ALL, "");
    bindtextdomain("inginstgui", langdir);
    textdomain("inginstgui");
	
    /* check we're running as root */
    if ( getuid() != (uid_t)0 )
    {
            SIprintf( ERROR_NOT_ROOT );
            SIflush( stdout );
            return( FAIL );
    }

    /*
    ** Get info about the packages contained in the saveset and
    ** and the ones already installed. Location of file should
    ** have been passed in as an argument
    **
    ** (previous call)
    ** rc = query_rpm_package_info( argc, argv, &inst_state, &inst_count );
    */
    if ( argc > 1 )
    {
	if ( STlen(argv[1]) < MAX_LOC)
	{
	    STcopy(argv[1], pkginfofile);
	    LOfroms(PATH|FILENAME, pkginfofile, &pkginfoloc);
	    if (LOexist(&pkginfoloc) == OK)
    		rc = gipImportPkgInfo(&pkginfoloc, &inst_state, &inst_count );
	    else
		SIprintf(ERROR_PKGINFO_NOT_FOUND, pkginfofile);
	}
	else
	    SIprintf(ERROR_PATH_TOO_LONG);
    }
    else
	SIprintf(ERROR_MISSING_ARGUMENT);
	 
    if ( rc != OK )
    {
	SIprintf( ERROR_PKG_QUERY_FAIL );
	return( rc );
    }

    SET_II_SYSTEM( default_ii_system );

    /* create main install window, hookup the quit prompt box and show it */
    IngresInstall=create_IngresInstall();
    g_signal_connect (G_OBJECT (IngresInstall), "delete_event",
                      G_CALLBACK (delete_event), NULL);
	
    /* initialize system info */
    if ( gipInitSysInfo(FALSE) != OK )
	printf( WARN_SYSINFO_INIT_FAIL );

    /* initialize New Installation mode */
    if ( init_new_install_mode() != OK )
    {
	  printf( ERROR_INSTALL_INIT_FAIL );
	  return( FAIL );
    }
  
    /* load license file */
    snprintf( licfile, MAX_LOC, "%s/%s", exedir, "LICENSE" );
    LOfroms(PATH|FILENAME, licfile, &licfileloc);
    if (LOexist(&licfileloc) == OK)
    {
	widget_ptr=lookup_widget(IngresInstall, "inst_lic_view");
	lic_text_buff=gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget_ptr));
	write_file_to_text_buffer(licfile, lic_text_buff);
    }
    else
	SIprintf(ERROR_LICENSE_NOT_FOUND, licfile);

    if ( inst_state & (UM_RENAME|UM_TRUE) )
    {
	/* 
	** We have a non-renamed instance so we'll need to
	** rename the RPMs. Create a temporary location for
	** doing this
	*/
	if ( create_temp_rename_loc() != OK )
 	{
	    printf( ERROR_CREATE_TEMP_LOC );
	    return( FAIL );
	}
    }
    
    /* If we found other installations then we're in upgrade mode */
    if ( inst_state & UM_TRUE )
    {
  	if ( init_upgrade_modify_mode() != OK )
 	{
	    printf( ERROR_UPGRADE_INIT_FAIL );
	    return( FAIL );
	}

	/* load upgrade license file */
	widget_ptr=lookup_widget(IngresInstall, "upg_lic_view");
	lic_text_buff=gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget_ptr));
	write_file_to_text_buffer(licfile, lic_text_buff);
    }

    /* set the correct starting screen */
    set_screen(START_SCREEN);

    /* show the wizard */
    gtk_widget_show(IngresInstall);
    
    gdk_threads_enter();
    gtk_main ();
    gdk_threads_leave();
  
    /* clean up before exit */
    if ( remove_temp_files() != OK )
    {
	printf( ERROR_REMOVE_TEMP_LOC );
    }

    while( existing_instances[i] != NULL )
    {
	MEfreetag( existing_instances[i]->memtag );
	existing_instances[i] = NULL;
	i++;
    }

    return( EXIT_STATUS );
}

gint
init_new_install_mode(void)
{
    GtkWidget	*widget_ptr;
    GtkWidget	*locale_box;
    GtkWidget	*db_config_type;
    GtkWidget	*ing_misc_ops;
    GtkWidget	*ivw_misc_ops;
    GtkWidget	*ivw_cfg_box;
    GtkWidget	*log_loc_box;

    char	*IDptr;
    gchar	textbuf[10];
    i4 		i = 0;


    /* set mode */
    runmode = MA_INSTALL;
    instmode = BASIC_INSTALL;

    /* check which product we're installing */
    if ( STcompare( "vectorwise", new_pkgs_info.product) == OK )
    {	
	instmode |= IVW_INSTALL;
	stage_names = basic_ivw_stages;
	pkgs_to_install=VECTORWISE_SERVER;
	dfltID[0]='V';
	if ( dfltID[1] == 'I' )
	    dfltID[1]='W';
	prodname=IVWPRODNAME;
    }

    /* set product name dependent strings */
    while ( prod_label_list[i].labelname )
    {
	gchar	strbuf[1024];
	widget_ptr=lookup_widget(IngresInstall, prod_label_list[i].labelname);
	snprintf( strbuf, 1023, prod_label_list[i].string, prodname );
	gtk_label_set_markup( GTK_LABEL(widget_ptr), strbuf );
	i++;
    }
 
    /* check installation ID */
    STprintf( textbuf, "Ingres %s", dfltID );
    if ( new_pkgs_info.pkgfmt != PKGDEB )
    {
	i = 1;
	while ( OK == inst_name_in_use( textbuf ) ) 
	{
	    char IDbuf[2];

	    STprintf( IDbuf, "%d", i++ );
	    dfltID[1] = IDbuf[0];
	    STprintf( textbuf, "Ingres %s", dfltID );
	} 

	IDptr = STrstrindex( default_ii_system, RFAPI_DEFAULT_INST_ID, 0, FALSE );
	IDptr[0] = instID[0] = dfltID[0];
	IDptr[1] = instID[1] = dfltID[1];
    }
    else
    {
	/* no instid in II_SYSTEM for DEBS */
	instID[0] = dfltID[0];
	instID[1] = dfltID[1];
    }

    /* and set the widgets */
    widget_ptr = lookup_widget( IngresInstall, "instname_label" );
    gtk_label_set_text( GTK_LABEL( widget_ptr ), textbuf );

    /* set ii_system, fixed for debs */
    widget_ptr = lookup_widget( IngresInstall, "ii_system_entry" );
    gtk_entry_set_text( GTK_ENTRY( widget_ptr), default_ii_system );

    if ( new_pkgs_info.pkgfmt == PKGDEB )
    {
	widget_ptr = lookup_widget( IngresInstall, "ii_system_table" );
	gtk_widget_set_sensitive( widget_ptr, FALSE);
    }

    if ( ! ( new_pkgs_info.pkgfmt == PKGDEB && inst_state&UM_TRUE ) )
    {
	widget_ptr = lookup_widget(IngresInstall, "instID_entry2");
	gtk_entry_set_text( GTK_ENTRY( widget_ptr ), dfltID );
    }

    
    /* Create dialogs to act as combo boxes */
    /* timezones */
    TimezoneTreeModel = initialize_timezones();
    if ( instmode & IVW_INSTALL )
	widget_ptr = lookup_widget( IngresInstall, "IVWTimezoneEntry" );
    else
	widget_ptr = lookup_widget( IngresInstall, "TimezoneEntry" );

    TimezoneDialog = create_text_view_dialog_from_model( TimezoneTreeModel,
							widget_ptr,
							timezone_iter );

    /* charsets */
    if ( ~instmode & IVW_INSTALL ) /* charset always UTF8 for IVW */
    {
	CharsetTreeModel = initialize_charsets( linux_charsets );
	widget_ptr = lookup_widget( IngresInstall, "CharsetEntry" );

	CharsetDialog = create_text_view_dialog_from_model( CharsetTreeModel,
							widget_ptr,
							charset_iter );
    }

    /* hide/show required widgets */
    widget_ptr=lookup_widget(IngresInstall, "bkp_log_file_loc");
    gtk_widget_set_sensitive(widget_ptr, FALSE);
    widget_ptr=lookup_widget(IngresInstall, "bkp_log_browse");
    gtk_widget_set_sensitive(widget_ptr, FALSE);
    widget_ptr=lookup_widget(IngresInstall, "bkp_log_label");
    gtk_widget_set_sensitive(widget_ptr, FALSE);	
    widget_ptr=lookup_widget(IngresInstall, "RfPlatformBox" );
    gtk_widget_hide(widget_ptr);
    widget_ptr=lookup_widget(IngresInstall, "RfEndTable" );
    gtk_widget_hide(widget_ptr);

    /* b119603, make sure back button is disabled on both screens. */ 
    widget_ptr=lookup_widget(IngresInstall, "ug_back_button" );
    gtk_widget_set_sensitive(widget_ptr, FALSE);
    widget_ptr=lookup_widget(IngresInstall, "back_button" );
    gtk_widget_set_sensitive(widget_ptr, FALSE);

    
    /* set default package lists */
    set_inst_pkg_check_buttons();
    
    /* create formatting tags for summary & finish screens */
    widget_ptr=lookup_widget(IngresInstall, "SummaryTextView");
    create_summary_tags( gtk_text_view_get_buffer(
				GTK_TEXT_VIEW( widget_ptr ) ) );
    widget_ptr=lookup_widget(IngresInstall, "FinishTextView");
    create_summary_tags( gtk_text_view_get_buffer( 
			 	GTK_TEXT_VIEW( widget_ptr ) ) );
							
    /* initialize product specific config */
    locale_box=lookup_widget(IngresInstall, "tr_locale_box");
    db_config_type=lookup_widget(IngresInstall, "tr_config_type_box");
    ing_misc_ops=lookup_widget(IngresInstall, "ing_misc_ops_frame");
    ivw_misc_ops=lookup_widget(IngresInstall, "ivw_misc_ops_frame");
    ivw_cfg_box=lookup_widget(IngresInstall, "tr_ivw_cfg_box");
    log_loc_box=lookup_widget(IngresInstall, "tr_log_loc_box");

    if ( instmode & IVW_INSTALL )
    {
	/* hide/show vectorwise related config in train */
	gtk_widget_hide(locale_box);
	gtk_widget_hide(db_config_type);
	gtk_widget_hide(ing_misc_ops);
	gtk_widget_hide(log_loc_box);
	gtk_widget_show(ivw_misc_ops);
	gtk_widget_show(ivw_cfg_box);

 	/* VW data location */
	i=0;
	while(ivwdata_objs[i])
	{
	    widget_ptr=lookup_widget(IngresInstall, ivwdata_objs[i]);
	    gtk_widget_show(widget_ptr);
	    i++;
        }
        
	init_ivw_cfg();

	ing_config_type.val_idx = IVW;
    }
    else
    {
	gtk_widget_show(locale_box);
	gtk_widget_show(db_config_type);
	gtk_widget_show(ing_misc_ops);
	gtk_widget_show(log_loc_box);
	gtk_widget_hide(ivw_misc_ops);
	gtk_widget_hide(ivw_cfg_box);

	/* Set default config type */
	widget_ptr=lookup_widget(IngresInstall, "ing_cfg_type_radio_tx");
	gtk_button_clicked(GTK_BUTTON(widget_ptr));

 	/* VW data location */
	i=0;
	while(ivwdata_objs[i])
	{
	    widget_ptr=lookup_widget(IngresInstall, ivwdata_objs[i]);
	    gtk_widget_hide(widget_ptr);
	    i++;
        }
        
    }

    return(OK);
}

gint
init_upgrade_modify_mode(void)
{
    GtkWidget	*instance_swindow;
    GtkWidget	*new_inst_table;
    GtkWidget	*ug_install_table;
    GtkWidget	*inst_sel_table;
    GtkWidget	*ug_mod_table;
    GtkWidget	*widget_ptr;
    GtkTreeModel *instance_list;
    GtkTreeSelection *ug_select;
    GtkTreeIter	first;

 
    /* set the run mode */
    runmode=MA_UPGRADE;
    ug_mode|=UM_TRUE;

    new_inst_table=lookup_widget(IngresInstall, "new_inst_table");
    ug_install_table=lookup_widget(IngresInstall, "ug_install_table");
    ug_mod_table=lookup_widget(IngresInstall, "ug_mod_table");

    /* 
    ** If an older installation is detect then upgrade is our default mode
    ** we only set UM_MOD initially if only instances of the same version 
    ** have been detected
    **
    ** Then initialize train apropriately
    */
    if ( inst_state & UM_UPG )
    {
	ug_mode|=UM_UPG;
	gtk_widget_hide(new_inst_table);
	gtk_widget_hide(ug_install_table);
	gtk_widget_hide(ug_mod_table);
    }
    else
    {
	ug_mode|=UM_MOD;		
	gtk_widget_hide(new_inst_table);
	gtk_widget_show(ug_install_table);
	gtk_widget_show(ug_mod_table);
    }

    /* setup multiple installation stuff if needed */
    int 	i=0;
    
    /* initialized container for displaying instances */
    instance_list=initialize_instance_list();

    
    /* attach instance list to window */
    InstanceTreeView = gtk_tree_view_new_with_model( instance_list );
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW ( InstanceTreeView ), TRUE);
    gtk_tree_view_set_search_column (GTK_TREE_VIEW ( InstanceTreeView ),
					    COL_NAME);
    g_object_unref (instance_list);
    /* pack it into the scrollwindow */
    instance_swindow=lookup_widget( IngresInstall, "instance_swindow" );
    gtk_container_add( GTK_CONTAINER( instance_swindow ),
				InstanceTreeView );
    
    /* add the appropriate columns */
    add_instance_columns( GTK_TREE_VIEW( InstanceTreeView ) );
    
    gtk_list_store_clear( GTK_LIST_STORE( instance_list ) );
    while ( i < inst_count )
    {
	/* and load that into the list */
	    if ( existing_instances[i]->action & ug_mode )
		add_instance_to_store( GTK_LIST_STORE(instance_list),
					existing_instances[i] );
    
 	    i++;
	    DBG_PRINT("Added instance %d of %d to %s store\n",
					i,
					inst_count,
					ug_mode & UM_MOD ?
					"modify" : "upgrade" );
    }

    /* Create and connect selection handler */
    ug_select = gtk_tree_view_get_selection (
				GTK_TREE_VIEW( InstanceTreeView ));
    gtk_tree_selection_set_mode (ug_select, GTK_SELECTION_SINGLE);
    g_signal_connect (G_OBJECT (ug_select),
			"changed",
			G_CALLBACK (instance_selection_changed),
			NULL); 

    /* select the first item */
    gtk_tree_model_get_iter_first( instance_list, &first );
    gtk_tree_selection_select_iter( ug_select, &first );

    gtk_widget_show( InstanceTreeView );

    /* set installed packages check boxes for the selected instance */
    set_mod_pkg_check_buttons( selected_instance->installed_pkgs );

    /* display the right icons on the train */
    init_ug_train_icons( inst_state );

    /* create formatting tags for summary & finish screens */
    widget_ptr=lookup_widget(IngresInstall, "UgSummaryTextView");
    create_summary_tags( gtk_text_view_get_buffer( 
      GTK_TEXT_VIEW( widget_ptr ) ) );
    widget_ptr=lookup_widget(IngresInstall, "UgFinishTextView");
    create_summary_tags( gtk_text_view_get_buffer( 
      GTK_TEXT_VIEW( widget_ptr ) ) );

    if ( new_pkgs_info.pkgfmt == PKGDEB )
    {
	/* Limited to one install for DEB so grey out new install */
	GtkWidget    wptr;

	widget_ptr=lookup_widget( IngresInstall, "ug_ssame_install" );
        gtk_widget_set_sensitive( widget_ptr, FALSE );
	widget_ptr=lookup_widget( IngresInstall, "ug_sold_install" );
        gtk_widget_set_sensitive( widget_ptr, FALSE );
    }
    else if ( instmode & IVW_INSTALL && ug_mode & UM_MOD )
    {
	/* Default to new install if it's a VW RPM "modify" */
	GtkWidget    wptr;

	widget_ptr=lookup_widget( IngresInstall, "ug_ssame_install" );
        gtk_button_clicked( GTK_BUTTON( widget_ptr ) );
    }

   return( OK );
}

static STATUS
create_temp_rename_loc( void )
{
    LOCATION            tmploc; /* temporary LOCATION  storage */
    char		tmpbuf[MAX_LOC]; /* temporary path storage */
    char                *stptr; /* misc string pointer */
    PID                 pid; /* process ID of current process */
    STATUS              clstat; /* cl return codes */

    /* find a temporary dir */
    NMgtAt( TMPVAR, &stptr );
    if ( stptr == NULL || *stptr == '\0' )
    {
        stptr = TMPDIR ;
    }
    STcopy( stptr, rndirstr );
    clstat = LOfroms( PATH, rndirstr, &rndirloc );

    if ( clstat != OK )
	return( clstat );

    /* use the PID to create genreate a rename dir name */
    PCpid( &pid );
    snprintf( tmpbuf, MAX_LOC, "%s.%d", RPM_RENAME_DIR, pid );
    clstat = LOfroms( PATH, tmpbuf, &tmploc );

    if ( clstat != OK )
	return( clstat );

    /* now combine them */
    clstat = LOaddpath( &rndirloc, &tmploc, &rndirloc );	

    if ( clstat != OK )
	return( clstat );

    /* finally create the directory */
    clstat = LOcreate( &rndirloc );
    return( clstat );

}

static STATUS
remove_temp_files( void )
{
    char		*stdiofiles[] = { inst_stdout, inst_stderr, NULL }; 
    i2			i = 0;
    LOCATION		tmpfileloc; 
    STATUS              clstat; /* cl return codes */

    DBG_PRINT("Cleaning up temp files...\n");

    /* Clean up stdin/out files used during installation */
    while( i < 2 )
    {
	if ( *stdiofiles[i] )
	{
	    LOfroms( PATH & FILENAME, stdiofiles[i], &tmpfileloc );
	    if ( LOexist( &tmpfileloc ) == OK )
        	clstat = LOdelete( &tmpfileloc );

	    if ( clstat == OK )
		DBG_PRINT("\tremoving %s\n", stdiofiles[i]);
	    else
		DBG_PRINT("Error occurred removing %s\n", stdiofiles[i]);
	}
	i++;
    }
	
    /* if we have a rename location and it's valid, remove it */
    if ( rndirstr && ( clstat = LOexist( &rndirloc ) == OK ) )
    {
        clstat = LOdelete( &rndirloc );
	if ( clstat == OK )
	    DBG_PRINT("\tremoving %s\n", rndirstr );
	else
	    DBG_PRINT("Error occurred removing %s\n", rndirstr );
    }

    if ( new_pkgs_info.pkgfmt == PKGDEB )
    {
	char debsrclst[MAX_LOC];

        snprintf( debsrclst, MAX_LOC,
		    "/etc/apt/sources.list.d/%s_local.list",
		    new_pkgs_info.product );

	LOfroms( PATH & FILENAME, debsrclst, &tmpfileloc );
	if ( LOexist( &tmpfileloc ) == OK )
	{
	    clstat = LOdelete( &tmpfileloc );

	    if ( clstat == OK )
		DBG_PRINT("\tremoving %s\n", debsrclst);
	    else
		DBG_PRINT("Error occurred removing %s\n", debsrclst);
	}
    }
    return( clstat );
}

GtkTreeModel *
initialize_instance_list()
{
     GtkTreeIter	row_iter;
     GtkListStore *instance_store;
     gint i;
	
     /* create a list store to hold the instance info */
     instance_store=gtk_list_store_new(NUM_COLS,
					G_TYPE_STRING,
					G_TYPE_STRING,
					G_TYPE_STRING);	
     /* Return the pointer to the list */
     return GTK_TREE_MODEL( instance_store );
}

static void
inst_proc_exit_handler( int sig )
{
    PID	cpid; /* child pid */
    int status, exitstatus;

    if ( sig == SIGCHLD )
    {
	/* clear SIGCHLD by calling PCreap(). */
	cpid = PCreap();
	if ( cpid > 0 )
	{
	    childinfo.exit_status = PCwait( cpid );
	    switch( childinfo.exit_status )
	    {
		case E_CL1623_PC_WT_INTR:
		case E_CL1628_PC_WT_EXEC:
		    childinfo.alive = TRUE;
		    break;
		default:
		    childinfo.alive = FALSE;
	    }
	}
	else
	    childinfo.exit_status = FAIL;

	if ( childinfo.alive == TRUE )
	    DBG_PRINT( "Child pid %d, received not fatal signal\n",
				cpid );
	else
	    DBG_PRINT( "Child pid %d, reaped with exit code %d\n",
				cpid, childinfo.exit_status  );
	   
    }
    else
	/* something went very wrong */
	PCexit( FAIL );

    /* replant the handler */
    EXsetsig( sig, inst_proc_exit_handler );
}

static void
init_ug_train_icons( UGMODE state )
{
    GtkWidget	*upgrade_inst_mod;
    GtkWidget	*upgrade_inst_ug;
    GtkWidget	*upgrade_inst_multi;

    /* look up icons we need to affect */
    upgrade_inst_mod = lookup_widget( IngresInstall, "upgrade_inst_mod" );
    upgrade_inst_ug = lookup_widget( IngresInstall, "upgrade_inst_ug" );
    upgrade_inst_multi = lookup_widget( IngresInstall, "upgrade_inst_multi" );

    /* hide all and show based on state */
    gtk_widget_hide( upgrade_inst_mod );
    gtk_widget_hide( upgrade_inst_ug );
    gtk_widget_hide( upgrade_inst_multi );

    if ( state & UM_MOD && state & UM_UPG )
	gtk_widget_show( upgrade_inst_multi );
    else if ( state & UM_MOD )
	gtk_widget_show( upgrade_inst_mod );
    else
	gtk_widget_show( upgrade_inst_ug );
}

static STATUS
init_ivw_cfg()
{
    GtkWidget *widget_ptr;
    i4	i = 0;
    char field_buf[100];
    SIZE_TYPE	sysmemkb;
    i4 cpus;

    DBG_PRINT("Checking shmmax\n");
    gipGetSysShmmax(&sysmemkb);

    DBG_PRINT("Checking system memory\n");
    if (gipGetSysPMem(&sysmemkb) != OK)
	sysmemkb=0;

    DBG_PRINT("Getting CPU info\n");
    if (gipGetSysCpu(&cpus) != OK)
	cpus=1;

    /* initialize vectorwise config */
    while (vw_cfg_info[i])
    {
	vw_cfg *param=vw_cfg_info[i];

	/*
	**  if we can, use the amount of system memory to set default for
	** memory parameters
        */
	if ( sysmemkb > 0 && param->bit &
		 (GIP_VWCFG_MAX_MEMORY|GIP_VWCFG_BUFFERPOOL) )
	{
	    if (param->bit & GIP_VWCFG_MAX_MEMORY)
	    {
		/* default to 50% of system memory */
		param->dfval = GIP_INT_DIV_MACRO(sysmemkb, (1024 * 2));
		param->dfunit = VWMB;
	    }
	    if (param->bit & GIP_VWCFG_BUFFERPOOL)
	    {
		/* default to 25% of system memory */
		param->dfval = GIP_INT_DIV_MACRO(sysmemkb, (1024 * 4));
		param->dfunit = VWMB;
	    }

	    /*
	    ** Find a sensible unit, only use Gb if we're above 4Gb
	    ** otherwise we could get bitten by rouding
	    */
	    if (param->dfval > 4096)
	    {
		param->dfval = GIP_INT_DIV_MACRO(param->dfval, 1024);
	 	param->dfunit = VWGB;
	    }
	    param->value=param->dfval;
	    param->unit=param->dfunit;
	   
	}
		
	if ( param->bit & GIP_VWCFG_MAX_PARALLELISM )
	    /* limit default to 8 */
	    param->dfval = param->value = (cpus > 8) ? 8 : cpus; 

	/* set field value */
	STprintf(field_buf, "%s_val", param->field_prefix);
	widget_ptr=lookup_widget(IngresInstall, field_buf);
	DBG_PRINT("Setting %s to %ld\n", field_buf, param->value);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget_ptr), param->value);

	/* set units */
	if (param->unit != VWNOUNIT)
	{
	    STprintf(field_buf, "%s_unit", param->field_prefix);
	    DBG_PRINT("Setting default unit for %s\n", field_buf);
	    widget_ptr=lookup_widget(IngresInstall, field_buf);
	    gtk_combo_box_set_active(GTK_COMBO_BOX(widget_ptr), param->unit);
	}
	i++;
    }
}

# else /* xCL_GTK_EXISTS */

/*
** Dummy main() to quiet link errors on platforms which don't support 
** GTK & RPM
*/
i4
main()
{
    return( FAIL );
}

# endif /* xCL_GTK_EXISTS */
