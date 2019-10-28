/*
** Copyright 2008-2011 Actian Corporation. All rights reserved
**
** Description:
**	Wrapper around Actian Tools API functions to make them
**	available to Java-based programs.
**
** History:
**    March-2008 (whiro01)
**	Initial prototype version.
**    05-Dec-2009 (whiro01)
**	Converted completely to C++, added some more functions
**	to demonstrate feasibility.  Added many new Ingres header
**	files necessary to get deep into the internal functions.
**    15-Dec-2009 (whiro01)
**	Changed to use the new handle API for OpenAPI handles
**	(instead of just passing around the raw C pointers).
**    04-Jan-2010 (whiro01)
**	Adding stuff to support the "invocation" program so we
**	don't have to do "system('java ...')" to run our jar files.
**	Add function comments.
**    07-Jan-2010 (whiro01)
**	Rearranged OnUnload stuff because it never gets called into
**	_term_() method which MUST BE CALLED by main program of caller.
**    12-Jan-2010 (whiro01)
**	Rearranged again to put _term_() into the 'atexit' list so
**	Java callers won't have to worry about it.
**    13-Jan-2010 (whiro01)
**	This last fix didn't work because the JVM is probably not
**	active by 'atexit' time, so the JNIEnv pointer we got had
**	a NULL function table pointer, resulting in a crash.  So,
**	put the JNI-related termination into OnUnload and all other
**	stuff in _term_().
**    05-Apr-2010 (whiro01)
**	Added "loadInstallation" and "unloadInstallation" methods and
**	drastically change the call interface so that shared libraries
**	from multiple installations can be loaded simultaneously.  The
**	"Instance" pointer is kept in the Java object (and managed by
**	the handle routines) which is a virtual-function table pointer
**	provided by the "GetInterface" method of the loaded Tools library.
**    20-Apr-2010 (whiro01)
**	Drastic rearrangement to move the guts of the API into the C++
**	code and make this a much thinner wrapper.
**    02-Jun-2010 (whiro01)
**	Fixed SysInfo to create a VersionInfo structure for "version"
**	field; made a createVersionInfo() method to help.
**    17-Jun-2010 (whiro01)
**	Made JNU_GetStringUTFChars into an inline wrapper for
**	the simple jenv->GetStringUTFChars(); then changed all
**	"free(str)" into JNU_FreeStringUTFChars() which just
**	calls jenv->ReleaseStringUTFChars();
**    22-Jun-2010 (whiro01)
**	Part of rework of the Tools API: rename ToolsAPI to
**	ToolsInterface.
**    23-Jun-2010 (whiro01)
**	Removed KeyValuePair since I'm not using it anymore;
**	moved the data conversion stuff into the C/C++ API.
**    29-Jun-2010 (whiro01)
**	Check for NULL descriptor vector coming back from the C++
**	API in getDescriptors.
**    01-Jul-2010 (whiro01)
**	More functions implemented; changed HashMap to TreeMap
**	so that natural ordering of keys is preserved and Java
**	results match C++ results.
**    08-Jul-2010 (whiro01)
**	Minor fix caught by OSX 64-bit compiler (CallObjectMethod
**	should be CallIntMethod).
**    20-Jul-2010 (whiro01)
**	Add optional define NO_DEBUG_BREAK to build with DEBUG
**	(as with Ingres 9.1) and disable the "INT 3" break.
**    21-Jul-2010 (whiro01)
**	Deal with NULL inst pointer if "IIloadInstallation" fails
**	to find an installation there; canonicalize the installation
**	path so the "equals" in the "equals" method of SysInfo works.
**    27-Jul-2010 (whiro01)
**	Throwing exceptions when "version.rel" is not found; cleanup
**	logging in various areas (esp. termination).
**    24-Aug-2010 (whiro01)
**	Updates for new vector datatypes in C++ API.
**    20-Sep-2010 (whiro01)
**	Eliminate the explicit IIinitialize() and IIterminate() calls and
**	associated env handles because we're doing that under the covers in
**	APIinit() and APIterm().
**    21-Sep-2010 (dansa01)
**	Added first pass at the Netu subset of the API.
**    04-Oct-2010 (dansa01)
**	Added Java classes and helper functions to support a simplified
**	handle management mechanism.  (This is not yet being used.)
**	These new classes are not nested in IIapi, and that required a
**	small change to JNU_GetStructFieldIDs, and to the way our ClassInfo
**	structure initializations are done, to distinguish between nested
**	and non-nested classes.  Also consolidated frequently-used exception
**	name strings into shared definitions (following the example of
**	IIapi_exception).
**    05-Oct-2010 (dansa01)
**	Moved Netu data classes out of IIapi, into com.ingres package.
**    14-Oct-2010 (dansa01)
**	Replaced all calls to toolsapi.dll methods having C++ STL-based input
**	or output parameters with calls to their new implementations using plain
**	C-compatible structs.  This required substantial changes here to work
**	with C structs instead of STL objects.  Added some checks for uncommon
**	error conditions, but there is still more work needed here to make the
**	error handling fully robust.  Also reformatted some things (which I am
**	in general reluctant to do, but decided it would be worthwhile to move
**	toward closer conformance with Ingres coding standards).
**    25-Oct-2010 (whiro01)
**	Parse platform identifier out of release string and put into VersionInfo.
**    01-Nov-2010 (dansa01)
**	Adapted to new JNI-like TOOLSAPI interface (renamed from ToolsInterface).
**    09-Nov-2010 (dansa01)
**	Modified GetToolsAPIInstance signature to return an error status, which
**	will become necessary in the future when we have more failure modes than
**	simply OutOfMemory.
**    15-Nov-2010 (dansa01)
**	Removed the obsolete ErrDesc type.  Now all error information will be
**	reported to Java code via exceptions (although the actual conversion of
**	the C-API II_ERRORS reporting structure into IIapi.Exception objects has
**	not been implemented yet).
**    19-Nov-2010 (dansa01)
**	Implemented throwLastAPIErrors to convert from our C-API II_ERRORS
**	struct into a chain of IIapi.Exception objects.  Also cleaned up the
**	"unused variable" warning in loadInstallation.  Note that at this time
**	only a few of our JNI functions actually use throwLastAPIErrors.  That
**	will be an on-going task.
**    23-Nov-2010 (dansa01)
**	Adapted to new signatures for the OpenAPI wrapper functions, now that
**	they all return an i4 status.  Also added some missing error checks.
**	NOTE: Some of our Java methods still return status codes, but those
**	status values are moot now that we are using exceptions.  The client
**	will never actually see a non-zero return code, because an exception
**	will have been thrown.  Still, our JNI signatures require a return
**	value, so you will still see some moot "return II_ST_ERROR" and
**	"return status" statements here.
**    29-Nov-2010 (dansa01)
**	Continuing the conversion to a more uniform signature model, now
**	with the PM wrapper functions.  Also added more error handling.
**    02-Dec-2010 (dansa01)
**	Completed conversion to a more uniform signature model.  Also added
**	more error checking (and changed some error checks to use a more
**	consistent style throughout).
**    03-Dec-2010 (whiro01)
**	Corrected small compile problem on OSX (gcc 4.2.1).
**    06-Dec-2010 (dansa01)
**	Following the example of JDBC, don't link our IIapi.Exception chains
**	via the cause field.  Use a separate private field, and provide a
**	getNextException method.  Removed integer status return values
**	from all methods because we are now using exceptions to report errors.
**	Replaced long handle values with typesafe handle object references.
**	Used IllegalArgumentException instead of IIapi.Exception for a can't-
**	happen programming error in getJavaDataObject, since we don't want to
**	throw IIapi.Exception anymore without filling in the type, code, and
**	source fields.  Removed now-unnecessary JNH initialization, termination,
**	and header include.
**    08-Dec-2010 (dansa01)
**	Missed a couple IIapi_exception throws in getInstancePtr.  Changed them
**	to IllegalStateException.  Also removed references to ScanHandle class.
**    14-Dec-2010 (dansa01)
**	Restructured to help balance out source file sizes, and to prepare for
**	adding charset conversion classes and functions.  Consolidated all
**	cached class references, methodIDs, fieldIDs and their initialization
**	code into IIapi_jni_util.cpp.  Stopped treating the IIapi class
**	reference, methodID and fieldID as a special case, and just folded those
**	into the same structures as all the other classes.  Also moved most of
**	the static helper functions into IIapi_jni_util.cpp.  Now IIapi_jni.cpp
**	is basically just JNI entrypoints.
**    15-Dec-2010 (dansa01)
**	Added basic support for Ingres (and Native) charset conversions.  Moved
**	IIapi instance creation out of JNI and up into IIapi.java (which makes
**	it easier to map exceptions and do other charset-related work).  This is
**	just phase one.  Retrofitting existing code to fully use these new tools
**	will be phase two.
**    16-Dec-2010 (dansa01)
**	Fixed a very nasty bug (my bad!) in getColumns where the descriptor
**	array was being freed prematurely.  (Many thanks to Roger for debugging
**	this!)
**    17-Dec-2010 (dansa01)
**	Applied new charset-aware utility functions to all string handling.
**	Using the Ingres charset in most places, but using Native charset in
**	places that seem more closely tied to the system than a database (like
**	file paths).  The choices of which charset to use were difficult, and
**	somewhat arbitrary.  And if the Ingres and Native charsets are not the
**	same, we are probably hosed anyway!  Also added the new private native
**	method "bootstrapCharsetName" to ensure that we never try to use Ingres
**	charset conversions before the appropriate Ingres CharSet object has
**	been attached to the IIapi instance.
**    20-Dec-2010 (dansa01)
**	The compiler's memory is a whole lot better than mine, so let the
**	compiler take care of calling free() at the appropriate time after we
**	do a malloc.  Replaced some manual malloc/free usage with the new
**	MallocFreePtr template.
**    21-Dec-2010 (dansa01)
**	The OSX compiler wants an explicit cast on the MallocFreePtr that is
**	passed to a varargs function (jniLogger->error call in IIgetColumns).
**    03-Jan-2011 (whiro01)
**	Removed unneeded connection handle from autoCommitOff signature.
**    14-Jan-2011 (dansa01)
**	The cancel method should not invalidate the statement handle.  Only
**	the close method should do that.
**    19-Jan-2011 (dansa01)
**	Added more error checks and tracing during DLL initialization.
**    09-Feb-2011 (dansa01)
**	Added a generalized connect method that supports optional connection
**	parameters, and removed native implementations of connectDatabase
**	and connectNameServer.  (Also took this opportunity to make some minor
**	improvements: to be tidy, abort should clear the ConnectionHandle, and
**	autoCommitOff should clear the TransactionHandle.)
**    12-Feb-2011 (dansa01)
**	Added executeProcedure method.  Also beefed up null object protection
**	in getColumns and checkPrivilege methods.
**    18-Feb-2011 (dansa01)
**	Added commit and rollback procedures.  Adapted JNI layer to new
**	signatures for IIquery and IIexecuteProcedure, since the transaction
**	handle is now both input and output.  The Java signatures have not
**	changed, but the semantics have.  The TransactionHandle object content
**	may now be updated by the query and executeProcedure methods.
**    03-Mar-2011 (whiro01)
**	Improved error reporting if ToolsAPIInit fails; moved ToolsAPIInit
**	and ToolsAPITerm calls around so they happen in better places.
**    11-Mar-2011 (whiro01)
**	Casting to satisfy "gcc" re: IngresChars passed to "...".
**    20-Apr-2011 (dansa01)
**	Refactored executeProcedure method to extract parameter conversion
**	code that can be shared with a new putParameters method (coming soon).
**    21-Apr-2011 (dansa01)
**	Added putParameters method and bumped the ApiReleaseString version.
**    21-Apr-2011 (dansa01)
**	Removed obsolete NetuXxx methods, and bumped ApiReleaseString version.
**    22-Apr-2011 (whiro01)
**	Change "getAPIVersionInfo" and "getAPIVersionString" to be instance
**	methods so they can be overridden by the remote instance; bump
**	ApiReleaseString again; add detection of installation type for
**	VersionInfo.
**    06-May-2011 (gordy)
**	Added GCusrpwd method.
**    04-Jun-2011 (whiro01)
**	Added defines for Linux.
**    06-Jun-2011 (gordy)
**	Renamed native methods so that Java cover methods could be created
**	with the original names to do authorization checks prior to calls:
**	    executeUtility -> nativeExecuteUtility
**	    IDname -> nativeIDname
**	    PMdelete -> nativePMdelete
**	    PMinsert -> nativePMinsert
**	    PMwrite -> nativePMwrite
**	    UTcreateDatabase -> createDatabase
**	    UTdestroyDatabase -> destroyDatabase
**	    UTstartIngres -> startIngres
**	    UTstopIngres -> stopIngres
**    13-Jun-2011 (whiro01)
**	Better error checking in JNI_OnLoad; return JVM reported version
**	in case newer versions return a later version than the one
**	we require.
**    13-Jun-2011 (gordy)
**	Add effUser to CreateDBOptions.
**	Renamed native methods so that Java cover methods can be created
**	with the original names to do authorization checks prior to calls:
**	    abort -> nativeAbort
**	    disconnect -> nativeDisconnect
**	    query -> nativeQuery
**    20-Jun-2011 (dansa01)
**	Massive (but mechanical) renaming of functions.  Now that IIapi is an
**	interface, the class representing our local JNI implementation is now
**	"LocalIIapi", and all these function names had to adapt.  Removed the
**	parseVersionString method because that is needed in both the local and
**	remote cases, so it is now handled in pure Java with no JNI dependency.
**	Also bumped the ApiReleaseString version, just because this class
**	restructuring is a major change to the underlying architecture.
**    28-Jun-2011 (dansa01)
**	Adapting to (but not yet utilizing) the new logfile parameter on the
**	C-API functions UTexecuteUtility, UTstartIngres, UTstopIngres.
**    29-Jun-2011 (dansa01)
**	Adapting to (but not yet utilizing) the new logfile parameter on the
**	C-API functions UTcreateDatabase and UTdestroyDatabase.
**    29-Jun-2011 (dansa01)
**	Now exposing the new logfile parameter at the JNI entrypoints for
**	nativeExecuteUtility, createDatabase, destroyDatabase, startIngres and
**	stopIngres.  Bumped ApiReleaseString to 0.4.7 to match toolsapi.impl.
**    29-Jun-2011 (gordy)
**	Replaced getColumns with getRows which supports multi-row fetches.
**    01-Jul-2011 (whiro01)
**	Fix to PLATFORM test for 64-bit Windows.
**    17-Aug-2011 (whiro01)
**	Remove UTcreateDatabase and UTdestroyDatabase since we're not using
**	them anymore.
**    18-Aug-2011 (whiro01)
**	Return C-level API version string and eliminate our separate one
**	(for "getAPIVersionInfo" and "getAPIVersionString").
**    19-Aug-2011 (dansa01)
**	Removed obsolete executeUtility, startIngres, stopIngres.  Renamed
**	unloadInstallation to nativeUnloadInstallation because we now have some
**	Java work to do (clearing left-over UtilityProcessHandles) before the
**	JNI/C-API part.
**    26-Sep-2011 (whiro01)
**	Change the header file include paths for compilation in Ingres codelines.
**    29-Sep-2011 (whiro01)
**	Removed redundant #include of stdlib.h (already in IIapi_jni_log.h).
**    10-Oct-2011 (dansa01)
**	Added a "_this" parameter to all the Native charset functions (and
**	createVersionInfo) so that they can call into the IIapi instance and
**	take advantage of the java.nio encoder/decoder we cache there.  This
**	ensures exceptions are thrown for malformed or unmappable characters.
**    17-Oct-2011 (whiro01)
**	Don't use PMhost for SysInfo -- the host name will have been altered
**	for use in PM keys, use GChostname instead; add this function to API.
**    20-Oct-2011 (whiro01)
**	Fix bogus declaration of EntryExitLogger in "getSysInfo".
**    21-Oct-2011 (whiro01)
**	Adding new LGgetLogPaths, isTransactionLogConfigured and CRsetPMval methods to the API;
**	also renamed some methods that didn't conform to Java naming conventions.
**
** TODO:
**	Look for "TODO:" and "HOT:" for stuff that is not done or
**	needs help to get right still.
*/
// The interface to the local Tools API library
#include "toolsapi.h"
// The generated interface to IIapi.java
#include "IIapi_jni.h"
// Local utility functions
#include "IIapi_jni_log.h"


