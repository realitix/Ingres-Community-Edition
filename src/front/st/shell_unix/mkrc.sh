:
# Copyright (c) 2004, 2010 Actian Corporation
#
#  Name: mkrc
#
#  Usage:
#       mkrc [-i [123456]] [-r]
#
#  Description:
#       Generates an rc style script under $II_SYSTEM/ingres/files/rcfiles 
#	called:
#
#		ingresXX 	where XX is the installation ID.
#  	
#	-i	Script is installed under $ETCRCFILES. 
#		If 1 or more run level is specified, links to the 
#		corresponding rc directories are created. 
#		If no run level is specified links will be created under
#		the levels defined by $RCDEFLNK.
#		Both ETCRCFILES and RCDEFLNK are defined in iisysdep
#
#	-r 	Remove all RC files and links from $ETCRCFILES for the current
#		installation
#       
#
#  Exit Status:
#       0       ok
#       1       failed to generate file
#	2	failed to install file
#	3	failed to create link
#	4	failed to delete rc files
#
#  PROGRAM = (PROG0PRFX)mkrc
#
#  DEST = utility
## History:
##	09-Mar-2004 (hanje04)
##	    SIR 111952
##	    Created.
##	07-Feb-2005 (hanje04)
##	    Use chkconfig to install into RC system if it's there.
##	06-Apr-2005 (hanje04)
##	    Make sure ETCRCFILES is set when we echo useage message
##	20-May-2006 (hanje04)
##	    BUG 116174
##	    Split up SuSE and RedHat config as then can conflict
##       6-Nov-2006 (hanal04) SIR 117044
##          Add int.rpl for Intel Rpath Linux build.
##	05-Jun-2007 (hanje04)
##	    BUG 119214
##	    Add support for Debian/Ubuntu systems.
##	04-Jan-2008 (hanje04)
##	    BUG 119699
##	    Need to use -f when checking for /etc/debconf.conf.
##	22-Feb-2008 (bonro01)
##	    Add support for Fedora systems.
##	    Fix parameter parsing bug.
##	    Use documented default RC levels 235.
##	    Create correct RC scripts for RedHat, Fedora,
##	    Debian and Ubuntu instead of creating system defaults.
##	14-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##	22-Sep-2011 (hanje04)
##	    Bug 125805
##	    Mantis 2495
##	    Add support for generating and installing different services
##	    based templates. Pre cursor to adding MGMT Tools service script.
##	    Also add support for Upstart.
##	02-Oct-2011 (hanje04)
##	    Bug 125688
##	    Remove extra lines from usage message
##

(LSBENV)

. iisysdep
. iishlib

self=`basename $0`
GENRC=true
INSTALL=false
RMRC=false
SVC=ingres
RCLNK=
USER=`whoami`
CHKCFG=/sbin/chkconfig
CHKINS=/sbin/insserv
UPDRCD=/usr/sbin/update-rc.d
INSTDIR=$ETCRCFILES
UPSTART=false
TMPLTSFX=.rc
OUTSFX=
usage()
{
cat << !

Usage:
    $self [-i [123456]]|[-r] -u -s <service>
  	
	Generates an system service script under: 

		\$II_SYSTEM/ingres/files/rcfiles 

	called:

		<service>XX		- for SYSV RC
	      & <service>XX.conf	- for Upstart

		where <service> is the service name (ingres or iimgmtsvc)
		and XX is the installation ID.
	-i	Install service script in appropriate directory
		If 1 or more run level is specified, it will be enabled
		for just those run levels.
		If no run level is specified it will be enabled for
		the levels defined by $RCDEFLNK.
	-r 	Remove service script
 	-u	Use "upstart" instead of SYSV RC (Defaults to SYSV)
	-s	Specify service type: ingres or iimgmtsvc. (Defaults to ingres)

!
}

