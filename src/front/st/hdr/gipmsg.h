/*
** Copyright Actian Corporation 2011. All rights reserved.
*/

# include <gipsup.h>

/*
** Name: gtkmsg.h
**
** Description:
**
**	Header file for ALL non-interface messages, errors and strings
**	in the Linux GUI installer.
**	ALL user messages should now be added here using the _() gettext()
**	macro.
**
** History:
**	01-Feb-2011 (hanje04)
**	    Created.
**	24-Feb-2011 (hanje04)
**	    Add new messages from 1.5
**	16-Mar-2011 (hanje04)
**	    Add INFO_DIALOG_TITLE from giputil.h
**	17-May-2011 (hanje04)
**	    Mantis 1942
**	    Use "prodname" in messages rather than "Ingres".
**	    Correct working for "shutdown" messages.
**	06-Oct-2011 (hanje04)
**	    Bug 125688
**	    Define WARN_NO_VALID_JRE_FOUND for popup box highlighting the
**	    missing JRE required by the remote management service
**	20-Oct-2011 (hanje04)
**	    Mantis 2771
**	    Reformat WARN_NO_VALID_JRE_FOUND.
*/

# define ERROR_NOT_ROOT _("Error: Must be run as root\n")
# define ERROR_PATH_TOO_LONG _("Error: Path too long\n")
# define ERROR_PKGINFO_NOT_FOUND _("Error: %s does not exist\n")
# define ERROR_MISSING_ARGUMENT _("Error: Missing argument, need pkginfo file\n")
# define ERROR_PKG_QUERY_FAIL _("Failed to query package info\n")
# define ERROR_INSTALL_INIT_FAIL _("Failed to initialize install mode\n")
# define ERROR_LICENSE_NOT_FOUND _("Error: %s does not exist\n")
# define ERROR_CREATE_TEMP_LOC _("Failed to create temporary location\n")
# define ERROR_UPGRADE_INIT_FAIL _("Failed to initialize upgrade mode\n")
# define ERROR_REMOVE_TEMP_LOC _("Failed to remove temporary location\n")
# define RFGEN_IVW_TITLE _("VectorWise Installation Wizard")
# define RFGEN_TITLE _("Ingres Installation Wizard")
# define ERROR_COULD_NOT_COMPLETE_INSTALL _("The installation process did not\ncomplete successfully")
# define WARN_COULD_NOT_SAVE_RF _("Could not save response file: %s")
# define ERROR_CMDLINE_BUFF_OVERRUN _("FATAL ERROR: Command line exceeds allocated space. Aborting")
# define QUIT_DIALOG_TITLE _("Quit?")
# define QUIT_DIALOG_LABEL _("Are you sure you want to quit?")
# define BROWSE_DIALOG_TITLE _("Choose a location")
# define SHUTDOWN_FAILED _("Unable to shut down the %s %s instance. The instance must be shut down before the installation can continue.")
# define ERROR_INST_NAME_IN_USE _("The instance name you have\nselected is already in use.")
# define SAVE_RESPONSE_FILE_DIALOG_TITLE _("Save Response File")
# define ERROR_RF_SAVE_FAILED _("Failed to save response file to %s\n")
# define WARNING_REMOVE_FILES _("All files under the following locations will be removed:\n")
# define WARN_DIALOG_TITLE _("Warning")
# define ERROR_DIALOG_TITLE _("Error")
# define INFO_DIALOG_TITLE _("Information")
# define ERROR_RF_SAVE_FAILED _("Failed to save response file to %s\n")
# define ERROR_CREATE_RF_FAIL _("Response file creation failed with:\n\t%s")
# define ERROR_GEN_CMD_FAIL _("Failed to generate command line")
# define SHUTDOWN_RUNNING_INSTANCE _("The Ingres %s instance appears to be running. It must be shutdown for the installation to continue, do you want to shut it down now?")
# define QUESTION_DIALOG_TITLE _("Question")
# define GIP_STR_YES _("Yes")
# define GIP_STR_NO _("No")
# define WARN_SYSINFO_INIT_FAIL _("Warning: Failed to initialize the system info\n")
# define MESSAGE_SHMMAX_TOO_LOW _("System parameter 'kernel.shmmax' is currently %ld Mb,\nwhich is insufficient for %s.\nIncreasing to %ld Mb, see the Getting Started\nGuide for more information.")
# define WARN_COULD_NOT_SAVE_RF _("Could not save response file: %s")
# define ERROR_CMDLINE_BUFF_OVERRUN _("FATAL ERROR: Command line exceeds allocated space. Aborting")
# define GIP_MEM_REQ_EXCEED_PHYSICAL _( "Settings for Processing Memory and Buffer Pool Memory exceed 90% of\nphysical system memory. This may cause the VectorWise Server to swap\nheavily or fail on startup. Reduce one or both settings")
# define WARN_NO_VALID_JRE_FOUND _("For the %s Remote Manager Service to run, Java\nRuntime Environment "MINJREVERS" or later must be installed. A valid\nversion was not detected on your machine. The discovery service\nwill not work without a compatible version of JRE installed.")
