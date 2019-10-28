:
## Copyright (c) 2011 Actian Corporation. All rights reserved
##
## Name:
##	checksig_rpm.sh
##
## Usage:
##	checksig_rpm <rpm(s) to verify>
##
## Description:
##	Verifies GPG signature of RPMs.
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
keyqryfmt="%{name}-%{version}-%{release} - %{summary}\n"
keyqrycmd="-q gpg-pubkey --qf "
rpmlist=''

usage()
{
    cat << EOF
usage:
    $self <path to RPM packages>

EOF
    exit 1
}

rpm --version > /dev/null 2>&1 ||
{
    echo "ERROR: Cannot locate RPM binary"
    exit 1
}

while [ "$1" ]
do
    if [ -d "${1}" ] ; then
	rpmlist="${1}/*.rpm "
    else
	rpmlist="${1} "
    fi
    shift
done
[ -z "$rpmlist" ] && usage

echo "Checking public key..."
rpm ${keyqrycmd} "${keyqryfmt}" | grep "${keyname}"
[ $? != 0 ] && echo "Warning: No key for \"${keyname}\" found"

printf "Verifiying RPMs... "
outfile=/tmp/rpmcs.$$
rpm --checksig ${rpmlist} > $outfile 2>&1
rc=$?
if [ $rc = 0 ] ; then
    echo OK
else
    echo FAIL
    rc=10
    cat $outfile
fi
rm -f $outfile

[ $rc != 0 ] &&
{
    cat << EOF
Warning!
Unable to verify packages as authentic. Please check you have the latest

    ${keyname} (RPM)

public key installed. It can be downloaded from http://esd.actian.com
and installed by running:

    rpm --import <keyfile>

as root.

EOF
}
	
exit $rc


