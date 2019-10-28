/**
**    Copyright (c) 2010-2011 Actian Corporation.  All rights reserved.
**
**	Actian Tools API interface
**
**	This header defines the TOOLSAPI function interface, and the DLL
**	entrypoints.  It is the only header that clients of the API are
**	required to include.
**
**	It is patterned after the example of the Java Native Interface (jni.h),
**	providing a basic C function-table interface for the TOOLSAPI instance
**	(comparable to JNIEnv), with a C++ wrapper for the convenience of C++
**	clients.  The C++ wrapper simply allows programmers to write:
**
**	    inst->IDname()
**
**	in C++ instead of:
**
**	    (*inst)->IDname(inst)
**
**	in C.
**
#*    History:
#*	13-Apr-2010 (whiro01)
#*	    Created from original POC code.  Using ideas found in
#*	    jni.h as part of the Java language native interface.
#*	16-Jun-2010 (whiro01)
#*	    Created separate "datatypes.h" for the stuff commonly needed
#*	    both internally and externally.
#*	21-Jun-2010 (whiro01)
#*	    Completely redone because it didn't work the way I had it
#*	    originally.  With comments from reviewers, we now have an
#*	    interface that works.  The stumbling block was that there
#*	    was a difference between the way the C compiler implemented
#*	    calls to C++ member functions vs. a similar call using straight
#*	    C (even with the extra "this" pointer simulated).  So, two
#*	    separate interfaces are needed.  This actually makes some
#*	    things simpler.
#*	07-Jul-2010 (whiro01)
#*	    Fixed definition of API_FUNC_EXTERN for non-Windows platforms.
#*	13-Jul-2010 (whiro01)
#*	    Add reference counting to the mix so that multiple loads
#*	    of the same interface won't crash when freeing the second one.
#*	02-Sep-2010 (dansa01)
#*	    Added some private helper methods for the Netu subset of the API.
#*	    Not all of the methods added here currently need access to the
#*	    instance pointer, but they are included now just to simplify
#*	    adding more functionality in the future (possibly in the area of
#*	    error handling and error reporting, where access to per-client
#*	    state may be helpful).
#*	09-Sep-2010 (dansa01)
#*	    Added NetuQueryResult class to buffer query result strings until
#*	    they can be transferred to the consolidated block for the API
#*	    caller.  The methods of this class return a status and do not
#*	    throw exceptions.
#*	16-Sep-2010 (dansa01)
#*	    Trying to be const-correct with the Netu subset this time.  Because
#*	    I expected to be using lots of old APIs that don't have const on
#*	    their char* input, I thought it would be simplest to just expose
#*	    our API the same way.  But now I realize that was a mistake because
#*	    it simply pushes the burden up to our caller.  Better to handle any
#*	    const_cast nuisance inside the toolsapi layer.  (And I discovered
#*	    that the old Ingres APIs have improved, even though the CL spec has
#*	    not been updated to reflect that, and there really wasn't much
#*	    const_cast'ing required after all.)
#*	24-Sep-2010 (dansa01)
#*	    Yeah, my C++ is rusty.  I blew the signatures of the private copy
#*	    constructor and assignment operator of the NetuQueryResult class.
#*	08-Oct-2010 (dansa01)
#*	    Refactored, renamed, and generalized NetuQueryResult class to buffer
#*	    multirow string output from any source, not just Netu queries
#*	    (for example, NMsymbols or PMscan).  Now named StrQryResult.
#*	02-Nov-2010 (dansa01)
#*	    Converted to a more JNI-like TOOLSAPI interface.  (Preprocessing of
#*	    this template is currently disabled, but may be reactivated in the
#*	    future to ensure that all aspects of the TOOLSAPI_VT structure,
#*	    TOOLSAPI_CPP class, and the vtable pointer initializations stay
#*	    precisely in sync.  For now, we just copy toolsapi.template to
#*	    toolsapi.h.)
#*	09-Nov-2010 (dansa01)
#*	    Modified GetToolsAPIInstance signature to return an error status,
#*	    which will become necessary in the future when we have more failure
#*	    modes than simply OutOfMemory (which will be the case after we split
#*	    the DLL into Core, Client, and Server partitions, and/or we support
#*	    C-API remoting).
#*	15-Nov-2010 (dansa01)
#*	    Changed getLastError to getLastErrors, and changed the signature to
#*	    return a pointer to an II_ERRORS block.
#*	23-Nov-2010 (dansa01)
#*	    Beginning the conversion to a more uniform signature model, where
#*	    (almost) every ToolsAPI function will return an i4 status.  Starting
#*	    with the OpenAPI wrapper functions first.
#*	29-Nov-2010 (dansa01)
#*	    Continuing the conversion to a more uniform signature model, now
#*	    with the PM wrapper functions.
#*	02-Dec-2010 (dansa01)
#*	    Completed conversion to a more uniform signature model, and replaced
#*	    size_t with i_size to protect against future portability or remoting
#*	    complications.
#*	03-Jan-2011 (whiro01)
#*	    Removed unneeded connection handle from autoCommitOff signature.
#*	09-Feb-2011 (dansa01)
#*	    Added optional connection parameters to the IIconnectDatabase
#*	    function signature.
#*	10-Feb-2011 (dansa01)
#*	    Added new IIexecuteProcedure function.
#*	18-Feb-2011 (dansa01)
#*	    Added new IIcommit and IIrollback functions.  Modified signatures
#*	    of IIquery and IIexecuteProcedure, because the transaction handle
#*	    is now both an input and an output.
#*	25-Feb-2011 (whiro01)
#*	    Added UTcreateDatabase, UTdestroyDatabase, UTstartIngres and UTstopIngres
#*	    functions.
#*	19-Apr-2011 (dansa01)
#*	    Added IIputParameters function to provide query parameter support
#*	    for the ToolsAPI query interface.  (IIquery already includes the
#*	    needParms flag, so this just makes that usable.)
#*	21-Apr-2011 (dansa01)
#*	    Removed obsolete NetuXxx functions.
#*	06-May-2011 (gordy)
#*	    Added GCusrpwd function to validate passwords.
#*	19-May-2011 (whiro01)
#*	    Get the TOOLS_API_VERSION again from the environment.
#*	06-Jun-2011 (gordy)
#*	    Add effective user parameter to UTdestroyDatabase.
#*	28-Jun-2011 (dansa01)
#*	    Added optional logfile parameter to executeUtility (and renamed
#*	    the entrypoint UTexecuteUtility).  Also added a logfile parameter
#*	    to UTstartIngres and UTstopIngres.  This represents just an
#*	    initial step toward providing real-time GUI feedback of the console
#*	    output from utility processes.
#*	29-Jun-2011 (dansa01)
#*	    Added optional logfile parameter to UTcreateDatabase and
#*	    UTdestroyDatabase.
#*	29-Jun-2011 (gordy)
#*	    Add row count parameters to IIgetColumns.
#*	17-Aug-2011 (whiro01)
#*	    Removed the create and destroy database functions which we're
#*	    no longer using.
#*	19-Aug-2011 (dansa01)
#*	    Removed obsolete UTexecuteUtility, UTstartIngres, UTstopIngres.
#*	20-Sep-2011 (whiro01)
#*	    Remove double ## now that this lives in Ingres code and isn't
#*	    being preprocessed in the Tools area.
#*	20-Sep-2011 (whiro01)
#*	    Rename the external symbol $(TOOLSAPI_VERSION) to just TOOLSAPIVERSION.
#*	21-Sep-2011 (whiro01)
#*	    Now including "version.properties" which is now a C header file.
#*	22-Aug-2011 (whiro01)
#*	    Using NT_GENERIC instead of WINDOWS for the platform tests.
#*	17-Oct-2011 (whiro01)
#*	    Added new GChostname function to use instead of PMhost sometimes.
#*	20-Oct-2011 (whiro01)
#*	    Add two new functions: "CRsetPMval" and "LGgetLogPaths".
*/
#ifndef _TOOLS_API_H_INCLUDED
#define _TOOLS_API_H_INCLUDED

