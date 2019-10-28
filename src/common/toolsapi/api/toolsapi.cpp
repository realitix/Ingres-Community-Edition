/**
**    Copyright (c) 2010-2011 Actian Corporation.  All rights reserved.
**
**	Actian Tools API Implementation
**
**    History:
**	13-Apr-2010 (whiro01)
**	    Created from original POC code.  Using ideas found in
**	    jni.h as part of the Java language native interface.
**	16-Apr-2010 (whiro01)
**	    Added a token for the "GenHeaders" program to recognize
**	    as the place where new implementation function stubs
**	    should be inserted; moved a bunch of stuff from IIapi_jni.cpp
**	    into here since we're now making a C++ interface that is
**	    pretty much the same as the Java version, so the guts
**	    of hooking into Ingres has to be in here.
**	18-Jun-2010 (whiro01)
**	    Due to a technicality in the way that C++ actually passes
**	    the "this" pointer to a method (not really pushed on the stack)
**	    we have to explicitly declare an "_this" pointer (and use it
**	    instead of "this"!) to make things work.  Ugly, but true.
**	21-Jun-2010 (whiro01)
**	    Not enough!  We redid the entire structure to make a C
**	    interface and a C++ interface and went back to a single
**	    header file for both external callers and ourselves.
**	28-Jun-2010 (whiro01)
**	    Ingres version dependencies: CL_ERR_DESC is different
**	    pre-Ingres 9.3; the Elevation stuff is missing from 9.1.
**	30-Jun-2010 (whiro01)
**	    Adding more functions after initial success getting
**	    this all working.
**	07-Jul-2010 (whiro01)
**	    Getting code to work under OSX.
**	13-Jul-2010 (whiro01)
**	    Add reference counting to the mix so that multiple loads
**	    of the same interface won't crash when freeing the second one.
**	21-Jul-2010 (whiro01)
**	    Add more API functions.
**	27-Jul-2010 (whiro01)
**	    Changed the name of the Logging environment variables.
**	01-Sep-2010 (dansa01)
**	    Added a first pass at the Netu... subset of the API.
**	09-Sep-2010 (dansa01)
**	    Reworked the Netu subset to eliminate the possibility (however
**	    unlikely) of unhandled exceptions from STL objects.  Added a
**	    NetuQueryResult helper class, which buffers the query result
**	    in a simple list until we can repackage it into a consolidated
**	    block for the API caller.  The helper class methods always return
**	    a status code instead of throwing an exception.  Tried to tighten
**	    up conformance with Ingres Coding Standard by using i4 instead of
**	    int.  Also added the function header comment blocks that were
**	    missing from the first pass (although I cheated somewhat by
**	    pretending that "History" began today).
**	16-Sep-2010 (dansa01)
**	    Trying to be const-correct with the Netu subset this time.  Because
**	    I expected to be using lots of old APIs that don't have const on
**	    their char* input, I thought it would be simplest to just expose
**	    our API the same way.  But now I realize that was a mistake because
**	    it simply pushes the burden up to our caller.  Better to handle any
**	    const_cast nuisance inside the toolsapi layer.  (And I discovered
**	    that the old Ingres APIs have improved, even though the CL spec has
**	    not been updated to reflect that, and there really wasn't much
**	    const_cast'ing required after all.)  Because this change is so
**	    trivial, and so extensive, I did not make a duplicate history entry
**	    in every method comment header.
**	20-Sep-2010 (whiro01)
**	    Eliminate the explicit IIinitialize() and IIterminate() calls and
**	    associated env handles because we're doing that under the covers in
**	    APIinit() and APIterm().
**	23-Sep-2010 (dansa01)
**	    Surprisingly, OpenAPI DOES actually set a non-NULL output handle
**	    even when the IIapi_connect or IIapi_autocommit fails, so we must
**	    not interpret a non-NULL handle as any indication of success.  So
**	    I changed IIautoCommitOn and do_connect to check the status before
**	    trusting the output handle.  Also changed IIquery, even though I'm
**	    not sure if IIapi_query exhibits the same strange error behavior.
**	    It just seemed like the prudent and safe thing to do.
**	28-Sep-2010 (dansa01)
**	    My bad!  Please disregard my history comment from 23-Sep-2010,
**	    because it was based on shameful ignorance.  The handle behavior
**	    of OpenAPI is clearly documented on page 16 of the OpenAPI User
**	    Guide.  Today's change is an attempt to properly fulfill my
**	    obligation to release handle resources after a failed IIapi_connect,
**	    IIapi_query, or IIapi_autoCommit.
**	08-Oct-2010 (dansa01)
**	    Refactored, renamed, and generalized NetuQueryResult class to buffer
**	    multirow string output from any source, not just Netu queries
**	    (for example, NMsymbols or PMscan).  Now named StrQryResult.
**	14-Oct-2010 (dansa01)
**	    Replaced all methods having C++ STL-based input or output parameters
**	    with equivalent implementations using plain C-compatible structs.
**	02-Nov-2010 (dansa01)
**	    Converted to a more JNI-like interface model.  All entrypoints now
**	    have pure C linkage.  A C++ wrapper (in toolsapi.h) is provided for
**	    the convenience of C++ clients (just like they do in jni.h), but the
**	    underlying DLL interface is pure C.  (toolsapi.cpp is now created
**	    as a simple copy of toolsapi.impl instead of being preprocessed, but
**	    that could change in the future.)
**	03-Nov-2010 (dansa01)
**	    Porting fixes.  Put back "using namespace std" (needed for "min/max"
**	    on non-Windows platforms).  Also had to add (PTR) cast on the MEfree
**	    argument for the benefit of Ingres versions before 9.3.
**	09-Nov-2010 (dansa01)
**	    Modified GetToolsAPIInstance signature to return an error status,
**	    which will become necessary in the future when we have more failure
**	    modes than simply OutOfMemory (which will be the case after we split
**	    the DLL into Core, Client, and Server partitions, and/or we support
**	    C-API remoting).
**	15-Nov-2010 (dansa01)
**	    Added an ErrorStack class and began the process of using it as the
**	    foundation of our error reporting infrastructure.  This conversion
**	    process will be on-going for a while, because there are still many
**	    potential opportunities to provide more detailed error messages.
**	    Started to lay the groundwork for using the ER message facility,
**	    but the full utilization of ER is also an incomplete task at this
**	    time.
**	23-Nov-2010 (dansa01)
**	    Beginning the conversion to a more uniform signature model, where
**	    (almost) every ToolsAPI function will return an i4 status.  Starting
**	    with the OpenAPI wrapper functions first.
**	29-Nov-2010 (dansa01)
**	    Continuing the conversion to a more uniform signature model, now
**	    with the PM wrapper functions.  Also added more error handling.
**	02-Dec-2010 (dansa01)
**	    Completed conversion to a more uniform signature model, and added
**	    more error handling.  Also replaced size_t with i_size in the
**	    external interface to protect against future portability or remoting
**	    complications.  Removed unused stralloccpy function (because it
**	    utilized the C++ new operator unnecessarily and didn't handle the
**	    potential std::bad_alloc exception).
**	03-Jan-2011 (whiro01)
**	    Removed unneeded connection handle from autoCommitOff signature.
**	04-Jan-2011 (dansa01)
**	    Moved all the detailed header includes out of toolsapi.cpp and into
**	    toolsapiimpl.h so that our ToolsImplementation methods can use
**	    Ingres types (ER_ARGUMENT, for example) in their call signatures.
**	04-Jan-2011 (dansa01)
**	    Replaced simulated ERlookup with the real thing.  Reorganized the
**	    pushError methods a bit and added getMessageText method.  Also added
**	    langcode initialization during ToolsAPIInit.  NOTE that the current
**	    pushError methods do NOT support merge parameters (except implicitly
**	    from the CL_ERR_DESC), but the underlying getMessateText does.  We
**	    can add another flavor of pushError method in the future if we want
**	    to explicitly supply merge parameters.
**	14-Jan-2011 (dansa01)
**	    Changed the loop in wait_for_completion to ensure that we continue
**	    to loop on IIapi_wait until the completion status is observed.  (This
**	    eliminates the spurious "Warning" status we were always getting from
**	    IIcancel, and guarantees that we never free an async parameter block
**	    prematurely.  What we have now is almost identical to what we see in
**	    the ODBC driver code.)
**	17-Jan-2011 (dansa01)
**	    Reworked the IIcancel logic to treat the expected "Query cancelled"
**	    "Failure" status as a successful cancel from our point of view.
**	19-Jan-2011 (dansa01)
**	    Added more tracing of error conditions during ToolsAPIInit.
**	25-Jan-2011 (dansa01)
**	    Added NetuCheckQueryInfo and a new pushError overload to support
**	    checking the QueryInfo flags for possible errors after a Netu query.
**	09-Feb-2011 (dansa01)
**	    Added optional connection parameters to the I_IIconnectDatabase
**	    function signature.  Created new helper functions (setConnectParam,
**	    doConnect) to support this.
**	10-Feb-2011 (dansa01)
**	    Added new I_IIexecuteProcedure function.  Also had to change
**	    I_IIconvertData because it wasn't propagating the IIapi_convertData
**	    dv_null, dv_length result back to the caller's II_DATAVALUE struct.
**	    It turns out that IIapi_getDescriptors may actually return a status
**	    of IIAPI_ST_NO_DATA, but our Java getDescriptors API is not set up
**	    reflect that (unlike the getColumns method), so I'm trapping that
**	    here and returning SUCCESS with a zero-count descriptor struct.
**	18-Feb-2011 (dansa01)
**	    Added new I_IIcommit and I_IIrollback functions.  Modified I_IIquery
**	    and I_IIexecuteProcedure, because the transaction handle must now be
**	    both an input and an output.
**	25-Feb-2011 (whiro01)
**	    Added UTcreateDatabase, UTdestroyDatabase, UTstartIngres and UTstopIngres
**	    functions.
**	18-Mar-2011 (whiro01)
**	    Changed UTcreateDatabase so the "-l" flag is only passed if a collation
**	    name is given and the database is not to be Unicode-enabled.
**	19-Apr-2011 (dansa01)
**	    Added I_IIputParameters function to provide query parameter support
**	    for the ToolsAPI query interface.  (I_IIquery already includes the
**	    needParms flag, so this just makes that usable.)  Refactored the
**	    I_IIexecuteProcedure function to share the new doPutParms helper.
**	    Updated ApiReleaseString to 0.4.2/100.
**	21-Apr-2011 (dansa01)
**	    Removed obsolete NetuXxx functions and updated ApiReleaseString to
**	    0.4.3/100.
**	06-May-2011 (gordy)
**	    Added GCusrpwd function to validate passwords.
**	    Updated ApiReleaseString to 0.4.4/100.
**	01-Jun-2011 (whiro01)
**	    Calling ERinit differently.
**	06-Jun-2011 (gordy)
**	    Add effective user parameter to I_UTdestroyDatabase.
**	13-Jun-2011 (gordy)
**	    Add effUser to II_CREATEDB_OPTIONS.
**	28-Jun-2011 (dansa01)
**	    Added optional logfile parameter to I_executeUtility (and renamed
**	    the entrypoint I_UTexecuteUtility).  Also added a logfile parameter
**	    to I_UTstartIngres and I_UTstopIngres.  This represents just an
**	    initial step toward providing real-time GUI feedback of the console
**	    output from utility processes.  Updated ApiReleaseString to
**	    0.4.6/100.
**	29-Jun-2011 (dansa01)
**	    Added optional logfile parameter to I_UTcreateDatabase and
**	    I_UTdestroyDatabase, and bumped ApiReleaseString to 0.4.7/100.
**	29-Jun-2011 (gordy)
**	    Add row count parameters to IIgetColumns.
**	12-Aug-2011 (dansa01)
**	    Extracted scs_chk_priv from scf!scs!scssvc.c and made it static here,
**	    to remove the ToolsAPI dependency on Ingres server libraries.
**	12-Aug-2011 (whiro01)
**	    Add alternative to DB_OWN_MAXNAME, which isn't defined prior to
**	    Ingres 10.
**	17-Aug-2011 (whiro01)
**	    Removed the create and destroy database functions which we're
**	    no longer using.
**	18-Aug-2011 (whiro01)
**	    Change the "ApiReleaseString" to conform to our version string
**	    parsing now that we're returning this version for the API version.
**	19-Aug-2011 (dansa01)
**	    Removed obsolete UTexecuteUtility, UTstartIngres, UTstopIngres.
**	22-Aug-2011 (whiro01)
**	    Using NT_GENERIC instead of WINDOWS for the platform tests; redid
**	    some of the platform identifiers to match Ingres usage.
**	07-Oct-2011 (dansa01)
**	    Corrected my interpretation of IIAPI_ST_MESSAGE and IIAPI_ST_WARNING
**	    status returns.  Those mean that the function actually DID succeed,
**	    AND there is additional information on the error handle.  Now we
**	    push those additional messages onto the ErrorStack (causing them to
**	    be logged as a side-effect), and then convert the return status to
**	    IIAPI_ST_SUCCESS so that the normal results will be processed.  Our
**	    caller has the option to check the error stack for such additional
**	    messages after an IIAPI_ST_SUCCESS status return, but our JNI code
**	    currently chooses to ignore them (like ODBC).
**	17-Oct-2011 (whiro01)
**	    Make new function GChostname to use (instead of PMhost in some
**	    contexts) to get the real host name); calling PMlowerOn after default
**	    PMinit() because that's the way it's supposed to be done (according
**	    to iigenres, et. al.; change "int" param to PMsetDefault and
**	    PMgetDefault to "i4".
**	20-Oct-2011 (whiro01)
**	    Add two new functions: "CRsetPMval" and "LGgetLogPaths"; now using
**	    SystemCfgPrefix instead of the #define (SYSTEM_CFG_PREFIX).
*/

#include "toolsapiimpl.h"

#if defined(NT_GENERIC)
  #ifdef _M_IX86
    #define PLATFORM "int.w32"
  #else
    #ifdef _M_X64
      #define PLATFORM "a64.win"
    #endif
  #endif
#elif defined(DARWIN) || defined(OSX)
  #ifdef __x86_64__
    #define PLATFORM "int.osx"
  #else
    #define PLATFORM "int.osx"
  #endif
#elif defined(LINUX)
  #ifdef __x86_64__
    #define PLATFORM "a64.lnx"
  #else
    #define PLATFORM "int.lnx"
  #endif
#else
  #error Must define PLATFORM for this platform!
#endif
static const char *ApiReleaseString = "II 0.4.20 (" PLATFORM "/100) (" __DATE__ ")";

static const int NO_TIMEOUT = -1;
static const int CANCEL_TIMEOUT = 10;	/* 10 seconds to wait for statement to cancel */

static const II_HANDLE II_NULL_HANDLE = static_cast<II_HANDLE>(0);

const char *API = "ToolsAPI:";

//
// Required features of OpenAPI that we need:
//	IIAPI_VERSION_5 means we can only run with Ingres versions 9.x and above
//	(supports up to ANSI date/time)
// Note: we probably only need up to IIAPI_VERSION_4 (i8 support) -- not sure what
// Ingres version that corresponds to.
// OpenROAD 2006 uses Ingres 2.6 which only supports up to IIAPI_VERSION_3 (UCS-2 support)
//
#define REQUIRED_VERSION	IIAPI_VERSION_5


// Must agree with enum Privileges in "datatypes.h" and "IIapi.java"
static char *PrivilegeStrings[] = {
	"SERVER_CONTROL",
	"NET_ADMIN",
	"MONITOR",
	"TRUSTED"
};


/*
** The "vtable" of TOOLSAPI interface functions
*/
struct TOOLSAPI_VT toolsapivt = {
	I_APIgetVersion,
	I_IIconnectNameServer,
	I_IIconnectDatabase,
	I_IIdisconnect,
	I_IIautoCommitOn,
	I_IIautoCommitOff,
	I_IIquery,
	I_IIabort,
	I_IIcancel,
	I_IIclose,
	I_IIgetDescriptors,
	I_IIgetColumns,
	I_IIgetQueryInfo,
	I_IIconvertData,
	I_MEfree,
	I_CMgetCharsetName,
	I_CMgetStdCharsetName,
	I_UTreadVersionRel,
	I_NMgtAt,
	I_NMgtAtBuf,
	I_NMsymbols,
	I_IDname,
	I_IDnameBuf,
	I_IDnameService,
	I_IDnameServiceBuf,
	I_ElevationRequired,
	I_ElevationRequiredWarning,
	I_IsElevationRequired,
	I_CheckPrivilege,
	I_getLastErrors,
	I_GCtcpIpPort,
	I_PMinit,
	I_PMfree,
	I_PMget,
	I_PMinsert,
	I_PMdelete,
	I_PMnumElem,
	I_PMlowerOn,
	I_PMload,
	I_PMwrite,
	I_PMgetDefault,
	I_PMsetDefault,
	I_PMhost,
	I_PMrestrict,
	I_PMexpToRegExp,
	I_PMscan,
	I_IIexecuteProcedure,
	I_IIcommit,
	I_IIrollback,
	I_IIputParameters,
	I_GCusrpwd,
	I_GChostname,
	I_CRsetPMval,
	I_LGgetLogPaths
};


/*
** The "vtable" of TOOLSAPI proxy functions
**
**	When/if we implement remoting at the C API layer, we will have
**	an alternative vtable that consists of remoting proxy entrypoints.
**	The GetToolsAPIInstance function will set up the TOOLSAPI struct
**	to point to the primary vtable or the proxy vtable depending on
**	whether or not a remote connection is requested.
**
struct TOOLSAPI_VT toolsapivtp = {
	IP_APIgetVersion,
	IP_IIconnectNameServer,
	IP_IIconnectDatabase,
	IP_IIdisconnect,
	. . .
*/


/*
** DLL global state
*/
II_PTR g_env_handle = 0;
i4     g_langcode = 0;
char  *g_trace_file = NULL;
i4     g_trace_level = 0;



