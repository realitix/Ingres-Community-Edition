/*
** Copyright 2008-2011 Actian Corporation. All rights reserved
**
** Description:
**	Header file defining the utility functions in IIapi_jni_util
**
** History:
**    March 2008 (whiro01)
**	Initial prototype version.
**    07-Dec-2009 (whiro01)
**	Added header comments.
**    08-Dec-2009 (whiro01)
**	Added more stuff from the incipient Tools API spec.
**    04-Jan-2010 (whiro01)
**	Added new "sprintf" wrapper that allocates destination memory.
**    05-Jan-2010 (whiro01)
**	Moved field ID / method ID definitions from C code into here
**	so they can be shared among modules.
**    03-Feb-2010 (whiro01)
**	Added FLD_VERSIONINFO and processing for it.
**    20-Apr-2010 (whiro01)
**	Not using LO functions directly (must use C++ API for the
**	specific instance).
**    14-Jun-2010 (whiro01)
**	Ditto for all other Ingres functions.
**    17-Jun-2010 (whiro01)
**	Made JNU_GetStringUTFChars into an inline wrapper for
**	the simple jenv->GetStringUTFChars(); then changed all
**	"free(str)" into JNU_FreeStringUTFChars() which just
**	calls jenv->ReleaseStringUTFChars();
**    22-Jun-2010 (whiro01)
**	Part of rework of the Tools API: rename ToolsAPI to
**	ToolsInterface.
**    21-Sep-2010 (dansa01)
**	Added helper classes (StringUTFChars, MEfreePtr) to handle cleanup
**	bookkeeping for us and simplify error return paths in JNI code.  Also
**	added declaration for JNU_ThrowLastApiError helper function.
**    28-Sep-2010 (dansa01)
**	Fixed a typo in the MEfreePtr template (revealed by OSX build).
**    04-Oct-2010 (dansa01)
**	Added externs for other frequently-used exception class name strings
**	(following the example of IIapi_exception).
**    14-Oct-2010 (dansa01)
**	Added bool and pointer conversion operators to MEfreePtr template.
**    15-Oct-2010 (dansa01)
**	Removed the bool conversion operator from the MEfreePtr template
**	because it caused ambiguity on older compilers.
**    25-Oct-2010 (whiro01)
**	Parse platform identifier out of release string and put into VersionInfo.
**    01-Nov-2010 (dansa01)
**	Adapted to new JNI-like TOOLSAPI interface (renamed from ToolsInterface).
**    08-Dec-2010 (dansa01)
**	Removed IIapi_exception variable to minimize temptation to throw
**	IIapi.Exception by name.  We should now always use the constructor
**	that allows us to fill in the type, code, and source fields.
**	Added IllegalStateException.
**    09-Dec-2010 (whiro01)
**	Put some of the mutex stuff that used to only be in the handle code
**	(now obsolete) back in here so we can use it elsewhere in the JNI layer.
**    14-Dec-2010 (dansa01)
**	Added externs for cached classrefs, fieldIDs, and methodIDs, now that
**	those have all been consolidated into IIapi_jni_util.cpp.  Also added
**	externs for helper functions that were moved from IIapi_jni.cpp into
**	IIapi_jni_util.cpp.
**    15-Dec-2010 (dansa01)
**	Added charset conversion tools.
**    16-Dec-2010 (dansa01)
**	Split string-handling utility functions into two flavors (for Ingres and
**	Native charsets).  Had to add "_this" parameter to the Ingres flavors to
**	access the IIapi methods that do the actual charset conversions.
**    20-Dec-2010 (dansa01)
**	Added a MallocFreePtr template so that we can rely on the compiler to
**	take care of the free() for any malloc() we do.  (This is analogous to
**	the MEfreePtr template but suitable for allocations made outside of the
**	toolsapi DLL.)
**    08-Feb-2011 (dansa01)
**	Added fieldID cache for ConnectOptions type.
**    11-Feb-2011 (dansa01)
**	Added BufferList class and ungetJavaDataObject function.  Added new
**	methodIDs for extracting scalar values from Byte, Short, Integer, etc.
**	Java objects.
**    02-Mar-2011 (whiro01)
**	Added CreateDBOptions to the class and method lists; added prototype for
**	"throwStartupError" helper method to assist in reporting errors before the
**	error stack is setup; added set() method to IngresChars and NativeChars.
**    19-Apr-2011 (dansa01)
**	Added makeCAPIdescVdataV helper function, which factors out code from
**	the executeProcedure method so that it can be shared with the new
**	putParameters method (coming soon).
**    21-Apr-2011 (dansa01)
**	Removed references to obsolete NetuXxx classes.
**    26-Apr-2011 (whiro01)
**	Add InstallType enum and type field to VersionInfo and parsing to
**	figure out Ingres vs. Vectorwise installation.
**    04-Jun-2011 (whiro01)
**	Add machine-dependent code for Linux.
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
**    19-Aug-2011 (dansa01)
**	Removed obsolete CreateDBOptions stuff.
**    27-Aug-2011 (whiro01)
**	Make the mutex recursive on *nix systems to match the behavior of
**	critical sections on Windows.
**    22-Aug-2011 (whiro01)
**	Using NT_GENERIC instead of WINDOWS for the platform tests; add
**	OSX in addition to DARWIN.
**    11-Oct-2011 (dansa01)
**	Now using IIapi instance methods (utilizing java.nio.charset classes) to
**	do charset conversions for both the Ingres and Native cases.  Added
**	"_this" parameter to functions that do Native charset conversions so that
**	they can now access the IIapi instance.  Added new IIapi method IDs, and
**	encode/decode helper function declarations.
**    18-Oct-2011 (whiro01)
**	Added fullyQualifiedHostName to the SysInfo structure.
*/
#ifndef IIAPI_JNI_UTIL_H
#define IIAPI_JNI_UTIL_H

