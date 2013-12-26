
static char rcsid[] = "@(#)$Id: from.c,v 4.1 90/04/28 22:44:41 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1 $   $State: Exp $
 *
 * 			Copyright (c) 1986, 1987 Dave Taylor
 * 			Copyright (c) 1988, 1989, 1990 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log:	from.c,v $
 * Revision 4.1  90/04/28  22:44:41  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** print out whom each message is from in the pending folder or specified
    one, including a subject line if available..

**/

#include <stdio.h>
#include <pwd.h>
#include "defs.h"

static char ident[] = { WHAT_STRING };

#ifdef MMDF
char username[SLEN] = {0};
#endif /* MMDF */

#define LINEFEED	(char) 10

#define metachar(c)	(c == '=' || c == '+' || c == '%')

FILE *mailfile;

int   number = 0,	/* should we number the messages?? */
      verbose = 0;	/* and should we prepend a header? */

main(argc, argv)
int argc;
char *argv[];
{
	char infile[SLEN], *cp ;
	int  multiple_files = 0, output_files = 0, c;
	struct passwd *pass;
#ifndef	_POSIX_SOURCE
	struct passwd *getpwuid();
#endif
	extern int optind;

        initpaths();

	while ((c = getopt(argc, argv, "nv")) != EOF)
	  switch (c) {
	    case (int)'n': number++;		break;
	    case (int)'v': verbose++;	break;
	    case (int)'?': printf("Usage: %s [-n] [-v] {filename | username}\n",
			     argv[0]);
	                   exit(1);
	  }

	if((pass = getpwuid(getuid())) == NULL) {
	  printf("You have no password entry!");
	  exit(1);
	}
	strcpy(username,pass->pw_name);

	infile[0] = '\0';
	if (optind == argc) {
	/*
	 *	determine mail file from environment variable if found,
	 *	else use password entry
	 */
	  optind -= 1;	/* ensure one pass through loop */
	  if ((cp = getenv("MAIL")) == NULL)
	    strcpy(infile, argv[optind] = username);
	  else
	    strcpy(infile, argv[optind] = cp);
	}

	multiple_files = (argc - optind > 1);

	while (optind < argc) {

	  if (multiple_files) {
	    strcpy(infile, argv[optind]);
	    printf("%s%s: \n", output_files++ > 0 ? "\n":"", infile);
	  }
	  else if (infile[0] == '\0')
	    strcpy(infile, argv[optind]);

	  if (metachar(infile[0])) {
	    if (expand(infile) == 0) {
	       fprintf(stderr, "%s: couldn't expand filename %s!\n",
		       argv[0], infile);
	       exit(1);
	    }
	  }

	  if ((mailfile = fopen(infile,"r")) == NULL) {
	      if (infile[0] == '/')
	        printf("Couldn't open folder \"%s\".\n", infile);
	      else {
	        sprintf(infile,"%s%s", mailhome, argv[optind]);
	        if ((mailfile = fopen(infile,"r")) == NULL)
	          printf("Couldn't open folders \"%s\" or \"%s\".\n",
			 argv[optind], infile);
	        else {
		  if (read_headers(1)==0)
	            printf("No mail.\n");
	          fclose(mailfile);
		}
	      }
	  } else {
	    if (read_headers(0)==0)
	      printf("No messages in that folder!\n");
	    fclose(mailfile);
	  }

	  optind++;
	}
	exit(0);
}

