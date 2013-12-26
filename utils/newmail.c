
static char rcsid[] = "@(#)$Id: newmail.c,v 4.1.1.3 90/12/05 15:05:39 syd Exp $";

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
 * $Log:	newmail.c,v $
 * Revision 4.1.1.3  90/12/05  15:05:39  syd
 * Remove unused opterr variable, some getopts dont define it
 * From: Syd via Peter King
 *
 * Revision 4.1.1.2  90/10/07  21:10:35  syd
 * newmail did not correctly present sender name if the source
 * of the mail is local from the system.
 * From: JT McDuffie <guardian!jt@Sun.COM>
 *
 * Revision 4.1.1.1  90/10/07  19:43:44  syd
 * Fixes when newmail detects that the mail folder has grown in size it prints a newline, even
 * if there were no new subjects in the folder.
 * From: Uwe Doering <gemini%geminix.mbx.sub.org@RELAY.CS.NET>
 *
 * Revision 4.1  90/04/28  22:44:48  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This is actually two programs folded into one - 'newmail()' and
    'wnewmail()'.  They perform essentially the same function, to
    monitor the mail arriving in a set of/a mailbox or folder, but
    newmail is designed to run in background on a terminal, and
    wnewmail is designed to have a window of its own to run in.

    The main difference is that wnewmail checks for mail more often.

    The usage parameters are:

	-i <interval>  		how often to check for mail
				(default: 60 secs if newmail,
					  10 secs if wnewmail)

	<filename>		name of a folder to monitor
				(can prefix with '+'/'=', or can
			 	default to the incoming mailbox)

	<filename>=prefix	file to monitor, output with specified
				prefix when mail arrives.

    If we're monitoring more than one mailbox the program will prefix
    each line output (if 'newmail') or each cluster of mail (if 'wnewmail')
    with the basename of the folder the mail has arrived in.  In the
    interest of exhaustive functionality, you can also use the "=prefix"
    suffix (eh?) to specify your own strings to prefix messages with.

    The output format is either:

	  newmail:
	     >> New mail from <user> - <subject>
	     >> Priority mail from <user> - <subject>

	     >> <folder>: from <user> - <subject>
	     >> <folder>: Priority from <user> - <subject>

	  wnewmail:
	     <user> - <subject>
	     Priority: <user> - <subject>

	     <folder>: <user> - <subject>
	     <folder>: Priority: <user> - <subject>\fR

**/

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#include "defs.h"

#include <signal.h>	/* background jobs ignore some signals... */

static char ident[] = { WHAT_STRING };

#define LINEFEED		(char) 10
#define BEGINNING		0			/* seek fseek(3S) */
#define DEFAULT_INTERVAL	60

#define MAX_FOLDERS		25		/* max we can keep track of */

#define NO_SUBJECT	"(No Subject Specified)"

#define metachar(c)	(c == '+' || c == '=' || c == '%')

char  *getusername();
long  bytes();

struct folder_struct {
	  char		foldername[SLEN];
	  char		prefix[NLEN];
	  FILE 		*fd;
	  long		filesize;
       } folders[MAX_FOLDERS];

int  interval_time,		/* how long to sleep between checks */
     debug = 0,			/* include verbose debug output?    */
     in_window = 0,		/* are we running as 'wnewmail'?    */
     total_folders = 0,		/* # of folders we're monitoring    */
     current_folder = 0;	/* struct pointer for looping       */

#ifdef PIDCHECK
int  parent_pid;		/* See if sucide should be attempt  */
#endif /* PIDCHECK */

char hostname[SLEN],            /* name of machine we're on         */
     hostdomain[SLEN];          /* name of domain we're in          */

#ifdef BSD
extern int errno;
#endif

