
static char rcsid[] = "@(#)$Id: bouncebk.c,v 4.1 90/04/28 22:42:33 syd Exp $";

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
 * $Log:	bouncebk.c,v $
 * Revision 4.1  90/04/28  22:42:33  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This set of routines implement the bounceback feature of the mailer.
    This feature allows mail greater than 'n' hops away (n specified by
    the user) to have a 'cc' to the user through the remote machine.

    Due to the vagaries of the Internet addressing (uucp -> internet -> uucp)
    this will NOT generate bounceback copies with mail to an internet host!

**/

#include "headers.h"

char *bounce_off_remote(),		/* forward declaration */
     *strcat(), *strcpy();

int
uucp_hops(to)
char *to;
{
	/** Given the entire "To:" list, return the number of hops in the
	    first address (a hop = a '!') or ZERO iff the address is to a
  	    non uucp address.
	**/

	register int hopcount = 0, iindex;

	for (iindex = 0; ! whitespace(to[iindex]) && to[iindex] != '\0'; iindex++) {
	  if (to[iindex] == '!')
	    hopcount++;
	  else if (to[iindex] == '@' || to[iindex] == '%' || to[iindex] == ':')
	    return(0);	/* don't continue! */
	}

	return(hopcount);
}

char *bounce_off_remote(to)
char *to;
{
	/** Return an address suitable for framing (no, that's not it...)
	    Er, suitable for including in a 'cc' line so that it ends up
	    with the bounceback address.  The method is to take the first
	    address in the To: entry and break it into machines, then
	    build a message up from that.  For example, consider the
	    following address:
			a!b!c!d!e!joe
	    the bounceback address would be;
			a!b!c!d!e!d!c!b!a!ourmachine!ourname
	    simple, eh?
	**/

	static char address[LONG_STRING];	/* BEEG address buffer! */

	char   host[MAX_HOPS][NLEN];	/* for breaking up addr */
	register int hostcount = 0, hindex = 0,
	       iindex;

	for (iindex = 0; !whitespace(to[iindex]) && to[iindex] != '\0'; iindex++) {
	  if (to[iindex] == '!') {
	    host[hostcount][hindex] = '\0';
	    hostcount++;
	    hindex = 0;
	  }
	  else
	    host[hostcount][hindex++] = to[iindex];
	}

	/* we have hostcount hosts... */

	strcpy(address, host[0]);	/* initialize it! */

	for (iindex=1; iindex < hostcount; iindex++) {
	  strcat(address, "!");
	  strcat(address, host[iindex]);
	}

	/* and now the same thing backwards... */

	for (iindex = hostcount -2; iindex > -1; iindex--) {
	  strcat(address, "!");
	  strcat(address, host[iindex]);
	}

	/* and finally, let's tack on our machine and login name */

	strcat(address, "!");
	strcat(address, hostname);
	strcat(address, "!");
	strcat(address, username);

	/* and we're done!! */

	return( (char *) address );
}
