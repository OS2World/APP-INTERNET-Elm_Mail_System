
static char rcsid[] = "@(#)$Id: errno.c,v 4.1 90/04/28 22:42:58 syd Exp $";

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
 * $Log:	errno.c,v $
 * Revision 4.1  90/04/28  22:42:58  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This routine maps error numbers to error names and error messages.
    These are all directly ripped out of the include file errno.h, and
    are HOPEFULLY standardized across the different breeds of Unix!!

    If (alas) yours are different, you should be able to use awk to
    mangle your errno.h file quite simply...

**/

#include "headers.h"

char *err_name[] = {
/* 0 */	        "NOERROR", "No error status currently",
/* 1 */		"EPERM",   "Not super-user",
/* 2 */		"ENOENT",  "No such file or directory",
/* 3 */		"ESRCH",   "No such process",
/* 4 */		"EINTR",   "Interrupted system call",
/* 5 */		"EIO",     "I/O error",
/* 6 */		"ENXIO",   "No such device or address",
/* 7 */		"E2BIG",   "Arg list too long",
/* 8 */		"ENOEXEC", "Exec format error",
/* 9 */		"EBADF",   "Bad file number",
/* 10 */	"ECHILD",  "No children",
/* 11 */	"EAGAIN",  "No more processes",
/* 12 */	"ENOMEM",  "Not enough core",
/* 13 */	"EACCES",  "Permission denied",
/* 14 */	"EFAULT",  "Bad address",
/* 15 */	"ENOTBLK", "Block device required",
/* 16 */	"EBUSY",   "Mount device busy",
/* 17 */	"EEXIST",  "File exists",
/* 18 */	"EXDEV",   "Cross-device link",
/* 19 */	"ENODEV",  "No such device",
/* 20 */	"ENOTDIR", "Not a directory",
/* 21 */	"EISDIR",  "Is a directory",
/* 22 */	"EINVAL",  "Invalid argument",
/* 23 */	"ENFILE",  "File table overflow",
/* 24 */	"EMFILE",  "Too many open files",
/* 25 */	"ENOTTY",  "Not a typewriter",
/* 26 */	"ETXTBSY", "Text file busy",
/* 27 */	"EFBIG",   "File too large",
/* 28 */	"ENOSPC",  "No space left on device",
/* 29 */	"ESPIPE",  "Illegal seek",
/* 30 */	"EROFS",   "Read only file system",
/* 31 */	"EMLINK",  "Too many links",
/* 32 */	"EPIPE",   "Broken pipe",
/* 33 */	"EDOM",    "Math arg out of domain of func",
/* 34 */	"ERANGE",  "Math result not representable",
/* 35 */	"ENOMSG",  "No message of desired type",
/* 36 */	"EIDRM",   "Identifier removed"
	};

char *strcpy();

char *error_name(errnumber)
int errnumber;
{
	static char buffer[50];

	if (errnumber < 0 || errnumber > 36)
	  sprintf(buffer,"ERR-UNKNOWN (%d)", errnumber);
	else
	  strcpy(buffer, err_name[2*errnumber]);

	return( (char *) buffer);
}

char *error_description(errnumber)
int errnumber;
{
	static char buffer[50];

	if (errnumber < 0 || errnumber > 36)
	  sprintf(buffer,"Unknown error - %d - No description", errnumber);
	else
	  strcpy(buffer, err_name[2*errnumber + 1]);

	return ( (char *) buffer);
}
