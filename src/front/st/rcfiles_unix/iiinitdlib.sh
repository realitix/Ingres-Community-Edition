:
# Copyright (c) 2011 Ingres Corporation
#
#
#
#  Name: iiinitlib -- shared functions used by init.d service scripts
#
#  Usage:
#	. iiinitlib 
#
#  Description:
#	Executed by /etc/init.d/ingresXX service scripts when startup is
#	controlled at system level
#
## History:
##	16-Sep-2011 (hanje04)
##	    Created.
##	27-Sep-2011 (hanje04)
##	    Bug 125688
##	    Add control functions for remote manager service

shopt -s expand_aliases
export use_rc=false
unset BASH_ENV
logexit=/bin/true
PATH=${II_SYSTEM?}/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
LD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
export II_SYSTEM PATH LD_LIBRARY_PATH
CONFIG_HOST=`iipmhost`
iiuid=`iigetres "ii.${CONFIG_HOST}.setup.owner.user"`
iigid=`iigetres "ii.${CONFIG_HOST}.setup.owner.group"`
II_USERID=${iiuid:-ingres}
II_GROUPID=${iigid:-ingres}


save_status()
{
    [ "$1" = "-v" ] && { flag=-v ; shift ; }
    if $use_rc ; then
	rc_failed $1
	rc_status $flag
    else
        export STATUS=$1
    fi
}

exit_status()
{
    $logexit
    exit $STATUS
}

# Control Ingres
check_status()
{
    $runuser ${II_USERID} \
       -c "$II_SYSTEM/ingres/utility/iirunsvc ingres status"
}

run_syscheck()
{
    $runuser ${II_USERID} \
       -c "$II_SYSTEM/ingres/utility/iirunsvc ingres syscheck"
}
    
run_ingstart()
{
    $runuser ${II_USERID} \
       -c "$II_SYSTEM/ingres/utility/iirunsvc ingres start"
}
    
run_ingstop()
{
    $runuser ${II_USERID} \
       -c "$II_SYSTEM/ingres/utility/iirunsvc ingres stop"
}

run_setup()
{
     $II_SYSTEM/ingres/utility/iirunsvc ingres configure ${1:-}
}

# Control mgmt tools service
start_iimgmtsvc()
{
    $runuser ${II_USERID} \
       -c "$II_SYSTEM/ingres/utility/iirunsvc mgmtsvc start"
}
    
stop_iimgmtsvc()
{
    $runuser ${II_USERID} \
       -c "$II_SYSTEM/ingres/utility/iirunsvc mgmtsvc stop"
}
    
configure_iimgmtsvc()
{
    $runuser ${II_USERID} \
       -c "$II_SYSTEM/ingres/utility/iirunsvc mgmtsvc configure ${1:-}"
}
    

if [ -f /etc/rc.status ] ; then 
# Shell functions sourced from /etc/rc.status:
#      rc_check         check and set local and overall rc status
#      rc_status        check and set local and overall rc status
#      rc_status -v     ditto but be verbose in local rc status
#      rc_status -v -r  ditto and clear the local rc status
#      rc_failed        set local and overall rc status to failed
#      rc_failed <num>  set local and overall rc status to <num>
#      rc_reset         clear local rc status (overall remains)
#      rc_exit          exit appropriate to overall rc status
    use_rc=true
    . /etc/rc.status
    echon='echo -n'
    exit=rc_exit
    rc_reset
elif [ -f /etc/init.d/functions ] ; then
    . /etc/init.d/functions
    echon=echo
    exit=exit_status
    export STATUS=0
elif [ -f /lib/lsb/init-functions ] ; then
    . /lib/lsb/init-functions
    echon=log_action_msg
    logexit=log_end_msg
    exit=exit_status
fi

# Get command for user switching, user runuser if its there
if [ -x /sbin/runuser ] 
then
    runuser=/sbin/runuser
else
    runuser=/bin/su
fi
export runuser
 
# Return values acc. to LSB for all commands but status:
# 0 - success
# 1 - generic or unspecified error
# 2 - invalid or excess argument(s)
# 3 - unimplemented feature (e.g. "reload")
# 4 - insufficient privilege
# 5 - program is not installed
# 6 - program is not configured
# 7 - program is not running
# 
# Note that starting an already running service, stopping
# or restarting a not-running service as well as the restart
# with force-reload (in case signalling is not supported) are
# considered a success.

