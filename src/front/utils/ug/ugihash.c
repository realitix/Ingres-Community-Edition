/*
**	Copyright (c) 2004 Actian Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<ex.h>
#include	<er.h>
#include	<gl.h>
#include	<sl.h>
#include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	"feloc.h"

/**
** Name:	ugihash.c -     Hash Table Manipulation Utility. 
**
** Description
**	These routines manage hash tables, allowing the caller to
**	enter / delete and find keyed data in the table.  Multiple
**	tables may be maintained.
**
**	Collisions are resolved through maintenance of linked lists
**	of table nodes.  This means that densities of >100% are
**	possible and no upper bound is enforced on the number of
**	table entries.
**
**	Caller specified key comparison routines and hash functions
**	are permitted, although defaults can be used.  The caller can
**	also override the default actions taken in the event of memory
**	allocation failure.
**
** Defines:
**	IIUGihiIntHtabInit()	initialize a hash table.
**	IIUGihfIntHtabFind()	find an entry.
**	IIUGihdIntHtabDel()		delete an entry.
**	IIUGiheIntHtabEnter()	enter an item.
**	IIUGihrIntHtabRel()		release a hash table from memory.
**	IIUGihsIntHtabScan()	scan through entries.
**
** History:
**	Revision 6.0  87/04  bobm
**		Initial revision.
**	17-jun-92 (leighb) DeskTop Porting Change:
**		Chgd "CONTEXT" to "IICONTEXT" to avoid name conflict in WIN/NT.
**	1-Mar-1993 (twai)
**		Change char *s in hash() to unsigned char *s to accept
**		double-byte characters column name. (Bug no. 49959)
**	16-mar-93 (fraser)
**		Added structure tag for IICONTEXT so that Microsoft prototyping
**		tool will work.
**	13-jul-93 (essi)
**		This file, ugihash.c, is a copy of ughash.c. The only 
**		difference is that in here we only work with integers as
**		keys. This make it easier for those who wish to use integer 
**		keys with simpe hash and comparison functions.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** hash table node structure
*/
typedef struct _htab
{
	struct _htab *next;	/* next node */
	i4 key;		/* key */
	char *data;		/* actual data */
} IHASHTAB;

/*
** context block structure for table
*/
typedef struct s_IICONTEXT
{
	IHASHTAB **tab;		/* actual table */
	FREELIST *pool;		/* list pool for node allocation */
	VOID (*afail)();	/* allocation failure routine */
	i4 (*compare)();	/* key comparison function */
	i4 (*hashfunc)();	/* hash function */
	IHASHTAB *sc;		/* current scan node for HtabScan */
	i4 scidx;		/* current scan index for HtabScan */
	i4 size;		/* size of hash table */
	TAGID tag;		/* storage tag for table array & context */
	i2 magic;
} IICONTEXT;			 

FREELIST *FElpcreate();
PTR FElpget();

#define HASHMAGIC 0x666
#define CHECKMAGIC(S,C) if ((C)->magic != HASHMAGIC) EXsignal(EXFEBUG, 1, (S))

static	i4	hash();
static	i4	icomp();

