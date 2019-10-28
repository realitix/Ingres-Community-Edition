:
#  Copyright (c) 2004 Actian Corporation
#  Name: pfsave
#
#  Usage:
#   wrapper for pffiles	
#
#  Description:
#	Executed in-line by ingbuild on patch installation
#
## History:
##   15-March-1996 (angusm)	
##		created.
##   28-apr-2000 (somsa01)
##		Enabled for different product builds.
##
#  DEST = utility
#----------------------------------------------------------------------------

if [ "$1" = "" ]
then
	exit 255
fi

$(PRODLOC)/(PROD2NAME)/install/pffiles $1 in || exit 255

exit 0