/*{
** Name: ToolsAPIInit - Initialize the Toolsapi DLL
**
** Description:
**	External entrypoint to Toolsapi DLL.
**
**	This function handles the initialization of the Toolsapi DLL.  It must
**	be called once and only once by the client before using any other
**	Toolsapi functions.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	Nonzero if initialization failed.
**
** Side Effects:
**	Sets DLL-global variables.
**
** History:
**	01-Nov-2010 (dansa01)
**	    Adapted from CPPToolsInterface::APIinit().
**	04-Jan-2011 (dansa01)
**	    Added ER initialization, using ER_MSGTEST so that we can make the
**	    ToolsAPI mnx file a separate deliverable from the Ingres mnx file.
**	    (We are following the example of IVM here, with respect to the
**	    ER_SEMAPHORE and ER_MU_SEMFUNCS flags, since ToolsAPI, like IVM,
**	    is a multi-threaded frontend.)
**	19-Jan-2011 (dansa01)
**	    Added tracing calls on all error exits.
*/
i4 TOOLSAPICALL
ToolsAPIInit()
{
	char *value;
	i4 status;
	CL_ERR_DESC err;

	// Ingres initialization
	::MEadvise(ME_INGRES_ALLOC);

	status = ::CMset_charset(&err);
	if (status != OK)
	{
	    TRdisplay("%s %@ CMset_charset failed. Status = %d\n", API, status);
	    return status;
	}

	// As long as we deliver a separate s266.mnx file for our "TA" messages
	// we need the ER_MSGTEST mode.
	::ERinit(ER_SEMAPHORE | ER_MSGTEST, NULL, NULL, NULL, NULL);

	status = ::ERlangcode(NULL, &g_langcode);
	if (status != OK)
	{
	    TRdisplay("%s %@ ERlangcode failed. Status = %d\n", API, status);
	    return status;
	}

	status = ::PMinit();
	if (status != OK)
	{
	    TRdisplay("%s %@ PMinit failed. Status = %d\n", API, status);
	    return status;
	}

	::PMlowerOn();

	status = ::PMload( (LOCATION *)NULL, (PM_ERR_FUNC *)NULL );
	if (status != OK)
	{
	    TRdisplay("%s %@ PMload(NULL, NULL) failed. Status = %d\n", API, status);
	    return status;
	}

	status = ::PMsetDefault( 0, SystemCfgPrefix );
	if (status != OK)
	{
	    TRdisplay("%s %@ PMsetDefault(0, SystemCfgPrefix) failed. Status = %d\n", API, status);
	    return status;
	}

	status = ::PMsetDefault( 1, ::PMhost() );
	if (status != OK)
	{
	    TRdisplay("%s %@ PMsetDefault(1, PMhost()) failed. Status = %d\n", API, status);
	    return status;
	}

	// OpenAPI initialization
	IIAPI_INITPARM  initParm;

	initParm.in_version = REQUIRED_VERSION;
	initParm.in_timeout = (II_LONG)-1;

	IIapi_initialize( &initParm );
	if ( initParm.in_status != IIAPI_ST_SUCCESS )
	{
	    TRdisplay("%s %@ IIapi_initialize failed. Status = %d\n", API, initParm.in_status);
	    return initParm.in_status;
	}

	g_env_handle = initParm.in_envHandle;

	// Setup our tracing based on II_TOOLS_LOGGING
	// Note: we do it here so that our trace file is the active
	// one in case GCA and/or API tracing is also turned on
	::NMgtAt( "II_TOOLS_LOGGING", &value );
	if ( ! (value && *value)  &&
	    ::PMget( "!.tools_logging", &value ) != OK )
		value = NULL;
	if ( value && *value )
	{
	    char num[30];
	    int i;
	    // Syntax should be: "level,filename"
	    for (i = 0; i < sizeof(num)-1 && value[i] >= '0' && value[i] <= '9'; i++)
		num[i] = value[i];
	    num[i] = '\0';
	    CVal( num, &g_trace_level );
	    while (value[i] == ' ' || value[i] == '\t')
		i++;
	    if (value[i] == ',')
		i++;
	    while (value[i] == ' ' || value[i] == '\t')
		i++;
	    value = &value[i];
	}

	if ( value  &&  *value  &&
	    (g_trace_file = reinterpret_cast<char *>(
		     MEreqmem( 0, STlength(value) + 16, FALSE, &status ))) != NULL )
	{
		STprintf( g_trace_file, value /*, init_count++ */ );
		TRset_file( TR_F_OPEN, g_trace_file,
			    STlength( g_trace_file ), &err );
	}
	return 0;
}


/*{
** Name: ToolsAPITerm - Shut down the Toolsapi DLL
**
** Description:
**	External entrypoint to the Toolsapi DLL.
**
**	This function handles the termination of the Toolsapi DLL.  It must
**	be called once and only once by the client after releasing all TOOLSAPI
**	instances.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Sets DLL-global variables.
**
** History:
**	01-Nov-2010 (dansa01)
**	    Adapted from CPPToolsInterface::APIterm().
**	04-Jan-2011 (dansa01)
**	    Added ERclose.
*/
void TOOLSAPICALL
ToolsAPITerm()
{
	IIAPI_RELENVPARM relEnvParm;
	IIAPI_TERMPARM termParm;
	CL_ERR_DESC err;

	relEnvParm.re_envHandle = g_env_handle;
	IIapi_releaseEnv( &relEnvParm );
	if (relEnvParm.re_status != IIAPI_ST_SUCCESS)
	{
	    TRdisplay("%s %@ IIapi_releaseEnv failed. Status=%d\n", API, relEnvParm.re_status);
	}

	IIapi_terminate( &termParm );
	if (termParm.tm_status != IIAPI_ST_SUCCESS)
	{
	    TRdisplay("%s %@ IIapi_terminate failed. Status=%d\n", API, termParm.tm_status);
	}

	// Other Ingres termination stuff
	::PMfree();

	// Close trace file -- IIapi_terminate may have already done it if API tracing
	// is also enabled.
	if ( g_trace_file ) {
	    TRset_file( TR_F_CLOSE, g_trace_file,
			STlength( g_trace_file ), &err );
	    ::MEfree(g_trace_file);
	}

	::ERclose();

	g_trace_level = 0;
	g_trace_file = NULL;
}


/*{
** Name: GetToolsAPIInstance - Allocate a TOOLSAPI instance
**
** Description:
**	External entrypoint to the Toolsapi DLL.
**
**	This function allocates and outputs a TOOLSAPI instance, which should
**	later be freed by calling FreeToolsAPIInstance.
**
** Inputs:
**	version		The minimum required API_VERSION.
**	options		For future extensions (such as a remote server address
**			when/if we support remoting at the C API layer).
**			Can be NULL.
**	ppInst		Address of location to receive instance pointer.
**			Must not be NULL.
**
** Outputs:
**	*ppInst		Receives a pointer to the new TOOLSAPI instance.
**
** Returns:
**	Status.
**
** Side Effects:
**	None.
**
** History:
**	01-Nov-2010 (dansa01)
**	    Initial coding.
**	09-Nov-2010 (dansa01)
**	    Modified signature to return an error status, which will become
**	    necessary when we have more failure modes than simply OutOfMemory.
*/
i4 TOOLSAPICALL
GetToolsAPIInstance(API_VERSION version, const char *options, TOOLSAPI **ppInst)
{
	UNREFERENCED(version);
	UNREFERENCED(options);

	//TODO: Implement version checking

	i4 status = II_ST_SUCCESS;
	ToolsImplementation *impl = NULL;
	TOOLSAPI *inst = NULL;

	try
	{
	    impl = new ToolsImplementation();
	    inst = new TOOLSAPI();
	}
	catch (...)
	{
	}

	if (inst && impl)
	{
	    inst->functions = &toolsapivt;
	    inst->impl = impl;
	    *ppInst = inst;
	}
	else
	{
	    delete impl;
	    delete inst;
	    status = II_ST_OUT_OF_MEMORY;
	}

	return status;
}


/*{
** Name: FreeToolsAPIInstance - Free a TOOLSAPI instance
**
** Description:
**	External entrypoint to the Toolsapi DLL.
**
**	This function frees a TOOLSAPI instance previously allocated
**	by GetToolsAPIInstance.
**
** Inputs:
**	inst		The TOOLSAPI instance to free.
*
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	None.
**
** History:
**	01-Nov-2010 (dansa01)
**	    Initial coding.
*/
void TOOLSAPICALL
FreeToolsAPIInstance(TOOLSAPI *inst)
{
	if (inst)
	{
	    delete inst->impl;
	    delete inst;
	}
}


/*
** Name: ToolsImplementation - TOOLSAPI internal implementation class
**
** Description:
**	This class provides the implementation view of the TOOLSAPI instance,
**	and it stores all the per-instance state.
**
**	These members are public because all our C-API entrypoints are logically
**	"members" of this ToolsImplementation class and should have full access
**	to the instance state.
**
**	(The IIapi env_handle and the ER langcode are actually global to the
**	DLL, but we include a copy of them in the instance state just for
**	convenience.)
**
** History:
**	15-Nov-2010 (dansa01)
**	    Initial documentation, and significant rework of existing class.
*/

/*{
** Name: ToolsImplementation::ToolsImplementation - Constructor
**
** Description:
**	No-argument constructor initializes with no allocations and
**	cannot fail.
**
** History:
**	15-Nov-2010 (dansa01)
**	    Converted from existing implementation.
**	21-Apr-2011 (dansa01)
**	    Removed obsolete netu connection handle and transaction handle.
*/
ToolsImplementation::ToolsImplementation()
:   env_handle(g_env_handle),
    langcode(g_langcode)
{
}

/*{
** Name: ToolsImplementation::~ToolsImplementation - Destructor
**
** Description:
**	This destructor doesn't have much to do, as the ErrorStack destructor
**	is invoked implicitly.
**
** History:
**	15-Nov-2010 (dansa01)
**	    Converted from existing implementation.
*/
ToolsImplementation::~ToolsImplementation()
{
}


/*{
** Name: ToolsImplementation::pushError - Push an error status
**
** Description:
**	This method accepts an error status code and a source string, looks up
**	the message text associated with that status code and then pushes the
**	error message onto the ErrorStack.
**
** Inputs:
**	status		An OpenAPI status or unique ER_MSGID code.
**	source		A string identifying the source of the error (usually
**			the name of the function calling this method).
**
** Outputs:
**	None.
**
** Returns:
**	The status code that was passed in (for convenience).
**
** Side Effects:
**	Pushes an entry onto the ErrorStack.
**
** History:
**	15-Nov-2010 (dansa01)
**	    Initial coding.
**	04-Jan-2011 (dansa01)
**	    Adapted to use the real ERlookup via the getMessageText method.
*/
i4
ToolsImplementation::pushError(i4 status, const char *source)
{
	getMessageText(status, NULL, 0, NULL);
	pushError(II_GE_TOOLSAPI, status, source, msgbuf);

	return status;
}


/*{
** Name: ToolsImplementation::pushError - Push an error status
**
** Description:
**	This overload is the same as the pushError above, except that it accepts
**	an array of ER_ARGUMENT parameters to be merged into the message.
**
** Inputs:
**	status		An OpenAPI status or unique ER_MSGID code.
**	source		A string identifying the source of the error (usually
**			the name of the function calling this method).
**	num_params	Number of explicit merge parameters.
**	params		Pointer to an array of ER_ARGUMENT parameter structs.
**
** Outputs:
**	None.
**
** Returns:
**	The status code that was passed in (for convenience).
**
** Side Effects:
**	Pushes an entry onto the ErrorStack.
**
** History:
**	25-Jan-2011 (dansa01)
**	    Initial coding.
*/
i4
ToolsImplementation::pushError(i4 status, const char *source, i4 num_params, ER_ARGUMENT *params)
{
	getMessageText(status, NULL, num_params, params);
	pushError(II_GE_TOOLSAPI, status, source, msgbuf);

	return status;
}


/*{
** Name: ToolsImplementation::pushNonAPIError - Push a non-API error status
**
** Description:
**	This method is used when the status code is NOT from the ToolsAPI,
**	OpenAPI, status "namespace" (such as a CL status return code).
**	Besides handling an optional CL_ERR_DESC pointer, the purpose of this
**	method is to deal with potential conflicts between CL status codes and
**	ToolsAPI status codes.
**
**	The ToolsAPI "namespace" of status codes includes the union of the
**	OpenAPI status codes (0 through 8), plus all Ingres ER_MSGID numbers
**	(EXCEPT for message class zero, the E_USxxx messages, which we never
**	expect to see as a status return code from any functions outside of
**	the server).
**
**	MOST of the CL status codes are legitimate ER_MSGID numbers, so they
**	fall nicely into this same namespace.  HOWEVER, some old (and some
**	not-so-old) Ingres code tends to get sloppy and return "FAIL" as a
**	status code.  Unfortunately, "FAIL" is #defined as 1 in compat.h, and
**	that value conflicts with the II_ST_MESSAGE value.  So we have to
**	filter out any FAIL status codes and map them to a generic II_ST_FAILURE
**	code before we fetch a message string and push it.
**
**	This method also uses the CL_ER_DESC (if supplied) when calling
**	ERlookup, to obtain the most meaningful message text possible.
**
** Inputs:
**	status		An Ingres CL status code.
**	source		A string identifying the source of the error.
**	pCLerr		Optional pointer to CL_ERR_DESC containing
**			platform-specific details.
**
** Outputs:
**	None.
**
** Returns:
**	The resolved status code, after mapping into the ToolsAPI "namespace".
**
** Side Effects:
**	Pushes an entry onto the ErrorStack.
**
** History:
**	15-Nov-2010 (dansa01)
**	    Initial coding.
**	24-Nov-2010 (dansa01)
**	    Renamed and repurposed to deal with the dreaded "FAIL" status code
**	    in addition to an optional CL_ERR_DESC.
**	04-Jan-2011 (dansa01)
**	    Adapted to use the real ERlookup via the getMessageText method.
*/
i4
ToolsImplementation::pushNonAPIError(i4 status, const char *source, CL_ERR_DESC *pCLerr)
{
	//
	// Since this method is called ONLY when the status did NOT come from
	// OpenAPI or ToolsAPI, we assume that anything below the range of an
	// Ingres CL ER_MSGID (such as "FAIL") is a non-specific failure code.
	// (We also map -1, just to be proactive, but I haven't yet seen any
	// example of -1 coming out of an Ingres function.)
	//
	if (status < 0x00010000 || status == -1)
	{
	    status = II_ST_FAILURE;
	}

	getMessageText(status, pCLerr, 0, NULL);
	pushError(II_GE_TOOLSAPI, status, source, msgbuf);

	return status;
}


/*{
** Name: ToolsImplementation::getMessageText - Handles ERlookup calls
**
** Description:
**	This method handles the rather complex ERlookup calling sequence
**	(and even more complex ERlookup failure handling).
**
**	In theory, explicit merge parameters are supported, but we are not yet
**	using that capability.
**
**	NOTE that if you call this with a zero merge-parameter count, the raw
**	message text will be obtained, regardless of whether there are format
**	instructions in that message.  (ERlookup would normally treat a
**	mismatch between the number of format instructions and the number of
**	provided parameters as an error.)  This gives us a safety fall-back
**	in case we encounter a status code that has format instructions in its
**	associated message, and we don't know what parameters are appropriate
**	for it.  This is mostly a safety precaution, because the CL/GL messages
**	that contain format instructions are likely to arise only in conjuntion
**	with a CL_ERR_DESC, which implicitly provides the needed parameters.
**
** Inputs:
**	status		A ToolsAPI status code.
**	pCLerr		Optional pointer to CL_ERR_DESC.
**	num_params	Number of explicit merge parameters.
**	params		Pointer to an array of ER_ARGUMENT parameter structs.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Places error text into the ToolsImplementation msgbuf member.  No status
**	is returned.  If the ERlookup fails, the buffer will contain text
**	describing the ERlookup failure instead of the input status.
**
** History:
**	04-Jan-2011 (dansa01)
**	    Initial coding, based on examples in the online CL spec (and based
**	    on reading the ERlookup source code, because the ER_NOPARAM flag was
**	    never added to the online CL spec).
*/
void
ToolsImplementation::getMessageText(i4 status, CL_ERR_DESC *pCLerr, i4 num_params, ER_ARGUMENT *params)
{
	//
	// First, map any OpenAPI status codes to text messages.  It is a bit
	// unfortunate that we have a "mixed namespace" of OpenAPI status codes
	// (0-8), plus unique ER_MSGID codes (excluding message class zero, the
	// E_USxxx codes).  But allowing the OpenAPI status codes to be part
	// of our "namespace" seems to be the most practical compromise to fit
	// in with our existing code, and with our heavy usage of OpenAPI calls.
	//
	// (Although there exist messages for II_ST_SUCCESS and II_ST_NO_DATA,
	// we really don't expect callers to be passing those in.)
	//
	i4 msgid;
	switch (status)
	{
	    case II_ST_SUCCESS:
		msgid = E_TA0000_ST_SUCCESS;
		break;
	    case II_ST_MESSAGE:
		msgid = E_TA0001_ST_MESSAGE;
		break;
	    case II_ST_WARNING:
		msgid = E_TA0002_ST_WARNING;
		break;
	    case II_ST_NO_DATA:
		msgid = E_TA0003_ST_NO_DATA;
		break;
	    case II_ST_ERROR:
		msgid = E_TA0004_ST_ERROR;
		break;
	    case II_ST_FAILURE:
		msgid = E_TA0005_ST_FAILURE;
		break;
	    case II_ST_NOT_INITIALIZED:
		msgid = E_TA0006_ST_NOT_INITIALIZED;
		break;
	    case II_ST_INVALID_HANDLE:
		msgid = E_TA0007_ST_INVALID_HANDLE;
		break;
	    case II_ST_OUT_OF_MEMORY:
		msgid = E_TA0008_ST_OUT_OF_MEMORY;
		break;
	    default:
		msgid = status;
		break;
	}

	i4 flags;
	i4 lookupStatus;
	i4 text_length;
	i4 currentLength;
	CL_ERR_DESC sys_err;
	CL_ERR_DESC ignore;

	if (!pCLerr)
	{
	    //
	    // No CL_ERR_DESC.  This is the simpler case.
	    //
	    // If no merge parameters are supplied, make sure we retrieve the raw
	    // message text regardless of whether it contains format instructions.
	    // (Also, omit the timestamp and message number from the text, because
	    // those are already provided at the higher levels of our error-
	    // reporting infrastructure.)
	    //
	    if (num_params == 0)
		flags = ER_TEXTONLY | ER_NOPARAM;
	    else
		flags = ER_TEXTONLY;

	    lookupStatus = ERlookup(msgid,
				    (CL_ERR_DESC *)NULL,
				    flags,
				    (i4 *)NULL,
				    msgbuf,
				    (i4)sizeof(msgbuf),
				    langcode,
				    &text_length,
				    &sys_err,
				    num_params,
				    params);
	    if (lookupStatus != OK)
	    {
		//
		// ERlookup failed.  Use some hard-wired text to say that, and then
		// try to append a real message describing the ERlookup failure code.
		// (We are realistically assuming here that our msgbuf member has plenty
		// of space for all our hard-wired failure messages, including the
		// possibly appended "failed twice" message.)
		//
		STprintf(msgbuf, "Couldn't look up message %08lx (reason: error %08lx)\n", msgid, lookupStatus);
		currentLength = STlength(msgbuf);

		lookupStatus = ERlookup(lookupStatus,
					&sys_err,
					(i4)0,
					(i4 *)NULL,
					&msgbuf[currentLength],
					(i4)(sizeof(msgbuf) - currentLength),
					langcode,
					&text_length,
					&ignore,
					0,
					(ER_ARGUMENT *)NULL );
	       if (lookupStatus != OK)
	       {
		   STprintf(&msgbuf[currentLength], "ERlookup failed twice (status = %08lx)\n", lookupStatus);
	       }
	    }
	}
	else
	{
	    //
	    // We have a CL_ERR_DESC, so follow the required two-phase calling sequence.
	    //
	    // Explicit merge parameters are not valid in this case.  Parameters (if any)
	    // come from the CL_ERR_DESC.  (But just to be safe, use ER_NOPARAM in the one
	    // case where we don't pass in a CL_ERR_DESC.)
	    //
	    if (CLERROR(msgid))
		flags = ER_TEXTONLY;
	    else
		flags = ER_TEXTONLY | ER_NOPARAM;

	    //
	    // PHASE ONE: Start by looking up the msgid (and IF it is in the CLERROR range,
	    // combine it with the CL_ERR_DESC to provide the necessary merge parameters).
	    //
	    // Limit the amount of buffer we can use in phase one to ensure that we can
	    // safely append all possible hard-wired failure messages after that.
	    //
	    i4 phaseOneSizeLimit = (i4)sizeof(msgbuf) - 120;

	    lookupStatus = ERlookup(msgid,
				    CLERROR(msgid) ? pCLerr : (CL_ERR_DESC *)NULL,
				    flags,
				    (i4 *)NULL,
				    msgbuf,
				    phaseOneSizeLimit,
				    langcode,
				    &text_length,
				    &sys_err,
				    0,
				    (ER_ARGUMENT *)NULL );
	    if (lookupStatus == OK)
	    {
		//
		// PHASE TWO: Then APPEND whatever can be scraped out of the CL_ERR_DESC alone.
		//
		currentLength = text_length;
		msgbuf[currentLength++] = '\n';

		lookupStatus = ERlookup(0,
			       pCLerr,
			       (i4)0,
			       (i4 *)NULL,
			       &msgbuf[currentLength],
			       (i4)(sizeof(msgbuf) - currentLength),
			       langcode,
			       &text_length,
			       &sys_err,
			       0,
			       (ER_ARGUMENT *)NULL);
		if (lookupStatus != OK)
		{
		    //
		    // ERlookup failed during the append phase, so APPEND some hard-wired
		    // text saying that we failed.
		    //
		    STprintf(&msgbuf[currentLength],
			     "FAILED to append CL_ERR_DESC info (reason: error %08lx)\n", lookupStatus);
		    currentLength = STlength(msgbuf);

		    //
		    // And then try to lookup a real message for the ERlookup failure code.
		    //
		    lookupStatus = ERlookup(lookupStatus,
					    &sys_err,
					    (i4)0,
					    (i4 *)NULL,
					    &msgbuf[currentLength],
					    (i4)(sizeof(msgbuf) - currentLength),
					    langcode,
					    &text_length,
					    &ignore,
					    0,
					    (ER_ARGUMENT *)NULL );
		    if (lookupStatus != OK)
		    {
		       STprintf(&msgbuf[currentLength], "ERlookup failed twice (status = %08lx)\n", lookupStatus);
		    }
		}
	    }
	    else
	    {
		//
		// The phase-one ERlookup failed.  Use some hard-wired text to say that, and
		// then try to append a real message describing the ERlookup failure code.
		//
		STprintf(msgbuf, "Couldn't look up message %08lx (reason: error %08lx)\n", msgid, lookupStatus);
		currentLength = STlength(msgbuf);

		lookupStatus = ERlookup(lookupStatus,
					&sys_err,
					(i4)0,
					(i4 *)NULL,
					&msgbuf[currentLength],
					(i4)(sizeof(msgbuf) - currentLength),
					langcode,
					&text_length,
					&ignore,
					0,
					(ER_ARGUMENT *)NULL );
	       if (lookupStatus != OK)
	       {
		   STprintf(&msgbuf[currentLength], "ERlookup failed twice (status = %08lx)\n", lookupStatus);
	       }
	    }
	}
}


