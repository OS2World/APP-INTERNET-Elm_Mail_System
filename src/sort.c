
static char rcsid[] = "@(#)$Id: sort.c,v 4.1 90/04/28 22:44:12 syd Exp $";

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
 * $Log:	sort.c,v $
 * Revision 4.1  90/04/28  22:44:12  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Sort folder header table by the field specified in the global
    variable "sortby"...if we're sorting by something other than
    the default SENT_DATE, also put some sort of indicator on the
    screen.

**/

#include "headers.h"

char *sort_name(), *skip_re();
void   qsort();

sort_mailbox(entries, visible)
int entries, visible;
{
	/** Sort the header_table definitions... If 'visible', then
	    put the status lines etc **/

	int last_index = -1;
	int compare_headers();	/* for sorting */

	dprint(2, (debugfile, "\n** sorting folder by %s **\n\n",
		sort_name(FULL)));

	/* Don't get last_index if no entries or no current. */
	/* There would be no current if we are sorting a new mail file. */
	if (entries > 0 && current > 0)
	  last_index = headers[current-1]->index_number;

	if (entries > 30 && visible)
	  error1("Sorting messages by %s...", sort_name(FULL));

	if (entries > 1)
	  qsort(headers, (unsigned) entries, sizeof (struct header_rec *),
	      compare_headers);

	if (last_index > -1)
	  find_old_current(last_index);

	clear_error();
}

int
compare_headers(p1, p2)
struct header_rec **p1, **p2;
{
	/** compare two headers according to the sortby value.

	    Sent Date uses a routine to compare two dates,
	    Received date is keyed on the file offsets (think about it)
	    Sender uses the truncated from line, same as "build headers",
	    and size and subject are trivially obvious!!
	    (actually, subject has been modified to ignore any leading
	    patterns [rR][eE]*:[ \t] so that replies to messages are
	    sorted with the message (though a reply will always sort to
	    be 'greater' than the basenote)
	 **/

	char from1[SLEN], from2[SLEN];	/* sorting buffers... */
	struct header_rec *first, *second;
	int ret;
	long diff;

	first = *p1;
	second = *p2;

	switch (abs(sortby)) {
	case SENT_DATE:
 		diff = first->time_sent - second->time_sent;
 		if ( diff < 0 )	ret = -1;
 		else if ( diff > 0 ) ret = 1;
 		else ret = 0;
  		break;

	case RECEIVED_DATE:
		ret = compare_parsed_dates(first->received, second->received);
		break;

	case SENDER:
		tail_of(first->from, from1, first->to);
		tail_of(second->from, from2, second->to);
		ret = strcmp(from1, from2);
		break;

	case SIZE:
		ret = (first->lines - second->lines);
		break;

	case MAILBOX_ORDER:
		ret = (first->index_number - second->index_number);
		break;

	case SUBJECT:
		/* need some extra work 'cause of STATIC buffers */
		strcpy(from1, skip_re(shift_lower(first->subject)));
		ret = strcmp(from1, skip_re(shift_lower(second->subject)));
		break;

	case STATUS:
		ret = (first->status - second->status);
		break;

	default:
		/* never get this! */
		ret = 0;
		break;
	}

	if (sortby < 0)
	  ret = -ret;

	return ret;
}

char *sort_name(type)
int type;
{
	/** return the name of the current sort option...
	    type can be "FULL", "SHORT" or "PAD"
	**/
	int pad, abr;

	pad = (type == PAD);
	abr = (type == SHORT);

	if (sortby < 0) {
	  switch (- sortby) {
	    case SENT_DATE    : return(
		              pad?     "Reverse Date Mail Sent  " :
			      abr?     "Reverse-Sent" :
				       "Reverse Date Mail Sent");
	    case RECEIVED_DATE: return(
			      abr?     "Reverse-Received":
			      "Reverse Date Mail Rec'vd" );

	    case MAILBOX_ORDER: return(
			      pad?     "Reverse Mailbox Order   " :
			      abr?     "Reverse-Mailbox":
			               "Reverse Mailbox Order");

	    case SENDER       : return(
			      pad?     "Reverse Message Sender  " :
			      abr?     "Reverse-From":
				       "Reverse Message Sender");
	    case SIZE         : return(
			      abr?     "Reverse-Lines" :
				       "Reverse Lines in Message");
	    case SUBJECT      : return(
			      pad?     "Reverse Message Subject " :
			      abr?     "Reverse-Subject" :
				       "Reverse Message Subject");
	    case STATUS	      : return(
			      pad?     "Reverse Message Status  " :
			      abr?     "Reverse-Status":
			               "Reverse Message Status");
	  }
	}
	else {
	  switch (sortby) {
	    case SENT_DATE    : return(
		                pad?   "Date Mail Sent          " :
		                abr?   "Sent" :
				       "Date Mail Sent");
	    case RECEIVED_DATE: return(
	                        pad?   "Date Mail Rec'vd        " :
	                        abr?   "Received" :
				       "Date Mail Rec'vd");
	    case MAILBOX_ORDER: return(
	                        pad?   "Mailbox Order           " :
	                        abr?   "Mailbox" :
	                               "Mailbox Order");
	    case SENDER       : return(
			        pad?   "Message Sender          " :
			        abr?   "From" :
				       "Message Sender");
	    case SIZE         : return(
	    			pad?   "Lines in Message        " :
	    			abr?   "Lines" :
	    			       "Lines in Message");
	    case SUBJECT      : return(
			        pad?   "Message Subject         " :
			        abr?   "Subject" :
				       "Message Subject");
	    case STATUS	      : return(
			        pad?   "Message Status          " :
			        abr?   "Status" :
			               "Message Status");
	  }
	}

	return("*UNKNOWN-SORT-PARAMETER*");
}

find_old_current(iindex)
int iindex;
{
	/** Set current to the message that has "index" as it's
	    index number.  This is to track the current message
	    when we resync... **/

	register int i;

	dprint(4, (debugfile, "find-old-current(%d)\n", iindex));

	for (i = 0; i < message_count; i++)
	  if (headers[i]->index_number == iindex) {
	    current = i+1;
	    dprint(4, (debugfile, "\tset current to %d!\n", current));
	    return;
	  }

	dprint(4, (debugfile,
		"\tcouldn't find current index.  Current left as %d\n",
		current));
	return;		/* can't be found.  Leave it alone, then */
}

char *skip_re(string)
char *string;
{
	/** this routine returns the given string minus any sort of
	    "re:" prefix.  specifically, it looks for, and will
	    remove, any of the pattern:

		( [Rr][Ee][^:]:[ ] ) *

	    If it doesn't find a ':' in the line it will return it
	    intact, just in case!
	**/

	static char buffer[SLEN];
	register int i=0;

	while (whitespace(string[i])) i++;

	do {
	  if (string[i] == '\0') return( (char *) string);	/* forget it */

	  if (string[i] != 'r' || string[i+1] != 'e')
	    return( (char *) string);				/*   ditto   */

	  i += 2;	/* skip the "re" */

	  while (string[i] != ':')
	    if (string[i] == '\0')
	      return( (char *) string);		      /* no colon in string! */
	    else
	      i++;

	  /* now we've gotten to the colon, skip to the next non-whitespace  */

	  i++;	/* past the colon */

	  while (whitespace(string[i])) i++;

	} while (string[i] == 'r' && string[i+1] == 'e');

	/* and now copy it into the buffer and sent it along... */

	strcpy(buffer, (char *) string + i);

	return( (char *) buffer);
}
