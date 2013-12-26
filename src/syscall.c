
static char rcsid[] = "@(#)$Id: syscall.c,v 4.1.1.4 90/07/12 22:41:55 syd Exp $";

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
 * $Log:	syscall.c,v $
 * Revision 4.1.1.4  90/07/12  22:41:55  syd
 * Make it aware of the fact that we loose the cursor position on
 * some system calls, so set it far enough off an absolute move will
 * be done on the next cursor address, and then place it where we want it.
 * From: Syd, reported by Douglas Lamb
 *
 * Revision 4.1.1.3  90/06/21  22:48:17  syd
 * patch to fix up the Log headers.
 * From: pdc%lunch.wpd@sgi.com (Paul Close)
 *
 * Revision 4.1.1.2  90/06/09  22:00:13  syd
 * Use a close-on-exec pipe to diagnose exec() failures.
 * From: tct!chip@uunet.UU.NET (Chip Salzenberg)
 *
 * Revision 4.1.1.1  90/06/09  21:33:22  syd
 * Some wait(2) system calls return -1 and set errno=EINTR (interrupted system
 * call) when the editor is invoked, suspended, and then resumed.  Loop until
 * wait either returns pid, or returns -1 with errno != EINTR.
 * From: pdc%lunch.wpd@sgi.com (Paul Close)
 *
 * Revision 4.1  90/04/28  22:44:18  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** These routines are used for user-level system calls, including the
    '!' command and the '|' commands...

**/

#include "headers.h"

#include <signal.h>
#include <errno.h>

#ifdef BSD
#  include <sys/wait.h>
#endif

#ifdef OS2
#  include <process.h>
#endif

char *argv_zero();
void  _exit();

#ifdef ALLOW_SUBSHELL

int
subshell()
{
	/** spawn a subshell with either the specified command
	    returns non-zero if screen rewrite needed
	**/

	char command[SLEN];
	int  old_raw, helpful, ret;

	helpful = (user_level == 0);

	if (helpful)
#ifdef OS2
	  PutLine0(LINES-3,COLUMNS-40,"(Enter empty command for a shell.)");
#else
	  PutLine0(LINES-3,COLUMNS-40,"(Use the shell name for a shell.)");
#endif
	PutLine0(LINES-2,0,"Shell command: ");
	CleartoEOS();
	command[0] = '\0';
	(void) optionally_enter(command, LINES-2, 15, FALSE, FALSE);
#ifndef OS2
	if (command[0] == 0) {
	  if (helpful)
	    MoveCursor(LINES-3,COLUMNS-40);
	  else
	    MoveCursor(LINES-2,0);
	  CleartoEOS();
	  return 0;
	}
#endif

	MoveCursor(LINES,0);
	CleartoEOLN();

	if ((old_raw = RawState()) == ON)
	  Raw(OFF);
	softkeys_off();
	if (cursor_control)
	  transmit_functions(OFF);

	umask(original_umask);	/* restore original umask so users new files are ok */
	ret = system_call(command, USER_SHELL, TRUE, TRUE);
	umask(077);		/* now put it back to private for mail files */

	SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
	Raw(ON);
	if (command[0]) {
	    PutLine0(LINES, 0, "\r\n\r\nPress any key to return to ELM: ");
	    (void) getchar();
	}
	if (old_raw == OFF)
	  Raw(OFF);
	softkeys_on();
	if (cursor_control)
	  transmit_functions(ON);

	if (ret && command[0])
	  error1("Return code was %d.", ret);

	return 1;
}

#endif /* ALLOW_SUBSHELL */

