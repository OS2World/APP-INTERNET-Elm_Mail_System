
static char rcsid[] = "@(#)$Id: read_rc.c,v 4.1.1.1 90/06/21 23:28:49 syd Exp $";

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
 * $Log:	read_rc.c,v $
 * Revision 4.1.1.1  90/06/21  23:28:49  syd
 * Add apollo check for //node
 * From: Russ Johnson
 *
 * Revision 4.1  90/04/28  22:43:46  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This file contains programs to allow the user to have a .elm/elmrc file
    in their home directory containing any of the following:

	fullname= <username string>
	maildir = <directory>
	tmpdir  = <directory>
	sentmail = <file>
	editor  = <editor>
	receviedmail= <file>
	calendar= <calendar file name>
	shell   = <shell>
	print   = <print command>
	weedout = <list of headers to weed out>
	prefix  = <copied message prefix string>
	pager   = <command to use for displaying messages>

	escape  = <single character escape, default = '~' >

--
	signature = <.signature file for all outbound mail>
OR:
	localsignature = <.signature file for local mail>
	remotesignature = <.signature file for non-local mail>
--

	bounceback= <hop count threshold, or zero to disable>
	timeout = <seconds for main menu timeout or zero to disable>
	userlevel = <0=amateur, 1=okay, 2 or greater = expert!>

	sortby  = <sent, received, from, size, subject, mailbox, status>

	alternatives = <list of addresses that forward to us>

    and/or the logical arguments:

	autocopy    [on|off]
	askcc	    [on|off]
	copy        [on|off]
	resolve     [on|off]
	weed        [on|off]
	noheader    [on|off]
	titles      [on|off]
	savebyname  [on|off]
	forcename   [on|off]
	movepage    [on|off]
	pointnew    [on|off]
	hpkeypad    [on|off]
	hpsoftkeys  [on|off]
	alwayskeep [on|off]
	alwaysstore [on|off]
	alwaysdel   [on|off]
	arrow	    [on|off]
	menus	    [on|off]
	forms	    [on|off]
	warnings    [on|off]
	names	    [on|off]
	ask	    [on|off]
	keepempty   [on|off]
	promptafter[on|off]
	sigdashes   [on|off]


    Lines starting with '#' are considered comments and are not checked
    any further!

    Modified 10/85 to know about "Environment" variables..
    Modified 12/85 for the 'prefix' option
    Modified  2/86 for the new 3.3 flags
    Modified  8/86 (was I supposed to be keeping this up to date?)
**/

#include <ctype.h>
#include "headers.h"

#ifdef BSD
#undef tolower
#endif

extern char *pmalloc();
extern int errno;

char  *error_name(), *error_description(), *shift_lower(),
      *strtok(), *getenv(), *strcpy();
void  exit();

#define  metachar(c)	(c == '+' || c == '%' || c == '=')

#define ASSIGNMENT      0
#define WEEDOUT		1
#define ALTERNATIVES	2

