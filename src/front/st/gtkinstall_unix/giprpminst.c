/*
** Copyright 2011 Actian Corporation. All rights reserved.
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
** Name: giprpminst.c
**
** Description:
**
**      RPM specific installation module for the Linux GUI installer.
**
** History:
**
**	16-May-2011 (hanje04)
**          Created from gipmain.c
**      20-Jun-2011 (hanje04)
**	    Mantis 2135 & 2138
**	    Allow installation of signed saveset even if it can't be verified
*/

# define RF_VARIABLE "export II_RESPONSE_FILE"

/* local prototypes */
int calc_rpm_cmd_len(size_t *cmdlen);
static size_t get_rpm_rename_cmd_len( PKGLST pkglst );
static void add_rpmlst_to_rmv_command_line( char **cmd_line, PKGLST pkglist );
static void add_rpmlst_to_inst_command_line( char **cmd_line,
						PKGLST pkglist,
						bool renamed );
static size_t get_rpm_list_size( PKGLST pkglist );

int
gen_rpm_command_line( char **cmd_line )
{
    char	tmpbuf[MAX_LOC];
    GtkWidget   *checkButton;

    /* write to buffer */
    if ( ug_mode & UM_INST ) /* new installation */
    {
	/* do we need to rename? */
 	if ( inst_state & UM_RENAME )
	{
	    /* 
	    ** Use the 'iirpmrename' to re-package the RPM packages
	    ** to include the instance ID in the package name. The
	    ** new packages will be created in the directory pointed
	    ** to by 'rndirloc'
	    */
	
	    /* CD to temporary dir */
	    snprintf( tmpbuf, MAX_LOC, "cd %s && %s/bin/%s ",
			rndirstr,
			new_pkgs_info.file_loc,
			RPM_RENAME_CMD );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );

	    /* add package list */
	    add_rpmlst_to_inst_command_line( cmd_line, pkgs_to_install, FALSE );

	    /* add installation id and command suffix */
	    snprintf( tmpbuf, MAX_LOC, " %s && ", instID );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );

	    /*
	    ** Finally, update the saveset info to reflect the new
	    ** location of the RPM packages.
	    */
	    STcopy( rndirstr, new_pkgs_info.file_loc );
	}
	    

	/* rpm command and II_SYSTEM prefix */
	snprintf( tmpbuf, MAX_LOC, "%s %s=%s ",
		RPM_INSTALL_CMD,
		RPM_PREFIX,
		iisystem.path );
 	gipAppendCmdLine( *cmd_line, tmpbuf );

	if ( ! new_pkgs_info.checksig )
	    gipAppendCmdLine( *cmd_line, RPM_ALLOW_UNAUTH );

	add_rpmlst_to_inst_command_line( cmd_line, pkgs_to_install,
					(bool)( inst_state & UM_RENAME ) );


	if (instmode & IVW_INSTALL)
            checkButton = lookup_widget(IngresInstall, "ivwcheckdocpkg");
	else
            checkButton = lookup_widget(IngresInstall, "checkdocpkg");

        /* add doc last if it's needed: 
        **
        ** 1. no doc on renamed instance. 
        ** 2. no doc if the check box for documentation pkg is not 
        ** pressed in, which means documentation RPM might not be there. 
        ** (b119687)
        */ 
	if ( pkgs_to_install & PKG_DOC && 
             ~inst_state & UM_RENAME   &&
             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton))
           ) 
	{
	    snprintf( tmpbuf, MAX_LOC, " && %s ", RPM_INSTALL_CMD );
	    gipAppendCmdLine( *cmd_line, tmpbuf );

	    if ( ! new_pkgs_info.checksig )
		gipAppendCmdLine( *cmd_line, RPM_ALLOW_UNAUTH );

	    snprintf( tmpbuf, MAX_LOC, "%s/%s/%s-documentation-%s.%s.rpm ",
		new_pkgs_info.file_loc,
		new_pkgs_info.pkgdir,
		new_pkgs_info.pkg_basename,
		new_pkgs_info.version,
		new_pkgs_info.arch);
	    gipAppendCmdLine( *cmd_line, tmpbuf );

	}
    }
    else if ( ug_mode & UM_UPG ) /* upgrade existing */
    {
	PKGLST upg_list;

	/*
	** The list of packages we're installing as part of the upgrade
	** may not be identical to the list of packages already installed.
	** The upg_list is therefore the list of packages which are both
	** installed and present in the saveset.
	*/
	upg_list = new_pkgs_info.pkgs & selected_instance->installed_pkgs;

	/* 
	** There are a number of RPM issues associated with upgrading.
	** These can be avoided by using a 2 phase upgrade process.
	**
	** We first force install the new packages as an additional RPM
	** instance.
	*/

	/* do we need to rename? */
 	if ( selected_instance->action & UM_RENAME )
	{
	    /* 
	    ** Use the 'iirpmrename' to re-package the RPM packages
	    ** to include the instance ID in the package name. The
	    ** new packages will be created in the directory pointed
	    ** to by 'rndirloc'
	    */
	
	    /* CD to temporary dir */
	    snprintf( tmpbuf, MAX_LOC, "cd %s && %s/bin/%s ",
			rndirstr,
			new_pkgs_info.file_loc,
			RPM_RENAME_CMD );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );

	    /* add package list */
	    add_rpmlst_to_inst_command_line( cmd_line, upg_list, FALSE );

	    /* add installation id and command suffix */
	    snprintf( tmpbuf, MAX_LOC, " %s && ", selected_instance->instance_ID );
 	    gipAppendCmdLine( *cmd_line, tmpbuf );

	    /*
	    ** Finally, update the saveset info to reflect the new
	    ** location of the RPM packages.
	    */
	    STcopy( rndirstr, new_pkgs_info.file_loc );
	}

	/* rpm command and II_SYSTEM prefix */
	snprintf( tmpbuf, MAX_LOC, "%s %s %s=%s ",
		RPM_INSTALL_CMD,
		RPM_FORCE_INSTALL,
		RPM_PREFIX,
		selected_instance->inst_loc );
 	gipAppendCmdLine( *cmd_line, tmpbuf );

	if ( ! new_pkgs_info.checksig )
	    gipAppendCmdLine( *cmd_line, RPM_ALLOW_UNAUTH );

	add_rpmlst_to_inst_command_line( cmd_line, upg_list,
					(bool)( selected_instance->action
							& UM_RENAME ) );

	/*
	** Add command separator, use && so that if the first part
	** fails we don't continue and trash the system
	*/
	gipAppendCmdLine( *cmd_line, "&& " );

	/*
	** The old package info is then removed from the RPM
	** database.
	*/
 	gipAppendCmdLine( *cmd_line, RPM_UPGRMV_CMD );
	add_rpmlst_to_rmv_command_line( cmd_line,
					    selected_instance->installed_pkgs );

	/* add doc last if it's needed */
	if ( upg_list & new_pkgs_info.pkgs & PKG_DOC )
	{
	    snprintf( tmpbuf, MAX_LOC, " && %s ", RPM_INSTALL_CMD );
	    gipAppendCmdLine( *cmd_line, tmpbuf );	

	    if ( ! new_pkgs_info.checksig )
		gipAppendCmdLine( *cmd_line, RPM_ALLOW_UNAUTH );

	    snprintf( tmpbuf, MAX_LOC, "%s/%s/%s-documentation-%s.%s.rpm ",
		new_pkgs_info.file_loc,
		new_pkgs_info.pkgdir,
		new_pkgs_info.pkg_basename,
		new_pkgs_info.version,
		new_pkgs_info.arch);
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	}
    }
    else if ( ug_mode & UM_MOD ) /* modify existing */
    {
	/*
	** Modify can also be a 2 phase process as user can choose to
	** both add and remove packages. Perform the removal first.
	*/
	if ( mod_pkgs_to_remove != PKG_NONE )
	{
	    /*
	    ** if only the doc package is being removed,
	    ** skip adding any command line for the rest
	    */
	    if ( mod_pkgs_to_remove != PKG_DOC )
	    {
		/* rpm command */
		gipAppendCmdLine( *cmd_line, RPM_REMOVE_CMD );
		add_rpmlst_to_rmv_command_line( cmd_line, mod_pkgs_to_remove );
	    }
	   
	    /* add doc last if it's needed */
	    if ( mod_pkgs_to_remove & PKG_DOC )
	    {
		snprintf( tmpbuf, MAX_LOC, " && %s %s-documentation-%s ",
		    RPM_REMOVE_CMD,
		    selected_instance->pkg_basename,
		    selected_instance->version );
		gipAppendCmdLine( *cmd_line, tmpbuf );
	    }

	    /* add command separator if there's more to follow */
	    if ( mod_pkgs_to_install != PKG_NONE )
	        gipAppendCmdLine( *cmd_line, "&& " );
	}

	/* Now install the new packages */
	if ( mod_pkgs_to_install != PKG_NONE )
	{
	    /*
	    ** if only the doc package is being removed,
	    ** skip adding any command line for the rest
	    */
	    if ( mod_pkgs_to_install != PKG_DOC )
	    {
	        /* do we need to rename? */
 	        if ( selected_instance->action & UM_RENAME )
	        {
		    /* 
		    ** Use the 'iirpmrename' to re-package the RPM packages
		    ** to include the instance ID in the package name. The
		    ** new packages will be created in the directory pointed
		    ** to by 'rndirloc'
		    */
		
		    /* CD to temporary dir */
		    snprintf( tmpbuf, MAX_LOC, "cd %s && %s/bin/%s ",
				rndirstr,
				new_pkgs_info.file_loc,
				RPM_RENAME_CMD );
		    gipAppendCmdLine( *cmd_line, tmpbuf );

		    /* add package list */
		    add_rpmlst_to_inst_command_line( cmd_line,
						mod_pkgs_to_install,
						FALSE );
    
		    /* pass it as ID to renamd command */
		    snprintf( tmpbuf, MAX_LOC, " %s && ",
				selected_instance->instance_ID );
		    gipAppendCmdLine( *cmd_line, tmpbuf );

		    /*
		    ** Finally, update the saveset info to reflect the new
		    ** location of the RPM packages.
		    */
		    STcopy( rndirstr, new_pkgs_info.file_loc );
		}

		/* rpm command and II_SYSTEM prefix */
		snprintf( tmpbuf, MAX_LOC, "%s %s=%s ",
		    RPM_INSTALL_CMD,
		    RPM_PREFIX,
		    selected_instance->inst_loc );
		gipAppendCmdLine( *cmd_line, tmpbuf );

		if ( ! new_pkgs_info.checksig )
		    gipAppendCmdLine( *cmd_line, RPM_ALLOW_UNAUTH );

		/* add packages */
		add_rpmlst_to_inst_command_line( cmd_line, mod_pkgs_to_install,
					(bool)( selected_instance->action
							& UM_RENAME ) );

	    } /* ! doc only */

	    /* add doc last if it's needed */
	    if ( mod_pkgs_to_install & new_pkgs_info.pkgs & PKG_DOC )
	    {
		if ( mod_pkgs_to_install != PKG_DOC )
		    snprintf( tmpbuf, MAX_LOC, " && %s ", RPM_INSTALL_CMD );
		else
		    snprintf( tmpbuf, MAX_LOC, " %s ", RPM_INSTALL_CMD );

		gipAppendCmdLine( *cmd_line, tmpbuf );

		if ( ! new_pkgs_info.checksig )
		    gipAppendCmdLine( *cmd_line, RPM_ALLOW_UNAUTH );

		snprintf( tmpbuf, MAX_LOC, "%s/%s/%s-documentation-%s.%s.rpm ",
		    new_pkgs_info.file_loc,
		    new_pkgs_info.pkgdir,
		    new_pkgs_info.pkg_basename,
		    new_pkgs_info.version,
		    new_pkgs_info.arch);
		gipAppendCmdLine( *cmd_line, tmpbuf );
	    }
	}
    }
    else /* unknown or NULL state */
	return( FAIL );

    return( OK );
    
}
	 
