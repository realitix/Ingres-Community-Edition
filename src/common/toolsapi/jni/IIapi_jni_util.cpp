/*
** Copyright 2008-2011 Actian Corporation. All rights reserved
**
** Description:
**	Utility methods for the Actian Tools API JNI wrapper.
**
** History:
**    March 2008 (whiro01)
**	Initial prototype version.
**    07-Dec-2009 (whiro01)
**	Cleaned up; added comments.
**    04-Jan-2010 (whiro01)
**	Added new "sprintf" wrapper that allocates destination memory.
**    05-Jan-2010 (whiro01)
**	Moved the field ID / method ID helpers into here to be shared
**	among modules.
**    03-Feb-2010 (whiro01)
**	Added FLD_VERSIONINFO and processing for it.
**    06-Jun-2010 (whiro01)
**	Changed to use non-Ingres functions where appropriate.
**    17-Jun-2010 (whiro01)
**	Made JNU_GetStringUTFChars into an inline wrapper for
**	the simple jenv->GetStringUTFChars(); then changed all
**	"free(str)" into JNU_FreeStringUTFChars() which just
**	calls jenv->ReleaseStringUTFChars();
**    22-Jun-2010 (whiro01)
**	Part of rework of the Tools API: rename ToolsAPI to
**	ToolsInterface.
**    28-Jun-2010 (whiro01)
**	Extra conditions on using the new "secure" Windows
**	CRT functions (VS2008 or higher).
**    27-Jul-2010 (whiro01)
**	Fix the IIapi_exception name.
**    21-Sep-2010 (dansa01)
**	Added JNU_ThrowLastApiError helper function.
**    04-Oct-2010 (dansa01)
**	Consolidated other frequently-used exception name strings into
**	shared definitions (following the example of IIapi_exception).
**	Generalized JNU_GetStructFieldIDs a bit, to allow classes that
**	are under com/ingres, but not nested in IIapi.
**    25-Oct-2010 (whiro01)
**	Parse platform identifier out of release string and put into VersionInfo.
**    01-Nov-2010 (dansa01)
**	Adapted to new JNI-like TOOLSAPI interface (renamed from ToolsInterface).
**    19-Nov-2010 (dansa01)
**	Removed JNU_ThrowLastApiError.  It has been replaced by a new function
**	(throwLastAPIErrors) in IIapi_jni.cpp (where it can more easily access
**	the IIapi.Exception class object and method IDs).
**    02-Dec-2010 (dansa01)
**	Removed unused JNU_ReadVersionRel function.  Replaced C++ new operator
**	in JNU_ParseVersionString because existing code did not handle the
**	potential std::bad_alloc exception.
**    08-Dec-2010 (dansa01)
**	Removed IIapi_exception variable to minimize temptation to throw
**	IIapi.Exception by name.  We should now always use the constructor
**	that allows us to fill in the type, code, and source fields.
**	Added IllegalStateException.
**    14-Dec-2010 (dansa01)
**	Restructured to help balance out source file sizes, and to prepare for
**	adding charset conversion classes and functions.  Consolidated all
**	cached class references, methodIDs, fieldIDs into this module, and
**	simplified their initialization by using a single ClassInfo array and
**	a common set of initialization functions (instead of treating classes
**	inside the com.ingres package as a special case).  Also moved most of
**	the helper functions from IIapi_jni.cpp into here.  Now IIapi_jni.cpp is
**	basically just JNI entrypoints.
**    15-Dec-2010 (dansa01)
**	Added basic charset conversion support (IngresChars, NativeChars,
**	getStringFromIngresChars, getStringFromNativeChars, IIapi encode/decode
**	methods, and ([B) String constructor).  This is just phase one.
**	Retrofitting existing code to fully use these tools will be phase two.
**    16-Dec-2010 (dansa01)
**	Split string-handling utility functions into two flavors (for Ingres and
**	Native charsets).  Had to add "_this" parameter to the Ingres flavors to
**	access the IIapi methods that do the actual charset conversions.
**    19-Jan-2011 (dansa01)
**	Must break out of the loop in JNU_GetFieldIDs and JNU_GetMethodIDs when
**	a pending exception is detected.
**    08-Feb-2011 (dansa01)
**	Added fieldID cache for ConnectOptions type.
**    11-Feb-2011 (dansa01)
**	Added BufferList class and ungetJavaDataObject function.  Added new
**	methodIDs for extracting scalar values from Byte, Short, Integer, etc.
**	Java objects.
**    02-Mar-2011 (whiro01)
**	Added CreateDBOptions to the class and method lists; added "set" method
**	to IngresChars and NativeChars; added "throwStartupError" method
**	to better report errors during ToolsAPIInit().
**    19-Apr-2011 (dansa01)
**	Added makeCAPIdescVdataV helper function, which factors out code from
**	the executeProcedure method so that it can be shared with the new
**	putParameters method (coming soon).
**    21-Apr-2011 (dansa01)
**	Removed references to obsolete NetuXxx classes.
**    26-Apr-2011 (whiro01)
**	Add InstallType enum and type field to VersionInfo and parsing to
**	figure out Ingres vs. Vectorwise installation; throw exception
**	if datatype is weird coming into the conversion functions.
**    13-Jun-2011 (gordy)
**	Add effUser to CreateDBOptions.
**    20-Jun-2011 (dansa01)
**	Renamed IIapi class references to LocalIIapi, since IIapi is now an
**	interface, implemented by the LocalIIapi class in conjunction with this
**	JNI DLL.  (Changing the variable names was not strictly necessary,
**	but seemed an appropriate thing to do in the interest of clarity.)
**    22-Jun-2011 (dansa01)
**	Replaced the remaining C++ version string parser with the new Java regex
**	parser now in the VersionInfo constructor.  Let's do all version string
**	parsing only one way, in only one place.
**    17-Aug-2011 (whiro01)
**	Removed the CreateDBOptions stuff now that we're not implementing that
**	code in here anymore.
**    26-Sep-2011 (whiro01)
**	Change the header file include paths for compilation in Ingres codelines.
**    29-Sep-2011 (whiro01)
**	Remove redundant #include of stdarg.h (already included in IIapi_jni_log.h).
**    11-Oct-2011 (dansa01)
**	Now using IIapi instance methods (utilizing java.nio.charset classes) to
**	do charset conversions for both the Ingres and Native cases.  Added
**	"_this" parameter to functions that do Native charset conversions so that
**	they can now access the IIapi instance.  Added new IIapi method IDs, and
**	refactored to take advantage of shared encode/decode helper functions.
**    18-Oct-2011 (whiro01)
**	Added fullyQualifiedHostName to the SysInfo structure.
*/
#include "toolsapi.h"
#include "IIapi_jni_log.h"

/*
** Logger to use to output messages
*/
extern Logger *jniLogger;


/***************************************************************************************
**                                                                                    **
**     Field ID & method ID descriptors for referencing Java stuff from C code        **
**                                                                                    **
***************************************************************************************/
/*	public class LocalIIapi {
		private long Instance = null;
	}
*/
CLASSDEF(LocalIIapi);
  FIELDDEF(LocalIIapi, Instance);
  METHODDEF(LocalIIapi, encodeNativeChars);
  METHODDEF(LocalIIapi, decodeNativeChars);
  METHODDEF(LocalIIapi, encodeIngresChars);
  METHODDEF(LocalIIapi, decodeIngresChars);

FIELDINFO(LocalIIapi) = {
	FIELDREF(LocalIIapi, Instance,	FLD_LONG)
};

METHODINFO(LocalIIapi) = {
	METHODREF(LocalIIapi, encodeNativeChars, "(Ljava/lang/String;)[B", false),
	METHODREF(LocalIIapi, decodeNativeChars, "([B)Ljava/lang/String;", false),
	METHODREF(LocalIIapi, encodeIngresChars, "(Ljava/lang/String;)[B", false),
	METHODREF(LocalIIapi, decodeIngresChars, "([B)Ljava/lang/String;", false)
};


/*	public static class SysInfo {
		public String IIinstallation = null;
		public String IIsystem = null;
		public String hostName = null;
		public String fullyQualifiedHostName = null;
		public VersionInfo version = null;
	}
*/
CLASSDEF(SysInfo);
  METHODDEF(SysInfo, init);
  FIELDDEF(SysInfo, IIinstallation);
  FIELDDEF(SysInfo, IIsystem);
  FIELDDEF(SysInfo, hostName);
  FIELDDEF(SysInfo, fullyQualifiedHostName);
  FIELDDEF(SysInfo, version);

FIELDINFO(SysInfo) = {
	FIELDREF(SysInfo, IIinstallation,	FLD_STRING),
	FIELDREF(SysInfo, IIsystem,		FLD_STRING),
	FIELDREF(SysInfo, hostName,		FLD_STRING),
	FIELDREF(SysInfo, fullyQualifiedHostName,FLD_STRING),
	FIELDREF(SysInfo, version,		FLD_VERSIONINFO)
};

METHODINFO(SysInfo) = {
	METHODINI(SysInfo, init, "()V")
};


CLASSDEF(InstallType);
  METHODDEF(InstallType, getPrefix);
  METHODDEF(InstallType, findType);

METHODINFO(InstallType) = {
	METHODREF(InstallType, getPrefix, "()Ljava/lang/String;", false),
	METHODREF(InstallType, findType, "(Ljava/lang/String;)Lcom/ingres/IIapi$InstallType;", true)
};


/*	public static class VersionInfo {
		int majorVersion = -1;
		int minorVersion = -1;
		int revision = -1;
		String platform = null;
		int build = -1;
		String releaseString = null;
		InstallType type;
	}
*/
CLASSDEF(VersionInfo);
  METHODDEF(VersionInfo, init);
  FIELDDEF(VersionInfo, majorVersion);
  FIELDDEF(VersionInfo, minorVersion);
  FIELDDEF(VersionInfo, revision);
  FIELDDEF(VersionInfo, platform);
  FIELDDEF(VersionInfo, build);
  FIELDDEF(VersionInfo, releaseString);
  FIELDDEF(VersionInfo, type);

FIELDINFO(VersionInfo) = {
	FIELDREF(VersionInfo, majorVersion,	FLD_INT),
	FIELDREF(VersionInfo, minorVersion,	FLD_INT),
	FIELDREF(VersionInfo, revision,		FLD_INT),
	FIELDREF(VersionInfo, platform,		FLD_STRING),
	FIELDREF(VersionInfo, build,		FLD_INT),
	FIELDREF(VersionInfo, releaseString,	FLD_STRING),
	FIELDREF(VersionInfo, type,		FLD_INSTALLTYPE)
};

METHODINFO(VersionInfo) = {
	METHODINI(VersionInfo, init, "(Ljava/lang/String;)V")
};


/*	public static enum Privileges {
		SERVER_CONTROL,
		NET_ADMIN,
		MONITOR,
		TRUSTED
	}
*/
CLASSDEF(Privileges);
  METHODDEF(Privileges, ordinal);

METHODINFO(Privileges) = {
	METHODREF(Privileges, ordinal, "()I", false)
};


/*	public static class Descriptor {
		public short dataType = 0;
		public boolean nullable = false;
		public short length = 0;
		public short precision = 0;
		public short scale = 0;
		public short columnType = 0;
		public String columnName = null;
	}
*/
CLASSDEF(Descriptor);
  METHODDEF(Descriptor, init);
  FIELDDEF(Descriptor, dataType);
  FIELDDEF(Descriptor, nullable);
  FIELDDEF(Descriptor, length);
  FIELDDEF(Descriptor, precision);
  FIELDDEF(Descriptor, scale);
  FIELDDEF(Descriptor, columnType);
  FIELDDEF(Descriptor, columnName);

