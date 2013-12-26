
static char rcsid[] = "@(#)$Id: returnadd.c,v 4.1.1.3 90/12/11 15:35:56 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.3 $   $State: Exp $
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
 * $Log:	returnadd.c,v $
 * Revision 4.1.1.3  90/12/11  15:35:56  syd
 * Add back missing  strlen line, fixes segv problem
 * From: Syd
 *
 * Revision 4.1.1.2  90/12/05  22:33:58  syd
 * Fix missing close brace due to indention error
 * From: Syd
 *
 * Revision 4.1.1.1  90/12/05  21:59:41  syd
 * Fix where header could be going past end on return due to line
 * combination on header continuation.
 * From: Syd via report from Tom Davis
 *
 * Revision 4.1  90/04/28  22:43:54  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This set of routines is used to generate real return addresses
    and also return addresses suitable for inclusion in a users
    alias files (ie optimized based on the pathalias database).

**/

#include "headers.h"

#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

char *shift_lower();

#ifndef OS2
extern int errno;
#endif

char *error_name(), *strcat(), *strcpy();

#ifdef OPTIMIZE_RETURN

optimize_return(address)
char *address;
{
	/** This routine tries to create an optimized address, that is,
	    an address that has the minimal information needed to
	    route a message to this person given the current path
	    database...
	**/

#ifndef INTERNET
	char    bogus_internet[SLEN];

	sprintf(bogus_internet, "@%s", hostfullname);

	/** first off, let's see if we need to strip off the localhost
	    address crap... **/

	/** if we have a uucp part (e.g.a!b) AND the bogus address...**/

	if (chloc(address,'!') != -1 && in_string(address, bogus_internet))
	  address[strlen(address)-strlen(bogus_internet)] = '\0';
#endif

	/** next step is to figure out what sort of address we have... **/

	if (chloc(address, '%') != -1)
	  optimize_cmplx_arpa(address);
	else if (chloc(address, '@') != -1)
	  optimize_arpa(address);
	else
	  optimize_usenet(address);
}

optimize_cmplx_arpa(address)
char *address;
{
	/** Try to optimize a complex ARPA address.  A Complex address is one
	    that contains '%' (deferred '@').  For example:
		veeger!hpcnof!hplabs!joe%sytech@syte
	    is a complex address (no kidding, right?).  The algorithm for
	    trying to resolve it is to move all the way to the right, then
	    back up left until the first '!' then from there to the SECOND
	    metacharacter on the right is the name@host address...(in this
            example, it would be "joe%sytech").  Check this in the routing
	    table.  If not present, keep backing out to the right until we
	    find a host that is present, or we hit the '@' sign.  Once we
	    have a 'normal' ARPA address, hand it to optimize_arpa().
	**/

	char name[NLEN], buffer[SLEN], junk[SLEN];
	char host[NLEN], old_host[NLEN];
	register int i, loc, nloc = 0, hloc = 0, passes = 1;

	/** first off, get the name%host... **/

	for (loc = strlen(address)-1; address[loc] != '!' && loc > -1; loc--)
	   ;

	while (address[loc] != '\0') {

	  if (passes == 1) {
	    loc++;

	    while (address[loc] != '%' && address[loc] != '@')
	      name[nloc++] = address[loc++];
	  }
	  else {
	    for (i=0; old_host[i] != '\0'; i++)
	      name[nloc++] = old_host[i];
	  }

	  loc++;

	  while (address[loc] != '%' && address[loc] != '@')
	    host[hloc++] = address[loc++];

	  host[hloc] = name[nloc] = '\0';

	  strcpy(old_host, host);

	  sprintf(buffer, "%s@%s", name, shift_lower(host));

	  if (expand_site(buffer, junk) == 0) {
	    strcpy(address, buffer);
	    return;
	  }
	  else if (address[loc] == '@') {
	    optimize_arpa(address);
	    return;
	  }
	  else
	    name[nloc++] = '%';	/* for next pass through */

	}
}

optimize_arpa(address)
char *address;
{
	/** Get an arpa address and simplify it to the minimal
	    route needed to get mail to this person... **/

	char name[NLEN], buffer[SLEN], junk[SLEN];
	char host[NLEN];
	register int loc, nloc = 0, hloc = 0, at_sign = 0;

	for (loc = strlen(address)-1; address[loc] != '!' && loc > -1; loc--) {
	  if (address[loc] == '@')
	     at_sign++;	/* remember this spot! */
	  else if (at_sign)
	    name[nloc++] = address[loc];
	  else
	    host[hloc++] = address[loc];
	}

	name[nloc] = host[hloc] = '\0';

	reverse(name);
	reverse(host);

	sprintf(buffer,"%s@%s", name, shift_lower(host));

	if (expand_site(buffer, junk) == 0) {
	  strcpy(address, buffer);
	  return;
	}

	optimize_usenet(address);	/* that didn't work... */
}

