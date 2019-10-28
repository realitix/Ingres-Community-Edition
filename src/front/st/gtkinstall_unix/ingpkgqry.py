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

# Name: ingpkgqry
#
# Description:
#       Utility for querying package management system (eg RPM) for
#       existing Ingres instances and writing out the information
#	as XML on a per instance basis.
#	The packages contained in the saveset are also queried.
#
#	Currently only supported for RPM packages but contains code
#	for querying DEBs (Debian package manager) too.
#
# Usage:
#    ingpkgqry -s <pkgdir> [-o <filename>][-p]
#	-s		Search <pkgdir> for Ingres packages to compare against
#			those installed
#	-o		Write output to <filename>
#			(default is to write to ./ingpkginfo.xml)
#	-p		Pretty print to stdout
#			(ingpkginfo.xml will still be written)
#
## History:
##	06-Aug-2009 (hanje04)
##	    BUG 122571
##	    Created from 2007 prototype
##	15-Apr-2010 (bonro01)
##	    Updated to detect Ingres vectorwise installs
##	13-Apr-2010 (hanje04)
##	    BUG 123788
##	    Continue if we fail to load the XML libraries. Only fail after 
##	    trying to import the rpm/apt libraries too, so we can report both
##	    errors if neccessary.	
##	20-May-2010 (hanje04)
##	    SIR 123791
##	    Add 'product' variable and use it to define exectution mode for
##	    installer. Passed to GUI as attribute of saveset element.
##	25-May-2010 (hanje04)
##	    SIR 123791
##	    Make sure we get ALL the locations in dblocdict
##	25-Jun-2010 (hanje04)
##	    BUG 123984
##	    Make sure core package is added to ssinfo once we've verified it's
##	    there
##	10-Aug-2010 (hanje04)
##	    BUG 124253
##	    For non-renamed installs, get installation ID from symbol.tbl as
##	    it may not be II
##	19-Aug-2010 (hanje04)
##	    BUG 124284
##	    Only close() cfgfile if we've opened it in findInstID(). Improve
##	    error meessage too.
##	22-Sep-2010 (hanje04)
##	    BUG 124480
##	    SD 126680
##	    Correct package name for 32bit RPM so we find it when we look.
##	04-Oct-2010 (hanje04)
##	    BUG 124284/124536
##	    SD 147034
##	    Move close() outside of while loop in findInstID(), to stop it 
##	    prematurely being closed.
##	06-Dec-2010 (hanje04)
##	    Don't present v1.0 instances for upgrade, not supported
##	10-Jan-2011 (hanje04)
##	    BUG 124915
##	    SD 148439
##	    Add II_SYSTEM to lists of locations otherwise generated index
##	    values are wrong.
##	23-Feb-2010
##	    Mantis 1448
##	    Use version-release for finding other packges in installation
##	    so we don't inadvertantly try and remove the wrong doc version.
##	18-Jan-2011 (hanje04)
##	    Include gettext library and wrap all message strings in _( ).
##	01-Mar-2011 (hanje04)
##	    Move cfgfile.close() outside read loop in findInstID() otherwise
##	    we exception if II_INSTALLATION isn't first in symbol.tbl
##      18-May-2011 (hanje04)
##         Major overhaul to add DEBs support. Package query methods broken
##         out into the 3 new modules/classes:
##             - debqry.py for DEBs/Apt
##             - rpmqry.py for RPM
##             - instutil.py for generic search classes used by the other 2
##         Also prevent RPM saveset from trying to run DEBs machine and vice
##         versa.
##      31-May-2011 (hanje04)
##         Mantis 2014
##         Don't use "exception MyError as e:" syntax, not supported for 
##         Python 2.4 (RHEL 5.6)
##      20-Jun-2011 (hanje04)
##          Mantis 2135 & 2138
##          Allow installation of signed saveset even if it can't be verified
##	25-Jul-2011
##	   Merge changes from codev.
##	01-Sep-2011
##	   Work out the product from the top level directory name.
##      20-Sep-2011 (hanje04)
##          Bug 125688
##          Add JRE discovery for remote mgmt service installation
##      20-Oct-2011 (hanje04)
##          Mantis 2771
##          Handle case where find_java() returns None (i.g. no java found)