#include <jni.h>

#ifndef FALSE
  #define FALSE	0
  #define TRUE	1
#endif

#define UNREFERENCED(x)	(x)

// TODO: This could probably be done in individual template C++ classes, so that
// each object could become its own C++ class
typedef enum {
	FLD_SHORT = 1,
	FLD_INT,
	FLD_LONG,
	FLD_BOOLEAN,
	FLD_STRING,
	FLD_OBJECT,
	FLD_OBJARRAY,
	FLD_VERSIONINFO,
	FLD_INSTALLTYPE
} FieldTypes;

typedef struct {
	jfieldID *fieldID;
	FieldTypes type;
	const char *fieldname;
} FieldInfo;

typedef struct {
	jmethodID *methodID;
	const char *signature;
	const char *methodname;
	bool isStatic;
} MethodInfo;

typedef struct {
	jclass *pClass;
	const char *classname;
	FieldInfo *fields;
	int numfields;
	MethodInfo *methods;
	int nummethods;
} ClassInfo;


#define CLASSDEF(CLASSNAME)	jclass CLS_##CLASSNAME	= NULL
#define CLASSREF(CLASSNAME)	CLS_##CLASSNAME

#define FIELDDEF(CLASSNAME, FIELDNAME)	jfieldID FID_##CLASSNAME##_##FIELDNAME	= NULL
#define FIELDINFO(CLASSNAME)	FieldInfo CLASSNAME##_fields[]
#define FIELDREF(CLASSNAME, FIELDNAME, FIELDTYPE)	{ &FID_##CLASSNAME##_##FIELDNAME, FIELDTYPE, #FIELDNAME }
#define FIELDID(CLASSNAME, FIELDNAME)	FID_##CLASSNAME##_##FIELDNAME

#define METHODDEF(CLASSNAME, METHODNAME)	jmethodID MID_##CLASSNAME##_##METHODNAME	= NULL
#define METHODINFO(CLASSNAME)	MethodInfo CLASSNAME##_methods[]
#define METHODREF(CLASSNAME, METHODNAME, SIGNATURE, STATIC)	{ &MID_##CLASSNAME##_##METHODNAME, SIGNATURE, #METHODNAME, STATIC }
#define METHODINI(CLASSNAME, METHODNAME, SIGNATURE)		{ &MID_##CLASSNAME##_##METHODNAME, SIGNATURE, "<init>", false }
#define METHODID(CLASSNAME, METHODNAME)	MID_##CLASSNAME##_##METHODNAME

#define FIELDLIST(CLASSNAME)	CLASSNAME##_fields, sizeof(CLASSNAME##_fields) / sizeof(FieldInfo)
#define METHODLIST(CLASSNAME)	CLASSNAME##_methods, sizeof(CLASSNAME##_methods) / sizeof(MethodInfo)