# Generate the RC file from the template
genrcfile()
{
    # check we have template and output file
    [ $# -lt 2 ] || [ ! -r "$1" ] || [ -z "$2" ] && return 1

    tmplt="$1"
    outfile="$2"
    shell="/bin/bash"
    echo "#! ${shell}" > $outfile
    
    # need an extra step for Linux
    case $VERS in
	*_lnx|int_rpl)
	    if [ -f /etc/rc.status ] || [ -f /etc/debconf.conf ] ; then
		# SuSE/Debian style RC structure
		cat $tmplt | $SEDCMD | $AWK 'BEGIN{
		    prnt=1 ; skip=0 
		    }
		    $1 == "--REDHAT_CFG" { prnt=0 ; skip=1 }
		    $1 == "--REDHAT_CFG_END" { prnt=1 ; skip=1 }
		    $1 == "--SUSE_CFG" { prnt=1 ; skip=1 }
		    $1 == "--SUSE_CFG_END" { prnt=1 ; skip=1 }
		    prnt == 1 && skip == 0 { print }
		    { skip=0 }' \
			>> $outfile
	    else # RedHat style
		cat $tmplt | $SEDCMD | $AWK 'BEGIN{
		    prnt=1 ; skip=0 
		    }
		    $1 == "--REDHAT_CFG" { prnt=1 ; skip=1 }
		    $1 == "--REDHAT_CFG_END" { prnt=1 ; skip=1 }
		    $1 == "--SUSE_CFG" { prnt=0 ; skip=1 }
		    $1 == "--SUSE_CFG_END" { prnt=1 ; skip=1 }
		    prnt == 1 && skip == 0 { print }
		    { skip=0 }' \
			>> $outfile
	    fi
		;;
	*)	
		cat $tmplt | $SEDCMD >> $outfile
		;;
    esac
    rc=$?
    return $rc	
}

# Check II_SYSTEM is set
[ -z "$II_SYSTEM" ] && {
echo '$II_SYSTEM must be set to run this script'
exit 1
}

CONFIG_HOST=`iipmhost`
tmpid=`iigetres ii.${CONFIG_HOST}.setup.owner.user`
II_USERID=${tmpid:-ingres}
tmpid=`iigetres ii.${CONFIG_HOST}.setup.owner.group`
II_GROUPID=${tmpid:-ingres}

while [ $# != 0 ]
do
    case "$1" in 

    -r) # Remove RC files
	INSTALL=false
	GENRC=false
	RMRC=true
	break
	;;
    -i) # Install previously generated file
	INSTALL=true
	GENRC=false
  	shift
	;;
    -s) # specify alternate service (e.g. iimgmtsvc)
	case $2 in
	    ingres|\
	  iimgmtsvc) SVC=$2
		     shift ; shift
		     ;;
		  *)
		     echo "Error: $2 is not a valid service"
		     usage
		     exit 1
		     ;;
	 esac
	;;
    -u) # use upstart instead of sys5/RC
	UPSTART=true
	TMPLTSFX=.up
	OUTSFX=.conf
	INSTDIR=/etc/init
	shift
	;;
     *)
	if [ "$INSTALL" = "true" ]; then
	    INVALID=`echo $1 | tr -d "123456"`
	    if [ -z "$INVALID" ]; then
		# Run levels under which to install rc scripts
		NRCLNK=`echo $1 | sed "s/1/1 /;s/2/2 /;s/3/3 /;s/4/4 /;s/5/5 /;s/6/6 /"`
		RCLNK="$NRCLNK $RCLNK"
	    else
		usage
		exit 1
	    fi
	else
	    usage
	    exit 1
	fi
	shift
	;;
    esac
	
done

if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    INST_ID=""
    II_SYSTEM=/usr/libexec
    TMPLT=$II_CONFIG/rcfiles/$RCTMPLT
else
    INST_ID=`ingprenv II_INSTALLATION`
    TMPLT=$II_CONFIG/rcfiles/${SVC}${TMPLTSFX}
fi

SVCFNAME=${SVC}${INST_ID}${OUTSFX}
OUTFILE=$II_CONFIG/rcfiles/${SVCFNAME}
SEDCMD="sed -e s,INST_ID,${INST_ID},g -e s,INST_LOC,$II_SYSTEM,g -e s,II_USERID,${II_USERID},g -e s,II_GROUPID,${II_GROUPID},g"


