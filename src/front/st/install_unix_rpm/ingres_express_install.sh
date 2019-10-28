:
#
#  Copyright (c) 2005, 2011 Actian Corporation. All rights reserved. 
#
#  Name: ingres_express_install.sh -- Single point install script for Ingres.
#				   
#
#  Usage:
#       ingres_express_install.sh [installation path] [installation ID]
#
#  Description:
#	Installs Ingres with default values from RPMS or tar archive
#
#  Exit Status:
#       0       installation procedure completed.
#	1	Not run as root
#	2	Invalid Installation ID
#	3	Invalid installation path
#	5	Installation failed
#	10	Invalid argument
#
#	Also see platform specific routines
#
## DEST = utility
##
## History:
##
##      02-Apr-2004 (hanje04)
##          Created.
##      07-Apr-2004 (hanje04)
##          Use RPMARCH not ARCH for licesnsing RPM.
##      30-Jul-2004 (hanje04)
##          Updated for Open Source.
##      26-Jan-2005 (hanje04)
##          Get version info from VERS
##      07-Mar-2004 (hanje04)
##          Can't use ccpp directly as it's a build script. Use
##          ING_VERSION macro which will be replaced by jam.
##      12-May-2005 (kodse01)
##          Add support for other Linux platforms.
##	02-Jun-2005 (hanje04)
##	    SIR 114618
##	    Major overhaul to add support for installing tar distributions
##	    as well as RPMs. Also now verified to work on Solaris, will 
##	    need tweaking to work on other UNIXES.
##		- Split functionality up into 2 functions install_rpm &
##		  install_tar.
##		- Added version info to error/info messages
##		- Add -tar flag fo Linux so that tar install is supported
##		- on all platforms.
##	02-Jun-2005 (hanje04)
##	    Update usage message
##	21-Jul-2005 (bonro01)
##	    Update script to work for all current Ingres platforms.
##	16-Aug-2005 (bonro01)
##	    Add code to execute mkvalidpw. Check for Ingres installation,
##	    valid II_SYSTEM, and copy .ingXXsh to user home directory, to
##	    make install consistent with RPM install on Linux.
##	31-Aug-2005 (bonro01)
##	    Fix split lines.
##	    Only setup II_SHADOW_PWD if the system is running shadow
##	    passwords.
##	07-Sep-2005 (bonro01)
##	    Test for AIX shadow password file.
##	30-Sep-2005 (hanje04)
##	    Update header as script is no longer only used for Linux.
##	28-Oct-2005 (hanje04)
##	    SIR 115239
##	    Add spatial to package list. Remove invalid comment.
##	05-Jan-2006 (bonro01)
##	    Change default install location to /opt/Ingres/IngresII
##	23-Jan-2006 (hanje04)
##	    SIR 115662
##	    Replace ca-ingres with ingres2006 in package names for Ingres
##	    2006 release.
##	    Also remove references to CATOSL as license is no longer used.
##	    Temporarily remove license check until licensing method is 
##	    determined.
##	07-Feb-2006 (hanje04)
##	    Re-add license checking code and invoke ingres-LICENSE if
##	    needed.
##	08-Feb-2006 (hanje04)
##	    Fix architecture checking for installing i386 on x86_64
##	02-Mar-2006 (hanje04)
##	    BUG 115806
##	    Update header comments to match usage() message.
##	    Fix error handling to correctly report incorrect path.
##	18-Mar-2006 (hanje04)
##	    BUG 115856
##	    Due to various RPM bugs trying to upgrade can go very 
##	    wrong. Add checks for previous installed versions and
##	    abort apropriately.
##	7-Apr-2006 (bonro01)
##	    Remove explicit reference to Ingres r3
##	07-May-2006 (hanje04)
##	   Bump version to ingres2007
##	20-Jul-2006 (hweho01)
##	    For AIX, the shared library path name is LIBPATH. 
##	27-Sep-2006 (bonro01)
##	    Allow this script to detect ingres.tar install on linux
##	19-Oct-2006 (bonro01)
##	    The 'which' command is not available in the UnixWare default path.
##	    Use /bin/tar which is available on all supported platforms.
##	18-Dec-2006 (bonro01)
##	    Fix filesystem authorization checking to force use of bourne
##	    shell because csh shell does not recognize the test syntax
##	    especially on UnixWare. This change prevents the userid's
##	    default shell from being used because test could fail under csh.
##	12-Dec-2006 (hanje04)
##	    SIR 117239
##	    Update script for new saveset layout (RPM packages now in rpm
##	    subdirectory.)
##	    Handle RPMs without licensing dependencies.
##	05-Jan-2007 (bonro01)
##	    Change Getting Started Guide to Installation Guide.
##	22-Jan-2007 (bonro01)
##	    Change version back for ingres2006 release 3
##	12-Feb-2007 (bonro01)
##	    Remove JDBC package.
##       2-Mar-2007 (hanal04) Bug 117589
##          The user may not have write priviledges to $II_SYSTEM.
##          Use $II_SYSTEM/ingres for .ingXXcsh and .ingXXsh instead.
##       31-May-2007 (hweho01)
##          Need to modify the platform specific library path for    
##          Tru64, UnixWare, AMD and Itanium Linux platforms in the 
##          generated script file. 
##	7-Mar-2008 (bonro01)
##	    Some Linux distributions like Mandriva have TMPDIR set to
##	     ~root/tmp which is inaccessible to the Ingres administrator
##	     userid, so this is set to /tmp
##	28-Jul-2008 (hanje04)
##	    Add support for passing in a response file. Also fix up for OSX
##	    so I could run it in the first place.
##	 11-Aug-2008 (hweho01)
##	    Change package name from ingres2006 to ingres for 9.2 release,
##	    use (PROD_PKGNAME) which will be substituted by jam during build.
##	    Also change the prod_name from 'Ingres 2006' to 'Ingres' by 
##	    using (PROD1NAME). 
##	15-Oct-2008 (bonro01)
##	    Install fails on Ubuntu during userid check if user login
##	    script returns a non-zero return code.
##      20-Oct-2008 (hweho01)
##          Remove ice from the package list, so it will not be installed
##          by running this script.
##	28-Apr-2010 (hanje04)
##	    Bug 123648
##	    Add support for installing as a user other than 'ingres' for tar
##	    based installs.
##	07-July-2010 (hanje04)
##	   Bug 124046
##	   Add license prompt for RPM and -acceptlicnse flag for batch
##	   operation.
##	23-Sep-2010 (hweho01)
##	    Correct the message typo at the beginning of the installation.  
##	20-Jan-2011 (hanje04)
##	    Bug 124950
##	    SD 148834
##	    Allow script to be run from any directory for RPM installs too,
##	    using $selfdir.
##	08-Apr-2011 (hanje04)
##	    Add support for DEB/apt based savesets on Ubuntu/Debian
##	    install_deb(). Consolidate error conditions across methods as 
##	    much as possible.
##	02-May-2011 (hanje04)
##	    debpkg etc. default to "" so update check appropriately when
##	    validating the install location
##	18-May-2011 (hanje04)
##	    Add II_ADMIN to list of problem causing variables to unset
##	    Remove DEB config file after use to prevent errors when 
##	    saveset is no longer in place.
##	31-May-2011 (hanje04)
##	    Mantis 1938
##	    Improve package/platform checking to stop RPM saveset being
##	    launched on DEBs platform
##	    Mantis 2023
##	    Fix II_SYSTEM for DEBs install to match new fixed location	
##	07-Jun-2011 (hanje04)
##	    Add support for signed builds. Build is signed if "checksig" is
##	    present in the 'bin' directory. Added validate_signature()
##	    to perform the validation using checksig. Prompt user for 
##	    continue if we fail.
##	14-Jun-2011 (hanje04)
##	    Mantis 1938
##	    Correct generated repo.list file to use "lucid" instead of "stable"
##	    until we support other platforms.
##	    Fix "$signrelease" code for non-signed releases.
##	21-Jun-2011 (hanje04)
##	    Mantis 2135 & 2138
##	    Allow unauthenticated RPMs to be installed.
##	02-Sep-2011 (hanje04)
##	    Mantis 2180
##	    Bug 125721
##	    Only display "-user" in usage message when it's valid (i.e. for
##	    tar based installs). Also remove -tar option form usage message
##	    as it's only useful internally.

