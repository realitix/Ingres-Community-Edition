/*
** Copyright (c) 2011 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BTallzeros -- check bitvector for being all zeros
**
** Description:
**		Check that bitvector has no bits set.
**
** Inputs:
**	vector		input vector to check
**	size		number of bits in byte streams to check
**			NOTE: Rounded up to BITSPERBYTE.
**
** Outputs:
**	Returns:
**		bool	TRUE if no bit set.
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	16-Mar-2011 (kiria01)
**	    Created
*/

bool
BTallzeros(
    const void	*vector,
    i4		size)

{
    register const char *b;
    register u_i4 bytes = BT_NUMBYTES_MACRO(size);
 
    /*
    ** use i4's or i8's to grab 32 or 64 bits at a time.
    ** Common cases of "size" are 126/128 and 1024.
    ** 126 is # of range variable bits in OPF bitmaps.
    ** 1024 is # of column bits in domain bitmaps.
    ** Note that many machines like --i loops better than i-- loops...
    */

#ifdef LP64
    /*
    ** test for long alignment, so that we may work with 64 bit values
    */

    if (bytes >= sizeof(u_i8) &&
		((SCALARP)vector & 7) == 0)
    {
	register const u_i8 *l = vector;

	/* Ok, vector is are long-aligned.
	** Useful size special case is 16 bytes == 2 longs.
	** The 1024 common-case is 16 longs, maybe a little
	** too much for unrolling.  (** Untested assumption!) */

	if (bytes == 2 * sizeof(u_i8))
	{
	    return (l[0] == 0 && l[1] == 0);
	}
	else
	{
	    /* At least one long, do long and leftovers */
	    register u_i4 lnum;    /*  Must 'and' 'lnum' longs  */

	    lnum = bytes / sizeof(u_i8);	/* Nonzero */
	    bytes &= sizeof(u_i8)-1;	/* Bytes left over */

	    do {
		if (*l != 0)
		    return FALSE;
		l++;
	    } while (--lnum != 0);

	    if (bytes != 0)
	    {
		b = (const char*)l;   /* And this takes care of the bytes */

		do {
		    if (*b != 0)
			return FALSE;
		    b++;
		} while (--bytes != 0);
	    }
	    return TRUE;
	}
    } /* end of long-aligned cases */
    else
#endif /* LP64 */
 
    /*
    ** test for word alignment, so that we may work with 32 bit values
    */

    if (bytes >= sizeof(u_i4) &&
		((SCALARP)vector & 3) == 0)
    {
	register const u_i4 *w = vector;

	/* Ok, vectors are i4-aligned.
	** Useful size special case is 16 bytes == 4 words.
	** The 1024 common-case is 32 words, probably a little
	** too much for unrolling.
	*/

	if (bytes == 4 * sizeof(u_i4))
	{
	    return (w[0] == 0 && w[1] == 0 &&
		    w[2] == 0 && w[3] == 0);
	}
	else
	{
	    /* At least one word, do words and leftovers */
	    register u_i4 wnum;    /*  Must 'and' 'wnum' words  */
 
	    wnum = bytes / sizeof(u_i4);	/* Nonzero */
	    bytes &= sizeof(u_i4)-1;	/* Bytes left over */
 
	    do {
		if (*w != 0)
		    return FALSE;
		w++;
	    } while (--wnum != 0);
 
	    if (bytes != 0)
	    {
		b = (const char*)w;   /* And this takes care of the bytes */
 
		do {
		    if (*b != 0)
			return FALSE;
		    b++;
		} while (--bytes != 0);
	    }
	    return TRUE;
	}
    } /* word-aligned pointers */

    /*
    **  No alignment, or short size, so degrade to simply a byte at a time.
    **  this is not expected to happen, but if it does, this code is the
    **  catch-all that will take care of it.  Dealing with unaligned
    **  addresses would have unnecessarily complicated the code.
    */

    b = vector;
    if (bytes != 0)
    {
	do {
	    if (*b != 0)
		return FALSE;
	    b++;
	} while (--bytes != 0);
    }
    return TRUE;
} 


