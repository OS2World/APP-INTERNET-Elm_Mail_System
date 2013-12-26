
static char rcsid[] = "@(#)$Id: string2.c,v 4.1 90/04/28 22:44:14 syd Exp $";

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
 * $Log:	string2.c,v $
 * Revision 4.1  90/04/28  22:44:14  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This file contains string functions that are shared throughout the
    various ELM utilities...

**/

#include "headers.h"
#include <ctype.h>

#ifdef BSD
#undef tolower
#undef toupper
#endif

char *shift_lower(string)
char *string;
{
	/** return 'string' shifted to lower case.  Do NOT touch the
	    actual string handed to us! **/

	static char buffer[VERY_LONG_STRING];
	register char *bufptr = buffer;

	for (; *string; string++, bufptr++)
	  if (isupper(*string))
	    *bufptr = tolower(*string);
	  else
	    *bufptr = *string;

	*bufptr = 0;

	return( (char *) buffer);
}


int
in_list(list, target)
char *list, *target;

{
	/* Returns TRUE iff target is an item in the list - case ignored.
	 * If target is simple (an atom of an address) match must be exact.
	 * If target is complex (contains a special character that separates
	 * address atoms), the target need only match a whole number of atoms
	 * at the right end of an item in the list. E.g.
	 * target:	item:			match:
	 * joe		joe			yes
	 * joe		jojoe			no (wrong logname)
	 * joe		machine!joe		no (similar logname on a perhaps
	 *					   different machine - to
	 *					   test this sort of item the
	 *					   passed target must include
	 *					   proper machine name, is
	 *					   in next two examples)
	 * machine!joe	diffmachine!joe		no  "
	 * machine!joe	diffmachine!machine!joe	yes
	 * joe@machine	jojoe@machine		no  (wrong logname)
	 * joe@machine	diffmachine!joe@machine	yes
	 */

	register char	*rest_of_list,
			*next_item,
			ch;
	int		offset;
	char		*shift_lower(),
				lower_list[VERY_LONG_STRING],
				lower_target[SLEN];

	rest_of_list = strcpy(lower_list, shift_lower(list));
	strcpy(lower_target, shift_lower(target));
	while((next_item = strtok(rest_of_list, ", \t\n")) != NULL) {
	    /* see if target matches the whole item */
	    if(strcmp(next_item, lower_target) == 0)
		return(TRUE);

	    if(strpbrk(lower_target,"!@%:") != NULL) {

	      /* Target is complex */

	      if((offset = strlen(next_item) - strlen(lower_target)) > 0) {

		/* compare target against right end of next item */
		if(strcmp(&next_item[offset], lower_target) == 0) {

		  /* make sure we are comparing whole atoms */
		  ch=next_item[offset-1];
		  if(ch == '!' || ch == '@' || ch == '%' || ch == ':')
		    return(TRUE);
		}
	      }
	    }
	    rest_of_list = NULL;
	}
	return(FALSE);
}


int
in_string(buffer, pattern)
char *buffer, *pattern;
{
	/** Returns TRUE iff pattern occurs IN IT'S ENTIRETY in buffer. **/

	register int i = 0, j = 0;

	while (buffer[i] != '\0') {
	  while (buffer[i++] == pattern[j++])
	    if (pattern[j] == '\0')
	      return(TRUE);
	  i = i - j + 1;
	  j = 0;
	}
	return(FALSE);
}

int
chloc(string, ch)
char *string, ch;
{
	/** returns the index of ch in string, or -1 if not in string **/
	register int i, len;

	for (i=0, len = strlen(string); i<len; i++)
	  if (string[i] == ch) return(i);
	return(-1);
}

int
occurances_of(ch, string)
char ch, *string;
{
	/** returns the number of occurances of 'ch' in string 'string' **/

	register int count = 0;

	for (; *string; string++)
	  if (*string == ch) count++;

	return(count);
}

remove_possible_trailing_spaces(string)
char *string;
{
	/** an incredibly simple routine that will read backwards through
	    a string and remove all trailing whitespace.
	**/

	register int i;

	for ( i = strlen(string); --i >= 0 && whitespace(string[i]); )
		/** spin backwards, semicolon intented **/ ;

	string[i+1] = '\0';	/* note that even in the worst case when there
				   are no trailing spaces at all, we'll simply
				   end up replacing the existing '\0' with
				   another one!  No worries, as M.G. would say
				*/
}
