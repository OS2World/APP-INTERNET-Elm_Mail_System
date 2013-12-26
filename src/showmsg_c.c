
static char rcsid[] = "@(#)$Id: showmsg_c.c,v 4.1 90/04/28 22:44:08 syd Exp $";

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
 * $Log:	showmsg_c.c,v $
 * Revision 4.1  90/04/28  22:44:08  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This is an interface for the showmsg command line.  The possible
    functions that could be invoked from the showmsg command line are
    almost as numerous as those from the main command line and include
    the following;

	   |    = pipe this message to command...
	   !    = call Unix command
	   <    = scan message for calendar info
	   b    = bounce (remail) message
	   d    = mark message for deletion
	   f    = forward message
	   g    = group reply
	   h    = redisplay this message from line #1, showing headers
	   <CR> = redisplay this message from line #1, weeding out headers
	   i,q  = move back to the index page (simply returns from function)
	   J    = move to body of next message
	   j,n  = move to body of next undeleted message
	   K    = move to body of previous message
	   k    = move to body of previous undeleted message
	   m    = mail a message out to someone
	   p    = print this (all tagged) message
	   r    = reply to this message
	   s    = save this message to a maibox/folder
	   t    = tag this message
	   u    = undelete message
	   x    = Exit Elm NOW

    all commands not explicitly listed here are beeped at.  Use i)ndex
    to get back to the main index page, please.

    This function returns when it is ready to go back to the index
    page.
**/

#include "headers.h"

int	screen_mangled = 0;
char    msg_line[SLEN];
#define store_msg(a)	(void)strcpy(msg_line,a)
#define put_prompt()	PutLine0(LINES-3, 0, "Command:")
#define put_help()	PutLine0(LINES-3, 45, "(Use 'i' to return to index.)")
#define POST_PROMPT_COL	strlen("Command: ")


