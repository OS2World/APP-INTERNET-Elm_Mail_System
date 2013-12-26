
static char rcsid[] = "@(#)$Id: strings.c,v 4.1.1.3 90/08/15 21:48:07 syd Exp $";

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
 * $Log:	strings.c,v $
 * Revision 4.1.1.3  90/08/15  21:48:07  syd
 * the user's (unmodified) limit criteria was being compared w/
 * the lower-case version of the header contents.
 * From: dwolfe@earth.sps.mot.com (Dave Wolfe)
 *
 * Revision 4.1.1.2  90/06/21  22:45:06  syd
 * Make display not show To user if user is also sender
 * From: Marius Olafsson
 *
 * Revision 4.1.1.1  90/06/05  20:38:58  syd
 * Allow nesting on () in comment in address
 * From: Chip Rosenthal <chip@chinacat.Unicom.COM>
 *
 * Revision 4.1  90/04/28  22:44:16  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This file contains all the string oriented functions for the
    ELM Mailer, and lots of other generally useful string functions!

    For BSD systems, this file also includes the function "tolower"
    to translate the given character from upper case to lower case.

**/

#include "headers.h"
#include <ctype.h>

#ifdef BSD
#undef tolower
#undef toupper
#endif

/** forward declarations **/

char *format_long(), *strip_commas(), *tail_of_string(),
     *get_token(), *strip_parens(), *argv_zero(), *strcpy(), *strncpy();

char *index();


copy_sans_escape(dest, source, len)
char *dest, *source;
int  len;
{
	/** this performs the same function that strncpy() does, but
	    also will translate any escape character to a printable
	    format (e.g. ^(char value + 32))
	**/

	register int i = 0, j = 0;

	while (i < len && source[i] != '\0') {
	  if (iscntrl(source[i]) && source[i] != '\t') {
	     dest[j++] = '^';
	     dest[j++] = source[i++] + 'A' - 1;
	  }
	  else
	    dest[j++] = source[i++];
	}

	dest[j] = '\0';
}

/**
    This routine will return true if the "addr" contains the "user" subject
    to the following contstraints:  (1) either the "user" is at the front
    of "addr" or it is preceded by an appropriate meta-char, and (2)
    either the "user" is at the end of "addr" or it is suceeded by an
    appropriate meta-char.
**/
int addr_matches_user(addr,user)
register char *addr, *user;
{
	int len = strlen(user);
	static char c_before[] = "!:%";	/* these can appear before a username */
	static char c_after[] = ":%@";	/* these can appear after a username  */

	do {
	  if ( strncmp(addr,user,len) == 0 ) {
	    if ( addr[len] == '\0' || index(c_after,addr[len]) != NULL )
	      return TRUE;
	  }
	} while ( (addr=strpbrk(addr,c_before)) != NULL && *++addr != '\0' ) ;
	return FALSE;
}

