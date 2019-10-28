
	Ingres OpenAPI Synchronous Wrappers Library
	-------------------------------------------

This SIG directory contains the source code to a set of functions
that facilitate the writing of Ingres OpenAPI applications. Most
of the functions use a synchronous mode, i.e., they call
IIapi_wait and wait for the preceding OpenAPI call to complete.


Overview
--------
The functions deal with most, but not all, of the functionality
available through OpenAPI. Here are the major areas covered:

-  API initialization and termination
-  Establishment and termination of connections to the DBMS
-  Transaction control, including support for two phase commit
-  Queries, including support for singleton SELECTs and SELECT
   loops, repeated queries and LONG VARCHAR and LONG BYTE
   datatypes
-  Execution of database procedures
-  Issuance and retrieval of database events

The major area that is missing is the use of cursors for
fetching rows from the database.

In addition to the functions, macros are provided to facilitate
the initialization of OpenAPI data structures prior to calling
the wrapper functions.


Header Files
------------
Two header files are provided, sapiw.h and sapiwi.h. The first
one contains the helper macros and function prototypes for the
public interface functions. It can be placed together with the
OpenAPI iiapi.h header, in the installation "files" directory, 
or in some other project-wide directory for inclusion by the
modules that will call the wrapper library. The second header is
for internal use and can remain in the SIG directory with the
other source files.


Database Connections
--------------------
The following functions are provided:

IIsw_connect:  Open connection to a specified database
IIsw_dbConnect:  Connect to a database as a given user
IIsw_disconnect:  Disconnect from a database
IIsw_setConnParam:  Set an II_CHAR miscellaneous connection
    parameter


Transaction Control
-------------------
The following functions are provided:

IIsw_commit:  Commit a transaction
IIsw_rollback:  Roll back a transaction
IIsw_registerXID:  Register a two phase commit transaction ID
IIsw_prepareCommit:  Begin a two phase commit transaction
IIsw_releaseXID:  Release a two phase commit transaction ID


Queries
-------
The following functions are available:

IIsw_query:  Simple query (INSERT, UPDATE, CREATE, etc.), with or
    without parameters, including support for blob datatypes
IIsw_queryRepeated:  Repeated simple query
IIsw_selectSingleton:  Singleton SELECT, including blob support
IIsw_selectSingletonRepeated:  Repeated singleton SELECT
IIsw_selectLoop:  Initiate or continue a SELECT loop, including
    blob support
IIsw_selectLoopRepeated:  Repeated SELECT loop
IIsw_queryQry:  Generic query call, primarily for internal use


Database Procedures
-------------------
The following functions are available:

IIsw_execProcedure:  Execute a database procedure
IIsw_execProcDesc:  Execute a procedure that returns a tuple.


Database Events
---------------
The following functions are provided:

IIsw_raiseEvent:  Raise an event
IIsw_catchEvent:  Prepare to catch an event
IIsw_waitEvent:  Wait for an event to arrive
IIsw_getEventInfo:  Get event text
IIsw_cancelEvent:  Cancel the catching of an event


Miscellaneous
-------------
The following functions are provided:

IIsw_initialize:  Initialize the API
IIsw_terminate:  Terminate the API
IIsw_close:  Close a query, database procedure or event retrieval
IIsw_getQueryInfo:  Get query status information
IIsw_getErrorInfo:  Get error status information
IIsw_getRowCount:  Get affected row count
IIsw_getColumns:  Get columns, primarily for internal use


Building and Using the Library
------------------------------
A sample makefile may be provided for your platform. The library
is built by simply compiling the six source files and adding the
resulting objects to an archive or library file.

To use the library, you include the sapiw.h header file in your
module and then link your executable with the library file.
