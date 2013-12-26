
static char rcsid[] = "@(#)$Id: forms.c,v 4.1 90/04/28 22:43:08 syd Exp $";

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
 * $Log:	forms.c,v $
 * Revision 4.1  90/04/28  22:43:08  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This set of files supports the 'forms' options (AT&T Mail Forms) to
    the mail system.  The specs are drawn from a document from AT&T entitled
    "Standard for Exchanging Forms on AT&T Mail", version 1.9.

**/

/** Some notes on the format of a FORM;

	First off, in AT&T Mail parlance, this program only supports SIMPLE
	forms, currently.  This means that while each form must have three
 	sections;

		[options-section]
		***
		[form-image]
		***
		[rules-section]

	this program will ignore the first and third sections completely.  The
	program will assume that the user merely enteres the form-image section,
	and will append and prepend the triple asterisk sequences that *MUST*
	be part of the message.  The messages are also expected to have a
	specific header - "Content-Type: mailform" - which will be added on all
	outbound mail and checked on inbound...
**/

#include "headers.h"
#include <errno.h>

#ifndef OS2
extern int errno;
#endif

char *error_name(), *strcat(), *strcpy();

check_form_file(filename)
char *filename;
{
	/** This routine returns the number of fields in the specified file,
	    or -1 if an error is encountered. **/

	FILE *form;
	char buffer[SLEN];
	register int field_count = 0;

	if ((form = fopen(filename, "r")) == NULL) {
	  error2("Error %s trying to open %s to check fields!",
		  error_name(errno), filename);
	  return(-1);
	}

	while (fgets(buffer, SLEN, form) != NULL) {
	  field_count += occurances_of(COLON, buffer);
	}

	fclose(form);

	return(field_count);
}

format_form(filename)
char *filename;
{
	/** This routine accepts a validated file that is the middle
	    section of a form message and prepends and appends the appropriate
	    instructions.  It's pretty simple.
	    This returns the number of forms in the file, or -1 on errors
	**/

	FILE *form, *newform;
	char  newfname[SLEN], buffer[SLEN];
	register form_count = 0;

	dprint(4, (debugfile, "Formatting form file '%s'\n", filename));

	/** first off, let's open the files... **/

	if ((form = fopen(filename, "r")) == NULL) {
	  error("Can't read the message to validate the form!");
	  dprint(1, (debugfile,
              "** Error encountered opening file \"%s\" - %s (check_form) **\n",
	      filename, error_name(errno)));
	  return(-1);
	}

	sprintf(newfname, "%s%d%s", temp_dir, getpid(), temp_form_file);

	if ((newform = fopen(newfname, "w")) == NULL) {
	  error("Couldn't open newform file for form output!");
	  dprint(1, (debugfile,
              "** Error encountered opening file \"%s\" - %s (check_form) **\n",
	      newfname, error_name(errno)));
	  return(-1);
	}

	/** the required header... **/

	/* these are actually the defaults, but let's be sure, okay? */

	fprintf(newform, "WIDTH=78\nTYPE=SIMPLE\nOUTPUT=TEXT\n***\n");

	/** and let's have some fun transfering the stuff across... **/

	while (fgets(buffer, SLEN, form) != NULL) {
	  fputs(buffer, newform);
	  form_count += occurances_of(COLON, buffer);
	}

	fprintf(newform, "***\n");	/* that closing bit! */

	fclose(form);
	fclose(newform);

	if (form_count > 0) {
	  if (unlink(filename) != 0) {
	    error2("Error %s unlinking file %s.", error_name(errno), filename);
	    return(-1);
	  }
	  if (rename(newfname, filename)) {
	    error3("Error %s renaming %s to %s.", error_name(errno),
		    newfname, filename);
	    return(-1);
	  }
	}
        else if (unlink(newfname)) {
	  error2("Error %s unlinking file %s.", error_name(errno), newfname);
	  return(-1);
	}

	return(form_count);
}