/*
** Name: ErrorStack - Helper class to track errors
**
** Description:
**	This class helps us maintain a stack of errors (which may help reveal
**	underlying causes better than a single error message, and which allows
**	us to report all the information that comes from IIapi_getErrorInfo).
**
**	Each stack entry contains the following logical items:
**
**	    type
**	        The type identifies the "namespace" of the error code.  If the
**	        type is zero (II_GE_TOOLSAPI), then the code will be an OpenAPI
**	        status code (0 thru 8) or any unique Ingres ER_MSGID code ABOVE
**	        message class zero.  (The E_USxxx message numbers, class zero,
**	        are not allowed as status return codes within the ToolsAPI
**	        namespace.  However, they can be pushed onto the error stack
**	        using the II_GE_ERROR type code.)  If the type is non-zero, then
**	        that stack entry came from IIapi_getErrorInfo, and the type and
**	        code correspond to .ge_type and .ge_errorCode from the
**	        IIAPI_GETEINFOPARM structure.
**
**	    code
**	        The status code or error code.
**
**	    source
**	        A string identifying the source of the error.  By convention,
**	        this will typically be the name of the function from which the
**	        error entry was pushed.
**
**	    message
**	        The error message string, with parameters (if any) already
**	        merged in.
**
**	Each stack entry is stored in a variable-length ERROR_NODE block.  The
**	source and message strings are stored (contiguously) in a variable-
**	length buffer at the end of the block.  The ERROR_NODE has i4 members
**	which hold the length of the source string (INCLUDING its EOS), and the
**	length of the message string (INCLUDING its EOS).
**
**	PUSH BUT NO POP
**	    We push errors one at a time, but do not pop them individually.
**	    When we want to report the contents of the ErrorStack, the getErrors
**	    method returns the entire stack as a unit.  It returns an II_ERRORS
**	    block that contains an array of II_ERROR entries, where errors[0]
**	    represents what was on the top of the stack.  getErrors does not
**	    modify the stack.  To clear the stack, we call clear explicitly.
**
**	EACH TOOLSAPI ENTRYPOINT MUST CLEAR THE ERRORSTACK
**	    We must clear the ErrorStack at the beginning of every TOOLSAPI
**	    entrypoint.  This is because old (irrelevant) errors might still
**	    be on the stack left over from previous calls.
**
**	FREEZING THE ERRORSTACK
**	    Sometimes we might use TOOLSAPI entrypoints internally, to help do
**	    cleanup operations.  If we are cleaning up after an error, we don't
**	    want a successful cleanup call to clear the ErrorStack.  Similarly,
**	    we don't want cleanup errors to obscure the error that provoked the
**	    cleanup in the first place.  When we want to do some cleanup without
**	    impacting the ErrorStack, we can freeze the stack.  Then the stack
**	    will ignore all state-modifying calls until it is unfrozen.  (The
**	    frozen state is represented by a count, just to allow reentrancy in
**	    case some cleanup operation might be invoked within the scope of
**	    another cleanup operation.)
**
**	REVERSING THE PUSH ORDER
**	    Normally the first item pushed is the lowest-level cause of the
**	    error.  Then higher-level failure messages get added as the failed
**	    state bubbles back up the call stack.  Each level can provide an
**	    error message appropriate to the particular abstraction which that
**	    level represents.
**
**	    But OpenAPI maintains its own error stack, and IIapi_getErrorInfo
**	    reports those stack entries from the top down.  If we push them onto
**	    our ErrorStack in that order, then we would end up reporting them in
**	    the reverse order from which OpenAPI reported them.  Note that
**	    IIapi_getErrorInfo reports errors iteratively, and we can't change
**	    the order in which they are retrieved, so we have to save them
**	    somewhere temporarily so that we can then push them onto our stack
**	    in the reverse order.
**
**	    That temporary storage is integrated into our ErrorStack as a
**	    temporary stack.  After you call beginReversePushOrder, all
**	    subsequent push calls push the new node onto the temporary stack.
**	    When you call endReversePushOrder, the temporary stack is popped
**	    onto the main stack (thus reversing the order of those entries),
**	    and the push method reverts to using the main stack.
**
**	CONSOLIDATED TRACING
**	    Now that we have a centralized error reporting infrastructure, we
**	    can consolidate any tracing calls that were simply reporting error
**	    conditions.  When an error is pushed, it is automatically logged to
**	    the trace.  (Note that we only trace when errors are pushed onto the
**	    main stack, not the temporary stack.  This way, the tracing output
**	    always shows in a consistent order.)
**
**	OUT_OF_MEMORY STATE
**	    Allocation errors are particularly difficult to report because after
**	    an allocation error there is a reasonably high probability that you
**	    won't be able to make the allocations needed by the error reporting
**	    mechanism.
**
**	    The ErrorStack can encounter an allocation failure in two places:
**	    when allocating a node during a push operation, and when allocating
**	    the II_ERRORS block during a reporting operation.
**
**	    When the ErrorStack is unable to allocate a node for a push, it goes
**	    into an OUT_OF_MEMORY state, and it will ignore all subsequent push
**	    operations until it is explicitly cleared.  If you call getErrors
**	    when the stack is in an OUT_OF_MEMORY state, it will return a NULL
**	    II_ERRORS block pointer, just as it would if it failed to allocate
**	    the II_ERRORS block.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/


/*{
** Name: ErrorStack::ErrorStack - Constructor
**
** Description:
**	No-argument constructor initializes with no allocations and
**	cannot fail.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
ErrorStack::ErrorStack()
:   mainStackTop(NULL),
    tempStackTop(NULL),
    mainStackNodeCount(0),
    mainStackStringSize(0),
    freezeCount(0),
    reversePushOrder(false),
    outOfMemory(false)
{
}

/*{
** Name: ErrorStack::~ErrorStack - Destructor
**
** Description:
**	Destructor frees all stack nodes, regardless of current state.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
ErrorStack::~ErrorStack()
{
	//
	// Force a full clear, even if frozen.
	//
	clearAll();
}


/*{
** Name: ErrorStack::clearAll - Clear everything, even if frozen
**
** Description:
**	Private method that frees all stack nodes and forces the ErrorStack
**	back into its initial state, regardless of the current values of
**	freezeCount or outOfMemory.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Returns the ErrorStack to its initial state.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
void
ErrorStack::clearAll()
{
	mainStackNodeCount = 0;
	mainStackStringSize = 0;
	freezeCount = 0;
	reversePushOrder = false;
	outOfMemory = false;

	//
	// Walk BOTH internal stacks, freeing all nodes.
	//
	ERROR_NODE *a[2] = {mainStackTop, tempStackTop};
	for (int i = 0; i < sizeof(a)/sizeof(a[0]); i++)
	{
	    ERROR_NODE *pCurrent = a[i];
	    while (pCurrent)
	    {
		ERROR_NODE *pNext = pCurrent->next;
		free(pCurrent);
		pCurrent = pNext;
	    }
	}
	mainStackTop = NULL;
	tempStackTop = NULL;
}


/*{
** Name: ErrorStack::pushMain - Push a node onto the main stack
**
** Description:
**	Private method that pushes onto the main stack.
**
**	Note that tracing is done here (instead of during pushTemp or push)
**	to ensure a consistent ordering of the trace output.
**
** Inputs:
**	node		Pointer to an allocated and filled-in ERROR_NODE.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Pushes an entry onto the main stack.  Updates the node count and
**	total string size, and traces the new message.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
void
ErrorStack::pushMain(ErrorStack::ERROR_NODE *node)
{
	node->next = mainStackTop;
	mainStackTop = node;

	mainStackNodeCount++;
	mainStackStringSize += node->sourceSize + node->messageSize;

	//
	// Trace now, because at this level we get consistent ordering.
	//
	i4 type = node->type;
	i4 code = node->code;
	char *source = node->buf;
	char *message = node->buf + node->sourceSize;
	TRdisplay("%s %@ type=%d code=%x source='%s' msg='%s'\n", API, type, code, source, message);
}


/*{
** Name: ErrorStack::pushTemp - Push a node onto the temporary stack
**
** Description:
**	Private method that pushes onto the temporary stack (where it is held
**	until it can be popped over to the main stack, reversing the ordering).
**
** Inputs:
**	node		Pointer to an allocated and filled-in ERROR_NODE.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Pushes an entry onto the temporary stack.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
void
ErrorStack::pushTemp(ErrorStack::ERROR_NODE *node)
{
	node->next = tempStackTop;
	tempStackTop = node;
}


/*{
** Name: ErrorStack::popTemp - Pop a node from the temporary stack
**
** Description:
**	Private method that pops a node from the temporary stack (so that it
**	can be pushed onto the main stack, reversing the ordering).
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	The popped ERROR_NODE.  Will be NULL if the stack is empty.
**
** Side Effects:
**	Pops an entry off the temporary stack.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
ErrorStack::ERROR_NODE*
ErrorStack::popTemp()
{
	ERROR_NODE *node = tempStackTop;
	if (node)
	{
	    tempStackTop = node->next;
	}

	return node;
}


/*{
** Name: ErrorStack::clear - Clear the ErrorStack
**
** Description:
**	Returns the ErrorStack to its initial state, UNLESS it is frozen.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Resets the state of the ErrorStack.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
void
ErrorStack::clear()
{
	if (freezeCount > 0)
	    return;

	clearAll();
}


/*{
** Name: ErrorStack::push - Push an error
**
** Description:
**	Allocates a new node and pushes it onto the stack, UNLESS the stack is
**	frozen or in an OUT_OF_MEMORY state.
**
** Inputs:
**	type		The code type.
**	code		The status or error code.
**	source		Source of the error (usually a function name).
**	message		The complete error message.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Pushes a node on the stack.  Traces if an OUT_OF_MEMORY state transition
**	occurs.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
void
ErrorStack::push(i4 type, i4 code, const char *source, const char *message)
{
	if (outOfMemory || freezeCount > 0)
	    return;

	if (!source)
	    source = "";

	if (!message)
	    message = "";

	size_t sourceSize = STlength(source) + 1;   // includes EOS
	size_t messageSize = STlength(message) + 1; // includes EOS

	//
	// The first byte of string storage (buf[0]) is accounted for in the
	// sizeof(ERROR_NODE) so we can shave 1 off sourceSize + messageSize.
	//
	size_t bytesNeeded = sizeof(ERROR_NODE) + (sourceSize + messageSize - 1);

	ERROR_NODE *node = (ERROR_NODE *)malloc(bytesNeeded);
	if (!node)
	{
	    clearAll();  // (Try to free some memory, just to help our caller.)
	    outOfMemory = true;
	    TRdisplay("%s %@ %s\n", API, "ErrorStack is now in OUT_OF_MEMORY state.");
	}
	else
	{
	    node->type = type;
	    node->code = code;

	    node->sourceSize = sourceSize;
	    STcopy(source, node->buf);

	    node->messageSize = messageSize;
	    STcopy(message, node->buf + sourceSize);

	    if (reversePushOrder)
		pushTemp(node);
	    else
		pushMain(node);
	}
}


/*{
** Name: ErrorStack::beginReversePushOrder - Set reversePushOrder state
**
** Description:
**	Sets the reversePushOrder state UNLESS the ErrorStack is frozen or
**	in an OUT_OF_MEMORY state.  (All subsequent push operations will use
**	the temporary stack, until the reversePushOrder state is cleared.)
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Sets reversePushOrder state.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
void
ErrorStack::beginReversePushOrder()
{
	if (outOfMemory || freezeCount > 0)
	    return;

	reversePushOrder = true;
}


/*{
** Name: ErrorStack::beginReversePushOrder - Clear reversePushOrder state
**
** Description:
**	Clears the reversePushOrder state, and pops the contents of the
**	temporary stack over to the main stack, UNLESS the ErrorStack is frozen
**	or in an OUT_OF_MEMORY state.
**
**	Now all the nodes we have been collecting on the temporary stack can go
**	onto the main stack, in the reverse order than they were originally
**	pushed.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Clears reversePushOrder state, and transfers from temp to main stacks.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
void
ErrorStack::endReversePushOrder()
{
	if (outOfMemory || freezeCount > 0)
	    return;

	ERROR_NODE *node = popTemp();
	while (node)
	{
	    pushMain(node);
	    node = popTemp();
	}

	reversePushOrder = false;
}


/*{
** Name: ErrorStack::freeze - Freeze the ErrorStack
**
** Description:
**	Increments the freezeCount.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Increments the freezeCount.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
void
ErrorStack::freeze()
{
	++freezeCount;
}


/*{
** Name: ErrorStack::unfreeze - Unfreeze the ErrorStack
**
** Description:
**	Decrements the freezeCount.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side Effects:
**	Decrements the freezeCount.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
void
ErrorStack::unfreeze()
{
	--freezeCount;
}


/*{
** Name: ErrorStack::getErrors - Get the contents of the ErrorStack
**
** Description:
**	Returns a pointer to an II_ERRORS block that reflects the entire
**	content of the main stack.
**
**	The caller must free that block with the TOOLSAPI MEfree function.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	A pointer to an allocated II_ERRORS block (which the caller must free).
**	A NULL return value means OUT_OF_MEMORY.
**
** Side Effects:
**	None.
**
** History:
**	09-Nov-2010 (dansa01)
**	    Initial coding.
*/
II_ERRORS*
ErrorStack::getErrors()
{
	if (outOfMemory)
	    return NULL;

	i4 trailingStructCount = mainStackNodeCount > 0 ? mainStackNodeCount - 1 : 0;
	size_t totalStructSize = sizeof(II_ERRORS) + sizeof(II_ERROR) * trailingStructCount;
	size_t bytesNeeded = totalStructSize + mainStackStringSize;

	II_ERRORS *iiErrors = (II_ERRORS *)MEreqmem(0, bytesNeeded, TRUE, NULL);
	if (iiErrors)
	{
	    iiErrors->count = mainStackNodeCount;

	    ERROR_NODE *node = mainStackTop;
	    char *nextString = (char *)iiErrors + totalStructSize;

	    for (i4 i = 0; i < mainStackNodeCount; i++)
	    {
		iiErrors->errors[i].type = node->type;
		iiErrors->errors[i].code = node->code;

		STcopy(node->buf, nextString);
		iiErrors->errors[i].source = nextString;
		nextString += node->sourceSize;

		STcopy((node->buf + node->sourceSize), nextString);
		iiErrors->errors[i].message = nextString;
		nextString += node->messageSize;

		node = node->next;
	    }
	}

	return iiErrors;
}


/*
** Name: gen_sprintf - clone of sprintf that also does
**	memory allocation (to avoid buffer overflows)
**
** Description:
**	Format a "printf" format string into an allocated
**	buffer guaranteed large enough to hold the entire string.
**	TODO: Only handles %%, and %s and %d without qualifiers
**
** Inputs:
**	format	- "printf" style format string
**	...	- variables according to the embedded format specifiers
**
**	Note: NULL strings are handled gracefully in that nothing
**	is copied into the destination.
**
** Outputs:
**	None
**
** Returns:
**	char *	- pointer to allocated and formatted buffer
**		or NULL if not enough memory is available
**
**		NOTE: user must call "free()" on this buffer
**		when done with it.
**
** History:
**    04-Jan-2010 (whiro01)
**	Cloned from the Piccolo client code routine "gen_sprintf".
*/
char *
gen_sprintf(const char *format, ...)
{
	va_list args;
	size_t size;
	char *buf;

	/* Start counting size of fixed string plus space for formatted arguments */
	va_start(args, format);
#ifdef NT_GENERIC
	size = _vscprintf(format, args);
#elif __STDC_VERSION__ >= 199901L
	size = vsnprintf(NULL, 0, format, args);
#else
	const char *ptr;
	int intarg;
	char *strarg;
	char *out;

	for (ptr = format, size = 0; *ptr; ptr++)
	{
		if (*ptr == '%')
		{
			ptr++;
			if (*ptr == '%')
				size++;
			else if (*ptr == 'd')
			{
				intarg = va_arg(args, int);
				size += 11;	/* max size of 32-bit int in decimal */
			}
			else if (*ptr == 's')
			{
				strarg = va_arg(args, char *);
				if (strarg != NULL)
					size += strlen(strarg);
			}
			/* HOT: more format support here */
		}
		else
			size++;
	}
#endif
	size++;		/* for EOS */
	va_end(args);

	/* Allocate sufficient memory to hold the completely formatted string */
	if ((buf = reinterpret_cast<char *>(malloc(size * sizeof(char)))) == NULL)
	{
		return buf;
	}

	/* Now start the actual formatting of the destination */
	va_start(args, format);
#ifdef NT_GENERIC
	vsprintf(buf, format, args);
#elif __STDC_VERSION__ >= 199901L
	vsnprintf(buf, size, format, args);
#else
	for (ptr = format, out = buf, size = 0; *ptr; ptr++)
	{
		if (*ptr == '%')
		{
			ptr++;
			if (*ptr == '%')
				*out++ = *ptr;
			else if (*ptr == 'd')
			{
				intarg = va_arg(args, int);
				size = sprintf(out, "%d", intarg);
				out += size;
			}
			else if (*ptr == 's')
			{
				strarg = va_arg(args, char *);
				if (strarg != NULL)
				{
					size = strlen(strarg);
					memcpy(out, strarg, size);
					out += size;
				}
			}
			/* HOT: more format support here */
		}
		else
			*out++ = *ptr;
	}
	*out = '\0';
#endif
	va_end(args);

	return buf;
}


