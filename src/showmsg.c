
static char rcsid[] = "@(#)$Id: showmsg.c,v 4.1.1.1 90/07/12 23:04:26 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.1 $   $State: Exp $
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
 * $Log:	showmsg.c,v $
 * Revision 4.1.1.1  90/07/12  23:04:26  syd
 * Fix MMDF case, where MSG_SEPERATOR has newline, buffer check
 * didnt, thus it didnt detect the MSG_SEPERATOR.
 * From: jbwaters@bsu-cs.bsu.edu (J. Brian Waters)
 *
 * Revision 4.1  90/04/28  22:44:06  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This file contains all the routines needed to display the specified
    message.
**/

#include "headers.h"
#include <ctype.h>
#include <errno.h>

#ifdef BSD
# include <sys/wait.h>
# undef       tolower
#endif

#ifdef OS2
# include <sys/wait.h>
#else
extern int errno;
#endif

char *error_name(), *strcat(), *strcpy();
void   _exit();

int    memory_lock = FALSE;	/* is it available?? */
int    pipe_abort  = FALSE;	/* did we receive a SIGNAL(SIGPIPE)? */

FILE *pipe_wr_fp;		/* file pointer to write to external pager */
extern int lines_displayed,	/* defined in "builtin" */
	   lines_put_on_screen;	/*    ditto too!        */