int
mail_filled_in_form(address, subject)
char *address, *subject;
{
	/** This is the interesting routine.  This one will read the
	    message and prompt the user, line by line, for each of
	    the fields...returns non-zero if it succeeds
	**/

	FILE  	     *fd;
	register int lines = 0, count;
	char         buffer[SLEN], *ptr;

	dprint(4, (debugfile,
		"replying to form with;\n\taddress=%s and\n\t subject=%s\n",
		 address, subject));

        if (fseek(mailfile, headers[current-1]->offset, 0) == -1) {
	  dprint(1, (debugfile,
		   "Error: seek %ld resulted in errno %s (%s)\n",
		   headers[current-1]->offset, error_name(errno),
		   "mail_filled_in_form"));
	  error2("ELM [seek] couldn't read %d bytes into file (%s).",
	         headers[current-1]->offset, error_name(errno));
	  return(0);
        }

	/* now we can fly along and get to the message body... */

	while ((ptr = fgets(buffer, SLEN, mailfile)) != NULL) {
	  if (strlen(buffer) == 1)	/* <return> only */
	    break;
	  else if (strncmp(buffer,"From ", 5) == 0 && lines++ > 0) {
	    error("No form in this message!?");
	    return(0);
	  }
	}

	if (ptr == NULL) {
	  error("No form in this message!?");
	  return(0);
	}

	dprint(6, (debugfile, "- past header of form message -\n"));

	/* at this point we're at the beginning of the body of the message */

	/* now we can skip to the FORM-IMAGE section by reading through a
	   line with a triple asterisk... */

	while ((ptr = fgets(buffer, SLEN, mailfile)) != NULL) {
	  if (strcmp(buffer, "***\n") == 0)
	    break;	/* we GOT it!  It's a miracle! */
	  else if (strncmp(buffer, "From ",5) == 0) {
	    error("Badly constructed form.  Can't reply!");
	    return(0);
	  }
	}

	if (ptr == NULL) {
	  error("Badly constructed form.  Can't reply!");
	  return(0);
	}

	dprint(6, (debugfile, "- skipped the non-forms-image stuff -\n"));

	/* one last thing - let's open the tempfile for output... */

	sprintf(buffer, "%s%s%d", temp_dir, temp_form_file, getpid());

	dprint(2, (debugfile,"-- forms sending using file %s --\n", buffer));

	if ((fd = fopen(buffer,"w")) == NULL) {
	  error2("Can't open \"%s\" as output file! (%s).", buffer,
		 error_name(errno));
	  dprint(1, (debugfile,
		  "** Error %s encountered trying to open temp file %s;\n",
		  error_name(errno), buffer));
	  return(0);
	}

	/* NOW we're ready to read the form image in and start prompting... */

	Raw(OFF);
	ClearScreen();

	while ((ptr = fgets(buffer, SLEN, mailfile)) != NULL) {
	  dprint(9, (debugfile, "- read %s", buffer));
	  if (strcmp(buffer, "***\n") == 0) /* end of form! */
	    break;

	  switch ((count = occurances_of(COLON, buffer))) {
	    case 0 : printf("%s", buffer);		/* output line */
		     fprintf(fd, "%s", buffer);
		     break;
            case 1 : if (buffer[0] == COLON) {
	               printf(
"(Enter as many lines as needed, ending with a '.' by itself on a line)\n");
                       while (fgets(buffer, SLEN, stdin) != NULL) {
		         no_ret(buffer);
	                 if (strcmp(buffer, ".") == 0)
	                   break;
	                 else
			   fprintf(fd,"%s\n", buffer);
		       }
	             }
	             else
		       prompt_for_entries(buffer, fd, count);
	             break;
            default: prompt_for_entries(buffer, fd, count);
	  }
	}

	Raw(ON);
	fclose(fd);

	/** let's just mail this off now... **/

	mail_form(address, subject);

	return(1);
}

prompt_for_entries(buffer, fd, entries)
char *buffer;
FILE *fd;
int  entries;
{
	/** deals with lines that have multiple colons on them.  It must first
	    figure out how many spaces to allocate for each field then prompts
	    the user, line by line, for the entries...
	**/

	char mybuffer[SLEN], prompt[SLEN], spaces[SLEN];
	register int  field_size, i, j, offset = 0, extra_tabs = 0;

	dprint(7, (debugfile,
		"prompt-for-multiple [%d] -entries \"%s\"\n", entries,
		buffer));

	strcpy(prompt, "No Prompt Available:");

	while (entries--) {
	  j=0;
	  i = chloc((char *) buffer + offset, COLON) + 1;
	  while (j < i - 1) {
	    prompt[j] = buffer[j+offset];
	    j++;
	  }
	  prompt[j] = '\0';

	  field_size = 0;

	  while (whitespace(buffer[i+offset])) {
	    if (buffer[i+offset] == TAB) {
	      field_size += 8 - (i % 8);
	      extra_tabs += (8 - (i % 8)) - 1;
	    }
	    else
	      field_size += 1;
	    i++;
	  }

	  offset += i;

	  if (field_size == 0) 	/* probably last prompt in line... */
	    field_size = 78 - (offset + extra_tabs);

	  prompt_for_sized_entry(prompt, mybuffer, field_size);

	  spaces[0] = ' ';	/* always at least ONE trailing space... */
	  spaces[1] = '\0';

	  /*  field_size-1 for the space spaces[] starts with  */
	  for (j = strlen(mybuffer); j < field_size-1; j++)
	    strcat(spaces, " ");

	  fprintf(fd, "%s: %s%s", prompt, mybuffer, spaces);
	  fflush(fd);
	}

	fprintf(fd, "\n");
}

prompt_for_sized_entry(prompt, buffer, field_size)
char *prompt, *buffer;
int   field_size;
{
	/* This routine prompts for an entry of the size specified. */

	register int i;

	dprint(7, (debugfile, "prompt-for-sized-entry \"%s\" %d chars\n",
		prompt, field_size));

	printf("%s: ", prompt);

	for (i=0;i<field_size; i++)
	  putchar('_');
	for (i=0;i<field_size; i++)
	  putchar(BACKSPACE);
	fflush(stdout);

	fgets(buffer, SLEN, stdin);
	no_ret(buffer);

	if (strlen(buffer) > field_size) buffer[field_size-1] = '\0';
}