FIELDINFO(Descriptor) = {
	FIELDREF(Descriptor, dataType,	FLD_SHORT),
	FIELDREF(Descriptor, nullable,	FLD_BOOLEAN),
	FIELDREF(Descriptor, length,	FLD_SHORT),
	FIELDREF(Descriptor, precision,	FLD_SHORT),
	FIELDREF(Descriptor, scale,	FLD_SHORT),
	FIELDREF(Descriptor, columnType,FLD_SHORT),
	FIELDREF(Descriptor, columnName,FLD_STRING)
};

METHODINFO(Descriptor) = {
	METHODINI(Descriptor, init, "()V")
};


/*	public static class DataValue {
		public boolean isnull = true;
		public int length = 0;
		public Object value = null;
	}
*/
CLASSDEF(DataValue);
  METHODDEF(DataValue, init);
  FIELDDEF(DataValue, isnull);
  FIELDDEF(DataValue, length);
  FIELDDEF(DataValue, value);

FIELDINFO(DataValue) = {
	FIELDREF(DataValue, isnull, FLD_BOOLEAN),
	FIELDREF(DataValue, length, FLD_INT),
	FIELDREF(DataValue, value,  FLD_OBJECT)
};

METHODINFO(DataValue) = {
	METHODINI(DataValue, init, "()V")
};


/*	public static class QueryInfo {
		public long flags = 0;
		public long mask = 0;
		public long rowCount = 0;
		public boolean readOnly = false;
		public long procReturn = 0;
		public long procHandle = 0;
		public long repeatQueryHandle = 0;
		public String tableKey = null;
		public String objectKey = null;
		public long cursorType = 0;
		public long rowStatus = 0;
		public long rowPosition = 0;
	}
*/
CLASSDEF(QueryInfo);
  METHODDEF(QueryInfo, init);
  FIELDDEF(QueryInfo, flags);
  FIELDDEF(QueryInfo, mask);
  FIELDDEF(QueryInfo, rowCount);
  FIELDDEF(QueryInfo, readOnly);
  FIELDDEF(QueryInfo, procReturn);
  FIELDDEF(QueryInfo, procHandle);
  FIELDDEF(QueryInfo, repeatQueryHandle);
  FIELDDEF(QueryInfo, tableKey);
  FIELDDEF(QueryInfo, objectKey);
  FIELDDEF(QueryInfo, cursorType);
  FIELDDEF(QueryInfo, rowStatus);
  FIELDDEF(QueryInfo, rowPosition);

FIELDINFO(QueryInfo) = {
	FIELDREF(QueryInfo, flags,		FLD_LONG),
	FIELDREF(QueryInfo, mask,		FLD_LONG),
	FIELDREF(QueryInfo, rowCount,		FLD_LONG),
	FIELDREF(QueryInfo, readOnly,		FLD_BOOLEAN),
	FIELDREF(QueryInfo, procReturn,		FLD_LONG),
	FIELDREF(QueryInfo, procHandle,		FLD_LONG),
	FIELDREF(QueryInfo, repeatQueryHandle,	FLD_LONG),
	FIELDREF(QueryInfo, tableKey,		FLD_STRING),
	FIELDREF(QueryInfo, objectKey,		FLD_STRING),
	FIELDREF(QueryInfo, cursorType,		FLD_LONG),
	FIELDREF(QueryInfo, rowStatus,		FLD_LONG),
	FIELDREF(QueryInfo, rowPosition,	FLD_LONG)
};

METHODINFO(QueryInfo) = {
	METHODINI(QueryInfo, init, "()V")
};


/*	public static class IIapi.Exception {
	}
*/
CLASSDEF(APIException);
  METHODDEF(APIException, init);

METHODINFO(APIException) = {
	METHODINI(APIException, init, "(IILjava/lang/String;Ljava/lang/String;Lcom/ingres/IIapi$Exception;)V")
};


/*	public abstract class HandleBase {
		private long handleValue = 0;
	}
*/
CLASSDEF(HandleBase);
  FIELDDEF(HandleBase, handleValue);

FIELDINFO(HandleBase) = {
	FIELDREF(HandleBase, handleValue, FLD_LONG)
};


/*	public final class ConnectionHandle extends HandleBase {
	}
*/
CLASSDEF(ConnectionHandle);
  METHODDEF(ConnectionHandle, init);

METHODINFO(ConnectionHandle) = {
	METHODINI(ConnectionHandle, init, "()V")
};


/*	public final class TransactionHandle extends HandleBase {
	}
*/
CLASSDEF(TransactionHandle);
  METHODDEF(TransactionHandle, init);

METHODINFO(TransactionHandle) = {
	METHODINI(TransactionHandle, init, "()V")
};


/*	public final class StatementHandle extends HandleBase {
	}
*/
CLASSDEF(StatementHandle);
  METHODDEF(StatementHandle, init);

METHODINFO(StatementHandle) = {
	METHODINI(StatementHandle, init, "()V")
};


/*	public final class ContextHandle extends HandleBase {
	}
*/
CLASSDEF(ContextHandle);
  METHODDEF(ContextHandle, init);

METHODINFO(ContextHandle) = {
	METHODINI(ContextHandle, init, "()V")
};


/*	public final class ConnectOptions {
		public String effectiveUser = null;
		public String group = null;
		public String role = null;
		public String dbmsPassword = null;
	}
*/
CLASSDEF(ConnectOptions);
  FIELDDEF(ConnectOptions, effectiveUser);
  FIELDDEF(ConnectOptions, group);
  FIELDDEF(ConnectOptions, role);
  FIELDDEF(ConnectOptions, dbmsPassword);

FIELDINFO(ConnectOptions) = {
	FIELDREF(ConnectOptions, effectiveUser, FLD_STRING),
	FIELDREF(ConnectOptions, group,         FLD_STRING),
	FIELDREF(ConnectOptions, role,          FLD_STRING),
	FIELDREF(ConnectOptions, dbmsPassword,  FLD_STRING)
};


CLASSDEF(String);
  METHODDEF(String, init);
  METHODDEF(String, getBytes);

METHODINFO(String) = {
	METHODINI(String, init, "([B)V"),
	METHODREF(String, getBytes, "()[B", false)
};


CLASSDEF(ArrayList);
  METHODDEF(ArrayList, init);
  METHODDEF(ArrayList, add);

METHODINFO(ArrayList) = {
	METHODINI(ArrayList, init, "()V"),
	METHODREF(ArrayList, add, "(Ljava/lang/Object;)Z", false)
};


CLASSDEF(TreeMap);
  METHODDEF(TreeMap, init);
  METHODDEF(TreeMap, put);

METHODINFO(TreeMap) = {
	METHODINI(TreeMap, init, "()V"),
	METHODREF(TreeMap, put, "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;", false)
};


CLASSDEF(Byte);
  METHODDEF(Byte, valueOf);
  METHODDEF(Byte, byteValue);

METHODINFO(Byte) = {
	METHODREF(Byte, valueOf, "(B)Ljava/lang/Byte;", true),
	METHODREF(Byte, byteValue, "()B", false)
};


CLASSDEF(Short);
  METHODDEF(Short, valueOf);
  METHODDEF(Short, shortValue);

METHODINFO(Short) = {
	METHODREF(Short, valueOf, "(S)Ljava/lang/Short;", true),
	METHODREF(Short, shortValue, "()S", false)
};


CLASSDEF(Integer);
  METHODDEF(Integer, valueOf);
  METHODDEF(Integer, intValue);

METHODINFO(Integer) = {
	METHODREF(Integer, valueOf, "(I)Ljava/lang/Integer;", true),
	METHODREF(Integer, intValue, "()I", false)
};


CLASSDEF(Long);
  METHODDEF(Long, valueOf);
  METHODDEF(Long, longValue);

METHODINFO(Long) = {
	METHODREF(Long, valueOf, "(J)Ljava/lang/Long;", true),
	METHODREF(Long, longValue, "()J", false)
};


CLASSDEF(Float);
  METHODDEF(Float, valueOf);
  METHODDEF(Float, floatValue);

METHODINFO(Float) = {
	METHODREF(Float, valueOf, "(F)Ljava/lang/Float;", true),
	METHODREF(Float, floatValue, "()F", false)
};


CLASSDEF(Double);
  METHODDEF(Double, valueOf);
  METHODDEF(Double, doubleValue);

METHODINFO(Double) = {
	METHODREF(Double, valueOf, "(D)Ljava/lang/Double;", true),
	METHODREF(Double, doubleValue, "()D", false)
};


/*
** Master list of our classes to be defined
*/
static ClassInfo Classes[] =
{
  { &CLASSREF(LocalIIapi),        "com/ingres/LocalIIapi",        FIELDLIST(LocalIIapi),     METHODLIST(LocalIIapi)        },
  { &CLASSREF(InstallType),       "com/ingres/IIapi$InstallType", FIELDLIST_EMPTY,           METHODLIST(InstallType)       },
  { &CLASSREF(SysInfo),           "com/ingres/IIapi$SysInfo",     FIELDLIST(SysInfo),        METHODLIST(SysInfo)           },
  { &CLASSREF(VersionInfo),       "com/ingres/IIapi$VersionInfo", FIELDLIST(VersionInfo),    METHODLIST(VersionInfo)       },
  { &CLASSREF(Privileges),        "com/ingres/IIapi$Privileges",  FIELDLIST_EMPTY,           METHODLIST(Privileges)        },
  { &CLASSREF(Descriptor),        "com/ingres/IIapi$Descriptor",  FIELDLIST(Descriptor),     METHODLIST(Descriptor)        },
  { &CLASSREF(DataValue),         "com/ingres/IIapi$DataValue",   FIELDLIST(DataValue),      METHODLIST(DataValue)         },
  { &CLASSREF(QueryInfo),         "com/ingres/IIapi$QueryInfo",   FIELDLIST(QueryInfo),      METHODLIST(QueryInfo)         },
  { &CLASSREF(APIException),      "com/ingres/IIapi$Exception",   FIELDLIST_EMPTY,           METHODLIST(APIException)      },
  { &CLASSREF(HandleBase),        "com/ingres/HandleBase",        FIELDLIST(HandleBase),     METHODLIST_EMPTY              },
  { &CLASSREF(ConnectionHandle),  "com/ingres/ConnectionHandle",  FIELDLIST_EMPTY,           METHODLIST(ConnectionHandle)  },
  { &CLASSREF(TransactionHandle), "com/ingres/TransactionHandle", FIELDLIST_EMPTY,           METHODLIST(TransactionHandle) },
  { &CLASSREF(StatementHandle),   "com/ingres/StatementHandle",   FIELDLIST_EMPTY,           METHODLIST(StatementHandle)   },
  { &CLASSREF(ContextHandle),     "com/ingres/ContextHandle",     FIELDLIST_EMPTY,           METHODLIST(ContextHandle)     },
  { &CLASSREF(ConnectOptions),    "com/ingres/ConnectOptions",    FIELDLIST(ConnectOptions), METHODLIST_EMPTY              },
  { &CLASSREF(String),            "java/lang/String",             FIELDLIST_EMPTY,           METHODLIST(String)            },
  { &CLASSREF(ArrayList),         "java/util/ArrayList",          FIELDLIST_EMPTY,           METHODLIST(ArrayList)         },
  { &CLASSREF(TreeMap),           "java/util/TreeMap",            FIELDLIST_EMPTY,           METHODLIST(TreeMap)           },
  { &CLASSREF(Byte),              "java/lang/Byte",               FIELDLIST_EMPTY,           METHODLIST(Byte)              },
  { &CLASSREF(Short),             "java/lang/Short",              FIELDLIST_EMPTY,           METHODLIST(Short)             },
  { &CLASSREF(Integer),           "java/lang/Integer",            FIELDLIST_EMPTY,           METHODLIST(Integer)           },
  { &CLASSREF(Long),              "java/lang/Long",               FIELDLIST_EMPTY,           METHODLIST(Long)              },
  { &CLASSREF(Float),             "java/lang/Float",              FIELDLIST_EMPTY,           METHODLIST(Float)             },
  { &CLASSREF(Double),            "java/lang/Double",             FIELDLIST_EMPTY,           METHODLIST(Double)            }
};


