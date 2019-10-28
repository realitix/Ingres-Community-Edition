#
#   NMAKE makefile for Synchronous OpenAPI Wrappers library
#

.SUFFIXES:
.SUFFIXES: .exe .lib .obj .c .h

CC     = CL
LIBCMD = LIB $(LIBFLAGS)
CFLAGS = /G5 /Gd /W4 /Ox /MT -I%II_SYSTEM%\ingres\files

SRCS = apiconn.c apievent.c apiproc.c apiquery.c apisupp.c apitran.c
OBJS = $(SRCS:.c=.obj)

.c.obj:
	$(CC) $(CFLAGS) -c $*.c > $*.err

all:        syncapiw.lib

syncapiw.lib: $(OBJS)
    !$(LIBCMD) /OUT:$@ $(OBJS)
