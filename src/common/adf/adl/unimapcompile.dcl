$! Enable DECC$FILENAME_UNIX_REPORT for the unimapcompile
$
$ full_path = F$PARSE( "''P2'",,,, "NO_CONCEAL") - "]["
$
$ d_path = F$ELEMENT(0, "]", full_path) + "]"
$
$ filenm = F$ELEMENT( 0, ";", full_path - d_path)
$
$ SET DEF 'd_path'
$
$ define/user DECC$FILENAME_UNIX_REPORT enable
$ mc ING_BUILD:[utility]UNIMAPCOMPILE.EXE 'p1' 'filenm' 'p3' 'p4' 'p5' 'p6' 'p7' 'p8'
