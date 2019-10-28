/**
** Copyright (c) 2010-2011 Actian Corporation.  All rights reserved.
**
** Description:
**	This header defines the implementation view of the TOOLSAPI interface.
**
** History:
**	28-Oct-2010 (dansa01)
**	    Initial coding, to implement the JNI-like interface approach.
**	15-Nov-2010 (dansa01)
**	    Added ErrorStack and integrated it into the ToolsImplementation.
**	23-Nov-2010 (dansa01)
**	    Beginning the conversion to a more uniform signature model, where
**	    (almost) every ToolsAPI function will return an i4 status.  Starting
**	    with the OpenAPI wrapper functions first.
**	29-Nov-2010 (dansa01)
**	    Continuing the conversion to a more uniform signature model, now
**	    with the PM wrapper functions.
**	02-Dec-2010 (dansa01)
**	    Completed conversion to a more uniform signature model, and replaced
**	    size_t with i_size in the external interface to protect against
**	    future portability or remoting complications.
**	03-Jan-2011 (whiro01)
**	    Removed unneeded connection handle from autoCommitOff signature.
**	04-Jan-2011 (dansa01)
**	    Moved all the detailed header includes out of toolsapi.cpp and into
**	    toolsapiimpl.h so that our ToolsImplementation methods can use
**	    Ingres types (ER_ARGUMENT, for example) in their call signatures.
**	04-Jan-2011 (dansa01)
**	    Removed temporary ER_MSGID definitions and added erta.h #include.
**	    Also added the g_langcode global and ToolsImplementation langcode,
**	    and the getMessageText method.
**	25-Jan-2011 (dansa01)
**	    Added a pushError overload that takes an ER_ARGUMENT array.
**	09-Feb-2011 (dansa01)
**	    Added optional connection parameters to the I_IIconnectDatabase
**	    function signature.
**	10-Feb-2011 (dansa01)
**	    Added new I_IIexecuteProcedure function.
**	18-Feb-2011 (dansa01)
**	    Added new IIcommit and IIrollback functions.  Modified signatures
**	    of IIquery and IIexecuteProcedure, because the transaction handle
**	    is now both an input and an output.
**	25-Feb-2011 (whiro01)
**	    Added UTcreateDatabase and UTdestroyDatabase functions;
**	    also UTstartIngres and UTstopIngres functions.
**	19-Apr-2011 (dansa01)
**	    Added I_IIputParameters function to provide query parameter support
**	    for the ToolsAPI query interface.  (I_IIquery already includes the
**	    needParms flag, so this just makes that usable.)
**	21-Apr-2011 (dansa01)
**	    Removed obsolete NetuXxx functions, and removed the netuConnHandle,
**	    netuTranHandle state variables from ToolsImplementation class.
**	06-May-2011 (gordy)
**	    Added GCusrpwd function to validate passwords.
**	06-Jun-2011 (gordy)
**	    Add effective user parameter to I_UTdestroyDatabase.
**	28-Jun-2011 (dansa01)
**	    Added optional logfile parameter to I_executeUtility (and renamed
**	    the entrypoint I_UTexecuteUtility).  Also added a logfile parameter
**	    to I_UTstartIngres and I_UTstopIngres.  This represents just an
**	    initial step toward providing real-time GUI feedback of the console
**	29-Jun-2011 (dansa01)
**	    Added optional logfile parameter to I_UTcreateDatabase and
**	    I_UTdestroyDatabase.
**	29-Jun-2011 (gordy)
**	    Add row count parameters to IIgetColumns.
**	12-Aug-2011 (dansa01)
**	    Removed some backend header references (sc.h, scf.h, scc.h, scs.h).
**	17-Aug-2011 (whiro01)
**	    Removed the create and destroy database functions which we're
**	    no longer using.
**	19-Aug-2011 (dansa01)
**	    Removed obsolete UTexecuteUtility, UTstartIngres, UTstopIngres.
**	22-Aug-2011 (whiro01)
**	    Using NT_GENERIC instead of WINDOWS for the platform tests.
**	26-Sep-2011 (whiro01)
**	    Now referencing "nmsyms.h" instead of "nmlocal.h" to allow
**	    building in the Ingres codelines; separately reference "s_list".
**	26-Sep-2011 (whiro01)
**	    It is "nmsym.h" not "nmsyms.h".
**	26-Sep-2011 (whiro01)
**	    Move "s_list" inside the extern "C".
**	11-Oct-2011 (whiro01)
**	    Removed some unneeded headers now that we don't call "scs_chk_priv";
**	    #include <nmsym.h> in later Ingres versions.
**	18-Oct-2011 (whiro01)
**	    Changed "int" parameters to PMsetDefault and PMgetDefault to be "i4";
**	    added "GChostname" function.
**	19-Oct-2011 (kiria01) b125861
**	    Tidy up windows NM exports
**	20-Oct-2011 (whiro01)
**	    Add two new functions: "CRsetPMval" and "LGgetLogPaths"; add some
**	    header files to support these functions.
**	21-Oct-2011 (whiro01)
**	    Remove now redundant (and wrong) reference to "s_list" for Ingres main.
**	24-Oct-2011 (whiro01)
**	    Further tweaking of the NM symbol stuff for "ingres10".
**	24-Oct-2011 (whiro01)
**	    Rearrange header file order for Ingres 9.x.
*/

