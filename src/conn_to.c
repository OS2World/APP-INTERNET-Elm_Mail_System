
static char rcsid[] = "@(#)$Id: conn_to.c,v 4.1 90/04/28 22:42:37 syd Exp $";

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
 * $Log:	conn_to.c,v $
 * Revision 4.1  90/04/28  22:42:37  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This contains the routine(s) needed to have the Elm mailer figure
    out what machines the current machine can talk to.
    It will invoke uuname to a file, then read the file in!
**/

#include "headers.h"

char *strcpy();

#ifndef DONT_TOUCH_ADDRESSES
get_connections()
{

	/** get the direct connections that this machine has
	    using the uuname routine to get the names.
	**/

	FILE *fd;
	char  buffer[SLEN], filename[SLEN];
	struct lsys_rec *system_record, *previous_record;
	int   loc_on_line;


	if (! warnings) {		/* skip this - they don't care! */
	  talk_to_sys = NULL;
	  return;
	}

	if (strlen(uuname) == 0) {	/* skip this - no way to get connections */
	  warnings = NO;
	  talk_to_sys = NULL;
	  dprint(1, (debugfile, "No uuname - clearing warnings\n"));
	  error("Warning: no uuname command, clearing option warnings");
	  return;
	}

	sprintf(filename, "%s%d%s", temp_dir, getpid(), temp_uuname);
	sprintf(buffer,"%s > %s", uuname, filename);

	if (system_call(buffer, SH, FALSE, FALSE) != 0) {
	  dprint(1, (debugfile, "Can't get uuname info - system() failed!\n"));
	  goto unable_to_get;
	}

	if ((fd = fopen(filename, "r")) == NULL) {
	  dprint(1, (debugfile,
		"Can't get uuname info - can't open file %s for reading\n",
		 filename));
	  goto unable_to_get;
	}

	previous_record = NULL;

	while (fgets(buffer, SLEN, fd) != NULL) {
	  no_ret(buffer);
	  if (previous_record == NULL) {
	    dprint(2, (debugfile, "uuname\tdirect connection to %s, ", buffer));
	    loc_on_line = 30 + strlen(buffer);
	    previous_record = (struct lsys_rec *) pmalloc(sizeof *talk_to_sys);

	    strcpy(previous_record->name, buffer);
	    previous_record->next = NULL;
	    talk_to_sys = previous_record;
	  }
	  else {	/* don't have to check uniqueness - uuname does that! */
	    if (loc_on_line + strlen(buffer) > 80) {
	      dprint(2, (debugfile, "\n\t"));
	      loc_on_line = 8;
	    }
	    dprint(2, (debugfile, "%s, ", buffer));
	    loc_on_line += (strlen(buffer) + 2);
	    system_record = (struct lsys_rec *) pmalloc(sizeof *talk_to_sys);

	    strcpy(system_record->name, buffer);
	    system_record->next = NULL;
	    previous_record->next = system_record;
	    previous_record = system_record;
	  }
	}

	fclose(fd);

	(void) unlink(filename);		/* kill da temp file!! */

	dprint(2, (debugfile, "\n"));		/* for a nice format! Yeah! */

	return;					/* it all went okay... */

unable_to_get:
	unlink(filename);	/* insurance */
	error("Warning: couldn't figure out system connections...");
	talk_to_sys = NULL;
}
#endif