main(argc, argv)
int argc;
char *argv[];
{
	extern char *optarg;
	extern int   optind;
	char *ptr;
	int c, i, done;
	long lastsize,
	     newsize;			/* file size for comparison..      */

        initpaths();

#ifdef HOSTCOMPILED
	strncpy(hostname, HOSTNAME, sizeof(hostname));
#else
	gethostname(hostname, sizeof(hostname));
#endif
	gethostdomain(hostdomain, sizeof(hostdomain));

#ifdef PIDCHECK				/* This will get the pid that         */
	parent_pid = getppid();		/* started the program, ie: /bin/sh   */
					/* If it dies for some reason (logout)*/
#endif /* PIDCHECK */			/* Then exit the program if PIDCHECK  */

	interval_time = DEFAULT_INTERVAL;

	/** let's see if the first character of the basename of the
	    command invoked is a 'w' (e.g. have we been called as
	    'wnewmail' rather than just 'newmail'?)
	**/

	for (i=0, ptr=(argv[0] + strlen(argv[0])-1); !i && ptr > argv[0]; ptr--)
	  if (*ptr == '/') {
	    in_window = (*(ptr+1) == 'w');
	    i++;
	  }

	if (ptr == argv[0] && i == 0 && argv[0][0] == 'w')
	  in_window = 1;

	while ((c = getopt(argc, argv, "di:wh?")) != EOF) {
	  switch (c) {
	    case 'd' : debug++;					break;
	    case 'i' : interval_time = atoi(optarg);		break;
	    case 'w' : in_window = 1;				break;
	    default  : usage(argv[0]);				exit(1);
	 }
	}

	if (interval_time < 10)
	  fprintf(stderr,
"Warning: interval set to %d second%s.  I hope you know what you're doing!\n",
	  interval_time, interval_time == 1 ? "" : "s");

	/* now let's parse the foldernames, if any are given */

	if (optind >= argc) /* get default */
	  add_default_folder();
	else {
	  while (optind < argc)
	    add_folder(argv[optind++]);
	  pad_prefixes();			/* for nice output...*/
	}

#ifdef AUTO_BACKGROUND
	if (! in_window) {
	  if (fork())	    /* automatically puts this task in background! */
	    exit(0);

	  (void) signal(SIGINT, SIG_IGN);
	  (void) signal(SIGQUIT, SIG_IGN);
	}
#endif
#ifdef SIGHUP
	(void) signal(SIGHUP, SIG_DFL);
#endif

	if (in_window && ! debug)
	  printf("Incoming mail:\n");

	while (1) {

#ifndef OS2
#ifdef PIDCHECK
	if ( kill(parent_pid,0))
		exit(0);
#else
#ifndef AUTO_BACKGROUND		/* won't work if we're nested this deep! */
	  if (getppid() == 1) 	/* we've lost our shell! */
	    exit();
#endif /* AUTO_BACKGROUND */
#endif /* PIDCHECK */
#endif

	  if (! isatty(1))	/* we're not sending output to a tty any more */
	     exit();

	  if (debug) printf("\n----\n");

	  /* move_incoming_mail(); */

	  for (i = 0; i < total_folders; i++) {

	    if (debug)
	      printf("[checking folder #%d: %s]\n", i, folders[i].foldername);

	    if (folders[i].fd == (FILE *) NULL) {

	      if ((folders[i].fd = fopen(folders[i].foldername,"r")) == NULL)
	        if (errno == EACCES) {
		   fprintf(stderr, "\nPermission to monitor %s denied!\n\n",
			 folders[i].foldername);
	           sleep(5);
	           exit(1);
	        }
	    }

	    if ((newsize = bytes(folders[i].foldername)) >
	        folders[i].filesize) {	/* new mail has arrived! */

	      if (debug)
	        printf(
		   "\tnew mail has arrived!  old size = %ld, new size=%ld\n",
		   folders[i].filesize, newsize);

	      /* skip what we've read already... */

	      if (fseek(folders[i].fd, folders[i].filesize,
			BEGINNING) != 0)
	        perror("fseek()");

	      folders[i].filesize = newsize;

	      /* read and display new mail! */
	      if (read_headers(i) && ! in_window)
	        printf("\n\r");
	    }
	    else if (newsize != folders[i].filesize) {	/* file SHRUNK! */

	      folders[i].filesize = bytes(folders[i].foldername);
	      (void) fclose(folders[i].fd);	/* close it and ...         */
	      folders[i].fd = (FILE *) NULL;	/* let's reopen the file    */

	      lastsize = folders[i].filesize;
	      done     = 0;

	      while (! done) {
	        sleep(0);	/* basically gives up our CPU slice */
	        newsize = bytes(folders[i].foldername);
	        if (newsize != lastsize)
	          lastsize = newsize;
		else
	          done++;
	      }

	      folders[i].filesize = newsize;
	    }
	  }

	  sleep(interval_time);
	}
}

