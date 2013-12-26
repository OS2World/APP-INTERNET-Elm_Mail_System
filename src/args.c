
static char rcsid[] = "@(#)$Id: args.c,v 4.1 90/04/28 22:42:31 syd Exp $";

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
 * $Log:	args.c,v $
 * Revision 4.1  90/04/28  22:42:31  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** starting argument parsing routines for ELM system...

**/

#include "headers.h"
#include "patchlevel.h"

extern char *optarg;		/* optional argument as we go */
extern int   optind;			/* argnum + 1 when we leave   */

void exit();	/* just keeping lint happy.... */

char *
parse_arguments(argc, argv, to_whom)
int argc;
char *argv[], *to_whom;
{
	/** Set flags according to what was given to program.  If we are
	    fed a name or series of names, put them into the 'to_whom' buffer
	    and if the check_only flag wasn't presented, set mail_only to ON,
	    and if stdin is not a tty, set batch_only  to ON;
	    Return req_mfile, which points to a named mail file or is empty.
	    **/

	register int c = 0;
	char *strcpy();
	static char req_mfile[SLEN];

	to_whom[0] = '\0';
	batch_subject[0] = '\0';

        while ((c = getopt(argc, argv, "?acd:f:hkKmns:uVvwz")) != EOF) {
	   switch (c) {
	     case 'a' : arrow_cursor++;		break;
	     case 'c' : check_only++;		break;
	     case 'd' : debug = atoi(optarg);	break;
	     case 'f' : strcpy(req_mfile, optarg);	break;
	     case '?' :
	     case 'h' : args_help();
	     case 'k' : hp_terminal++;	break;
	     case 'K' : hp_terminal++; hp_softkeys++;	break;
	     case 'm' : mini_menu = 0;	break;
	     case 'n' : no_save++;	break;
	     case 's' : strcpy(batch_subject, optarg);	break;
	     case 'u' : mmdf_strict++;	break; /* i.e. UUPC */
             case 'V' : sendmail_verbose++;     break;
	     case 'v' : args_version();
	     case 'w' : warnings = 0;	break;
	     case 'z' : check_size++;   break;
	    }
	 }


#ifndef DEBUG
	if (debug)
	  printf(
     "Warning: system created without debugging enabled - request ignored\n");
	debug = 0;
#endif

	if (optind < argc) {
	  while (optind < argc) {
		if (strlen(to_whom) + strlen(to_whom[0] != '\0'? " " : "") +
			strlen(argv[optind]) > SLEN)
				exit(printf("\n\rToo many addresses, or addresses too long!\n\r"));

	    sprintf(to_whom, "%s%s%s", to_whom,
	            to_whom[0] != '\0'? " " : "", argv[optind]);
	    if(!check_only)
	      mail_only++;
	    optind++;
	  }
	  check_size = 0;	/* NEVER do this if we're mailing!! */
	}

	 if (strlen(batch_subject) > 0 && ! mail_only)
	   exit(printf(
     "\n\rDon't understand specifying a subject and no-one to send to!\n\r"));

	if (!isatty(fileno(stdin)) && !check_only) {
	  batch_only = ON;
	  if(*batch_subject == '\0')
	    strcpy(batch_subject, DEFAULT_BATCH_SUBJECT);
	}
	return(req_mfile);


}

args_help()
{
	/**  print out possible starting arguments... **/

	printf("\nPossible Starting Arguments for ELM program:\n\n");
	printf("\targ\t\t\tMeaning\n");
	printf("\t -a \t\tArrow - use the arrow pointer regardless\n");
	printf("\t -c \t\tCheckalias - check the given aliases only\n");
	printf("\t -dn\t\tDebug - set debug level to 'n'\n");
	printf(
	  "\t -fx\t\tFolder - read folder 'x' rather than incoming mailbox\n");
	printf("\t -h \t\tHelp - give this list of options\n");
	printf("\t -k \t\tKeypad - enable HP 2622 terminal keyboard\n");
	printf("\t -K \t\tKeypad&softkeys - enable use of softkeys + \"-k\"\n");
	printf("\t -m \t\tMenu - Turn off menu, using more of the screen\n");
	printf("\t -n \t\tNosave - Do not save outgoing mail - for batchmailing\n");
	printf("\t -sx\t\tSubject 'x' - for batchmailing\n");
	printf("\t -u \t\tMore restrictive mailbox scanning - for UUPC mailboxes\n");
        printf("\t -V \t\tEnable sendmail voyeur mode.\n");
	printf("\t -v \t\tPrint out ELM version information.\n");
	printf("\t -w \t\tSupress warning messages...\n");
	printf("\t -z \t\tZero - don't enter ELM if no mail is pending\n");
	printf("\n");
	printf("\n");
	exit(1);
}

args_version()
{
	/** print out version information **/

	printf("\nElm Version and Identification Information:\n\n");
	printf("\tElm %s PL%d, of %s\n",VERSION,PATCHLEVEL,VERS_DATE);
	printf("\t(C) Copyright 1986, 1987 Dave Taylor\n");
	printf("\t(C) Copyright 1988, 1989, 1990 USENET Community Trust\n");
	printf("\t----------------------------------\n");
	printf("\tConfigured %s\n", CONFIGURE_DATE);
	printf("\t----------------------------------\n");

#ifdef USE_EMBEDDED_ADDRESSES
	printf("\tFrom: and Reply-To: addresses are good: USE_EMBEDDED_ADDRESSES\n");
#else /* USE_EMBEDDED_ADDRESSES */
	printf("\tFrom: and Reply-To: addresses ignored: not USE_EMBEDDED_ADDRESSES\n");
#endif /* USE_EMBEDDED_ADDRESSES */

#ifdef OPTIMIZE_RETURN
	printf("\tReturn addresses will be optimized: OPTIMIZE_RETURN\n");
#else /* OPTIMIZE_RETURN */
	printf("\tReturn addresses will not be optimized: not OPTIMIZE_RETURN\n");
#endif

#ifdef INTERNET
	printf("\tPrefers Internet address formats: INTERNET\n");
#else /* INTERNET */
	printf("\tInternet address formats not used: not INTERNET\n");
#endif /* INTERNET */

#ifdef DEBUG
	printf("\tDebug options are available: DEBUG\n");
#else /* DEBUG */
	printf("\tNo debug options are available: not DEBUG\n");
#endif /* DEBUG */

#ifdef CRYPT
	printf("\tCrypt function enabled: CRYPT\n");
#else /* CRYPT */
	printf("\tCrypt function disabled: not CRYPT\n");
#endif /* CRYPT */

#ifdef ALLOW_MAILBOX_EDITING
	printf("\tMailbox editing included: ALLOW_MAILBOX_EDITING\n");
#else /* ALLOW_MAILBOX_EDITING */
	printf("\tMailbox editing not included: not ALLOW_MAILBOX_EDITING\n");
#endif /* ALLOW_MAILBOX_EDITING */

#ifdef ENABLE_CALENDAR
	printf("\tCalendar file feature enabled: ENABLE_CALENDAR\n");
	printf("\t\t(Default calendar file is %s)\n",dflt_calendar_file);
#else /* ENABLE_CALENDAR */
	printf("\tCalendar file feature disabled: not ENABLE_CALENDAR\n");
#endif /* ENABLE_CALENDAR */

	printf("\n\n");
	exit(1);

}