read_rc_file()
{
	/** this routine does all the actual work of reading in the
	    .rc file... **/

	FILE *file;
	char buffer[SLEN], filename[SLEN], *cp, word1[SLEN], word2[SLEN];
	int  i, ch, len,
	  rc_has_recvdmail = 0, rc_has_sentmail = 0, lineno = 0, errors = 0;

	/* Establish some defaults in case elmrc is incomplete or not there.
	 * Defaults for other elmrc options were established in their
	 * declaration - in elm.h.  And defaults for sent_mail and recvd_mail
	 * are established after the elmrc is read in since these default
	 * are based on the folders directory name, which may be given
	 * in the emrc.
	 * Also establish alternative_editor here since it is based on
	 * the default editor and not on the one that might be given in the
	 * elmrc.
	 */

	default_weedlist();

	alternative_addresses = NULL; 	/* none yet! */

	raw_local_signature[0] = raw_remote_signature[0] =
		local_signature[0] = remote_signature[0] = '\0';
		/* no defaults for those */

	strcpy(raw_shell,((cp = getenv("SHELL")) == NULL)? default_shell : cp);
	strcpy(shell, raw_shell);

	strcpy(raw_pager,((cp = getenv("PAGER")) == NULL)? default_pager : cp);
	strcpy(pager, raw_pager);

	strcpy(raw_editor,((cp = getenv("EDITOR")) == NULL)? default_editor:cp);

#ifdef OS2
        strcpy(temp_dir, tempdir);
#else
	strcpy(temp_dir,((cp = getenv("TMP")) == NULL)? default_temp:cp);
#endif
	if (temp_dir[strlen (temp_dir)-1] != '/')
                strcat(temp_dir, "/");

	strcpy(alternative_editor, raw_editor);
	strcpy(editor, raw_editor);

	strcpy(raw_printout, default_printout);
	strcpy(printout, raw_printout);

	sprintf(raw_folders, "%s/%s", home, default_folders);
	strcpy(folders, raw_folders);

	sprintf(raw_calendar_file, "%s/%s", home, dflt_calendar_file);
	strcpy(calendar_file, raw_calendar_file);

	/* see if the user has a $HOME/.elm directory */
	sprintf(filename, "%s/%s", home, dotelm);

	if (access(filename, 00) == -1) {
	  if(batch_only)  {
	    printf("\n\rNotice:\
\n\rThis version of ELM requires the use of a .elm directory to store your\
\n\relmrc and alias files. I'd like to create the directory .elm for you\
\n\rand set it up, but I can't in \"batch mode\".\
\n\rPlease run ELM in \"normal mode\" first.\n\r");
	    exit(0);
	  }

	  printf("\n\rNotice:\
\n\rThis version of ELM requires the use of a .elm directory in your home\
\n\rdirectory to store your elmrc and alias files. Shall I create the\
\n\rdirectory .elm for you and set it up (y/n)? y%c",BACKSPACE);

	  fflush(stdout);
	  ch=getchar();
	  if (ch == 'n' || ch == 'N') {
	    printf("No.\n\rVery well. I won't create it.\
	    \n\rBut, you may run into difficulties later.\n\r");
	    sleep(4);
	  }
	  else {
	    printf("Yes.\n\rGreat! I'll do it now.\n\r");
	    create_new_elmdir();
	  }
	}

	/* Look for the elmrc file */
	sprintf(filename,"%s/%s", home, elmrcfile);

	if ((file = fopen(filename, "r")) == NULL) {
	  dprint(2,(debugfile,"Warning:User has no \".elm/elmrc\" file\n\n"));

	  /* look for old-style .elmrc file in $HOME */
	  sprintf(filename, "%s/%src", home, dotelm);
	  if (access(filename, 00) != -1) {
	    move_old_files_to_new();

	    /* try to open elmrc file again */
	    sprintf(filename,"%s/%s", home, elmrcfile);
	    if((file = fopen(filename, "r")) == NULL) {
	      dprint(2, (debugfile,
		"Warning: could not open new \".elm/elmrc\" file.\n"));
	      dprint(2, (debugfile, "** %s - %s **\n", error_name(errno),
		   error_description(errno)));
	      printf("Warning: could not open new \".elm/elmrc\" file!");
	      printf(" Using default parameters.\n\r");
	      sleep(4);
	    }
	  }
	}

	if(file) {
	  int   last = ASSIGNMENT, this;

	  while (fgets(buffer, SLEN, file) != NULL) {
	    lineno++;
	    no_ret(buffer);	 	/* remove return */
	    if (buffer[0] == '#'        /* comment       */
	     || strlen(buffer) < 2)     /* empty line    */
	      continue;

	    this = ASSIGNMENT;

	    if(breakup(buffer, word1, word2) == -1)
	      continue;		/* word2 is null - let default value stand */

	    strcpy(word1, shift_lower(word1));	/* to lower case */

	    if (equal(word1,"maildir") || equal(word1,"folders")) {
	      strcpy(raw_folders, word2);
	      expand_env(folders, word2);
	    }
	    else if (equal(word1,"tmpdir")) {
	      expand_env(temp_dir, word2);
	      if (temp_dir[strlen (temp_dir)-1] != '/')
                strcat(temp_dir, "/");
	    }
	    else if (equal(word1, "fullname") || equal(word1,"username") ||
		     equal(word1, "name")) {
	      strcpy(full_username, word2);
	    }
	    else if (equal(word1, "prefix")) {
	      for (i=0, len = strlen(word2); i < len; i++)
		prefixchars[i] = (word2[i] == '_' ? ' ' : word2[i]);
	      prefixchars[i] = '\0';
	    }
	    else if (equal(word1, "shell")) {
	      strcpy(raw_shell, word2);
	      expand_env(shell, word2);
	    }
	    else if (equal(word1, "sort") || equal(word1, "sortby")) {
	      strcpy(word2, shift_lower(word2));
	      if (equal(word2, "sent"))
		 sortby = SENT_DATE;
	      else if (equal(word2, "received") || equal(word2,"recieved"))
		 sortby = RECEIVED_DATE;
	      else if (equal(word2, "from") || equal(word2, "sender"))
		 sortby = SENDER;
	      else if (equal(word2, "size") || equal(word2, "lines"))
		sortby = SIZE;
	      else if (equal(word2, "subject"))
		sortby = SUBJECT;
	      else if (equal(word2, "mailbox") || equal(word2, "folder"))
		sortby = MAILBOX_ORDER;
	      else if (equal(word2, "status"))
		sortby = STATUS;
	      else if (equal(word2, "reverse-sent") || equal(word2,"rev-sent"))
		 sortby = - SENT_DATE;
	      else if (strncmp(word2, "reverse-rec",11) == 0 ||
		       strncmp(word2,"rev-rec",7) == 0)
		 sortby = - RECEIVED_DATE;
	      else if (equal(word2, "reverse-from") || equal(word2, "rev-from")
		    || equal(word2,"reverse-sender")||equal(word2,"rev-sender"))
		 sortby = - SENDER;
	      else if (equal(word2, "reverse-size") || equal(word2, "rev-size")
		    || equal(word2, "reverse-lines")|| equal(word2,"rev-lines"))
		sortby = - SIZE;
	      else if (equal(word2, "reverse-subject") ||
		       equal(word2, "rev-subject"))
		sortby = - SUBJECT;
	      else if (equal(word2, "reverse-mailbox") ||
		       equal(word2, "rev-mailbox") ||
		       equal(word2, "reverse-folder") ||
		       equal(word2, "rev-folder"))
		sortby = - MAILBOX_ORDER;
	      else if (equal(word2, "reverse-status") ||
		       equal(word2, "rev-status"))
		sortby = - STATUS;
	      else {
		errors++;
		printf(
      "I can't understand sort key %s in line %d in your \".elm/elmrc\" file\n",
		     word2, lineno);
	        continue;
	      }
	    }
	    else if (equal (word1, "receivedmail") || equal(word1, "mailbox")) {
	      /* the last is an old name of this option - here for
	       * compatibility in case the user has never written out
	       * a new elmrc while in elm since the name change.
	       */
	      rc_has_recvdmail = TRUE;
	      strcpy(raw_recvdmail, word2);
	      expand_env(recvd_mail, word2);
	    }
	    else if (equal(word1, "editor") || equal(word1,"mailedit")) {
	      strcpy(raw_editor, word2);
	      expand_env(editor, word2);
	    }
	    else if (equal(word1, "sentmail") ||
		equal(word1, "savemail") || equal(word1, "saveto")) {
	      /* the last two were old names of this option - here for
	       * compatibility in case the user has never written out
	       * a new elmrc while in elm since the name change.
	       */
	      rc_has_sentmail = TRUE;
	      strcpy(raw_sentmail, word2);
	      expand_env(sent_mail, word2);
	    }
	    else if (equal(word1, "calendar")) {
	      strcpy(raw_calendar_file, word2);
	      expand_env(calendar_file, word2);
	    }
	    else if (equal(word1, "print") || equal(word1, "printmail")) {
	      strcpy(raw_printout, word2);
	      expand_env(printout, word2);
	    }
	    else if (equal(word1, "pager") || equal(word1, "page")) {
	      strcpy(raw_pager, word2);
	      expand_env(pager, word2);
	      if (equal(pager,"builtin+") || equal(pager,"internal+"))
		clear_pages++;
	    }
	    else if (equal(word1, "signature")) {
	      if (equal(shift_lower(word2), "on") ||
		  equal(shift_lower(word2), "off")) {
		errors++;
		printf(
	"\"signature\" used in obsolete way in .elm/elmrc file. Ignored!\n\r");
		printf(
 "\t(Signature should specify the filename to use rather than on/off.)\n\r\n");
	      }
	      else {
		strcpy(raw_local_signature, word2);
		strcpy(raw_remote_signature, raw_local_signature);
		expand_env(local_signature, word2);
		strcpy(remote_signature, local_signature);
	      }
	    }
	    else if (equal(word1, "localsignature")) {
	      strcpy(raw_local_signature, word2);
	      expand_env(local_signature, word2);
	    }
	    else if (equal(word1, "remotesignature")) {
	      strcpy(raw_remote_signature, word2);
	      expand_env(remote_signature, word2);
	    }
	    else if (equal(word1, "sigdashes")) {
	      sig_dashes = is_it_on(word2);
	    }
	    else if (equal(word1, "escape")) {
	      escape_char = word2[0];
	    }
	    else if (equal(word1, "attribution")) {
	      strcpy(attribution, word2);
	    }
	    else if (equal(word1, "autocopy")) {
	      auto_copy = is_it_on(word2);
	    }
	    else if (equal(word1, "copy") || equal(word1, "auto_cc")) {
	      auto_cc = is_it_on(word2);
	    }
	    else if (equal(word1, "names")) {
	      names_only = is_it_on(word2);
	    }
	    else if (equal(word1, "resolve")) {
	      resolve_mode = is_it_on(word2);
	    }
	    else if (equal(word1, "weed")) {
	      filter = is_it_on(word2);
	    }
	    else if (equal(word1, "noheader")) {
	      noheader = is_it_on(word2);
	    }
	    else if (equal(word1, "titles")) {
	      title_messages = is_it_on(word2);
	    }
	    else if (equal(word1, "savebyname") || equal(word1, "savename")) {
	      save_by_name = is_it_on(word2);
	    }
	    else if (equal(word1, "forcename")) {
	      force_name = is_it_on(word2);
	    }
	    else if (equal(word1, "movepage") || equal(word1, "page") ||
		     equal(word1, "movewhenpaged")) {
	      move_when_paged = is_it_on(word2);
	    }
	    else if (equal(word1, "pointnew") || equal(word1, "pointtonew")) {
	      point_to_new = is_it_on(word2);
	    }
	    else if (equal(word1, "keypad") || equal(word1, "hpkeypad")) {
	      hp_terminal = is_it_on(word2);
	    }
	    else if (equal(word1, "softkeys") || equal(word1, "hpsoftkeys")) {
	      if (hp_softkeys = is_it_on(word2))
		hp_terminal = TRUE;	/* must be set also! */
	    }
	    else if (equal(word1, "arrow")) {
	      arrow_cursor += is_it_on(word2);	/* may have been set already */
	    }
	    else if (strncmp(word1, "form", 4) == 0) {
	      allow_forms = (is_it_on(word2)? MAYBE : NO);
	    }
	    else if (equal(word1, "promptafter")) {
	      prompt_after_pager = is_it_on(word2);
	    }
	    else if (strncmp(word1, "menu", 4) == 0) {
	      /* if not turned off by -m cmd line arg,
	       * obey elmrc file setting */
	      if(mini_menu)
		mini_menu = is_it_on(word2);
	    }
	    else if (strncmp(word1, "warning", 7) == 0) {
	      warnings = is_it_on(word2);
	    }
	    else if (equal(word1, "alwaysleave")) {
	      /* this is an old option - here for
	       * compatibility in case the user has never written out
	       * a new elmrc while in elm since the split of
	       * alwaysleave into alwayskeep and alwaysstore
	       */
	      always_keep = is_it_on(word2);
	      always_store = !is_it_on(word2);
	    }
	    else if (equal(word1, "alwayskeep")) {
	      always_keep = is_it_on(word2);
	    }
	    else if (equal(word1, "alwaysstore") || equal(word1, "store")) {
	      always_store = is_it_on(word2);
	    }
	    else if (equal(word1, "alwaysdelete") || equal(word1, "delete")) {
	      always_del = is_it_on(word2);
	    }
	    else if (equal(word1, "askcc") || equal(word1, "cc")) {
	      prompt_for_cc = is_it_on(word2);
	    }
	    else if (equal(word1, "ask") || equal(word1, "question")) {
	      question_me = is_it_on(word2);
	    }
	    else if (equal(word1, "keep") || equal(word1, "keepempty")) {
	      keep_empty_files = is_it_on(word2);
	    }
	    else if (equal(word1, "bounce") || equal(word1, "bounceback")) {
	      bounceback = atoi(word2);
	      if (bounceback > MAX_HOPS) {
		errors++;
		printf(
	"Warning: bounceback is set to greater than %d (max-hops). Ignored.\n",
			 MAX_HOPS);
		bounceback = 0;
	      }
	    }
	    else if (equal(word1, "userlevel")) {
	      user_level = atoi(word2);
	    }
	    else if (equal(word1, "timeout")) {
	      timeout = atoi(word2);
	      if (timeout < 10) {
		errors++;
		printf(
	     "Warning: timeout is set to less than 10 seconds. Ignored.\n");
		timeout = 0;
	      }
	    }
	    else if (equal(word1, "weedout")) {
	      weedout(word2);
	      this = WEEDOUT;
	    }
	    else if (equal(word1, "alternatives")) {
	      alternatives(word2);
	      this = ALTERNATIVES;
	    }
	    else if (last == WEEDOUT) {
	      weedout(buffer);
	      this = WEEDOUT;
	    }
	    else if (last == ALTERNATIVES) {
	      alternatives(buffer);
	      this = ALTERNATIVES;
	    }
	    else {
	      errors++;
	      printf(
	     "I can't understand line %d in your \".elm/elmrc\" file:\n> %s\n",
		     lineno, buffer);
	    }
	    last = this;
	  }
	  /* sleep two seconds for each error and then some so user
	   * can read them before screen is cleared */
	  if(errors)
	    sleep((errors * 2) + 2);

          fclose(file);
	}

	/* see if the user has a folders directory */
	if (access(folders, 00) == -1) {
	  if(batch_only)  {
	    printf("\n\rNotice:\
\n\rELM requires the use of a folders directory to store your mail folders in.\
\n\rI'd like to create the directory %s for you,\
\n\rbut I can't in \"batch mode\". Please run ELM in \"normal mode\" first.\
\n\r", folders);
	    exit(0);
	  }

	  printf("\n\rNotice:\
\n\rELM requires the use of a folders directory to store your mail folders in.\
\n\rShall I create the directory %s for you (y/n)? y%c", folders, BACKSPACE);

	  fflush(stdout);
	  ch=getchar();
	  if (ch == 'n' || ch == 'N') {
	    printf("No.\n\rVery well. I won't create it.\
	    \n\rBut, you may run into difficulties later.\n\r");
	    sleep(4);
	  }
	  else {
	    printf("Yes.\n\rGreat! I'll do it now.\n\r");
	    create_new_folders();
	  }
	}

	/* If recvd_mail or sent_mail havent't yet been established in
	 * the elmrc, establish them from their defaults.
	 * Then if they begin with a metacharacter, replace it with the
	 * folders directory name.
	 */
	if(!rc_has_recvdmail) {
	  strcpy(raw_recvdmail, default_recvdmail);
	  strcpy(recvd_mail, raw_recvdmail);
	}
	if(metachar(recvd_mail[0])) {
	  strcpy(buffer, &recvd_mail[1]);
	  sprintf(recvd_mail, "%s/%s", folders, buffer);
	}

	if(!rc_has_sentmail) {
	  sprintf(raw_sentmail, default_sentmail);
	  sprintf(sent_mail, default_sentmail);
	}
	if(metachar(sent_mail[0])) {
	  strcpy(buffer, &sent_mail[1]);
	  sprintf(sent_mail, "%s/%s", folders, buffer);
	}

	if (debug > 10) 	/** only do this if we REALLY want debug! **/
	  dump_rc_results();

}

