/*
**    Copyright (c) 1986, 2008 Actian Corporation
*/

#include	<compat.h>
#include	<gl.h>
#include        <iicommon.h>
#include	<cv.h>
#include	<me.h>
#include	<st.h>
#include	<lo.h>
#include	<nm.h>
#include	<er.h>
#include	<cm.h>
#include	<cs.h>	    /* Needed for "erloc.h" */
#include	"erloc.h"
#include	<descrip.h>
#include	<secdef.h>
#include	<va_rangedef.h>
#include	<starlet.h>

/**
** Name:    ERinternal.c- The group of internal used routines in ER.
**
** Description:
**      This is a group of internal used routines in ER compatibility library.
**	If you use these routine in your program, you must be included
**	'erloc.h' header file.
**
**	cer_get         Set string corresponding to ER_MSGID to buffer.
**	cer_fndindex	find the index of ERmulti table
**	cer_nxtindex	find next available index of ERmulti table
**	cer_init	initialize message system
**	cer_finit	initialize fast message system
**	cer_sinit	initialize slow message system
**	cer_fstr	search fast message and return the message pointer
**	cer_read	set up message number table and message area
**	cer_isopen	check if index file opened or not.
** 	cer_sstr	search slow message and set the message to buffer.
**	cer_tclose	close a file if running test mode.
**	cer_tfile	get a file name for messages.
**
** History:    
**      01-Oct-1986 (kobayashi) - first written
**	25-jun-1987 (peter)
**		Add support for test mode.
**	18-aug-1987 (peter)
**		Add support for II_MSGDIR
**	02-nov-1988 (greg)
**	    If you don't like to AV, you must check the return status
**	    of NMloc, LOfaddpath, etc.
**	22-feb-1989 (greg)
**	    CL_ERR_DESC member error must be initialized to OK
**	07-mar-1989 (Mike S)
**	    Map fast message file into memory.
**	01-may-89 (jrb)
**	    Change interface for generic error return and make changes for new
**	    SLOW.MNX format.
**      25-mar-91 (stevet)
**          Added korean, chinese, thai and greek to Language table.
**      25-mar-91 (stevet)
**          Backed out of the Language Table change to investigate if other
**          changes are needed to support this correctly.
**    	4/91 (Mike S)
**          Construct file names for unbunbled products
**	6-jun-91 (stevet)
**	    Added korean, chinese, thai and greek to Language table.
**      16-jul-93 (ed)
**	    added gl.h
**	19-jul-2000 (kinte01)
**	    Correct prototype definitions by updating calls to NMgetpart
**	    as it should have an additional parameter
**	19-jul-2000 (kinte01)
**	   Add missing external function references
**	01-dec-2000	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	07-feb-01 (kinte01)
**	    Add casts to quiet compiler warnings
**	16-Oct-2001 (kinte01)
**	   Add prototype for cer_istest
**	26-oct-2001 (kinte01)
**	   clean up compiler warnings
**	02-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	13-nov-2008 (joea)
**	    Use CL_CLEAR_ERR to initialize CL_ERR_DESC.  Replace uses of
**	    CL_SYS_ERR by CL_ERR_DESC.  Remove non-VMS code.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
*/


/*
**	The STACKSIZE can be used to allow the slow messages used by
**	ERget to keep a list of the last 'STACKSIZE' messages before
**	overwriting the older ones.  This was put in to allow some
**	caching of slow messages, but the CL committee determined
**	that it was conducive to bad coding habits, and suggested
**	that it be changed to 1.
*/

STATUS cer_getdata();
FUNC_EXTERN bool cer_istest();

#define	STACKSIZE   1
#define MAX_PARTNAME 8 /* Largest part name allowed */

/*
**  This array of structer includes internal language code, class address, both
**  fast and slow of internal file id and internal stream id for each language.
**  Please show erloc.h, if you would like to know about ERMULTI structer.
*/
GLOBALDEF ERMULTI	ERmulti[ER_MAXLANGUAGE] = {{0,0,0,0,{{0,0},{0,0}}}};

/*
**	The number of longnat indices that can fit on each page following
**	the leading index page.
*/
#define PERBLOCK	(sizeof(INDEX_PAGE)/sizeof(i4))

#define SRECFACTOR	(sizeof(INDEX_PAGE)/512)


/*
**  Language table
**
**  This table is used to get directory name and internal code.
**  (in ERlangcode and ERlangstr)
*/
GLOBALDEF ER_LANGT_DEF	ER_langt[] = {
	ERx("english"),  	1   ,
	ERx("japanese"), 	2   ,
	ERx("french"),   	3   ,
	ERx("flemish"),  	4   ,
	ERx("german"),   	5   ,
	ERx("italian"),  	6   ,
	ERx("dutch"),    	7   ,
	ERx("finnish"),  	8   ,
	ERx("danish"),   	9   ,
	ERx("spanish"),  	10  ,
	ERx("swedish"),  	11  ,
	ERx("norwegian"),	12  ,
	ERx("hebrew"),		13  ,
	ERx("korean"),		14  ,
	ERx("chinese"),		15  ,
	ERx("thai"),		16  ,
	ERx("greek"),		17  ,
	NULL,0
};


