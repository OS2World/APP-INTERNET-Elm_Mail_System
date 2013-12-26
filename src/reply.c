
static char rcsid[] = "@(#)$Id: reply.c,v 4.1 90/04/28 22:43:51 syd Exp $";

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
 * $Log:	reply.c,v $
 * Revision 4.1  90/04/28  22:43:51  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/*** routine allows replying to the sender of the current message

***/

#include "headers.h"
#include <errno.h>

#ifndef BSD
#  include <sys/types.h>
# ifndef VMS
# ifndef OS2
#  include <sys/utsname.h>
# endif
# endif
#endif

/** Note that this routine generates automatic header information
    for the subject and (obviously) to lines, but that these can
    be altered while in the editor composing the reply message!
**/

char *strip_parens(), *get_token();

#ifndef OS2
extern int errno;
#endif

char *error_name(), *strcat(), *strcpy();

int
reply()
{
	/** Reply to the current message.  Returns non-zero iff
	    the screen has to be rewritten. **/

	char return_address[SLEN], subject[SLEN];
	int  return_value, form_letter;

	form_letter = (headers[current-1]->status & FORM_LETTER);

	if (get_return(return_address, current-1)) {
	  strcpy(subject, headers[current-1]->subject);
	  if (form_letter)
	    return_value = mail_filled_in_form(return_address, subject);
	  else
	    return_value = sendmsg(return_address, "", subject, TRUE, NO, TRUE);
	}
	else if (headers[current-1]->subject[0] != '\0') {
	  if ((strncmp("Re:", headers[current-1]->subject, 3) == 0) ||
	      (strncmp("RE:", headers[current-1]->subject, 3) == 0) ||
	      (strncmp("re:", headers[current-1]->subject, 3) == 0))
	    strcpy(subject, headers[current-1]->subject);
	  else {
	    strcpy(subject,"Re: ");
	    strcat(subject,headers[current-1]->subject);
	  }
	  if (form_letter)
	    return_value = mail_filled_in_form(return_address, subject);
	  else
	    return_value = sendmsg(return_address, "", subject, TRUE, NO, TRUE);
	}
	else
	  if (form_letter)
	    return_value = mail_filled_in_form(return_address,
						"Filled in Form");
	  else
	    return_value = sendmsg(return_address, "", "Re: your mail",
				TRUE, NO, TRUE);

	return(return_value);
}

int
reply_to_everyone()
{
	/** Reply to everyone who received the current message.
	    This includes other people in the 'To:' line and people
	    in the 'Cc:' line too.  Returns non-zero iff the screen
            has to be rewritten. **/

	char return_address[SLEN], subject[SLEN];
	char full_address[VERY_LONG_STRING];
	int  return_value;

	get_return(return_address, current-1);

	full_address[0] = '\0';			/* no copies yet    */
	get_and_expand_everyone(return_address, full_address);

	if (headers[current-1]->subject[0] != '\0') {
	  if ((strncmp("Re:", headers[current-1]->subject, 3) == 0) ||
	      (strncmp("RE:", headers[current-1]->subject, 3) == 0) ||
	      (strncmp("re:", headers[current-1]->subject, 3) == 0))
	    strcpy(subject, headers[current-1]->subject);
	  else {
	    strcpy(subject,"Re: ");
	    strcat(subject,headers[current-1]->subject);
	  }
	  return_value = sendmsg(return_address, full_address, subject,
				 TRUE, NO, TRUE);
	}
	else
	  return_value = sendmsg(return_address, full_address,
			      "Re: your mail", TRUE, NO, TRUE);

	return(return_value);

}

