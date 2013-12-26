
static char rcsid[] = "@(#)$Id: file.c,v 4.1.1.1 90/10/07 19:48:05 syd Exp $";

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
 * $Log:	file.c,v $
 * Revision 4.1.1.1  90/10/07  19:48:05  syd
 * fix the bounce problem reported earlier when using MMDF submit as the MTA.
 * From: Jim Clausing <jac%brahms.tinton.ccur.com@RELAY.CS.NET>
 *
 * Revision 4.1  90/04/28  22:43:02  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** File I/O routines, mostly the save to file command...

**/

#ifdef PWDINSYS
#  include <sys/pwd.h>
#else
#  include <pwd.h>
#endif

#include "headers.h"
#include <ctype.h>
#include <errno.h>

#ifdef BSD
#undef tolower
#endif

#ifndef OS2
extern int errno;
#endif

char *error_name(), *error_description(), *strcpy(), *getenv(), *nameof();
unsigned long sleep();

int
save(redraw, silently, type)
int *redraw, silently, type;
{
	/** Save all tagged messages + current in a folder.  If no messages
	    are tagged, save the current message instead!  This routine
	    will return ZERO if the operation failed.
	    'redraw' is set to TRUE iff we use the '?' and mess up
	    the screen.  Pretty reasonable, eh?  If "silently" is set,
	    then don't output the "D" character upon marking for
	    deletion...
	    If delete is set, then delete the saved messages, else
	    we are just copying the messages without deletion.
	**/

	register int tagged = 0, i, oldstat, appending = 0;
	int mesgnum;	/* message whose address is used for save-by-name fn */
	char filename[SLEN], address[LONG_STRING], buffer[LONG_STRING];
	static char helpmsg[LONG_STRING];
	FILE *save_file;
	int delete = (type != 'C');
	int reverse = (type == 'S');

	oldstat = headers[current-1]->status;	/* remember */
	*redraw = FALSE;

	for (i=0; i < message_count; i++) {
	  if (ison(headers[i]->status, TAGGED)) {
	    if(!tagged)
	      mesgnum = i;	/* first tagged msg -  use this one for
				 * save-by-name folder name */
	    tagged++;
	  }
	}

	if (tagged == 0) {
	  mesgnum = current-1;	/* use this one for save-by-name folder name */
	  tagged = 1;
	  setit(headers[current-1]->status, TAGGED);
	}

	dprint(4, (debugfile, "%d message%s tagged for saving (save)\n", tagged,
		plural(tagged)));

	while (1) {

	  PutLine2(LINES-2, 0, "%s message%s to: ",
	  	(delete ? "Save" : "Copy"), plural(tagged));

	  if (save_by_name) {
	    /** build default filename to save to **/
	    if (reverse) {
	      get_existing_address(buffer, mesgnum);
	      strcpy(address, strip_parens(buffer));
	    } 
	    else
	      get_return(address, mesgnum);
	    get_return_name(address, buffer, TRUE);
	    sprintf(filename, "=%s", buffer);
	  }
	  else
	    filename[0] = '\0';

	  if (tagged > 1)
	    optionally_enter(filename, LINES-2, 19, FALSE, FALSE);
	  else
	    optionally_enter(filename, LINES-2, 18, FALSE, FALSE);


	  if (strlen(filename) == 0) {  /** <return> means 'cancel', right? **/
	    headers[current-1]->status = oldstat;	/* BACK! */
	    return(0);
	  }

	  if (strcmp(filename,"?") == 0) {	/* user asked for listing */
	    *redraw = TRUE;	/* set the flag so we know what to do later */
	    if(!*helpmsg) {	/* format helpmsg if not yet done */

	      strcpy(helpmsg, "\n\r\n\rEnter: <nothing> to not ");
	      strcat(helpmsg, (delete ? "save" : "copy"));
	      strcat(helpmsg, " your message");
	      strcat(helpmsg, (plural(tagged) ? "s" : ""));
	      strcat(helpmsg, "\n\r       '>' to ");
	      strcat(helpmsg, (delete ? "save" : "copy"));
	      strcat(helpmsg, " your message");
	      strcat(helpmsg, (plural(tagged) ? "s" : ""));
	      strcat(helpmsg, " to your \"received\" folder (");
	      strcat(helpmsg, nameof(recvd_mail));
	      strcat(helpmsg, ")\n\r       '<' to ");
	      strcat(helpmsg, (delete ? "save" : "copy"));
	      strcat(helpmsg, " your message");
	      strcat(helpmsg, (plural(tagged) ? "s" : ""));
	      strcat(helpmsg, " to your \"sent\" folder (");
	      strcat(helpmsg, nameof(sent_mail));
	      strcat(helpmsg, ") \n\r       a filename");
	      strcat(helpmsg, " (leading '=' denotes your folder directory ");
	      strcat(helpmsg, folders);
	      strcat(helpmsg, ").\n\r");
	    }

	    list_folders(4, helpmsg);
	    continue;
	  }

	  /* else - got a folder name - check it out */
	  if (! expand_filename(filename, TRUE)) {
	    dprint(2, (debugfile,
		  "Error: Failed on expansion of filename %s (save)\n",
		  filename));
	    continue;
	  }
	  if ((errno = can_open(filename, "a"))) {
	    error2("Cannot %s message to folder %s!",
	      delete ? "save":"copy", filename);
	    continue;
	  }
	  break;	/* got a valid filename */
	}

	save_file_stats(filename);

	if (access(filename,ACCESS_EXISTS)== 0) 	/* already there!! */
	  appending = 1;

	dprint(4,(debugfile, "Saving mail to folder '%s'...\n", filename));

	if ((save_file = fopen(filename,"a")) == NULL) {
	  dprint(2, (debugfile,
		"Error: couldn't append to specified folder %s (save)\n",
		filename));
	  error1("Couldn't append to folder %s!", filename);
	  headers[current-1]->status = oldstat;	/* BACK! */
	  return(0);
	}

	/* if we need a redraw that means index screen no longer present
	 * so whatever silently was, now it's true - we can't show those
	 * delete markings.
	 */
	if(*redraw) silently = TRUE;

	for (i=0; i < message_count; i++) 	/* save each tagged msg */
	  if (headers[i]->status & TAGGED)
	    save_message(i, filename, save_file, (tagged > 1), appending++,
			 silently, delete);

	fclose(save_file);

	restore_file_stats(filename);

	if (tagged > 1)
	  error2("Message%s %s.", plural(tagged), delete ? "saved": "copied");
	return(1);
}

