
static char rcsid[] = "@(#)$Id: aliaslib.c,v 4.1.1.4 90/08/02 21:57:53 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.4 $   $State: Exp $
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
 * $Log:	aliaslib.c,v $
 * Revision 4.1.1.4  90/08/02  21:57:53  syd
 * The newly introduced function 'stricmp' has a name conflict with a libc
 * function under SunOS 4.1.  Changed name to istrcmp.
 * From: scs@lokkur.dexter.mi.us (Steve Simmons)
 *
 * Revision 4.1.1.3  90/07/12  23:18:17  syd
 * Make domain name checking case independent
 * From: Syd, reported by Steven Baur
 *
 * Revision 4.1.1.2  90/06/05  21:31:34  syd
 * Fix now spurious error message for alias recursive expansion
 * when alias lookup is on a string over 20 chars long.  If that
 * long, its just not an alias, so just return.
 * From: Syd
 *
 * Revision 4.1.1.1  90/06/05  20:41:25  syd
 * Fix boundary condition in add_name_to_list() where it fails to
 * print error message.
 * From: Chip Rosenthal <chip@chinacat.Unicom.COM>
 *
 * Revision 4.1  90/04/28  22:42:29  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Library of functions dealing with the alias system...

 **/

#include "headers.h"
#include <ctype.h>

char *get_alias_address(), *get_token(), *strpbrk(), *index();
long lseek();

#ifdef DONT_TOUCH_ADDRESSES
char *expand_system();
#endif

/*
 * Expand "name" as an alias and return a pointer to static data containing
 * the expansion.  If "name" is not an alias, then NULL is returned.
 */
char *get_alias_address(name, mailing)
char *name;	/* name to expand as an alias				*/
int mailing;	/* TRUE to fully expand group names & recursive aliases	*/
{
	static char buffer[VERY_LONG_STRING];
	char *bufptr;
	int bufsize;

	/* reads files iff changed since last read */
	read_alias_files();

	/* if name is an alias then return its expansion */
	bufptr = buffer;
	bufsize = sizeof(buffer);
	if ( do_get_alias(name, &bufptr, &bufsize, mailing, FALSE, 0) )
	  return buffer+2;	/* skip comma/space from add_name_to_list() */

	/* nope...not an alias */
	return (char *) NULL;
}


/*
 * Determine if "name" is an alias, and if so expand it and store the result in
 * "*bufptr".  TRUE returned if any expansion occurs, else FALSE is returned.
 */
int do_get_alias(name, bufptr, bufsizep, mailing, sysalias, depth)
char *name;	/* name to expand as an alias				*/
char **bufptr;	/* place to store result of expansion			*/
int *bufsizep;	/* available space in the buffer			*/
int mailing;	/* TRUE to fully expand group names & recursive aliases	*/
int sysalias;	/* TRUE to suppress checks of the user's aliases	*/
int depth;	/* recursion depth - initially call at depth=0		*/
{
	char abuf[LONG_STRING];
	int loc;

	/* update the recursion depth counter */
	++depth;

	dprint(6, (debugfile, "%*s->attempting alias expansion on \"%s\"\n",
		(depth*2), "", name));

	/* strip out (comments) and leading/trailing whitespace */
	remove_possible_trailing_spaces( name = strip_parens(name) );
	for ( ; isspace(*name)  ; ++name ) ;

	/* throw back empty addresses */
	if ( *name == '\0' )
	  return FALSE;

	/* check for a user alias, unless in the midst of sys alias expansion */
	if ( !sysalias && user_data != -1 ) {
	  if ( (loc = find(name, user_hash_table, MAX_UALIASES)) >= 0 ) {
	    lseek(user_data, ntohl(user_hash_table[loc].byte), 0L);
	    get_line(user_data, abuf);
	    goto do_expand;
	  }
	}

	/* check for a system alias */
	if ( system_data != -1 ) {
	  if ( (loc = find(name, system_hash_table, MAX_SALIASES)) >= 0 ) {
	    lseek(system_data, ntohl(system_hash_table[loc].byte), 0L);
	    get_line(system_data, abuf);
	    sysalias = TRUE;
	    goto do_expand;
	  }
	}

	/* nope...this name wasn't an alias */
	return FALSE;

do_expand:

	/* at this point, alias is expanded into "abuf" - now what to do... */

	dprint(7, (debugfile, "%*s  ->expanded alias to \"%s\"\n",
	    (depth*2), "", abuf));

	/* check for an exact match */
	loc = strlen(name);
	if (strncmp(name, abuf, loc) == 0 && (abuf[loc] == ' ' || abuf[loc] == '\0'))
#ifdef DONT_TOUCH_ADDRESSES
	  return add_name_to_list(abuf, bufptr, bufsizep);
#else
	  return add_name_to_list(expand_system(abuf, TRUE), bufptr, bufsizep);
#endif

	/* see if we are stuck in a loop */
	if ( depth > 12 ) {
	  dprint(2, (debugfile,
	      "alias expansion loop detected at \"%s\" - bailing out\n", name));
	    error1("Error expanding \"%s\" - probable alias definition loop.",
	      name);
	    return FALSE;
	}

	/* see if the alias equivalence is a group name */
	if ( mailing && abuf[0] == '!' )
	  return do_expand_group(abuf+1, bufptr, bufsizep, sysalias, depth);

	/* see if the alias equivalence is an email address */
	if ( strpbrk(abuf,"!@:") != NULL ) {
#ifdef DONT_TOUCH_ADDRESSES
	  return add_name_to_list(abuf, bufptr, bufsizep);
#else
	  return add_name_to_list(expand_system(abuf, TRUE), bufptr, bufsizep);
#endif
	}

	/* see if the alias equivalence is itself an alias */
	if ( mailing && do_get_alias(abuf,bufptr,bufsizep,TRUE,sysalias,depth) )
	  return TRUE;

	/* the alias equivalence must just be a local address */
	return add_name_to_list(abuf, bufptr, bufsizep);
}


