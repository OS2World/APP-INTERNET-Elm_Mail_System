#
#  Makefile for the Elm system utilities for OS/2
#

release:
	$(MAKE) all CC="gcc -O -Zomf" O=".obj" \
	CFLAGS="" LFLAGS="-s -Zcrtdll" LIBS="../os2/elm.def"

debug:
	$(MAKE) all CC="gcc -g" O=".o" \
	CFLAGS="" LFLAGS="" LIBS="../os2/elm.def"

# Variables

INCLDIR		= ../hdrs
OS2DIR		= ../os2
DEFINES		= -DOS2 -I$(INCLDIR) -I$(OS2DIR)
RM		= rm -f
TOUCH		= touch

# Lists

BINARY_LIST	= answer.exe arepdaemon.exe autoreply.exe fastmail.exe \
	from.exe listalias.exe newalias.exe newmail.exe readmsg.exe \
	rcvmail.exe sndmail.exe

OS2_OBJ		= getopt$O os2util$O # movemail$O
UTIL_OBJ	= answer$O arepdaem$O autoreply$O expand$O \
	fastmail$O from$O listalias$O newalias$O newmail$O readmsg$O \
	opt_utils$O string2$O validname$O errno$O

# Lists of source and object files for each C program

ANSWER_OBJ	= answer$O opt_utils$O string2$O $(OS2_OBJ)
AREPDAEMON_OBJ	= arepdaem$O opt_utils$O errno$O $(OS2_OBJ)
AUTOREPLY_OBJ	= autoreply$O opt_utils$O $(OS2_OBJ)
FASTMAIL_OBJ	= fastmail$O opt_utils$O $(OS2_OBJ)
FRM_OBJ		= from$O expand$O opt_utils$O string2$O $(OS2_OBJ)
LISTALIAS_OBJ	= listalias$O $(OS2_OBJ)
NEWALIAS_OBJ	= newalias$O validname$O opt_utils$O string2$O $(OS2_OBJ)
NEWMAIL_OBJ	= newmail$O expand$O opt_utils$O string2$O $(OS2_OBJ)
READMSG_OBJ	= readmsg$O expand$O opt_utils$O string2$O $(OS2_OBJ)
RCVMAIL_OBJ     = rcvmail$O $(OS2_OBJ)
SNDMAIL_OBJ     = sndmail$O $(OS2_OBJ)

# Standard targets

all:		$(BINARY_LIST)

#	This unusual target enables highly efficial compilation of object files
#	on systems that have the parallel make feature.

objects:	$(UTIL_OBJ) $(OS2_OBJ)

clean:			
		$(RM) $(UTIL_OBJ) $(BINARY_LIST)

# Dependencies and rules for compiling C programs

.SUFFIXES: .c $O

.c$O:
		$(CC) $(CFLAGS) $(DEFINES) -c $<

# Dependencies and rules

.PRECIOUS:	$(INCLDIR)/defs.h $(INCLDIR)/elm.h $(INCLDIR)/headers.h

answer.exe:	$(ANSWER_OBJ)
		$(CC) $(LFLAGS) -o $@ $(ANSWER_OBJ) $(LIBS)

arepdaemon.exe:	$(AREPDAEMON_OBJ)
		$(CC) $(LFLAGS) -o $@ $(AREPDAEMON_OBJ) $(LIBS)

autoreply.exe:	$(AUTOREPLY_OBJ)
		$(CC) $(LFLAGS) -o $@ $(AUTOREPLY_OBJ) $(LIBS)

fastmail.exe:	$(FASTMAIL_OBJ)
		$(CC) $(LFLAGS) -o $@ $(FASTMAIL_OBJ) $(LIBS)

from.exe:	$(FRM_OBJ)
		$(CC) $(LFLAGS) -o $@ $(FRM_OBJ) $(LIBS)

listalias.exe:	$(LISTALIAS_OBJ)
		$(CC) $(LFLAGS) -o $@ $(LISTALIAS_OBJ) $(LIBS)

newalias.exe:	$(NEWALIAS_OBJ)
		$(CC) $(LFLAGS) -o $@ $(NEWALIAS_OBJ) $(LIBS)

newmail.exe:	$(NEWMAIL_OBJ)
		$(CC) $(LFLAGS) -o $@ $(NEWMAIL_OBJ) $(LIBS)

readmsg.exe:	$(READMSG_OBJ)
		$(CC) $(LFLAGS) -o $@ $(READMSG_OBJ) $(LIBS)

rcvmail.exe:	$(RCVMAIL_OBJ)
		$(CC) $(LFLAGS) -o $@ $(RCVMAIL_OBJ) $(LIBS)

sndmail.exe:	$(SNDMAIL_OBJ)
		$(CC) $(LFLAGS) -o $@ $(SNDMAIL_OBJ) $(LIBS)

# Dependencies of header files upon other header files they include

$(INCLDIR)/defs.h:	$(INCLDIR)/../config.h $(INCLDIR)/sysdefs.h
			$(TOUCH) $@

$(INCLDIR)/elm.h:	$(INCLDIR)/curses.h $(INCLDIR)/defs.h
			$(TOUCH) $@

$(INCLDIR)/headers.h:	$(INCLDIR)/curses.h $(INCLDIR)/defs.h
			$(TOUCH) $@

# Dependencies of C object files

answer$O:	$(INCLDIR)/defs.h
arepdaem$O:	$(INCLDIR)/defs.h
autoreply$O:	$(INCLDIR)/defs.h
expand$O:	$(INCLDIR)/defs.h
fastmail$O:	$(INCLDIR)/defs.h $(INCLDIR)/patchlevel.h
from$O:		$(INCLDIR)/defs.h
listalias$O:	$(INCLDIR)/defs.h $(INCLDIR)/sysdefs.h
newalias$O:	$(INCLDIR)/defs.h $(INCLDIR)/sysdefs.h
newmail$O:	$(INCLDIR)/defs.h
readmsg$O:	$(INCLDIR)/defs.h

errno$O:	../src/errno.c $(INCLDIR)/headers.h
		$(CC) $(CFLAGS) $(DEFINES) -c ../src/errno.c
opt_utils$O:	../src/opt_utils.c $(INCLDIR)/headers.h
		$(CC) $(CFLAGS) $(DEFINES) -c ../src/opt_utils.c
string2$O:	../src/string2.c $(INCLDIR)/headers.h
		$(CC) $(CFLAGS) $(DEFINES) -c ../src/string2.c
validname$O:	../src/validname.c $(INCLDIR)/headers.h
		$(CC) $(CFLAGS) $(DEFINES) -c ../src/validname.c

getopt$O:	$(OS2DIR)/getopt.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/getopt.c
os2util$O:	$(OS2DIR)/os2util.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/os2util.c
movemail$O:	$(OS2DIR)/movemail.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/movemail.c
