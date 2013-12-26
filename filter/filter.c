
static char rcsid[] ="@(#)$Id: filter.c,v 4.1.1.1 90/10/24 16:11:44 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.1 $   $State: Exp $
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
 * $Log:	filter.c,v $
 * Revision 4.1.1.1  90/10/24  16:11:44  syd
 * Fix out of order lines
 * From: Steve Campbell
 *
 * Revision 4.1  90/04/28  22:41:55  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/


/** This program is used as a filter within the users ``.forward'' file
    and allows intelligent preprocessing of mail at the point between
    when it shows up on the machine and when it is actually put in the
    mailbox.

    The program allows selection based on who the message is FROM, TO, or
    what the subject is.  Acceptable actions are for the program to DELETE_MSG
    the message, SAVE the message in a specified folder, FORWARD the message
    to a specified user, SAVE the message in a folder, but add a copy to the
    users mailbox anyway, or simply add the message to the incoming mail.

    Filter also keeps a log of what it does as it goes along, and at the
    end of each `quantum' mails a summary of actions, if any, to the user.

    Uses the files: $HOME/.filter for instructions to this program, and
    $HOME/.filterlog for a list of what has been done since last summary.

**/

#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include "defs.h"
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#include <fcntl.h>

#define  MAIN_ROUTINE			/* for the filter.h file, of course! */
#include "filter.h"

#undef fflush

main(argc, argv)
int argc;
char *argv[];
{
	extern char *optarg;
	FILE   *fd;				/* for output to temp file! */
	struct passwd  *passwd_entry;
#ifndef	_POSIX_SOURCE
	struct passwd  *getpwuid();		/* for /etc/passwd          */
#endif
	char filename[SLEN],			/* name of the temp file    */
	     buffer[MAX_LINE_LEN];		/* input buffer space       */
	int  in_header = TRUE,			/* for header parsing       */
	     in_to     = FALSE,			/* are we on 'n' line To: ? */
	     summary   = FALSE,			/* a summary is requested?  */
	     c;					/* var for getopt routine   */

        initpaths();

	outfname[0] = to[0] = '\0';	/* nothing read in yet, right? */

#ifdef HOSTCOMPILED
	strncpy(hostname, HOSTNAME, sizeof(hostname));
#else
	gethostname(hostname, sizeof(hostname));
#endif

#ifdef OS2
	getfromdomain(hostfromname, sizeof(hostfromname));
#else
	strcpy(hostfromname, hostname);
#endif

	/* now parse the starting arguments... */

	while ((c = getopt(argc, argv, "clno:rSsu:v")) != EOF) {
	  switch (c) {
	    case 'c' : clear_logs = TRUE;			break;
	    case 'l' : log_actions_only = TRUE;			break;
	    case 'o' : strcpy(outfname, optarg);		break;
	    case 'r' : printing_rules = TRUE;			break;

	    case 's' : summary = TRUE;				break;
	    case 'S' : long_summary = TRUE;			break;

	    case 'n' : show_only = TRUE;			break;
	    case 'u' : strcpy(username, optarg);		break;
	    case 'v' : verbose = TRUE;				break;
	    case '?' : usage(argv[0]);
	  }
	}

	/* first off, let's get the info from /etc/passwd */
	passwd_entry = username[0] ? getpwnam(username) : getpwuid(getuid());
	if (passwd_entry == NULL)
	  leave("Cannot get password entry for this uid!");

	strcpy(home, passwd_entry->pw_dir);
	strcpy(username, passwd_entry->pw_name);

	if (isatty(0) && !summary && !long_summary && !printing_rules)
          usage(argv[0]);

	/* let's open our outfd logfile as needed... */

	if (outfname[0] == '\0') 	/* default is stdout */
	  outfd = stdout;
	else
	  if ((outfd = fopen(outfname, "a")) == NULL) {
	    if (isatty(fileno(stderr)))
	      fprintf(stderr,"filter (%s): couldn't open log file %s\n",
		      username, outfname);
	  }

	if (summary || long_summary) {
          if (get_filter_rules() == -1) {
	    if (outfd != NULL) fclose(outfd);
	    exit(1);
	  }
	  show_summary();
	  if (outfd != NULL) fclose(outfd);
	  exit(0);
	}

	if (printing_rules) {
          if (get_filter_rules() == -1)
	    fprintf(outfd,"filter (%s): Couldn't get rules!\n", username);
          else
	    print_rules();
	  if (outfd != NULL) fclose(outfd);
          exit(0);
	}

	/* next, create the tempfile and save the incoming message */

	sprintf(filename, "%s%d.fil", tempdir, getpid());

	if ((fd = fopen(filename,"w")) == NULL)
	  leave("Cannot open temporary file!");

	while (fgets(buffer, MAX_LINE_LEN, stdin) != NULL) {

	  remove_return(buffer);

	  if (in_header) {

	    if (! whitespace(buffer[0]))
		in_to = FALSE;

	    if (the_same(buffer, "From "))
	      save_from(buffer);
	    else if (the_same(buffer, "Subject:"))
	      save_subject(buffer);
	    else if (the_same(buffer, "To:") || the_same(buffer, "Cc:")) {
	      in_to++;
	      save_to(buffer);
	    }
	    else if (the_same(buffer, "X-Filtered-By:"))
	      already_been_forwarded++;	/* could be a loop here! */
#ifdef USE_EMBEDDED_ADDRESSES
	    else if (the_same(buffer, "From:"))
	      save_embedded_address(buffer, "From:");
	    else if (the_same(buffer, "Reply-To:"))
	      save_embedded_address(buffer, "Reply-To:");
#endif
	    else if (strlen(buffer) < 2)
	      in_header = 0;
	    else if (whitespace(buffer[0]) && in_to)
	      strcat(to, buffer);
	  }

          fprintf(fd, "%s\n", buffer);	/* and save it regardless! */
	  fflush(fd);
	  lines++;
	}

	fclose(fd);

	/** next let's see if the user HAS a filter file, and if so what's in
            it (and so on) **/

	if (get_filter_rules() == -1)
	  mail_message(username);
	else {
	  switch (action_from_ruleset()) {

	    case DELETE_MSG : if (verbose && outfd != NULL)
			    fprintf(outfd, "filter (%s): Message deleted\n",
				    username);
			  log(DELETE_MSG);				break;

	    case SAVE   : if (save_message(rules[rule_choosen].argument2)) {
			    mail_message(username);
			    log(FAILED_SAVE);
			  }
			  else
		 	    log(SAVE);					break;

	    case SAVECC : if (save_message(rules[rule_choosen].argument2))
			    log(FAILED_SAVE);
			  else
		            log(SAVECC);
			  mail_message(username);			break;

	    case FORWARD: mail_message(rules[rule_choosen].argument2);
			  log(FORWARD);					break;

	    case EXEC   : execute(rules[rule_choosen].argument2);
			  log(EXEC);					break;

	    case LEAVE  : mail_message(username);
			  log(LEAVE);					break;
	  }
	}

	(void) unlink(filename);	/* remove the temp file, please! */
	if (outfd != NULL) fclose(outfd);
	exit(0);
}


