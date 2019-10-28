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
** Name: gipdebinst.c
**
** Description:
**
**      DEB/APT specific installation module for the Linux GUI installer.
**
** History:
**
**	16-May-2011 (hanje04)
**          Created.
*/

/* local prototypes */
int calc_deb_cmd_len(size_t *cmdlen);
static void add_deblst_to_command_line( char **cmd_line, PKGLST pkglist );
static size_t get_deb_list_size( PKGLST pkglist );

int
gen_deb_command_line( char **cmd_line )
{
    char	tmpbuf[MAX_LOC];
    GtkWidget   *checkButton;

    /* write to buffer */
    if ( ug_mode & UM_INST ) /* new installation */
    {
	/* deb install cmd */
 	gipAppendCmdLine( *cmd_line, DEB_INSTALL_CMD );
	add_deblst_to_command_line( cmd_line, pkgs_to_install );
 	gipAppendCmdLine( *cmd_line, DEB_HIDE_PROGRESS );


	if (instmode & IVW_INSTALL)
            checkButton = lookup_widget(IngresInstall, "ivwcheckdocpkg");
	else
            checkButton = lookup_widget(IngresInstall, "checkdocpkg");

        /* add doc last if it's needed: 
        **
        ** 1. no doc on renamed instance. 
        ** 2. no doc if the check box for documentation pkg is not 
        ** pressed in, which means documentation deb might not be there. 
        ** (b119687)
        */ 
	if ( pkgs_to_install & PKG_DOC && 
             ~inst_state & UM_RENAME   &&
             gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(checkButton))
           ) 
	{
	    snprintf( tmpbuf, MAX_LOC, " && %s ", DEB_INSTALL_CMD );
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	    snprintf( tmpbuf, MAX_LOC, "%s-documentation=%s",
		new_pkgs_info.pkg_basename,
		new_pkgs_info.version);
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	    gipAppendCmdLine( *cmd_line, DEB_HIDE_PROGRESS );
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

	/* deb upgrade command */
 	gipAppendCmdLine( *cmd_line, DEB_UPGRADE_CMD );
	add_deblst_to_command_line( cmd_line, upg_list );
 	gipAppendCmdLine( *cmd_line, DEB_HIDE_PROGRESS );

	/* add doc last if it's needed */
	if ( upg_list & new_pkgs_info.pkgs & PKG_DOC )
	{
	    snprintf( tmpbuf, MAX_LOC, " && %s ", DEB_UPGRADE_CMD );
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	    snprintf( tmpbuf, MAX_LOC, "%s-documentation=%s ",
		new_pkgs_info.pkg_basename,
		new_pkgs_info.version);
	    gipAppendCmdLine( *cmd_line, tmpbuf );
 	    gipAppendCmdLine( *cmd_line, DEB_HIDE_PROGRESS );
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
		/* deb command */
		gipAppendCmdLine( *cmd_line, DEB_REMOVE_CMD );
		add_deblst_to_command_line( cmd_line, mod_pkgs_to_remove );
 		gipAppendCmdLine( *cmd_line, DEB_HIDE_PROGRESS );
	    }
	   
	    /* add doc last if it's needed */
	    if ( mod_pkgs_to_remove & PKG_DOC )
	    {
		snprintf( tmpbuf, MAX_LOC, " && %s %s-documentation-%s ",
		    DEB_REMOVE_CMD,
		    selected_instance->pkg_basename,
		    selected_instance->version );
		gipAppendCmdLine( *cmd_line, tmpbuf );
 		gipAppendCmdLine( *cmd_line, DEB_HIDE_PROGRESS );
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
		/* deb command and II_SYSTEM prefix */
		gipAppendCmdLine( *cmd_line, DEB_INSTALL_CMD );

		/* add packages */
		add_deblst_to_command_line( cmd_line, mod_pkgs_to_install );
	        gipAppendCmdLine( *cmd_line, DEB_HIDE_PROGRESS );

	    } /* ! doc only */

	    /* add doc last if it's needed */
	    if ( mod_pkgs_to_install & new_pkgs_info.pkgs & PKG_DOC )
	    {
		if ( mod_pkgs_to_install != PKG_DOC )
		    snprintf( tmpbuf, MAX_LOC, " && %s ", DEB_INSTALL_CMD );
		else
		    snprintf( tmpbuf, MAX_LOC, " %s ", DEB_INSTALL_CMD );

		gipAppendCmdLine( *cmd_line, tmpbuf );
		snprintf( tmpbuf, MAX_LOC, "%s-documentation=%s ",
		    new_pkgs_info.pkg_basename,
		    new_pkgs_info.version);
		gipAppendCmdLine( *cmd_line, tmpbuf );
	        gipAppendCmdLine( *cmd_line, DEB_HIDE_PROGRESS );
	    }
	}
    }
    else /* unknown or NULL state */
	return( FAIL );

    return( OK );
    
}
	 