/*{
** Name:	BTallones -- check bitvector for being all ones
**
** Description:
**		Check that bitvector has all bits set.
**
** Inputs:
**	vector		input vector to check that all bits are set
**	size		number of bits in byte streams to 'and'.
**			Rounded up to BITSPERBYTE.
**
** Outputs:
**	Returns:
**		bool	TRUE if all bits set.
**	Exceptions:
**		None
**
** Side Effects:
**		None
**
** History:
**	16-Mar-2011 (kiria01)
**	    Created
*/

bool
BTallones(
    const void	*vector,
    i4		size)

{
    register const char *b;
    register u_i4 bytes = BT_NUMBYTES_MACRO(size);

    /*
    ** use i4's or i8's to grab 32 or 64 bits at a time.
    ** Common cases of "size" are 126/128 and 1024.
    ** 126 is # of range variable bits in OPF bitmaps.
    ** 1024 is # of column bits in domain bitmaps.
    ** Note that many machines like --i loops better than i-- loops...
    */

#ifdef LP64
    /*
    ** test for long alignment, so that we may work with 64 bit values
    */

    if (bytes >= sizeof(u_i8) &&
	    ((SCALARP)vector & 7) == 0)
    {
	register const u_i8 *l = vector;

	/* Ok, vector is are long-aligned.
	** Useful size special case is 16 bytes == 2 longs.
	** The 1024 common-case is 16 longs, maybe a little
	** too much for unrolling.  (** Untested assumption!)
	*/

	if (bytes == 2 * sizeof(u_i8))
	{
	    return (~l[0] == 0 && ~l[1] == 0);
	}
	else
	{
	    /* At least one long, do long and leftovers */
	    register u_i4 lnum;    /*  Must 'and' 'lnum' longs  */
 
	    lnum = bytes / sizeof(u_i8);	/* Nonzero */
	    bytes &= sizeof(u_i8)-1;	/* Bytes left over */
 
	    do {
		if (~*l != 0)
		    return FALSE;
		l++;
	    } while (--lnum != 0);
 
	    if (bytes != 0)
	    {
		b = (const char*)l;   /* And this takes care of the bytes */
 
		do {
		    if (~*b != 0)
			return FALSE;
		    b++;
		} while (--bytes != 0);
	    }
	    return TRUE;
	}
 
 
    } /* end of long-aligned cases */
    else
#endif /* LP64 */
 
    /*
    ** test for word alignment, so that we may work with 32 bit values
    */

    if (bytes >= sizeof(u_i4) &&
	    ((SCALARP)vector & 3) == 0)
    {
	register const u_i4 *w = vector;

	/* Ok, vectors are i4-aligned.
	** Useful size special case is 16 bytes == 4 words.
	** The 1024 common-case is 32 words, probably a little
	** too much for unrolling. */

	if (bytes == 4 * sizeof(u_i4))
	{
	    return (~w[0] == 0 && ~w[1] == 0 &&
		    ~w[2] == 0 && ~w[3] == 0);
	}
	else
	{
	    /* At least one word, do words and leftovers */
	    register u_i4 wnum;    /*  Must 'and' 'wnum' words  */
 
	    wnum = bytes / sizeof(u_i4);	/* Nonzero */
	    bytes &= sizeof(u_i4)-1;	/* Bytes left over */
 
	    do {
		if (~*w != 0)
		    return FALSE;
		w++;
	    } while (--wnum != 0);
 
	    if (bytes != 0)
	    {
		b = (const char*)w;   /* And this takes care of the bytes */
 
		do {
		    if (~*b != 0)
			return FALSE;
		    b++;
		} while (--bytes != 0);
	    }
	    return TRUE;
	}
	    } /* word-aligned pointers */

    /*
    **  No alignment, or short size, so degrade to simply a byte at a time.
    **  this is not expected to happen, but if it does, this code is the
    **  catch-all that will take care of it.  Dealing with unaligned
    **  addresses would have unnecessarily complicated the code.
    */

    b = vector;
    if (bytes != 0)
	do {
	    if (~*b != 0)
		return FALSE;
	    b++;
	} while (--bytes != 0);
    return TRUE;
} 
