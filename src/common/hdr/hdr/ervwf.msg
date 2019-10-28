SQLSTATE_MAPPING_REQUIRED

/*
**    Copyright (c) 2010 Actian Corporation 
**    Message file for Facility code: VW
**
**	Each error is defined by its name followed by a colon and then
**	by the subsidiary error class. See sqlstate.h for the subsidiary
**	error code classes.
**	After the error code line should be the error text surrounded in
**	double quotes. After that may come the optional comment block that
**	describs the error and what it means both internally and practically.
**
**	For VW all error parameters are strings so the %0c should be the only
**	one used.
**
** History:
**
**	03-Mar-2010 (kiria01)
**	    Created
*/

/* Should be same value as DB_VW_ID */
#define _VW_CLASS 20


E_VW0000_OK:SS00000_SUCCESS
"VWF successful return status."

/*************************************************************************
**									**
**	Numbers 0100 - 0FFF have an error severity level of WARNING	**
**									**
*************************************************************************/

E_VW0FFF_NOT_IMPLEMENTED_YET:SS5000B_INTERNAL_ERROR
"VWF function has not been implemented yet."

/*************************************************************************
**									**
**	Numbers 1000 - 8FFF have an error severity level of ERROR	**
**									**
*************************************************************************/
E_VW1001_INTDIVBYZERO:SS22012_DIVISION_BY_ZERO
"Integer divide by zero detected in query."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operation VWF was asked to perform resulted in a division where
**	the divisor was a number with a zero value.  VWF was set to generate
**	errors for all arithmetic exceptions, so the query was immediately
**	aborted.
**
**	The query was aborted.
**
** Recommendation:
**	If this error was unexpected, it may be occurring because a
**	small value was truncated to zero while being coerced or converted
**	to a packed decimal number with insufficient scale to hold this small
**	number.  Using the decimal() conversion function to convert the
**	intermediate result to a value with more scale digits might help.
**
**	Alternative handling for arithmetic exceptions is available and fully
**	explained in the documentation set.
*/
E_VW1002_INTOUTOFRANGE:SS22003_NUM_VAL_OUT_OF_RANGE
"Integer '%0c' out of range."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operation VWF was asked to perform resulted in an expression or
**	sub-expression whose result exceeded the available precision in the
**	result value.
**
**	VWF was set to generate errors for all arithmetic exceptions, so this
**	query will be aborted.
**
** System Status:
**	The query was aborted.
**
** Recommendation:
**	If this error was unexpected, try simplifying the expression which
**	is generating this overflow until you can reproduce the problem with as
**	few variables, column names, arithmetic operators, etc. as possible.
**	Then using the formulas given in the documentation, calculate
**	the result size for your simplified expression.  It should become clear
**	why the expression is overflowing.
*/
E_VW1003_FLTDIVBYZERO:SS22012_DIVISION_BY_ZERO
"Floating point divide by zero detected in query."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operation VWF was asked to perform resulted in a division where
**	the divisor was a number with a zero value.  VWF was set to generate
**	errors for all arithmetic exceptions, so the query was immediately
**	aborted.
**
** System Status:
**	The query was aborted.
**
** Recommendation:
**	If this error was unexpected, it may be occurring because a
**	small value was truncated to zero while being coerced or converted
**	to a packed decimal number with insufficient scale to hold this small
**	number.  Using the decimal() conversion function to convert the
**	intermediate result to a value with more scale digits might help.
**
**	Alternative handling for arithmetic exceptions is available and fully
**	explained in the documentation set.
*/
E_VW1004_FLTOVERFLOW:SS22003_NUM_VAL_OUT_OF_RANGE
"Floating overflow detected in query."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operation VWF was asked to perform resulted in an expression or
**	sub-expression whose result exceeded the available precision in the
**	result value.
**
**	VWF was set to generate errors for all arithmetic exceptions, so this
**	query will be aborted.
**
** System Status:
**	The query was aborted.
**
** Recommendation:
**	If this error was unexpected, try simplifying the expression which
**	is generating this overflow until you can reproduce the problem with as
**	few variables, column names, arithmetic operators, etc. as possible.
**	Then using the formulas given in the documentation, calculate
**	the result size for your simplified expression.  It should become clear
**	why the expression is overflowing.
*/
E_VW1005_FLTUNDERFLOW:SS22003_NUM_VAL_OUT_OF_RANGE
"Floating underflow detected in query."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operation VWF was asked to perform resulted in an expression or
**	sub-expression whose non-zero result exceeded the available precision in the
**	result value but was stored as 0.
**
**	VWF was set to generate errors for all arithmetic exceptions, so this
**	query will be aborted.
**
** System Status:
**	The query was aborted.
**
** Recommendation:
**	If this error was unexpected, try resarranging the expression which
**	is generating this underflow as it may be possible to compute the correct
**	result in a more accurate manner without loosing significant precision.
*/
E_VW1006_GENERIC:SS5000B_INTERNAL_ERROR
"Generic internal error in VectorWise server"
E_VW1007_ERR_OOM:SS5000B_INTERNAL_ERROR
"VectorWise server out of memory"
E_VW1008_QRY_BAD_DT_ERROR:SS5000B_INTERNAL_ERROR
"An unhandled datatype was passed: dt=%0c qry=%1c"
E_VW1009_QRY_SYN_ERR:SS42000_SYN_OR_ACCESS_ERR
"Ingres-VW syntax error: %0c"
E_VW1010_BKLD_STATE_ERROR:SS5000B_INTERNAL_ERROR
"Bulk load not initialialised"
E_VW1011_BKLD_PUSH_DATA_ERROR:SS5000B_INTERNAL_ERROR
"Bulk load push data failed: status=%0c"
E_VW1012_SIMPLE_QRY_ERROR:SS5000B_INTERNAL_ERROR
"Simple query failed during execution: qry=%0c"
E_VW1013_PKEY_ERROR:SS5000B_INTERNAL_ERROR
"Error listing primary key candidates: db=%0c table=%1c"
E_VW1014_FKEY_ERROR:SS5000B_INTERNAL_ERROR
"Error listing foriegn key candidates: db=%0c table=%1c"
E_VW1015_KILL:SS5000B_INTERNAL_ERROR
"Killed. Received SIGINT or SIGKILL"
E_VW1016_PARSE:SS42000_SYN_OR_ACCESS_ERR
"Parse error in %0c at line %1c, column %2c: %3c."
E_VW1017_INVALIDDOMAIN:SS50000_MISC_ERRORS
"Argument outside of domain of math function."
E_VW1018_INVALIDNUMERIC:SS42000_SYN_OR_ACCESS_ERR
"String '%0c' cannot be converted to numeric."
E_VW1019_INVALIDTIME:SS22007_INV_DATETIME_FMT
"'%0c' is not a valid time value."
E_VW1020_INVALIDTIMESTAMP:SS22007_INV_DATETIME_FMT
"'%0c' is not a valid timestamp value."
E_VW1021_INVALIDINTERVALYM:SS22007_INV_DATETIME_FMT
"'%0c' is not a valid value for interval year to month."
E_VW1022_INVALIDINTERVALDS:SS22007_INV_DATETIME_FMT
"'%0c' is not a valid value for interval day to second."
E_VW1023_INVALIDDATE:SS22007_INV_DATETIME_FMT
"'%0c' is not a valid date/time value."
E_VW1024_INVALIDMONEY:SS22000_DATA_EXCEPTION
"'%0c' is not a valid money value."
E_VW1025_INVALIDBOOL:SS22000_DATA_EXCEPTION
"'%0c' is not a valid boolean value."
E_VW1026_CAST:SS22018_INV_VAL_FOR_CAST
"Error casting '%0c' from type '%1c' to type '%2c'."
E_VW1027_REWRITER:SS5000B_INTERNAL_ERROR
"Rewriter error."
E_VW1028_BUILDER:SS5000B_INTERNAL_ERROR
"Error building operator tree."
E_VW1029_OPERATORTREE:SS5000B_INTERNAL_ERROR
"Error in operator tree."
E_VW1030_INVALIDSYSCALL:SS5000B_INTERNAL_ERROR
"Invalid system call '%0c'."
E_VW1031_COMMIT:SS50000_MISC_ERRORS
"Error committing transaction."
E_VW1032_INSUNIQUECONSVIOLAT:SS23501_UNIQUE_CONS_VIOLATION
"Duplicate unique key detected."
E_VW1033_PKDELVIOLATION:SS23504_REF_PK_DEL_VIOLATION
"Cannot DELETE because the values are still used."
E_VW1034_FKINSVIOLATION:SS23502_REF_FK_INS_VIOLATION
"Cannot INSERT because the values do not match referenced table."
E_VW1035_CONNECTION:SS08006_CONNECTION_FAILURE
"Cannot connect to VectorWise server."
E_VW1036_NETWORK:SS08000_CONNECTION_EXCEPTION
"VectorWise server network communication error."
E_VW1037_INTERNAL:SS5000B_INTERNAL_ERROR
"Internal VectorWise error: %0c."
E_VW1038_CHECKPOINT:SS5000B_INTERNAL_ERROR
"Combine/checkpointing failed."
E_VW1039_SINGLEWRITER:SS5000B_INTERNAL_ERROR
"Failed to hold single writer lock - abort write query."
E_VW1040_TABLE_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Table '%0c' already exists."
E_VW1041_TABLE_NOT_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Table '%0c' does not exist."
E_VW1042_KEY_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Key '%0c' already exists."
E_VW1043_KEY_NOT_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Key '%0c' on table '%1c' does not exist."
E_VW1044_INDEX_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Index '%0c' already exists."
E_VW1045_INDEX_NOT_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Index '%0c' on table '%1c' does not exist."
E_VW1046_CS_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Columnspace '%0c' already exists."
E_VW1047_CS_NOT_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Columnspace '%0c' does not exist."
E_VW1048_COL_NOT_FOUND:SS42000_SYN_OR_ACCESS_ERR
"Column '%0c' in table '%1c', not found."
E_VW1049_HAS_PRIMARY:SS42000_SYN_OR_ACCESS_ERR
"Table '%0c' already has primary index."
E_VW1050_INCORRECT_REF_KEY:SS42000_SYN_OR_ACCESS_ERR
"Incorrect referenced key type."
E_VW1051_JOIN_NULLABLE:SS42000_SYN_OR_ACCESS_ERR
"Join index on nullable columns not allowed."
E_VW1052_KEY_NON_EMPTY:SS42000_SYN_OR_ACCESS_ERR
"Key on non-empty table not allowed."
E_VW1053_IDX_NON_EMPTY:SS42000_SYN_OR_ACCESS_ERR
"Index on non-empty table not allowed."
E_VW1054_IDX_TYPE_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Table '%1c' already has %0c index."
E_VW1055_SPARSE_JOIN_INV_COL:SS42000_SYN_OR_ACCESS_ERR
"First column in sparse index should be join index, if one is defined."
E_VW1056_ERR_DML_JI_NOT_SUP:SS42000_SYN_OR_ACCESS_ERR
"DML operations on tables with join indexes not supported."
E_VW1057_WRITE:SS5000B_INTERNAL_ERROR
"Failed writing block."
E_VW1058_WRITELOG:SS5000B_INTERNAL_ERROR
"Failed writing log."
E_VW1059_WRITESORT:SS5000B_INTERNAL_ERROR
"Failed writing during disk spilling sort."
E_VW1060_WRITECHECKPOINT:SS5000B_INTERNAL_ERROR
"Failed writing during checkpointing."
E_VW1061_READ:SS5000B_INTERNAL_ERROR
"Failed reading block."
E_VW1062_READLOG:SS5000B_INTERNAL_ERROR
"Failed reading log."
E_VW1063_READSORT:SS5000B_INTERNAL_ERROR
"Failed reading during disk spilling sort."
E_VW1064_CHECKPOINTDELIMITER:SS5000B_INTERNAL_ERROR
"Unexepected delimiter '%0c' at position %1."
E_VW1065_CHECKPOINTTABLE:SS5000B_INTERNAL_ERROR
"Table '%0c' not found in the vectorwise database."
E_VW1066_CHECKPOINTINSMAIN:SS5000B_INTERNAL_ERROR
"Table '%0c' cannot be inserted because it is the main table."
E_VW1067_CHECKPOINTEXISTS:SS5000B_INTERNAL_ERROR
"Table '%0c' cannot be used multiple times in one system call."
E_VW1068_TABLESTRUCT:SS5000B_INTERNAL_ERROR
"Table '%0c' and table '%1c' not equal in structure."
E_VW1069_APPEND_IDX_NOT_SUP:SS42000_SYN_OR_ACCESS_ERR
"Appending to non empty tables with indexes not supported."
E_VW1070_UPDUNIQUECONSVIOLAT:SS23501_UNIQUE_CONS_VIOLATION
"Duplicate unique key detected."
E_VW1071_FKUPDVIOLATION:SS23503_REF_FK_UPD_VIOLATION
"Cannot UPDATE because the new values do not match referenced table."
E_VW1072_PKUPDVIOLATION:SS23505_REF_PK_UPD_VIOLATION
"Cannot UPDATE because the old values are still used."
E_VW1073_CHECKPOINTNOSPACE:SS5000B_INTERNAL_ERROR
"Combine/checkpointing cannot be executed because not enough free space in the columnspace."
E_VW1074_DROP_INC_JI:SS23504_REF_PK_DEL_VIOLATION
"Cannot drop table with incoming join index."
E_VW1075_DATE_DIV_ANSI_INTV:SS42000_SYN_OR_ACCESS_ERR
"Cannot mix ANSI intervals in division."
E_VW1076_INVALID_ARG:SS5000B_INTERNAL_ERROR
"Invalid argument type for operation '%0c'."
E_VW1077_UNICODE_CONVERSION:SS5000B_INTERNAL_ERROR
"Unicode conversion failed."
E_VW1078_NO_TYPE_CONVERSION:SS5000B_INTERNAL_ERROR
"No conversion defined for result data type."
E_VW1079_DECOVF_ERROR:SS22003_NUM_VAL_OUT_OF_RANGE
"Packed decimal overflow."
E_VW1080_INVALID_EXTRACT:SS42000_SYN_OR_ACCESS_ERR
"Invalid parameters to extract."
E_VW1081_INTOVERFLOW:SS22003_NUM_VAL_OUT_OF_RANGE
"Integer overflow detected in query."
E_VW1082_NULL_TO_NONNULL:SS5000A_QUERY_ERROR
"An attempt to place a null value in a non-nullable datatype."
E_VW1083_UPD_IDX_NOT_SUP:SS42000_SYN_OR_ACCESS_ERR
"Updating indexed attributes is not supported."
E_VW1084_NO_ACCESS_TO_TABLE:SS42000_SYN_OR_ACCESS_ERR
"You do not have any privilege to access table '%0c'"
E_VW1085_ADCOL_UPD_CONFLICT:SS5000B_INTERNAL_ERROR
"Cannot add or drop column for tables with in-memory updates. Please checkpoint the table first."
E_VW1086_COLUMN_EXISTS:SS42000_SYN_OR_ACCESS_ERR
"Column '%0c' already exists."
E_VW1087_CHECKPOINT_APPEND:SS5000B_INTERNAL_ERROR
"Combine/checkpointing failed on appending data."
E_VW1088_CHECKPOINT_CONSTRAINT:SS5000B_INTERNAL_ERROR
"Combine/checkpointing failed on constraint violation."
E_VW1089_COMBINE_START_FRESH:SS42000_SYN_OR_ACCESS_ERR
"The current transaction contains uncommitted changes, to continue this operation first commit or abort."
E_VW1090_DPCOL_IDX_CONFLICT:SS5000B_INTERNAL_ERROR
"Column cannot be dropped because of indices, drop index first before dropping column."
E_VW1091_JI_PARENT_MODIFY:SS5000B_INTERNAL_ERROR
"Cannot modify sort key of joinindex parent."
E_VW1092_BLK_VERSION:SS5000B_INTERNAL_ERROR
"Block version verification failed. Data is not compatible with this release of database."
E_VW1093_INVALIDSYSCALL_ARGS:SS5000B_INTERNAL_ERROR
"Invalid system call arguments."
E_VW1094_WRITE_LOCK_OTHERS:SS5000B_INTERNAL_ERROR
"Failed to acquire global write lock. Abort or commit all concurrent update transactions and try again."
E_VW1095_WRITE_LOCK_SELF:SS5000B_INTERNAL_ERROR
"There are uncommitted changes in this transaction, abort or commit to continue."
E_VW1096_WRITE_LOCK_HOLD:SS5000B_INTERNAL_ERROR
"Global write lock is being held. Try to unlock it first."
E_VW1097_CYCLIC_FK_NOT_SUP:SS5000B_INTERNAL_ERROR
"Cyclic foreign key references are not allowed."
E_VW1098_SERVER_LIMIT:SS08006_CONNECTION_FAILURE
"Active VectorWise databases limit reached."
E_VW1099_SERVER_START:SS08006_CONNECTION_FAILURE
"Cannot start VectorWise server."