int
read_headers(user_mailbox)
int user_mailbox;
{
	/** Read the headers, output as found.  User-Mailbox is to guarantee
	    that we get a reasonably sensible message from the '-v' option
	 **/

	char buffer[SLEN], from_whom[SLEN], subject[SLEN];
	register int in_header = 0, count = 0;
#ifdef MMDF
	int newheader = 0;
#endif /* MMDF */

	while (fgets(buffer, SLEN, mailfile) != NULL) {
	  if (index(buffer, '\n') == NULL && !feof(mailfile)) {
	    int c;
	    while ((c = getc(mailfile)) != EOF && c != '\n')
	      ; /* keep reading */
	  }

#ifdef MMDF
          if (strcmp(buffer, MSG_SEPERATOR) == 0 || !newheader &&
  	      first_word(buffer,"From ") && real_from(buffer, from_whom)) {
	    newheader = 1;
	    subject[0] = '\0';
	    in_header = 1;
	  }
#else
	  if (first_word(buffer,"From ")
	   && real_from(buffer, from_whom)) {
	    subject[0] = '\0';
	    in_header = 1;
	  }
#endif /* MMDF */
	  else if (in_header) {
#ifdef MMDF
            newheader = 0;
	    if (first_word(buffer,"From "))
	      real_from(buffer, from_whom);
#endif /* MMDF */
	    if (first_word(buffer,">From "))
	      forwarded(buffer, from_whom); /* return address */
	    else if (first_word(buffer,"Subject:") ||
		     first_word(buffer,"Re:")) {
	      if (subject[0] == '\0') {
	        remove_first_word(buffer);
		strcpy(subject, buffer);
	      }
	    }
	    else if (first_word(buffer,"From:") ||
		    first_word(buffer, ">From:"))
	      parse_arpa_from(buffer, from_whom);
	    else if (buffer[0] == LINEFEED) {
	      if (verbose && count == 0)
	        printf("%s contains the following messages:\n\n",
			user_mailbox?"Your mailbox" : "Folder");
#ifdef MMDF
	      if (*from_whom == '\0')
                strcpy(from_whom,username);
#endif /* MMDF */
	      ++count;
	      show_header(count, from_whom, subject);
	      in_header = 0;
	    }
	  }
	}
	return(count);
}

int
real_from(buffer, who)
char *buffer, *who;
{
	/***** returns true iff 's' has the seven 'from' fields,
	       initializing the who to the sender *****/

	char junk[SLEN];

	junk[0] = '\0';
	sscanf(buffer, "%*s %s %*s %*s %*s %*s %s",
	            who, junk);
	return(junk[0] != '\0');
}

forwarded(buffer, who)
char *buffer, *who;
{
	/** change 'from' and date fields to reflect the ORIGINATOR of
	    the message by iteratively parsing the >From fields... **/

	char machine[SLEN], buff[SLEN], holding_from[SLEN];

	machine[0] = '\0';
	holding_from[0] = '\0';
	sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %*s %*s %s",
	            holding_from, machine);

	if(machine[0] == '\0')	/* try for address with timezone in date */
	sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %*s %s",
	            holding_from, machine);

	if (machine[0] == '\0') /* try for srm address */
	  sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %s",
	            holding_from, machine);

	if (machine[0] == '\0')
	  sprintf(buff, holding_from[0] ? holding_from : "anonymous");
	else
	  sprintf(buff,"%s!%s", machine, holding_from);

	strncpy(who, buff, SLEN);
}

remove_first_word(string)
char *string;
{	/** removes first word of string, ie up to first non-white space
	    following a white space! **/

	register int loc;

	for (loc = 0; string[loc] != ' ' && string[loc] != '\0'; loc++)
	    ;

	while (string[loc] == ' ' || string[loc] == '\t')
	  loc++;

	move_left(string, loc);
}

move_left(string, chars)
char string[];
int  chars;
{
	/** moves string chars characters to the left DESTRUCTIVELY **/

	register int i;

	chars--; /* index starting at zero! */

	for (i=chars; string[i] != '\0' && string[i] != '\n'; i++)
	  string[i-chars] = string[i];

	string[i-chars] = '\0';
}

