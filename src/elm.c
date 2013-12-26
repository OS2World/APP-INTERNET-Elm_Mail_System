
static char rcsid[] = "@(#)$Id: elm.c,v 4.1.1.1 90/12/19 09:44:06 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.1 $   $State: Exp $
 *
 * This file and all associated files and documentation:
 * 			Copyright (c) 1986, 1987 Dave Taylor
 * 			Copyright (c) 1988, 1989, 1990 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log:	elm.c,v $
 * Revision 4.1.1.1  90/12/19  09:44:06  syd
 * Fix not checking for mail before scanning
 * From: Syd via report from Joern Lubkoll
 *
 * Revision 4.1  90/04/28  22:42:54  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/* Main program of the ELM mail system!
*/

#include "elm.h"

#ifdef BSD
#undef        toupper
#undef        tolower
#endif

long bytes();
char *format_long(), *parse_arguments();

main(argc, argv)
int argc;
char *argv[];
	{
	int  ch;
	char address[SLEN], to_whom[SLEN], *req_mfile;
	int  key_offset;        /** Position offset within keyboard string   **/
	int  redraw, 		/** do we need to rewrite the entire screen? **/
	     nucurr, 		/** change message list or just the current message pointer...   **/
	     nufoot; 		/** clear lines 16 thru bottom and new menu  **/
	int  i,j;      		/** Random counting variables (etc)          **/
	int  pageon, 		/** for when we receive new mail...          **/
	     last_in_folder;	/** for when we receive new mail too...      **/
	long num;		/** another variable for fun..               **/
	extern char version_buff[];
#ifndef OS2
	extern int errno;
#endif

        initpaths();

	req_mfile = parse_arguments(argc, argv, to_whom);

	initialize(req_mfile);

	if (mail_only) {
	   dprint(3, (debugfile, "Mail-only: mailing to\n-> \"%s\"\n",
		   format_long(to_whom, 3)));
	   if(!batch_only) {
	     sprintf(address, "Send only mode [ELM %s]", version_buff);
	     Centerline(1, address);
	   }
	   (void) sendmsg(to_whom, "", batch_subject, TRUE,
	     (batch_only ? NO : allow_forms), FALSE);
	   leave(0);
	} else if (check_only) {
	   do_check_only(to_whom);
	   leave(0);
	}

	ScreenSize(&LINES, &COLUMNS);

	showscreen();

	while (1) {
	  redraw = 0;
	  nufoot = 0;
	  nucurr = 0;
	  /* move_incoming_mail(); */
	  if ((num = bytes(cur_folder)) != mailfile_size) {
	    dprint(2, (debugfile, "Just received %d bytes more mail (elm)\n",
		    num - mailfile_size));
	    error("New mail has arrived! Hang on...");
	    fflush(stdin);	/* just to be sure... */
	    last_in_folder = message_count;
	    pageon = header_page;

	    if ((errno = can_access(cur_folder, READ_ACCESS)) != 0) {
	      dprint(1, (debugfile,
		    "Error: given file %s as folder - unreadable (%s)!\n",
		    cur_folder, error_name(errno)));
	      fprintf(stderr,"Can't open folder '%s' for reading!\n", cur_folder);
	      leave();
	      }

	    newmbox(cur_folder, TRUE);	/* last won't be touched! */
	    clear_error();
	    header_page = pageon;

	    if (on_page(current))   /* do we REALLY have to rewrite? */
	      showscreen();
	    else {
	      update_title();
	      ClearLine(LINES-1);	     /* remove reading message... */
	      error2("%d new message%s received.",
		     message_count - last_in_folder,
		     plural(message_count - last_in_folder));
	    }
	    /* mailfile_size = num; */
	    if (cursor_control)
	      transmit_functions(ON);	/* insurance */
	  }

	  prompt("Command: ");

	  CleartoEOLN();
	  ch = GetPrompt();
	  CleartoEOS();
#ifdef DEBUG
	  if (! movement_command(ch))
	    dprint(4, (debugfile, "\nCommand: %c [%d]\n\n", ch, ch));
#endif

	  set_error("");	/* clear error buffer */

	  MoveCursor(LINES-3,strlen("Command: "));

	  switch (ch) {

	    case '?' 	:  if (help(FALSE))
	  		     redraw++;
			   else
			     nufoot++;
			   break;

	    case '$'    :  PutLine0(LINES-3, strlen("Command: "),
	    		     "Resynchronize folder");
			   redraw = resync();
			   break;

next_page:
	    case '+'	:  /* move to next page if we're not on the last */
			   if((selected &&
			     ((header_page+1)*headers_per_page < selected))
			   ||(!selected &&
			     ((header_page+1)*headers_per_page<message_count))){

			     header_page++;
			     nucurr = NEW_PAGE;

			     if(move_when_paged) {
			       /* move to first message of new page */
			       if(selected)
				 current = visible_to_index(
				   header_page * headers_per_page + 1) + 1;
			       else
				 current = header_page * headers_per_page + 1;
			     }
			   } else error("Already on last page.");
			   break;

prev_page:
	    case '-'	:  /* move to prev page if we're not on the first */
			   if(header_page > 0) {
			     header_page--;
			     nucurr = NEW_PAGE;

			     if(move_when_paged) {
			       /* move to first message of new page */
			       if(selected)
				 current = visible_to_index(
				   header_page * headers_per_page + 1) + 1;
			       else
				 current = header_page * headers_per_page + 1;
			     }
			   } else error("Already on first page.");
			   break;

first_msg:
	    case '='    :  if (selected)
			     current = visible_to_index(1)+1;
			   else
			     current = 1;
			   nucurr = get_page(current);
			   break;

last_msg:
	    case '*'    :  if (selected)
			     current = (visible_to_index(selected)+1);
			   else
			     current = message_count;
			   nucurr = get_page(current);
			   break;

	    case '|'    :  Writechar('|');
			   if (message_count < 1) {
			     error("No mail to pipe!");
			     fflush(stdin);
			   } else {
	    		     softkeys_off();
                             redraw = do_pipe();
			     softkeys_on();
			   }
			   break;

#ifdef ALLOW_SUBSHELL
	    case '!'    :  Writechar('!');
                           redraw = subshell();
			   break;
#endif

	    case '%'    :  if (current > 0) {
			     get_return(address, current-1);
			     clear_error();
			     PutLine1(LINES,(COLUMNS-strlen(address))/2,
				      "%.78s", address);
			   } else {
			     error("No mail to get return address of!");
			     fflush(stdin);
			   }
			   break;

	    case '/'    :  /* scan mbox for string */
			   if  (message_count < 1) {
			     error("No mail to scan!");
			     fflush(stdin);
			   }
			   else if (pattern_match())
			     nucurr = get_page(current);
			   else {
			     error("pattern not found!");
			     fflush(stdin);
			   }
			   break;

	    case '<'    :  /* scan current message for calendar information */
#ifdef ENABLE_CALENDAR
			   if  (message_count < 1) {
			     error("No mail to scan!");
			     fflush(stdin);
			   }
			   else {
			       PutLine0(LINES-3, strlen("Command: "),
				   "Scan message for calendar entries...");
			       scan_calendar();
			   }
#else
	 		   error("Sorry. Calendar function disabled.");
			   fflush(stdin);
#endif
			   break;

	    case 'a'    :  if(alias()) redraw++;
			   else nufoot++;
			   define_softkeys(MAIN); 	break;

	    case 'b'    :  PutLine0(LINES-3, strlen("Command: "),
			     "Bounce message");
			   fflush(stdout);
			   if (message_count < 1) {
	  		     error("No mail to bounce!");
			     fflush(stdin);
			   }
			   else
			     nufoot = remail();
			   break;

	    case 'c'    :  PutLine0(LINES-3, strlen("Command: "),
			      "Change folder");
			   define_softkeys(CHANGE);
			   redraw = change_file();
			   define_softkeys(MAIN);
			   break;

	    case ctrl('D') :
	    case '^'    :
	    case 'd'    :  if (message_count < 1) {
			     error("No mail to delete!");
			     fflush(stdin);
			   }
			   else {
			     if(ch == ctrl('D')) {

			       /* if current message did not become deleted,
				* don't to move to the next undeleted msg. */
			       if(!meta_match(DELETED)) break;

			     } else
 			       delete_msg((ch == 'd'), TRUE);

			     if (resolve_mode) 	/* move after mail resolved */
			       if((i=next_message(current-1, TRUE)) != -1) {
				 current = i+1;
				 nucurr = get_page(current);
			       }
			   }
			   break;


#ifdef ALLOW_MAILBOX_EDITING
	    case 'e'    :  PutLine0(LINES-3,strlen("Command: "),"Edit folder");
			   if (current > 0) {
			     edit_mailbox();
	    		     if (cursor_control)
			       transmit_functions(ON);	/* insurance */
	   		   }
			   else {
			     error("Folder is empty!");
			     fflush(stdin);
			   }
			   break;
#else
	    case 'e'    : error(
		    "Folder editing isn't configured in this version of ELM.");
			  fflush(stdin);
			  break;
#endif

	    case 'f'    :  PutLine0(LINES-3, strlen("Command: "), "Forward");
			   define_softkeys(YESNO);
			   if (current > 0) {
			     if(forward()) redraw++;
			     else nufoot++;
			   } else {
			     error("No mail to forward!");
			     fflush(stdin);
			   }
			   define_softkeys(MAIN);
			   break;

	    case 'g'    :  PutLine0(LINES-3,strlen("Command: "), "Group reply");
			   fflush(stdout);
			   if (current > 0) {
			     if (headers[current-1]->status & FORM_LETTER) {
			       error("Can't group reply to a Form!!");
			       fflush(stdin);
			     }
			     else {
			       define_softkeys(YESNO);
			       redraw = reply_to_everyone();
			       define_softkeys(MAIN);
			     }
			   }
			   else {
			     error("No mail to reply to!");
			     fflush(stdin);
			   }
			   break;

	    case 'h'    :  if (filter)
			     PutLine0(LINES-3, strlen("Command: "),
				"Message with headers...");
			   else
			     PutLine0(LINES-3, strlen("Command: "),"Display message");
			   if(current > 0) {
			     fflush(stdout);
			     j = filter;
			     filter = FALSE;
			     i = show_msg(current);
			     while (i)
				i = process_showmsg_cmd(i);
			     filter = j;
			     redraw++;
			     (void)get_page(current);
			   } else error("No mail to read!");
			   break;

	    case 'J'    :  if(current > 0) {
			     if((i=next_message(current-1, FALSE)) != -1) {
			       current = i+1;
			       nucurr = get_page(current);
			     } else error("No more messages below.");
			   } else error("No mail in folder!");
			   break;

next_undel_msg:
	    case 'j'    :  if(current > 0) {
			     if((i=next_message(current-1, TRUE)) != -1) {
			       current = i+1;
			       nucurr = get_page(current);
			     } else error("No more undeleted messages below.");
			   } else error("No mail in folder!");
			   break;

	    case 'K'    :  if(current > 0) {
			     if((i=prev_message(current-1, FALSE)) != -1) {
			       current = i+1;
			       nucurr = get_page(current);
			     } else error("No more messages above.");
			   } else error("No mail in folder!");
			   break;

prev_undel_msg:
	    case 'k'    :  if(current > 0) {
			     if((i=prev_message(current-1, TRUE)) != -1) {
			       current = i+1;
			       nucurr = get_page(current);
			     } else error("No more undeleted messages above.");
			   } else error("No mail in folder!");
			   break;

	    case 'l'    :  PutLine0(LINES-3, strlen("Command: "),
				   "Limit displayed messages by...");
			   clear_error();
			   if (limit() != 0) {
			     get_page(current);
			     redraw++;
			   } else {
			     nufoot++;
			   }
			   break;

	    case 'm'    :  PutLine0(LINES-3, strlen("Command: "), "Mail");
			   redraw = sendmsg("", "", "", TRUE,allow_forms,FALSE);
			   break;

	    case ' '    :
	    case ctrl('J'):
	    case ctrl('M'):PutLine0(LINES-3, strlen("Command: "),
	    		      "Display message");
			   fflush(stdout);
			   if(current > 0 ) {
			     define_softkeys(READ);

			     i = show_msg(current);
			     while (i)
				i = process_showmsg_cmd(i);
			     redraw++;
			     (void)get_page(current);
			   }else error ("No mail to read!");
			   break;

	    case 'n'    :  PutLine0(LINES-3,strlen("Command: "),"Next Message");
			   fflush(stdout);
			   define_softkeys(READ);

			   if(current > 0 ) {
			     define_softkeys(READ);

			     i = show_msg(current);
			     while (i)
			       i = process_showmsg_cmd(i);
			     redraw++;
			     if (++current > message_count)
			       current = message_count;
			     (void)get_page(current);
			   }else error ("No mail to read!");
			   break;

	    case 'o'    :  PutLine0(LINES-3, strlen("Command: "), "Options");
			   if((i=options()) > 0)
			     get_page(current);
			   else if(i < 0)
			     leave();
			   redraw++;	/* always fix da screen... */
			   break;

	    case 'p'    :  PutLine0(LINES-3, strlen("Command: "), "Print mail");
			   fflush(stdout);
			   if (message_count < 1) {
			     error("No mail to print!");
			     fflush(stdin);
			   }
			   else
			     print_msg();
			   break;

	    case 'q'    :  PutLine0(LINES-3, strlen("Command: "), "Quit");

			   if (mailfile_size != bytes(cur_folder)) {
			     error("New Mail!  Quit cancelled...");
			     fflush(stdin);
	  		     if (folder_type == SPOOL) unlock();
			   }
			   else
			     quit(TRUE);

			   break;

	    case 'Q'    :  PutLine0(LINES-3, strlen("Command: "), "Quick quit");

			   if (mailfile_size != bytes(cur_folder)) {
			     error("New Mail!  Quick Quit cancelled...");
	  		     if (folder_type == SPOOL) unlock();
			   }
			   else
			     quit(FALSE);

			   break;

	    case 'r'    :  PutLine0(LINES-3, strlen("Command: "),
			      "Reply to message");
			   if (current > 0)
			     redraw = reply();
			   else {
			     error("No mail to reply to!");
			     fflush(stdin);
			   }
			   softkeys_on();
			   break;

	    case '>'    : /** backwards compatibility **/

	    case 'C'	:
	    case 'S'	:
	    case 's'    :  if  (message_count < 1) {
			     error1("No mail to %s!",
			       ch != 'C' ? "save" : "copy");
			     fflush(stdin);
			   }
			   else {
			     PutLine1(LINES-3, strlen("Command: "),
				      "%s to folder",
				      ch != 'C' ? "Save" : "Copy");
			     PutLine0(LINES-3,COLUMNS-40,
				"(Use '?' to list your folders)");
			     if (save(&redraw, FALSE, ch)
				 && resolve_mode && ch != 'C') {
			       if((i=next_message(current-1, TRUE)) != -1) {
				 current = i+1;
				 nucurr = get_page(current);
			       }
			     }
			   }
			   ClearLine(LINES-2);
			   break;

            case ctrl('T') :
	    case 't'       :  if (message_count < 1) {
				error("No mail to tag!");
				fflush(stdin);
			      }
			      else if (ch == 't')
				tag_message(TRUE);
			      else
				meta_match(TAGGED);
			      break;

	    case 'u'    :  if (message_count < 1) {
			     error("No mail to mark as undeleted!");
			     fflush(stdin);
			   }
			   else {
			     undelete_msg(TRUE);
			     if (resolve_mode) 	/* move after mail resolved */
			       if((i=next_message(current-1, FALSE)) != -1) {
				 current = i+1;
				 nucurr = get_page(current);
			       }
/*************************************************************************
 **  What we've done here is to special case the "U)ndelete" command to
 **  ignore whether the next message is marked for deletion or not.  The
 **  reason is obvious upon usage - it's a real pain to undelete a series
 **  of messages without this quirk.  Thanks to Jim Davis @ HPLabs for
 **  suggesting this more intuitive behaviour.
 **
 **  The old way, for those people that might want to see what the previous
 **  behaviour was to call next_message with TRUE, not FALSE.
**************************************************************************/
			   }
			   break;

	    case ctrl('U') : if (message_count < 1) {
			       error("No mail to undelete!");
			       fflush(stdin);
			     }
			     else
			       meta_match(UNDELETE);
			     break;

	    case 'X'    :  PutLine0(LINES-3, strlen("Command: "), "Quick Exit");
                           fflush(stdout);
			   leave();
			   break;

	    case ctrl('Q') :
	    case 'x'    :  PutLine0(LINES-3, strlen("Command: "), "Exit");
                           fflush(stdout);
			   exit_prog();
			   break;

	    case ctrl('L') : redraw++;	break;

            case EOF :  leave();  /* Read failed, control tty died? */
                        break;

	    case '@'    : debug_screen();  redraw++;	break;

	    case '#'    : if (message_count) {
			    debug_message();
			    redraw++;
			  }
			  else {
			    error("No mail to check.");
			    fflush(stdin);
			  }
			  break;

	    case NO_OP_COMMAND : break;	/* noop for timeout loop */

	    case ESCAPE : if (cursor_control) {
			    key_offset = 1;
			    ch = ReadCh();

                            if ( ch == '[' || ch == 'O')
                            {
                              ch = ReadCh();
                              key_offset++;
                            }

			    if(ch == up[key_offset]) goto prev_undel_msg;
			    else if(ch == down[key_offset]) goto next_undel_msg;
			    else if(ch == right[key_offset]) goto next_page;
			    else if(ch == left[key_offset]) goto prev_page;
			    else if (hp_terminal) {
			      switch (ch) {
			      case 'U':		goto next_page;
			      case 'V':		goto prev_page;
			      case 'h':
			      case 'H':		goto first_msg;
			      case 'F':		goto last_msg;
			      case 'A':
			      case 'D':
			      case 'i':		goto next_undel_msg;
			      case 'B':
			      case 'I':
			      case 'C':		goto prev_undel_msg;
			      default: PutLine2(LINES-3, strlen("Command: "),
					"%c%c", ESCAPE, ch);
			      }
			    } else /* false hit - output */
			      PutLine2(LINES-3, strlen("Command: "),
					  "%c%c", ESCAPE, ch);
			  }

			  /* else fall into the default error message! */

	    default	: if (ch > '0' && ch <= '9') {
			    PutLine0(LINES-3, strlen("Command: "),
				    "New Current Message");
			    i = read_number(ch);

			    if( i > message_count)
			      error("Not that many messages.");
			    else if(selected
				&& isoff(headers[i-1]->status, VISIBLE))
			      error("Message not in limited display.");
			    else {
			      current = i;
			      nucurr = get_page(current);
			    }
			  }
			  else {
	 		    error("Unknown command. Use '?' for help.");
			    fflush(stdin);
			  }
	  }

	  if (redraw)
	    showscreen();

	  if ((current < 1) || (selected && compute_visible(current) < 1)) {
	    if (message_count > 0) {
	      /* We are out of range! Get to first message! */
	      if (selected)
		current = compute_visible(1);
	      else
		current = 1;
	    }
	    else
	      current = 0;
	  }
	  else if ((current > message_count)
	       || (selected && compute_visible(current) > selected)) {
	    if (message_count > 0) {
	      /* We are out of range! Get to last (visible) message! */
	      if (selected)
		current = visible_to_index(selected)+1;
	      else
		current = message_count;
	    }
	    else
	      current = 0;
	  }

	  if (nucurr == NEW_PAGE)
	    show_headers();
	  else if (nucurr == SAME_PAGE)
	    show_current();
	  else if (nufoot) {
	    if (mini_menu) {
	      MoveCursor(LINES-7, 0);
              CleartoEOS();
	      show_menu();
	    }
	    else {
	      MoveCursor(LINES-4, 0);
	      CleartoEOS();
	    }
	    show_last_error();	/* for those operations that have to
				 * clear the footer except for a message.
				 */
	  }

	} /* the BIG while loop! */
}

