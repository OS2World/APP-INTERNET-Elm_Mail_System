
static char rcsid[] = "@(#)$Id: editmsg.c,v 4.1.1.4 91/01/07 20:36:26 syd Exp $";

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
 * $Log:	editmsg.c,v $
 * Revision 4.1.1.4  91/01/07  20:36:26  syd
 * Fix warning message on compiling editmsg on voidsig machines
 * From: Chip Salzenberg
 *
 * Revision 4.1.1.3  90/07/12  22:43:08  syd
 * Make it aware of the fact that we loose the cursor position on
 * some system calls, so set it far enough off an absolute move will
 * be done on the next cursor address, and then place it where we want it.
 * From: Syd, reported by Douglas Lamb
 *
 * Revision 4.1.1.2  90/06/21  21:14:09  syd
 * Force Carriage return on return from editor, as column is lost
 * From: Steve Cambell
 *
 * Revision 4.1.1.1  90/06/09  23:33:06  syd
 * Only say cannot invoke on -1 error which is cannot do exec in system call
 * From: Syd
 *
 * Revision 4.1  90/04/28  22:42:47  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This contains routines to do with starting up and using an editor (or two)
    from within Elm.  This stuff used to be in mailmsg2.c...
**/

#include "headers.h"
#include <errno.h>
#ifndef BSD
/* BSD has already included setjmp.h in headers.h */
#include <setjmp.h>
#endif /* BSD */
#include <signal.h>
#include <ctype.h>

#ifdef BSD
#undef        tolower
#endif

#ifndef OS2
extern int errno;
#endif

char *error_name(), *error_description(), *strcpy(), *format_long();
unsigned long sleep();

int
edit_the_message(filename, already_has_text)
char *filename;
int  already_has_text;
{
	/** Invoke the users editor on the filename.  Return when done.
	    If 'already has text' then we can't use the no-editor option
	    and must use 'alternative editor' (e.g. $EDITOR or default_editor)
	    instead... **/

        char buffer[SLEN];
	register int stat, return_value = 0, old_raw;

	buffer[0] = '\0';

	if (strcmp(editor, "builtin") == 0 || strcmp(editor, "none") == 0) {
	  if (already_has_text)
	    sprintf(buffer, "%s %s", alternative_editor, filename);
	  else
	    return(no_editor_edit_the_message(filename));
	}

	PutLine0(LINES, 0, "Invoking editor...");  fflush(stdout);
        if (strlen(buffer) == 0)
          sprintf(buffer,"%s %s", editor, filename);

	os2path(buffer);
	chown(filename, userid, groupid);	/* file was owned by root! */

	if (( old_raw = RawState()) == ON)
	  Raw(OFF);

	if (cursor_control)
	  transmit_functions(OFF);		/* function keys are local */

	if ((stat = system_call(buffer, SH, TRUE, FALSE)) == -1) {
	  dprint(1,(debugfile,
		  "System call failed with stat %d (edit_the_message)\n",
		  stat));
	  dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
		error_description(errno)));
	  ClearLine(LINES-1);
	  error1("Can't invoke editor '%s' for composition.", editor);
	  sleep(2);
	  return_value = 1;
	}

	if (old_raw == ON)
	   Raw(ON);

        ClearScreen();
	SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
	MoveCursor(LINES, 0);	/* dont know where we are, force last row, col 0 */

	if (cursor_control)
	  transmit_functions(ON);		/* function keys are local */

	return(return_value);
}

static char simple_continue[] = "(Continue.)\n\r";
static char post_ed_continue[] =
"(Continue entering message.  Type ^D or '.' on a line by itself to end.)\n\r";

extern char to[VERY_LONG_STRING], cc[VERY_LONG_STRING],
	    expanded_to[VERY_LONG_STRING], expanded_cc[VERY_LONG_STRING],
	    bcc[VERY_LONG_STRING], expanded_bcc[VERY_LONG_STRING],
            subject[SLEN];

int      interrupts_while_editing;	/* keep track 'o dis stuff         */
jmp_buf  edit_location;		        /* for getting back from interrupt */

char *strip_commas();
long  fsize();

