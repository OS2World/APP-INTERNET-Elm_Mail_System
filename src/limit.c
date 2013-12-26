
static char rcsid[] = "@(#)$Id: limit.c,v 4.1 90/04/28 22:43:21 syd Exp $";

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
 * $Log:	limit.c,v $
 * Revision 4.1  90/04/28  22:43:21  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This stuff is inspired by MH and dmail and is used to 'select'
    a subset of the existing mail in the folder based on one of a
    number of criteria.  The basic tricks are pretty easy - we have
    as status of VISIBLE associated with each header stored in the
    (er) mind of the computer (!) and simply modify the commands to
    check that flag...the global variable `selected' is set to the
    number of messages currently selected, or ZERO if no select.
**/

#include "headers.h"

#define TO		1
#define FROM		2

char *shift_lower();

int
limit()
{
	/** returns non-zero if we changed selection criteria = need redraw **/

	char criteria[STRING], first[STRING], rest[STRING], msg[STRING];
	static char prompt[] = "Enter criteria or '?' for help: ";
	int  last_selected, all;

	last_selected = selected;
	all = 0;

	if (selected) {
	  PutLine1(LINES-2, 0,
		"Already have selection criteria - add more? (y/n) n%c",
		BACKSPACE);
	  criteria[0] = ReadCh();
	  if (tolower(criteria[0]) == 'y') {
	    Write_to_screen("Yes.", 0);
	    PutLine0(LINES-3, COLUMNS-30, "Adding criteria...");
	  } else {
	    Write_to_screen("No.", 0);
	    selected = 0;
	    PutLine0(LINES-3, COLUMNS-30, "Change criteria...");
	  }
	}

	while(1) {
	  PutLine1(LINES-2, 0, prompt);
	  CleartoEOLN();

	  criteria[0] = '\0';
	  optionally_enter(criteria, LINES-2, strlen(prompt), FALSE, FALSE);
	  error("");

	  if (strlen(criteria) == 0) {
	    /* no change */
	    selected = last_selected;
	    return(FALSE);
	  }

	  split_word(criteria, first, rest);

	  if (equal(first, "?")) {
	     if(last_selected)
	       error(
	          "Enter: {\"subject\",\"to\",\"from\"} [pattern] OR \"all\"");
	     else
	       error("Enter: {\"subject\",\"to\",\"from\"} [pattern]");
	     continue;
	  } else if (equal(first, "all")) {
	    all++;
	    selected = 0;
	  }
	  else if (equal(first, "subj") || equal(first, "subject"))
	    selected = limit_selection(SUBJECT, rest, selected);
	  else if (equal(first, "to"))
	    selected = limit_selection(TO, rest, selected);
	  else if (equal(first, "from"))
	    selected = limit_selection(FROM, rest, selected);
	  else {
	    error1("\"%s\" not a valid criterion.", first);
	    continue;
	  }
	  break;
	}

	if(all && last_selected)
	  strcpy(msg, "Returned to unlimited display.");
	else if(selected)
	  sprintf(msg, "%d message%s selected.", selected, plural(selected));
	else
	  strcpy(msg, "No messages selected.");
	set_error(msg);

	/* we need a redraw if there had been a selection or there is now. */
	if(last_selected || selected) {
	  /* if current message won't be on new display, go to first message */
	  if(selected && !(headers[current-1]->status & VISIBLE))
	    current = visible_to_index(1)+1;
	  return(TRUE);
	} else {
	  return(FALSE);
	}
}

int
limit_selection(based_on, pattern, additional_criteria)
int based_on, additional_criteria;
char *pattern;
{
	/** Given the type of criteria, and the pattern, mark all
	    non-matching headers as ! VISIBLE.  If additional_criteria,
	    don't mark as visible something that isn't currently!
	**/

	register int iindex, count = 0;

	dprint(2, (debugfile, "\n\n\n**limit on %d - '%s' - (%s) **\n\n",
		   based_on, pattern, additional_criteria?"add'tl":"base"));

	if (based_on == SUBJECT) {
	  for (iindex = 0; iindex < message_count; iindex++)
	    if (! in_string(shift_lower(headers[iindex]->subject), pattern))
	      headers[iindex]->status &= ~VISIBLE;
	    else if (additional_criteria &&
		     !(headers[iindex]->status & VISIBLE))
	      headers[iindex]->status &= ~VISIBLE;	/* shut down! */
	    else { /* mark it as readable */
	      headers[iindex]->status |= VISIBLE;
	      count++;
	      dprint(5, (debugfile,
		     "  Message %d (%s from %s) marked as visible\n",
			iindex, headers[iindex]->subject,
			headers[iindex]->from));
	    }
	}
	else if (based_on == FROM) {
	  for (iindex = 0; iindex < message_count; iindex++)
	    if (! in_string(shift_lower(headers[iindex]->from), pattern))
	      headers[iindex]->status &= ~VISIBLE;
	    else if (additional_criteria &&
		     !(headers[iindex]->status & VISIBLE))
	      headers[iindex]->status &= ~VISIBLE;	/* shut down! */
	    else { /* mark it as readable */
	      headers[iindex]->status |= VISIBLE;
	      count++;
	      dprint(5, (debugfile,
			"  Message %d (%s from %s) marked as visible\n",
			iindex, headers[iindex]->subject,
			headers[iindex]->from));
	    }
	}
	else if (based_on == TO) {
	  for (iindex = 0; iindex < message_count; iindex++)
	    if (! in_string(shift_lower(headers[iindex]->to), pattern))
	      headers[iindex]->status &= ~VISIBLE;
	    else if (additional_criteria &&
		     !(headers[iindex]->status & VISIBLE))
	      headers[iindex]->status &= ~VISIBLE;	/* shut down! */
	    else { /* mark it as readable */
	      headers[iindex]->status |= VISIBLE;
	      count++;
	      dprint(5, (debugfile,
			"  Message %d (%s from %s) marked as visible\n",
			iindex, headers[iindex]->subject,
			headers[iindex]->from));
	    }
	}

	dprint(4, (debugfile, "\n** returning %d selected **\n\n\n", count));

	return(count);
}

int
next_message(iindex, skipdel)
register int iindex, skipdel;
{
	/** Given 'iindex', this routine will return the actual iindex into the
	    array of the NEXT message, or '-1' iindex is the last.
	    If skipdel, return the iindex for the NEXT undeleted message.
	    If selected, return the iindex for the NEXT message marked VISIBLE.
	**/

	register int remember_for_debug;

	if(iindex < 0) return(-1);	/* invalid argument value! */

	remember_for_debug = iindex;

	for(iindex++;iindex < message_count; iindex++)
	  if (((headers[iindex]->status & VISIBLE) || (!selected))
	    && (!(headers[iindex]->status & DELETED) || (!skipdel))) {
	      dprint(9, (debugfile, "[Next%s%s: given %d returning %d]\n",
		  (skipdel ? " undeleted" : ""),
		  (selected ? " visible" : ""),
		  remember_for_debug+1, iindex+1));
	      return(iindex);
	  }
	return(-1);
}

int
prev_message(iindex, skipdel)
register int iindex, skipdel;
{
	/** Like next_message, but the PREVIOUS message. **/

	register int remember_for_debug;

	if(iindex >= message_count) return(-1);	/* invalid argument value! */

	remember_for_debug = iindex;
	for(iindex--; iindex >= 0; iindex--)
	  if (((headers[iindex]->status & VISIBLE) || (!selected))
	    && (!(headers[iindex]->status & DELETED) || (!skipdel))) {
	      dprint(9, (debugfile, "[Previous%s%s: given %d returning %d]\n",
		  (skipdel ? " undeleted" : ""),
		  (selected ? " visible" : ""),
		  remember_for_debug+1, iindex+1));
	      return(iindex);
	  }
	return(-1);
}


int
compute_visible(message)
int message;
{
	/** return the 'virtual' iindex of the specified message in the
	    set of messages - that is, if we have the 25th message as
	    the current one, but it's #2 based on our limit criteria,
	    this routine, given 25, will return 2.
	**/

	register int iindex, count = 0;

	if (! selected) return(message);

	if (message < 1) message = 1;	/* normalize */

	for (iindex = 0; iindex < message; iindex++)
	   if (headers[iindex]->status & VISIBLE)
	     count++;

	dprint(4, (debugfile,
		"[compute-visible: displayed message %d is actually %d]\n",
		count, message));

	return(count);
}

int
visible_to_index(message)
int message;
{
	/** Given a 'virtual' iindex, return a real one.  This is the
	    flip-side of the routine above, and returns (message_count+1)
	    if it cannot map the virtual iindex requested (too big)
	**/

	register int iindex = 0, count = 0;

	for (iindex = 0; iindex < message_count; iindex++) {
	   if (headers[iindex]->status & VISIBLE)
	     count++;
	   if (count == message) {
	     dprint(4, (debugfile,
		     "visible-to-index: (up) index %d is displayed as %d\n",
		     message, iindex));
	     return(iindex);
	   }
	}

	dprint(4, (debugfile, "index %d is NOT displayed!\n", message));

	return(message_count+1);
}