/*{
** Name:	cer_get    - set string corresponding to ER_MSGID to buffer
**
** Description: 
**	This procedure searches the message files (either fast or slow)
**	for the message corresponding to the ER_MSGID 'id' and copies up
**	to 'bufsize' characters in the string into buffer 'buf.'  
**	The string requested is for the language code represented
**	by 'language'.  This call can be used in the server environment,
**	by using the language from the session control block.
**
**	If the 'id' message cannot be found, a string is returned which
**	indicates an error condition.
**
** Inputs:
**	ER_MSGID id		key of the message
**	char **buf		pointer of buffer pointer to set message
**	i4 bufsize		max size of buffer
**	i4 language		language id of the message
**
** Outputs:
**	Returns:
**	STATUS	OK			scucess
**
**		ER_DIRERR		if something cannot do LO call.
**		ER_NO_FILE		if something cannot do open on file.
**		ER_BADREAD		if something cannot do read on file.
**		ER_NOALLOC		if something cannot do allocate.
**		ER_MSGOVER		if message table is over flow.
**		ER_BADLANGUAGE		if language is bad.
**		ER_TOOSMALL		if buffer size is too small.
**			
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**	01-Oct-1986 (kobayashi)
**	    first written
**	01-may-1989 (jrb)
**	    Changed interface on cer_sstr call to get slow message text.  Also,
**	    cleaned this routine up a bit (what a mess!!).
*/
STATUS
cer_get(id, buf, bufsize, langcode)
ER_MSGID    id;
char	    **buf;
i4	    bufsize;
i4	    langcode;
{
    i4		fors;		        /* flag to show fast or slow */
    ER_CLASS	class_no;		/* class number */
    i4		mess_no;		/* message number */
    i4		erindex;		/* index of ERmulti table */
    STATUS	status;
    CL_ERR_DESC  err_code;		/* error structer of system call */
    static char msgbuf[ER_MAX_LEN + 1];

    /* Only use slow message */

    i4	    i;
    static  i4	    nextp = 0;
    static  struct SLOWSTACK {	/* This size depend on STACKSIZE */
	ER_MSGID    msgid;
	char	    *msgp;
    }  slowstack[] = {
	0,  NULL,
	0,  NULL,
	0,  NULL,
	0,  NULL,
	0,  NULL
    };

    /*
    **	get information about class number, message number and
    ** 	if fast or slow.
    */

    fors = ((id & ER_MSGIDMASK1) >> ER_MSGHISHIFT) & ER_MSGIDMASK4;
    class_no = ((id & ER_MSGIDMASK2) >> ER_MSGMIDSHIFT) & ER_MSGIDMASK5;
    mess_no = id & ER_MSGIDMASK3;

    /*
    **	search index of ERmulti that ERmulti[erindex].language 
    **	is parameter 'language' and file already opened.
    **	If it is not found, return '-1'.
    */

    if ((erindex = cer_fndindex(langcode)) == -1)
    {
	if ((status = cer_nxtindex(langcode,&erindex)) != OK)
	{   /* Error in initializing the language */
	    return(status);
	}
    }

    /*	
    **	initialize message system on language
    */
    if (!cer_isopen(erindex,fors))
    {	/* File is not open.  Open it */
    	if (fors == ER_FASTSIDE)
    	{
            /* initialize fast message system */
            if ((status = cer_finit(langcode,id,erindex,&err_code)) != OK)
	    {
	    	return(status);
	    }
        }
        else
        {
            /* initialize slow message system */

            if ((status = cer_sinit(langcode,id,erindex,&err_code)) != OK)
            {
    	        return(status);
	    }
        }
    }

    if (fors == ER_FASTSIDE)
    {

	/*
	** fast message process
	**    Search message string and return pointer message
	**    string to buffer
	*/

	*buf = cer_fstr(class_no,mess_no,erindex);

	/*
	**  If null is returned, consider that there are not this
	**  message number.
	*/
	
	if (*buf == NULL)
	{
	    STprintf(msgbuf,ERx("*** BAD MESSAGE NO. CLASS NO =0x%x, MESSAGE NO=0x%x ***"),class_no,mess_no);
	    *buf = STalloc(msgbuf);
	}
	return(OK);
    }
    else
    {
	if ((status =
	       cer_sstr(id, NULL, msgbuf, bufsize, erindex, &err_code, ER_GET))
	    != OK)
	{
	    if (status == ER_NOT_FOUND)
	    {
		/* not found message */
		STprintf(msgbuf,ERx("*** MESSAGE NOT FOUND id = %x ***"),id);
	    }
	    else
	    {
		return(status);
	    }
	}
	if (slowstack[nextp].msgp != NULL)
	    MEfree(slowstack[nextp].msgp);
	slowstack[nextp].msgp = STalloc(msgbuf);
	*buf = slowstack[nextp].msgp;
	slowstack[nextp++].msgid = id;
	nextp %= STACKSIZE;
	return(OK);
    }
}