int
forward()
{
	/** Forward the current message.  What this actually does is
	    to temporarily set forwarding to true, then call 'send' to
	    get the address and route the mail.   Modified to also set
	    'noheader' to FALSE also, so that the original headers
	    of the message sent are included in the message body also.
	    Return TRUE if the main part of the screen has been changed
	    (useful for knowing whether a redraw is needed.
	**/

	char subject[SLEN], address[VERY_LONG_STRING];
	int  results, edit_msg = FALSE;

	forwarding = TRUE;

	address[0] = '\0';

	if (headers[current-1]->status & FORM_LETTER)
	  PutLine0(LINES-3,COLUMNS-40,"<No editing allowed.>");
	else {
	  edit_msg = (want_to("Edit outgoing message? (y/n) ",'y') != 'n');
	}

	if (strlen(headers[current-1]->subject) > 0) {

	  strcpy(subject, headers[current-1]->subject);

	  /* this next strange compare is to see if the last few chars are
	     already '(fwd)' before we tack another on */

	  if (strlen(subject) < 6 || (strcmp((char *) subject+strlen(subject)-5,
					     "(fwd)") != 0))
	    strcat(subject, " (fwd)");

	  results = sendmsg(address, "", subject, edit_msg,
	    headers[current-1]->status & FORM_LETTER?
	    PREFORMATTED : allow_forms, FALSE);
	}
	else
	  results = sendmsg(address, "", "Forwarded mail...", edit_msg,
	    headers[current-1]->status & FORM_LETTER?
	    PREFORMATTED : allow_forms, FALSE);

	forwarding = FALSE;

	return(results);
}

get_and_expand_everyone(return_address, full_address)
char *return_address, *full_address;
{
	/** Read the current message, extracting addresses from the 'To:'
	    and 'Cc:' lines.   As each address is taken, ensure that it
	    isn't to the author of the message NOR to us.  If neither,
	    prepend with current return address and append to the
	    'full_address' string.
	**/

    char ret_address[SLEN], buf[SLEN], new_address[SLEN],
	 address[SLEN], comment[SLEN];
    int  in_message = 1, first_pass = 0, iindex, line_pending = 0;

    /** First off, get to the first line of the message desired **/

    if (fseek(mailfile, headers[current-1]->offset, 0) == -1) {
	dprint(1,(debugfile,"Error: seek %ld resulted in errno %s (%s)\n",
		 headers[current-1]->offset, error_name(errno),
		 "get_and_expand_everyone"));
	error2("ELM [seek] couldn't read %d bytes into file (%s).",
	       headers[current-1]->offset, error_name(errno));
	return;
    }

    /** okay!  Now we're there!  **/

    /** let's fix the ret_address to reflect the return address of this
	message with '%s' instead of the persons login name... **/

    translate_return(return_address, ret_address);

    /** now let's parse the actual message! **/

    while (in_message) {
      if (! line_pending)
        in_message = (int) (fgets(buf, SLEN, mailfile) != NULL);
      line_pending = 0;
      if (first_word(buf, "From ") && first_pass++ != 0)
	in_message = FALSE;
      else if (first_word(buf, "To:") || first_word(buf, "Cc:") ||
	       first_word(buf, "CC:") || first_word(buf, "cc:")) {
	do {
	  no_ret(buf);

	  /** we have a buffer with a list of addresses, each of either the
	      form "address (name)" or "name <address>".  Our mission, should
	      we decide not to be too lazy, is to break it into the two parts.
	  **/

	  if (!whitespace(buf[0]))
	    iindex = chloc(buf, ':')+1;		/* skip header field */
	  else
	    iindex = 0;				/* skip whitespace   */

	  while (break_down_tolist(buf, &iindex, address, comment)) {

	    if (okay_address(address, return_address)) {
	      sprintf(new_address, ret_address, address);
	      optimize_and_add(new_address, full_address);
	    }
	  }

          in_message = (int) (fgets(buf, SLEN, mailfile) != NULL);

	  if (in_message) dprint(2, (debugfile, "> %s", buf));

	} while (in_message && whitespace(buf[0]));
	line_pending++;
      }
      else if (strlen(buf) < 2)	/* done with header */
	 in_message = FALSE;
    }
}

int
okay_address(address, return_address)
char *address, *return_address;
{
	/** This routine checks to ensure that the address we just got
	    from the "To:" or "Cc:" line isn't us AND isn't the person
	    who sent the message.  Returns true iff neither is the case **/

	char our_address[SLEN];
	struct addr_rec  *alternatives;

	if (in_list(address, return_address))
	  return(FALSE);

	if(in_list(address, username))
	  return(FALSE);

	sprintf(our_address, "%s!%s", hostname, username);
	if (in_list(address, our_address))
	  return(FALSE);

	sprintf(our_address, "%s!%s", hostfullname, username);
	if (in_list(address, our_address))
	  return(FALSE);

	sprintf(our_address, "%s@%s", username, hostname);
	if (in_list(address, our_address))
	  return(FALSE);

	sprintf(our_address, "%s@%s", username, hostfullname);
	if (in_list(address, our_address))
	  return(FALSE);

	alternatives = alternative_addresses;

	while (alternatives != NULL) {
	  if (in_list(address, alternatives->address))
	    return(FALSE);
	  alternatives = alternatives->next;
	}

	return(TRUE);
}

