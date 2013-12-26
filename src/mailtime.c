
static char rcsid[] = "@(#)$Id: mailtime.c,v 4.1 90/04/28 22:43:31 syd Exp $";

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
 * $Log:	mailtime.c,v $
 * Revision 4.1  90/04/28  22:43:31  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This set of routines is used to figure out when the user last read
    their mail and to also figure out if a given message is new or not.

**/

#include "headers.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif

resolve_received(entry)
struct header_rec *entry;
{
	/** Entry has the data for computing the time and date the
	    message was received.  Fix it and return **/

	switch (tolower(entry->month[0])) {
	  case 'j' : if (tolower(entry->month[1]) == 'a')
		       entry->received.month = JANUARY;
		     else if (tolower(entry->month[2]) == 'n')
	               entry->received.month = JUNE;
		     else
	               entry->received.month = JULY;
	             break;
	  case 'f' : entry->received.month = FEBRUARY;
	 	     break;
	  case 'm' : if (tolower(entry->month[2]) == 'r')
	               entry->received.month = MARCH;
		     else
		       entry->received.month = MAY;
	             break;
	  case 'a' : if (tolower(entry->month[1]) == 'p')
	               entry->received.month = APRIL;
	             else
	               entry->received.month = AUGUST;
		     break;
	  case 's' : entry->received.month = SEPTEMBER;
		     break;
	  case 'o' : entry->received.month = OCTOBER;
		     break;
	  case 'n' : entry->received.month = NOVEMBER;
	  	     break;
	  case 'd' : entry->received.month = DECEMBER;
		     break;
	}

	sscanf(entry->day, "%d", &(entry->received.day));

	sscanf(entry->year, "%d", &(entry->received.year));
	if (entry->received.year > 100) entry->received.year -= 1900;

	sscanf(entry->time, "%d:%d", &(entry->received.hour),
	       &(entry->received.minute));
}