if $RMRC # Remove all RC files
then
    if [ -x ${CHKCFG} ] # Use chkconfig if we can
    then
	${CHKCFG} --del ${SVCFNAME} > /dev/null 2>&1
    elif [ -x ${UPDRCD} ] # Use update-rc.d
    then
	${UPDRCD} -f ${SVCFNAME} remove > /dev/null 2>&1
    else
	for dir in $RC1 $RC2 $RC3 $RC4 $RC5 
	do
	    # Remove startup links
	    [ -L $dir/${RCSTART}${SVCFNAME} ] && {
	       rm -f $dir/${RCSTART}${SVCFNAME}	
	       [ $? != 0 ] && {
	        cat << !

Could not remove:

    $dir/${RCSTART}${SVCFNAME}
!
	        }

	    }
	    # Remove shutdown links
	    [ -L $dir/${RCSTOP}${SVCFNAME} ] && {
	        rm $dir/${RCSTOP}${SVCFNAME}
	        [ $? != 0 ] && {
	            cat << !

Could not remove:

    $dir/${RCSTOP}${SVCFNAME}
!
	        }

	    }
        done
    fi

    # Remove RC file
    [ -f $INSTDIR/${SVCFNAME} ]  && {
	rm -f $INSTDIR/${SVCFNAME}  
        [ $? != 0 ] && {
	    cat << !

Could not remove:

    $INSTDIR/${SVCFNAME}
!
        }
	exit 4

    }
    exit 0

elif $GENRC
then
# Check template file exists
    [ ! -f $TMPLT ] && {
cat << !

Cannot locate RC template file:
	$TMPLT
Aborting...
!
	usage
	exit 1
    }

# Generate file

    genrcfile $TMPLT $OUTFILE
    [ $? != 0 ] && {
	echo "Failed to create $OUTFILE"
	exit 1
    }

    chmod 755 $OUTFILE
    exit 0

elif $INSTALL
then
	
    #Check RC file exists
       [ ! -f $OUTFILE ] && {
    cat << !

Cannot locate RC file:
 $OUTFILE
It can be created by running:
 $0
!
            exit 2
    }
    
    #Check Install location is writeable
    [ ! -w $INSTDIR ] && {
    cat << !

$INSTDIR is not writeable by user "$USER"
Re-run as root.
!
    exit 2
    }
    
    if  ! $UPSTART
    then 
	# Install RC file
	cp $OUTFILE $INSTDIR
	if [ $? != 0 ]
	then
	    cat << !

Failed to copy

    $OUTFILE
    
to

    $INSTDIR
!
	    exit 2
	fi

	chmod 755 $INSTDIR/${SVCFNAME}
    fi
    
    if $UPSTART
    then
	# Copy the conf file into place, enable the service and
	# update the runlevels
	cat $OUTFILE | sed -e "s,^#start on,start on,g"\
			-e "s,RLEVELS,$RCSTART,g" > \
			${INSTDIR}/${SVCFNAME}
	if [ $? != 0 ]
	then
	    cat << EOF
Error: Failed to create Upstart script:

    ${INSTDIR}/${SVCFNAME}

