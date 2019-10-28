:
#
#  Copyright (c) 2001, 2010 Actian Corporation
#
#  Name:
#	iisuapi -- set-up script for Ingres remote API 
#
#  Usage:
#	iisuapi 
#
#  Description:
#       This script should only be called after the remote API 
#       package is untarred, it will setup the environment variables 
#       and parameters in config.dat file.
#       
#  Exit Status:
#	0	setup procedure completed.
#	1	setup procedure did not complete.
#
## History:
##	25-Mar-2004 (hweho01)
##         This script file is added for remote API.  
##      30-Apr-2004 (hweho01)
##         Fixed the typo error.
##      12-Jul-2004 (hweho01)
##         Fixed another typo error.
##      12-Jul-2004 (hweho01)
##         Change the gcc.*.registry_type from default to peer, 
##         make it consistent with other platform.
##      12-Jul-2004 (hweho01)
##         Set gcf.mech.null.enabled to 'true' and    
##         gcf.security_mechanism to 'null'.
##	19-Aug-2004 (hanje04)
##	   Set ingstart.root.allowed to 'true' so that thin client can be
##	   started as root.
##	21-Sep-2004 (hanje04)
##	    SIR 113101
##	    Remove all trace of EMBEDDED or MDB setup. Moving to separate 
##	    package.
##      04-Nov-2004 (bonro01)
##          Set the SETUID bits in the install scripts because the pax
##          program does not set the bits when run as a normal user.
##	02-Dec-2004 (hanje04)
##	    BUG 113561
##	    Use net.rfm for setup not tng.rfm
##      22-Dec-2004 (nansa02)
##              Changed all trap 0 to trap : 0 for bug (113670).
##      07-Apr-2005 (bonro01)
##          The response file should contain II_CHARSET not II_CHARSETxx
##	    Allow II_CHARSET and II_TIMEZONE_NAME to be set in environment.
##	05-Jan-2007 (bonro01)
##	    Change Getting Started Guide to Installation Guide.
##	02-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##      10-Feb-2011 (maspa05) bug 125022
##          Ensure ING_EDIT is set to something that actually exists
##      26-May-2011 (maspa05)
##          Replaced code to get/set charset with get_set_charset() from
##          iishlib
##
#  PROGRAM = iisuapi
#
#  DEST = utility
#----------------------------------------------------------------------------

. iisysdep
. iishlib
if [ x"$conf_LSB_BUILD" = x"TRUE" ] ; then
    cfgloc=$II_ADMIN
    symbloc=$II_ADMIN
else
    II_CONFIG=$(PRODLOC)/(PROD2NAME)/files
    export II_CONFIG
    cfgloc=$II_CONFIG
    symbloc=$II_CONFIG
fi
export cfgloc symbloc


[ "$VERS" = "hpb_us5" -o "$VERS" = "hp8_us5" ] && [ ! -d "/opt/sna/lib" ] &&
{
	[ -f $II_SYSTEM/ingres/lib/libappc.1 ] && rm $II_SYSTEM/ingres/lib/libappc.1
	[ -f $II_SYSTEM/ingres/lib/libcsv.1 ] && rm $II_SYSTEM/ingres/lib/libcsv.1
	[ -f $II_SYSTEM/ingres/lib/libmgr.sl ] && rm $II_SYSTEM/ingres/lib/libmgr.sl
	ln -s $II_SYSTEM/ingres/lib/libcompat.1.sl $II_SYSTEM/ingres/lib/libappc.1
	ln -s $II_SYSTEM/ingres/lib/libcompat.1.sl $II_SYSTEM/ingres/lib/libcsv.1
	ln -s $II_SYSTEM/ingres/lib/libcompat.1.sl $II_SYSTEM/ingres/lib/libmgr.sl
}

BATCH=false
NOTBATCH=true
INSTLOG="2>&1 | tee -a $II_LOG/install.log"
WRITE_RESPONSE=false
READ_RESPONSE=false
RESOUTFILE=ingrsp.rsp
RESINFILE=ingrsp.rsp
IGNORE_RESPONSE="true"
export IGNORE_RESPONSE

# check for batch flag
if [ "$1" = "-batch" ] ; then
   BATCH=true
   NOTBATCH=false
   INSTLOG="2>&1 | cat >> $II_LOG/install.log"
   shift ;
elif [ "$1" = "-mkresponse" ] ; then
   WRITE_RESPONSE=true
   BATCH=false
   NOTBATCH=true
   if [ "$2" ] ; then
        RESOUTFILE="$2"
        shift ;
   fi
   shift ;
elif [ "$1" = "-exresponse" ] ; then
   READ_RESPONSE=true
   BATCH=true
   NOTBATCH=false
   if [ "$2" ] ; then
        RESINFILE="$2"
        shift ;
   fi
   shift ;
elif [ "$1" = "-vbatch" ] ; then
   BATCH=true
   NOTBATCH=false
   shift;
fi

export WRITE_RESPONSE
export READ_RESPONSE
export RESOUTFILE
export RESINFILE