/*{
** Name:	cer_fndindex	- search the index of ERmulti table
**
** Description:
**	This procedure searches the index of ERmulti table for the 'language'
**	specified to return the ERmulti index.  If the index is not found, 
**	return (-1).
**
**	If parameter 'language' is -1, this is considered default language.
**
** Inputs:
**	i4 language		language to search
**	i4 fors		flag of fast or slow
**
** Outputs:
**	none
**	Returns:
**		i4 index	the index of ERmulti table
**		-1		the index not found
**	Exceptions:
**		none
**
** Side Effects:
**	none
**
** History:
**	01-Oct-1986 (kobayashi) - first written
*/
i4
cer_fndindex(langcode,fors)
i4 langcode;
i4 fors;
{
    i4 i;

    /* 
    **	If language is '-1', it searches for the default language.
    */

    if (langcode == -1)	/* default language */
    {
    	for (i = 0; i < ER_MAXLANGUAGE; ++i)
	{
	    if (ERmulti[i].language == 0)
	    {
	        continue;
	    }
	    if (ERmulti[i].deflang == 1)
	    {
		/*
		**  Default language is set deflang in
		**  ERmulti table.
		** This i is index of ERmulti table that deflang is ON 
		*/

		return(i);
	    }
	}
    }
    else	/* It is't default language */
    {
	for (i = 0; i < ER_MAXLANGUAGE; ++i)
	{
	    if (ERmulti[i].language == 0)
	    {
		/*
		** If ERmulti[i].language is 0,the element is not used
		** used. 
		*/
		continue;
	    }
	    if (ERmulti[i].language == langcode) 
	    {
		return(i);
	    }
	}
    }
    /*	No matching language found. */
    return(-1);
}


/*{
** Name:	cer_nxtindex	- search for the next available index of 
**				    ERmulti table
**
** Description:
**	This procedure gets the next available index of ERmulti 
**	table to use for the 'language' specified in the call.
**	Since the ERmulti table allows up to ER_MAXLANGUAGE languages to
**	be used at the same time, this will start up a new language
**	entry in the table.  It fills in the corresponding fields
**	by getting various settings, if needed.
**
**	If the langcode passed in is -1, then the II_LANGUAGE name
**	will be looked up.
**
** Inputs:
**	i4	langcode    language code to search
**
** Outputs:
**	none
**	Returns:
**		i4 index	the next index of ERmulti table
**		-1		the available index not found (table over flow)
**	Exceptions:
**		none
**
** Side Effects:
**	none
**
** History:
**	01-Oct-1986 (kobayashi) - first written
**	03-jul-1987 (peter)
**		Rewritten.
*/
STATUS
cer_nxtindex(langcode,index)
i4 langcode;
i4 *index;
{
    i4 i;
    char nm[ER_MAX_LANGSTR];
    STATUS status;

    for (i = 0; i < ER_MAXLANGUAGE; ++i)
    {
	*index = i;
	if (ERmulti[i].language != 0)
	{	/* Language already being used.  Try the next one. */
	    continue;
	}

	/* Language slot is available */

        if (langcode == -1)	/* default language */
        {
	    /* Check default language */

	    if ((status = ERlangcode((char *)NULL,&langcode)) != OK)
	    {
	        return(status);
	    }
	    ERmulti[i].language = langcode;

	    /* set deflang flag in ERmulti to show default  language */

	    ERmulti[i].deflang = 1;

	    /* set pointer language string */

	    if ((status = ERlangstr(langcode,nm)) != OK)
	    {   /* Bad language code */
	        ERmulti[i].language = 0;
	        ERmulti[i].deflang = 0;
	        return(status);
	    }
        }
        else
        {
	    ERmulti[i].language = langcode;

	    /* set pointer language string */

	    if ((status = ERlangstr(langcode,nm)) != OK)
	    {
	        ERmulti[i].language = 0;
	        return(status);
	    }
        }
    
	return(OK);
    }

    *index = -1;
    return(ER_MSGOVER);
}


