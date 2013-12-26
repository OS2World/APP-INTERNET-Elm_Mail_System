
static char rcsid[] = "@(#)$Id: mailmsg1.c,v 4.1.1.1 90/06/05 20:52:21 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.1 $   $State: Exp $
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
 * $Log:	mailmsg1.c,v $
 * Revision 4.1.1.1  90/06/05  20:52:21  syd
 * Fixes the 'g' Group Reply command to send to the cc list also.
 * A bad variable name caused it to be ignored.
 * From: chip@chinacat.Unicom.COM (Chip Rosenthal)
 *
 * Revision 4.1  90/04/28  22:43:26  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Interface to allow mail to be sent to users.  Part of ELM  **/


#include "headers.h"

/** strings defined for the hdrconfg routines **/

char subject[SLEN], in_reply_to[SLEN], expires[SLEN],
     action[SLEN], priority[SLEN], reply_to[SLEN], to[VERY_LONG_STRING],
     cc[VERY_LONG_STRING], expanded_to[VERY_LONG_STRING],
     expanded_cc[VERY_LONG_STRING], user_defined_header[SLEN],
     bcc[VERY_LONG_STRING], expanded_bcc[VERY_LONG_STRING];

char *format_long(), *strip_commas(), *tail_of_string(), *strcpy();
unsigned long sleep();

int
sendmsg(given_to, given_cc, given_subject, edit_message, form_letter, replying)
char *given_to, *given_cc, *given_subject;
int   edit_message, form_letter, replying;
{
	/** Prompt for fields and then call mail() to send the specified
	    message.  If 'edit_message' is true then don't allow the
            message to be edited. 'form_letter' can be "YES" "NO" or "MAYBE".
	    if YES, then add the header.  If MAYBE, then add the M)ake form
	    option to the last question (see mailsg2.c) etc. etc.
	    if (replying) then add an In-Reply-To: header...
	    Return TRUE if the main part of the screen has been changed
	    (useful for knowing whether a redraw is needed.
	**/

	int  copy_msg = FALSE, is_a_response = FALSE;

	/* First: zero all current global message strings */

	cc[0] = bcc[0] = reply_to[0] = expires[0] = '\0';
	action[0] = priority[0] = user_defined_header[0] = in_reply_to[0] ='\0';
	expanded_to[0] = expanded_cc[0] = expanded_bcc[0] = '\0';

	strcpy(subject, given_subject);		/* copy given subject */
	strcpy(to, given_to);			/* copy given to:     */
	strcpy(cc, given_cc);			/*  and so on..       */

	/******* And now the real stuff! *******/

	copy_msg=copy_the_msg(&is_a_response); /* copy msg into edit buffer? */

	if (get_to(to, expanded_to) == 0)   /* get the To: address and expand */
	  return(0);
	if ( cc[0] != '\0' )			/* expand out CC addresses */
	  build_address(strip_commas(cc), expanded_cc);

	/** if we're batchmailing, let's send it and GET OUTTA HERE! **/

	if (batch_only) {
	  return(mail(FALSE, FALSE, form_letter));
	}

	display_to(expanded_to);	/* display the To: field on screen... */

	dprint(3, (debugfile, "\nMailing to \"%s\"\n", expanded_to));

	if (get_subject(subject) == 0)	    /* get the Subject: field */
	  return(0);

	dprint(4, (debugfile, "Subject is %s\n", subject));

	if (prompt_for_cc) {
	  if (get_copies(cc, expanded_to, expanded_cc, copy_msg) == 0)
	    return(0);

	  if (strlen(cc) > 0)
	    dprint(4, (debugfile, "Copies to %s\n", expanded_cc));
	}

	MoveCursor(LINES,0);	/* so you know you've hit <return> ! */

	/** generate the In-Reply-To: header... **/

	if (is_a_response && replying)
	  generate_reply_to(current-1);

#ifdef OS2
	/* Reply-To: from UUPC.RC (see os2util.c), from Frank Behrens */
	if (strlen(_reply_to) > 0)
        {
	   strcpy(reply_to, _reply_to);
	   dprint(4, (debugfile, "Add default Reply-To: %s\n", _reply_to));
	}
#endif

	/* and mail that puppy outta here! */

	return(mail(copy_msg, edit_message, form_letter));
}

