
static char rcsid[] ="@(#)$Id: utils.c,v 4.1.1.2 90/10/24 16:08:29 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.2 $   $State: Exp $
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
 * $Log:	utils.c,v $
 * Revision 4.1.1.2  90/10/24  16:08:29  syd
 * Add FAILED_SAVE case to log
 * From: Steve Cambell
 *
 * Revision 4.1.1.1  90/07/12  20:23:19  syd
 * patch fixes some minor typing mistakes in error messages
 * From: hz247bi@duc220.uni-duisburg.de (Bieniek)
 *
 * Revision 4.1  90/04/28  22:42:03  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Utility routines for the filter program...

**/

#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <fcntl.h>

#include "defs.h"
#include "filter.h"

#undef fflush

leave(reason)
char *reason;
{
	if (outfd != NULL)
	  fprintf(outfd,"filter (%s): LEAVE %s\n", username, reason);
	if (outfd != NULL) fclose(outfd);
	exit(1);
}

log(what)
int what;
{
	/** make an entry in the log files for the specified entry **/

	FILE *fd;
	char filename[SLEN];

	if (! show_only) {
	  sprintf(filename, "%s/%s", home, filtersum);	/* log action once! */
	  if ((fd = fopen(filename, "a")) == NULL) {
	    if (outfd != NULL)
	      fprintf(outfd, "filter (%s): Couldn't open log file %s\n",
		    username, filename);
	    fd = stdout;
	  }
	  fprintf(fd, "%d\n", rule_choosen);
	  fclose(fd);
	}

	sprintf(filename, "%s/%s", home, filterlog);

	if (show_only)
	  fd = stdout;
	else if ((fd = fopen(filename, "a")) == NULL) {
	  if (outfd != NULL)
	    fprintf(outfd, "filter (%s): Couldn't open log file %s\n",
		  username, filename);
	  fd = stdout;
	}

#ifdef _IOFBF
	setvbuf(fd, NULL, _IOFBF, BUFSIZ);
#endif

	if (strlen(from) + strlen(subject) > 60)
	  fprintf(fd, "\nMail from %s\n\tabout %s\n", from, subject);
	else
	  fprintf(fd, "\nMail from %s about %s\n", from, subject);

	if (rule_choosen != -1)
	  if (rules[rule_choosen].condition->matchwhat == TO)
	    fprintf(fd, "\t(addressed to %s)\n", to);

	switch (what) {
	  case DELETE_MSG : fprintf(fd, "\tDELETED");			break;
	  case FAILED_SAVE: fprintf(fd, "\tSAVE FAILED for file \"%s\"",
				rules[rule_choosen].argument2);		break;
	  case SAVE       : fprintf(fd, "\tSAVED in file \"%s\"",
				rules[rule_choosen].argument2);		break;
	  case SAVECC     : fprintf(fd,"\tSAVED in file \"%s\" AND PUT in mailbox",
				rules[rule_choosen].argument2);  	break;
	  case FORWARD    : fprintf(fd, "\tFORWARDED to \"%s\"",
				rules[rule_choosen].argument2);		break;
	  case EXEC       : fprintf(fd, "\tEXECUTED \"%s\"",
				rules[rule_choosen].argument2);		break;
	  case LEAVE      : fprintf(fd, "\tPUT in mailbox");		break;
	}

	if (rule_choosen != -1)
	  fprintf(fd, " by rule #%d\n", rule_choosen+1);
	else
	  fprintf(fd, ": the default action\n");

	fflush(fd);
	fclose(fd);
}

int
contains(string, pattern)
char *string, *pattern;
{
	/** Returns TRUE iff pattern occurs IN IT'S ENTIRETY in buffer. **/

	register int i = 0, j = 0;

	while (string[i] != '\0') {
	  while (tolower(string[i++]) == tolower(pattern[j++]))
	    if (pattern[j] == '\0')
	      return(TRUE);
	  i = i - j + 1;
	  j = 0;
	}
	return(FALSE);
}

char *itoa(i, two_digit)
int i, two_digit;
{
	/** return 'i' as a null-terminated string.  If two-digit use that
	    size field explicitly!  **/

	static char value[10];

	if (two_digit)
	  sprintf(value, "%02d", i);
	else
	  sprintf(value, "%d", i);

	return( (char *) value);
}

lowercase(string)
char *string;
{
	/** translate string into all lower case **/

	register int i;

	for (i= strlen(string); --i >= 0; )
	  if (isupper(string[i]))
	    string[i] = tolower(string[i]);
}