static const int NO_TIMEOUT = -1;
static const int CANCEL_TIMEOUT = 10;	/* 10 seconds to wait for statement to cancel */

const char *API = "IIapi_jni";
Logger *jniLogger = NULL;

//
// Required features of OpenAPI that we need:
//	IIAPI_VERSION_5 means we can only run with Ingres versions 9.x and above
//	(supports up to ANSI date/time)
// Note: we probably only need up to IIAPI_VERSION_4 (i8 support) -- not sure what
// Ingres version that corresponds to.
// OpenROAD 2006 uses Ingres 2.6 which only supports up to IIAPI_VERSION_3 (UCS-2 support)
//
#define REQUIRED_VERSION	IIAPI_VERSION_5

static TimerHandle timer = 0;

static JavaVM *g_jvm = NULL;

static void _term_(void);


/*
** Name: JNI_OnLoad - Standard method called when this JNI helper library is loaded.
**
** Inputs:
**	jvm		The Java Virtual Machine pointer
**	reserved	Not used
**
** Outputs:
**	None
**	Global class, field and method IDs are setup and other
**	static initialization is performed.  OpenAPI init is done
**	and the environment handle is saved.
**
** Returns:
**	Required version of JVM that we require in order to run
**	or JNI_ERR if something went wrong
**
** Side Effects:
**	PMinit() and PMload() are called for the default context
**	and PMsetDefault for the 0 and 1 parameters are called with
**	the standard Ingres values.
**
** History:
**    March 2008 (whiro01)
**	Part of original coding.
**    20-Jul-2010 (whiro01)
**	Add NO_DEBUG_BREAK check before compiling in the "int 3".
**    14-Dec-2010 (dansa01)
**	Handle (unlikely) exception from C++ new.
**    19-Jan-2011 (dansa01)
**	Added more error checks and tracing.
**    13-Jun-2011 (whiro01)
**	Added more specific error reporting from "GetEnv"
**	and return whatever version the JVM reports.
*/
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *jvm, void *reserved)
{
	JNIEnv *jenv;

	UNREFERENCED(reserved);

#if defined(DEBUG) && !defined(NO_DEBUG_BREAK)
	__asm { int 3 }
#endif

	// Save JavaVM pointer if ever needed by anyone else
	g_jvm = jvm;

	// Register our exit handler that will undo all this initialization
	atexit(_term_);

	try
	{
	    jniLogger = new Logger();
	}
	catch (...)
	{
	}
	if (!jniLogger)
	    return JNI_ERR;

	jniLogger->setPrefix(API);

	EntryExitLogger eel(jniLogger, "JNI_OnLoad"); // s/b __func__

	jint required_version = JNI_VERSION_1_6;
	jint ret = jvm->GetEnv((void **)&jenv, required_version);
	switch (ret)
	{
	    case JNI_OK:
		break;
	    case JNI_EDETACHED:
		jniLogger->critical("Thread not attached to JVM!");
		return JNI_ERR;
	    case JNI_EVERSION:
		jniLogger->critical("JVM does not support required (%08x) version", required_version);
		return JNI_ERR;
	    default:
		jniLogger->critical("Unknown error from GetEnv: %08x", ret);
		return JNI_ERR;
	}

	timer = InitTimer();

	/* Also initialize our utility library */
	JNU_Initialize(jenv);
	if (jenv->ExceptionCheck())
	{
	    jniLogger->critical("JNU_Initialize failed!");
	    return JNI_ERR;
	}
	// Returning whatever version we are given
	jint version = jenv->GetVersion();
	jniLogger->info("JNI version: %08x", version);
	return version;
}


