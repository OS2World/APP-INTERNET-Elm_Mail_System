
static char rcsid[] = "@(#)$Id: newmbox.c,v 4.1.1.5 90/10/24 15:46:47 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.5 $   $State: Exp $
 *
 * 			Copyright (c) 1988, USENET Community Trust
 * 			Copyright (c) 1988, 1989, 1990 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log:	newmbox.c,v $
 * Revision 4.1.1.5  90/10/24  15:46:47  syd
 * Init time zone variable to avoid garbage strings
 * From: Norman J. Meluch
 *
 * Revision 4.1.1.4  90/10/10  12:53:42  syd
 * allow words in either case
 * quickie fix
 * From: Syd
 *
 * Revision 4.1.1.3  90/10/07  19:54:56  syd
 * fix where x.400 type mailers cause Elm to tag all messages as urgent.
 * From: ldk@udev.cdc.com (ld kelley x-6857)
 *
 * Revision 4.1.1.2  90/06/26  20:18:06  syd
 * Fix double word
 * From: Peter Kendell <pete@tcom.stc.co.uk>
 *
 *
 * Revision 4.1.1.1  90/06/21  21:10:33  syd
 * Add another fixed mailbox id
 * From: Syd
 *
 * Revision 4.1  90/04/28  22:43:34  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/**  read new folder **/

#include <ctype.h>
#include "headers.h"

#ifdef BSD
#undef tolower		/* we have our own "tolower" routine instead! */
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif

#ifndef OS2
extern int errno;
#endif

char *error_name(), *error_description();
char *malloc(), *realloc();
#ifndef __GNUC__
char *alloca();
#endif
char *strcpy(), *strncpy(), *rindex(), *index();
unsigned long sleep();
void rewind();
void exit();
long bytes();

int
newmbox(new_file, adds_only)
char *new_file;
int adds_only;
{
	/** Read a folder.

	    new_file	- name of folder  to read. It is up to the calling
			  function to make sure that the file can be
			  read by the user. This is not checked in this
			  function. The reason why it is not checked here
			  is due to the situation where the user wants to
			  change folders: the new folder must be checked
			  for access *before* leaving the old one, which
			  is before this function gets called.
	    adds_only	- set if we only want to read newly added messages to
				same old folder.

	**/

	int  same_file;
	int  new_folder_type;
	int err;
	char new_tempfile[SLEN];

#ifdef OS2
	char *new_file2 = (char *) alloca(SLEN);
	_fullpath(new_file2, new_file, SLEN);
	strlwr(new_file2);
	unixpath(new_file2);
#define new_file new_file2
#endif

	/* determine type of new mailfile and calculate temp file name */
	if((new_folder_type = get_folder_type(new_file)) == SPOOL)
	  mk_temp_mail_fn(new_tempfile, new_file);
	else
	  *new_tempfile = '\0';

	/* determine whether we are changing files */
	same_file = !(strcmp(new_file, cur_folder));

	/* If we are changing files and we are changing to a spool file,
	 * make sure there isn't a temp file for it, because if
	 * there is, someone else is using ELM to read the new file,
	 * and we don't want to be reading it at the same time.
	 */
	if((new_folder_type == SPOOL) && (!same_file)) {
	  if (access(new_tempfile, ACCESS_EXISTS) != -1) {
	    if(folder_type != NO_NAME) ClearScreen();
	    Centerline(15,
	      "Hey! An instantiation of ELM is already reading this mail!");
	    Centerline(17,
	      "If this is in error, then you'll need to save a copy of");
	    Centerline(18, "the following file then remove it:");
	    Centerline(19, new_tempfile);
	    MoveCursor(LINES, 0);  /* so shell prompt upon exit is on newline */
	    silently_exit();
	  }
	}

	if (mailfile != NULL)
	  (void) fclose(mailfile);  /* close it first, to avoid too many open */

	/* If we were reading a spool file and we are not just reading
	 * in the additional new messages to the same file, we need to
	 * remove the corresponding tempfile.
	 */

	if((folder_type == SPOOL) && !adds_only) {
	  if (access(cur_tempfolder, ACCESS_EXISTS) != -1) {
	    if (unlink(cur_tempfolder) != 0) {
	      error2("Sorry, can't unlink the temp file %s [%s]!\n\r",
	        cur_tempfolder, error_name(errno));
	      silently_exit();
	    }
	  }
	}

	/* Okay! Now establish this new file as THE file */
	strcpy(cur_folder, new_file);
	folder_type = new_folder_type;
	strcpy(cur_tempfolder, new_tempfile);

	clear_error();
	clear_central_message();

	if ((mailfile = fopen(cur_folder,"rb")) == NULL)  {
	  if (errno != ENOENT ) { /* error on anything but file not exist */
	    err = errno;
	    Write_to_screen("\n\rfail on open in newmbox, open %s failed!!\n\r", 1,
		    cur_folder);
	    Write_to_screen("** %s - %s. **\n\r", 2,
		    error_name(err), error_description(err));
	    dprint(1, (debugfile, "fail on open in newbox, file %s!!\n",
		    cur_folder));
	    rm_temps_exit();
	  }
	  else {
	    mailfile_size = 0;         /* must non-existant folder */
	    message_count = 0;
	    selected = 0;
	  }
	} else {                          /* folder exists, read headers */
	  read_headers(adds_only);
	}

	if(!same_file)		/* limit mode off if this is a new file */
	  selected = 0;
	if (!adds_only)		/* limit mode off if recreating headers */
	  selected = 0;		/* because we loose the 'Visible' flag */

	dprint(1, (debugfile,
	  "New folder %s type %s temp file %s (%s)\n", cur_folder,
	  (folder_type == SPOOL ? "spool" : "non-spool"),
	  (*cur_tempfolder ? cur_tempfolder : "none"), "newmbox"));

	return(0);
}