#define FIELDLIST_EMPTY		NULL, 0
#define METHODLIST_EMPTY	NULL, 0

#define CLASSINFO(CLASSNAME)	{ &CLS_##CLASSNAME, #CLASSNAME, FIELDLIST(CLASSNAME), METHODLIST(CLASSNAME) }

#define ARRAY_SIZE(ARRAY)	(sizeof(ARRAY) / sizeof(ARRAY[0]))


//
// Make class references, methodIDs and fieldIDs available to other modules.
//
extern jclass CLASSREF(LocalIIapi);
extern jmethodID METHODID(LocalIIapi, encodeNativeChars);
extern jmethodID METHODID(LocalIIapi, decodeNativeChars);
extern jmethodID METHODID(LocalIIapi, encodeIngresChars);
extern jmethodID METHODID(LocalIIapi, decodeIngresChars);
extern jfieldID FIELDID(LocalIIapi, Instance);

extern jclass CLASSREF(InstallType);
extern jmethodID METHODID(InstallType, getPrefix);
extern jmethodID METHODID(InstallType, findType);

extern jclass CLASSREF(SysInfo);
extern jmethodID METHODID(SysInfo, init);
extern jfieldID FIELDID(SysInfo, IIinstallation);
extern jfieldID FIELDID(SysInfo, IIsystem);
extern jfieldID FIELDID(SysInfo, hostName);
extern jfieldID FIELDID(SysInfo, fullyQualifiedHostName);
extern jfieldID FIELDID(SysInfo, version);

extern jclass CLASSREF(VersionInfo);
extern jmethodID METHODID(VersionInfo, init);
extern jfieldID FIELDID(VersionInfo, majorVersion);
extern jfieldID FIELDID(VersionInfo, minorVersion);
extern jfieldID FIELDID(VersionInfo, revision);
extern jfieldID FIELDID(VersionInfo, platform);
extern jfieldID FIELDID(VersionInfo, build);
extern jfieldID FIELDID(VersionInfo, releaseString);
extern jfieldID FIELDID(VersionInfo, type);

extern jclass CLASSREF(Privileges);
extern jmethodID METHODID(Privileges, ordinal);

extern jclass CLASSREF(Descriptor);
extern jmethodID METHODID(Descriptor, init);
extern jfieldID FIELDID(Descriptor, dataType);
extern jfieldID FIELDID(Descriptor, nullable);
extern jfieldID FIELDID(Descriptor, length);
extern jfieldID FIELDID(Descriptor, precision);
extern jfieldID FIELDID(Descriptor, scale);
extern jfieldID FIELDID(Descriptor, columnType);
extern jfieldID FIELDID(Descriptor, columnName);

extern jclass CLASSREF(DataValue);
extern jmethodID METHODID(DataValue, init);
extern jfieldID FIELDID(DataValue, isnull);
extern jfieldID FIELDID(DataValue, length);
extern jfieldID FIELDID(DataValue, value);

extern jclass CLASSREF(QueryInfo);
extern jmethodID METHODID(QueryInfo, init);
extern jfieldID FIELDID(QueryInfo, flags);
extern jfieldID FIELDID(QueryInfo, mask);
extern jfieldID FIELDID(QueryInfo, rowCount);
extern jfieldID FIELDID(QueryInfo, readOnly);
extern jfieldID FIELDID(QueryInfo, procReturn);
extern jfieldID FIELDID(QueryInfo, procHandle);
extern jfieldID FIELDID(QueryInfo, repeatQueryHandle);
extern jfieldID FIELDID(QueryInfo, tableKey);
extern jfieldID FIELDID(QueryInfo, objectKey);
extern jfieldID FIELDID(QueryInfo, cursorType);
extern jfieldID FIELDID(QueryInfo, rowStatus);
extern jfieldID FIELDID(QueryInfo, rowPosition);

extern jclass CLASSREF(APIException);
extern jmethodID METHODID(APIException, init);

extern jclass CLASSREF(HandleBase);
extern jfieldID FIELDID(HandleBase, handleValue);

extern jclass CLASSREF(ConnectionHandle);
extern jmethodID METHODID(ConnectionHandle, init);

