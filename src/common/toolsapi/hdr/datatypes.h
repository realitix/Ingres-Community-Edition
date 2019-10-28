/**
**    Copyright (c) 2010-2011 Actian Corporation.  All rights reserved.
**
**	Actian Tools API Data Structure Definitions
**
**	These are the portable declarations of data types and
**	structures that are passed in and out of the Tools API.
**
**    History:
**	16-Jun-2010 (whiro01)
**	    Separated from the original header templates and made
**	    universal.
**	06-Jul-2010 (whiro01)
**	    Added VersionInfo structure (for Discovery).
**	07-Jul-2010 (whiro01)
**	    Support for OSX.
**	23-Aug-2010 (whiro01)
**	    Added "linked_ptr.h" for use with STL objects;
**	    changed some typedefs.
**	26-Aug-2010 (whiro01)
**	    Update data type definitions for non-Microsoft
**	    Windows compilers.
**	31-Aug-2010 (dansa01)
**	    Added C structs for returning a variable number of rows of
**	    vnode data from the Netu functions.
**	16-Sep-2010 (dansa01)
**	    Trying to be const-correct with the Netu subset this time.  Because
**	    I expected to be using lots of old APIs that don't have const on
**	    their char* input, I thought it would be simplest to just expose
**	    our API the same way.  But now I realize that was a mistake because
**	    it simply pushes the burden up to our caller.  Better to handle any
**	    const_cast nuisance inside the toolsapi layer.  (And I discovered
**	    that the old Ingres APIs have improved, even though the CL spec has
**	    not been updated to reflect that, and there really wasn't much
**	    const_cast'ing required after all.)
**	14-Oct-2010 (dansa01)
**	    Replaced CharPtrPair, CharPtrMap, DescVector, and DataVector with
**	    C-compatible alternatives.  Changed II_DESCRIPTOR and II_DATAVALUE
**	    members (isNullable, isNull) to be the same type when compiled in
**	    C and C++ (instead of C++ bool under C++).  Made similar changes to
**	    II_QUERY_INFO (readOnly) and II_NETU_INPARM (privateScope).
**	15-Nov-2010 (dansa01)
**	    Adapted to the new error-reporting infrastructure.  Added II_ERROR
**	    and II_ERRORS structs, and added the II_GE_xxx type categories to
**	    differentiate between the various OpenAPI error-code namespaces.
**	    II_GE_TOOLSAPI means the code is from our ToolsAPI namespace (which
**	    includes the union of II_ST_xxx status codes, and Ingres ER_MSGID
**	    codes).  All other types come direclty from IIapi_getErrorInfo.
**	    Also removed the obsolete II_ERR_DESC, STL header references, and
**	    the unused C++ methods from our other C-API structs.
**	01-Dec-2010 (whiro01)
**	    Put back #include <stddef.h> for "size_t" definition on non-Windows
**	    platforms.
**	02-Dec-2010 (dansa01)
**	    Added i_size as a hedge against possible (however unlikely) future
**	    portability and remoting issues.  Also removed unused const_char.
**	07-Feb-2011 (dansa01)
**	    Added II_CONNECT_OPTIONS structure to support a variety of new
**	    optional connection parameters.
**	10-Feb-2011 (dansa01)
**	    Added II_COL_xxx columnType values needed for database procedure
**	    parameter descriptors.
**	26-Feb-2011 (whiro01)
**	    Added II_CREATEDB_PARM for "UTcreateDatabase" method; added flags
**	    for UTstartIngres and UTstopIngres functions.
**	21-Apr-2011 (dansa01)
**	    Removed obsolete II_NETU_xxx structures.
**	04-Jun-2011 (whiro01)
**	    Added defines and so forth for x64 Linux.
**	13-Jun-2011 (gordy)
**	    Add eff_user to II_CREATEDB_OPTIONS.
**	19-Aug-2011 (dansa01)
**	    Removed unused definitions relating to obsolete UTcreateDatabase,
**	    UTstartIngres, UTstopIngres functions.
**	22-Aug-2011 (whiro01)
**	    Using NT_GENERIC instead of WINDOWS for the platform tests; added
**	    OSX in addition to DARWIN.
**	22-Oct-2011 (whiro01)
**	    Adding LG_PRIMARY_LOG and LG_DUAL_LOG as parameters for LGgetLogPaths
**	    with the same definitions as in IIapi.java (and same meanings as the
**	    similar definitions in "lg.h"); add LG_MAX_PATHS with same value as
**	    LG_MAX_FILE in "lg.h"
*/