int
show_msg(number)
int number;
{
	/*** Display number'th message.  Get starting and ending lines
	     of message from headers data structure, then fly through
	     the file, displaying only those lines that are between the
	     two!

	     Return 0 to return to the index screen or a character entered
	     by the user to initiate a command without returning to
	     the index screen (to be processed via process_showmsg_cmd()).
	***/

	char title1[SLEN], title2[SLEN], title3[SLEN], titlebuf[SLEN];
	char who[LONG_STRING], buffer[VERY_LONG_STRING];
#if defined(BSD) && !defined(WEXITSTATUS)
	union wait wait_stat;
#else
	int wait_stat;
#endif

	int crypted = 0;			/* encryption */
	int weed_header, weeding_out = 0;	/* weeding    */
	int using_to,				/* misc use   */
	    pipe_fd[2],				/* pipe file descriptors */
	    new_pipe_fd,			/* dup'ed pipe fil des */
	    lines,				/* num lines in msg */
	    fork_ret,				/* fork return value */
	    wait_ret,				/* wait return value */
	    form_letter = FALSE,		/* Form ltr?  */
	    form_letter_section = 0,		/* section    */
	    padding = 0,			/*   counter  */
	    builtin = FALSE,			/* our pager? */
	    val = 0,				/* return val */
	    buf_len;				/* line length */
	struct header_rec *current_header = headers[number-1];

	FILE *msgfile = mailfile;
	char clearfile[SLEN];

	lines = current_header->lines;

	dprint(4, (debugfile,"displaying %d lines from message %d using %s\n",
		lines, number, pager));

	if (number > message_count || number < 1)
	  return(val);

	if(ison(current_header->status, NEW)) {
	  clearit(current_header->status, NEW);   /* it's been read now! */
	  current_header->status_chgd = TRUE;
	}
	if(ison(current_header->status, UNREAD)) {
	  clearit(current_header->status, UNREAD);   /* it's been read now! */
	  current_header->status_chgd = TRUE;
	}

	memory_lock = FALSE;

	/* some explanation for that last one - We COULD use memory locking
	   to speed up the paging, but the action of "ClearScreen" on a screen
	   with memory lock turned on seems to vary considerably (amazingly so)
	   so it's safer to only allow memory lock to be a viable bit of
	   trickery when dumping text to the screen in scroll mode.
	   Philosophical arguments should be forwarded to Bruce at the
	   University of Walamazoo, Australia, via ACSNet  *wry chuckle* */

	if (fseek(mailfile, current_header->offset, 0) == -1) {
	  dprint(1, (debugfile,
		  "Error: seek %d bytes into file, errno %s (show_message)\n",
		  current_header->offset, error_name(errno)));
	  error2("ELM failed seeking %d bytes into file (%s).",
		  current_header->offset, error_name(errno));
	  return(val);
	}
	if(current_header->encrypted)
	  getkey(OFF);

	if (filter)
	  lines += perhaps_pgp_decode(number, clearfile);
	fseek(mailfile, current_header->offset, 0); /* again */

	if(builtin=(first_word(pager,"builtin")||first_word(pager,"internal")))

	  start_builtin(lines);

	else {

	  /* put terminal out of raw mode so external pager has normal env */
	  Raw(OFF);

#ifdef OS2
          if ( (pipe_wr_fp = popen(pager, "w")) == NULL )
            return -1;
#else
	  /* create pipe for external pager and fork */

	  if(pipe(pipe_fd) == -1) {
	    dprint(1, (debugfile, "Error: pipe failed, errno %s (show_msg)\n",
	      error_name(errno)));
	    error1("Could not prepare for external pager(pipe()-%s).",
	      error_name(errno));
	    Raw(ON);
	    return(val);
	  }

	  if((fork_ret = fork()) == -1) {

	    dprint(1, (debugfile, "Error: fork failed, errno %s (show_msg)\n",
	      error_name(errno)));
	    error1("Could not prepare for external pager(fork()-%s).",
	      error_name(errno));
	    Raw(ON);
	    return(val);

	  } else if (fork_ret == 0) {

	    /* child fork */

	    /* close write-only pipe fd and fit read-only pipe fd to stdin */

	    close(pipe_fd[1]);
	    close(fileno(stdin));
	    if((new_pipe_fd = dup(pipe_fd[0])) == -1) {
	      dprint(1, (debugfile, "Error: dup failed, errno %s (show_msg)\n",
		error_name(errno)));
	      error1("Could not prepare for external pager(dup()-%s).",
		error_name(errno));
	      _exit(errno);
	    }
	    close(pipe_fd[0]);	/* original pipe fd no longer needed */

	    /* use stdio on new pipe fd */
	    if(fdopen(new_pipe_fd, "r") == NULL) {
	      dprint(1,
		(debugfile, "Error: child fdopen failed, errno %s (show_msg)\n",
		error_name(errno)));
	      error1("Could not prepare for external pager(child fdopen()-%s).",
		error_name(errno));
	      _exit(errno);
	    }

	    /* now execute pager and exit */

	    /* system_call() will return user to user's normal permissions.
	     * This is what makes this pipe secure - user won't have elm's
	     * special setgid permissions (if so configured) and will only
	     * be able to execute a pager that user normally has permission
	     * to execute */

	    _exit(system_call(pager, SH, TRUE, TRUE));

	  } /* else this is the parent fork */

	  /* close read-only pipe fd and do write-only with stdio */
	  close(pipe_fd[0]);

	  if((pipe_wr_fp = fdopen(pipe_fd[1], "w")) == NULL) {
	    dprint(1,
	      (debugfile, "Error: parent fdopen failed, errno %s (show_msg)\n",
	      error_name(errno)));
	    error1("Could not prepare for external pager(parent fdopen()-%s).",
	      error_name(errno));

	    /* Failure - must close pipe and wait for child */
	    close(pipe_fd[1]);
	    while ((wait_ret = wait(&wait_stat)) != fork_ret && wait_ret!= -1)
	      ;

	    Raw(OFF);
	    return(val);	/* pager may have already touched the screen */
	  }
#endif

	  /* and that's it! */
	  lines_displayed = 0;
	}

	ClearScreen();

	if (cursor_control) transmit_functions(OFF);

	pipe_abort = FALSE;

	if (form_letter = (current_header->status&FORM_LETTER)) {
	  if (filter)
	    form_letter_section = 1;	/* initialize to section 1 */
	}

	if (title_messages && filter) {

	  using_to =
	    tail_of(current_header->from, who, current_header->to);

	  sprintf(title1, "%s %d/%d  ",
		    headers[current-1]->status & DELETED ? "[deleted]" :
		    form_letter ? "Form": "Message",
		    number, message_count);
	  sprintf(title2, "%s %s", using_to? "To" : "From", who);
	  sprintf(title3, "  %s %s '%d at %s %s",
     		   current_header->month, current_header->day,
	           atoi(current_header->year), current_header->time,
		   current_header->time_zone);

	  /* truncate or pad title2 portion on the right
	   * so that line fits exactly */
	  padding =
	    COLUMNS - 1 -
	    (strlen(title1) + (buf_len=strlen(title2)) + strlen(title3));

	  sprintf(titlebuf, "%s%-*.*s%s\n", title1, buf_len+padding,
	      buf_len+padding, title2, title3);

	  if (builtin)
	    display_line(titlebuf);
	  else
	    fprintf(pipe_wr_fp, "%s", titlebuf);

	  /** if there's a subject, let's output it next,
	      centered if it fits on a single line.  **/

	  if ((buf_len = strlen(current_header->subject)) > 0 &&
		matches_weedlist("Subject:")) {
	    padding = (buf_len < COLUMNS ? COLUMNS - buf_len : 0);
	    sprintf(buffer, "%*s%s\n", padding/2, "", current_header->subject);
	  } else
	    strcpy(buffer, "\n");

	  if (builtin)
	    display_line(buffer);
	  else
	    fprintf(pipe_wr_fp, "%s", buffer);

	  /** was this message address to us?  if not, then to whom? **/

	  if (! using_to && matches_weedlist("To:") && filter &&
	      strcmp(current_header->to,username) != 0 &&
	      strlen(current_header->to) > 0) {
	    if (strlen(current_header->to) > 60)
	      sprintf(buffer, "%s(message addressed to %.60s)\n",
	            strlen(current_header->subject) > 0 ? "\n" : "",
		    current_header->to);
	    else
	      sprintf(buffer, "%s(message addressed to %s)\n",
	            strlen(current_header->subject) > 0 ? "\n" : "",
		    current_header->to);
	    if (builtin)
	      display_line(buffer);
	    else
	      fprintf(pipe_wr_fp, "%s", buffer);
	  }

	  /** The test above is: if we didn't originally send the mail
	      (e.g. we're not reading "mail.sent") AND the user is currently
	      weeding out the "To:" line (otherwise they'll get it twice!)
	      AND the user is actually weeding out headers AND the message
	      wasn't addressed to the user AND the 'to' address is non-zero
	      (consider what happens when the message doesn't HAVE a "To:"
	      line...the value is NULL but it doesn't match the username
	      either.  We don't want to display something ugly like
	      "(message addressed to )" which will just clutter up the
	      screen!).

	      And you thought programming was EASY!!!!
	  **/

	  /** one more friendly thing - output a line indicating what sort
	      of status the message has (e.g. Urgent etc).  Mostly added
	      for X.400 support, this is nonetheless generally useful to
	      include...
	  **/

	  buffer[0] = '\0';

	  /* we want to flag Urgent, Confidential, Private and Expired tags */

	  if (current_header->status & PRIVATE)
	    strcpy(buffer, "\n(** This message is tagged Private");
	  else if (current_header->status & CONFIDENTIAL)
	    strcpy(buffer, "\n(** This message is tagged Company Confidential");

	  if (current_header->status & URGENT) {
	    if (buffer[0] == '\0')
	      strcpy(buffer, "\n(** This message is tagged Urgent");
	    else if (current_header->status & EXPIRED)
	      strcat(buffer, ", Urgent");
	    else
	      strcat(buffer, " and Urgent");
	  }

	  if (current_header->status & EXPIRED) {
	    if (buffer[0] == '\0')
	      strcpy(buffer, "\n(** This message has Expired");
	    else
	      strcat(buffer, ", and has Expired");
	  }

	  if (buffer[0] != '\0') {
	    strcat(buffer, " **)\n");
	    if (builtin)
	      display_line(buffer);
	    else
	      fprintf(pipe_wr_fp, buffer);
	  }

	  if (builtin)			/* this is for a one-line blank    */
	    display_line("\n");		/*   separator between the title   */
	  else				/*   stuff and the actual message  */
	    fprintf(pipe_wr_fp, "\n");	/*   we're trying to display       */

	}

	weed_header = filter;	/* allow us to change it after header */

	while (lines > 0 && pipe_abort == FALSE) {

	    if (fgets(buffer, VERY_LONG_STRING, msgfile) == NULL) {
	      
	      if (msgfile == mailfile) {
		dprint(1, (debugfile,
		  "Premature end of file! Lines left = %d msg = %s (show_msg)\n",
		  lines, number));

		error("Premature end of file!");
		sleep(2);
		break;
	      }
	      else {
		fclose(msgfile);
		unlink(clearfile);
		clearfile[0] = 0;
		msgfile = mailfile;
		continue;
	      }
	    }
	    if ((buf_len=strlen(buffer)) > 0)  {
	      if(buffer[buf_len - 1] == '\n') {
		lines--;
	        lines_displayed++;
		}
              no_ret(buffer);
	    }

  	    if (strlen(buffer) == 0) {
	      weed_header = 0;		/* past header! */
	      weeding_out = 0;
	    }

	    if (form_letter && weed_header)
		/* skip it.  NEVER display random headers in forms! */;
	    else if (weed_header && matches_weedlist(buffer))
	      weeding_out = 1;	 /* aha!  We don't want to see this! */
	    else if (buffer[0] == '[') {
	      if (strcmp(buffer, START_ENCODE)==0)
	        crypted = ON;
	      else if (strcmp(buffer, END_ENCODE)==0)
	        crypted = OFF;
	      else if (crypted) {
                encode(buffer);
	        val = show_line(buffer, builtin);
	      }
	      else if (strlen(buffer) >= 5 && strncmp(buffer, "[pgp-", 5) == 0
			&& buffer[strlen(buffer) - 1] == ']')
		/* skip */ ;
	      else
	        val = show_line(buffer, builtin);
	    }
	    else if (crypted) {
	      encode(buffer);
	      val = show_line(buffer, builtin);
	    }
	    else if (weeding_out) {
	      weeding_out = (whitespace(buffer[0]));	/* 'n' line weed */
	      if (! weeding_out) 	/* just turned on! */
	        val = show_line(buffer, builtin);
	    }
	    else if (form_letter && first_word(buffer,"***") && filter) {
	      strcpy(buffer,
"\n------------------------------------------------------------------------------\n");
	      val = show_line(buffer, builtin);	/* hide '***' */
	      form_letter_section++;
	    }
	    else if (form_letter_section == 1 || form_letter_section == 3)
	      /** skip this stuff - we can't deal with it... **/;
	    else if (strcmp(buffer, "-----BEGIN PGP MESSAGE-----") == 0
		     && filter) 
	    {
	      /* skip PGP block and insert cleartext file for it */
	      do 
	      {
		if (fgets(buffer, VERY_LONG_STRING, msgfile) == NULL)
		  break;
		if ((buf_len=strlen(buffer)) > 0)
		  no_ret(buffer);
	      } while (strcmp(buffer, "-----END PGP MESSAGE-----") != 0);
	      if (strlen(clearfile) > 0)
		msgfile = fopen(clearfile, "r");
	    }
	    else
	      val = show_line(buffer, builtin);

	    if (val != 0)	/* discontinue the display */
	      break;
	}

        if (cursor_control) transmit_functions(ON);

	if (msgfile != mailfile) {
	  fclose(msgfile);
	  unlink(clearfile);
	}

	if (!builtin) {
#ifdef OS2
	  wait_stat = pclose(pipe_wr_fp);
	  val = WIFEXITED(wait_stat) ? WEXITSTATUS(wait_stat) : 0;
#else
	  fclose(pipe_wr_fp);
	  while ((wait_ret = wait(&wait_stat)) != fork_ret
		  && wait_ret!= -1)
	    ;
#endif
	  /* turn raw on **after** child terminates in case child
	   * doesn't put us back to cooked mode after we return ourselves
	   * to raw.
	   */
	  Raw(ON);
	  EndBold();
	  ClearScreen();
	}

	/* If we are to prompt for a user input command and we don't
	 * already have one */
	if ((prompt_after_pager || builtin) && val == 0) {
	  MoveCursor(LINES,0);
	  StartBold();
	  Write_to_screen(" Command ('i' to return to index): ", 0);
	  EndBold();
	  fflush(stdout);
	  val = ReadCh();
	}

	if (memory_lock) EndMemlock();	/* turn it off!! */

	/* 'q' means quit current operation and pop back up to previous level -
	 * in this case it therefore means return to index screen.
	 */
	return(val == 'i' || val == 'q' ? 0 : val);
}

