#
#  Makefile for the Elm system filter program for OS/2
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

OS2_OBJ		= getopt$O os2util$O
FILTER_OBJ	= actions$O buffer$O filter$O lock$O parse$O \
		  rules$O summarize$O utils$O opt_utils$O $(OS2_OBJ)

# Standard targets

all:		filter.exe
		
clean:		
		$(RM) $(FILTER_OBJ)

# Dependencies and rules for compiling programs

filter.exe:	$(FILTER_OBJ) $(DEF)
		$(CC) $(LFLAGS) -o $@ $(DEF) $(FILTER_OBJ) $(LIBS)

# Dependencies and rules for compiling C programs

.SUFFIXES: .c $O

.c$O:
		$(CC) $(CFLAGS) $(DEFINES) -c $<

# Dependencies of header files upon other header files they include

.PRECIOUS:		$(INCLDIR)/defs.h $(INCLDIR)/elm.h $(INCLDIR)/headers.h

$(INCLDIR)/defs.h:	../config.h $(INCLDIR)/sysdefs.h
			$(TOUCH) $@

$(INCLDIR)/elm.h:	$(INCLDIR)/curses.h $(INCLDIR)/defs.h
			$(TOUCH) $@

$(INCLDIR)/headers.h:	$(INCLDIR)/curses.h $(INCLDIR)/defs.h
			$(TOUCH) $@

# Dependencies and rules for C object files

actions$O:	$(INCLDIR)/defs.h $(INCLDIR)/filter.h
filter$O:	$(INCLDIR)/defs.h $(INCLDIR)/filter.h
lock$O:		$(INCLDIR)/defs.h $(INCLDIR)/filter.h
parse$O:	$(INCLDIR)/defs.h $(INCLDIR)/filter.h
rules$O:	$(INCLDIR)/defs.h $(INCLDIR)/filter.h
summarize$O:	$(INCLDIR)/defs.h $(INCLDIR)/filter.h
utils$O:	$(INCLDIR)/defs.h $(INCLDIR)/filter.h

opt_utils$O:	../src/opt_utils.c $(INCLDIR)/headers.h
		$(CC) $(CFLAGS) $(DEFINES) -c ../src/opt_utils.c

getopt$O:	$(OS2DIR)/getopt.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/getopt.c
os2util$O:	$(OS2DIR)/os2util.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/os2util.c
movemail$O:	$(OS2DIR)/movemail.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/movemail.c
