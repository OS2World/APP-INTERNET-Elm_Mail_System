
static char rcsid[] = "@(#)$Id: delete.c,v 4.1 90/04/28 22:42:43 syd Exp $";

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
 * $Log:	delete.c,v $
 * Revision 4.1  90/04/28  22:42:43  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/**  Delete or undelete files: just set flag in header record!
     Also tags specified message(s)...

**/

#include "headers.h"

char *show_status();

delete_msg(real_del, update_screen)
int real_del, update_screen;
{
	/** Delete current message.  If real-del is false, then we're
	    actually requested to toggle the state of the current
	    message... **/

	if (real_del)
	  headers[current-1]->status |= DELETED;
	else if (ison(headers[current-1]->status, DELETED))
	  clearit(headers[current-1]->status, DELETED);
	else
	  setit(headers[current-1]->status, DELETED);

	if (update_screen)
	  show_msg_status(current-1);
}

undelete_msg(update_screen)
int update_screen;
{
	/** clear the deleted message flag **/

	clearit(headers[current-1]->status, DELETED);

	if (update_screen)
	  show_msg_status(current-1);
}

show_msg_status(msg)
int msg;
{
	/** show the status of the current message only.  **/

	char tempbuf[3];

	strcpy(tempbuf, show_status(headers[msg]->status));

	if (on_page(msg)) {
	  MoveCursor(((compute_visible(msg+1)-1) % headers_per_page) + 4, 2);
	  if (msg+1 == current && !arrow_cursor) {
	    StartBold();
	    Writechar( tempbuf[0] );
	    EndBold();
	  }
	  else
	    Writechar( tempbuf[0] );
	}
}

int
tag_message(update_screen)
int update_screen;
{
	/** Tag current message and return TRUE.
	    If already tagged, untag it and return FALSE. **/

	int istagged;

	if (ison(headers[current-1]->status, TAGGED)) {
	  clearit(headers[current-1]->status, TAGGED);
	  istagged = FALSE;
	} else {
	  setit(headers[current-1]->status, TAGGED);
	  istagged = TRUE;
	}

	if(update_screen)
	    show_msg_tag(current-1);
	return(istagged);
}

show_msg_tag(msg)
int msg;
{
	/** show the tag status of the current message only.  **/

	if (on_page(msg)) {
	  MoveCursor(((compute_visible(msg+1)-1) % headers_per_page) + 4, 4);
	  if (msg+1 == current && !arrow_cursor) {
	    StartBold();
	    Writechar( ison(headers[msg]->status, TAGGED)? '+' : ' ');
	    EndBold();
	  }
	  else
	    Writechar( ison(headers[msg]->status, TAGGED)? '+' : ' ');
	}
}

show_new_status(msg)
int msg;
{
	/** If the specified message is on this screen, show
	    the new status (could be marked for deletion now,
	    and could have tag removed...)
	**/

	if (on_page(msg))
	  if (msg+1 == current && !arrow_cursor) {
	    StartBold();
	    PutLine2(((compute_visible(msg+1)-1) % headers_per_page) + 4,
		   2, "%s%c", show_status(headers[msg]->status),
		   ison(headers[msg]->status, TAGGED )? '+' : ' ');
	    EndBold();
	  }
	  else
	    PutLine2(((compute_visible(msg+1)-1) % headers_per_page) + 4,
		   2, "%s%c", show_status(headers[msg]->status),
		   ison(headers[msg]->status, TAGGED )? '+' : ' ');
}