int
save_message(number, filename, fd, pause, appending, silently, delete)
int number, pause, appending, silently, delete;
char *filename;
FILE *fd;
{
	/** Save an actual message to a folder.  This is called by
	    "save()" only!  The parameters are the message number,
	    and the name and file descriptor of the folder to save to.
	    If 'pause' is true, a sleep(2) will be done after the
	    saved message appears on the screen...
	    'appending' is only true if the folder already exists
	    If 'delete' is true, mark the message for deletion.
	**/

	register int save_current, is_new;

	dprint(4, (debugfile, "\tSaving message %d to folder...\n", number));

	save_current = current;
	current = number+1;

	/* change status from NEW before copy and reset to what it was
	 * so that copy doesn't look new, but we can preserve new status
	 * of message in this mailfile. This is important because if
	 * user does a resync, we don't want NEW status to be lost.
	 * I.e. NEW becomes UNREAD when we "really" leave a mailfile.
	 */
	if(is_new = ison(headers[number]->status, NEW))
	  clearit(headers[number]->status, NEW);
      copy_message("", fd, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE);
	if(is_new)
	  setit(headers[number]->status, NEW);
	current = save_current;

	if (delete)
	  setit(headers[number]->status, DELETED); /* deleted, but ...   */
	clearit(headers[number]->status, TAGGED);  /* not tagged anymore */

	if (appending)
	  error2("Message %d appended to folder %s.", number+1, filename);
	else
	  error3("Message %d %s to folder %s.", number+1,
	     delete ? "saved" : "copied", filename);

	if (! silently)
	  show_new_status(number);	/* update screen, if needed */

	if (pause && (!silently) && (!appending))
	  sleep(2);
}

