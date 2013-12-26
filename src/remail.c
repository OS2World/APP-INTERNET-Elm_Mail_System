
static char rcsid[] = "@(#)$Id: remail.c,v 4.1.1.2 90/10/10 12:45:07 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.2 $   $State: Exp $
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
 * $Log:	remail.c,v $
 * Revision 4.1.1.2  90/10/10  12:45:07  syd
 * Make the symbol look less like a typo, its real
 * From: Syd
 *
 * Revision 4.1.1.1  90/10/07  19:48:15  syd
 * fix the bounce problem reported earlier when using MMDF submit as the MTA.
 * From: Jim Clausing <jac%brahms.tinton.ccur.com@RELAY.CS.NET>
 *
 * Revision 4.1  90/04/28  22:43:50  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** For those cases when you want to have a message continue along
    to another person in such a way as they end up receiving it with
    the return address the person YOU received the mail from (does
    this comment make any sense yet??)...

**/

#include "headers.h"
#include <errno.h>

#ifndef OS2
extern int errno;
#endif

char *error_name(), *error_description();

int
remail()
{
	/** remail a message... returns TRUE if new foot needed ... **/

	FILE *mailfd;
	char entered[VERY_LONG_STRING], expanded[VERY_LONG_STRING];
	char *filename, buffer[VERY_LONG_STRING], ch;
	char mailerflags[NLEN];
	extern char *tempnam();
        int sys_status;

	entered[0] = '\0';

	get_to(entered, expanded);
	if (strlen(entered) == 0)
	  return(0);

	display_to(expanded);

	if((filename=tempnam(temp_dir, "snd")) == NULL) {
	  dprint(1, (debugfile, "couldn't make temp file nam! (remail)\n"));
	  sprintf(buffer, "Sorry - couldn't make file temp file name.");
	  set_error(buffer);
	  return(1);
	}

	if ((mailfd = fopen(filename, "w")) == NULL) {
	  dprint(1, (debugfile, "couldn't open temp file %s! (remail)\n",
		  filename));
	  dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
		  error_description(errno)));
	  sprintf(buffer, "Sorry - couldn't open file %s for writing (%s).",
		  error_name(errno));
	  set_error(buffer);
	  return(1);
	}

	/** now let's copy the message into the newly opened
	    buffer... **/

	chown (filename, userid, groupid);

#ifdef _MMDF
	if (strcmp(submitmail, mailer) == 0)
	  do_mmdf_addresses(mailfd, strip_parens(strip_commas(expanded)));
#endif /* MMDF */

	copy_message("", mailfd, FALSE, TRUE, FALSE, TRUE, TRUE, FALSE);

	fclose(mailfd);

	/** Got the messsage, now let's ensure the person really wants to
	    remail it... **/

	ClearLine(LINES-1);
	ClearLine(LINES);
	PutLine1(LINES-1,0,
	    "Are you sure you want to remail this message (y/n)? y%c",
	    BACKSPACE);
	fflush(stdin);
	fflush(stdout);
	ch = ReadCh();
	if (tolower(ch) == 'n') { /* another day, another No... */
	  Write_to_screen("No.", 0);
	  set_error("Bounce of message cancelled.");
	  (void) unlink(filename);
	  return(1);
	}
	Write_to_screen("Yes.", 0);

	if (strcmp(sendmail, mailer) == 0)
	{
	  sprintf(buffer,"sndmail %s -af %s -f %s@%s %s >nul 2>&1", 
		  background ? "-bg" : "",
		  filename, username, hostfromname, 
		  strip_parens(strip_commas(expanded)));
	}
	else {
	  sprintf(buffer,"%s -f %s %s 2>nul", 
		  mailer, filename, strip_parens(strip_commas(expanded)));
	}

	PutLine0(LINES,0,"Resending mail...");

	if ( sys_status = system_call(buffer, SH, FALSE, FALSE) ) {
		/* problem case: */
		sprintf(buffer, "mailer returned error status %d", sys_status);
		set_error(buffer);
	} else {
	        set_error("Mail resent.");
        }

	if (!background)
	  unlink(filename);

	return(1);
}

#ifdef MMDF
do_mmdf_addresses(dest_file,buffer)
FILE *dest_file;
char *buffer;
{
	char old[VERY_LONG_STRING], first[VERY_LONG_STRING],
		rest[VERY_LONG_STRING];

	strcpy(old,buffer);
	split_word(old, first, rest);
	while (strcmp(first, "") != 0) {
	  fprintf(dest_file, "%s\n", first);
	  strcpy(old, rest);
	  split_word(old, first, rest);
	}
	fprintf(dest_file, "\n");
}
#endif /* MMDF */