int
no_editor_edit_the_message(filename)
char *filename;
{
	/** If the current editor is set to either "builtin" or "none", then
	    invoke this program instead.  As it turns out, this and the
	    routine above have a pretty incestuous relationship (!)...
	**/

	FILE *edit_fd;
	char buffer[SLEN], editor_name[SLEN], buf[SLEN];
	int      old_raw;
#ifdef VOIDSIG
	void	edit_interrupt();
	void	(*oldint)(), (*oldquit)();
#else
	int	edit_interrupt();
	int	(*oldint)(), (*oldquit)();
#endif

	if ((edit_fd = fopen(filename, "a")) == NULL) {
	  error2("Couldn't open %s for appending [%s].", filename,
		  error_name(errno));
	  sleep (2);
	  dprint(1, (debugfile,
		"Error encountered trying to open file %s;\n", filename));
	  dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
		    error_description(errno)));
	  return(1);
	}

	/** is there already text in this file? **/

	if (fsize(edit_fd) > 0L)
	  strcpy(buf, "\n\rContinue entering message.");
	else
	  strcpy(buf, "\n\rEnter message.");
	strcat(buf, "  Type Elm commands on lines by themselves.\n\r");
	sprintf(buf + strlen(buf),
    "Commands include:  ^D or '.' to end, %cp to list, %c? for help.\n\r\n\r",
		escape_char, escape_char);
	CleartoEOS();
	Write_to_screen(buf, 0);

	oldint  = signal(SIGINT,  edit_interrupt);
	oldquit = signal(SIGQUIT, edit_interrupt);

	interrupts_while_editing = 0;

	if (setjmp(edit_location) != 0) {
	  if (interrupts_while_editing > 1) {

	    (void) signal(SIGINT,  oldint);
	    (void) signal(SIGQUIT, oldquit);

	    if (edit_fd != NULL)	/* insurance... */
	      fclose(edit_fd);
	    return(1);
	  }
	  goto more_input;	/* read input again, please! */
	}

