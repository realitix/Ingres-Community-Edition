:
#
# Copyright (c) 2004 Actian Corporation
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
#
#	@(#)dirname.sh	1.2
exec expr \
  ${1-.}'/' : '\(/\)[^/]*/$' \
  \| ${1-.}'/' : '\(.*[^/]\)//*[^/][^/]*//*$' \
  \| .
