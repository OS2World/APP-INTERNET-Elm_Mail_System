
/* $Id: headers.h,v 4.1.1.1 90/10/24 15:33:07 syd Exp $ */

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
 * $Log:	headers.h,v $
 * Revision 4.1.1.1  90/10/24  15:33:07  syd
 * Remove variables no longer used
 * From: W. David Higgins
 *
 * Revision 4.1  90/04/28  22:42:11  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/**  This is the header file for ELM mail system.  **/


#include <stdio.h>
#include <fcntl.h>

#include "curses.h"
#include "defs.h"

#ifdef OS2
#include "../os2/os2elm.h"
#include <setjmp.h>
#endif

#ifdef	BSD
#include <setjmp.h>
#endif

#ifndef       clearerr
#define       clearerr(p)     (void)((p)->_flag &= ~(_IOERR|_IOEOF))
#endif

/******** global variables accessable by all pieces of the program *******/

extern int check_size;		/* don't start mailer if no mail */
extern int current;		/* current message number  */
extern int header_page;         /* current header page     */
extern int last_header_page;    /* last header page        */
extern int message_count;	/* max message number      */
extern int headers_per_page;	/* number of headers/page  */
extern int original_umask;	/* original umask, for restore before subshell */
extern int sendmail_verbose;    /* Allow extended debugging on sendmail */
extern int no_save;		/* Do not save outgoing mail */
extern int mmdf_strict;		/* Strict MMDF mailbox scanning */
extern char cur_folder[SLEN];	/* name of current folder */
extern char cur_tempfolder[SLEN]; /* name of temp folder open for a mailbox */
extern char defaultfile[SLEN];	/* name of default folder */
extern char temp_dir[SLEN];     /* name of temp directory */
extern char hostname[SLEN];	/* name of machine we're on*/
extern char hostdomain[SLEN];	/* name of domain we're in */
extern char hostfullname[SLEN]; /* name of FQDN we're in */
extern char hostfromname[SLEN]; /* name of FQDN we pretend to be in */
extern char username[SLEN];	/* return address name!    */
extern char full_username[SLEN];/* Full username - gecos   */
extern char home[SLEN];		/* home directory of user  */
extern char folders[SLEN];	/* folder home directory   */
extern char raw_folders[SLEN];	/* unexpanded folder home directory   */
extern char recvd_mail[SLEN];	/* folder for storing received mail	*/
extern char raw_recvdmail[SLEN];/* unexpanded recvd_mail name */
extern char editor[SLEN];	/* default editor for mail */
extern char raw_editor[SLEN];	/* unexpanded default editor for mail */
extern char alternative_editor[SLEN];/* the 'other' editor */
extern char printout[SLEN];	/* how to print messages   */
extern char raw_printout[SLEN];	/* unexpanded how to print messages   */
extern char sent_mail[SLEN];	/* name of file to save copies to */
extern char raw_sentmail[SLEN];	/* unexpanded name of file to save to */
extern char calendar_file[SLEN];/* name of file for clndr  */
extern char raw_calendar_file[SLEN];/* unexpanded name of file for clndr  */
extern char attribution[SLEN];  /* attribution string for replies     */
extern char prefixchars[SLEN];	/* prefix char(s) for msgs */
extern char shell[SLEN];	/* default system shell    */
extern char raw_shell[SLEN];	/* unexpanded default system shell    */
extern char pager[SLEN];	/* what pager to use...    */
extern char raw_pager[SLEN];	/* unexpanded what pager to use...    */
extern char batch_subject[SLEN];/* subject buffer for batchmail */
extern char local_signature[SLEN];/* local msg signature file   */
extern char raw_local_signature[SLEN];/* unexpanded local msg signature file */
extern char remote_signature[SLEN];/* remote msg signature file */
extern char raw_remote_signature[SLEN];/* unexpanded remote msg signature file*/

extern char backspace,		/* the current backspace char  */
	    escape_char,	/* '~' or something else...    */
	    kill_line;		/* the current kill_line char  */

extern char up[SHORT],
	    down[SHORT],
	    left[SHORT],
	    right[SHORT];	/* cursor control seq's    */
