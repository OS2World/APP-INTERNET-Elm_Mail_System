
static char rcsid[] = "@(#)$Id: init.c,v 4.1.1.4 90/12/05 14:34:08 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.4 $   $State: Exp $
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
 * $Log:	init.c,v $
 * Revision 4.1.1.4  90/12/05  14:34:08  syd
 * fix dropping of unused vars, dropped to much
 * From: Syd
 *
 * Revision 4.1.1.3  90/10/24  15:33:11  syd
 * Remove variables no longer used
 * From: W. David Higgins
 *
 * Revision 4.1.1.2  90/08/02  21:57:56  syd
 * The newly introduced function 'stricmp' has a name conflict with a libc
 * function under SunOS 4.1.  Changed name to istrcmp.
 * From: scs@lokkur.dexter.mi.us (Steve Simmons)
 *
 * Revision 4.1.1.1  90/07/12  23:19:17  syd
 * Make domain name checking case independent
 * From: Syd, reported by Steven Baur
 *
 * Revision 4.1  90/04/28  22:43:15  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/***** Initialize - read in all the defaults etc etc
*****/

#include "headers.h"
#include "patchlevel.h"

#ifdef TERMIOS
#  include <termios.h>
   typedef struct termios term_buff;
#else
# ifdef TERMIO
#  include <termio.h>
#  define tcgetattr(fd,buf)	ioctl((fd),TCGETA,(buf))
   typedef struct termio term_buff;
# else
#  include <sgtty.h>
#  define tcgetattr(fd,buf)	ioctl((fd),TIOCGETP,(buf))
   typedef struct sgttyb term_buff;
# endif
#endif

#ifdef PWDINSYS
#  include <sys/pwd.h>
#else
#  include <pwd.h>
#endif

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif

#include <signal.h>
#include <ctype.h>
#include <errno.h>

#ifdef BSD
#undef toupper
#undef tolower
#endif

#ifndef OS2
extern int errno;		/* system error number on failure */
#endif
extern char version_buff[];

char *error_name(), *error_description();

char *getenv(), *getlogin(), *strcpy(), *strcat(), *index();
unsigned short getgid(), getuid();
void exit();
#ifndef	_POSIX_SOURCE
struct passwd *getpwuid();
#endif
char *get_full_name();

#ifdef VOIDSIG
  void
#else
  int
#endif
#ifdef SIGTSTP
	sig_user_stop(), sig_return_from_user_stop(),
#endif
	quit_signal(), term_signal(), ill_signal(),
	fpe_signal(),  bus_signal(),  segv_signal(),
	alarm_signal(), pipe_signal(), hup_signal();

