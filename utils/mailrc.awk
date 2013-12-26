#
# @(#)$Id: mailrc.awk,v 4.1 90/04/28 22:44:44 syd Exp $
#	Copyright (c) 1986, 1987 Dave Taylor
#	Copyright (c) 1988, 1989, 1990 USENET Community Trust
# Bug reports, patches, comments, suggestions should be sent to:
#
#	Syd Weinstein, Elm Coordinator - elm@DSI.COM
#					 dsinc!elm
#
# $Log:	mailrc.awk,v $
# Revision 4.1  90/04/28  22:44:44  syd
# checkin of Elm 2.3 as of Release PL0
#
#


BEGIN {
	print "# MSG alias_text file, from a .mailrc file..."
	print ""
      }

next_line == 1 {

	next_line = 0;
        group = ""
	for (i = 1; i <= NF; i++) {
	  if (i == NF && $i == "\\") sep = ""
	  else                       sep = ", "

	  if ($i == "\\") {
	    group = sprintf("%s,", group)
	    next_line = 1;
	  }
	  else if (length(group) > 0)
	    group = sprintf("%s%s%s", group, sep, $i);
	  else
	    group = $i;
	  }
	  print "\t" group

	}

$1 ~ /[Aa]lias|[Gg]roup/ {

	if ( NF == 3)
	  print $2 " = user alias = " $3;
	else {
	  group = ""
	  for (i = 3; i <= NF; i++) {
	    if (i == NF && $i == "\\") sep = ""
	    else        sep = ", "

	    if ($i == "\\") {
 	      group = sprintf("%s,", group)
 	      next_line = 1;
	    }
	    else if (length(group) > 0)
 	      group = sprintf("%s%s%s", group, sep, $i);
	    else
 	      group = $i;
	    }
	    print $2 " = group alias = " group;
	  }
 	}
