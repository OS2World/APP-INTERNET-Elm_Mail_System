
static char rcsid[] = "@(#)$Id: readmsg.c,v 4.1.1.2 90/06/21 22:40:12 syd Exp $";

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
 * $Log:	readmsg.c,v $
 * Revision 4.1.1.2  90/06/21  22:40:12  syd
 * Reduce occurrences of unprotected "From " confusing message count
 * From: Marius Olafsson
 *
 * Revision 4.1.1.1  90/06/21  22:33:51  syd
 * Fix error message in readmsg and clear variable for use
 * From: Hans Buurman
 *
 * Revision 4.1  90/04/28  22:44:52  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This routine adds the functionality of the "~r" command to the Elm mail
    system while still allowing the user to use the editor of their choice.

    The program, without any arguments, tries to read a file in the users home
    directory called ".readmsg" (actually defined in the sysdefs.h system
    defines file) and if it finds it reads the current message.  If it doesn't
    find it, it will return a usage error.

    The program can also be called with an explicit message number, list of
    message numbers, or a string to match in the message (including the header).
    NOTE that when you use the string matching option it will match the first
    message containing that EXACT (case sensitive) string and then exit.
**/

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>

#include "defs.h"

/** three defines for what level of headers to display **/

#define ALL		1
#define WEED		2
#define NONE		3

#define metachar(c)	(c == '=' || c == '+' || c == '%')

static char ident[] = { WHAT_STRING };

#define  MAX_LIST	25		/* largest single list of arguments */

#define  LAST_MESSAGE	9999		/* last message in list ('$' char)  */
#define  LAST_CHAR	'$'		/* char to delimit last message..   */
#define  STAR		'*'		/* char to delimit all messages...  */

int read_message[MAX_LIST]; 		/* list of messages to read	    */
int messages = 0;			/* index into list of messages      */

int numcmp();				/* strcmp, but for numbers          */
char *words();				/* function defined below...        */

struct passwd *getpwuid();
struct passwd *pass;
char home[SLEN];		/* the users home directory  */

extern char *optarg;		/* for parsing the ... 		    */
extern int   optind;			/*  .. starting arguments           */

char *getenv();				/* keep lint happy */

