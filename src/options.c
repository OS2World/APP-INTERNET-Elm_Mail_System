
static char rcsid[] = "@(#)$Id: options.c,v 4.1 90/04/28 22:43:38 syd Exp $";

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
 * $Log:	options.c,v $
 * Revision 4.1  90/04/28  22:43:38  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This set of routines allows the alteration of a number of paramaters
    in the Elm mailer, including the following;

	calendar-file	<where to put calendar entries>
	display pager	<how to page messages>
	editor		<name of composition editor>
	folder-dir	<folder directory>
	sort-by		<how to sort folders>
	sent-mail	<file to save outbound message copies to>
	printmail	<how to print messages>
	full_username	<your full user name for outgoing mail>

	arrow-cursor	<on or off>
	menu-display    <on or off>

	user-level	<BEGINNER|INTERMEDIATE|EXPERT>
        names-only      <on or off>

    And others as they seem useful.

**/

#include "headers.h"

#ifdef BSD
#undef tolower
#endif

#undef onoff
#define   onoff(n)	(n == 1? "ON ":"OFF")

char *one_liner_for(), *level_name();
unsigned long sleep();

options()
{
	/** change options... **/
	/* return:
	 *	> 0	if restort was done - to indicate we might need to
	 *	 	change the page of our headers as a consequence
	 *		of the new sort order
	 *	< 0	if user entered 'x' to quit elm immediately
	 *	0	otherwise
	 */

	int	ch,
	     	resort = 0;
	char	*strcpy(),
	     	temp[SLEN];	/* needed when an option is run through
				 * expand_env(), because that function
				 * is destructive of the original
				 */

	display_options();

	clearerr(stdin);

	while(1) {
	  ClearLine(LINES-4);

	  Centerline(LINES-4,
 "Select first letter of option line, '>' to save, or 'i' to return to index.");

	  PutLine0(LINES-2, 0, "Command: ");

	  ch = ReadCh();
	  ch = tolower(ch);

	  clear_error();	/* remove possible "sorting" message etc... */

	  one_liner(one_liner_for(ch));

	  switch (ch) {
	    case 'c' : optionally_enter(raw_calendar_file, 2, 23, FALSE, FALSE);
		       strcpy(temp, raw_calendar_file);
		       expand_env(calendar_file, temp);
		       break;
	    case 'd' : optionally_enter(raw_pager, 3, 23, FALSE, FALSE);
		       strcpy(temp, raw_pager);
		       expand_env(pager, temp);
		       clear_pages = (equal(pager, "builtin+") ||
			             equal(pager, "internal+"));
		       break;
	    case 'e' : optionally_enter(raw_editor, 4, 23, FALSE, FALSE);
		       strcpy(temp, raw_editor);
		       expand_env(editor, temp);
	               break;
	    case 'f' : optionally_enter(raw_folders, 5, 23, FALSE, FALSE);
		       strcpy(temp, raw_folders);
		       expand_env(folders, temp);
		       break;
	    case 's' : if(change_sort(6,23)) resort++;			break;
	    case 'o' : optionally_enter(raw_sentmail, 7, 23, FALSE, FALSE);
		       strcpy(temp, raw_sentmail);
		       expand_env(sent_mail, temp);
		       break;
	    case 'p' : optionally_enter(raw_printout, 8, 23, FALSE, FALSE);
		       strcpy(temp, raw_printout);
		       expand_env(printout, temp);
		       break;
	    case 'y' : optionally_enter(full_username, 9, 23, FALSE, FALSE);
		       break;
	    case 'a' : on_or_off(&arrow_cursor, 12, 23); 		break;
	    case 'm' : on_or_off(&mini_menu, 13, 23);
		       headers_per_page = LINES - (mini_menu ? 13 : 8); break;

	    case 'u' : switch_user_level(&user_level,15, 23);		break;
	    case 'n' : on_or_off(&names_only, 16, 23);			break;

	    case '?' : options_help();
	               PutLine0(LINES-2,0,"Command: ");			break;

	    case '>' : printf("Save options in .elm/elmrc...");
		       fflush(stdout);    save_options();		break;

	    case 'x' :	return(-1);	/* exit elm */
	    case 'q' :	/* pop back up to previous level, in this case == 'i' */
	    case 'i' :  /* return to index screen */
			return(resort ? 1 : 0);
	    case ctrl('L'): display_options();				break;
	    default: error("Command unknown!");
	  }

	}
}

