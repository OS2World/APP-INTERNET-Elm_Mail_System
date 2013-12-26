
static char rcsid[] = "@(#)$Id: date.c,v 4.1 90/04/28 22:42:41 syd Exp $";

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
 * $Log:	date.c,v $
 * Revision 4.1  90/04/28  22:42:41  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** return the current date and time in a readable format! **/
/** also returns an ARPA RFC-822 format date...            **/


#include "headers.h"

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#ifdef BSD
#  include <sys/types.h>
#  include <sys/timeb.h>
#endif

#include <ctype.h>

#ifndef	_POSIX_SOURCE
extern struct tm *localtime(), *gmtime();
extern time_t	  time();
#endif

#ifdef BSD
#undef toupper
#undef tolower
#endif

#define MONTHS_IN_YEAR	11	/* 0-11 equals 12 months! */
#define FEB		 1	/* 0 = January 		  */
#define DAYS_IN_LEAP_FEB 29	/* leap year only 	  */

#define ampm(n)		(n > 12? n - 12 : n)
#define am_or_pm(n)	(n > 11? (n > 23? "am" : "pm") : "am")
#define leapyear(year)	((year % 4 == 0) && (year % 100 != 0))

char *arpa_dayname[] = { "Sun", "Mon", "Tue", "Wed", "Thu",
		  "Fri", "Sat", "" };

