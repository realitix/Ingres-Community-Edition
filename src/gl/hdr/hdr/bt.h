/*
** Copyright (c) 1985, 2008, 2011 Actian Corporation
*/
# ifndef BT_HDR_INCLUDED
# define BT_HDR_INCLUDED

/**CL_SPEC
** Name: BT.H - Global definitions for the BT compatibility library.
**
** Specification:
**
** Description:
**      The file contains the type used by BT and the definition of the
**      BT functions.  These are used for bit array manipulation.
**
** History:
**      01-oct-1985 (jennifer)
**          Updated to coding standard for 5.0.
**      13-dec-1988 (seputis)
**          made compatible with CL spec used i4  instead of i4
**	6-jan-92 (seputis)
**	    added BTxor
**	22-aug-92 (seputis)
**	    moved to GL, changed to use prototypes, and use ii prefix
**	    Removed BT_OK reference
**	21-apr-95 (tutto01)
**	    Added INTSIZE 4 to take advantage of 32 bit systems.
**	30-sep-1996 (canor01)
**	    Protect against multiple inclusion of bt.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	22-oct-2004 (gupsh01)
**	    Added BT_SWAP_BYTES_MACRO and 
**	    BT_BYTE_NUMS_MACRO.
**    06-apr-2005 (gupsh01)
**	    Fixed BT_SWAP_BYTES_MACRO, to correctly identify
**	    swapping mechanism for boundary line values.
**      12-dec-2008 (joea)
**          Replace BYTE_SWAP by BIG/LITTLE_ENDIAN_INT.
**	6-Jan-2009 (kibro01)
**	    Add BTclearmask
**   26-feb-2009 (Ralph Loen) Bug 121726)
**          Revised BT_BYTES_NUM_MACRO to account for bytes with
**          maximum values (0xFF).
**	20-Jun-2009 (kschendel) SIR 122138
**	    Remove INTSIZE, not used in bt and doesn't belong here anyway.
**	16-Mar-2011 (kiria01)
**	    Added BTallones, BTallzeros, BTintersect, BTclearmask3, BTand3,
**	    BTor3, BTxor3 and BTnot2 and cleaned up the parameters to use void*
**	    and const void* for readonly parameters.
**/

/*
**  Forward and/or External function references.
*/


/* use defines to force inclusion of BT.H and to reduce name space
** pollution */
#define		BTallones	iiBTallones
#define		BTallzeros	iiBTallzeros
#define		BTand		iiBTand
#define		BTand3		iiBTand3
#define		BTclear		iiBTclear
#define		BTclearmask	iiBTclearmask
#define		BTclearmask3	iiBTclearmask3
#define		BTcount		iiBTcount
#define		BThigh		iiBThigh
#define		BTintersects	iiBTintersects
#define		BTnext		iiBTnext
#define		BTnot		iiBTnot
#define		BTnot2		iiBTnot2
#define		BTor		iiBTor
#define		BTor3		iiBTor3
#define		BTset		iiBTset
#define		BTsubset	iiBTsubset
#define		BTtest		iiBTtest
#define		BTxor		iiBTxor
#define		BTxor3		iiBTxor3

/* Name: BT_SWAP_BYTES_MACRO - swaps the bytes on a non byte swapped
**
** Description -
**
**      This MACRO is used on a little endian machine, to obtain an
**      equivalent byte swapped value, which has sequence of bytes
**      from the highers order to the lowest order.
*/
#ifdef LITTLE_ENDIAN_INT
#define BT_SWAP_BYTES_MACRO(i) ( (i <= 0xFF) ? (i) : ( (i <= 0xFFFF) \
  ? (((i & 0xFFFF) >> 8) | ((i & 0xFF) << 8)) \
    : (( i <= 0xFFFFFF ) \
    ? (((i & 0xFFFFFF) >> 16) | ((i & 0xFFFF) << 16)) \
      : (((i & 0xFFFFFFFF) >> 16) | ((i & 0xFFFF) << 16)))))
#else
#define BT_SWAP_BYTES_MACRO(i) (i)
#endif

#define BT_BYTE_NUMS_MACRO(i) ((i <= 0xFF) \
        ? (1) : ((i <= 0xFFFF) ? (2) \
                  : ((i <= 0xFFFFFF) ? (3):(4))))

/*
** Single bit operations:
**	These have identical parameters and ASSUME that the
**	'pos' requested will be within the size (implied).
**	In common with other modifier operations BTclear and BTset return
**	the address of the updated bitvector.
**
**  v = BTclear(pos, v);			v{pos} = 0
**  v = BTset(pos, v);				v{pos} = 1
**  bool BTtest(pos, v);			v{pos}
**
** Bitwise modifier operations:
**	These all return the address of the modified bitvector and have
**	size in bits (assumed a multiple of bits per byte) as first parameter.
**
**  v = BTnot(size, v);				v = ~v;
**  v2 = BTand(size, v1, v2);			v2 &= v1
**  v2 = BTor(size, v1, v2);			v2 |= v1;
**  v2 = BTxor(size, v1, v2);			v2 ^= v1;
**  v2 = BTclearmask(size, v1, v2);		v2 &= ~v1;
**
** Bitwise operators:
**	These all return the address of the result bitvector (the last
**	parameter) and have size in bits (assumed a multiple of bits per byte)
**	as first parameter.
**
**  result = BTnot2(size, v, result);		result = ~v;
**  result = BTand3(size, v1, v2, result);	result = v2 & v1
**  result = BTor3(size, v1, v2, result);	result = v2 | v1;
**  result = BTxor3(size, v1, v2, result);	result = v2 ^ v1;
**  result = BTclearmask3(size, v1, v2, result);result = v2 & ~v1;
**
** Testing & enquiry operations:
**	These all specify the size in bits (assumed a multiple of bits per byte)
**	as the last parameter.
**
**  bool BTallones(v, size);			vector == 0
**  bool BTallzeros(v, size);			vector == ~0
**  bool BTintersects(p1, p2, size);		p1 intersects p2
**  bool BTsubset(sub, set, size);		sub contained-in set
**  n = BTcount(v, size);			number of bits set
**  n = BThigh(v, size);			pos of highest set bit
**  n = BTnext(start, v, size);			next set after start
*/