/*
** Names of frequently used exception classes.
*/
const char *OutOfMemoryError = "java/lang/OutOfMemoryError";
const char *NullPointerException = "java/lang/NullPointerException";
const char *IllegalArgumentException = "java/lang/IllegalArgumentException";
const char *IllegalStateException = "java/lang/IllegalStateException";
const char *ArrayIndexOutOfBoundsException = "java/lang/ArrayIndexOutOfBoundsException";


/*
** Module initialization and termination functions.
*/

void
JNU_Initialize(JNIEnv *jenv)
{
	jniLogger->debug("Trying to load Java classes...");
	JNU_LoadClasses(jenv, Classes, ARRAY_SIZE(Classes));
}


void
JNU_Terminate(JNIEnv *jenv)
{
	jniLogger->debug("Unloading Java classes...");
	JNU_UnloadClasses(jenv, Classes, ARRAY_SIZE(Classes));
}


/*{
** Name: JNU_LoadClasses - Load and cache class refs, FIDs, and MIDs
**
** Description:
**	This helper loads all the Java classes specified in our ClassInfo
**	array, and then caches the class references, fieldIDs, and methodIDs
**	in the associated class definition structures.
**
** Inputs:
**	jenv		The JNIEnv.
**	classes		Array of ClassInfo structures.
**	numclasses	Number of elements in the above array.
**
** Outputs:
**	The class definition structures are filled in.
**
** Returns:
**	None.
**
** Side	Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck
**	after calling this.
**
** History:
**	14-Dec-2010 (dansa01)
**	    Adapted from existing inline code.
*/
void
JNU_LoadClasses(JNIEnv *jenv, ClassInfo classes[], int numclasses)
{
	for (int i = 0; i < numclasses; i++)
	{
	    *classes[i].pClass = JNU_LoadClass(jenv, classes[i].classname);
	    if (jenv->ExceptionCheck())
		return;

	    JNU_GetFieldIDs(jenv,
			    *classes[i].pClass,
			    classes[i].classname,
			    classes[i].fields,
			    classes[i].numfields);
	    if (jenv->ExceptionCheck())
		return;

	    JNU_GetMethodIDs(jenv,
			     *classes[i].pClass,
			     classes[i].classname,
			     classes[i].methods,
			     classes[i].nummethods);
	    if (jenv->ExceptionCheck())
		return;
	}
}


/*
** Name: JNU_UnloadClasses - Helper function to loop through a list of loaded classes
**	(system or our own) and free the global ref that we got when we loaded them.
**
** Inputs:
**	jenv		The Java execution environment pointer
**	classes		Array of ClassInfo structures describing the class(es)
**			to unload
**	numclasses	Number of elements in the above array
**
** Outputs:
**	The class is unloaded and the stored class reference is zero'd out.
**
** Returns:
**	None
**
** History:
**    05-Jan-2010 (whiro01)
**	Second round of expanded coding.
*/
void
JNU_UnloadClasses(JNIEnv *jenv, ClassInfo classes[], int numclasses)
{
	for (int i = 0; i < numclasses; i++)
	{
	    jclass *pClass = classes[i].pClass;
	    jenv->DeleteWeakGlobalRef(*pClass);
	    *pClass = NULL;
	}
}


/*{
** Name: JNU_LoadClass - Load and cache a Java class reference
**
** Description:
**	This helper loads the named Java class, turns it into a WeakGlobalRef,
**	and returns the WeakGlobalRef.
**
** Inputs:
**	jenv		The JNIEnv.
**	szClassName	The full name of the class to load.
**
** Outputs:
**	None.
**
** Returns:
**	A WeakGlobalRef to the loaded class.
**
** Side	Effects:
**	May leave an exception pending in the JVM.
**
** History:
**	14-Dec-2010 (dansa01)
**	    Documented existing function.
*/
jclass
JNU_LoadClass(JNIEnv *jenv, const char *szClassName)
{
	jclass _class = jenv->FindClass(szClassName);

	if (_class == NULL)
	{
	    jniLogger->error("Could not load %s class!", szClassName);
	}
	else
	{
	    jclass globalClass = static_cast<jclass>(jenv->NewWeakGlobalRef(_class));
	    if (globalClass == NULL)
	    {
		jniLogger->error("Could not create global reference to %s class!", szClassName);
	    }
	    jenv->DeleteLocalRef(_class);
	    _class = globalClass;
	}

	return _class;
}


/*
** Name: JNU_GetFieldIDs - Load Java field IDs for all classes in our list
**
** Description:
**	This function fills our class definition structures with field IDs for
**	all the fields we want to use.  The field definition structures contain
**	a FieldType enum that signifies which Java type signature must be used
**	when calling GetFieldID.
**
** Inputs:
**	jenv		The Java execution environment pointer
**	_class		Previously loaded class reference
**	classname	Name of that class
**	fields		Array of FieldInfo structures describing the fields
**	numfields	Number of elements in the above array
**
** Outputs:
**	The field ID value referenced by each of the "fields" elements is
**	filled in appropriately.
**
** Returns:
**	None.
**
** Side Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck
**	after calling this function.
**
** History:
**    March 2008 (whiro01)
**	Part of original coding.
**    04-Oct-2010 (dansa01)
**	Generalized a bit to allow classes that are not nested in IIapi.
**    14-Dec-2010 (dansa01)
**	Restructured to work just like GetMethodIDs.  Now we use the same
**	pattern for all types of classes, system or otherwise.  The LoadClass
**	is done first, followed by calls to GetFieldsIDs and GetMethodIDs.
*/
void
JNU_GetFieldIDs(JNIEnv *jenv, jclass _class, const char *classname, FieldInfo fields[], int numfields)
{
	const char *fieldtype;

	for (int i = 0; i < numfields; i++)
	{
	    switch (fields[i].type)
	    {
		case FLD_SHORT:
		    fieldtype = "S";
		    break;
		case FLD_INT:
		    fieldtype = "I";
		    break;
		case FLD_LONG:
		    fieldtype = "J";
		    break;
		case FLD_BOOLEAN:
		    fieldtype = "Z";
		    break;
		case FLD_STRING:
		    fieldtype = "Ljava/lang/String;";
		    break;
		case FLD_OBJECT:
		    fieldtype = "Ljava/lang/Object;";
		    break;
		case FLD_OBJARRAY:
		    fieldtype = "[Ljava/lang/Object;";
		    break;
		case FLD_VERSIONINFO:
		    fieldtype = "Lcom/ingres/IIapi$VersionInfo;";
		    break;
		case FLD_INSTALLTYPE:
		    fieldtype = "Lcom/ingres/IIapi$InstallType;";
		    break;
		default:
		    fieldtype = "<unknown>";
		    break;
	    }

	    jfieldID id = jenv->GetFieldID(_class, fields[i].fieldname, fieldtype);

	    if (id == NULL)
	    {
		jniLogger->error("Could not find field '%s' (%s) in class '%s'!",
		    fields[i].fieldname, fieldtype, classname);
		break;
	    }

	    *fields[i].fieldID = id;
	}
}


/*
** Name: JNU_GetMethodIDs - Load Java method IDs for all classes in our list
**
** Description:
**	This function fills our class definition structures with method IDs for
**	all the methods we want to use.  The constructor is usual, but there may
**	be others as well.  Note that we must treat static methods differently
**	than regular instance methods, so there is an "isStatic" flag associated
**	with each method to tell the difference.
**
** Inputs:
**	jenv		The Java execution environment pointer
**	_class		Previously loaded class reference
**	classname	Name of that class
**	methods		Array of MethodInfo structures describing the methods
**	nummethods	Number of elements in the above array
**
** Outputs:
**	The method ID value referenced by each of the "methods" structures
**	is filled in appropriately.
**
** Returns:
**	None
**
** Side Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck
**	after calling this function.
**
** History:
**    Dec 2009 (whiro01)
**	Part of first round of expanded coding.
*/
void
JNU_GetMethodIDs(JNIEnv *jenv, jclass _class, const char *classname, MethodInfo methods[], int nummethods)
{
	for (int i = 0; i < nummethods; i++)
	{
	    jmethodID id;

	    if (methods[i].isStatic)
		id = jenv->GetStaticMethodID(_class, methods[i].methodname, methods[i].signature);
	    else
		id = jenv->GetMethodID(_class, methods[i].methodname, methods[i].signature);

	    if (id == NULL)
	    {
		jniLogger->error("Could not find method '%s%s' in class '%s'!",
		    methods[i].methodname, methods[i].signature, classname);
		break;
	    }

	    *methods[i].methodID = id;
	}
}


/*
** Code taken from Section 6.1.2 (pg. 75) of "jni.pdf":
** "The Java™ Native Interface  Programmer's Guide and Specification"
** Copyright 1999 Sun Microsystems, Inc.
*/
void
JNU_ThrowByName(JNIEnv *jenv, const char *name, const char *msg)
{
	jclass cls = jenv->FindClass(name);

	/* if cls is NULL, an exception has already been thrown */
	if (cls != NULL) {
		jenv->ThrowNew(cls, msg);

		/* free the local ref */
		jenv->DeleteLocalRef(cls);
	}
}


/*
** Name: JNU_sprintf - clone of sprintf that also does
**	memory allocation (to avoid buffer overflows)
**
** Description:
**	Format a "printf" format string into an allocated
**	buffer guaranteed large enough to hold the entire string.
**	TODO: Only handles %%, and %s and %d without qualifiers
**	if using the non-Windows, non-C99 version
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
JNU_sprintf(JNIEnv *jenv, const char *format, ...)
{
	va_list args;
	size_t size;
	char *buf;

	/* Start counting size of fixed string plus space for formatted arguments */
	va_start(args, format);
#if defined(WIN32) && (_MSC_VER >= 1500)
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
	if ((buf = (char *)malloc(size * sizeof(char))) == NULL)
	{
		JNU_ThrowByName(jenv, OutOfMemoryError, 0);
		return buf;
	}

	/* Now start the actual formatting of the destination */
	va_start(args, format);
#if defined(WIN32) && (_MSC_VER >= 1500)
	vsprintf_s(buf, size, format, args);
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