EOF
	    rm -f ${INSTDIR}/${SVCFNAME}
	fi
	exit 2
    elif [ -x ${UPDRCD} ] # Use Upstart
    then
	# ubuntu and debian style RC
        [ -z "$RCLNK" ] && RCLNK="$RCDEFLNK"
	RCLNK2=`echo $RCLNK | tr -d " "`
	RCLNK_STOP=`echo "0 1 2 3 4 5 6" | tr -d "$RCLNK2"`
	RCSTART_NUM=`echo $RCSTART | tr -d "S"`
	RCSTOP_NUM=`echo $RCSTOP | tr -d "K"`
	${UPDRCD} -f ${SVCFNAME} remove > /dev/null 2>&1
	${UPDRCD} ${SVCFNAME} start $RCSTART_NUM $RCLNK . stop $RCSTOP_NUM $RCLNK_STOP . > /dev/null 2>&1
    elif [ -x ${CHKCFG} ] # Use chkconfig if we can
    then
	# SuSE, RedHat and Fedora style RC
	${CHKCFG} --add ${SVCFNAME} > /dev/null 2>&1
        [ -z "$RCLNK" ] && RCLNK="$RCDEFLNK"
	[ "$RCLNK" ] &&
	{
	    #strip spaces
	    RCLNK=`echo $RCLNK | sed -e 's, ,,g'`
	    if [ -x ${CHKINS} ]; then
		# SuSE style RC
		${CHKCFG} ${SVCFNAME} $RCLNK
	    else
		# Redhat and Fedora style RC
		${CHKCFG} ${SVCFNAME} off
		${CHKCFG} --level ${RCLNK} ${SVCFNAME} on
	    fi
	}
    elif [ -f /etc/rc.status ] # SuSE style RC structure
    then
        #If no run levels are specified create the default links
        [ -z "$RCLNK" ] && RCLNK="$RCDEFLNK"
        for run_level in $RCLNK
        do
            LNK_OK=true
            case "$run_level" in
            1) #If RC1 defined create links
	        if [ "$RC1" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		    ${RC1}/${RCSTART}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		    ${RC1}/${RCSTOP}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;
    
            2) #If RC2 defined create links
	        if [ "$RC2" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		        ${RC2}/${RCSTART}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		        ${RC2}/${RCSTOP}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
    	        fi
	        ;;	
    
            3) #If RC3 defined create links
	        if [ "$RC3" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		        ${RC3}/${RCSTART}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		        ${RC3}/${RCSTOP}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		    cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	
    
            4) #If RC4 defined create links
	        if [ "$RC4" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		        ${RC4}/${RCSTART}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		        ${RC4}/${RCSTOP}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	
    
            5) #If RC5 defined create links
	        if [ "$RC5" ] ; then
	            #Startup Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		        ${RC5}/${RCSTART}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            #Shutdown Link
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		    ${RC5}/${RCSTOP}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
    
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	
    
            *) # Any other value
 	        cat << !

$run_level is not a valid RC run level
Please check and try again
!
	        exit 3
	        ;;
    
            esac
        done
    else # Redhat/Solaris style RC structure
	#If no run levels are specified create the default links
        [ -z "$RCLNK" ] && RCLNK="$RCDEFLNK"
	
	#Create all shutdown links appart from the levels want to
	#start Ingres from

	RC1PRFX=$RCSTOP
	RC2PRFX=$RCSTOP
	RC3PRFX=$RCSTOP
	RC4PRFX=$RCSTOP
	RC5PRFX=$RCSTOP

	for run_level in $RCLNK
	do 
	    eval RC${run_level}PRFX=$RCSTART
	done
	
	for run_level in 1 2 3 4 5
	do
	    LNK_OK=true
	    case $run_level in
	    1) #If RC1 defined create links
	 	if [ "$RC1" ] ; then
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		    ${RC1}/${RC1PRFX}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    2) #If RC2 defined create links
	 	if [ "$RC2" ] ; then
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		    ${RC2}/${RC2PRFX}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    3) #If RC3 defined create links
	 	if [ "$RC3" ] ; then
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		    ${RC3}/${RC3PRFX}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    4) #If RC4 defined create links
	 	if [ "$RC4" ] ; then
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		    ${RC4}/${RC4PRFX}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    5) #If RC5 defined create links
	 	if [ "$RC5" ] ; then
	            ln -s ${ETCRCFILES}/${SVCFNAME} \
		    ${RC5}/${RC5PRFX}${SVCFNAME}
	            [ $? != 0 ] && LNK_OK=false
	            # Check success
	            if ! $LNK_OK ; then
		        cat << !

Failed to create links for run level $run_level
!
	            fi
    
                else
	            echo "No location defined for run level $run_level, no links created"
	        fi
	        ;;	

	    *) # Any other value
 	        cat << !

$run_level is not a valid RC run level
Please check and try again
!
	        exit 3
	        ;;
    
            esac
        done

    fi # [ -f /etc/rc.status ]

fi

exit 0
