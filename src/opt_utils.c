
static char rcsid[] = "@(#)$Id: opt_utils.c,v 4.1 90/04/28 22:43:37 syd Exp $";

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
 * $Log:	opt_utils.c,v $
 * Revision 4.1  90/04/28  22:43:37  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This file contains routines that might be needed for the various
     machines that the mailer can run on.  Please check the Makefile
     for more help and/or information.

**/

#include "headers.h"
#include <ctype.h>

#ifdef PWDINSYS
#  include <sys/pwd.h>
#else
#  include <pwd.h>
#endif

#ifdef BSD
#undef tolower
#undef toupper
#endif

#ifndef GETHOSTNAME
# ifdef DOUNAME
#  include <sys/types.h>
#  include <sys/utsname.h>
# endif
#endif

char *index();

#ifndef GETHOSTNAME

gethostname(cur_hostname,size) /* get name of current host */
char *cur_hostname;
int size;
{
	/** Return the name of the current host machine. **/

#if defined(XENIX) & !defined(DOUNAME)
	char    buf[32];
	FILE    *fp;
	char    *p;

	if ((fp = fopen("/etc/systemid", "r")) != 0) {
	  fgets(buf, sizeof(buf) - 1, fp);
	  fclose(fp);
	  if ((p = index(buf, '\n')) != NULL)
	    *p = '\0';
	  (void) strncpy(cur_hostname, buf, size - 1);
	  cur_hostname[size - 1] = '\0';
	  return 0;
	}

#else   /* XENIX */

#ifdef DOUNAME
	/** This routine compliments of Scott McGregor at the HP
	    Corporate Computing Center **/

	int uname();
	struct utsname name;

	(void) uname(&name);
	(void) strncpy(cur_hostname,name.nodename,size-1);
#else
	(void) strncpy(cur_hostname, HOSTNAME, size-1);
#endif	/* DOUNAME */

	cur_hostname[size - 1] = '\0';
	return 0;

#endif  /* XENIX */
}

#endif  /* GETHOSTNAME */


#ifndef OS2
gethostdomain(hostdom, size)    /* get domain of current host */
char *hostdom;
int size;
{
	char    buf[64];
	FILE    *fp;
	char    *p;

	if (size < 2)
	  return -1;

        sprintf(buf, "%s/%s", elmhome, hostdomfile);

	if ((fp = fopen(buf, "r")) != 0) {
	  fgets(buf, sizeof(buf) - 1, fp);
	  fclose(fp);
	  if ((p = index(buf, '\n')) != NULL)
	    *p = '\0';
	}
	else {
	  strncpy(buf, DEFAULT_DOMAIN, sizeof(buf) - 1);
	}
	if (buf[0] != '\0' && buf[0] != '.') {
	  *hostdom++ = '.';
	  --size;
	}
	(void) strncpy(hostdom, buf, size - 1);
	hostdom[size - 1] = '\0';

	return 0;
}
#endif


#ifdef NEED_CUSERID

char *cuserid(uname)
     char *uname;
{
	/** Added for compatibility with Bell systems, this is the last-ditch
	    attempt to get the users login name, after getlogin() fails.  It
	    instantiates "uname" to the name of the user...(it also tries
	    to use "getlogin" again, just for luck)
	**/
	/** This wasn't really compatible.  According to our man page,
	 ** It was inconsistent.  If the parameter is NULL then you return
	 ** the name in a static area.  Else the ptr is supposed to be a
	 ** pointer to l_cuserid bytes of memory [probally 9 bytes]...
	 ** It's not mention what it should return if you copy the name
	 ** into the array, so I chose NULL.
	 ** 					Sept 20, 1988
	 **					**WJL**
	 **/

  struct passwd *password_entry;
#ifndef _POSIX_SOURCE
  struct passwd *getpwuid();
#endif
  char   *name, *getlogin();
  static char buf[10];
  register returnonly = 0;

  if (uname == NULL) ++returnonly;

  if ((name = getlogin()) != NULL) {
    if (returnonly) {
      return(name);
    } else {
      strcpy(uname, name);
      return name;
    }
  }
  else
    if (( password_entry = getpwuid(getuid())) != NULL)
      {
	if (returnonly)
	  {
	    return(password_entry->pw_name);
	  }
	else
	  {
	    strcpy(uname, password_entry->pw_name);
	    return name;
	  }
      }
    else
      {
	return NULL;
      }
}

#endif

#if defined(BSD) && !defined(_POSIX_SOURCE)

/** some supplementary string functions for Berkeley Unix systems **/

int
tolower(ch)
char ch;
{
	/** This should be a macro call, but if you use this as a macro
	    calls to 'tolower' where the argument is a function call will
	    cause the function to be called TWICE which is obviously the
	    wrong behaviour.  On the other hand, to just blindly translate
	    assuming the character is always uppercase can cause BIG
	    problems, so...
	**/

	return ( isupper(ch) ? ch - 'A' + 'a' : ch );
}

