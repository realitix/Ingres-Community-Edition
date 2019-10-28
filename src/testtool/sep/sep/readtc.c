/*
**    Include file
*/

#include <compat.h>
#include <lo.h>
#include <er.h>
#include <si.h>
#include <tc.h>
#include <cm.h>

#ifdef SEP_SYNC
#include <clconfig.h>
#include <rusage.h>
#include <tc_sync.h>
#endif 

#define readtc_c

#include <sepdefs.h>
#include <sepfiles.h>
#include <fndefs.h>

/*
** History:
**	20-apr-1994 (donj)
**	    Created
**	31-may-1994 (donj)
**	    Took out #include for sepparam.h. QA_C was complaining about all
**	    the unecessary GLOBALREF's.
**	22-jun-1994 (donj)
**	    Added a timeout_try_again function to send an TC_EOQ then try a
**	    TCgetc() again if we get a timeout.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Sep-2000 (hanch04)
**          replace nat and longnat with i4
**      06-Sep-2010 (coomi01) b124361
**          Sep improvements
**      05-Jan-2010 (coomi01) b124361
**          Move header file inside SEP_SYNC conditional above.
**      10-Feb-2011 (coomi01) b125021
**          Use pramga to avoid unnecessary timeout.
**      15-Feb-2011 (coomi01) b125021
**          Adjust pramga to unnecessary timeout 10min for empty commands. 
**	06-Sep-2011 (kiria01) b125725/b125726
**	    Ensure UTF-8 chars handled properly.
**	16-Sep-2011 (kiria01) b125725/b125726
**	    Pin the char handling to keeping UTF8 chars integral regardless
**	    of CHARSET handling.
**      21-Sep-2011 (hanal04) b125725/b125726
**          Include clconfig.h before rusage.h
*/

/*
**  defines
*/

#define TC_RETRY_TIMEOUT  11

/*
**  Reference global variables
*/

GLOBALREF    i4            tracing ;
GLOBALREF    i4            SEPtcsubs ;

GLOBALREF    FILE         *traceptr ;

GLOBALREF    SEPFILE      *sepResults ;

GLOBALREF    TCFILE       *SEPtcinptr ;

#define simple_trace_if(sw,str) \
 if (tracing&sw) { SIfprintf(traceptr,ERx(str)); SIflush(traceptr); }