int
read_headers(current_folder)
int current_folder;
{
	/** read the headers, output as found given current_folder,
	    the prefix of that folder, and whether we're in a window
	    or not.
	**/

	char buffer[SLEN], from_whom[SLEN], subject[SLEN];
	register int subj = 0, in_header = 0, count = 0, priority=0;
#ifdef MMDF
	int newheader = 0;
#endif /* MMDF */

	from_whom[0] = '\0';

	while (fgets(buffer, SLEN, folders[current_folder].fd) != NULL) {
#ifdef MMDF
          if (strcmp(buffer, MSG_SEPERATOR) == 0) {
            newheader = 1;
            if (newheader) {
#else
	  if (first_word(buffer,"From ")) {
	    if (real_from(buffer, from_whom)) {
#endif /* MMDF */
	      subj = 0;
	      priority = 0;
	      in_header = 1;
	      subject[0] ='\0';
	      if (in_window)
	        putc((char) 007, stdout);		/* BEEP!*/
	      else
	        printf("\n\r");	/* blank lines surrounding message */

	    }
	  }
	  else if (in_header) {
#ifdef MMDF
            if (first_word(buffer,"From "))
              real_from(buffer, from_whom);
#endif /* MMDF */
	    if (first_word(buffer,">From"))
	      forwarded(buffer, from_whom); /* return address */
	    else if (first_word(buffer,"Subject:") ||
		     first_word(buffer,"Re:")) {
	      if (! subj++) {
	        remove_first_word(buffer);
		strcpy(subject, buffer);
	      }
	    }
	    else if (first_word(buffer,"Priority:"))
	      priority++;
	    else if (first_word(buffer,"From:"))
	      parse_arpa_from(buffer, from_whom);
	    else if (buffer[0] == LINEFEED) {
	      in_header = 0;	/* in body of message! */
#ifdef MMDF
              if (*from_whom == '\0')
                strcpy(from_whom,getusername());
#endif /* MMDF */
	      show_header(priority, from_whom, subject, current_folder);
	      count++;
	      from_whom[0] = '\0';
	    }
	  }
	}
	return(count);
}

add_folder(name)
char *name;
{
	/* add the specified folder to the list of folders...ignore any
	   problems we may having finding it (user could be monitoring
	   a mailbox that doesn't currently exist, for example)
	*/

	char *cp, buf[SLEN];

	if (current_folder > MAX_FOLDERS) {
	  fprintf(stderr,
              "Sorry, but I can only keep track of %d folders.\n", MAX_FOLDERS);
	  exit(1);
	}

	/* now let's rip off the suffix "=<string>" if it's there... */

	for (cp = name + strlen(name); cp > name+1 && *cp != '=' ; cp--)
	  /* just keep stepping backwards */ ;

	/* if *cp isn't pointing to the first character we'e got something! */

	if (cp > name+1) {

	  *cp++ = '\0';		/* null terminate the filename & get prefix */

	  if (metachar(*cp)) cp++;

	  strcpy(folders[current_folder].prefix, cp);
	}
	else {			/* nope, let's get the basename of the file */
	  for (cp = name + strlen(name); cp > name && *cp != '/'; cp--)
	    /* backing up a bit... */ ;

	  if (metachar(*cp)) cp++;
	  if (*cp == '/') cp++;

	  strcpy(folders[current_folder].prefix, cp);
	}

	/* and next let's see what kind of weird prefix chars this user
	   might be testing us with.  We can have '+'|'='|'%' to expand
	   or a file located in the incoming mail dir...
	*/

	if (metachar(name[0]))
	  expand_filename(name, folders[current_folder].foldername);
	else if (access(name, 00) == -1) {
	  /* let's try it in the mail home directory */
	  sprintf(buf, "%s%s", mailhome, name);
	  if (access(buf, 00) != -1) 		/* aha! */
	    strcpy(folders[current_folder].foldername, buf);
	  else
	    strcpy(folders[current_folder].foldername, name);
	}
	else
	  strcpy(folders[current_folder].foldername, name);

	/* now let's try to actually open the file descriptor and grab
	   a size... */

	if ((folders[current_folder].fd =
	      fopen(folders[current_folder].foldername, "r")) == NULL)
          if (errno == EACCES) {
	    fprintf(stderr, "\nPermission to monitor \"%s\" denied!\n\n",
			 folders[current_folder].foldername);
	    exit(1);
	  }

	folders[current_folder].filesize =
	      bytes(folders[current_folder].foldername);

	/* and finally let's output what we did */

	if (debug)
	  printf("folder %d: \"%s\" <%s> %s, size = %ld\n",
	      current_folder,
	      folders[current_folder].foldername,
	      folders[current_folder].prefix,
	      folders[current_folder].fd == NULL? "not found" : "opened",
	      folders[current_folder].filesize);

	/* and increment current-folder please! */

	current_folder++;
	total_folders++;
}

add_default_folder()
{
	char	*cp;

	/* this routine will add the users home mailbox as the folder
	 * to monitor.  Since there'll only be one folder we'll never
	 * prefix it either...
	 *	determine mail file from environment variable if found,
	 *	else use password entry
	 */
	if ((cp = getenv("MAIL")) == NULL)
	  sprintf(folders[0].foldername, "%s%s", mailhome, getusername());
	else
	  strcpy(folders[0].foldername, cp);

	folders[0].fd       = fopen(folders[0].foldername, "r");
	folders[0].filesize = bytes(folders[0].foldername);

	if (debug)
	  printf("default folder: \"%s\" <%s> %s, size = %ld\n",
	      folders[0].foldername,
	      folders[0].prefix,
	      folders[0].fd == NULL? "not found" : "opened",
	      folders[0].filesize);

	total_folders = 1;
}

int
real_from(buffer, who)
char *buffer, *who;
{
	/***** returns true iff 's' has the seven 'from' fields,
	       initializing the who to the sender *****/

	char junk[SLEN], who_tmp[SLEN];

	junk[0] = '\0';
	who_tmp[0] = '\0';

	sscanf(buffer, "%*s %s %*s %*s %*s %*s %s",
		    who_tmp, junk);

	if (junk[0] != '\0')
		strcpy(who, who_tmp);

	return(junk[0] != '\0');
}

forwarded(buffer, who)
char *buffer, *who;
{
	/** change 'from' and date fields to reflect the ORIGINATOR of
	    the message by iteratively parsing the >From fields... **/

	char machine[SLEN], buff[SLEN];

	machine[0] = '\0';
	sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %*s %*s %s",
	            who, machine);

	if(machine[0] == '\0')	/* try for address with timezone in date */
	sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %*s %s",
	            who, machine);

	if (machine[0] == '\0') /* try for srm address */
	  sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %s",
	            who, machine);

	if (machine[0] == '\0')
	  sprintf(buff,"anonymous");
	else
	  sprintf(buff,"%s!%s", machine, who);

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

