:
# Actian Corporation 2011, all rights reserved
#
## Name: gipgenpot.sh
##
## Usage:
##
##	gipgenpot [-x <exclude file>][-o <outfile]
##
##	-x - Exclude contents of exclude file from output
##	-o - Write output to <outfile>, if file exist output
##	     will be merged. (defaults to ${potfile})
##
##
## Description:
##
##	Wrapper script for xgettext, used to generate .pot template
##	file from the gettext message strings in the GUI installer 
##	(inginstgui). This is then used to create lang.po files
##	to be translated.
## 
## History:
##	28-Jan-2011 (hanje04)
##	    Created.
##	24-Feb-2011 (hanje04)
##	    Updated for 1.5
##	01-Sep-2011 (hanje04)
##	    Update for main VW 2.0 Ingres 10.1, remove the message location
##	    markers. Just confusing when used across multiple codelines

pkgname=inginstgui
pkgvers=2.0
potfile=${pkgname}.pot
jflag=""
xflag=""
xfile=""
copyright="Actian Corporation 2011, all rights reserved"
bugemail="support@actian.com"

usage()
{
    cat << EOF
Usage:

    gipgenpot [-x <exclude file>][-o <outfile>]

	-x - Exclude contents of exclude file from output
	-o - Write output to <outfile>, if file exist output
	     will be merged. (defaults to ${potfile})

EOF
    exit 1
}


while [ "$1" ]
do
    case $1 in
	-x) xflag="-x" # exclude contents of file
	    if [ -r "$2" ] ; then
		xfile=$2
		shift ; shift
	    else
		echo "Cannot locate $2, aborting..."
		usage
	    fi
	    ;;
	-o) if [ "$2" ] ; then
		case "$2" in
		    -*) usage
			;;
		     *) :
			;;
		esac
	    else
		usage
	    fi
	    potfile=$2
	    shift ; shift
	    ;;
    esac
done

# if output file aready exists, merge
[ -r ${potfile} ] && jflag=-j

# command for generating .pot template file for translation
xgettext ${jflag} --keyword="N_" --keyword="Q_" --keyword="_" \
	--copyright-holder="${copyright}" \
	--msgid-bugs-address="${bugemail}" \
	--default-domain=${pkgname} --sort-output \
	${xflag} ${xfile} -o ${potfile} --no-location \
	$ING_SRC/front/st/gtkinstall_unix/ingpkgqry.py \
	$ING_SRC/front/st/gtkinstall_unix*/gip*.c \
	$ING_SRC/front/st/hdr/gip*.h