/*
** Name: strbufcpy - copy a string up to a maximum buffer size
**
** Description:
**	To avoid buffer overflows, this function takes a buffer
**	pointer and size and copies only as much of the source
**	string as will fit in the destination size.
**	Usage would typically be thus:
**		strbufcpy(src, buf, sizeof(buf));
**	If the source string is longer than the given buffer length
**	a maximum of (len-1) bytes of the source will be copied to
**	the destination and a '\0' byte added to the destination
**	at location buf[len-1].
**	If the source pointer is NULL only a trailing \0 byte will
**	be copied to the buffer (if len != 0).
**
** Inputs:
**	src	- EOS-terminated string to be copied
**	buf	- output buffer to copy the string to
**	len	- sizeof(buf)
**
**	Note: if len == 0 then nothing will be copied to the output
**	(not even the trailing \0 byte)
**
** Outputs:
**	Source copied to "buf" and terminated with \0 byte
**	(unless input "len" is set to 0).
**
** Returns:
**	Number of bytes copied from the src to the buffer
**	(including the trailing \0 byte).
**
** History:
**    16-Jun-2010 (whiro01)
**	Initial coding.
*/
static size_t
strbufcpy(const char *src, char *buf, size_t len)
{
	size_t count = 0;
	if (len != 0 && buf != NULL) {
		if (src != NULL) {
			while (src[count] != '\0' && count < len - 1) {
				buf[count] = src[count];
				count++;
			}
		}
		buf[count++] = '\0';
	}
	return count;
}


/*{
**
**  Name: scs_chk_priv - validate a given user's ownership of a given privilege
**
**  Description:
**	Calls PM to determine the user's allotted privileges, then
**	checks whether the specific required privilege is among them.
**
**  Inputs:
**	usr  -	user to validate
**	priv -	name of required privilege
**
**  Returns:
**	TRUE/FALSE	- user has the privilege
**
**  History:
**      28-Oct-92 (brucek)
**          Created.
**	26-Jul-1993 (daveb)
**	    swiped from gca_chk_priv.
**       9-Dec-1993 (daveb)
**          Changed to meet current LRC approved PM vars.
**       2-Mar-1994 (daveb) 58423
**          make public so it can be used in scs_initate for monitor
**  	    session validation.  Limit possible length to DB_MAXNAME in case it
**  	    isn't null terminated
**      21-Apr-1994 (daveb)
**          Renamed/moved from scd to scs_chk_priv for VMS shared
**          library reasons.
**      12-Aug-2011 (dansa01)
**          Extracted from scf!scs!scssvc.c to remove our dependency on
**          server libraries.
**      12-Aug-2011 (whiro01)
**          Provide alternative definition of DB_OWN_MAXNAME for prior versions
**          of Ingres; fix compiler warning.
**/
static bool
scs_chk_priv( char *user_name, char *priv_name )
{
#ifndef DB_OWN_MAXNAME
  #ifdef DB_MAXNAME
    #define DB_OWN_MAXNAME	DB_MAXNAME
  #else
    #define DB_OWN_MAXNAME	32
  #endif
#endif
	char	pmsym[128], userbuf[DB_OWN_MAXNAME+1], *value, *valueptr ;
	char	*strbuf = 0;
	int	priv_len;

	/*
	** privileges entries are of the form
	**   ii.<host>.*.privilege.<user>:   SERVER_CONTROL,NET_ADMIN,...
	**
	** Currently known privs are:  SERVER_CONTROL,NET_ADMIN,
	**  	    MONITOR,TRUSTED
	*/

	STncpy( userbuf, user_name, DB_OWN_MAXNAME );
	userbuf[ DB_OWN_MAXNAME ] = '\0';
	STtrmwhite( userbuf );
	STprintf(pmsym, "$.$.privileges.user.%s", userbuf);

	/* check to see if entry for given user */
	/* Assumes PMinit() and PMload() have already been called */

	if( PMget(pmsym, &value) != OK )
	    return FALSE;

	valueptr = value ;
	priv_len = STlength(priv_name) ;

	/*
	** STindex the PM value string and compare each individual string
	** with priv_name
	*/
	while ( *valueptr != EOS && (strbuf=STchr(valueptr, ',' )) != NULL )
	{
	    if (!STscompare(valueptr, priv_len, priv_name, priv_len))
		return TRUE ;

	    /* skip blank characters after the ','*/
	    valueptr = STskipblank(strbuf+1, 10);
	}

	/* we're now at the last or only (or NULL) word in the string */
	if ( *valueptr != EOS  &&
	      !STscompare(valueptr, priv_len, priv_name, priv_len))
		return TRUE ;

	return FALSE;
}


/*{
** Name: checkErrors - Check for and report OpenAPI errors
**
** Description:
**	This helper function checks the status of an OpenAPI genparm struct.
**	If an error is detected, it pushes that error status, and pushes all
**	other cause messages that can be obtained from the errorHandle.
**
** Inputs:
**	inst		TOOLSAPI instance pointer.
**	source		Source string to use for any errors pushed.
**	genParm		Pointer to IIAPI_GENPARM substructure to check.
**
** Outputs:
**	None.
**
** Returns:
**	OpenAPI status code from the IIAPI_GENPARM structure.
**
** Side Effects:
**	Pushes errors onto the ErrorStack.
**
** History:
**	15-Nov-2010 (dansa01)
**	    Adapted from existing printErrors function.
**	07-Oct-2011 (dansa01)
**	    Treat IIAPI_ST_MESSAGE and IIAPI_ST_WARNING as successful status
**	    codes, because they indicate that the IIapi function actually did
**	    succeed, and we need to process the results.
*/
static i4
checkErrors(TOOLSAPI *inst, const char *source, IIAPI_GENPARM *genParm)
{
	i4 status = genParm->gp_status;

	if (status == IIAPI_ST_SUCCESS || status == IIAPI_ST_NO_DATA)
	    return status;

	inst->impl->beginReversePushOrder();
	{
	    inst->impl->pushError(status, source);

	    II_PTR errorHandle = genParm->gp_errorHandle;
	    if (errorHandle != NULL)
	    {
		IIAPI_GETEINFOPARM getEInfoParm = {0};
		getEInfoParm.ge_errorHandle = errorHandle;
		for(;;)
		{
		    IIapi_getErrorInfo(&getEInfoParm);

		    i4 infoStatus = getEInfoParm.ge_status;

		    if (infoStatus == IIAPI_ST_SUCCESS)
		    {
			// Push this message, and then look for more.
			inst->impl->pushError(getEInfoParm.ge_type,
					      getEInfoParm.ge_errorCode,
					      source,
					      getEInfoParm.ge_message);
		    }
		    else if (infoStatus == IIAPI_ST_NO_DATA)
		    {
			// Normal termination.
			break;
		    }
		    else if (infoStatus == IIAPI_ST_NOT_INITIALIZED)
		    {
			inst->impl->pushError(II_ST_NOT_INITIALIZED, "checkErrors");
			break;
		    }
		    else if (infoStatus == IIAPI_ST_INVALID_HANDLE)
		    {
			inst->impl->pushError(II_ST_INVALID_HANDLE, "checkErrors");
			break;
		    }
		    else
		    {
			// Wtf?  At least prevent an infinite loop.
			inst->impl->pushError(E_TA0105_BAD_INFO_STATUS, "checkErrors");
			break;
		    }
		}
	    }
	}
	inst->impl->endReversePushOrder();

	//
	// Convert MESSAGE and WARNING status to SUCCESS.  The function actually
	// did succeed, and the normal results need to be processed.  (The extra
	// messages and warnings are now on the ErrorStack, in case the caller
	// wants to check for them.)
	//
	if (status == IIAPI_ST_MESSAGE || status == IIAPI_ST_WARNING)
	    status = IIAPI_ST_SUCCESS;

	return status;
}


static int
wait_for_completion(IIAPI_GENPARM *genParm, int iWaitSeconds)
{
	IIAPI_WAITPARM waitParm = { -1 };

	if (iWaitSeconds > 0)
		waitParm.wt_timeout = (II_LONG)(iWaitSeconds) * 1000;

	while( !genParm->gp_completed )
	{
		IIapi_wait( &waitParm );

		/* Sometimes the wait doesn't succeed.
		**  In this case, if we call it again we will hang.
		**  So, return the error status from the wait as the final status.
		*/
		if ( waitParm.wt_status != IIAPI_ST_SUCCESS )
		{
			genParm->gp_status = waitParm.wt_status;
			break;
		}
	}

	/* As a convenience, return the final status */
	return genParm->gp_status;
}


/*{
** Name: makeDataValuesBlock - Allocate a consolidated data values block
**
** Description:
**	This helper function allocates a consolidated block that we can use to
**	package the IIapi_getColumns results into a C struct that the caller can
**	later dispose of using a single MEfree call.
**
**	The consolidated block must include enough space for the array of
**	II_DATAVALUE structs, plus all the value buffer segments pointed to
**	by those structs.  The only tricky part is that those buffer segments
**	must be ALIGNED.
**
**	Note that we keep the buffer pointers in pApiValues and pDataV in sync.
**	pApiValues is for IIapi_getColumns to use.  pDataV is for the Toolsapi
**	caller.  The structs are similar, but not guaranteed to be identical.
**	However, we initialize things so that pApiValues[i].dv_data and
**	pDataV->values[i].data both point to the same buffer segment.  That way
**	we only need to copy out the dv_null and dv_length fields after the
**	IIapi_getColumns call, and no data values need to be copied.
**
**	NOTE that each value buffer segment is allocated one byte longer than
**	necessary, because the existing JNI code sometimes tacks an EOS
**	directly onto the buffer.  (Not sure if I entirely agree with that
**	approach in principle, but I don't want to risk changing that now.)
**
** Inputs:
**	inst		TOOLSAPI instance pointer (for error reporting).
**	pDescV		Pointer to query descriptors.
**	rowCount	Number of rows requested.
**	pApiValues	Pointer to uninitialized array of IIAPI_DATAVALUEs.
**			(This array must have the same number of elements as
**			the number of input descriptors * row count.)
**	ppDataV		Pointer to location to receive address of allocated
**			and initialized data values block.
**
** Outputs:
**	*pApiValues	dv_data pointers are initialized to point into the
**			the allocated block.
**	*ppDataV	Receives address of the allocated and initialized
**			block.
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	None.
**
** History:
**	12-oct-2010 (dansa01)
**	    Initial coding.
**	15-nov-2010 (dansa01)
**	    Added inst parameter for error reporting.
**	29-Jun-2011 (gordy)
**	    Added rowCount parameter for multi-row requests.
*/
static i4
makeDataValuesBlock(TOOLSAPI *inst, II_DESCRIPTORS *pDescV, i4 rowCount, IIAPI_DATAVALUE *pApiValues, II_DATAVALUES **ppDataV)
{
	i4 ncols = pDescV->count;
	i4 tcols = ncols * rowCount;

	size_t totalStructSize = sizeof(II_DATAVALUES) +
				 sizeof(II_DATAVALUE) * (tcols > 0 ? tcols - 1 : 0);

	//
	// Make a first pass through the descriptors, accumulating the needed
	// value buffer sizes, plus alignment padding, so that we can determine
	// the total block size to allocate.  (We assume our block starts at
	// address zero, but any ALIGN_RESTRICT-compliant base would do.  We
	// are only interested in offsets during this pass.)
	//
	char *blockBase = 0;
	char *nextValueBuf = blockBase + totalStructSize;
	nextValueBuf = ME_ALIGN_MACRO(nextValueBuf, sizeof(ALIGN_RESTRICT));
	for (i4 i = 0; i < tcols; i++)
	{
	    nextValueBuf += pDescV->descriptors[ i % ncols ].length + 1; //JNI wants EOS room
	    nextValueBuf = ME_ALIGN_MACRO(nextValueBuf, sizeof(ALIGN_RESTRICT));
	}
	size_t bytesNeeded = nextValueBuf - blockBase;

	//
	// Now that we know the total size needed, try to allocate it.
	//
	II_DATAVALUES *pDataV = reinterpret_cast<II_DATAVALUES *>(MEreqmem(0, bytesNeeded, TRUE, NULL));
	if (pDataV == NULL)
	{
	    return inst->impl->pushError(II_ST_OUT_OF_MEMORY, "makeDataValuesBlock");
	}

	//
	// Now make a second pass through the descriptors, setting the same
	// aligned value buffer pointers in both the IIAPI_DATAVALUE array and
	// our II_DATAVALUE array.
	//
	pDataV->count = tcols;
	blockBase = reinterpret_cast<char *>(pDataV);
	nextValueBuf = blockBase + totalStructSize;
	nextValueBuf = ME_ALIGN_MACRO(nextValueBuf, sizeof(ALIGN_RESTRICT));
	for (i4 i = 0; i < tcols; i++)
	{
	    pDataV->values[i].data = reinterpret_cast<II_PTR>(nextValueBuf);
	    pApiValues[i].dv_value = reinterpret_cast<II_PTR>(nextValueBuf);

	    nextValueBuf += pDescV->descriptors[ i % ncols ].length + 1; //JNI wants EOS room
	    nextValueBuf = ME_ALIGN_MACRO(nextValueBuf, sizeof(ALIGN_RESTRICT));
	}

	*ppDataV = pDataV;

	return II_ST_SUCCESS;
}


/*
** Name: StrQryResult - Helper class to temporarily buffer string query results
**
** Description:
**	This class helps us buffer multirow "query" output (of string and/or
**	varchar data) when we can't know the number of rows in advance, and
**	then helps us repackage the final result into a consolidated output
**	block for the API caller.  Methods that might fail return status codes.
**
**	Results are buffered as a one-dimensional list of query output strings
**	in row-major order.  But this class has no knowledge of row boundaries
**	during the acquisition phase.  After the query is complete, the caller
**	specifies the expected row size during the validation phase.
**
**	During retrieval of rows from OpenAPI (as in the case of a Netu query),
**	the address of each varchar data value is passed to the addVarchar
**	method, which copies the string into a list (adding an EOS), keeping
**	track of the total number of strings AND their total length (INCLUDING
**	all the EOS terminators).  When retrieving rows from other sources
**	that don't produce varchar data (such as NMsymbols, or PMscan), strings
**	can be passed to the addString method, which operates similarly.
**
**	After retrieval of all rows is complete, a consolidated output block
**	of sufficient size to hold the entire result can be obtained from the
**	getCBuffer method.  This method takes the expected number of result
**	columns as a parameter, and it verifies that the list contains an
**	integral multiple of that number.  It also sets the row count, and
**	initializes the internal state to begin iterative output of strings.
**
**	Then the results can be transferred to the consolidated output block
**	by iteratively calling the copyNextToBuf method.  It copies the next
**	result string to the given location, increments the internal iteration
**	state, and returns an updated buffer address indicating where the next
**	string should go.
**
** History:
**	09-Sep-2010 (dansa01)
**	    Initial coding.
**	08-Oct-2010 (dansa01)
**	    Refactored, renamed (formerly NetuQueryResult), and generalized to
**	    buffer multirow string output from any source, not just Netu queries
**	    (for example, NMsymbols or PMscan).
**	15-nov-2010 (dansa01)
**	    Added inst member for error reporting.*/


/*{
** Name: StrQryResult::StrQryResult - Constructor
**
** Description:
**	Constructor initializes with no allocations and cannot fail.
**
** History:
**	09-sep-2010 (dansa01)
**	    Initial coding.
**	15-nov-2010 (dansa01)
**	    Added inst member for error reporting.
*/
StrQryResult::StrQryResult(TOOLSAPI *pInst)
:   inst(pInst),
    count(0),
    rowCount(0),
    totalStringLength(0),
    listHead(NULL),
    listTail(NULL),
    nextOutput(NULL)
{
	// Constructor does no allocations, and cannot fail.
}


/*{
** Name: StrQryResult::~StrQryResult - Destructor
**
** Description:
**	Destructor walks the node list, freeing all the node allocations.
**
** History:
**	09-Sep-2010 (dansa01)
**	    Initial coding.
*/
StrQryResult::~StrQryResult()
{
	STRING_NODE *pCurrent = listHead;
	while (pCurrent)
	{
	    STRING_NODE *pNext = pCurrent->next;
	    free(pCurrent);
	    pCurrent = pNext;
	}
}


/*{
** Name: StrQryResult::addString - Add a string to the query result
**
** Description:
**	This method accepts a char* and an explicit length, allocates a string
**	result node of sufficient size, copies the string into the node as an
**	EOS-terminated C string, and appends the node to the tail of the result
**	list.
**
** Inputs:
**	pc		Pointer to a char array (which MIGHT NOT have an EOS).
**	stringLen	Number of chars to pick up from that char array.
**
** Outputs:
**	None.
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	Allocates a node and adds it to the instance result list.
**
** History:
**	09-Sep-2010 (dansa01)
**	    Initial coding.
*/
i4
StrQryResult::addString(const char *pc, i4 stringLen)
{
	i4 status = II_ST_SUCCESS;

	//
	// Allocate a node large enough for this string.  (Note that the extra
	// one byte we need to add an EOS is already included in the length of
	// the struct definition, by virtue of the "chars[1]" declaration.)
	//
	STRING_NODE *qrn = (STRING_NODE *)malloc(sizeof(STRING_NODE) + stringLen);

	if (qrn == NULL)
	{
	    status = inst->impl->pushError(II_ST_OUT_OF_MEMORY, "StrQryResult::addString");
	}
	else
	{
	    qrn->next = NULL;
	    qrn->charsLen = stringLen + 1;  // includes EOS
	    MEcopy(pc, stringLen, qrn->chars);
	    qrn->chars[stringLen] = EOS;

	    count++;
	    totalStringLength += qrn->charsLen;

	    if (listTail == NULL)
	    {
		listTail = qrn;
		listHead = qrn;
	    }
	    else
	    {
		listTail->next = qrn;
		listTail = qrn;
	    }
	}

	return status;
}


