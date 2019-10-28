/*
** Copyright 2006 Actian Corporation. All rights reserved.
*/

/*
** Name: gipdata.h
** 
** Description:
**	Header file for defining GLOBALREFs for Linux GUI installer and
**	package manager.
**
** History:
**	02-Oct-2006 (hanje04)
**	    SIR 116877
**	    Created.
**	25-Oct-2006 (hanje04)
**	    SIR 116877
**	    Add globals for Charset and Timzone dialogs
**	    Add groupid
**	01-Dec-2006 (hanje04)
**	    Update GLOBALREF for existing_instance.
**	    Add GLOBALREF for date_type_alias structure.
**	07-Dec-2006 (hanje04)
**	    BUG 117278
**	    SD 112944
**	    Add adv_nodbms_stages[] for case where DBMS package is NOT selected
**	    for installation. 
**	22-Feb-2007 (hanje04)
**	    SIR 116911
**	    Add run_platform to define the OS we're running on so we can tell
**	    if it differs from the deployment platform.
**	08-Jul-2009 (hanje04)
**	    SIR 122309
**	    Add GLOBALREF for ing_config_type.
**	19-May-2010 (hanje04)
**	    SIR 123791
**	    Add GLOBALREFs for new VW config structures and stages.
**	01-Jun-2010 (hanje04)
**	    Mantis 741
**	    GLOBALREF for prodname (Ingres|VectorWise)
**	22-Nov-2010 (hanje04)
**	    Added GLOBALREFs for IVW client install path and upgrade
**	    modify "action" strings.
**	10-Dec-2010 (hanje04)
**	    Add IVW as II_CONFIG_TYPE
**	28-Jan-2011 (hanje04)
**	    Mantis 1303, 1348 & 1434
**	    Add santiy checking and upper limits to VW config parameters
**	    Move typedefs here from gip.h, where they really belong. Had to 
**	    move some stuff around fix compilation issues with the new
**	    gipsysconf.c module.
**	18-May-2011 (hanje04)
**	    Define DEBDefaultII_SYSTEM as location is fixed and different from
**	    other installs (/opt/Ingres/SYSTEM_PRODUCT_NAME)
**	31-Aug-2011 (hanje04)
**	    Add prod_label to store labelname:string pairs so that the product
**	    name (Ingres|VectorWise) and be substituted in as required
**	02-Oct-2011 (hanje04)
**	    Bug 125688
**	    Defite javainfo struture for remote manager service jre info
*/

/* Global refs */
/* Instance location and ID */
GLOBALREF char LnxDefaultII_SYSTEM[];
GLOBALREF char DEBDefaultII_SYSTEM[];
GLOBALREF char WinDefaultII_SYSTEM[];
GLOBALREF char *default_ii_system;
GLOBALREF char instID[];
GLOBALREF char dfltID[];

/* DB locations */
typedef struct _location {
	char name[30]; 
	char symbol[15];
	char dirname[10];
	II_RFAPI_PNAME rfapi_name;
	char path[MAX_LOC] ;
	} location ;
	
enum {
    INST_II_SYSTEM,
    INST_II_DATABASE,
    INST_II_CHECKPOINT,
    INST_II_JOURNAL,
    INST_II_WORK,
    INST_II_DUMP,
    INST_II_VWDATA,
    NUM_LOCS
} location_index;

GLOBALREF location iisystem;
GLOBALREF location *dblocations[];

/* transaction log */
typedef struct _log_info {
	location log_loc;
	location dual_loc;
	size_t size;
	bool duallog;
	} log_info;

GLOBALREF log_info txlog_info;

/* vectorwise config */
typedef enum {
	GIP_VWCFG_MAX_MEMORY = 0x01,
	GIP_VWCFG_BUFFERPOOL = 0x02,
	GIP_VWCFG_COLUMNSPACE = 0x04,
	GIP_VWCFG_BLOCK_SIZE = 0x08,
	GIP_VWCFG_GROUP_SIZE = 0x10,
	GIP_VWCFG_MAX_PARALLELISM = 0x20,
	} VWCFG ;

typedef enum {
	VWKB,
	VWMB,
	VWGB,
	VWTB,
	VWNOUNIT,
	} VWUNIT ;

typedef struct _vw_cfg {
	VWCFG bit;
	II_RFAPI_PNAME rfapi_name;
	const char *descrip;
	const char *field_prefix;
	SIZE_TYPE value;
	SIZE_TYPE dfval;
	VWUNIT unit;
	VWUNIT dfunit;
} vw_cfg ;

# define GIP_VWCFG_MAX_BLK_SIZE_MB 64
enum {
	GIP_VWCFG_IDX_MAX_MEM,
	GIP_VWCFG_IDX_BUFF_POOL,
	GIP_VWCFG_IDX_BLK_SIZE,
	GIP_VWCFG_IDX_GRP_SZ,
	GIP_VWCFG_IDX_MAX_PAR
} ;

GLOBALREF vw_cfg *vw_cfg_info[];
GLOBALREF char vw_cfg_units[];


