#
# : Makefile.SH,v 4.1.1.1 90/10/24 15:42:13 syd Exp $
#
#  Makefile for the documentation of the ELM mail system
#
#         (C) Copyright 1989, 1990, USENET Community Trust
#
#  $Log:	Makefile.SH,v $
# Revision 4.1.1.1  90/10/24  15:42:13  syd
# Add uninstall target
# From: Rusty Haddock
# 
# Revision 4.1  90/04/28  22:41:07  syd
# checkin of Elm 2.3 as of Release PL0
# 
#
# Variables
#	Variables established by Configure
CHMOD		=	chmod
CP		=	cp
DEST		=	/bin
ECHO		=	echo
FORMATTER	=	groff
LIB		=	/usr/local/lib
LN		=	ln
MAN		=	/usr/man/man1
MANEXT		=	1
RM		= 	rm -f
TBL       	=	/usr/bin/tbl

#	Other variables
SHELL		=	/bin/sh

# List of installed man pages (except for wnemail.1 - handled differently)
MAN_LIST		=	$(MAN)/answer.$(MANEXT)		\
				$(MAN)/autoreply.$(MANEXT)	\
				$(MAN)/checkalias.$(MANEXT)	\
				$(MAN)/elm.$(MANEXT)		\
				$(MAN)/fastmail.$(MANEXT)	\
				$(MAN)/filter.$(MANEXT)		\
				$(MAN)/frm.$(MANEXT)		\
				$(MAN)/listalias.$(MANEXT)	\
				$(MAN)/messages.$(MANEXT)	\
				$(MAN)/newalias.$(MANEXT)	\
				$(MAN)/newmail.$(MANEXT)	\
				$(MAN)/printmail.$(MANEXT)	\
				$(MAN)/readmsg.$(MANEXT)

# List of remotely installed man pages (except for wnemail.1 - handled
# differently)
REMOTE_MAN_LIST		=	$(REMOTE)$(MAN)/answer.$(MANEXT)	\
				$(REMOTE)$(MAN)/autoreply.$(MANEXT)	\
				$(REMOTE)$(MAN)/checkalias.$(MANEXT)	\
				$(REMOTE)$(MAN)/elm.$(MANEXT)		\
				$(REMOTE)$(MAN)/fastmail.$(MANEXT)	\
				$(REMOTE)$(MAN)/filter.$(MANEXT)	\
				$(REMOTE)$(MAN)/frm.$(MANEXT)		\
				$(REMOTE)$(MAN)/listalias.$(MANEXT)	\
				$(REMOTE)$(MAN)/messages.$(MANEXT)	\
				$(REMOTE)$(MAN)/newalias.$(MANEXT)	\
				$(REMOTE)$(MAN)/newmail.$(MANEXT)	\
				$(REMOTE)$(MAN)/printmail.$(MANEXT)	\
				$(REMOTE)$(MAN)/readmsg.$(MANEXT)

# List of files installed in LIB directory
LIB_LIST		=	$(LIB)/elm-help.0	\
				$(LIB)/elm-help.1	\
				$(LIB)/elm-help.2	\
				$(LIB)/elm-help.3	\
				$(LIB)/elmrc-info

# List of files installed in remote LIB directory
REMOTE_LIB_LIST		=	$(REMOTE)$(LIB)/elm-help.0	\
				$(REMOTE)$(LIB)/elm-help.1	\
				$(REMOTE)$(LIB)/elm-help.2	\
				$(REMOTE)$(LIB)/elm-help.3	\
				$(REMOTE)$(LIB)/elmrc-info

# List of formatted guides
FORMATTED_LIST		=	Alias.fmtd	\
				Config.fmtd 	\
				Cover.fmtd	\
				Filter.fmtd	\
				Form.fmtd	\
				Ref.fmtd	\
				Users.fmtd

# Targets
#	The following are no-ops
release:
debug:
all:
lint:
clean:	

install:	$(LIB_LIST) $(MAN_LIST) $(MAN)/wnewmail.$(MANEXT)

uninstall:
		$(RM) $(LIB_LIST) $(MAN_LIST) $(MAN)/wnewmail.$(MANEXT)

#	Dependencies and rules for installing man pages and lib files
#	taken from a remote system
#	This is the only target that gets installed even if not out-of-date
#	with respect the files from which it is installed.
rmt-install:	rmt-defined
		$(RM) $(MAN)/newmail.$(MANEXT) $(MAN)/wnewmail.$(MANEXT)
		$(CP) $(REMOTE_MAN_LIST) $(MAN)
		$(CHMOD) u+w,a+r $(MAN_LIST)
		$(LN) $(MAN)/newmail.$(MANEXT) $(MAN)/wnewmail.$(MANEXT)
		$(CP) $(REMOTE_LIB_LIST) $(LIB)
		$(CHMOD) u+w,a+r $(LIB_LIST)

rmt-defined:
	@(if [ "$(REMOTE)" = "" ];\
	  then\
	    $(ECHO) "You need to define 'REMOTE' as the remote file system";\
	    $(ECHO) "for this particular command. The easiest way to do this";\
	    $(ECHO) "to type:";\
	    $(ECHO) "        make REMOTE=<remote file system> rmt-install";\
	    exit 1;\
	  fi);

#	Dependencies and rules for installing man pages and lib files
$(MAN)/answer.$(MANEXT):	answer.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/autoreply.$(MANEXT):	autoreply.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/checkalias.$(MANEXT):	chkalias.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/elm.$(MANEXT):		elm.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/fastmail.$(MANEXT):	fastmail.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/filter.$(MANEXT):	filter.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/frm.$(MANEXT):		frm.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/listalias.$(MANEXT):	listalias.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/messages.$(MANEXT):	messages.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/newalias.$(MANEXT):	newalias.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/newmail.$(MANEXT):	newmail.1
				$(RM) $@
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/printmail.$(MANEXT):	printmail.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/readmsg.$(MANEXT):	readmsg.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(MAN)/wnewmail.$(MANEXT):	$(MAN)/newmail.$(MANEXT)
				$(RM) $@
				$(LN) $? $@

$(LIB)/elm-help.0:		elm-help.0
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(LIB)/elm-help.1:		elm-help.1
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(LIB)/elm-help.2:		elm-help.2
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(LIB)/elm-help.3:		elm-help.3
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

$(LIB)/elmrc-info:		elmrc-info
				$(CP) $? $@
				$(CHMOD) u+w,a+r $@

#	Dependencies and rules for formatting guides
documentation:	$(FORMATTED_LIST)

Alias.fmtd:	Alias.guide
		$(TBL) $? | $(FORMATTER) tmac.n - > $@

Config.fmtd:	Config.guid
		$(TBL) $? | $(FORMATTER) tmac.n - > $@

Cover.fmtd:	Elm.cover
		$(FORMATTER) tmac.n $? > $@

Filter.fmtd:	Filter.guid
		$(TBL) $? | $(FORMATTER) tmac.n - > $@

Form.fmtd:	Form.guide
		$(FORMATTER) tmac.n $? > $@

Ref.fmtd:	Ref.guide
		$(FORMATTER) tmac.n $? > $@

Users.fmtd:	Users.guide
		$(TBL) $? | $(FORMATTER) tmac.n - > $@
