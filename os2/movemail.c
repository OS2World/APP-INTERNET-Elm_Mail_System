/* movemail.c  -- Convert all the mail messages in files listed in
 * argv[1] into a single file argv[02] so that the EMACS rmail
 * command can find & process it.
 *
 * Return Code is ...
 *     0 - Everything okay.
 *     1 - Some sort of error, look at stderr for the meaning.
 *
 *  Stuart Wilson
 *  stuartw@pec.co.nz
 *  July 1992.
 *
 * Copyright (C) 1992, 1993  Stuart Wilson.
 *
 * movemail.c is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * movemail.c is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Emacs; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *
 * Fixes & Adulterations:
 * ~~~~~~~~~~~~~~~~~~~~~~
 *
 *  Make it change dir to the mail spool directory on the correct disk.
 *
 *  Paul Andrew
 *  paula@pec.co.nz
 *  October 1992.
 *
 *
 *  Tidied up the way it decides what directory to change to. Now it
 *  looks at argv[1] to decide, rather than use a constant.
 *
 *  Problems parsing the index file. Sometimes there is a from person
 *  and from machine.
 *
 *  Write the resulting mail file better, so that Emacs rmail code can
 *  actually find the mail messages. The result file is written in the
 *  babyl format that Emacs uses as it's internal format.
 *
 *  Stuart Wilson
 *  stuartw@pec.co.nz
 *  April 1993.
 */

/* Modified by Eberhard Mattes, Jun 1994
 * - Fix infinite loop in parser
 * - Fix return codes
 * - Open the index file in SH_DENYRW mode
 */

/* changed for use in elm by Kai Uwe Rommel, rommel@ars.muc.de, Sept. 1993 */

#include <stdio.h>
#include <ctype.h>
#include <share.h>

#ifndef EMACS
#include "sysdefs.h"
#endif

/* A few states for the Index file parsing state machine */
#define SKIP_SPACE_1         0
#define SKIP_FROM_USER       1
#define SKIP_SPACE_2         2
#define SKIP_FROM_MACHINE    3
#define SKIP_SPACE_3         4
#define GET_NOTE_NAME        5
#define SKIP_SPACE_4         6
#define GET_NOTE_EXTN        7
#define DONE_NOTE            8
#define NO_FILE              9

FILE *resultf;

#ifdef TEST
main (argc, argv)
int argc;
char *argv[];
{
    /* Check the number of command line arguments */
    if(argc != 3) {
       fprintf(stderr, "movemail: incorrect usage\n");
         exit(0);
     }

    return(movemail(argv[1], argv[2]));
}
#endif