#
# Multi-platform whoami
#
iiwhoami()
{
if [ -f /usr/ucb/whoami ]; then
	/usr/ucb/whoami
elif [ -f /usr/bin/whoami ]; then
	/usr/bin/whoami
elif [ -f /usr/bin/id ]; then
	IFS="()"
	set - `/usr/bin/id`
	echo $2
elif [ -f /bin/id ]; then
	IFS="()"
	set - `/bin/id`
	echo $2
else
	touch /tmp/who$$
	set - `ls -l /tmp/who$$`
	echo $3
	rm -f /tmp/who$$
fi
}

usage()
{
    cat << EOF
usage:
$self [-respfile filename] ${userflag}[Installation ID] [Install dir]

	-respfile	- Use 'filename' as response file when running setup
EOF
    if [ "${userflag}" ] ; then
	cat << EOF
	-user username	- Install as 'user' instead of ingres, username must
			  exist.  (Non-RPM only)
EOF
    fi
    cat << EOF
	Installation ID - 2 character string where the first character must be
			  and UPPER CASE letter and the second character must
			  be an UPPPER CASE letter or a number from 0-9
	Install dir	- Full path to location where Ingres is to be installed
			  (II_SYSTEM) Note: Not valid for DEBs, hard coded to:
			  ${definst} 
EOF
}