#if defined(NT_GENERIC) || defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <string.h>
#include <stdio.h>

#define _TOOLS_API_IMPLEMENTATION
#include <compat.h>
// Conflict with "cstdlib" header
#undef bsearch
#include <iiapi.h>

#include "toolsapi.h"

#define UNREFERENCED(x)	(x)

extern "C" {
// Ingres-specific headers for structures, defines and such
#include <iicommon.h>
#include <cm.h>
#include <cs.h>
#include <cv.h>
#if ING_VER > 910
#include <ep.h>
#endif
#include <er.h>
#include <erdmf.h>
#include <erst.h>
#include <ex.h>
#include <gc.h>
#include <gca.h>
#include <gl.h>
#include <gv.h>
#include <id.h>
#include <lc.h>
#include <lo.h>
#include <me.h>
#include <nm.h>
#include <nmsym.h>
#include <pc.h>
#include <lg.h>
#include <cs.h>
#include <cx.h>
#include <pm.h>
#include <cr.h>
#include <si.h>
#include <st.h>
#include <tr.h>
#ifdef NT_GENERIC
__declspec(dllimport) STATUS	GCtcpip_port( char *, i4 , char *, char * );
#else
STATUS	BS_tcp_port( char *, i4 , char *, char * );
#endif
} /* extern "C" */

// <algorithm> and the "using namespace std" is needed for the min/max templates
// (except on Windows, where <windows.h> supplies equivalent macros which lead
// Microsoft to omit the standard template definitions from their <algorithm>).
#include <algorithm>
using namespace std;

#include <stdlib.h>
#include <string>

#include "erta.h"



