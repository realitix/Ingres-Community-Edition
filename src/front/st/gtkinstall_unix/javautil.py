# Copyright 2011 Actian Corporation. All Rights Reserved
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

# Name: javautil.py
#
# Description:
#	Search for java installations in common locations, JAVA_HOME and
#       execution path
#
# Usage:
#	from javautil import find_java
#       java_list=find_java()
#
## History:
##	20-Sep-2011 (hanje04)
##	    Created.
##	24-Oct-2011 (hanje04)
##	    Mantis 2792
##	    If reported version is a.b.c rather than a.b.c_xx, just use the 
##	    first 3 version numbers for comparison.
##	25-Oct-2011 (hanje04)
##	    Correct typo in is64bit list


def find_java(pname="java", minvers=None, need64bit=False, debug=False):
    """
    utility for locating java JREs and querying version
    """

    import os
    from instutil import debugPrint
    from subprocess import call, PIPE, STDOUT, Popen
    import instutil

    jresearchlist = [ "/usr/lib/jvm", "/usr/jvm", "/usr/local/lib/jvm", "/usr/local/jvm" ]
    jrelocs = []
    jlist = []
    instutil.Debug = debug

    def find_java_home(arg, dirname, fnames):
        if dirname.endswith == "jvm":
            for f in fnames:
                fpath = os.path.join(dirname, f)
                if os.path.islink(fpath) or os.path.isfile(fpath):
                    fnames.remove(f)
            return

        for f in fnames:
            fpath = os.path.join(dirname, f)
            if f.startswith("jre") and not os.path.islink(fpath) and fpath not in jrelocs:
                jrelocs.append(fpath)

    def is_exe(ppath):
        return os.path.exists(ppath) and os.access(ppath, os.X_OK)

    def get_version(ppath):

        vfound = False
        getvers = False
	is64bit = [ "64-Bit", "amd64-64", "x86-64" ]
        bits = 32
        try:
            p = Popen([ppath, '-version'], stderr=STDOUT, stdout=PIPE)
            (out, err) = p.communicate()
        except OSError, e:
            debugPrint("Failed to execute %s: %s\nSkipping..." % (ppath, e))
            raise

        for ent in out.split():
            if not vfound:
                if getvers:
                    version = ent[1:len(ent) - 1]
                    vfound = True
                if ent.startswith("version"):
                    getvers = True
                    continue
            if ent in is64bit:
                bits = 64

        if not vfound:
            version = None

        return (version, bits)

    def jhome_from_exe(ppath):
        rpath = os.path.realpath(ppath)
        if "alternatives" in rpath.split(os.sep):
            return None
        else:
            path = os.sep.join(rpath.split(os.sep)[:-2])
            if path not in jrelocs:
                jrelocs.append(path)

    def jhome_from_path():
        """
        Walk the contents of the PATH variable and check for
        Java executables
        """
        pathlist = os.environ["PATH"].split(os.pathsep)
        for path in pathlist:
            exe_file = os.path.join(path, pname)
            if is_exe(exe_file):
                jhome_from_exe(exe_file)

    def check_version(vers, minvers=minvers):
        debugPrint("Comparing minvers=%s, vers=%s\n" % (minvers, vers))
        minvers = tup_from_vers(minvers)
        vers = tup_from_vers(vers)
	vlen=len(vers)
        debugPrint("Version length is %s " % vlen)
	if vers >= minvers[:vlen - 1]:
            debugPrint("vers OK")
            return True
        else:
            debugPrint("vers too low")
            return False

    def tup_from_vers(vers):
        """ version string looks like x.y.z1_z2 """
        vers = vers.split('_')
        vsplit = vers[0].split('.')
        try:
            vsplit.append(vers[1])
        except IndexError:
            pass
        return tuple(vsplit)

    """
    main body of method starts here
    Search JAVA_HOME
    """
    try:
        jhome = os.path.join(os.environ["JAVA_HOME"])
        if jhome not in jrelocs:
            jrelocs = [os.path.join(os.environ["JAVA_HOME"])] + jrelocs
    except KeyError:
        pass

    """ Search common locations """
    for dir in jresearchlist:
        os.path.walk(dir, find_java_home, None)

    """ Search execution path """
    jhome_from_path()

    """ Store version and location info for all JREs """
    for jhome in jrelocs:
        try:
            exe_file = os.path.join(jhome, os.sep.join(["bin", "java"]))
            (vers, bits) = get_version(exe_file)
            debugPrint("adding " + exe_file)
            if minvers:
                versok = check_version(vers)
            else:
                versok = True

            if need64bit and bits != 64:
                bitsok = False
            elif bits == 32:
                bitsok = False
            else:
                bitsok = True

            if versok and bitsok:
                jlist.append({'JAVA_HOME': jhome, 'version': vers, 'bits': str(bits)})
        except OSError:
            continue

    if len(jlist) > 0:
        return jlist
    else:
        return None

if __name__ == "__main__":
    from platform import architecture as arch
    if arch()[0] == "64bit":
        need64bit = True
    else:
        need64bit = False

    list = find_java(minvers="1.6.0_14", need64bit=need64bit, debug=True)
    print list