weedout(string)
char *string;
{
	/** This routine is called with a list of headers to weed out.   **/

	char *strptr, *header;
	register int i, len;

	strptr = string;

	while ((header = strtok(strptr, "\t ,\"'")) != NULL) {
	  if (strlen(header) > 0) {
	    if (! strcmp(header, "*end-of-user-headers*")) break;
	    if (weedcount > MAX_IN_WEEDLIST) {
	      printf("Too many weed headers!  Leaving...\n");
	      exit(1);
	    }
	    if ((weedlist[weedcount] = pmalloc(strlen(header) + 1))
		== NULL) {
	      printf("Too many weed headers! Out of memory!  Leaving...\n");
	      exit(1);
	    }

	    for (i=0, len = strlen(header); i< len; i++)
	      if (header[i] == '_') header[i] = ' ';

	    strcpy(weedlist[weedcount], header);
	    weedcount++;
	  }
	  strptr = NULL;
	}
}

alternatives(string)
char *string;
{
	/** This routine is called with a list of alternative addresses
	    that you may receive mail from (forwarded) **/

	char *strptr, *address;
	struct addr_rec *current_record, *previous_record;

	previous_record = alternative_addresses;	/* start 'er up! */
	/* move to the END of the alternative addresses list */

	if (previous_record != NULL)
	  while (previous_record->next != NULL)
	    previous_record = previous_record->next;

	strptr = (char *) string;

	while ((address = strtok(strptr, "\t ,\"'")) != NULL) {
	  if (previous_record == NULL) {
	    previous_record = (struct addr_rec *) pmalloc(sizeof
		*alternative_addresses);

	    strcpy(previous_record->address, address);
	    previous_record->next = NULL;
	    alternative_addresses = previous_record;
	  }
	  else {
	    current_record = (struct addr_rec *) pmalloc(sizeof
		*alternative_addresses);

	    strcpy(current_record->address, address);
	    current_record->next = NULL;
	    previous_record->next = current_record;
	    previous_record = current_record;
	  }
	  strptr = (char *) NULL;
	}
}