extern jclass CLASSREF(TransactionHandle);
extern jmethodID METHODID(TransactionHandle, init);

extern jclass CLASSREF(StatementHandle);
extern jmethodID METHODID(StatementHandle, init);

extern jclass CLASSREF(ContextHandle);
extern jmethodID METHODID(ContextHandle, init);

extern jclass CLASSREF(ConnectOptions);
extern jfieldID FIELDID(ConnectOptions, effectiveUser);
extern jfieldID FIELDID(ConnectOptions, group);
extern jfieldID FIELDID(ConnectOptions, role);
extern jfieldID FIELDID(ConnectOptions, dbmsPassword);

extern jclass CLASSREF(String);
extern jmethodID METHODID(String, init);
extern jmethodID METHODID(String, getBytes);

extern jclass CLASSREF(ArrayList);
extern jmethodID METHODID(ArrayList, init);
extern jmethodID METHODID(ArrayList, add);

extern jclass CLASSREF(TreeMap);
extern jmethodID METHODID(TreeMap, init);
extern jmethodID METHODID(TreeMap, put);

extern jclass CLASSREF(Byte);
extern jmethodID METHODID(Byte, valueOf);
extern jmethodID METHODID(Byte, byteValue);

extern jclass CLASSREF(Short);
extern jmethodID METHODID(Short, valueOf);
extern jmethodID METHODID(Short, shortValue);

extern jclass CLASSREF(Integer);
extern jmethodID METHODID(Integer, valueOf);
extern jmethodID METHODID(Integer, intValue);

extern jclass CLASSREF(Long);
extern jmethodID METHODID(Long, valueOf);
extern jmethodID METHODID(Long, longValue);

extern jclass CLASSREF(Float);
extern jmethodID METHODID(Float, valueOf);
extern jmethodID METHODID(Float, floatValue);

extern jclass CLASSREF(Double);
extern jmethodID METHODID(Double, valueOf);
extern jmethodID METHODID(Double, doubleValue);


extern const char *OutOfMemoryError;
extern const char *NullPointerException;
extern const char *IllegalArgumentException;
extern const char *IllegalStateException;
extern const char *ArrayIndexOutOfBoundsException;


/*
** Make utility functions and classes available to other modules.
*/
extern void JNU_Initialize(JNIEnv *jenv);
extern void JNU_Terminate(JNIEnv *jenv);

extern void JNU_ThrowByName(JNIEnv *jenv, const char *name, const char *msg);

inline char *JNU_GetStringUTFChars(JNIEnv *jenv, jstring jstr)
{
	if (jstr == NULL)
		return NULL;
	return (char *)jenv->GetStringUTFChars(jstr, NULL);
}
inline void JNU_FreeStringUTFChars(JNIEnv *jenv, jstring jstr, const char *str)
{
	if (str != NULL)
		jenv->ReleaseStringUTFChars(jstr, str);
}

extern int JNU_ReadVersionRel(TOOLSAPI *inst, char *buf, size_t len);

extern char *JNU_sprintf(JNIEnv *jenv, const char *format, ...);

extern TOOLSAPI *getInstancePtr(JNIEnv *jenv, jobject _this);
extern void throwStartupError(JNIEnv *jenv, const char *fn, i4 status);
extern void throwLastAPIErrors(JNIEnv *jenv, jobject _this, const char *fn, int status, TOOLSAPI *inst);

extern jstring getStringFromIngresChars(JNIEnv *jenv, jobject _this, const char *str);
extern jstring getStringFromNativeChars(JNIEnv *jenv, jobject _this, const char *str);
extern void setIngresStringField(JNIEnv *jenv, jobject _this, jobject jobj, jfieldID fid, const char *str);
extern void setNativeStringField(JNIEnv *jenv, jobject _this, jobject jobj, jfieldID fid, const char *str);

extern jobject createObject(JNIEnv *jenv, jclass _class, jmethodID init);
extern jobject createVersionInfo(JNIEnv *jenv, jobject _this, const char *version);