system_call(string, shell_type, allow_signals, allow_interrupt)
char *string;
int   shell_type, allow_signals, allow_interrupt;
{
	/** execute 'string', setting uid to userid... **/
	/** if shell-type is "SH" /bin/sh is used regardless of the
	    users shell setting.  Otherwise, "USER_SHELL" is sent.
	    If allow_signals is TRUE, then allow the executed
	    command handle hangup, and optionally if allow_interrupt
	    is also true handle interrupt in its own way.
	    This is useful for executed programs with
	    user interaction that handle those signals on their
	    own terms. It is especially important for vi, so that
	    a message being edited when a user connection is
	    dropped is recovered by vi's expreserve program **/

	int pfd[2], stat, pid, w, iteration;
	char *sh;
#if defined(BSD) && !defined(WEXITSTATUS)
	union wait status;
#else
	int status;
#endif
#ifdef VOIDSIG
	register void (*istat)(), (*qstat)();
# ifdef SIGTSTP
	register void (*oldstop)(), (*oldstart)();
# endif
#else
	register int (*istat)(), (*qstat)();
# ifdef SIGTSTP
	register int (*oldstop)(), (*oldstart)();
# endif
#endif
#ifndef OS2
	extern int errno;
#endif

	sh = (shell_type == USER_SHELL) ? shell : "/bin/sh";
	dprint(2, (debugfile, "System Call: %s\n\t%s\n", sh, string));

#ifdef OS2
        tflush();

	if ( shell_type != USER_SHELL )
          if ( (sh = getenv("COMSPEC")) == NULL )
            if ( (sh = getenv("OS2_SHELL")) == NULL )
              sh = default_shell;

	if (string[0] == 0) /* interactive subshell */
	  stat = spawnlp(P_WAIT, sh, sh, NULL);
	else
	{
#if 0
	  char cmd[VERY_LONG_STRING], *ptr;

	  strcpy(cmd, string);
	  ptr = strchr(cmd, ' ');

	  if (ptr == NULL)
	    stat = spawnlp(P_WAIT, sh, sh, "/c", string, NULL);
	  else
	  {
	    *ptr++ = 0;

	    if ((stat = spawnlp(P_WAIT, cmd, cmd, ptr, NULL)) == -1)
	      stat = spawnlp(P_WAIT, sh, sh, "/c", string, NULL);
	  }
#else
	  stat = spawnlp(P_WAIT, sh, sh, "/c", string, NULL);
#endif
	}
#else
	/*
	 * Note the neat trick with close-on-exec pipes.
	 * If the child's exec() succeeds, then the pipe read returns zero.
	 * Otherwise, it returns the zero byte written by the child
	 * after the exec() is attempted.  This is the cleanest way I know
	 * to discover whether an exec() failed.   --CHS
	 */

	if (pipe(pfd) == -1) {
	  perror("pipe");
	  return -1;
	}
	fcntl(pfd[0], F_SETFD, 1);
	fcntl(pfd[1], F_SETFD, 1);

	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
#ifdef SIGTSTP
	oldstop = signal(SIGTSTP, SIG_DFL);
	oldstart = signal(SIGCONT, SIG_DFL);
#endif

	stat = -1;		/* Assume failure. */

	for (iteration = 0; iteration < 5; ++iteration) {
	  if (iteration > 0)
	    sleep(2);

#ifdef VFORK
	  pid = vfork();
#else
	  pid = fork();
#endif

	  if (pid != -1)
	    break;
	}

	if (pid == -1) {
	  perror("fork");
	}
	else if (pid == 0) {
	  /*
	   * Set group and user back to their original values.
	   * Note that group must be set first.
	   */
	  setgid(groupid);
	  setuid(userid);

	  /*
	   * Program to exec may or may not be able to handle
	   * interrupt, quit, hangup and stop signals.
	   */
	  (void) signal(SIGHUP, allow_signals ? SIG_DFL : SIG_IGN);
	  (void) signal(SIGINT, (allow_signals && allow_interrupt)?SIG_DFL:SIG_IGN);
	  (void) signal(SIGQUIT, (allow_signals && allow_interrupt)?SIG_DFL:SIG_IGN);
#ifdef SIGTSTP
	  (void) signal(SIGTSTP, allow_signals ? SIG_DFL : SIG_IGN);
	  (void) signal(SIGCONT, allow_signals ? SIG_DFL : SIG_IGN);
#endif

	  /* Go for it. */
	  execl(sh, argv_zero(sh), "-c", string, (char *) 0);

	  /* If exec fails, we write a byte to the pipe before exiting. */
	  perror(sh);
	  write(pfd[1], "", 1);
	  _exit(127);
	}
	else {
	  int rd;
	  char ch;

	  /* Try to read a byte from the pipe. */
	  close(pfd[1]);
	  rd = read(pfd[0], &ch, 1);
	  close(pfd[0]);

	  while ((w = wait(&status)) != pid)
	      if (w == -1 && errno != EINTR)
		  break;

	  /* If we read a byte from the pipe, the exec failed. */
	  if (rd > 0)
	    stat = -1;
	  else if (w == pid) {
#ifdef	WEXITSTATUS
	    stat = WEXITSTATUS(status);
#else
# ifdef	BSD
	    stat = status.w_retcode;
# else
	    stat = status;
# endif
#endif
	  }
  	}

	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
#ifdef SIGTSTP
	(void) signal(SIGTSTP, oldstop);
	(void) signal(SIGCONT, oldstart);
#endif

#endif  /* OS2 */

	return(stat);
}

