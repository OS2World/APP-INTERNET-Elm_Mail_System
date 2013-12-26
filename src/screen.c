
static char rcsid[] = "@(#)$Id: screen.c,v 4.1 90/04/28 22:44:04 syd Exp $";

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
 * $Log:	screen.c,v $
 * Revision 4.1  90/04/28  22:44:04  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/**  screen display routines for ELM program

**/

#include "headers.h"

#define  minimum(a,b)	((a) < (b) ? (a) : (b))

static   int  last_current	 = -1;

char *strcpy(), *strncpy(), *nameof(), *show_status(), *index();

extern char version_buff[];

showscreen()
{

	ClearScreen();

	update_title();

	last_header_page = -1;	 	/* force a redraw regardless */
	show_headers();

	if (mini_menu)
	  show_menu();

	show_last_error();

	if (hp_terminal)
	  define_softkeys(MAIN);
}

update_title()
{
	/** display a new title line, probably due to new mail arriving **/

	char buffer[SLEN];

	if (selected)
	  sprintf(buffer,
	      "%s is '%s' with %d shown out of %d [ELM %s]",
	      (folder_type == SPOOL ? "Mailbox" : "Folder"),
	      nameof(cur_folder), selected, message_count, version_buff);
	else
	  sprintf(buffer, "%s is '%s' with %d message%s [ELM %s]",
	      (folder_type == SPOOL ? "Mailbox" : "Folder"),
	      nameof(cur_folder), message_count,
	      plural(message_count), version_buff);

	ClearLine(1);

	Centerline(1, buffer);
}

show_menu()
{
	/** write main system menu... **/

	if (user_level == 0) {	/* a rank beginner.  Give less options  */
	  Centerline(LINES-7,
  "You can use any of the following commands by pressing the first character;");
          Centerline(LINES-6,
"d)elete or u)ndelete mail,  m)ail a message,  r)eply or f)orward mail,  q)uit");
	  Centerline(LINES-5,
  "To read a message, press <return>.  j = move down, k = move up, ? = help");
	} else {
	Centerline(LINES-7,
  "|=pipe, !=shell, ?=help, <n>=set current to n, /=search pattern");
        Centerline(LINES-6,
"a)lias, C)opy, c)hange folder, d)elete, e)dit, f)orward, g)roup reply, m)ail,"
);
	Centerline(LINES-5,
  "n)ext, o)ptions, p)rint, q)uit, r)eply, s)ave, t)ag, u)ndelete, or e(x)it");
	}
}

int
show_headers()
{
	/** Display page of headers (10) if present.  First check to
	    ensure that header_page is in bounds, fixing silently if not.
	    If out of bounds, return zero, else return non-zero
	    Modified to only show headers that are "visible" to ze human
	    person using ze program, eh?
	**/

	register int this_msg = 0, line = 4, last = 0, last_line,
		     displayed = 0, using_to;
	char newfrom[SLEN], buffer[SLEN];

	if (fix_header_page())
	  return(FALSE);

	if (selected) {
	  if ((header_page*headers_per_page) > selected)
	    return(FALSE); 	/* too far! too far! */

	  this_msg = visible_to_index(header_page * headers_per_page + 1);
	  displayed = header_page * headers_per_page;

	  last = displayed+headers_per_page;

	}
	else {
	  if (header_page == last_header_page) 	/* nothing to do! */
	    return(FALSE);

	  /** compute last header to display **/

	  this_msg = header_page * headers_per_page;
	  last = this_msg + (headers_per_page - 1);
	}

	if (last >= message_count) last = message_count-1;

	/** Okay, now let's show the header page! **/

	ClearLine(line);	/* Clear the top line... */

	MoveCursor(line, 0);	/* and move back to the top of the page... */

	while ((selected && displayed < last) || this_msg <= last) {
	  using_to = tail_of(headers[this_msg]->from, newfrom,
	    headers[this_msg]->to);

	  if (this_msg == current-1)
	    build_header_line(buffer, headers[this_msg], this_msg+1,
			    TRUE, newfrom, using_to);
	  else
	    build_header_line(buffer, headers[this_msg],
			    this_msg+1, FALSE, newfrom, using_to);
	  if (selected)
	    displayed++;

	  if (this_msg == current-1 && has_highlighting && ! arrow_cursor) {
	      StartInverse();
	      Write_to_screen("%s\n\r", 1, buffer);	/* avoid '%' probs */
	      EndInverse();
	  } else
	      Write_to_screen("%s\n\r", 1, buffer);	/* avoid '%' probs */
	  CleartoEOLN();
	  line++;		/* for clearing up in a sec... */

	  if (selected) {
	    if ((this_msg = next_message(this_msg, FALSE)) < 0)
	      break;	/* GET OUTTA HERE! */

	    /* the preceeding looks gross because we're using an INDEX
	       variable to pretend to be a "current" counter, and the
	       current counter is always 1 greater than the actual
	       index.  Does that make sense??
	     */
	  }
	  else
	    this_msg++;					/* even dumber...  */
	}

	/* clear unused lines */

	if (mini_menu)
	  last_line = LINES-8;
	else
	  last_line = LINES-4;

	while (line < last_line) {
	  CleartoEOLN();
	  NewLine();
	  line++;
	}

	display_central_message();

	last_current = current;
	last_header_page = header_page;

	return(TRUE);
}

