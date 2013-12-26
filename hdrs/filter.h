
/* $Id: filter.h,v 4.1 90/04/28 22:42:09 syd Exp $ */

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
 * $Log:	filter.h,v $
 * Revision 4.1  90/04/28  22:42:09  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Headers for the filter program.

**/

#ifdef   BSD
# undef  tolower
#endif

/** define a few handy macros for later use... **/

#define  the_same(a,b)	(strncmp(a,b,strlen(b)) == 0)

#define relationname(x)  (x == 1?"<=":x==2?"<":x==3?">=":x==4?">":x==5?"!=":"=")

#define quoteit(x)	 (x == LINES? "" : "\"")

#define remove_return(s)	{ if (s[strlen(s)-1] == '\n') \
				    s[strlen(s)-1] = '\0';    \
			   	}

/** some of the files we'll be using, where they are, and so on... **/

#define  filter_temp	"filter"
#define  filterfile	"elm/filter.rul"
#define  filterlog	"elm/filter.log"
#define  filtersum	"elm/filter.sum"

#define  EMERGENCY_MAILBOX      "emergenc.mbx"
#define  EMERG_MBOX             "emerg.mbx"

/** and now the hardwired constraint of the program.. **/

#define  MAXRULES	25		/* can't have more den dis, boss! */

/** some random defines for mnemonic stuff in the program... **/

#ifdef SUBJECT
# undef SUBJECT
#endif

#define  TO		1
#define  FROM		2
#define  LINES		3
#define  SUBJECT	4
#define  CONTAINS	5
#define  ALWAYS		6

#define  DELETE_MSG 	7
#define  SAVE		8
#define  SAVECC		9
#define  FORWARD	10
#define  LEAVE		11
#define  EXEC		12

#define  FAILED_SAVE	20

/** Some conditionals... **/

#define LE		1
#define LT		2
#define GE		3
#define GT		4
#define NE		5
#define EQ		6

/** A funky way to open a file using open() to avoid file locking hassles **/

#define  FOLDERMODE	O_WRONLY | O_APPEND | O_CREAT

/** cheap but easy way to have two files share the same #include file **/

#ifdef MAIN_ROUTINE

char home[SLEN],				/* the users home directory */
     hostname[SLEN],			/* the machine name...      */
     username[SLEN];			/* the users login name...  */
char hostfromname[SLEN];                /* name of FQDN we pretend to be in */

char to[VERY_LONG_STRING],
     from[LONG_STRING],
     subject[LONG_STRING];		/* from current message     */

FILE *outfd;
char outfname[SLEN];

int  total_rules = 0,				/* how many rules to check  */
     show_only = FALSE,				/* just for show?           */
     long_summary = FALSE,			/* what sorta summary??     */
     verbose   = FALSE,				/* spit out lots of stuff   */
     lines     = 0,				/* lines in message..       */
     clear_logs = FALSE,			/* clear files after sum?   */
     already_been_forwarded = FALSE,		/* has this been filtered?  */
     log_actions_only = FALSE,			/* log actions | everything */
     printing_rules = FALSE,			/* are we just using '-r'?  */
     rule_choosen;				/* which one we choose      */

#else

extern char home[SLEN],				/* the users home directory */
            hostname[SLEN],			/* the machine name...      */
            username[SLEN];			/* the users login name...  */
extern char hostfromname[SLEN];         /* name of FQDN we pretend to be in */

extern char to[VERY_LONG_STRING],
            from[LONG_STRING],
            subject[LONG_STRING];		/* from current message     */

extern FILE *outfd;
extern char outfname[SLEN];

extern int total_rules,				/* how many rules to check  */
           show_only,				/* just for show?           */
           long_summary,			/* what sorta summary??     */
           verbose,				/* spit out lots of stuff   */
           lines,				/* lines in message..       */
           clear_logs,			        /* clear files after sum?   */
	   already_been_forwarded,		/* has this been filtered?  */
           log_actions_only,			/* log actions | everything */
           printing_rules,			/* are we just using '-r'?  */
           rule_choosen;			/* which one we choose      */
#endif

/** and our ruleset record structure... **/

struct condition_rec {
	int     matchwhat;			/* type of 'if' clause      */
	int     relation;			/* type of match (eq, etc)  */
	char    argument1[SLEN];		/* match against this       */
	struct  condition_rec  *next;		/* next condition...	    */
      };

struct ruleset_record {
	char  	printable[SLEN];		/* straight from file...    */
	struct  condition_rec  *condition;
	int     action;				/* what action to take      */
	char    argument2[SLEN];		/* argument for action      */
      };

#ifdef MAIN_ROUTINE
  struct ruleset_record rules[MAXRULES];
#else
  extern struct ruleset_record rules[MAXRULES];
#endif

/** finally let's keep LINT happy with the return values of all these pups! ***/

char *itoa();

#ifdef	_POSIX_SOURCE		/*{_POSIX_SOURCE*/
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#else				/*}_POSIX_SOURCE{*/
unsigned short getuid();

unsigned long sleep();

char *malloc(), *strcpy(), *strcat();

void	exit();

#ifdef BSD

  FILE *popen();

#ifdef MAIN_ROUTINE
  char  _vbuf[5*BUFSIZ];              /* space for file buffering */
#else
  extern char  _vbuf[5*BUFSIZ];		/* space for file buffering */
#endif

#ifndef _IOFBF
# define _IOFBF		0		/* doesn't matter - ignored */
#endif

# define setvbuf(fd,a,b,c)	setbuffer(fd, _vbuf, 5*BUFSIZ)

#endif
#endif				/*}_POSIX_SOURCE*/
