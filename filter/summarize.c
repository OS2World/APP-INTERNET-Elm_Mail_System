
static char rcsid[] ="@(#)$Id: summarize.c,v 4.1 90/04/28 22:42:02 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1 $   $State: Exp $
 *
 * 			Copyright (c) 1986, 1987 Dave Taylor
 * 			Copyright (c) 1988, 1989, 1990 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein - elm@DSI.COM
 *			dsinc!elm
 *
 *******************************************************************************
 * $Log:	summarize.c,v $
 * Revision 4.1  90/04/28  22:42:02  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This routine is called from the filter program (or can be called
    directly with the correct arguments) and summarizes the users filterlog
    file.  To be honest, there are two sorts of summaries that are
    available - either the '.filterlog' file can be output (filter -S)
    or a summary by rule and times acted upon can be output (filter -s).
    Either way, this program will delete the two associated files each
    time ($HOME/.filterlog and $HOME/.filtersum) *if* the -c option is
    used to the program (e.g. clear_logs is set to TRUE).

**/

#include <stdio.h>

#include "defs.h"

#include "filter.h"

show_summary()
{
	/* Summarize usage of the program... */

	FILE   *fd;				/* for output to temp file! */
	char filename[SLEN],			/* name of the temp file    */
	     buffer[SLEN];			/* input buffer space       */
	int  erroneous_rules = 0,
	     default_rules   = 0,
	     messages_filtered = 0,		/* how many have we touched? */
	     rule,
	     applied[MAXRULES];

	sprintf(filename, "%s/%s", home, filtersum);

	if ((fd = fopen(filename, "r")) == NULL) {
	  if (outfd != NULL)
	    fprintf(outfd,"filter (%s): Can't open filtersum file %s!\n",

		    username, filename);
	  if (outfd != NULL) fclose(outfd);
	  exit(1);
	}

	for (rule=0;rule < MAXRULES; rule++)
	  applied[rule] = 0;			/* initialize it all! */

	/** Next we need to read it all in, incrementing by which rule
	    was used.  The format is simple - each line represents a
	    single application of a rule, or '-1' if the default action
	    was taken.  Simple stuff, eh?  But oftentimes the best.
	**/

	while (fgets(buffer, SLEN, fd) != NULL) {
	  if ((rule = atoi(buffer)) > total_rules || rule < -1) {
	    if (outfd != NULL)
	      fprintf(outfd,
      "filter (%s): Warning - rule #%d is invalid data for short summary!!\n",
	            username, rule);
	    erroneous_rules++;
	  }
	  else if (rule == -1)
	    default_rules++;
	  else
	    applied[rule]++;
	  messages_filtered++;
	}

	fclose(fd);

	/** now let's summarize the data... **/

	if (outfd == NULL) return;		/* no reason to go further */

	fprintf(outfd,
		"\n\t\t\tA Summary of Filter Activity\n");
	fprintf(outfd,
		  "\t\t\t----------------------------\n\n");

	fprintf(outfd,"A total of %d message%s %s filtered:\n\n",
		messages_filtered, plural(messages_filtered),
		messages_filtered > 1 ? "were" : "was");

	if (erroneous_rules)
	  fprintf(outfd,
	          "[Warning: %d erroneous rule%s logged and ignored!]\n\n",
		   erroneous_rules, erroneous_rules > 1? "s were" : " was");

	if (default_rules) {
	   fprintf(outfd,
 "The default rule of putting mail into your mailbox\n");
	   fprintf(outfd, "\tapplied %d time%s (%d%%)\n\n",
		   default_rules, plural(default_rules),
		   (default_rules*100+(messages_filtered>>1))/messages_filtered
	  	  );
	}

	 /** and now for each rule we used... **/

	 for (rule = 0; rule < total_rules; rule++) {
	   if (applied[rule]) {
	      fprintf(outfd, "Rule #%d: ", rule+1);
	      switch (rules[rule].action) {
		  case LEAVE:	    fprintf(outfd, "(leave mail in mailbox)");
				    break;
		  case DELETE_MSG:  fprintf(outfd, "(delete message)");
				    break;
		  case SAVE  :      fprintf(outfd, "(save in \"%s\")",
					    rules[rule].argument2);		break;
		  case SAVECC:      fprintf(outfd,
					    "(left in mailbox and saved in \"%s\")",
					    rules[rule].argument2);		break;
		  case FORWARD:     fprintf(outfd, "(forwarded to \"%s\")",
					    rules[rule].argument2);		break;
		  case EXEC  :      fprintf(outfd, "(given to command \"%s\")",
					    rules[rule].argument2);		break;
	     }
	     fprintf(outfd, "\n\tapplied %d time%s (%d%%)\n\n",
		     applied[rule], plural(applied[rule]),
	            (applied[rule]*100+(messages_filtered>>1))/messages_filtered
		    );
	  }
	}

	if (long_summary) {

	  /* next, after a ^L, include the actual log file... */

	  sprintf(filename, "%s/%s", home, filterlog);

	  if ((fd = fopen(filename, "r")) == NULL) {
	    fprintf(outfd,"filter (%s): Can't open filterlog file %s!\n",
		      username, filename);
	  }
	  else {
	    fprintf(outfd, "\n\n\n%c\n\nExplicit log of each action;\n\n",
		    (char) 12);
	    while (fgets(buffer, SLEN, fd) != NULL)
	      fprintf(outfd, "%s", buffer);
	    fprintf(outfd, "\n-----\n");
	    fclose(fd);
	  }
	}

	/* now remove the log files, please! */

	if (clear_logs) {
	  sprintf(filename, "%s/%s", home, filterlog);
	  unlink(filename);
	  sprintf(filename, "%s/%s", home, filtersum);
	  unlink(filename);
	}

	return;
}
