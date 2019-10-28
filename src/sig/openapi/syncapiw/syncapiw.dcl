$ set noon
$!
$!   Build file for Synchronous OpenAPI Wrappers library
$!
$! create the syncapiw object library if it doesn't already exist
$!
$ if( f$search( "ii_system:[ingres.sig.syncapiw]syncapiw.olb" ) .eqs. "" )
$ then
$	lib/create ii_system:[ingres.sig.syncapiw]syncapiw.olb
$ endif
$! 
$! compile synchronous wrappers library
$!
$ CFLAGS := "/FLOAT=IEEE_FLOAT"
$ cc'CFLAGS'/include=(ii_system:[ingres.files], -
	ii_system:[ingres.sig.syncapiw],sys$library) -
	apiconn.c, -
	apievent.c, -
	apiproc.c, -
	apiquery.c, -
	apisupp.c, -
	apitran.c
$!
$! insert compiled files into the object library
$!
$ lib/replace syncapiw.olb *.obj
$!
$! remove the object files since they are now in the shared library
$!
$ del/nolog ii_system:[ingres.sig.syncapiw]*.obj;
$!
$ set on
$ exit
