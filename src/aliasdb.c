
static char rcsid[] = "@(#)$Id: aliasdb.c,v 4.1.1.1 90/06/21 23:21:47 syd Exp $";

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
 * $Log:	aliasdb.c,v $
 * Revision 4.1.1.1  90/06/21  23:21:47  syd
 * Add missing check for eof and for failure of scanf
 * From: Ross Johnson
 *
 * Revision 4.1  90/04/28  22:42:28  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Alias database files...

**/


#include "headers.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef OS2
extern int errno;
#endif

#ifdef USE_DBM
# include <dbm.h>
#endif

#define  absolute(x)		((x) > 0 ? x : -(x))

char *find_path_to(), *strcat(), *strcpy();
unsigned long sleep();

#ifndef DONT_TOUCH_ADDRESSES
int  findnode_has_been_initialized = FALSE;
#endif

findnode(name, display_error)
char *name;
int   display_error;
{
	/** break 'name' into machine!user or user@machine and then
	    see if you can find 'machine' in the path database..
	    If so, return name as the expanded address.  If not,
	    return what was given to us!   If display_error, then
	    do so...
	**/

#ifndef DONT_TOUCH_ADDRESSES

	char   old_name[SLEN];
	char   address[SLEN];

	if (strlen(name) == 0)
	  return;

	if (! findnode_has_been_initialized) {
	  if (warnings)
	    error("Initializing internal tables...");
#ifndef USE_DBM
	  get_connections();
	  open_domain_file();
#endif
	  init_findnode();
	  clear_error();
          findnode_has_been_initialized = TRUE;
	}

	strcpy(old_name, name);		/* save what we were given */

	if (expand_site(name, address) == -1) {
          if (display_error && name[0] != '!') {
	    dprint(3, (debugfile, "Couldn't expand host %s in address.\n",
	             name));
	    if (! check_only && warnings) {
	      error1("Couldn't expand system %s.", name);
	      sleep(1);
	    }
	  }
	  strcpy(name, old_name);	/* and restore... */
	}
	else
	  strcpy(name, address);
#endif
	return;
}

#if defined(OPTIMIZE_RETURN) || !defined(DONT_TOUCH_ADDRESSES)
int
expand_site(cryptic, expanded)
char *cryptic, *expanded;
{

	/** Given an address of the form 'xyz@site' or 'site!xyz'
	    return an address of the form <expanded address for site>
            with 'xyz' embedded according to the path database entry.
	    Note that 'xyz' can be eiher a simple address (as in "joe")
	    or a complex address (as in "joe%xerox.parc@Xerox.ARPA")!
	    0 = found, -1 return means unknown site code

	    Modified to strip out parenthetical comments...
	**/

#ifdef ACSNET

	strcpy(expanded, cryptic);	/* fast and simple */
	return(0);

#else
# ifdef USE_DBM
	datum  key, contents;
# endif

	char   name[VERY_LONG_STRING], sitename[VERY_LONG_STRING],
               temp[VERY_LONG_STRING], old_name[VERY_LONG_STRING],
	       comment[LONG_STRING];
	char   *expand_domain(), *addr;
	register int i = 0, j = 0, in_parens = 0, domain_name;

	strcpy(old_name, cryptic);	/* remember what we were given */

	/** break down **/

	/** first, rip out the comment, if any, noting nested parens **/

	if ((i = chloc(cryptic, '(')) > -1) {
	  comment[j++] = ' ';			/* leading space */
	  do {
  	    switch(comment[j++] = cryptic[i++]) {
	    case '(':	in_parens++;
			break;
	    case ')':	in_parens--;
			break;
	    }
	  } while(in_parens && cryptic[i] != '\0');
	  comment[j] = '\0';

	  /* and remove this from cryptic string too... */
	  if (cryptic[(j = chloc(cryptic,'('))-1] == ' ')
	    cryptic[j-1] = '\0';
	  else
	    cryptic[j] = '\0';
	}
	else
	  comment[0] = '\0';

	i = j = 0;	/* reset */

	while (cryptic[i] != AT_SIGN && cryptic[i] != BANG &&
	       cryptic[i] != '\0' && cryptic[i] != '(')
	  sitename[j++] = cryptic[i++];

	sitename[j++] = '\0';

	j = 0;

	if (cryptic[i] == '\0') return(-1);	/* nothing to expand! */

	domain_name = (cryptic[i] == AT_SIGN);

	i++;

	while (cryptic[i] != '\0' && cryptic[i] != '(' &&
               ! whitespace(cryptic[i]))
	  name[j++] = cryptic[i++];

	name[j] = '\0';

	if (domain_name) {
	  strcpy(temp, name);
	  strcpy(name, sitename);
	  strcpy(sitename, temp);
	}

	dprint(5, (debugfile, "\nBroke address into '%s' @ '%s' '%s'\n\n",
		name, sitename, comment));

#ifdef USE_DBM

	if (size_of_pathfd == 0)
	  return(-1);

	key.dptr  = sitename;
	key.dsize = strlen(sitename) + 1;

	contents = fetch(key);

	if (contents.dptr == 0)
	  return(-1);			/* can't find it! */

	sprintf(expanded, contents.dptr, name);
	strcat(expanded, " ");			/* add a single space... */
	strcat(expanded, comment);		/*    ...and add comment */
	return(0);
#else

#ifndef LOOK_CLOSE_AFTER_SEARCH

	if (talk_to(sitename)) {
	  strcpy(expanded, old_name);	/* restore! */
	  return(0);
	}
#endif

	if ((addr = find_path_to(sitename, TRUE)) == NULL) {

#ifdef LOOK_CLOSE_AFTER_SEARCH

	    if (talk_to(sitename)) {
	      strcpy(expanded, old_name);	/* restore! */
	      return(0);
	    }
	    else
#endif
	    if ((addr = expand_domain(cryptic)) != NULL) {
	       strcpy(expanded, addr);	/* into THIS buffer */
	       strcat(expanded, comment);	/* patch in comment */
	       return(0);
	    }
	    else  if (size_of_pathfd == 0) {	/* no path database! */
	      strcpy(expanded, old_name);	/* restore! */
	      return(0);
	    }
	    else {			     /* We just can't get there! */
	      strcpy(expanded, old_name);	/* restore! */
	      return(-1);
	    }
	}
	else {			/* search succeeded */
	   sprintf(expanded, addr, name);
	   strcat(expanded, comment);		/* add comment */
	   return(0);
	}
#endif
#endif
}
#endif /* defined(OPTIMIZE_RETURN) || !defined(DONT_TOUCH_ADDRESSES) */