extern jobject convertJavaDataObject(JNIEnv *jenv, jobject _this, TOOLSAPI *inst, II_DESCRIPTOR *desc, II_DATAVALUE *dval, i2 dstType);
extern jobject getJavaDataObject(JNIEnv *jenv, jobject _this, TOOLSAPI *inst, II_DESCRIPTOR *desc, II_DATAVALUE *dval);
extern void ungetJavaDataObject(JNIEnv *jenv, jobject _this, TOOLSAPI *inst, jobject jValue, II_DESCRIPTOR *pDstDesc, II_DATAVALUE *pDstData);
class BufferList;
extern void makeCAPIdescVdataV(JNIEnv *jenv, jobject _this, TOOLSAPI *inst, BufferList& buffers, jobjectArray jDescV, jobjectArray jDataV, II_DESCRIPTORS **ppDescV, II_DATAVALUES **ppDataV);

extern void setHandleValue(JNIEnv *jenv, jobject handleObj, II_HANDLE handleVal);
extern II_HANDLE getHandleValue(JNIEnv *jenv, jobject handleObj);
extern jobject newHandleObj(JNIEnv *jenv, jclass handleClass, jmethodID init, II_HANDLE handleVal);


/*
** Forward references internal to this module.
*/
void JNU_LoadClasses(JNIEnv *jenv, ClassInfo classes[], int numclasses);
void JNU_UnloadClasses(JNIEnv *jenv, ClassInfo classes[], int numclasses);
jclass JNU_LoadClass(JNIEnv *jenv, const char *szClassName);
void JNU_GetFieldIDs(JNIEnv *jenv, jclass _class, const char *classname, FieldInfo fields[], int numfields);
void JNU_GetMethodIDs(JNIEnv *jenv, jclass _class, const char *classname, MethodInfo methods[], int nummethods);
char *encode(JNIEnv *jenv, jobject _this, jmethodID encodingMethod, jstring jstr);
jstring decode(JNIEnv *jenv, jobject _this, jmethodID decodingMethod, const char *str);

/*
** Name: IngresChars - Self-releasing Ingres char pointer
**
** Description:
**	JNI coding requires meticulous cleanup after the many early returns
**	that can arise from ExceptionCheck calls.  This simple class allows
**	the compiler to handle the cleanup bookkeeping for us.
**
**	The constructor for this class also takes care of the charset
**	conversion from Unicode to the Ingres charset associated with the
**	IIapi instance, which is why we need to pass the iiapi instance pointer
**	("_this") to the constructor.
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
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding.
**	08-Mar-2011 (whiro01)
**	    Added "set" method and default constructor.
*/
class IngresChars
{
public:
	IngresChars(JNIEnv *jenv, jobject _this, jstring jstr);
	IngresChars();

	~IngresChars();

	void set(JNIEnv *jenv, jobject _this, jstring jstr);

	operator const char *()
	{
	    return const_cast<const char *>(m_cstr);
	}

private:
	char *m_cstr;

	// Prevent copying and assignment.
	IngresChars(const IngresChars&);
	IngresChars& operator=(const IngresChars&);
};


/*
** Name: NativeChars - Self-releasing native char pointer
**
** Description:
**	JNI coding requires meticulous cleanup after the many early returns
**	that can arise from ExceptionCheck calls.  This simple class allows
**	the compiler to handle the cleanup bookkeeping for us.
**
**	The constructor for this class also takes care of the charset
**	conversion from Unicode to the native charset associated with the
**	Java installation.
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
** History:
**	15-Dec-2010 (dansa01)
**	    Initial coding.
**	08-Mar-2011 (whiro01)
**	    Added "set" method and default constructor.
**	10-Oct-2011 (dansa01)
**	    Added "_this" parameter to allow access to IIapi instance methods
**	    for charset conversions.
*/
class NativeChars
{
public:
	NativeChars(JNIEnv *jenv, jobject _this, jstring jstr);
	NativeChars();

	~NativeChars();

	void set(JNIEnv *jenv, jobject _this, jstring jstr);

	operator const char *()
	{
	    return const_cast<const char *>(m_cstr);
	}

private:
	char *m_cstr;

	// Prevent copying and assignment.
	NativeChars(const NativeChars&);
	NativeChars& operator=(const NativeChars&);
};


