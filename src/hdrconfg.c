
static char rcsid[] = "@(#)$Id: hdrconfg.c,v 4.1.1.2 90/06/05 20:59:30 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.2 $   $State: Exp $
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
 * $Log:	hdrconfg.c,v $
 * Revision 4.1.1.2  90/06/05  20:59:30  syd
 * Fix log
 *
 * Revision 4.1.1.1  90/06/05  20:58:23  syd
 * Fixes when ALLOW_SUBSHELL #define'd and you are in the
 * Message Header Edit Screen and the mail you just composed
 * is not a reply THEN the subshell command is executed.
 * From: zvr@natasha.cs.wisc.EDU (Alexios Zavras)
 *
 * Revision 4.1  90/04/28  22:43:10  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/**   This file contains the routines necessary to be able to modify
      the mail headers of messages on the way off the machine.  The
      headers currently supported for modification are:

	Subject:
	To:
	Cc:
	Bcc:
	Reply-To:
	Expires:
	Priority:
        In-Reply-To:
	Action:

	<user defined>
**/

#include "headers.h"

#include <ctype.h>

#ifdef BSD
#undef toupper
#undef tolower
#endif

/*
 * Allow three lines for To and Cc.
 * Allow two lines for Bcc, Subject and In-reply-to.
 * Allow one line for all others.
 */
#define TO_LINE			2
#define CC_LINE                 5
#define BCC_LINE                8
#define SUBJECT_LINE            10
#define REPLY_TO_LINE           12
#define ACTION_LINE             13
#define EXPIRES_LINE            14
#define PRIORITY_LINE           15
#define IN_REPLY_TO_LINE        16
#define USER_DEFINED_HDR_LINE   18
#define INSTRUCT_LINE           LINES-4
#define EXTRA_PROMPT_LINE	LINES-3
#define INPUT_LINE		LINES-2
#define ERROR_LINE		LINES-1

static  put_header();

#define put_to()        put_header(TO_LINE, 3, "To", expanded_to)
#define put_cc()        put_header(CC_LINE, 3, "Cc", expanded_cc)
#define put_bcc()       put_header(BCC_LINE, 2, "Bcc", expanded_bcc)
#define put_subject()   put_header(SUBJECT_LINE, 2, "Subject", subject)
#define put_replyto()   put_header(REPLY_TO_LINE, 1, "Reply-to", reply_to)
#define put_action()    put_header(ACTION_LINE, 1, "Action", action)
#define put_expires()   put_header(EXPIRES_LINE, 1, "Expires", expires)
#define put_priority()  put_header(PRIORITY_LINE, 1, "Priority", priority)
#define put_inreplyto() put_header(IN_REPLY_TO_LINE, 2, \
					"In-reply-to", in_reply_to)
#define put_userdefined() put_header(USER_DEFINED_HDR_LINE, 1, \
					(char *) NULL, user_defined_header)

/* these are all defined in the mailmsg file! */
extern char subject[SLEN], in_reply_to[SLEN], expires[SLEN],
            action[SLEN], priority[SLEN], reply_to[SLEN], to[VERY_LONG_STRING],
	    cc[VERY_LONG_STRING], expanded_to[VERY_LONG_STRING],
	    expanded_cc[VERY_LONG_STRING], user_defined_header[SLEN],
	    bcc[VERY_LONG_STRING], expanded_bcc[VERY_LONG_STRING];

char *strip_commas(), *strcpy();

edit_headers()
{
	/** Edit headers.  **/
	int c, displayed_error = NO;
	char expanded_address[VERY_LONG_STRING];

	/*  Expand address-type headers for main part of display */
	/*  (Unexpanded ones are used on the 'edit-line') */
	(void) build_address(strip_commas(to), expanded_to);
	(void) build_address(strip_commas(cc), expanded_cc);
	(void) build_address(strip_commas(bcc), expanded_bcc);

	display_headers();

	clearerr(stdin);

	while (TRUE) {	/* forever */
	  PutLine0(INPUT_LINE,0,"Choice: ");
	  if (displayed_error)
	    displayed_error = NO;
	  else
	    CleartoEOS();
	  c = ReadCh();
	  /* c = getchar(); */
	  if (isupper(c))
	    c = tolower(c);
	  clear_error();
	  if (c == EOF)
		return(0);
	  switch (c) {
	    case RETURN:
	    case LINE_FEED:
	    case 'q' :  MoveCursor(INSTRUCT_LINE, 0);
			CleartoEOS();
			return(0);

	    case ctrl('L') :
			display_headers();
			break;

	    case 't' :  PutLine0(INPUT_LINE, 0, "To: "); CleartoEOLN();
	             	if (optionally_enter(to, INPUT_LINE, 4, TRUE, FALSE) == -1)
			  return(0);
			(void) build_address(strip_commas(to), expanded_to);
			put_to();
			break;

	    case 's' :  PutLine0(INPUT_LINE, 0, "Subject: "); CleartoEOLN();
	    		if (optionally_enter(subject,
			      INPUT_LINE, 9, FALSE, FALSE) == -1)
			  return(0);
			put_subject();
			break;

	    case 'b' :  PutLine0(INPUT_LINE, 0, "Bcc: "); CleartoEOLN();
	    		if (optionally_enter(bcc,
			      INPUT_LINE, 5, TRUE, FALSE) == -1)
			  return(0);
			(void) build_address(strip_commas(bcc), expanded_bcc);
			put_bcc();
			break;

	    case 'c' :  PutLine0(INPUT_LINE, 0, "Cc: "); CleartoEOLN();
	    		if (optionally_enter(cc, INPUT_LINE, 4, TRUE, FALSE) == -1)
			  return(0);
			(void) build_address(strip_commas(cc), expanded_cc);
			put_cc();
			break;

	    case 'r' :  PutLine0(INPUT_LINE, 0, "Reply-To: "); CleartoEOLN();
	    		if(optionally_enter(reply_to,
			      INPUT_LINE, 10, FALSE, FALSE) == -1)
			  return(0);
			(void) build_address(strip_commas(reply_to), expanded_address);
			strcpy(reply_to, expanded_address);
			put_replyto();
			break;

	    case 'a' :  PutLine0(INPUT_LINE, 0, "Action: "); CleartoEOLN();
	    		if (optionally_enter(action,
			      INPUT_LINE, 8, FALSE, FALSE) == -1)
			  return(0);
			put_action();
			break;

	    case 'p' :  PutLine0(INPUT_LINE, 0, "Priority: "); CleartoEOLN();
	    		if (optionally_enter(priority,
			      INPUT_LINE, 10, FALSE, FALSE)==-1)
			  return(0);
			put_priority();
			break;

	    case 'e' :  enter_date(expires);
			put_expires();
			break;

	    case 'u' :  PutLine0(EXTRA_PROMPT_LINE, 0, "User defined header: ");
			CleartoEOLN();
	   	 	if (optionally_enter(user_defined_header,
			     INPUT_LINE, 0, FALSE, FALSE)==-1)
			  return(0);
			check_user_header(user_defined_header);
			put_userdefined();
			ClearLine(EXTRA_PROMPT_LINE);
			break;

#ifdef ALLOW_SUBSHELL
	    case '!':   if (subshell())
			  display_headers();
		        break;
#endif

	    case 'i' :  if (strlen(in_reply_to) > 0) {
			  PutLine0(INPUT_LINE, 0, "In-Reply-To: "); CleartoEOLN();
	                  if (optionally_enter(in_reply_to,
			      INPUT_LINE, 13, FALSE, FALSE) == -1)
			    return(0);
			  put_inreplyto();
			  break;
		       }

	    default  : Centerline(ERROR_LINE, "No such header!");
		       displayed_error = YES;
	  }
	}
}