/*
**  Read_TCresults
*/
i4
Read_TCresults(	TCFILE *tcoutptr,TCFILE *tcinptr,i4 waiting,bool *fromBOS,
		i4 *sendEOQ_ptr,bool waitforit,STATUS *tcerr)
{
    i4                     c = 0 ;
    i4                     lastc = 0 ;
    i4                     last_TC = 0 ;
    i4                     tmp_waiting = waiting ;
    bool                   keepgoing = TRUE ;
    bool                   nothing_yet = TRUE ;
    bool                   timeout_try_again = TRUE ;

#ifdef SEP_SYNC
    int                     count;
#endif
    simple_trace_if(TRACE_DIALOG,"Read_TCresults> dialog fm fe (recording) >\n");

    for (*tcerr = OK; (keepgoing && (*tcerr == OK)); lastc = c)
    {
	if (!waitforit)
	    if (TCempty(tcoutptr))
	    {
		keepgoing = FALSE;
		continue;
	    }

#ifdef SEP_SYNC
	/*
	**  Gobble as much as possible, 
	**  ~ rather than running up and down the stack fetching one char at a time. 
	*/
	if ( sepSyncGlobalVars.syncMode )
	{
	    /* Fast transfer sync data direct to results file, 
	    ** ~ other than those special characters upon which we act.
	    */
	    count = 0;
	    do
	    {
		count += TCtransfer(tcoutptr, sepResults->_fptr, tmp_waiting, &lastc, &c);
	    }
	    while ( TC_CONTINUE == c );

	    nothing_yet = nothing_yet && (count == 0);

	    if      (c == TC_EOF) SEPtcsubs--;
	    else if (c == TC_BOS) SEPtcsubs++;
	}
	else
#endif
	{
	    /* Do it the hard way, one character at a time */
	    c = SEP_TCgetc_trace(tcoutptr, tmp_waiting);
	}

	tmp_waiting = waiting; /* reset to timeout value */

	if (((c == TC_EQR)||(c == TC_EOF)) &&
	    (lastc != '\n'))
	{
	    simple_trace_if(TRACE_DIALOG,"\n");
	}

	switch (c)
	{
#ifdef SEP_SYNC
	   /* 
	   ** Need this should things get messed up, and we want to break out here
	   ** ~ Abandon ship by breaking out of this routine.
	   */
	   case TC_BUSY:
	       c = TC_EQR ;
	       keepgoing = FALSE;
	       break;
#endif
	   case TC_EQR:
			keepgoing = nothing_yet;

#ifdef SEP_SYNC
			if (sepSyncGlobalVars.pragmaFlags & SEPSYNC_NOTIMEOUT)
			{
			    /* Not obvious, but the copying of bits NOTIMEOUT to 
			    ** NOTIMEOUT_1 allows us to decouple the high level
			    ** switch from the lower level one. For 'one read'
			    ** sep might do multiple timeouts.
			    */
			    sepSyncGlobalVars.pragmaFlags &= ~SEPSYNC_NOTIMEOUT;
			    sepSyncGlobalVars.pragmaFlags |=  SEPSYNC_NOTIMEOUT_1;
			}
#endif
			tmp_waiting = (keepgoing&&(waiting > TC_RETRY_TIMEOUT))
				    ? TC_RETRY_TIMEOUT : waiting;

			last_TC = c;
			break;

	   case TC_EOF:
			keepgoing = (SEPtcsubs == 0) ? FALSE : keepgoing;
			*fromBOS  = FALSE;

			if (SEPtcsubs <= 0)
			{
			    *tcerr = INGRES_FAIL;
			}
			else if (*sendEOQ_ptr == SEPtcsubs)
			{
			    endOfQuery(SEPtcinptr,ERx("Read_TC1"));
			    *sendEOQ_ptr = 0;
			}

			last_TC = c;
			break;

	   case TC_TIMEDOUT:

#ifdef SEP_SYNC
			if (sepSyncGlobalVars.pragmaFlags & SEPSYNC_NOTIMEOUT)
			{
			    sepSyncGlobalVars.pragmaFlags &= ~SEPSYNC_NOTIMEOUT;
			    c = last_TC;
			    keepgoing = FALSE;
			}
			else
#endif
			if (nothing_yet && last_TC == TC_EQR)
			{
			    c = last_TC;
			    keepgoing = FALSE;
			}
			else if (timeout_try_again)
			{
			    keepgoing = TRUE;
			    endOfQuery(SEPtcinptr,ERx("Read_TC2"));
			    timeout_try_again = FALSE;
			}
			else
			{
			    last_TC = c;
			    keepgoing = FALSE;
			}

			break;

	   case TC_EOQ:
			last_TC = c;
			break;

	   case TC_BOS:
			endOfQuery(tcinptr,ERx("Read_TC2"));
			*fromBOS = TRUE;
			last_TC = c;
			break;
	   default:
	       {	char tmp[6];
			i4 l, i = 1;
			tmp[0] = c;
			l = CMUTF8cnt(tmp);
			while (i < l)
				tmp[i++] = SEP_TCgetc_trace(tcoutptr, tmp_waiting);
			nothing_yet = FALSE;
			SEPputcn(tmp, l, sepResults);
			break;
	       }
	}
    }

    SEPputc(SEPENDFILE, sepResults);
    SEPrewind(sepResults, TRUE);
    simple_trace_if(TRACE_DIALOG,"Read_TCresults> End.>\n");

    return (c);
}