default_weedlist()
{
	/** Install the default headers to weed out!  Many gracious
	    thanks to John Lebovitz for this dynamic method of
	    allocation!
	**/

	static char *default_list[] = { ">From", "In-Reply-To:",
		       "References:", "Newsgroups:", "Received:",
		       "Apparently-To:", "Message-Id:", "Content-Type:",
		       "From", "X-Mailer:", "Status:",
		       "*end-of-defaults*", NULL
		     };

	for (weedcount = 0; default_list[weedcount] != (char *) 0;weedcount++){
	  if ((weedlist[weedcount] =
	      pmalloc(strlen(default_list[weedcount]) + 1)) == NULL) {
	    printf("\n\rNot enough memory for default weedlist. Leaving.\n\r");
	    leave(1);
	  }
	  strcpy(weedlist[weedcount], default_list[weedcount]);
	}
}

int
matches_weedlist(buffer)
char *buffer;
{
	/** returns true iff the first 'n' characters of 'buffer'
	    match an entry of the weedlist **/

	register int i;

	for (i=0;i < weedcount; i++)
	  if (strncmp(buffer, weedlist[i], strlen(weedlist[i])) == 0)
	    return(1);

	return(0);
}

int
breakup(buffer, word1, word2)
char *buffer, *word1, *word2;
{
	/** This routine breaks buffer down into word1, word2 where
	    word1 is alpha characters only, and there is an equal
	    sign delimiting the two...
		alpha = beta
	    For lines with more than one 'rhs', word2 is set to the
	    entire string.
	    Return -1 if word 2 is of zero length, else 0.
	**/

	register int i;

	for (i=0;buffer[i] != '\0' && ok_rc_char(buffer[i]); i++)
	  if (buffer[i] == '_')
	    word1[i] = '-';
	  else if (isupper(buffer[i]))
	    word1[i] = tolower(buffer[i]);
	  else
	    word1[i] = buffer[i];

	word1[i++] = '\0';	/* that's the first word! */

	/** spaces before equal sign? **/

	while (whitespace(buffer[i])) i++;
	if (buffer[i] == '=') i++;

	/** spaces after equal sign? **/

	while (whitespace(buffer[i])) i++;

	if (buffer[i] != '\0')
	  strcpy(word2, (char *) (buffer + i));
	else
	  word2[0] = '\0';

	/* remove trailing spaces from word2! */
	i = strlen(word2) - 1;
	while(i && (whitespace(word2[i]) || word2[i] == '\n'))
	  word2[i--] = '\0';

	return(*word2 == '\0' ? -1 : 0 );

}

