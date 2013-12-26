
static char rcsid[] = "@(#)$Id: help.c,v 4.1 90/04/28 22:43:11 syd Exp $";

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
 * $Log:	help.c,v $
 * Revision 4.1  90/04/28  22:43:11  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/*** help routine for ELM program

***/

#include <ctype.h>
#include "headers.h"

help(pager_help)
int pager_help;
{
	/** Process the request for help [section] from the user.
	    If pager_help is TRUE, then act a little differently from
	    if pager_help is FALSE (index screen)
	 **/

	char ch;		/* character buffer for input */
	char *s;		/* string pointer...	      */
	int prompt_line, info_line;
	static char help_message[] =
   "Press the key you want help for, '?' for a key list, or '.' to exit help";
	static char help_prompt[] = "Help for key: ";

	MoveCursor(LINES-4,0);
	CleartoEOS();

	if(pager_help) {
	  put_border();
	  Centerline(LINES, help_message);
	  prompt_line = LINES-3;
	} else {
	  Centerline(LINES-4, "ELM Help System");
	  Centerline(LINES-3, help_message);
	  prompt_line = LINES-2;
	}
	info_line = prompt_line + 1;

	PutLine0(prompt_line, 0, help_prompt);

	do {
	  MoveCursor(prompt_line, strlen(help_prompt));
	  ch = ReadCh();

	  if (ch == '.') return(0);	/* zero means footer rewrite only */

	  s = "Unknown command.  Use '?' for a list of commands.";

	  switch (ch) {

	    case '?': display_helpfile(pager_help? PAGER_HELP : MAIN_HELP);
		      return(1);

	    case '$': if(!pager_help) s =
"$ = Force resynchronization of the current folder. This will purge deleted mail.";
		      break;

	    case '!': s =
     "! = Escape to the Unix shell of your choice, or just to enter commands.";
	              break;

	    case '@': if(!pager_help) s =
	   "@ = Debug - display a summary of the messages on the header page.";
		      break;

	    case '|': s =
   "| = Pipe the current message or tagged messages to the command specified.";
		      break;

	    case '#': if(!pager_help) s =
	    "# = Debug - display all information known about current message.";
		      break;

	    case '%': s =
     "% = Debug - display the computed return address of the current message.";
		      break;

	    case '*': if(!pager_help)
		       s = "* = Go to the last message in the current folder.";
		      break;

	    case '-': if(!pager_help) s =
"- = Go to the previous page of messages.  This is the same as the LEFT arrow.";
		      break;

	    case '=': if(!pager_help) s =
			"'=' = Go to the first message in the current folder.";
		      break;

	    case ' ': if(pager_help) s =
  "<space> = Display next screen of current message (or first screen of next).";
		      else s = "<space> = Display the current message.";
		      break;

	    case '+': if(!pager_help)
	    		s =
  "+ = Go to the next page of messages.  This is the same as the RIGHT arrow.";
		      break;

	    case '/': if(!pager_help)
			s = "/ = Search for specified pattern in folder.";
		      break;

	    case '<': s =
	       "< = Scan current message for calendar entries (if enabled).";
		      break;

	    case '>': s =
	       "> = Save current message or tagged messages to specified file.";
		      break;

	    case '^': if(!pager_help) s =
	       "^ = Toggle the Delete/Undelete status of the current message.";
		      break;

	    case 'a': if(!pager_help) s =
	   "a = Enter the alias sub-menu section.  Create and display aliases.";
		      break;

	    case 'b': s =
      "b = Bounce (remail) a message to someone as if you have never seen it.";
		      break;

	    case 'C': s =
               "C = Copy current message or tagged messages to specified file.";
		      break;

	    case 'c': if(!pager_help) s =
	 "c = Change folders, leaving the current folder as if 'quitting'.";
		      break;

	    case 'd': s = "d = Mark the current message for future deletion.";
		      break;

	    case ctrl('D') : if(!pager_help) s =
	      "^D = Mark for deletion all messages with the specified pattern.";
		      break;

	    case 'e': if(!pager_help) s =
     "e = Invoke the editor on the entire folder, resynchronizing when done.";
		      break;

	    case 'f': s =
	"f = Forward the current message to someone, return address is yours.";
		      break;

	    case 'g': s =
   "g = Group reply not only to the sender, but to everyone who received msg.";
		      break;

	    case 'h': s =
		 "h = Display message with all Headers (ignore weedout list).";
	              break;

	    case 'i': if(pager_help) s = "i = Return to the index.";
		      break;

	    case 'J': s = "J = Go to the next message.";
		      break;

	    case 'j': s =
  "j = Go to the next undeleted message.  This is the same as the DOWN arrow.";
		      break;

	    case 'K': s = "K = Go to the previous message.";
		      break;

	    case 'k': s =
"k = Go to the previous undeleted message.  This is the same as the UP arrow.";
		      break;

	    case 'l': if(!pager_help) s =
               "l = Limit displayed messages based on the specified criteria.";
		      break;

	    case 'm': s =
		 "m = Create and send mail to the specified person or persons.";
		      break;

	    case 'n': if(pager_help)
			s = "n = Display the next message.";
		      else
			s =
	   "n = Display the current message, then move current to next messge.";
		      break;

	    case 'o': if(!pager_help) s = "o = Go to the options submenu.";
	              break;

	    case 'p': s =
		      "p = Print the current message or the tagged messages.";
	              break;

	    case 'q': if(pager_help) s =
			"q = Quit the pager and return to the index.";
		      else s =
		    "q = Quit the mailer, asking about deletion, saving, etc.";
		      break;

	    case 'r': s =
"r = Reply to the message.  This only sends to the originator of the message.";
	              break;

	    case 's': s =
               "s = Save current message or tagged messages to specified file.";
		      break;

	    case 't': s =
               "t = Tag a message for further operations (or untag if tagged).";
		      break;

	    case ctrl('T') : if(!pager_help) s =
			    "^T = Tag all messages with the specified pattern.";
		      break;

	    case 'u': s =
		      "u = Undelete - remove the deletion mark on the message.";
		      break;

	    case 'x': s = "x = Exit the mail system quickly.";
		      break;

	    case 'Q': if(!pager_help) s =
		"Q = Quick quit the mailer, save read, leave unread, delete deleted.";
		      break;

	    case '\n':
	    case '\r': if(pager_help)
			 s =
  "<return> = Display current message, or (builtin pager only) scroll forward.";
		       else
			 s = "<return> = Display the current message.";
		       break;

	    case ctrl('L'): if(!pager_help) s = "^L = Rewrite the screen.";
		       break;

            case ctrl('?'):					    /* DEL */
	    case ctrl('Q'): if(!pager_help) s = "Exit the mail system quickly.";
		       break;

	    default : if (isdigit(ch) && !pager_help)
	            s = "<number> = Make specified number the current message.";
	  }

	  ClearLine(info_line);
	  Centerline(info_line, s);

	} while (ch != '.');

	/** we'll never actually get here, but that's okay... **/

	return(0);
}

