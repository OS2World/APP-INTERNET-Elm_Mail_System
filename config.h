/* config.h
 * This file was produced by running the config.h.SH script, which
 * gets its values from config.sh, which is generally produced by
 * running Configure.
 *
 * Feel free to modify any of this as the need arises.  Note, however,
 * that running config.h.SH again will wipe out any changes you've made.
 * For a more permanent change edit config.sh and rerun config.h.SH.
 */


/* BIN:
 *	This symbol holds the name of the directory in which the user wants
 *	to put publicly executable images for the package in question.  It
 *	is most often a local directory such as /usr/local/bin.
 */
#define BIN "/bin"             /**/

/* BYTEORDER:
 *	This symbol contains an encoding of the order of bytes in a long.
 *	Usual values (in octal) are 01234, 04321, 02143, 03412...
 */
#define BYTEORDER 0x1234		/**/

/* CPPSTDIN:
 *	This symbol contains the first part of the string which will invoke
 *	the C preprocessor on the standard input and produce to standard
 *	output.	 Typical value of "cc -E" or "/lib/cpp".
 */
/* CPPMINUS:
 *	This symbol contains the second part of the string which will invoke
 *	the C preprocessor on the standard input and produce to standard
 *	output.  This symbol will have the value "-" if CPPSTDIN needs a minus
 *	to specify standard input, otherwise the value is "".
 */
#define CPPSTDIN "/lib/cpp"
#define CPPMINUS ""

/* CRYPT:
 *	This symbol, if defined, indicates that the crypt routine is available
 *	to encrypt passwords and the like.
 */
#define	CRYPT		/**/

/* GETOPT:
 *	This symbol, if defined, indicates that the getopt() routine exists.
 */
#define	GETOPT		/**/

/* HAVETERMLIB:
 *	This symbol, when defined, indicates that termlib-style routines
 *	are available.  There is nothing to include.
 */
/* #define	HAVETERMLIB	/**/

/* MKDIR:
 *	This symbol, if defined, indicates that the mkdir routine is available
 *	to create directories.  Otherwise you should fork off a new process to
 *	exec /bin/mkdir.
 */
#define	MKDIR		/**/

/* RENAME:
 *	This symbol, if defined, indicates that the rename routine is available
 *	to rename files.  Otherwise you should do the unlink(), link(), unlink()
 *	trick.
 */
#define	RENAME		/**/

/* SIGVEC:
 *	This symbol, if defined, indicates that BSD reliable signals are
 *	supported.
 */
/* SIGVECTOR:
 *	This symbol, if defined, indicates that the sigvec() routine is called
 *	sigvector() instead, and that sigspace() is provided instead of
 *	sigstack().  This is probably only true for HP-UX.
 */
/* #define	SIGVEC		/**/

/* #define	SIGVECTOR	/**/

/* SYMLINK:
 *	This symbol, if defined, indicates that the symlink routine is available
 *	to create symbolic links.
 */
/* #define	SYMLINK		/**/

/* VFORK:
 *	This symbol, if defined, indicates that vfork() exists.
 */
/* #define	VFORK	/**/

/* WHOAMI:
 *	This symbol, if defined, indicates that the program may include
 *	whoami.h.
 */
/*#undef	WHOAMI		/**/

/* DEFEDITOR:
 *	This symbol contains the name of the default editor.
 */
#define DEFEDITOR "me"		/**/

/* HOSTNAME:
 *	This symbol contains name of the host the program is going to run on.
 *	The domain is not kept with hostname, but must be gotten from MYDOMAIN.
 *	The dot comes with MYDOMAIN, and need not be supplied by the program.
 *	If gethostname() or uname() exist, HOSTNAME may be ignored.
 */
/* MYDOMAIN:
 *	This symbol contains the domain of the host the program is going to
 *	run on.  The domain must be appended to HOSTNAME to form a complete
 *	host name.  The dot comes with MYDOMAIN, and need not be supplied by
 *	the program.  If the host name is derived from PHOSTNAME, the domain
 *	may or may not already be there, and the program should check.
 */
