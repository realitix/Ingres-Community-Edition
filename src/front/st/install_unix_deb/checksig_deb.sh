:
## Copyright (c) 2011 Actian Corporation. All rights reserved
##
## Name:
##	checksig_apt.sh
##
## Usage:
##	checksig_apt <path to APT repos>
##
## Description:
##	Verifies GPG signature of APT repository (DEBs)
##
## Returns:
##	0 - On success
##	1 - On generic error
##	10 - On validation failure
##
## History:
##	06-Jun-2011 (hanje04)
##	    Created.
##	21-Jun-2011 (hanje04)
##	    Mantis 2135 & 2138
##	    Improve error message on validation error to include
##	    how to import key

self=`basename $0`
keyname="(PROD1NAME) ING_VERSION"
vrfycmd="gpgv --keyring /etc/apt/trusted.gpg "
aptdir=''
outfile=/tmp/aptcs.$$

usage()
{
    cat << EOF
usage:
    $self <path to APT repository>
    (Requires root access)
EOF
    exit 1
}

[ $(id -u) != 0 ] && usage

[ -x /usr/bin/apt-key ] ||
{
    echo "ERROR: Cannot locate apt-key binary"
    exit 1
}

[ -x /usr/bin/gpgv ] ||
{
    echo "ERROR: Cannot locate gpgv binary"
    exit 1
}

if [ -d "${1}" -a -d "${1}/dists" ] ; then
	aptdir="${1}"
fi
[ -z "$aptdir" ] && usage

printf "Updating key database... "
apt-key update > ${outfile} 2>&1
if [ $? != 0 ] ; then
    echo FAIL
    echo "Log written to: ${outfile}"
    usage
else
    echo OK
fi
rm ${outfile}

echo "Checking public key..."
apt-key adv --list-key "${keyname}" | grep -v ^Executing
[ $? != 0 ] && echo "Warning: No key for \"${keyname}\" found"

printf "Verifiying repository... "
for rel in `find ${aptdir} -type f -name Release`
do
    if [ -f "${rel}.gpg" ]
    then
        ${vrfycmd} ${rel}.gpg ${rel} > $outfile 2>&1
        rc=$?
        [ $rc != 0 ] && break
    fi
done
    
if [ $rc = 0 ] ; then
    echo OK
    rm -f $outfile
else
    echo FAIL
    rc=10
    echo "Log written to: $outfile"
fi

[ $rc != 0 ] &&
{
    cat << EOF
Warning!
Unable to verify save set as authentic. Please check you have the latest

    ${keyname} (DEB)

public key installed. It can be downloaded from http://esd.actian.com
and installed by running:

    apt-key add <keyfile>

as root.

EOF
}
	
exit $rc