get_to(to_field, address)
char *to_field, *address;
{
	/** prompt for the "To:" field, expanding into address if possible.
	    This routine returns ZERO if errored, or non-zero if okay **/

	if (strlen(to_field) == 0) {
	  if (user_level < 2) {
	    PutLine0(LINES-2, 0, "Send the message to: ");
	    (void) optionally_enter(to_field, LINES-2, 21, FALSE, FALSE);
	  }
	  else {
	    PutLine0(LINES-2, 0, "To: ");
	    (void) optionally_enter(to_field, LINES-2, 4, FALSE, FALSE);
	  }
	  if (strlen(to_field) == 0) {
	    ClearLine(LINES-2);
	    return(0);
	  }
	  (void) build_address(strip_commas(to_field), address);
	}
	else if (mail_only)
	  (void) build_address(strip_commas(to_field), address);
	else
	  strcpy(address, to_field);

	if (strlen(address) == 0) {	/* bad address!  Removed!! */
	  ClearLine(LINES-2);
	  return(0);
	}

	return(1);		/* everything is okay... */
}

get_subject(subject_field)
char *subject_field;
{
	char	ch;

	/** get the subject and return non-zero if all okay... **/
	int len = 9, prompt_line;

	prompt_line = mail_only ? 4 : LINES-2;

	if (user_level == 0) {
	  PutLine0(prompt_line,0,"Subject of message: ");
	  len = 20;
	}
	else
	  PutLine0(prompt_line,0,"Subject: ");

	CleartoEOLN();

	if(optionally_enter(subject_field, prompt_line, len, TRUE, FALSE)==-1){
	  /** User hit the BREAK key! **/
	  MoveCursor(prompt_line,0);
	  CleartoEOLN();
	  error("Mail not sent.");
	  return(0);
	}

	if (strlen(subject_field) == 0) {	/* zero length subject?? */
	  PutLine1(prompt_line,0,
	    "No subject - Continue with message? (y/n) n%c", BACKSPACE);

	  ch = ReadCh();
	  if (tolower(ch) != 'y') {	/* user says no! */
	    Write_to_screen("No.", 0);
	    ClearLine(prompt_line);
	    error("Mail not sent.");
	    return(0);
	  }
	  else {
	    Write_to_screen("Yes.", 0);
	    PutLine0(prompt_line,0,"Subject: <none>");
	    CleartoEOLN();
	  }
	}

	return(1);		/** everything is cruising along okay **/
}

get_copies(cc_field, address, addressII, copy_message)
char *cc_field, *address, *addressII;
int   copy_message;
{
	/** Get the list of people that should be cc'd, returning ZERO if
	    any problems arise.  Address and AddressII are for expanding
	    the aliases out after entry!
	    If 'bounceback' is nonzero, add a cc to ourselves via the remote
	    site, but only if hops to machine are > bounceback threshold.
	    If copy-message, that means that we're going to have to invoke
	    a screen editor, so we'll need to delay after displaying the
	    possibly rewritten Cc: line...
	**/
	int prompt_line;

	prompt_line = mail_only ? 5 : LINES - 1;
	PutLine0(prompt_line,0,"Copies to: ");

	fflush(stdout);

	if (optionally_enter(cc_field, prompt_line, 11, FALSE, FALSE) == -1) {
	  ClearLine(prompt_line-1);
	  ClearLine(prompt_line);

	  error("Mail not sent.");
	  return(0);
	}

	/** The following test is that if the build_address routine had
	    reason to rewrite the entry given, then, if we're mailing only
	    print the new Cc line below the old one.  If we're not, then
	    assume we're in screen mode and replace the incorrect entry on
	    the line above where we are (e.g. where we originally prompted
	    for the Cc: field).
	**/

	if (build_address(strip_commas(cc_field), addressII)) {
	  PutLine1(prompt_line, 11, "%s", addressII);
	  if ((strcmp(editor, "builtin") != 0 && strcmp(editor, "none") != 0)
	      || copy_message)
	    sleep(2);
	}

	if (strlen(address) + strlen(addressII) > VERY_LONG_STRING) {
	  dprint(2, (debugfile,
		"String length of \"To:\" + \"Cc\" too long! (get_copies)\n"));
	  error("Too many people. Copies ignored.");
	  sleep(2);
	  cc_field[0] = '\0';
	}

	return(1);		/* everything looks okay! */
}