#define HOSTNAME "myhost"		/**/
#define MYDOMAIN ".UUCP"		/**/

/* I_TIME:
 *	This symbol is defined if the program should include <time.h>.
 */
/* I_SYSTIME:
 *	This symbol is defined if the program should include <sys/time.h>.
 */
/* I_SYSTIMEKERNEL:
 *	This symbol is defined if the program should include <sys/time.h>
 *	with KERNEL defined.
 */
#define		I_TIME	 	/**/
/*#define	I_SYSTIME 	/**/
/*#undef	SYSTIMEKERNEL 	/**/

/* PREFSHELL:
 *	This symbol contains the full name of the preferred user shell on this
 *	system.  Usual values are /bin/csh, /bin/ksh, /bin/sh.
 */
#define PREFSHELL "cmd.exe"		/**/

/* EUNICE:
 *	This symbol, if defined, indicates that the program is being compiled
 *	under the EUNICE package under VMS.  The program will need to handle
 *	things like files that don't go away the first time you unlink them,
 *	due to version numbering.  It will also need to compensate for lack
 *	of a respectable link() command.
 */
/* VMS:
 *	This symbol, if defined, indicates that the program is running under
 *	VMS.  It is currently only set in conjunction with the EUNICE symbol.
 */
/*#undef	EUNICE		/**/
/*#undef	VMS		/**/

/* CONFIGURE_DATE
 *	This symbol contains the last date that configure was run for elm -v output.
 */
#define		CONFIGURE_DATE	"Sat Feb  1 14:15:54 MEZ 1992"

/* ASCII_CTYPE:
 *	This symbol, if defined, indicates that the ctype functions and
 *	macros are ascii specific and not 8 bit clean.
 */
#define	ASCII_CTYPE	/**/

/* ENABLE_CALENDAR:
 *	This symbol, if defined, indicates that the calendar feature
 *	should be supported.
 */
#define	ENABLE_CALENDAR	/**/
#define dflt_calendar_file	"calendar"	

/* NEED_CUSERID:
 *	This symbol, if defined, means to include our own cuserid().
 */
/*#undef NEED_CUSERID		/**/

/* LOCK_BY_FLOCK
 *	This symbol, if defined, indicates that the flock mailbox locking should be used.
 */
/* LOCK_FLOCK_ONLY
 *	This symbol, if defined, indicates that the only flock mailbox locking should also be used.
 */
/* LOCK_DIR
 *	This symbol is the name of the lock directory for access (not mailbox) locks.
 *	It will be /usr/spool/locks or /usr/spool/uucp
 */
/*#undef	LOCK_BY_FLOCK		/**/

/*#undef	LOCK_FLOCK_ONLY		/**/

#define		LOCK_DIR	"c:/uupc/locks"	/**/

/* GETHOSTNAME:
 *	This symbol, if defined, indicates that the C program may use the
 *	gethostname() routine to derive the host name.  See also DOUNAME
 *	and PHOSTNAME.
 */
/* DOUNAME:
 *	This symbol, if defined, indicates that the C program may use the
 *	uname() routine to derive the host name.  See also GETHOSTNAME and
 *	PHOSTNAME.
 */
/* PHOSTNAME:
 *	This symbol, if defined, indicates that the C program may use the
 *	contents of PHOSTNAME as a command to feed to the popen() routine
 *	to derive the host name.  See also GETHOSTNAME and DOUNAME.
 */
/* HOSTCOMPILED:
 *	This symbol, if defined, indicated that the host name is compiled
 *	in from the string hostname
 */
#define	GETHOSTNAME	/**/
/*#undef	DOUNAME		/**/
/*#undef	PHOSTNAME ""	/**/
/*#define	HOSTCOMPILED	/**/

