:
#
# Copyright (c) 2004, 2010 Actian Corporation
#
##      01-Aug-2004 (hanch04)
##          First line of a shell script needs to be a ":" and
##          then the copyright should be next.
##	16-Feb-2010 (hanje04)
##	    SIR 123296
##	    Add support for LSB builds
##	20-Apr-2010 (hanje04)
##	    SIR 123608
##	    Invoke SQL session instead of QUEL
#
(LSBENV)
exec tm -qSQL $*