int
copy_the_msg(is_a_response)
int *is_a_response;
{
	/** Returns True iff the user wants to copy the message being
	    replied to into the edit buffer before invoking the editor!
	    Sets "is_a_response" to true if message is a response...
	**/

	int answer = FALSE;

	if (forwarding)
	  answer = TRUE;
	else if (strlen(to) > 0 && !mail_only) {  /* predefined 'to' line! */
	  if (auto_copy)
	    answer = TRUE;
	  else
	    answer = (want_to("Copy message? (y/n) ", 'n') == 'y');
	  *is_a_response = TRUE;
	}

	return(answer);
}

static int to_line, to_col;

display_to(address)
char *address;
{
	/** Simple routine to display the "To:" line according to the
	    current configuration (etc)
	 **/
	register int open_paren;

	to_line = mail_only ? 3 : LINES - 3;
	to_col = mail_only ? 0 : COLUMNS - 50;
	if (names_only)
	  if ((open_paren = chloc(address, '(')) > 0) {
	    if (open_paren < chloc(address, ')')) {
	      output_abbreviated_to(address);
	      return;
	    }
	  }
	if(mail_only)
	  if(strlen(address) > 80)
	    PutLine1(to_line, to_col, "To: (%s)",
	        tail_of_string(address, 75));
	  else
	    PutLine1(to_line, to_col, "To: %s", address);
	else if (strlen(address) > 45)
	  PutLine1(to_line, to_col, "To: (%s)",
	      tail_of_string(address, 40));
	else {
	  if (strlen(address) > 30)
	    PutLine1(to_line, to_col, "To: %s", address);
	  else
	    PutLine1(to_line, to_col, "          To: %s", address);
	  CleartoEOLN();
	}
}

output_abbreviated_to(address)
char *address;
{
	/** Output just the fields in parens, separated by commas if need
	    be, and up to COLUMNS-50 characters...This is only used if the
	    user is at level BEGINNER.
	**/

	char newaddress[LONG_STRING];
	register int iindex, newindex = 0, in_paren = 0, add_len;

	iindex = 0;

	add_len = strlen(address);
	while (newindex < 55 && iindex < add_len) {
	  if (address[iindex] == '(') in_paren++;
	  else if (address[iindex] == ')') {
	    in_paren--;
	    if (iindex < add_len-4) {
	      newaddress[newindex++] = ',';
	      newaddress[newindex++] = ' ';
	    }
	  }

	  /* copy if in_paren but not at the opening outer parens */
	  if (in_paren && !(address[iindex] == '(' && in_paren == 1))
	      newaddress[newindex++] = address[iindex];

	  iindex++;
	}

	newaddress[newindex] = '\0';

	if (mail_only)
	  if (strlen(newaddress) > 80)
	    PutLine1(to_line, to_col, "To: (%s)",
	       tail_of_string(newaddress, 60));
	  else
	    PutLine1(to_line, to_col, "To: %s", newaddress);
	else if (strlen(newaddress) > 50)
	   PutLine1(to_line, to_col, "To: (%s)",
	       tail_of_string(newaddress, 40));
	 else {
	   if (strlen(newaddress) > 30)
	     PutLine1(to_line, to_col, "To: %s", newaddress);
	   else
	     PutLine1(to_line, to_col, "          To: %s", newaddress);
	   CleartoEOLN();
	 }

	return;
}