/* USE_DBM
 *	This symbol, when defined, indicates that the pathalias file
 *	is available as a dbm file.  There is nothing to include.
 */
/*#undef	USE_DBM	/**/

/* index:
 *	This preprocessor symbol is defined, along with rindex, if the system
 *	uses the strchr and strrchr routines instead.
 */
/* rindex:
 *	This preprocessor symbol is defined, along with index, if the system
 *	uses the strchr and strrchr routines instead.
 */
#define	index strchr	/* cultural */
#define	rindex strrchr	/*  differences? */

/* INTERNET:
 *	This symbol, if defined, indicates that there is a mailer available
 *	which supports internet-style addresses (user@site.domain).
 */
#define	INTERNET	/**/

/* ALLOW_MAILBOX_EDITING:
 *	This symbol, if defined, indicates that the E)dit mailbox
 *	function is to be allowed.
 */
#define	ALLOW_MAILBOX_EDITING	/**/

/* MMDF:
 *	This symbol, if defined, indicates that mailboxes are in
 *	the MMDF format.
 */
#define	MMDF	/**/

/* AUTO_BACKGROUND:
 *	This symbol, if defined, indicates that newmail should go to
 *	the background automatically.
 */
/* #define AUTO_BACKGROUND /**/

/* NFS_CAPABLE
 *	This symbol, if defined, indicates NFS is available.
 */
/* NETWORK_ORDER
 *	This symbol, if defined, indicates that the internal files should be kept
 *	in network byte order.
 */
/* #define	NFS_CAPABLE		/**/
/* #define	NETWORK_ORDER		/**/

/* NO_XHEADER:
 *	This symbol, if defined, will not automatically add "X-Mailer:"
 *	headers.
 */
/*#undef	NO_XHEADER	/**/

/* OPTIMIZE_RETURN:
 *	This symbol, if defined, indicates that Elm should optimize the
 *	return address of aliases.
 */
/*#undef OPTIMIZE_RETURN /**/

/* LOOK_CLOSE_AFTER_SEARCH:
 *	This symbol, if defined, indicates that the pathalias route
 *	should be used for machines we talk to directly.
 */
/* DONT_TOUCH_ADDRESSES:
 *	This symbol, if defined, indicates that elm should not
 *	touch outbound addresses
 */
/* DONT_ADD_FROM:
 *	This symbol, if defined, indicates that elm should not adD
 *	the From: header
 */
/* USE_DOMAIN:
 *	This symbol, if defined, indicates that elm should add
 *	the domain name to our address
 */
/* NOCHECK_VALIDNAME:
 *	This symbol, if defined, indicates that elm should not
 *	check the addresses against mailboxes on this system.
 */
/*#undef	LOOK_CLOSE_AFTER_SEARCH /**/
#define	DONT_TOUCH_ADDRESSES /**/
/*#undef	DONT_ADD_FROM /**/
#define	USE_DOMAIN /**/
/*#undef NOCHECK_VALIDNAME	/**/

/* PIDCHECK:
 *	This symbol, if defined, means that the kill(pid, 0) will
 *	check for an active pid.
 */
/* #define PIDCHECK		/**/

/* PORTABLE:
 *	This symbol, if defined, indicates to the C program that it should
 *	not assume that it is running on the machine it was compiled on.
 *	The program should be prepared to look up the host name, translate
 *	generic filenames, use PATH, etc.
 */
#define	PORTABLE	/**/

/* PTEM:
 *	This symbol, if defined, indicates that the sys/ptem.h include file is
 *	needed for window sizing.
 */
/*#undef	PTEM		/**/

/* REMOVE_AT_LAST:
 *	This symbol, if defined, tells the C code to remove the lock
 *	file on lock failure.
 */
/* MAX_ATTEMPTS:
 *	This symbol defines to the C code the number of times to try
 *	locking the mail file.
 */
/*#undef REMOVE_AT_LAST	/**/
#define MAX_ATTEMPTS	6

