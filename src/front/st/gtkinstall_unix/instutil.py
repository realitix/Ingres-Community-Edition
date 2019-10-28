#!/usr/bin/python
#
# Copyright 2009, 2011 Actian Corporation. All Rights Reserved
#
# vim: tabstop=8 expandtab shiftwidth=4 smarttab smartindent

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Name: instutil.py
#
# Description:
#	Generic super classes and utilities for searching managed package
#	savesets and existing instances installed via package management
#	e.g RPM or DEBs.
#
# Usage:
#	from instutil import *
#
#	class MyInstallClass(IngInstall):
#
## History:
##	11-Aug-2009 (hanje04)
##	    Created from ingpkgqry.py as part of adding DEBs support
##      20-Jun-2011 (hanje04)
##          Mantis 2135 & 2138
##          Allow installation of signed saveset even if it can't be verified
##	01-Sep-2011 (hanje04)
##	    Raise error if product pass in to IngInstall() isn't valid
##      20-Sep-2011 (hanje04)
##          Move FAIL_OPEN_FILE so it has the appropriate context
##      01-Nov-2011 (hanje04)
##          Use FAIL_OPEN_FILE with correct context
import re
import sys
import os
from gettext import gettext as _

# Global Defines

# WARNING! Correct generation of XML is dependent on the order of ingpkgs
# entires matching EXACTLY those defined be PKGIDX in front!st!hdr gip.h
# DO NOT ALTER UNLESS PKGIDX unless has also been changed
ingpkgs = ( 'core', '32bit', 'abf', 'bridge', 'c2', 'das', 'dbms',
        'esql', 'i18n', 'ice', 'jdbc', 'net', 'odbc', 'ome', 'qr_run', 'rep',
        'star', 'tux', 'vision', 'documentation' )
ingpkgsdict = dict( zip( ingpkgs, range( 0, len( ingpkgs ) ) ) )

# WARNING! Correct generation of XML is dependent on the order of dblocnames
# entires matching EXACTLY those defined be location_index in front!st!hdr gip.h
# DO NOT ALTER UNLESS location_index unless has also been changed
ingdblocnames = ( 'II_SYSTEM', 'II_DATABASE', 'II_CHECKPOINT', 'II_JOURNAL',
		'II_WORK', 'II_DUMP' )
ingdblocdict = dict(zip(ingdblocnames, range(0, len(ingdblocnames))))
vwdblocnames = ( 'II_SYSTEM', 'II_DATABASE', 'II_CHECKPOINT', 'II_JOURNAL',
		'II_WORK', 'II_DUMP', 'II_VWDATA' )
vwdblocdict = dict(zip(vwdblocnames, range(0, len(vwdblocnames))))

# search masks
rnm = '[A-Z]([A-Z]|[0-9])' # rename mask
sm = ( 'ca-ingres$', 'ca-ingres-%s$' % rnm, 'ingres2006$',
        'ingres2006-%s$' % rnm, 'ingres$', 'ingres-%s$' % rnm,
        'ingresvw$', 'ingresvw-%s$' % rnm )

# misc globals
filesloc = os.path.join('ingres', 'files')
symtbl='symbol.tbl'
cfgdat='config.dat'

# error messages
FAIL_GET_DBLOCS = _("ERROR: Failed to determine DB locations")
FAIL_GET_LOGLOCS = _("ERROR: Failed to determine log locations")
FAIL_GET_IIINST = _("ERROR: Failed to determine value for II_INSTALLATION")
FAIL_SSET_QRY=_("ERROR: Failed to query %s saveset")
INVALID_PKG_NAME = _("ERROR: %(name)s-%(version)s-%(release)s-%(arch)s.rpm is not a valid package.\nAborting")
FAIL_MODE_NOT_SET = _("ERROR: Execution mode must be set to 'ingres' or 'vectorwise'\nAborting...")

Debug=False
def debugPrint( message ):
    """Basic trace module, turned on by Debug=True"""
    if Debug:
        print message

class IngInstError(Exception):
    """Generic Installer Exception"""
    pass

class SSError(IngInstError):
    """Generic Saveset Exception"""
    pass

class InstanceError(IngInstError):
    """Generic Instance Exception"""
    pass