int
tail_of(from, buffer, to)
char *from, *buffer, *to;
{
	/** Return last two words of 'from'.  This is to allow
	    painless display of long return addresses as simply the
	    machine!username.
	    Or if the first word of the 'from' address is username or
	    full_username and 'to' is not NULL, then use the 'to' line
	    instead of the 'from' line.
	    If the 'to' line is used, return 1, else return 0.

	    Also modified to know about X.400 addresses (sigh) and
	    that when we ask for the tail of an address similar to
	    a%b@c we want to get back a@b ...
	**/

	/** Note: '!' delimits Usenet nodes, '@' delimits ARPA nodes,
	          ':' delimits CSNet & Bitnet nodes, '%' delimits multi-
		  stage ARPA hops, and '/' delimits X.400 addresses...
	          (it is fortunate that the ASCII character set only has
	   	  so many metacharacters, as I think we're probably using
		  them all!!) **/

	register int loc, i = 0, cnt = 0, using_to = 0;

#ifndef INTERNET

	/** let's see if we have an address appropriate for hacking:
	    what this actually does is remove the spuriously added
	    local bogus Internet header if we have one and the message
	    has some sort of UUCP component too...
	**/

	sprintf(buffer, "@%s", hostfullname);
	if (chloc(from,'!') != -1 && in_string(from, buffer))
	   from[strlen(from)-strlen(buffer)] = '\0';

#endif

	/**
	    Produce a simplified version of the from into buffer.  If the
	    from is just "username" or "Full Username" it will be preserved.
	    If it is an address, the rightmost "stuff!stuff", "stuff@stuff",
	    or "stuff:stuff" will be used.
	**/
	for (loc = strlen(from)-1; loc >= 0 && cnt < 2; loc--) {
	  if (from[loc] == BANG || from[loc] == AT_SIGN ||
	      from[loc] == COLON) cnt++;
	  if (cnt < 2) buffer[i++] = from[loc];
	}
	buffer[i] = '\0';
	reverse(buffer);

#ifdef MMDF
	if (strlen(buffer) == 0) {
	  if(to && *to != '\0' && !addr_matches_user(to, username)) {
	    tail_of(to, buffer, (char *)0);
	    using_to = 1;
	  } else
	    strcpy(buffer, full_username);
        }
#endif /* MMDF */

	if ( strcmp(buffer,full_username) == 0 ||
	  addr_matches_user(buffer,username) ) {

	  /* This message is from the user, so use the "to" header instead
	   * if possible, to be more informative. Otherwise be nice and
	   * use full_username rather than the bare username even if
	   * we've only matched on the bare username.
	   */

	  if(to && *to != '\0' && !addr_matches_user(to, username)) {
	    tail_of(to, buffer, (char *)0);
	    using_to = 1;
	  } else
	    strcpy(buffer, full_username);

	} else {					/* user%host@host? */

	  /** The logic here is that we're going to use 'loc' as a handy
	      flag to indicate if we've hit a '%' or not.  If we have,
	      we'll rewrite it as an '@' sign and then when we hit the
	      REAL at sign (we must have one) we'll simply replace it
	      with a NULL character, thereby ending the string there.
	  **/

	  loc = 0;

	  for (i=0; buffer[i] != '\0'; i++)
	    if (buffer[i] == '%') {
	      buffer[i] = AT_SIGN;
	      loc++;
	    }
	    else if (buffer[i] == AT_SIGN && loc)
	      buffer[i] = '\0';
	}
	return(using_to);

}

char *format_long(inbuff, init_len)
char *inbuff;
int   init_len;
{
	/** Return buffer with \n\t sequences added at each point where it
	    would be more than 80 chars long.  It only allows the breaks at
	    legal points (ie commas followed by white spaces).  init-len is
	    the characters already on the first line...  Changed so that if
            this is called while mailing without the overhead of "elm", it'll
            include "\r\n\t" instead.
	    Changed to use ',' as a separator and to REPLACE it after it's
	    found in the output stream...
	**/

#ifdef OS2
#define MAXLENGTH 1
#else
#define MAXLENGTH 80
#endif

	static char ret_buffer[VERY_LONG_STRING];
	register int iindex = 0, current_length = 0, depth=15, i, len;
	char     buffer[VERY_LONG_STRING];
	char     *word, *bufptr;

	strcpy(buffer, inbuff);

	bufptr = (char *) buffer;

	current_length = init_len + 2;	/* for luck */

	while ((word = get_token(bufptr,",", depth)) != NULL) {

	    /* first, decide what sort of separator we need, if any... */

	  if (strlen(word) + current_length > MAXLENGTH) {
	    if (iindex > 0) {
	      ret_buffer[iindex++] = ',';	/* close 'er up, doctor! */
	      ret_buffer[iindex++] = '\n';
	      ret_buffer[iindex++] = '\t';
	    }

	    /* now add this pup! */

	    for (i=(word[0] == ' '? 1:0), len = strlen(word); i<len; i++)
	      ret_buffer[iindex++] = word[i];
	    current_length = len + 8;	/* 8 = TAB */
	  }

	  else {	/* just add this address to the list.. */

	    if (iindex > 0) {
	      ret_buffer[iindex++] = ',';	/* comma added! */
	      ret_buffer[iindex++] = ' ';
	      current_length += 2;
	    }
	    for (i=(word[0] == ' '? 1:0), len = strlen(word); i<len; i++)
	      ret_buffer[iindex++] = word[i];
	    current_length += len;
	  }

	  bufptr = NULL;
	}

	ret_buffer[iindex] = '\0';

	return( (char *) ret_buffer);
}