/*
** MACRO: BT_FNS_DECL
**	Defines typesafe wrapper functions for the BT facility.
**
**	Ideally these should be inline functions & BT_FNS_DECL referenced in
**	facility headers
*/
#define BT_FNS_DECL(_pfx,_typ) \
static _typ *_pfx##clear(i4 pos, _typ *v){return BTclear(pos, v);}\
static _typ *_pfx##set(i4 pos, _typ *v){return BTset(pos, v);}\
static bool  _pfx##test(i4 pos, _typ *v){return BTtest(pos, v);}\
static _typ *_pfx##not(_typ *v){return BTnot(BITS_IN(*v),v);}\
static _typ *_pfx##and(const _typ *v1, _typ *v2){return BTand(BITS_IN(*v1),v1,v2);}\
static _typ *_pfx##or(const _typ *v1, _typ *v2){return BTor(BITS_IN(*v1),v1,v2);}\
static _typ *_pfx##xor(const _typ *v1, _typ *v2){return BTxor(BITS_IN(*v1),v1,v2);}\
static _typ *_pfx##clearmask(const _typ *v1, _typ *v2){return BTclearmask(BITS_IN(*v1),v1,v2);}\
static _typ *_pfx##not2(const _typ *v, _typ *r){return BTnot2(BITS_IN(*v),v,r);}\
static _typ *_pfx##and3(const _typ *v1, const _typ *v2, _typ *r){return BTand3(BITS_IN(*v1),v1,v2,r);}\
static _typ *_pfx##or3(const _typ *v1, const _typ *v2, _typ *r){return BTor3(BITS_IN(*v1),v1,v2,r);}\
static _typ *_pfx##xor3(const _typ *v1, const _typ *v2, _typ *r){return BTxor3(BITS_IN(*v1),v1,v2,r);}\
static _typ *_pfx##clearmask3(const _typ *v1, const _typ *v2, _typ *r){return BTclearmask3(BITS_IN(*v1),v1,v2,r);}\
static bool _pfx##allones(const _typ *v){return BTallones(v, BITS_IN(*v));}\
static bool _pfx##allzeros(const _typ *v){return BTallzeros(v, BITS_IN(*v));}\
static bool _pfx##intersects(const _typ *v1, const _typ *v2){return BTintersects(v1,v2,BITS_IN(*v1));}\
static bool _pfx##subset(const _typ *v1, const _typ *v2){return BTsubset(v1,v2,BITS_IN(*v1));}\
static i4 _pfx##count(const _typ *v){return BTcount(v, BITS_IN(*v));}\
static i4 _pfx##high(const _typ *v){return BThigh(v, BITS_IN(*v));}\
static i4 _pfx##next(i4 start, const _typ *v){return BTnext(start, v, BITS_IN(*v));}\
static bool _pfx##equal(const _typ *v1, const _typ *v2){return 0==memcmp(v1,v2,sizeof(*v1));}\
/*END of BT_FNS_DECL*/

FUNC_EXTERN bool BTallones(
	const void	*vector,
	i4		size);

FUNC_EXTERN bool BTallzeros(
	const void	*vector,
	i4		size);

FUNC_EXTERN void *BTand(
	i4		size,
	const void	*vector1,
	void		*vector2);

FUNC_EXTERN void *BTand3(
	i4		size,
	const void	*vector1,
	const void	*vector2,
	void		*result);

FUNC_EXTERN void *BTclear(
	i4		pos,
	void		*vector);

FUNC_EXTERN void *BTclearmask(
	i4		size,
	const void	*vector1,
	void		*vector2);

FUNC_EXTERN void *BTclearmask3(
	i4		size,
	const void	*vector1,
	const void	*vector2,
	void		*result);

FUNC_EXTERN i4  BTcount(
	const void	*vector,
	i4		size);

FUNC_EXTERN i4  BThigh(
	const void	*vector,
	i4		size);

FUNC_EXTERN bool BTintersects(
	const void	*p1,
	const void	*p2,
	i4		size);

FUNC_EXTERN i4  BTnext(
	i4		n,
	const void	*vector,
	i4		size);

FUNC_EXTERN void *BTnot(
	i4		size,
	void		*vector);

FUNC_EXTERN void *BTnot2(
	i4		size,
	const void	*vector,
	void		*result);

FUNC_EXTERN void *BTor(
	i4		size,
	const void	*vector1,
	void		*vector2);
FUNC_EXTERN void *BTor3(
	i4		size,
	const void	*vector1,
	const void	*vector2,
	void		*result);

FUNC_EXTERN void *BTset(
	i4		pos,
	void		*vector);

FUNC_EXTERN bool BTsubset(
	const void	*sub,
	const void	*set,
	i4		size);

FUNC_EXTERN bool BTtest(
	i4		pos,
	const void	*vector);

FUNC_EXTERN void *BTxor(
	i4		size,
	const void	*vector1,
	void		*vector2);

FUNC_EXTERN void *BTxor3(
	i4		size,
	const void	*vector1,
	const void	*vector2,
	void		*result);

# endif /* BT_HDR_INCLUDED */

