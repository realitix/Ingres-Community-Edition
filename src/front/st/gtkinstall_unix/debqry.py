#!/usr/bin/python
#
# Copyright Actian Corporation 2009, 2011. All Rights Reserved
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

# Name: debqry.py
#
# Description:
#       Module for querying DEB/APT saveset and database info.
#
# Usage:
#    import debqry
#
## History:
##	02-May-2011 (hanje04)
##	    Created.
##	01-Sep-2011 (hanje04)
##	    Fix up package handling after merging with main to stop
##	    conflict between Ingres and VectorWise if one is present
##	    whilst the other is being installed.
from instutil import InstanceError, SSError, IngInstall, debugPrint
import os
from apt.cache import FilteredCache, Cache, Filter
from apt import Package
from apt.progress.text import OpProgress
from gettext import gettext as _

class NewerInstanceError(InstanceError):
    pass

class IngDebFilter(Filter):
    def __init__(self, product):
	Filter.__init__(self)
	if product == 'vectorwise':
	     self.basename='ingresvw'
	else:
	     self.basename='ingres'

    def apply(self, pkg):
	if pkg.name.startswith(self.basename):
	    return True
	else:
	    return False

class IngDebInstall(IngInstall):
    def qrySS(self):
        self.setupDebSSRepo()

    def qryDB(self):
        self.searchDebCache()

    def setupDebSSRepo(self):
       """ Setup DEB saveset repository to search"""
       repodir=os.path.join(self.sslocation, "apt")
       srclist=os.path.join("/etc/apt/sources.list.d",
				self.product + "_local.list")
       repoconf="deb file:%s stable non-free\n" % repodir

       # sources.list file for ss
       try:
            srcfd=open(srclist, 'w')
            srcfd.write(repoconf)
            srcfd.close()
       except IOError, e:
            print str(e)
            raise SSError(self.FAIL_OPEN_FILE % srclist)

    def findInstLocFromDeb(self, pkg):
        '''Determine instances location from DEB Package info'''
        if not isinstance(pkg, Package):
            raise TypeError

        i = 0
        flist = pkg.installed_files
        while True:
            try:
                endof = flist[i].index('/ingres/')
                break
            except ValueError:
                i += 1
            except IndexError:
                raise InstanceError(self.FAIL_FIND_INST_LOC)

        return flist[i][:endof]

    def searchDebCache(self):
        '''Use apt.Cache to search for installed and uninstalled
        DEB packages. Uninstalled packages will be those in the 
        saveset. Use results to populate installation info'''
        txtprog = OpProgress()
        cache = Cache(progress=txtprog)
        cache.update()
        cache.open(None)

        ingcache = FilteredCache(cache)
        ingcache.set_filter(IngDebFilter(self.product))

        if len(ingcache) == 0:
            raise InstanceError(self.FAIL_NO_PKGS_FOUND)

        try:
            corepkg=ingcache[self.pkg_basename]
	    corenew=corepkg.candidate
        except Exception, e:
            raise InstanceError(self.FAIL_NO_CORE_PKG_FOUND % e)

        self.ssinfo['package'].append('core')
        self.ssinfo['version'] = [corenew.version]
        self.ssinfo['arch'] = [corenew.architecture]
        self.ssinfo['format'] = ['deb']

        if corepkg.is_installed:
            instpkgs = ['core']
            basename = corepkg.name
            version = corepkg.installed.version
            instloc = self.findInstLocFromDeb(corepkg)
            instid = self.findInstID(instloc)

            if len(instid) != 2:
                print _("Invalid installation ID: '%s', found for %s-%s, using %s") % \
                                    (instid, basename, version, self.dfltinstid)
                instid = self.dfltinstid


            if corepkg.is_upgradable:
                action = 'upgrade'
                debugPrint("upgraded to version: %s" % corenew.version)
            else:
                if len(corepkg.versions) == 1:
                    action = 'modify'
                else:
                    raise NewerInstanceError

            # check the rest of the packages (installed and not)
	    # skip packages from other Ingres products
            for pkg in ingcache:
                if pkg.name == basename:
                    continue # skip core, already have it or don't care
		elif pkg.name.split('-')[0] == basename:
                    if pkg.is_installed:
                        debugPrint("Found: %s" % '_'.join([pkg.name, pkg.installed.version]))
                        instpkgs.append(pkg.name.split('-')[1])
                    else:
                        self.ssinfo['package'].append(pkg.name.split('-')[1])

		    if pkg.is_upgradable:
                        self.ssinfo['package'].append(pkg.name.split('-')[1])
		else:
		    continue # skip the rest

            # create dictionary to store info
            instinfo = { 'ID' : [instid],
                        'basename' : [basename],
                        'version' : [version],
                        'package' : instpkgs,
                        'location' : [instloc],
                        'action' : [action],
                        # always false for DEBs, rename not supported
                        'renamed' : ['False'] }

            # Check for database and log locations and add them
            auxlocs = self.findAuxLoc( instloc )
            for loctype in auxlocs.keys():
                locs = auxlocs[ loctype ]
                for loc in locs.keys():
                    path = ( locs[loc] )
                    instinfo[ loc ] = path

            # finally add it to the list
            self.instlist.append( instinfo )

        else:
            debugPrint("No packages installed")
            debugPrint("The following can be installed:")
            for pkg in ingcache:
                debugPrint('_'.join([pkg.name, pkg.candidate.version]))

