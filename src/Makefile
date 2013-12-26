#
# Makefile for the Elm mail program for OS/2
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
CRYPTDIR	= ../crypt
DEFINES		= -DOS2 -DDEBUG -I$(INCLDIR) -I$(OS2DIR) -I$(CRYPTDIR)
RM		= rm -f
TOUCH		= touch

# Lists

OS2_OBJ		= alarm$O getopt$O termcap$O os2util$O movemail$O
CRYPT_OBJ	= crypt$O crypt_util$O

ELM_OBJ		= $(OS2_OBJ) $(CRYPT_OBJ) \
	addr_util$O alias$O aliasdb$O aliaslib$O args$O bouncebk$O \
	builtin$O calendar$O conn_to$O curses$O date$O delete$O \
	domains$O edit$O editmsg$O elm$O encode$O errno$O exitprog$O \
	expires$O file$O file_util$O fileio$O forms$O hdrconfg$O help$O \
	init$O in_utils$O leavembox$O limit$O mailmsg1$O mailmsg2$O \
	mailtime$O mkhdrs$O newmbox$O opt_utils$O options$O out_utils$O \
	pattern$O pmalloc$O quit$O read_rc$O remail$O reply$O returnadd$O \
	save_opts$O savecopy$O screen$O showmsg$O showmsg_c$O signals$O \
	softkeys$O sort$O string2$O strings$O syscall$O utils$O validname$O

# Standard targets

all:		elm.exe

clean:		
		$(RM) $(ELM_OBJ)

# Dependencies and rules for compiling programs

elm.exe:	$(ELM_OBJ)
		$(CC) $(LFLAGS) -o $@ $(ELM_OBJ) $(LIBS)

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

# Dependencies of C object files

addr_util$O:	$(INCLDIR)/headers.h
alias$O:	$(INCLDIR)/headers.h
aliasdb$O:	$(INCLDIR)/headers.h
aliaslib$O:	$(INCLDIR)/headers.h
args$O:		$(INCLDIR)/headers.h $(INCLDIR)/patchlevel.h
bouncebk$O:	$(INCLDIR)/headers.h
builtin$O:	$(INCLDIR)/headers.h
calendar$O:	$(INCLDIR)/headers.h
conn_to$O:	$(INCLDIR)/headers.h
curses$O:	$(INCLDIR)/headers.h
date$O:		$(INCLDIR)/headers.h
delete$O:	$(INCLDIR)/headers.h
domains$O:	$(INCLDIR)/headers.h
edit$O:		$(INCLDIR)/headers.h
editmsg$O:	$(INCLDIR)/headers.h
elm$O:		$(INCLDIR)/elm.h
encode$O:	$(INCLDIR)/headers.h
errno$O:	$(INCLDIR)/headers.h
exitprog$O:	$(INCLDIR)/headers.h
expires$O:	$(INCLDIR)/headers.h
file$O:		$(INCLDIR)/headers.h
file_util$O:	$(INCLDIR)/headers.h
fileio$O:	$(INCLDIR)/headers.h
forms$O:	$(INCLDIR)/headers.h
hdrconfg$O:	$(INCLDIR)/headers.h
help$O:		$(INCLDIR)/headers.h
in_utils$O:	$(INCLDIR)/headers.h
init$O:		$(INCLDIR)/headers.h $(INCLDIR)/patchlevel.h
leavembox$O:	$(INCLDIR)/headers.h
limit$O:	$(INCLDIR)/headers.h
mailmsg1$O:	$(INCLDIR)/headers.h
mailmsg2$O:	$(INCLDIR)/headers.h
mailtime$O:	$(INCLDIR)/headers.h
mkhdrs$O:	$(INCLDIR)/headers.h
newmbox$O:	$(INCLDIR)/headers.h
opt_utils$O:	$(INCLDIR)/headers.h
options$O:	$(INCLDIR)/headers.h
out_utils$O:	$(INCLDIR)/headers.h
pattern$O:	$(INCLDIR)/headers.h
pmalloc$O:	$(INCLDIR)/defs.h
quit$O:		$(INCLDIR)/headers.h
read_rc$O:	$(INCLDIR)/headers.h
remail$O:	$(INCLDIR)/headers.h
reply$O:	$(INCLDIR)/headers.h
returnadd$O:	$(INCLDIR)/headers.h
save_opts$O:	$(INCLDIR)/save_opts.h $(INCLDIR)/headers.h
savecopy$O:	$(INCLDIR)/headers.h
screen$O:	$(INCLDIR)/headers.h
showmsg$O:	$(INCLDIR)/headers.h
showmsg_c$O:	$(INCLDIR)/headers.h
signals$O:	$(INCLDIR)/headers.h
softkeys$O:	$(INCLDIR)/headers.h
sort$O:		$(INCLDIR)/headers.h
string2$O:	$(INCLDIR)/headers.h
strings$O:	$(INCLDIR)/headers.h
syscall$O:	$(INCLDIR)/headers.h
utils$O:	$(INCLDIR)/headers.h
validname$O:	$(INCLDIR)/defs.h

alarm$O:	$(OS2DIR)/alarm.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/alarm.c
getopt$O:	$(OS2DIR)/getopt.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/getopt.c
termcap$O:	$(OS2DIR)/termcap.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/termcap.c
os2util$O:	$(OS2DIR)/os2util.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/os2util.c
movemail$O:	$(OS2DIR)/movemail.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(OS2DIR)/movemail.c

crypt$O:	$(CRYPTDIR)/crypt.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(CRYPTDIR)/crypt.c
crypt_util$O:	$(CRYPTDIR)/crypt_util.c
		$(CC) $(CFLAGS) $(DEFINES) -c $(CRYPTDIR)/crypt_util.c