optimize_usenet(address)
char *address;
{
	/** optimize the return address IFF it's a standard usenet
	    address...
	**/

	char name[NLEN],  new_address[SLEN], buffer[SLEN], junk[SLEN];
	register int loc, nloc = 0, aloc = 0, passes = 1;

	for (loc = strlen(address)-1; address[loc] != '!' && loc > -1; loc--)
	  name[nloc++] = address[loc];
	name[nloc] = '\0';

	reverse(name);

	new_address[0] = '\0';

	/* got name, now get machine until we can get outta here */

	while (loc > -1) {

	  new_address[aloc++] = address[loc--];	/* the '!' char */

	  while (address[loc] != '!' && loc > -1)
	    new_address[aloc++] = address[loc--];

	  new_address[aloc] = '\0';

	  strcpy(buffer, new_address);
	  reverse(buffer);

	  if (expand_site(buffer, junk) == 0) {
	    if (passes == 1 && chloc(name, '@') == -1) {
	      buffer[strlen(buffer) - 1] = '\0';	/* remove '!' */
	      sprintf(address, "%s@%s", name, buffer);
	    }
	    else
	      sprintf(address, "%s%s", buffer, name);
	    return;		/* success! */
	  }
	  passes++;
	}

	return;		/* nothing to do! */
}

#endif	/* OPTIMIZE_RETURN */

int
get_return(buffer, msgnum)
char *buffer;
int msgnum;
{
	/** reads msgnum message again, building up the full return
	    address including all machines that might have forwarded
	    the message.  Returns whether it is using the To line **/

	char buf[SLEN], name1[SLEN], name2[SLEN], lastname[SLEN];
	char hold_return[SLEN], alt_name2[SLEN], buf2[SLEN];
	int ok = 1, lines, len_buf, len_buf2;
	int using_to = FALSE, in_header = FALSE;

	/* now initialize all the char buffers [thanks Keith!] */

	buf[0] = name1[0] = name2[0] = lastname[0] = '\0';
	hold_return[0] = alt_name2[0] = buf2[0] = '\0';

	/** get to the first line of the message desired **/

	if(msgnum < 0 || msgnum >= message_count || message_count < 1) {
	  dprint(1, (debugfile,
		"Error: %d not a valid message number message_count = %d (%s)",
		msgnum, message_count, "get_return"));
	  error1("%d not a valid message number!");
	  return(using_to);
	}

	if (fseek(mailfile, headers[msgnum]->offset, 0) == -1) {
	  dprint(1, (debugfile,
		"Error: seek %ld bytes into file hit errno %s (%s)",
		headers[msgnum]->offset, error_name(errno),
	        "get_return"));
	  error2("Couldn't seek %d bytes into file (%s).",
	       headers[msgnum]->offset, error_name(errno));
	  return(using_to);
	}

	/** okay!  Now we're there!  **/

	lines = headers[msgnum]->lines;
	in_header = TRUE;

	buffer[0] = '\0';

	ok = (int) (fgets(buf2, SLEN, mailfile) != NULL);
	if (ok) {
	  fixline(buf2);
	  len_buf2 = strlen(buf2);
	  if(buf2[len_buf2-1] == '\n') lines--; /* got a full line */
	}

	while (ok && lines) {
	  buf[0] = '\0';
	  strncat(buf, buf2, SLEN);
	  len_buf = strlen(buf);
	  ok = (int) (fgets(buf2, SLEN, mailfile) != NULL);
	  if (ok) {
	    fixline(buf2);
	    len_buf2 = strlen(buf2);
	    if(buf2[len_buf2-1] == '\n') lines--; /* got a full line */
	  }
	  while (ok && lines && whitespace(buf2[0]) && len_buf >= 2) {
	    if (buf[len_buf-1] == '\n') {
	      len_buf--;
	      buf[len_buf] = '\0';
	    }
	    strncat(buf, buf2, (SLEN-len_buf-1));
	    len_buf = strlen(buf);
	    ok = (int) (fgets(buf2, SLEN, mailfile) != NULL);
	    if (ok) {
	      len_buf2 = strlen(buf2);
	      if(buf2[len_buf2-1] == '\n') lines--; /* got a full line */
	    }
	  }

/* At this point, "buf" contains the unfolded header line, while "buf2" contains
   the next single line of text from the mail file */

	  if (in_header) {
	    if (len_buf == 1) /* \n only */
	      in_header = FALSE;
	  }
	  else {
	    if (first_word(buf, "Forwarded "))
	      in_header = TRUE;
	  }

	  if (!in_header)
	    continue;

	  if (first_word(buf, "From "))
	    sscanf(buf, "%*s %s", hold_return);
	  else if (first_word(buf, ">From")) {
	    sscanf(buf,"%*s %s %*s %*s %*s %*s %*s %*s %*s %s %s",
	           name1, name2, alt_name2);
	    if (strcmp(name2, "from") == 0)		/* remote from xyz  */
	      strcpy(name2, alt_name2);
	    else if (strcmp(name2, "by") == 0)	/* forwarded by xyz */
	      strcpy(name2, alt_name2);
	    add_site(buffer, name2, lastname);
	  }

#ifdef USE_EMBEDDED_ADDRESSES

	  else if (first_word(buf, "From:")) {
	    get_address_from("From:", buf, hold_return);
	    buffer[0] = '\0';
          }
          else if (first_word(buf, "Reply-To:")) {
	    get_address_from("Reply-To:", buf, buffer);
	    return(using_to);
          }

#endif

	  else if (len_buf < 2)	/* done with header */
            lines = 0; /* let's get outta here!  We're done!!! */
	}

	if (buffer[0] == '\0')
	  strcpy(buffer, hold_return); /* default address! */
	else
	  add_site(buffer, name1, lastname);	/* get the user name too! */

	if (first_word(buffer, "To:")) {	/* for backward compatibility */
	  get_existing_address(buffer,msgnum);
	  using_to = TRUE;
	}
	else {
	  /*
	   * KLUDGE ALERT - DANGER WILL ROBINSON
	   * We can't just leave a bare login name as the return address,
	   * or it will be alias-expanded.
	   * So we qualify it with the current host name (and, maybe, domain).
	   * Sigh.
	   */

	  if (chloc(buffer, '@') < 0
	   && chloc(buffer, '%') < 0
	   && chloc(buffer, '!') < 0)
	  {
#ifdef INTERNET
	    sprintf(buffer + strlen(buffer), "@%s", hostfullname);
#else
	    strcpy(buf, buffer);
	    sprintf(buffer, "%s!%s", hostname, buf);
#endif
	  }

	  /*
	   * If we have a space character,
	   * or we DON'T have '!' or '@' chars,
	   * append the user-readable name.
	   */
	  if (chloc(headers[msgnum]->from, ' ') >= 0 ||
	      (chloc(headers[msgnum]->from, '!') < 0 &&
	       chloc(headers[msgnum]->from, '@') < 0)) {
	       sprintf(buffer + strlen(buffer),
		       " (%s)", headers[msgnum]->from);
          }
	}

	return(using_to);
}