int
calc_rpm_cmd_len(size_t *cmdlen)
{
    int i; /* bit counter */   
    int	numpkgs = 0;
    int pkg_path_len;

    if ( ug_mode & UM_INST ) /* new installation */
    {
	/* do we need to rename? */
 	if ( inst_state & UM_RENAME )
	    *cmdlen += get_rpm_rename_cmd_len( pkgs_to_install );

	/*
	** rpm command:
	** rpm -ivh --prefix $II_SYSTEM 
	*/
 	*cmdlen += STlen( RPM_INSTALL_CMD ) /* RPM command */
			+ STlen( RPM_PREFIX ) /* --prefix flag */
			+ STlen( iisystem.path ) /* value for II_SYSTEM */
			+ STlen( RPM_ALLOW_UNAUTH ) /* disable sig checking */
			+ 5 ; /* space for extras */

	/* package list */
	*cmdlen += get_rpm_list_size( pkgs_to_install );

	/*
	** finally add doc if it's needed
	**
	** rpm -ivh <doc-pkg>
	*/
 	if ( pkgs_to_install & PKG_DOC )
	    *cmdlen += STlen( RPM_INSTALL_CMD )
			+ STlen( RPM_ALLOW_UNAUTH ) /* disable sig checking */
			+ get_rpm_list_size( PKG_DOC );
			+ 5 ; /* extras */
    }
    else if ( ug_mode & UM_UPG ) /* upgrade */
    {
	/* do we need to rename? */
 	if ( selected_instance->action & UM_RENAME )
	    *cmdlen += get_rpm_rename_cmd_len( selected_instance->installed_pkgs );

	/*
	** rpm command:
	** rpm -e --justdb <pkglst> ; rpm -ivh --prefix $II_SYSTEM <pkg list>
	*/
 	*cmdlen += STlen( RPM_UPGRMV_CMD ) /* Remove command */
			+ STlen( RPM_INSTALL_CMD ) /* Install command */
			+ STlen( RPM_PREFIX ) /* --prefix flag */
			+ STlen( iisystem.path ) /* value for II_SYSTEM */
			+ STlen( RPM_ALLOW_UNAUTH ) /* disable sig checking */
			+ 5 ; /* space for extras */

	/* package list (x2, once for each command) */
	*cmdlen += 2 * get_rpm_list_size( selected_instance->installed_pkgs );

	/*
	** finally add doc if it's needed and available
	**
	** rpm -ivh <doc-pkg>
	*/
 	if ( selected_instance->installed_pkgs & new_pkgs_info.pkgs & PKG_DOC )
	    *cmdlen += STlen( RPM_INSTALL_CMD )
			+ STlen( RPM_ALLOW_UNAUTH ) /* disable sig checking */
			+ get_rpm_list_size( PKG_DOC );
			+ 5 ; /* extras */
    }
    else if ( ug_mode & UM_MOD ) /* modify */
    {
	/* do we need to rename? */
 	if ( selected_instance->action & UM_RENAME )
	    *cmdlen += get_rpm_rename_cmd_len( mod_pkgs_to_install );

	/* packages to remove */
	if ( mod_pkgs_to_remove != PKG_NONE )
	{
 	    *cmdlen += STlen( RPM_REMOVE_CMD ); /* RPM command */
	    *cmdlen += get_rpm_list_size( mod_pkgs_to_remove );

	    /*
	    ** finally remove doc if it needed
	    **
	    ** rpm -e <doc-pkg>
	    */
 	    if ( mod_pkgs_to_remove & PKG_DOC )
	        *cmdlen += STlen( RPM_REMOVE_CMD )
			+ get_rpm_list_size( PKG_DOC );
			+ 5 ; /* extras */
	}

	/* packages to add */
	if ( mod_pkgs_to_install != PKG_NONE )
	{
	    /* rpm command */
 	    *cmdlen += STlen( RPM_INSTALL_CMD ) /* RPM command */
			+ STlen( RPM_PREFIX ) /* --prefix flag */
			+ STlen( iisystem.path ) /* value for II_SYSTEM */
			+ STlen( RPM_ALLOW_UNAUTH ) /* disable sig checking */
			+ 5 ; /* space for extras */

	    *cmdlen += get_rpm_list_size( mod_pkgs_to_install );

	    /*
	    ** add doc if it's needed and available
	    **
	    ** rpm -ivh <doc-pkg>
	    */
	    if ( mod_pkgs_to_install & new_pkgs_info.pkgs & PKG_DOC )
		*cmdlen += STlen( RPM_INSTALL_CMD )
			+ get_rpm_list_size( PKG_DOC );
			+ STlen( RPM_ALLOW_UNAUTH ) /* disable sig checking */
			+ 5 ; /* extras */
	}

    }
    else /* should never get here */
	return(FAIL);

    return(OK);
}