#ifndef _DATATYPES_H_INCLUDED
#define _DATATYPES_H_INCLUDED


#define SUCCESS		(0)

# define II_ST_SUCCESS		0
# define II_ST_MESSAGE		1
# define II_ST_WARNING		2
# define II_ST_NO_DATA		3
# define II_ST_ERROR		4
# define II_ST_FAILURE		5
# define II_ST_NOT_INITIALIZED	6
# define II_ST_INVALID_HANDLE	7
# define II_ST_OUT_OF_MEMORY	8

# define II_GE_TOOLSAPI		0
# define II_GE_ERROR		1
# define II_GE_WARNING		2
# define II_GE_MESSAGE		3
# define II_GE_XAERR		4


/*
**  Typedefs needed for this interface, but which will
**   be defined within Ingres itself for the internal
**   implementation version of the Tools API.
*/
#if !defined(COMPAT_INCLUDE)
#if defined(NT_GENERIC) || defined(WIN32)
#if defined(_MSC_VER)
  #if _MSC_VER < 1600 /* earlier than Visual Studio 2010 */
    typedef __int64 int64_t;
    typedef unsigned __int64 uint64_t;
  #else
    #include <stdint.h>
  #endif
#else
  #include <stdint.h>
#endif
typedef char i1;
typedef short i2;
typedef int i4;
typedef int64_t i8;
typedef unsigned char u_i1;
typedef unsigned short u_i2;
typedef unsigned int u_i4;
typedef uint64_t u_i8;
typedef	float		f4;
typedef	double		f8;
#elif defined(DARWIN) || defined(OSX)
typedef char i1;
typedef short i2;
typedef int i4;
typedef long i8;
typedef unsigned char u_i1;
typedef unsigned short u_i2;
typedef unsigned int u_i4;
typedef unsigned long u_i8;
typedef float	f4;
typedef double	f8;
#elif defined(LINUX)
typedef char i1;
typedef short i2;
typedef int i4;
typedef long i8;
typedef unsigned char u_i1;
typedef unsigned short u_i2;
typedef unsigned int u_i4;
typedef unsigned long u_i8;
typedef float	f4;
typedef double	f8;
#else
	#error	Need to define basic data types for this platform!
#endif
#endif	/* !def COMPAT_INCLUDE */

#include <stddef.h>

/*
**	Typedefs and #defines for common data structures
**	used by many functions.
*/
// TODO: II_HANDLE should be u_i8 to allow C-API remoting to 64-bit target
typedef void *II_HANDLE;
typedef void *II_PTR;
typedef u_i4 i_size;