int
do_pipe()
{
	/** pipe the current message or tagged messages to
	    the specified sequence.. **/

	char command[SLEN], buffer[SLEN], message_list[SLEN];
	register int  ret, to_pipe;
	int	old_raw;

	to_pipe = make_msg_list(message_list);
	sprintf(buffer, "Pipe message%s to: ", plural(to_pipe));
        PutLine0(LINES-2,0,buffer);

	command[0] = '\0';

	(void) optionally_enter(command, LINES-2, strlen(buffer), FALSE, FALSE);
	if (strlen(command) == 0) {
	  MoveCursor(LINES-2,0);	CleartoEOLN();
	  return(0);
	}

	MoveCursor(LINES,0); 	CleartoEOLN();
	if (( old_raw = RawState()) == ON)
	  Raw(OFF);

	if (cursor_control)  transmit_functions(OFF);

	sprintf(buffer, "%s -f %s -h %s | %s",
		readmsg,
		(folder_type == NON_SPOOL ? cur_folder : cur_tempfolder),
		message_list,
		command);

	ret = system_call(buffer, USER_SHELL, TRUE, TRUE);

	SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
	PutLine0(LINES, 0, "\n\nPress any key to return to ELM.");
	if (old_raw == ON)
	   Raw(ON);
	(void) getchar();
	if (cursor_control)  transmit_functions(ON);

	if (ret != 0) error1("Return code was %d.", ret);
	return(1);
}

print_msg()
{
	/** Print current message or tagged messages using 'printout'
	    variable.  Error message iff printout not defined! **/

	char buffer[SLEN], filename[SLEN], printbuffer[SLEN];
	char message_list[SLEN];
	register int  retcode, to_print, cnt;

	if (strlen(printout) == 0) {
	  error("Don't know how to print - option \"printmail\" undefined!");
	  return;
	}

	to_print = make_msg_list(message_list);

	sprintf(filename,"%s%d%s", temp_dir, getpid(), temp_print);
	os2path(filename);

	if (in_string(printout, "%s"))
	  sprintf(printbuffer, printout, filename);
	else
	  sprintf(printbuffer, "%s %s", printout, filename);

	sprintf(buffer,"%s -p -f %s %s >%s & %s 1>nul 2>nul",
		readmsg,
		(folder_type == NON_SPOOL ? cur_folder : cur_tempfolder),
		message_list,
		filename,
		printbuffer);

	dprint(2, (debugfile, "Printing system call...\n"));

  	Centerline(LINES, "Queuing...");

	if ((retcode = system_call(buffer, SH, FALSE, FALSE)) == 0) {
	  sprintf(buffer, "Message%s queued up to print.", plural(to_print));
	  Centerline(LINES, buffer);
	}
	else
	  error1("Printout failed with return code %d.", retcode);

	unlink(filename);	/* remove da temp file! */
}

make_msg_list(message_list)
char *message_list;
{
	/** make a list of the tagged or just the current, if none tagged.
		check for overflow on messsage length
         **/

	int i, msgs_selected = 0;

	*message_list = '\0';	/* start with an empty list */

	for (i=0; i < message_count; i++)
	  if (headers[i]->status & TAGGED) {
	    if (strlen(message_list) + 6 >= SLEN) {
	      error1("Too many messages selected, messages from %d on not used", i);
	      return(msgs_selected);
	      }
	    sprintf(message_list, "%s %d", message_list,
		    headers[i]->index_number);
	    msgs_selected++;
	  }

	if (! msgs_selected) {
	  sprintf(message_list," %d", headers[current-1]->index_number);
	  msgs_selected = 1;
	}

	return(msgs_selected);
}

list_folders(numlines, helpmsg)
unsigned numlines;
char *helpmsg;
{
	/** list the folders in the users FOLDERHOME directory.  This is
	    simply a call to "ls -C"
	    Numlines is the number of lines to scroll afterwards. This is
	    useful when a portion of the screen needs to be cleared for
	    subsequent prompts, but you don't want to overwrite the
	    list of folders.
	    Helpmsg is what should be printed before the listing if not NULL.
	**/

        char buffer[SLEN], foldersbs[SLEN];

	Raw(OFF);
	ClearScreen();
	MoveCursor(0, 0);
	if(helpmsg)
	  printf(helpmsg);

        strcpy(foldersbs,folders);
        os2path(foldersbs);
        sprintf(buffer, "dir /w %s", foldersbs);

	printf("\n\rContents of your folder directory:\n\r\n\r");
	system_call(buffer, SH, FALSE, FALSE);
	while(numlines--)
	    printf("\n\r");
	Raw(ON);
}
