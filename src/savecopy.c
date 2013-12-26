
static char rcsid[] = "@(#)$Id: savecopy.c,v 4.1 90/04/28 22:44:02 syd Exp $";

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
 * $Log:	savecopy.c,v $
 * Revision 4.1  90/04/28  22:44:02  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Save a copy of the specified message in a folder.

**/

#include "headers.h"
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif

#include <errno.h>

char *format_long();
char *error_name(), *error_description();
char *ctime();

extern char in_reply_to[SLEN];	/* In-Reply-To: string */
#ifndef OS2
extern int errno;
#endif

char *strcat(), *strcpy();
unsigned long sleep();
time_t time();

save_copy(to, cc, bcc, filename, copy_file, form)
char *to, *cc, *bcc, *filename, *copy_file;
int form;
{
	/** This routine appends a copy of the outgoing message to the
	    file specified.  **/

	FILE *save,		/* file id for file to save to */
	     *message,		/* file id for file with message body */
	     *write_header_info();
	char  buffer[SLEN],	/* read buffer 		       */
	      savename[SLEN];	/* name of file saving into    */


	/* presume copy_file is okay as is for now */
	strcpy(savename, copy_file);

	/* if save-by-name wanted */
	if((strcmp(copy_file, "=") == 0)  || (strcmp(copy_file, "=?") == 0)) {

	  get_return_name(to, buffer, TRUE);	/* determine 'to' login */
	  if (strlen(buffer) == 0) {

	    /* can't get file name from 'to' -- use sent_mail instead */
	    dprint(3, (debugfile,
		"Warning: get_return_name couldn't break down %s\n", to));
	    error1(
"Cannot determine `to' name to save by! Saving to \"sent\" folder %s instead.",
	      sent_mail);
	    strcpy(savename, "<");
	    sleep(3);
	  } else
	    sprintf(savename, "=%s", buffer);		/* good! */
	}

	expand_filename(savename, TRUE);	/* expand special chars */

	/* If saving conditionally by logname but folder doesn't exist
	 * save to sent folder instead. */
	if((strcmp(copy_file, "=?") == 0)
	      && (access(savename, ACCESS_EXISTS) != 0)) {
	  dprint(5, (debugfile,
	    "Conditional save by name: file %s doesn't exist - using \"<\".\n",
	    savename));
	  strcpy(savename, "<");
	  expand_filename(savename, TRUE);
	}

	if ((errno = can_open(savename, "a"))) {
	  dprint(2, (debugfile,
	  "Error: attempt to autosave to a file that can't be appended to!\n"));
	  dprint(2, (debugfile, "\tfilename = \"%s\"\n", savename));
	  dprint(2, (debugfile, "** %s - %s **\n", error_name(errno),
		  error_description(errno)));

	  /* Lets try sent_mail before giving up */
	  if(strcmp(sent_mail, savename) == 0) {
	    /* we are ALREADY using sent_mail! */
	    error1("Cannot save to %s!", savename);
	    sleep(3);
	    return(FALSE);
	  }

	  if ((errno = can_open(sent_mail, "a"))) {
	    dprint(2, (debugfile,
	  "Error: attempt to autosave to a file that can't be appended to!\n"));
	    dprint(2, (debugfile, "\tfilename = \"%s\"\n", sent_mail));
	    dprint(2, (debugfile, "** %s - %s **\n", error_name(errno),
		    error_description(errno)));
	    error2("Cannot save to %s nor to \"sent\" folder %s!",
		    savename, sent_mail);
	    sleep(3);
	    return(FALSE);
	  }
	  error2("Cannot save to %s! Saving to \"sent\" folder %s instead.",
	      savename, sent_mail);
	  sleep(3);
	  strcpy(savename, sent_mail);
	}

	save_file_stats(savename);

	/* Write header */
	if ((save = write_header_info(savename, to, cc, bcc,
	      form == YES, TRUE, FALSE)) == NULL)
	  return(FALSE);

	/* Now add file with message as handed to mailer */
	if ((message = fopen(filename, "r")) == NULL) {
	  fclose(save);
	  dprint(1, (debugfile,
		 "Error: Couldn't read folder %s (save_copy)\n", filename));
	  dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
		  error_description(errno)));
	  error1("Couldn't read folder %s!", filename);
	  sleep(3);
	  return(FALSE);
	}

        copy_message_across(message, save, TRUE);


	fclose(save);
	fclose(message);

	restore_file_stats(savename);

	return(TRUE);
}
char *
cf_english(fn)
char *fn;
{
    /** Return "English" expansion for special copy file name abbreviations
	or just the file name  **/

    if(!*fn)
      return("<no save>");
    else if(!fn[1]) {
      if(*fn == '=')
	return("<unconditionally save by name>");
      else if(*fn == '<')
	return("<\"sent\" folder>");
    } else if ((fn[0] == '=') && (fn[1] == '?'))
      return("<conditionally save by name>");

    return(fn);
}

#define NCF_PROMPT	"Save copy in (use '?' for help/to list folders): "
int
name_copy_file(fn)
char *fn;
{
    /** Prompt user for name of file for saving copy of outbound msg to.
	Return true if we need a redraw. **/

    int redraw = 0;	/* set when we ask for help = need redraw */
    char buffer[SLEN], origbuffer[SLEN];
    static char helpmsg[LONG_STRING];

    /* expand passed copy file name into English */
    strcpy(buffer, cf_english(fn));

    /* prepare screen with instructions */
    MoveCursor(LINES-2, 0);
    CleartoEOS();
    PutLine0(LINES-2, 0, NCF_PROMPT);

    while(1) {

      /* get file name from user input */
      strcpy(origbuffer, buffer);
      optionally_enter(buffer, LINES-2, strlen(NCF_PROMPT), FALSE, FALSE);

      if(strcmp(buffer, "?") != 0) { /* got what we wanted - non-help choice */

	if(strcmp(origbuffer, buffer) != 0)
	  /* user changed from our English expansion
	   * so we'd better copy user input to fn
	   */
	  strcpy(fn, buffer);

	/* else user presumably left our English expansion - no change in fn */

	/* display English expansion of new user input a while */
	PutLine1(LINES-2, strlen(NCF_PROMPT), cf_english(fn));
	MoveCursor(LINES, 0);
	sleep(1);
	MoveCursor(LINES-2, 0);
	CleartoEOS();

	return(redraw);
      }

      /* give help and list folders */
      redraw = TRUE;
      if(!*helpmsg) 	/* help message not yet formulated */
        sprintf(helpmsg,
	"\n\r%s\n\r%s%s%s\n\r%s\n\r%s\n\r%s\n\r%s\n\r%s\n\r\n\r",
	"Enter: <nothing> to not save a copy of the message,",
	"       '<'       to save in your \"sent\" folder (", sent_mail, "),",
	"       '='       to save by name (the folder name depends on whom the",
	"                     message is to, in the end),",
	"       '=?'      to save by name if the folder already exists,",
	"                     and if not, to your \"sent\" folder,",
	"       or a filename (a leading '=' denotes your folder directory).");

      list_folders(4, helpmsg);
      PutLine0(LINES-2, 0, NCF_PROMPT);

      /* restore as default to English version of the passed copy file name */
      strcpy(buffer, cf_english(fn));

    }
}