int
get_folder_type(filename)
char *filename;
{
	/** returns the type of mailfile filename is
	    NO_NAME = no name
	    SPOOL = consisting only of mailhome plus base file name
		    (no intervening directory name)
	    NON_SPOOL = a name that is not SPOOL type above
	 **/

	char *last_slash;

	/* if filename is null or is of zero length */
	if((filename == NULL) || (*filename == '\0'))
	  return(NO_NAME);

#ifdef OS2
#define prefix_path(s, w) (strnicmp(s,w, strlen(w)) == 0)
#else
#define prefix_path(s, w) first_word(s,w)
#endif

	/* if filename begins with mailhome,
	 * and there is a slash in filename,
	 * and there is a filename after it (i.e. last slash is not last char),
	 * and the last character of mailhome is last slash in filename,
	 * it's a spool file .
	 */
	if (prefix_path(filename, mailhome) &&
	    (last_slash = rindex(filename, '/')) != NULL &&
	    *(last_slash + 1) != '\0') 
	{
#ifdef OS2
	  if (!maildir && filename + strlen(mailhome) - 1 == last_slash)
	    return(SPOOL);
	  if (maildir && strncmp(last_slash, "/newmail", 8) == 0)
	  { 
	    char *previous_slash;
	    *last_slash = 0;
	    previous_slash = rindex(filename, '/');
            *last_slash = '/';
	    if (previous_slash != NULL &&
		filename + strlen(mailhome) - 1 == previous_slash)
	      return(SPOOL);
	  }
#else
	  if (filename + strlen(mailhome) - 1 == last_slash)
	    return(SPOOL);
#endif
        }
	/* if file name == default mailbox, its a spool file also
	 * even if its not in the spool directory. (SVR4)
	 */
	if (strcmp(filename, defaultfile) == 0)
	    return(SPOOL);

	return(NON_SPOOL);
}

mk_temp_mail_fn(tempfn, mbox)
char *tempfn, *mbox;
{
	/** create in tempfn the name of the temp file corresponding to
	    mailfile mbox. Mbox is presumed to be a file in mailhome;
	    Strangeness may result if it is not!
	 **/

	char *cp, mb[128];

        strcpy(tempfn, default_temp);
	if (tempfn[strlen (tempfn)-1] != '/')
                strcat(tempfn, "/");
	if((cp = rindex(mbox, '/')) != NULL)
          strcpy(mb, ++cp);
	else
	  strcpy(mb, mbox);
#ifdef OS2
	if ( (cp = strrchr(mb, '.')) != NULL && strlen(cp) <= 4 )
	  *cp = 0;
#endif
	if (strcmp(mb, "mbox") == 0 || strcmp(mb, "mailbox") == 0 ||
	    strcmp(mb, "inbox") == 0)
	  strcat(tempfn, username);
#ifdef OS2
	else if (strncmp(mb, "newmail", 7) == 0) {
	  strcpy(mb, mbox + strlen(mailhome));
	  cp = strchr(mb, '/');
	  if (strncmp(cp, "/newmail", 8) == 0)
	    *cp = 0;
	  strcat(tempfn, mb);
	}
#endif
	else
	  strcat(tempfn, mb);
        strcat(tempfn, temp_mbox);
}

