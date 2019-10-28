/*
** Copyright 2011 Actian Corporation. All rights reserved.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

# include <compat.h>
# include <unistd.h>
# if defined( xCL_GTK_EXISTS )

# include <gl.h>
# include <clconfig.h>
# include <pc.h>
# include <st.h>

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
** Name: gipinst.c
**
** Description:
**
**      Generic installation module for the Linux GUI installer.
**	Contains package independent functions for performing the installation
**
** History:
**
**	16-May-2011 (hanje04)
**          Created from gipmain.c
**	02-Oct-2011 (hanje04)
**	    Bug 125688
**	    Add config and startup for remote manager service (iimgmtsvc)
**	    Add popup warning suitable JRE for manager service isn't present
*/

# define RF_VARIABLE "export II_RESPONSE_FILE"

STATUS gipAppendCmdLine( char *cmdbuff, char *cmd );
/* local prototypes */
static STATUS do_install( char *inst_cmdline );
size_t get_rename_cmd_len( PKGLST pkglst );
void add_rpmlst_to_rmv_command_line( char **cmd_line, PKGLST pkglist );
int calc_cmd_len(size_t *cmdlen);
int generate_command_line( char **cmd_line );
void add_rpmlst_to_inst_command_line( char **cmd_line,
						PKGLST pkglist,
						bool renamed );
void add_deblst_to_command_line( char **cmd_line, PKGLST pkglist );
gboolean gipReadInstBuffer( GIOChannel *source, GIOCondition cond, gpointer data);

/* static variables */
static size_t	cmdlen = 0;