show_header(priority, from, subject, current_folder)
int   priority;
char *from, *subject;
int   current_folder;
{
	/** output header in clean format, including abbreviation
	    of return address if more than one machine name is
	    contained within it! **/
	char buffer[SLEN];
	int  loc, i=0, exc=0, len;

#ifndef INTERNET
	/* Remove bogus "@host.domain" string. */

	sprintf(buffer, "@%s", hostname); /* hostname should contain FQDN! */

	if (chloc(from, '!') != -1 && in_string(from, buffer))
	  from[strlen(from) - strlen(buffer)] = '\0';
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
	  strcpy(from, buffer);
	}

	if (strlen(subject) < 2)
	  strcpy(subject, NO_SUBJECT);

	if (in_window)
	  if (total_folders > 1)
	    printf("%s: %s%s -- %s\n",
		   folders[current_folder].prefix,
		   priority? "Priority " : "", from, subject);
	  else
	    printf("%s%s -- %s\n",
		   priority? "Priority " : "", from, subject);
	else
	  if (total_folders > 1)
	    printf(">> %s: %sail from %s - %s\n\r",
		  folders[current_folder].prefix,
		  priority? "Priority m" : "M", from, subject);
	  else
	    printf(">> %sail from %s - %s\n\r",
		  priority? "Priority m" : "M", from, subject);
}

