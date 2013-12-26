
static char rcsid[] = "@(#)$Id: quit.c,v 4.1 90/04/28 22:43:44 syd Exp $";

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
 * $Log:	quit.c,v $
 * Revision 4.1  90/04/28  22:43:44  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** quit: leave the current folder and quit the program.

**/

#include "headers.h"
#include <errno.h>

#ifndef OS2
extern int errno;		/* system error number on failure */
#endif

long bytes();

quit(prompt)
int prompt;
{
	/* a wonderfully short routine!! */

	if (leave_mbox(FALSE, TRUE, prompt) == -1)
	  /* new mail - leave not done - can't change to another file yet
	   * check for change in mailfile_size in main() will do the work
	   * of calling newmbox to add in the new messages to the current
	   * file and fix the sorting sequence that leave_mbox may have
	   * changed for its own purposes */
	  return;

	leave();
}

int
resync()
{
	/** Resync on the current folder. Leave current and read it back in.
	    Return indicates whether a redraw of the screen is needed.
	 **/

	  if(leave_mbox(TRUE, FALSE, TRUE) ==-1)
	    /* new mail - leave not done - can't change to another file yet
	     * check for change in mailfile_size in main() will do the work
	     * of calling newmbox to add in the new messages to the current
	     * file and fix the sorting sequence that leave_mbox may have
	     * changed for its own purposes */
	    return(FALSE);

	  if ((errno = can_access(cur_folder, READ_ACCESS)) != 0) {
	    dprint(1, (debugfile,
		  "Error: given file %s as folder - unreadable (%s)!\n",
		  cur_folder, error_name(errno)));
	    fprintf(stderr,"Can't open folder '%s' for reading!\n", cur_folder);
	    leave();
	    }

	  newmbox(cur_folder, FALSE);
	  return(TRUE);
}

change_file()
{
	  /* Prompt user for name of folder to change to.
	   * If all okay with that folder, leave the current folder.
	   * If leave goes okay (i.e. no new messages in current folder),
	   * change to the folder that the user specified.
	   *
	   * Return value indicates whether a redraw is needed.
	   */

	  int redraw = FALSE;
	  char newfile[SLEN];
	  static char helpmsg[LONG_STRING];

	  char	*nameof();


	  /* get new file name */

	  MoveCursor(LINES-3, 30);
	  CleartoEOS();
	  PutLine0(LINES-3, 38, "(Use '?' for help/to list your folders.)");
	  PutLine0(LINES-2,0,"Change to which folder: ");
	  while(1) {
	    newfile[0] = '\0';
	    (void) optionally_enter(newfile, LINES-2, 24, FALSE, FALSE);
	    clear_error();

	    if(*newfile == '\0') {	/* if user didn't enter a file name */
	      MoveCursor(LINES-3, 30);	/* abort changing file process */
	      CleartoEOS();
	      return(redraw);

	    } else if (strcmp(newfile, "?") == 0) {

	      /* user wants to list folders */
	      if(!*helpmsg) {	/* format helpmsg if not yet done */

		strcpy(helpmsg,
		  "\n\r\n\rEnter: <nothing> to not change to a new folder,");
		strcat(helpmsg,
		  "\n\r       '!' to change to your incoming mailbox (");
		strcat(helpmsg, defaultfile);
		strcat(helpmsg,
		  ")\n\r       '>' to change to your \"received\" folder (");
		strcat(helpmsg, nameof(recvd_mail));
		strcat(helpmsg,
		  ")\n\r       '<' to change to your \"sent\" folder (");
		strcat(helpmsg, nameof(sent_mail));
		strcat(helpmsg, ")\n\r       or a filename");
		strcat(helpmsg,
		  " (leading '=' denotes your folder directory ");
		strcat(helpmsg, folders);
		strcat(helpmsg, ").\n\r");
	      }
	      list_folders(4, helpmsg);
	      PutLine0(LINES-2,0,"Change to which folder: ");	/* reprompt */
	      redraw = TRUE;		/* we'll need to clean the screen */

	    } else {

	      /* user entered a file name - expand it */
	      if (! expand_filename(newfile, TRUE))
		continue;	/* prompt again */

	      /* don't accept the same file as the current */
	      if (strcmp(newfile, cur_folder) == 0) {
		error("Already reading that folder!");
		continue;	/* prompt again */
	      }

	      /* Make sure this is a file the user can open, unless it's the
	       * default mailfile, which is openable even if empty */
	      if ((errno = can_access(newfile, READ_ACCESS)) != 0 ) {
		if (strcmp(newfile, defaultfile) != 0 || errno != ENOENT) {
		  error1("Can't open folder '%s' for reading!", newfile);
		  continue; 	/* prompt again */
		}
	      }
	      break;	/* exit loop - we got the name of a good file */
	    }
	  }

	  /* All's clear with the new file to go ahead and leave the current. */
	  MoveCursor(LINES-3, 30);
	  CleartoEOS();

	  if(leave_mbox(FALSE, FALSE, TRUE) ==-1) {
	    /* new mail - leave not done - can't change to another file yet
	     * check for change in mailfile_size in main() will do the work
	     * of calling newmbox to add in the new messages to the current
	     * file and fix the sorting sequence that leave_mbox may have
	     * changed for its own purposes */
	    return(redraw);
	  }

	  redraw = 1;
	  newmbox(newfile, FALSE);
	  return(redraw);
}