void *
install_control_thread( void *arg )
{
    GtkWidget		*OutTextView;
    GtkWidget		*ErrTextView;
    GtkWidget		*InstProgBar;
    GtkTextBuffer	*inst_out_buffer;
    GtkTextBuffer	*inst_err_buffer;
    GIOChannel		*outchannel;
    GIOChannel		*errchannel;
    II_RFAPI_STATUS	rfstat;
    PID			ppid;
    char	*inst_cmdline;
    int	i = 1; /* loop counter */
    SIZE_TYPE 		sys_shmmax = 0;

    /* initialize log file names */
    PCpid( &ppid );
    snprintf( inst_stdout, MAX_LOC, "%s.%d", STDOUT_LOGFILE_NAME, ppid );
    snprintf( inst_stderr, MAX_LOC, "%s.%d", STDERR_LOGFILE_NAME, ppid );

    /* generate response file */
    rfstat = generate_response_file();
	
    if ( rfstat != II_RF_ST_OK )
        DBG_PRINT( "ERROR: Failed to create response file with error %s\n",
			IIrfapi_errString(rfstat) );

    /* generate installation command line */
    if ( generate_command_line( &inst_cmdline ) != OK )
	DBG_PRINT( "ERROR: Failed to generate command line\n" );

    DBG_PRINT( "Command line is %s\n", inst_cmdline );

    /* Re-check system parameters */
    gipInitSysInfo(TRUE);

    /*
    ** Check shmmax for installing/upgradeing IVW, default config
    ** requires increased SHMMAX
    **
    ** FIX ME!!
    ** This is not the best way to do this as we can currently only
    ** support checking a fixed value.
    ** Using syscheck to determin what we need would be much better
    ** but the current way of installing prevents us from doing
    ** this.
    */
# define IVW_SHMMAX_NEED_MB 200
    if ( ~ug_mode & UM_RMV &&
	 ( gipGetSysShmmax(&sys_shmmax) != OK ||
		 sys_shmmax < IVW_SHMMAX_NEED_MB * 1024 * 1024 ))
    {
	char	shmmax_msg[256];

	STprintf(shmmax_msg, MESSAGE_SHMMAX_TOO_LOW, sys_shmmax/(1024 * 1024),
			prodname, IVW_SHMMAX_NEED_MB);
	gdk_threads_enter();
	popup_info_box( shmmax_msg );
	gdk_threads_leave();
 	gipSetSysShmmax(IVW_SHMMAX_NEED_MB * 1024 * 1024);
    }

# ifdef conf_BUILD_MGMTSVC
    {
	/*
	** Check we have a valid JRE present, warn if we don't
	*/
	LOCATION javaloc;

	LOfroms( PATH, javainfo.home, &javaloc);
        if ( ( ug_mode & (UM_INST|UM_MOD) && ~ug_mode & UM_RMV ) &&
	    ( ! ( strlen(javainfo.home) > 0 && LOexist(&javaloc) == OK )))
	{
	    char msgbuf[strlen(WARN_NO_VALID_JRE_FOUND) + 20];

	    STprintf(msgbuf, WARN_NO_VALID_JRE_FOUND, prodname);
	    gdk_threads_enter();
	    popup_info_box(msgbuf);
	    gdk_threads_leave();
	}
    }
# endif
    if ( do_install( inst_cmdline ) != OK )
	DBG_PRINT( "ERROR: Failed to lauch install process\n" );

    /* get GTK thread lock before doing GTK stuff */
    gdk_threads_enter();

    /* lookup text buffer to write to etc */
    OutTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstOutTextView" :
						"UpgOutTextView" );
    inst_out_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(
						 OutTextView ) );
    ErrTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstErrTextView" :
						"UpgErrTextView" );
    inst_err_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(
						 ErrTextView ) );
    InstProgBar = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"installprogress" : 
						"upgprogress" );
    gtk_progress_bar_set_pulse_step( GTK_PROGRESS_BAR( InstProgBar ), 0.01 );

    /* release GTK thread lock so that we don't hang the app whilst we sleep */
    gdk_threads_leave();

    /* poll the output files until the installer child exits */
    do
    {
	/* only re-write output every 100 loops to stop flickering */
	if ( i > 100 )
	{
            /*  wait and then try again, the output files may just be empty */
	    DBG_PRINT( "Calling write_file_to_text_buffer()\n" );
	    if ( write_file_to_text_buffer( inst_stdout,
					inst_out_buffer ) != OK )
		DBG_PRINT( "ERROR: Failed to write to output buffer\n" );

	    if ( write_file_to_text_buffer( inst_stderr,
					inst_err_buffer ) != OK )
		DBG_PRINT( "ERROR: Failed to write to output buffer\n" );
	    
	    /* reset counter */
	    i = 0;
	}

	/* get GTK thread lock */
	gdk_threads_enter();

	/* Show we're doing something */
	gtk_progress_bar_pulse( GTK_PROGRESS_BAR( InstProgBar ) );

	/* release GTK thread lock */
	gdk_threads_leave();

	PCsleep( 20 );
	i++; /* bump count */
    } while ( childinfo.alive ) ;

    /* one last read to make sure we got everything */
    if ( write_file_to_text_buffer( inst_stdout, inst_out_buffer ) != OK )
	DBG_PRINT( "ERROR: Failed to write to output buffer\n" );

    if ( write_file_to_text_buffer( inst_stderr, inst_err_buffer ) != OK )
	DBG_PRINT( "ERROR: Failed to write to output buffer\n" );

    if ( childinfo.exit_status != OK )
    {
	GtkWidget	*FailErrTextView;
	GtkWidget	*FailOutTextView;
	GtkWidget	*FailLogTextView;
        GtkTextBuffer	*fail_err_buffer;
        GtkTextBuffer	*fail_out_buffer;
        GtkTextBuffer	*fail_log_buffer;
	char		inst_log[MAX_LOC];
	char		*logdir;

	/* take GTK thread lock */
	gdk_threads_enter();
	popup_error_box( ERROR_COULD_NOT_COMPLETE_INSTALL );
	/* mark the stage as failed */
	stage_names[current_stage]->status = ST_FAILED;

        /* ready the install log for display */
	if ( runmode == MA_INSTALL )
	    logdir = iisystem.path;
	else
	    logdir = selected_instance->inst_loc;
	    
        snprintf( inst_log, MAX_LOC, "%s/%s", logdir, INSTALL_LOGFILE_NAME );

	/* write output and install.log to failure window */
	FailErrTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstFailErrorTextView" :
						"UpgFailErrorTextView" );
	fail_err_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(
						 FailErrTextView ) );

	FailOutTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstFailOutputTextView" :
						"UpgFailOutputTextView" );
	fail_out_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(
						 FailOutTextView ) );
	FailLogTextView = lookup_widget( IngresInstall, ug_mode & UM_INST ?
						"InstFailInstLogTextView" :
						"UpgFailInstLogTextView" );
	fail_log_buffer = gtk_text_view_get_buffer( GTK_TEXT_VIEW(
						 FailLogTextView ) );

	/* Release the lock before calling write_file_to_buffer() */
	gdk_threads_leave();

	if ( write_file_to_text_buffer( inst_stderr, fail_err_buffer ) != OK )
	     DBG_PRINT( "ERROR: Failed to write %s\nto error buffer\n",
								inst_stderr );

	if ( write_file_to_text_buffer( inst_stdout, fail_out_buffer ) != OK )
	     DBG_PRINT( "ERROR: Failed to write %s\nto output buffer\n",
								inst_stdout );

	if ( write_file_to_text_buffer( inst_log, fail_log_buffer ) != OK )
	     DBG_PRINT( "ERROR: Failed to write %s\nto log buffer\n",
								inst_log);

    }

    if ( ug_mode & (UM_INST|UM_MOD) && ~ug_mode & UM_RMV &&
	     LOexist( &rfnameloc ) == OK )
    {
	char	 rfsavestr[MAX_LOC] = {'\0'};
	LOCATION rfsaveloc;
	char	*instid;
	
	/* Install was successfull, copy response file to /var/lib/ingres/XX */
	if ( ug_mode & UM_INST )
	    instid = instID;
	else
	    instid = selected_instance->instance_ID;

	rfstat = gen_rf_save_name( rfsavestr, instid );

	if ( rfstat == OK )
	    rfstat = LOfroms(PATH & FILENAME, rfsavestr, &rfsaveloc);

	if ( rfstat == OK )
	    rfstat = SIcopy(&rfnameloc, &rfsaveloc);

	if ( rfstat != OK )
	{
	    char warnmsg[MAX_LOC + 50 ];

	    snprintf( warnmsg, MAX_LOC + 50,
			WARN_COULD_NOT_SAVE_RF, rfsavestr );
	    gdk_threads_enter();
	    popup_warning_box( warnmsg );
	    gdk_threads_leave();
	}
    }
	    
    /* Install is complete, remove the response file */
    if ( LOexist( &rfnameloc ) == OK )
    	LOdelete( &rfnameloc );

    /* get GTK thread lock again before moving on */
    gdk_threads_enter();
        
    on_master_next_clicked( NULL, NULL ); /* move on again */

    /* release GTK thread lock */
    gdk_threads_leave();

    /* done */
    return(NULL); 
}

