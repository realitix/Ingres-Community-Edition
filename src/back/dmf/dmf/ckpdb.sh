:
#
# Copyright (c) 2004, 2011 Actian Corporation
#
#
# Name: ckpdb	- take checkpoint of a database
#
# Description:
#      This shell program provides the INGRES `ckpdb' command by
#      executing the journaling support program, passing along the
#      command line arguments.
## History:
##      06-jul-1988 (mikem)
##          Created.
##      17-aug-1988 (roger)
##          Added header, wired in JSP pathname.
##      15-mar-1993 (dianeh)
##          Changed first line to a colon (":"); changed History
##          comment block to double pound-signs.
##      20-sep-1993 (jnash)
##          Change $* to "$@" to work with delimited id's.
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          the second line should be the copyright.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB build.
##	10-Nov-2010 (bolke01/bonro01) SIR 124813
##          Created from ckpdb.
##	23-Feb-2011 (bonro01)
##	    Fix typo
##	04-Mar-2011 (bonro01) Mantis 1509
##	    Terminiate ckpdb if VW database is already locked.
##      29-apr-2011 (stial01) m1700
##          Moved x100 ckpdb processing into dmfcpp.
##
# PROGRAM = ckpdb
#
(LSBENV)


exec ${II_SYSTEM?}/ingres/bin/dmfjsp ckpdb "$@"
