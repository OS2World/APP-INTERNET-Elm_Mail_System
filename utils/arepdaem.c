
static char rcsid[] = "@(#)$Id: arepdaem.c,v 4.1.1.4 90/12/05 15:12:52 syd Exp $";

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
 * $Log:	arepdaem.c,v $
 * Revision 4.1.1.4  90/12/05  15:12:52  syd
 * Fix lock file flags, wrong mode flag used
 * From: Syd via Terry Furman
 *
 * Revision 4.1.1.3  90/10/07  20:39:31  syd
 * Added missing parens to an imbedded assignment.
 * From: Phil Hochstetler <phil@sequent.com>
 *
 * Revision 4.1.1.2  90/08/15  22:50:14  syd
 * Fix last size to time call
 * From: Syd
 *
 * Revision 4.1.1.1  90/08/15  22:33:54  syd
 * Fix to use time instead of bytes for changes to file and to process
 * each entry on delete properly
 * From: Denis Lambot <d241s016!lde@swn.siemens.be>
 *
 * Revision 4.1  90/04/28  22:44:33  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Keep track of mail as it arrives, and respond by sending a 'recording'
    file to the sender as new mail is received.

    Note: the user program that interacts with this program is the
    'autoreply' program and that should be consulted for further
    usage information.

    This program is part of the 'autoreply' system, and is designed
    to run every hour and check all mailboxes listed in the file
    "/etc/autoreply.data", where the data is in the form:

	username	replyfile	current-mailfile-size

    To avoid a flood of autoreplies, this program will NOT reply to mail
    that contains header "X-Mailer: fastmail".  Further, each time the
    program responds to mail, the 'mailfile size' entry is updated in
    the file /etc/autoreply.data to allow the system to be brought
    down and rebooted without any loss of data or duplicate messages.

    This daemon also uses a lock semaphore file, /usr/spool/uucp/LCK..arep,
    to ensure that more than one copy of itself is never running.  For this
    reason, it is recommended that this daemon be started up each morning
    from cron, since it will either start since it's needed or simply see
    that the file is there and disappear.

    Since this particular program is the main daemon answering any
    number of different users, it must be run with uid root.
**/

#include <stdio.h>
#include "defs.h"

#ifdef BSD
# include <sys/time.h>
#else
# include <time.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

static char ident[] = { WHAT_STRING };

char     arep_lock_file[SLEN];  /* autoreply lock file  */
char     autoreply_file[SLEN];  /* autoreply data file  */
char     logfile[SLEN];         /* first choice   */
#define logfile2	("/" AUTOREP_LOG)	/* second choice  */

#define BEGINNING	0		/* see fseek(3S) for info */
#define SLEEP_TIME	1800 /* 3600		/* run once an hour       */
#define MAX_PEOPLE	20		/* max number in program  */

#define EXISTS		00		/* lock file exists??     */

#define remove_return(s)	if (strlen(s) > 0) { \
			          if (s[strlen(s)-1] == '\n') \
				    s[strlen(s)-1] = '\0'; \
		                }

struct replyrec {
	char 	username[NLEN];		/* login name of user */
	char	mailfile[SLEN];		/* name of mail file  */
	char    replyfile[SLEN];	/* name of reply file */
	long    mailsize;		/* mail file size     */
	int     in_list;		/* for new replies    */
      } reply_table[MAX_PEOPLE];

FILE  *logfd;				/* logfile (log action)   */
time_t autoreply_time = 0L;		/* modif date of autoreply file */
int   active = 0;			/* # of people 'enrolled' */

FILE  *open_logfile();			/* forward declaration    */

long  bytes();				/*       ditto 		  */
time_t ModTime();			/*       ditto		  */

#ifdef VOIDSIG
void	term_signal();
#else
int	term_signal();
#endif

