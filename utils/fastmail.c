
static char rcsid[] = "@(#)$Id: fastmail.c,v 4.1.1.3 90/12/06 10:38:55 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.3 $   $State: Exp $
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
 * $Log:	fastmail.c,v $
 * Revision 4.1.1.3  90/12/06  10:38:55  syd
 * Fix getlogin returning null causing a core dump
 * From: Nigel Metheringham <nigelm@ohm.york.ac.uk>
 *
 * Revision 4.1.1.2  90/10/07  20:56:25  syd
 * Add ifndef NO_XHEADER to X-Mailer
 * From: syd via request of Frank Elsner
 *
 * Revision 4.1.1.1  90/06/26  20:30:22  syd
 * Fix boundary check on argument count
 * From: Syd
 *
 * Revision 4.1  90/04/28  22:44:39  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This program is specifically written for group mailing lists and
    such batch type mail processing.  It does NOT use aliases at all,
    it does NOT read the /etc/password file to find the From: name
    of the user and does NOT expand any addresses.  It is meant
    purely as a front-end for either /bin/mail or /usr/lib/sendmail
    (according to what is available on the current system).

         **** This program should be used with CAUTION *****

**/

/** The calling sequence for this program is:

	fastmail {args}  filename full-email-address

   where args could be any (or all) of;

	   -b bcc-list		(Blind carbon copies to)
	   -c cc-list		(carbon copies to)
	   -d			(debug on)
	   -f from 		(from name)
	   -F from-addr		(the actual address to be put in the From: line)
	   -r reply-to-address 	(Reply-To:)
	   -s subject 		(subject of message)
**/

#include <stdio.h>
#include "defs.h"
#include "patchlevel.h"

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#ifdef BSD
#  include <sys/types.h>
#  include <sys/timeb.h>
#endif

static char ident[] = { WHAT_STRING };

char *arpa_dayname[] = { "Sun", "Mon", "Tue", "Wed", "Thu",
		  "Fri", "Sat", "" };