static STATUS
do_install( char *inst_cmdline )
{
    PID		instpid;
    STATUS	forkrc;
    int		i = 0;
    char	*argv[5];

    if ( inst_cmdline == NULL || *inst_cmdline == '\0' )
	return( FAIL );

    /* prepare command for execution */
    argv[i++] = "/bin/sh" ;
    if ( new_pkgs_info.pkgfmt == PKGDEB )
	argv[i++] = "-c" ;
    else
    {
	argv[i++] = "-p" ;
	argv[i++] = "-c" ;
    }
    argv[i++] = inst_cmdline ;
    argv[i++] = NULL ;

    /* fork to run the install */
    instpid = PCfork(&forkrc);
    if ( instpid == 0 ) /* child, run the install */
    {
	i4	rc = -1;
	pid_t	cpid = getpid();
  
	/* restore the default handler in the child */
	EXsetsig( SIGCHLD, SIG_DFL );

	/* launch install */
	execvp( argv[0], argv );
	DBG_PRINT( "Child %d, exited with %d\n", cpid ,rc );
	sleep( 2 );
	PCexit( rc );
    }
    else if ( instpid > 0 ) /* parent, return to caller */
    {
	DBG_PRINT( "Installation launched with pid = %d\n", instpid );
	childinfo.alive = TRUE;
        /* give the install process a chance to start up */
	PCsleep( 2000 );
	return( forkrc );
    }
    else /* fork failed */
	return( forkrc );
    
}