main()
{
	long size;
	int  person, data_changed;
	time_t time;

        initpaths();
        sprintf(autoreply_file, "%s/%s", elmhome, AUTOREP_FILE);
        sprintf(logfile,        "%s/%s", elmhome, AUTOREP_LOG);
        sprintf(arep_lock_file, "%s/%s", elmhome, AUTOREP_LOCK);

#ifndef OS2
	if (fork()) exit(0);
#endif

	if (! lock())
	  exit(0);	/* already running! */

	signal(SIGINT, term_signal); 	/* Terminate signal         */
	signal(SIGTERM, term_signal); 	/* Terminate signal         */

/*
 *	note the usage of the BSD style setpgrp wont hurt
 *	system V as its calling sequence is no arguments.
 *	The idea is to disassociate from the terminal to
 *	prevent signals.
 */
#ifndef OS2
	person = getpid();
	setpgrp(person, person);
#endif

	while (1) {

	  logfd = open_logfile();	/* open the log */

	  /* 1. check to see if autoreply table has changed.. */

	  if ((time = ModTime(autoreply_file)) != autoreply_time) {
	    read_autoreply_file();
	    autoreply_time = time;
	  }

	  /* 2. now for each active person... */

	  /* move_incoming_mail(); */
	  data_changed = 0;

	  for (person = 0; person < active; person++) {
	    if ((size = bytes(reply_table[person].mailfile)) !=
		reply_table[person].mailsize) {
	      if (size > reply_table[person].mailsize)
	        read_newmail(person);
	      /* else mail removed - resync */
	      reply_table[person].mailsize = size;
	      data_changed++;
	    }
	  }

	  /* 3. if data changed, update autoreply file */

	  if (data_changed)
	    update_autoreply_file();

	  close_logfile();		/* close the logfile again */

	  /* 4. Go to sleep...  */

	  sleep(SLEEP_TIME);
	}
}

int
read_autoreply_file()
{
	/** We're here because the autoreply file has changed!!  It
	    could either be because someone has been added or because
	    someone has been removed...since the list will always be in
	    order (nice, eh?) we should have a pretty easy time of it...
	**/

	FILE *file;
	char username[SLEN], 	replyfile[SLEN];
	int  person;
 	long size;

	log("Autoreply data file has changed!  Reading...");

/*
 * clear old entries prior to reread
 */
	for (person = 0; person < active; person++)
	  reply_table[person].in_list = 0;

	if ((file = fopen(autoreply_file,"r")) == NULL) {
	  log("No-one is using autoreply...");
	} else {
	  while (fscanf(file, "%s %s %ld", username, replyfile, &size) != EOF) {
	    /* check to see if this person is already in the list */
	    if ((person = in_list(username)) != -1) {
	      reply_table[person].in_list = 1;
	      reply_table[person].mailsize = size;	 /* sync */
	    }
	    else { 	/* if not, add them */
	      if (active == MAX_PEOPLE) {
		unlock();
		exit(log("Couldn't add %s - already at max people!",
			   username));
	      }
	      log("adding %s to the active list", username);
	      strcpy(reply_table[active].username, username);
	      sprintf(reply_table[active].mailfile, "%s%s", mailhome, username);
	      strcpy(reply_table[active].replyfile, replyfile);
	      reply_table[active].mailsize = size;
	      reply_table[active].in_list = 1;	/* obviously! */
	      active++;
	    }
	  }
	  fclose(file);
	}

	/** now check to see if anyone has been removed... **/

	person = 0;
	while (person < active)
	  if (reply_table[person].in_list) {
	    person++;
	  }
	  else {
	    log("removing %s from the active list",
		   reply_table[person].username);
	    strcpy(reply_table[person].username,
		   reply_table[active-1].username);
	    strcpy(reply_table[person].mailfile,
		   reply_table[active-1].mailfile);
	    strcpy(reply_table[person].replyfile,
		   reply_table[active-1].replyfile);
	    reply_table[person].mailsize = reply_table[active-1].mailsize;
	    active--;
	  }
}

update_autoreply_file()
{
	/** update the entries in the autoreply file... **/

	FILE *file;
	register int person;

	if ((file = fopen(autoreply_file,"w")) == NULL) {
          log("Couldn't update autoreply file!");
	  return;
	}

	for (person = 0; person < active; person++)
	  fprintf(file, "%s %s %ld\n",
		  reply_table[person].username,
		  reply_table[person].replyfile,
		  reply_table[person].mailsize);

	fclose(file);

/*	printf("updated autoreply file\n"); */
	autoreply_time = ModTime(autoreply_file);
}