# load external modules
import sys
import os
import re
import getopt
from platform import architecture as arch
from gettext import gettext as _
from gettext import bindtextdomain, textdomain
import gettext
from locale import setlocale, LC_ALL
from instutil import SSError, InstanceError
from javautil import find_java
 
# Setup local language support
setlocale(LC_ALL, '')

# Assume we're being run from the saveset dir to start with
cwd=os.getcwd()
argv0=sys.argv[0]
if argv0.startswith('/'):
   tmploc=os.path.dirname(os.path.dirname(argv0))
else:
   tmploc=cwd + "/" + os.path.dirname(os.path.dirname(argv0))

if os.path.basename(tmploc) == ".":
    tmploc=os.path.dirname(tmploc)

gtdomain="inginstgui"
gtmsgdir=tmploc + "/locale"

bindtextdomain(gtdomain, gtmsgdir)
textdomain(gtdomain)



# define globals
Debug = False
pprint = False
outfile='ingpkginfo.xml'
xmlok=False
vectorwise=False
ingres=False
minjavavers='1.6.0_14'
if arch()[0] == '64bit':
    need64bit = True
else:
    need64bit = False

# define product here as 'ingres' or 'vectorwise' to
# dictate which path is followed through the installer
if os.path.basename(tmploc).startswith('ingresvw'):
    vectorwise = True
elif os.path.basename(tmploc).startswith('ingres'):
    ingres = True
else:
    print tmploc

if ingres:
    product='ingres'
else:
    product='vectorwise'
checksig=False

# error messages
FAIL_IMPORT_RPMQRY = _("ERROR: Failed to load Python module \"rpm\" \n\
The 'rpm-python' package must be installed to run this installer")
FAIL_IMPORT_DEBQRY = _("ERROR: Failed to load Python module \"apt\" \n\
The 'python-apt' package must be installed to run this installer")
FAIL_IMPORT_XML = _("ERROR: Failed to load Python module \"xml\" \n\
The 'python-xml' package must be installed to run this installer")
FAIL_WRITE_XML=_("ERROR: Failed to write XML to: %s\n%s\n")
FAIL_NEWER_DEB_INSTANCE = _("ERROR: A newer version of the product is already installed.\nMultiple instances are not supported for DEBs packaging,\nno further action is possible.")

def usage():
    print _("Usage:\n    %s -s <pkgdir> [-o <filename>][-p][-c]") % sys.argv[0].split('/')[-1]
    print _("\t-s\t\tSearch <pkgdir> for Ingres packages to compare against\n\t\t\tthose installed")
    print _("\t-o\t\tWrite output to <filename>\n\t\t\t(default is to write to ./ingpkginfo.xml)")
    print _("\t-p\t\tPretty print to stdout\n\t\t\t(ingpkginfo.xml will still be written)")
    print _("\t-c\t\tEnable GPG signature checking")
    sys.exit(-1)

def debugPrint( message ):
    if Debug:
        print message

    return

def writeXML( filename, xmldoc ):
    """Write XML Document to file"""
    try:
        outf = file( filename, 'w' )
        outf.truncate()
        outf.writelines(xmldoc.toxml())
    except OSError, e:
	print FAIL_WRITE_XML % (outfile, repr(e))
        return 1

    outf.close()
    return 0

def genXMLTagsFromDict( xmldoc, ele, dict ):
    """Creates Element-Textnode pairs from dictionary keys and data"""
    for tag in dict.keys():
	for ent in dict[ tag ]:
	    newele = xmldoc.createElement( tag ) # create new element
	    if tag == "package": # need to add attribute special cases
		newele.setAttribute( "idx", "%d" % ingpkgsdict[ ent ] ) 
	    elif re.match( "II_", tag ) != None:
		newele.setAttribute( "idx", "%d" % dblocdict[ tag ] ) 

	    ele.appendChild( newele ) # attach it to current element
	    tn = xmldoc.createTextNode( ent ) # create text node
	    newele.appendChild( tn ) # attch it to the new element

    return
	    