/*
** Implementation functions.
**
**	These names are the same as the TOOLSAPI_VT member names, prefixed
**	with "I_" to avoid collisions with CL functions or others with names
**	matching our member names.
*/
extern "C" {
	i4 TOOLSAPICALL I_APIgetVersion(TOOLSAPI *inst, const char **ppVers);
	i4 TOOLSAPICALL I_IIconnectNameServer(TOOLSAPI *inst, const char *target, const char *user, const char *password, i4 timeout, II_HANDLE *pConnHandle);
	i4 TOOLSAPICALL I_IIconnectDatabase(TOOLSAPI *inst, const char *target, const char *user, const char *password, i4 timeout, II_CONNECT_OPTIONS *pOptions, II_HANDLE *pConnHandle);
	i4 TOOLSAPICALL I_IIdisconnect(TOOLSAPI *inst, II_HANDLE connHandle);
	i4 TOOLSAPICALL I_IIautoCommitOn(TOOLSAPI *inst, II_HANDLE connHandle, II_HANDLE *pTranHandle);
	i4 TOOLSAPICALL I_IIautoCommitOff(TOOLSAPI *inst, II_HANDLE tranHandle);
	i4 TOOLSAPICALL I_IIquery(TOOLSAPI *inst, II_HANDLE connHandle, II_HANDLE *pTranHandle, const char *query, i4 needParms, II_HANDLE *pStmtHandle);
	i4 TOOLSAPICALL I_IIabort(TOOLSAPI *inst, II_HANDLE connHandle);
	i4 TOOLSAPICALL I_IIcancel(TOOLSAPI *inst, II_HANDLE stmtHandle);
	i4 TOOLSAPICALL I_IIclose(TOOLSAPI *inst, II_HANDLE stmtHandle);
	i4 TOOLSAPICALL I_IIgetDescriptors(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS **ppDescV);
	i4 TOOLSAPICALL I_IIgetColumns(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS *pDescV, i2 rowCount, II_DATAVALUES **ppDataV, i2 *pRows);
	i4 TOOLSAPICALL I_IIgetQueryInfo(TOOLSAPI *inst, II_HANDLE stmtHandle, II_QUERY_INFO *pqInfo);
	i4 TOOLSAPICALL I_IIconvertData(TOOLSAPI *inst, II_DESCRIPTOR *srcDesc, II_DATAVALUE *srcData, II_DESCRIPTOR *dstDesc, II_DATAVALUE *dstData);
	void TOOLSAPICALL I_MEfree(TOOLSAPI *inst, II_PTR ptr);
	i4 TOOLSAPICALL I_CMgetCharsetName(TOOLSAPI *inst, char *buf, i_size len);
	i4 TOOLSAPICALL I_CMgetStdCharsetName(TOOLSAPI *inst, char *buf, i_size len);
	i4 TOOLSAPICALL I_UTreadVersionRel(TOOLSAPI *inst, char *buf, i_size len);
	i4 TOOLSAPICALL I_NMgtAt(TOOLSAPI *inst, const char *name, char **ppValue);
	i4 TOOLSAPICALL I_NMgtAtBuf(TOOLSAPI *inst, const char *name, char *buf, i_size len, i_size *pBytesUsed);
	i4 TOOLSAPICALL I_NMsymbols(TOOLSAPI *inst, II_NAME_VALUE_PAIRS **ppPairs);
	i4 TOOLSAPICALL I_IDname(TOOLSAPI *inst, char **ppName);
	i4 TOOLSAPICALL I_IDnameBuf(TOOLSAPI *inst, char *buf, i_size len, i_size *pBytesUsed);
	i4 TOOLSAPICALL I_IDnameService(TOOLSAPI *inst, char **ppName);
	i4 TOOLSAPICALL I_IDnameServiceBuf(TOOLSAPI *inst, char *buf, i_size len, i_size *pBytesUsed);
	i4 TOOLSAPICALL I_ElevationRequired(TOOLSAPI *inst);
	i4 TOOLSAPICALL I_ElevationRequiredWarning(TOOLSAPI *inst);
	i4 TOOLSAPICALL I_IsElevationRequired(TOOLSAPI *inst, i4 *pIsRequired);
	i4 TOOLSAPICALL I_CheckPrivilege(TOOLSAPI *inst, const char *user, enum Privileges priv, i4 *pIsAllowed);
	II_ERRORS* TOOLSAPICALL I_getLastErrors(TOOLSAPI *inst);
	i4 TOOLSAPICALL I_GCtcpIpPort(TOOLSAPI *inst, const char *pin, i4 subport, char *pout, char *poutSymbolic);
	i4 TOOLSAPICALL I_PMinit(TOOLSAPI *inst, II_HANDLE *pCtxHandle);
	i4 TOOLSAPICALL I_PMfree(TOOLSAPI *inst, II_HANDLE ctxHandle);
	i4 TOOLSAPICALL I_PMget(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, const char **ppValue);
	i4 TOOLSAPICALL I_PMinsert(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, const char *value);
	i4 TOOLSAPICALL I_PMdelete(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey);
	i4 TOOLSAPICALL I_PMnumElem(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, i4 *pNum);
	i4 TOOLSAPICALL I_PMlowerOn(TOOLSAPI *inst, II_HANDLE ctxHandle);
	i4 TOOLSAPICALL I_PMload(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *path);
	i4 TOOLSAPICALL I_PMwrite(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *path);
	i4 TOOLSAPICALL I_PMgetDefault(TOOLSAPI *inst, II_HANDLE ctxHandle, i4 idx, const char **ppValue);
	i4 TOOLSAPICALL I_PMsetDefault(TOOLSAPI *inst, II_HANDLE ctxHandle, i4 idx, const char *value);
	i4 TOOLSAPICALL I_PMhost(TOOLSAPI *inst, II_HANDLE ctxHandle, const char **ppHostName);
	i4 TOOLSAPICALL I_PMrestrict(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *regexp);
	i4 TOOLSAPICALL I_PMexpToRegExp(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmexp, const char **ppRegexp);
	i4 TOOLSAPICALL I_PMscan(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmexp, II_NAME_VALUE_PAIRS **ppPairs);
	i4 TOOLSAPICALL I_IIexecuteProcedure(TOOLSAPI *inst, II_HANDLE connHandle, II_HANDLE *pTranHandle, II_DESCRIPTORS *parmDesc, II_DATAVALUES *parmData, II_HANDLE *pStmtHandle);
	i4 TOOLSAPICALL I_IIcommit(TOOLSAPI *inst, II_HANDLE tranHandle);
	i4 TOOLSAPICALL I_IIrollback(TOOLSAPI *inst, II_HANDLE tranHandle);
	i4 TOOLSAPICALL I_IIputParameters(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS *parmDesc, II_DATAVALUES *parmData);
	i4 TOOLSAPICALL I_GCusrpwd(TOOLSAPI *inst, const char *usr, const char *pwd);
	i4 TOOLSAPICALL I_GChostname(TOOLSAPI *inst, char *name, i4 len, u_i2 fullyQualified);
	i4 TOOLSAPICALL I_CRsetPMval(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, const char *value);
	i4 TOOLSAPICALL I_LGgetLogPaths(TOOLSAPI *inst, i4 which, const char *nodename, char paths[][256], i4 *pNumPaths);

} /* extern "C" */