main(argc, argv)
int argc;
char *argv[];
{
	FILE *file;			        /* generic file descriptor! */
	char filename[SLEN], 			/* filename buffer          */
	     infile[SLEN],			/* input filename	    */
	     buffer[SLEN], 			/* file reading buffer      */
	     string[SLEN],			/* string match buffer      */
	     *cp,
             *prog = argv[0];

	int current_in_queue = 0, 		/* these are used for...     */
	    current = 0,			/* ...going through msgs     */
	    list_all_messages = 0,		/* just list 'em all??       */
	    num, 				/* for argument parsing      */
	    page_breaks = 0,			/* use "^L" breaks??         */
            total,				/* number of msgs current    */
	    include_headers = WEED, 		/* flag: include msg header? */
	    last_message = 0, 			/* flag: read last message?  */
	    not_in_header = 0,			/* flag: in msg header?      */
#ifdef MMDF
	    newheader = 0,			/* flag: hit ^A^A^A^A line   */
#endif /* MMDF */
	    string_match = 0;			/* flag: using string match?  */
	    string[0] = '\0';			/* init match string to empty */
	    infile[0] = '\0';			/* init mail file to empty    */

        initpaths();

	/**** start of the actual program ****/

	while ((num = getopt(argc, argv, "nhf:p")) != EOF) {
	  switch (num) {
	    case 'n' : include_headers = NONE;		break;
	    case 'h' : include_headers = ALL;		break;
	    case 'f' : strcpy(infile, optarg);
		       if (metachar(infile[0]))
	                 if (expand(infile) == 0)
	                   printf("%s: couldn't expand filename %s!\n",
			          argv[0], infile);
		       break;
	    case 'p' : page_breaks++;			break;
	    case '?' : usage(prog);
	  	       exit(1);
	  }
	}

	/** whip past the starting arguments so that we're pointing
	    to the right stuff... **/

	*argv++;	/* past the program name... */

	while (optind-- > 1) {
	  *argv++;
	  argc--;
	}

	/** now let's figure out the parameters to the program... **/

	if (argc == 1) {	/* no arguments... called from 'Elm'? */
	  if((pass = getpwuid(getuid())) == NULL) {
	    printf("You have no password entry!\n");
	    exit(1);
	  }
	  strcpy(home, pass->pw_dir);
	  sprintf(filename, "%s/%s", home, readmsg_file);
	  if ((file = fopen(filename, "r")) != NULL) {
	    fscanf(file, "%d", &(read_message[messages++]));
	    fclose(file);
	  }
	  else {	/* no arguments AND no .readmsg file!! */
            usage(prog);
	    exit(1);
	  }
	}
	else if (! isdigit(*argv[0]) && *argv[0] != LAST_CHAR &&
	         *argv[0] != STAR) {
	  string_match++;

	  while (*argv)
	    sprintf(string, "%s%s%s", string, string[0] == '\0'? "" : " ",
		    *argv++);
	}
	else if (*argv[0] == STAR) 		/* all messages....   */
	  list_all_messages++;
	else { 					  /* list of nums   */

	  while (--argc > 0) {
	    num = -1;

	    sscanf(*argv,"%d", &num);

	    if (num < 0) {
	      if (*argv[0] == LAST_CHAR) {
	        last_message++;
		num = LAST_MESSAGE;
	      }
	      else {
	        fprintf(stderr,"I don't understand what '%s' means...\n",
			*argv);
	       	exit(1);
	      }
	    }
	    else if (num == 0) {	/* another way to say "last" */
	      last_message++;
	      num = LAST_MESSAGE;
	    }

	    *argv++;

	    read_message[messages++] = num;
	  }

	  /** and now sort 'em to ensure they're in a reasonable order... **/

	  qsort(read_message, messages, sizeof(int), numcmp);
	}

	/** Now let's get to the mail file... **/

	if (strlen(infile) == 0) {
	  if ((cp = getenv("MAIL")) == NULL) {
	    if ((cp = getenv("LOGNAME")) == NULL)
	      sprintf(infile, "%s/%s", mailhome, getenv("USER"));
	    else
	      sprintf(infile, "%s/%s", mailhome, cp);
	  }
	  else
	    strcpy(infile, cp);
	}

	if ((file = fopen(infile, "r")) == NULL) {
	  printf("But you have no mail! [ file = %s ]\n", infile);
	  exit(0);
	}

	/** Now it's open, let's display some 'ole messages!! **/

	if (string_match || last_message) {   /* pass through it once */

	  if (last_message) {
	    total = count_messages(file);	/* instantiate count */
	    for (num=0; num < messages; num++)
	      if (read_message[num] == LAST_MESSAGE)
		read_message[num] = total;
	  }
	  else if (string_match)
	    match_string(file, string);		/* stick msg# in list */

	  if (total == 0 && ! string_match) {
	    printf("There aren't any messages to read!\n");
	    exit(0);
	  }
	}

 	/** now let's have some fun! **/
#ifdef MMDF
	newheader = 0;
#endif /* MMDF */

	while (fgets(buffer, SLEN, file) != NULL) {
#ifdef MMDF
	  if (strcmp(buffer, MSG_SEPERATOR) == 0 ||
              !newheader && real_from(buffer))
            newheader = 1; /* !newheader; */
          else
            newheader = 0;
	  if (newheader) {
#else
	  if (real_from(buffer)) {
#endif /* MMDF */
	    if (! list_all_messages) {
	      if (current == read_message[current_in_queue])
	        current_in_queue++;
	      if (current_in_queue >= messages)
	        exit(0);
	    }
	    current++;
	    not_in_header = 0;	/* we're in the header! */
	  }

	  if (current == read_message[current_in_queue] || list_all_messages)
#ifdef MMDF
	    if ((include_headers==ALL || not_in_header)
		&& strcmp(buffer, MSG_SEPERATOR) != 0)
#else
	    if (include_headers==ALL || not_in_header)
#endif /* MMDF */
	      printf("%s", buffer);
	    else if (strlen(buffer) < 2) {
	      not_in_header++;
	      if (include_headers==WEED)
		list_saved_headers(page_breaks);
	    }
	    else if (include_headers==WEED)
	      possibly_save(buffer); 	/* check to see if we want this */
	}

	exit(0);
}

usage(prog)
char *prog;
{
  printf("\nUsage: %s [-n|-h] [-f filename] [-p] <message list>\n", prog);
  printf("\n  -n   don't print any headers"
         "\n  -h   print all headers"
         "\n  -p   printf form feeds between messages\n"
         "\n  -f filename    use this instead of default mailbox\n");
}

int
count_messages(file)
FILE *file;
{
	/** Returns the number of messages in the file **/

	char buffer[SLEN];
	int  count = 0;
#ifdef MMDF
	int  newheader = 0;
#endif /* MMDF */

	while (fgets(buffer, SLEN, file) != NULL)
#ifdef MMDF
	  if (strcmp(buffer, MSG_SEPERATOR) == 0
              || !newheader && real_from(buffer)) {
	    newheader = 1;
#else
	  if (real_from(buffer)) |
#endif /* MMDF */
	    count++;
          }
#ifdef MMDF
          else
	    newheader = 0;
#endif /* MMDF */

	rewind( file );
	return( count );
}

match_string(mailfile, string)
FILE *mailfile;
char *string;
{
	/** Increment "messages" and put the number of the message
	    in the message_count[] buffer until we match the specified
	    string... **/

	char buffer[SLEN];
	int  message_count = 0;
#ifdef MMDF
	int  newheader = 0;
#endif /* MMDF */

	while (fgets(buffer, SLEN, mailfile) != NULL) {
#ifdef MMDF
	  if (strcmp(buffer, MSG_SEPERATOR) == 0
              || !newheader && real_from(buffer)) {
	    newheader = 1;
#else
	  if (real_from(buffer)) {
#endif /* MMDF */
	    message_count++;
          }
#ifdef MMDF
          else
	    newheader = 0;
#endif /* MMDF */

	  if (in_string(buffer, string)) {
	    read_message[messages++] = message_count;
	    rewind(mailfile);
	    return;
	  }
	}

	fprintf(stderr,"Couldn't find message containing '%s'\n", string);
	exit(1);
}

int
numcmp(a, b)
int *a, *b;
{
	/** compare 'a' to 'b' returning less than, equal, or greater
	    than, accordingly.
	 **/

	return(*a - *b);
}

static char from[SLEN], subject[SLEN], date[SLEN], to[SLEN];

possibly_save(buffer)
char *buffer;
{
	/** Check to see what "buffer" is...save it if it looks
	    interesting... We'll always try to get SOMETHING
	    by tearing apart the "From " line...  **/

	if (strncmp(buffer, "Date:", 5) == 0)
	  strcpy(date, buffer);
	else if (strncmp(buffer, "Subject:", 8) == 0)
	  strcpy(subject,buffer);
	else if (strncmp(buffer,"From:", 5) == 0)
	  strcpy(from, buffer);
	else if (strncmp(buffer,"To: ", 3) == 0)
	  strncpy(to, buffer, SLEN);
	else if (strncmp(buffer,"From ", 5) == 0) {
	  sprintf(from, "From: %s\n", words(2,1, buffer));
	  sprintf(date,"Date: %s",    words(3,7, buffer));
	  to[0] = '\0';
	  subject[0] = '\0';
	}
}

list_saved_headers(page_break)
int page_break;
{
	/** This routine will display the information saved from the
	    message being listed...If it displays anything it'll end
	    with a blank line... **/

	register int displayed_line = FALSE;
	static int messages_listed = 0;

	if (messages_listed++)
	  if (page_break)
	    putc(FORMFEED, stdout);
	  else
	    printf(
"\n--------------------------------------------------------------------\n\n\n");

	if (strlen(from)    > 0) { printf("%s", from);    displayed_line++;}
	if (strlen(subject) > 0) { printf("%s", subject); displayed_line++;}
	if (strlen(to)      > 0) { printf("%s", to);      displayed_line++;}
	if (strlen(date)    > 0) { printf("%s", date);    displayed_line++;}

	if (displayed_line)
	   putc('\n', stdout);
}

char *words(word, num_words, buffer)
int word, num_words;
char *buffer;
{
	/** Return a buffer starting at 'word' and containing 'num_words'
	    words from buffer.  Assume white space will delimit each word.
	**/

	static char internal_buffer[SLEN];
	char   *wordptr, *bufptr, mybuffer[SLEN], *strtok();
	int    wordnumber = 0, copying_words = 0;

	internal_buffer[0] = '\0';	/* initialize */

	strcpy(mybuffer, buffer);
	bufptr = (char *) mybuffer;	/* and setup */

	while ((wordptr = strtok(bufptr, " \t")) != NULL) {
	  if (++wordnumber == word) {
	    strcpy(internal_buffer, wordptr);
	    copying_words++;
	    num_words--;
	  }
	  else if (copying_words) {
	    strcat(internal_buffer, " ");
	    strcat(internal_buffer, wordptr);
	    num_words--;
	  }

	  if (num_words < 1)
	    return((char *) internal_buffer);

	  bufptr = NULL;
	}

	return( (char *) internal_buffer);
}

int
real_from(buffer)
char *buffer;
{
	/***** Returns true iff 's' has the seven 'from' fields, (or
	       8 - some machines include the TIME ZONE!!!) *****/

	char sixthword[STRING], seventhword[STRING],
	     eighthword[STRING], ninthword[STRING];

	/* From <user> <day> <month> <day> <hr:min:sec> <year> */

	if(strncmp(buffer, "From ", 5) != 0)
	  return(FALSE);

	/* Extract 6th, 7th, 8th, and 9th words */
	seventhword[0] = eighthword[0] = ninthword[0] = '\0';
	sscanf(buffer, "%*s %*s %*s %*s %*s %s %s %s %s",
	  sixthword, seventhword, eighthword, ninthword);

	/* Not a from line if 6th word doesn't have colons for time field */
	if(strlen(sixthword) < 3)
	  return(FALSE);
	if (sixthword[1] != ':' && sixthword[2] != ':')
	  return(FALSE);

	/* Not a from line if there is no seventh word */
	if(seventhword[0] == '\0')
	  return(FALSE);

	/* Not a from line if there is a ninthword */
	if (eighthword[0] != '\0') {
	  if(ninthword[0] != '\0')
	    return(FALSE);
	}

	return(TRUE);
}
