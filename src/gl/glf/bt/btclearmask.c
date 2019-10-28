/*
** Copyright (c) 2004, 2011 Actian Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<bt.h>
# include	"bttab.h"

/*{
** Name:	BTclearmask -- 'clearmask' two byte streams
**
** Description:
**		Perform C 'clearmask' operation (&~) on byte streams
**
**		NOTE: Generated from BTops.template. Any changes to this file will
**		likely need applying to each derivative file so would probably be
**		best applied to the template and then the sources regenerating.
**		Applies to: btand.c, btor.c, btxor.c and btclearmask.c
** Inputs:
**	size		number of bits in byte streams to 'clearmask'.
**			Rounded up to BITSPERBYTE.
**	vector1		input vector to 'clearmask' with vector2
**	vector2		'clearmask'ed with vector1
**
** Outputs:
**	vector2		result of 'clearmask'ing vector1 and vector2
**
**	Returns:
**		None
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	29-dec-85 (daveb)
**		Cleaned up conditional compilation, 
**		removed size check form C not present in asm.
**	15-jan-86 (greg)
**		Use BT_XXX_MACROs and substitute `--' for `=- BITSPERBYTE'.
**		Conform to coding standards.
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**      21-apr-95 (tutto01)
**          Take advantage of 32 bit architectures.
**	14-sep-1995 (sweeney)
**	    purge WECOV, VAX defines, elide useless RCS history.
**	5-Aug-2004 (schka24)
**	    Update special-case unrolling; add i8-at-a-time on LP64;
**	    help compiler out with unsigned, --i loops.
**	16-Mar-2011 (kiria01)
**	    Update BT prototypes for void* args and returns.
**	    Generated this from btops.template.
*/

void *
BTclearmask(
    i4		size,
    const void	*vector1,
    void	*vector2)
{
    register const char *b1;
    register char *b2;
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
	    ((SCALARP)vector1 & 7) == 0 &&
	    ((SCALARP)vector2 & 7) == 0)
    {
	register const u_i8 *l1 = vector1;
	register u_i8 *l2 = vector2;
	register u_i4 lnum = bytes / sizeof(u_i8);	/* Nonzero */

	/* Ok, vectors are long-aligned.
	** Useful size special case is 16 bytes == 2 longs.
	** The 1024 common-case is 16 longs, maybe a little
	** too much for unrolling.  (** Untested assumption!) */
	switch (bytes)
	{
#ifdef BT_UNROLL
	case 16 * sizeof(u_i8):
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    /*FALLTHROUGH*/
	case 8 * sizeof(u_i8):
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    /*FALLTHROUGH*/
	case 4 * sizeof(u_i8):
	    *l2++ &= ~*l1++;
	    *l2++ &= ~*l1++;
	    /*FALLTHROUGH*/
#endif
	case 2 * sizeof(u_i8):
	    *l2++ &= ~*l1++;
	    *l2 &= ~*l1;
	    return vector2;
	default:
	    /* At least one long, do long and leftovers */
	    bytes &= sizeof(u_i8)-1;	/* Bytes left over */
 
	    do {
		*l2++ &= ~*l1++;
	    } while (--lnum != 0);
 
	    if (bytes != 0)
	    {
		b1 = (const char*)l1;   /* And this takes care of the bytes */
		b2 = (char*)l2;
 
		do {
		    *b2++ &= ~*b1++;
		} while (--bytes != 0);
	    }
	    return vector2;
	}
 
    } /* end of long-aligned cases */
    else
#endif /* LP64 */
 
    /*
    ** test for word alignment, so that we may work with 32 bit values
    */
    if (bytes >= sizeof(u_i4) &&
	    ((SCALARP)vector1 & 3) == 0 &&
	    ((SCALARP)vector2 & 3) == 0)
    {
	register const u_i4 *w1 = vector1;
	register u_i4 *w2 = vector2;
	register u_i4 wnum = bytes / sizeof(u_i4);	/* Nonzero */

	/* Ok, vectors are i4-aligned.
	** Useful size special case is 16 bytes == 4 words.
	** The 1024 common-case is 32 words, probably a little
	** too much for unrolling.  */
	switch (bytes)
	{
#ifdef BT_UNROLL
	case 32 * sizeof(u_i4):
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    /*FALLTHROUGH*/
	case 16 * sizeof(u_i4):
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    /*FALLTHROUGH*/
	case 8 * sizeof(u_i4):
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    /*FALLTHROUGH*/
#endif
	case 4 * sizeof(u_i4):
	    *w2++ &= ~*w1++; *w2++ &= ~*w1++;
	    *w2++ &= ~*w1++; *w2 &= ~*w1;
	    return vector2;
	default:
	    /* At least one word, do words and leftovers */
	    bytes &= sizeof(u_i4)-1;	/* Bytes left over */
 
	    do {
		*w2++ &= ~*w1++;
	    } while (--wnum != 0);
 
	    if (bytes != 0)
	    {
		b1 = (const char*)w1;   /* And this takes care of the bytes */
		b2 = (char*)w2;
 
		do {
		    *b2++ &= ~*b1++;
		} while (--bytes != 0);
	    }
	    return vector2;
	}
    } /* word-aligned pointers */

    /*
    ** No alignment, or short size, so degrade to simply a byte at a time.
    ** this is not expected to happen, but if it does, this code is the
    ** catch-all that will take care of it.  Dealing with unaligned
    ** addresses would have unnecessarily complicated the code.
    */

    b1 = vector1;
    b2 = vector2;
    if (bytes != 0)
    {
	do {
	    *b2++ &= ~*b1++;
	} while (--bytes != 0);
    }
    return vector2;
} 