show_header(count, from, subject)
int  count;
char *from, *subject;
{
	/** output header in clean format, including abbreviation
	    of return address if more than one machine name is
	    contained within it! **/

	char buffer[SLEN];
	int  loc, i=0, exc=0, len;

#ifndef INTERNET
	char *p;

	if (chloc(from,'!') != -1 && chloc(from,'@') > 0) {
	  for (p=from;*p != '@'; p++) ;
	  *p = '\0';
	}
#endif

	loc = strlen(from);

	while (exc < 2 && loc > 0)
	  if (from[--loc] == '!')
	    exc++;

	if (exc == 2) { /* lots of machine names!  Get last one */
	  loc++;
	  len = strlen(from);
	  while (loc < len && loc < SLEN)
	    buffer[i++] = from[loc++];
	  buffer[i] = '\0';
	  if (number)
	    printf("%3d: %-20s  %s\n", count, buffer, subject);
	  else
	    printf("%-20s  %s\n", buffer, subject);
	}
	else
	  if (number)
	    printf("%3d: %-20s  %s\n", count, from, subject);
	  else
	    printf("%-20s  %s\n", from, subject);
}

parse_arpa_from(buffer, newfrom)
char *buffer, *newfrom;
{
	/** try to parse the 'From:' line given... It can be in one of
	    two formats:
		From: Dave Taylor <hpcnou!dat>
	    or  From: hpcnou!dat (Dave Taylor)
	    Change 'newfrom' ONLY if sucessfully parsed this entry and
	    the resulting name is non-null!
	**/

	char temp_buffer[SLEN], *temp;
	register int i, j = 0, in_parens;

	temp = (char *) temp_buffer;
	temp[0] = '\0';

	no_ret(buffer);		/* blow away '\n' char! */

	if (lastch(buffer) == '>') {
	  for (i=strlen("From: "); buffer[i] != '\0' && buffer[i] != '<' &&
	       buffer[i] != '('; i++)
	    temp[j++] = buffer[i];
	  temp[j] = '\0';
	}
	else if (lastch(buffer) == ')') {
	  in_parens = 1;
	  for (i=strlen(buffer)-2; buffer[i] != '\0' && buffer[i] != '<'; i--) {
	    switch(buffer[i]) {
	    case ')':	in_parens++;
			break;
	    case '(':	in_parens--;
			break;
	    }
	    if(!in_parens) break;
	    temp[j++] = buffer[i];
	  }
	  temp[j] = '\0';
	  reverse(temp);
	}

/* this stuff copied from src/addr_util.c */
#ifdef USE_EMBEDDED_ADDRESSES

	/** if we have a null string at this point, we must just have a
	    From: line that contains an address only.  At this point we
	    can have one of a few possibilities...

		From: address
		From: <address>
		From: address ()
	**/

	if (strlen(temp) == 0) {
	  if (lastch(buffer) != '>') {
	    for (i=strlen("From:");buffer[i] != '\0' && buffer[i] != '('; i++)
	      temp[j++] = buffer[i];
	    temp[j] = '\0';
	  }
	  else {	/* get outta '<>' pair, please! */
	    for (i=strlen(buffer)-2;buffer[i] != '<' && buffer[i] != ':';i--)
	      temp[j++] = buffer[i];
	    temp[j] = '\0';
	    reverse(temp);
	  }
	}
#endif

	if (strlen(temp) > 0) {		/* mess with buffer... */

	  /* remove leading spaces... */

	  while (whitespace(temp[0]))
	    temp = (char *) (temp + 1);		/* increment address! */

	  /* remove trailing spaces... */

	  i = strlen(temp) - 1;

	  while (whitespace(temp[i]))
	   temp[i--] = '\0';

	  /* remove surrounding paired quotation marks */
	  if((temp[i] == '"') & (*temp == '"')) {
	    temp[i] = '\0';
	    temp++;
	  }

	  /* if anything is left, let's change 'from' value! */

	  if (strlen(temp) > 0)
	    strcpy(newfrom, temp);
	}
}

reverse(string)
char *string;
{
	/** reverse string... pretty trivial routine, actually! **/

	char buffer[SLEN];
	register int i, j = 0;

	for (i = strlen(string)-1; i >= 0; i--)
	  buffer[j++] = string[i];

	buffer[j] = '\0';

	strcpy(string, buffer);
}
