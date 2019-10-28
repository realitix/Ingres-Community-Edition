/*
**    Copyright (c) 1987, Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include       <iicommon.h>
# include	<me.h>
# include	<si.h>
# include	<nm.h>
# include	<lo.h>
# include	<pc.h>
# include	"nmlocal.h"
 
/*
NEEDLIBS = COMPAT 
**
UNDEFS = II_copyright
*/

/*
** Name: ingprenv.c - Print INGRES Environment Variables
**
** Description:
**	This program prints on the terminal the INGRES environment variables
**	and their values as defined in the file, "~ingres/files/symbol.tbl".
**
**    
**      main() - Main program to print out INGRES env variables
**
** History Log:	ingprenv.c,v
**
**	Revision 1.1  88/08/05  13:43:09  roger
**	UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
** 
**	Revision 1.3  87/11/13  11:32:51  mikem
**	use nmlocal.h rather than "nm.h" to disambiguate the global nm.h
**	header from the local nm.h header.
**      
**      Revision 1.2  87/11/10  14:43:48  mikem
**      Initial integration of 6.0 changes into 50.04 cl to create 
**      6.0 baseline cl
**      
**      Revision 1.3  87/07/27  11:05:01  mikem
**      Updated to meet jupiter coding standards.
**      
**	2/18/87 (daveb)	- Call to MEadvise, added MALLOCLIB hint.
**	27-may-97 (mcgem01)
**	    Clean up compiler warnings.
**	02-feb-2004 (somsa01)
**	    Added a "-f" flag to force ingprenv to exit cleanly and returning
**	    nothing when NMreadsyms() fails, which is only valid when printing
**	    an individual symbol.
**	03-feb-2004 (somsa01)
**	    Backed out last change for now.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**      19-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**/

/* # defines */
/* typedefs */
/* forward references */
/* externs */
/* statics */


/*
** Name: main - main program for printing ingres environment variables.
**
** Description:
**	This program prints on the terminal the INGRES environment variables
**	and their values as defined in the file, "~ingres/files/symbol.tbl".
**	If passed one symbol name as an argument, it will print just the
**	value of that symbol if defined.
**
** Inputs:
**	none
**
** Output:
**	none
**
**      Returns:
**	    none
** History:
**	20-jul-87 (mmm)
**          Updated to meet jupiter standards.
**	9-nov-92 (tyler)
**          Changed ingprenv to accept a single symbol name argument
**          and display only the value of that symbol if it is defined.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/
 
/*ARGSUSED*/
main( argc, argv )
int	argc;
char	**argv;
{
	register SYM	*sp;
 
	if (argc > 2)
	{
		SIfprintf( stderr, 
			"\nUsage: %s [ symbol ]\n\n", argv[ 0 ] );
		PCexit( 1 );
	}
 
	/* Use the ingres allocator instead of malloc/free default (daveb) */
	MEadvise( ME_INGRES_ALLOC );

	if ( NMreadsyms() != OK )
	{
		SIfprintf( stderr, "%s: Unable to read symbols from \"%s\"\n",
			argv[ 0 ], NMSymloc.path );
	}
	else
	{
		if( argc == 1 )
		{
			/* display names and values of all symbols */
			for( sp = s_list; sp != NULL; sp = sp->s_next )
				SIprintf( "%s=%s\n", sp->s_sym, sp->s_val );
		}
		else
		{
			/* display only the value of a specified symbol */
			char *s_val;

			NMgtIngAt( argv[ 1 ], &s_val );
			if( s_val != NULL && *s_val != EOS )
				SIprintf( "%s\n", s_val );
		}
	}
 
	PCexit(0);
}