int
read_headers(add_new_only)
int add_new_only;
{
	/** Reads the headers into the headers[] array and leaves the
	    file rewound for further I/O requests.   If the file being
	    read is a mail spool file (ie incoming) then it is copied to
	    a temp file and closed, to allow more mail to arrive during
	    the elm session.  If 'add_new_only' is set, the program will copy
	    the status flags from the previous data structure to the new
	    one if possible and only read in newly added messages.
	**/

	FILE *temp;
	struct header_rec *current_header = NULL;
	char buffer[LONG_STRING], *c;
	long fbytes = 0L, line_bytes = 0L;
	register int line = 0, count = 0, another_count,
	  subj = 0, copyit = 0, in_header = 0;
	int count_x, count_y = 17, err;
	int in_to_list = FALSE, forwarding_mail = FALSE, first_line = TRUE;

	static int first_read = 0;
#ifdef MMDF
        int newheader = 0;
        int fromtoo = 1;
#endif /* MMDF */

	if (folder_type == SPOOL) {
	  lock(INCOMING);	/* ensure no mail arrives while we do this! */
	  if (! add_new_only) {
	    if (access(cur_tempfolder, ACCESS_EXISTS) != -1) {
	      /* Hey!  What the hell is this?  The temp file already exists? */
	      /* Looks like a potential clash of processes on the same file! */
	      unlock();				     /* so remove lock file! */
	      error("What's this?  The temp folder already exists??");
	      sleep(2);
	      error("Ahhhh... I give up.");
	      silently_exit();	/* leave without tampering with it! */
	    }
	    if ((temp = fopen(cur_tempfolder,"wb")) == NULL) {
	     err = errno;
	     unlock();	/* remove lock file! */
	     Raw(OFF);
	     Write_to_screen(
		     "\n\rCouldn't open file %s for use as temp file.\n\r",
		     1, cur_tempfolder);
	     Write_to_screen("** %s - %s. **\n\r", 2,
		     error_name(err), error_description(err));
	     dprint(1, (debugfile,
                "Error: Couldn't open file %s as temp mbox.  errno %s (%s)\n",
	         cur_tempfolder, error_name(err), "read_headers"));
	     rm_temps_exit();
	    }
	   copyit++;
	   chown(cur_tempfolder, userid, groupid);
	   chmod(cur_tempfolder, 0700);	/* shut off file for other people! */
	 }
	 else {
	   if ((temp = fopen(cur_tempfolder,"ab")) == NULL) {
	     err = errno;
	     unlock();	/* remove lock file! */
	     Raw(OFF);
	     Write_to_screen(
		     "\n\rCouldn't reopen file %s for use as temp file.\n\r",
		     1, cur_tempfolder);
	     Write_to_screen("** %s - %s. **\n\r", 2,
		     error_name(err), error_description(err));
	     dprint(1, (debugfile,
                "Error: Couldn't reopen file %s as temp mbox.  errno %s (%s)\n",
	         cur_tempfolder, error_name(err), "read_headers"));
	     rm_temps_exit();
	    }
	   copyit++;
	  }
	}

	if (! first_read++) {
	  ClearLine(LINES-1);
	  ClearLine(LINES);
	  if (add_new_only)
	    PutLine2(LINES, 0, "Reading in %s, message: %d", cur_folder,
		     message_count);
	  else
	    PutLine1(LINES, 0, "Reading in %s, message: 0", cur_folder);
	  count_x = LINES;
          count_y = 22 + strlen(cur_folder);
	}
	else {
	  count_x = LINES-2;
	  PutLine0(LINES-2, 0, "Reading message: 0");
	}

#ifdef MMDF
#ifdef OS2
	if (fgets(buffer, LONG_STRING, mailfile)) {
          fixline(buffer);
          fromtoo = !mmdf_strict || strcmp(buffer, MSG_SEPERATOR) != 0;
        }
        rewind(mailfile);
#endif
#endif
	if (add_new_only) {
	   if (fseek(mailfile, mailfile_size, 0) == -1) {
	     err = errno;
	     Write_to_screen(
		"\n\rCouldn't seek to %ld (end of folder) in %s!\n\r", 2,
	     	mailfile_size, cur_folder);
	     Write_to_screen("** %s - %s. **\n\r", 2,
		     error_name(err), error_description(err));
	     dprint(1, (debugfile,
     "Error: Couldn't seek to end of folder %s: (offset %ld) Errno %s (%s)\n",
	        cur_folder, mailfile_size, error_name(err), "read_headers"));
	     emergency_exit();
	   }
	   count = message_count;		/* next available  */
	   fbytes = mailfile_size;		/* start correctly */
	}

	/** find the size of the folder then unlock the file **/

	mailfile_size = bytes(cur_folder);
	unlock();

	/** now let's copy it all across accordingly... **/

	while (fbytes < mailfile_size) {

	  if (fgets(buffer, LONG_STRING, mailfile) == NULL) break;

	  if (copyit)
	    if (fputs(buffer, temp) == EOF) {
		err = errno;
		Write_to_screen("\n\rWrite to tempfile %s failed!!\n\r", 1,
				cur_tempfolder);
		Write_to_screen("** %s - %s. **\n\r", 2,
				error_name(err), error_description(err));
		dprint(1, (debugfile, "Can't write to tempfile %s!!\n",
			   cur_tempfolder));
		rm_temps_exit();
	    }
	  line_bytes = (long) strlen(buffer);
          fixline(buffer);

	  /* Fix below to increment line count ONLY if we got a full line.
	   * Input lines longer than the fgets buffer size would
	   * get counted each time a subsequent part of them was
	   * read in. This meant that when the faulty line count was used
	   * to display the message, part of the next message
	   * was displayed at the end of the message.
	   */
	  if(buffer[strlen(buffer)-1] == '\n') line++;

	  if (fbytes == 0L || first_line) { 	/* first line of file... */
	    if (folder_type == SPOOL) {
	      if (first_word(buffer, "Forward to ")) {
	        set_central_message("Mail being forwarded to %s",
                   (char *) (buffer + 11));
	        forwarding_mail = TRUE;
	      }
	    }

	    /** flush leading blank lines before next test... **/
	    if (strlen(buffer) == 1) {
	      fbytes++;
	      continue;
	    }
	    else
	      first_line = FALSE;

#ifdef MMDF
	    if (!forwarding_mail && strcmp(buffer, MSG_SEPERATOR) != 0
	        && !first_word(buffer, "From ") ) {
#else
	    if (! first_word(buffer, "From ") && !forwarding_mail) {
#endif /* MMDF */
	      PutLine0(LINES, 0,
		  "\n\rFolder is corrupt!!  I can't read it!!\n\r\n\r");
	      fflush(stderr);
	      dprint(1, (debugfile,
			   "\n\n**** First mail header is corrupt!! ****\n\n"));
	      dprint(1, (debugfile, "Line is;\n\t%s\n\n", buffer));
              mail_only++;	/* to avoid leave() cursor motion */
              leave();
	    }
	  }

#ifdef MMDF
	  if (strcmp(buffer, MSG_SEPERATOR) == 0
	      || !newheader && fromtoo && first_word(buffer,"From ")
                            && real_from(buffer, NULL)) {
            newheader = 1; /* !newheader; */
#else
	  if (first_word(buffer,"From ")) {
#endif /* MMDF */
	    /** allocate new header pointers, if needed... **/

	    if (count >= max_headers) {
	      struct header_rec **new_headers;
	      int new_max;

	      new_max = max_headers + KLICK;
	      if (max_headers == 0) {
		new_headers = (struct header_rec **)
		  malloc(new_max * sizeof(struct header_rec *));
	      }
	      else {
		new_headers = (struct header_rec **)
		  realloc(headers, new_max * sizeof(struct header_rec *));
	      }
	      if (new_headers == NULL) {
	        error1(
      "\n\r\n\rCouldn't allocate enough memory! Message #%d.\n\r\n\r",
			count);
	        leave();
	      }
	      headers = new_headers;
	      while (max_headers < new_max)
		headers[max_headers++] = NULL;
	    }

	    /** allocate new header structure, if needed... **/

	    if (headers[count] == NULL) {
	      struct header_rec *h;

	      if ((h = (struct header_rec *)
			malloc(sizeof(struct header_rec))) == NULL) {
	        error1(
      "\n\r\n\rCouldn't allocate enough memory! Message #%d.\n\r\n\r",
			count);
	        leave();
	      }
	      headers[count] = h;
	    }

	    if (real_from(buffer, headers[count])) {
	      current_header = headers[count];

	      current_header->offset = (long) fbytes;
	      current_header->index_number = count+1;
	      /* set default status - always 'visible'  - and
	       * if a spool file, presume 'new', otherwise
	       * 'read', for the time being until overridden
	       * by a Status: header.
	       * We presume 'read' for nonspool mailfile messages
	       * to be compatible messages stored with older versions of elm,
	       * which didn't support a Status: header.
	       */
	      if(folder_type == SPOOL)
		current_header->status = VISIBLE | NEW | UNREAD;
	      else
		current_header->status = VISIBLE;

	      strcpy(current_header->subject, "");	/* clear subj    */
	      strcpy(current_header->to, "");		/* clear to    */
	      strcpy(current_header->mailx_status, "");	/* clear status flags */
	      strcpy(current_header->time_zone, "");	/* clear time zone name */
	      strcpy(current_header->messageid, "<no.id>"); /* set no id into message id */
	      current_header->encrypted = 0;		/* clear encrypted */
	      current_header->exit_disposition = UNSET;
	      current_header->status_chgd = FALSE;

	      /* Set the number of lines for the _preceding_ message,
	       * but only if there was a preceding message and
	       * only if it wasn't calculated already. It would
	       * have been calculated already if we are only
	       * reading headers of new messages that have just arrived,
	       * and the preceding message was one of the old ones.
	       */
	      if ((count) && (!add_new_only || count > message_count))
	        headers[count-1]->lines = line;

	      count++;
	      subj = 0;
	      line = 0;
	      in_header = 1;
	      PutLine1(count_x, count_y, "%d", count);
#ifdef MMDF
	    } else if (newheader) {
	      current_header = headers[count];

	      current_header->offset = (long) fbytes;
	      current_header->index_number = count+1;

	      /* set default status - always 'visible'  - and
	       * if a spool file, presume 'new', otherwise
	       * 'read', for the time being until overridden
	       * by a Status: header.
	       * We presume 'read' for nonspool mailfile messages
	       * to be compatible messages stored with older versions of elm,
	       * which didn't support a Status: header.
	       */
	      if(folder_type == SPOOL)
		current_header->status = VISIBLE | NEW | UNREAD;
	      else
		current_header->status = VISIBLE;

	      strcpy(current_header->from, "");		/* clear from    */
	      strcpy(current_header->dayname, "");	/* clear dayname */
	      strcpy(current_header->month, "");	/* clear month   */
	      strcpy(current_header->day, "");		/* clear day     */
	      strcpy(current_header->time, "");		/* clear time    */
	      strcpy(current_header->year, "");		/* clear year    */
	      strcpy(current_header->subject, "");	/* clear subj    */
	      strcpy(current_header->to, "");		/* clear to    */
	      strcpy(current_header->mailx_status, "");	/* clear status flags */
	      strcpy(current_header->messageid, "<no.id>"); /* set no id into message id */
	      current_header->encrypted = 0;		/* clear encrypted */
	      current_header->exit_disposition = UNSET;
	      current_header->status_chgd = FALSE;

	      /* Set the number of lines for the _preceding_ message,
	       * but only if there was a preceding message and
	       * only if it wasn't calculated already. It would
	       * have been calculated already if we are only
	       * reading headers of new messages that have just arrived,
	       * and the preceding message was one of the old ones.
	       */
	      if ((count) && (!add_new_only || count > message_count))
	        headers[count-1]->lines = line;

	      count++;
	      subj = 0;
	      line = 0;
	      in_header = 1;
	      PutLine1(count_x, count_y, "%d", count);
	      dprint(1, (debugfile,
			   "\n\n**** Added header record ****\n\n"));
#endif /* MMDF */
	    } else if (count == 0) {
	      /* if this is the first "From" in file but the "From" line is
	       * not of the proper format, we've got a corrupt folder.
	       */
	      PutLine0(LINES, 0,
		  "\n\rFolder is corrupt!!  I can't read it!!\n\r\n\r");
	      fflush(stderr);
	      dprint(1, (debugfile,
			   "\n\n**** First mail header is corrupt!! ****\n\n"));
	      dprint(1, (debugfile, "Line is;\n\t%s\n\n", buffer));
              mail_only++;	/* to avoid leave() cursor motion */
              leave();
	    }
	  }
	  else if (in_header) {
            newheader = 0;
#ifdef MMDF
	    if (first_word(buffer,"From "))
	      real_from(buffer, current_header);
#endif /* MMDF */
	    if (first_word(buffer,">From:"))
	      parse_arpa_who(buffer, current_header->from, FALSE);
	    else if (first_word(buffer,">From"))
	      forwarded(buffer, current_header); /* return address */
	    else if (first_word(buffer,"Subject:") ||
		     first_word(buffer,"Subj:") ||
		     first_word(buffer,"Re:")) {
	      if (! subj++) {
	        remove_first_word(buffer);
	        copy_sans_escape(current_header->subject, buffer, STRING);
		remove_possible_trailing_spaces(current_header->subject);
	      }
	    }
	    else if (first_word(buffer,"From:")) {
#ifdef MMDF
	      parse_arpa_who(buffer, current_header->from, TRUE);
	      dprint(1, (debugfile,
			   "\n\n**** Calling parse_arpa_who for from ****\n\n"));
#else
	      parse_arpa_who(buffer, current_header->from, FALSE);
#endif /* MMDF */

	    }
	    else if (first_word(buffer, "Message-Id:") ||
		     first_word(buffer, "Message-ID:")) {
	      buffer[strlen(buffer)-1] = '\0';
	      strcpy(current_header->messageid,
		     (char *) buffer + 12);
	    }

	    else if (first_word(buffer, "Expires:"))
	      process_expiration_date((char *) buffer + 9,
				      &(current_header->status));

	    /** when it was sent... **/

	    else if (first_word(buffer, "Date:")) {
	      dprint(1, (debugfile,
			   "\n\n**** Calling parse_arpa_date ****\n\n"));
	      remove_first_word(buffer);
	      parse_arpa_date(buffer, current_header);
	    }

	    /** some status things about the message... **/

	    else if ((first_word(buffer, "Priority:") ||
		     first_word(buffer, "Importance: 2")) &&
		   !(first_word(buffer, "Priority: normal") ||
		     first_word(buffer, "Priority: Normal") ||
		     first_word(buffer, "Priority: Non-urgent") ||
		     first_word(buffer, "Priority: non-urgent")))
	      current_header->status |= URGENT;
	    else if (first_word(buffer, "Sensitivity: 2"))
	      current_header->status |= PRIVATE;
	    else if (first_word(buffer, "Sensitivity: 3"))
	      current_header->status |= CONFIDENTIAL;
	    else if (first_word(buffer, "Content-Type: mailform"))
	      current_header->status |= FORM_LETTER;
	    else if (first_word(buffer, "Action:"))
	      current_header->status |= ACTION;

	    /** next let's see if it's to us or not... **/

	    else if (first_word(buffer, "To:")) {
	      in_to_list = TRUE;
	      current_header->to[0] = '\0';	/* nothing yet */
	      figure_out_addressee((char *) buffer +3,
				   current_header->to);
	    }
	    else if (first_word(buffer, "Status:")) {
	      remove_first_word(buffer);
	      strncpy(current_header->mailx_status, buffer, WLEN-1);
	      current_header->mailx_status[WLEN-1] ='\0';

	      c = index(current_header->mailx_status, '\n');
	      if (c != NULL)
		*c = '\0';
	      c = index(current_header->mailx_status, '\r');
	      if (c != NULL)
		*c = '\0';
	      remove_possible_trailing_spaces(current_header->mailx_status);

	      /* Okay readjust the status. If there's an 'R', message
	       * is read; if there is no 'R' but there is an 'O', message
	       * is unread. In any case it isn't new because a new message
	       * wouldn't have a Status: header.
	       */
	      if (index(current_header->mailx_status, 'R') != NULL)
		current_header->status &= ~(NEW | UNREAD);
	      else if (index(current_header->mailx_status, 'O') != NULL) {
		current_header->status &= ~NEW;
		current_header->status |= UNREAD;
	      }
	    }

	    else if (buffer[0] == LINE_FEED || buffer[0] == '\0') {
	      if (in_header) {
	        in_header = 0;	/* in body of message! */
	        fix_date(current_header);
	      }
	    }
	    else if (in_header) {
	       if ((!whitespace(buffer[0])) && index(buffer, ':') == NULL) {
	        in_header = 0;	/* in body of message! */
	        fix_date(current_header);
	      }
	    }
	    else if (in_to_list == TRUE) {
	      if (whitespace(buffer[0]))
	        figure_out_addressee(buffer, current_header->to);
	      else in_to_list = FALSE;
	    }
	  }
	  if (!in_header && first_word(buffer, START_ENCODE))
	    current_header->encrypted = 1;
	  if (!in_header && first_word(buffer, "Forwarded "))
	    in_header = 1;
	  fbytes += (long) line_bytes;
	}

	if (count)
	  headers[count-1]->lines = line + 1;

	if (folder_type == SPOOL) {
	  unlock();	/* remove lock file! */
	  if ((ferror(mailfile)) || (fclose(mailfile) == EOF)) {
	      err = errno;
	      Write_to_screen("\n\rClose on folder %s failed!!\n\r", 1,
			      cur_folder);
	      Write_to_screen("** %s - %s. **\n\r", 2,
			      error_name(err), error_description(err));
	      dprint(1, (debugfile, "Can't close on folder %s!!\n",
			 cur_folder));
	      rm_temps_exit();
	  }
	  if ((ferror(temp)) || (fclose(temp) == EOF)) {
	      err = errno;
	      Write_to_screen("\n\rClose on tempfile %s failed!!\n\r", 1,
			      cur_tempfolder);
	      Write_to_screen("** %s - %s. **\n\r", 2,
			      error_name(err), error_description(err));
	      dprint(1, (debugfile, "Can't close on tempfile %s!!\n",
			 cur_tempfolder));
	      rm_temps_exit();
	  }
	  /* sanity check on append - is resulting temp file longer??? */
	  if ( bytes(cur_tempfolder) != mailfile_size) {
	     Write_to_screen(
	       "\n\rnewmbox - length of mbox. != spool mailbox length!!\n\r",
		0);
	    dprint(0, (debugfile, "newmbox - mbox. != spool mail length"));
	    rm_temps_exit();
	  }
	  if ((mailfile = fopen(cur_tempfolder,"rb")) == NULL) {
	    err = errno;
	    MoveCursor(LINES,0);
	    Raw(OFF);
	    Write_to_screen(
		   "\n\rAugh! Couldn't reopen %s as temp file.\n\r",
	           1, cur_tempfolder);
	    Write_to_screen("** %s - %s. **\n\r", 2, error_name(err),
		   error_description(err));
	    dprint(1, (debugfile,
		  "Error: Reopening %s as temp file failed!  errno %s (%s)\n",
	           cur_tempfolder, error_name(errno), "read_headers"));
	    leave();
	  }
	}
	else
          rewind(mailfile);

#ifdef OS2
	fcntl(fileno(mailfile), F_SETFD, 1);
#endif

	/* Sort folder *before* we establish the current message, so that
	 * the current message is based on the post-sort order.
	 * Note that we have to set the global variable message_count
	 * before the sort for the sort to correctly keep the correct
	 * current message if we are only adding new messages here. */

	message_count = count;
	sort_mailbox(count, 1);

	/* Now lets figure what the current message should be.
	 * If we are only reading in newly added messages from a mailfile
	 * that already had some messages, current should remain the same.
	 * If we have a folder of no messages, current should be zero.
	 * Otherwise, if we have point_to_new on then the current message
	 * is the first message of status NEW if there is one.
	 * If we don't have point_to_new on or if there are no messages of
	 * of status NEW, then the current message is the first message.
	 */
	if(!(add_new_only && current != 0)) {
	  if(count == 0)
	    current = 0;
	  else {
	    current = 1;
	    if (point_to_new) {
	      for(another_count = 0; another_count < count; another_count++) {
		if(ison(headers[another_count]->status, NEW)) {
		  current = another_count+1;
		  break;	/* first one found give up */
		}
	      }
	    }
	  }
	}
        get_page(current);
	return(count);
}
