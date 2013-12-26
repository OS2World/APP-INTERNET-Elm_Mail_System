
static char rcsid[] = "@(#)$Id: builtin.c,v 4.1 90/04/28 22:42:34 syd Exp $";

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
 * $Log:	builtin.c,v $
 * Revision 4.1  90/04/28  22:42:34  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This is the built-in pager for displaying messages while in the Elm
    program.  It's a bare-bones pager with precious few options. The idea
    is that those systems that are sufficiently slow that using an external
    pager such as 'more' is too slow, then they can use this!

    Also added support for the "builtin+" pager (clears the screen for
    each new page) including a two-line overlap for context...

**/

#include "headers.h"
#include <ctype.h>

#define  BEEP		007		/* ASCII Bell character */

static	unfilled_lines,
	form_title;

int	lines_displayed,	    /* total number of lines displayed      */
	total_lines_to_display,	    /* total number of lines in message     */
	pages_displayed; 	    /* for the nth page titles and all      */

start_builtin(lines_in_message)
int lines_in_message;
{
	/** clears the screen and resets the internal counters... **/

	dprint(8,(debugfile,
		"displaying %d lines from message using internal pager\n",
		lines_in_message));

	unfilled_lines = LINES;
	form_title = 1;
	lines_displayed = 0;
        pages_displayed = 1;

	total_lines_to_display = lines_in_message;
}

extern int tabspacing;

int
next_line(inputptr, output, width)
char **inputptr, *output;
register unsigned width;
{
	/* Copy characters from input to output and copy
	 * remainder of output to output. In copying use ^X notation for
	 * control characters, '?' non-ascii characters, expand tabs
	 * to correct number of spaces till next tab stop.
	 * Column zero of the next line is considered to be the tab stop
	 * that follows the last one that fits on a line.
	 * Copy until newline/return encountered, null char encountered,
	 * width characters producted in output buffer.
	 * Formfeed is handled exceptionally. If encountered it
	 * is removed from input and 1 is returned. Otherwise 0 is returned.
	 */

	register unsigned char *optr, *iptr;
	register unsigned chars_output, nt;
	int ret_val;

	optr = output;
	iptr = *inputptr;
	chars_output = 0;

	ret_val = 0;	/* presume no formfeed */
	while(1) {

	  if(chars_output >= width) {		/* no more room on line */
	    *optr++ = '\n';
	    *optr++ = '\r';
	    /* if next input character is newline or return,
	     * we can skip over it since we are outputing a newline anyway */
	    if((*iptr == '\n') || (*iptr == '\r'))
	      iptr++;
	    break;
	  } else if (*iptr == '\n' || *iptr == '\r') {	/*newline or return */
	    *optr++ = '\r';
	    *optr++ = '\n';
	    iptr++;
	    break;			/* end of line */
	  } else if(*iptr == '\f') {		/* formfeed */
	    /* if next input character is newline or return,
	     * we can skip over it since we are outputing a formfeed anyway */
	    if((*++iptr == '\n') || (*iptr == '\r'))
	      iptr++;
	    ret_val = 1;
	    break;			/* leave rest of screen clear */
	  } else if(*iptr == '\0') {		/* none left in input string */
	    break;
	  } else if(*iptr == '\t') {		/* tab stop */
	    if((nt=next_tab(chars_output+1)) > width) {
#ifndef OS2
	      *optr++ = '\r';		/* won't fit on this line - autowrap */
	      *optr++ = '\n';		/* tab by tabbing so-to-speak to 1st */
#endif
	      iptr++;			/* column of next line */
	      break;
	    } else {		/* will fit - output proper num of spaces */
	      while(chars_output < nt-1) {
		chars_output++;
		*optr++ = ' ';
	      }
	      iptr++;
	    }
#ifdef OS2
	  } else if(isprint(*iptr) || (128 <= *iptr && *iptr < 255)) {
#else
	  } else if(isprint(*iptr)) {
#endif
	    *optr++ = *iptr++;			/* printing character */
	    chars_output++;
	  } else {			/* non-white space control character */
	    if(chars_output + 2 <= width) {
	      *optr++ = '^';
	      *optr++ = (*iptr == '\177' ? '?' : (*iptr&0177) + 'A' - 1);
	      iptr++;
	      chars_output += 2;
	    } else {			/* no space on line for both chars */
	      break;
	    }
	  }
	}
	*optr = '\0';
	*inputptr = iptr;
	return(ret_val);
}


