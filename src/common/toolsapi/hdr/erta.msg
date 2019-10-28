/*
**    Copyright (c) 2011 Actian Corporation.  All rights reserved.
*/

/**
** Name:	erta.msg -	Message file for class TA
**
** Description:
**	This file defines the messages for class TA (ToolsAPI).
**
** History:
**	04-Jan-2011 (dansa01)
**	    Created for ToolsAPI project.  Since this is an API and not a front-
**	    end application, there is not a lot we can say in the System Status
**	    and Recommendation comments for most of these.
**	25-Jan-2011 (dansa01)
**	    Added E_TA0107_QUERY_INFO_ERR.
**	10-Feb-2011 (dansa01)
**	    Added E_TA0108_UNEQUAL_PROC_PARAMS and E_TA0109_EMPTY_INPUT.
**	19-Apr-2011 (dansa01)
**	    Generalized E_TA0108 so that it can be shared by both
**	    IIexecuteProcedure and IIputParameters.
**	21-Apr-2011 (dansa01)
**	    Removed obsolete NETU_XXX messages.
*/

#define	_TA_CLASS	266

E_TA0000_ST_SUCCESS
"Success."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is text associated with the ToolsAPI II_ST_SUCCESS status.
**
** System Status:
**	This message exists only for the sake of completeness, and should
**	never actually be displayed.
**
** Recommendation:
**	N/A.
*/

E_TA0001_ST_MESSAGE
"Message."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is text associated with the ToolsAPI II_ST_MESSAGE status.
**	It is expected to be accompanied by additional messages containing
**	more specific information.
**
** System Status:
**	N/A.
**
** Recommendation:
**	N/A.
*/

E_TA0002_ST_WARNING
"Warning."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is text associated with the ToolsAPI II_ST_WARNING status.
**	It is expected to be accompanied by additional messages containing
**	more specific information.
**
** System Status:
**	N/A.
**
** Recommendation:
**	N/A.
*/

E_TA0003_ST_NO_DATA
"No Data."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is text associated with the ToolsAPI II_ST_NO_DATA status.
**
** System Status:
**	No more data is available in the current query result.  This is
**	typically NOT an error condition.  This message exists only for the
**	sake of completeness, and should never actually be displayed.
**
** Recommendation:
**	N/A.
*/

E_TA0004_ST_ERROR
"Error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is text associated with the ToolsAPI II_ST_ERROR status.
**	It is expected to be accompanied by additional messages containing
**	more specific information.
**
** System Status:
**	N/A.
**
** Recommendation:
**	N/A.
*/

E_TA0005_ST_FAILURE
"Failure."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is text associated with the ToolsAPI II_ST_FAILURE status.
**	It is expected to be accompanied by additional messages containing
**	more specific information.
**
** System Status:
**	N/A.
**
** Recommendation:
**	N/A.
*/

E_TA0006_ST_NOT_INITIALIZED
"OpenAPI not initialized.  You must call ToolsAPIInit before GetToolsAPIInstance."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is text associated with the ToolsAPI II_ST_NOT_INITIALIZED status.
**
** System Status:
**	The ToolsAPI DLL has not been properly initialized.
**
** Recommendation:
**	Be sure to call ToolsAPIInit before GetToolsAPIInstance.
*/

E_TA0007_ST_INVALID_HANDLE
"An invalid handle was passed to the API function, please check your code."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is text associated with the ToolsAPI II_ST_INVALID_HANDLE status.
**
** System Status:
**	N/A.
**
** Recommendation:
**	Be sure to check the status returned by any function that outputs a
**	handle before attempting to use that handle in subsequent calls.
*/

E_TA0008_ST_OUT_OF_MEMORY
"Out of Memory!"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is text associated with the ToolsAPI II_ST_OUT_OF_MEMORY status.
**
** System Status:
**	Once an allocation failure has occurred, the application may become
**	unstable.
**
** Recommendation:
**	Terminating the process may be the only recourse in most cases.
*/

E_TA0100_INVALID_QRY_RESULT
"Unexpected internal error: A row of the query result is incomplete."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an unexpected internal error condition.
**
** System Status:
**	N/A.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Actian Corporation Technical Support as a bug.
*/

E_TA0104_BUF_TOO_SMALL
"The supplied output buffer is too small."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The output buffer provided to a ToolsAPI function (such as
**	CMgetCharsetName) is too small.
**
** System Status:
**	N/A.
**
** Recommendation:
**	This indicates an improper usage of the API, and must be corrected in
**	the caller's code.
*/

E_TA0105_BAD_INFO_STATUS
"Unexpected internal error: Unknown status from IIapi_getErrorInfo."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an unexpected internal error condition.
**
** System Status:
**	N/A.
**
** Recommendation:
**	This error message indicates a software problem, and it should
**	be reported to Actian Corporation Technical Support as a bug.
*/

E_TA0106_PATH_TOO_LONG
"Path string length exceeds the maximum allowed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The path string provided to a ToolsAPI function (such as PMload,
**	PMwrite, or executeUtility) exceeds the maximum path length.
**
** System Status:
**	N/A.
**
** Recommendation:
**	This indicates an improper usage of the API, and must be corrected in
**	the caller's code.
*/

E_TA0107_QUERY_INFO_ERR
"Unexpected flags retured with Query Info (0x%0x)."
/*%
** Parameters:
**	0	The II_QUERY_INFO flags field.
**
** Explanation:
**	The query returned unexpected error indications in the Query Info.
**
** System Status:
**	N/A.
**
** Recommendation:
**	Check the Ingres error log for possible messages from the Name Server.
*/

E_TA0108_UNEQUAL_DESC_DATA
"Unequal number of parameter descriptors and parameter data values."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When calling a database procedure, or sending query parameters,
**	the number of parameter descriptors supplied did not equal the
**	number of parameter data values supplied.
**
** System Status:
**	N/A.
**
** Recommendation:
**	This indicates an improper usage of the API, and must be corrected in
**	the caller's code.
*/

E_TA0109_EMPTY_INPUT
"Illegal NULL or empty input structure."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A required input structure pointer was NULL, or the structure contained
**	zero elements (e.g., II_DESCRIPTORS or II_DATAVALUES).
**
** System Status:
**	N/A.
**
** Recommendation:
**	This indicates an improper usage of the API, and must be corrected in
**	the caller's code.
*/

