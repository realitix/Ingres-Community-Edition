/*
**  (C) Copyright 1995, 2011 Actian Corporation
*/

#include <compat.h> 
#include <gl.h>
#include <iicommon.h>
#include <er.h>
#ifndef VMS
#include <systypes.h>
#endif
#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <stdarg.h>
#include <cv.h>
#include <me.h>
#include <nm.h>
#include <st.h> 
#include <tr.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"               /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */
#include <idmsucry.h>
#include <idmsocvt.h>               /* ODBC conversion routines */


/*
**  Module Name: TRACE.C
**
**  Description: Debugging routines for ODBC driver.
**
**
**  Change History
**  --------------
**
**  date        programmer          description
**
**   3/15/1995  Dave Ross          Initial coding
**   3/15/1999  Dave Thole         Port to UNIX
**   4/01/2002  Ralph Loen         Stubbed out UtPrint and UtSnap for VMS.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**	23-apr-2004 (somsa01)
**	    Include systypes.h to avoid compile errors on HP.
**      24-jun-2004 (loera01)
**          Ported to VMS.
**      03-sep-2004 (Ralph.Loen@ca.com) Bug 112975
**          In TraceOdbc(), retrieved short arguments as an int rather than
**          i2, otherwise segvs happen on Linux.
**      15-nov-2004 (Ralph.Loen@ca.com) Bug 113462
**          Use IItrace global block to handle tracing.
**      23-oct-2006 (Ralph Loen) SIR 116577
**          Added support for SQLBrowseConnect(), SQLColumnPrivileges(),
**          SQLDescribeParam(), SQLExtendedFetch(), SQLFetchScroll(),
**          and SQLTablePrivileges().
**      19-may-2009 (Ralph Loen) Bug 122082
**          Add support for SQLGetFunctions().
**      10-jul-2009 (Ralph Loen) Bug 122082
**          Fixed invalid cast of a char pointer to i2.
**      15-Feb-2011 (Ralph Loen) Bug 125041
**          Fix compiler warnings on 64-bit Windows.
**      13-Jul-2011 (hanal04) SIR 125458
**          Update cs_elog calls inline with sc0e_putAsFcn changes.
**      27-Jul-2011 (Bruce Lunsford) Bug 125522
**          Fix pointer and SQLLEN displays for 64-bit systems; they
**          don't fit in traceValue[] i4's. Instead put them into
**          charPtr[]'s; though not typed exactly right, SQLLEN and
**          pointers are both 4 bytes long on 32-bit systems and 
**          8 bytes long on 64-bit systems. Also change display
**          format from %.8lx and %ld to 0x%p to automatically handle 
**          32-64-bit system difference. With STprintf, the former
**          formats truncated the high-order 32-bits.
**          Also correct spelling of Uknown.
**          Move trace buffer from static to local stack to better
**          handle tracing in multi-threaded environment.
*/

/*
**  Parameter format strings:
*/
static const char TRACE_FMT[] = "ODBC Trace: %s (%s)\n";