int
process_showmsg_cmd(command)
int command;
{
	int     i, intbuf;		/* for dummy parameters...etc */
	int 	ch;			/* for arrow keys */
	int	key_offset;		/* for arrow keys */
	int	istagged;		/* for tagging and subsequent msg */

	Raw(ON);

	while (TRUE) {
	  clear_error();
	  switch (command) {
	    case '?' : if (help(TRUE)) {
			 ClearScreen();
			 build_bottom();
		       } else screen_mangled = TRUE;
		       break;

	    case '|' : put_cmd_name("Pipe", TRUE);
		       (void) do_pipe();     /* do pipe - ignore return val */
		       ClearScreen();
		       build_bottom();
		       break;

#ifdef ALLOW_SUBSHELL
	    case '!' : put_cmd_name("System call", TRUE);
		       (void) subshell();
		       ClearScreen();
		       build_bottom();
		       break;
#endif

	    case '<' :
#ifdef ENABLE_CALENDAR
		       put_cmd_name("Scan messages for calendar entries", TRUE);
		       scan_calendar();
#else
		       store_msg("Can't scan for calendar entries!");
#endif
		       break;

	    case '%' : put_cmd_name("Display return address", TRUE);
		       get_return(msg_line, current-1);
		       break;

	    case 'b' : put_cmd_name("Bounce message", TRUE);
		       remail();
		       break;

	    case 'd' : delete_msg(TRUE, FALSE); /* really delete it, silent */
		       if (! resolve_mode)
			 store_msg("Message marked for deletion.");
		       else
			 goto next_undel_msg;
		       break;

	    case 'f' : put_cmd_name("Forward message", TRUE);
		       if(forward()) put_border();
		       break;

	    case 'g' : put_cmd_name("Group reply", TRUE);
		       (void) reply_to_everyone();
		       break;

	    case 'h' : screen_mangled = 0;
		       if (filter) {
		         filter = 0;
		         intbuf = show_msg(current);
		         filter = 1;
			 return(intbuf);
		       } else
		         return(show_msg(current));

	    case 'q' :
	    case 'i' : (void) get_page(current);
		       clear_error();		/* zero out pending msg   */
		       if (cursor_control)
			 transmit_functions(ON);
		       screen_mangled = 0;
		       return(0);		/* avoid <return> looping */

next_undel_msg :	/* a target for resolve mode actions */

	    case ' ' :
	    case 'j' :
	    case 'n' : screen_mangled = 0;
		       if((i=next_message(current-1, TRUE)) != -1)
			 return(show_msg(current = i+1));
		       else return(0);

next_msg:
	    case 'J' : screen_mangled = 0;
		       if((i=next_message(current-1, FALSE)) != -1)
			 return(show_msg(current = i+1));
		       else return(0);

prev_undel_msg:
	    case 'k' : screen_mangled = 0;
		       if((i=prev_message(current-1, TRUE)) != -1)
			 return(show_msg(current = i+1));
		       else return(0);

	    case 'K' : screen_mangled = 0;
		       if((i=prev_message(current-1, FALSE)) != -1)
			 return(show_msg(current = i+1));
		       else return(0);

	    case 'm' : put_cmd_name("Mail message", TRUE);
		       if(sendmsg("","","", TRUE, allow_forms, FALSE))
			 put_border();
		       break;

	    case 'p' : put_cmd_name("Print message", FALSE);
		       print_msg();
		       store_msg("Queued for printing.");
		       break;

	    case 'r' : put_cmd_name("Reply to message", TRUE);
		       if(reply()) put_border();
		       break;

	    case '>' :
	    case 'C' :
	    case 's' : put_cmd_name((command != 'C' ? "Save" : "Copy"), TRUE);
		       (void) save(&intbuf, TRUE, (command != 'C'));
		       if (resolve_mode && command != 'C')
			 goto next_undel_msg;
		       break;

	    case 't' : istagged=tag_message(FALSE);
		       if(istagged)
			 store_msg("Message tagged.");
		       else
			 store_msg("Message untagged.");
		       break;

	    case 'u' : undelete_msg(FALSE); /* undelete it, silently */
		       if (! resolve_mode)
			 store_msg("Message undeleted.");
		       else {
/******************************************************************************
 ** We're special casing the U)ndelete command here *not* to move to the next
 ** undeleted message ; instead it'll blindly move to the next message in the
 ** list.  See 'elm.c' and the command by "case 'u'" for further information.
 ** The old code was:
			 goto next_undel_msg;
*******************************************************************************/
			 goto next_msg;
		       }
		       break;

	    case 'x' : put_cmd_name("Exit", TRUE);
		       exit_prog();
		       break;

	    case ctrl('J'):
	    case ctrl('M'):  screen_mangled = 0;
			     return(show_msg(current));


            case ESCAPE : if (cursor_control) {

                            key_offset = 1;

                            ch = ReadCh();

                            if (ch == ESCAPE)
                             ch = ReadCh();

                            if ( ch == '[' || ch == 'O')
                            {
                              ch = ReadCh();
                              key_offset++;
                            }

                            if (ch == up[key_offset])
			      goto prev_undel_msg;
                            else if (ch == down[key_offset])
			      goto next_undel_msg;
                            else {
			      screen_mangled = 0;
                              return(0);
			    }
                          }
                          else          /* Eat 2 chars for escape codes */
                          {
                            ch = ReadCh();
                            ch = ReadCh();
                            putchar((char) 007);
                            fflush(stdout);
			    screen_mangled = 0;
                            return(0);
                           }

	    default  : putchar((char) 007);	/* BEEP! */
	  }

	  /* display prompt */
	  if (screen_mangled) {
	    /* clear what was left over from previous command
	     * and display last generated message.
	     */
	    put_prompt();
	    CleartoEOS();
	    put_help();
	    Centerline(LINES, msg_line);
	    MoveCursor(LINES-3, POST_PROMPT_COL);
	  } else {
	    /* display bottom line prompt with last generated message */
	    MoveCursor(LINES, 0);
	    CleartoEOS();
	    StartBold();
	    Write_to_screen("%s Command ('i' to return to index): ",
		1, msg_line);
	    EndBold();
	  }
	  *msg_line = '\0';	/* null last generated message */

	  command = ReadCh();	/* get next command from user */
	}
}

put_cmd_name(command, will_mangle)
char *command;
int will_mangle;
{

	/* If screen is or will be mangled display the command name
	 * and erase the bottom of the screen.
	 * But first if the border line hasn't yet been drawn, draw it.
	 */
	if(will_mangle && !screen_mangled) {
	  build_bottom();
	  screen_mangled = TRUE;
	}
	if(screen_mangled) {
	  PutLine0(LINES-3, POST_PROMPT_COL, command);
	  CleartoEOS();
	}
}

put_border()
{
	 PutLine0(LINES-4, 0,
"--------------------------------------------------------------------------\n");
}

build_bottom()
{
	 MoveCursor(LINES-4, 0);
	 CleartoEOS();
	 put_border();
	 put_prompt();
	 put_help();
}