static void
add_rpmlst_to_inst_command_line( char **cmd_line, PKGLST pkglist, bool renamed )
{
    char	tmpbuf[MAX_LOC]; /* path buffer */
    i4		i; /* loop counter */

    /*
    ** Package location and names differ between renamed
    ** and regular packages.
    */
 
    /* add core package first (if we want it) as it's a bit different */
    if ( pkglist & PKG_CORE )
    {
	gipAppendCmdLine( *cmd_line, new_pkgs_info.file_loc );

	/* and file name of package */
	if ( renamed )
	{
	    snprintf( tmpbuf, MAX_LOC, "/%s-%s-%s.%s.rpm ",
		new_pkgs_info.pkg_basename,
		ug_mode & UM_INST ? 
		instID : selected_instance->instance_ID,
		new_pkgs_info.version,
		new_pkgs_info.arch);
	}
	else
	{
	    snprintf( tmpbuf, MAX_LOC, "/%s/%s-%s.%s.rpm ",
		new_pkgs_info.pkgdir,
		new_pkgs_info.pkg_basename,
		new_pkgs_info.version,
		new_pkgs_info.arch);
	}
	gipAppendCmdLine( *cmd_line, tmpbuf );
    }

    /* now do the rest, skipping core, and documentation */
    i = 1;
    while( packages[i] != NULL )
    {
	/* skip doc package, it's different */    
	if ( packages[i]->bit == PKG_DOC )
	{
	    i++;
	    continue;
	}

	if( pkglist & packages[i]->bit )
	{
	    /* add package location */
	    gipAppendCmdLine( *cmd_line, new_pkgs_info.file_loc );

	    if ( renamed )
	    {
		/* and file name of package */
		snprintf( tmpbuf, MAX_LOC, "/%s-%s-%s-%s.%s.rpm ",
			new_pkgs_info.pkg_basename,
			packages[i]->pkg_name,
			ug_mode & UM_INST ? 
			instID : selected_instance->instance_ID,
			new_pkgs_info.version,
			new_pkgs_info.arch);

	    }
	    else
	    {
		/* and file name of package */
		snprintf( tmpbuf, MAX_LOC, "/%s/%s-%s-%s.%s.rpm ",
			new_pkgs_info.pkgdir,
			new_pkgs_info.pkg_basename,
			packages[i]->pkg_name,
			new_pkgs_info.version,
			new_pkgs_info.arch);
	    }
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	}
	i++;
    }
}