int
generate_command_line( char **cmd_line )
{
    char	tmpbuf[MAX_LOC];

    /* calculate the length of the command line we're going to generate */
    if ( calc_cmd_len(&cmdlen) || cmdlen == 0 )
	return( FAIL );

    DBG_PRINT("Allocating %ld bytes for command line", cmdlen);
    /* allocate memory for the command line */
    *cmd_line = malloc( cmdlen );

    if ( *cmd_line == NULL )
	return( FAIL );

    **cmd_line = '\0';

    /* whole command wrapped in '( )' so all output can be re-directed */
    /* Add opening '(' and set umask and response file */
    snprintf( tmpbuf, MAX_LOC, "( %s ; %s=%s; ",
		UMASK_CMD,
		RF_VARIABLE,
		rfnameloc.string );
    gipAppendCmdLine(*cmd_line, tmpbuf);

    if ( new_pkgs_info.pkgfmt == PKGRPM )
	gen_rpm_command_line( cmd_line );
    else
	gen_deb_command_line( cmd_line );

    /* add setup/startup command */
    gen_setup_command_line( cmd_line );

    if ( misc_ops & GIP_OP_REMOVE_ALL_FILES )
	gen_rmv_files_command_line( cmd_line );
    /* add closing ')' */
    gipAppendCmdLine( *cmd_line, " ) " );

    /* finally add log files */
    snprintf( tmpbuf, MAX_LOC, " 1> %s", inst_stdout );
    gipAppendCmdLine( *cmd_line, tmpbuf );
    snprintf( tmpbuf, MAX_LOC, " 2> %s", inst_stderr );
    gipAppendCmdLine( *cmd_line, tmpbuf );

}

int
gen_rmv_files_command_line( char **cmd_line )
{
    auxloc	*locptr;
    char	tmpbuf[MAX_LOC];
    /*
    ** remove $II_SYSTEM/ingres/. and any aux data locations 
    */
    /* remove database locations first... */
    snprintf( tmpbuf, MAX_LOC, " && %s ", RM_CMD );
    gipAppendCmdLine( *cmd_line, tmpbuf );

    locptr = selected_instance->datalocs;
    if ( locptr == NULL )
	DBG_PRINT( "auxlocs is NULL, no aux locations found\n" );

    while ( locptr != NULL )
    {
	snprintf( tmpbuf, MAX_LOC, "%s/ingres/%s/ ",
		    locptr->loc,
		    dblocations[locptr->idx]->dirname );
	gipAppendCmdLine( *cmd_line, tmpbuf );
	locptr = locptr->next_loc ;
    }
	
    /* ...then log file locations... */
    snprintf( tmpbuf, MAX_LOC, " && %s ", RM_CMD );
    gipAppendCmdLine( *cmd_line, tmpbuf );

    locptr = selected_instance->loglocs;
    if ( locptr == NULL )
	DBG_PRINT( "auxlocs is NULL, no aux locations found\n" );

    while ( locptr != NULL )
    {
	snprintf( tmpbuf, MAX_LOC, "%s/ingres/log/ ",
		    locptr->loc );
	gipAppendCmdLine( *cmd_line, tmpbuf );
	locptr = locptr->next_loc ;
    }
	
    /* ...then the status dir */
    snprintf( tmpbuf, MAX_LOC, "/var/lib/ingres/%s ",
			selected_instance->instance_ID );
    gipAppendCmdLine( *cmd_line, tmpbuf );

    /* ...then II_SYSTEM */
    snprintf( tmpbuf, MAX_LOC, "%s/ingres/", selected_instance->inst_loc );
    gipAppendCmdLine( *cmd_line, tmpbuf );
}

int
gen_setup_command_line( char ** cmd_line )
{
    char	tmpbuf[MAX_LOC];
    char	*instid;

    if ( ug_mode & UM_INST )
	instid=instID ;
    else
	instid=selected_instance->instance_ID;
    
    if ( (ug_mode & (UM_INST|UM_UPG)) || /* new install or upgrade */
             mod_pkgs_to_install != PKG_NONE ) /* or modify to add */
    {
	/*
	** Run the setup using the init script
	** If everything went OK start the instance, if it fails,
	** make sure it's shut down properly
	*/
	gipAppendCmdLine( *cmd_line, " && ( " );
	snprintf( tmpbuf, MAX_LOC, INGCONFIG_CMD, instid, rfnameloc.string );
	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " && " );
	snprintf( tmpbuf, MAX_LOC, INGSTART_CMD, instid );
	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " || ( " );
	snprintf( tmpbuf, MAX_LOC, INGSTOP_CMD, instid );
	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " ; exit 1 ) ) " );