more_input: buffer[0] = '\0';

	while (optionally_enter(buffer, -1,-1, FALSE, FALSE) == 0) {

	  interrupts_while_editing = 0;	/* reset to zero... */

	  if (strcmp(buffer, ".") == 0)
	    break;	/* '.' is as good as a ^D to us dumb programs :-) */
	  if (buffer[0] == escape_char) {
	    switch (tolower(buffer[1])) {
              case '?' : tilde_help();
			 goto more_input;

	      case TILDE_ESCAPE: move_left(buffer, 1);
			  	 goto tilde_input;	/*!!*/

	      case 't' : get_with_expansion("\n\rTo: ",
	      		   to, expanded_to, buffer);
			 goto more_input;
	      case 'b' : get_with_expansion("\n\rBcc: ",
			   bcc, expanded_bcc, buffer);
			 goto more_input;
	      case 'c' : get_with_expansion("\n\rCc: ",
			   cc, expanded_cc, buffer);
			 goto more_input;
	      case 's' : get_with_expansion("\n\rSubject: ",
			   subject,NULL,buffer);
   			 goto more_input;

	      case 'h' : get_with_expansion("\n\rTo: ", to, expanded_to, NULL);
	                 get_with_expansion("Cc: ", cc, expanded_cc, NULL);
	                 get_with_expansion("Bcc: ", bcc,expanded_bcc, NULL);
	                 get_with_expansion("Subject: ", subject, NULL, NULL);
			 goto more_input;

	      case 'r' : read_in_file(edit_fd, (char *) buffer + 2, 1);
		  	 goto more_input;
	      case 'e' : if (strlen(emacs_editor) > 0)
	                   if (access(emacs_editor, ACCESS_EXISTS) == 0) {
	                     strcpy(buffer, editor);
			     strcpy(editor, emacs_editor);
	  		     fclose(edit_fd);
			     (void) edit_the_message(filename,0);
			     strcpy(editor, buffer);
			     edit_fd = fopen(filename, "a");
                             Write_to_screen(post_ed_continue, 0);
	                     goto more_input;
		           }
		           else
	                     Write_to_screen(
			"\n\r(Can't find Emacs on this system! Continue.)\n\r",
			0);
			 else
			   Write_to_screen(
			"\n\r(Don't know where Emacs would be. Continue.)\n\r",
			0);
			 goto more_input;

	       case 'v' : NewLine();
			  strcpy(buffer, editor);
			  strcpy(editor, default_editor);
			  fclose(edit_fd);
			  (void) edit_the_message(filename,0);
			  strcpy(editor, buffer);
			  edit_fd = fopen(filename, "a");
                          Write_to_screen(post_ed_continue, 0);
	                  goto more_input;

	       case 'o' : Write_to_screen(
			     "\n\rPlease enter the name of the editor: ", 0);
			  editor_name[0] = '\0';
			  optionally_enter(editor_name,-1,-1, FALSE, FALSE);
			  NewLine();
	                  if (strlen(editor_name) > 0) {
	                    strcpy(buffer, editor);
			    strcpy(editor, editor_name);
			    fclose(edit_fd);
			    (void) edit_the_message(filename,0);
			    strcpy(editor, buffer);
			    edit_fd = fopen(filename, "a");
                            Write_to_screen(post_ed_continue, 0);
	                    goto more_input;
		          }
	  		  Write_to_screen(simple_continue, 0);
	                  goto more_input;

		case '<' : NewLine();
			   if (strlen(buffer) < 3)
			     Write_to_screen(
		 "(You need to use a specific command here. Continue.)\n\r");
			   else {
			     sprintf(buf, " > %s%d.%s", temp_dir, getpid(), temp_edit);
			     strcat(buffer, buf);
			     if (( old_raw = RawState()) == ON)
			       Raw(OFF);
			     (void) system_call((char *) buffer+2, SH, TRUE, TRUE);
			     if (old_raw == ON)
			        Raw(ON);
			     sprintf(buffer, "~r %s%d.%s", temp_dir, getpid(), temp_edit);
	      	             read_in_file(edit_fd, (char *) buffer + 3, 0);
			     (void) unlink((char *) buffer+3);
			     SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
			     MoveCursor(LINES, 0);	/* and go to a known location, last row col 0 */
			   }
	                   goto more_input;

		case '!' : NewLine();
			   if (( old_raw = RawState()) == ON)
			     Raw(OFF);
			   if (strlen(buffer) < 3)
			     (void) system_call(shell, USER_SHELL, TRUE, TRUE);
			   else
	                     (void) system_call((char *) buffer+2, USER_SHELL, TRUE, TRUE);
			   if (old_raw == ON)
			      Raw(ON);
			   SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */
			   MoveCursor(LINES, 0);	/* and go to a known location, last row col 0 */
	    		   Write_to_screen(simple_continue, 0);
			   goto more_input;

		 case 'm' : /* same as 'f' but with leading prefix added */

	         case 'f' : /* this can be directly translated into a
			       'readmsg' call with the same params! */
			    NewLine();
			    read_in_messages(edit_fd, (char *) buffer + 1);
			    goto more_input;

	         case 'p' : /* print out message so far.  Soooo simple! */
			    print_message_so_far(edit_fd, filename);
			    goto more_input;

		 default  : sprintf(buf,
			 "\n\r(Don't know what %c%c is. Try %c? for help.)\n\r",
				    escape_char, buffer[1], escape_char);
			    Write_to_screen(buf, 0);
	       }
	     }
	     else {
tilde_input:
	       fprintf(edit_fd, "%s\n", buffer);
	       NewLine();
	     }
	  buffer[0] = '\0';
	}


	Write_to_screen("\n\r<end-of-message>\n\r\n\r\n\r\n\r", 0);

	(void) signal(SIGINT,  oldint);
	(void) signal(SIGQUIT, oldquit);

	if (edit_fd != NULL)	/* insurance... */
	  fclose(edit_fd);

	return(0);
}

tilde_help()
{
	/* a simple routine to print out what is available at this level */

	char	buf[SLEN];

	Write_to_screen("\n\rAvailable options at this point are:\n\r\n\r", 0);
	sprintf(buf, "\t%c?\tPrint this help menu.\n\r", escape_char);
	Write_to_screen(buf, 0);
	if (escape_char == TILDE_ESCAPE) /* doesn't make sense otherwise... */
	  Write_to_screen(
	      "\t~~\tAdd line prefixed by a single '~' character.\n\r", 0);

	sprintf(buf,
	  "\t%cb\tChange the addresses in the Blind-carbon-copy list.\n\r",
	  escape_char);
	Write_to_screen(buf, 0);

	sprintf(buf,
		"\t%cc\tChange the addresses in the Carbon-copy list.\n\r",
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
	      "\t%ce\tInvoke the Emacs editor on the message, if possible.\n\r",
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
		"\t%cf\tAdd the specified message or current.\n\r",
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
	      "\t%ch\tChange all available headers (to, cc, bcc, subject).\n\r",
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
		"\t%cm\tSame as '%cf', but with the current 'prefix'.\n\r",
		escape_char, escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
		"\t%co\tInvoke a user specified editor on the message.\n\r",
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
	      "\t%cp\tPrint out message as typed in so far.\n\r", escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
		"\t%cr\tRead in the specified file.\n\r", escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
		"\t%cs\tChange the subject of the message.\n\r", escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
		"\t%ct\tChange the addresses in the To list.\n\r",
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
		"\t%cv\tInvoke the Vi visual editor on the message.\n\r",
		escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
          "\t%c!\tExecute a unix command (or give a shell if no command).\n\r",
	  escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
      "\t%c<\tExecute a unix command adding the output to the message.\n\r",
	  escape_char);
	Write_to_screen(buf, 0);
	sprintf(buf,
      "\t.  \tby itself on a line (or a control-D) ends the message.\n\r");
	Write_to_screen(buf, 0);
	Write_to_screen("(Continue.)\n\r", 0);
}

