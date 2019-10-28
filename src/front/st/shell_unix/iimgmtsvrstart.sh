:
#
# Copyright (c) 2011 Actian Corporation
#
#
# Name: iimgmtsvrstart	- start the iimgmtsvr server
#
# Description:
#      Wrapper script for starting the iimgmtsvr process.
#      Verifies environment is set correctly before starting the server
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
	# FIX ME!
	# process should be started by iirun
	# and not write to CWD
	# $II_SYSTEM/ingres/utility/iirun iimgmtsvr
	II_CONFIG=`$II_SYSTEM/ingres/bin/ingprenv II_CONFIG`
	rundir=${II_CONFIG:-$II_SYSTEM/ingres/files}
	if [ -d "$rundir" ] ; then
	    cd $rundir
	    ( $II_SYSTEM/ingres/bin/iimgmtsvr isremoterunning ||
    	        $II_SYSTEM/ingres/bin/iimgmtsvr start ) > /dev/null 2>&1
	    rc=$?
	else
	    rc=2
	fi
    fi
else
    rc=3
fi
exit $rc