usage(name)
char *name;
{
  printf("\nUsage: filter [-nrv] [-o output] [-u user]"
         "\n   or: filter [-s|-S] [-c] [-o output] [-u user]\n"
         "\nWhere: -n   not really, only output what would happen"
         "\n       -v   be verbose for each message filtered"
         "\n       -r   list rules currently beeing used"
         "\n       -s   list summary of message filtered log"
         "\n       -S   list more verbose summary that -s"
         "\n       -c   clear log files after summarizing with -s or -S"
         "\n       -o output    redirect log message to 'output'"
	 "\n       -u user      work under user ID 'user'\n");
  exit(1);
}

save_from(buffer)
char *buffer;
{
	/** save the SECOND word of this string as FROM **/

	register char *f = from;

	while (*buffer != ' ')
	  buffer++;				/* get to word     */

	for (buffer++; *buffer != ' ' && *buffer; buffer++, f++)
	  *f = *buffer;				/* copy it and     */

	*f = '\0';				/* Null terminate! */
}

save_subject(buffer)
char *buffer;
{
	/** save all but the word "Subject:" for the subject **/

	register int skip = 8;  /* skip "Subject:" initially */

	while (buffer[skip] == ' ') skip++;

	strcpy(subject, (char *) buffer + skip);
}

save_to(buffer)
char *buffer;
{
	/** save all but the word "To:" or "Cc:" for the to list **/

	register int skip = 3;	/* skip "To:" or "Cc:" initially */

	while (buffer[skip] == ' ') skip++;

	strcat(to, (char *) buffer + skip);
}

#ifdef USE_EMBEDDED_ADDRESSES

save_embedded_address(buffer, fieldname)
char *buffer, *fieldname;
{
	/** this will replace the 'from' address with the one given,
	    unless the address is from a 'reply-to' field (which overrides
	    the From: field).  The buffer given to this routine can have one
            of three forms:
		fieldname: username <address>
		fieldname: address (username)
		fieldname: address
	**/

	static int processed_a_reply_to = 0;
	char address[LONG_STRING];
	register int i, j = 0;

	/** first let's extract the address from this line.. **/

	if (buffer[strlen(buffer)-1] == '>') {	/* case #1 */
	  for (i=strlen(buffer)-1; buffer[i] != '<' && i > 0; i--)
		/* nothing - just move backwards .. */ ;
	  i++;	/* skip the leading '<' symbol */
	  while (buffer[i] != '>')
	    address[j++] = buffer[i++];
	  address[j] = '\0';
	}
	else {	/* get past "from:" and copy until white space or paren hit */
	  for (i=strlen(fieldname); whitespace(buffer[i]); i++)
	     /* skip past that... */ ;
	  while (buffer[i] != '(' && ! whitespace(buffer[i]) && buffer[i]!='\0')
	    address[j++] = buffer[i++];
	  address[j] = '\0';
	}

	/** now let's see if we should overwrite the existing from address
	    with this one or not.. **/

	if (processed_a_reply_to)
	  return;	/* forget it! */

	strcpy(from, address);			/* replaced!! */

	if (strcmp(fieldname, "Reply-To:") == 0)
	  processed_a_reply_to++;
}
#endif