if [ "$WRITE_RESPONSE" = "true" ] ; then
trap "rm -f ${cfgloc}/config.lck /tmp/*.$$ 1>/dev/null \
2>/dev/null; exit 1" 0 1 2 3 15
fi		

do_setup()
{
echo "Setting up Ingres API networking..."

# create the response file if it does not exist.
check_response_file #check for response files.

iisulock "Ingres API networking setup" || exit 1

#
# Set the SETUID bit for all necessary files extracted from
# the install media.
#
set_setuid

touch_files # make sure symbol.tbl and config.dat exist

if [ "$WRITE_RESPONSE" = "true" ] ; then
     mkresponse_msg
else ## **** Skip everything that we do in WRITE_RESPONSE mode ****

if [ "$II_CONFIG_LOCAL" ] ; then
# create backup of original config.dat - if config.nfs already exists
   if [ -f ${cfgloc}/config.nfs ] ; then
      cp -p ${cfgloc}/config.nfs ${cfgloc}/config.dat
   else
      cp -p ${cfgloc}/config.dat ${cfgloc}/config.nfs
   fi
fi

# grant owner and root all privileges
iisetres ii.$CONFIG_HOST.ingstart.root.allowed true
iisetres ii.$CONFIG_HOST.privileges.user.root \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED
iisetres ii.$CONFIG_HOST.privileges.user.$ING_USER \
   SERVER_CONTROL,NET_ADMIN,MONITOR,TRUSTED

fi #end WRITE_RESPONSE mode

ingsetenv II_TEMPORARY  /tmp


# Get the version string by grab the first line
VERSION=`head -1 $II_SYSTEM/ingres/version.rel`

if [ "$WRITE_RESPONSE" = "false" ] ; then

RELEASE_ID=`echo $VERSION | sed "s/[ ().\/]//g"`

SETUP=`iigetres ii.$CONFIG_HOST.config.net.$RELEASE_ID` || exit 1

[ "$SETUP" = "complete" ] &&
{
   cat << !

The $VERSION version of Ingres API networking has already been
set up on "$HOSTNAME".

!
   $BATCH || pause
   trap : 0
   clean_exit
}

CONFIG_SERVER_HOST=`iigetres ii."*".config.server_host`

SERVER_HOST=`iigetres ii.$CONFIG_SERVER_HOST.gcn.local_vnode`

fi # end of WRITE_RESPONSE mode 

   # configuring Ingres Networking on a client 

      
      cat << !

This procedure will set up the following version Ingres Networking:

	$VERSION

to run on local host:

	$HOSTNAME

This procedure will set up an Ingres networking installation which
will allow you to access (remote or local) Ingres servers.

!
      $BATCH || prompt "Do you want to continue this setup procedure?" y \
         || exit 1


#check for write_response if set skip
    if [ "$WRITE_RESPONSE" = "false" ] ; then

      if [ "$SETUP" != "defaults" ] ; then
         # generate default configuration resources
         cat << !

Generating default configuration...
!
        NET_RFM_FILE=$II_CONFIG/net.rfm

        if iigenres $CONFIG_HOST $NET_RFM_FILE ; then
            iisetres ii.$CONFIG_HOST.config.net.$RELEASE_ID defaults
         else
            cat << !
An error occurred while generating the default configuration.
 
!
            exit 1
         fi
      else
         cat << !

Default configuration generated.
!
      fi
    fi # end WRITE_RESPONSE mode

      # get II_INSTALLATION 
      II_INSTALLATION=`ingprenv II_INSTALLATION`
      if [ "$II_INSTALLATION" ] ; then 
         cat << !
      
II_INSTALLATION configured as $II_INSTALLATION. 
!
      else
         if $BATCH ; then
	    if [ "$READ_RESPONSE" = "true" ] ; then
	       II_INSTALLATION=`iiread_response II_INSTALLATION $RESINFILE` 
	       [ -z "$II_INSTALLATION" ] && II_INSTALLATION="II"
            elif [ "$2" ] ; then
               II_INSTALLATION="$2"
            else
               II_INSTALLATION="II"
            fi
         else
            get_installation_code 
         fi
      fi

   if [ "$WRITE_RESPONSE" = "true" ] ; then
	eval check_response_write II_INSTALLATION $II_INSTALLATION replace
   fi 

   #
   # set charset if not set already
   #
   get_set_charset

   if [ "$WRITE_RESPONSE" = "false" ] ; then
   # get defaults from configuration rules
   for SYMBOL in II_TEMPORARY ING_EDIT 
   do
      DEFAULT=`iigetres ii."*".setup.$SYMBOL`
      VALUE=`ingprenv $SYMBOL`
      if [ -z "$VALUE" ] ; then
         REASON="$SYMBOL not defaulted in configuration rule base."
         MSGTYPE=Error
          

         #
         # for ING_EDIT make sure the default exists and if not then
         #  look for vi on the path
         #
         if [ "$SYMBOL" = "ING_EDIT" -a ! -f $DEFAULT ]
         then
            cat << !

$SYMBOL default, $DEFAULT, does not exist, vi will be used instead.
Checking for vi on PATH.

!
            REASON="vi not found on PATH, $SYMBOL is not set."
            MSGTYPE=Warning

            # 
            # look for vi - ignoring aliases
            #               type gives us a return code if not found
            #               which gives us the path
            #
            unalias vi >/dev/null 2>&1
            type vi > /dev/null 2>&1
            if [ $? -eq 0 ]
            then
               DEFAULT=`which vi`
               echo "vi found at $DEFAULT"
            else
               DEFAULT=""
            fi
         fi

	 if [ "$DEFAULT" ] ; then 
	    DEFAULT=`eval echo $DEFAULT`
            eval "$SYMBOL=`eval echo $DEFAULT`"
	 else
	    cat << !
   
$MSGTYPE: $REASON
       
!
            if [ "$MSGTYPE" = "Error" ]
            then
	       exit 1
            fi
         fi
      else
         eval "$SYMBOL=`eval echo $VALUE`"
      fi
   done
   fi #end WRITE_RESPONSE mode

   # get II_TIMEZONE_NAME value
   [ -z "$II_TIMEZONE_NAME" ] && II_TIMEZONE_NAME=`eval II_ADMIN=$CLIENT_ADMIN ingprenv II_TIMEZONE_NAME`
   if [ "$II_TIMEZONE_NAME" ] ; then 
      cat << !

II_TIMEZONE_NAME configured as $II_TIMEZONE_NAME. 
   
!
   else
      TZ_RULE_MAP=$II_CONFIG/net.rfm
      if $BATCH ; then
	 if [ "$READ_RESPONSE" = "true" ] ; then
            II_TIMEZONE_NAME=`iiread_response II_TIMEZONE_NAME $RESINFILE`
         fi
         [ -z "$II_TIMEZONE_NAME" ] && 
		II_TIMEZONE_NAME=`eval II_ADMIN=${symbloc} \
			ingprenv II_TIMEZONE_NAME`
         #If we still cannot set II_TIMEZONE then this should be set to NA-PACIFIC 
         [ -z "$II_TIMEZONE_NAME" ] && II_TIMEZONE_NAME="NA-PACIFIC"
      else
         get_timezone 
         echo ""
      fi
   fi

   if [ "$WRITE_RESPONSE" = "true" ] ; then
	eval check_response_write II_TIMEZONE_NAME $II_TIMEZONE_NAME replace
   else 
   # set miscellaneous symbols
        eval II_ADMIN=$CLIENT_ADMIN ingsetenv II_TIMEZONE_NAME $II_TIMEZONE_NAME
        eval II_ADMIN=$CLIENT_ADMIN ingsetenv II_INSTALLATION $II_INSTALLATION
        eval II_ADMIN=$CLIENT_ADMIN ingsetenv II_TEMPORARY $II_TEMPORARY

   # ING_EDIT may not be set if the config default/vi was not found
   # only set in symbol.tbl if we have a value
        if [ "$ING_EDIT" ]
        then
             eval II_ADMIN=$CLIENT_ADMIN ingsetenv ING_EDIT $ING_EDIT 
        fi
   fi #end WRITE_RESPONSE mode

# set gcn local_vnode resource

if [ "$WRITE_RESPONSE" = "false" ] ; then
iisetres ii.$CONFIG_HOST.gcn.local_vnode $HOSTNAME

echo "Configuring Net server listen addresses..."

# set default Net server listen addresses
iisetres ii.$CONFIG_HOST.gcc."*".async.port ""
iisetres ii.$CONFIG_HOST.gcc."*".decnet.port II_GCC${II_INSTALLATION}
iisetres ii.$CONFIG_HOST.gcc."*".iso_oslan.port OSLAN_${II_INSTALLATION}
iisetres ii.$CONFIG_HOST.gcc."*".iso_x25.port X25_${II_INSTALLATION}
iisetres ii.$CONFIG_HOST.gcc."*".sna_lu62.port "<none>"
iisetres ii.$CONFIG_HOST.gcc."*".sockets.port $II_INSTALLATION
iisetres ii.$CONFIG_HOST.gcc."*".spx.port $II_INSTALLATION
iisetres ii.$CONFIG_HOST.gcc."*".tcp_ip.port $II_INSTALLATION
iisetres ii.$CONFIG_HOST.gcc."*".tcp_wol.port $II_INSTALLATION
# set up the registry type to peer and enable the protocol
iisetres ii.$CONFIG_HOST.gcf.mech.null.enabled   true
iisetres ii.$CONFIG_HOST.security_mechanism   null
iisetres ii.$CONFIG_HOST.gcc.*.registry_type   peer
iisetres ii.$CONFIG_HOST.gcn.registry_type  peer
iisetres ii.$CONFIG_HOST.registry.tcp_ip.status ON 


fi #end of WRITE_RESPONSE


if [ -f $II_SYSTEM/ingres/bin/mkvalidpw ] ; then
  echo " "
  echo "If shadow passwords are used on this machine, please run mkvalidpw "
  echo "as root to compile the password verification program (ingvalidpw). " 
fi
    
trap : 0
clean_exit
}

eval do_setup $INSTLOG
trap : 0
exit 0