/*
** Global State
*/
extern II_PTR g_env_handle;
extern i4 g_langcode;
extern char *g_trace_file;
extern i4 g_trace_level;


/*
** ErrorStack
**
**	This class helps us maintain a stack of errors (which may help
**	reveal underlying causes better than a single error message).
*/
class ErrorStack {
private:
	typedef struct _ERROR_NODE {
	    struct _ERROR_NODE *next;
	    i4   type;
	    i4   code;
	    i4   sourceSize;  // includes EOS
	    i4   messageSize; // includes EOS
	    char buf[1];      // buffer for source and message strings
	} ERROR_NODE;

	ERROR_NODE *mainStackTop;
	ERROR_NODE *tempStackTop;
	i4 mainStackNodeCount;
	i4 mainStackStringSize;
	i4 freezeCount;
	bool reversePushOrder;
	bool outOfMemory;

	void clearAll();
	void pushMain(ERROR_NODE *node);
	void pushTemp(ERROR_NODE *node);
	ERROR_NODE* popTemp();

	// This class does not support copying or assignment.
	ErrorStack(const ErrorStack&);
	ErrorStack& operator=(const ErrorStack&);

public:
	ErrorStack();
	~ErrorStack();

	void clear();
	void push(i4 type, i4 code, const char *source, const char *message);
	void beginReversePushOrder();
	void endReversePushOrder();
	void freeze();
	void unfreeze();
	II_ERRORS *getErrors();
};