display_headers()
{
	char buffer[SLEN];

	ClearScreen();

	Centerline(0,"Message Header Edit Screen");

	put_to();
	put_cc();
	put_bcc();
	put_subject();
	put_replyto();
	put_action();
	put_expires();
	put_priority();
	if (in_reply_to[0])
	  put_inreplyto();
	if (user_defined_header[0])
	  put_userdefined();

	strcpy(buffer, "Choose first letter of header, u)ser defined header, ");
#ifdef ALLOW_SUBSHELL
	strcat(buffer, "!)shell, ");
#endif
	strcat(buffer, "or <return>.");
	Centerline(INSTRUCT_LINE, buffer);
}

static
put_header(hline, hcount, field, value)
int     hline, hcount;
char    *field, *value;
{
	char    *p;
	int     x, y;

	MoveCursor(hline, 0);

	if (field) {
	  for (p = field; *p; ++p)
	    Writechar(*p);
	  Writechar(':');
	  Writechar(' ');
	}

	GetXYLocation(&x, &y);

	for (p = value; *p; ++p) {
	  if (x >= (hline + hcount))
	      break;
	  /* neat hack alert -- danger will robinson */
	  if ((x + 1) == (hline + hcount)
	   && (y + 4) == COLUMNS && strlen(p) > 4)
	    p = " ...";
	  Writechar(*p);
	  ++y;
	  if (*p < 0x20 || *p >= 0x7F || y >= COLUMNS)
	    GetXYLocation(&x, &y);
	}

	if (x < (hline + hcount)) {
	  CleartoEOLN();

	  while (++x < (hline + hcount)) {
	    MoveCursor(x, 0);
	    CleartoEOLN();
	  }
	}
}

enter_date(datebuf)
char *datebuf;
{
	/** Enter the number of days this message is valid for, then
	    display at (x,y) the actual date of expiration.  This
	    routine relies heavily on the routine 'days_ahead()' in
	    the file date.c
	**/

	int days;
	char numdaysbuf[SLEN];

	static char prompt[] =
	  "How many days in the future should this message expire? ";

	PutLine0(INPUT_LINE,0, prompt);
	CleartoEOLN();
	*datebuf = *numdaysbuf = '\0';

	optionally_enter(numdaysbuf, INPUT_LINE, strlen(prompt), FALSE, FALSE);
	sscanf(numdaysbuf, "%d", &days);
	if (days < 1)
	  Centerline(ERROR_LINE, "That doesn't make sense!");
	else if (days > 56)
	  Centerline(ERROR_LINE,
	     "Expiration date must be within eight weeks of today.");
	else {
	  days_ahead(days, datebuf);
	}
}

check_user_header(header)
char *header;
{
	/** check the header format...if bad print error and erase! **/

	register int i = -1;

	if (strlen(header) == 0)
	   return;

	if (whitespace(header[0])) {
	  error ("You can't have leading white space in a header!");
	  header[0] = '\0';
	  ClearLine(USER_DEFINED_HDR_LINE);
	  return;
	}

	if (header[0] == ':') {
	  error ("You can't have a colon as the first character!");
	  header[0] = '\0';
	  ClearLine(USER_DEFINED_HDR_LINE);
	  return;
	}

	while (header[++i] != ':') {
	  if (header[i] == '\0') {
	    Centerline(ERROR_LINE, "You need to have a colon ending the field name!");
	    header[0] = '\0';
	    ClearLine(USER_DEFINED_HDR_LINE);
	    return;
	  }
	  else if (whitespace(header[i])) {
	    Centerline(ERROR_LINE, "You can't have white space imbedded in the header name!");
	    header[0] = '\0';
	    ClearLine(USER_DEFINED_HDR_LINE);
	    return;
	  }
	}

	return;
}