#include "version.properties"

/*
** Current Version Number for this interface
*/
typedef enum API_VERSIONS {
	TOOLS_API_VERSION	= TOOLSAPIVERSION
/*
** Previous version numbers for backward compatibility
*/
	, TOOLS_API_VERSION_0	= 0
} API_VERSION;


/*
** Platform-dependent definitions
*/
#ifdef NT_GENERIC
#define TOOLSAPI_EXPORT __declspec(dllexport)
#define TOOLSAPI_IMPORT __declspec(dllimport)
#define TOOLSAPICALL __stdcall
#else	/* not NT_GENERIC */
#define TOOLSAPI_EXPORT
#define TOOLSAPI_IMPORT extern
#define TOOLSAPICALL
#endif

#ifdef _TOOLS_API_IMPLEMENTATION
#define TOOLSAPI_IMPORT_OR_EXPORT TOOLSAPI_EXPORT
#else
#define TOOLSAPI_IMPORT_OR_EXPORT TOOLSAPI_IMPORT
#endif

#include "datatypes.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
** TOOLSAPI instance declarations.
**
**	Viewed from C, the TOOLSAPI instance is simply a pointer to the "vtable"
**	struct containing function pointers to all the C API functions.
**
**	Viewed from C++, the TOOLSAPI instance is a class instance, which has as
**	its first member a pointer to the struct containing function pointers to
**	all the C API functions.
*/
struct TOOLSAPI_VT;
#ifdef __cplusplus
struct TOOLSAPI_CPP;
struct ToolsImplementation;
typedef TOOLSAPI_CPP TOOLSAPI;
#else
typedef const struct TOOLSAPI_VT *TOOLSAPI;
#endif


