/*
**Copyright (c) 2004 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<systypes.h>
# include	<pwd.h>
# include	<id.h>

/*
**	IDsetsys
**		Set up generic system identifiers.
**
**		Override system defaults for:
**		    SystemLocationVariable     ("II_SYSTEM")
**		    SystemLocationSubdirectory ("ingres")
**		    SystemProductName          ("CA-OpenIngres")
**		    SystemDBMSName             ("INGRES")
**		    SystemVarPrefix            ("II")
**		    SystemCfgPrefix            ("ii")
**		    SystemCatPrefix            ("ii")
**		    SystemAdminUser            ("ingres")
**		    SystemSyscatOwner          ("$ingres")
**
**		Defaults are defined in gl.h.
**
** History:
**	10-oct-1996 (canor01)
**	    Created.
**	15-nov-2010 (stephenb)
**	    Include id.h for prototypes.
*/

VOID
IDsetsys(
          char *SysLocationVariable,
          char *SysLocationSubdirectory,
          char *SysProductName,
          char *SysDBMSName,
          char *SysVarPrefix,
          char *SysCfgPrefix,
          char *SysCatPrefix,
          char *SysAdminUser,
          char *SysSyscatOwner
)
{
    if ( SysLocationVariable != NULL )
        SystemLocationVariable = SysLocationVariable;
    if ( SysLocationSubdirectory != NULL )
        SystemLocationSubdirectory = SysLocationSubdirectory;
    if ( SysProductName != NULL )
        SystemProductName = SysProductName;
    if ( SysDBMSName != NULL )
        SystemDBMSName = SysDBMSName;
    if ( SysVarPrefix != NULL )
        SystemVarPrefix = SysVarPrefix;
    if ( SysCfgPrefix != NULL )
        SystemCfgPrefix = SysCfgPrefix;
    if ( SysCatPrefix != NULL )
        SystemCatPrefix = SysCatPrefix;
    if ( SysAdminUser != NULL )
        SystemAdminUser = SysAdminUser;
    if ( SysSyscatOwner != NULL )
        SystemSyscatOwner = SysSyscatOwner;
    return;
}
