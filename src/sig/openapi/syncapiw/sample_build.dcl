$ set noon
$!
$! compile the OpenAPI Synchronous wrappers library
$!
$ @ii_system:[ingres.sig.syncapiw]syncapiw.com
$!
$! compile the sample test api program (tstsapi.c)
$!
$ CFLAGS := "/FLOAT=IEEE_FLOAT"
$ cc'CFLAGS'/include=(ii_system:[ingres.files], -
        ii_system:[ingres.sig.syncapiw],sys$library) -
	testsapi
$!
$! Build the application:
$!	sample program (tstsapi and the OpenAPI Synchronous wrappers library
$!	
$ link testsapi, syncapiw/lib,-
  ii_system:[ingres.files]iiapi.opt/opt
$!
$! purge the area
$!
$ purge/nolog ii_system:[ingres.sig.syncapiw]
$!
$ set on
$ exit
