
static char rcsid[] = "@(#)$Id: mkhdrs.c,v 4.1 90/04/28 22:43:32 syd Exp $";

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
 * $Log:	mkhdrs.c,v $
 * Revision 4.1  90/04/28  22:43:32  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This contains all the header generating routines for the ELM
    program.

**/

#include "headers.h"

extern char in_reply_to[SLEN];

char *strcpy();
unsigned long sleep();

generate_reply_to(msg)
int msg;
{
	/** Generate an 'in-reply-to' message... **/
	char buffer[SLEN];


	if (msg == -1)		/* not a reply! */
	  in_reply_to[0] = '\0';
	else {
	  if (chloc(headers[msg]->from, '!') != -1)
	    tail_of(headers[msg]->from, buffer, 0);
	  else
	    strcpy(buffer, headers[msg]->from);
	  sprintf(in_reply_to, "%s from \"%s\" at %s %s %s %s",
                  headers[msg]->messageid[0] == '\0'? "<no.id>":
		  headers[msg]->messageid,
		  buffer,
		  headers[msg]->month,
		  headers[msg]->day,
		  headers[msg]->year,
		  headers[msg]->time);
	}
}

add_mailheaders(filedesc)
FILE *filedesc;
{
	/** Add the users .mailheaders file if available.  Allow backquoting
	    in the file, too, for fortunes, etc...*shudder*
	**/

	FILE *fd;
	char filename[SLEN], buffer[SLEN];

	sprintf(filename, "%s/%s", home, mailheaders);

	if ((fd = fopen(filename, "r")) != NULL) {
	  while (fgets(buffer, SLEN, fd) != NULL)
	    if (strlen(buffer) < 2) {
	      dprint(2, (debugfile,
	         "Strlen of line from .elmheaders is < 2 (write_header_info)"));
	      error1("Warning: blank line in %s ignored!", filename);
	      sleep(2);
	    }
	    else if (occurances_of(BACKQUOTE, buffer) >= 2)
	      expand_backquote(buffer, filedesc);
	    else
	      fprintf(filedesc, "%s", buffer);

	    fclose(fd);
	}
}

expand_backquote(buffer, filedesc)
char *buffer;
FILE *filedesc;
{
	/** This routine is called with a line of the form:
		Fieldname: `command`
	    and is expanded accordingly..
	**/

	FILE *fd;
	char command[SLEN], command_buffer[SLEN], fname[SLEN],
	     prefix[SLEN];
	register int i, j = 0;

	for (i=0; buffer[i] != BACKQUOTE; i++)
	  prefix[j++] = buffer[i];
	prefix[j] = '\0';

	j = 0;

	for (i=chloc(buffer, BACKQUOTE)+1; buffer[i] != BACKQUOTE;i++)
	  command[j++] = buffer[i];
	command[j] = '\0';

	sprintf(fname,"%s%d%s", temp_dir, getpid(), temp_print);

	sprintf(command_buffer, "%s > %s", command, fname);

	system_call(command_buffer, SH, FALSE, FALSE);

	if ((fd = fopen(fname, "r")) == NULL) {
	  error1("Backquoted command \"%s\" in elmheaders failed.", command);
	  return;
	}

	/* If we get a line that is less than 80 - length of prefix then we
	   can toss it on the same line, otherwise, simply prepend each line
	   *starting with this line* with a leading tab and cruise along */

	if (fgets(command_buffer, SLEN, fd) == NULL)
	  fprintf(filedesc, prefix);
	else {
	  if (strlen(command_buffer) + strlen(prefix) < 80)
	    fprintf(filedesc, "%s%s", prefix, command_buffer);
	  else
	    fprintf(filedesc, "%s\n\t%s", prefix, command_buffer);

	  while (fgets(command_buffer, SLEN, fd) != NULL)
	    fprintf(filedesc, "\t%s", command_buffer);

	  fclose(fd);
	}

	unlink(fname);	/* don't leave the temp file laying around! */
}