E_VW1101_CONFL_CREATE_DUP_NAME:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict creating table - table name already in use."
E_VW1102_CONFL_DROPTBL_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict dropping table - table dropped or table schema has changed."
E_VW1103_CONFL_CREATEKEY_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict creating key - table dropped or table schema has changed."
E_VW1104_CONFL_DROPKEY_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict dropping key - table dropped or table schema has changed."
E_VW1105_CONFL_CREATEIDX_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict creating index - table dropped or table schema has changed."
E_VW1106_CONFL_DROPIDX_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict dropping index - table dropped or table schema has changed."
E_VW1107_CONFL_ADDCOL_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict adding column - table dropped or table schema has changed."
E_VW1108_CONFL_DROPCOL_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict dropping column - table dropped or table schema has changed."
E_VW1109_CONFL_APPEND_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict appending to table - table dropped or table schema has changed."
E_VW1110_CONFL_UPDATE_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict updating table - table dropped or table schema has changed."
E_VW1111_CONFL_CKT_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict checkpointing - table dropped or table schema has changed."
E_VW1112_CONFL_COMB_DDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict combining - table dropped or table schema has changed."
E_VW1113_CONFL_DROPTBL_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict dropping table - concurrent append or update."
E_VW1114_CONFL_CREATEKEY_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict creating key - concurrent append or update."
E_VW1115_CONFL_DROPKEY_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict dropping key - concurrent append or update."
E_VW1116_CONFL_CREATEIDX_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict creating index - concurrent append or update."
E_VW1117_CONFL_DROPIDX_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict dropping index - concurrent append or update."
E_VW1118_CONFL_ADDCOL_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict adding column - concurrent append or update."
E_VW1119_CONFL_DROPCOL_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict dropping column - concurrent append or update."
E_VW1120_CONFL_APPEND_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict appending to table - concurrent append or update."
E_VW1121_CONFL_UPDATE_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict updating table - concurrent append."
E_VW1122_CONFL_CKT_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict checkpointing - concurrent append."
E_VW1123_CONFL_COMB_DML:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict combining - concurrent append or update."
E_VW1124_CONFL_CREATEFK_REFDDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict creating foreign key - referenced table dropped or table schema has changed."
E_VW1125_CONFL_CREATEJI_REFDDL:SS40001_SERIALIZATION_FAIL
"Error committing transaction: conflict creating joinindex - referenced table dropped or table schema has changed."
E_VW1126_CONFL_UPDATE_UPDATE:SS40001_SERIALIZATION_FAIL
"Error committing transaction: updates conflict with updates in another transaction."
E_VW1127_CONFL_UNIQ:SS40002_CONSTR_VIOLATION
"Error committing transaction: unique constraint conflict with another transaction."
E_VW1128_CONFL_FK:SS40002_CONSTR_VIOLATION
"Error committing transaction: foreign key constraint conflict with another transaction."
E_VW1129_MNYOVF_ERROR:SS22003_NUM_VAL_OUT_OF_RANGE
"Exceeded the maximum money value allowed."
E_VW1130_MNYUDF_ERROR:SS22003_NUM_VAL_OUT_OF_RANGE
"Exceeded the minimum money value allowed."
E_VW1131_CONFL_UPDATE_CKT:SS40001_SERIALIZATION_FAIL
"Error committing transaction: updates conflict with recently committed background checkpointing process."
/*************************************************************************
**									**
**	Numbers 9000 - FFFF have an error severity level of FATAL	**
**									**
*************************************************************************/
E_VW9997_UNMAPPED_ERROR:SS5000B_INTERNAL_ERROR
"An unmapped VWF error has been detected (%0c)."
/*%
** Parameters:
**	%x - hex value of status received.
**
** Explanation:
**	The VW Facility tried to report an unmapped error.
**
** System Status:
**	Depending on what the error is, the operation, query, or session
**	may be aborted.
**
** Recommendation:
**	This error usually means that a bug exists, and should be reported to
**	Actian Corporation Technical Support.  Be sure to report the entire
**	error message exactly.
*/
E_VW9998_INTERNAL_ERROR:SS5000B_INTERNAL_ERROR
"A VWF internal coding error has been detected (%0c)."
/*%
** Parameters:
**	%0c - a string tag indicating where the error was detected.
**
** Explanation:
**	The VW Facility found itself in an unexpected state.  This is usually
**	a bug in VWF itself, and should be reported.
**
** System Status:
**	Depending on what the error is, the operation, query, or session
**	may be aborted.
**
** Recommendation:
**	This error usually means that a bug exists, and should be reported to
**	Actian Corporation Technical Support.  Be sure to report the entire
**	error message exactly.
*/
E_VW9999_INTERNAL_ERROR:SS5000B_INTERNAL_ERROR
"A VWF internal coding error has been detected."
/*%
** Parameters:
**	none
**
** Explanation:
**	The VW Facility found itself in an unexpected state.  This is usually
**	a bug in VWF itself, and should be reported.
**
** System Status:
**	Depending on what the error is, the operation, query, or session
**	may be aborted.
**
** Recommendation:
**	This error usually means that a bug exists, and should be reported to
**	Actian Corporation Technical Support.  Be sure to report the entire
**	error message exactly.
*/

NO_SQLSTATE_MAPPING