expand_env(dest, buffer)
char *dest, *buffer;
{
	/** expand possible metacharacters in buffer and then copy
	    to dest...

	    BEWARE!! Because strtok() is used on buffer, buffer may be changed.

	    This routine knows about "~" being the home directory,
	    and "$xxx" being an environment variable.
	**/

	char  *word, *string, next_word[SLEN];

	if (buffer[0] == '/') {
	  dest[0] = '/';
	  dest[1] = '\0';
/* Added for Apollos - handle //node */
	  if (buffer[1] == '/') {
	    dest[1] = '/';
	    dest[2] = '\0';
	  }
	}
	else
	  dest[0] = '\0';

	string = (char *) buffer;

	while ((word = strtok(string, "/")) != NULL) {
	  if (word[0] == '$') {
	    next_word[0] = '\0';
	    if (getenv((char *) (word + 1)) != NULL)
	    strcpy(next_word, getenv((char *) (word + 1)));
	    if (strlen(next_word) == 0)
	      leave(printf("\n\rCan't expand environment variable '%s'.\n\r",
		    word));
	  }
	  else if (word[0] == '~' && word[1] == '\0')
	    strcpy(next_word, home);
	  else
	    strcpy(next_word, word);

	  sprintf(dest, "%s%s%s", dest,
		 (strlen(dest) > 0 && lastch(dest) != '/' ? "/":""),
		 next_word);

	  string = (char *) NULL;
	}
}