display_helpfile(section)
int section;
{
	/*** Help me!  Read file 'helpfile.<section>' and echo to screen ***/

	char buffer[SLEN];

	sprintf(buffer, "%s/%s.%d", helphome, helpfile, section);
	return(display_file(buffer));
}

display_file(file)
char *file;
{
	/*** Display file to screen ***/

	FILE *fileptr;
	int  lines=0;
	char buffer[SLEN];

	if ((fileptr = fopen(file,"r")) == NULL) {
	  dprint(1, (debugfile,
		 "Error: Couldn't open file %s (help)\n", file));
	  error1("Couldn't open file %s.",file);
	  return(FALSE);
	}

	ClearScreen();

	while (fgets(buffer, SLEN, fileptr) != NULL) {
	  if (lines > LINES-3) {
	    PutLine0(LINES,0,"Press <space> to continue, 'q' to return.");
	    if(ReadCh() == 'q') {
	      clear_error();
	      fclose(fileptr);
	      return(TRUE);
	    }
	    lines = 0;
	    ClearScreen();
	    Write_to_screen("%s\r", 1, buffer);
	  }
	  else
	    Write_to_screen("%s\r", 1, buffer);

	  lines += strlen(buffer)/COLUMNS + 1;
	}

        PutLine0(LINES,0,"Press any key to return.");

	(void) ReadCh();
	clear_error();

	fclose(fileptr);
	return(TRUE);
}