#
# For signed builds, use script to validate GPG package signatures
# If we can't prompt for continue
#
validate_signature()
{
    $rpmpkg && pkgdir=${selfdir}/rpm
    $debpkg && pkgdir=${selfdir}/apt
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
		# RPM bails by default if we don't tell it not to
		$rpmpkg && allow_unauth=--nosignature
		$debpkg && signrelease=false
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
# Linux rpm install routine
#
#  Description:
#       This script performs a default install of all the Ingres
#       RPMs in the current working directory. The default installation 
#	ID will be II unless another is specified. 
#
#    Exit Status:
#       3       ingres-LICENSE not run
#       4       Cannot locate core RPM
#	6	Ingres already installed.
#	7	Failed to start ingres
#
install_rpm()
{
    needlic=true    
    RPMVERS=ING_VERSION
    RPMBLD=BUILD_NO
    COREPKG="(PROD_PKGNAME)-${RPMVERS}-${RPMBLD}"
    RPMARCH=`rpm -qp --qf "%{arch}\n" ${selfdir}/rpm/${COREPKG}.*.rpm`
    [ "$RPMARCH" ] ||
    {
	echo "Failed to determine RPM architecture"
	echo "Aborting..."
	exit 1
    }

    cat << EOF
Checking for current installations...

EOF
    # check for r3, if it's not there check for ingres200x
    instcore=`rpm -q ca-ingres`
    [ $? = 0 ] || instcore=''
    [ -z "$instcore" ] && 
    {
	instcore=`rpm -q ingres2006`
    	[ $? = 0 ] || instcore=''
	[ -z "$instcore" ] &&
	{	
		instcore=`rpm -q (PROD_PKGNAME)`
		[ $? = 0 ] || instcore=''
	}
    }
	
    # We can't handle upgrade, so if we found previous
    # installs abort
    [ "$instcore" ] &&
    {
	pkglist=`rpm -qa | grep $instcore | grep -v [[:upper:]]`

	cat << EOF
The following Ingres packages are already installed:

$pkglist

$self does not support upgrade.

See the Installation Guide for more information.
EOF
	exit 6
    }

    CORERPM="${selfdir}/rpm/(PROD_PKGNAME)-${RPMVERS}-${RPMBLD}.${RPMARCH}.rpm"
    INGLICRPM=`rpm -qp --requires ${CORERPM} | grep (PROD_PKGNAME)-license | \
		cut -d' ' -f1`

    cat << EOF
Checking licensing requirements...

EOF


    ## Check LICENSE has been accepted
    $licaccept && needlic=false

    $needlic &&
    {
	cat << EOF
The License for $prod_name $prod_rel must be read and accepted before
installation of this product can commence. 
Invoking ./ingres-LICENSE...

EOF
	${selfdir}/bin/ingres-LICENSE || exit 3
    }

    cat << EOF
Building package list...

EOF
    ## Locate core RPM, if we can't find it, abort
    rpm -q (PROD_PKGNAME)-${RPMVERS}-${RPMBLD} > /dev/null 2>&1
    if [ $? != 0 ] ; then
	[ ! -f "$CORERPM" ] && {
	    cat << EOF
Cannot locate the Ingres ${RPMVERS}.${RPMBLD} core package:

	$CORERPM

EOF
	    usage
	    exit 4
	}
	FILELST=$CORERPM
    fi

    ## Generate install list
    for pkg in $PKGLST
    do
	[ -f "${selfdir}/rpm/(PROD_PKGNAME)-${pkg}-${RPMVERS}-${RPMBLD}.${RPMARCH}.rpm" ] && 
	{
	    FILELST="$FILELST ${selfdir}/rpm/(PROD_PKGNAME)-${pkg}-${RPMVERS}-${RPMBLD}.${RPMARCH}.rpm"
	}
    done

## Do the install
    cat << EOF
II_SYSTEM: $instloc
II_INSTALLATION: $II_INSTALLATION

Invoking RPM...

EOF

    # Use response file if we have one
    if $userespfile ; then
	echo "Using response file: $respfile"
	export II_RESPONSE_FILE=$respfile
    fi
    if [ "$instloc" ] ; then
	rpm -ivh ${allow_unauth} --prefix "$instloc" $FILELST
    else
	rpm -ivh ${allow_unauth} $FILELST
    fi

    if [ $? = 0 ] ; then
	cat << EOF
$prod_name $prod_rel has installed successfully. 
The instance will now be started...

EOF
    ## Start up Ingres
    [ -x /etc/init.d/ingres${inst_id} ] &&
	 /etc/init.d/ingres${inst_id} start || exit 7
    else
	cat << EOF

An error has occured during the installation of $prod_name $prod_rel.

EOF
	exit 5
    fi

# Install doc separately if it's there
    [ -f "${selfdir}/rpm/(PROD_PKGNAME)-documentation-${RPMVERS}-${RPMBLD}.${RPMARCH}.rpm" ] && 
    {
	cat << EOF
Installing documentation...

EOF
	rpm -ivh "${selfdir}/rpm/(PROD_PKGNAME)-documentation-${RPMVERS}-${RPMBLD}.${RPMARCH}.rpm"
    }

} # install_rpm

#
# Linux DEB archive install routine
#
#  Description:
#       This script performs a default install of all the Ingres
#       DEB packages contained apt repository
#	ID will be II unless another is specified. 
#
#    Exit Status:
#	8	Error querying existing instance
#	9	Error updating APT repo info
#
install_deb()
{
    needlic=true    
    debupg=false
    DEBVERS=ING_VERSION
    DEBBLD=BUILD_NO
    pkgname="(PROD_PKGNAME)"
    repocfgdir=/etc/apt/sources.list.d
    repocfgfile=${repocfgdir}/${prod_name}_${DEBVERS}-${DEBBLD}_local.list
    repoconf="deb file:${selfdir}/apt lucid non-free"
    
    # check for DEB utilities
    for util in dpkg apt-get
    do
	($util --version > /tmp/${util}.$$ 2>&1) ||
 	{
	    cat << EOF
Failed to locate ${util}

EOF
	    cat /tmp/${util}.$$
	    rm /tmp/${util}.$$
	    usage
	    exit 1
         }
    done

    cat << EOF
Checking for instances...

EOF
    [ "$(dpkg --get-selections ${pkgname}*)" ] && debupg=true
    $debupg && 
    {
	cat << EOF
Found existing instance, attempting upgrade...

EOF
	# override II_INSTALLATION
	ingprenv=$(dpkg-query --listfiles ${pkgname} | grep ingprenv)
	export II_SYSTEM=$instloc
	II_INSTALLATION=$($ingprenv II_INSTALLATION)
	[ -z "$II_INSTALLATION" ] &&
	{
	    cat << EOF
Error:
Unable determine II_INSTALLATION for existing instance, aborting...

EOF
	    exit 8
	}
    }
        
    ## Check LICENSE has been accepted
    $licaccept && needlic=false

    $needlic &&
    {
	cat << EOF
The License for $prod_name $prod_rel must be read and accepted before
installation of this product can commence. 
Invoking ./ingres-LICENSE...

EOF
	sleep 1
	${selfdir}/bin/ingres-LICENSE || exit 3
	echo 
    }

    echo "Adding local repository..."
    if [ ! -d ${repocfgdir} ]
    then
	cat << EOF
Cannot locate APT configuration location:

    ${repocfgdir}

Aborting...
EOF
	exit 9
    fi

    echo "Updating cache..."
    # Add saveset location to repository
    echo ${repoconf} > ${repocfgfile}
    apt-get -q update > /tmp/aptupd.$$ 2>&1

    [ $? != 0 ] &&
    {
	cat  /tmp/aptupd.$$
	cat << EOF
An error occurred updating the APT repository cache, aborting...

EOF
	## remove apt config file
	[ -f ${repocfgfile} ] && rm -f ${repocfgfile}

	exit 9
    }
    rm -f  /tmp/aptupd.$$

    
## Do the install
    cat << EOF

II_SYSTEM: $instloc
II_INSTALLATION: $II_INSTALLATION

Invoking apt-get...

EOF

    # Use response file if we have one
    if $userespfile ; then
	echo "Using response file: $respfile"
	export II_RESPONSE_FILE=$respfile
    fi

    ## Check sigs if its a signed release
    $signrelease || allow_unauth="--yes --allow-unauthenticated"
    pkgs=`apt-cache search --names-only "${pkgname}*" | \
		awk '{print $1}' | xargs`
    if [ "$pkgs" ] ; then
	[ -n "${dryrun}" ] && echo "Installing ${pkgs}"
        apt-get install $pkgs ${dryrun} ${allow_unauth}
       rc=$?
    else
	echo "Empty package list"
	rc=-1
    fi
    [ -n "${dryrun}" ] && exit 0

    ## remove apt config file
    [ -f ${repocfgfile} ] && rm -f ${repocfgfile}

    if [ $rc = 0 ] ; then
	cat << EOF
$prod_name $prod_rel has installed successfully. 
The instance will now be started...

EOF
        ## Start up Ingres
        invoke-rc.d ingres${inst_id} start || exit 7
    else
	cat << EOF

An error has occured during the installation of $prod_name $prod_rel.

EOF
	exit 5
    fi

} # install_deb

genenv()
{
    # Pick up term type from environment
    [ "$TERM_INGRES" ] || TERM_INGRES=vt100fx

    unames=`uname -s`
    unamem=`uname -m`
    case $unames in
        HP-UX)
            case $unamem in
                ia64)
                    libpathname=SHLIB_PATH
                    libpathdir=$II_SYSTEM/ingres/lib/lp32
                    libpathname64=LD_LIBRARY_PATH
                    libpathdir64=$II_SYSTEM/ingres/lib
                    ;;
                *)
                    libpathname=SHLIB_PATH
                    libpathdir=$II_SYSTEM/ingres/lib
                    libpathname64=LD_LIBRARY_PATH
                    libpathdir64=$II_SYSTEM/ingres/lib/lp64
                    ;;
            esac
            ;;
        AIX)
            libpathname=LIBPATH
            libpathdir=$II_SYSTEM/ingres/lib:$II_SYSTEM/ingres/lib/lp64
            ;;
        OSF1|\
        UnixWare)
            libpathname=LD_LIBRARY_PATH
            libpathdir=$II_SYSTEM/ingres/lib
            ;;
        Linux)
           case $unamem in
                 ia64|\
                 x86_64)
                 libpathname=LD_LIBRARY_PATH
                 libpathdir=$II_SYSTEM/ingres/lib:$II_SYSTEM/ingres/lib/lp32
                   ;; 
                 *)
                 libpathname=LD_LIBRARY_PATH
                 libpathdir=$II_SYSTEM/ingres/lib
                   ;; 
            esac    
            ;;
	Darwin)
	    libpathname=DYLD_LIBRARY_PATH
            libpathdir=$II_SYSTEM/ingres/lib
	    ;;
        *)
            libpathname=LD_LIBRARY_PATH
            libpathdir=$II_SYSTEM/ingres/lib
            libpathname64=LD_LIBRARY_PATH_64
            libpathdir64=$II_SYSTEM/ingres/lib/lp64
            ;;
    esac

    cat << ! > $II_SYSTEM/ingres/.ing${II_INSTALLATION}sh
