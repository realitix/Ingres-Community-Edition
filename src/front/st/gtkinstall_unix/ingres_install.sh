:
#
# Copyright (c) 2006 Actian Corporation
#
# Name: ingres_install.sh -- startup script for Ingres GUI installer on
#			     Linux
#
# Usage:
#	ingres_install
#
# Description:
#	This script is run by users to install Ingres. It is a wrapper script
#	for the Ingres GUI installer which needs to be run as root.
#
# Exit Status:
#	0	Success
#	1	Failure
#
## History:
##	26-Sep-2006 (hanje04)
##	    SIR 116877
##	    Created.
##	13-Nov-2006 (hanje04)
##	    Create dummy lib directory with links to RPM libraries before
##	    attempting to start the GUI. This is currently the only way
##	    to get the installer to work against versions of RPM other than
##	    then one it was built against. (Currently 4.1)
##	    Also remove use of GUI su tools has setting LD_LIBRARY_PATH
##	    for them is proving problematic.
##	16-Nov-2006 (hanje04)
##	    SD 111979
##	    If we're root we don't need to SU.
##	01-Dec-2006 (hanje04)
##	    SIR 116877
##	    Echo note to user that we are searching RPM database as this
##	    can take a while.
##	    Make script executable from anywhere.
##	    Remove "inginstgui" from error message.
##	22-Dec-2006 (hanje04)
##	    Fix up library locations to work on x86_64 Linux.
##	01-Mar-2007 (hanje04)
##	    BUG 117812
##	    Only print exit message on error. Clean exit doesn't
##	    always been we've installed.
##	10-Aug-2009 (hanje04)
##	    BUG 122571
##	    RPM saveset and database probing is now done by a separate python
##	    script. Remove RPM library hack, and fix up execution to run 
##	    everything in the right order.
##	10-Dec-2009 (hanje04)
##	    BUG 122944
##	    Make sure license is displayed and accepted before launching
##	    installer.
##	12-May-2011 (hanje04)
##	    Set PYTHONPATH before launching ingpkgqry as it's now more than
##	    one module
##	    Unset any potentially problematic local variables
##	    Correct support URL
##	20-May-2010 (hanje04)
##	    SIR 123791
##	    License acceptance now part of GUI, no need to run ingres-LICENSE
##	    Leave in check for LICENSE file though.
##	07-Jun-2011 (hanje04)
##	    Add support for signed builds. Build is signed if "checksig" is
##	    present in the 'bin' directory. Added validate_signature()
##	    to perform the validation using checksig. Prompt user for 
##	    continue if we fail.
##	16-Jun-2011 (hanje04)
##	    Mantis 2056
##	    Add explicit platform check to prevent errors trying to run
##	    APT specific commands on an RPM based system and vice versa
##      20-Jun-2011 (hanje04)
##          Mantis 2135 & 2138
##          Allow installation of signed saveset even if it can't be verified
##	21-Oct-2011 (hanje04)
##	    Bug 125882
##	    SD 145356
##	    Add check_deps() to check we have all the libraries available 
##	    for running the GUI before launching it. Report what's missing
##	    if needed.


# useful variables
unamep=`uname -p`
self=`basename $0`
instloc=`dirname $0`
instexe=${instloc}/bin/inginstgui
inglic=${instloc}/LICENSE
pkgqry=${instloc}/bin/ingpkgqry
checksig=${instloc}/bin/checksig
checksig_flag=""
pkginfo=/tmp/ingpkg$$.xml
prod_name="(PROD1NAME)"
prod_rel=ING_VERSION
uid=`id -u`
relaunch=false
unset II_SYSTEM II_CONFIG II_ADMIN

# set umask
umask 022

#
# localsu() - wrapper function for calling /bin/su if one of the GUI
#		equivalents cannot be found.
#
localsu()
{
    ROOTMSG="Installation of $prod_name requires 'root' access"

    echo $ROOTMSG

    if [ -f /etc/debian_version ] ; then
	echo "Trying sudo..."
	echo ""
	sudo ${1}
    else
	echo "Enter the password for the 'root' account to continue"
	su -c "${1}" root
    fi

    return $?
}

#
# noexe() - error handler for missing executables
#
noexe()
{
    exe=$1
    cat << EOF
Error: $self

Cannot locate the $prod_name installer executable:

	${instexe}

EOF
    exit 1
}