/*{
** Name: getInstancePtr - Get TOOLSAPI instance from IIapi Java instance
**
** Description:
**	This helper function extracts the TOOLSAPI instance pointer from the
**	current IIapi Java instance (passed as "_this" parameter).
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The current IIapi instance.
**
** Outputs:
**	None.
**
** Returns:
**	TOOLSAPI instance pointer.  If NULL, an exception was thrown.
**
** Side	Effects:
**	May leave an exception pending in the JVM.  Always test result for NULL
**	before using.
**
** History:
**	14-Dec-2010 (dansa01)
**	    Documented existing function.
*/
TOOLSAPI*
getInstancePtr(JNIEnv *jenv, jobject _this)
{
	TOOLSAPI *inst = NULL;

	inst = (TOOLSAPI *)jenv->GetLongField(_this, FIELDID(LocalIIapi, Instance));
	if (inst == NULL)
	{
	    JNU_ThrowByName(jenv, IllegalStateException, "Ingres installation instance not loaded.");
	}

	// TODO: need to check for NULL return in every caller!  UGH!
	return inst;
}


/*{
** Name: throwStartupError - Translate C-API status into an exception
**
** Description:
**	This helper function translates an API error status into an exception
**	and throws it.  It is meant to be called only for startup functions that
**	return errors before the error stack is setup and the regular error reporting
**	mechanisms can be invoked.  Therefore, it has no dependencies on the Ingres
**	ToolsAPI being available to assist.
**
**	It is, therefore, dependent on interpreting specific Ingres status codes
**	to come up with an (English-only) message describing the failure in more
**	detail.
**
**	NOTE: The "fn" parameter should be the name of the C-API function, not
**	the Java native function calling this.  The Java native function is
**	implicitly part of the exception stack trace, so adding its name to the
**	message text doesn't provide new information.  What is more useful is
**	the name of the C-API function that failed.
**
**	Call this function ONLY if you actually have an error status to report.
**	(Checking for success here is not helpful, because the typical usage
**	model requires you to test before you call this anyway.)
**
** Inputs:
**	jenv		The JNIEnv.
**	fn		The name of the C-API function that returned an error.
**	status		The Ingres status code indicating an error.
**
** Outputs:
**	None.
**
** Returns:
**	void
**
** Side	Effects:
**	Leaves an exception pending in the JVM.  Every exception object created
**	is logged.
**
** History:
**	03-Mar-2011 (whiro01)
**	    Initial coding based on other exception code in here.
*/
# define E_CL_MASK	0x00010000
# define E_GL_MASK	0x00D50000
# define E_ER_MASK	0x00000900
# define E_SI_MASK	0x00001900
# define E_CM_MASK	0x00002A00
# define E_PM_MASK	0x00006000

# define II_ER_CM_NOCHARSET	(E_CL_MASK + E_CM_MASK + 0x01)
# define II_ER_ER_BADLANGUAGE	(E_CL_MASK + E_ER_MASK + 0x0F)
# define II_ER_PM_NO_MEMORY	(E_GL_MASK + E_PM_MASK + 0x0a)
# define II_ER_PM_NO_II_SYSTEM	(E_GL_MASK + E_PM_MASK + 0x08)
# define II_ER_PM_FILE_BAD	(E_GL_MASK + E_PM_MASK + 0x02)
# define II_ER_SI_CANT_OPEN	(E_CL_MASK + E_SI_MASK + 0x04)

void
throwStartupError(JNIEnv *jenv, const char *fn, i4 status)
{
	jniLogger->critical("%s failed!  Status = 0x%x", fn, status);

	// Check for some specific "known" errors that we can more specifically report on
	const char *msg = NULL;
	switch (status)
	{
	    // From CMset_charset
	    case II_ER_CM_NOCHARSET:
		msg = "Unable to set default Ingres character set (CM_NOCHARSET).";
		break;
	    // From ERlangcode
	    case II_ER_ER_BADLANGUAGE:
		msg = "Unable to determine the language code for errors (ER_BADLANGUAGE).";
		break;
	    // From PMinit or PMsetDefault
	    case II_ER_PM_NO_MEMORY:
		msg = "Unable to allocate memory to initialize parameter system (PM_NO_MEMORY).";
		break;
	    // From PMload
	    case II_ER_PM_NO_II_SYSTEM:
		msg = "II_SYSTEM not set in the environment (PM_NO_II_SYSTEM).";
		break;
	    case II_ER_PM_FILE_BAD:
		msg = "Error parsing the 'config.dat' file (PM_FILE_BAD).";
		break;
	    case II_ER_SI_CANT_OPEN:
		msg = "Cannot open the 'config.dat' file (SI_CANT_OPEN).";
		break;
	}

	jint type = II_GE_TOOLSAPI;
	jint code = status;

	jniLogger->error("\t%s", msg);

	//
	// (Using UTF is OK here, because these are all our own program-
	// literal strings in 7-bit ASCII.)
	//
	jstring source = jenv->NewStringUTF(fn);
	if (jenv->ExceptionCheck())
	    return;

	jstring message = jenv->NewStringUTF(msg);
	if (jenv->ExceptionCheck())
	    return;

	jobject exception = jenv->NewObject(CLASSREF(APIException),
					    METHODID(APIException, init),
					    type, code, source, message, NULL);
	if (jenv->ExceptionCheck())
	    return;

	jenv->Throw((jthrowable)exception);

	jenv->DeleteLocalRef(source);
	jenv->DeleteLocalRef(message);
	jenv->DeleteLocalRef(exception);
}


/*{
** Name: throwLastAPIErrors - Translate C-API status into an exception
**
** Description:
**	This helper function translates an API error status into an exception
**	(or chain of exceptions) and throws it.
**
**	It digs down into the C-API DLL to obtain whatever details about the
**	latest errors are available in the II_ERRORS stack.  If an II_ERRORS
**	stack can be obtained, and it is not empty, then it is translated into
**	a chain of IIapi.Exception objects, and the object representing the
**	top of the stack is thrown.  If the II_ERRORS stack is empty, then a
**	generic IIapi.Exception is constructed from the given status code and
**	function name.
**
**	NOTE: The "fn" parameter should be the name of the C-API function, not
**	the Java native function calling this.  The Java native function is
**	implicitly part of the exception stack trace, so adding its name to the
**	message text doesn't provide new information.  What is more useful is
**	the name of the C-API function that failed, and it is only used in the
**	(unlikely) case that the C-API function didn't push anything onto the
**	II_ERRORS stack.
**
**	Call this function ONLY if you actually have an error status to report.
**	(Checking for success here is not helpful, because the typical usage
**	model requires you to test before you call this anyway.)
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The current IIapi instance (for charset conversion).
**	fn		The name of the C-API function that returned an error.
**	status		The TOOLSAPI status code indicating an error.
**	inst		The TOOLSAPI instance pointer so that we can call down
**			into the C-API DLL to obtain the II_ERRORS stack.
**
** Outputs:
**	None.
**
** Returns:
**	void
**
** Side	Effects:
**	Leaves an exception pending in the JVM.  Every exception object created
**	is logged.
**
** History:
**	21-Sep-2010 (dansa01)
**	    Initial coding.
**	19-Nov-2010 (dansa01)
**	    Modified to use the new II_ERRORS struct, and moved it from
**	    IIapi_jni_util.cpp into IIapi_jni.cpp because of the added
**	    CLASSREF.
**	28-Nov-2010 (dansa01)
**	    Clarified the intent of the "fn" parameter, and removed the
**	    redundant test for II_ST_SUCCESS.  Don't call this function unless
**	    you intend to throw.
**	03-Dec-2010 (dansa01)
**	    When throwing the "default" IIapi.Exception, at least make sure
**	    that all the fields are filled in the way they normally should be.
**	16-Dec-2010 (dansa01)
**	    Added "_this" parameter to support Ingres charset conversions.
*/
void
throwLastAPIErrors(JNIEnv *jenv, jobject _this, const char *fn, int status, TOOLSAPI *inst)
{
	if (jenv->EnsureLocalCapacity(3) < 0)
	    return;

	//
	// See if we can get the II_ERRORS information.
	//
	MEfreePtr<II_ERRORS> pErrors(inst, inst->getLastErrors());
	if (!pErrors)
	{
	    jniLogger->critical("ToolsAPI getLastErrors OUT OF MEMORY");
	    JNU_ThrowByName(jenv, OutOfMemoryError, "ToolsAPI getLastErrors OUT OF MEMORY");
	    return;
	}

	//
	// If II_ERRORS is empty, just synthesize an exception using what little
	// information we have.
	//
	if (pErrors->count == 0)
	{
	    jniLogger->error("Error status from %s = %d", fn, status);

	    jint type = II_GE_TOOLSAPI;
	    jint code = status;

	    //
	    // (Using UTF is OK here, because these are all our own program-
	    // literal strings in 7-bit ASCII.)
	    //
	    jstring source = jenv->NewStringUTF(fn);
	    if (jenv->ExceptionCheck())
		return;

	    jstring message = jenv->NewStringUTF("Error status from ToolsAPI C interface.");
	    if (jenv->ExceptionCheck())
		return;

	    jobject exception = jenv->NewObject(CLASSREF(APIException),
						METHODID(APIException, init),
						type, code, source, message, NULL);
	    if (jenv->ExceptionCheck())
		return;

	    jenv->Throw((jthrowable)exception);

	    jenv->DeleteLocalRef(source);
	    jenv->DeleteLocalRef(message);
	    jenv->DeleteLocalRef(exception);

	    return;
	}

	//
	// Otherwise, walk backward through the array of II_ERROR structs, so
	// that we can construct IIapi.Exception objects in the order necessary
	// to chain them via their constructor.
	//
	// Each newly-created IIapi.Exception object becomes the new "top" of
	// the stack, and it points (via its cause field) to the previous top.
	//
	// Once we have the final top exception, throw it.
	//
	jthrowable top = NULL;
	i4 last = pErrors->count - 1;

	for (int i = last; i >= 0; i--)
	{
	    II_ERROR *pe = &pErrors->errors[i];

	    jniLogger->error("IIapi.Exception: type=%d code=%d src='%s' msg='%s'",
			     pe->type, pe->code, pe->source, pe->message);

	    jint type = pe->type;
	    jint code = pe->code;

	    jstring source = getStringFromIngresChars(jenv, _this, pe->source);
	    if (jenv->ExceptionCheck())
		return;

	    jstring message = getStringFromIngresChars(jenv, _this, pe->message);
	    if (jenv->ExceptionCheck())
		return;

	    jobject exception = jenv->NewObject(CLASSREF(APIException),
						METHODID(APIException, init),
						type, code, source, message, top);
	    if (jenv->ExceptionCheck())
		return;

	    // Now that top and the strings are held by the new exception,
	    // we can free the LocalRefs.
	    jenv->DeleteLocalRef(source);
	    jenv->DeleteLocalRef(message);
	    jenv->DeleteLocalRef(top);

	    // The newly-created exception becomes the new top.
	    top = (jthrowable)exception;
	}

	jenv->Throw(top);

	jenv->DeleteLocalRef(top);
}