optimize_and_add(new_address, full_address)
char *new_address, *full_address;
{
	/** This routine will add the new address to the list of addresses
	    in the full address buffer IFF it doesn't already occur.  It
	    will also try to fix dumb hops if possible, specifically hops
	    of the form ...a!b...!a... and hops of the form a@b@b etc
	**/

	register int len, host_count = 0, i;
	char     hosts[MAX_HOPS][SLEN];	/* array of machine names */
	char     *host, *addrptr;

	if (in_list(full_address, new_address))
	  return(1);	/* duplicate address */

	/** optimize **/
	/*  break down into a list of machine names, checking as we go along */

	addrptr = (char *) new_address;

	while ((host = get_token(addrptr, "!", 1)) != NULL) {
	  for (i = 0; i < host_count && ! equal(hosts[i], host); i++)
	      ;

	  if (i == host_count) {
	    strcpy(hosts[host_count++], host);
	    if (host_count == MAX_HOPS) {
	       dprint(2, (debugfile,
              "Error: hit max_hops limit trying to build return address (%s)\n",
		      "optimize_and_add"));
	       error("Can't build return address. Hit MAX_HOPS limit!");
	       return(1);
	    }
	  }
	  else
	    host_count = i + 1;
	  addrptr = NULL;
	}

	/** fix the ARPA addresses, if needed **/

	if (chloc(hosts[host_count-1], '@') > -1)
	  fix_arpa_address(hosts[host_count-1]);

	/** rebuild the address.. **/

	new_address[0] = '\0';

	for (i = 0; i < host_count; i++)
	  sprintf(new_address, "%s%s%s", new_address,
	          new_address[0] == '\0'? "" : "!",
	          hosts[i]);

	if (full_address[0] == '\0')
	  strcpy(full_address, new_address);
	else {
	  len = strlen(full_address);
	  full_address[len  ] = ',';
	  full_address[len+1] = ' ';
	  full_address[len+2] = '\0';
	  strcat(full_address, new_address);
	}

	return(0);
}

get_return_name(address, name, trans_to_lowercase)
char *address, *name;
int   trans_to_lowercase;
{
	/** Given the address (either a single address or a combined list
	    of addresses) extract the login name of the first person on
	    the list and return it as 'name'.  Modified to stop at
	    any non-alphanumeric character. **/

	/** An important note to remember is that it isn't vital that this
	    always returns just the login name, but rather that it always
	    returns the SAME name.  If the persons' login happens to be,
	    for example, joe.richards, then it's arguable if the name
	    should be joe, or the full login.  It's really immaterial, as
	    indicated before, so long as we ALWAYS return the same name! **/

	/** Another note: modified to return the argument as all lowercase
	    always, unless trans_to_lowercase is FALSE... **/

	char single_address[SLEN];
	register int i, loc, iindex = 0;

	dprint(6, (debugfile,"get_return_name called with (%s, <>, shift=%s)\n",
		   address, onoff(trans_to_lowercase)));

	/* skip leading white space */
	while (whitespace(*address))
	  address++;

	/* skip possible "" quoted full name and possible opening < */
	if (*address == '"') {
	  for (address++; *address != '"' && *address != 0; address++);
	  if (*address == '"')
	    address++;
	  while (whitespace(*address))
	    address++;
	}
	if (*address == '<')
	  address++;

	/* First step - copy address up to a comma, space, or EOLN */

	for (i=0; address[i] != ',' && ! whitespace(address[i]) &&
	     address[i] != '\0'; i++)
	  single_address[i] = address[i];
	single_address[i] = '\0';

	/* Now is it an ARPA address?? */

	if ((loc = chloc(single_address, '@')) != -1) {	  /* Yes */

	  /* At this point the algorithm is to keep shifting our copy
	     window left until we hit a '!'.  The login name is then
             located between the '!' and the first metacharacter to
	     it's right (ie '%', ':' or '@'). */

	  for (i=loc; single_address[i] != '!' && i > -1; i--)
	      if (single_address[i] == '%' ||
	          single_address[i] == ':' ||
		  single_address[i] == '@') loc = i-1;

	  if (i < 0 || single_address[i] == '!') i++;

	  for (iindex = 0; iindex < loc - i + 1; iindex++)
	    if (trans_to_lowercase)
	      name[iindex] = tolower(single_address[iindex+i]);
	    else
	      name[iindex] = single_address[iindex+i];
	  name[iindex] = '\0';
	}
	else {	/* easier - standard USENET address */

	  /* This really is easier - we just cruise left from the end of
	     the string until we hit either a '!' or the beginning of the
             line.  No sweat. */

	  loc = strlen(single_address)-1; 	/* last char */

	  for (i = loc; single_address[i] != '!' && single_address[i] != '.'
	       && i > -1; i--)
	     if (trans_to_lowercase)
	       name[iindex++] = tolower(single_address[i]);
	     else
	       name[iindex++] = single_address[i];
	  name[iindex] = '\0';
	  reverse(name);
	}
}

