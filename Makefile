#
# : Makefile.SH,v 4.1 90/04/28 22:40:54 syd Exp $
#
#  Makefile for the entire ELM mail system
#
#         (C) Copyright 1986, 1987, by Dave Taylor
#         (C) Copyright 1988, 1989, 1990, USENET Community Trust
#
#  $Log:	Makefile.SH,v $
# Revision 4.1  90/04/28  22:40:54  syd
# checkin of Elm 2.3 as of Release PL0
# 
#

# Targets
#	Give default target first and alone
default_target: release

#	Targets that are simply executed in each subordinate makefile as is
release debug install uninstall lint clean:
		cd src & $(MAKE) -$(MAKEFLAGS) $@
		cd utils & $(MAKE) -$(MAKEFLAGS) $@
		cd filter & $(MAKE) -$(MAKEFLAGS) $@
		cd doc & $(MAKE) -$(MAKEFLAGS) $@

#	Targets that are really in subordinate make files
documentation:
	cd doc & $(MAKE) -$(MAKEFLAGS) $@

elm:
	cd src & $(MAKE) -$(MAKEFLAGS)

#	The dummy dependency here prevents make from thinking the target is the
#	filter directory that sits in the current directory, rather than
#	an abstract target.
filter: _filter

_filter:
	cd filter & $(MAKE) -$(MAKEFLAGS)

#	The dummy dependency here prevents make from thinking the target is the
#	utils directory that sits in the current directory, rather than
#	an abstract target.
utils: _utils

_utils:
	cd utils & $(MAKE) -$(MAKEFLAGS)