show_current()
{
	/** Show the new header, with all the usual checks **/

	register int first = 0, last = 0, last_line, new_line, using_to;
	char     newfrom[SLEN], old_buffer[SLEN], new_buffer[SLEN];

	(void) fix_header_page();	/* Who cares what it does? ;-) */

	/** compute the first and last header on this page **/
	first = header_page * headers_per_page + 1;
	last  = first + (headers_per_page - 1);

	/* if not a full page adjust last to be the real last */
	if (selected && last > selected)
	  last = selected;
	if (!selected && last > message_count)
	  last = message_count;

	/** okay, now let's show the pointers... **/

	/** have we changed??? **/
	if (current == last_current)
	  return;

	if (selected) {
	  last_line = ((compute_visible(last_current)-1) %
			 headers_per_page)+4;
	  new_line  = ((compute_visible(current)-1) % headers_per_page)+4;
	} else {
	  last_line = ((last_current-1) % headers_per_page)+4;
	  new_line  = ((current-1) % headers_per_page)+4;
	}

	if (has_highlighting && ! arrow_cursor) {

	  using_to = tail_of(headers[current-1]->from, newfrom,
	    headers[current-1]->to);
	  build_header_line(new_buffer, headers[current-1],  current,
		  TRUE, newfrom, using_to);

	  /* clear last current if it's in proper range */
	  if (last_current > 0		/* not a dummy value */
	      && compute_visible(last_current) <= last
	      && compute_visible(last_current) >= first) {

	    dprint(5, (debugfile,
		  "\nlast_current = %d ... clearing [1] before we add [2]\n",
		   last_current));
	    dprint(5, (debugfile, "first = %d, and last = %d\n\n",
		  first, last));

	    using_to = tail_of(headers[last_current-1]->from, newfrom,
	      headers[last_current-1]->to);
	    build_header_line(old_buffer, headers[last_current-1],
		 last_current, FALSE, newfrom, using_to);

	    ClearLine(last_line);
	    PutLine0(last_line, 0, old_buffer);
	  }
	  MoveCursor(new_line, 0);
	  StartInverse();
	  Write_to_screen("%s", 1, new_buffer);
	  EndInverse();
	}
	else {
	  if (on_page(last_current-1))
	    PutLine0(last_line,0,"  ");	/* remove old pointer... */
	  if (on_page(current-1))
	    PutLine0(new_line, 0,"->");
	}

	last_current = current;
}