int
break_down_tolist(buf, iindex, address, comment)
char *buf, *address, *comment;
int  *iindex;
{
	/** This routine steps through "buf" and extracts a single address
	    entry.  This entry can be of any of the following forms;

		address (name)
		name <address>
		address

	    Once it's extracted a single entry, it will then return it as
	    two tokens, with 'name' (e.g. comment) surrounded by parens.
	    Returns ZERO if done with the string...
	**/

	char buffer[LONG_STRING];
	register int i, loc = 0, hold_index, len;

	if (*iindex > strlen(buf)) return(FALSE);

	while (whitespace(buf[*iindex])) (*iindex)++;

	if (*iindex > strlen(buf)) return(FALSE);

	/** Now we're pointing at the first character of the token! **/

	hold_index = *iindex;

	while (buf[*iindex] != ',' && buf[*iindex] != '\0')
	  buffer[loc++] = buf[(*iindex)++];

	(*iindex)++;
	buffer[loc] = '\0';

	while (whitespace(buffer[loc])) 	/* remove trailing whitespace */
	  buffer[--loc] = '\0';

	if (strlen(buffer) == 0) return(FALSE);

	dprint(5, (debugfile, "\n* got \"%s\"\n", buffer));

	if (buffer[loc-1] == ')') {	/*   address (name)  format */
	  for (loc = 0, len = strlen(buffer);buffer[loc] != '(' && loc < len; loc++)
		/* get to the opening comment character... */ ;

	  loc--;	/* back up to just before the paren */
	  while (whitespace(buffer[loc])) loc--;	/* back up */

	  /** get the address field... **/

	  for (i=0; i <= loc; i++)
	    address[i] = buffer[i];
	  address[i] = '\0';

	  /** now get the comment field, en toto! **/

	  loc = 0;

	  for (i = chloc(buffer, '('), len = strlen(buffer); i < len; i++)
	    comment[loc++] = buffer[i];
	  comment[loc] = '\0';
	}
	else if (buffer[loc-1] == '>') {	/*   name <address>  format */
	  dprint(7, (debugfile, "\tcomment <address>\n"));
	  for (loc = 0, len = strlen(buffer);buffer[loc] != '<' && loc < len; loc++)
		/* get to the opening comment character... */ ;
	  while (whitespace(buffer[loc])) loc--;	/* back up */

	  /** get the comment field... **/

	  comment[0] = '(';
	  for (i=1; i < loc; i++)
	    comment[i] = buffer[i-1];
	  comment[i++] = ')';
	  comment[i] = '\0';

	  /** now get the address field, en toto! **/

	  loc = 0;

	  for (i = chloc(buffer,'<') + 1, len = strlen(buffer); i < len - 1; i++)
	    address[loc++] = buffer[i];

	  address[loc] = '\0';
	}
	else {
	  /** the next section is added so that all To: lines have commas
	      in them accordingly **/

	  for (i=0; buffer[i] != '\0'; i++)
	    if (whitespace(buffer[i])) break;
	  if (i < strlen(buffer)) {	/* shouldn't be whitespace */
	    buffer[i] = '\0';
	    *iindex = hold_index + strlen(buffer) + 1;
	  }
	  strcpy(address, buffer);
	  comment[0] = '\0';
	}

	dprint(5, (debugfile, "-- returning '%s' '%s'\n", address, comment));

	return(TRUE);
}