read_in_file(fd, filename, show_user_filename)
FILE *fd;
char *filename;
int   show_user_filename;
{
	/** Open the specified file and stream it in to the already opened
	    file descriptor given to us.  When we're done output the number
	    of lines and characters we added, if any... **/

	FILE *myfd;
	char myfname[SLEN], buffer[SLEN];
	register int n;
	register int lines = 0, nchars = 0;

	for ( n = 0 ; whitespace(filename[n]) ; n++ );

	/** expand any shell variables, '~' or other notation... **/
	/* temp copy of filename to buffer since expand_env is destructive */
	strcpy(buffer, &filename[n]);
	expand_env(myfname, buffer);

	if (strlen(myfname) == 0) {
	  Write_to_screen(
	      "\n\r(No filename specified for file read! Continue.)\n\r", 0);
	  return;
	}

	if ((myfd = fopen(myfname,"r")) == NULL) {
	  Write_to_screen("\n\r(Couldn't read file '%s'! Continue.)\n\r", 1,
		 myfname);
	  return;
	}

	while (fgets(buffer, SLEN, myfd) != NULL) {
	  n = strlen(buffer);
	  if(buffer[n-1] == '\n') lines++;
	  nchars += n;
  	  fputs(buffer, fd);
	  fflush(stdout);
	}

	fclose(myfd);

	if (show_user_filename)
	  sprintf(buffer,
		"\n\r(Added %d line%s [%d char%s] from file %s. Continue.)\n\r",
		lines, plural(lines), nchars, plural(nchars), myfname);
	else
	  sprintf(buffer,
		"\n\r(Added %d line%s [%d char%s] to message. Continue.)\n\r",
		lines, plural(lines), nchars, plural(nchars));
	Write_to_screen(buffer, 0);

	return;
}

print_message_so_far(edit_fd, filename)
FILE *edit_fd;
char *filename;
{
	/** This prints out the message typed in so far.  We accomplish
	    this in a cheap manner - close the file, reopen it for reading,
	    stream it to the screen, then close the file, and reopen it
	    for appending.  Simple, but effective!

	    A nice enhancement would be for this to -> page <- the message
	    if it's sufficiently long.  Too much work for now, though.
	**/

	char buffer[SLEN];

	fclose(edit_fd);

	if ((edit_fd = fopen(filename, "r")) == NULL) {
	  Write_to_screen("\n\rMayday!  Mayday!  Mayday!\n\r", 0);
	  Write_to_screen("\n\rPanic: Can't open file for reading!  Bail!\n\r",
	      0);
	  emergency_exit();
	}

	Write_to_screen("\n\rTo: %s\n\r", 1, format_long(to, 4));
	Write_to_screen("Cc: %s\n\r", 1, format_long(cc, 4));
	Write_to_screen("Bcc: %s\n\r", 1, format_long(bcc, 5));
	Write_to_screen("Subject: %s\n\r\n\r", 1, subject);

	while (fgets(buffer, SLEN, edit_fd) != NULL) {
	  Write_to_screen(buffer, 0);
	  CarriageReturn();
        }

	fclose(edit_fd);

	if ((edit_fd = fopen(filename, "a")) == NULL) {
	  Write_to_screen("Mayday!  Mayday!  Abandon Ship!  Aiiieeeeee\n\r", 0);
	  Write_to_screen("\n\rPanic: Can't reopen file for appending!\n\r", 0);
	  emergency_exit();
	}

	Write_to_screen("\n\r(Continue entering message.)\n\r", 0);
}