static const char TRACE_ATT[] = "0x%p, %d, %d, 0x%p, %d, 0x%p, 0x%p";
static const char TRACE_BIC[] = "0x%p, %d, %d, 0x%p, 0x%p, 0x%p";
static const char TRACE_BIP[] = "0x%p, %d, %d, %d, %d, 0x%p, %d, 0x%p, 0x%p%, 0x%p";
static const char TRACE_COL[] = "0x%p, 0x%p, %d, 0x%p, %d, 0x%p, %d, 0x%p, %d";
static const char TRACE_CON[] = "0x%p, 0x%p, %d, 0x%p, %d, 0x%p, %d";
static const char TRACE_DDS[] = "0x%p, 0x%p, %d";
static const char TRACE_DDU[] = "0x%p, 0x%p, %d";
static const char TRACE_DES[] = "0x%p, %d, 0x%p, %d, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p";
static const char TRACE_DRV[] = "0x%p, %.4X, 0x%p, %d, 0x%p, %d, 0x%p, %d";
static const char TRACE_DW1[] = "0x%p";
static const char TRACE_DW2[] = "0x%p, 0x%p";
static const char TRACE_ERR[] = "0x%p, 0x%p, 0x%p, 0x%p, 0x%p, 0x%p, %d, 0x%p";
static const char TRACE_FST[] = "0x%p, %d";
static const char TRACE_GCS[] = "0x%p, 0x%p, %d%, 0x%p";
static const char TRACE_GET[] = "0x%p, %d, %d, 0x%p, 0x%p, 0x%p";
static const char TRACE_INF[] = "0x%p, %d, 0x%p, %d, 0x%p";
static const char TRACE_OPT[] = "0x%p, %d, 0x%p";
static const char TRACE_PRE[] = "0x%p, 0x%p, %d";
static const char TRACE_PRM[] = "0x%p, 0x%p%, 0x%p";
static const char TRACE_SPE[] = "0x%p, %d, 0x%p, %d, 0x%p, %d, 0x%p, %d, %d, %d";
static const char TRACE_STA[] = "0x%p, 0x%p, %d, 0x%p, %d, 0x%p, %d, %d, %d";
static const char TRACE_TYP[] = "0x%p, %d";
static const char TRACE_UNK[] = "%d";
static const char TRACE_DSC[] = "p, %d, 0x%p, 0x%p, 0x%p, 0x%p";
IITRACE IItrace;