/*{
** Name: StrQryResult::addVarchar - Add a varchar to the query result
**
** Description:
**	This method accepts a pointer to a varchar data buffer, allocates a
**	result node of sufficient size, copies the varchar string into the
**	node as an EOS-terminated C string, and appends the node to the tail
**	of the result list.
**
** Inputs:
**	pv	Pointer to an OpenAPI varchar data buffer.
**
** Outputs:
**	None.
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	Allocates a node and adds it to the instance result list.
**
** History:
**	09-Sep-2010 (dansa01)
**	    Initial coding.
**	08-Oct-2010 (dansa01)
**	    Refactored to use new addString method.
*/
i4
StrQryResult::addVarchar(const void *pv)
{
	//
	// We are pointing to a varchar buffer.  Now do the nasty stuff to
	// extract the length field of the varchar buffer, and point to the
	// beginning of the actual string.
	//
	II_INT2 varcharLen;
	MEcopy(pv, sizeof(II_INT2), &varcharLen);

	return addString((char*)pv + sizeof(II_INT2), varcharLen);
}


/*{
** Name: StrQryResult::validateForIteration - Prepare for output phase
**
** Description:
**	This method takes the expected number of result columns as a parameter
**	and it verifies that the list contains an integral multiple of that
**	number of nodes.  It also sets the row count, and initializes the
**	internal state to begin iterative output.
**
**	(It is possible to call this again, to restart the output iteration,
**	but it is unlikely to ever be used that way.)
**
** Inputs:
**	expectedNumCols		Expected number of result columns from query.
**
** Outputs:
**	None.
**
** Returns:
**	OpenAPI status code, or an ER_MSGID.
**
** Side Effects:
**	Sets the instance rowCount, and sets the nextOutput pointer to the
**	head of the list.
**
** History:
**	09-Sep-2010 (dansa01)
**	    Initial coding.
*/
i4
StrQryResult::validateForIteration(i4 expectedNumCols)
{
	i4 status = II_ST_SUCCESS;

	if ((count % expectedNumCols) != 0)
	{
	    // Something is wrong.  The shape is not rectangular.
	    status = inst->impl->pushError(E_TA0100_INVALID_QRY_RESULT, "StrQryResult::validateForIteration");
	}
	else
	{
	    // Ok, prepare to iterate through the result.
	    rowCount = count / expectedNumCols;
	    nextOutput = listHead;
	}

	return status;
}


/*{
** Name: StrQryResult::getCBuffer - Allocate a consolidated buffer
**
** Description:
**	This method allocates a consolidated buffer that we can use to package
**	up the query results into a C struct that the caller can later dispose
**	of using a single MEfree call.  It assumes a standard layout composed
**	of an outer struct ending with a variable-length array of some inner
**	struct, followed by sequentially-packed EOS-terminated strings
**	referenced by pointers in the struct members.
**
**	This method also validates the shape of the query result, and prepares
**	it for iterative output.
**
** Inputs:
**	sizeofOuter	Size of the outer struct.
**	sizeofInner	Size of the inner (repeating) struct.
**	cols		Expected number of columns in the query result.
**	ppCBuf		Pointer to location to receive new buffer address.
**			Must not be NULL.
**	ppStrings	Pointer to location to receive string storage address.
**			Must not be NULL.
**
** Outputs:
**	*ppCBuf		Receives address of the allocated buffer.
**	*ppStrings	Receives address within that buffer where the
**			sequential string storage area should begin.
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	None.
**
** History:
**	09-sep-2010 (dansa01)
**	    Initial coding.
**	08-Oct-2010 (dansa01)
**	    Refactored from CPPToolsInterface class into StrQryResult class.
*/
i4
StrQryResult::getCBuffer(size_t sizeofOuter, size_t sizeofInner, i4 cols, char **ppCBuf, char **ppStrings)
{
	//
	// First make sure the query result contains a multiple of the expected
	// number of columns, and set the row count, and prepare for iteration.
	//
	i4 status = validateForIteration(cols);

	if (status == II_ST_SUCCESS)
	{
	    //
	    // totalStructLength represents the TOTAL space taken by the outer
	    // struct plus any trailing repetitions of the inner struct.  The outer
	    // struct ALWAYS contains ONE instance of the inner struct, possibly
	    // followed by "rows - 1" additional repetitions of the inner.
	    //
	    // Embedded storage for the strings pointed to by the struct members
	    // begins after that (represented by totalStringLength), and it
	    // includes space for all the strings AND their EOS teminators.
	    //
	    size_t rows = getRowCount();
	    size_t trailingInners = sizeofInner * (rows > 0 ? rows - 1 : 0);
	    size_t totalStructLength = sizeofOuter + trailingInners;
	    size_t totalStringLength = getTotalStringLength();

	    size_t bytesNeeded = totalStructLength + totalStringLength;

	    *ppCBuf = (char *)MEreqmem(0, bytesNeeded, TRUE, NULL);
	    if (*ppCBuf == NULL)
	    {
		status = inst->impl->pushError(II_ST_OUT_OF_MEMORY, "StrQryResult::getCBuffer");
	    }
	    else
	    {
		// And tell our caller where the embedded string storage begins.
		*ppStrings = *ppCBuf + totalStructLength;
	    }
	}

	return status;
}


/*{
** Name: StrQryResult::copyNextToBuf - Output the next result string
**
** Description:
**	This method makes it convenient to copy the buffered result strings
**	iteratively from the node list and lay them down sequentially into a
**	consolidated output block.
**
** Inputs:
**	pBuf		Address where next string should be copied.
**
** Outputs:
**	*pBuf		Buffer receives the copied string.
**
** Returns:
**	Updated buffer address, pointing one byte beyond what was copied.
**
** Side Effects:
**	Updates nextOutput pointer to the next available list node.
**
** History:
**	09-Sep-2010 (dansa01)
**	    Initial coding.
*/
char *
StrQryResult::copyNextToBuf(char *pBuf)
{
	i4 bytesToCopy = 0;

	if (nextOutput != NULL)
	{
	    bytesToCopy = nextOutput->charsLen;
	    MEcopy(nextOutput->chars, bytesToCopy, pBuf);
	    nextOutput = nextOutput->next;
	}

	return (pBuf + bytesToCopy);
}


/*{
** Name: StrQryResult::getRowCount - Field accessor
**
** Description:
**	Gets the row count (as determined by a prior call to the
**	validateForIteration method).
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	Row count (only valid after validateForIteration is called).
**
** Side Effects:
**	None.
**
** History:
**	09-Sep-2010 (dansa01)
**	    Initial coding.
*/
i4
StrQryResult::getRowCount()
{
	return rowCount;
}


/*{
** Name: StrQryResult::getTotalStringLength - Field accessor
**
** Description:
**	Gets the total aggregate length of strings currently stored in the
**	result list (including their EOS terminators).  This is needed in order
**	to allocate a consolidated buffer of adequate size for the entire
**	result.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	Total string length.
**
** Side Effects:
**	None.
**
** History:
**	09-Sep-2010 (dansa01)
**	    Initial coding.
*/
i4
StrQryResult::getTotalStringLength()
{
	return totalStringLength;
}


/*{
** Name: setConnectParam - Set a given connection parameter
**
** Description:
**	This helper function simply factors out the repetitive code required for
**	setting optional connection parameters.
**
**	The connection handle is both an input and an output, because OpenAPI
**	will allocate a new handle the first time this is called.  Then we must
**	continue using that new handle as input to all subsequent calls.
**
** Inputs:
**	inst		TOOLSAPI instance pointer (for error reporting).
**	paramID		IIapi_setConnectParam paramID.
**	paramValue	IIapi_setConnectParam paramValue.
**	pConnHandle	Pointer to location of connection handle to use
**			(and possibly update).
**
** Outputs:
**	*pConnHandle	Receives the new connection handle (if OpenAPI allocated
**			a new one).
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	None.
**
** History:
**	08-Feb-2011 (dansa01)
**	    Initial coding.
*/
static i4
setConnectParam(TOOLSAPI *inst, II_LONG paramID, II_PTR paramValue, II_HANDLE *pConnHandle)
{
	IIAPI_SETCONPRMPARM scpParm = {0};

	scpParm.sc_connHandle = *pConnHandle;
	scpParm.sc_paramID = paramID;
	scpParm.sc_paramValue = paramValue;

	IIapi_setConnectParam(&scpParm);

	wait_for_completion(&scpParm.sc_genParm, NO_TIMEOUT);
	i4 status = checkErrors(inst, "setConnectParam", &scpParm.sc_genParm);

	if (status == IIAPI_ST_SUCCESS)
	{
	    *pConnHandle = scpParm.sc_connHandle;
	}
	else
	{
	    if (scpParm.sc_connHandle != *pConnHandle)
	    {
		//
		// OpenAPI allocated a connection handle for us, but we are in
		// a failed state.  Release that handle (and leave the current
		// error status undisturbed).
		//
		IIAPI_ABORTPARM abortParm = {0};
		abortParm.ab_connHandle = scpParm.sc_connHandle;

		IIapi_abort(&abortParm);
		wait_for_completion(&abortParm.ab_genParm, NO_TIMEOUT);
	    }
	}

	return status;
}


/*{
** Name: doConnect - Open connection to database or name server
**
** Description:
**	This helper function opens a connection to a database, or the Name
**	Server, and applies any specified optional connection parameters to the
**	new connection.
**
** Inputs:
**	inst		TOOLSAPI instance pointer.
**	pConnParm	Pointer to IIAPI_CONNPARM structure, already set up by
**			the caller with all needed parameters (except for the
**			co_connHandle member).
**	pOptions	Pointer to II_CONNECT_OPTIONS structure containing
**			options to be applied with IIapi_setConnectParam.
**			Can be NULL if no options are desired.  The option
**			values are all strings, and each can be NULL to signify
**			that the particular option value is not specified.
**	pConnHandle	Address to receive new connection handle.
**
** Outputs:
**	*pConnHandle	Receives new connection handle.
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	None.
**
** History:
**	09-Feb-2011 (dansa01)
**	    Initial coding.
*/
static i4
doConnect(TOOLSAPI *inst, IIAPI_CONNPARM *pConnParm, II_CONNECT_OPTIONS *pOptions, II_HANDLE *pConnHandle)
{
	//
	// First, apply all the optional connection parameters.  (The first one
	// applied causes a connection handle to be allocated, and we must then
	// carry that handle forward through all the subsequent steps.  But we
	// always start with the Environment Handle so that the new connection
	// handle gets created in the proper context.)
	//
	II_HANDLE connHandle = inst->impl->env_handle;
	i4 status = IIAPI_ST_SUCCESS;

	if (pOptions)
	{
	    if (pOptions->effectiveUser)
	    {
		status = setConnectParam(inst, IIAPI_CP_EFFECTIVE_USER, (II_PTR)pOptions->effectiveUser, &connHandle);
	    }

	    if (status == IIAPI_ST_SUCCESS && pOptions->group)
	    {
		status = setConnectParam(inst, IIAPI_CP_GROUP_ID, (II_PTR)pOptions->group, &connHandle);
	    }

	    if (status == IIAPI_ST_SUCCESS && pOptions->role)
	    {
		status = setConnectParam(inst, IIAPI_CP_APP_ID, (II_PTR)pOptions->role, &connHandle);
	    }

	    if (status == IIAPI_ST_SUCCESS && pOptions->dbmsPassword)
	    {
		status = setConnectParam(inst, IIAPI_CP_DBMS_PASSWORD, (II_PTR)pOptions->dbmsPassword, &connHandle);
	    }
	}

	//
	// Now that all the optional connection parameters have been applied,
	// try the actual connect.
	//
	if (status == IIAPI_ST_SUCCESS)
	{
	    pConnParm->co_connHandle = connHandle;

	    IIapi_connect(pConnParm);

	    wait_for_completion(&pConnParm->co_genParm, NO_TIMEOUT);
	    status = checkErrors(inst, "doConnect", &pConnParm->co_genParm);

	    if (status == IIAPI_ST_SUCCESS)
	    {
		connHandle = pConnParm->co_connHandle;
	    }
	}

	if (status == IIAPI_ST_SUCCESS)
	{
	    *pConnHandle = connHandle;
	}
	else
	{
	    if (connHandle != inst->impl->env_handle)
	    {
		//
		// OpenAPI allocated a connection handle for us, but we are in
		// a failed state.  Release that handle (and leave the current
		// error status undisturbed).
		//
		IIAPI_ABORTPARM abortParm = {0};
		abortParm.ab_connHandle = connHandle;

		IIapi_abort(&abortParm);
		wait_for_completion(&abortParm.ab_genParm, NO_TIMEOUT);
	    }
	}

	return status;
}


/*{
** Name: doPutParms - Put query parameters
**
** Description:
**	This internal helper function takes the given II_DESCRIPTORS,
**	II_DATAVALUES pair, converts them to IIapi format, and then calls
**	IIapi_setDescriptor and IIapi_putParms.  It assumes the caller has
**	already validated the parmDesc/parmData pair to ensure that they are
**	not NULL or empty and that their counts match.
**
** Inputs:
**	inst		TOOLSAPI instance pointer.
**	stmtHandle	Statement handle of the query expecting parameters.
**	parmDesc	Parameter descriptors.  Must not be NULL or empty.
**	parmData	Parameter data values.  Must not be NULL or empty,
**			and must contain the same number of values as there
**			are descriptors.
**
** Outputs:
**	None.
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	Changes query state.
**
** History:
**	19-Apr-2011 (dansa01)
**	    Initial coding.
*/
static i4
doPutParms(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS *parmDesc, II_DATAVALUES *parmData)
{
	i4 status = IIAPI_ST_SUCCESS;
	i4 parmCount = parmDesc->count;

	//
	// Create IIAPI descriptor/data arrays to match what is in the ToolsAPI
	// descriptor/data structs.  (These are temporary inputs, so a shallow
	// copy is fine.)
	//
	IIAPI_DESCRIPTOR *apiDesc = (IIAPI_DESCRIPTOR *)malloc(parmCount * sizeof(IIAPI_DESCRIPTOR));
	IIAPI_DATAVALUE  *apiData = (IIAPI_DATAVALUE  *)malloc(parmCount * sizeof(IIAPI_DATAVALUE));
	if (!apiDesc || !apiData)
	{
	    free(apiDesc);
	    free(apiData);
	    return inst->impl->pushError(II_ST_OUT_OF_MEMORY, "doPutParms");
	}

	for (int i = 0; i < parmCount; i++)
	{
	    apiDesc[i].ds_dataType   = parmDesc->descriptors[i].dataType;
	    apiDesc[i].ds_nullable   = parmDesc->descriptors[i].isNullable;
	    apiDesc[i].ds_length     = parmDesc->descriptors[i].length;
	    apiDesc[i].ds_precision  = parmDesc->descriptors[i].precision;
	    apiDesc[i].ds_scale      = parmDesc->descriptors[i].scale;
	    apiDesc[i].ds_columnType = parmDesc->descriptors[i].columnType;
	    apiDesc[i].ds_columnName = parmDesc->descriptors[i].columnName;

	    apiData[i].dv_null   = parmData->values[i].isNull;
	    apiData[i].dv_length = parmData->values[i].length;
	    apiData[i].dv_value  = parmData->values[i].data;
	}

	IIAPI_SETDESCRPARM setDescrParm = {0};

	setDescrParm.sd_stmtHandle = stmtHandle;
	setDescrParm.sd_descriptorCount = (II_INT2)parmCount;
	setDescrParm.sd_descriptor = apiDesc;

	IIapi_setDescriptor(&setDescrParm);

	wait_for_completion(&setDescrParm.sd_genParm, NO_TIMEOUT);
	status = checkErrors(inst, "IIapi_setDescriptor", &setDescrParm.sd_genParm);

	if (status == IIAPI_ST_SUCCESS)
	{
	    IIAPI_PUTPARMPARM putParmParm = {0};

	    putParmParm.pp_stmtHandle = stmtHandle;
	    putParmParm.pp_parmCount = (II_INT2)parmCount;
	    putParmParm.pp_parmData = apiData;

	    IIapi_putParms(&putParmParm);

	    wait_for_completion(&putParmParm.pp_genParm, NO_TIMEOUT);
	    status = checkErrors(inst, "IIapi_putParms", &putParmParm.pp_genParm);
	}

	free(apiDesc);
	free(apiData);

	return status;
}




/****************************************************************************************
**
** TOOLSAPI interface functions
**
****************************************************************************************/
i4 TOOLSAPICALL
I_IIabort(TOOLSAPI *inst, II_HANDLE connHandle)
{
	inst->impl->clearErrors();

	IIAPI_ABORTPARM abParm = {0};
	abParm.ab_connHandle = connHandle;

	IIapi_abort( &abParm );

	wait_for_completion( &abParm.ab_genParm, NO_TIMEOUT );
	return checkErrors(inst, "IIabort", &abParm.ab_genParm);
}


i4 TOOLSAPICALL
I_IIautoCommitOn(TOOLSAPI *inst, II_HANDLE connHandle, II_HANDLE *pTranHandle)
{
	inst->impl->clearErrors();

	IIAPI_AUTOPARM acParm = {0};
	acParm.ac_connHandle = connHandle;

	IIapi_autocommit( &acParm );

	wait_for_completion( &acParm.ac_genParm, NO_TIMEOUT );
	i4 status = checkErrors(inst, "IIautoCommitOn", &acParm.ac_genParm);

	if (status == IIAPI_ST_SUCCESS)
	{
	    *pTranHandle = acParm.ac_tranHandle;
	}
	else
	{
	    if (acParm.ac_tranHandle)
	    {
		// Release any handle resources that may remain after a failed
		// request (and leave the current error status undisturbed).
		IIAPI_AUTOPARM acOffParm = {0};
		acOffParm.ac_tranHandle = acParm.ac_tranHandle;

		IIapi_autocommit( &acOffParm );
		wait_for_completion( &acOffParm.ac_genParm, NO_TIMEOUT );
	    }
	}

	return status;
}


i4 TOOLSAPICALL
I_IIautoCommitOff(TOOLSAPI *inst, II_HANDLE tranHandle)
{
	inst->impl->clearErrors();

	IIAPI_AUTOPARM acParm = {0};
	acParm.ac_tranHandle = tranHandle;	/* Set non-zero to indicate autocommit off */

	IIapi_autocommit( &acParm );

	wait_for_completion( &acParm.ac_genParm, NO_TIMEOUT );
	return checkErrors(inst, "IIautoCommitOff", &acParm.ac_genParm);
}


i4 TOOLSAPICALL
I_IIconnectNameServer(TOOLSAPI *inst, const char *target, const char *user, const char *password, i4 timeout, II_HANDLE *pConnHandle)
{
	inst->impl->clearErrors();

	IIAPI_CONNPARM connParm = {0};

	connParm.co_type = IIAPI_CT_NS;
	connParm.co_target = (II_CHAR *)target;
	connParm.co_username = (II_CHAR *)user;
	connParm.co_password = (II_CHAR *)password;
	connParm.co_timeout = timeout;

	return doConnect(inst, &connParm, NULL, pConnHandle);
}


i4 TOOLSAPICALL
I_IIconnectDatabase(TOOLSAPI *inst, const char *target, const char *user, const char *password, i4 timeout, II_CONNECT_OPTIONS *pOptions, II_HANDLE *pConnHandle)
{
	inst->impl->clearErrors();

	IIAPI_CONNPARM connParm = {0};

	connParm.co_type = IIAPI_CT_SQL;
	connParm.co_target = (II_CHAR *)target;
	connParm.co_username = (II_CHAR *)user;
	connParm.co_password = (II_CHAR *)password;
	connParm.co_timeout = timeout;

	return doConnect(inst, &connParm, pOptions, pConnHandle);
}