char *arpa_monname[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", ""};

char *get_arpa_date();

#if defined(BSD) && !defined(_POSIX_SOURCE)
  char *timezone();
#else
#ifndef OS2
  extern char *tzname[];
#endif
#endif

main(argc, argv)
int argc;
char *argv[];
{

	extern char *optarg;
	extern int optind;
	FILE *tempfile, *mailpipe;
	char hostname[NLEN], username[NLEN], from_string[SLEN], subject[SLEN];
	char filename[SLEN], tempfilename[SLEN], command_buffer[256];
	char replyto[SLEN], cc_list[SLEN], bcc_list[SLEN], to_list[SLEN];
	char from_addr[SLEN], buffer[SLEN], hostfromname[SLEN];
	char *tmplogname;
	int  c, sendmail_available, debug = 0;

        initpaths();

	from_string[0] = '\0';
	subject[0] = '\0';
	replyto[0] = '\0';
	cc_list[0] = '\0';
	bcc_list[0] = '\0';
	to_list[0] = '\0';
	from_addr[0] = '\0';

	while ((c = getopt(argc, argv, "b:c:df:F:r:s:")) != EOF) {
	  switch (c) {
	    case 'b' : strcpy(bcc_list, optarg);		break;
	    case 'c' : strcpy(cc_list, optarg);		break;
	    case 'd' : debug++;					break;
	    case 'f' : strcpy(from_string, optarg);	break;
	    case 'F' : strcpy(from_addr, optarg);		break;
	    case 'r' : strcpy(replyto, optarg);		break;
	    case 's' : strcpy(subject, optarg);		break;
	    case '?' :
	      usage();
 	  }
	}

	if (optind >= argc)
	  usage();

	strcpy(filename, argv[optind++]);

	if (optind >= argc)
          usage();

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

	tmplogname = getenv("LOGNAME");
	if (tmplogname != NULL)
	  strcpy(username, tmplogname);
	else
	  username[0] = '\0';

#ifndef OS2
	if (strlen(username) == 0)
	  cuserid(username);
#endif

	if (access(filename, READ_ACCESS) == -1) {
	  fprintf(stderr, "Error: can't find file %s!\n", filename);
	  exit(1);
	}

	sprintf(tempfilename, "%s%d.fm", tempdir, getpid());

	if ((tempfile = fopen(tempfilename, "w")) == NULL) {
	  fprintf(stderr, "Couldn't open temp file %s\n", tempfilename);
	  exit(1);
	}

	/** Subject must appear even if "null" and must be first
	    at top of headers for mail because the
	    pure System V.3 mailer, in its infinite wisdom, now
	    assumes that anything the user sends is part of the
	    message body unless either:
		1. the "-s" flag is used (although it doesn't seem
		   to be supported on all implementations??)
		2. the first line is "Subject:".  If so, then it'll
		   read until a blank line and assume all are meant
		   to be headers.
	    So the gory solution here is to move the Subject: line
	    up to the top.  I assume it won't break anyone elses program
	    or anything anyway (besides, RFC-822 specifies that the *order*
	    of headers is irrelevant).  Gahhhhh....
	**/
	fprintf(tempfile, "Subject: %s\n", subject);

	if (strlen(from_addr) == 0)
	  sprintf(from_addr, "%s@%s", username, hostfromname);

	if (strlen(from_string) > 0)
	  fprintf(tempfile, "From: %s (%s)\n", from_addr, from_string);
	else
	  fprintf(tempfile, "From: %s\n", from_addr);

	fprintf(tempfile, "Date: %s\n", get_arpa_date());

	if (strlen(replyto) > 0)
	  fprintf(tempfile, "Reply-To: %s\n", replyto);

	while (optind < argc)
          sprintf(to_list, "%s%s%s", to_list, (strlen(to_list) > 0? " ":""),
		  argv[optind++]);

	fprintf(tempfile, "To: %s\n", to_list);

	if (strlen(cc_list) > 0)
	  fprintf(tempfile, "Cc: %s\n", cc_list);

#ifndef NO_XHEADER
	fprintf(tempfile, "X-Mailer: fastmail [version %s PL%d]\n",
	  VERSION, PATCHLEVEL);
#endif /* !NO_XHEADER */
	fprintf(tempfile, "\n");

	fclose(tempfile);

	/** now we'll cat both files to /bin/rmail or sendmail... **/

	sendmail_available = (access(sendmail, EXECUTE_ACCESS) != -1);

	if (debug)
		printf("Mailing to %s%s%s%s%s [via %s]\n", to_list,
			(strlen(cc_list) > 0 ? " ":""), cc_list,
			(strlen(bcc_list) > 0 ? " ":""), bcc_list,
			sendmail_available? "sendmail" : mailer);

#ifdef OS2
	if (strcmp(sendmail, mailer) == 0)
	  sprintf(command_buffer, "%s -f %s %s %s %s", mailer, 
		  from_addr, to_list, cc_list, bcc_list);
	else
	  sprintf(command_buffer, "%s -t", mailer);
#else
	sprintf(command_buffer, "cat %s %s | %s %s %s %s",
		tempfilename, filename,
	        sendmail_available? sendmail : mailer,
		to_list, cc_list, bcc_list);
#endif

	if (debug)
	  printf("%s\n", command_buffer);

#ifdef OS2
        mailpipe = popen(command_buffer, "w");
	tempfile = fopen(tempfilename, "r");
        while ( fgets(buffer, sizeof(buffer), tempfile) != NULL )
          fputs(buffer, mailpipe);
        fclose(tempfile);
	tempfile = fopen(filename, "r");
        while ( fgets(buffer, sizeof(buffer), tempfile) != NULL )
          fputs(buffer, mailpipe);
        fclose(tempfile);
        pclose(mailpipe);
#else
	c = system(command_buffer);
#endif

	unlink(tempfilename);

	exit(c != 0);
}


usage()
{
  printf("\nUsage: fastmail {args} filename address(es)\n");
  printf( "\nwhere {args} can be:\n\n");
  printf("\t-b bcc-list       addresses to send blind-carbon copies to\n");
  printf("\t-c cc-list        addresses to send carbon copies to\n");
  printf("\t-d                debug\n");
  printf("\t-f from-name      sender's full name\n");
  printf("\t-F from-addr      sender's mail address\n");
  printf("\t-r reply-to       reply-to mail address\n");
  printf("\t-s subject        subject of the message\n");
  exit(1);
}


char *get_arpa_date()
{
	/** returns an ARPA standard date.  The format for the date
	    according to DARPA document RFC-822 is exemplified by;

	       	      Mon, 12 Aug 85 6:29:08 MST

	**/

	static char buffer[SLEN];	/* static character buffer       */
	struct tm *the_time;		/* Time structure, see CTIME(3C) */
	long	   junk;		/* time in seconds....		 */
#ifndef	_POSIX_SOURCE
	struct tm *localtime();
#endif
#ifdef BSD
#  ifndef TZ_MINUTESWEST
	struct timeb loc_time;	/* of course this is different! */
#    ifndef _POSIX_SOURCE
	long time();
#    endif
#  else
	struct  timeval  time_val;
	struct  timezone time_zone;
#  endif
#else
	time_t time();
#endif

#ifdef BSD
#  ifndef TZ_MINUTESWEST
	junk = (long) time((long *) 0);
	ftime(&loc_time);
#  else
	gettimeofday(&time_val, &time_zone);
	junk = time_val.tv_sec;
#  endif
#else
	junk = time(0);	/* this must be here for it to work! */
#endif
	the_time = localtime(&junk);

	sprintf(buffer, "%s, %d %s %d %d:%02d:%02d %s",
	  arpa_dayname[the_time->tm_wday],
	  the_time->tm_mday % 32,
	  arpa_monname[the_time->tm_mon],
	  the_time->tm_year % 100,
	  the_time->tm_hour % 24,
	  the_time->tm_min  % 61,
	  the_time->tm_sec  % 61,
#if defined(BSD) && !defined(_POSIX_SOURCE)
#  ifndef TZ_MINUTESWEST
	  timezone(loc_time.time_zone, the_time->tz_isdst));
#  else
#   ifdef GOULD_NP1
	  the_time->tm_zone);
#   else
	  timezone(time_zone.tz_minuteswest, time_zone.tz_dsttime));
#   endif
#  endif
#else
	  tzname[the_time->tm_isdst]);
#endif

	return( (char *) buffer);
}