def genXML( instlist, ssinfo ):
    """Write Ingres instance data out in XML."""
    # Create document for instance data and base element
    instdoc = Document()
    pkginfo = instdoc.createElement( "IngresPackageInfo" )
    instdoc.appendChild( pkginfo )

    # Saveset element
    saveset = instdoc.createElement("saveset")
    saveset.setAttribute( "product", product )
    saveset.setAttribute( "checksig", str(checksig) )
    pkginfo.appendChild( saveset )

    # Instances element
    instances = instdoc.createElement("instances")
    pkginfo.appendChild( instances )

    # write out saveset info first
    genXMLTagsFromDict( instdoc, saveset, ssinfo )

    # loop through the list of instances and add to DOM
    for inst in instlist:
	# Create instance
	instance = instdoc.createElement("instance")
	instances.appendChild( instance )

	# now iterate over all the keys using the keys as XML 
	genXMLTagsFromDict( instdoc, instance, inst )
	    
    # loop through the list of JREs
    java = instdoc.createElement("java")
    pkginfo.appendChild(java)

    try:
        for inst in jrelist:
            # JRE element
            jreinst = instdoc.createElement("jre")
            for att in inst.keys():
                jreinst.setAttribute(att, inst[att])

            java.appendChild( jreinst )
    except TypeError:
        pass

    if pprint:
	print instdoc.toprettyxml(indent="  ")

    return writeXML( outfile, instdoc )
   
# Main body of script
if not ingres and not vectorwise:
    print FAIL_MODE_NOT_SET
    usage()

if len(sys.argv) < 3:
    usage()

try:
    opts, args = getopt.getopt(sys.argv[1:], "s:o:pcd")
except getopt.GetoptError, e:
    print repr(e)
    usage()

for o, a in opts:
    if o in ("-s"):
	if a.startswith('/'):
	   sslocation=os.path.dirname(os.path.dirname(a))
	else:
	   sslocation=cwd + "/" + os.path.dirname(os.path.dirname(a))
    elif o in ("-o"):
	outfile=a
    elif o in ("-p"):
	pprint=True
    elif o in ("-c"):
	checksig=True
    elif o in ("-d"):
	Debug=True
	pprint=True
    else:
	usage()

if sslocation is None:
    usage()

# import xml module if we can
try:
    from xml.dom.minidom import Document
    xmlok=True
except ImportError:
    print FAIL_IMPORT_XML

# Which saveset are we working with
if os.path.exists(os.path.join(sslocation,"rpm")):
    pkg="RPM"
else:
    pkg="DEB"

# Search for Ingres packages apropriately
if pkg == "DEB":
    # Load apt module to query DEB repository
    try:
	from debqry import NewerInstanceError
	from debqry import IngDebInstall
	ingpkgqry=IngDebInstall(sslocation, product, checksig)
    except ImportError, e:
	print FAIL_IMPORT_DEBQRY
	sys.exit(-2)
    except NewerInstanceError, e:
	print FAIL_NEWER_DEB_INSTANCE
	sys.exit(-3)
else:
    # Load rpm module to query the repository
    try:
	from rpmqry import IngRPMInstall
	ingpkgqry=IngRPMInstall(sslocation, product, checksig)
    except ImportError, e:
	print FAIL_IMPORT_RPMQRY
	sys.exit(-2)

try:
    ssinfo=ingpkgqry.getSSInfo()
    instlist=ingpkgqry.getInstList()
    ingpkgsdict=ingpkgqry.ingpkgsdict
    dblocdict=ingpkgqry.dblocdict
    jrelist=find_java(minvers=minjavavers, need64bit=need64bit)
    sys.exit(genXML( instlist, ssinfo ))
except SSError, e:
    print repr(e)
    sys.exit(-1)
except InstanceError, e:
    print repr(e)
    sys.exit(-1)