/*
**  ODBC API Function name and parameter format table:
*/
static const struct
{
    int    iApi;                        /* ODBC API function index */
    LPCSTR szName;                      /* ODBC API function name */
    LPCSTR szFmt;                       /* ODBC API parm format string */
}
API[] =
{
    /*
    ** Core Functions
    */
    { SQL_API_SQLALLOCCONNECT,     "SQLAllocConnect",       TRACE_DW2 },
    { SQL_API_SQLALLOCENV,         "SQLAllocEnv",           TRACE_DW1 },
    { SQL_API_SQLALLOCSTMT,        "SQLAllocStmt",          TRACE_DW2 },
    { SQL_API_SQLBINDCOL,          "SQLBindCol",            TRACE_BIC },
    { SQL_API_SQLBINDPARAMETER,    "SQLBindParameter",      TRACE_BIP },
    { SQL_API_SQLBROWSECONNECT,    "SQLBrowseConnect",      TRACE_CON },
    { SQL_API_SQLCANCEL,           "SQLCancel",             TRACE_DW1 },
    { SQL_API_SQLCOLATTRIBUTES,    "SQLColAttributes",      TRACE_ATT },
    { SQL_API_SQLCOLUMNPRIVILEGES, "SQLColumnPrivileges",   TRACE_COL },
    { SQL_API_SQLCONNECT,          "SQLConnect",            TRACE_CON },
    { SQL_API_SQLDESCRIBECOL,      "SQLDescribeCol",        TRACE_DES },
    { SQL_API_SQLDESCRIBEPARAM,    "SQLDescribeParam",      TRACE_DSC },
    { SQL_API_SQLDISCONNECT,       "SQLDisconnect",         TRACE_DW1 },
    { SQL_API_SQLERROR,            "SQLError",              TRACE_ERR },
    { SQL_API_SQLEXECDIRECT,       "SQLExecDirect",         TRACE_PRE },
    { SQL_API_SQLEXECUTE,          "SQLExecute",            TRACE_DW1 },
    { SQL_API_SQLEXTENDEDFETCH,    "SQLExtendedFetch",      TRACE_INF },
    { SQL_API_SQLFETCH,            "SQLFetch",              TRACE_DW1 },
    { SQL_API_SQLFETCHSCROLL,      "SQLFetchScroll",        TRACE_OPT },
    { SQL_API_SQLFREECONNECT,      "SQLFreeConnect",        TRACE_DW1 },
    { SQL_API_SQLFREEENV,          "SQLFreeEnv",            TRACE_DW1 },
    { SQL_API_SQLFREESTMT,         "SQLFreeStmt",           TRACE_FST },
    { SQL_API_SQLGETCURSORNAME,    "SQLGetCursorName",      TRACE_GCS },
    { SQL_API_SQLGETFUNCTIONS,     "SQLGetFunctions",       TRACE_OPT },
    { SQL_API_SQLNUMRESULTCOLS,    "SQLNumResultCols",      TRACE_DW2 },
    { SQL_API_SQLPREPARE,          "SQLPrepare",            TRACE_PRE },
    { SQL_API_SQLROWCOUNT,         "SQLRowCount",           TRACE_DW2 },
    { SQL_API_SQLSETCURSORNAME,    "SQLGetCursorName",      TRACE_DDS },
    { SQL_API_SQLTRANSACT,         "SQLTransact",           TRACE_DDU },
    { SQL_API_SQLTABLEPRIVILEGES,  "SQLTablePrivileges",    TRACE_COL },
    /*
    ** Level 1 Functions
    */
    { SQL_API_SQLCOLUMNS,          "SQLColumns",            TRACE_COL },
    { SQL_API_SQLDRIVERCONNECT,    "SQLDriverConnect",      TRACE_DRV },
    { SQL_API_SQLGETCONNECTOPTION, "SQLGetConnectOption",   TRACE_OPT },
    { SQL_API_SQLGETDATA,          "SQLGetData",            TRACE_GET },
    { SQL_API_SQLGETINFO,          "SQLGetInfo",            TRACE_INF },
    { SQL_API_SQLGETSTMTOPTION,    "SQLGetStmtOption",      TRACE_OPT },
    { SQL_API_SQLGETTYPEINFO,      "SQLGetTypeInfo",        TRACE_TYP },
    { SQL_API_SQLPARAMDATA,        "SQLParamData",          TRACE_DW2 },
    { SQL_API_SQLPUTDATA,          "SQLPutData",            TRACE_PRE },
    { SQL_API_SQLSETCONNECTOPTION, "SQLSetConnectOption",   TRACE_OPT },
    { SQL_API_SQLSETSTMTOPTION,    "SQLSetStmtOption",      TRACE_OPT },
    { SQL_API_SQLSPECIALCOLUMNS,   "SQLSpecialColuumns",    TRACE_SPE },
    { SQL_API_SQLSTATISTICS,       "SQLStatistices",        TRACE_STA },
    { SQL_API_SQLTABLES,           "SQLTables",             TRACE_COL },
    /*
    ** Level 2 Functions
    */
    { SQL_API_SQLMORERESULTS,      "SQLMoreResults",        TRACE_DW1 },
    { SQL_API_SQLPARAMOPTIONS,     "SQLParamOptions",       TRACE_PRM },
    /*
    ** Bug'n d'bug:
    */
    { 0,                           "Unknown ODBC API",      TRACE_UNK }
};
#define API_MAX sizeof API / sizeof API[0] - 1