i4 TOOLSAPICALL
I_IIdisconnect(TOOLSAPI *inst, II_HANDLE connHandle)
{
	inst->impl->clearErrors();

	IIAPI_DISCONNPARM disconnParm = {0};
	disconnParm.dc_connHandle = connHandle;

	IIapi_disconnect( &disconnParm );

	wait_for_completion( &disconnParm.dc_genParm, NO_TIMEOUT );
	return checkErrors(inst, "IIdisconnect", &disconnParm.dc_genParm);
}


i4 TOOLSAPICALL
I_IIquery(TOOLSAPI *inst, II_HANDLE connHandle, II_HANDLE *pTranHandle, const char *query, i4 needParms, II_HANDLE *pStmtHandle)
{
	inst->impl->clearErrors();

	IIAPI_QUERYPARM queryParm = {0};
	queryParm.qy_connHandle = connHandle;
	queryParm.qy_queryType = IIAPI_QT_QUERY;
	queryParm.qy_queryText = (II_CHAR *)query;
	queryParm.qy_parameters = needParms;
	queryParm.qy_tranHandle = *pTranHandle;

	IIapi_query( &queryParm );

	wait_for_completion( &queryParm.qy_genParm, NO_TIMEOUT );
	i4 status = checkErrors(inst, "IIquery", &queryParm.qy_genParm);

	if (status == IIAPI_ST_SUCCESS)
	{
	    *pStmtHandle = queryParm.qy_stmtHandle;

	    if (*pTranHandle != queryParm.qy_tranHandle)
	    {
		//
		// A new MST was initiated (with a new transaction handle).
		//
		*pTranHandle = queryParm.qy_tranHandle;
	    }
	}
	else
	{
	    //
	    // Release any handle resources that may remain after a failed
	    // request (and leave the current error status undisturbed).
	    //
	    if (queryParm.qy_stmtHandle)
	    {
		IIAPI_CANCELPARM cancelParm = {0};
		cancelParm.cn_stmtHandle = queryParm.qy_stmtHandle;
		IIapi_cancel(&cancelParm);
		wait_for_completion(&cancelParm.cn_genParm, CANCEL_TIMEOUT);

		IIAPI_CLOSEPARM closeParm = {0};
		closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;
		IIapi_close(&closeParm);
		wait_for_completion(&closeParm.cl_genParm, NO_TIMEOUT);
	    }

	    if (*pTranHandle != queryParm.qy_tranHandle)
	    {
		IIAPI_ROLLBACKPARM rollbackParm = {0};
		rollbackParm.rb_tranHandle = queryParm.qy_tranHandle;
		IIapi_rollback(&rollbackParm);
		wait_for_completion(&rollbackParm.rb_genParm, NO_TIMEOUT);
	    }
	}

	return status;
}


i4 TOOLSAPICALL
I_IIcancel(TOOLSAPI *inst, II_HANDLE stmtHandle)
{
	inst->impl->clearErrors();

	IIAPI_CANCELPARM cnParm = {0};
	cnParm.cn_stmtHandle = stmtHandle;

	IIapi_cancel( &cnParm );

	wait_for_completion( &cnParm.cn_genParm, CANCEL_TIMEOUT );

	//
	// Cancel gets a bit complicated, because what we would consider to be a
	// "successful" cancel operation will be reported as a "Failure" status
	// (with "Query cancelled" error) by OpenAPI, as described on page 44 of
	// the OpenAPI User Guide.  So we can't always use our normal error
	// handling.
	//
	i4 status = cnParm.cn_genParm.gp_status;
	II_PTR errorHandle = cnParm.cn_genParm.gp_errorHandle;

	if (status != IIAPI_ST_FAILURE || errorHandle == NULL)
	{
	    //
	    // The current state is NOT one of the special cases that we can
	    // translate, so fall back to the default error handling here.
	    //
	    return checkErrors(inst, "IIcancel", &cnParm.cn_genParm);
	}

	//
	// ASSERT: status == IIAPI_ST_FAILURE && errorHandle != NULL
	//
	// Now we have a "Failure" status and something in the errorHandle.  We
	// need to use the ErrorStack to collect the messages from OpenAPI,
	// because we can't know in advance how many there might be, and we want
	// them to be on the stack if we discover that they aren't something we
	// want to translate into success.  HOWEVER, we don't want messages to
	// be LOGGED if we are going to later clear them and declare success.
	// Fortunately, the ReversePushOrder mechanism helps us here, because
	// the logging doesn't take place until the endReversePushOrder is
	// called.  So if we decide to declare success, we simply clear the
	// ErrorStack without ever calling the endReversePushOrder method.
	//
	inst->impl->beginReversePushOrder();

	inst->impl->pushError(status, "IIcancel");

	IIAPI_GETEINFOPARM getEInfoParm = {0};
	getEInfoParm.ge_errorHandle = errorHandle;
	bool gotCancelMsg = false;
	bool gotOtherMsg = false;

	IIapi_getErrorInfo(&getEInfoParm);
	while (getEInfoParm.ge_status == IIAPI_ST_SUCCESS)
	{
	    i4 type = getEInfoParm.ge_type;
	    i4 code = getEInfoParm.ge_errorCode;

	    inst->impl->pushError(type, code, "IIcancel", getEInfoParm.ge_message);

	    if (type == II_GE_ERROR
	    && (code == E_AP0009_QUERY_CANCELLED || code == E_AP0008_QUERY_DONE))
	    {
		gotCancelMsg = true;
	    }
	    else
	    {
		gotOtherMsg = true;
	    }

	    IIapi_getErrorInfo(&getEInfoParm);
	}

	if (gotCancelMsg && !gotOtherMsg)
	{
	    //
	    // If we saw a cancel message, and nothing else, consider this
	    // a successful cancel operation.
	    //
	    inst->impl->clearErrors();
	    status = II_ST_SUCCESS;
	}
	else
	{
	    //
	    // We didn't see precisely what we expected for a successful cancel,
	    // so allow this to bubble up as an error.
	    //
	    inst->impl->endReversePushOrder();
	}

	return status;
}


i4 TOOLSAPICALL
I_IIclose(TOOLSAPI *inst, II_HANDLE stmtHandle)
{
	inst->impl->clearErrors();

	IIAPI_CLOSEPARM closeParm = {0};
	closeParm.cl_stmtHandle = stmtHandle;

	IIapi_close( &closeParm );

	wait_for_completion( &closeParm.cl_genParm, NO_TIMEOUT );
	return checkErrors(inst, "IIclose", &closeParm.cl_genParm);
}


i4
TOOLSAPICALL I_IIgetDescriptors(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS **ppDescV)
{
	inst->impl->clearErrors();

	IIAPI_GETDESCRPARM getDescrParm = {0};
	getDescrParm.gd_stmtHandle = stmtHandle;

	IIapi_getDescriptor( &getDescrParm );

	wait_for_completion( &getDescrParm.gd_genParm, NO_TIMEOUT );
	i4 status = checkErrors(inst, "IIgetDescriptors", &getDescrParm.gd_genParm);

	if (status == IIAPI_ST_NO_DATA)
	{
	    getDescrParm.gd_descriptorCount = 0;
	    status = IIAPI_ST_SUCCESS;
	}

	II_DESCRIPTORS *pDescV = NULL;

	if (status == IIAPI_ST_SUCCESS)
	{
	    i4 ncols = getDescrParm.gd_descriptorCount;
	    size_t totalStructLength = sizeof(II_DESCRIPTORS) +
				       sizeof(II_DESCRIPTOR) * (ncols > 0 ? ncols - 1 : 0);
	    //
	    // In order to create a consolidated buffer, we first need to
	    // determine how much space is needed for all the column name
	    // strings.  So walk the descriptor array and add them up.
	    //
	    size_t totalStringLength = 0;
	    for (int i = 0; i < ncols; i++)
	    {
	       if (getDescrParm.gd_descriptor[i].ds_columnName != NULL)
	       {
		   totalStringLength += STlength(getDescrParm.gd_descriptor[i].ds_columnName) + 1; // EOS
	       }
	    }
	    size_t bytesNeeded = totalStructLength + totalStringLength;

	    pDescV = reinterpret_cast<II_DESCRIPTORS *>(MEreqmem(0, bytesNeeded, TRUE, NULL));
	    if (!pDescV)
	    {
		return inst->impl->pushError(II_ST_OUT_OF_MEMORY, "IIgetDescriptors");
	    }

	    char *nextString = reinterpret_cast<char *>(pDescV) + totalStructLength;
	    pDescV->count = ncols;
	    for (int i = 0; i < ncols; i++)
	    {
	       pDescV->descriptors[i].dataType   = getDescrParm.gd_descriptor[i].ds_dataType;
	       pDescV->descriptors[i].isNullable = getDescrParm.gd_descriptor[i].ds_nullable ? TRUE : FALSE;
	       pDescV->descriptors[i].length     = getDescrParm.gd_descriptor[i].ds_length;
	       pDescV->descriptors[i].precision  = getDescrParm.gd_descriptor[i].ds_precision;
	       pDescV->descriptors[i].scale      = getDescrParm.gd_descriptor[i].ds_scale;
	       pDescV->descriptors[i].columnType = getDescrParm.gd_descriptor[i].ds_columnType;

	       if (getDescrParm.gd_descriptor[i].ds_columnName != NULL)
	       {
		   pDescV->descriptors[i].columnName = nextString;
		   STcopy(getDescrParm.gd_descriptor[i].ds_columnName, nextString);
		   nextString += STlength(nextString) + 1; // EOS
	       }
	    }

	    *ppDescV = pDescV;
	}

	return status;
}


i4 TOOLSAPICALL
I_IIgetColumns(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS *pDescV, i2 rowCount, II_DATAVALUES **ppDataV, i2 *pRows )
{
	inst->impl->clearErrors();
	*pRows = 0;

	if (!pDescV || pDescV->count == 0)
	{
	    return inst->impl->pushError(E_TA0109_EMPTY_INPUT, "IIgetColumns");
	}

	II_INT2 ncols = (II_INT2)pDescV->count;
	i4 tcols = ncols * rowCount;
	IIAPI_DATAVALUE *pDataValArray = (IIAPI_DATAVALUE *)malloc(sizeof(IIAPI_DATAVALUE) * tcols);
	if (!pDataValArray)
	{
	    return inst->impl->pushError(II_ST_OUT_OF_MEMORY, "IIgetColumns");
	}

	//
	// Create a single consolidated block for the II_DATAVALUES struct
	// and all the referenced value buffer segments.
	//
	II_DATAVALUES *pDataV = NULL;
	i4 status = makeDataValuesBlock(inst, pDescV, rowCount, pDataValArray, &pDataV);

	if (status == IIAPI_ST_SUCCESS)
	{
	    IIAPI_GETCOLPARM getColParm = {0};
	    getColParm.gc_rowCount = (II_INT2)rowCount;
	    getColParm.gc_columnCount = ncols;
	    getColParm.gc_columnData = pDataValArray;
	    getColParm.gc_stmtHandle = stmtHandle;

	    IIapi_getColumns( &getColParm );

	    wait_for_completion( &getColParm.gc_genParm, NO_TIMEOUT );
	    status = checkErrors(inst, "IIgetColumns", &getColParm.gc_genParm);
	    *pRows = getColParm.gc_rowsReturned;
	}

	if (status == IIAPI_ST_SUCCESS)
	{
	    tcols = ncols * *pRows;

	    // Now sync our II_DATAVALUE structs with OpenAPI's IIAPI_DATAVALUE structs
	    for (i4 i = 0; i < tcols; i++)
	    {
		pDataV->values[i].isNull = pDataValArray[i].dv_null ? TRUE : FALSE;
		pDataV->values[i].length = pDataValArray[i].dv_length;
	    }

	    *ppDataV = pDataV;
	}
	else
	{
	    *pRows = 0;
	    ::MEfree((PTR)pDataV);
	}

	free(pDataValArray);

	return status;
}


i4 TOOLSAPICALL
I_IIgetQueryInfo(TOOLSAPI *inst, II_HANDLE stmtHandle, II_QUERY_INFO *pqInfo)
{
	inst->impl->clearErrors();

	IIAPI_GETQINFOPARM getQInfoParm = {0};
	getQInfoParm.gq_stmtHandle = stmtHandle;

	IIapi_getQueryInfo( &getQInfoParm );

	wait_for_completion( &getQInfoParm.gq_genParm, NO_TIMEOUT );
	i4 status = checkErrors(inst, "IIgetQueryInfo", &getQInfoParm.gq_genParm);

	if (status == IIAPI_ST_SUCCESS)
	{
	    /* Transfer the fields one by one, depending on their availability */
	    pqInfo->flags             = getQInfoParm.gq_flags;
	    pqInfo->mask              = getQInfoParm.gq_mask;

	    pqInfo->rowCount   = (getQInfoParm.gq_mask & IIAPI_GQ_ROW_COUNT) ? getQInfoParm.gq_rowCount : 0;
	    pqInfo->readOnly   = ((getQInfoParm.gq_mask & IIAPI_GQ_CURSOR) ? getQInfoParm.gq_readonly : 0) ? true : false;
	    pqInfo->procReturn = (getQInfoParm.gq_mask & IIAPI_GQ_PROCEDURE_RET) ? getQInfoParm.gq_procedureReturn : 0;
	    pqInfo->procHandle = (getQInfoParm.gq_mask & IIAPI_GQ_PROCEDURE_ID) ? getQInfoParm.gq_procedureHandle : 0;
	    pqInfo->repeatQueryHandle = (getQInfoParm.gq_mask & IIAPI_GQ_REPEAT_QUERY_ID) ? getQInfoParm.gq_repeatQueryHandle : 0;

#ifdef IIAPI_VERSION_6
	    pqInfo->cursorType        = (getQInfoParm.gq_mask & IIAPI_GQ_CURSOR) ? getQInfoParm.gq_cursorType : 0;
	    pqInfo->rowStatus         = (getQInfoParm.gq_mask & IIAPI_GQ_ROW_STATUS) ? getQInfoParm.gq_rowStatus : 0;
	    pqInfo->rowPosition       = (getQInfoParm.gq_mask & IIAPI_GQ_ROW_STATUS) ? getQInfoParm.gq_rowPosition : 0;
#else
	    pqInfo->cursorType        = 0;
	    pqInfo->rowStatus         = 0;
	    pqInfo->rowPosition       = 0;
#endif
	    if (getQInfoParm.gq_mask & IIAPI_GQ_TABLE_KEY)
		MEcopy(&getQInfoParm.gq_tableKey[0], II_TABLE_KEY_SIZE, &pqInfo->tableKey[0]);

	    if (getQInfoParm.gq_mask & IIAPI_GQ_OBJECT_KEY)
		MEcopy(&getQInfoParm.gq_objectKey[0], II_OBJECT_KEY_SIZE, &pqInfo->objectKey[0]);
	}

	return status;
}


i4 TOOLSAPICALL
I_IIconvertData(TOOLSAPI *inst, II_DESCRIPTOR *srcDesc, II_DATAVALUE *srcData, II_DESCRIPTOR *dstDesc, II_DATAVALUE *dstData)
{
	inst->impl->clearErrors();

	IIAPI_CONVERTPARM cvtParm;

	/* Source description is what is passed in */
	cvtParm.cv_srcDesc.ds_dataType	= srcDesc->dataType;
	cvtParm.cv_srcDesc.ds_nullable	= srcDesc->isNullable;
	cvtParm.cv_srcDesc.ds_length	= srcDesc->length;
	cvtParm.cv_srcDesc.ds_precision	= srcDesc->precision;
	cvtParm.cv_srcDesc.ds_scale	= srcDesc->scale;
	cvtParm.cv_srcDesc.ds_columnType= srcDesc->columnType;
	cvtParm.cv_srcDesc.ds_columnName= srcDesc->columnName;
	cvtParm.cv_srcValue.dv_null	= srcData->isNull;
	cvtParm.cv_srcValue.dv_length	= srcData->length;
	cvtParm.cv_srcValue.dv_value	= srcData->data;

	/* Destination description is also what's passed in */
	cvtParm.cv_dstDesc.ds_dataType	= dstDesc->dataType;
	cvtParm.cv_dstDesc.ds_nullable	= dstDesc->isNullable;
	cvtParm.cv_dstDesc.ds_length	= dstDesc->length;
	cvtParm.cv_dstDesc.ds_precision	= dstDesc->precision;
	cvtParm.cv_dstDesc.ds_scale	= dstDesc->scale;
	cvtParm.cv_dstDesc.ds_columnType= dstDesc->columnType;
	cvtParm.cv_dstDesc.ds_columnName= dstDesc->columnName;
	//
	// We provide the output buffer location, but IIapi_convert
	// will set the dv_null and dv_length attributes to reflect
	// the actual result of the conversion.
	//
	cvtParm.cv_dstValue.dv_value	= dstData->data;

	IIapi_convertData(&cvtParm);

	i4 status = cvtParm.cv_status;
	if (status != IIAPI_ST_SUCCESS)
	{
	    inst->impl->pushError(status, "IIconvertData");
	}
	else
	{
	    //
	    // Propagate the result to the caller's destination struct.
	    //
	    dstData->isNull = (u_i2)cvtParm.cv_dstValue.dv_null;
	    dstData->length = cvtParm.cv_dstValue.dv_length;
	}

	return status;
}