/*{
** Name: IngresChars::IngresChars - Constructor
**
** Description:
**	Constructs an IngresChars instance that behaves like a "const char *"
**	EOS-terminated string of chars in the Ingres installation charset.
**
**	Defers to the "set" method to do the heavy lifting described here.
**
**	This constructor takes care of the charset conversion from Unicode to
**	the Ingres charset associated with the IIapi instance, which is why we
**	need to pass the iiapi instance pointer	("_this") to the constructor.
**	The heavy lifting is done by the IIapi.encodeIngresChars method.
**
**	NULL jstring input is considered a legal value, and results in a NULL
**	char*, so be sure to call ExceptionCheck (instead of a NULL test)
**	immediately after constructing one of these.
**
**	Simply construct one of these on the stack and then you can forget
**	about cleanup.  The intended usage model is the following:
**
**	    IngresChars szUser(jenv, _this, sUser);
**	    if (jenv->ExceptionCheck())
**	        return;
**	    Then szUser can be used just like any const char * variable.
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The IIapi instance.
**	jstr		The Java string reference.
**
** Side	Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck
**	(and NOT a NULL check) after constructing.
**
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding (based loosely on JNI book example).
**	09-Mar-2011 (whiro01)
**	    Moved the guts into the "set" method.
*/
IngresChars::IngresChars(JNIEnv *jenv, jobject _this, jstring jstr)
:	m_cstr(NULL)
{
	set(jenv, _this, jstr);
}


/*{
** Name: IngresChars::IngresChars - Constructor
**
** Description:
**	Constructs an empty IngresChars instance that must be filled by
**	assignment from a regularly constructed instance.
**
** Inputs:
**	None.
**
** Side	Effects:
**	None.
**
** History:
**	08-Mar-2011 (whiro01)
**	    Initial coding.
*/
IngresChars::IngresChars()
:	m_cstr(NULL)
{
}


/*{
** Name: IngresChars::~IngresChars - Destructor
**
** Description:
**	The destructor frees the allocated Ingres character string owned by
**	this IngresChars instance.
**
** Side	Effects:
**	None.
**
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding.
*/
IngresChars::~IngresChars()
{
	free(m_cstr);
}


/*{
** Name: IngresChars::set - Assign a JNI string value
**
** Description:
**	Fills in an IngresChars instance that behaves like a "const char *"
**	EOS-terminated string of chars in the Ingres installation charset.
**
**	This method takes care of the charset conversion from Unicode to
**	the Ingres charset associated with the IIapi instance, which is why we
**	need to pass the iiapi instance pointer	("_this") to this method.
**
**	NULL jstring input is considered a legal value, and results in a NULL
**	char*, so be sure to call ExceptionCheck (instead of a NULL test)
**	immediately after calling this method.
**
**	This method is intended for use when the variable needs to be available
**	outside the scope of the Java object that will be used to populate it.
**	The intended usage model is the following:
**
**	    IngresChars szUser;
**	    if (some condition) {
**	        szUser.set(jenv, _this, sUser);
**	        if (jenv->ExceptionCheck())
**	            return;
**	    }
**	    Then szUser can be used just like any const char * variable.
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The current IIapi instance (for charset conversion).
**	jstr		The Java string reference.
**
** Side	Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck
**	(and NOT a NULL check) after constructing.
**
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding (based loosely on JNI book example).
**	09-Mar-2011 (whiro01)
**	    Moved from constructor into this method.
**	10-Oct-2011 (dansa01)
**	    Refactored to share the encode function with NativeChars.
*/
void
IngresChars::set(JNIEnv *jenv, jobject _this, jstring jstr)
{
	free(m_cstr);
	m_cstr = encode(jenv, _this, METHODID(LocalIIapi, encodeIngresChars), jstr);
}


/*{
** Name: NativeChars::NativeChars - Constructor
**
** Description:
**	Constructs a NativeChars instance that behaves like a "const char *"
**	EOS-terminated string of native chars.  This constructor takes care of
**	the charset conversion from Unicode to the default native charset
**	associated with the JVM.
**
**	Defers to the "set" method to do the heavy lifting described here.
**
**	NULL jstring input is considered a legal value, and results in a NULL
**	char*, so be sure to call ExceptionCheck (instead of a NULL test)
**	immediately after constructing one of these.
**
**	Simply construct one of these on the stack and then you can forget
**	about cleanup.  The intended usage model is the following:
**
**	    NativeChars szUser(jenv, _this, sUser);
**	    if (jenv->ExceptionCheck())
**	        return;
**	    Then szUser can be used just like any const char * variable.
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The current IIapi instance (for charset conversion).
**	jstr		The Java string reference.
**
** Side	Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck
**	(and NOT a NULL check) after constructing.
**
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding (based loosely on JNI book example).
**	09-Mar-2011 (whiro01)
**	    Moved the guts into the "set" method.
**	10-Oct-2011 (dansa01)
**	    Added "_this" parameter to allow access to IIapi instance methods.
*/
NativeChars::NativeChars(JNIEnv *jenv, jobject _this, jstring jstr)
:	m_cstr(NULL)
{
	set(jenv, _this, jstr);
}


/*{
** Name: NativeChars::NativeChars - Constructor
**
** Description:
**	Constructs an empty NativeChars instance that must be filled by
**	assignment from a regularly constructed instance.
**
** Inputs:
**	None.
**
** Side	Effects:
**	None.
**
** History:
**	08-Mar-2011 (whiro01)
**	    Initial coding.
*/
NativeChars::NativeChars()
:	m_cstr(NULL)
{
}


/*{
** Name: NativeChars::~NativeChars - Destructor
**
** Description:
**	The destructor frees the allocated native character string owned by
**	this NativeChars instance.
**
** Side	Effects:
**	None.
**
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding.
*/
NativeChars::~NativeChars()
{
	free(m_cstr);
}


/*{
** Name: NativeChars::set - Assign a JNI string value
**
** Description:
**	Fills in a NativeChars instance that behaves like a "const char *"
**	EOS-terminated string of native chars.  This method takes care of
**	the charset conversion from Unicode to the default native charset
**	associated with the JVM.
**
**	NULL jstring input is considered a legal value, and results in a NULL
**	char*, so be sure to call ExceptionCheck (instead of a NULL test)
**	immediately after calling this method.
**
**	This method is intended for use when the variable needs to be available
**	outside the scope of the Java object that will be used to populate it.
**	The intended usage model is the following:
**
**	    NativeChars szFile;
**	    if (some condition) {
**	        szFile.set(jenv, _this, sFile);
**	        if (jenv->ExceptionCheck())
**	            return;
**	    }
**	    Then szFile can be used just like any const char * variable.
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The current IIapi instance (for charset conversion).
**	jstr		The Java string reference.
**
** Side	Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck
**	(and NOT a NULL check) after constructing.
**
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding (based loosely on JNI book example).
**	09-Mar-2011 (whiro01)
**	    Moved from constructor into this method.
**	10-Oct-2011 (dansa01)
**	    Added "_this" parameter to allow access to IIapi instance methods,
**	    and refactored to share encode function with IngresChars.
*/
void
NativeChars::set(JNIEnv *jenv, jobject _this, jstring jstr)
{
	free(m_cstr);
	m_cstr = encode(jenv, _this, METHODID(LocalIIapi, encodeNativeChars), jstr);
}


/*{
** Name: getStringFromIngresChars - Create Java String from Ingres char string
**
** Description:
**	This helper function encapsulates the JNI work of charset conversion
**	from the Ingres charset to Unicode when constructing a Java String.
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The current IIapi instance (for charset conversion).
**	str		The char string in the current Ingres charset.
**
** Outputs:
**	None.
**
** Returns:
**	Java String object reference, or NULL if the input string was NULL.
**
** Side	Effects:
**	A local reference is created to the String object that is returned
**	which can be safely ignored if the value is passed back to Java.
**	But, if the value is used internally, "DeleteLocalRef" should be
**	called on the returned String object.
**
**	May leave an exception pending in the JVM.  Always use ExceptionCheck
**	(and NOT a NULL check) after this.
**
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding (based on JNI book example, and some comments
**	    stolen from getString).
**	10-Oct-2011 (dansa01)
**	    Refactored to share the decode function with NativeChars.
*/
jstring
getStringFromIngresChars(JNIEnv *jenv, jobject _this, const char *str)
{
	return decode(jenv, _this, METHODID(LocalIIapi, decodeIngresChars), str);
}


/*{
** Name: getStringFromNativeChars - Create Java String from native char string
**
** Description:
**	This helper function encapsulates the JNI work of charset conversion
**	from the system charset to Unicode when constructing a Java String.
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The current IIapi instance (for charset conversion)
**	str		The char string in the current native charset.
**
** Outputs:
**	None.
**
** Returns:
**	Java String object reference, or NULL if the input string was NULL.
**
** Side	Effects:
**	A local reference is created to the String object that is returned
**	which can be safely ignored if the value is passed back to Java.
**	But, if the value is used internally, "DeleteLocalRef" should be
**	called on the returned String object.
**
**	May leave an exception pending in the JVM.  Always use ExceptionCheck
**	(and NOT a NULL check) after this.
**
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding (based on JNI book example, and some comments
**	    stolen from getString).
**	10-Oct-2011 (dansa01)
**	    Added "_this" parameter to allow access to IIapi instance methods,
**	    and refactored to share decode function with IngresChars.
**/
jstring
getStringFromNativeChars(JNIEnv *jenv, jobject _this, const char *str)
{
	return decode(jenv, _this, METHODID(LocalIIapi, decodeNativeChars), str);
}


/*
** Name: setIngresStringField - Set String field from Ingres char string
**
** Inputs:
**	jenv		The JNI execution environment pointer
**	_this		The current IIapi instance (for charset conversion)
**	jobj		The Java object (structure) in which to set the value
**	fid		The field ID to be set
**	str		The character string value to set into the structure
**
** Outputs:
**	The specified field value is updated with the new character string data.
**
** Returns:
**	None
**
** Side Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck
**	after using this.
**
** History:
**    March 2008 (whiro01)
**	Part of original coding.
**    16-Dec-2010 (dansa01)
**	Renamed from setStringField and added Ingres charset conversion.
*/
void
setIngresStringField(JNIEnv *jenv, jobject _this, jobject jobj, jfieldID fid, const char *str)
{
	jstring jstr = getStringFromIngresChars(jenv, _this, str);
	if (jenv->ExceptionCheck())
	    return;

	jniLogger->trace("Setting String field value to '%s'", (str==NULL)?"<null>":str);
	jenv->SetObjectField(jobj, fid, jstr);
	jenv->DeleteLocalRef(jstr);
}


/*
** Name: setNativeStringField - Set String field from Native char string
**
** Inputs:
**	jenv		The JNI execution environment pointer
**	_this		The current IIapi instance (for charset conversion)
**	jobj		The Java object (structure) in which to set the value
**	fid		The field ID to be set
**	str		The character string value to set into the structure
**
** Outputs:
**	The specified field value is updated with the new character string data.
**
** Returns:
**	None
**
** Side Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck
**	after using this.
**
** History:
**    March 2008 (whiro01)
**	Part of original coding.
**    16-Dec-2010 (dansa01)
**	Renamed from setStringField and added Native charset conversion.
**    10-Oct-2011 (dansa01)
**	Added "_this" parameter to allow access to IIapi instance methods,
*/
void
setNativeStringField(JNIEnv *jenv, jobject _this, jobject jobj, jfieldID fid, const char *str)
{
	jstring jstr = getStringFromNativeChars(jenv, _this, str);
	if (jenv->ExceptionCheck())
	    return;

	jniLogger->trace("Setting String field value to '%s'", (str==NULL)?"<null>":str);
	jenv->SetObjectField(jobj, fid, jstr);
	jenv->DeleteLocalRef(jstr);
}


