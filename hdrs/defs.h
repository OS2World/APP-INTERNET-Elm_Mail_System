
/* $Id: defs.h,v 4.1.1.2 90/06/05 21:23:19 syd Exp $ */

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
 * $Log:	defs.h,v $
 * Revision 4.1.1.2  90/06/05  21:23:19  syd
 * Fix other side of same problem
 * From: Syd
 *
 * Revision 4.1.1.1  90/06/05  21:16:42  syd
 * Try and avoid double definitions for the null
 * macro for htonl when the system includes aready
 * have it
 * From: Syd
 *
 * Revision 4.1  90/04/28  22:42:06  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/**  define file for ELM mail system.  **/


#include "../config.h"
#include "sysdefs.h"	/* system/configurable defines */

#ifdef OS2
#include "../os2/os2elm.h"
#endif


# define VERSION 		"2.3"		/* Version number... */
# define VERS_DATE	"May 1, 1990"		/* for elm -v option */
# define WHAT_STRING	\
	"@(#) Version 2.3, USENET supported version, released May 1990"

#define KLICK		25

#define SLEN		256	    /* long for ensuring no overwrites... */
#define SHORT		10	    /* super short strings!		  */
#define NLEN		48	    /* name length for aliases            */
#define WLEN		20
#define STRING		128	/* reasonable string length for most..      */
#define LONG_STRING	512	/* even longer string for group expansion   */
#define VERY_LONG_STRING 2560	/* huge string for group alias expansion    */
#define MAX_LINE_LEN	5120	/* even bigger string for "filter" prog..   */

#define BREAK		'\0'  		/* default interrupt    */
#define BACKSPACE	'\b'     	/* backspace character  */
#define TAB		'\t'            /* tab character        */
#define RETURN		'\r'     	/* carriage return char */
#define LINE_FEED	'\n'     	/* line feed character  */
#define FORMFEED	'\f'     	/* form feed (^L) char  */
#define COMMA		','		/* comma character      */
#define SPACE		' '		/* space character      */
#define DOT		'.'		/* period/dot character */
#define BANG		'!'		/* exclaimation mark!   */
#define AT_SIGN		'@'		/* at-sign character    */
#define PERCENT		'%'		/* percent sign char.   */
#define COLON		':'		/* the colon ..		*/
#define BACKQUOTE	'`'		/* backquote character  */
#define TILDE_ESCAPE	'~'		/* escape character~    */
#define ESCAPE		'\033'		/* the escape		*/

#define NO_OP_COMMAND	'\0'		/* no-op for timeouts   */

#define STANDARD_INPUT  0		/* file number of stdin */

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

#define NO		0
#define YES		1
#define MAYBE		2		/* a definite define, eh?  */
#define FORM		3		/*      <nevermind>        */
#define PREFORMATTED	4		/* forwarded form...       */

#define SAME_PAGE	1		/* redraw current only     */
#define NEW_PAGE	2		/* redraw message list     */
#define ILLEGAL_PAGE	0		/* error in page list, punt */

#define PAD		0		/* for printing name of    */
#define FULL		1		/*   the sort we're using  */

#define OUTGOING	0		/* defines for lock file   */
#define INCOMING	1		/* creation..see lock()    */

#define SH		0		/* defines for system_call */
#define USER_SHELL	1		/* to work correctly!      */

#define EXECUTE_ACCESS	01		/* These five are 	   */
#define WRITE_ACCESS	02		/*    for the calls	   */
#define READ_ACCESS	04		/*       to access()       */
#define ACCESS_EXISTS	00		/*           <etc>         */
#define EDIT_ACCESS	06		/*  (this is r+w access)   */

#define BIG_NUM		999999		/* big number!             */
#define BIGGER_NUM	9999999 	/* bigger number!          */

#define START_ENCODE	"[encode]"
#define END_ENCODE	"[clear]"

#define DONT_SAVE	"[no save]"
#define DONT_SAVE2	"[nosave]"


/** some defines for the 'userlevel' variable... **/

#define RANK_AMATEUR	0
#define AMATEUR		1
#define OKAY_AT_IT	2
#define GOOD_AT_IT	3
#define EXPERT		4
#define SUPER_AT_IT	5