build_header_line(buffer, entry, message_number, highlight, from, really_to)
char *buffer;
struct header_rec *entry;
int message_number, highlight, really_to;
char *from;
{
	/** Build in buffer the message header ... entry is the current
	    message entry, 'from' is a modified (displayable) from line,
	    'highlight' is either TRUE or FALSE, and 'message_number'
	    is the number of the message.
	**/

	/** Note: using 'strncpy' allows us to output as much of the
	    subject line as possible given the dimensions of the screen.
	    The key is that 'strncpy' returns a 'char *' to the string
	    that it is handing to the dummy variable!  Neat, eh? **/

	int who_width = 18, subj_width;
	char *dot = index(from, '.');
	char *bang = index(from, '!');

	/* truncate 'from' to 18 characters -
	 * this includes the leading "To" if really_to is true.
	 * Note:
	 *	'from' is going to be of three forms
	 *		- full name (truncate on the right for readability)
	 *		- logname@machine (truncate on the right to preserve
	 *			logname over machine name
	 *		- machine!logname -- a more complex situation
	 *			If this form doesn't fit, either machine
	 *			or logname are long. If logname is long,
	 *			we can stand to loose part of it, so we
	 *			truncate on the right. If machine name is
	 *			long, we'd better truncate on the left,
	 *			to insure we get the logname. Now if the
	 *			machine name is long, it will have "." in
	 *			it.
	 *	Therfore, we truncate on the left if there is a "." and a "!"
	 *	in 'from', else we truncate on the right.
	 */

	/* Note that one huge sprintf() is too hard for some compilers. */
	sprintf(buffer, "%s%s%c%-3d %3.3s %-2d ",
		(highlight && arrow_cursor)? "->" : "  ",
		show_status(entry->status),
		(entry->status & TAGGED?  '+' : ' '),
	        message_number,
	        entry->month,
		atoi(entry->day));

	/* show "To " in a way that it can never be truncated. */
	if (really_to) {
	  strcat(buffer, "To ");
	  who_width -= 3;
	}

	/* truncate 'from' on left if needed.
	 * sprintf will truncate on right afterward if needed. */
	if ((strlen(from) > who_width) && dot && bang && (dot < bang)) {
	  from += (strlen(from) - who_width);
	}

	/* Set the subject display width.
	 * If it is too long, truncate it to fit.
	 * If it is highlighted but not with the arrow  cursor,
	 * expand it to fit so that the reverse video bar extends
	 * aesthetically the full length of the line.
	 */
	if ((highlight && !arrow_cursor)
		|| (COLUMNS-44 < (subj_width =strlen(entry->subject))))
	    subj_width = COLUMNS-44;

	/* complete line with sender, length and subject. */
	sprintf(buffer + strlen(buffer), "%-*.*s (%d) %s%-*.*s",
		/* give max and min width parameters for 'from' */
		who_width,
		who_width,
		from,

		entry->lines,
		(entry->lines / 1000   > 0? ""   :	/* spacing the  */
		  entry->lines / 100   > 0? " "  :	/* same for the */
		    entry->lines / 10  > 0? "  " :	/* lines in ()  */
		                            "   "),     /*   [wierd]    */

		subj_width, subj_width, entry->subject);
}

int
fix_header_page()
{
	/** this routine will check and ensure that the current header
	    page being displayed contains messages!  It will silently
	    fix 'header-page' if wrong.  Returns TRUE if changed.  **/

	int last_page, old_header;

	old_header = header_page;

	last_page = (int) ((message_count-1) / headers_per_page);

	if (header_page > last_page)
	  header_page = last_page;
	else if (header_page < 0)
          header_page = 0;

	return(old_header != header_page);
}

int
on_page(message)
int message;
{
	/** Returns true iff the specified message is on the displayed page. **/

	if (selected) message = compute_visible(message);

	if (message >= header_page * headers_per_page)
	  if (message < ((header_page+1) * headers_per_page))
	    return(TRUE);

	return(FALSE);
}

char *show_status(status)
int status;
{
	/** This routine returns a pair of characters indicative of
	    the status of this message.  The first character represents
	    the interim status of the message (e.g. the status within
	    the mail system):

		E = Expired message
		N = New message
		O = Unread old message	dsi mailx emulation addition
		D = Deleted message
		_ = (space) default

	    and the second represents the permanent attributes of the
	    message:

		C = Company Confidential message
	        U = Urgent (or Priority) message
		P = Private message
		A = Action associated with message
		F = Form letter
		_ = (space) default
	**/

	static char mybuffer[3];

	/** the first character, please **/

	     if (status & DELETED)	mybuffer[0] = 'D';
	else if (status & EXPIRED)	mybuffer[0] = 'E';
	else if (status & NEW)		mybuffer[0] = 'N';
	else if (status & UNREAD)	mybuffer[0] = 'O';
	else                            mybuffer[0] = ' ';

	/** and the second... **/

	     if (status & CONFIDENTIAL) mybuffer[1] = 'C';
	else if (status & URGENT)       mybuffer[1] = 'U';
	else if (status & PRIVATE)      mybuffer[1] = 'P';
	else if (status & ACTION)       mybuffer[1] = 'A';
	else if (status & FORM_LETTER)  mybuffer[1] = 'F';
	else 			        mybuffer[1] = ' ';

	mybuffer[2] = '\0';

	return( (char *) mybuffer);
}