static void
add_rpmlst_to_rmv_command_line( char **cmd_line, PKGLST pkglist )
{
    char	tmpbuf[MAX_LOC]; /* path buffer */
    i4		i; /* loop counter */
    bool	renamed = FALSE ; 

    /*
    ** add core package first (if we're removing it)
    ** as it's a bit different
    */
    if ( pkglist & PKG_CORE )
    {
	snprintf( tmpbuf, MAX_LOC, "%s-%s ",
			selected_instance->pkg_basename,
			selected_instance->version );
        gipAppendCmdLine( *cmd_line, tmpbuf );
    }

    /*
    ** Now do the rest, skipping core. The package location and
    ** names differ between renamed and regular packages.
    */
    renamed = selected_instance->action & UM_RENAME;
    DBG_PRINT("%s %s a renamed package\n",
		selected_instance->pkg_basename,
		renamed ? "is" : "is not" );
    i = 1;

    while( packages[i] != NULL )
    {
	/* skip doc package, it's different */    
	if ( packages[i]->bit == PKG_DOC )
	{
	    i++;
	    continue;
	}

	if( pkglist & packages[i]->bit )
	{
	    /* add package */
	    if ( renamed )
	    {
		char *ptr1, *ptr2;

		STcopy( selected_instance->pkg_basename, tmpbuf );
		/*
		** search backwards for the '-' so we don't get
		** caught out by ca-ingres packages
		*/
		ptr1 = STrindex( tmpbuf, "-", 0 );
		ptr2 = STrindex( selected_instance->pkg_basename, "-", 0 );
		STprintf( ++ptr1, "%s-%s-%s ",
				packages[i]->pkg_name, 
				++ptr2,
				selected_instance->version );
		
	    }
	    else
		snprintf( tmpbuf, MAX_LOC, "%s-%s-%s ",
			selected_instance->pkg_basename,
			packages[i]->pkg_name,
			selected_instance->version );

	    DBG_PRINT("Adding %s to removal command line\n", tmpbuf);
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	}
	i++;
    }
}