class IngInstall(object):
    """Base class for finding existing instances of product"""
    FAIL_NO_PKGS_FOUND = _("ERROR: No packages found")
    FAIL_NO_CORE_PKG_FOUND = _("ERROR: no core package found\n%s")
    FAIL_FIND_INST_LOC = _("ERROR: Could not determing instance location\n")
    FAIL_OPEN_FILE = _("ERROR: Failed to open file: %s")
    instlist=[]
    initialized=False

    def __init__(self, sslocation, product, checksig=False):
	if product == 'ingres':
	    self.pkg_basename = 'ingres'
	    self.dfltinstid = 'II'
	    self.dblocdict = ingdblocdict
	elif product == 'vectorwise':
	    self.pkg_basename = 'ingresvw'
	    self.dfltinstid = 'VW'
	    self.dblocdict = vwdblocdict
	else:
	    raise InstanceError

    	self.ssinfo = {'basename' : [ self.pkg_basename ], 'version' : '',
			'arch' : '', 'format' : '', 'location' : [], 
			'package' : [] }
	self.ingpkgsdict=ingpkgsdict
        self.sslocation = sslocation
        self.ssinfo['location'] = [ sslocation ]
        self.product=product
        self.checksig=checksig
	self.qrySS()
	self.qryDB()
        self.initialized=True

    def getSSInfo(self):
	"""Return saveset info determined after parsing"""
        if not self.initialized:
            raise SSError("ssinfo not initialized")
        else:
            return self.ssinfo

    def getInstList(self):
	"""Return list of existing instance info"""
        if not self.initialized:
            raise InstanceError("instlist not initialized")
        else:
            return self.instlist

    def findInstID(self, instloc):
        """Read II_INSTALLATION from symbol table of target instance
            and return it"""
        symtblpath = os.path.join(instloc, filesloc, symtbl)
        instid = "undefined"

        try:
            cfgfile = open( symtblpath, 'r' )
        except IOError:
            print self.FAIL_OPEN_FILE % symtblpath
            print FAIL_GET_IIINST
        else:
            # start reading the file
            for l in cfgfile:
                # find II_INSTALLATION
                if l.startswith("II_INSTALLATION"):
                    instid = l.split()[1]
                    break
            
            cfgfile.close()
        return instid

    def findAuxLoc(self, instloc):
        """Get database and log locations for an Ingres instance at
            a given location"""
        symtblpath = os.path.join(instloc, filesloc, symtbl)
        cfgdatpath = os.path.join(instloc,filesloc,cfgdat)
        logparts = "ii\..*\.rcp.log.log_file_parts"
        primlog = "ii\..*\.rcp.log.log_file_[1-9]+:"
        duallog = "ii\..*\.rcp.log.dual_log_[1-9]+:"
        AuxLocs = { 'dblocs' : {}, 'loglocs' : {} }
        dblocs = AuxLocs[ 'dblocs' ]
        loglocs = AuxLocs[ 'loglocs' ]
        loglocs[ 'primarylog' ] = [] 
        loglocs[ 'duallog' ] = [] 

        # first look at the symbol table
        try:
            cfgfile = open( symtblpath, 'r' )
        except IOError:
            print self.FAIL_OPEN_FILE % symtblpath
            print FAIL_GET_DBLOCS
	except Exception:
	    raise
        else:
            # start reading the file
            ent = cfgfile.readline()
            while ent != '':
                # match each line against each location
                for loc in self.dblocdict.keys():
                    if re.match( loc, ent ) != None:
                        # when we find a match, check it against II_SYSTEM
                        tmplst = re.split( '/', ent, maxsplit = 1 )
                        path = [ '/' + tmplst[1].strip() ]
                        pat = instloc + '$'
                        if re.match( pat, path[0] ) == None:
                            # if it differs store it
                            dblocs[ loc ] = path 
            
                # and move on
                ent = cfgfile.readline()

        # now config.dat
        try:
            cfgfile = open( cfgdatpath, 'r' )
        except:
            print self.FAIL_OPEN_FILE % cfgdatpath
            print FAIL_GET_LOGLOCS
        else:
            # start reading the file
            ent = cfgfile.readline()
            while ent != '':
                # check for primary log
                if re.match( primlog, ent ) != None:
                    tmplst = re.split( ':', ent, maxsplit = 1 )
                    path = [ tmplst[1].strip() ]
                    pat = instloc + '$'
                    if re.match( pat, path[0] ) == None:
                        loglocs[ 'primarylog' ].append( path[0] )
                # then dual
                elif re.match( duallog, ent ) != None:
                    tmplst = re.split( ':', ent, maxsplit = 1 )
                    path = [ tmplst[1].strip() ]
                    pat = instloc + '$'
                    if re.match( pat, path[0] ) == None:
                        loglocs[ 'duallog' ].append( path[0] )

                ent = cfgfile.readline()

        return AuxLocs