initialize(requestedmfile)
char *requestedmfile;	/* first mail file to open, empty if the default */
{
	/** initialize the whole ball of wax.
	**/
	struct passwd *pass;
	register int i, hostlen, domlen;

#if defined(SIGVEC) & defined(SV_INTERRUPT)
	struct sigvec alarm_vec;
#endif
	char     buffer[SLEN], *cp;

	sprintf(version_buff, "%s PL%d", VERSION, PATCHLEVEL);
	Raw(ON);

	/* save original user and group ids */
	userid  = getuid();
	groupid = getgid();

	/* make all newly created files private */
	original_umask = umask(077);

	/* Get username (logname), home (login directory), and full_username
	 * (part of GCOS) field from the password entry for this user id.
	 * Full_username will get overridden by fullname in elmrc, if defined.
	 */

	if((pass = getpwuid(userid)) == NULL) {
	  error("You have no password entry!");
	  Raw(OFF);
	  exit(1);
	}
	strcpy(username, pass->pw_name);
	strcpy(home, pass->pw_dir);

	if((cp = get_full_name(username)) != NULL)
	  strcpy(full_username, cp);
	else
	  strcpy(full_username, username);	/* fall back on logname */

#ifdef DEBUG
	if (debug) {		/* setup for dprint() statements! */
	  char newfname[SLEN], filename[SLEN];

	  sprintf(filename, "%s/%s", home, DEBUGFILE);
	  if (access(filename, ACCESS_EXISTS) == 0) {	/* already one! */
	    sprintf(newfname,"%s/%s", home, OLDEBUG);
	    (void) rename(filename, newfname);
	  }

	  /* Note what we just did up there: we always save the old
	     version of the debug file as OLDEBUG, so users can mail
	     copies of bug files without trashing 'em by starting up
	     the mailer.  Dumb, subtle, but easy enough to do!
 	  */

	  if ((debugfile = fopen(filename, "w")) == NULL) {
	    debug = 0;	/* otherwise 'leave' will try to log! */
	    leave(fprintf(stderr,"Could not open file %s for debug output!\n",
		  filename));
	  }
	  chown(filename, userid, groupid); /* file owned by user */

	  fprintf(debugfile,
     "Debug output of the ELM program (at debug level %d).  Version %s\n\n",
		  debug, version_buff);
	}
#endif

	/*
	 * If debug level is fairly low, ignore keyboard signals
	 * until the screen is set up.
	 */
	if (debug < 5) {
	  signal(SIGINT,  SIG_IGN);
	  signal(SIGQUIT, SIG_IGN);
	}

	if(!check_only && !batch_only) {
	  if ((i = InitScreen()) < 0) {
	    if (i == -1) {
	      printf(
"Sorry, but you must specify what type of terminal you're on if you want to\r\n");
	      printf(
"run the \"elm\" program. (You need your environment variable \"TERM\" set.)\r\n"
		     );
	      dprint(1,(debugfile,"No $TERM variable in environment!\n"));
	    }
	    else if (i == -2) {
	      printf(
"You need a cursor-addressable terminal to run \"elm\" and I can't find any\r\n");
	      printf(
"kind of termcap entry for \"%s\" - check your \"TERM\" setting...\r\n",
		   getenv("TERM"));
	      printf(
"Or check your TERMCAP setting or termcap database file.\r\n");
	      dprint(1,
		(debugfile,"$TERM variable is an unknown terminal type!\n"));
	    } else {
	      printf("Failed trying to initialize your terminal entry: unknown return code %d\r\n", i);
	      dprint(1, (debugfile, "Initscreen returned unknown code: %d\r\n",
		  i));
	    }
	    Raw(OFF);
	    exit(1);	/* all the errors share this exit statement */
	  }
          EndBold();
          EndHalfbright();
          EndInverse();
	}

	if (debug < 5) {	/* otherwise let the system trap 'em! */
	  signal(SIGQUIT, quit_signal);		/* Quit signal 	            */
	  signal(SIGTERM, term_signal); 	/* Terminate signal         */
	  signal(SIGILL,  ill_signal);		/* Illegal instruction      */
	  signal(SIGFPE,  fpe_signal);		/* Floating point exception */
#ifndef OS2
	  signal(SIGBUS,  bus_signal);		/* Bus error  		    */
	  signal(SIGHUP,  hup_signal);		/* HangUp (line dropped)    */
#endif
	  signal(SIGSEGV, segv_signal);		/* Segmentation Violation   */
	}
	else {
	  dprint(3,(debugfile,
  "\n*** Elm-Internal Signal Handlers Disabled due to debug level %d ***\n\n",
		    debug));
	}
#if defined(SIGVEC) & defined(SV_INTERRUPT)
	alarm_vec.sv_handler = alarm_signal;
	alarm_vec.sv_flags = SV_INTERRUPT;
	sigvec (SIGALRM, &alarm_vec, (struct sigvec *)0);	/* Process Timer Alarm	    */
#else
	signal(SIGALRM, alarm_signal);		/* Process Timer Alarm      */
#endif
#ifndef OS2
	signal(SIGPIPE, pipe_signal);		/* Illegal Pipe Operation   */
#endif
#ifdef SIGTSTP
	signal(SIGTSTP, sig_user_stop);		/* Suspend signal from tty  */
	signal(SIGCONT, sig_return_from_user_stop);	/* Continue Process */
#endif

	get_term_chars();

	/*
	 * Get the host name as per configured behavior.
	 */
#ifdef HOSTCOMPILED
	strncpy(hostname, HOSTNAME, sizeof(hostname) - 1);
	hostname[sizeof(hostname) - 1] = '\0';
#else
	gethostname(hostname, sizeof(hostname));
#endif

	/*
	 * now get the domain name, used to build the full name
	 */
	gethostdomain(hostdomain, sizeof(hostdomain));

	/*
	 * now the tough part:
	 *	we need to make three variables out of this stuff:
	 *	hostname = just the hostname, as in bangpaths,
	 *		this is whatever the user gave us so far,
	 *		we wont change this one
	 *	hostdomain = this is the domain considered local to this
	 *		machine, and should be what we got above.
	 *	hostfullname = this is the full FQDN of this machine,
	 *		and is a strange combination of the first two.
	 *	if tail(hostname) == hostdomain
	 *		then hostfullname = hostname
	 *			ie: node.ld.domain.type, ld.domain.type -> node.ld.domain.type
	 *	else if hostname == hostdomain + 1
	 *		then hostfullname = hostname
	 *			ie: domain.type, .domain.type -> domain.type
	 *
	 *	else hostfullname = hostname + hostdomain
	 *			ie: host, .domain.type -> host.domain.type
	 * lost yet?
	 */
	hostlen = strlen(hostname);
	domlen = strlen(hostdomain);
	if (hostlen >= domlen) {
	  if (istrcmp(&hostname[hostlen - domlen], hostdomain) == 0)
	    strcpy(hostfullname, hostname);
	  else {
	    strcpy(hostfullname, hostname);
	    strcat(hostfullname, hostdomain);
	  }
	} else {
	  if (istrcmp(hostname, hostdomain + 1) == 0)
	    strcpy(hostfullname, hostname);
	  else {
	    strcpy(hostfullname, hostname);
	    strcat(hostfullname, hostdomain);
	  }
	}

#ifdef OS2
	getfromdomain(hostfromname, sizeof(hostfromname));
#else
	strcpy(hostfromname, hostfullname);
#endif

	/* Determine the default mail file name.
	 *
	 * First look for an environment variable MAIL, then
	 * use then mailhome if it is not found
	 */
	if ((cp = getenv("MAIL")) == NULL)
#ifdef OS2
	    if (maildir)
		sprintf(defaultfile, "%s%s/newmail%s", mailhome, username, mailext);
	    else
#endif
		sprintf(defaultfile, "%s%s%s", mailhome, username, mailext);
	else
		strcpy(defaultfile, cp);

	/* Determine options that might be set in the .elm/elmrc */
	read_rc_file();

	/* Determine the mail file to read */
	if (*requestedmfile == '\0')
	  strcpy(requestedmfile, defaultfile);
	else if(!expand_filename(requestedmfile, FALSE)) {
	    Raw(OFF);
	    exit(0);
        }
	if (check_size)
	  if(check_mailfile_size(requestedmfile) != 0) {
	      Raw(OFF);
	      exit(0);
	  }

	/* check for permissions only if not send only mode file */
	if (! mail_only) {
	  if ((errno = can_access(requestedmfile, READ_ACCESS)) != 0) {
	    if (strcmp(requestedmfile, defaultfile) != 0 || errno != ENOENT) {
	      dprint(1, (debugfile,
		    "Error: given file %s as folder - unreadable (%s)!\n",
		    requestedmfile, error_name(errno)));
	      fprintf(stderr,"Can't open folder '%s' for reading!\n",
		    requestedmfile);
	      Raw(OFF);
	      exit(1);
	    }
	  }
	}

	/** check to see if the user has defined a LINES or COLUMNS
	    value different to that in the termcap entry (for
	    windowing systems, of course!) **/

	ScreenSize(&LINES, &COLUMNS);

	if ((cp = getenv("LINES")) != NULL && isdigit(*cp)) {
	  sscanf(cp, "%d", &LINES);
	  LINES -= 1;	/* kludge for HP Window system? ... */
	}

	if ((cp = getenv("COLUMNS")) != NULL && isdigit(*cp))
	  sscanf(cp, "%d", &COLUMNS);

	/** fix the shell if needed **/

#ifndef OS2
	if (shell[0] != '/') {
	   sprintf(buffer, "/bin/%s", shell);
	   strcpy(shell, buffer);
	}
#endif

	if (! mail_only && ! check_only) {

	  /* get the cursor control keys... */

	  cursor_control = FALSE;

	  if ((cp = return_value_of("ku")) != NULL) {
	    strcpy(up, cp);
	    if ((cp = return_value_of("kd")) != NULL) {
	      strcpy(down, cp);
	      if ((cp = return_value_of("kl")) != NULL) {
		strcpy(left, cp);
		if ((cp = return_value_of("kr")) != NULL) {
		  strcpy(right, cp);
		  cursor_control = TRUE;
		  transmit_functions(ON);
		}
	      }
	    }
	  }
	  if (!arrow_cursor) 	/* try to use inverse bar instead */
	    if (return_value_of("so") != NULL && return_value_of("se") != NULL)
	        has_highlighting = TRUE;
	}

	/** clear the screen **/
	if(!check_only && !batch_only)
	  ClearScreen();

	if (! mail_only && ! check_only) {
	  if (mini_menu)
	    headers_per_page = LINES - 13;
	  else
	    headers_per_page = LINES -  8;	/* 5 more headers! */

	  newmbox(requestedmfile, FALSE);	/* read in the folder! */
	}

#ifdef DEBUG
	if (debug >= 2 && debug < 10) {
	  fprintf(debugfile,
"hostname = %-20s \tusername = %-20s \tfullname = %-20s\n",
	         hostname, username, full_username);

	  fprintf(debugfile,
"home     = %-20s \teditor   = %-20s \trecvd_mail  = %-20s\n",
		 home, editor, recvd_mail);

	  fprintf(debugfile,
"cur_folder   = %-20s \tfolders  = %-20s \tprintout = %-20s\n",
		 cur_folder, folders, printout);

	  fprintf(debugfile,
"sent_mail = %-20s \tprefix   = %-20s \tshell    = %-20s\n\n",
		sent_mail, prefixchars, shell);

	  if (local_signature[0])
	    fprintf(debugfile, "local_signature = \"%s\"\n",
			local_signature);
	  if (remote_signature[0])
	    fprintf(debugfile, "remote_signature = \"%s\"\n",
			remote_signature);
	  if (local_signature[0] || remote_signature[0])
	    fprintf(debugfile, "\n");
	}
#endif
}

get_term_chars()
{
	/** This routine sucks out the special terminal characters
	    ERASE and KILL for use in the input routine.  The meaning
            of the characters are (dare I say it?) fairly obvious... **/

	term_buff term_buffer;

	if (tcgetattr(STANDARD_INPUT,&term_buffer) == -1) {
	  dprint(1, (debugfile,
		   "Error: %s encountered on ioctl call (get_term_chars)\n",
		   error_name(errno)));
	  /* set to defaults for terminal driver */
	  backspace = BACKSPACE;
	  kill_line = ctrl('U');
	}
	else {
#if defined(TERMIO) || defined(TERMIOS)
	  backspace = term_buffer.c_cc[VERASE];
	  kill_line = term_buffer.c_cc[VKILL];
#else
	  backspace = term_buffer.sg_erase;
	  kill_line = term_buffer.sg_kill;
#endif
	}
}
