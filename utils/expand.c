
static char rcsid[] = "@(#)$Id: expand.c,v 4.1 90/04/28 22:44:37 syd Exp $";

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
 * $Log:	expand.c,v $
 * Revision 4.1  90/04/28  22:44:37  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This is a library routine for the various utilities that allows
    users to have the standard 'Elm' folder directory nomenclature
    for all filenames (e.g. '+', '=' or '%').  It should be compiled
    and then linked in as needed.

**/

#include <stdio.h>
#include <pwd.h>
#include "defs.h"

char *expand_define();
static struct passwd *pass;
static char *home;

int
expand(filename)
char *filename;
{
	/** Expand the filename since the first character is a meta-
	    character that should expand to the "maildir" variable
	    in the users ".elmrc" file...

	    Note: this is a brute force way of getting the entry out
	    of the .elmrc file, and isn't recommended for the faint
	    of heart!
	**/

	FILE *rcfile;
	char  buffer[SLEN], *expanded_dir, *bufptr;
	int   foundit = 0;

	bufptr = (char *) buffer;		/* same address */

	if((pass = getpwuid(getuid())) == NULL) {
	    printf("You have no password entry!\n");
	    exit(1);
	}
	home = pass->pw_dir;

	sprintf(buffer, "%s/%s", home, elmrcfile);

	if ((rcfile = fopen(buffer, "r")) == NULL) {
	  printf("Can't open your \".elmrc\" file (%s) for reading!\n",
		 buffer);
	  return(NO);
	}

	while (fgets(buffer, SLEN, rcfile) != NULL && ! foundit) {
	  if (strncmp(buffer, "maildir", 7) == 0 ||
	      strncmp(buffer, "folders", 7) == 0) {
	    while (*bufptr != '=' && *bufptr)
	      bufptr++;
	    bufptr++;			/* skip the equals sign */
	    while (whitespace(*bufptr) && *bufptr)
	      bufptr++;
	    home = bufptr;		/* remember this address */

	    while (! whitespace(*bufptr) && *bufptr != '\n')
	      bufptr++;

	    *bufptr = '\0';		/* remove trailing space */
	    foundit++;
	  }
	}

	fclose(rcfile);			/* be nice... */

	if (! foundit) {
	  printf("Couldn't find \"maildir\" in your .elmrc file!\n");
	  return(NO);
	}

	/** Home now points to the string containing your maildir, with
	    no leading or trailing white space...
	**/

	if ((expanded_dir = expand_define(home)) == NULL)
		return(NO);

	sprintf(buffer, "%s%s%s", expanded_dir,
		(expanded_dir[strlen(expanded_dir)-1] == '/' ||
		filename[0] == '/') ? "" : "/", (char *) filename+1);

	strcpy(filename, buffer);
	return(YES);
}

char *expand_define(maildir)
char *maildir;
{
	/** This routine expands any occurances of "~" or "$var" in
	    the users definition of their maildir directory out of
	    their .elmrc file.

	    Again, another routine not for the weak of heart or staunch
	    of will!
	**/

	static char buffer[SLEN];	/* static buffer AIEE!! */
	char   name[SLEN],		/* dynamic buffer!! (?) */
	       *nameptr,	       /*  pointer to name??     */
	       *value;		      /* char pointer for munging */

	if (*maildir == '~')
	  sprintf(buffer, "%s%s", home, ++maildir);
	else if (*maildir == '$') { 	/* shell variable */

	  /** break it into a single word - the variable name **/

	  strcpy(name, (char *) maildir + 1);	/* hurl the '$' */
	  nameptr = (char *) name;
	  while (*nameptr != '/' && *nameptr) nameptr++;
	  *nameptr = '\0';	/* null terminate */

	  /** got word "name" for expansion **/

	  if ((value = getenv(name)) == NULL) {
	    printf("Couldn't expand shell variable $%s in .elmrc!\n", name);
	    return(NULL);
	  }
	  sprintf(buffer, "%s%s", value, maildir + strlen(name) + 1);
	}
	else strcpy(buffer, maildir);

	return( ( char *) buffer);
}