/*{
** Name: encode - Create a C string from a Java String
**
** Description:
**	Returns a malloc'ed, EOS-terminated string of chars encoded from the
**	given Java String.  The heavy lifting is done by the specified IIapi
**	encodeXxxChars method, which determines the character set to be used.
**
**	NULL jstring input is considered a legal value, and results in a NULL
**	char*, so be sure to use ExceptionCheck (instead of a NULL test)
**	after calling this method.
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The IIapi instance.
**	encodingMethod	The IIapi instance method to use.
**	jstr		The Java String reference.
**
** Returns:
**	A malloc'ed C string representing the encoded Java String, or NULL
**	if the input Java String reference was NULL.
**
** Side	Effects:
**	May leave an exception pending in the JVM.  Always use ExceptionCheck
**	(and NOT a NULL check) after this.
**
** History:
**	10-Oct-2011 (dansa01)
**	    Extracted from IngresChars for sharing with NativeChars now that
**	    both are handled via IIapi methods.
*/
char *
encode(JNIEnv *jenv, jobject _this, jmethodID encodingMethod, jstring jstr)
{
	char *cstr = NULL;

	if (!jstr)
	    return cstr;

	if (jenv->EnsureLocalCapacity(1) < 0)
	    return cstr;

	jbyteArray bytes = static_cast<jbyteArray>(jenv->CallObjectMethod(_this, encodingMethod, jstr));
	if (jenv->ExceptionCheck())
	    return cstr;

	jsize len = jenv->GetArrayLength(bytes);

	cstr = reinterpret_cast<char *>(malloc(len + 1));
	if (!cstr)
	{
	    JNU_ThrowByName(jenv, OutOfMemoryError, 0);
	}
	else
	{
	    jenv->GetByteArrayRegion(bytes, 0, len, (jbyte *)cstr);
	    cstr[len] = 0; //EOS
	}

	jenv->DeleteLocalRef(bytes);

	return cstr;
}


/*{
** Name: decode - Create a Java String from a C string
**
** Description:
**	Returns a Java String decoded from the given C string.  The heavy
**	lifting is done by the specified IIapi decodeXxxChars method, which
**	determines the character set to be used.
**
**	NULL str input is considered a legal value, and results in a NULL
**	Java String reference, so be sure to use ExceptionCheck (instead of
**	a NULL test) after calling this method.
**
** Inputs:
**	jenv		The JNIEnv.
**	_this		The IIapi instance.
**	decodingMethod	The IIapi instance method to use.
**	str		The char string.
**
** Outputs:
**	None.
**
** Returns:
**	Java String object reference, or NULL if the input string was NULL.
**
** Side	Effects:
**	A local reference is created to the String object that is returned
**	which can be safely ignored if the value is passed back to Java.
**	But, if the value is used internally, "DeleteLocalRef" should be
**	called on the returned String object.
**
**	May leave an exception pending in the JVM.  Always use ExceptionCheck
**	(and NOT a NULL check) after this.
**
** History:
**	10-Oct-2011 (dansa01)
**	    Extracted from getStringFromIngresChars to be shared with Native
**	    charset conversions now that both are handled via IIapi methods.
*/
jstring
decode(JNIEnv *jenv, jobject _this, jmethodID decodingMethod, const char *str)
{
	jstring jstr = NULL;
	if (str)
	{
	    if (jenv->EnsureLocalCapacity(2) < 0)
		return  jstr;

	    jsize len = strlen(str);

	    jbyteArray bytes = jenv->NewByteArray(len);
	    if (bytes)
	    {
		jenv->SetByteArrayRegion(bytes, 0, len, (jbyte *)str);
		jstr = static_cast<jstring>(jenv->CallObjectMethod(_this, decodingMethod, bytes));
		jenv->DeleteLocalRef(bytes);
	    }
	}

	return jstr;
}


/*
** Name: createObject - Create a Java object of the given class and run its
**	default constructor method.
**
** Inputs:
**	jenv		The JNI execution environment pointer
**	_class		Class of the object to be created
**	init		Method ID of the default constructor method ("<init>")
**
** Outputs:
**	None
**
** Returns:
**	Java object pointer if successful, NULL if an exception occurred.
**
**	(Throws a NullPointerException if the class reference is NULL, or the
**	init method ID is zero.)
**
** Side Effects:
**	The default constructor is executed.
**
** History:
**    Dec 2009 (whiro01)
**	Part of first expansion of functionality.
**    23-Nov-2010 (dansa01)
**	Reworked to ensure that this will now always exit in a well-defined
**	state.  Now it will never return an uninitialized object, and if the
**	return value is NULL, you can be sure that an exception HAS been thrown.
*/
jobject
createObject(JNIEnv *jenv, jclass _class, jmethodID init)
{
	if (_class == NULL)
	{
	    JNU_ThrowByName(jenv, NullPointerException, "createObject: Null class reference.");
	    return NULL;
	}

	if (init == 0)
	{
	    JNU_ThrowByName(jenv, NullPointerException, "createObject: Bad method ID.");
	    return NULL;
	}

	return jenv->NewObject(_class, init);
}


/*
** Name: createVersionInfo - Create a VersionInfo object from the given
**	version string.
**
** Inputs:
**	jenv		The JNI execution environment pointer
**	_this		The current IIapi instance (for charset conversion)
**	version		Version string to be parsed
**
** Outputs:
**	None
**
** Returns:
**	Java object pointer if successful, NULL if failure.
**
** Side Effects:
**	Same as "createObject"
**
** History:
**    02-Jun-2010 (whiro01)
**	Make sure all demo code works.
**    22-Jun-2011 (dansa01)
**	Use the new VersionInfo constructor to do the parsing.  That way we do all
**	version string parsing only one way, in only one place.
*/
jobject
createVersionInfo(JNIEnv *jenv, jobject _this, const char *version)
{
	jobject vInfo = NULL;

	jstring jVersionString = getStringFromNativeChars(jenv, _this, version);
	if (jenv->ExceptionCheck())
	    return vInfo;

	vInfo = jenv->NewObject(CLASSREF(VersionInfo), METHODID(VersionInfo, init), jVersionString);
	jenv->DeleteLocalRef(jVersionString);

	return vInfo;
}


/*
** Name: convertJavaDataObject - Convert a given Ingres data value to a Java data object.
**	Makes use of "IIapi_convertData" to do the conversion and "getJavaDataObject" to
**	create the final object.  The destination data type should be chosed to be compatible
**	with the desired Java object class.
**	Note: this is usually a recursive call from "getJavaDataObject" for Ingres data types
**	that are not directly Java-compatible.
**
** Inputs:
**	jenv		The JNI execution environment pointer
**	_this		The current IIapi instance (for charset conversion)
**	inst		Pointer to the loaded Ingres installation
**	dval		Pointer to input data value
**	desc		Pointer to descriptor of the input data
**	dstType		Type of data to convert the input to before creating Java object
**
** Outputs:
**	None
**
** Returns:
**	Java object pointer if successful, or NULL if the datavalue was null or some
**	other error (including allocation of Java object) occurs.
**
** History:
**    March 2008 (whiro01)
**	Part of original coding.
**    16-Dec-2010 (dansa01)
**	Added "_this" parameter to support Ingres charset conversions.
*/
jobject
convertJavaDataObject(JNIEnv *jenv, jobject _this, TOOLSAPI *inst, II_DESCRIPTOR *desc, II_DATAVALUE *dval, i2 dstType)
{
	jobject jobj = NULL;

	II_DESCRIPTOR dstDesc;
	II_DATAVALUE dstData;

	/* Destination descriptor is a new type and new buffer, but otherwise same as source */
	dstDesc.dataType   = dstType;
	dstDesc.isNullable = desc->isNullable;
	dstDesc.length     = desc->length;
	dstDesc.precision  = desc->precision;
	dstDesc.scale      = desc->scale;
	dstDesc.columnType = desc->columnType;
	dstDesc.columnName = desc->columnName;

	dstData.isNull = dval->isNull;
	dstData.length = dval->length;
	dstData.data   = malloc(dstData.length + 1);
	if (!dstData.data)
	{
	    JNU_ThrowByName(jenv, OutOfMemoryError, NULL);
	    return jobj;
	}

	i4 status = inst->IIconvertData(desc, dval, &dstDesc, &dstData);
	if (status != II_ST_SUCCESS)
	{
	    throwLastAPIErrors(jenv, _this, "IIconvertData", status, inst);
	}
	else
	{
	    jobj = getJavaDataObject(jenv, _this, inst, &dstDesc, &dstData);
	}
	free(dstData.data);

	return jobj;
}