char *strip_commas(string)
char *string;
{
	/** return string with all commas changed to spaces.  This IS
	    destructive and will permanently change the input string.. **/

	register char *strptr = string;

	for (; *strptr; strptr++)
	  if (*strptr == COMMA)
	    *strptr = SPACE;

	return( (char *) string);
}

char *strip_parens(string)
char *string;
{
	/**
	    Remove parenthesized information from a string.  More specifically,
	    comments as defined in RFC822 are removed.  This procedure is
	    non-destructive - a pointer to static data is returned.
	**/
	static char  buffer[VERY_LONG_STRING];
	register char *bufp;
	register int depth;

	for ( bufp = buffer, depth = 0 ; *string != '\0' ; ++string ) {
	  switch ( *string ) {
	  case '(':			/* begin comment on '('		*/
	    ++depth;
	    break;
	  case ')':			/* decr nesting level on ')'	*/
	    --depth;
	    break;
	  case '\\':			/* treat next char literally	*/
	    if ( *++string == '\0' ) {		/* gracefully handle	*/
	      *bufp++ = '\\';			/* '\' at end of string	*/
	      --string;				/* even tho it's wrong	*/
	    } else if ( depth == 0 ) {
	      *bufp++ = '\\';
	      *bufp++ = *string;
	    }
	    break;
	  default:			/* a regular char		*/
	    if ( depth == 0 )
	      *bufp++ = *string;
	    break;
	  }
	}
	*bufp = '\0';
	return( (char *) buffer);
}

move_left(string, chars)
char string[];
int  chars;
{
	/** moves string chars characters to the left DESTRUCTIVELY **/

	register int i;

	/*  chars--; /* index starting at zero! */

	for (i=chars; string[i] != '\0' && string[i] != '\n'; i++)
	  string[i-chars] = string[i];

	string[i-chars] = '\0';
}

remove_first_word(string)
char *string;
{	/** removes first word of string, ie up to first non-white space
	    following a white space! **/

	register int loc;

	for (loc = 0; string[loc] != ' ' && string[loc] != '\0'; loc++)
	    ;

	while (string[loc] == ' ' || string[loc] == '\t')
	  loc++;

	move_left(string, loc);
}

split_word(buffer, first, rest)
char *buffer, *first, *rest;
{
	/** Rip the buffer into first word and rest of word, translating it
	    all to lower case as we go along..
	**/

	/** skip leading white space, just in case.. **/

	while(whitespace(*buffer)) buffer++;

	/** now copy into 'first' until we hit white space or EOLN **/

	for (; *buffer && ! whitespace(*buffer); buffer++, first++)
	  if (islower(*buffer))
	    *first = *buffer;
	  else
	    *first = tolower(*buffer);

	*first = '\0';

	while (whitespace(*buffer)) buffer++;

	for (; *buffer; buffer++, rest++)
	  if (islower(*buffer))
	    *rest = *buffer;
	  else
	    *rest = tolower(*buffer);

	*rest = '\0';

	return;
}

char *tail_of_string(string, maxchars)
char *string;
int  maxchars;
{
	/** Return a string that is the last 'maxchars' characters of the
	    given string.  This is only used if the first word of the string
	    is longer than maxchars, else it will return what is given to
	    it...
	**/

	static char buffer[SLEN];
	register int iindex, i, len;

	for (iindex=0, len = strlen(string);! whitespace(string[iindex]) && iindex < len;
	     iindex++)
	  ;

	if (iindex < maxchars) {
	  strncpy(buffer, string, maxchars-2);	/* word too short */
	  buffer[maxchars-2] = '.';
	  buffer[maxchars-1] = '.';
	  buffer[maxchars]   = '.';
	  buffer[maxchars+1] = '\0';
	}
	else {
	  i = maxchars;
	  buffer[i--] = '\0';
	  while (i > 1)
	    buffer[i--] = string[iindex--];
	  buffer[2] = '.';
	  buffer[1] = '.';
	  buffer[0] = '.';
	}

	return( (char *) buffer);
}

