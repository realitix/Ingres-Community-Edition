:
#
#  Copyright (c) 2011 Actian Corporation, all rights reserved.
#
#  Name: uninstall_ingres_deb.sh -- Uninstall Ingres on Debian
#
#  Usage:
#       uninstall_ingres [ -y (--yes) ] [ -c (--clean) ]
#
#  Description:
#	Wrapper script for removing DEB instance from machine.
#	Will also remove associated directories if asked.
#
#  Parameters:
#	-y	- Answer 'yes' to all prompts (do not prompt)
#	-c	- Remove the $II_SYSTEM/ingres directory after
#		  the uninstall has completed.
#
#  Exit Status:
#       0       uninstall procedure completed.
#       1       not run as root
#       2       invalid argument
#       3       uninstall declined
#
## DEST = utility
##
## History:
##
##    15-Jun-2011 (hanje04)
##	Mantis 2018
##	Created from uninstall_ingres (for RPM)
##
self=`basename $0`
host=`hostname`
rm_all=false
clean=false
prompt=true
force=false

#
#-[ usage() -  Print syntax message ]-----------------------------------------
#
usage()
{
    cat << EOF
Usage:
    $self [ -y (--yes) ] [ -c (--clean) ]

where:
		-y	- Answer 'yes' to all prompts (do not prompt)
		-c	- Remove the \$II_SYSTEM/ingres directory after
			  the uninstall has completed.

WARNING!
If the -y option is specified with the -c option the installation location 
and everything underneath it will be removed without additional prompting.
EOF
}
# usage()

prompt()
{
    read -p "Do you want to continue? " resp
    case "$resp" in
	     [Yy]|\
     [Yy][Ee][Ss])
		    return 0
	   	    ;;
	        *)
		    return 1
		    ;;
    esac
}
# prompt()

#
#-[ clean_exit() ] - Clean up and exit with specified error or zero-----------
#
clean_exit()
{
    # Set return code
    rc=0
    [ $1 ] && rc=$1

    # Reset any traps and exit
    trap - 0
    exit $rc
}
# clean_exit()

#
#-[ deb_remove() ] - Remove specified DEBs
#
deb_remove()
{
    [ "$1" ] || return 1

    $prompt || flags="--yes "
    $force && flags="--force-yes"

    # remove all versions of specified packages
    apt-get purge $flags "$@"
}
# deb_remove()

#
#-[ remove_installation() ] - Remove DEB installation.
# There can be only one so that's easy
#
remove_installation()
{
    [ "$1" ] || return 1
    pkglist=$1

    # If we're cleaning out everything get II_SYSTEM
    $clean && ii_sys="/opt/Ingres/(PROD1NAME)"

    # Remove DEBs 
    deb_remove $pkglist
    rc=$?
    [ $rc = 0 ] || return $rc

# If removal is sucessfull cleanup if we're meant to
    $clean && [ -d "$ii_sys" ] &&
    {
	$prompt &&
	{
	    cat << EOF
The following directory and everything underneath it is about to be removed:

	$ii_sys/ingres

EOF
	    prompt || return 3
	}

   	echo "Removing $ii_sys/ingres..."
	rm -rf $ii_sys/ingres
    }
}
# remove_installation

#
#-[ Main - main body of script ]----------------------------------------------
#

# Startup message
cat << EOF
Uninstall (PROD1NAME)

EOF

## Check user
if [ "`whoami`" != "root" ] ; then
    cat << EOF
$self must be run as root

EOF
    usage
    exit 1
fi

while [ "$1" ]
do
    case $1 in 
	  --yes|-y)
		    prompt=false
		    shift
		    ;;
	--clean|-c)
		    clean=true
		    shift
		    ;;
		 *)
		    usage
		    exit 2
		    ;;
    esac
done

remove_installation (PROD_PKGNAME)

rc=$?

exit $rc
	   