# Ingres environment for $II_INSTALLATION installation
# Generated at installation time

TERM_INGRES=$TERM_INGRES
II_SYSTEM=$II_SYSTEM
PATH=\$II_SYSTEM/ingres/bin:\$II_SYSTEM/ingres/utility:\$PATH
if [ \$${libpathname} ] ; then
    ${libpathname}=${libpathdir}:\$${libpathname}
else
    ${libpathname}=/lib:/usr/lib:${libpathdir}
fi
export II_SYSTEM TERM_INGRES PATH ${libpathname}
!
    if [ "$libpathname64" ]; then
    cat << ! >> $II_SYSTEM/ingres/.ing${II_INSTALLATION}sh
if [ \$${libpathname64} ] ; then
    ${libpathname64}=${libpathdir64}:\$${libpathname64}
else
    ${libpathname64}=/lib:/usr/lib:${libpathdir64}
fi
export ${libpathname64}
!
    fi

    cat << ! > $II_SYSTEM/ingres/.ing${II_INSTALLATION}csh
# Ingres environment for $II_INSTALLATION installation
# Generated at installation time

setenv II_SYSTEM $II_SYSTEM
set path=(\$II_SYSTEM/ingres/{bin,utility} \$path)
if ( \$?${libpathname} ) then
    setenv ${libpathname} ${libpathdir}:\$${libpathname}