get_existing_address(buffer, msgnum)
char *buffer;
int msgnum;
{
	/** This routine is called when the message being responded to has
	    "To:xyz" as the return address, signifying that this message is
	    an automatically saved copy of a message previously sent.  The
	    correct to address can be obtained fairly simply by reading the
	    To: header from the message itself and (blindly) copying it to
	    the given buffer.  Note that this header can be either a normal
	    "To:" line (Elm) or "Originally-To:" (previous versions e.g.Msg)
	**/

	char mybuf[LONG_STRING];
	register char ok = 1, in_to = 0;

	buffer[0] = '\0';

	/** first off, let's get to the beginning of the message... **/

	if(msgnum < 0 || msgnum >= message_count || message_count < 1) {
	  dprint(1, (debugfile,
		"Error: %d not a valid message number message_count = %d (%s)",
		msgnum, message_count, "get_existing_address"));
	  error1("%d not a valid message number!");
	  return;
	}
        if (fseek(mailfile, headers[msgnum]->offset, 0) == -1) {
	    dprint(1, (debugfile,
		    "Error: seek %ld bytes into file hit errno %s (%s)",
		    headers[msgnum]->offset, error_name(errno),
		    "get_existing_address"));
	    error2("Couldn't seek %d bytes into the file (%s).",
	           headers[msgnum]->offset, error_name(errno));
	    return;
        }

        /** okay!  Now we're there!  **/

        while (ok) {
          ok = (int) (fgets(mybuf, LONG_STRING, mailfile) != NULL);
	  no_ret(mybuf);	/* remove return character */

          if (first_word(mybuf, "To:")) {
	    in_to = TRUE;
	    strcpy(buffer, (char *) mybuf + strlen("To: "));
          }
	  else if (first_word(mybuf, "Original-To:")) {
	    in_to = TRUE;
	    strcpy(buffer, (char *) mybuf + strlen("Original-To:"));
	  }
	  else if (in_to && whitespace(mybuf[0])) {
	    strcat(buffer, " ");		/* tag a space in   */
	    strcat(buffer, (char *) mybuf + 1);	/* skip 1 whitespace */
	  }
	  else if (strlen(mybuf) < 2)
	    return;				/* we're done for!  */
	  else
	    in_to = 0;
      }
}