/* misc options */
GLOBALREF misc_op_info *misc_ops_info[];
GLOBALREF MISCOPS misc_ops;
GLOBALREF MISCOPS WinMiscOps;

/* locale settings */
typedef struct _TIMEZONE {
    char *region;
    char *timezone;
  } TIMEZONE ;

typedef struct _CHARSET {
    char *region;
    char *charset;
    char *description;
  } CHARSET ;

typedef struct _TERMTYPE {
    char *class;
    char *terminal;
    } TERMTYPE ;

typedef struct _locale_info {
	char *timezone ;
	char *charset ;
	} ing_locale ;

GLOBALREF ing_locale locale_info;
GLOBALREF TIMEZONE timezones[];
GLOBALREF CHARSET linux_charsets[];
GLOBALREF CHARSET windows_charsets[];

/* ingres config type */
typedef struct {
		II_RFAPI_PNAME	rfapi_name;	
		const char	*values[5];
		const char	*descrip[5];
		enum {
		    TXN,
		    BI,
		    CM,
		    IVW,
		    TRAD
		    }		val_idx;
		} _ing_config_type ;
GLOBALREF _ing_config_type ing_config_type ;

/* date type alias */
typedef struct {
		II_RFAPI_PNAME	rfapi_name;	
		const char	*values[2];
		enum {
		    ANSI,
		    INGRES,
		    }		val_idx;
	} _date_type_alias ;

GLOBALREF _date_type_alias date_type_alias;
/* packages */
GLOBALREF package *packages[] ;
GLOBALREF package *licpkgs[] ;

/* GTK Globals */
GLOBALREF GtkWidget *IngresInstall;
GLOBALREF GtkWidget *CharsetDialog;
GLOBALREF GtkWidget *TimezoneDialog;
GLOBALREF GtkWidget *InstanceTreeView;
GLOBALREF GtkTreeIter *timezone_iter;		
GLOBALREF GtkTreeIter *charset_iter;		
GLOBALREF GtkTreeModel *CharsetTreeModel;
GLOBALREF GtkTreeModel *TimezoneTreeModel;

/* Installer variables */
GLOBALREF i4 instmode;
GLOBALREF i4 runmode;
GLOBALREF PKGLST pkgs_to_install;
GLOBALREF PKGLST mod_pkgs_to_install;
GLOBALREF PKGLST mod_pkgs_to_remove;
GLOBALREF const gchar install_notebook[];
GLOBALREF const gchar upgrade_notebook[];
GLOBALREF const gchar *current_notebook;
GLOBALREF bool upgrade;
GLOBALREF UGMODE ug_mode;
GLOBALREF UGMODE inst_state;
GLOBALREF instance *existing_instances[];
GLOBALREF instance *selected_instance;
GLOBALREF saveset new_pkgs_info;
GLOBALREF const char *WinServiceID;
GLOBALREF const char *WinServicePwd;
GLOBALREF char *userid;
GLOBALREF char *groupid;
GLOBALREF const char *defaultuser;
GLOBALREF i4	inst_count;
GLOBALREF i4 debug_trace;
GLOBALREF const char *prodname;

/* Stage info */
GLOBALREF stage_info *adv_stages[];
GLOBALREF stage_info *adv_ivw_stages[];
GLOBALREF stage_info *ivw_nodbms_stages[];
GLOBALREF stage_info *adv_nodbms_stages[];
GLOBALREF stage_info *basic_stages[];
GLOBALREF stage_info *basic_ivw_stages[];
GLOBALREF stage_info *rfgen_lnx_stages[];
GLOBALREF stage_info *rfgen_win_stages[];
GLOBALREF stage_info *ug_stages[];
GLOBALREF stage_info **stage_names;
GLOBALDEF i4 current_stage;

/* Response file and installation globals */
GLOBALREF LOCATION	rfnameloc ;
GLOBALREF char	rfnamestr[] ;
GLOBALREF char	rfdirstr[] ;
GLOBALREF LOCATION      rndirloc ;
GLOBALREF char          rndirstr[] ;
GLOBALREF II_RFAPI_PLATFORMS dep_platform ;
GLOBALREF II_RFAPI_PLATFORMS run_platform ;
GLOBALREF char  inst_stdout[];
GLOBALREF char  inst_stderr[];
GLOBALREF _childinfo childinfo;
typedef struct {
	char	home[MAX_LOC];
# define MAXJVLEN 10
# define MINJREVERS "1.6.0_14"
	char	version[MAXJVLEN];
	int	bits;
	} _javainfo;
GLOBALREF _javainfo javainfo;

/* Upgrade modify strings */
GLOBALREF const char	*ugactionstrlc;
GLOBALREF const char	*ugactionstrlced;
GLOBALREF const char	*ugactionstrcc;
GLOBALREF const char	*ugactionstr_cc;

/* Product dependent GUI strings/labels */
typedef struct _prod_label {
    char *labelname;
    char *string;
    } prod_label ;
GLOBALREF prod_label prod_label_list[];