/*{
** Name:    IIUGihiIntHtabInit() -	initialize a hash table
**
** Description:
**	Initialize a hash table for use in subsequent hash table operations.
**	Multiple tables may be maintained.  Returns a pointer to be used as
**	an input parameter in the other hash table routines.  The caller
**	will simply maintain this pointer, not look at its contents.
**
** Inputs:
**	size	size of hash table to create.
**
**	allocfail	routine to call if memory allocation fails.  This
**			routine will probably either return, set an exception
**			or produce a fatal error.  If NULL, a routine
**			producing a fatal error will be provided.  If this
**			routine does not return, IIUGiheIntHtabEnter and this
**			routine cannot return failure.  Called with no
**			arguments.
**
**	comp	routine to compare table keys.  If NULL, == will be used.
**		Two keys are passed as the arguments. The first is the
**		argument passed to IIUGihfIntHtabFind or IIUGihrIntHtabRelease, 
**		the second is a key which has been added to the hash table by 
**		IIUGiheIntHtabEnter.  This routine should return 0 if they are 
**		equal, non-zero otherwise.  
**
**	hfunc	hash function.  If NULL, a "integer divide modulo table size"
**		function will be provided.  The hash function is called
**		with a key as the first argument, and a table size as the
**		second argument which may be ignored if knowledge of the
**		size is built into the function.  IMPORTANT NOTE:
**
**			If hfunc is NULL, IIUGihiIntHtabInit may adjust the
**			table size upwards a small amount to produce a
**			size which will work better with the default hash
**			function.  If hfunc is specified, the hash table
**			will be exactly the size asked for, and had better
**			be suitable for use with the provided function.
**
** Outputs:
**
**	tab	Context block pointer to use with other table manipulation
**		calls to use this particular table.
**
**     return:
**	OK
**	FAIL if allocfail can return
**
**
** Exceptions:
**	as initiated by allocfail().
**
** Side effects:
**	allocates memory.
**
** History:
**	4/87 (bobm)	written
**	7/6/87 (danielt) changed type of tag variable to TAGID
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	23-sep-93 (vijay) Remove STcompare defn since st.h is included.
**	28-sep-93 (vijay) Remove IIUGbmaBadMemoryAllocation defn too, include
**		ug.h.
*/

STATUS
IIUGihiIntHtabInit(size, allocfail, comp, hfunc, tab)
i4	size;
VOID	(*allocfail)();
i4	(*comp)();
i4	(*hfunc)();
PTR	*tab;
{
	register IICONTEXT *cb;		 
	register IHASHTAB **h;
	register i4  i;
	TAGID tag;

	if (allocfail == NULL)
		allocfail = IIUGbmaBadMemoryAllocation;

	if (comp == NULL)
		comp = icomp;

	/*
	** if users hash function is NULL adjust size upward to
	** next larger prime number - our hash function is a
	** modulus calculation
	*/
	if (hfunc == NULL)
	{
		size = IIUGnpNextPrime(size);
		hfunc = hash;
	}

	tag = FEbegintag();

	if ((*tab = FEreqmem(0,sizeof(IICONTEXT),FALSE,(STATUS*)NULL)) == NULL)		 
	{
		(*allocfail)(ERx("IIUGihiIntHtabInit"));
		return FAIL;
	}

	/* make sure this block has bad magic until everything works */
	cb = (IICONTEXT *) *tab;		 
	cb->magic = HASHMAGIC+1;

	if ((cb->tab = (IHASHTAB **)FEreqmem(0, (size * sizeof(IHASHTAB *)),
						FALSE, (STATUS *)NULL)) == NULL)
	{
		(*allocfail)(ERx("IIUGihiIntHtabInit"));
		return FAIL;
	}

	FEendtag();

	if ((cb->pool = FElpcreate((i2) sizeof(IHASHTAB))) == NULL)
	{
		(*allocfail)(ERx("IIUGihiIntHtabInit"));
		return FAIL;
	}

	/* set remaining context block pointers */
	cb->size = size;
	cb->afail = allocfail;
	cb->compare = comp;
	cb->hashfunc = hfunc;
	cb->tag = tag;
	cb->magic = HASHMAGIC;

	/* set scan index so "continue" will return "no more elements" */
	cb->scidx = size;

	/* initialize hash table */
	h = (IHASHTAB **) cb->tab;
	for (i=cb->size; i > 0; i--,h++)
		*h = NULL;

	return OK;
}