/*
** ToolsImplementation
**
**	This class provides the implementation view of the TOOLSAPI instance,
**	and it stores all the per-instance state.
**
**	It is declared as a struct instead of a class so that we don't have to
**	ifdef it for our C clients (an opaque ToolsImplementation pointer is
**	part of the TOOLSAPI instance, and therefore the type name is visible),
**	and because the public default for member accessibility is appropriate
**	here (since all our C-API entrypoints are logically "members" of this
**	ToolsImplementation class and should have access to the instance state).
**
**	(The IIapi env_handle and the ER langcode are actually global to the
**	DLL, but we include a copy in the instance state for convenience.)
*/
struct ToolsImplementation
{
	ErrorStack errStack;
	II_PTR env_handle;
	i4 langcode;
	char msgbuf[1500];

	ToolsImplementation();
	~ToolsImplementation();

	/*
	** These methods help isolate the rest of the ToolsAPI code
	** from the complexities of dealing with ERlookup.
	*/
	i4 pushError(i4 status, const char *source);
	i4 pushError(i4 status, const char *source, i4 num_params, ER_ARGUMENT *params);
	i4 pushNonAPIError(i4 status, const char *source, CL_ERR_DESC *pCLerr);
	void getMessageText(i4 status, CL_ERR_DESC *pCLerr, i4 num_params, ER_ARGUMENT *params);

	/*
	** The following simple inline wrappers provide consistency with the
	** way we use the pushError methods above, and they make the invocation
	** statements a bit simpler than they would be if we accessed the
	** ErrorStack directly from the rest of the ToolsAPI code.
	*/
	void clearErrors()
	{
	    errStack.clear();
	};

	void pushError(i4 type, i4 code, const char *source, const char *message)
	{
	    errStack.push(type, code, source, message);
	};

	void beginReversePushOrder()
	{
	    errStack.beginReversePushOrder();
	};

	void endReversePushOrder()
	{
	    errStack.endReversePushOrder();
	};

	void freezeErrorStack()
	{
	    errStack.freeze();
	};

	void unfreezeErrorStack()
	{
	    errStack.unfreeze();
	};

	II_ERRORS* getErrors()
	{
	    return errStack.getErrors();
	};

private:
	// This type does not support copying or assignment.
	ToolsImplementation(const ToolsImplementation&);
	ToolsImplementation& operator=(const ToolsImplementation&);
};


/*
** StrQryResult
**
**	This class helps us buffer multirow "query" output (of string and/or
**	varchar data) when we can't know the number of rows in advance, and
**	then helps us repackage the final result into a consolidated output
**	block for the API caller.  The methods do not throw any exceptions.
*/
class StrQryResult {
private:
	typedef struct _STRING_NODE {
	    struct _STRING_NODE *next;
	    i4                   charsLen;
	    char                 chars[1];
	} STRING_NODE;

	TOOLSAPI *inst;
	i4 count;
	i4 rowCount;
	i4 totalStringLength;
	STRING_NODE *listHead;
	STRING_NODE *listTail;
	STRING_NODE *nextOutput;

	// This class does not support copying or assignment.
	StrQryResult(const StrQryResult&);
	StrQryResult& operator=(const StrQryResult&);

public:
	StrQryResult(TOOLSAPI *inst);
	~StrQryResult();
	i4 getRowCount();
	i4 getTotalStringLength();
	i4 addVarchar(const void *pv);
	i4 addString(const char *pc, i4 stringLen);
	i4 validateForIteration(i4 expectedNumCols);
	i4 getCBuffer(size_t sizeofOuter, size_t sizeofInner, i4 cols, char **ppCBuf, char **ppStrings);
	char *copyNextToBuf(char *pBuf);
};