char *arpa_monname[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
		  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", ""};

int  days_in_month[] = { 31,    28,    31,    30,    31,     30,
		  31,     31,    30,   31,    30,     31,  -1};

char *tzoffset(long time)
{
  static char offset[SLEN];
  struct tm tm = *gmtime(&time);
  long utime = mktime(&tm);
  sprintf(offset, "%+03d00", (time - utime) / 3600);
  return offset;
}

char *get_arpa_date()
{
	/** returns an ARPA standard date.  The format for the date
	    according to DARPA document RFC-822 is exemplified by:

	       	      Mon, 12 Aug 85 6:29:08 -0800

	**/

	static char buffer[SLEN];	/* static character buffer       */
	struct tm the_time;		/* Time structure, see CTIME(3C) */
	long	   junk;		/* time in seconds....		 */

#ifdef BSD
# ifndef TZ_MINUTESWEST
	junk = time((long *) 0);
# else
	struct  timeval  time_val;
	struct  timezone time_zone;

	gettimeofday(&time_val, &time_zone);
	junk = time_val.tv_sec;
# endif

#else
	junk = time((long *) 0);	/* this must be here for it to work! */
#endif

	the_time = *localtime(&junk);

	sprintf(buffer, "%s, %d %s %d %d:%02d:%02d %s",
	  arpa_dayname[the_time.tm_wday],
	  the_time.tm_mday % 32,
	  arpa_monname[the_time.tm_mon],
	  the_time.tm_year % 100,
	  the_time.tm_hour % 24,
	  the_time.tm_min  % 61,
	  the_time.tm_sec  % 61,
	  tzoffset(junk));

	return( (char *) buffer);
}

days_ahead(days, buffer)
int days;
char *buffer;
{
	/** return in buffer the date (Day, Mon Day, Year) of the date
	    'days' days after today.
	**/

	struct tm *the_time;		/* Time structure, see CTIME(3C) */
	long	   junk;		/* time in seconds....		 */

	junk = time((long *) 0);	/* this must be here for it to work! */
	the_time = localtime(&junk);

	/* increment the day of the week */

	the_time->tm_wday = (the_time->tm_wday + days) % 7;

	/* the day of the month... */
	the_time->tm_mday += days;

        while (the_time->tm_mday > days_in_month[the_time->tm_mon]) {
          if (the_time->tm_mon == FEB && leapyear(the_time->tm_year)) {
            if (the_time->tm_mday > DAYS_IN_LEAP_FEB) {
              the_time->tm_mday -= DAYS_IN_LEAP_FEB;
              the_time->tm_mon += 1;
            }
            else
              break;            /* Is Feb 29, so leave */
          }
          else {
            the_time->tm_mday -= days_in_month[the_time->tm_mon];
            the_time->tm_mon += 1;
          }

          /* check the month of the year */
          if (the_time->tm_mon > MONTHS_IN_YEAR) {
            the_time->tm_mon -= (MONTHS_IN_YEAR + 1);
            the_time->tm_year += 1;
          }
        }

        /* now, finally, build the actual date string */

	sprintf(buffer, "%s, %d %s %d",
	  arpa_dayname[the_time->tm_wday],
	  the_time->tm_mday % 32,
	  arpa_monname[the_time->tm_mon],
	  the_time->tm_year % 100);
}

fix_date(entry)
struct header_rec *entry;
{
	/** This routine will 'fix' the date entry for the specified
	    message.  This consists of 1) adjusting the year to 0-99
	    and 2) altering time from HH:MM:SS to HH:MM am|pm **/

	if (atoi(entry->year) > 99)
	  sprintf(entry->year,"%d", atoi(entry->year) - 1900);

	fix_time(entry->time);
}

fix_time(timestring)
char *timestring;
{
	/** Timestring in format HH:MM:SS (24 hour time).  This routine
	    will fix it to display as: HH:MM [am|pm] **/

	int hour, minute;

	sscanf(timestring, "%d:%d", &hour, &minute);

	if (hour < 1 || hour == 24)
	  sprintf(timestring, "12:%02d am", minute);
	else if (hour < 12)
	  sprintf(timestring, "%d:%02d am", hour, minute);
	else if (hour == 12)
	  sprintf(timestring, "12:%02d pm", minute);
	else if (hour < 24)
	  sprintf(timestring, "%d:%02d pm", hour-12, minute);
}

int
compare_parsed_dates(rec1, rec2)
struct date_rec rec1, rec2;
{
	/** This function is very similar to the compare_dates
	    function but assumes that the two record structures
	    are already parsed and stored in "date_rec" format.
	**/

	if (rec1.year != rec2.year)
	  return( rec1.year - rec2.year );

	if (rec1.month != rec2.month)
	  return( rec1.month - rec2.month );

	if (rec1.day != rec2.day)
	  return( rec1.day - rec2.day );

	if (rec1.hour != rec2.hour)
	  return( rec1.hour - rec2.hour );

	return( rec1.minute - rec2.minute );		/* ignore seconds... */
}

int
month_number(name)
char *name;
{
	/** return the month number given the month name... **/

	char ch;

	switch (tolower(name[0])) {
	 case 'a' : if ((ch = tolower(name[1])) == 'p')	return(APRIL);
		    else if (ch == 'u') return(AUGUST);
		    else return(-1);	/* error! */

	 case 'd' : return(DECEMBER);
	 case 'f' : return(FEBRUARY);
	 case 'j' : if ((ch = tolower(name[1])) == 'a') return(JANUARY);
		    else if (ch == 'u') {
	              if ((ch = tolower(name[2])) == 'n') return(JUNE);
		      else if (ch == 'l') return(JULY);
		      else return(-1);		/* error! */
	            }
		    else return(-1);		/* error */
	 case 'm' : if ((ch = tolower(name[2])) == 'r') return(MARCH);
		    else if (ch == 'y') return(MAY);
		    else return(-1);		/* error! */
	 case 'n' : return(NOVEMBER);
	 case 'o' : return(OCTOBER);
	 case 's' : return(SEPTEMBER);
	 default  : return(-1);
	}
}

#ifdef SITE_HIDING

char *get_ctime_date()
{
	/** returns a ctime() format date, but a few minutes in the
	    past...(more cunningness to implement hidden sites) **/

	static char buffer[SLEN];	/* static character buffer       */
	struct tm *the_time;		/* Time structure, see CTIME(3C) */
	long	   junk;		/* time in seconds....		 */

#ifdef BSD
	struct  timeval  time_val;
	struct  timezone time_zone;
#endif

#ifdef BSD
	gettimeofday(&time_val, &time_zone);
	junk = time_val.tv_sec;
#else
	junk = time((long *) 0);	/* this must be here for it to work! */
#endif
	the_time = localtime(&junk);

	sprintf(buffer, "%s %s %d %02d:%02d:%02d %d",
	  arpa_dayname[the_time->tm_wday],
	  arpa_monname[the_time->tm_mon],
	  the_time->tm_mday % 32,
	  min(the_time->tm_hour % 24, (rand() % 24)),
	  min(abs(the_time->tm_min  % 61 - (rand() % 60)), (rand() % 60)),
	  min(abs(the_time->tm_sec  % 61 - (rand() % 60)), (rand() % 60)),
	  the_time->tm_year % 100 + 1900);

	return( (char *) buffer);
}

#endif