/*
**  TraceOdbc
**
**  Trace an ODBC API function as it enters this DLL.
**
**  On entry: iApi = ODBC API function index as defined for SQLGetFunctions
**            ...  = Function parameters as passed from Driver Manager
**
**  Returns:  Nothing.
*/
void TraceOdbc (int iApi, ...)
{
    int     i;
    va_list marker;
    SQLHANDLE handle;
    SQLCHAR *charPtr[10];
    char traceFormat[200];
    char szTrace[256]={EOS};              /* Trace parm build area */
    i4 traceValue[15];
    i2 traceShort[15];
    va_start (marker, iApi);

    for (i = 0; i < API_MAX; i++)
    {
        if (API[i].iApi == iApi)
        break;
    }

    if (API[i].szFmt && i <= API_MAX)
    {
        STcopy(API[i].szFmt, traceFormat);
        switch (API[i].iApi)
        {
        case SQL_API_SQLALLOCENV:
        case SQL_API_SQLEXECUTE:
        case SQL_API_SQLDISCONNECT:
        case SQL_API_SQLCANCEL:
            handle = va_arg(marker, void *);
            STprintf (szTrace, traceFormat, handle);
            break;
        case SQL_API_SQLALLOCCONNECT:
        case SQL_API_SQLALLOCSTMT:
            handle = va_arg(marker, void *);
            charPtr[0] = va_arg(marker, SQLCHAR *);
            STprintf (szTrace, traceFormat, handle, charPtr[0]);
            break;
        case SQL_API_SQLALLOCHANDLE:
            traceShort[0] = (i2)va_arg(marker, i4);
            handle = va_arg(marker, void *);
            charPtr[0] = va_arg(marker, SQLCHAR *);
            STprintf (szTrace, traceFormat, traceShort[0], handle, charPtr[0]);
            break;
        case SQL_API_SQLEXECDIRECT:
        case SQL_API_SQLPREPARE:
            handle = va_arg(marker, void *);
            charPtr[0] = va_arg(marker, SQLCHAR *);
            traceValue[0] = va_arg(marker, i4);
            STprintf (szTrace, traceFormat, handle, charPtr[0], traceValue[0]);
            break;
        case SQL_API_SQLGETINFO:
            handle = va_arg(marker, void *);
            traceShort[0] = (i2)va_arg(marker, i4);
            charPtr[0] = va_arg(marker, SQLCHAR *);
            traceShort[1] = (i2)va_arg(marker, i4);
            charPtr[1] = va_arg(marker, SQLCHAR *);
            STprintf (szTrace, traceFormat, handle, traceShort[0],
                charPtr[0], traceShort[1], charPtr[1]);
            break;
        case SQL_API_SQLTRANSACT:
            handle = va_arg(marker, void *);
            charPtr[0] = va_arg(marker, SQLCHAR *);
            traceShort[0] = (i2)va_arg(marker, i4);
            STprintf (szTrace, traceFormat, handle, charPtr[0],
                traceShort[0]);
            break;
        case SQL_API_SQLBINDCOL:
            handle = va_arg(marker, void *);
            for(i = 0; i < 2; i++)
                traceShort[i] = (i2)va_arg(marker, i4);
            for(i = 0; i < 3; i++)
                charPtr[i] = va_arg(marker, SQLCHAR *);
            STprintf (szTrace, traceFormat, handle, traceShort[0],
                traceShort[1], charPtr[0], charPtr[1], charPtr[2]);
            break;
        case SQL_API_SQLBINDPARAMETER:
            handle = va_arg(marker, void *);
            for(i = 0; i < 4; i++)
                traceShort[i] = (i2)va_arg(marker, i4);
            charPtr[0] = va_arg(marker, SQLCHAR *);
            traceShort[i] = (i2)va_arg(marker, i4);
            for(i = 1; i < 4; i++)
                charPtr[i] = va_arg(marker, SQLCHAR *);
            STprintf (szTrace, traceFormat, handle, traceShort[0],
                traceShort[1], traceShort[2], traceShort[3], charPtr[0],
                traceShort[4], charPtr[1], charPtr[2], charPtr[3]);
            break;
        case SQL_API_SQLCONNECT:
            handle = va_arg(marker, void *);
            charPtr[0] = va_arg(marker, SQLCHAR *);
            traceShort[0] = (i2)va_arg(marker, i4);
            charPtr[1] = va_arg(marker, SQLCHAR *);
            traceShort[1] = (i2)va_arg(marker, i4);
            charPtr[2] = va_arg(marker, SQLCHAR *);
            traceShort[2] = (i2)va_arg(marker, i4);
            STprintf (szTrace, traceFormat, handle, charPtr[0],
                traceShort[0], charPtr[1], traceShort[1], charPtr[2],
                traceShort[2]);
            break;
        case SQL_API_SQLGETFUNCTIONS:
            handle = va_arg(marker, void *);
            traceShort[0] = (i2)va_arg(marker, i4);
            charPtr[0] = va_arg(marker, SQLCHAR *);
            STprintf (szTrace, traceFormat, handle, traceShort[0],
                charPtr[0]);
            break;
        case 0:  /* SQL API function was not found in the table; ie UNKNOWN */
            STprintf (szTrace, traceFormat, iApi);  /* display unknown fn id */
            break;
        default: /* SQL API function in table but parms not yet traced */
            szTrace[0] = '\0';
            break;
        } /* switch (API[i].iApi) */
        TRdisplay ((char *)TRACE_FMT, API[i].szName, szTrace);
    } /* if (API[i].szFmt && i <= API_MAX) */
    va_end(marker);
    return;
}