/*
** Name: MEfreePtr - Self-freeing structure pointer template
**
** Description:
**	A number of different Toolsapi methods return an output block that
**	must be freed with a call to MEfree.  This simple template allows the
**	compiler to handle that automatically for us, no matter how we exit
**	the local scope of the current JNI function.
**
**	The intended usage model is something like the following:
**
**	    MEfreePtr<II_NETU_LOGINS> pnl(inst);
**	    ret = inst->NetuShowLogin(&inParm, &pnl);
**	    if (ret == II_ST_SUCCESS)
**	    {
**	        for (int i = 0; i < pnl->count; i++)
**	        {
**	            const char * vnode = pnl->logins[i].vnode;
**	            . . .
**
**	Note that content is usually set via the address-of operator, because
**	that is the way we obtain such output blocks from the API.  We pass the
**	address of a pointer to the API function, which then sets that pointer
**	to a block which must later be freed via MEfree.  (Of course, you must
**	not attempt to set the same MEfreePtr more than once, without calling
**	clear() first.  It can't detect or protect against being overwritten
**	via the & operator.)
**
** History:
**	21-Sep-2010 (dansa01)
**	    Initial coding.
**	28-Sep-2010 (dansa01)
**	    Second constructor was missing a comma in the initializer list, but
**	    since that constructor is currently unused, it didn't show up as an
**	    error until the OSX compiler got ahold of it.
**	14-Oct-2010 (dansa01)
**	    Added conversion operators (bool and structType*) to allow simple
**	    "if" test for NULL pointer content, and to allow this type to be
**	    more easily used in different pointer contexts.
**	15-Oct-2010 (dansa01)
**	    Removed the bool conversion operator because of ambiguity on older
**	    compilers.  The pointer conversion alone is sufficient because of
**	    the standard conversion from pointer to bool.
*/
template <class structType> class MEfreePtr
{
public:
	MEfreePtr(TOOLSAPI *inst)
	:   m_inst(inst),
	    m_ptr(NULL)
	{
	};

	MEfreePtr(TOOLSAPI *inst, structType *ptr)
	:   m_inst(inst),
	    m_ptr(ptr)
	{
	};

	~MEfreePtr()
	{
	    clear();
	};

	void clear()
	{
	    if (m_ptr)
	    {
	        m_inst->MEfree(m_ptr);
	        m_ptr = NULL;
	    }
	};

	structType ** operator&()
	{
	    return &m_ptr;
	};

	structType * operator->()
	{
	    return m_ptr;
	};

	operator structType *()
	{
	    return m_ptr;
	};

private:
	TOOLSAPI *m_inst;
	structType *m_ptr;

	// Prevent copying and assignment.
	MEfreePtr(const MEfreePtr&);
	MEfreePtr& operator=(const MEfreePtr&);
};


/*
** Name: MallocFreePtr - Self-freeing malloc pointer template
**
** Description:
**	This template is analogous to the MEfreePtr, but suitable for buffers
**	that are allocated via malloc, outside of the ToolsAPI DLL.
**
**	The intended usage model is something like the following.  Pass in the
**	desired buffer size, and it will be malloc'ed for you:
**
**	    MallocFreePtr<II_DESCRIPTORS> pDescV(bytesNeeded);
**
**	or, if you already have a malloc'ed pointer, pass that in directly:
**
**	    MallocFreePtr<char> msg(JNU_sprintf(jenv, ...));
**
**	Then the compiler will call free() on the contained pointer when the
**	MallocFreePtr instance goes out of scope.
**
**	The value is typically set using the constructor, but it can also be
**	set via the address-of operator.  (Of course, you must not attempt to
**	set the same MallocFreePtr more than once, without calling clear()
**	first.  It can't detect or protect against being overwritten via
**	the & operator.)
**
** History:
**	20-Dec-2010 (dansa01)
**	    Initial coding.
**	21-Dec-2010 (dansa01)
**	    Implemented Roger's suggestion for a constructor that does the
**	    malloc call for us.
*/
template <class bufType> class MallocFreePtr
{
public:
	MallocFreePtr()
	:   m_ptr(NULL)
	{
	};

	MallocFreePtr(size_t bytesNeeded)
	:   m_ptr(reinterpret_cast<bufType *>(malloc(bytesNeeded)))
	{
	};

	MallocFreePtr(void *ptr)
	:   m_ptr(reinterpret_cast<bufType *>(ptr))
	{
	};

	~MallocFreePtr()
	{
	    clear();
	};

	void clear()
	{
	    if (m_ptr)
	    {
	        free(m_ptr);
	        m_ptr = NULL;
	    }
	};

	bufType ** operator&()
	{
	    return &m_ptr;
	};

	bufType * operator->()
	{
	    return m_ptr;
	};

	operator bufType *()
	{
	    return m_ptr;
	};

private:
	bufType *m_ptr;

	// Prevent copying and assignment.
	MallocFreePtr(const MallocFreePtr&);
	MallocFreePtr& operator=(const MallocFreePtr&);
};