display_options()
{
	/** Display all the available options.. **/

	char *sort_name();

	ClearScreen();
	Centerline(0,"-- ELM Options Editor --");

#ifdef ENABLE_CALENDAR
	PutLine1(2, 0, "C)alendar file       : %s", raw_calendar_file);
#endif
	PutLine1(3, 0, "D)isplay mail using  : %s", raw_pager);
	PutLine1(4, 0, "E)ditor              : %s", raw_editor);
	PutLine1(5, 0, "F)older directory    : %s", raw_folders);
	PutLine1(6, 0, "S)orting criteria    : %s", sort_name(FULL));
	PutLine1(7, 0, "O)utbound mail saved : %s", raw_sentmail);
	PutLine1(8, 0, "P)rint mail using    : %s", raw_printout);
	PutLine1(9, 0, "Y)our full name      : %s", full_username);

	PutLine1(12,0, "A)rrow cursor        : %s", onoff(arrow_cursor));
	PutLine1(13,0, "M)enu display        : %s", onoff(mini_menu));

	PutLine1(15,0, "U)ser level          : %s", level_name(user_level));
	PutLine1(16,0, "N)ames only          : %s", onoff(names_only));
}

on_or_off(var, x, y)
int *var, x,y;
{
	/** 'var' field at x.y toggles between on and off... **/

	char ch;

     	PutLine0(x, y+6,
		"(use <space> to toggle, any other key to leave)");

	MoveCursor(x,y+3);	/* at end of value... */

	do {
	  ch = ReadCh();

	  if (ch == SPACE) {
	    *var = ! *var;
	    PutLine0(x,y, onoff(*var));
	  }
	} while (ch == SPACE);

	MoveCursor(x,y+4); 	CleartoEOLN();	/* remove help prompt */
}


switch_user_level(ulevel, x, y)
int *ulevel, x, y;
{
	/** step through possible user levels... **/

     	PutLine0(x, y+20, "<space> to change");

	MoveCursor(x,y);	/* at end of value... */

	while (ReadCh() == ' ') {
	  *ulevel = (*ulevel >= 2? 0 : *ulevel + 1);
	  PutLine1(x,y, "%s", level_name(*ulevel));
	}

	MoveCursor(x,y+20); 	CleartoEOLN();	/* remove help prompt */
}

change_sort(x, y)
int x,y;
{
	char *sort_name();
	/** change the sorting scheme... **/
	/** return !0 if new sort order, else 0 **/

	int last_sortby,	/* so we know if it changes... */
	    sign = 1;		/* are we reverse sorting??    */
	int ch;			/* character typed in ...      */

	last_sortby = sortby;	/* remember current ordering   */

	PutLine0(x, COLUMNS-29, "(SPACE for next, or R)everse)");
	sort_one_liner(sortby);
	MoveCursor(x, y);

	do {
	  ch = ReadCh();
	  ch = tolower(ch);
	  switch (ch) {
	    case SPACE : if (sortby < 0) {
	    		   sign = -1;
	    		   sortby = - sortby;
	  		 }
			 else sign = 1;		/* insurance! */
	  		 sortby = sign * ((sortby + 1) % (STATUS+2));
			 if (sortby == 0) sortby = sign;  /* snicker */
	  		 PutLine0(x, y, sort_name(PAD));
			 sort_one_liner(sortby);
	  		 MoveCursor(x, y);
			 break;

	    case 'r'   : sortby = - sortby;
	  		 PutLine0(x, y, sort_name(PAD));
			 sort_one_liner(sortby);
	  		 MoveCursor(x, y);
	 }
        } while (ch == SPACE || ch == 'r');

	MoveCursor(x, COLUMNS-30);	CleartoEOLN();

	if (sortby != last_sortby) {
	  error("Resorting folder...");
	  sleep(1);
	  sort_mailbox(message_count, 0);
	}
	ClearLine(LINES-2);		/* clear sort_one_liner()! */
	return(sortby-last_sortby);
}