#if defined(UNIX) || defined(VMS)
/*
**  UtPrint
**
**  Writes a line to the trace file.  Stubbed out for Unix and VMS.
**
**  On entry: A variable number of parms in the same form as fprintf.
**
**  Returns:  Nothing.
*/
void UtPrint (LPCSTR szFormat, ...)
{
    return;
}


/*
**  UtSnap
**
**  Utility function to snap memory to the trace file.  Stubbed out for Unix.
**
**  Input:
**    Pointer to a title to prefix the dump.
**    Pointer to the area to dump.
**    Length of the area to dump.
**
**  Output:
**    The Hexadecimal representation of the area is written to
**    the trace file.
**
**  Returns:  Nothing.
*/
void  UtSnap(
        LPCSTR  pTitle,                 /* snap title                 */
        const void     * pArea,         /* snap area                  */
        long    nLength)                /* snap length                */
{
    return;
}

#endif /* UNIX || VMS */

/*
** Name: InitTrace
**
** Description:
**     This function initializes the tracing capability in the ODBC driver.
**
** Return value:
**     none
**
** Re-eneventcy:
**	This function does not update shared memory.
**
** History:
**     23-apr-2004 (loera01)
**        Created.
*/

void InitTrace( )
{
    char	*retVal;
    STATUS	status;
    CL_ERR_DESC	err_code;
    i4  i4val = 0;
    
    NMgtAt( "II_ODBC_TRACE", &retVal );

    if ( retVal  &&  *retVal )
    {
	CVal( retVal, &i4val);
        IItrace.fTrace  = (UWORD)i4val ;
    }	  
    else 
        IItrace.fTrace = 0;

    NMgtAt( "II_ODBC_LOG", &retVal );

    if ( retVal  &&  *retVal  &&  (IItrace.odbc_trace_file = (char *)
        MEreqmem( 0, STlength(retVal) + 16, FALSE, &status )) )
	{
        STcopy( retVal, IItrace.odbc_trace_file );
        TRset_file( TR_F_APPEND | TR_NOOVERRIDE_MASK, IItrace.odbc_trace_file, 
            (i4)STlength( IItrace.odbc_trace_file ), &err_code );
    }
    else
        IItrace.odbc_trace_file = NULL;
    
    return;
}




/*
** Name: TermTrace
**
** Description:
**     This function shuts down the tracing capability in the ODBC driver.
**
** Return value:
**     none
**
** Re-eneventcy:
**	This function does not update shared memory.
**
** History:
**     23-apr-2004 (loera01)
**        Created.
*/

void TermTrace( )
{
    CL_ERR_DESC	    err_code;
    
	if ( IItrace.odbc_trace_file )
	    TRset_file( TR_F_CLOSE, IItrace.odbc_trace_file, 
	        (i4)STlength( IItrace.odbc_trace_file ), &err_code );

	IItrace.fTrace = 0;
	IItrace.odbc_trace_file = NULL;
  
    return;
}
