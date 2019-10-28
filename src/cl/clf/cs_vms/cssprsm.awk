# Copyright (c) 2011 Actian Corporation
#
# Scan the file looking for %structure,field% tokens, If a token is found,
# create a chkoffset.c file that will will be used to create a SED command
# file that will replace the tokens with the offset of the field (in bytes)
# from the start of the structure.
#
# Usage:
#    (AWK) -f cssprsm.awk
#
# History:
#    26-may-2011 (horda03)
#       Created.
#    19-Jul-2011 (horda03) b125458
#       iicommon.h required with cs.h

function write_cmd (txt) {

   printf( "%s\n", txt) > "chkoffset.c";
}

function do_c_header() {

   write_cmd( "#include <compat.h>");
   write_cmd( "#include <iicommon.h>");         
   write_cmd( "#include <cs.h>");         
   write_cmd( "#include <csinternal.h>");
   write_cmd( "#include <stdio.h>");
   write_cmd( "" );
   write_cmd( "#define CHK_OFFSET(token, structure, field) \\");
   write_cmd( "           printf( \"s+%s+%d+\\n\", token, &(((structure *) 0)->field))");
   write_cmd( "" );
   write_cmd( "int main()");
   write_cmd( "{");
}

BEGIN {
        found = 0;
}

$1 == ";" { next }

$0 ~/\%.*,.*\%/ { n = split( $0, t, "%" );

                  if ( token [ t [2] ] == 0 )
                  {
                     token [ t [2] ] = 1;

                     n = split( t [2], p, ",");

                     if (!found)
                     {
                        do_c_header();
                        found = 1;
                     }

                     write_cmd( "   CHK_OFFSET( " "\"%" t [2] "%\", " p [1] ", "  p [2] ");");
                  }
}

END {
      if (found)
      {
         write_cmd( "   exit(0);" );
         write_cmd( "}" );
      }
}