one_liner(string)
char *string;
{
	/** A single-line description of the selected item... **/

	ClearLine(LINES-4);
	if (string)
		Centerline(LINES-4, string);
}

sort_one_liner(sorting_by)
int sorting_by;
{
	/** A one line summary of the particular sorting scheme... **/

	ClearLine(LINES-2);

	switch (sorting_by) {

	  case -SENT_DATE : Centerline(LINES-2,
"This sort will order most-recently-sent to least-recently-sent");	break;
	  case -RECEIVED_DATE : Centerline(LINES-2,
"This sort will order most-recently-received to least-recently-received");
		 	    break;
	  case -MAILBOX_ORDER : Centerline(LINES-2,
"This sort will order most-recently-added-to-folder to least-recently");
		 	    break;
	  case -SENDER : Centerline(LINES-2,
"This sort will order by sender name, in reverse alphabetical order");	break;
	  case -SIZE   : Centerline(LINES-2,
"This sort will order messages by longest to shortest");		break;
	  case -SUBJECT : Centerline(LINES-2,
"This sort will order by subject, in reverse alphabetical order");	break;
	  case -STATUS  : Centerline(LINES-2,
"This sort will order by reverse status - Deleted through Tagged...");	break;

	  case SENT_DATE : Centerline(LINES-2,
"This sort will order least-recently-sent to most-recently-sent");	break;
	  case RECEIVED_DATE : Centerline(LINES-2,
"This sort will order least-recently-received to most-recently-received");
	                    break;
	  case MAILBOX_ORDER : Centerline(LINES-2,
"This sort will order least-recently-added-to-folder to most-recently");
		 	    break;
	  case SENDER : Centerline(LINES-2,
		        "This sort will order by sender name");	break;
	  case SIZE   : Centerline(LINES-2,
		        "This sort will order messages by shortest to longest");
			break;
	  case SUBJECT : Centerline(LINES-2,
            		"This sort will order messages by subject");	break;
	  case STATUS  : Centerline(LINES-2,
"This sort will order by status - Tagged through Deleted...");		break;
	}
}

char *one_liner_for(c)
char c;
{
	/** returns the one-line description of the command char... **/

	switch (c) {
	    case 'c' : return(
"This is the file where calendar entries from messages are saved.");

	    case 'd' : return(
"This is the program invoked to display individual messages (try 'builtin')");

	    case 'e' : return(
"This is the editor that will be used for sending messages, etc.");

	    case 'f' : return(
"This is the folders directory used when '=' (etc) is used in filenames");

	    case 'm' : return(
"This determines if you have the mini-menu displayed or not");

	    case 'n' : return(
"Whether to display the names and addresses on mail, or names only");
	    case 'o' : return(
"This is where copies of outbound messages are saved automatically.");

	    case 'p' : return(
"This is how printouts are generated.  \"%s\" will be replaced by the filename.");

	    case 's' : return(
"This is used to specify the sorting criteria for the folders");

	    case 'y' : return(
"When mail is sent out, this is what your full name will be recorded as.");

	    case 'a' : return(
"This defines whether the ELM cursor is an arrow or a highlight bar.");

	   case 'u' : return(
"The level of knowledge you have about the ELM mail system.");

	    default : return(NULL);	/* nothing if we don't know! */
	}
}

options_help()
{
	/** help menu for the options screen... **/

	char c, *ptr;

	Centerline(LINES-3,
  "Press the key you want help for, '?' for a key list, or '.' to exit help");

	lower_prompt("Key : ");

	while ((c = ReadCh()) != '.') {
	  c = tolower(c);
	  if (c == '?') {
	     display_helpfile(OPTIONS_HELP);
	     display_options();
	     return;
	  }
	  if ((ptr = one_liner_for(c)) != NULL)
	    error2("%c = %s.", c, ptr);
	  else
	    error1("%c isn't used in this section.", c);
	  lower_prompt("Key : ");
	}
}

char *level_name(n)
int n;
{
	/** return the 'name' of the level... **/

	switch (n) {
	  case 0 : return("Beginning User   ");
	  case 1 : return("Intermediate User");
	  default: return("Expert User      ");
	}
}