else
    setenv ${libpathname} /lib:/usr/lib:${libpathdir}
endif
setenv TERM_INGRES $TERM_INGRES
!
    if [ "$libpathname64" ]; then
    cat << ! >> $II_SYSTEM/ingres/.ing${II_INSTALLATION}csh
if ( \$?${libpathname64} ) then
    setenv ${libpathname64} ${libpathdir64}:\$${libpathname64}
else
    setenv ${libpathname64} /lib:/usr/lib:${libpathdir64}
endif
!
    fi

chown $ingusr $II_SYSTEM/ingres/.ing${II_INSTALLATION}sh $II_SYSTEM/ingres/.ing${II_INSTALLATION}csh

} # genenv

#
# Unix tar archive install routine 
#
#  Description:
#       Performs a default install of the ingres.tar archive 
#       in the current working directory. The default installation 
#	ID will be II unless another is specified. 
#
#    Exit Status:
#       6       Cannot locate tar binary or archive
#	7	$ingusr user doesn't exist
#	8	Cannot create II_SYSTEM
#	9	ingbuild failed
#
install_tar()
{
    ingtar=$selfdir/ingres.tar
    [ -x "/bin/tar" ] && tar=/bin/tar
    [ -z "$tar" ] && tar=`which tar 2>/dev/null`
# Check tar and archive exists
    [ -x "$tar" ] ||
    {
	cat << EOF
Cannot locate tar command. Check path and try again

EOF
	exit 6
    }

    [ -r "$ingtar" ] ||
    {
	cat << EOF
Cannot locate $ingtar

EOF
	exit 6
    }

if [ "$doinstall" != "true" ] # First time through just do the setup
then
    II_SYSTEM=$instloc
    export II_SYSTEM
    PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
    export PATH

# Check $ingusr user exists
    su $ingusr -c "exit 0" ||
    {
	cat << EOF
User "$ingusr" must exist before the installation can proceed

EOF
	exit 7
    }

# Need to make sure II_SYSTEM is readable by user $ingusr. If it isn't we
# abort the install.
    [ -d "$II_SYSTEM" ] &&
    {
	if su $ingusr -c "sh -c 'test ! -r \"$II_SYSTEM\" ' " ; then
	    rc=7
	    cat << !
Specified location for II_SYSTEM is not readable by user $ingusr

        II_SYSTEM=$II_SYSTEM

II_SYSTEM must be readable by user $ingusr for the installation to proceed.
!
	    exit $rc
	fi

	if su $ingusr -c "sh -c 'test ! -x \"$II_SYSTEM\" ' " ; then
	    rc=7
	    cat << !
User $ingusr does NOT have execute permission for the location specified
for II_SYSTEM.

        II_SYSTEM=$II_SYSTEM

User $ingusr must have execute permission in II_SYSTEM for installation
to continue.
!
	    exit $rc
	fi
    }

# Echo config
    cat << EOF
II_SYSTEM: $instloc
II_INSTALLATION: $II_INSTALLATION

EOF

# Create II_SYSTEM
    II_SYSTEM=$instloc
    export II_SYSTEM
    echo "Creating $II_SYSTEM..."
    umask 22
    mkdir -p $II_SYSTEM/ingres && \
    chown $ingusr $II_SYSTEM/ingres || 
    {
	cat << EOF
Failed to create II_SYSTEM:

	$II_SYSTEM

EOF
	exit 8
    }
   
# Generate environment setup scripts
    genenv

# Call self again, as $ingusr to perform the rest of the installation
    doinstall=true
    export doinstall
    if $force_tar ; then
        su $ingusr -c "sh $0 -tar $acclicflag $II_INSTALLATION $instloc $respflag" || exit $? 
    else
        su $ingusr -c "sh $0 $acclicflag $II_INSTALLATION $instloc $respflag" || exit $?
    fi

# Build password validation program if we can
    [ -f /etc/shadow -o -f /etc/security/passwd ] && \
    {
	if [ -x $II_SYSTEM/ingres/bin/mkvalidpw ] ; then
	    $II_SYSTEM/ingres/bin/mkvalidpw
	elif [ ! -x II_SYSTEM/ingres/bin/ingvalidpw ] ; then
	    # Use the distributed version
	    [ -f "$II_SYSTEM/ingres/files/iipwd/ingvalidpw.dis" ] && \
		cp -f $II_SYSTEM/ingres/files/iipwd/ingvalidpw.dis \
		    "$II_SYSTEM/ingres/bin/ingvalidpw"

	    # Set II_SHADOW_PWD if we need to
	    $II_SYSTEM/ingres/bin/ingsetenv II_SHADOW_PWD $II_SYSTEM/ingres/bin/ingvalidpw
	fi
    }

    # If ingvalidpw exists then it needs to be owned by root
    # and have SUID set.
    [ -x $II_SYSTEM/ingres/bin/ingvalidpw ] && {
	chown root $II_SYSTEM/ingres/bin/ingvalidpw
	chmod 4755 $II_SYSTEM/ingres/bin/ingvalidpw
    }

    homedir=`awk -F: -v user=$ingusr '$1 == user { print $6 }' /etc/passwd`
    [ -d "$homedir" ] && [ -w "$homedir" ] ||
    {
	cat << EOF
The home directory for the user $ingusr:

        $homedir

does not exist or is not writeable. The environment setup script for
this instance of Ingres will be written to:

        $II_SYSTEM/ingres

EOF
	eval homedir=$II_SYSTEM/ingres
    }
# Copy it to home directory if we can
    [ "$homedir" != "$II_SYSTEM/ingres" ] && \
	su $ingusr -c "cp -f $II_SYSTEM/ingres/.ing*sh $homedir"

else # dosetup=true 
# Setup environment
    II_SYSTEM=$instloc
    export II_SYSTEM
    PATH=$II_SYSTEM/ingres/bin:$II_SYSTEM/ingres/utility:$PATH
    export PATH
    unames=`uname -s`
    unamem=`uname -m`
    case $unames in
        HP-UX)
            case $unamem in
                ia64)
                    if [ "$SHLIB_PATH" ] ; then
                        SHLIB_PATH=$II_SYSTEM/ingres/lib/lp32:$SHLIB_PATH
                    else
                        SHLIB_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib/lp32
                    fi
                    export SHLIB_PATH
                    if [ "$LD_LIBRARY_PATH" ] ; then
                        LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib:$LD_LIBRARY_PATH
                    else
                        LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib
                    fi
                    export LD_LIBRARY_PATH
                    ;;
                *)
                    if [ "$SHLIB_PATH" ] ; then
                        SHLIB_PATH=$II_SYSTEM/ingres/lib:$SHLIB_PATH
                    else
                        SHLIB_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
                    fi
                    export SHLIB_PATH
                    if [ "$LD_LIBRARY_PATH" ] ; then
                        LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64:$LD_LIBRARY_PATH
                    else
                        LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib/lp64
                    fi
                    export LD_LIBRARY_PATH
                    ;;
            esac
            ;;
        AIX)
            if [ "$LIBPATH" ] ; then
                LIBPATH=$II_SYSTEM/ingres/lib:$II_SYSTEM/ingres/lib/lp64:$LIBPATH
            else
                LIBPATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib:$II_SYSTEM/ingres/lib/lp64
            fi
            export LIBPATH
            ;;
	Darwin)
            if [ "$DYLD_LIBRARY_PATH" ] ; then
                DYLD_LIBRARY_PATH=$II_SYSTEM/ingres/lib:$DYLD_LIBRARY_PATH
            else
                DYLD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
            fi
            export DYLD_LIBRARY_PATH
	    ;;
        *)
            if [ "$LD_LIBRARY_PATH" ] ; then
                LD_LIBRARY_PATH=$II_SYSTEM/ingres/lib:$LD_LIBRARY_PATH
            else
                LD_LIBRARY_PATH=/lib:/usr/lib:$II_SYSTEM/ingres/lib
            fi
                export LD_LIBRARY_PATH
            if [ "$LD_LIBRARY_PATH_64" ] ; then
                LD_LIBRARY_PATH_64=$II_SYSTEM/ingres/lib/lp64:$LD_LIBRARY_PATH_64
            else
                LD_LIBRARY_PATH_64=$II_SYSTEM/ingres/lib/lp64
            fi
                export LD_LIBRARY_PATH_64
            ;;
    esac