/*{
** Name:	cer_finit	- initialize fast message system
**
** Description:
**	This procedure initializes the fast message system. First,
**	do ready to open file and the file is opened. 
**	Next, it reads control record to get the
**	information about size of class table that allocates in memory.
**	Then it allocates class table in memory and clears out in table to
**	set null.
**
**	When testing, this procedure opens up the test files, but 
**	does not clear out the memory previously in place (as some
**	of the classes are already loaded.)
**	If testing, this procedure opens up a file of the form 'sxxx.mnx'
**	where xxx is the class number, in decimal.  If that file does not
**	exist, then the general 'slow.mnx' file is used.
**
**
** Inputs:
**	i4 langcode			internal code for language
**	ER_MSGID			message id
**	i4 *erindex			index of ERmulti table
**
** Outputs:
**	CL_ERR_DESC *err_code		if error happen, code is set to this.
**	Returns:
**		STATUS	OK   if everything worked
**
**			ER_NO_FILE
**			ER_BADOPEN
**			ER_BADREAD
**			ER_NOALLOC
**			ER_MSGOVER
**			ER_BADLANGUAGE
**			ER_DIRERR
**
**	Exceptions:
**		none
**
** Side Effects:
**		none
** History:
**	01-Oct-1986 (kobayashi) - first written
**	29-jun-1987 (peter)
**		Add support for test files.
**	07-mar-89 (Mike S)
**		Try to map default language's file
**	29-Apr-89 (Mike S)
**		Don't map section if II_MSGDIR is defined
**	4/91 (Mike S)
**		Don't map section for unbundled products
*/
STATUS
cer_finit ( langcode, msgid, erindex, err_code )
i4 langcode;
ER_MSGID	msgid;
i4 erindex;
CL_ERR_DESC *err_code;
{
    ER_CLASS	classsize;
    ER_CLASS_TABLE	*erclass;
    STATUS status;
    LOCATION	loc;
    char	loc_string[MAX_LOC];
    struct dsc$descriptor section_desc;
    char	fbuf[MAX_LOC];
    VA_RANGE	inadr;
    VA_RANGE	retadr;
    char 	*msgloc;
    bool	no_msgloc;
    char        p[MAX_PARTNAME + 1];

    CL_CLEAR_ERR(err_code);

    /* 
    ** Look for fast message section, if we're not testing, and II_MSGDIR
    ** isn't pointing off anywhere, and we're not an unbundled product.
    */
    NMgtAt( ERx("II_MSGDIR"), &msgloc);
    no_msgloc = (msgloc == NULL) || (*msgloc == EOS);
    if ( !cer_istest() && no_msgloc && NMgetpart(p) == NULL)
    {
    	/*
	**	Form section name by concatenating ER_FASTSECT with the
	**	language name.
	*/  
	STcopy(ER_FASTSECT, loc_string);
    	status = ERlangstr(ERmulti[erindex].language, 
			   loc_string+STlength(loc_string));
	CVupper(loc_string);
	if (status) return status;
	inadr.va_range$ps_start_va = inadr.va_range$ps_end_va = 0;
	section_desc.dsc$w_length = STlength(loc_string);
	section_desc.dsc$a_pointer = loc_string;
    	status = sys$mgblsc(&inadr, &retadr, 0, SEC$M_EXPREG|SEC$M_SYSGBL,
    			    &section_desc, 0, 0);

    	if ( (status & 1) != 0)
	{
	    classsize = *((ER_CLASS *)retadr.va_range$ps_start_va);
  	    ERmulti[erindex].class = (ER_CLASS_TABLE*)
		MEreqmem(0,classsize*sizeof(ER_CLASS_TABLE), TRUE, NULL);
    	    if (ERmulti[erindex].class == NULL)
    	    {
        	/* fail of alloc */
		return(ER_NOALLOC);
    	    }
	    ERmulti[erindex].ERfile[ER_FASTSIDE].mapped = TRUE;
	    ERmulti[erindex].ERfile[ER_FASTSIDE].map_addr =
						retadr.va_range$ps_start_va;
    	    ERmulti[erindex].number_class = classsize;
	    return(OK);
	}
    }

    /* If testing, and the class table is already loaded, return */
    if ( cer_istest() && (erclass = ERmulti[erindex].class) != NULL )
    {	/* Testing and the files have been loaded. */
	ER_CLASS	class_no;

    	class_no = ((msgid & ER_MSGIDMASK2) >> ER_MSGMIDSHIFT) & ER_MSGIDMASK5;
	if (erclass[class_no].message_pointer != NULL)
	{	/* Already loaded. */
	    return(OK);
	}
    }

    /*
    ** Decide location
    */

    if ((status = cer_init(langcode,erindex,&loc,loc_string,ER_FASTSIDE)) != OK)
    {
	return(status);
    }

    /* get fast file name */

    LOfstfile(cer_tfile(msgid,ER_FASTSIDE,fbuf,cer_istest()),&loc);
    if (LOexist(&loc) != OK)
    {
	if (cer_istest())
	{	/* If testing, try the main file */
    	    LOfstfile(cer_tfile(msgid,ER_FASTSIDE,fbuf,FALSE),&loc);
            if (LOexist(&loc) != OK)
            {	/* Even the other file does not exist */
	        return(ER_NO_FILE);
            }
	}
	else
	{
	    return(ER_NO_FILE);
	}
    }

    /*
    **	This process opens message file
    */

    if ((status = cer_open(&loc,&(ERmulti[erindex].ERfile[ER_FASTSIDE]),
	err_code)) != OK)
    {
	return(status);
    }
	
    /*
    **	read control record and get the information
    **	about size of class table
    */

    if ((status = cer_getdata(&classsize,sizeof(classsize),1,
	&(ERmulti[erindex].ERfile[ER_FASTSIDE]),err_code)) != OK)
    {
	cer_close(&(ERmulti[erindex].ERfile[ER_FASTSIDE]));
	return(status);
    }

    if (cer_istest())
    {	/* In tests, allocate the class table the first time only. */
	if (ERmulti[erindex].class != NULL)
	{	/* The class table has already been initialized */
	    return(OK);
	}
	/* The class table has never been initialized */
	classsize = CLASS_SIZE;		/* Use maximum possible */
    }

    /* 
    **	allocate class table in memory and initalize
    **	to zero (null).
    */

    ERmulti[erindex].class = (ER_CLASS_TABLE*)
		MEreqmem(0,classsize*sizeof(ER_CLASS_TABLE), TRUE, NULL);
    if (ERmulti[erindex].class == NULL)
    {
        /* fail of alloc */
	cer_close(&(ERmulti[erindex].ERfile[ER_FASTSIDE]));
	ERmulti[erindex].class = NULL;
	return(ER_NOALLOC);
    }

    /* 
    **	set classsize to number_class of each language of ERmulti 
    */

    ERmulti[erindex].number_class = classsize;

    return(OK);
}