int
calc_deb_cmd_len(size_t *cmdlen)
{
    int i; /* bit counter */   
    int	numpkgs = 0;
    int pkg_path_len;

    if ( ug_mode & UM_INST ) /* new installation */
    {
	/*
	** apt command
	** apt-get install
	*/
 	*cmdlen += STlen( DEB_INSTALL_CMD ) /* deb command */
			+ 3 ; /* space for extras (spaces etc.) */

	/* package list */
	*cmdlen += get_deb_list_size( pkgs_to_install );

	/* remove "scrolling" output, need better fix but this will do fo now */
 	*cmdlen += STlen( DEB_HIDE_PROGRESS ) /* deb command */
			+ 3 ; /* space for extras (spaces etc.) */

	/*
	** finally add doc if it's needed
	**
	** apt-get install <doc-pkg>
	*/
 	if ( pkgs_to_install & PKG_DOC )
	{
	    *cmdlen += STlen( DEB_INSTALL_CMD )
			+ get_deb_list_size( PKG_DOC )
	    		+ STlen( DEB_HIDE_PROGRESS )
			+ 3 ; /* space for extras (spaces etc.) */
	}

    }
    else if ( ug_mode & UM_UPG ) /* upgrade */
    {
	/*
	** apt command:
	** apt-get install <pkg list>
	*/
 	*cmdlen += + STlen( DEB_INSTALL_CMD ) /* Install command */
			+ 3 ; /* space for extras (spaces etc.) */

	/* package list */
	*cmdlen += get_deb_list_size( selected_instance->installed_pkgs );
	*cmdlen += STlen( DEB_HIDE_PROGRESS );

	/*
	** finally add doc if it's needed and available
	**
	** apt-get install <doc pkg>
	*/
 	if ( selected_instance->installed_pkgs & new_pkgs_info.pkgs & PKG_DOC )
	    *cmdlen += STlen( DEB_INSTALL_CMD )
			+ get_deb_list_size( PKG_DOC )
			+ STlen( DEB_HIDE_PROGRESS )
			+ 3 ; /* space for extras (spaces etc.) */
    }
    else if ( ug_mode & UM_MOD ) /* modify */
    {
	/* packages to remove */
	if ( mod_pkgs_to_remove != PKG_NONE )
	{
	    /*
	    ** apt command
	    ** apt-get remove <pkg-list>
	    */
 	    *cmdlen += STlen( DEB_REMOVE_CMD ) 
	    		+ get_deb_list_size( mod_pkgs_to_remove )
			+ STlen( DEB_HIDE_PROGRESS )
			+ 3 ; /* space for extras (spaces etc.) */

	    /*
	    ** finally remove doc if it needed
	    **
	    ** apt-get remove <doc pkg>
	    */
 	    if ( mod_pkgs_to_remove & PKG_DOC )
	        *cmdlen += STlen( DEB_REMOVE_CMD )
			+ get_deb_list_size( PKG_DOC )
			+ STlen( DEB_HIDE_PROGRESS )
			+ 3 ; /* space for extras (spaces etc.) */
	}

	/* packages to add */
	if ( mod_pkgs_to_install != PKG_NONE )
	{
	    /* deb command */
 	    *cmdlen += STlen( DEB_INSTALL_CMD ) /* deb command */
			+ STlen( DEB_HIDE_PROGRESS )
			+ 5 ; /* space for extras (spaces etc.) */

	    *cmdlen += get_deb_list_size( mod_pkgs_to_install );

	    /*
	    ** add doc if it's needed and available
	    **
	    ** apt-get install <doc-pkg>
	    */
	    if ( mod_pkgs_to_install & new_pkgs_info.pkgs & PKG_DOC )
		*cmdlen += STlen( DEB_INSTALL_CMD )
			+ get_deb_list_size( PKG_DOC )
			+ STlen( DEB_HIDE_PROGRESS )
			+ 5 ; /* space for extras (spaces etc.) */
	}
    }
    else /* should never get here */
	return(FAIL);

    return(OK);
}

static void
add_deblst_to_command_line( char **cmd_line, PKGLST pkglist )
{
    char	tmpbuf[MAX_LOC]; /* path buffer */
    i4		i; /* loop counter */

    /* add core package first (if we want it) as it's a bit different */
    if ( pkglist & PKG_CORE )
    {
	snprintf( tmpbuf, MAX_LOC, "%s=%s ",
	    new_pkgs_info.pkg_basename,
	    new_pkgs_info.version );
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
	    /* and file name of package */
	    snprintf( tmpbuf, MAX_LOC, "%s-%s=%s ",
		    new_pkgs_info.pkg_basename,
		    packages[i]->pkg_name,
		    new_pkgs_info.version );
	    gipAppendCmdLine( *cmd_line, tmpbuf );
	}
	i++;
    }
}

static size_t
get_deb_list_size( PKGLST pkglist )
{

    size_t     list_size;
    u_i4       i = 1; 
    int        numpkgs = 0;
    /* package list */

    while ( i <= ( MAXI4 ) )
    {
       if ( pkglist & i )
           numpkgs++; 
       i <<= 1;
    }

   /*
   ** for debs it's just:
   **
   **    prodname-pkg_x.y.z
   */
   DBG_PRINT("%d deb packages in list\n", numpkgs);
   list_size = numpkgs * ( MAX_REL_LEN + 
		       MAX_PKG_NAME_LEN +
		       MAX_VERS_LEN +
		       5 /* separators */
		       );

    /* finally add on fudge factor for spaces and ;s etc */
    list_size += 5;

    return(list_size);
}


# endif /* xCL_GTK_EXISTS */
