:
#
# Copyright (c) 2011 Actian Corporation
#
#
# Name: iimgmtsvrstop	- stop the iimgmtsvr server
#
# Description:
#      Wrapper script for stopping the iimgmtsvr process.
#      Verifies environment is set correctly before stopping the server
#
## History:
##      05-Oct-2011 (hanje04)
##          Created.

if [ -x ${II_SYSTEM?}/ingres/bin/iimgmtsvr ]
then
    # set java home
    JAVA_HOME=`$II_SYSTEM/ingres/bin/ingprenv II_MTS_JAVA_HOME`
    if [ x"$JAVA_HOME" = x ] ; then
	rc=2
    else
	export JAVA_HOME
	II_CONFIG=`$II_SYSTEM/ingres/bin/ingprenv II_CONFIG`
	rundir=${II_CONFIG:-$II_SYSTEM/ingres/files}
	if [ -d "$rundir" ] ; then
	    cd $rundir
    	    $II_SYSTEM/ingres/bin/iimgmtsvr stop > /dev/null 2>&1
	    rc=$?
	else
	    rc=2
	fi
    fi
else
    rc=3
fi
exit $rc