/*
** Name: getJavaDataObject - Given an Ingres data value and its descriptor, construct a
**	compatible Java data object out of it.
**	Note: can be recursively called for some data types that need to be converted
**	to a different type by calling "convertJavaDataObject".
**
** Inputs:
**	jenv		The JNI execution environment pointer
**	_this		The current IIapi instance (for charset conversion)
**	inst		Pointer to the loaded Ingres installation
**	desc		Pointer to descriptor of the input data
**	dval		Pointer to input data value
**
** Outputs:
**	None
**
** Returns:
**	Java object pointer if successful, or NULL if the datavalue was null or some
**	other error (including allocation of Java object) occurs.
**
** Side Effects:
**	Creates a local reference to the Java object returned.  If only used
**	internally, the object should be free'd with "DeleteLocalRef".
**
** Exceptions:
**	Can throw an IIapi_exception for cases of bad data types or sizes.
**
** History:
**    March 2008 (whiro01)
**	Part of original coding.
**    16-Dec-2010 (dansa01)
**	Added "_this" parameter to support Ingres charset conversions.
*/
jobject
getJavaDataObject(JNIEnv *jenv, jobject _this, TOOLSAPI *inst, II_DESCRIPTOR *desc, II_DATAVALUE *dval)
{
	jobject jobj = NULL;
	u_i2 len = dval->length;
	char *val = reinterpret_cast<char *>(dval->data);	/* Had +1 length (see allocation in "_II_DATAVALUE") */

	jniLogger->trace("getJavaDataObject: dataType = %d", desc->dataType);

	/* Null values will result in a "null" object in Java */
	if (dval->isNull)
		return jobj;

	switch (desc->dataType)
	{
	    case II_CHR_TYPE     :	/* Text Fixed */
	    case II_CHA_TYPE	    :	/* Character Fixed */
		val[len] = '\0';
		jobj = getStringFromIngresChars(jenv, _this, val);
		break;

	    case II_VCH_TYPE	    :	/* Character Variable */
		/* 2 byte length followed by data */
		memcpy(&len, val, sizeof(u_i2));
		val[len+sizeof(u_i2)] = '\0';
		jniLogger->trace("II_VCH_TYPE: len = %d", len);
		jobj = getStringFromIngresChars(jenv, _this, &val[sizeof(u_i2)]);
		break;

	    case II_LVCH_TYPE    :	/* Character Long */
#ifdef II_LCLOC_TYPE
	    case II_LCLOC_TYPE   :	/* Character Long Locator */
#endif
	    case II_NCHA_TYPE    :	/* Unicode Fixed */
	    case II_NVCH_TYPE    :	/* Unicode Variable */
	    case II_LNVCH_TYPE   :	/* Unicode Long */
#ifdef II_LNLOC_TYPE
	    case II_LNLOC_TYPE   :	/* Unicode Long Locator */
#endif
	    case II_TXT_TYPE     :	/* Text Variable */
	    case II_LTXT_TYPE    :	/* Text Var (typeless NULL) */
		/* TODO: look up the implementation of the data and do the right thing */
		break;

	    case II_BYTE_TYPE    :	/* Binary Fixed */
	    case II_VBYTE_TYPE   :	/* Binary Variable */
	    case II_LBYTE_TYPE   :	/* Binary Long */
#ifdef II_LBLOC_TYPE
	    case II_LBLOC_TYPE   :	/* Binary Long Locator */
#endif
		/* TODO: look up the implementation of the data and do the right thing */
		break;

	    case II_INT_TYPE     :	/* Integer */
		if (len == sizeof(i1)) {
		    // Create Java Byte object using Byte.valueOf(byte)
		    jobj = jenv->CallStaticObjectMethod(CLASSREF(Byte), METHODID(Byte, valueOf), *((jbyte *)val));
		}
		else if (len == sizeof(i2)) {
		    // Create Java Short object using Short.valueOf(short)
		    jobj = jenv->CallStaticObjectMethod(CLASSREF(Short), METHODID(Short, valueOf), *((jshort *)val));
		}
		else if (len == sizeof(i4)) {
		    // Create Java Integer object using Integer.valueOf(int)
		    jobj = jenv->CallStaticObjectMethod(CLASSREF(Integer), METHODID(Integer, valueOf), *((jint *)val));
		}
		else if (len == sizeof(i8)) {
		    // Create Java Long object using Long.valueOf(long)
		    jobj = jenv->CallStaticObjectMethod(CLASSREF(Long), METHODID(Long, valueOf), *((jlong *)val));
		}
		else {
		    // Throw an exception about unknown data size
		    JNU_ThrowByName(jenv, IllegalArgumentException, "Unknown integer data size");
		}
		break;

	    case II_MNY_TYPE     :	/* Money */
	    case II_DEC_TYPE	    :	/* Decimal */
		/* Convert these numeric types into floating-point */
		jobj = convertJavaDataObject(jenv, _this, inst, desc, dval, II_FLT_TYPE);
		break;

	    case II_FLT_TYPE     :	/* Floating Point */
		if (len == sizeof(f4)) {
		    // Create Java Float object using Float.valueOf(float)
		    jobj = jenv->CallStaticObjectMethod(CLASSREF(Float), METHODID(Float, valueOf), *((jfloat *)val));
		}
		else if (len == sizeof(f8)) {
		    // Create Java Double object using Double.valueOf(double)
		    jobj = jenv->CallStaticObjectMethod(CLASSREF(Double), METHODID(Double, valueOf), *((jdouble *)val));
		}
		else {
		    // Throw an exception about unknown data size
		    JNU_ThrowByName(jenv, IllegalArgumentException, "Unknown floating-point data size");
		}
		break;

	    case II_DTE_TYPE     :	/* Ingres Date */
	    case II_DATE_TYPE    :	/* ANSI Date */
	    case II_TIME_TYPE    :	/* Ingres Time */
	    case II_TMWO_TYPE    :	/* Time without Timezone */
	    case II_TMTZ_TYPE    :	/* Time with Timezone */
	    case II_TS_TYPE      :	/* Ingres Timestamp */
	    case II_TSWO_TYPE    :	/* Timestamp without Timezone */
	    case II_TSTZ_TYPE    :	/* Timestamp with Timezone */
	    case II_INTYM_TYPE   :	/* Interval Year to Month */
	    case II_INTDS_TYPE   :	/* Interval Day to Second */
		/* Arrange to convert all these "strange" types into character data */
		jobj = convertJavaDataObject(jenv, _this, inst, desc, dval, II_VCH_TYPE);
		break;

	    case II_LOGKEY_TYPE  :	/* Logical Key */
	    case II_TABKEY_TYPE  :	/* Table Key */
		/* TODO: look up the implementation of the data and do the right thing */
		break;

	    default:
		char *msg = JNU_sprintf(jenv, "Unknown data type %d", desc->dataType);
		if (msg != NULL)
		{
		    JNU_ThrowByName(jenv, IllegalArgumentException, msg);
		    free(msg);
		}
		break;
	}
	return jobj;
}


/*{
** Name: ungetJavaDataObject - Convert a Java object into a C II_DATAVALUE
**
** Description:
**	The somewhat silly name of this helper function is meant to reflect the
**	fact that this does essentially the reverse of getJavaDataObject.  It
**	converts the given Java object into an II_DATAVALUE consistent with the
**	given II_DESCRIPTOR, if possible.
**
** Inputs:
**	jenv		The JNI execution environment pointer.
**	_this		The current IIapi instance (for charset conversion).
**	inst		Pointer to the loaded Ingres installation.
**	jValue		The Java object to be converted.
**	pDstDesc	Pointer to the destination descriptor specifying the
**			desired output type.
**	pDstData	Pointer to the destination data struct to be filled in
**			with the converted value.  It is ASSUMED that this
**			struct already points to a data buffer of the length
**			indicated by the descriptor.
**
** Outputs:
**	*pDstData	Filled in with C data.
**
** Returns:
**	None.
**
** Side	Effects:
**	Can leave an exception pending in the JVM.
**
** History:
**	11-Feb-2011 (dansa01)
**	    Initial coding.
*/
void
ungetJavaDataObject(JNIEnv *jenv, jobject _this, TOOLSAPI *inst, jobject jValue, II_DESCRIPTOR *pDstDesc, II_DATAVALUE *pDstData)
{
	//
	// First we create a temporary source Desc/Data structure pair that
	// reflects the actual Java data value we received.  Then we let OpenAPI
	// (attempt to) convert that to the specified output type.
	//
	// (Using an initial default-sized buffer allows us to eliminate lots of
	// getNewBuffer and ExceptionCheck calls in the scalar cases below.  And
	// since it will cover most typical string lengths, reallocation should
	// rarely be necessary.)
	//
	II_DESCRIPTOR srcDesc = {0};
	II_DATAVALUE  srcData = {0};
	BufferList bufferList;
	u_i2 defaultBufferSize = 32;

	srcData.data = bufferList.getNewBuffer(jenv, defaultBufferSize);
	if (jenv->ExceptionCheck())
	    return;

	if (!jValue)
	{
	    //
	    // We have a null value.  IIconvertData doesn't allow null values,
	    // so handle this as a special case.  All we need to do is set the
	    // destination Data struct directly, and we're done.
	    //
	    if (pDstDesc->isNullable)
	    {
		pDstData->isNull = TRUE;
		pDstData->length = 0;
	    }
	    else
	    {
		JNU_ThrowByName(jenv, IllegalArgumentException, "Null value supplied for non-nullable data type.");
	    }
	    return;
	}
	else if (jenv->IsInstanceOf(jValue, CLASSREF(Byte)))
	{
	    srcDesc.dataType = II_INT_TYPE;
	    srcDesc.length = sizeof(i1);
	    srcData.length = sizeof(i1);
	    *reinterpret_cast<i1 *>(srcData.data) = jenv->CallByteMethod(jValue, METHODID(Byte, byteValue));
	}
	else if (jenv->IsInstanceOf(jValue, CLASSREF(Short)))
	{
	    srcDesc.dataType = II_INT_TYPE;
	    srcDesc.length = sizeof(i2);
	    srcData.length = sizeof(i2);
	    *reinterpret_cast<i2 *>(srcData.data) = jenv->CallShortMethod(jValue, METHODID(Short, shortValue));
	}
	else if (jenv->IsInstanceOf(jValue, CLASSREF(Integer)))
	{
	    srcDesc.dataType = II_INT_TYPE;
	    srcDesc.length = sizeof(i4);
	    srcData.length = sizeof(i4);
	    *reinterpret_cast<i4 *>(srcData.data) = jenv->CallIntMethod(jValue, METHODID(Integer, intValue));
	}
	else if (jenv->IsInstanceOf(jValue, CLASSREF(Long)))
	{
	    srcDesc.dataType = II_INT_TYPE;
	    srcDesc.length = sizeof(i8);
	    srcData.length = sizeof(i8);
	    *reinterpret_cast<i8 *>(srcData.data) = jenv->CallLongMethod(jValue, METHODID(Long, longValue));
	}
	else if (jenv->IsInstanceOf(jValue, CLASSREF(Float)))
	{
	    srcDesc.dataType = II_FLT_TYPE;
	    srcDesc.length = sizeof(f4);
	    srcData.length = sizeof(f4);
	    *reinterpret_cast<f4 *>(srcData.data) = jenv->CallFloatMethod(jValue, METHODID(Float, floatValue));
	}
	else if (jenv->IsInstanceOf(jValue, CLASSREF(Double)))
	{
	    srcDesc.dataType = II_FLT_TYPE;
	    srcDesc.length = sizeof(f8);
	    srcData.length = sizeof(f8);
	    *reinterpret_cast<f8 *>(srcData.data) = jenv->CallDoubleMethod(jValue, METHODID(Double, doubleValue));
	}
	else if (jenv->IsInstanceOf(jValue, CLASSREF(String)))
	{
	    IngresChars szValue(jenv, _this, (jstring)jValue);
	    if (jenv->ExceptionCheck())
		return;

	    u_i2 charCount = (u_i2)strlen(szValue);
	    if (charCount > defaultBufferSize)
	    {
		srcData.data = bufferList.getNewBuffer(jenv, charCount); // NO EOS
		if (jenv->ExceptionCheck())
		    return;
	    }

	    srcDesc.dataType = II_CHA_TYPE;
	    srcDesc.length = charCount;
	    srcData.length = charCount;
	    strncpy(reinterpret_cast<char *>(srcData.data), szValue, charCount);
	}
	else
	{
	    JNU_ThrowByName(jenv, IllegalArgumentException, "Unsupported type of DB procedure parameter object.");
	}

	// (Consolidated ExceptionCheck covers all those CallxxxMethod invocations above.)
	if (jenv->ExceptionCheck())
	    return;

	i4 status = inst->IIconvertData(&srcDesc, &srcData, pDstDesc, pDstData);
	if (status != II_ST_SUCCESS)
	{
	    //
	    // TODO: There are endless opportunities to make the error reports
	    // about DB procedure parameters more detailed and more descriptive.
	    // But that will be a lot of work, so for now it remains on the
	    // future improvements list.
	    //
	    throwLastAPIErrors(jenv, _this, "IIconvertData", status, inst);
	}
}


