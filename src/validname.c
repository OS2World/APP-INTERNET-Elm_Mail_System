
static char rcsid[] = "@(#)$Id: validname.c,v 4.1 90/04/28 22:44:21 syd Exp $";

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
 * $Log:	validname.c,v $
 * Revision 4.1  90/04/28  22:44:21  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

#include <stdio.h>

#ifndef NOCHECK_VALIDNAME		 /* Force a return of valid */
# ifdef PWDINSYS
#  include <sys/pwd.h>
# else
#  include <pwd.h>
# endif
#endif

#include "defs.h"

int
valid_name(name)
char *name;
{
	/** Determine whether "name" is a valid logname on this system.
	    It is valid if there is a password entry, or if there is
	    a mail file in the mail spool directory for "name".
	 **/

#ifdef NOCHECK_VALIDNAME		 /* Force a return of valid */

	return(TRUE);

#else

	char filebuf[SLEN];
	struct passwd *getpwnam();

	if(getpwnam(name) != NULL)
	  return(TRUE);

#ifdef OS2
	if (maildir)
	    sprintf(filebuf, "%s%s/newmail%s", mailhome, name, mailext);
	else
#endif
	sprintf(filebuf,"%s%s%s", mailhome, name, mailext);
	if (access(filebuf, ACCESS_EXISTS) == 0)
	  return(TRUE);

	return(FALSE);

#endif
}
