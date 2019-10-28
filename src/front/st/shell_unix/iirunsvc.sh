:
#
# Copyright Ingres Corporation 2011. All Rights Reserved.
#
#
#  Name: iirunsvc -- start/stop named service from rc/upstart subsystem
#
#  Usage:
#       iirunsvc service action
#
#  Description:
#      Wrapper script run from RC or Upstart script to start and stop
#      Ingres processes as service. iimgmtsvc or ingsvc
#
#  Exit Status:
#       0       operation succeeded
#       1       generic error
#	2	failed to set environment
#	3	action failed
#
##  DEST = utility
## History:
##	12-Sep-2011 (hanje04)
##	    Created.
##	04-Oct-2011 (hanje04)
##	    Bug 125688
##	    Update remote manager service (iimgmtsvc) control functions
##	    to use new scripts. Correct name

self=`basename $0`
dirname=`dirname $0`
instloc=""
action=""
service=""
arglist=""

usage()
{
    cat << EOF | fmt -w 70 -s
usage:
    ${self} service action

     where:
        service - service to be manipulated (mgmtsvc or ingres)
	action - start, stop, restart, status, configure [respfile]

    Note: Utility must be run using full path

EOF
    exit 1
}

start_mgmtsvc()
{
    if [ -x $II_SYSTEM/ingres/utility/iimgmtsvrstart ]
    then
	$II_SYSTEM/ingres/utility/iimgmtsvrstart
	rc=$?
    else
	rc=3
    fi
    return $rc
}

start_ingres()
{
    if [ -x $II_SYSTEM/ingres/utility/ingstart ]
    then
	$II_SYSTEM/ingres/utility/ingstart > /dev/null 2>&1
	rc=$?
    else
	rc=3
    fi
    return $rc
} 

stop_mgmtsvc()
{
    if [ -x $II_SYSTEM/ingres/utility/iimgmtsvrstop ]
    then
	$II_SYSTEM/ingres/utility/iimgmtsvrstop
	rc=$?
    else
	rc=3
    fi
    return $rc
}

stop_ingres()
{
    if [ -x $II_SYSTEM/ingres/utility/ingstop ]
    then
	$II_SYSTEM/ingres/utility/ingstop -kill > /dev/null 2>&1
	rc=$?
    else
	rc=3
    fi
    return $rc
} 

status_mgmtsvc()
{
    if [ -x $II_SYSTEM/ingres/bin/iimgmtsvr ]
    then
        # set java home
        JAVA_HOME=`$II_SYSTEM/ingres/bin/ingprenv II_MTS_JAVA_HOME`
        if [ -z "$JAVA_HOME" ] ; then
	    rc=2
	else
	    export JAVA_HOME
	    $II_SYSTEM/ingres/bin/iimgmtsvr isremoterunning > /dev/null 2>&1
	    rc=$?
	fi
    else
	rc=3
    fi
    return $rc
} 

status_ingres()
{
    if [ -x $II_SYSTEM/ingres/utility/ingstatus ]
    then
	$II_SYSTEM/ingres/utility/ingstatus
	rc=$?
    else
	rc=3
    fi
    return $rc
} 

syscheck_mgmtsvc()
{
    /bin/true
}

syscheck_ingres()
{
    if [ -x $II_SYSTEM/ingres/utility/syscheck ]
    then
	$II_SYSTEM/ingres/utility/syscheck
	rc=$?
    else
	rc=3
    fi
    return $rc
} 

configure_mgmtsvc()
{
    setupcmd=$II_SYSTEM/ingres/utility/iisumgmtsvc
    rc=0
    # check for response file
    if [ "$1" ] ; then
        if [ -r "$1" ] ; then
	    respfile=$1
        else
	    echo "$1 is not a valid response file"
	    rc=3
        fi
    else
	echo "No response file, cannot run setup"
        rc=2
    fi

    if [ $rc -eq 0 -a -x $setupcmd ]
    then
        $setupcmd -exresponse ${respfile:-}
        rc=$?
    fi
    return $rc
    
}

configure_ingres()
{
    # need to be root
    [ `id -u` = 0 ] ||
    {
	echo "Error: Must be root to run configure"
	return 3
    }
    respfile=''
    presetupcmd=$II_SYSTEM/ingres/utility/iiposttrans
    setupcmd=$II_SYSTEM/ingres/utility/setupingres
    # check for response file
    if [ "$1" ] ; then
        if [ -r "$1" ] ; then
	    respfile=$1
        else
	    echo "$1 is not a valid response file"
	    rc=3
        fi
    fi
    $presetupcmd
    if [ $? = 0 ]
    then
        $setupcmd ${respfile:-}
        rc=$?
    fi
    return $rc
}

# main body of script starts here
while [ "$1" ]
do
    case "$1" in
     ingres|\
     mgmtsvc)
	    service=${1}
	    shift
	    ;;
      start|\
       stop|\
     status|\
   syscheck|\
   configure)
	    action=${1}
	    if [ ${action} = "configure" ]
	    then
		arglist=${2}
		shift
	    fi
	    shift
	    ;;
           *)
	    usage
	    ;;
    esac
done


# Full path of exe is used to determine II_SYSTEM so bail if it's not.
if `echo $dirname | grep -q ^/` ; then
   # derive II_SYSTEM from location of exe, II_SYSTEM/ingres/utility
   dirname=`dirname ${dirname}` # II_SYSTEM/ingres
   instloc=`dirname ${dirname}` # II_SYSTEM
fi
[ -z "${instloc}" ] || [ -z "${service}" ] || [ -z "${action}" ] && usage

# set environment and everything from $II_SYSTEM/ingres so CWD is writable
export II_SYSTEM=${instloc}
cd $II_SYSTEM/ingres >> /dev/null 2>&1

if [ -x "$II_SYSTEM/ingres/bin/ingprenv" ]
then
    instid=`$II_SYSTEM/ingres/bin/ingprenv II_INSTALLATION`
    [ -z "${instid}" ] &&
    {
        echo "Error: II_INSTALLATION is not set"
        exit 2
    }
    PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
    LD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
    export II_SYSTEM PATH LD_LIBRARY_PATH
else
    echo "Error: Failed to determin II_SYSTEM"
    exit 2
fi

# do work
${action}_${service} ${arglist}
rc=$?
if [ $rc != 0 ] ; then
   echo "Error: Failed to ${action} ${service}"
fi

exit $rc