parse_arpa_from(buffer, newfrom)
char *buffer, *newfrom;
{
	/** try to parse the 'From:' line given... It can be in one of
	    three formats:
		From: Dave Taylor <hpcnou!dat>
	    or  From: hpcnou!dat (Dave Taylor)
	    or  From: hpcnou!dat
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
	else
	/* Unusual to have address like -  From: hpcnou!dat
	 * but valid */
	{
	  for (i=strlen("From: "); buffer[i] != '\0'; i++)
	    temp[j++] = buffer[i];
	  temp[j] = '\0';
	}

	if (strlen(temp) > 0) {		/* mess with buffer... */

	  /* remove leading spaces... */

	  while (whitespace(temp[0]))
	    temp = (char *) (temp + 1);		/* increment address! */

	  /* remove trailing spaces... */

	  i = strlen(temp) - 1;

	  while (whitespace(temp[i]))
	   temp[i--] = '\0';

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

long
bytes(name)
char *name;
{
	/** return the number of bytes in the specified file.  This
	    is to check to see if new mail has arrived....  **/

	int ok = 1;
	struct stat buffer;

	if (stat(name, &buffer) != 0)
	  if (errno != 2) {
	    fprintf(stderr,"Error %d attempting fstat on %s", errno, name);
	    exit(1);
	  }
	  else
	    ok = 0;

	return(ok ? buffer.st_size : 0);
}

char  *getusername()
{
	/** Getting the username on some systems is a real pain, so...
	   This routine is guaranteed to return a usable username **/

  struct passwd *password_entry;
  struct passwd *getpwuid();

  if (( password_entry = getpwuid(getuid())) != NULL)
    return(password_entry->pw_name);
}

usage(name)
char *name;
{
	/* print a nice friendly usage message */

	fprintf(stderr,
"\nUsage: %s [-d] [-i interval] [-w] {folders}\n", name);
	fprintf(stderr, "\nWhere:\n");
	fprintf(stderr, "  -d\tturns on debugging output\n");
	fprintf(stderr,
"  -i D\tsets the interval checking time to 'D' seconds\n");
	fprintf(stderr,
"  -w\tforces 'window'-style output, and bypasses auto-background\n\n");
	fprintf(stderr,
"folders can be specified by relative or absolute path names, can be the name\n");
	fprintf(stderr,
"of a mailbox in the incoming mail directory to check, or can have one of the\n");
	fprintf(stderr,
"standard Elm mail directory prefix chars (e.g. '+', '=' or '%').\n");
	fprintf(stderr,
"Furthermore, any folder can have '=string' as a suffix to indicate a folder\n");
	fprintf(stderr,
"identifier other than the basename of the file\n\n");
}


expand_filename(name, store_space)
char *name, *store_space;
{
	strcpy(store_space, name);
	if (expand(store_space) == 0) {
	  fprintf(stderr,"Sorry, but I couldn't expand \"%s\"\n",name);
	  exit(1);
	}
}

pad_prefixes()
{
	/** This simple routine is to ensure that we have a nice
	    output format.  What it does is whip through the different
	    prefix strings we've been given, figures out the maximum
	    length, then space pads the other prefixes to match.
	**/

	register int i, j, len = 0;

	for (i=0; i < total_folders; i++)
	  if (len < (j=strlen(folders[i].prefix)))
	    len = j;

	for (i=0; i < total_folders; i++)
	  for (j = strlen(folders[i].prefix); j < len; j++)
	    strcat(folders[i].prefix, " ");
}