/*
** Toolsapi DLL entry points.
*/
TOOLSAPI_IMPORT_OR_EXPORT i4 TOOLSAPICALL ToolsAPIInit();
TOOLSAPI_IMPORT_OR_EXPORT void TOOLSAPICALL ToolsAPITerm();
TOOLSAPI_IMPORT_OR_EXPORT i4 TOOLSAPICALL GetToolsAPIInstance(API_VERSION version, const char *options, TOOLSAPI **ppInst);
TOOLSAPI_IMPORT_OR_EXPORT void TOOLSAPICALL FreeToolsAPIInstance(TOOLSAPI *inst);


struct TOOLSAPI_VT {
	i4 (TOOLSAPICALL * APIgetVersion)(TOOLSAPI *inst, const char **ppVers);
	i4 (TOOLSAPICALL * IIconnectNameServer)(TOOLSAPI *inst, const char *target, const char *user, const char *password, i4 timeout, II_HANDLE *pConnHandle);
	i4 (TOOLSAPICALL * IIconnectDatabase)(TOOLSAPI *inst, const char *target, const char *user, const char *password, i4 timeout, II_CONNECT_OPTIONS *pOptions, II_HANDLE *pConnHandle);
	i4 (TOOLSAPICALL * IIdisconnect)(TOOLSAPI *inst, II_HANDLE connHandle);
	i4 (TOOLSAPICALL * IIautoCommitOn)(TOOLSAPI *inst, II_HANDLE connHandle, II_HANDLE *pTranHandle);
	i4 (TOOLSAPICALL * IIautoCommitOff)(TOOLSAPI *inst, II_HANDLE tranHandle);
	i4 (TOOLSAPICALL * IIquery)(TOOLSAPI *inst, II_HANDLE connHandle, II_HANDLE *pTranHandle, const char *query, i4 needParms, II_HANDLE *pStmtHandle);
	i4 (TOOLSAPICALL * IIabort)(TOOLSAPI *inst, II_HANDLE connHandle);
	i4 (TOOLSAPICALL * IIcancel)(TOOLSAPI *inst, II_HANDLE stmtHandle);
	i4 (TOOLSAPICALL * IIclose)(TOOLSAPI *inst, II_HANDLE stmtHandle);
	i4 (TOOLSAPICALL * IIgetDescriptors)(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS **ppDescV);
	i4 (TOOLSAPICALL * IIgetColumns)(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS *pDescV, i2 rowCount, II_DATAVALUES **ppDataV, i2 *pRows);
	i4 (TOOLSAPICALL * IIgetQueryInfo)(TOOLSAPI *inst, II_HANDLE stmtHandle, II_QUERY_INFO *pqInfo);
	i4 (TOOLSAPICALL * IIconvertData)(TOOLSAPI *inst, II_DESCRIPTOR *srcDesc, II_DATAVALUE *srcData, II_DESCRIPTOR *dstDesc, II_DATAVALUE *dstData);
	void (TOOLSAPICALL * MEfree)(TOOLSAPI *inst, II_PTR ptr);
	i4 (TOOLSAPICALL * CMgetCharsetName)(TOOLSAPI *inst, char *buf, i_size len);
	i4 (TOOLSAPICALL * CMgetStdCharsetName)(TOOLSAPI *inst, char *buf, i_size len);
	i4 (TOOLSAPICALL * UTreadVersionRel)(TOOLSAPI *inst, char *buf, i_size len);
	i4 (TOOLSAPICALL * NMgtAt)(TOOLSAPI *inst, const char *name, char **ppValue);
	i4 (TOOLSAPICALL * NMgtAtBuf)(TOOLSAPI *inst, const char *name, char *buf, i_size len, i_size *pBytesUsed);
	i4 (TOOLSAPICALL * NMsymbols)(TOOLSAPI *inst, II_NAME_VALUE_PAIRS **ppPairs);
	i4 (TOOLSAPICALL * IDname)(TOOLSAPI *inst, char **ppName);
	i4 (TOOLSAPICALL * IDnameBuf)(TOOLSAPI *inst, char *buf, i_size len, i_size *pBytesUsed);
	i4 (TOOLSAPICALL * IDnameService)(TOOLSAPI *inst, char **ppName);
	i4 (TOOLSAPICALL * IDnameServiceBuf)(TOOLSAPI *inst, char *buf, i_size len, i_size *pBytesUsed);
	i4 (TOOLSAPICALL * ElevationRequired)(TOOLSAPI *inst);
	i4 (TOOLSAPICALL * ElevationRequiredWarning)(TOOLSAPI *inst);
	i4 (TOOLSAPICALL * IsElevationRequired)(TOOLSAPI *inst, i4 *pIsRequired);
	i4 (TOOLSAPICALL * CheckPrivilege)(TOOLSAPI *inst, const char *user, enum Privileges priv, i4 *pIsAllowed);
	II_ERRORS* (TOOLSAPICALL * getLastErrors)(TOOLSAPI *inst);
	i4 (TOOLSAPICALL * GCtcpIpPort)(TOOLSAPI *inst, const char *pin, i4 subport, char *pout, char *poutSymbolic);
	i4 (TOOLSAPICALL * PMinit)(TOOLSAPI *inst, II_HANDLE *pCtxHandle);
	i4 (TOOLSAPICALL * PMfree)(TOOLSAPI *inst, II_HANDLE ctxHandle);
	i4 (TOOLSAPICALL * PMget)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, const char **ppValue);
	i4 (TOOLSAPICALL * PMinsert)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, const char *value);
	i4 (TOOLSAPICALL * PMdelete)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey);
	i4 (TOOLSAPICALL * PMnumElem)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, i4 *pNum);
	i4 (TOOLSAPICALL * PMlowerOn)(TOOLSAPI *inst, II_HANDLE ctxHandle);
	i4 (TOOLSAPICALL * PMload)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *path);
	i4 (TOOLSAPICALL * PMwrite)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *path);
	i4 (TOOLSAPICALL * PMgetDefault)(TOOLSAPI *inst, II_HANDLE ctxHandle, i4 idx, const char **ppValue);
	i4 (TOOLSAPICALL * PMsetDefault)(TOOLSAPI *inst, II_HANDLE ctxHandle, i4 idx, const char *value);
	i4 (TOOLSAPICALL * PMhost)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char **ppHostName);
	i4 (TOOLSAPICALL * PMrestrict)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *regexp);
	i4 (TOOLSAPICALL * PMexpToRegExp)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmexp, const char **ppRegexp);
	i4 (TOOLSAPICALL * PMscan)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmexp, II_NAME_VALUE_PAIRS **ppPairs);
	i4 (TOOLSAPICALL * IIexecuteProcedure)(TOOLSAPI *inst, II_HANDLE connHandle, II_HANDLE *pTranHandle, II_DESCRIPTORS *parmDesc, II_DATAVALUES *parmData, II_HANDLE *pStmtHandle);
	i4 (TOOLSAPICALL * IIcommit)(TOOLSAPI *inst, II_HANDLE tranHandle);
	i4 (TOOLSAPICALL * IIrollback)(TOOLSAPI *inst, II_HANDLE tranHandle);
	i4 (TOOLSAPICALL * IIputParameters)(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS *parmDesc, II_DATAVALUES *parmData);
	i4 (TOOLSAPICALL * GCusrpwd)(TOOLSAPI *inst, const char *usr, const char *pwd);
	i4 (TOOLSAPICALL * GChostname)(TOOLSAPI *inst, char *name, i4 len, u_i2 fullyQualified);
	i4 (TOOLSAPICALL * CRsetPMval)(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, const char *value);
	i4 (TOOLSAPICALL * LGgetLogPaths)(TOOLSAPI *inst, i4 whichLog, const char *nodename, char path[][256], i4 *pNumPaths);
};