int
expand_filename(filename, use_cursor_control)
char *filename;
int use_cursor_control;
{
	/** Expands	~/	to the current user's home directory
			~user/	to the home directory of "user"
			=,+,%	to the user's folder's directory
			!	to the user's incoming mailbox
			>	to the user's received folder
			<	to the user's sent folder
			shell variables (begun with $)

	    Returns 	1	upon proper expansions
			0	upon failed expansions
	 **/

	char temp_filename[SLEN], varname[SLEN],
	    env_value[SLEN], logname[SLEN], *ptr;
	register int iindex;
	struct passwd *pass, *getpwnam();
	char *getenv();

	ptr = filename;
	while (*ptr == ' ') ptr++;	/* leading spaces GONE! */
	strcpy(temp_filename, ptr);

	/** New stuff - make sure no illegal char as last **/
	if (lastch(temp_filename) == '\n' || lastch(temp_filename) == '\r')
	  lastch(temp_filename) = '\0';

	/** Strip off any trailing backslashes **/
	while (lastch(temp_filename) == '\\')
		lastch(temp_filename) = '\0';

	if (temp_filename[0] == '~') {
	  if(temp_filename[1] == '/')
	    sprintf(filename, "%s%s%s",
		  home, (lastch(home) != '/' ? "/" : ""), &temp_filename[2]);
	  else {
	    for(ptr = &temp_filename[1], iindex = 0; *ptr && *ptr != '/'; ptr++, iindex++)
	      logname[iindex] = *ptr;
	    logname[iindex] = '\0';
	    if((pass = getpwnam(logname)) == NULL) {
	      dprint(3,(debugfile,
		      "Error: Can't get home directory for %s (%s)\n",
		      logname, "expand_filename"));
	      if(use_cursor_control)
		error1("Don't know what the home directory of \"%s\" is!",
			logname);
	      else
		printf(
		    "\n\rDon't know what the home directory of \"%s\" is!\n\r",
		    logname);
	      return(0);
	    }
	    sprintf(filename, "%s%s", pass->pw_dir, ptr);
	  }

	}
	else if (temp_filename[0] == '=' || temp_filename[0] == '+' ||
	 	 temp_filename[0] == '%') {
	  sprintf(filename, "%s%s%s", folders,
		(temp_filename[1] != '/' && lastch(folders) != '/')? "/" : "",
	  	&temp_filename[1]);
	}
	else if (temp_filename[0] == '$') {	/* env variable! */
	  for(ptr = &temp_filename[1], iindex = 0; isalnum(*ptr); ptr++, iindex++)
	    varname[iindex] = *ptr;
	  varname[iindex] = '\0';

	  env_value[0] = '\0';			/* null string for strlen! */
	  if (getenv(varname) != NULL)
	    strcpy(env_value, getenv(varname));

	  if (strlen(env_value) == 0) {
	    dprint(3,(debugfile,
		    "Error: Can't expand environment variable $%s (%s)\n",
		    varname, "expand_filename"));
	    if(use_cursor_control)
	      error1("Don't know what the value of $%s is!", varname);
	    else
	      printf("\n\rDon't know what the value of $%s is!\n\r", varname);
	    return(0);
	  }

	  sprintf(filename, "%s%s%s", env_value,
		(*ptr != '/' && lastch(env_value) != '/')? "/" : "", ptr);

	} else if (strcmp(temp_filename, "!") == 0) {
	  strcpy(filename, defaultfile);
	} else if (strcmp(temp_filename, ">") == 0) {
	  strcpy(filename, recvd_mail);
	} else if (strcmp(temp_filename, "<") == 0) {
	  strcpy(filename, sent_mail);
	} else
	  strcpy(filename, temp_filename);

	return(1);
}
