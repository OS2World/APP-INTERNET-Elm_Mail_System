
static char rcsid[] = "@(#)$Id: file_util.c,v 4.1 90/04/28 22:43:04 syd Exp $";

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
 * $Log:	file_util.c,v $
 * Revision 4.1  90/04/28  22:43:04  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** File oriented utility routines for ELM

**/

#include "headers.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#ifdef BSD
# undef tolower
#endif

#include <signal.h>

#ifdef BSD
# include <sys/wait.h>
#endif

#ifndef OS2
extern int errno;		/* system error number */
#endif

char *error_name(), *error_description(), *strcpy(), *getlogin();
long  fsize();

long
bytes(name)
char *name;
{
	/** return the number of bytes in the specified file.  This
	    is to check to see if new mail has arrived....  (also
	    see "fsize()" to see how we can get the same information
	    on an opened file descriptor slightly more quickly)
	**/

	int ok = 1;
	struct stat buffer;

	if (stat(name, &buffer) != 0)
	  if (errno != 2) {
	    dprint(1,(debugfile,
		     "Error: errno %s on fstat of file %s (bytes)\n",
		     error_name(errno), name));
	    Write_to_screen("\n\rError attempting fstat on file %s!\n\r",
		     1, name);
	    Write_to_screen("** %s - %s. **\n\r", 2, error_name(errno),
		  error_description(errno));
	    emergency_exit();
	  }
	  else
	    ok = 0;

	return(ok ? (long) buffer.st_size : 0L);
}

int
can_access(file, mode)
char *file;
int   mode;
{
	/** returns ZERO iff user can access file or "errno" otherwise **/

	int the_stat = 0, pid, w;
	struct stat stat_buf;

#ifdef OS2
        the_stat = (access(file, mode) == 0) ? 0 : (errno == 0) ? 1 : errno;
#else
	void _exit(), exit();
#if defined(BSD) && !defined(WEXITSTATUS)
	union wait status;
#else
	int status;
#endif
#ifdef VOIDSIG
	register void (*istat)(), (*qstat)();
#else
	register int (*istat)(), (*qstat)();
#endif

#ifdef VFORK
	if ((pid = vfork()) == 0) {
#else
	if ((pid = fork()) == 0) {
#endif
	  setgid(groupid);
	  setuid(userid);		/** back to normal userid **/

	  errno = 0;

	  if (access(file, mode) == 0)
	    _exit(0);
	  else
	    _exit(errno != 0? errno : 1);	/* never return zero! */
	  _exit(127);
	}

	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);

	while ((w = wait(&status)) != pid && w != -1)
		;

#if	defined(WEXITSTATUS)
	/* Use POSIX macro if defined */
	the_stat = WEXITSTATUS(status);
#else
#ifdef BSD
	the_stat = status.w_retcode;
#else
	the_stat = status >> 8;
#endif
#endif	/*WEXITSTATUS*/

	signal(SIGINT, istat);
	signal(SIGQUIT, qstat);
#endif

	if (the_stat == 0) {
	  if (stat(file, &stat_buf) == 0) {
	    w = stat_buf.st_mode & S_IFMT;
#ifdef S_IFLNK
	    if (w != S_IFREG && w != S_IFLNK)
#else
	    if (w != S_IFREG)
#endif
	      the_stat = 1;
	  }
	}

	return(the_stat);
}

int
can_open(file, mode)
char *file, *mode;
{
	/** Returns 0 iff user can open the file.  This is not
	    the same as can_access - it's used for when the file might
	    not exist... **/

	FILE *fd;
	int the_stat = 0, pid, w, preexisted = 0;
#ifdef OS2
	errno = 0;
	if (access(file, ACCESS_EXISTS) == 0)
	  preexisted = 1;
	if ((fd = fopen(file, mode)) == NULL)
	  the_stat = errno;
	else {
	  fclose(fd);		/* don't just leave it open! */
	  if(!preexisted)	/* don't leave it if this test created it! */
	    unlink(file);
	  the_stat = 0;
	}
#else
	void _exit(), exit();
#if defined(BSD) && !defined(WEXITSTATUS)
	union wait status;
#else
	int status;
#endif
#ifdef VOIDSIG
	register void (*istat)(), (*qstat)();
#else
	register int (*istat)(), (*qstat)();
#endif

#ifdef VFORK
	if ((pid = vfork()) == 0) {
#else
	if ((pid = fork()) == 0) {
#endif
	  setgid(groupid);
	  setuid(userid);		/** back to normal userid **/
	  errno = 0;
	  if (access(file, ACCESS_EXISTS) == 0)
	    preexisted = 1;
	  if ((fd = fopen(file, mode)) == NULL)
	    _exit(errno);
	  else {
	    fclose(fd);		/* don't just leave it open! */
	    if(!preexisted)	/* don't leave it if this test created it! */
	      unlink(file);
	    _exit(0);
	  }
	  _exit(127);
	}

	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);

	while ((w = wait(&status)) != pid && w != -1)
		;

#ifdef WEXITSTATUS
	the_stat = WEXITSTATUS(status);
#else
#ifdef BSD
	the_stat = status.w_retcode;
#else
	the_stat = status >> 8;
#endif
#endif /*WEXITSTATUS*/

	signal(SIGINT, istat);
	signal(SIGQUIT, qstat);
#endif

	return(the_stat);
}