#ifdef __cplusplus
} /* extern "C" */
#endif


#ifdef __cplusplus
struct TOOLSAPI_CPP {
	const struct TOOLSAPI_VT *functions;
	ToolsImplementation *impl;

	i4 APIgetVersion(const char **ppVers) {
	    return functions->APIgetVersion(this, ppVers);
	}
	i4 IIconnectNameServer(const char *target, const char *user, const char *password, i4 timeout, II_HANDLE *pConnHandle) {
	    return functions->IIconnectNameServer(this, target, user, password, timeout, pConnHandle);
	}
	i4 IIconnectDatabase(const char *target, const char *user, const char *password, i4 timeout, II_CONNECT_OPTIONS *pOptions, II_HANDLE *pConnHandle) {
	    return functions->IIconnectDatabase(this, target, user, password, timeout, pOptions, pConnHandle);
	}
	i4 IIdisconnect(II_HANDLE connHandle) {
	    return functions->IIdisconnect(this, connHandle);
	}
	i4 IIautoCommitOn(II_HANDLE connHandle, II_HANDLE *pTranHandle) {
	    return functions->IIautoCommitOn(this, connHandle, pTranHandle);
	}
	i4 IIautoCommitOff(II_HANDLE tranHandle) {
	    return functions->IIautoCommitOff(this, tranHandle);
	}
	i4 IIquery(II_HANDLE connHandle, II_HANDLE *pTranHandle, const char *query, i4 needParms, II_HANDLE *pStmtHandle) {
	    return functions->IIquery(this, connHandle, pTranHandle, query, needParms, pStmtHandle);
	}
	i4 IIabort(II_HANDLE connHandle) {
	    return functions->IIabort(this, connHandle);
	}
	i4 IIcancel(II_HANDLE stmtHandle) {
	    return functions->IIcancel(this, stmtHandle);
	}
	i4 IIclose(II_HANDLE stmtHandle) {
	    return functions->IIclose(this, stmtHandle);
	}
	i4 IIgetDescriptors(II_HANDLE stmtHandle, II_DESCRIPTORS **ppDescV) {
	    return functions->IIgetDescriptors(this, stmtHandle, ppDescV);
	}
	i4 IIgetColumns(II_HANDLE stmtHandle, II_DESCRIPTORS *pDescV, i2 rowCount, II_DATAVALUES **ppDataV, i2 *pRows) {
	    return functions->IIgetColumns(this, stmtHandle, pDescV, rowCount, ppDataV, pRows);
	}
	i4 IIgetQueryInfo(II_HANDLE stmtHandle, II_QUERY_INFO *pqInfo) {
	    return functions->IIgetQueryInfo(this, stmtHandle, pqInfo);
	}
	i4 IIconvertData(II_DESCRIPTOR *srcDesc, II_DATAVALUE *srcData, II_DESCRIPTOR *dstDesc, II_DATAVALUE *dstData) {
	    return functions->IIconvertData(this, srcDesc, srcData, dstDesc, dstData);
	}
	void MEfree(II_PTR ptr) {
	    functions->MEfree(this, ptr);
	}
	i4 CMgetCharsetName(char *buf, i_size len) {
	    return functions->CMgetCharsetName(this, buf, len);
	}
	i4 CMgetStdCharsetName(char *buf, i_size len) {
	    return functions->CMgetStdCharsetName(this, buf, len);
	}
	i4 UTreadVersionRel(char *buf, i_size len) {
	    return functions->UTreadVersionRel(this, buf, len);
	}
	i4 NMgtAt(const char *name, char **ppValue) {
	    return functions->NMgtAt(this, name, ppValue);
	}
	i4 NMgtAtBuf(const char *name, char *buf, i_size len, i_size *pBytesUsed) {
	    return functions->NMgtAtBuf(this, name, buf, len, pBytesUsed);
	}
	i4 NMsymbols(II_NAME_VALUE_PAIRS **ppPairs) {
	    return functions->NMsymbols(this, ppPairs);
	}
	i4 IDname(char **ppName) {
	    return functions->IDname(this, ppName);
	}
	i4 IDnameBuf(char *buf, i_size len, i_size *pBytesUsed) {
	    return functions->IDnameBuf(this, buf, len, pBytesUsed);
	}
	i4 IDnameService(char **ppName) {
	    return functions->IDnameService(this, ppName);
	}
	i4 IDnameServiceBuf(char *buf, i_size len, i_size *pBytesUsed) {
	    return functions->IDnameServiceBuf(this, buf, len, pBytesUsed);
	}
	i4 ElevationRequired() {
	    return functions->ElevationRequired(this);
	}
	i4 ElevationRequiredWarning() {
	    return functions->ElevationRequiredWarning(this);
	}
	i4 IsElevationRequired(i4 *pIsRequired) {
	    return functions->IsElevationRequired(this, pIsRequired);
	}
	i4 CheckPrivilege(const char *user, enum Privileges priv, i4 *pIsAllowed) {
	    return functions->CheckPrivilege(this, user, priv, pIsAllowed);
	}
	II_ERRORS* getLastErrors() {
	    return functions->getLastErrors(this);
	}
	i4 GCtcpIpPort(const char *pin, i4 subport, char *pout, char *poutSymbolic) {
	    return functions->GCtcpIpPort(this, pin, subport, pout, poutSymbolic);
	}
	i4 PMinit(II_HANDLE *pCtxHandle) {
	    return functions->PMinit(this, pCtxHandle);
	}
	i4 PMfree(II_HANDLE ctxHandle) {
	    return functions->PMfree(this, ctxHandle);
	}
	i4 PMget(II_HANDLE ctxHandle, const char *pmkey, const char **ppValue) {
	    return functions->PMget(this, ctxHandle, pmkey, ppValue);
	}
	i4 PMinsert(II_HANDLE ctxHandle, const char *pmkey, const char *value) {
	    return functions->PMinsert(this, ctxHandle, pmkey, value);
	}
	i4 PMdelete(II_HANDLE ctxHandle, const char *pmkey) {
	    return functions->PMdelete(this, ctxHandle, pmkey);
	}
	i4 PMnumElem(II_HANDLE ctxHandle, const char *pmkey, i4 *pNum) {
	    return functions->PMnumElem(this, ctxHandle, pmkey, pNum);
	}
	i4 PMlowerOn(II_HANDLE ctxHandle) {
	    return functions->PMlowerOn(this, ctxHandle);
	}
	i4 PMload(II_HANDLE ctxHandle, const char *path) {
	    return functions->PMload(this, ctxHandle, path);
	}
	i4 PMwrite(II_HANDLE ctxHandle, const char *path) {
	    return functions->PMwrite(this, ctxHandle, path);
	}
	i4 PMgetDefault(II_HANDLE ctxHandle, i4 idx, const char **ppValue) {
	    return functions->PMgetDefault(this, ctxHandle, idx, ppValue);
	}
	i4 PMsetDefault(II_HANDLE ctxHandle, i4 idx, const char *value) {
	    return functions->PMsetDefault(this, ctxHandle, idx, value);
	}
	i4 PMhost(II_HANDLE ctxHandle, const char **ppHostName) {
	    return functions->PMhost(this, ctxHandle, ppHostName);
	}
	i4 PMrestrict(II_HANDLE ctxHandle, const char *regexp) {
	    return functions->PMrestrict(this, ctxHandle, regexp);
	}
	i4 PMexpToRegExp(II_HANDLE ctxHandle, const char *pmexp, const char **ppRegexp) {
	    return functions->PMexpToRegExp(this, ctxHandle, pmexp, ppRegexp);
	}
	i4 PMscan(II_HANDLE ctxHandle, const char *pmexp, II_NAME_VALUE_PAIRS **ppPairs) {
	    return functions->PMscan(this, ctxHandle, pmexp, ppPairs);
	}
	i4 IIexecuteProcedure(II_HANDLE connHandle, II_HANDLE *pTranHandle, II_DESCRIPTORS *parmDesc, II_DATAVALUES *parmData, II_HANDLE *pStmtHandle) {
	    return functions->IIexecuteProcedure(this, connHandle, pTranHandle, parmDesc, parmData, pStmtHandle);
	}
	i4 IIcommit(II_HANDLE tranHandle) {
	    return functions->IIcommit(this, tranHandle);
	}
	i4 IIrollback(II_HANDLE tranHandle) {
	    return functions->IIrollback(this, tranHandle);
	}
	i4 IIputParameters(II_HANDLE stmtHandle, II_DESCRIPTORS *parmDesc, II_DATAVALUES *parmData) {
	    return functions->IIputParameters(this, stmtHandle, parmDesc, parmData);
	}
	i4 GCusrpwd(const char *usr, const char *pwd) {
	    return functions->GCusrpwd(this, usr, pwd);
	}
	i4 GChostname(char *name, i4 len, u_i2 fullyQualified) {
	    return functions->GChostname(this, name, len, fullyQualified);
	}
	i4 CRsetPMval(II_HANDLE ctxHandle, const char *pmkey, const char *value) {
	    return functions->CRsetPMval(this, ctxHandle, pmkey, value);
	}
	i4 LGgetLogPaths(i4 whichLog, const char *nodename, char paths[][256], i4 *pNumPaths) {
	    return functions->LGgetLogPaths(this, whichLog, nodename, paths, pNumPaths);
	}
};
#endif /* __cplusplus */


#endif /* _TOOLS_API_H_INCLUDED */