int
toupper(ch)
char ch;
{
	/** see comment for above routine - tolower() **/

	return ( islower(ch) ? ch - 'a' + 'A' : ch );
}

char *strtok(source, keys)
char *source, *keys;
{
	/** This function returns a pointer to the next word in source
	    with the string considered broken up at the characters
	    contained in 'keys'.  Source should be a character pointer
	    when this routine is first called, then NULL subsequently.
	    When strtok has exhausted the source string, it will
	    return NULL as the next word.

	    WARNING: This routine will DESTROY the string pointed to
	    by 'source' when first invoked.  If you want to keep the
	    string, make a copy before using this routine!!
	 **/

	register int  last_ch;
	static   char *sourceptr;
		 char *return_value;

	if (source != NULL)
	  sourceptr = source;

	if (*sourceptr == '\0')
	  return(NULL);		/* we hit end-of-string last time!? */

	sourceptr += strspn(sourceptr, keys);	/* skip leading crap */

	if (*sourceptr == '\0')
	  return(NULL);		/* we've hit end-of-string */

	last_ch = strcspn(sourceptr, keys);	/* end of good stuff */

	return_value = sourceptr;		/* and get the ret   */

	sourceptr += last_ch;			/* ...value 	     */

	if (*sourceptr != '\0')		/* don't forget if we're at END! */
	  sourceptr++;			   /* and skipping for next time */

	return_value[last_ch] = '\0';		/* ..ending right    */

	return((char *) return_value);		/* and we're outta here! */
}

char *strpbrk(source, keys)
char *source, *keys;
{
	/** Returns a pointer to the first character of source that is any
	    of the specified keys, or NULL if none of the keys are present
	    in the source string.
	**/

	register int loc = 0, key_index = 0;

	while (source[loc] != '\0') {
	  key_index = 0;
	  while (keys[key_index] != '\0')
	    if (keys[key_index++] == source[loc])
	      return((char *) (source + loc));
	  loc++;
	}

	return(NULL);
}

#endif

#ifndef STRSPN

strspn(source, keys)
char *source, *keys;
{
	/** This function returns the length of the substring of
	    'source' (starting at zero) that consists ENTIRELY of
	    characters from 'keys'.  This is used to skip over a
	    defined set of characters with parsing, usually.
	**/

	register int loc = 0, key_index = 0;

	while (source[loc] != '\0') {
	  key_index = 0;
	  while (keys[key_index] != source[loc])
	    if (keys[key_index++] == '\0')
	      return(loc);
	  loc++;
	}

	return(loc);
}

#endif

#ifndef STRCSPN

strcspn(source, keys)
char *source, *keys;
{
	/** This function returns the length of the substring of
	    'source' (starting at zero) that consists entirely of
	    characters NOT from 'keys'.  This is used to skip to a
	    defined set of characters with parsing, usually.
	    NOTE that this is the opposite of strspn() above
	**/

	register int loc = 0, key_index = 0;

	while (source[loc] != '\0') {
	  key_index = 0;
	  while (keys[key_index] != '\0')
	    if (keys[key_index++] == source[loc])
	      return(loc);
	  loc++;
	}

	return(loc);
}

#endif

#ifndef TEMPNAM
/* and a tempnam for temporary files */
static int cnt = 0;

char *tempnam( dir, pfx)
 char *dir, *pfx;
{
	char space[SLEN];
	char *newspace;

	char	*malloc();

	if (dir == NULL) {
		dir = "/usr/tmp";
	} else if (*dir == '\0') {
		dir = "/usr/tmp";
	}

	if (pfx == NULL) {
		pfx = "";
	}

	sprintf(space, "%s%s%d.%d", dir, pfx, getpid(), cnt);
	cnt++;

	newspace = malloc(strlen(space) + 1);
	if (newspace != NULL) {
		strcpy(newspace, space);
	}
	return newspace;
}

#endif

#ifndef GETOPT

/*LINTLIBRARY*/
#define NULL	0
#define EOF	(-1)
#define ERR(s, c)	if(opterr){\
	extern int strlen(), write();\
	char errbuf[2];\
	errbuf[0] = c; errbuf[1] = '\n';\
	(void) write(2, argv[0], (unsigned)strlen(argv[0]));\
	(void) write(2, s, (unsigned)strlen(s));\
	(void) write(2, errbuf, 2);}

extern int strcmp();

int	opterr = 1;
int	optind = 1;
int	optopt;
char	*optarg;

int
getopt(argc, argv, opts)
int	argc;
char	**argv, *opts;
{
	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1)
		if(optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == NULL) {
			optind++;
			return(EOF);
		}
	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=index(opts, c)) == NULL) {
		ERR(": illegal option -- ", c);
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			ERR(": option requires an argument -- ", c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}

#endif

#ifndef RENAME
int rename(tmpfname, fname)
char *tmpfname, *fname;
{
	int status;

	(void) unlink(fname);
	if ((status = link(tmpfname, fname)) != 0)
		return(status);
	(void) unlink(tmpfname);
	return(0);
}
#endif