/*
** Name: JNI_OnUnload - Standard method called when this JNI helper library is unloaded.
**	Note: this will probably never be called since it is only called from the
**	finalize method of the class loader which loaded us.  Most likely this will
**	be the system class loader which is never deleted and so the finalize method
**	never is called.
**
** Inputs:
**	jvm		The Java Virtual Machine pointer
**	reserved	Not used
**
** Outputs:
**	Global VM pointer is cleared.
**
** Returns:
**	None
**
** Side Effects:
**	None
**
** History:
**    March 2008 (whiro01)
**	Part of original coding.
**    07-Jan-2010 (whiro01)
**	Gutted and everything moved to _term_() instead.
**    13-Jan-2010 (whiro01)
**	Moved the JNI-related termination back to here.  The problem
**	is that by 'atexit' time the JVM may also have been unloaded
**	and the JNIEnv pointer we get back has a NULL function interface
**	so any calls would crash.
*/
JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *jvm, void *reserved)
{
	JNIEnv *jenv;
	jint ret;

	UNREFERENCED(reserved);

	jniLogger->debug("JNI_OnUnload called");

	// Quit right away if OnLoad was never called or
	// if OnUnload has already been called
	if (g_jvm == NULL)
		return;
	g_jvm = NULL;

	if ((ret = jvm->GetEnv((void **)&jenv, JNI_VERSION_1_6)) == JNI_EDETACHED) {
		// Attached current thread if not already done
		JavaVMAttachArgs args;
		args.version = JNI_VERSION_1_6;
		args.name = NULL;
		args.group = NULL;
		if (jvm->AttachCurrentThread((void **)&jenv, &args) != 0) {
			return;
		}
	}
	else if (ret == JNI_OK)
		;
	else
		return;

	JNU_Terminate(jenv);

	jniLogger->debug("Deleting logger object.");
	if (jniLogger != NULL)
		delete jniLogger;
	jniLogger = NULL;
}


