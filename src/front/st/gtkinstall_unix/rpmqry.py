#!/usr/bin/python
#
# Copyright 2009, 2011 Actian Corporation. All Rights Reserved
#
# vim: tabstop=8 expandtab shiftwidth=4 smarttab smartindent
#
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

# Name: rpmqry.py
#
# Description:
#       Module for querying RPM saveset and database info.
#
# Usage:
#    import rpmqry
#
#    rpminfo=rpmqry.IngRPMInstall()
#
## History:
##	09-May-2011 (hanje04)
##	    Created from ingpkgqry.py as part of adding DEBs support
##      20-Jun-2011 (hanje04)
##          Mantis 2135 & 2138
##          Allow installation of signed saveset even if it can't be verified
##      19-Oct-2011 (hanje04)
##          SD 145367
##          BUG 125864
##          Make sure we ingore instances that aren't of the same product
##          we're installing.
##          Add self test.

import rpm
import os, re
from instutil import InstanceError, SSError, IngInstall
from instutil import debugPrint, ingpkgs, sm, rnm
from gettext import gettext as _

class IngRPMInstall(IngInstall):
    def __init__(self, sslocation, product, checksig=False):
        if product == "ingres":
            self.validpkgbnames = [ "ca-ingres", "ingres2006", "ingres" ]
        elif product == "vectorwise":
            self.validpkgbnames = [ "ingresvw" ]
        IngInstall.__init__(self, sslocation, product, checksig=False)
            
    def qrySS(self):
        self.sshdr=self.getSaveSetInfo()

    def qryDB(self):
        self.searchRPMDB()

    def isRenamed(self, pkgname):
        """Has package been renamed"""
        # Has pkg been renamed
        if re.search( rnm, pkgname ) != None:
            debugPrint( "%s is renamed" % pkgname )
            renamed = True
        else:
            renamed = False

        return renamed

    def getSaveSetInfo(self):
        """Load RPM header info form saveset and store in ssinfo"""

        # Get a list of available packages
        rpmloc = self.sslocation + '/rpm/'
        filelist = os.listdir( rpmloc )

        # check first file is valid, then use it to populate ssinfo
        ssts = rpm.ts()

        if not self.checksig:
            # disable signature checking
            ssts.setVSFlags(rpm._RPMVSF_NOSIGNATURES)

        fdno = os.open( os.path.join(rpmloc, filelist[0]), os.O_RDONLY )
        hdr = ssts.hdrFromFdno( fdno )
        if re.match( self.pkg_basename, hdr[ 'name' ] ) == None:
            print INVALID_PKG_NAME % hdr
            return None
        else:
            # Add core package to ssinfo
            self.ssinfo[ 'package' ].append('core')

        self.ssinfo[ 'version' ] = [ "%(version)s-%(release)s" % hdr  ]
        self.ssinfo[ 'arch' ] = [ "%(arch)s" % hdr ]
        self.ssinfo[ 'format' ] = [ "rpm" ]
     
        # Done with file
        os.close( fdno )

        # now scroll through the entries, adding the valid packages
        for f in filelist:
            for pkg in ingpkgs:
                if re.search( pkg, f ) != None:
                    self.ssinfo[ 'package' ].append( pkg )

        return hdr
        
    def searchRPMDB(self):
        """Search RPM database for Ingres packages"""
        # RPM db connection
        ts = rpm.TransactionSet() # handler for querying RPM database
        ss_ds = self.sshdr.dsOfHeader() # dependency set for saveset

        # now search results set using search masks
        for pat in sm :
            pkgs = ts.dbMatch() # RPM package list
            debugPrint( "Searching with %s" % pat )
            # Search list using mask
            pkgs.pattern( 'name', rpm.RPMMIRE_DEFAULT, pat )

            for inst in pkgs :
                debugPrint( "Found %(name)s-%(version)s-%(release)s" % inst )
                inst_ds = inst.dsOfHeader() # dependency set instance
                pkgname = inst['name']
                
                # we have at least one package for this instance
                # all need to be lists so we don't itterate over the characters
                instpkgs = [ 'core' ]
                instid = [ 'II' ] 
                basename = [ inst['name'] ]
                version = [ "%(version)s-%(release)s" % inst ]

                # Has pkg been renamed
                if self.isRenamed( pkgname ) == True:
                    # pkg renamed, strip out inst ID
                    tmplist = re.split( "-", inst['name'], 4 )
                    instid[0] = tmplist[len(tmplist) - 1] 
                    debugPrint( "%s is renamed, ID is %s" % ( inst['name'], instid[0] ) )
                    renamed = True
                else:
                    # Non-renamed can have non-default instid but we need 
                    # more info before we can find it
                    renamed = False


                # What's version is the instance and what can we do to it
                debugPrint( 'Saveset version: %s\nInstalled version: %s' %  \
                    ( ss_ds.EVR(), inst_ds.EVR() ) )
                res=rpm.versionCompare(self.sshdr, inst)

                # ignore other products
                if pkgname.split("-")[0] not in self.validpkgbnames:
                    action = [ 'ignore' ]
                elif res > 0:
                    if inst['version'].startswith("1.0"):
                        # Can't upgrade v1.0 so ignore it
                        action = [ 'ignore' ]
                    else:
                        action = [ 'upgrade' ] # older so upgrade
                elif res == 0:
                    action = [ 'modify' ] # same version so we can only modify
                else:
                    action = [ 'ignore' ] # newer so just ignore it
                debugPrint( 'Install action should be: %s' % str(action) )

                # Find where it's installed
                instloc = inst[rpm.RPMTAG_INSTPREFIXES]
                debugPrint( "installed under: %s" % instloc )
                debugPrint( "Searching for other packages" )
                allpkgs = ts.dbMatch()
                for p in allpkgs :
                    for ingpkg in ingpkgs:
                        if renamed == True :
                            instpat = "%s-%s-%s-%s" % ( ingpkg, instid[0],
                                                            inst['version'],
                                                            inst['release'] )
                            string = "%(name)s-%(version)s-%(release)s" % p
                        else:
                            instpat = "%s-%s-%s-%s" % ( inst['name'],
                                                            ingpkg, 
                                                            inst['version'],
                                                            inst['release'] )
                            string = "%(name)s-%(version)s-%(release)s" % p

                        if re.search( instpat, string, 0 ) != None:
                            debugPrint( "\tFound %(name)s-%(version)s-%(release)s" % p )
                            instpkgs.append( ingpkg )

                # find instID for non-renamed installs
                if not renamed:
                    instid[0] = self.findInstID(instloc[0])

                # sanity check what we got back
                if len(instid[0]) != 2:
                    print _("Invalid installation ID: '%s', found for %s-%s, using %s") % \
                                    (instid[0], basename[0], version[0], self.dfltinstid)
                    instid[0] = self.dfltinstid

                # create dictionary to store info
                instinfo = { 'ID' : instid,
                            'basename' : basename,
                            'version' : version,
                            'package' : instpkgs,
                            'location' : instloc,
                            'action' : action,
                            'renamed' : [str(renamed)] }

                # Check for database and log locations and add them
                auxlocs = self.findAuxLoc( instloc[0] )
                for loctype in auxlocs.keys():
                    locs = auxlocs[ loctype ]
                    for loc in locs.keys():
                        path = ( locs[loc] )
                        instinfo[ loc ] = path

                # finally add it to the list
                self.instlist.append( instinfo ) 

# self test
if __name__ == "__main__":
    import sys
    rpminfo=IngRPMInstall(sys.argv[1], "ingres")
    ssinfo=rpminfo.getSSInfo()
    instlist=rpminfo.getInstList()
    print "Saveset:"
    print ssinfo
    print "Installations:"
    print instlist