int
binary_search(name, address)
char *name, *address;
{
	/* binary search file for name.  Return 0 if found, -1 if not */

	char machine[40];
	register long first = 0, last, middle;
	register int  compare;

	address[0] = '\0';

	last = size_of_pathfd;

	do {

	  middle = (long) ((first+last) / 2);

	  get_entry(machine, address, pathfd, middle);

	  compare = strcmp(name, machine);

	  if (compare < 0)
	    last = middle - 1;
	  else if (compare == 0)
	    return(0);
	  else  /* greater */
	    first = middle + 1;
	} while (absolute(last) - absolute(first) > FIND_DELTA);

	/* It could be that our target entry lies exactly at `first'.
	 * Since get_entry() compares at the entry beginning after
	 * the passed offset (unless it's 0), we need to decrement it
	 * (unless it's 0), and give it one more try.
	 */
	get_entry(machine, address, pathfd, (first == 0L ? first : --first));
	compare = strcmp(name, machine);
	return(compare == 0 ? 0 : -1);
}

get_entry(machine, address, fileid, offset)
char *machine, *address;
FILE *fileid;
long offset;
{
	int	ch;

	/** get entry...return machine and address immediately
	    following given offset in fileid.  **/

	(void) fseek(fileid, offset, 0);

	/* To get to the beginning of a record, if we are not at offset 0,
	 * read until we hit an end-of-line */

	if(offset != 0L)
	  while ((ch = getc(fileid)) != EOF)
	    if (ch == '\n')
	      break;

	*machine = '\0';
	*address = '\0';
	fscanf(fileid, "%s\t%s", machine, address);
}

#ifndef DONT_TOUCH_ADDRESSES
init_findnode()
{
	/** Initialize the FILE and 'size_of_file' values for the
	    findnode procedure **/

	struct stat buffer;
	char   *path_filename;
	char buf[BUFSIZ];

#ifdef USE_DBM
	sprintf(buf,"%s/%s.pag", elmhome, pathfile);
#else
	sprintf(buf,"%s/%s", elmhome, pathfile);
#endif
	path_filename = buf;

	if (stat(path_filename, &buffer) == -1) {
	  dprint(2, (debugfile,
		  "Warning: pathalias file \"%s\" wasn't found by %s\n",
		  path_filename, "init_findnode"));
	  size_of_pathfd = 0;
	  return;
	}

	size_of_pathfd = (long) buffer.st_size;

#ifdef USE_DBM

	if (dbminit(pathfile) != 0) {
	  dprint(2, (debugfile,
		 "Warning: couldn't initialize DBM database %s\n",
		 pathfile));
	  dprint(2, (debugfile, "** %s - %s **\n\n", error_name(errno),
		     error_description(errno)));
	  size_of_pathfd = 0;	/* error flag, in this case */
	  return;
	}

	return;
#else

	if ((pathfd = fopen(pathfile,"r")) == NULL) {
	  dprint(2, (debugfile,
		"Warning: Can't read pathalias file \"%s\" within %s\n",
		   pathfile, "init_findnode"));
	  size_of_pathfd = 0;
	}
	else
	  dprint(3, (debugfile, "\nOpened '%s' as pathalias database.\n\n",
		  pathfile));
#endif
}
#endif /* DONT_TOUCH_ADDRESSES */

char *find_path_to(machine, printf_format)
char *machine;
int   printf_format;
{
	/** Returns either the path to the specified machine or NULL if
	    not found.  If "printf_format" is TRUE, then it leaves the
	    '%s' intact, otherwise it assumes that the address is a uucp
	    address for the domain expansion program and removes the
	    last three characters of the expanded name ("!%s") since
	    they're redundant with the expansion!
        **/

	static char buffer[SLEN];	/* space for path */

	if (size_of_pathfd > 0)
	  if (binary_search(machine, buffer) != -1) {   	/* found it! */
	    if (! printf_format && strlen(buffer) > 3)
	      buffer[strlen(buffer)-3] = '\0';
	    return( (char *) buffer);
	  }

	return(NULL);			            /* failed if it's here! */
}