/*
 * Expand the comma-delimited group of names in "group", storing the result
 * in "*bufptr".  Returns TRUE if expansion occurs OK, else FALSE in the
 * event of errors.
 */
int do_expand_group(group, bufptr, bufsizep, sysalias, depth)
char *group;	/* group list to expand					*/
char **bufptr;	/* place to store result of expansion			*/
int *bufsizep;	/* available space in the buffer			*/
int sysalias;	/* TRUE to suppress checks of the user's aliases	*/
int depth;	/* nesting depth					*/
{
	char *name;

	/* go through each comma-delimited name in the group */
	while ( group != NULL ) {

	  /* extract the next name from the list */
	  for ( name = group ; isspace(*name) ; ++name ) ;
	  if ( (group = index(name,',')) != NULL )
	      *group++ = '\0';
	  remove_possible_trailing_spaces(name);
	  if ( *name == '\0' )
	    continue;

	  /* see if this name is really an alias */
	  if ( do_get_alias(name, bufptr, bufsizep, TRUE, sysalias, depth) )
	    continue;

	  /* verify it is a valid address */
	  if ( !valid_name(name) ) {
	    dprint(3, (debugfile,
		"Illegal address %s during list expansion in %s\n",
		name, "do_get_alias"));
	    error1("%s is an illegal address!", name);
	    return FALSE;
	  }

	  /* add it to the list */
	  if ( !add_name_to_list(name, bufptr, bufsizep) )
	    return FALSE;

	}

	return TRUE;
}


/*
 * Append "<comma><space>name" to the list, checking to ensure the buffer
 * does not overflow.  Upon return, *bufptr and *bufsizep will be updated to
 * reflect the stuff added to the buffer.  If a buffer overflow would occur,
 * an error message is printed and FALSE is returned, else TRUE is returned.
 */
int add_name_to_list(name,bufptr,bufsizep)
register char *name;	/* name to append to buffer			*/
register char **bufptr;	/* pointer to pointer to end of buffer		*/
register int *bufsizep;	/* pointer to space remaining in buffer		*/
{
	if ( *bufsizep < 0 )
	    return FALSE;

	*bufsizep -= strlen(name)+2;
	if ( *bufsizep <= 0 ) {
	    *bufsizep = -1;
	    error("Alias expansion is too long.");
	    return FALSE;
	}

	*(*bufptr)++ = ',';
	*(*bufptr)++ = ' ';
	while ( *name != '\0' )
	  *(*bufptr)++ = *name++ ;
	**bufptr = '\0';

	return TRUE;
}


#ifndef DONT_TOUCH_ADDRESSES
char *expand_system(buffer, show_errors)
char *buffer;
int   show_errors;
{
	/** This routine will check the first machine name in the given path
	    (if any) and expand it out if it is an alias...if not, it will
	    return what it was given.  If show_errors is false, it won't
	    display errors encountered...
	**/

	dprint(6, (debugfile, "expand_system(%s, show-errors=%s)\n", buffer,
		onoff(show_errors)));
	findnode(buffer, show_errors);

	return( (char *) buffer);
}
#endif

int
find(word, table, size)
char *word;
struct alias_rec table[];
int size;
{
	/** find word and return loc, or -1 **/
	register int loc;

	/** cannot be an alias if its longer than 20 chars **/
	if (strlen(word) > 20)
	  return(-1);

	loc = hash_it(word, size);

	while (istrcmp(word, table[loc].name) != 0) {
	  if (table[loc].name[0] == '\0')
	    return(-1);
	  loc = (loc + 1) % size;
	}

	return(loc);
}

int
istrcmp(s1,s2)
register char *s1, *s2;
{
	/* case insensitive comparison */
	register int d;
	for (;;) {
	  d = ( isupper(*s1) ? tolower(*s1) : *s1 )
		  - ( isupper(*s2) ? tolower(*s2) : *s2 ) ;
	  if ( d != 0 || *s1 == '\0' || *s2 == '\0' )
	    return d;
	  ++s1;
	  ++s2;
	}
	/*NOTREACHED*/
}

int
strincmp(s1,s2,n)
register char *s1, *s2;
register int n;
{
	/* case insensitive comparison */
	register int d;
	while (--n >= 0) {
	  d = ( isupper(*s1) ? tolower(*s1) : *s1 )
		  - ( isupper(*s2) ? tolower(*s2) : *s2 ) ;
	  if ( d != 0 || *s1 == '\0' || *s2 == '\0' )
	    return d;
	  ++s1;
	  ++s2;
	}
	return(0);
}

int
hash_it(string, table_size)
register char *string;
int   table_size;
{
	/** compute the hash function of the string, returning
	    it (mod table_size) **/

	register int sum = 0;
	for ( ; *string != '\0' ; ++string )
	  sum += (int) ( isupper(*string) ? tolower(*string) : *string );

	return(sum % table_size);
}

get_line(fd, buffer)
int fd;
char *buffer;
{
	/* Read from file fd.  End read upon reading either
	   EOF or '\n' character (this is where it differs
	   from a straight 'read' command!) */

	register int i= 0;
	char     ch;

	while (read(fd, &ch, 1) > 0)
	  if (ch == '\n' || ch == '\r') {
	    buffer[i] = 0;
	    return;
	  }
	  else
	    buffer[i++] = ch;
}