/*{
** Name: makeCAPIdescVdataV - Convert Java arrays to equivalent C-API structs
**
** Description:
**	This function takes a parallel pair of Descriptor[] and DataValue[] Java
**	arrays and produces an equivalent pair of II_DESCRIPTORS, II_DATAVALUES
**	structures needed for calling the C-API (as with IIexecuteProcedure or
**	IIputParameters).
**
** Inputs:
**	jenv		The JNI execution environment pointer.
**	_this		The current IIapi instance (for charset conversion).
**	inst		Pointer to the loaded Ingres installation.
**	buffers		REFERENCE to a BufferList object (declared within the
**			caller's local scope).  All allocations made within this
**			function are done using the BufferList, so that they
**			will persist after this function returns (for use by
**			our caller), and they will be automatically freed when
**			the caller's local BufferList object destructs.
**	jDescV		The Java parameter Descriptor array.
**	jDataV		The Java parameter DataValue array.
**	ppDescV		Pointer to a location to receive the address of the new
**			II_DESCRIPTORS structure.  (The structure allocation
**			will be freed automatically by the BufferList.)
**	ppDataV		Pointer to a location to receive the address of the new
**			II_DATAVALUES structure.  (The structure allocation
**			will be freed automatically by the BufferList.)
**
** Outputs:
**	*ppDescV	Filled in with address of the new II_DESCRIPTORS struct.
**	*ppDataV	Filled in with address of the new II_DATAVALUES struct.
**
** Returns:
**	None.
**
** Side	Effects:
**	Can leave an exception pending in the JVM.
**
** History:
**	19-Apr-2011 (dansa01)
**	    Initial coding.
*/
void
makeCAPIdescVdataV(JNIEnv *jenv, jobject _this, TOOLSAPI *inst, BufferList& buffers, jobjectArray jDescV, jobjectArray jDataV, II_DESCRIPTORS **ppDescV, II_DATAVALUES **ppDataV)
{
	if (!jDescV)
	{
	    JNU_ThrowByName(jenv, NullPointerException, "Null Descriptor array reference.");
	    return;
	}

	if (!jDataV)
	{
	    JNU_ThrowByName(jenv, NullPointerException, "Null DataValue array reference.");
	    return;
	}

	int ncols = jenv->GetArrayLength(jDescV);
	int ndatacols = jenv->GetArrayLength(jDataV);
	if (ncols != ndatacols)
	{
	    JNU_ThrowByName(jenv, IllegalArgumentException, "Descriptor array length is not the same as DataValue array length.");
	    return;
	}

	if (ncols == 0)
	{
	    JNU_ThrowByName(jenv, IllegalArgumentException, "Descriptor array is empty.");
	    return;
	}

	size_t bytesNeeded = sizeof(II_DESCRIPTORS) + sizeof(II_DESCRIPTOR) * (ncols > 0 ? ncols - 1 : 0);
	II_DESCRIPTORS *pDescV = (II_DESCRIPTORS *)buffers.getNewBuffer(jenv, bytesNeeded);
	if (jenv->ExceptionCheck())
	    return;

	bytesNeeded = sizeof(II_DATAVALUES) + sizeof(II_DATAVALUE) * (ncols > 0 ? ncols - 1 : 0);
	II_DATAVALUES *pDataV = (II_DATAVALUES *)buffers.getNewBuffer(jenv, bytesNeeded);
	if (jenv->ExceptionCheck())
	    return;

	pDescV->count = ncols;
	pDataV->count = ncols;
	for (int i = 0; i < ncols; i++)
	{
	    jobject jDesc = jenv->GetObjectArrayElement(jDescV, i);
	    if (jenv->ExceptionCheck())
		return;

	    if (!jDesc)
	    {
		JNU_ThrowByName(jenv, NullPointerException, "Null parameter Descriptor object.");
		return;
	    }

	    jobject jData = jenv->GetObjectArrayElement(jDataV, i);
	    if (jenv->ExceptionCheck())
		return;

	    if (!jData)
	    {
		JNU_ThrowByName(jenv, NullPointerException, "Null parameter DataValue object.");
		return;
	    }

	    //
	    // Convert the next Java descriptor element into a corresponding C II_DESCRIPTOR element.
	    //
	    pDescV->descriptors[i].dataType   = jenv->GetShortField(jDesc, FIELDID(Descriptor, dataType));
	    pDescV->descriptors[i].isNullable = jenv->GetBooleanField(jDesc, FIELDID(Descriptor, nullable));
	    pDescV->descriptors[i].length     = jenv->GetShortField(jDesc, FIELDID(Descriptor, length));
	    pDescV->descriptors[i].precision  = jenv->GetShortField(jDesc, FIELDID(Descriptor, precision));
	    pDescV->descriptors[i].scale      = jenv->GetShortField(jDesc, FIELDID(Descriptor, scale));
	    pDescV->descriptors[i].columnType = jenv->GetShortField(jDesc, FIELDID(Descriptor, columnType));
	    pDescV->descriptors[i].columnName = NULL;

	    jstring jColumnName = (jstring)jenv->GetObjectField(jDesc, FIELDID(Descriptor, columnName));
	    if (jColumnName)
	    {
		//
		// In this context, ColumnName represents a DBproc parameter name, so it is needed.
		//
		IngresChars szColumnName(jenv, _this, jColumnName);
		if (jenv->ExceptionCheck())
		    return;

		char *columnNameBuf = (char *)buffers.getNewBuffer(jenv, strlen(szColumnName) + 1);
		if (jenv->ExceptionCheck())
		    return;

		strcpy(columnNameBuf, szColumnName);
		pDescV->descriptors[i].columnName = columnNameBuf;

		jenv->DeleteLocalRef(jColumnName);
	    }

	    //
	    // Convert the next Java value element into a corresponding C II_DATAVALUE element.
	    //
	    pDataV->values[i].data = buffers.getNewBuffer(jenv, pDescV->descriptors[i].length);
	    if (jenv->ExceptionCheck())
		return;

	    jobject jValue = jenv->GetObjectField(jData, FIELDID(DataValue, value));

	    ungetJavaDataObject(jenv, _this, inst, jValue, &pDescV->descriptors[i], &pDataV->values[i]);
	    if (jenv->ExceptionCheck())
		return;

	    jenv->DeleteLocalRef(jValue);
	    jenv->DeleteLocalRef(jDesc);
	    jenv->DeleteLocalRef(jData);
	}

	//
	// The BufferList will free the new structures automatically after the
	// caller is done with them, but we still need to tell our caller where
	// to find them.
	//
	*ppDescV = pDescV;
	*ppDataV = pDataV;
}


/*{
** Name: setHandleValue - Sets handle value in any HandleBase-derived object
**
** Description:
**	This helper function sets a given handle value into any type of handle
**	object (i.e., any object that extends HandleBase).
**
**	The handleObj input must be a HandleBase-derived object, or else the
**	result is undefined.
**
** Inputs:
**	jenv		The JNI execution environment pointer.
**	handleObj	The handle object reference.
**	handleVal	The value to be set in that handle object.
**
** Outputs:
**	None.
**
** Returns:
**	None.
**
** Side	Effects:
**	Can leave an exception pending in the JVM.
**
** Exceptions:
**	Will throw NullPointerException if handleObj is NULL.  Always call
**	ExceptionCheck after calling this function.
**
** History:
**	04-Oct-2010 (dansa01)
**	    Initial coding.
*/
void
setHandleValue(JNIEnv *jenv, jobject handleObj, II_HANDLE handleVal)
{
	if (handleObj)
	{
	    jenv->SetLongField(handleObj, FIELDID(HandleBase, handleValue), (jlong)handleVal);
	}
	else
	{
	    JNU_ThrowByName(jenv, NullPointerException, "Null handle reference.");
	}
}


/*{
** Name: getHandleValue - Gets handle value from any HandleBase-derived object
**
** Description:
**	This helper function extracts the handle value from any type of handle
**	object (i.e., any object that extends HandleBase).
**
**	The handleObj input must be a HandleBase-derived object, or else the
**	result is undefined.
**
** Inputs:
**	jenv		The JNI execution environment pointer.
**	handleObj	The handle object reference (can be null).
**
** Outputs:
**	None.
**
** Returns:
**	The extracted handle value or NULL if object reference is null.
**
** Side	Effects:
**	None.
**
** Exceptions:
**	None.
**
** History:
**	04-Oct-2010 (dansa01)
**	    Initial coding.
**	06-Dec-2010 (dansa01)
**	    The PM functions allow null handles, so we can't treat that as an
**	    error.  If the handle object is null, the handle value is NULL.
*/
II_HANDLE
getHandleValue(JNIEnv *jenv, jobject handleObj)
{
	if (handleObj)
	{
	    return (II_HANDLE)jenv->GetLongField(handleObj, FIELDID(HandleBase, handleValue));
	}
	else
	{
	    return NULL;
	}
}


/*{
** Name: newHandleObj - Creates a new HandleBase-derived object
**
** Description:
**	This helper function creates a handle object instance and initializes
**	it with the give handle value.
**
**	The handleClass input must be a HandleBase-derived type, or else the
**	result is undefined.
**
** Inputs:
**	jenv		The JNI execution environment pointer.
**	handleClass	A concrete HandleBase-derived class reference.
**	init		The MethodID of the default constructor for that class.
**	handleVal	The handle value to set in the new object.
**
** Outputs:
**	None.
**
** Returns:
**	The constructed object reference (NULL if exception was thrown).
**
** Side	Effects:
**	Can leave an exception pending in the JVM.
**
** Exceptions:
**	May throw OutOfMemoryError.  A NULL returned from this function means
**	an exception has been thrown.
**
** History:
**	04-Oct-2010 (dansa01)
**	    Initial coding.
*/
jobject
newHandleObj(JNIEnv *jenv, jclass handleClass, jmethodID init, II_HANDLE handleVal)
{
	jobject handleObj = createObject(jenv, handleClass, init);
	if (handleObj)
	{
	    setHandleValue(jenv, handleObj, handleVal);
	}

	return handleObj;
}


/*{
** Name: BufferList::BufferList - Constructor
**
** Description:
**	Constructs a BufferList instance that keeps track of
**	dynamically allocated buffers for later cleanup.
**
** Side	Effects:
**	None.
**
** History:
**	11-Feb-2011 (dansa01)
**	    Initial coding.
*/
BufferList::BufferList()
:	m_listHead(NULL)
{
}


/*{
** Name: BufferList::~BufferList - Destructor
**
** Description:
**	The destructor walks the list and deletes all the allocated
**	nodes.
**
** Side	Effects:
**	None.
**
** History:
**	11-Feb-2011 (dansa01)
**	    Initial coding.
*/
BufferList::~BufferList()
{
	BufferListNode *pCurrent = m_listHead;
	while (pCurrent)
	{
	    BufferListNode *pNext = pCurrent->m_next;
	    free(pCurrent);
	    pCurrent = pNext;
	}
}


/*{
** Name: BufferList::getNewBuffer - Allocate a buffer and track it for cleanup
**
** Description:
**	This method allocates a new buffer of the requested size, and adds
**	it to the list of objects to be cleaned up by the destructor.
**
** Inputs:
**	jenv		The JNIEnv (for throwing OutOfMemoryError).
**	bytesNeeded	The size of buffer required.
**
** Returns:
**	Pointer to an allocated, aligned, buffer of the requested size.
**
** Side	Effects:
**	May leave an exception pending in the JVM.  Always call ExceptionCheck.
**
** History:
**	11-Feb-2011 (dansa01)
**	    Initial coding.
*/
void *
BufferList::getNewBuffer(JNIEnv *jenv, size_t bytesNeeded)
{
	BufferListNode *pNode = NULL;

	pNode = (BufferListNode *)malloc(sizeof(BufferListNode) + bytesNeeded);
	if (!pNode)
	{
	    JNU_ThrowByName(jenv, OutOfMemoryError, "ToolsAPI BufferList OUT OF MEMORY");
	    return NULL;
	}

	pNode->m_next = m_listHead;
	m_listHead = pNode;

	//
	// The first sizeof(BufferListNode) bytes belong to us for linking
	// the blocks.  What follows that is buffer space for our caller.
	//
	return ((char *)pNode + sizeof(BufferListNode));
}