int
display_line(input_line)
char *input_line;
{
	/** Display the given line on the screen, taking into account such
	    dumbness as wraparound and such.  If displaying this would put
	    us at the end of the screen, put out the "MORE" prompt and wait
	    for some input.   Return non-zero if the user terminates the
	    paging (e.g. 'i') or zero if we should continue. Also,
            this will pass back the value of any character the user types in
	    at the prompt instead, if needed... (e.g. if it can't deal with
	    it at this point)
	**/

	char *pending, footer[SLEN], display_buffer[SLEN], ch;
	int formfeed, lines_more;

        fixline(input_line);

#ifdef MMDF
	if (strcmp(input_line, MSG_SEPERATOR) == 0)
	  strcpy(input_line," ");
#endif /* MMDF */
	pending = input_line;
	CarriageReturn();

	do {

	  /* while there is more space on the screen - leave prompt line free */
	  while(unfilled_lines > 0) {

	    /* display a screen's lineful of the input line
	     * and reset pending to point to remainder of input line */
	    formfeed = next_line(&pending, display_buffer, COLUMNS);

	    if(*display_buffer == '\0') {	/* no line to display */
	      if(!formfeed)	/* no "formfeed" to display
	     			 * need more lines for screen */
		return(FALSE);
	    } else
	      Write_to_screen(display_buffer, 0);

	    /* if formfeed, clear remainder of screen */
	    if(formfeed) {
	      CleartoEOS();
	      unfilled_lines=0;
	    }
	    else
	      unfilled_lines--;

	    /* if screen is not full (leave room for prompt)
	     * but we've used up input line, return */

	    if(unfilled_lines > 0 && *pending == '\0')
	      return(FALSE);	/* we need more lines to fill screen */

	    /* otherwise continue to display next part of input line */
	  }

	  /* screen is now full - prompt for user input */
	  lines_more = total_lines_to_display - lines_displayed;
	  sprintf(footer,
		  ( (user_level == 0) ?
  " There %s %d line%s left (%d%%). Press <space> for more, or 'i' to return. "
		  : (user_level == 1) ?
  " %s%d line%s more (%d%%). Press <space> for more, 'i' to return. "
		  :
  " %s%d line%s more (you've seen %d%%) "),
		   (user_level == 0 ?
		     (lines_more == 1 ? "is" : "are") : ""),
		   lines_more, plural(lines_more),
		   (int)((100L * lines_displayed) / total_lines_to_display));

	  MoveCursor(LINES, 0);
	  StartBold();
	  Write_to_screen(footer, 0);
	  EndBold();

	  switch(ch = ReadCh()) {

	    case '\n':
	    case '\r':	/* scroll down a line */
			unfilled_lines = 1;
			ClearLine(LINES);
			break;

	    case ' ':	/* scroll a screenful */
			unfilled_lines = LINES;
			if(clear_pages) {
			  ClearScreen();
			  MoveCursor(0,0);
			  CarriageReturn();

			  /* output title */
			  if(title_messages && filter) {
			    title_for_page(++pages_displayed);
			    unfilled_lines -= 2;
			  }
			} else ClearLine(LINES);

			/* and keep last line to be first line of next
			 * screenful unless we had a formfeed */
			if(!formfeed) {
			  if(clear_pages)
			    Write_to_screen(display_buffer, 0);
			  unfilled_lines--;
			}
			break;

	    default:	return(ch);
	  }
	  CarriageReturn();
	} while(*pending);
	return(FALSE);
}

title_for_page(page)
int page;
{
	/** Output a nice title for the second thru last pages of the message
	    we're currently reading. Note - this code is very similar to
	    that which produces the title for the first page, except that
	    page number replaces the date and the method by which it
	    gets to the screen **/

	static char title1[SLEN], title2[SLEN];
	char titlebuf[SLEN], title3[SLEN], who[SLEN];
	static t1_len, t2_len;
	register int padding, showing_to;

	/* format those parts of the title that are constant for a message */
	if(form_title) {

	  showing_to = tail_of(headers[current-1]->from, who,
	    headers[current-1]->to);

	  sprintf(title1, "%s %d/%d  ",
	      headers[current-1]->status & DELETED ? "[deleted]" :
	      headers[current-1]->status & FORM_LETTER ? "Form": "Message",
	      current, message_count);
	  t1_len = strlen(title1);
	  sprintf(title2, "%s %s", showing_to? "To" : "From", who);
	  t2_len = strlen(title2);
	}
	/* format those parts of the title that vary between pages of a mesg */
	sprintf(title3, "  Page %d", page);

	/* truncate or pad title2 portion on the right
	 * so that line fits exactly to the rightmost column */
	padding = COLUMNS - 1 - (t1_len + t2_len + strlen(title3));

	sprintf(titlebuf, "%s%-*.*s%s\n\r\n\r", title1, t2_len+padding,
	    t2_len+padding, title2, title3);
	    /* extra newline is to give a blank line after title */

	Write_to_screen(titlebuf, 0);
	form_title = 0;
}