/*
** Name: _term_ - 'atexit' termination handler called when the application exits
**	(but only if the OnLoad handler was already called putting us into the
**	'atexit' list.  This ensures that we are only called if necessary.
**	Note: the only things that should be in here are non-JNI-related stuff
**	(notably Ingres termination) because we can't be sure that the JVM
**	is still active by this time.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
** Returns:
**	None
**
** Side Effects:
**	All the initialization done in JNI_OnLoad is undone here.
**
** History:
**    12-Jan-2010 (whiro01)
**	Moved from JNI_OnUnload and made part of the 'atexit' processing
**	done by the C runtime.
**    13-Jan-2010 (whiro01)
**	The JNI stuff is moved to OnUnload while the Ingres stuff is still
**	in here.  The problem is that by 'atexit' time the JVM is/may be
**	unloaded already, so the JNIEnv pointer we get back has a NULL
**	interface pointer.
**    20-Jul-2010 (whiro01)
**	Add NO_DEBUG_BREAK check before compiling in the "int 3".
**    27-Jul-2010 (whiro01)
**	Cleanup jniLogger also.
**    03-Mar-2011 (whiro01)
**	Moved ToolsAPITerm() into here from JNI_OnUnload since that is
**	(usually) never called.
*/
void
_term_(void)
{
#if defined(DEBUG) && !defined(NO_DEBUG_BREAK)
	__asm { int 3 }
#endif

	jniLogger->debug("_term_ called");

	ToolsAPITerm();

	TermTimer(timer);
	timer = 0;

	jniLogger->debug("Deleting logger object.");
	if (jniLogger != NULL)
		delete jniLogger;
	jniLogger = NULL;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    InitAPI
 * Signature: ()V
 */
void JNICALL
Java_com_ingres_LocalIIapi_InitAPI(JNIEnv *jenv, jclass _class)
{
	UNREFERENCED(_class);

	i4 status = ToolsAPIInit();

	if (status != 0)
	{
	    throwStartupError(jenv, "ToolsAPIInit", status);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getToolsAPIInstance
 * Signature: ()J
 */
jlong JNICALL
Java_com_ingres_LocalIIapi_getToolsAPIInstance(JNIEnv *jenv, jclass _class)
{
	UNREFERENCED(jenv);
	UNREFERENCED(_class);

	jniLogger->trace("Trying to get Toolsapi instance");
	TOOLSAPI *inst = NULL;
	i4 status = GetToolsAPIInstance(TOOLS_API_VERSION, NULL, &inst);

	if (status != II_ST_SUCCESS)
	{
	    jniLogger->critical("GetToolsAPIInstance failed!  Status = %d", status);
	}

	return (jlong)inst;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    bootstrapCharsetName
 * Signature: ()Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_bootstrapCharsetName(JNIEnv *jenv, jobject _this)
{
	//
	// Since we are bootstrapping the Ingres charset conversion support
	// here, we must not call anything that would use Ingres charset
	// conversions.
	//
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	char name[25];
	i4 status = inst->CMgetCharsetName(name, sizeof(name));
	if (status != II_ST_SUCCESS)
	{
	    jniLogger->critical("CMgetCharsetName failed!  Status = %d", status);
	    return NULL;
	}
	else
	{
	    return getStringFromNativeChars(jenv, _this, name);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    nativeUnloadInstallation
 * Signature: ()V
 */
void JNICALL
Java_com_ingres_LocalIIapi_nativeUnloadInstallation(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	jniLogger->trace("Calling instance termination...");
	FreeToolsAPIInstance(inst);
	jniLogger->trace("Back from instance termination.");

	jenv->SetLongField(_this, FIELDID(LocalIIapi, Instance), 0);
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    nativeAbort
 * Signature: (Lcom/ingres/ConnectionHandle;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_nativeAbort(JNIEnv *jenv, jobject _this, jobject connHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiConnHandle = getHandleValue(jenv, connHandle);

	i4 status = inst->IIabort(iiConnHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIabort", status, inst);
	}
	else
	{
	    setHandleValue(jenv, connHandle, NULL);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    autoCommitOn
 * Signature: (Lcom/ingres/ConnectionHandle;)Lcom/ingres/TransactionHandle;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_autoCommitOn(JNIEnv *jenv, jobject _this, jobject connHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	StartTimer(timer);

	II_HANDLE iiConnHandle = getHandleValue(jenv, connHandle);
	II_HANDLE iiTranHandle = NULL;
	jobject jTranHandle = NULL;

	i4 status = inst->IIautoCommitOn(iiConnHandle, &iiTranHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIautoCommitOn", status, inst);
	}
	else
	{
	    jTranHandle = newHandleObj(jenv, CLASSREF(TransactionHandle), METHODID(TransactionHandle, init), iiTranHandle);
	    if (!jTranHandle)
	    {
		inst->IIautoCommitOff(iiTranHandle);
	    }
	}

	ReportSplit(timer, "autoCommitOn took {$t} seconds.");

	return jTranHandle;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    autoCommitOff
 * Signature: (Lcom/ingres/TransactionHandle;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_autoCommitOff(JNIEnv *jenv, jobject _this, jobject tranHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	StartTimer(timer);

	II_HANDLE iiTranHandle = getHandleValue(jenv, tranHandle);

	i4 status = inst->IIautoCommitOff(iiTranHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIautoCommitOff", status, inst);
	}
	else
	{
	    setHandleValue(jenv, tranHandle, NULL);
	}

	ReportSplit(timer, "autoCommitOff took {$t} seconds.");
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    commit
 * Signature: (Lcom/ingres/TransactionHandle;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_commit(JNIEnv *jenv, jobject _this, jobject tranHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiTranHandle = getHandleValue(jenv, tranHandle);

	i4 status = inst->IIcommit(iiTranHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIcommit", status, inst);
	}
	else
	{
	    setHandleValue(jenv, tranHandle, NULL);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    rollback
 * Signature: (Lcom/ingres/TransactionHandle;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_rollback(JNIEnv *jenv, jobject _this, jobject tranHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiTranHandle = getHandleValue(jenv, tranHandle);

	i4 status = inst->IIrollback(iiTranHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIrollback", status, inst);
	}
	else
	{
	    setHandleValue(jenv, tranHandle, NULL);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    connect
 * Signature: (ZLjava/lang/String;Ljava/lang/String;Ljava/lang/String;ILcom/ingres/ConnectOptions;)Lcom/ingres/ConnectionHandle;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_connect(JNIEnv *jenv, jobject _this, jboolean nameServer, jstring sTarget, jstring sUser, jstring sPassword, jint iTimeout, jobject jOptions)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	IngresChars szTarget(jenv, _this, sTarget);
	if (jenv->ExceptionCheck())
	    return NULL;

	IngresChars szUser(jenv, _this, sUser);
	if (jenv->ExceptionCheck())
	    return NULL;

	IngresChars szPassword(jenv, _this, sPassword);
	if (jenv->ExceptionCheck())
	    return NULL;

	jstring sEffectiveUser = NULL;
	jstring sGroup = NULL;
	jstring sRole = NULL;
	jstring sDbmsPassword = NULL;

	if (jOptions)
	{
	    sEffectiveUser = (jstring)jenv->GetObjectField(jOptions, FIELDID(ConnectOptions, effectiveUser));
	    sGroup         = (jstring)jenv->GetObjectField(jOptions, FIELDID(ConnectOptions, group));
	    sRole          = (jstring)jenv->GetObjectField(jOptions, FIELDID(ConnectOptions, role));
	    sDbmsPassword  = (jstring)jenv->GetObjectField(jOptions, FIELDID(ConnectOptions, dbmsPassword));
	}

	IngresChars szEffectiveUser(jenv, _this, sEffectiveUser);
	if (jenv->ExceptionCheck())
	    return NULL;

	IngresChars szGroup(jenv, _this, sGroup);
	if (jenv->ExceptionCheck())
	    return NULL;

	IngresChars szRole(jenv, _this, sRole);
	if (jenv->ExceptionCheck())
	    return NULL;

	IngresChars szDbmsPassword(jenv, _this, sDbmsPassword);
	if (jenv->ExceptionCheck())
	    return NULL;

	II_CONNECT_OPTIONS options;
	options.effectiveUser = szEffectiveUser;
	options.group = szGroup;
	options.role = szRole;
	options.dbmsPassword = szDbmsPassword;

	if (sEffectiveUser != NULL)
	    jenv->DeleteLocalRef(sEffectiveUser);
	if (sGroup != NULL)
	    jenv->DeleteLocalRef(sGroup);
	if (sRole != NULL)
	    jenv->DeleteLocalRef(sRole);
	if (sDbmsPassword != NULL)
	    jenv->DeleteLocalRef(sDbmsPassword);

	StartTimer(timer);

	II_HANDLE iiConnHandle = NULL;
	jobject jConnHandle = NULL;
	i4 status;

	if (nameServer)
	{
	    status = inst->IIconnectNameServer(szTarget, szUser, szPassword, (i4)iTimeout, &iiConnHandle);
	}
	else
	{
	    status = inst->IIconnectDatabase(szTarget, szUser, szPassword, (i4)iTimeout, &options, &iiConnHandle);
	}

	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIconnect", status, inst);
	}
	else
	{
	    jConnHandle = newHandleObj(jenv, CLASSREF(ConnectionHandle), METHODID(ConnectionHandle, init), iiConnHandle);
	    if (!jConnHandle)
	    {
		inst->IIdisconnect(iiConnHandle);
	    }
	}

	ReportSplit(timer, "Connect took {$t} seconds.");

	return jConnHandle;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    nativeDisconnect
 * Signature: (Lcom/ingres/ConnectionHandle;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_nativeDisconnect(JNIEnv *jenv, jobject _this, jobject connHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiConnHandle = getHandleValue(jenv, connHandle);

	i4 status = inst->IIdisconnect(iiConnHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIdisconnect", status, inst);
	}
	else
	{
	    setHandleValue(jenv, connHandle, NULL);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    nativeQuery
 * Signature: (Lcom/ingres/ConnectionHandle;Lcom/ingres/TransactionHandle;Ljava/lang/String;Z)Lcom/ingres/StatementHandle;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_nativeQuery(JNIEnv *jenv, jobject _this, jobject connHandle, jobject tranHandle, jstring sQuery, jboolean bNeedParms)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	if (!tranHandle)
	{
	    JNU_ThrowByName(jenv, NullPointerException, "Null TransactionHandle reference.");
	    return NULL;
	}

	StartTimer(timer);

	IngresChars szQuery(jenv, _this, sQuery);
	if (jenv->ExceptionCheck())
	    return NULL;

	II_HANDLE iiConnHandle = getHandleValue(jenv, connHandle);
	II_HANDLE iiTranHandle = getHandleValue(jenv, tranHandle);

	II_HANDLE iiTranHandleInitial = iiTranHandle;

	II_HANDLE iiStmtHandle = NULL;
	jobject jStmtHandle = NULL;

	i4 status = inst->IIquery(iiConnHandle, &iiTranHandle, szQuery, bNeedParms, &iiStmtHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIquery", status, inst);
	}
	else
	{
	    jStmtHandle = newHandleObj(jenv, CLASSREF(StatementHandle), METHODID(StatementHandle, init), iiStmtHandle);
	    if (!jStmtHandle)
	    {
		inst->IIcancel(iiStmtHandle);
		inst->IIclose(iiStmtHandle);
		if (iiTranHandle != iiTranHandleInitial)
		{
		    inst->IIrollback(iiTranHandle);
		}
	    }
	    else
	    {
		if (iiTranHandle != iiTranHandleInitial)
		{
		    //
		    // A new MST was started (with a new transaction handle).
		    //
		    setHandleValue(jenv, tranHandle, iiTranHandle);
		}
	    }
	}

	ReportSplit(timer, "Query took {$t} seconds.");

	return jStmtHandle;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    executeProcedure
 * Signature: (Lcom/ingres/ConnectionHandle;Lcom/ingres/TransactionHandle;[Lcom/ingres/IIapi$Descriptor;[Lcom/ingres/IIapi$DataValue;)Lcom/ingres/StatementHandle;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_executeProcedure(JNIEnv *jenv, jobject _this, jobject connHandle, jobject tranHandle, jobjectArray descriptors, jobjectArray datavalues)
{
	EntryExitLogger eel(jniLogger, "IIapi_executeProcedure");

	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	if (!tranHandle)
	{
	    JNU_ThrowByName(jenv, NullPointerException, "Null TransactionHandle reference.");
	    return NULL;
	}

	StartTimer(timer);

	II_HANDLE iiConnHandle = getHandleValue(jenv, connHandle);
	II_HANDLE iiTranHandle = getHandleValue(jenv, tranHandle);

	II_HANDLE iiTranHandleInitial = iiTranHandle;

	II_HANDLE iiStmtHandle = NULL;
	jobject jStmtHandle = NULL;

	//
	// This BufferList will track all the allocations made by makeCAPIdescVdataV
	// and free them when we exit this scope.
	//
	BufferList buffers;

	II_DESCRIPTORS *pDescV = NULL;
	II_DATAVALUES  *pDataV = NULL;

	makeCAPIdescVdataV(jenv, _this, inst, buffers, descriptors, datavalues, &pDescV, &pDataV);
	if (jenv->ExceptionCheck())
	    return NULL;

	i4 status = inst->IIexecuteProcedure(iiConnHandle, &iiTranHandle, pDescV, pDataV, &iiStmtHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIexecuteProcedure", status, inst);
	}
	else
	{
	    jStmtHandle = newHandleObj(jenv, CLASSREF(StatementHandle), METHODID(StatementHandle, init), iiStmtHandle);
	    if (!jStmtHandle)
	    {
		inst->IIcancel(iiStmtHandle);
		inst->IIclose(iiStmtHandle);
		if (iiTranHandle != iiTranHandleInitial)
		{
		    inst->IIrollback(iiTranHandle);
		}
	    }
	    else
	    {
		if (iiTranHandle != iiTranHandleInitial)
		{
		    //
		    // A new MST was started (with a new transaction handle).
		    //
		    setHandleValue(jenv, tranHandle, iiTranHandle);
		}
	    }
	}

	ReportSplit(timer, "executeProcedure took {$t} seconds.");

	return jStmtHandle;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    putParameters
 * Signature: (Lcom/ingres/StatementHandle;[Lcom/ingres/IIapi$Descriptor;[Lcom/ingres/IIapi$DataValue;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_putParameters(JNIEnv *jenv, jobject _this, jobject stmtHandle, jobjectArray descriptors, jobjectArray datavalues)
{
	EntryExitLogger eel(jniLogger, "IIapi_putParameters");

	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiStmtHandle = getHandleValue(jenv, stmtHandle);

	//
	// This BufferList will track all the allocations made by makeCAPIdescVdataV
	// and free them when we exit this scope.
	//
	BufferList buffers;

	II_DESCRIPTORS *pDescV = NULL;
	II_DATAVALUES  *pDataV = NULL;

	makeCAPIdescVdataV(jenv, _this, inst, buffers, descriptors, datavalues, &pDescV, &pDataV);
	if (jenv->ExceptionCheck())
	    return;

	i4 status = inst->IIputParameters(iiStmtHandle, pDescV, pDataV);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIputParameters", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    cancel
 * Signature: (Lcom/ingres/StatementHandle;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_cancel(JNIEnv *jenv, jobject _this, jobject stmtHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiStmtHandle = getHandleValue(jenv, stmtHandle);

	i4 status = inst->IIcancel(iiStmtHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIcancel", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    close
 * Signature: (Lcom/ingres/StatementHandle;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_close(JNIEnv *jenv, jobject _this, jobject stmtHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	StartTimer(timer);

	II_HANDLE iiStmtHandle = getHandleValue(jenv, stmtHandle);

	i4 status = inst->IIclose(iiStmtHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIclose", status, inst);
	}
	else
	{
	    setHandleValue(jenv, stmtHandle, NULL);
	}

	ReportSplit(timer, "Close took {$t} seconds.");
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getDescriptors
 * Signature: (Lcom/ingres/StatementHandle;)[Lcom/ingres/IIapi$Descriptor;
 */
jobjectArray JNICALL
Java_com_ingres_LocalIIapi_getDescriptors(JNIEnv *jenv, jobject _this, jobject stmtHandle)
{
	EntryExitLogger eel(jniLogger, "IIapi_getDescriptors");

	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	StartTimer(timer);

	II_HANDLE iiStmtHandle = getHandleValue(jenv, stmtHandle);

	MEfreePtr<II_DESCRIPTORS> pDescV(inst);

	i4 status = inst->IIgetDescriptors(iiStmtHandle, &pDescV);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIgetDescriptors", status, inst);
	    return NULL;
	}

	ReportSplit(timer, "IIgetDescriptors took {$t} seconds.");

	StartTimer(timer);

	jobjectArray descArray = NULL;
	if (status == II_ST_SUCCESS)
	{
	    i4 descCount = pDescV->count;

	    descArray = jenv->NewObjectArray(descCount, CLASSREF(Descriptor), NULL);
	    jniLogger->trace("Got descriptor array at %p", descArray);
	    if (descArray)
	    {
		for (int i = 0; i < descCount; i++)
		{
		    /* Allocate an initialized object using the default constructor */
		    jobject desc = createObject(jenv, CLASSREF(Descriptor), METHODID(Descriptor, init));
		    if (!desc)
		    {
			return NULL;
		    }
		    else
		    {
			/* Set each of the object fields in turn */
			jenv->SetShortField(desc, FIELDID(Descriptor, dataType),    pDescV->descriptors[i].dataType);
			jenv->SetBooleanField(desc, FIELDID(Descriptor, nullable),  pDescV->descriptors[i].isNullable ? JNI_TRUE : JNI_FALSE);
			jenv->SetShortField(desc, FIELDID(Descriptor, length),      pDescV->descriptors[i].length);
			jenv->SetShortField(desc, FIELDID(Descriptor, precision),   pDescV->descriptors[i].precision);
			jenv->SetShortField(desc, FIELDID(Descriptor, scale),       pDescV->descriptors[i].scale);
			jenv->SetShortField(desc, FIELDID(Descriptor, columnType),  pDescV->descriptors[i].columnType);

			setIngresStringField(jenv, _this, desc, FIELDID(Descriptor, columnName), pDescV->descriptors[i].columnName);
			if (jenv->ExceptionCheck())
			    return NULL;
		    }
		    jenv->SetObjectArrayElement(descArray, i, desc);
		    if (jenv->ExceptionCheck())
			return NULL;

		    jenv->DeleteLocalRef(desc);
		}
		ReportSplit(timer, "allocate java objects done in {$t} seconds.");
	    }
	}

	return descArray;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getRows
 * Signature: (Lcom/ingres/StatementHandle;[Lcom/ingres/IIapi$Descriptor;[[Lcom/ingres/IIapi$DataValue;)I
 */
jint JNICALL
Java_com_ingres_LocalIIapi_getRows(JNIEnv *jenv, jobject _this, jobject stmtHandle, jobjectArray descriptors, jobjectArray rows)
{
	EntryExitLogger eel(jniLogger, "IIapi_getRows");

	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return 0;

	II_HANDLE iiStmtHandle = getHandleValue(jenv, stmtHandle);

	if (!descriptors || !rows)
	{
	    JNU_ThrowByName(jenv, NullPointerException, "Null Descriptors or DataValues array reference.");
	    return 0;
	}

	int ncols = jenv->GetArrayLength(descriptors);
	int nrows = jenv->GetArrayLength( rows );

	if (ncols == 0)
	{
	    JNU_ThrowByName(jenv, IllegalArgumentException, "Zero descriptors for getRows.");
	    return 0;
	}

	if ( nrows == 0 )
	{
	    JNU_ThrowByName(jenv, IllegalArgumentException, "Zero rows for getRows.");
	    return 0;
	}

	for( int row = 0; row < nrows; row++ )
	{
	    /*
	    ** The data row arrays are optional and will be allocated below if not provided.
	    */
	    jobjectArray datavalues = static_cast<jobjectArray>(jenv->GetObjectArrayElement( rows, row ));
	    if ( ! datavalues )  continue;

	    /*
	    ** The provided data row arrays must be the correct length.
	    */
	    int ndatacols = jenv->GetArrayLength(datavalues);
	    if (ncols != ndatacols)
	    {
		const char *fmt = "IIapi_getRows: descriptor array length (%d) is not the same as data array length (%d)!";

		MallocFreePtr<char> msg(JNU_sprintf(jenv, fmt, ncols, ndatacols));
		if (msg)
		{
		    jniLogger->error("%s", (char *)msg);
		    JNU_ThrowByName(jenv, IllegalArgumentException, msg);
		}
		// if msg failed to allocate, JNU_sprintf has already thrown.  Don't double-throw.
		return 0;
	    }
	}

	StartTimer(timer);

	size_t bytesNeeded = sizeof(II_DESCRIPTORS) + sizeof(II_DESCRIPTOR) * (ncols > 0 ? ncols - 1 : 0);

	MallocFreePtr<II_DESCRIPTORS> pDescV(bytesNeeded);
	if (!pDescV)
	{
	    JNU_ThrowByName(jenv, OutOfMemoryError, "Failed to allocate IIapi_getRows descriptor block");
	    return 0;
	}

	pDescV->count = ncols;
	for (int i = 0; i < ncols; i++)
	{
	    jobject desc = jenv->GetObjectArrayElement(descriptors, i);
	    if (jenv->ExceptionCheck())
	    {
		return 0;
	    }

	    if (!desc)
	    {
		JNU_ThrowByName(jenv, NullPointerException, "Null descriptor array element.");
		return 0;
	    }

	    pDescV->descriptors[i].dataType   = jenv->GetShortField(desc, FIELDID(Descriptor, dataType));
	    pDescV->descriptors[i].isNullable = jenv->GetBooleanField(desc, FIELDID(Descriptor, nullable));
	    pDescV->descriptors[i].length     = jenv->GetShortField(desc, FIELDID(Descriptor, length));
	    pDescV->descriptors[i].precision  = jenv->GetShortField(desc, FIELDID(Descriptor, precision));
	    pDescV->descriptors[i].scale      = jenv->GetShortField(desc, FIELDID(Descriptor, scale));
	    pDescV->descriptors[i].columnType = jenv->GetShortField(desc, FIELDID(Descriptor, columnType));
	    pDescV->descriptors[i].columnName = NULL; // Not needed in this context -- simplifies management in here also

	    /* Done with with desc */
	    jenv->DeleteLocalRef(desc);
	}

	ReportSplit(timer, "Allocating memory took {$t} seconds.");

	MEfreePtr<II_DATAVALUES> pDataV(inst);
	i2 rowCount;
	i4 status = inst->IIgetColumns(iiStmtHandle, pDescV, (i2)nrows, &pDataV, &rowCount);
	ReportSplit(timer, "IIgetColumns took {$t} seconds.");

	if (status == II_ST_NO_DATA)
	{
	    return 0;
	}
	else if (status == II_ST_SUCCESS && rowCount == 0)
	{
	    return 0;
	}
	else if (status == II_ST_SUCCESS && pDataV->count == 0)
	{
	    //
	    // (I doubt that this case is actually possible, but the existing code
	    // made an effort to handle it, so just to be safe, I will too.)
	    //
	    return 0;
	}
	else if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIgetColumns", status, inst);
	    return 0;
	}

	int dv = 0;
	StartTimer(timer);
	/* Convert the native data value descriptor to a Java structure and value object */
	for( int row = 0; row < rowCount; row++ )
	{
	    jobjectArray datavalues = static_cast<jobjectArray>(jenv->GetObjectArrayElement( rows, row ));

	    /*
	    ** The data row arrays are optional, allocate if not provided.
	    */
	    if ( ! datavalues )
	    {
		datavalues = jenv->NewObjectArray( ncols, CLASSREF(DataValue), NULL );
		if ( ! datavalues )
		    return 0;

		jenv->SetObjectArrayElement( rows, row, datavalues );

		if (jenv->ExceptionCheck())
		{
		    jniLogger->error("Exception from setting row array element %d", row);
		    return 0;
		}
	    }

	    for (int column = 0; column < ncols; column++, dv++ )
	    {
		/*
		** Use any existing column DataValue, allocate if needed.
		*/
		jobject jData = jenv->GetObjectArrayElement( datavalues, column );
		jobject newData = NULL;

		if ( ! jData )
		{
		    newData = createObject(jenv, CLASSREF(DataValue), METHODID(DataValue, init));
		    if ( ! newData )
			return 0;

		    jenv->SetObjectArrayElement(datavalues, column, newData);
		    if (jenv->ExceptionCheck())
		    {
			jniLogger->error("Exception from setting II_DATAVALUE array element %d", column);
			return 0;
		    }

		    jData = newData;
		}

		jniLogger->trace("row %d col %d: length = %d, null = %d", row, column, pDataV->values[dv].length, pDataV->values[dv].isNull);

		jenv->SetBooleanField(jData, FIELDID(DataValue, isnull), pDataV->values[dv].isNull ? JNI_TRUE : JNI_FALSE);
		jenv->SetIntField(jData, FIELDID(DataValue, length), pDataV->values[dv].length);

		jobject jObj = getJavaDataObject(jenv, _this, inst, &pDescV->descriptors[column], &pDataV->values[dv]);
		if (jenv->ExceptionCheck())
		{
		    return 0;
		}

		jenv->SetObjectField(jData, FIELDID(DataValue, value), jObj);
		jenv->DeleteLocalRef(jObj);
		if ( newData )  jenv->DeleteLocalRef( newData );
	    }
	}
	ReportSplit(timer, "alloc java column objects done in {$t} seconds.");

	return (int)rowCount;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getQueryInfo
 * Signature: (Lcom/ingres/StatementHandle;)Lcom/ingres/IIapi$QueryInfo;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_getQueryInfo(JNIEnv *jenv, jobject _this, jobject stmtHandle)
{
	II_QUERY_INFO iiqInfo;

	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	II_HANDLE iiStmtHandle = getHandleValue(jenv, stmtHandle);

	jobject qInfo = NULL;

	StartTimer(timer);

	i4 status = inst->IIgetQueryInfo(iiStmtHandle, &iiqInfo);

	ReportSplit(timer, "IIgetQueryInfo took {$t} seconds.");
	StartTimer(timer);

	if (status == II_ST_SUCCESS)
	{
	    /* Allocate an initialized object using the default constructor */
	    qInfo = createObject(jenv, CLASSREF(QueryInfo), METHODID(QueryInfo, init));
	    if (qInfo)
	    {
		/* Transfer the fields one by one, depending on their availability */
		jenv->SetLongField(qInfo, FIELDID(QueryInfo, flags), iiqInfo.flags);
		jenv->SetLongField(qInfo, FIELDID(QueryInfo, mask), iiqInfo.mask);
		jenv->SetLongField(qInfo, FIELDID(QueryInfo, rowCount),
			(iiqInfo.mask & II_QINFO_MASK_ROW_COUNT) ? iiqInfo.rowCount : 0);
		jenv->SetBooleanField(qInfo, FIELDID(QueryInfo, readOnly),
			(iiqInfo.mask & II_QINFO_MASK_CURSOR) ? (iiqInfo.readOnly ? JNI_TRUE : JNI_FALSE) : 0);
		jenv->SetLongField(qInfo, FIELDID(QueryInfo, procReturn),
			(iiqInfo.mask & II_QINFO_MASK_PROCEDURE_RET) ? iiqInfo.procReturn : 0);
		jenv->SetLongField(qInfo, FIELDID(QueryInfo, procHandle),
			(jlong)((iiqInfo.mask & II_QINFO_MASK_PROCEDURE_ID) ? iiqInfo.procHandle : 0));
		jenv->SetLongField(qInfo, FIELDID(QueryInfo, repeatQueryHandle),
			(jlong)((iiqInfo.mask & II_QINFO_MASK_REPEAT_QUERY_ID) ? iiqInfo.repeatQueryHandle : 0));

		setIngresStringField(jenv, _this, qInfo, FIELDID(QueryInfo, tableKey),
			(iiqInfo.mask & II_QINFO_MASK_TABLE_KEY) ? iiqInfo.tableKey : NULL);
		if (jenv->ExceptionCheck())
		    return NULL;

		setIngresStringField(jenv, _this, qInfo, FIELDID(QueryInfo, objectKey),
			(iiqInfo.mask & II_QINFO_MASK_OBJECT_KEY) ? iiqInfo.objectKey : NULL);
		if (jenv->ExceptionCheck())
		    return NULL;

		jenv->SetLongField(qInfo, FIELDID(QueryInfo, cursorType),
			(iiqInfo.mask & II_QINFO_MASK_CURSOR) ? iiqInfo.cursorType : 0);
		jenv->SetLongField(qInfo, FIELDID(QueryInfo, rowStatus),
			(iiqInfo.mask & II_QINFO_MASK_ROW_STATUS) ? iiqInfo.rowStatus : 0);
		jenv->SetLongField(qInfo, FIELDID(QueryInfo, rowPosition),
			(iiqInfo.mask & II_QINFO_MASK_ROW_STATUS) ? iiqInfo.rowPosition : 0);
	    }
	}
	else
	{
	    throwLastAPIErrors(jenv, _this, "IIgetQueryInfo", status, inst);
	}

	ReportSplit(timer, "Fill in Java query info took {$t} seconds.");
	jniLogger->trace("Returning %x pointer for qInfo", qInfo);

	return qInfo;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getEnv
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_getEnv(JNIEnv *jenv, jobject _this, jstring symbol)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	NativeChars szSymbol(jenv, _this, symbol);
	if (jenv->ExceptionCheck())
	    return NULL;

	MEfreePtr<char> pValue(inst);
	i4 status = inst->NMgtAt(szSymbol, &pValue);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "NMgtAt", status, inst);
	    return NULL;
	}

	return getStringFromNativeChars(jenv, _this, pValue);
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    NMsymbols
 * Signature: ()Ljava/util/Map;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_NMsymbols(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	jobject jMap = NULL;

	MEfreePtr<II_NAME_VALUE_PAIRS> pNV(inst);

	i4 status = inst->NMsymbols(&pNV);
	if (status == II_ST_SUCCESS)
	{
	    // Create TreeMap to pass back
	    jMap = createObject(jenv, CLASSREF(TreeMap), METHODID(TreeMap, init));
	    if (!jMap)
		return NULL;

	    for (int i = 0; i < pNV->count; i++)
	    {
		jstring jKey = getStringFromNativeChars(jenv, _this, pNV->pairs[i].name);
		if (jenv->ExceptionCheck())
		    return NULL;

		jstring jVal = getStringFromNativeChars(jenv, _this, pNV->pairs[i].value);
		if (jenv->ExceptionCheck())
		    return NULL;

		jenv->CallObjectMethod(jMap, METHODID(TreeMap, put), jKey, jVal);
		if (jenv->ExceptionCheck())
		    return NULL;

		jenv->DeleteLocalRef(jKey);
		jenv->DeleteLocalRef(jVal);
	    }
	}
	else
	{
	    throwLastAPIErrors(jenv, _this, "NMsymbols", status, inst);
	}

	return jMap;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    nativeIDname
 * Signature: ()Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_nativeIDname(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	jstring jstr = NULL;

	MEfreePtr<char> pname(inst);

	i4 status = inst->IDname(&pname);
	if (status == II_ST_SUCCESS)
	{
	    jstr = getStringFromNativeChars(jenv, _this, pname);
	}
	else
	{
	    throwLastAPIErrors(jenv, _this, "IDname", status, inst);
	}

	return jstr;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    IDname_service
 * Signature: ()Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_IDname_1service(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	jstring jstr = NULL;

	MEfreePtr<char> pname(inst);

	i4 status = inst->IDnameService(&pname);
	if (status == II_ST_SUCCESS)
	{
	    jstr = getStringFromNativeChars(jenv, _this, pname);
	}
	else
	{
	    throwLastAPIErrors(jenv, _this, "IDnameService", status, inst);
	}

	return jstr;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getVersionInfo
 * Signature: ()Lcom/ingres/IIapi$VersionInfo;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_getVersionInfo(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	jobject vInfo = NULL;
	char line[200];

	i4 status = inst->UTreadVersionRel(line, sizeof(line));
	if (status == II_ST_SUCCESS)
	{
	    vInfo = createVersionInfo(jenv, _this, line);
	    if (jenv->ExceptionCheck())
		return NULL;
	}
	else
	{
	    throwLastAPIErrors(jenv, _this, "UTreadVersionRel", status, inst);
	}

	return vInfo;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getVersionString
 * Signature: ()Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_getVersionString(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	jstring rel = NULL;
	char line[200];

	i4 status = inst->UTreadVersionRel(line, sizeof(line));
	if (status == II_ST_SUCCESS)
	{
	    rel = getStringFromNativeChars(jenv, _this, line);
	}
	else
	{
	    throwLastAPIErrors(jenv, _this, "UTreadVersionRel", status, inst);
	}

	return rel;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getAPIVersionInfo
 * Signature: ()Lcom/ingres/IIapi$VersionInfo;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_getAPIVersionInfo(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	const char *szVersion = NULL;
	inst->APIgetVersion(&szVersion);

	jobject vInfo = createVersionInfo(jenv, _this, szVersion);
	if (jenv->ExceptionCheck())
	    return NULL;

	return vInfo;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getAPIVersionString
 * Signature: ()Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_getAPIVersionString(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	const char *szVersion = NULL;
	inst->APIgetVersion(&szVersion);

	return getStringFromNativeChars(jenv, _this, szVersion);
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    getSysInfo
 * Signature: ()Lcom/ingres/IIapi$SysInfo;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_getSysInfo(JNIEnv *jenv, jobject _this)
{
	EntryExitLogger eel(jniLogger, "IIapi_getSysInfo");

	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	jobject sysInfo = NULL;
	i4 status;

	sysInfo = createObject(jenv, CLASSREF(SysInfo), METHODID(SysInfo, init));
	if (!sysInfo)
	    return NULL;

	MEfreePtr<char> pIIinst(inst);
	status = inst->NMgtAt("II_INSTALLATION", &pIIinst);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "NMgtAt", status, inst);
	    return NULL;
	}
	setNativeStringField(jenv, _this, sysInfo, FIELDID(SysInfo, IIinstallation), pIIinst);
	if (jenv->ExceptionCheck())
	    return NULL;

	//
	// Get II_SYSTEM value and make drive letter uppercase
	//
	MEfreePtr<char> pstr(inst);
	status = inst->NMgtAt("II_SYSTEM", &pstr);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "NMgtAt", status, inst);
	    return NULL;
	}

	if (pstr != NULL && strlen(pstr) != 0)
	{
	    if (pstr[1] == ':')
	    {
		char ch = pstr[0];
		if ((ch >= 'a' && ch <= 'z'))
		    pstr[0] = ch + ('A' - 'a');
	    }
	    setNativeStringField(jenv, _this, sysInfo, FIELDID(SysInfo, IIsystem), pstr);
	}
	else
	{
	    setNativeStringField(jenv, _this, sysInfo, FIELDID(SysInfo, IIsystem), NULL);
	}
	if (jenv->ExceptionCheck())
	    return NULL;

	char szHost[256+1];
	status = inst->GChostname(szHost, sizeof(szHost), false);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "GChostname", status, inst);
	    return NULL;
	}

	setNativeStringField(jenv, _this, sysInfo, FIELDID(SysInfo, hostName), szHost);
	if (jenv->ExceptionCheck())
	    return NULL;

	status = inst->GChostname(szHost, sizeof(szHost), true);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "GChostname", status, inst);
	    return NULL;
	}

	setNativeStringField(jenv, _this, sysInfo, FIELDID(SysInfo, fullyQualifiedHostName), szHost);
	if (jenv->ExceptionCheck())
	    return NULL;

	char line[200];
	status = inst->UTreadVersionRel(line, sizeof(line));
	if (status == SUCCESS)
	{
	    jobject vInfo = createVersionInfo(jenv, _this, line);
	    if (jenv->ExceptionCheck())
		return NULL;

	    jenv->SetObjectField(sysInfo, FIELDID(SysInfo, version), vInfo);
	    jenv->DeleteLocalRef(vInfo);
	}
	else
	{
	    // Don't throw.  version.rel is optional here.
	    jenv->SetObjectField(sysInfo, FIELDID(SysInfo, version), NULL);
	}

	jniLogger->debug("Returning %p value for sysInfo", sysInfo);
	return sysInfo;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    elevationRequired
 * Signature: ()V
 */
void JNICALL
Java_com_ingres_LocalIIapi_elevationRequired(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	i4 status = inst->ElevationRequired();
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "ElevationRequired", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    elevationRequiredWarning
 * Signature: ()V
 */
void JNICALL
Java_com_ingres_LocalIIapi_elevationRequiredWarning(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	i4 status = inst->ElevationRequiredWarning();
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "ElevationRequiredWarning", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    isElevationRequired
 * Signature: ()Z
 */
jboolean JNICALL
Java_com_ingres_LocalIIapi_isElevationRequired(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return JNI_FALSE;

	i4 isRequired = FALSE;
	i4 status = inst->IsElevationRequired(&isRequired);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IsElevationRequired", status, inst);
	}

	return (isRequired ? JNI_TRUE : JNI_FALSE);
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    checkPrivilege
 * Signature: (Ljava/lang/String;Lcom/ingres/IIapi$Privileges;)Z
 */
jboolean JNICALL
Java_com_ingres_LocalIIapi_checkPrivilege(JNIEnv *jenv, jobject _this, jstring sUser, jobject ePriv)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return JNI_FALSE;

	if (!ePriv)
	{
	    JNU_ThrowByName(jenv, NullPointerException, "Null Privileges object.");
	    return JNI_FALSE;
	}

	IngresChars szUser(jenv, _this, sUser);
	if (jenv->ExceptionCheck())
	    return JNI_FALSE;

	jint jpriv = jenv->CallIntMethod(ePriv, METHODID(Privileges, ordinal));
	if (jenv->ExceptionCheck())
	    return JNI_FALSE;

	// If the enum is constructed correctly, "jpriv" will always be
	// a correct value and won't need to be checked here.
	i4 isAllowed = FALSE;
	i4 status = inst->CheckPrivilege(szUser, (enum Privileges)jpriv, &isAllowed);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "CheckPrivilege", status, inst);
	}

	return (isAllowed ? JNI_TRUE : JNI_FALSE);
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    GCusrpwd
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_GCusrpwd(JNIEnv *jenv, jobject _this, jstring sUser, jstring sPassword)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	NativeChars szUser(jenv, _this, sUser);
	if (jenv->ExceptionCheck())
	    return;

	NativeChars szPassword(jenv, _this, sPassword);
	if (jenv->ExceptionCheck())
	    return;

	if (szUser == NULL || *szUser == '\0')
	    JNU_ThrowByName(jenv, IllegalArgumentException, "Username required");
	else  if (szPassword == NULL || *szPassword == '\0')
	    JNU_ThrowByName(jenv, IllegalArgumentException, "Password required");
	else
	{
	    i4 status = inst->GCusrpwd(szUser, szPassword);
	    if (status != II_ST_SUCCESS)
		throwLastAPIErrors(jenv, _this, "GCusrpwd", status, inst);
	}

	return;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    GChostname
 * Signature: (Z)Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_GChostname(JNIEnv *jenv, jobject _this, jboolean bFullyQualified)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	char szHost[256+1];
	i4 status = inst->GChostname(szHost, sizeof(szHost), bFullyQualified);
	if (status != II_ST_SUCCESS)
	    throwLastAPIErrors(jenv, _this, "GChostname", status, inst);

	return getStringFromNativeChars(jenv, _this, szHost);
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    GCtcpIpPort
 * Signature: (Ljava/lang/String;I)I
 */
jint JNICALL
Java_com_ingres_LocalIIapi_GCtcpIpPort(JNIEnv *jenv, jobject _this, jstring sInput, jint subport)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return 0;

	NativeChars szInput(jenv, _this, sInput);
	if (jenv->ExceptionCheck())
	    return 0;

	i4 port = 0;
	if (szInput != NULL && *szInput != '\0')
	{
	    // These buffers will never be longer than the input (could be same)
	    // So dynamically allocate so we are sure not to overflow on bad input
	    size_t inputlen = strlen(szInput);
	    size_t buflen = (inputlen <= 25 ? 25 : inputlen) + 1;

	    MallocFreePtr<char> output(buflen);
	    MallocFreePtr<char> symbol(buflen);
	    if (!output || !symbol)
	    {
		JNU_ThrowByName(jenv, OutOfMemoryError, NULL);
	    }
	    else
	    {
		i4 status = inst->GCtcpIpPort(szInput, subport, output, symbol);
		if (status == II_ST_SUCCESS)
		{
		    port = atoi(output);
		}
		else
		{
		    throwLastAPIErrors(jenv, _this, "GCtcpIpPort", status, inst);
		}
	    }
	}

	return port;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    CMgetCharsetName
 * Signature: ()Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_CMgetCharsetName(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	char name[25];
	i4 status = inst->CMgetCharsetName(name, sizeof(name));
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "CMgetCharsetName", status, inst);
	    return NULL;
	}
	else
	{
	    return getStringFromNativeChars(jenv, _this, name);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    CMgetStdCharsetName
 * Signature: ()Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_CMgetStdCharsetName(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	char name[25];
	i4 status = inst->CMgetStdCharsetName(name, sizeof(name));
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "CMgetStdCharsetName", status, inst);
	    return NULL;
	}
	else
	{
	    return getStringFromNativeChars(jenv, _this, name);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMinit
 * Signature: ()Lcom/ingres/ContextHandle;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_PMinit(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	II_HANDLE iiCtxHandle = NULL;
	jobject jCtxHandle = NULL;

	i4 status = inst->PMinit(&iiCtxHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMinit", status, inst);
	}
	else
	{
	    jCtxHandle = newHandleObj(jenv, CLASSREF(ContextHandle), METHODID(ContextHandle, init), iiCtxHandle);
	    if (!jCtxHandle)
	    {
		inst->PMfree(iiCtxHandle);
	    }
	}

	return jCtxHandle;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    nativePMdelete
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_nativePMdelete(JNIEnv *jenv, jobject _this, jobject  ctxHandle, jstring sValue)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	IngresChars szValue(jenv, _this, sValue);
	if (jenv->ExceptionCheck())
	    return;

	i4 status = inst->PMdelete(iiCtxHandle, szValue);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMdelete", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMexpToRegExp
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;)Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_PMexpToRegExp(JNIEnv *jenv, jobject _this, jobject ctxHandle, jstring sExpr)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	IngresChars szExpr(jenv, _this, sExpr);
	if (jenv->ExceptionCheck())
	    return NULL;

	const char *regexp = NULL;
	i4 status = inst->PMexpToRegExp(iiCtxHandle, szExpr, &regexp);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMexpToRegExp", status, inst);
	    return NULL;
	}
	else
	{
	    return getStringFromIngresChars(jenv, _this, regexp);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMfree
 * Signature: (Lcom/ingres/ContextHandle;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_PMfree(JNIEnv *jenv, jobject _this, jobject ctxHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	i4 status = inst->PMfree(iiCtxHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMfree", status, inst);
	}
	else
	{
	    setHandleValue(jenv, ctxHandle, NULL);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMget
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;)Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_PMget(JNIEnv *jenv, jobject _this, jobject ctxHandle, jstring sExpr)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	IngresChars szExpr(jenv, _this, sExpr);
	if (jenv->ExceptionCheck())
	    return NULL;

	const char *szValue = NULL;
	i4 status = inst->PMget(iiCtxHandle, szExpr, &szValue);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMget", status, inst);
	    return NULL;
	}
	else
	{
	    return getStringFromIngresChars(jenv, _this, szValue);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMgetDefault
 * Signature: (Lcom/ingres/ContextHandle;I)Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_PMgetDefault(JNIEnv *jenv, jobject _this, jobject ctxHandle, jint idx)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	const char *szValue = NULL;
	i4 status = inst->PMgetDefault(iiCtxHandle, idx, &szValue);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMgetDefault", status, inst);
	    return NULL;
	}
	else
	{
	    return getStringFromIngresChars(jenv, _this, szValue);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMhost
 * Signature: (Lcom/ingres/ContextHandle;)Ljava/lang/String;
 */
jstring JNICALL
Java_com_ingres_LocalIIapi_PMhost(JNIEnv *jenv, jobject _this, jobject ctxHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	const char *szHost = NULL;
	i4 status = inst->PMhost(iiCtxHandle, &szHost);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMhost", status, inst);
	    return NULL;
	}
	else
	{
	    return getStringFromNativeChars(jenv, _this, szHost);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    nativePMinsert
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;Ljava/lang/String;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_nativePMinsert(JNIEnv *jenv, jobject _this, jobject ctxHandle, jstring sKey, jstring sValue)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	IngresChars szKey(jenv, _this, sKey);
	if (jenv->ExceptionCheck())
	    return;

	IngresChars szValue(jenv, _this, sValue);
	if (jenv->ExceptionCheck())
	    return;

	i4 status = inst->PMinsert(iiCtxHandle, szKey, szValue);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMinsert", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMload
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_PMload(JNIEnv *jenv, jobject _this, jobject ctxHandle, jstring sPath)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	NativeChars szPath(jenv, _this, sPath);
	if (jenv->ExceptionCheck())
	    return;

	i4 status = inst->PMload(iiCtxHandle, szPath);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMload", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMnumElem
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;)I
 */
jint JNICALL
Java_com_ingres_LocalIIapi_PMnumElem(JNIEnv *jenv, jobject _this, jobject ctxHandle, jstring sExpr)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return 0;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	IngresChars szExpr(jenv, _this, sExpr);
	if (jenv->ExceptionCheck())
	    return 0;

	i4 num = 0;
	i4 status = inst->PMnumElem(iiCtxHandle, szExpr, &num);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMnumElem", status, inst);
	}

	return num;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMrestrict
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_PMrestrict(JNIEnv *jenv, jobject _this, jobject ctxHandle, jstring sExpr)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	IngresChars szExpr(jenv, _this, sExpr);
	if (jenv->ExceptionCheck())
	    return;

	i4 status = inst->PMrestrict(iiCtxHandle, szExpr);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMrestrict", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMscan
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;)Ljava/util/Map;
 */
jobject JNICALL
Java_com_ingres_LocalIIapi_PMscan(JNIEnv *jenv, jobject _this, jobject ctxHandle, jstring sExpr)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	IngresChars szExpr(jenv, _this, sExpr);
	if (jenv->ExceptionCheck())
	    return NULL;

	MEfreePtr<II_NAME_VALUE_PAIRS> pNV(inst);
	i4 status = inst->PMscan(iiCtxHandle, szExpr, &pNV);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMscan", status, inst);
	    return NULL;
	}

	// Create TreeMap to pass back
	jobject jMap = createObject(jenv, CLASSREF(TreeMap), METHODID(TreeMap, init));
	if (!jMap)
	    return NULL;

	for (int i = 0; i < pNV->count; i++)
	{
	    jstring jKey = getStringFromIngresChars(jenv, _this, pNV->pairs[i].name);
	    if (jenv->ExceptionCheck())
		return NULL;

	    jstring jVal = getStringFromIngresChars(jenv, _this, pNV->pairs[i].value);
	    if (jenv->ExceptionCheck())
		return NULL;

	    jenv->CallObjectMethod(jMap, METHODID(TreeMap, put), jKey, jVal);
	    if (jenv->ExceptionCheck())
		return NULL;

	    jenv->DeleteLocalRef(jKey);
	    jenv->DeleteLocalRef(jVal);
	}

	return jMap;
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMsetDefault
 * Signature: (Lcom/ingres/ContextHandle;ILjava/lang/String;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_PMsetDefault(JNIEnv *jenv, jobject _this, jobject ctxHandle, jint idx, jstring sValue)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	IngresChars szValue(jenv, _this, sValue);
	if (jenv->ExceptionCheck())
	    return;

	i4 status = inst->PMsetDefault(iiCtxHandle, idx, szValue);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMsetDefault", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    nativePMwrite
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_nativePMwrite(JNIEnv *jenv, jobject _this, jobject ctxHandle, jstring sPath)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	NativeChars szPath(jenv, _this, sPath);
	if (jenv->ExceptionCheck())
	    return;

	i4 status = inst->PMwrite(iiCtxHandle, szPath);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMwrite", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    PMlowerOn
 * Signature: (Lcom/ingres/ContextHandle;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_PMlowerOn(JNIEnv *jenv, jobject _this, jobject ctxHandle)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	i4 status = inst->PMlowerOn(iiCtxHandle);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "PMlowerOn", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    nativeCRsetPMval
 * Signature: (Lcom/ingres/ContextHandle;Ljava/lang/String;Ljava/lang/String;)V
 */
void JNICALL
Java_com_ingres_LocalIIapi_nativeCRsetPMval(JNIEnv *jenv, jobject _this, jobject ctxHandle, jstring sKey, jstring sValue)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return;

	II_HANDLE iiCtxHandle = getHandleValue(jenv, ctxHandle);

	IngresChars szKey(jenv, _this, sKey);
	if (jenv->ExceptionCheck())
	    return;

	IngresChars szValue(jenv, _this, sValue);
	if (jenv->ExceptionCheck())
	    return;

	i4 status = inst->CRsetPMval(iiCtxHandle, szKey, szValue);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "CRsetPMval", status, inst);
	}
}


/*
 * Class:     com_ingres_LocalIIapi
 * Method:    LGgetLogPaths
 * Signature: (ILjava/lang/String;)[Ljava/lang/String;
 */
jobjectArray JNICALL
Java_com_ingres_LocalIIapi_LGgetLogPaths(JNIEnv *jenv, jobject _this, jint iWhichLog, jstring sNodeName)
{
	TOOLSAPI *inst = getInstancePtr(jenv, _this);
	if (!inst)
	    return NULL;

	NativeChars szNodeName(jenv, _this, sNodeName);
	if (jenv->ExceptionCheck())
	    return NULL;

	char paths[LG_MAX_PATHS+1][256];
	// Note: if status == II_ST_NO_DATA, the "numPaths" might not be valid without
	// setting it to default
	i4 numPaths = 0;
	jniLogger->debug("Calling LGgetLogPaths for (%d) log", iWhichLog);
	i4 status = inst->LGgetLogPaths(iWhichLog, szNodeName, paths, &numPaths);
	if (status != II_ST_SUCCESS && status != II_ST_NO_DATA)
	{
	    throwLastAPIErrors(jenv, _this, "LGgetLogPaths", status, inst);
	}

	jobjectArray pathArray = jenv->NewObjectArray(numPaths, CLASSREF(String), NULL);
	jniLogger->trace("Got path array at %p", pathArray);
	if (pathArray)
	{
	    for (int i = 0; i < numPaths; i++)
	    {
		jniLogger->debug("LGpath[%d] = '%s'", i, paths[i]);
		jenv->SetObjectArrayElement(pathArray, i, getStringFromNativeChars(jenv, _this, paths[i]));
		if (jenv->ExceptionCheck())
		    return NULL;
	    }
	}

	return pathArray;
}


