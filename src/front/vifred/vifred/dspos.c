/*
** Copyright (c) 2004 Actian Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ds.h>
# include	<feds.h>
# include	<fmt.h>
# include	"pos.h"
# include	<fedsz.h>

/*
** History:
**	29-oct-93 (swm)
**		Added history section, as it was missing.
**		Dummied out truncating i4 casts for offset definitions
**		when linting to avoid lint truncation warnings which can
**		safely be ignored.
*/

# ifndef	INCL_FEDSZ
# if defined(NODSPTRS) || defined(LINT)

# define	PSps_name	16
# define	PSps_feat	24
# define	PSps_column	28

# else

# define	PSps_name		(i4) &(((POS *)0)->ps_name)
# define	PSps_feat		(i4) &(((POS *)0)->ps_feat)
# define	PSps_column		(i4) &(((POS *)0)->ps_column)

# endif 	/* NODSPTRS */
# endif	/* INCL_FEDSZ */

/* Template of POS data structure */

static DSPTRS	pos_dsptr[] = {
	/* ds_off		       ds_kind   ds_sizeoff ds_temp */
	PSps_feat,   DSUNION, PSps_name, 0,
	PSps_column, DSNORMAL,         0, DS_POS
};

GLOBALDEF DSTEMPLATE DSpos = {
	/* ds_id */		DS_POS,
	/* ds_size */		sizeof(POS),
	/* ds_cnt */		2,
	/* dstemp_type */	DS_IN_CORE,
	/* ds_ptrs */		pos_dsptr,
	/* dstemp_file */	NULL,
	/* dstemp_func */	NULL	
};

static DSPTRS	ppos_ptrs[] = {
	/* ds_off	       ds_kind   ds_sizeoff ds_temp */
	0,			DSNORMAL,	0,	DS_POS,
};

GLOBALDEF DSTEMPLATE	DSppos = {
	/* ds_id */		DS_PPOS,
	/* ds_size */		sizeof(POS *),
	/* ds_cnt */		1,
	/* dstemp_type */	DS_IN_CORE,
	/* ds_ptrs */		ppos_ptrs,
	/* dstemp_file */	NULL,
	/* dstemp_func */	NULL	
};