/*{
** Name:	cer_sinit	- initialize slow message system
**
** Description:
**	This procedure initializes slow message system. This do ready to open
**	file and the file is  opened,
**
**	If testing, this procedure opens up a file of the form 'sxxx.mnx'
**	where xxx is the class number, in decimal.  If that file does not
**	exist, then the general 'slow.mnx' file is used.
**
** Inputs:
**	i4 langcode	        internal code for language
**	ER_MSGID id		message number.  used in getting file name
**				for testing.
**	i4 erindex		the index of ERmulti table
**
** Outputs:
**	CL_ERR_DESC err_code	if error happen,code is set to this.
**	Returns:
**		STATUS	OK   if everything worked
**
**			ER_NO_FILE
**			ER_BADOPEN
**			ER_MSGOVER
**			ER_BADLANGUAGE
**			ER_DIRERR
**
**	Exceptions:
**		none
**
** Side Effects:
**	none
** History:
**	01-Oct-1986 (kobayashi) - first written
**	29-jun-1987 (peter)
**		Add support for test files.
*/
STATUS
cer_sinit(langcode, msgid, erindex, err_code)
i4	    langcode;
ER_MSGID    msgid;
i4	    erindex;
CL_ERR_DESC  *err_code;
{
    STATUS	status;
    LOCATION	loc;
    char	loc_string[MAX_LOC];
    char	fbuf[MAX_LOC];
    i4		i;
    
    CL_CLEAR_ERR(err_code);

    /* Decide location */

    if ((status = cer_init(langcode,erindex,&loc,loc_string,ER_SLOWSIDE)) != OK)
    {
	return (status);
    }

    /* get fast file name */

    LOfstfile(cer_tfile(msgid,ER_SLOWSIDE,fbuf,cer_istest()),&loc);
    if (LOexist(&loc) != OK)
    {
	if (cer_istest())
	{	/* If testing, try the main file */
    	    LOfstfile(cer_tfile(msgid,ER_SLOWSIDE,fbuf,FALSE),&loc);
            if (LOexist(&loc) != OK)
            {	/* Even the other file does not exist */
	        return (ER_NO_FILE);
            }
	}
	else
	{
	    return (ER_NO_FILE);
	}
    }

    /*
    **	This process opens message file
    */
	
    if ((status = cer_open(&loc,&(ERmulti[erindex].ERfile[ER_SLOWSIDE]),
	err_code)) != OK)
    {
	cer_close(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	return (status);
    }

    return (OK);
}


/*{
** Name:	cer_init	- initialize message system
**
** Description:
**	This procedure decides the location of the message file. It gets the
**	message file name from a logical valiable (Shell variable), then we're
**	ready to open run-time message file.
**
** Inputs:
**	i4 langcode		internal code of language
**	i4 erindex		the index of ERmulti table
**	location *loc		location to set
**	char *loc_string	string to set to location
**	i4 fors		flag of fast or slow
**
** Outputs:
**	none.
**	Returns:
**		STATUS	OK   if everything worked
**
**			ER_MSGOVER
**			ER_BADLANGUAGE
**			ER_DIRERR
**
**	Exceptions:
**		none
**
** Side Effects:
**	none
** History:
**	01-Oct-1986 (kobayashi) - first written
**	02-Nov-1988 (greg) - check return status for NMloc, LOfaddpath
*/
STATUS
cer_init(langcode,erindex,loc,loc_string,fors)
i4	    langcode;
i4	    erindex;
LOCATION    *loc;
char	    *loc_string;
i4	    fors;
{
    char	nm[ER_MAX_LANGSTR];
    LOCATION	tmploc;
    STATUS	status;
    char	*msgbuf;

    /* initialize the path of message file */

    NMgtAt(ERx("II_MSGDIR"), &msgbuf);

    /* I replace logical-or to logical-and to protect access violetion. */

    if ((msgbuf != NULL) && (*msgbuf != EOS))
    {
	STcopy(msgbuf, loc_string);
    	LOfroms(PATH, loc_string,loc);                
    }
    else
    {
	if ((status = NMloc(FILES, PATH, NULL, &tmploc)) ||
    	    (status = ERlangstr(ERmulti[erindex].language, nm)))
    	{
	    return(status);
    	}

    	LOcopy(&tmploc, loc_string, loc);
    	
	if (status = LOfaddpath(loc, nm, loc))
	{
	    return(status);
	}
    }


    /* check if exist directory */

    if (LOexist(loc) != OK)
    {
	return(ER_DIRERR);
    }

    return(OK);
}


/*{
** Name:	cer_fstr- search fast message and return the message pointer
**
** Description:
**	This procedure searches fast message and returns the message pointer.
**	First, if element of class table which index is class_no, is null,
**	this procedure allocates area of number table and message in memory,
**	loads message data from file to the allocated memory, set each
**	message header address to number table and set number table header
**	address to element of class table which index is class_no.
**	Then it returns pointer that there is in element of number table
**	which index is message_no. Address of number table is in element
**	of class table which index is class_no.
**
**	When testing, after a class is read in, the file is closed so
**	that the next time, it will be reopened.
**
** Inputs:
**	ER_CLASS class_no	class number that searches message
**	i4 message_no		message number that searches message
**	i4 erindex 		the index of ERmulti  table
**
** Outputs:
**	none
**	Returns:
**		message string pointer
**	Exceptions:
**	    none
**
** Side Effects:
**	none
** History:
**	01-Oct-1986 (kobayashi) - first written
*/
char *
cer_fstr(class_no,mess_no,erindex)
ER_CLASS class_no;
i4 mess_no;
i4 erindex;
{
    ER_CLASS_TABLE *ERclass = ERmulti[erindex].class;
    STATUS status;
    static char errbuf[ER_MAX_LEN];

    /*
    **	Is class_no correct ?
    */

    if (class_no < 0 || class_no >= ERmulti[erindex].number_class)
    {
    	/* Bad class number */
	STprintf(errbuf,ERx("*** BAD CLASS NO.%x IN FAST MESSAGE ***"),class_no);
	return(errbuf);
    }

    /* 
    **	set up number table and message area 
    */
    if (ERclass[class_no].message_pointer == NULL)
    {
	/* It isn't set up number table and message area yet. */
	if ((status = cer_read(class_no,erindex)) != OK)
	{
	    /* It can't set up number table and message area */
	    STprintf(errbuf,ERx("*** CAN'T SET UP MESSAGE TABLE: CLASS NO.%x ***"),class_no);
	    /* If testing, close the file after the class is loaded. */
	    cer_tclose(&(ERmulti[erindex].ERfile[ER_FASTSIDE]));
	    return(errbuf);
	}
	/* If testing, close the file after the class is loaded. */
	cer_tclose(&(ERmulti[erindex].ERfile[ER_FASTSIDE]));
    }
    if (mess_no < 0 || mess_no >= ERclass[class_no].number_message)
    {
	return(NULL);
    }

    /* return pointer of message string that searched */	
    return((char *)*(ERclass[class_no].message_pointer + mess_no));
}


/*{
** Name:	cer_read	- set up number table and message area
**
** Description:
**	This procedure sets up number table and message area. This procedure
**	gets the information about size of number table and message area
**	from runtime fast message file, allocates them in memory, loads
**	message data from file to the allocated memory, set each message
**	header address to number table and set number table header address
**	to element of class table whitch index is class_no.
**
** Inputs:
**	ER_CLASS class_no		class number to set up number table and
**					message area
**	i4 erindex			the index of ER_language table
**
** Outputs:
**	none
**	Returns:
**		STATUS	OK   if everything worked
**
**			ER_BADREAD
**			ER_NOALLOC
**			ER_MSGOVER
**	Exceptions:
**		none
**
** Side Effects:
**	none
** History:
**	01-Oct-1986 (kobayashi) - first written
**	07-Mar-1989 (Mike S)
**	    	If we're mapped, access messages where they're mapped.
*/
STATUS
cer_read(class_no,erindex)
ER_CLASS class_no;
i4 erindex;
{
    i4	count;
    ERCONTROL	cont_r, *pcont_r;
    unsigned char 	*messpointer;
    unsigned char	*dummyp;
    i4	i;
    i4 	j;
    ER_CLASS_TABLE *ERclass = ERmulti[erindex].class;
    register ERFILE *fstfile = &(ERmulti[erindex].ERfile[ER_FASTSIDE]);
    register bool mapped = fstfile->mapped;

    /*
    **	search control_record in file and 
    **	get information about size of number table and
    **	message area 
    */

    if (mapped)
    {
	pcont_r = (ERCONTROL *)(fstfile->map_addr + sizeof(cont_r) * (class_no) + sizeof(ER_CLASS));
    }
    else
    {
        if (cer_dataset(sizeof(cont_r) * (class_no) + sizeof(ER_CLASS), 
    			sizeof(cont_r), &cont_r, fstfile) != OK)
        {
	    /* It is fail to read data from file */
            return(ER_BADREAD);
        }
    	pcont_r = &cont_r;
    }
    /*
    **	If class doesn't have all message,set only number of message to class
    **	table.
    */

    if (pcont_r->tblsize == 0)
    {
	ERclass[class_no].number_message = 0;
	return(OK);
    }

    /*
    **	allocate number table and message area
    */

    ERclass[class_no].message_pointer = (u_char**)
		MEreqmem(0, pcont_r->tblsize * sizeof(char *), FALSE, NULL);
    if (ERclass[class_no].message_pointer == NULL)
    {
	/* fail of allocate */
	return(ER_NOALLOC);
    }

    if (mapped)
    {
	ERclass[class_no].data_pointer = 
			(u_char *) (fstfile->map_addr + pcont_r->offset);
    }
    else
    {
        ERclass[class_no].data_pointer = (u_char*)
		MEreqmem(0,pcont_r->areasize, FALSE,NULL);
    	if (ERclass[class_no].data_pointer == NULL)
    	{
	    /* fail of allocate */
	    MEfree((PTR)ERclass[class_no].message_pointer);
	    ERclass[class_no].message_pointer = NULL;
	    return(ER_NOALLOC);
        }

    	/* 
   	**	load message data from runtime-fast message file to
    	**	allocated message area.
    	*/

    	if (cer_dataset(pcont_r->offset,pcont_r->areasize,
	    ERclass[class_no].data_pointer,
	    &(ERmulti[erindex].ERfile[ER_FASTSIDE])) != OK) 
    	{
	    /* It is fail to read data from file */
	    ERrelease(class_no);
	    return(ER_BADREAD);
        }
    }

    /*	
    **	set each message header pointer to number table
    */

    for (i = 0,j = 0,messpointer = ERclass[class_no].data_pointer;
	 i < pcont_r->areasize; ++i)
    {
	/* set starting string pointer to dummy pointer */

	dummyp = messpointer + i;

	/* check dummy data */

	if (*dummyp == 0xff && *(dummyp + 1) == EOS)
	{
	    dummyp = NULL;
	}

	/* Search null character */

	while (*(messpointer + i))
	{
	    if (++i >= pcont_r->areasize)
	    {
		/* Too many message string character */
		ERrelease(class_no);
		return(ER_MSGOVER);
	    }
	}

	/* set start string pointer to message table */

	*(ERclass[class_no].message_pointer + j) = dummyp;
        if (++j > pcont_r->tblsize)
        {
	    /* Too many message to set */
	    ERrelease(class_no);
	    return(ER_MSGOVER);
	}
    }
		

    /* This set size of message table to head element of message table */

    ERclass[class_no].number_message = pcont_r->tblsize;

    return(OK);
}


/*{
**  Name:   cer_isopen - check file opened or not.
**
**  Description:
**	This function checks the file opened or not.
**	The file's internsl file identify is in parameter 'file'->w_ifi
**	, And if it isn't NULL, file was already opened. If it is NULL,
**	file isn't opened yet.
**
**  Input:
**	i4	erindex	    Index in ERmulti table.
**	i4	fors	    Either ER_FASTSIDE or ER_SLOWSIDE to
**			    check which files are open.
**
**  Output:
**	none.
**	Return:
**	    FALSE		file isn't opened yet.
**	    TRUE		file was already opened.
**
**  Side Effect:
**	none.
**
**  History:
**	16-Oct-1986 (kobayashi) - first written.
**	03-jul-1987 (peter)
**		Rewritten.
**	07-Mar-1989 (Mike S)
**		Have VMS check mapped flag.
*/
bool
cer_isopen(erindex,fors)
i4	erindex;
i4	fors;
{
    ERFILE *file;

    file = &(ERmulti[erindex].ERfile[fors]);
    return((file->w_ifi || file->mapped) ? 1 : 0);
}


/*{
** Name:	cer_sstr- search slow message and set the message to buffer.
**
** Description:
**	This procedure searches slow message and set the message to buffer.
**	First, this read index page data from file. and search data page number
**	from index page, read data page. And search target message in data
**	page,then set that message to buffer.
**
** Inputs:
**	ER_MSGID msgid		message id to want to searches message
**	char *sqlstate		pointer where to store SQLSTATE status code
**	char buf		buffer to set message string
**	i4 bufsize		size of buffer
**	i4 erindex		the index of ERmulti table
**	i4 getorlookup		flag cer_get or ERlookup that this was called
**
** Outputs:
**	CL_ERR_DESC *err_code	pointer of error code structer
**	Returns:
**		OK	    scucess
**
**		ER_BADREAD
**		ER_NOT_FOUND
**		ER_TOOSMALL
**		
**	Exceptions:
**		none
**
** Side Effects:
**	changes to err_code.
** History:
**	01-Oct-1986 (kobayashi) - first written
**	25-jun-1987 (peter)
**		Add support for test mode.  Close the file after each
**		read if testing.
**	01-may-89 (jrb)
**	    Added support for generic errors: new parameter which passes back
**	    generic error acquired from the slow.mnx file.  If this parameter
**	    is NULL, nothing is passed back.
**	5/90 (bobm)
**	    Added support for > ER_SMAX_KEY buckets to prevent message file
**	    running out of room.  Also added magic number / version check.
**	23-oct-92 (andre)
**	    changed interface of cer_sstr() to return SQLSTATE status code.
*/
STATUS
cer_sstr(msgid, sqlstate, buf, bufsize, erindex, err_code, getorlookup)
ER_MSGID    msgid;
char	    *sqlstate;
char	    *buf;
i4	    bufsize;
i4	    erindex;
CL_ERR_DESC  *err_code;
i4	    getorlookup;
{
    i4	*srch;
    i4	total;
    i4	hnum;
    i4	bnum;
    i4	bucket;
    i4	last_message;
    INDEX_PAGE      index_page;
    char	    data_page[sizeof(INDEX_PAGE)];
    MESSAGE_ENTRY	*m;
    i4	    status;
    i4		    i;
    i4		    blength;
    char	    *textp;
    
    CL_CLEAR_ERR(err_code);

    /* Read relative block 1 to get the index page */

    if ((status = cer_getdata(&index_page,sizeof(index_page), (i4) 1,
	&(ERmulti[erindex].ERfile[ER_SLOWSIDE]),err_code)) != OK)
    {
	cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	return(status);
    }

    if (ER_GETMAG(index_page.sanity) != ER_MAGIC || index_page.index_count < 0)
    {
	cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	return(ER_FILE_FORMAT);
    }

    if (ER_GETVER(index_page.sanity) != ER_VERSION)
    {
	cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	return(ER_FILE_VERSION);
    }

    /*
    ** hnum is the number of records containing the index array
    */
    total = index_page.index_count;
    hnum = 1 + (total + PERBLOCK - 1 - ER_SMAX_KEY)/PERBLOCK;
    
    /*
    ** Search the index for the bucket containing the message
    ** This may entail reading extra records if there are > ER_SMAX_KEY
    ** buckets.  Notice that we are maintaining "bucket" as the proper
    ** argument to cer_getdata, rather than the actual bucket.
    */

    last_message = -1;
    srch = index_page.index_key;
    bnum = 1;
    blength = total > ER_SMAX_KEY ? ER_SMAX_KEY : total;
    bucket = SRECFACTOR*hnum + 1;
    for (;;)
    {
	for (i = 0; i < blength; i++, srch++, bucket += SRECFACTOR)
	{
	    if (*srch < last_message)
	    {
		cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
		return(ER_FILE_CORRUPTION);
	    }
	    if (msgid <= *srch)
	    {
		last_message = *srch;
		break;
	    }
	}

	/* if we broke, we found the bucket */
	if (i < blength)
	    break;

	/*
	** if we still have more records worth of index left, read another
	** record, otherwise return ER_NOT_FOUND
	*/
	if (bnum >= hnum)
	{
	    cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	    return(ER_NOT_FOUND);
	}

	if ((status = cer_getdata(&index_page,sizeof(index_page),
		SRECFACTOR*bnum + 1,
		&(ERmulti[erindex].ERfile[ER_SLOWSIDE]),err_code)) != OK)
	{
	    cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	    return(status);
	}

        /*
	** OK, treat the record as simply an added array of i4's,
	** and go around again.
	*/
	total -= blength;
	blength = total > PERBLOCK ? PERBLOCK : total;
	srch = (i4 *) &index_page;
	++bnum;
    }

    /* Read the page containing the message */

    if ((status = cer_getdata(data_page,sizeof(data_page), bucket,
	&(ERmulti[erindex].ERfile[ER_SLOWSIDE]),err_code)) != OK)
    {
	cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	return(status);
    }

    /* Search this bucket for the message */

    for (i = 0;;)
    {
	m = (MESSAGE_ENTRY *)&data_page[i];

	if (m->me_length <= 0 || (i + m->me_length) > sizeof(INDEX_PAGE))
	{
	    cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	    return(ER_FILE_CORRUPTION);
	}

	if (m->me_msg_number == msgid)
	{
	    /* found; set SQLSTATE status code if non-null ptr is present */
	    if (sqlstate != (char *) NULL)
	    {
		i4	j;

		for (j = 0; j < 5; j++)
		    sqlstate[j] = m->me_sqlstate[j];
	    }

	    break;
	}
	if (m->me_msg_number == last_message)
	{
	    /* not found */
	    cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	    return(ER_NOT_FOUND);
	}
	i += m->me_length;
    }

    /*	set message string to buffer */

    if (getorlookup == ER_GET)
    {
	/* 
	** When this function is called from cer_get,set only message text 
	** to buffer.
	*/
	if (bufsize < m->me_text_length + 1)
	{
	    cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	    return(ER_TOOSMALL);
	}
	textp = m->me_name_text + m->me_name_length +1;
	STcopy(textp,buf);
    }
    else
    {
	/* 
	** When this function is called from ERlookup,set name and message text 
	** to buffer.
	*/
	if (bufsize < m->me_name_length + 1 + m->me_text_length + 1)
	{
	    cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
	    return(ER_TOOSMALL);
	}
	STcopy(m->me_name_text, buf);
    }
    cer_tclose(&(ERmulti[erindex].ERfile[ER_SLOWSIDE]));
    return(OK);
}

/*{
**  Name:	cer_tclose	- close file if in test mode.
**
**  Description:
**	If the test mode is being used (see ERinit) then the
**	file is closed.  Otherwise, it is left open.  By
**	closing the file after each message, the message system
**	must go through and open up the next file.
**
**  Inputs:
**	fileptr		ERFILE to close.
**
**  Outputs:
**	none.
**
**  History:
**	25-jun-1987 (peter)
**		Written.
*/
VOID
cer_tclose(fileptr)
ERFILE	*fileptr;
{
	if (cer_istest())
	{
		cer_close(fileptr);
	}
	return;
}


/*{
**  Name:	cer_tfile	- generate a file name to open.
**
**  Description:
**	Generate a file name for the messages.  For core products,
**	if in standard non-test mode, this will be ER_SLOWFILE or
**	ER_FASTFILE.  For unbundled products in standard non-test mode,
**	this will be a filename costructed from ER_SLOWTMPLT or ER_FASTTMPLT
**	and the part name returned by NMgetpart.  If in test mode, it will
**	be 'f' or 's' (for slow or fast), followed by the class number, in
**	decimal.
**
**  Inputs:
**	msgid		Used to construct the file name.
**	fors		fast or slow.
**	fname		buffer to contain file name (must be MAX_LOC).
**	testing		TRUE is test file to be used.  FALSE tonight.
**
**  Outputs:
**	pointer to fname.
**
**  History:
**	25-jun-1987 (peter)
**		Written.
**	4/91 (Mike S) Construct filename for unbundled product.
*/
char *
cer_tfile(id, fors, fname, testing)
ER_MSGID	id;
i4		fors;
char		*fname;
bool		testing;
{
	char		numbuf[20];
	ER_CLASS	class_no;
	char		p[MAX_PARTNAME + 1];
	char		partbuffer[4];

        class_no = ((id & ER_MSGIDMASK2) >> ER_MSGMIDSHIFT) & ER_MSGIDMASK5;
	if (!testing)
	{
		/* Check for unbundled product */
		if ((NMgetpart(p)) != NULL)
		{
			STlcopy(p, partbuffer, 3);
			CVlower(partbuffer);
			
			if (fors == ER_FASTSIDE)
				STprintf(fname, ER_FASTTMPLT, partbuffer);
			else
				STprintf(fname, ER_SLOWTMPLT, partbuffer);
		}
		else if (fors == ER_FASTSIDE)
		{
			STcopy(ER_FASTFILE, fname);
		}
		else
		{
			STcopy(ER_SLOWFILE, fname);
		}
	}
	else
	{
		if (fors == ER_FASTSIDE)
		{
			STcopy("f", fname);
		}
		else
		{
			STcopy("s", fname);
		}
		CVla((i4)class_no, numbuf);
		STcat(fname, numbuf);
		STcat(fname, ".mnx");
	}
	return(fname);
}