#check to see if instance is running and abort if it is
    [ -x $II_SYSTEM/ingres/utility/csreport ] && csreport >> /dev/null
    if [ $? = 0 ]
    then
        rc=8
        echo "The $prod_name $prod_rel installation under $II_SYSTEM is running"
        echo "Aborting installation..."
        exit $rc
    fi

    # Some Linux distributions like Mandriva have TMPDIR set to ~root/tmp
    # which is inaccessible to Ingres administrator userid, so this is set to /tmp
    if [ "$TMPDIR" ]; then
	TMPDIR=/tmp
	export TMPDIR
    fi

# Install the save set
    echo "Beginning installation..."
    cd $II_SYSTEM/ingres >> /dev/null
    $tar xf $ingtar install
    if $userespfile ; then
	echo "Using response file: $respfile"
	install/ingbuild ${acclicflag} -all -exresponse -file="$respfile" $ingtar
    else
	install/ingbuild ${acclicflag} -express $ingtar
    fi

    ## Start up Ingres
    if [ $? -eq 0 ]; then
	[ -x "$II_SYSTEM/ingres/utility/ingstart" ] &&
	{
	    cat << EOF
$prod_name $prod_rel has installed successfully. 
The instance will now be started...

EOF
	    ingstart
	}
    else
	cat << EOF