int
copy(from, to)
char *from, *to;
{
	/** this routine copies a specified file to the destination
	    specified.  Non-zero return code indicates that something
	    dreadful happened! **/

	FILE *from_file, *to_file;
	char buffer[VERY_LONG_STRING];

	if ((from_file = fopen(from, "r")) == NULL) {
	  dprint(1, (debugfile, "Error: could not open %s for reading (copy)\n",
		 from));
	  error1("Could not open file %s.", from);
	  return(1);
	}

	if ((to_file = fopen(to, "w")) == NULL) {
	  dprint(1, (debugfile, "Error: could not open %s for writing (copy)\n",
		 to));
	  error1("Could not open file %s.", to);
	  return(1);
	}

	while (fgets(buffer, VERY_LONG_STRING, from_file) != NULL)
	  if (fputs(buffer, to_file) == EOF) {
	      Write_to_screen("\n\rWrite failed to tempfile in copy\n\r", 0);
	      perror(to);
	      fclose(to_file);
	      fclose(from_file);
	      return(1);
	  }
	fclose(from_file);
        if (fclose(to_file) == EOF) {
	  Write_to_screen("\n\rClose failed on tempfile in copy\n\r", 0);
	  perror(to);
	  return(1);
	}
	chown( to, userid, groupid);

	return(0);
}

int
append(fd, filename)
FILE *fd;
char *filename;
{
	/** This routine appends the specified file to the already
	    open file descriptor.. Returns non-zero if fails.  **/

	FILE *my_fd;
	char buffer[VERY_LONG_STRING];

	if ((my_fd = fopen(filename, "r")) == NULL) {
	  dprint(1, (debugfile,
		"Error: could not open %s for reading (append)\n", filename));
	  return(1);
	}

	while (fgets(buffer, VERY_LONG_STRING, my_fd) != NULL)
	  if (fputs(buffer, fd) == EOF) {
	      Write_to_screen("\n\rWrite failed to tempfile in append\n\r", 0);
	      perror(filename);
	      rm_temps_exit();
	  }

	if (fclose(my_fd) == EOF) {
	  Write_to_screen("\n\rClose failed on tempfile in append\n\r", 0);
	  perror(filename);
	  rm_temps_exit();
	}

	return(0);
}

#define FORWARDSIGN	"Forward to "
int
check_mailfile_size(mfile)
char *mfile;
{
	/** Check to ensure we have mail.  Only used with the '-z'
	    starting option. So we output a diagnostic if there is
	    no mail to read (including  forwarding).
	    Return 0 if there is mail,
		   <0 if no permission to check,
		   1 if no mail,
		   2 if no mail because mail is being forwarded.
	 **/

	char firstline[SLEN];
	int retcode;
	struct stat statbuf;
	FILE *fp;

	/* see if file exists first */
	if (access(mfile, ACCESS_EXISTS) != 0)
	  retcode = 1;					/* no file */

	/* exists - now see if user has read access */
	else if (can_access(mfile, READ_ACCESS) != 0)
	  retcode = -1;					/* no perm */

	/* read access - now see if file has a reasonable size */
	else if ((fp = fopen(mfile, "r")) == NULL)
	  retcode = -1;		/* no perm? should have detected this above! */
	else if (fstat(fileno(fp), &statbuf) == -1)
	  retcode = -1;					/* arg error! */
	else if (statbuf.st_size < 2)
	  retcode = 1;	/* empty or virtually empty, e.g. just a newline */

	/* file has reasonable size - see if forwarding */
	else if (fgets (firstline, SLEN, fp) == NULL)
	  retcode = 1;		 /* empty? should have detected this above! */
	else if (first_word(firstline, FORWARDSIGN))
	  retcode = 2;					/* forwarding */

	/* not forwarding - so file must have some mail in it */
	else
	  retcode = 0;

	/* now display the appropriate message if there isn't mail in it */
	switch(retcode) {

	case -1:	printf("\r\nYou have no permission to read %s!\r\n", mfile);
			break;
	case 1:		printf("\r\nYou have no mail.\r\n");
			break;
	case 2:		no_ret(firstline) /* remove newline before using */
			printf("Your mail is being forwarded to %s.\n\r",
			  firstline + strlen(FORWARDSIGN));
			break;
	}
	return(retcode);
}

create_readmsg_file()
{
	/** Creates the file ".current" in the users home directory
	    for use with the "readmsg" program.
	**/

	FILE *fd;
	char buffer[SLEN];

	sprintf(buffer,"%s/%s", home, readmsg_file);

	if ((fd = fopen (buffer, "w")) == NULL) {
	  dprint(1, (debugfile,
		 "Error: couldn't create file %s - error %s (%s)\n",
		 buffer, error_name(errno), "create_readmsg_file"));
	  return;	/* no error to user */
	}

	if (current)
	  fprintf(fd, "%d\n", headers[current-1]->index_number);
	else
	  fprintf(fd, "\n");

	fclose(fd);
	chown( buffer, userid, groupid);
}

long fsize(fd)
FILE *fd;
{
	/** return the size of the current file pointed to by the given
	    file descriptor - see "bytes()" for the same function with
	    filenames instead of open files...
	**/

	struct stat buffer;

	(void) fstat(fileno(fd), &buffer);

	return( (long) buffer.st_size );
}