#
# nopy() - error handler for missing executables
#
checkpy()
{
    # python exits?
    pyvers=`python -V 2>&1`
    [ $? != 0 ] || [ -z "$pyvers" ] &&
    {
	cat << EOF
Error:
Cannot locate 'python' executable which is required by this program.
Please check your \$PATH and try again.

EOF
	 return 1
    }

    # right version ?
    majvers=`echo $pyvers | cut -d' ' -f2 | cut -d. -f1`
    minvers=`echo $pyvers | cut -d' ' -f2 | cut -d. -f2`
    if [ $majvers -lt 2 ] || [ $minvers -lt 3 ] ; then
	cat << EOF
Error:
This program requires "Python" version 2.3 or higher. Only:

    ${pyvers}

was found.

EOF
	return 1
    else
	return 0
    fi
}

#
# For signed builds, use script to validate GPG package signatures
# If we can't prompt for continue
#
validate_signature()
{
    [ -d "${instloc}"/rpm ] && pkgdir=${instloc}/rpm
    [ -d "${instloc}"/apt ] && pkgdir=${instloc}/apt
    ${checksig} ${pkgdir}
    rc=$?

    if [ $rc = 0 ]
    then
	return
    elif [ $rc = 10 ]
    then
	read -p "Do you wish to continue? (y or N): " ans
	case "$ans" in
	    [Yy]*)
		checksig_flag=""
		return
		;;
	    *)
		exit 1
		;;
	esac
     else
	exit 1
     fi
}

#
# check_platform - Verify saveset is right for distribution
#
check_platform()
{
    if [ -f /etc/debian_version ]
    then
	# DEBs based so no RPM
	if [ -d "${instloc}"/rpm ]
	then
	    echo "ERROR: RPM is not supported for this version of Linux"
	    return 1
	else
	    return 0
	fi
    elif [ -d "${instloc}/apt" ] 
    then
	echo "ERROR: DEBs are not support on this version of Linux"
	return 1
    else
	return 0
    fi
}

#
# check_deps() verify we have the necessary runtime libraries to launch
# the installer
#
check_deps()
{
    rc=0
    arch=`file ${instexe} | cut -d' ' -f 3`
    for lib in `ldd ${instexe} | grep "not found"`
    do
	case "$lib" in
	    libgtk*) echo "ERROR: Cannot locate $arch gtk2 & gtk2-engines"
		     rc=1
		     ;;
	    libxml2*) echo "ERROR: Cannot locate $arch libxml2"
		     rc=1
		     ;;
	esac
    done

    return $rc
}

#
# clean_exit - Generic exit routine to make sure everything is cleaned up
# 		when we're done
clean_exit()
{
   rc=${1?-1}
   exit $rc
}
#
# Main - body of program
#

# su executable
# FIXME! Only use localsu for now, GUI su tools are causing problems
#
# if [ -x /opt/gnome/bin/gnomesu ]
# then
#     sucmd="exec /opt/gnome/bin/gnomesu"
# elif [ -x /opt/kde3/bin/kdesu ]
# then
#     sucmd="exec /opt/kde3/bin/kdesu"
# else
    sucmd=localsu
# fi
[ "$1" = "root" -a $uid -eq 0 ] && relaunch=true
if ! $relaunch
then
    echo "$prod_name $prod_rel Installer"
    echo

    # Sanity checking
    check_platform && checkpy ||
    {
	cat << EOF

An error occurred during installation.
For further assistance contact Actian Corporation Technical
Support at: http://www.actian.com/support-services/support

EOF
        exit 1
    }
    if [ ! -x ${instexe} ]
    then
	noexe ${instexe}
    elif [ ! -x ${pkgqry} ]
    then
	noexe ${pkgqry}
    fi

    # check dependecies
    if ! check_deps ; then
	cat << EOF

ERROR: One or more dependecies were not found. Please install the
missing packages and try again. For further assistance contact
Actian Corporation Technical Support at:
http://www.actian.com/support-services/support

EOF
        exit 1
    fi

    # check license, bail if it's not there
    [ -r ${inglic} ] ||
    {
	cat << EOF
ERROR: Cannot locate LICENSE file. For further assistance contact
Actian Corporation Technical Support at:
http://www.actian.com/support-services/support

EOF
    exit 3
    }
fi

# Launch installer as root, using sucmd if we need to
if [ $uid != 0 ]
then
    ${sucmd} "$0 root"
    rc=$?
else

    # Check for signed archive
    [ -x ${checksig} ] && 
    {
	checksig_flag=-c
	validate_signature
    }

    # Launch install
    PYTHONPATH=${instloc}/bin ${pkgqry} -s ${instloc} -o ${pkginfo} ${checksig_flag} &&
        ${instexe} ${pkginfo}
    rc=$?
    $relaunch && exit $rc
fi

# Gather saveset and install info
if [ $rc != 0 ]
then
    cat << EOF
Installation failed with error code ${rc}

An error occurred during installation.
For further assistance contact Actian Corporation Technical
Support at: http://www.actian.com/support-services/support

EOF
    exit 2
else
    cat << EOF

Done.
EOF
fi


clean_exit ${rc}