typedef struct _II_DESCRIPTOR {
	i2   dataType;
# define II_HNDL_TYPE	((i2) -1)	/* API Handle */
# define II_CHR_TYPE	((i2) 32)	/* Text Fixed */
# define II_CHA_TYPE	((i2) 20)	/* Character Fixed */
# define II_VCH_TYPE	((i2) 21)	/* Character Variable */
# define II_LVCH_TYPE	((i2) 22)	/* Character Long */
# define II_LCLOC_TYPE	((i2) 36)	/* Character Long Locator */
# define II_BYTE_TYPE	((i2) 23)	/* Binary Fixed */
# define II_VBYTE_TYPE	((i2) 24)	/* Binary Variable */
# define II_LBYTE_TYPE	((i2) 25)	/* Binary Long */
# define II_LBLOC_TYPE	((i2) 35)	/* Binary Long Locator */
# define II_NCHA_TYPE	((i2) 26)	/* Unicode Fixed */
# define II_NVCH_TYPE	((i2) 27)	/* Unicode Variable */
# define II_LNVCH_TYPE	((i2) 28)	/* Unicode Long */
# define II_LNLOC_TYPE	((i2) 29)	/* Unicode Long Locator */
# define II_TXT_TYPE	((i2) 37)	/* Text Variable */
# define II_LTXT_TYPE	((i2) 41)	/* Text Var (typeless NULL) */
# define II_INT_TYPE	((i2) 30)	/* Integer */
# define II_FLT_TYPE	((i2) 31)	/* Floating Point */
# define II_MNY_TYPE	((i2)  5)	/* Money */
# define II_DEC_TYPE	((i2) 10)	/* Decimal */
# define II_BOOL_TYPE	((i2) 38)	/* Boolean */
# define II_DTE_TYPE	((i2)  3)	/* Ingres Date */
# define II_DATE_TYPE	((i2)  4)	/* ANSI Date */
# define II_TIME_TYPE	((i2)  8)	/* Ingres Time */
# define II_TMWO_TYPE	((i2)  6)	/* Time without Timezone */
# define II_TMTZ_TYPE	((i2)  7)	/* Time with Timezone */
# define II_TS_TYPE	((i2) 19)	/* Ingres Timestamp */
# define II_TSWO_TYPE	((i2)  9)	/* Timestamp without Timezone */
# define II_TSTZ_TYPE	((i2) 18)	/* Timestamp with Timezone */
# define II_INTYM_TYPE	((i2) 33)	/* Interval Year to Month */
# define II_INTDS_TYPE	((i2) 34)	/* Interval Day to Second */
# define II_LOGKEY_TYPE	((i2) 11)	/* Logical Key */
# define II_TABKEY_TYPE	((i2) 12)	/* Table Key */
# define II_IDATE_TYPE	II_DTE_TYPE
# define II_ADATE_TYPE	II_DATE_TYPE

	u_i2 isNullable;
	u_i2 length;
	i2   precision;
	i2   scale;
	i2   columnType;
# define II_COL_TUPLE		0	/* tuple column type */
# define II_COL_PROCBYREFPARM	1	/* procedure byref parm column type */
# define II_COL_PROCPARM	2	/* procedure parm column type */
# define II_COL_SVCPARM		3	/* service parm column type */
# define II_COL_QPARM		4	/* query parm column type */
# define II_COL_PROCGTTPARM	5	/* procedure global temp table */
# define II_COL_PROCINPARM	2	/* procedure IN parm (alias) */
# define II_COL_PROCOUTPARM	6	/* procedure OUT parm column type */
# define II_COL_PROCINOUTPARM	1	/* procedure INOUT parm (alias) */

	char *columnName;
} II_DESCRIPTOR;

typedef struct _II_DESCRIPTORS {
	i4            count;
	II_DESCRIPTOR descriptors[1];
} II_DESCRIPTORS;


typedef struct _II_DATAVALUE {
	u_i2 isNull;
	u_i2 length;
	II_PTR data;
} II_DATAVALUE;

typedef struct _II_DATAVALUES {
	i4           count;
	II_DATAVALUE values[1];
} II_DATAVALUES;


typedef struct _II_NAME_VALUE_PAIR {
	const char *name;
	const char *value;
} II_NAME_VALUE_PAIR;

typedef struct _II_NAME_VALUE_PAIRS {
	i4                 count;
	II_NAME_VALUE_PAIR pairs[1];
} II_NAME_VALUE_PAIRS;


typedef struct _II_LOCATION {
	char    *node;
	char    *path;
	char    *fname;
	char    *string;
	i4      desc;
	II_PTR  *fp;
} II_LOCATION;


