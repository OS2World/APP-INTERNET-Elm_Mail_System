
static char rcsid[] = "@(#)$Id: calendar.c,v 4.1.1.1 90/06/21 22:16:50 syd Exp $";

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
 * $Log:	calendar.c,v $
 * Revision 4.1.1.1  90/06/21  22:16:50  syd
 * Add skip leading whitespace
 * From Jerry Pendergrafyt
 *
 * Revision 4.1  90/04/28  22:42:36  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This routine implements a rather snazzy idea suggested by Warren
    Carithers of the Rochester Institute of Technology that allows
    mail to contain entries formatted in a manner that will allow direct
    copying into a users calendar program.

    All lines in the current message beginning with "->", e.g.

	-> Mon 04/21 1:00p meet with chairman candidate

    get copied into the user's calendar file.

**/

#include "headers.h"

#ifdef ENABLE_CALENDAR		/* if not defined, this will be an empty file */

#include <errno.h>

#ifndef OS2
extern int errno;
#endif

char *error_name(), *error_description(), *strcpy();

scan_calendar()
{
	FILE *calendar;
	int  count;

	/* First step is to open the calendar file for appending... **/

	if (can_open(calendar_file, "a") != 0) {
	  dprint(2, (debugfile,
		  "Error: wrong permissions to append to calendar %s\n",
		  calendar_file));
	  dprint(2, (debugfile, "** %s - %s **\n",
		  error_name(errno), error_description(errno)));
	  error1("Not able to append to file %s!", calendar_file);
	  return;
	}

	save_file_stats(calendar_file);

	if ((calendar = fopen(calendar_file,"a")) == NULL) {
	  dprint(2, (debugfile,
		"Error: couldn't append to calendar file %s (scan)\n",
		calendar_file));
	  dprint(2, (debugfile, "** %s - %s **\n",
		  error_name(errno), error_description(errno)));
	  error1("Couldn't append to file %s!", calendar_file);
	  return;
	}

	count = extract_info(calendar);

	fclose(calendar);

	restore_file_stats(calendar_file);

	if (count > 0)
	  error2("%d entr%s saved in calendar file.",
		 count, count > 1 ? "ies" : "y");
	else
	  error("No calendar entries found in that message.");

	return;
}

int
extract_info(save_to_fd)
FILE *save_to_fd;
{
	/** Save the relevant parts of the current message to the given
	    calendar file.  The only parameter is an opened file
	    descriptor, positioned at the end of the existing file **/

	register int entries = 0, lines;
	char buffer[SLEN], *cp, *is_cal_entry();

    	/** get to the first line of the message desired **/

    	if (fseek(mailfile, headers[current-1]->offset, 0) == -1) {
       	  dprint(1,(debugfile,
		"ERROR: Attempt to seek %d bytes into file failed (%s)",
		headers[current-1]->offset, "extract_info"));
       	  error1("ELM [seek] failed trying to read %d bytes into file.",
	     	headers[current-1]->offset);
       	  return(0);
    	}

        /* how many lines in message? */

        lines = headers[current-1]->lines;

        /* now while not EOF & still in message... scan it! */

	while (lines) {

          if(fgets(buffer, SLEN, mailfile) == NULL)
	    break;

	  if(buffer[strlen(buffer)-1] == '\n')
	    lines--;					/* got a full line */

	  if((cp = is_cal_entry(buffer)) != NULL) {
	    entries++;
	    fprintf(save_to_fd,"%s", cp);
	  }

	}
	dprint(4,(debugfile,
		"Got %d calender entr%s.\n", entries, entries > 1? "ies":"y"));

	return(entries);
}

char *
is_cal_entry(string)
register char *string;
{
	/* If string is of the form
         * {optional white space} ->{optional white space} {stuff}
	 * return a pointer to stuff, otherwise return NULL.
	 */
	while( whitespace(*string) )
	  string++;      /* strip leading W/S */

	if(strncmp(string, "->", 2) == 0) {
	  for(string +=2 ; whitespace(*string); string++)
		  ;
	  return(string);
	}
	return(NULL);
}

#endif