int
in_list(name)
char *name;
{
	/** search the current active reply list for the specified username.
	    return the index if found, or '-1' if not. **/

	register int iindex;

	for (iindex = 0; iindex < active; iindex++)
	  if (strcmp(name, reply_table[iindex].username) == 0)
	    return(iindex);

	return(-1);
}

read_newmail(person)
int person;
{
	/** Read the new mail for the specified person. **/


	FILE *mailfile;
	char from_whom[SLEN], subject[SLEN];
	int  sendit;

	log("New mail for %s", reply_table[person].username);

        if ((mailfile = fopen(reply_table[person].mailfile,"rb")) == NULL)
           return(log("can't open mailfile for user %s",
		    reply_table[person].username));

        if (fseek(mailfile, reply_table[person].mailsize, BEGINNING) == -1)
           return(log("couldn't seek to %ld in mail file!",
	               reply_table[person].mailsize));

	while (get_return(mailfile, person, from_whom, subject, &sendit) != -1)
	  if (sendit)
	    reply_to_mail(person, from_whom, subject);

	return;
}

int
get_return(file, person, from, subject, sendit)
FILE *file;
int  person, *sendit;
char *from, *subject;
{
	/** Reads the new message and return the from and subject lines.
	    sendit is set to true iff it isn't a machine generated msg
	**/

    char name1[SLEN], name2[SLEN], lastname[SLEN];
    char buffer[SLEN], hold_return[NLEN];
    int done = 0, in_header = 0;

    from[0] = '\0';
    name1[0] = '\0';
    name2[0] = '\0';
    lastname[0] = '\0';
    *sendit = 1;

    while (! done) {

      if (fgets(buffer, SLEN, file) == NULL)
	return(-1);

      fixline(buffer);

      if (first_word(buffer, "From ")) {
	in_header++;
	sscanf(buffer, "%*s %s", hold_return);
      }
      else if (in_header) {
        if (first_word(buffer, ">From")) {
	  sscanf(buffer,"%*s %s %*s %*s %*s %*s %*s %*s %*s %s", name1, name2);
	  add_site(from, name2, lastname);
        }
        else if (first_word(buffer,"Subject:")) {
	  remove_return(buffer);
	  strcpy(subject, (char *) (buffer + 8));
        }
        else if (first_word(buffer,"X-Mailer: fastmail"))
	  *sendit = 0;
        else if (strlen(buffer) == 1)
	  done = 1;
      }
    }

    if (from[0] == '\0')
      strcpy(from, hold_return); /* default address! */
    else
      add_site(from, name1, lastname);	/* get the user name too! */

    return(0);
}

add_site(buffer, site, lastsite)
char *buffer, *site, *lastsite;
{
	/** add site to buffer, unless site is 'uucp', or the same as
	    lastsite.   If not, set lastsite to site.
	**/

	char local_buffer[SLEN], *strip_parens();

	if (strcmp(site, "uucp") != 0)
	  if (strcmp(site, lastsite) != 0) {
	      if (buffer[0] == '\0')
	        strcpy(buffer, strip_parens(site));         /* first in list! */
	      else {
	        sprintf(local_buffer,"%s!%s", buffer, strip_parens(site));
	        strcpy(buffer, local_buffer);
	      }
	      strcpy(lastsite, strip_parens(site)); /* don't want THIS twice! */
	   }
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

reply_to_mail(person, from, subject)
int   person;
char *from, *subject;
{
	/** Respond to the message from the specified person with the
	    specified subject... **/

	char buffer[SLEN];

	if (strlen(subject) == 0)
	  strcpy(subject, "Auto-reply Mail");
	else if (! first_word(subject,"Auto-reply")) {
	  sprintf(buffer, "Auto-reply to:%s", subject);
	  strcpy(subject, buffer);
	}

	log("auto-replying to '%s'", from);

	mail(from, subject, reply_table[person].replyfile, person);
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
	   unlock();
	   exit(fprintf(stderr,"Error %d attempting fstat on %s", errno, name));
	  }
	  else
	    ok = 0;

	return(ok ? buffer.st_size : 0);
}

time_t
ModTime(name)
char *name;
{
	/** return the modification time in the specified file.
	    This is to check to see if autoreply has changed....  **/

	int ok = 1;
	struct stat buffer;

	if (stat(name, &buffer) != 0)
	  if (errno != 2) {
	   unlock();
	   exit(fprintf(stderr,"Error %d attempting fstat on %s", errno, name));
	  }
	  else
	    ok = 0;

	return(ok ? buffer.st_mtime : (time_t) 0);
}