int
show_line(buffer, builtin)
char *buffer;
int  builtin;
{
	/** Hands the given line to the output pipe.  'builtin' is true if
	    we're using the builtin pager.
	    Return the character entered by the user to indicate
	    a command other than continuing with the display (only possible
	    with the builtin pager), otherwise 0. **/

	strcat(buffer, "\n");
#ifdef MMDF
	if (strcmp(buffer, MSG_SEPERATOR) == 0)
	  return(0);	/* no reason to show the ending terminator */
#endif /* MMDF */
	if (builtin) {
	  return(display_line(buffer));
	}
	errno = 0;
	fprintf(pipe_wr_fp, "%s", buffer);
	if (errno != 0)
	  dprint(1, (debugfile, "\terror %s hit!\n", error_name(errno)));
	return(0);
}


perhaps_pgp_decode(number, filename)
int number;
char *filename;
{
  char buffer[VERY_LONG_STRING], cryptname[SLEN], clearname[SLEN];
  int lines = headers[number-1] -> lines;
  int oldlines = 0, pgp = 0, sign = 0;
  FILE *tempfile;

  filename[0] = 0;
  sprintf(cryptname, "%s%d%s", temp_dir, getpid(), temp_file);
  sprintf(clearname, "%s%d.msg", temp_dir, getpid());
      
  if ((tempfile = fopen(cryptname, "w")) == NULL) 
  {
    if(batch_only)
      printf("Could not create file %s (%s).\n", cryptname, error_name(errno));
    else
      error2("Could not create file %s (%s).", cryptname, error_name(errno));
    return 0;
  }

  while (lines--) 
  {
    if (fgets(buffer, VERY_LONG_STRING, mailfile) == NULL)
      return 0;

    no_ret(buffer);

    if (pgp)
      oldlines++;

    if (strcmp(buffer, "-----BEGIN PGP MESSAGE-----") == 0)
      pgp = 1;
    else if (strcmp(buffer, "-----BEGIN PGP SIGNED MESSAGE-----") == 0)
      pgp = sign = 1;

    if (pgp)
      fprintf(tempfile, "%s\n", buffer);

    if (strcmp(buffer, "-----END PGP MESSAGE-----") == 0)
      pgp = 0;
    else if (strcmp(buffer, "-----END PGP SIGNATURE-----") == 0)
      pgp = 0;
  }

  fclose(tempfile);

  if (oldlines == 0)
  {
    unlink(cryptname);
    return 0;
  }

  if (sign)
    strcpy(clearname, "nul");

  sprintf(buffer, "pgp -f <%s >%s", cryptname, clearname);
  os2path(buffer);
  puts("\r\n");

  if (system_call(buffer, SH, FALSE, FALSE)) 
  {
    if (batch_only)
      printf("Error while decrypting. Try again.\n");
    else
      error2("Error while decrypting. Try again.");
    return 0;
  }

  if (sign)
  {
    puts("");
    PutLine0(LINES,0,"Please Press any key to continue.");
    (void) ReadCh();
    return 0;
  }

  if ((tempfile = fopen(clearname, "r")) == NULL) 
  {
    if(batch_only)
      printf("Could not read file %s (%s).\n", clearname, error_name(errno));
    else
      error2("Could not read file %s (%s).", clearname, error_name(errno));
    return 0;
  }

  for (lines = 0; fgets(buffer, VERY_LONG_STRING, tempfile) != NULL; lines++);
  fclose(tempfile);

  strcpy(filename, clearname);
  return lines - oldlines;
}