typedef struct _II_QUERY_INFO {
	u_i4    flags;
# define II_QINFO_FLAG_FAIL			0x0001	/* Query produced an error */
# define II_QINFO_FLAG_ALL_UPDATED		0x0002	/* SQLWARN4: no where clause */
# define II_QINFO_FLAG_NULLS_REMOVED		0x0004	/* SQLWARN2: NULLs eliminated */
# define II_QINFO_FLAG_UNKNOWN_REPEAT_QUERY	0x0008	/* Repeat Query not defined */
# define II_QINFO_FLAG_END_OF_DATA		0x0010	/* End of Data for cursor */
# define II_QINFO_FLAG_CONTINUE			0x0020	/* Copy stmt flow control */
# define II_QINFO_FLAG_INVALID_STATEMENT	0x0040	/* SQLWARN5: invalid stmt */
# define II_QINFO_FLAG_TRANSACTION_INACTIVE	0x0080	/* Inactive Transaction State */
# define II_QINFO_FLAG_OBJECT_KEY		0x0100	/* Object key returned */
# define II_QINFO_FLAG_TABLE_KEY		0x0200	/* Table key returned */
# define II_QINFO_FLAG_NEW_EFFECTIVE_USER	0x0400	/* Effective user ID changed */
# define II_QINFO_FLAG_FLUSH_QUERY_ID		0x0800	/* Query/Proc IDs invalidated */
# define II_QINFO_FLAG_ILLEGAL_XACT_STMT	0x1000	/* Transaction stmt attempted */
	u_i4    mask;
# define II_QINFO_MASK_ROW_COUNT	0x0001
# define II_QINFO_MASK_CURSOR		0x0002
# define II_QINFO_MASK_PROCEDURE_RET	0x0004
# define II_QINFO_MASK_PROCEDURE_ID	0x0008
# define II_QINFO_MASK_REPEAT_QUERY_ID	0x0010
# define II_QINFO_MASK_TABLE_KEY	0x0020
# define II_QINFO_MASK_OBJECT_KEY	0x0040
# define II_QINFO_MASK_ROW_STATUS	0x0080
	i4      rowCount;
	i4      readOnly;
	i4      procReturn;
	II_HANDLE procHandle;
	II_HANDLE repeatQueryHandle;
# define II_TABLE_KEY_SIZE	(8)
	char    tableKey[II_TABLE_KEY_SIZE];
# define II_OBJECT_KEY_SIZE	(16)
	char    objectKey[II_OBJECT_KEY_SIZE];
	u_i4    cursorType;
# define II_QINFO_CURSOR_UPDATE		0x0001	/* Updatable */
# define II_QINFO_CURSOR_SCROLL		0x0002	/* Scrollable */
	u_i4    rowStatus;
	i4      rowPosition;
} II_QUERY_INFO;


typedef struct _II_ERROR {
	i4          type;
	i4          code;
	const char *source;
	const char *message;
} II_ERROR;

typedef struct _II_ERRORS {
	i4       count;
	II_ERROR errors[1];
} II_ERRORS;


typedef struct _VersionInfo {
	int majorVersion;
	int minorVersion;
	int revision;
	int build;
	const char* releaseString;
#ifdef __cplusplus
	_VersionInfo();
	explicit _VersionInfo(const char *rel);
	_VersionInfo(const _VersionInfo& info);
	~_VersionInfo();
	_VersionInfo& operator=(const _VersionInfo& info);
#endif
} VersionInfo;


enum Privileges {
	SERVER_CONTROL,
	NET_ADMIN,
	MONITOR,
	TRUSTED
};


/*
**  Optional database connection parameters.
*/
typedef struct _II_CONNECT_OPTIONS {
	const char *effectiveUser;
	const char *group;
	const char *role;
	const char *dbmsPassword;
} II_CONNECT_OPTIONS;

/*
** Parameter values for LGgetLogPaths
*/
#define	LG_PRIMARY_TRANS_LOG	(0x01)
#define	LG_DUAL_TRANS_LOG	(0x02)
#define LG_MAX_PATHS		(16)


#endif	/* _DATATYPES_H_INCLUDED */