/*{
** Name:    IIUGihfIntHtabFind() -	Find an element in a hash table
**
** Description:
**	Finds an element in a hash table initialized through IIUGihiIntHtabInit
**
** Inputs:
**	tab	pointer returned from IIUGihiIntHtabInit
**
**	key	key to find.  Will return a node whose key COMPARES EQUAL
**		to this one, not a node whos key is the identical pointer.
**		(unless you made that your comparison function, of course)
**		If multiple entries equal to a given key have been made,
**		this returns the most recent entry.
**
** Outputs:
**
**	data	returned pointer to the data entered with the given key
**
**     return:
**	OK
**	FAIL, if no such entry.
**
** Side effects:
**	none, provided the comparison and hash functions don't generate any.
**
** History:
**	4/87 (bobm)	written
*/

STATUS
IIUGihfIntHtabFind(tab,key,data)
PTR tab;
i4	key;
PTR *data;
{
	register IHASHTAB *ptr;
	register IICONTEXT *cb;		 

	cb = (IICONTEXT *) tab;		 

	CHECKMAGIC(ERx("IIUGihfIntHtabFind"),cb);

	ptr = (cb->tab)[(*(cb->hashfunc))(key,cb->size)];
	for (; ptr != NULL; ptr = ptr->next)
	{
		if ((*(cb->compare))(key,ptr->key) == 0)
		{
			*data = ptr->data;
			return OK;
		}
	}

	return FAIL;
}

/*{
** Name:    IIUGihdIntHtabDel() -	Delete an element from a hash table
**
** Description:
**	Deletes an element from a hash table initialized through IIUGihiIntHtabInit
**
** Inputs:
**	tab	pointer returned from IIUGihiIntHtabInit
**
**	key	key to delete.  If multiple entries with identical keys have
**		been made, this deletes the most recent.
**
** Outputs:
**
**     return:
**	OK
**	FAIL, if no such entry.
**
** Side effects:
**	none, provided the comparison and hash functions don't generate any.
**
** History:
**	4/87 (bobm)	written
*/

STATUS
IIUGihdIntHtabDel(tab,key)
PTR tab;
i4	key;
{
	register IHASHTAB *ptr,*pred;
	register IICONTEXT *cb;		 
	i4 idx;

	cb = (IICONTEXT *) tab;		 

	CHECKMAGIC(ERx("IIUGihdIntHtabDel"),cb);

	pred = NULL;
	ptr = (cb->tab)[idx = (*(cb->hashfunc))(key,cb->size)];
	for ( ; ptr != NULL; ptr = ptr->next)
	{
		if ((*(cb->compare))(key,ptr->key) == 0)
			break;
		pred = ptr;
	}

	if (ptr == NULL)
		return FAIL;

	/*
	** found it: unlink from chain, and return to pool
	*/
	if (pred == NULL)
		(cb->tab)[idx] = ptr->next;	/* was head of list */
	else
		pred->next = ptr->next;
	FElpret(cb->pool,(i4 *)ptr);

	return OK;
}

/*{
** Name:    IIUGiheIntHtabEnter() -	Enter an element into a hash table
**
** Description:
**	Enters an element into a hash table initialized through IIUGihiIntHtabInit.
**	It is perfectly legal to enter multiple entries on one key.  If the
**	caller does not wish such a situation, IIUGihfIntHtabFind must be used
**	to check potential new entries.  This is the natural use, anyway.
**
** Inputs:
**	tab	pointer returned from IIUGihiIntHtabInit
**
**	key	key of element
**
**	data	data associated with key
**
**		IMPORTANT:
**
**		key and data must continue to point to legitimate memory
**		during use of table!  The table merely stores the pointers.
**
** Outputs:
**
**     return:
**	OK
**	FAIL if allocfail can return
**
** Exceptions:
**	As generated by allocfail() routine set in IIUGihiIntHtabInit
**
** Side effects:
**	Allocates memory.
**
** History:
**	4/87 (bobm)	written
*/

