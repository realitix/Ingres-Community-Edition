:
#  Copyright (c) 2011 Actian Corporation, All Rights Reserved
#
# Name: iisumgmtsvc.sh
#
# Purpose:
#	Setup program for the Remote Manager Service
#
## History
##	27-Sep-2011 (hanje04)
##	    Created.
##	24-Oct-2011 (hanje04)
##	    Mantis 2796
##	    If java version isn't a.b.c_xx, just use a.b.c to compare
##	    Also, don't follow relative links when determining
##	    JAVA_HOME, it gets too complicated.
#

# DEST = utility

# Initialize variables

. iishlib
. iisysdep

WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
RPM=false
REMOVE=false

# Process command line args
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
elif [ "$1" = "-mkresponse" ] ; then
   WRITE_RESPONSE=true
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
   if [ "$2" ] ; then
        RESOUTFILE="$2"
   fi
elif [ "$1" = "-exresponse" ] ; then
   READ_RESPONSE=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
   if [ "$2" ] ; then
        RESINFILE="$2"
   fi
elif [ "$1" = "-rpm" ] ; then
   RPM=true
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
elif [ "$1" = "-rmpkg" ] ; then
   REMOVE=true
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
else
   BATCH=false
   NOTBATCH=true
   INSTLOG="2>&1 | tee -a $II_LOG/install.log"
fi

export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE
DEFAULT_PATH=""
MINVERSION=1.6.0_14

check_java_version()
{
   [ "$1" ] || return 2
   targetvers=$1

   # major version
   reqmaj=`echo $MINVERSION | cut -d. -f1`
   targmaj=`echo $targetvers | cut -d. -f1`
   [ "$reqmaj" -a "$targmaj" ] && [ $targmaj -ge $reqmaj ] || return 2

   # minor version
   reqmin=`echo $MINVERSION | cut -d. -f2`
   targmin=`echo $targetvers | cut -d. -f2`
   [ "$reqmin" -a "$targmin" ] && [ $targmin -ge $reqmin ] || return 2

   # release
   reqrel=`echo $MINVERSION | cut -d. -f3 | cut -d_ -f1`
   targrel=`echo $targetvers | cut -d. -f3 | cut -d_ -f1`
   [ "$reqrel" -a "$targrel" ] && [ $targrel -ge $reqrel ] || return 2

   # release increment if there is one
   if [ "`echo $targetvers | grep '_'`" ] ; then
	reqinc=`echo $MINVERSION | cut -d_ -f2`
	targinc=`echo $targetvers | cut -d_ -f2`
	[ "$reqinc" -a "$targinc" ] && [ $targinc -ge $reqinc ] || return 2
   fi

   return 0
}

check_java()
{
    java=$1
    if [ -x "$java" ] ; then
	# need actual location not link
	while [ -L $java ]
	do
	    newdir=`readlink $java`
	    if echo $newdir | grep -q ^/  
            then
	        java=$newdir
	     else
	         # skip relative links
	         break
	     fi
  	done

        java_version=`$java -version 2>&1 | grep "java version" | cut -d\" -f2`

        check_java_version "$java_version" && return 0
    fi
    return 1
}

find_java()
{
    java=""
    [ -x "${JAVA_HOME}/bin/java" ] && java="${JAVA_HOME}/bin/java" 
    [ -z "$java" ] && java=`which java 2> /dev/null`

    check_java $java
    [ $? = 0 ] && 
    {
	# need JAVA_HOME
	export DEFAULT_PATH=`echo $java | sed -e 's,/bin/java,,'`
	return 0
    }
}