static size_t
get_rpm_rename_cmd_len( PKGLST pkglst )
{
    size_t 	len ;

    len = STlen( rndirstr ) /* temporary output directory */
	  + STlen( new_pkgs_info.file_loc ) /* saveset location */
	  + STlen( RPM_RENAME_CMD ) /* rename command */
	    + 5; /* "cd " + spaces etc */

    len += get_rpm_list_size( pkglst );

    return( len );
}

static size_t
get_rpm_list_size( PKGLST pkglist )
{

    size_t     list_size;
    size_t     pkg_path_len;
    u_i4       i = 1; 
    int        numpkgs = 0;

    /* set the path length */
    pkg_path_len = STlen( new_pkgs_info.file_loc ) /* saveset location */
                + STlen( new_pkgs_info.pkgdir ) /* package file sub dir */
                + 3 ; /* space for extras */ 

    /* package list */
    while ( i <= ( MAXI4 ) )
    {
       if ( pkglist & i )
           numpkgs++; 
       i <<= 1;
    }

   /*
   ** for each rpm we have:
   **
   **     /full/path/to/pkgs/prodname-pkg[-XX]-x.y-z.rpm
   */
   DBG_PRINT("%d rpm packages in list\n", numpkgs);
   list_size = numpkgs * ( MAX_REL_LEN + 
		       MAX_PKG_NAME_LEN +
		       MAX_VERS_LEN +
		       MAX_FORMAT_LEN +
		       2 + /* installation ID (for renamed packages ) */
		       5 + /* separators */
		       pkg_path_len + 1 );

    /* finally add on fudge factor for spaces and ;s etc */
    list_size += 5;

    return(list_size);
}
# endif /* xCL_GTK_EXISTS */