STATUS
IIUGiheIntHtabEnter(tab,key,data)
PTR tab;
i4	key;
PTR data;
{
	register IICONTEXT *cb;		 
	register IHASHTAB *ptr;
	register i4  i;

	cb = (IICONTEXT *) tab;		 

	CHECKMAGIC(ERx("IIUGiheIntHtabEnter"),cb);

	if ((ptr = (IHASHTAB *) FElpget(cb->pool)) == NULL)
	{
		(*cb->afail)(ERx("IIUGiheIntHtabEnter"));
		return FAIL;
	}

	ptr->next = (cb->tab)[i = (*(cb->hashfunc))(key,cb->size)];
	(cb->tab)[i] = ptr;
	ptr->key = key;
	ptr->data = data;

	return OK;
}

/*{
** Name:    IIUGihrIntHtabRelease() -	Release hash table memory.
**
** Description:
**	Releases memory for a hash table initialized through IIUGihiIntHtabInit.
**	Following this call, that table can no longer be used.
**
** Inputs:
**	tab	pointer returned from IIUGihiIntHtabInit
**
** Outputs:
**	none
**
** Side effects:
**	Frees memory.
**
** History:
**	4/87 (bobm)	written
*/

VOID
IIUGihrIntHtabRelease(tab)
PTR tab;
{
	register IICONTEXT *cb;		 

	cb = (IICONTEXT *) tab;		 

	CHECKMAGIC(ERx("IIUGihrIntHtabRelease"),cb);

	--(cb->magic);	/* destroy magic number - prevent use of freed table */

	FElpdestroy(cb->pool);
	FEfree(cb->tag);
}

/*{
** Name:    IIUGihsIntHtabScan() -	scan hash table
**
** Description:
**	Performs a linear scan through the hash table entries.  The order
**	they will be returned in is dependent on where they were hashed
**	to, of course, so this routine is simply to allow the caller to
**	scan through all current table entries in situations where the
**	scan order doesn't matter.  The return code is designed to allow
**	a caller to study a sample hash table distribution without paying
**	a penalty for callers that don't want the information.  You can
**	simply test for positive return value.
**
** Inputs:
**	tab	pointer returned from IIUGihiIntHtabInit
**	cont	TRUE to continue previous scan, false to start over.
**
** Outputs:
**	key	key of entry
**	dat	data associated with key
**
**	returns:
**		0 if no more entries
**		otherwise, the cell (1 to (real) table size), that the
**		entry was hashed to.
**
** Side effects:
**	none
**
** History:
**	4/87 (bobm)	written
*/

i4
IIUGihsIntHtabScan(tab,cont,key,dat)
PTR tab;
bool cont;
i4	*key;
PTR *dat;
{
	register IICONTEXT *cb;		 

	cb = (IICONTEXT *) tab;		 

	CHECKMAGIC(ERx("IIUGihsIntHtabScan"),cb);

	if (!cont)
	{
		cb->scidx = 0;
		cb->sc = (cb->tab)[0];
	}

	while (cb->scidx < cb->size)
	{
		if (cb->sc != NULL)
		{
			*key = (cb->sc)->key;
			*dat = (cb->sc)->data;
			cb->sc = (cb->sc)->next;
			return cb->scidx + 1;
		}
		if ((++(cb->scidx)) < cb->size)
			cb->sc = (cb->tab)[cb->scidx];
	}

	return 0;
}

/*{
** Name:        hash        hash function.
**
** Description:
**	Simple hash function for hashing integers.
**
** Inputs:
**      key     {i4}   key to hash.
**
** Returns:
**      a hash value.
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
static i4
hash(key,size)
i4 key;
i4  size;
{
	return( (u_i4)key % size );
}

/*{
** Name:        icomp        comparison function.
**
** Description:
**	Simple integer comparion function.
**
** Inputs:
**      key1    {i4}       first integer.
**      key2    {i4}       second integer.
**
** Returns:
**      0 for success and -1 for failure.
**
** History:
**      19-apr-93 (essi)
**              Written.
*/
static
i4
icomp(key1, key2)
i4	key1;
i4	key2;
{
	if ( key1 == key2 )
		return ( 0 );
	return( -1 );
}