reverse(string)
char *string;
{
	/** reverse string... pretty trivial routine, actually! **/

	char buffer[SLEN];
	register int i, j = 0;

	for (i = strlen(string)-1; i >= 0; i--)
	  buffer[j++] = string[i];

	buffer[j] = '\0';

	strcpy(string, buffer);
}

int
get_word(buffer, start, word)
char *buffer, *word;
int start;
{
	/**	return next word in buffer, starting at 'start'.
		delimiter is space or end-of-line.  Returns the
		location of the next word, or -1 if returning
		the last word in the buffer.  -2 indicates empty
		buffer!  **/

	register int loc = 0;

	while (buffer[start] == ' ' && buffer[start] != '\0')
	  start++;

	if (buffer[start] == '\0') return(-2);	 /* nothing IN buffer! */

	while (buffer[start] != ' ' && buffer[start] != '\0')
	  word[loc++] = buffer[start++];

	word[loc] = '\0';
	return(start);
}

Centerline(line, string)
int line;
char *string;
{
	/** Output 'string' on the given line, centered. **/

	register int length, col;

	length = strlen(string);

	if (length > COLUMNS)
	  col = 0;
	else
	  col = (COLUMNS - length) / 2;

	PutLine0(line, col, string);
}

char *argv_zero(string)
char *string;
{
	/** given a string of the form "/something/name" return a
	    string of the form "name"... **/

	static char buffer[NLEN];
	register int i, j=0;

	for (i=strlen(string)-1; string[i] != '/'; i--)
	  buffer[j++] = string[i];
	buffer[j] = '\0';

	reverse(buffer);

	return( (char *) buffer);
}

#define MAX_RECURSION		20		/* up to 20 deep recursion */

char *get_token(source, keys, depth)
char *source, *keys;
int   depth;
{
	/** This function is similar to strtok() (see "opt_utils")
	    but allows nesting of calls via pointers...
	**/

	register int  last_ch;
	static   char *buffers[MAX_RECURSION];
	char     *return_value, *sourceptr;

	if (depth > MAX_RECURSION) {
	   error1("Get_token calls nested greater than %d deep!",
		  MAX_RECURSION);
	   emergency_exit();
	}

	if (source != NULL)
	  buffers[depth] = source;

	sourceptr = buffers[depth];

	if (*sourceptr == '\0')
	  return(NULL);		/* we hit end-of-string last time!? */

	sourceptr += strspn(sourceptr, keys);	  /* skip the bad.. */

	if (*sourceptr == '\0') {
	  buffers[depth] = sourceptr;
	  return(NULL);			/* we've hit end-of-string   */
	}

	last_ch = strcspn(sourceptr, keys);   /* end of good stuff   */

	return_value = sourceptr;	      /* and get the ret     */

	sourceptr += last_ch;		      /* ...value            */

	if (*sourceptr != '\0')		/** don't forget if we're at end! **/
	  sourceptr++;

	return_value[last_ch] = '\0';	      /* ..ending right      */

	buffers[depth] = sourceptr;	      /* save this, mate!    */

	return((char *) return_value);	     /* and we're outta here! */
}


quote_args(out_string,in_string)
register char *out_string, *in_string;
{
	/** Copy from "in_string" to "out_string", collapsing multiple
	    white space and quoting each word.  Returns a pointer to
	    the resulting word.
	**/

	int empty_string = TRUE;

	while ( *in_string != '\0' ) {

	    /**	If this is a space then advance to the start of the next word.
		Otherwise, copy through the word surrounded by quotes.
	    **/

	    if ( isspace(*in_string) ) {
		while ( isspace(*in_string) )
			++in_string;
	    } else {
		*out_string++ = '"';
		while ( *in_string != '\0' && !isspace(*in_string) )
			*out_string++ = *in_string++;
		*out_string++ = '"';
		*out_string++ = ' ';
		empty_string = FALSE;
	    }

    }

    if ( !empty_string )
	--out_string;
    *out_string = '\0';
}

