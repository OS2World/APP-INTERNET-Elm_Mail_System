
static char rcsid[] = "@(#)$Id: edit.c,v 4.1.1.2 90/10/07 21:02:42 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.2 $   $State: Exp $
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
 * $Log:	edit.c,v $
 * Revision 4.1.1.2  90/10/07  21:02:42  syd
 * Fix EB29 using alternate editor all the time
 * From: Michael Clay
 *
 * Revision 4.1.1.1  90/07/12  22:43:05  syd
 * Make it aware of the fact that we loose the cursor position on
 * some system calls, so set it far enough off an absolute move will
 * be done on the next cursor address, and then place it where we want it.
 * From: Syd, reported by Douglas Lamb
 *
 * Revision 4.1  90/04/28  22:42:46  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This routine is for allowing the user to edit their current folder
    as they wish.

**/

#include "headers.h"
#include <errno.h>

#ifndef OS2
extern int errno;
#endif

char   *error_name(), *error_description(), *strcpy();
long   bytes();
unsigned long sleep();

#ifdef ALLOW_MAILBOX_EDITING

edit_mailbox()
{
	/** Allow the user to edit their folder, always resynchronizing
	    afterwards.   Due to intense laziness on the part of the
	    programmer, this routine will invoke $EDITOR on the entire
	    file.  The mailer will ALWAYS resync on the folder
	    even if nothing has changed since, not unreasonably, it's
	    hard to figure out what occurred in the edit session...

	    Also note that if the user wants to edit their incoming
	    mailbox they'll actually be editing the tempfile that is
	    an exact copy.  More on how we resync in that case later
	    in this code.
	**/

	FILE     *real_folder, *temp_folder;
        char     edited_file[SLEN], buffer[SLEN];

	if(folder_type == SPOOL) {
	  if(save_file_stats(cur_folder) != 0) {
	    error1("Problems saving permissions of folder %s!", cur_folder);
	    Raw(ON);
	    sleep(2);
	    return(0);
	  }
	}

	PutLine0(LINES-1,0,"Invoking editor...");

	strcpy(edited_file, (folder_type == NON_SPOOL ? cur_folder : cur_tempfolder));
	if (strcmp(editor, "builtin") == 0 || strcmp(editor, "none") == 0)
	  sprintf(buffer, "%s %s", alternative_editor, edited_file);
        else
          sprintf(buffer, "%s %s", editor, edited_file);

	Raw(OFF);

	fclose(mailfile);
	mailfile = NULL;

        os2path(buffer);
        
	if (system_call(buffer, SH, TRUE, FALSE) != 0) {
	  error1("Problems invoking editor %s!", alternative_editor);
	  Raw(ON);
	  sleep(2);
	  return(0);
	}

	Raw(ON);
        ClearScreen();
	SetXYLocation(0, 40);	/* a location not near the next request, so an absolute is used */

	if (folder_type == SPOOL) {	/* uh oh... now the toughie...  */

	  if (bytes(cur_folder) != mailfile_size) {

	     /* SIGH.  We've received mail since we invoked the editor
		on the folder.  We'll have to do some strange stuff to
	        remedy the problem... */

	     PutLine0(LINES, 0, "Warning: new mail received...");
	     CleartoEOLN();

	     if ((temp_folder = fopen(edited_file, "a")) == NULL) {
	       dprint(1, (debugfile,
		    "Attempt to open \"%s\" to append failed in %s\n",
		    edited_file, "edit_mailbox"));
	       set_error("Couldn't reopen tempfile. Edit LOST!");
	       return(1);
	     }
	     /** Now let's lock the folder up and stream the new stuff
		 into the temp file... **/

	     lock(OUTGOING);
	     if ((real_folder = fopen(cur_folder, "r")) == NULL) {
	       dprint(1, (debugfile,
	           "Attempt to open \"%s\" for reading new mail failed in %s\n",
 		   cur_folder, "edit_mailbox"));
	       sprintf(buffer, "Couldn't open %s for reading!  Edit LOST!",
		       cur_folder);
	       set_error(buffer);
	       unlock();
	       return(1);
	     }
	     if (fseek(real_folder, mailfile_size, 0) == -1) {
	       dprint(1, (debugfile,
			"Couldn't seek to end of cur_folder (offset %ld) (%s)\n",
			mailfile_size, "edit_mailbox"));
	       set_error("Couldn't seek to end of folder.  Edit LOST!");
	       unlock();
	       return(1);
	     }

	     /** Now we can finally stream the new mail into the tempfile **/

	     while (fgets(buffer, SLEN, real_folder) != NULL)
	       fprintf(temp_folder, "%s", buffer);

	     fclose(real_folder);
	     fclose(temp_folder);

 	   } else lock(OUTGOING);

	   /* remove real mail_file and then
	    * link or copy the edited mailfile to real mail_file */

	   (void)unlink(cur_folder);

	   if (link(edited_file, cur_folder) != 0)  {
	     if (errno == EXDEV || errno == EEXIST) {
	       /* attempt to link across file systems */
   	       if (copy(edited_file, cur_folder) != 0) {
		 Write_to_screen(
		    "\n\rCouldn't copy %s to mailfile %s!\n\r",
		    2, edited_file, cur_folder);
		 Write_to_screen(
		    "\n\rYou'll need to check out %s for your mail.\n\r",
		    1, edited_file);
		 Write_to_screen("** %s - %s. **\n\r", 2,
		    error_name(errno), error_description(errno));
		 unlock();					/* ciao!*/
		 emergency_exit();
	       }
	     } else {
		Write_to_screen("\n\rCouldn't link %s to mailfile %s!\n\r",2,
		  edited_file, cur_folder);
	        Write_to_screen(
		  "\n\rYou'll need to check out %s for your mail.\n\r",
		  1, edited_file);
		Write_to_screen("** %s - %s. **\n\r", 2,
		  error_name(errno), error_description(errno));
	        unlock();					/* ciao!*/
	        emergency_exit();
	     }
	   }

	   /* restore file permissions before removing lock */

	   if(restore_file_stats(cur_folder) != 1) {
	     error1("Problems restoring permissions of folder %s!", cur_folder);
	     Raw(ON);
	     sleep(2);
	   }

	   unlock();
	   unlink(edited_file);	/* remove the edited mailfile */
	   error("Changes incorporated into new mail...");

	} else
	  error("Resynchronizing with new version of folder...");

	/* sleep(2); */
	ClearScreen();
	newmbox(cur_folder, FALSE);
	showscreen();
	return(1);
}

#endif