/** some defines for the "status" field of the header record **/

#define ACTION		1		/* bit masks, of course */
#define CONFIDENTIAL	2
#define DELETED		4
#define EXPIRED		8
#define FORM_LETTER	16
#define NEW		32
#define PRIVATE		64
#define TAGGED		128
#define URGENT		256
#define VISIBLE		512
#define UNREAD		1024
#define STATUS_CHANGED	2048

#define UNDELETE	0		/* purely for ^U function... */

/** values for headers exit_disposition field */
#define UNSET	0
#define KEEP	1
#define	STORE	2
#define DELETE	3

/** some months... **/

#define JANUARY		0			/* months of the year */
#define FEBRUARY	1
#define MARCH		2
#define APRIL		3
#define MAY		4
#define JUNE		5
#define JULY		6
#define AUGUST		7
#define SEPTEMBER	8
#define OCTOBER		9
#define NOVEMBER	10
#define DECEMBER	11

#define equal(s,w)	(strcmp(s,w) == 0)
#define min(a,b)	(a < b? a : b)
#define ctrl(c)	        (c == '?' ? 127 : c - 'A' + 1)	/* control character mapping */
#define plural(n)	n == 1 ? "" : "s"
#define lastch(s)	s[strlen(s)-1]

/* find tab stops preceding or following a given column position 'a', where
 * the column position starts counting from 1, NOT 0!
 * The external integer "tabspacing" must be declared to use this. */
#define prev_tab(a)	(((((a-1)/tabspacing))*tabspacing)+1)
#define next_tab(a)	(((((a-1)/tabspacing)+1)*tabspacing)+1)

#define movement_command(c)	(c == 'j' || c == 'k' || c == ' ' || 	      \
				 c == BACKSPACE || c == ESCAPE || c == '*' || \
				 c == '-' || c == '+' || c == '=' ||          \
				 c == '#' || c == '@' || c == 'x' || 	      \
				 c == 'a' || c == 'q')

#define no_ret(s)	{ register int xyz; /* varname is for lint */	      \
		          for (xyz=strlen(s)-1; xyz >= 0 && 		      \
				(s[xyz] == '\r' || s[xyz] == '\n'); )	      \
			     s[xyz--] = '\0';                                 \
			}

#define first_word(s,w) (strncmp(s,w, strlen(w)) == 0)
#define ClearLine(n)	MoveCursor(n,0); CleartoEOLN()
#define whitespace(c)	(c == ' ' || c == '\t')
#define ok_rc_char(c)	(isalnum(c) || c == '-' || c == '_')
#define ok_alias_char(c) (isalnum(c) || c == '-' || c == '_' || c == '.')
#define quote(c)	(c == '"' || c == '\'')
#define onoff(n)	(n == 0 ? "OFF" : "ON")

/** The next function is so certain commands can be processed from the showmsg
    routine without rewriting the main menu in between... **/

#define special(c)	(c == 'j' || c == 'k')

/** and a couple for dealing with status flags... **/

#define ison(n,mask)	(n & mask)
#define isoff(n,mask)	(!ison(n, mask))

#define setit(n,mask)		n |= mask
#define clearit(n, mask)	n &= ~mask

/** a few for the usage of function keys... **/

#define f1	1
#define f2	2
#define f3	3
#define f4	4
#define f5	5
#define f6	6
#define f7	7
#define f8	8

#define MAIN	0
#define ALIAS   1
#define YESNO	2
#define CHANGE  3
#define READ	4

#define MAIN_HELP    0
#define OPTIONS_HELP 1
#define ALIAS_HELP   2
#define PAGER_HELP   3

/** types of folders **/
#define NO_NAME		0		/* variable contains no file name */
#define NON_SPOOL	1		/* mailfile not in mailhome */
#define SPOOL		2		/* mailfile in mailhome */

/* the following is true if the current mailfile is the user's spool file*/
#define USERS_SPOOL	(strcmp(cur_folder, defaultfile) == 0)

/** some possible sort styles... **/