movemail(inbox, mailbox)
char *inbox, *mailbox;
{
    char filename[256], indexline[1024];
    FILE *indexf, *notef;
    int index_state;
    char *p, *q, *notename;

    /* Go grovelling backwards through the name of the index file, to get
     * the name of the incomming mail spool directory.  We need to change
     * to it.
     */
    strcpy(filename, inbox);

    for(p = &filename[strlen(filename)-1] ;
        p != filename && *p != '/' && *p != '\\' ;
        p--);

    if(p == filename) {
     fprintf(stderr, "Can't find directory in \"%s\"\n", inbox);
      return(1);
    }

    notename = p + 1;

    /* Open the index file for reading */
    indexf=_fsopen(inbox, "r", SH_DENYRW);
    if (indexf == NULL) {
      return(1);
    }

    /* Open the result file for appending */
    resultf=fopen(mailbox, "a");
    if (resultf == NULL) {
      fprintf(stderr, "Can't open mailbox file \"%s\"\n", mailbox);
      return(1);
    }

    /* Read in the index file a line at a time, find the name of the note,
     * and process it
     */
    while (fgets(indexline, sizeof(indexline), indexf) != NULL) {

      /* First attempt at finding the note file name, assume that there is
       * a user and machine name in the index entry. we'll have to skip past
       * them.
       */
      p=indexline;
      index_state = SKIP_SPACE_1;
      while((index_state != DONE_NOTE) &&
            (index_state != NO_FILE)) {
        switch(index_state) {
        case SKIP_SPACE_1:
          if (! isspace(*p++))
            index_state = SKIP_FROM_USER;
          break;

        case SKIP_FROM_USER:
          if (isspace(*p++))
            index_state = SKIP_SPACE_2;
          break;

        case SKIP_SPACE_2:
          if (! isspace(*p++))
            index_state = SKIP_FROM_MACHINE;
          break;

        case SKIP_FROM_MACHINE:
          if (isspace(*p++))
            index_state = SKIP_SPACE_3;
          break;

        case SKIP_SPACE_3:
          if (! isspace(*p)) {
            index_state = GET_NOTE_NAME;
            q = notename;
            *q++ = *p++;
          }
          else p++;
          break;

        case GET_NOTE_NAME:
          if (! isspace(*p)) {
            *q++ = *p++;
          }
          else {
            *q++ = '.';
            index_state = SKIP_SPACE_4;
            p++;
          }
          break;

        case SKIP_SPACE_4:
          if (! isspace(*p)) {
            index_state = GET_NOTE_EXTN;
            *q++ = *p++;
          }
          else p++;         /* This line has been added by Eberhard Mattes */
          break;

        case GET_NOTE_EXTN:
          if(! isspace(*p)) {
            *q++ = *p++;
          }
          else {
            /* We've got all the name of the next, note...
             * terminate the string, and process the note.
             */
            *q = '\0';

            notef=fopen(filename, "r");
            if(notef != NULL) {
              process_note(notef);
              fclose(notef);
              unlink(filename);
              index_state = DONE_NOTE;
            }
            else index_state = NO_FILE;
          }
          break;
        }
      }
      if(index_state == DONE_NOTE)
        continue;

      /* Second attempt at finding the file name. Assume there is no user
       * or machine name before the note name in the index entry.
       */
      p=indexline;
      index_state = SKIP_SPACE_3;
      while((index_state != DONE_NOTE) &&
            (index_state != NO_FILE)) {
        switch(index_state) {
        case SKIP_SPACE_3:
          if (! isspace(*p)) {
            index_state = GET_NOTE_NAME;
            q = notename;
            *q++ = *p++;
          }
          else p++;
          break;

        case GET_NOTE_NAME:
          if (! isspace(*p)) {
            *q++ = *p++;
          }
          else {
            *q++ = '.';
            index_state = SKIP_SPACE_4;
            p++;
          }
          break;

        case SKIP_SPACE_4:
          if (! isspace(*p)) {
            index_state = GET_NOTE_EXTN;
            *q++ = *p++;
          }
          else p++;         /* This line has been added by Kai Uwe Rommel */
          break;

        case GET_NOTE_EXTN:
          if(! isspace(*p)) {
            *q++ = *p++;
          }
          else {
            /* We've got all the name of the next, note...
             * terminate the string, and process the note.
             */
            *q = '\0';
            notef=fopen(filename, "r");
            if(notef != NULL) {
              process_note(notef);
              fclose(notef);
              unlink(filename);
              index_state = DONE_NOTE;
            }
            else index_state = NO_FILE;
          }
          break;
        }
      }
      if(index_state == DONE_NOTE)
        continue;

      /* Any other attempts ?? */
      }

    /* Now remove the index file */
    fclose(indexf);
    /* Here's a timing window! */
    unlink(inbox);

    /* everything is Okay */
    fclose(resultf);
    return(0);
}


/* Return true if the line in the buffer starts with "From "
 */
int fromline(buff)
char *buff;
{
  return(((buff[0]=='F') &&
	  (buff[1]=='r') &&
	  (buff[2]=='o') &&
	  (buff[3]=='m') &&
	  (buff[4]==' ')) ? 1 : 0) ;
}


/* Return true if the line in the buffer has the following regexp
 *              ^[A-Z][a-z-]*:
 */

int header_line(buff)
char *buff;
{
  char *p;

  p=buff;
  if(! isupper(*p))
    return(0);
  p++;

  /* skip to the next character non lower-case and non '-' */
  while((islower(*p) || (*p == '-')))
    p++;

  /* It's a header line of some sort */
  if(*p == ':')
    return(1);

  /* failed */
  return(0);
}


/* Copy the contents of the note with name NOTE into the result file...
 * a few little items to think about....
 *
 * We'll unconfuse the Emacs rmail code by chucking out the "From " line
 * from the headers as well as any MMDF delimiters.
 *
 * Things will be alot easier if we write the result file in babyl format,
 * which is the format Emacs uses.
 */
process_note(notef)
FILE *notef;
{
    char buff[1024], *p;
#ifndef EMACS
    fputs(MSG_SEPERATOR, resultf);
    while(fgets(buff, sizeof(buff), notef) != NULL)
      fputs(buff, resultf);
#else
    int doing_headers;

    doing_headers = 1;
    fputs("\014\n0, unseen,,\n*** EOOH ***\n", resultf);

    while(fgets(buff, sizeof(buff), notef) != NULL) {
      /* chuck out any "From  blh@blhablah" lines */
      if(doing_headers && fromline(buff)) {
        continue;
      }
      if(strlen(buff) == 1)
        doing_headers=0;

      /* Flatten any \001 characters remaining from mmdf */
      for(p=buff; *p; p++)
	  *p = (*p == '\001') ? ' ' : *p ;

      fputs(buff, resultf);
    }

    fputs("\037", resultf);
#endif
}
