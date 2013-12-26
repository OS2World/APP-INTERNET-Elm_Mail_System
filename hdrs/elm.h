
/* $Id: elm.h,v 4.1.1.1 90/10/24 15:31:24 syd Exp $ */

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
 * $Log:	elm.h,v $
 * Revision 4.1.1.1  90/10/24  15:31:24  syd
 * Remove variables no longer used
 * From: W. David Higgins
 *
 * Revision 4.1  90/04/28  22:42:08  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/**  Main header file for ELM mail system.  **/


#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "../hdrs/curses.h"
#include "../hdrs/defs.h"

#if defined(BSD) || defined(OS2)
#include <setjmp.h>
#endif

/******** static character string containing the version number  *******/

static char ident[] = { WHAT_STRING };

/******** and another string for the copyright notice            ********/

static char copyright[] = {
		"@(#)          (C) Copyright 1986, 1987, Dave Taylor\n@(#)          (C) Copyright 1988, 1989, 1990, The Usenet Community Trust\n" };

/******** global variables accessable by all pieces of the program *******/

int check_size = 0;		/* don't start mailer if no mail */
int current = 0;		/* current message number  */
int header_page = 0;     	/* current header page     */
int last_header_page = -1;     	/* last header page        */
int message_count = 0;		/* max message number      */
int headers_per_page;		/* number of headers/page  */
int original_umask = 0;		/* original umask, for restore before subshell */
int sendmail_verbose = 0;       /* Extended mail debugging */
int no_save = 0;		/* Do not save outgoing mail */
int mmdf_strict = 0;		/* Strict MMDF mailbox scanning */
char cur_folder[SLEN];          /* name of current folder */
char cur_tempfolder[SLEN];      /* name of temp folder open for a mailbox */
char defaultfile[SLEN];         /* name of default folder */
char temp_dir[SLEN] = {0};      /* name of temp directory */
char hostname[SLEN];            /* name of machine we're on*/
char hostdomain[SLEN];          /* name of domain we're in */
char hostfullname[SLEN];        /* name of FQDN we're in */
char hostfromname[SLEN];        /* name of FQDN we pretend to be in */
char username[SLEN];            /* return address name!    */
char full_username[SLEN];       /* Full username - gecos   */
char home[SLEN];                /* home directory of user  */
char folders[SLEN];             /* folder home directory   */
char raw_folders[SLEN];         /* unexpanded folder home directory   */
char recvd_mail[SLEN];          /* folder for storing received mail     */
char raw_recvdmail[SLEN];       /* unexpanded recvd_mail name */
char editor[SLEN];              /* editor for outgoing mail*/
char raw_editor[SLEN];          /* unexpanded editor for outgoing mail*/
char alternative_editor[SLEN];  /* alternative editor...   */
char printout[SLEN];            /* how to print messages   */
char raw_printout[SLEN];        /* unexpanded how to print messages   */
char sent_mail[SLEN];           /* name of file to save copies to */
char raw_sentmail[SLEN];        /* unexpanded name of file to save to */
char calendar_file[SLEN];       /* name of file for clndr  */
char raw_calendar_file[SLEN];   /* unexpanded name of file for clndr  */
char attribution[SLEN];         /* attribution string for replies     */
char prefixchars[SLEN] = "> ";	/* prefix char(s) for msgs */
char shell[SLEN];               /* current system shell    */
char raw_shell[SLEN];           /* unexpanded current system shell    */
char pager[SLEN];               /* what pager to use       */
char raw_pager[SLEN];           /* unexpanded what pager to use       */
char batch_subject[SLEN];       /* subject buffer for batchmail */
char local_signature[SLEN];     /* local msg signature file     */
char raw_local_signature[SLEN]; /* unexpanded local msg signature file     */
char remote_signature[SLEN];    /* remote msg signature file    */
char raw_remote_signature[SLEN];/* unexpanded remote msg signature file    */
char version_buff[SLEN];        /* version buffer */

char backspace,			/* the current backspace char */
     escape_char = TILDE_ESCAPE,/* '~' or something else..    */
     kill_line;			/* the current kill-line char */