An error has occured during the installation of $prod_name $prod_rel.

EOF
	exit 5
    fi

fi
    
} # install_tar

#
# Main body of script starts here
#
self=`basename $0`
selfdir=`dirname $0`
[ "$selfdir" = "." -o "$selfdir" = "" ] && selfdir=`pwd`
USER=`iiwhoami`
prod_name=(PROD1NAME)
prod_rel=ING_VERSION
instloc=
PKGLST='dbms net abf c2audit das esql odbc ome qr_run rep star tuxedo vision spatial'
## Make sure unwanted II_SYSTEM or II_CONFIG value doesn't 
## affect the installation
unset II_SYSTEM II_CONFIG II_ADMIN
rpmpkg=false
debpkg=false
tarpkg=false
ls $selfdir/rpm/*.rpm > /dev/null 2>&1 && rpmpkg=true
ls $selfdir/apt/ > /dev/null 2>&1 && debpkg=true
ls $selfdir/ingres.tar > /dev/null 2>&1 && tarpkg=true

inst_id=II
if $debpkg ; then
    definst="/opt/Ingres/${prod_name}"
else
    definst=/opt/Ingres/Ingres${inst_id}
fi
userespfile=false
respflag=""
force_tar=false
ingusr=ingres
licaccept=false
acclicflag=""
dryrun=""
checksig="${selfdir}/bin/checksig"
allow_unauth=""
signrelease=false
if $tarpkg ; then
    userflag="[-user username] "
else
    userflag=""
fi
## Check installation ID if given
while [ "$1" ]
do
    case "$1" in
	[A-Z][A-Z]|\
	[A-Z][0-9])
		    inst_id=$1
		    shift
		    ;;
	      -tar)
		    force_tar=true
		    shift
		    [ "$tarpkg" = "" ] &&
		    {
			echo "ingres.tar package not found."
			usage
			exit 9
		    }

		    ;;
	  -respfile)
		    userespfile=true
		    respfile=$2
		    [ -r "$respfile" ] ||
		    {
			echo "Cannot locate response file: $respfile"
			usage
			exit 10
		    }
		    respflag="$1 $2"
		    shift;shift
		    ;;
	      -user)
		    if $rpmpkg || $debpkg || [ ! "$2" ] ; then
			echo "Error: $1 $2 invalid argument"
			usage
			exit 10
		    fi
		    ingusr=$2
		    shift ; shift
		    ;;
     -acceptlicense)
		    licaccept=true
		    acclicflag=-acceptlicense
		    shift
		    ;;
            -dryrun)
		    # don't actually install
		    dryrun=--dry-run
		    shift
		    ;;
		*/*)
		    # Installation location
		    # Check for full path
		    instloc=`echo $1 | grep '^/'`
		    ( [ "${debpkg}" = "true" ] || [ -z "$instloc" ] ) &&
		    {
			cat << EOF
"$1" is not a valid installation location

EOF
			usage
			exit 3
		    }
		    shift
		    ;;
		 *)
		    cat << EOF
$1 is an invalid installation ID

EOF
		    usage
		    exit 2
		    ;;
    esac
done

[ "$rpmpkg" = "false" -a "$debpkg" = "false" -a "$tarpkg" = "true" ] && force_tar="true"
[ "$instloc" ] || instloc=$definst
II_INSTALLATION=$inst_id
export II_INSTALLATION

# First time round
[ "$doinstall" != "true" ] &&
{
# Startup message
    cat << EOF
$prod_name $prod_rel Express Install

EOF

    # Check user
    if [ "$USER" != "root" ] ; then
	cat << EOF
$self must be run as root

EOF
	exit 1
    fi

}

# Check for signed archive
[ -x ${checksig} ] && 
{
    signrelease=true
    validate_signature
}

case `uname -s` in
    Linux) reset="- 0"
	   # tar only an "option" on Linux
	   if $force_tar ; then
	       install_tar
	   elif $debpkg ; then
	       install_deb
	   elif $rpmpkg ; then
	       install_rpm
	   else
	       echo "Error: Unknown archive type..."
	       usage
	   fi
	   ;;
        *) reset=0
	   install_tar
	   ;;
esac

trap $reset
exit 0