mail(to, subject, filename, person)
char *to, *subject, *filename;
int   person;
{
	/** Mail 'file' to the user from person... **/

	char buffer[VERY_LONG_STRING];

	sprintf(buffer, "fastmail -f \"%s [autoreply]\" -s \"%s\" %s %s",
		reply_table[person].username, subject, filename, to);

	system(buffer);
}

log(message, arg)
char *message;
char *arg;
{
	/** Put log entry into log file.  Use the format:
	      date-time: <message>
	**/

	struct tm *thetime;
	long	  clock;
#ifndef	_POSIX_SOURCE
	struct tm *localtime();
	time_t time();
#endif
	char      buffer[SLEN];

	/** first off, get the time and date **/

	clock = time((long *) 0);       /* seconds since ???   */
	thetime = localtime(&clock);	/* and NOW the time... */

	/** then put the message out! **/

	sprintf(buffer, message, arg);

	fprintf(logfd,"%d/%d-%d:%02d: %s\n",
		thetime->tm_mon+1, thetime->tm_mday,
	        thetime->tm_hour,  thetime->tm_min,
	        buffer);
}

FILE *open_logfile()
{
	/** open the logfile.  returns a valid file descriptor **/

	FILE *fd;

	if ((fd = fopen(logfile, "a")) == NULL)
	  if ((fd = fopen(logfile2, "a")) == NULL) {
	    unlock();
	    exit(1);	/* give up! */
	  }

	return( (FILE *) fd);
}

close_logfile()
{
	/** Close the logfile until needed again. **/

	fclose(logfd);
}

char *strip_parens(string)
char *string;
{
	/** Return string with all parenthesized information removed.
	    This is a non-destructive algorithm... **/

	static char  buffer[SLEN];
	register int depth = 0, buffer_index = 0;

	for (; *string; string++) {
	  if (*string == '(')
	    depth++;
	  else if (*string == ')')
	    depth--;
	  else if (depth == 0)
	    buffer[buffer_index++] = *string;
	}

	buffer[buffer_index] = '\0';

	return( (char *) buffer);
}

/*** LOCK and UNLOCK - ensure only one copy of this daemon running at any
     given time by using a file existance semaphore (wonderful stuff!) ***/

lock()
{
	char lock_name[SLEN];		/* name of lock file  */
	char pid_buffer[SHORT];
	int pid, create_fd;

	strcpy(lock_name, arep_lock_file);
#ifdef PIDCHECK
      /** first, try to read the lock file, and if possible, check the pid.
	  If we can validate that the pid is no longer active, then remove
	  the lock file.
       **/
	if((create_fd=open(lock_name,O_RDONLY)) != -1) {
	  if (read(create_fd, pid_buffer, SHORT) > 0) {
	    pid = atoi(pid_buffer);
	    if (pid) {
	      if (kill(pid, 0)) {
	        close(create_fd);
	        if (unlink(lock_name) != 0) {
		    printf("Error %s (%s)\n\ttrying to unlink file %s (%s)\n",
		    error_name(errno), error_description(errno), lock_name, "lock");
		    return(0);
	        }
	      } else /* kill pid check succeeded */
	        return(0);
	    } else /* pid was zero */
	      return(0);
	  } else /* read failed */
	    return(0);
	}
	/* ok, either the open failed or we unlinked it, now recreate it. */
#else
	if (access(lock_name, EXISTS) == 0)
	  return(0);	/* file already exists */
#endif

	if ((create_fd=creat(lock_name, 0444)) == -1)
	  return(0);	/* can't create file!!   */

	sprintf(pid_buffer,"%d\n", getpid() );		/* write the current pid to the file */
	write(create_fd, pid_buffer, strlen(pid_buffer));
	close(create_fd);				/* no need to keep it open */

	return(1);

}

unlock()
{
	/** remove lock file if it's there! **/

        chmod(arep_lock_file, 0666);
	(void) unlink(arep_lock_file);
}

#ifdef VOIDSIG
void	term_signal()
#else
int	term_signal()
#endif
{
	unlock();
	exit(1);	/* give up! */
}