debug_screen()
{
	/**** spit out all the current variable settings and the table
	      entries for the current 'n' items displayed. ****/

	register int i, j;
	char     buffer[SLEN];

	ClearScreen();
	Raw(OFF);

	PutLine2(0,0,"Current message number = %d\t\t%d message(s) total\r\n",
		current, message_count);
	PutLine2(2,0,"Header_page = %d           \t\t%d possible page(s)\r\n",
		header_page, (int) (message_count / headers_per_page) + 1);

	PutLine1(4,0,"\r\nCurrent mailfile is %s.\n\r\n\r", cur_folder);

	i = header_page*headers_per_page;	/* starting header */

	if ((j = i + (headers_per_page-1)) >= message_count)
	  j = message_count-1;

	Write_to_screen(
"Num      From                  Subject                         Lines  Offset\n\r\n\r",0);

	while (i <= j) {
	   sprintf(buffer,
	   "%3d  %-16.16s  %-40.40s  %4d  %ld\n\r",
		    i+1,
		    headers[i]->from,
		    headers[i]->subject,
		    headers[i]->lines,
		    headers[i]->offset);
	    Write_to_screen(buffer, 0);
	  i++;
	}

	Raw(ON);

	PutLine0(LINES,0,"Press any key to return.");
	(void) ReadCh();
}