read_in_messages(fd, buffer)
FILE *fd;
char *buffer;
{
	/** Read the specified messages into the open file.  If the
	    first character of "buffer" is 'm' then prefix it, other-
	    wise just stream it in straight...Since we're using the
	    pipe to 'readmsg' we can also allow the user to specify
	    patterns and such too...
	**/

	FILE *myfd, *popen();
	char  local_buffer[SLEN], *arg;
	register int add_prefix=0, mindex;
	register int n;
	int lines = 0, nchars = 0;

	add_prefix = tolower(buffer[0]) == 'm';

	/* strip whitespace to get argument */
	for(arg = &buffer[1]; whitespace(*arg); arg++)
		;

	/* if no argument or begins with a digit, then retrieve the
	 * appropriate message from the current folder, else
	 * just take the arguments as a pattern for readmsg to match in
	 * the current folder.
	 */
	if(isdigit(*arg) || *arg == '\0') {
	  if(message_count < 1) {
	    Write_to_screen("(No messages to read in! Continue.)\n\r", 0);
	    return;
	  }
	  if((mindex = atoi(arg)) == 0)	/* no argument - read in current msg */
	    mindex = current;
	  else if(mindex < 1 || mindex > message_count) {
	    sprintf(local_buffer,
	      "(Valid messsage numbers are between 1 and %d. Continue.)\n\r",
	      message_count);
	    Write_to_screen(local_buffer, 0);
	    return;
	  }

	  sprintf(local_buffer, "%s -f %s %d", readmsg, cur_folder,
		  headers[mindex-1]->index_number);

	} else
	  sprintf(local_buffer, "%s -f %s %s", readmsg, cur_folder, arg);


	/* now get output of readmsg */
	if ((myfd = popen(local_buffer, "r")) == NULL) {
	   Write_to_screen("(Can't find 'readmsg' command! Continue.)\n\r",
	       0);
	   return;
	}

	dprint(5, (debugfile, "** readmsg call: \"%s\" **\n", local_buffer));

	while (fgets(local_buffer, SLEN, myfd) != NULL) {
	  n = strlen(local_buffer);
	  nchars += n;
	  if (local_buffer[n-1] == '\n') lines++;
	  if (add_prefix)
	    fprintf(fd, "%s%s", prefixchars, local_buffer);
	  else
	    fputs(local_buffer, fd);
	}

	pclose(myfd);

	if (lines == 0)
	  sprintf(local_buffer,
	 	 "(Couldn't add the requested message. Continue.)\n\r");
	else
	  sprintf(local_buffer,
		"(Added %d line%s [%d char%s] to message. Continue.)\n\r",
		lines, plural(lines), nchars, plural(nchars));
	Write_to_screen(local_buffer, 0);

	return;
}

get_with_expansion(prompt, buffer, expanded_buffer, sourcebuf)
char *prompt, *buffer, *expanded_buffer, *sourcebuf;
{
	/** This is used to prompt for a new value of the specified field.
	    If expanded_buffer == NULL then we won't bother trying to expand
	    this puppy out!  (sourcebuf could be an initial addition)
	**/

	Write_to_screen(prompt, 0);	fflush(stdout);	/* output! */

	if (sourcebuf != NULL) {
	  while (!whitespace(*sourcebuf) && *sourcebuf != '\0')
	    sourcebuf++;
	  if (*sourcebuf != '\0') {
	    while (whitespace(*sourcebuf))
	      sourcebuf++;
	    if (strlen(sourcebuf) > 0) {
	      strcat(buffer, " ");
	      strcat(buffer, sourcebuf);
	    }
	  }
	}

	optionally_enter(buffer, -1, -1, TRUE, FALSE);	/* already data! */

	if(expanded_buffer != NULL) {
	  build_address(strip_commas(buffer), expanded_buffer);
	  if(*expanded_buffer != '\0') {
	    if (*prompt == '\n')
	      Write_to_screen("%s%s", 2, prompt, expanded_buffer);
	    else
	      Write_to_screen("\n\r%s%s", 2, prompt, expanded_buffer);
	  }
	}
	NewLine();

	return;
}

#ifdef VOIDSIG
void
#else
int
#endif
edit_interrupt()
{
	/** This routine is called when the user hits an interrupt key
	    while in the builtin editor...it increments the number of
	    times an interrupt is hit and returns it.
	**/

	signal(SIGINT, edit_interrupt);
	signal(SIGQUIT, edit_interrupt);

	if (interrupts_while_editing++ == 0)
	  Write_to_screen("(Interrupt. One more to cancel this letter.)\n\r",
	  	0);
	else
	  Write_to_screen("(Interrupt. Letter cancelled.)\n\r", 0);

	longjmp(edit_location, 1);		/* get back */
}