/* SAVE_GROUP_MAILBOX_ID:
 *	This symbol, if defined, indica;es that Elm needs to restore the
 *	group id of the file, as it is running setgid.
 */
/*#undef SAVE_GROUP_MAILBOX_ID	/**/

/* STRCSPN:
 *	This symbol, if defined, indicates that the strcspn() routine exists.
 */
#define	STRSPN		/**/

#define	STRCSPN		/**/

/* STRINGS:
 *	This symbol, if defined, indicates that the file strings.h
 *	should be included not string.h
 */
/* PWDINSYS:
 *	This symbol, if defined, indicates that the file pwd.h
 *	is in the sys sub directory
 */
/*#undef	STRINGS		/**/
/*#undef	PWDINSYS	/**/

/* ALLOW_SUBSHELL:
 *	This symbol, if defined, indicates that the '!' subshell
 *	function is to be allowed at various places.
 */
#define	ALLOW_SUBSHELL	/**/

/* TEMPNAM:
 *	This symbol, if defined, indicates that the tempnam() routine exists.
 */
#define	TEMPNAM		/**/

/* TERMIOS:
 *	This symbol, if defined, indicates that the program should include
 *	termios.h rather than sgtty.h or termio.h.  There are also differences
 *	in the ioctl() calls that depend on the value of this symbol.
 */
/* TERMIO:
 *	This symbol, if defined, indicates that the program should include
 *	termio.h rather than sgtty.h.  There are also differences in the
 *	ioctl() calls that depend on the value of this symbol.
 */
#define	TERMIOS		/**/

/*#undef	TERMIO		/**/

/* TZ_MINUTESWEST:
 *	This symbol is defined if this system uses tz_minutes west
 *	in time.h instead of timezone.  Only for BSD Systems
 */
#define	TZ_MINUTESWEST 	/**/

/* USE_EMBEDDED_ADDRESSES:
 *	This symbol, if defined, indicates that replyto: and from:
 *	headers can be trusted.
 */
#define USE_EMBEDDED_ADDRESSES	 /**/

/* NOUTIMBUF:
 *	This symbol, if defined, means to include our own struct utimbuf.
 */
/* #define NOUTIMBUF		/**/

/* VOIDSIG:
 *	This symbol is defined if this system declares "void (*signal())()" in
 *	signal.h.  The old way was to declare it as "int (*signal())()".  It
 *	is up to the package author to declare things correctly based on the
 *	symbol.
 */
#define	VOIDSIG 	/**/

/* MAX_SALIASES:
 *	This symbol defines the number of system wide aliases allowed.
 */
/* MAX_UALIASES:
 *	This symbol defines the number of per user aliases allowed.
 */
#define	MAX_SALIASES	503	/* number of system aliases allowed      */
#define	MAX_UALIASES	251	/* number of user aliases allowed 	 */

/* PASSNAMES:
 *	This symbol, if defined, indicates that full names are stored in
 *	the /etc/passwd file.
 */
/* BERKNAMES:
 *	This symbol, if defined, indicates that full names are stored in
 *	the /etc/passwd file in Berkeley format (name first thing, everything
 *	up to first comma, with & replaced by capitalized login id, yuck).
 */
/* USGNAMES:
 *	This symbol, if defined, indicates that full names are stored in
 *	the /etc/passwd file in USG format (everything after - and before ( is
 *	the name).
 */
#define	PASSNAMES /*  (undef to take name from ~/.fullname) */
/*#undef	BERKNAMES /* (that is, ":name,stuff:") */
/*#define	USGNAMES  /* (that is, ":stuff-name(stuff):") */

/* XENIX:
 *	This symbol, if defined, indicates this is a Xenix system,
 *	for knocking  out the far keyword in selected places.
 */
/* BSD:
 *	This symbol, if defined, indicates this is a BSD type system,
 */
/*#undef	XENIX	/**/
/*#undef	BSD	/**/

