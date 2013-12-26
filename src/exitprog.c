
static char rcsid[] = "@(#)$Id: exitprog.c,v 4.1 90/04/28 22:43:00 syd Exp $";

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
 * $Log:	exitprog.c,v $
 * Revision 4.1  90/04/28  22:43:00  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

#include "headers.h"

int
exit_prog()
{
	/** Exit, abandoning all changes to the mailbox (if there were
	    any, and if the user say's it's ok)
	**/

	char buffer[SLEN], answer;
	register int i, changes;

	dprint(1, (debugfile, "\n\n-- exiting --\n\n"));

	/* Determine if any messages are scheduled for deletion, or if
	 * any message has changed status
	 */
	for (changes = 0, i = 0; i < message_count; i++)
	  if (ison(headers[i]->status, DELETED) || headers[i]->status_chgd)
	    changes++;

	if (changes) {
	  /* YES or NO on softkeys */
	  if (hp_softkeys) {
	    define_softkeys(YESNO);
	    softkeys_on();
	  }
	  sprintf(buffer,"Abandon change%s to mailbox? (y/n) ",plural(changes));
	  answer = want_to(buffer, 'n');

	  if(answer != 'y') return -1;
	}

	fflush(stdout);
	return leave(0);
}