debug_message()
{
	/**** Spit out the current message record.  Include EVERYTHING
	      in the record structure. **/

	char buffer[SLEN];
	register struct header_rec *current_header = headers[current-1];

	ClearScreen();
	Raw(OFF);

	Write_to_screen("\t\t\t----- Message %d -----\n\r\n\r\n\r\n\r", 1,
		current);

	Write_to_screen("Lines : %-5d\t\t\tStatus: A  C  D  E  F  N  O  P  T  U  V\n\r", 1,
		current_header->lines);
	Write_to_screen("            \t\t\t        c  o  e  x  o  e  l  r  a  r  i\n\r", 0);
	Write_to_screen("            \t\t\t        t  n  l  p  r  w  d  i  g  g  s\n\r", 0);
	Write_to_screen("            \t\t\t        n  f  d  d  m        v  d  n  i\n\r", 0);

	sprintf(buffer,
		"\n\rOffset: %ld\t\t\t        %d  %d  %d  %d  %d",
		current_header->offset,
		(current_header->status & ACTION) != 0,
		(current_header->status & CONFIDENTIAL) != 0,
		(current_header->status & DELETED) != 0,
		(current_header->status & EXPIRED) != 0,
		(current_header->status & FORM_LETTER) != 0);
	sprintf(buffer + strlen(buffer),
		"  %d  %d  %d  %d  %d  %d\n",
		(current_header->status & NEW) != 0,
		(current_header->status & UNREAD) != 0,
		(current_header->status & PRIVATE) != 0,
		(current_header->status & TAGGED) != 0,
		(current_header->status & URGENT) != 0,
		(current_header->status & VISIBLE) != 0);

	Write_to_screen(buffer, 0);

	sprintf(buffer, "\n\rReceived on: %d/%d/%d at %d:%02d\n\r",
		current_header->received.month+1,
		current_header->received.day,
		current_header->received.year,
		current_header->received.hour,
		current_header->received.minute);
	Write_to_screen(buffer, 0);

	sprintf(buffer, "Message sent on: %s, %s %s, %s at %s\n\r",
		current_header->dayname,
		current_header->month,
		current_header->day,
		current_header->year,
		current_header->time);
	Write_to_screen(buffer, 0);

	Write_to_screen("From: %s\n\rSubject: %s", 2,
		current_header->from,
		current_header->subject);

	Write_to_screen("\n\rPrimary Recipient: %s\nInternal Index Reference Number = %d\n\r", 2,
		current_header->to,
		current_header->index_number);

	Write_to_screen("Message-ID: %s\n\r", 1,
		strlen(current_header->messageid) > 0 ?
		current_header->messageid : "<none>");

	Write_to_screen("Status: %s\n\r", 1, current_header->mailx_status);

	Raw(ON);

	PutLine0(LINES,0,"Please Press any key to return.");
	(void) ReadCh();
}

do_check_only(to_whom)
char *to_whom;
	{
	char buffer[VERY_LONG_STRING];

	dprint(3, (debugfile, "Check-only: checking \n-> \"%s\"\n",
		format_long(to_whom, 3)));
	(void) build_address(strip_commas(to_whom), buffer);
	printf("\r\nExpands to: %s\r\n", format_long(buffer, strlen("Expands to: ")));
	}