#define REVERSE		-		/* for reverse sorting           */
#define SENT_DATE	1		/* the date message was sent     */
#define RECEIVED_DATE	2		/* the date message was received */
#define SENDER		3		/* the name/address of sender    */
#define SIZE		4		/* the # of lines of the message */
#define SUBJECT		5		/* the subject of the message    */
#define STATUS		6		/* the status (deleted, etc)     */
#define MAILBOX_ORDER	7		/* the order it is in the file   */

/* some stuff for our own malloc call - pmalloc */

#define PMALLOC_THRESHOLD	256	/* if greater, then just use malloc */
#define PMALLOC_BUFFER_SIZE    2048	/* internal [memory] buffer size... */


/** the following macro is as suggested by Larry McVoy.  Thanks! **/

# ifdef DEBUG
#  define   dprint(n,x)		{ 				\
				   if (debug >= n)  {		\
				     fprintf x ; 		\
				     fflush(debugfile);         \
				   }				\
				}
# else
#  define   dprint(n,x)
# endif

/* some random structs... */

struct date_rec {
	int  month;		/** this record stores a **/
	int  day;		/**   specific date and  **/
	int  year;		/**     time...		 **/
	int  hour;
	int  minute;
       };

struct header_rec {
	int  lines;		/** # of lines in the message  **/
	int  status;		/** Urgent, Deleted, Expired?  **/
	int  index_number;	/** relative loc in file...    **/
	int  encrypted;		/** whether msg has encryption **/
	int  exit_disposition;	/** whether to keep, store, delete **/
	int  status_chgd;	/** whether became read or old, etc. **/
	long offset;		/** offset in bytes of message **/
	struct date_rec received; /** when elm received here   **/
	char from[STRING];	/** who sent the message?      **/
	char to[STRING];	/** who it was sent to	       **/
	char messageid[STRING];	/** the Message-ID: value      **/
	char dayname[8];	/**  when the                  **/
	char month[10];		/**        message             **/
	char day[3];		/**          was 	       **/
	char year[5];		/**            sent            **/
	char time[NLEN];	/**              to you!       **/
	char time_zone[12];	/**                incl. tz    */
	long time_sent;		/** gmt when sent for sorting  **/
	char subject[STRING];   /** The subject of the mail    **/
	char mailx_status[WLEN];/** mailx status flags (RO...) **/
       };

struct alias_rec {
	char   name[NLEN];	/* alias name 			     */
	long   byte;		/* offset into data file for address */
       };

struct lsys_rec {
	char   name[NLEN];	/* name of machine connected to      */
	struct lsys_rec *next;	/* linked list pointer to next       */
       };

struct addr_rec {
	 char   address[NLEN];	/* machine!user you get mail as      */
	 struct addr_rec *next;	/* linked list pointer to next       */
	};

#ifdef SHORTNAMES	/* map long names to shorter ones */
# include <shortname.h>
#endif

/** Let's make sure that we're not going to have any annoying problems with
    int pointer sizes versus char pointer sizes by guaranteeing that every-
    thing vital is predefined... (Thanks go to Detlev Droege for this one)
**/

#ifdef STRINGS
#  include <strings.h>
#else
#  include <string.h>
#endif

/*
 * Macros for network/external number representation conversion.
 *	Note, some system include files already have htonl defined
 *	as this same macro, the ifndef should prevent conflicts.
 */
#ifdef NETWORK_ORDER
#  ifndef ntohl
unsigned short	ntohs(), htons();
unsigned long	ntohl(), htonl();
#  endif
#else
#  ifndef ntohl
#     define	ntohl(x)	(x)
#     define	ntohs(x)	(x)
#     define	htonl(x)	(x)
#     define	htons(x)	(x)
#  endif
#endif

char *argv_zero();
char *bounce_off_remote();
char *ctime();
char *error_description();
char *error_name();
char *error_number();
char *expand_address();
char *expand_domain();
char *expand_group();
char *expand_logname();
char *expand_system();
char *find_path_to();
char *format_long();
char *get_alias_address();
char *get_arpa_date();
char *get_ctime_date();
char *get_date();
char *get_token();
char *getenv();
char *getlogin();
char *level_name();
char *match_and_expand_domain();
char *shift_lower();
char *strip_commas();
char *strip_parens();
char *strpbrk();
char *strtok();
char *tail_of_string();
char *tgetstr();
char *pmalloc();

long lseek();
long times();
long ulimit();
