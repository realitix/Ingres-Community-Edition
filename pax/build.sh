gcc  -Wl,-s -Ibsd -O2 -o pax -D_PATH_DEFTAPE='"/dev/st0"' -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 *.c bsd/*.c