#define on_off(s)	(s == 1? "ON " : "OFF")
dump_rc_results()
{

	register int i, len;

	fprintf(debugfile, "folders = %s ", folders);
	fprintf(debugfile, "temp_dir = %s ", temp_dir);
	fprintf(debugfile, "recvd_mail = %s ", recvd_mail);
	fprintf(debugfile, "editor = %s\n", editor);
	fprintf(debugfile, "printout = %s ", printout);
	fprintf(debugfile, "sent_mail = %s ", sent_mail);
	fprintf(debugfile, "calendar_file = %s\n", calendar_file);
	fprintf(debugfile, "prefixchars = %s ", prefixchars);
	fprintf(debugfile, "shell = %s ", shell);
	fprintf(debugfile, "pager = %s\n", pager);
	fprintf(debugfile, "\n");
	fprintf(debugfile, "escape = %c\n", escape_char);
	fprintf(debugfile, "\n");

	fprintf(debugfile, "mini_menu    = %s ", on_off(mini_menu));
	fprintf(debugfile, "filter_hdrs  = %s ", on_off(filter));
	fprintf(debugfile, "auto_copy      = %s\n", on_off(auto_copy));

	fprintf(debugfile, "resolve_mode   = %s ", on_off(resolve_mode));
	fprintf(debugfile, "auto_save_copy = %s ", on_off(auto_cc));
	fprintf(debugfile, "noheader     = %s\n", on_off(noheader));

	fprintf(debugfile, "title_msgs   = %s ", on_off(title_messages));
	fprintf(debugfile, "hp_terminal    = %s ", on_off(hp_terminal));
	fprintf(debugfile, "hp_softkeys    = %s\n", on_off(hp_softkeys));

	fprintf(debugfile, "save_by_name = %s ", on_off(save_by_name));
	fprintf(debugfile, "force_name = %s\n", on_off(force_name));

	fprintf(debugfile, "move_paged   = %s ", on_off(move_when_paged));
	fprintf(debugfile, "point_to_new   = %s ", on_off(point_to_new));
	fprintf(debugfile, "prompt_after_pager   = %s ",
	    on_off(prompt_after_pager));
	fprintf(debugfile, "bounceback     = %s\n", on_off(bounceback));

	fprintf(debugfile, "always_keep = %s ", on_off(always_keep));
	fprintf(debugfile, "always_store = %s ", on_off(always_store));
	fprintf(debugfile, "always_delete  = %s ", on_off(always_del));
	fprintf(debugfile, "arrow_cursor   = %s ", on_off(arrow_cursor));
	fprintf(debugfile, "names        = %s\n", on_off(names_only));

	fprintf(debugfile, "warnings     = %s ", on_off(warnings));
	fprintf(debugfile, "question_me    = %s ", on_off(question_me));
	fprintf(debugfile, "keep_nil_files = %s\n\n",
			   on_off(keep_empty_files));

	fprintf(debugfile, "local_signature  = %s\n", local_signature);
	fprintf(debugfile, "remote_signature = %s\n", remote_signature);
	fprintf(debugfile, "sig_dashes = %s\n", on_off(sig_dashes));

	fprintf(debugfile, "Userlevel is set to %s user: %d\n",
		user_level == 0 ? "beginning" :
		 user_level > 1 ? "expert" : "intermediate", user_level);

	fprintf(debugfile, "\nAnd we're skipping the following headers:\n\t");

	for (len=8, i=0; i < weedcount; i++) {
	  if (weedlist[i][0] == '*') continue;	/* skip '*end-of-defaults*' */
	  if (len + strlen(weedlist[i]) > 80) {
	    fprintf(debugfile, " \n\t");
	    len = 8;
	  }
	  fprintf(debugfile, "%s  ", weedlist[i]);
	  len += strlen(weedlist[i]) + 3;
	}

	fprintf(debugfile, "\n\n");
}

is_it_on(word)
char *word;
{
	/** Returns TRUE if the specified word is either 'ON', 'YES'
	    or 'TRUE', and FALSE otherwise.   We explicitly translate
	    to lowercase here to ensure that we have the fastest
	    routine possible - we really DON'T want to have this take
	    a long time or our startup will be major pain each time.
	**/

	static char mybuffer[NLEN];
	register int i, j;

	for (i=0, j=0; word[i] != '\0'; i++)
	  mybuffer[j++] = isupper(word[i]) ? tolower(word[i]) : word[i];
	mybuffer[j] = '\0';

	return(  (strncmp(mybuffer, "on",   2) == 0) ||
		 (strncmp(mybuffer, "yes",  3) == 0) ||
		 (strncmp(mybuffer, "true", 4) == 0)
	      );
}