char up[SHORT], down[SHORT],	/* cursor control seq's    */
     left[SHORT], right[SHORT];
int  cursor_control = FALSE;	/* cursor control avail?   */

int  has_highlighting = FALSE;	/* highlighting available? */

char *weedlist[MAX_IN_WEEDLIST];
int  weedcount;

int allow_forms = NO;		/* flag: are AT&T Mail forms okay?  */
int mini_menu = 1;		/* flag: menu specified?	    */
int prompt_after_pager = 1;	/* flag: prompt after pager exits   */
int folder_type = 0;		/* flag: type of folder		    */
int auto_copy = 0;		/* flag: automatically copy source? */
int filter = 1;			/* flag: weed out header lines?	    */
int resolve_mode = 1;		/* flag: delete saved mail?	    */
int auto_cc = 0;		/* flag: mail copy to user?	    */
int noheader = 1;		/* flag: copy + header to file?     */
int title_messages = 1;		/* flag: title message display?     */
int forwarding = 0;		/* flag: are we forwarding the msg? */
int hp_terminal = 0;		/* flag: are we on HP term?	    */
int hp_softkeys = 0;		/* flag: are there softkeys?        */
int save_by_name = 1;		/* flag: save mail by login name?   */
int force_name = 0;		/* flag: save by name forced?	    */
int mail_only = 0;		/* flag: send mail then leave?      */
int check_only = 0;		/* flag: check aliases then leave?  */
int batch_only = 0;		/* flag: send without prompting?    */
int move_when_paged = 0;	/* flag: move when '+' or '-' used? */
int point_to_new = 1;		/* flag: start pointing at new msg? */
int bounceback = 0;		/* flag: bounce copy off remote?    */
int always_keep = 1;		/* flag: always keep unread msgs?   */
int always_store = 0;		/* flag: always store read msgs?    */
int always_del = 0;		/* flag: always delete marked msgs? */
int arrow_cursor = 0;		/* flag: use "->" cursor regardless?*/
int debug = 0; 			/* flag: default is no debug!       */
int warnings = 1;		/* flag: output connection warnings?*/
int user_level = 0;		/* flag: how good is the user?      */
int selected = 0;		/* flag: used for select stuff      */
int names_only = 1;		/* flag: display user names only?   */
int question_me = 1;		/* flag: ask questions as we leave? */
int keep_empty_files = 0;	/* flag: leave empty folder files? */
int clear_pages = 0;		/* flag: act like "page" (more -c)? */
int prompt_for_cc = 1;		/* flag: ask user for "cc:" value?  */
int sig_dashes = 1;		/* flag: include dashes above sigs? */

int sortby = REVERSE SENT_DATE;	/* how to sort incoming mail...     */

long timeout = 600L;		/* timeout (secs) on main prompt    */

/** set up some default values for a 'typical' terminal *snicker* **/

int LINES=23;			/** lines per screen      **/
int COLUMNS=80;			/** columns per page      **/

long size_of_pathfd;		/** size of pathfile, 0 if none **/

FILE *mailfile;			/* current folder	    */
FILE *debugfile;		/* file for debug output    */
FILE *pathfd;			/* path alias file          */
FILE *domainfd;			/* domain file		    */

long mailfile_size;		/* size of current mailfile */

int   max_headers;		/* number of headers allocated */

struct header_rec **headers;    /* array of header structure pointers */

struct alias_rec user_hash_table[MAX_UALIASES];
struct alias_rec system_hash_table[MAX_SALIASES];

struct lsys_rec *talk_to_sys = NULL; /* what machines do we talk to? */

struct addr_rec *alternative_addresses;	/* how else do we get mail? */

int system_data = -1;		/* fileno of system data file */
int user_data = -1;		/* fileno of user data file   */

int userid;			/* uid for current user	      */
int groupid;			/* groupid for current user   */

#if defined(BSD) || defined(OS2)
jmp_buf GetPromptBuf;		/* setjmp buffer */
int InGetPrompt;		/* set if in GetPrompt() in read() */
#endif