/*{
** Name: I_IIexecuteProcedure - Execute a database procedure
**
** Description:
**	This ToolsAPI function executes a database procedure and outputs a
**	statement handle that can then be used to obtain any byrefs or returned
**	rows (via the I_IIgetColumns function) and/or a procedure return value
**	(via the I_IIgetQueryInfo function).
**
**	The procedure name (and optional procedure owner) must be included in
**	the parallel parmDesc/parmData procedure parameter structures (as
**	required by the OpenAPI IIapi_setDescriptor/IIapi_putParms sequence
**	used for calling database procedures).  Page 150 of the Ingres OpenAPI
**	9.3 User Guide describes the necessary columnType and dataType values.
**
** Inputs:
**	inst		TOOLSAPI instance pointer.
**	connHandle	Connection handle.
**	pTranHandle	Pointer to transaction handle.  If the input transaction
**			handle is NULL, a new transaction will be initiated and
**			the transaction handle updated.  Otherwise, this call
**			will continue within the current transaction scope.
**	parmDesc	Procedure parameter descriptors.
**	parmData	Procedure parameter values.
**	pStmtHandle	Pointer to location to receive new statement handle.
**
** Outputs:
**	*pStmtHandle	Receives the new statement handle.
**	*pTranHandle	Receives new transaction handle (if a new transaction
**			was started).
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	May start a new transaction.
**
** History:
**	10-Feb-2011 (dansa01)
**	    Initial coding.
**	18-Feb-2011 (dansa01)
**	    Added MST support.  Transaction handle is now both input and output.
**	19-Apr-2011 (dansa01)
**	    Refactored to share doPutParms helper with I_IIputParameters.
*/
i4 TOOLSAPICALL
I_IIexecuteProcedure(TOOLSAPI *inst, II_HANDLE connHandle, II_HANDLE *pTranHandle, II_DESCRIPTORS *parmDesc, II_DATAVALUES *parmData, II_HANDLE *pStmtHandle)
{
	inst->impl->clearErrors();

	II_HANDLE stmtHandle = NULL;
	i4 status = IIAPI_ST_SUCCESS;

	i4 parmDescCount = parmDesc ? parmDesc->count : 0;
	i4 parmDataCount = parmData ? parmData->count : 0;

	if (parmDescCount != parmDataCount)
	{
	    return inst->impl->pushError(E_TA0108_UNEQUAL_DESC_DATA, "IIexecuteProcedure");
	}

	if (parmDescCount == 0)
	{
	    return inst->impl->pushError(E_TA0109_EMPTY_INPUT, "IIexecuteProcedure");
	}

	//
	// Now execute the "query" to obtain a statement handle, and then call
	// IIapi_setDescriptor and IIapi_putParms.
	//
	IIAPI_QUERYPARM queryParm = {0};

	queryParm.qy_connHandle = connHandle;
	queryParm.qy_queryType = IIAPI_QT_EXEC_PROCEDURE;
	queryParm.qy_parameters = TRUE;
	queryParm.qy_tranHandle = *pTranHandle;

	IIapi_query(&queryParm);

	wait_for_completion(&queryParm.qy_genParm, NO_TIMEOUT);
	status = checkErrors(inst, "IIexecuteProcedure:IIapi_query", &queryParm.qy_genParm);

	stmtHandle = queryParm.qy_stmtHandle;

	if (status == IIAPI_ST_SUCCESS)
	{
	    status = doPutParms(inst, stmtHandle, parmDesc, parmData);
	}

	if (status == IIAPI_ST_SUCCESS)
	{
	    *pStmtHandle = stmtHandle;

	    if (*pTranHandle != queryParm.qy_tranHandle)
	    {
		//
		// A new MST was initiated (with a new transaction handle).
		//
		*pTranHandle = queryParm.qy_tranHandle;
	    }
	}
	else
	{
	    //
	    // Release any handle resources that may remain after a failed
	    // request (and leave the current error status undisturbed).
	    //
	    if (stmtHandle)
	    {
		IIAPI_CANCELPARM cancelParm = {0};
		cancelParm.cn_stmtHandle = stmtHandle;
		IIapi_cancel(&cancelParm);
		wait_for_completion(&cancelParm.cn_genParm, CANCEL_TIMEOUT);

		IIAPI_CLOSEPARM closeParm = {0};
		closeParm.cl_stmtHandle = stmtHandle;
		IIapi_close(&closeParm);
		wait_for_completion(&closeParm.cl_genParm, NO_TIMEOUT);
	    }

	    if (*pTranHandle != queryParm.qy_tranHandle)
	    {
		IIAPI_ROLLBACKPARM rollbackParm = {0};
		rollbackParm.rb_tranHandle = queryParm.qy_tranHandle;
		IIapi_rollback(&rollbackParm);
		wait_for_completion(&rollbackParm.rb_genParm, NO_TIMEOUT);
	    }
	}

	return status;
}


/*{
** Name: I_IIputParameters - Put query parameters
**
** Description:
**	This ToolsAPI function sends query parameters to an OpenAPI query that
**	is expecting them.
**
** Inputs:
**	inst		TOOLSAPI instance pointer.
**	stmtHandle	Statement handle of the query expecting parameters.
**	parmDesc	Parameter descriptors.
**	parmData	Parameter values.
**
** Outputs:
**	None.
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	Changes query state.
**
** History:
**	19-Apr-2011 (dansa01)
**	    Initial coding.
*/
i4 TOOLSAPICALL
I_IIputParameters(TOOLSAPI *inst, II_HANDLE stmtHandle, II_DESCRIPTORS *parmDesc, II_DATAVALUES *parmData)
{
	inst->impl->clearErrors();

	i4 parmDescCount = parmDesc ? parmDesc->count : 0;
	i4 parmDataCount = parmData ? parmData->count : 0;

	if (parmDescCount != parmDataCount)
	{
	    return inst->impl->pushError(E_TA0108_UNEQUAL_DESC_DATA, "IIputParameters");
	}

	if (parmDescCount == 0)
	{
	    return inst->impl->pushError(E_TA0109_EMPTY_INPUT, "IIputParameters");
	}

	return doPutParms(inst, stmtHandle, parmDesc, parmData);
}


/*{
** Name: I_IIcommit - Commit the current transaction
**
** Description:
**	This ToolsAPI function commits a transaction that was started with
**	I_IIquery or I_IIexecuteProcedure, and frees the OpenAPI transaction
**	handle.
**
**	This operation is illegal if any statement handles within this
**	transaction have not been closed.
**
** Inputs:
**	inst		TOOLSAPI instance pointer.
**	tranHandle	Transaction handle.
**
** Outputs:
**	None.
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	Changes the transaction state and invalidates the transaction handle.
**
** History:
**	18-Feb-2011 (dansa01)
**	    Initial coding.
*/
i4 TOOLSAPICALL
I_IIcommit(TOOLSAPI *inst, II_HANDLE tranHandle)
{
	inst->impl->clearErrors();

	IIAPI_COMMITPARM commitParm = {0};
	commitParm.cm_tranHandle = tranHandle;

	IIapi_commit(&commitParm);

	wait_for_completion(&commitParm.cm_genParm, NO_TIMEOUT);
	return checkErrors(inst, "IIcommit", &commitParm.cm_genParm);
}


/*{
** Name: I_IIrollback - Rollback the current transaction
**
** Description:
**	This ToolsAPI function rolls back a transaction that was started with
**	I_IIquery or I_IIexecuteProcedure, and frees the OpenAPI transaction
**	handle.
**
**	This operation is illegal if any statement handles within this
**	transaction have not been closed.
**
** Inputs:
**	inst		TOOLSAPI instance pointer.
**	tranHandle	Transaction handle.
**
** Outputs:
**	None.
**
** Returns:
**	OpenAPI status code.
**
** Side Effects:
**	Changes the transaction state and invalidates the transaction handle.
**
** History:
**	18-Feb-2011 (dansa01)
**	    Initial coding.
*/
i4 TOOLSAPICALL
I_IIrollback(TOOLSAPI *inst, II_HANDLE tranHandle)
{
	inst->impl->clearErrors();

	IIAPI_ROLLBACKPARM rollbackParm = {0};
	rollbackParm.rb_tranHandle = tranHandle;

	IIapi_rollback(&rollbackParm);

	wait_for_completion(&rollbackParm.rb_genParm, NO_TIMEOUT);
	return checkErrors(inst, "IIrollback", &rollbackParm.rb_genParm);
}


void TOOLSAPICALL
I_MEfree(TOOLSAPI *inst, II_PTR ptr)
{
	inst->impl->clearErrors();  // (Just to follow a uniform pattern.)

#if ING_VER >= 930
	::MEfree(ptr);
#else
	::MEfree((PTR)ptr);
#endif
}


i4 TOOLSAPICALL
I_CMgetCharsetName(TOOLSAPI *inst, char *buf, i_size len)
{
	inst->impl->clearErrors();

	if (buf == NULL || len < CM_MAXATTRNAME+1)
	{
	    return inst->impl->pushError(E_TA0104_BUF_TOO_SMALL, "CMgetCharsetName");
	}
	else
	{
	    CMget_charset_name(buf);
	    return II_ST_SUCCESS;
	}
}


i4 TOOLSAPICALL
I_CMgetStdCharsetName(TOOLSAPI *inst, char *buf, i_size len)
{
	inst->impl->clearErrors();

	// TODO: the buffer in LC_getStdLocale is GL_MAXNAME + CM_MAXATTRNAME + 1
	// which is 256 + 8 + 1.  Empirically, the maximum size in the translation
	// table (xmlnames.map) is 12, so we'll pick a somewhat larger than that
	// value here and *hope* we don't overflow with that.
	if (buf == NULL || len < 20 + 1)
	{
	    return inst->impl->pushError(E_TA0104_BUF_TOO_SMALL, "CMgetStdCharsetName");
	}
	else
	{
	    CL_ERR_DESC err = {0};
	    buf[0] = '\0';

	    i4 status = LC_getStdLocale (NULL, buf, &err);
	    // Special error checking because this function is
	    // not quite robust in this area
	    if (status == FAIL && err.errnum == 0)
	    {
		// Only from "SIopen", assume locale map file isn't there
		return inst->impl->pushError(SI_CANT_OPEN, "CMgetStdCharsetName");
	    }
	    else if (status != OK)
	    {
		return inst->impl->pushNonAPIError(status, "CMgetStdCharsetName:LC_getStdLocale", &err);
	    }
	    else
	    {
		return II_ST_SUCCESS;
	    }
	}
}


i4 TOOLSAPICALL
I_UTreadVersionRel(TOOLSAPI *inst, char *buf, i_size len)
{
	inst->impl->clearErrors();

	LOCATION loc;
	FILE *ver;
	i4 status;

	status = ::NMloc(SUBDIR, FILENAME, "version.rel", &loc);
	if (status != OK)
	{
	    return inst->impl->pushNonAPIError(status, "UTreadVersionRel:NMloc", NULL);
	}

	status = ::SIopen(&loc, "r", &ver);
	if (status != OK)
	{
	    return inst->impl->pushNonAPIError(status, "UTreadVersionRel:SIopen", NULL);
	}

	if ((status = ::SIgetrec(buf, len, ver)) == OK)
	{
	    int n = strlen(buf);
	    if (buf[n-1] == '\n')
		buf[n-1] = '\0';
	}
	else
	{
	    status = inst->impl->pushNonAPIError(status, "UTreadVersionRel:SIgetrec", NULL);
	}

	// Always close, but don't hide any previous error.
	if (status == OK)
	{
	    status = ::SIclose(ver);
	    if (status != OK)
	    {
		status = inst->impl->pushNonAPIError(status, "UTreadVersionRel:SIclose", NULL);
	    }
	}
	else
	{
	    ::SIclose(ver);
	}

	return status;
}


i4 TOOLSAPICALL
I_NMgtAt(TOOLSAPI *inst, const char *name, char **ppValue)
{
	inst->impl->clearErrors();

	char *value = NULL;
	char *cstr = NULL;

	::NMgtAt(const_cast<char *>(name), &value);
	if (value != NULL)
	{
	    cstr = reinterpret_cast<char *>(MEreqmem(0, STlength(value) + 1, TRUE, NULL));
	    if (cstr)
	    {
		STcopy(value, cstr);
	    }
	    else
	    {
		return inst->impl->pushError(II_ST_OUT_OF_MEMORY, "NMgtAt");
	    }
	}

	*ppValue = cstr;

	return OK;
}


i4 TOOLSAPICALL
I_NMgtAtBuf(TOOLSAPI *inst, const char *name, char *buf, i_size len, i_size *pBytesUsed)
{
	inst->impl->clearErrors();

	char *value = NULL;
	::NMgtAt(const_cast<char *>(name), &value);

	i_size bytesUsed = strbufcpy(value, buf, len);

	if (pBytesUsed)
	    *pBytesUsed = bytesUsed;

	return OK;
}


i4 TOOLSAPICALL
I_NMsymbols(TOOLSAPI *inst, II_NAME_VALUE_PAIRS **ppPairs)
{
	inst->impl->clearErrors();

	i4 status = II_ST_SUCCESS;

	char *cbuf = NULL;
	char *nextString = NULL;

	StrQryResult result(inst);

	// Iterate through symbols and add to result
	for (SYM *s = s_list; s != NULL; s = s->s_next)
	{
	    status = result.addString(s->s_sym, STlength(s->s_sym));
	    if (status != II_ST_SUCCESS)
		break;

	    status = result.addString(s->s_val, STlength(s->s_val));
	    if (status != II_ST_SUCCESS)
		break;
	}

	if (status == II_ST_SUCCESS)
	{
	    status = result.getCBuffer(sizeof(II_NAME_VALUE_PAIRS),
				       sizeof(II_NAME_VALUE_PAIR),
				       2,
				       &cbuf, &nextString);
	}

	if (status == II_ST_SUCCESS)
	{
	    II_NAME_VALUE_PAIRS * pNVP = reinterpret_cast<II_NAME_VALUE_PAIRS *>(cbuf);
	    pNVP->count = result.getRowCount();

	    for (i4 i = 0; i < pNVP->count; i++)
	    {
		pNVP->pairs[i].name = nextString;
		nextString = result.copyNextToBuf(nextString);

		pNVP->pairs[i].value = nextString;
		nextString = result.copyNextToBuf(nextString);
	    }

	    *ppPairs = pNVP;
	}

	return status;
}


i4 TOOLSAPICALL
I_IDname(TOOLSAPI *inst, char **ppName)
{
	inst->impl->clearErrors();

	i4 status = OK;

	char name[256+1];
	char *np = name;
	char *cstr = NULL;

	::IDname(&np);

	cstr = reinterpret_cast<char *>(MEreqmem(0, STlength(np) + 1, TRUE, NULL));
	if (cstr)
	{
	    STcopy(np, cstr);
	    *ppName = cstr;
	}
	else
	{
	    status = inst->impl->pushError(II_ST_OUT_OF_MEMORY, "IDname");
	}

	return status;
}


i4 TOOLSAPICALL
I_IDnameBuf(TOOLSAPI *inst, char *buf, i_size len, i_size *pBytesUsed)
{
	inst->impl->clearErrors();

	char name[256+1];
	char *np = name;

	::IDname(&np);

	i_size bytesUsed = strbufcpy(np, buf, len);

	if (pBytesUsed)
	    *pBytesUsed = bytesUsed;

	return OK;
}


i4 TOOLSAPICALL
I_IDnameService(TOOLSAPI *inst, char **ppName)
{
	inst->impl->clearErrors();

	i4 status = OK;

	char name[256+1];
	char *np = name;
	char *cstr = NULL;

#ifdef NT_GENERIC
	status = IDname_service(&np);
#else
	::IDname(&np);
#endif
	if (status == OK)
	{
	    cstr = reinterpret_cast<char *>(MEreqmem(0, STlength(np) + 1, TRUE, NULL));
	    if (cstr)
	    {
		STcopy(np, cstr);
		*ppName = cstr;
	    }
	    else
	    {
		status = inst->impl->pushError(II_ST_OUT_OF_MEMORY, "IDnameService");
	    }
	}

	return status;
}


i4 TOOLSAPICALL
I_IDnameServiceBuf(TOOLSAPI *inst, char *buf, i_size len, i_size *pBytesUsed)
{
	inst->impl->clearErrors();

	i4 status = OK;

	char name[256+1];
	char *np = name;

#ifdef NT_GENERIC
	status = ::IDname_service(&np);
#else
	::IDname(&np);
#endif
	if (status == OK)
	{
	    i_size bytesUsed = strbufcpy(np, buf, len);

	    if (pBytesUsed)
		*pBytesUsed = bytesUsed;
	}
	else
	{
	    inst->impl->pushNonAPIError(status, "IDnameServiceBuf", NULL);
	}

	return status;
}


i4 TOOLSAPICALL
I_ElevationRequired(TOOLSAPI *inst)
{
	inst->impl->clearErrors();

#if ING_VER > 910
	/* See the definition of ELEVATION_REQUIRED in cl!hdr!hdr epcl.h */
	ELEVATION_REQUIRED();
#endif
	return OK;
}


i4 TOOLSAPICALL
I_APIgetVersion(TOOLSAPI *inst, const char **ppVers)
{
	inst->impl->clearErrors();

	*ppVers = ApiReleaseString;

	return OK;
}


i4 TOOLSAPICALL
I_ElevationRequiredWarning(TOOLSAPI *inst)
{
	inst->impl->clearErrors();

#if ING_VER > 910
	/* See the definition of ELEVATION_REQUIRED_WARNING in cl!hdr!hdr epcl.h */
	ELEVATION_REQUIRED_WARNING();
#endif
	return OK;
}


i4 TOOLSAPICALL
I_IsElevationRequired(TOOLSAPI *inst, i4 *pIsRequired)
{
	inst->impl->clearErrors();

	*pIsRequired = FALSE;

#if ING_VER > 910
// See the definition of ELEVATION_REQUIRED in cl!hdr!hdr epcl.h
#ifdef NT_GENERIC
	if (GVvista() && !PCisAdmin())
	    *pIsRequired = TRUE;
#endif
#endif
	return OK;
}


i4 TOOLSAPICALL
I_CheckPrivilege(TOOLSAPI *inst, const char *user, enum Privileges priv, i4 *pIsAllowed)
{
	inst->impl->clearErrors();

	*pIsAllowed = FALSE;  // (Fail secure by default.)

	// Assumes PMinit() and PMload() have already been called
	// See "ToolsAPIinit" above
	// If the enum is constructed correctly, the "priv" value will always be
	// a correct value and won't need to be checked here.
	*pIsAllowed = scs_chk_priv(const_cast<char *>(user), PrivilegeStrings[priv]);

	return OK;
}


i4 TOOLSAPICALL
I_GCusrpwd(TOOLSAPI *inst, const char *usr, const char *pwd)
{
	inst->impl->clearErrors();

	CL_ERR_DESC err = {0};
	i4 status;

	status = ::GCusrpwd(const_cast<char *>(usr), const_cast<char *>(pwd), &err);
	if (status != OK)
	{
	    /*
	    ** GC errors don't have message text, but there
	    ** are corresponding GCF errors which do.  Apply
	    ** some bit twiddling magic to convert a GC error
	    ** code into a GCF error code.
	    */
	    if ( (status & ~0xFF) == (E_CL_MASK | E_GC_MASK) )
		status ^= (E_CL_MASK | E_GC_MASK) ^ E_GCF_MASK;

	    status = inst->impl->pushNonAPIError(status, "GCusrpwd", &err);
	}

	return status;
}


i4 TOOLSAPICALL
I_GCtcpIpPort(TOOLSAPI *inst, const char *pin, i4 subport, char *pout, char *poutSymbolic)
{
	inst->impl->clearErrors();

	i4 status;

#ifdef NT_GENERIC
	status = ::GCtcpip_port(const_cast<char *>(pin), subport, pout, poutSymbolic);
#else
	status = ::BS_tcp_port(const_cast<char *>(pin), subport, pout, poutSymbolic);
#endif
	if (status != OK)
	{
	    status = inst->impl->pushNonAPIError(status, "GCtcpip_port", NULL);
	}

	return status;
}


i4 TOOLSAPICALL
I_PMinit(TOOLSAPI *inst, II_HANDLE *pCtxHandle)
{
	inst->impl->clearErrors();

	i4 status;
	PM_CONTEXT *pctx;

	status = ::PMmInit(&pctx);
	if (status != OK)
	{
	    return inst->impl->pushNonAPIError(status, "PMinit", NULL);
	}

	::PMmLowerOn(pctx);

	// Set the 0th and 1st defaults that are not easily set by Java callers
	status = ::PMmSetDefault(pctx, 0, SystemCfgPrefix);
	if (status != OK)
	{
	    ::PMmFree(pctx);
	    return inst->impl->pushNonAPIError(status, "PMinit:PMsetDefault", NULL);
	}

	status = ::PMmSetDefault(pctx, 1, ::PMhost());
	if (status != OK)
	{
	    ::PMmFree(pctx);
	    return inst->impl->pushNonAPIError(status, "PMinit:PMsetDefault", NULL);
	}

	*pCtxHandle = (II_HANDLE)pctx;

	return status;
}


