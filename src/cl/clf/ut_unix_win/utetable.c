/*
**  Copyright (c) 1985, 2004 Actian Corporation
**  All rights reserved.
*/

#include <compat.h>
#include <gl.h>
#include <ds.h>
#include <ut.h>
#include <st.h>

/*
** UTetable
**
** Scan an in-memory data structure as the moduleDictionary
**
** History:
**	15-may-97 (mcgem01)
**	    Clean up compiler warning by adding an include for st.h
**	06-oct-2003 (somsa01)
**	    Properly prototyped UTemodtype().
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	29-Nov-2010 (frima01) SIR 124685
**	    Changed argument of UTetable for prototyping purposes.
*/

ModDict *
UTetable(modname, dict)
char			*modname;
ArrayModDictP		*dict;
{
	int	i;
	ModDict	*d;
	/* search the table */
	for (i = 0; i < dict->size; i++) {
		d = dict->array[i];
		if (!STcompare(modname, d->modName))
			return(d);
	}

	/* none found */
	return (ModDict *) NULL;
}

i4
UTemodtype(s)
char *s;
{
	switch (*s) {
	  case 'C': case 'c':	return(UT_CALL);
	  case 'P': case 'p':	return(UT_PROCESS);
	  case 'O': case 'o':	return(UT_OVERLAY);
	  default:	return(0);
	}
}