# ifdef conf_BUILD_MGMTSVC
	/*
	** Setup and start the remote management service
	*/
	gipAppendCmdLine( *cmd_line, " && ( " );
	snprintf( tmpbuf, MAX_LOC, MGMTCONFIG_CMD, instid, rfnameloc.string );
	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " && " );
	snprintf( tmpbuf, MAX_LOC, MGMTSTART_CMD, instid );
	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " || ( " );
	snprintf( tmpbuf, MAX_LOC, MGMTSTOP_CMD, instid );
	gipAppendCmdLine( *cmd_line, tmpbuf );
	gipAppendCmdLine( *cmd_line, " ; exit 1 ) ) " );
# endif

    }
 
    return(OK);
}

int
calc_cmd_len(size_t *cmdlen)
{
    /* start with the response file */
    *cmdlen = STlen( UMASK_CMD ) + 
		STlen( RF_VARIABLE ) +
		STlen( rfnameloc.string ) +
		7 ;

    /* add the pkg dependent stuff */
    if ( new_pkgs_info.pkgfmt == PKGRPM )
	calc_rpm_cmd_len(cmdlen);
    else
	calc_deb_cmd_len(cmdlen);

    /* remove $II_SYSTEM/ingres/. and other locs if we've been asked to */
    if ( misc_ops & GIP_OP_REMOVE_ALL_FILES )
    {
	auxloc	*locptr;

	*cmdlen += STlen( RM_CMD ) /* rm -rf */
		    + STlen( selected_instance->inst_loc ) /* II_SYSTEM */
		    + 9; /* for /ingres/.  and spaces */

	/* now add on space for any auxillery locations */
	locptr = selected_instance->datalocs;
	while ( locptr != NULL )
	{
	    *cmdlen += STlen( locptr->loc );
	    locptr = locptr->next_loc ;
	}
	
	locptr = selected_instance->loglocs;
	while ( locptr != NULL )
	{
	    *cmdlen += STlen( locptr->loc );
	    locptr = locptr->next_loc ;
	}
    } 

    /*
    ** Finally, on success, start the installation, if that fails then
    ** make sure it's shut down.
    **
    ** configuration and startup commands:
    ** && ( /etc/init.d/ingresXX configure /tmp/iirfinstall.6854 &&
    **		 /etc/init.d/ingresII start || 
    **	( /etc/init.d/ingresII stop > /dev/null 2>&1 ; exit 1 ) ) 
    */
    *cmdlen += STlen( INGCONFIG_CMD )
		+ STlen( rfnameloc.string )
		+ STlen( INGSTART_CMD )
		+ STlen( INGSTOP_CMD )
		+ 40 ; /* misc characters etc. */

# ifdef conf_BUILD_MGMTSVC
    /* 
    ** Configure and start the remote management service
    **
    ** && ( /etc/init.d/iimgmtsvcII configure response.file &&
    **		 /etc/init.d/iimgmtsvcII start || 
    **	( /etc/init.d/iimgmtsvcII stop > /dev/null 2>&1 ; exit 1 ) ) 
    ** 
    */
    *cmdlen += STlen( MGMTCONFIG_CMD )
		+ STlen( rfnameloc.string )
		+ STlen( MGMTSTART_CMD )
		+ STlen( MGMTSTOP_CMD )
		+ 40 ; /* misc characters etc. */

    /* log files */
    *cmdlen += STlen( inst_stdout ) + STlen( inst_stderr ) + 5 ;
# endif

    /* log files */
    *cmdlen += STlen( inst_stdout ) + STlen( inst_stderr ) + 5 ;

    DBG_PRINT( "%ld bytes have been allocated for the command line\n", *cmdlen );
    return(OK);
}

STATUS
gipAppendCmdLine( char *cmdbuff, char *newcmd )
{
    STATUS status = OK;

    if ( STlen(newcmd) + STlen(cmdbuff) + 1 > cmdlen )
    {
	/*
	** we're going to blow the command line buffer so error
	** and exit cleanly
	*/
	gdk_threads_enter();
	popup_error_box( ERROR_CMDLINE_BUFF_OVERRUN );
	gdk_threads_leave();
	gtk_main_quit();
    }
    else
        STcat(cmdbuff, newcmd);

    return(status);
}
# endif /* xCL_GTK_EXISTS */