/*
** Name: Mutex - Cross-platform mutex object.
**
** Description:
**	This class provides a synchronization primitive that behaves similarly
**	on all supported platforms in order to synchronize access to a global
**	data structure across multiple threads.
**
**	The intended usage model is something like the following:
**
**	    Mutex mobj;
**	    ...
**	    mobj.enter();
**	    ... access the shared object that "mobj" is protecting
**	    mobj.leave();
**
**	Note that the mutex object is destroyed when "mobj" goes out of scope
**	(which might be never in the case of a static object).
**
** History:
**	09-Dec-2010 (whiro01)
**	    Initial coding from the #defines that used to be in IIapi_jni_handle.cpp.
**	27-Aug-2011 (whiro01)
**	    Make the mutex recursive on *nix systems to match the behavior of
**	    critical sections on Windows.
*/
class Mutex
{
public:
	Mutex()
	{
#if defined(NT_GENERIC) || defined(WIN32)
		InitializeCriticalSection(&sync_obj);
#elif defined(LINUX) || defined(DARWIN) || defined(OSX)
		pthread_mutexattr_init(&attributes);
		pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&sync_obj, &attributes);
#endif
	};
	~Mutex()
	{
#if defined(NT_GENERIC) || defined(WIN32)
		DeleteCriticalSection(&sync_obj);
#elif defined(LINUX) || defined(DARWIN) || defined(OSX)
		pthread_mutex_destroy(&sync_obj);
		pthread_mutexattr_destroy(&attributes);
#endif
	};
	void enter()
	{
#if defined(NT_GENERIC) || defined(WIN32)
		EnterCriticalSection(&sync_obj);
#elif defined(LINUX) || defined(DARWIN) || defined(OSX)
		pthread_mutex_lock(&sync_obj);
#endif
	};
	void leave()
	{
#if defined(NT_GENERIC) || defined(WIN32)
		LeaveCriticalSection(&sync_obj);
#elif defined(LINUX) || defined(DARWIN) || defined(OSX)
		pthread_mutex_unlock(&sync_obj);
#endif
	};

private:
#if defined(NT_GENERIC) || defined(WIN32)
	CRITICAL_SECTION	sync_obj;
#elif defined(LINUX) || defined(DARWIN) || defined(OSX)
	pthread_mutexattr_t attributes;
	pthread_mutex_t	sync_obj;
#else
	#error Need to implement synchronization on this platform!
#endif
};


/*
** Name: BufferList - Self-cleaning list of data buffers
**
** Description:
**	When processing database procedure input parameters we have to juggle
**	an unpredictable (but typically small) number of data buffers, and
**	ensure that they all get freed on every exit path.  This simple class
**	simplifies that cleanup bookkeeping by maintaining a list of buffers
**	and automatically freeing them all when the BufferList instance is
**	destructed upon exit from its defining scope.
**
**	The "getNewBuffer" method allocates and returns a buffer of the
**	requested size and adds it to the cleanup list.
**
**	The intended usage model is like the following:
**
**	    BufferList bl;
**	    . . .
**	    II_PTR buf = bl.getNewBuffer(jenv, bytesNeeded);
**	    if (jenv->ExceptionCheck())
**	        return;
**
**	Then you can forget about freeing that buffer.  The ExceptionCheck is
**	needed because the getNewBuffer method may throw OutOfMemoryError.
**
** History:
**	11-Feb-2011 (dansa01)
**	    Initial coding.
*/
class BufferList
{
public:
	BufferList();

	~BufferList();

	void *getNewBuffer(JNIEnv *jenv, size_t bytesNeeded);

private:
	union BufferListNode
	{
	    BufferListNode *m_next;
	    double          m_alignment;
	};

	BufferListNode *m_listHead;

	// Prevent copying and assignment.
	BufferList(const BufferList&);
	BufferList& operator=(const BufferList&);
};


#endif /* not defined IIAPI_JNI_UTIL_H */