i4 TOOLSAPICALL
I_PMfree(TOOLSAPI *inst, II_HANDLE ctxHandle)
{
	inst->impl->clearErrors();

	i4 status = OK;

	if (ctxHandle == II_NULL_HANDLE)
	{
	    // Not allowed since we do it ourselves in the APIterm
	    status = inst->impl->pushNonAPIError(II_ST_INVALID_HANDLE, "PMfree", NULL);
	}
	else
	{
	    ::PMmFree(reinterpret_cast<PM_CONTEXT *>(ctxHandle));
	}

	return status;
}


i4 TOOLSAPICALL
I_PMdelete(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey)
{
	inst->impl->clearErrors();

	i4 status;

	if (ctxHandle == II_NULL_HANDLE)
	    status = ::PMdelete(const_cast<char *>(pmkey));
	else
	    status = ::PMmDelete(reinterpret_cast<PM_CONTEXT *>(ctxHandle), const_cast<char *>(pmkey));

	if (status == PM_NOT_FOUND)
	{
	    status = OK; // NOT FOUND is equivalent to a successful delete
	}
	else if (status != OK)
	{
	    inst->impl->pushNonAPIError(status, "PMdelete", NULL);
	}

	return status;
}


i4 TOOLSAPICALL
I_PMinsert(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, const char *value)
{
	inst->impl->clearErrors();

	i4 status;

	if (ctxHandle == II_NULL_HANDLE)
	    status = ::PMinsert( const_cast<char *>(pmkey), const_cast<char *>(value) );
	else
	    status = ::PMmInsert( reinterpret_cast<PM_CONTEXT *>(ctxHandle), const_cast<char *>(pmkey), const_cast<char *>(value) );

	if (status != OK)
	{
	    inst->impl->pushNonAPIError(status, "PMinsert", NULL);
	}

	return status;
}


i4 TOOLSAPICALL
I_PMnumElem(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, i4 *pNum)
{
	inst->impl->clearErrors();

	i4 num;

	if (ctxHandle == II_NULL_HANDLE)
	    num = ::PMnumElem( const_cast<char *>(pmkey) );
	else
	    num = ::PMmNumElem( reinterpret_cast<PM_CONTEXT *>(ctxHandle), const_cast<char *>(pmkey) );

	*pNum = num;

	return OK;
}


i4 TOOLSAPICALL
I_PMlowerOn(TOOLSAPI *inst, II_HANDLE ctxHandle)
{
	inst->impl->clearErrors();

	if (ctxHandle == II_NULL_HANDLE)
	    ::PMlowerOn();
	else
	    ::PMmLowerOn( reinterpret_cast<PM_CONTEXT *>(ctxHandle) );

	return OK;
}


static void PMerrFunc(STATUS status, i4 num, ER_ARGUMENT args[])
{
	UNREFERENCED(num);
	UNREFERENCED(args);
	TRdisplay("%s %@ Error in PMload parsing: %d\n", API, status);
}

i4 TOOLSAPICALL
I_PMload(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *path)
{
	inst->impl->clearErrors();

	LOCATION loc, *ploc = &loc;
	char buffer[MAX_LOC];

	i4 status = OK;

	if (path == NULL || *path == '\0')
	{
	     ploc = NULL;
	}
	else
	{
	    if (STlength(path) > MAX_LOC - 1)
	    {
		return inst->impl->pushError(E_TA0106_PATH_TOO_LONG, "PMload");
	    }
	    else
	    {
		STcopy(path, buffer);

		status = ::LOfroms(NODE & PATH & FILENAME, buffer, ploc);
		if (status != OK)
		{
		    return inst->impl->pushNonAPIError(status, "PMload:LOfroms", NULL);
		}
	    }
	}

	if (ctxHandle == II_NULL_HANDLE)
	    status = ::PMload(ploc, PMerrFunc);
	else
	    status = ::PMmLoad(reinterpret_cast<PM_CONTEXT *>(ctxHandle), ploc, PMerrFunc);

	if (status != OK)
	{
	    inst->impl->pushNonAPIError(status, "PMload", NULL);
	}

	return status;
}


i4 TOOLSAPICALL
I_PMwrite(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *path)
{
	inst->impl->clearErrors();

	LOCATION loc, *ploc = &loc;
	char buffer[MAX_LOC];

	i4 status = OK;

	if (path == NULL || *path == '\0')
	{
	     ploc = NULL;
	}
	else
	{
	    if (STlength(path) > MAX_LOC - 1)
	    {
		return inst->impl->pushError(E_TA0106_PATH_TOO_LONG, "PMwrite");
	    }
	    else
	    {
		STcopy(path, buffer);

		status = ::LOfroms(NODE & PATH & FILENAME, buffer, ploc);
		if (status != OK)
		{
		    return inst->impl->pushNonAPIError(status, "PMwrite:LOfroms", NULL);
		}
	    }
	}

	if (ctxHandle == II_NULL_HANDLE)
	    status = ::PMwrite(ploc);
	else
	    status = ::PMmWrite(reinterpret_cast<PM_CONTEXT *>(ctxHandle), ploc);

	if (status != OK)
	{
	    inst->impl->pushNonAPIError(status, "PMwrite", NULL);
	}

	return status;
}


i4 TOOLSAPICALL
I_PMsetDefault(TOOLSAPI *inst, II_HANDLE ctxHandle, i4 idx, const char *value)
{
	inst->impl->clearErrors();

	i4 status;

	if (ctxHandle == II_NULL_HANDLE)
	    status = ::PMsetDefault(idx, const_cast<char *>(value));
	else
	    status = ::PMmSetDefault(reinterpret_cast<PM_CONTEXT *>(ctxHandle), idx, const_cast<char *>(value));

	if (status != OK)
	{
	    inst->impl->pushNonAPIError(status, "PMsetDefault", NULL);
	}

	return status;
}


i4 TOOLSAPICALL
I_PMhost(TOOLSAPI *inst, II_HANDLE ctxHandle, const char **ppHostName)
{
	inst->impl->clearErrors();

	char *hostName;
	// Interesting note here:  the PM_CONTEXT is not used
	// internally to this function -- both return a pointer
	// to a static buffer that is initialized once on first
	// use, so all of the code in here is just overhead
	if (ctxHandle == II_NULL_HANDLE)
	    hostName = ::PMhost();
	else
	    hostName = ::PMmHost(reinterpret_cast<PM_CONTEXT *>(ctxHandle));

	i4 status = OK;

	if (!hostName)
	{
	    status = inst->impl->pushError(II_ST_ERROR, "PMhost");
	}
	else
	{
	    *ppHostName = const_cast<const char *>(hostName);
	}

	return status;
}


i4 TOOLSAPICALL
I_PMrestrict(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *regexp)
{
	inst->impl->clearErrors();

	i4 status;

	if (ctxHandle == II_NULL_HANDLE)
	    status = ::PMrestrict(const_cast<char *>(regexp));
	else
	    status = ::PMmRestrict(reinterpret_cast<PM_CONTEXT *>(ctxHandle), const_cast<char *>(regexp));

	if (status != OK)
	{
	    inst->impl->pushNonAPIError(status, "PMrestrict", NULL);
	}

	return status;
}


i4 TOOLSAPICALL
I_PMexpToRegExp(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmexp, const char **ppRegexp)
{
	inst->impl->clearErrors();

	char *regexp;

	if (ctxHandle == II_NULL_HANDLE)
	    regexp = ::PMexpToRegExp(const_cast<char *>(pmexp));
	else
	    regexp = ::PMmExpToRegExp(reinterpret_cast<PM_CONTEXT *>(ctxHandle), const_cast<char *>(pmexp));

	i4 status = OK;
	if (!regexp)
	{
	    status = inst->impl->pushNonAPIError(II_ST_ERROR, "PMexpToRegExp", NULL);
	}
	else
	{
	    *ppRegexp = const_cast<const char *>(regexp);
	}

	return status;
}


II_ERRORS* TOOLSAPICALL
I_getLastErrors(TOOLSAPI *inst)
{
	return inst->impl->getErrors();
}


i4 TOOLSAPICALL
I_PMget(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, const char **ppValue)
{
	inst->impl->clearErrors();

	char *value;
	i4 status;

	if (ctxHandle == II_NULL_HANDLE)
	    status = ::PMget(const_cast<char *>(pmkey), &value);
	else
	    status = ::PMmGet(reinterpret_cast<PM_CONTEXT *>(ctxHandle), const_cast<char *>(pmkey), &value);

	if (status == OK)
	{
	    *ppValue = const_cast<const char *>(value);
	}
	else if (status == PM_NOT_FOUND)
	{
	    status = OK;
	    *ppValue = NULL;
	}
	else
	{
	    inst->impl->pushNonAPIError(status, "PMget", NULL);
	}

	return status;
}


i4 TOOLSAPICALL
I_PMgetDefault(TOOLSAPI *inst, II_HANDLE ctxHandle, i4 idx, const char **ppValue)
{
	inst->impl->clearErrors();

	char *value;

	if (ctxHandle == II_NULL_HANDLE)
	    value = ::PMgetDefault(idx);
	else
	    value = ::PMmGetDefault(reinterpret_cast<PM_CONTEXT *>(ctxHandle), idx);

	//
	// NULL is a valid return from PMgetDefault, so there is no way to
	// detect a failure status (in the local case).
	//
	*ppValue = const_cast<const char *>(value);

	return OK;
}


i4 TOOLSAPICALL
I_PMscan(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmexp, II_NAME_VALUE_PAIRS **ppPairs)
{
	inst->impl->clearErrors();

	i4 status = OK;

	PM_CONTEXT *pCtx = reinterpret_cast<PM_CONTEXT *>(ctxHandle);
	PM_SCAN_REC pmrec = {0};
	char *pExpr = const_cast<char *>(pmexp);
	char *name, *value;

	StrQryResult result(inst);
	while (status == OK)
	{
	    if (pCtx == NULL)
		status = ::PMscan(pExpr, &pmrec, NULL, &name, &value);
	    else
		status = ::PMmScan(pCtx, pExpr, &pmrec, NULL, &name, &value);

	    pExpr = NULL;

	    if (status == PM_NOT_FOUND)
	    {
		status = OK; // normal termination
		break;
	    }
	    else if (status == OK)
	    {
		status = result.addString(name, STlength(name));
		if (status == OK)
		    status = result.addString(value, STlength(value));
	    }
	    else
	    {
		inst->impl->pushNonAPIError(status, "PMscan", NULL);
	    }
	}

	char *cbuf = NULL;
	char *nextString = NULL;

	if (status == OK)
	{
	    status = result.getCBuffer(sizeof(II_NAME_VALUE_PAIRS),
				       sizeof(II_NAME_VALUE_PAIR),
				       2,
				       &cbuf, &nextString);
	}

	if (status == OK)
	{
	    II_NAME_VALUE_PAIRS * pNVP = reinterpret_cast<II_NAME_VALUE_PAIRS *>(cbuf);
	    pNVP->count = result.getRowCount();

	    for (i4 i = 0; i < pNVP->count; i++)
	    {
		pNVP->pairs[i].name = nextString;
		nextString = result.copyNextToBuf(nextString);

		pNVP->pairs[i].value = nextString;
		nextString = result.copyNextToBuf(nextString);
	    }

	    *ppPairs = pNVP;
	}

	return status;
}


i4 TOOLSAPICALL
I_GChostname(TOOLSAPI *inst, char *name, i4 len, u_i2 fullyQualified)
{
	inst->impl->clearErrors();

	i4 status = OK;

	if (fullyQualified)
	{
	    status = GCdns_hostname(name, len);
	    if (status != OK)
	    {
		inst->impl->pushNonAPIError(status, "GChostname", NULL);
	    }
	}
	else
	{
	    GChostname(name, len);
	}

	return status;
}


i4 TOOLSAPICALL
I_CRsetPMval(TOOLSAPI *inst, II_HANDLE ctxHandle, const char *pmkey, const char *value)
{
	inst->impl->clearErrors();

	i4 status = OK;
	i4 info;
	FILE	*change_log_fp;
	LOCATION change_log_file;
	ER_ARGUMENT arg;

	// Note: this code is mostly copied from "iisetres.c" and "iiremres.c"
	PM_CONTEXT *pCtx = reinterpret_cast<PM_CONTEXT *>(ctxHandle);

	CRinit( pCtx );

	switch( CRload( NULL, &info ) )
	{
	    case OK:
		break;

	    case CR_NO_RULES_ERR:
		status = S_ST0418_no_rules_found;
		inst->impl->pushError(status, "CRsetPMval", 0, NULL);
		return status;

	    case CR_RULE_LIMIT_ERR:
		status = S_ST0416_rule_limit_exceeded;
		arg.er_size = ER_PTR_ARGUMENT;
		arg.er_value = reinterpret_cast<char*>(&info);
		inst->impl->pushError(status, "CRsetPMval", 1, &arg);
		return status;

	    case CR_RECURSION_ERR:
		status = S_ST0417_recursive_definition;
		arg.er_size = ER_PTR_ARGUMENT;
		arg.er_value = CRsymbol(info);
		inst->impl->pushError(status, "CRsetPMval", 1, &arg);
		return status;
	}

	/* prepare LOCATION for config.log */
	NMloc( LOG, FILENAME, ERx( "config.log" ), &change_log_file );

	/* open change log file for appending */
	if( SIfopen( &change_log_file , ERx( "a" ), SI_TXT, SI_MAX_TXT_REC,
		&change_log_fp ) != OK )
	{
	    // TODO: where to put ELEVATION_REQUIRED_WARNING for this operation?
	    status = S_ST0323_unable_to_open;
	    arg.er_size = ER_PTR_ARGUMENT;
	    arg.er_value = "config.log";
	    inst->impl->pushError(status, "CRsetPMval", 1, &arg);
	    return status;
	}

	CRsetPMval( const_cast<char*>(pmkey), const_cast<char*>(value), change_log_fp, FALSE, TRUE );

	/* close change log after appending */
	if (change_log_fp)
	    (void) SIclose( change_log_fp );

	return status;
}

extern "C"
{
    /* Return null function to pathname verification routine. */
    BOOLFUNC *CRpath_constraint( CR_NODE *n, u_i4 un )
    {
	UNREFERENCED(n);
	UNREFERENCED(un);
	return NULL;
    }

    /*
    ** Would like to do this here and override the PMerror in "util.c"
    ** but that is a static link library and so we get a duplicate symbol error
    void
    PMerror( STATUS status, i4  nargs, ER_ARGUMENT *args )
    {
	// TODO: would like to do this, but don't have the "inst" context here...
	// inst->impl->pushError(status, "CRsetPMval", nargs, args);
    }
    */
}


i4 TOOLSAPICALL
I_LGgetLogPaths(TOOLSAPI *inst, i4 whichLog, const char *nodename, char paths[][256], i4 *pNumPaths)
{
	inst->impl->clearErrors();

	i4 status = OK;

	// Note: this code is pretty much copied from "lgopen.c" without the "uleFormat" calls
	// and returning NO_DATE for "error" conditions like not finding the log names in "config.dat"
	char        *fstring;
	char        *nstring;
	char        resource[MAX_LOC];
	LOCATION    loc;
	char        file_ext[CX_MAX_NODE_NAME_LEN+2];
	char        file_name[256];
	i4          i;

	/* Construct the filename(s) -- if not defined it is an error */
	/*
	** This may now be an array of partitioned filenames, derived from the PM
	** variables ii.<node>.rcp.log.log_file_N. For now we will
	** force the filenames to be in a contiguous list, i.e. we will
	** expect to see log_file_1, log_file_2 etc. the user will not be
	** allowed to skip log_file numbers, any files not in contiguous
	** order will be ignored
	**
	** If partitioned log files are not defined, fall back to using
	** the NM variables II_LOG_FILE/II_LOG_FILE_NAME, etc., to construct
	** a single non-partitioned log file name.
	*/
	file_ext[0] = '\0';
	if ( CXcluster_enabled() )
	{
	    if ( NULL == nodename || '\0' == *nodename )
	    {
		nodename = CXnode_name(NULL);
	    }
	    CXdecorate_file_name( file_ext, const_cast<char*>(nodename) );
	}
	else
	{
	    nodename = "$";
	}

	if (whichLog == LG_PRIMARY_TRANS_LOG)
	{
	    STprintf( resource, "%s.%s.rcp.log.log_file_name",
		      SystemCfgPrefix, nodename );
	}
	else /* Dual log */
	{
	    STprintf( resource, "%s.%s.rcp.log.dual_log_name",
		      SystemCfgPrefix, nodename );
	}
	status = PMget(resource, &nstring);
	if (status == PM_NOT_FOUND || !nstring || !*nstring)
	{
	    if (whichLog == LG_PRIMARY_LOG)
		status = E_DMA477_LGOPEN_LOGFILE_NAME;
	    else
		status = E_DMA47B_LGOPEN_DUALLOG_NAME;
	    inst->impl->pushError(status, "LGgetLogPaths", 0, NULL);
	    return(II_ST_NO_DATA);
	}

	for  (i = 0; i < LG_MAX_FILE; i++)
	{
	    if (whichLog == LG_PRIMARY_TRANS_LOG)
	    {
		STprintf( resource, "%s.%s.rcp.log.log_file_%d",
		    SystemCfgPrefix, nodename, i + 1);
	    }
	    else /* Dual log */
	    {
		STprintf( resource, "%s.%s.rcp.log.dual_log_%d",
		    SystemCfgPrefix, nodename, i + 1);
	    }
	    STprintf(file_name, "%s.l%02d%s", nstring, i + 1, file_ext);
	    file_name[255] = '\0';

	    status = PMget(resource, &fstring);

	    if (status == PM_NOT_FOUND || !fstring || !*fstring)
	    {
		break;
	    }

	    STcopy(fstring, resource);
	    /* Construct the path and file names using LO magic. */
	    status = LOfroms(PATH, resource, &loc);
	    if (status == OK)
	    {
		status = LOfaddpath(&loc, "ingres", &loc);

		if (status == OK)
		{
		    status = LOfaddpath(&loc, "log", &loc);

		    if (status == OK)
		    {
			status = LOfstfile(file_name, &loc);
		    }
		}
	    }
	    if (status)
	    {
		status = E_DMA47A_LGOPEN_PATH_ERR;
		inst->impl->pushError(status, "LGgetLogPaths", 0, NULL);
		return status;
	    }

	    LOtos(&loc, &fstring);

	    STcopy(fstring, paths[i]);
	}

	*pNumPaths = i;
	/*
	** Null out the last log file path to indicate the end of the list
	*/
	if (i < LG_MAX_FILE)
	    *paths[i] = 0;

	if (i == 0)
	{
	    if (whichLog == LG_PRIMARY_LOG)
		status = E_DMA478_LGOPEN_LOGFILE_PATH;
	    else
		status = E_DMA47C_LGOPEN_DUALLOG_PATH;
	    inst->impl->pushError(status, "LGgetLogPaths", 0, NULL);
	    return(II_ST_NO_DATA);
	}

	return (OK);
}