extern int  cursor_control;	/* cursor control avail?   */

extern int  has_highlighting;	/* highlighting available? */

/** the following two are for arbitrary weedout lists.. **/

extern char *weedlist[MAX_IN_WEEDLIST];
extern int  weedcount;		/* how many headers to check?        */

extern int  allow_forms;	/* flag: are AT&T Mail forms okay?    */
extern int  prompt_after_pager;	/* flag: prompt after pager exits     */
extern int  mini_menu;		/* flag: display menu?     	      */
extern int  folder_type;	/* flag: type of folder		      */
extern int  auto_copy;		/* flag: auto copy source into reply? */
extern int  filter;		/* flag: weed out header lines?	      */
extern int  resolve_mode;	/* flag: resolve before moving mode?  */
extern int  auto_cc;		/* flag: mail copy to yourself?       */
extern int  noheader;		/* flag: copy + header to file?       */
extern int  title_messages;	/* flag: title message display?       */
extern int  forwarding;		/* flag: are we forwarding the msg?   */
extern int  hp_terminal;	/* flag: are we on an hp terminal?    */
extern int  hp_softkeys;	/* flag: are there softkeys?          */
extern int  save_by_name;  	/* flag: save mail by login name?     */
extern int  force_name;		/* flag: save by name forced?	      */
extern int  mail_only;		/* flag: send mail then leave?        */
extern int  check_only;		/* flag: check aliases and leave?     */
extern int  batch_only;		/* flag: send without prompting?      */
extern int  move_when_paged;	/* flag: move when '+' or '-' used?   */
extern int  point_to_new;	/* flag: start pointing at new msgs?  */
extern int  bounceback;		/* flag: bounce copy off remote?      */
extern int  always_keep;	/* flag: always keep unread msgs?     */
extern int  always_store;	/* flag: always store read mail?      */
extern int  always_del;		/* flag: always delete marked msgs?   */
extern int  arrow_cursor;	/* flag: use "->" regardless?	      */
extern int  debug;		/* flag: debugging mode on?           */
extern int  warnings;		/* flag: output connection warnings?  */
extern int  user_level;		/* flag: how knowledgable is user?    */
extern int  selected;		/* flag: used for select stuff        */
extern int  names_only;		/* flag: display names but no addrs?  */
extern int  question_me;	/* flag: ask questions as we leave?   */
extern int  keep_empty_files;	/* flag: keep empty files??	      */
extern int  clear_pages;	/* flag: clear screen w/ builtin pgr? */
extern int  prompt_for_cc;	/* flag: prompt user for 'cc' value?  */
extern int  sig_dashes;		/* flag: put dashes above signature?  */

extern int  sortby;		/* how to sort folders	      */

extern long timeout;		/* seconds for main level timeout     */

extern int LINES;		/** lines per screen    **/
extern int COLUMNS;		/** columns per line    **/

extern long size_of_pathfd;	/** size of pathfile, 0 if none **/

extern FILE *mailfile;		/* current folder 	    */
extern FILE *debugfile;		/* file for debut output    */
extern FILE *pathfd;		/* path alias file          */
extern FILE *domainfd;		/* domains file 	    */

extern long mailfile_size;	/* size of current mailfile */

extern int  max_headers;	/* number of headers currently allocated */

extern struct header_rec **headers; /* array of header structure pointers */

extern struct alias_rec user_hash_table  [MAX_UALIASES];
extern struct alias_rec system_hash_table[MAX_SALIASES];

extern struct lsys_rec *talk_to_sys;	/* who do we talk to? */

extern struct addr_rec *alternative_addresses;	/* how else do we get mail? */

extern int system_data;		/* fileno of system data file */
extern int user_data;		/* fileno of user data file   */

extern int userid;		/* uid for current user	      */
extern int groupid;		/* groupid for current user   */

#if defined(BSD) || defined(OS2)
extern jmp_buf GetPromptBuf;	/* setjmp buffer */
extern int InGetPrompt;		/* set if in GetPrompt() in read() */
#endif