/*{
** Name:	BTclearmask3 -- 'clearmask' two byte streams to distinct result
**
** Description:
**		Perform C 'clearmask' operation (&~) on byte streams
**
** Inputs:
**	size		number of bits in byte streams to 'clearmask'.
**			Rounded up to BITSPERBYTE.
**	vector1		input vector to 'clearmask' with vector2
**	vector2		'clearmask'ed with vector1
**
** Outputs:
**	result		result of 'clearmask'ing vector1 and vector2
**
**	Returns:
**			address of result
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**	16-Mar-2011 (kiria01)
**	    Created to avoid the frequent inevitable equivalent:
**		r = p2;
**		BTclearmask(sz,p1,r)
*/

void *
BTclearmask3(
    i4		size,
    const void	*vector1,
    const void	*vector2,
    void	*result)
{
    register const char *b1;
    register const char *b2;
    register char *br;
    register u_i4 bytes = BT_NUMBYTES_MACRO(size);

    /* Hand off to BTclearmask if result happens to one of vector1 or vector2 */
    if (vector2 == result)
	return BTclearmask(size, vector1, result);

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
		((SCALARP)vector1 & 7) == 0 &&
		((SCALARP)vector2 & 7) == 0 &&
		((SCALARP)result & 7) == 0)
    {
	register const u_i8 *l1 = vector1;
	register const u_i8 *l2 = vector2;
	register u_i8 *lr = result;
	register u_i4 lnum = bytes / sizeof(u_i8);	/* Nonzero */

	/* Ok, vectors are long-aligned.
	** Useful size special case is 16 bytes == 2 longs.
	** The 1024 common-case is 16 longs, maybe a little
	** too much for unrolling.  (** Untested assumption!) */
	switch (bytes)
	{
#ifdef BT_UNROLL
	case 16 * sizeof(u_i8):
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    /*FALLTHROUGH*/
	case 8 * sizeof(u_i8):
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    /*FALLTHROUGH*/
	case 4 * sizeof(u_i8):
	    *lr++ = (*l2++ & ~*l1++);
	    *lr++ = (*l2++ & ~*l1++);
	    /*FALLTHROUGH*/
#endif
	case 2 * sizeof(u_i8):
	    *lr++ = (*l2++ & ~*l1++);
	    *lr = (*l2 & ~*l1);
	    return result;
	default:
	    /* At least one long, do long and leftovers */
	    bytes &= sizeof(u_i8)-1;	/* Bytes left over */
 
	    do {
		*lr++ = (*l2++ & ~*l1++);
	    } while (--lnum != 0);
 
	    if (bytes != 0)
	    {
		b1 = (const char*)l1;   /* And this takes care of the bytes */
		b2 = (const char*)l2;
		br = (char*)lr;
		do {
		    *br++ = (*b2++ & ~*b1++);
		} while (--bytes != 0);
	    }
	    return result;
	}
    } /* end of long-aligned cases */
    else
#endif /* LP64 */
 
    /*
    ** test for word alignment, so that we may work with 32 bit values
    */
    if (bytes >= sizeof(u_i4) &&
	    ((SCALARP)vector1 & 3) == 0 &&
	    ((SCALARP)vector2 & 3) == 0 &&
	    ((SCALARP)result & 3) == 0)
    {
	register const u_i4 *w1 = vector1;
	register const u_i4 *w2 = vector2;
	register u_i4 *wr = result;
	register u_i4 wnum = bytes / sizeof(u_i4);	/* Nonzero */
	/* Ok, vectors are i4-aligned.
	** Useful size special case is 16 bytes == 4 words.
	** The 1024 common-case is 32 words, probably a little
	** too much for unrolling. */
	switch (bytes)
	{
#ifdef BT_UNROLL
	case 32 * sizeof(u_i4):
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    /*FALLTHROUGH*/
	case 16 * sizeof(u_i4):
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    /*FALLTHROUGH*/
	case 8 * sizeof(u_i4):
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    /*FALLTHROUGH*/
#endif
	case 4 * sizeof(u_i4):
	    *wr++ = (*w2++ & ~*w1++); *wr++ = (*w2++ & ~*w1++);
	    *wr++ = (*w2++ & ~*w1++); *wr = (*w2 & ~*w1);
	    return result;
	default:
	    /* At least one word, do words and leftovers */
	    bytes &= sizeof(u_i4)-1;	/* Bytes left over */
 
	    do {
		*wr++ = (*w2++ & ~*w1++);
	    } while (--wnum != 0);
 
	    if (bytes != 0)
	    {
		b1 = (const char*)w1;   /* And this takes care of the bytes */
		b2 = (const char*)w2;
		br = (char*)wr;
 
		do {
		    *br++ = (*b2++ & ~*b1++);
		} while (--bytes != 0);
	    }
	    return result;
	}
    } /* word-aligned pointers */

    /*
    ** No alignment, or short size, so degrade to simply a byte at a time.
    ** this is not expected to happen, but if it does, this code is the
    ** catch-all that will take care of it.  Dealing with unaligned
    ** addresses would have unnecessarily complicated the code.
    */
    b1 = vector1;
    b2 = vector2;
    br = result;
    if (bytes != 0)
    {
	do {
	    *br++ = (*b2++ & ~*b1++);
	} while (--bytes != 0);
    }
    return result;
}