#
# do_iisumgmtsvn - Setup package
#
do_iisumgmtsvc()
{
    echo "Setting up (PROD1NAME) Manager Service"

    check_response_file

    if $WRITE_RESPONSE ; then
   	mkresponse_msg
	rc=$?
    else ## **** Skip everything if we are in WRITE_RESPONSE mode ****
	if [ -x $II_SYSTEM/ingres/bin/iimgmtsvr ]
	then
	    II_MTS_JAVA_HOME=`ingprenv II_MTS_JAVA_HOME`
	    if [ "$II_MTS_JAVA_HOME" ] ; then 
		:
	    else
		if $BATCH ; then
		    if [ "$READ_RESPONSE" = "true" ] ; then
		       respval=`iiread_response II_MTS_JAVA_HOME $RESINFILE` 
		       check_java "${respval}/bin/java"
		       [ $? = 0 ] && II_MTS_JAVA_HOME="$respval"
		    elif [ -d "$DEFAULT_PATH" ] ; then
			II_MTS_JAVA_HOME=${DEFAULT_PATH}
		    fi
		elif [ "$DEFAULT_PATH" ] ; then
		    cat << !

The (PROD1NAME) Remote Manager Service requires a Java Runtime Environment
(JRE), version ${MINVERSION} or later to be installed. A compatible version
has been detected on your machine under:

	$DEFAULT_PATH

!
		    if prompt "Is this the JRE you want to use?" y
			then
			NOT_DONE=false
			II_MTS_JAVA_HOME="$DEFAULT_PATH"
		    else
			while $NOT_DONE
			do  
			    iiechonn "Please enter the location of the JRE you wish to use\n[$DEFAULT_PATH]: "
			    read NEWVALUE junk
			    echo ""
			    [ -z "$NEWVALUE" ] && NEWVALUE=$DEFAULT_PATH
			    if check_java "${NEWVALUE}/bin/java"
			    then 
				NOT_DONE=false
				II_MTS_JAVA_HOME="$NEWVALUE"
			    else
				cat << !
The JRE installed at:

	$NEWVALUE

is not compatible.

!
			    fi
			done
		    fi
		else
		    cat << !

The (PROD1NAME) Remote Manager Service requires a Java Runtime Environment
(JRE), version ${MINVERSION} or later to be installed. This version was not
detected on your machine. The service will not work without a compatible
version of JRE installed.

!
		    NOT_DONE=true
		    while $NOT_DONE
		    do  
			if prompt "Do you wish to specify a JRE to use?" y
			then
			    iiechonn "Please enter the location of the JRE you wish to use: "
			    read NEWVALUE junk
			    echo ""
			    [ -z "$NEWVALUE" ] && check_java $NEWVALUE
			    if [ $? = 0 ]
			    then 
				NOT_DONE=false
				II_MTS_JAVA_HOME="$NEWVALUE"
			    else
				cat << !
The JRE installed at:

	$NEWVALUE

is not compatible with the Remote Manager Service.

!
			    fi
			else
			    NOT_DONE=false
			fi
		    done
		fi
	    fi
	    if [ "$II_MTS_JAVA_HOME" ] ; then
		ingsetenv II_MTS_JAVA_HOME $II_MTS_JAVA_HOME
		cat << !
	      
II_MTS_JAVA_HOME configured as $II_MTS_JAVA_HOME 

!
		    rc=0
	    else
		cat << !

The (PROD1NAME) Remote Manager Service cannot be setup without a compatible
version of JRE installed. Re-run this script once one has been installed.

!
		rc=1
	    fi
	else
	    echo "The (PROD1NAME) Remote Manager Service is not installed"
	    rc=1
	fi
    fi #end WRITE_RESPONSE mode 

}

#
# do_mgmtsvn - Remove package
#
do_rmmgmtsvc()
{
    echo "Removing (PROD1NAME) Remote  Manager Service"
    iimgmtsvrstop && ingunset II_MTS_JAVA_HOME
    echo ""
}

#
# main body of script start here
#

find_java
if $REMOVE ; then
    eval do_rmmgmtsvc $INSTLOG
    rc=$?
    if [ $rc = 0 ] ; then
	iisetres "ii.`iipmhost`.ingstart.*.mgmtsvr" 0
    fi
else
    eval do_iisumgmtsvc $INSTLOG
    rc=$?
    if [ $rc = 0 ] ; then
	iisetres "ii.`iipmhost`.ingstart.*.mgmtsvr" 1
    fi
fi

exit $rc
