
static char rcsid[] ="@(#)$Id: lock.c,v 4.1.1.3 90/06/21 23:51:34 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.3 $   $State: Exp $
 *
 * 			Copyright (c) 1986, 1987 Dave Taylor
 * 			Copyright (c) 1988, 1989, 1990 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein - elm@DSI.COM
 *			dsinc!elm
 *
 *******************************************************************************
 * $Log:	lock.c,v $
 * Revision 4.1.1.3  90/06/21  23:51:34  syd
 * Fix typo
 *
 * Revision 4.1.1.2  90/06/21  23:02:37  syd
 * Make lock use same name as elm
 * From: Edwin WIles
 *
 * Revision 4.1.1.1  90/06/05  20:28:53  syd
 * The open system call in actions.c for EMERGENCY_MAILBOX and EMER_MBOX
 * were tested with the inequality >= 0 exactly backwards.
 * If the user's system mail box (/usr/spool/mail/user_id) is
 * removed the attempt of filter to flock it fails.  If it does not exist then
 * it should create it and then lock it.
 * From: john@hopf.math.nwu.edu (John Franks)
 *
 * Revision 4.1  90/04/28  22:41:57  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/


/** The lock() and unlock() routines herein duplicate exactly the
    equivalent routines in the Elm Mail System, and should also be
    compatible with sendmail, rmail, etc etc.


**/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "defs.h"
#include "filter.h"

static  int  we_locked_it;
static  char lockfile[SLEN];

#ifdef	LOCK_BY_FLOCK
#include <sys/types.h>
#include <sys/file.h>
static	flock_fd = -1;
static	char flock_name[SLEN];
#endif

char *
mk_lockname(home, user)
char *home, *user;
{
	/** Create the proper name of the lock file for file_to_lock.
	    Return lock_name for informational purposes.
	 **/

#ifdef XENIX
	/* lock is /tmp/[basename of file_to_lock].mlk */
	sprintf(lockfile, "/tmp/%.10s.mlk", user);
#else
	/* lock is [file_to_lock].lock */
	sprintf(lockfile, "%s%s.lck", home, user);
#endif
	return(lockfile);
}


int
lock()
{
	/** This routine will return 1 if we could lock the mailfile,
	    zero otherwise.
	**/

	int attempts = 0, ret;

#ifndef	LOCK_FLOCK_ONLY			/* { !LOCK_FLOCK_ONLY	*/
	mk_lockname(mailhome, username);
#ifdef PIDCHECK
	/** first, try to read the lock file, and if possible, check the pid.
	    If we can validate that the pid is no longer active, then remove
	    the lock file.
	**/
	if((ret=open(lockfile,O_RDONLY)) != -1) {
	  char pid_buffer[SHORT];
	  if (read(ret, pid_buffer, SHORT) > 0) {
	    attempts = atoi(pid_buffer);
	    if (attempts) {
	      if (kill(attempts, 0)) {
	        close(ret);
	        if (unlink(lockfile) != 0)
		  return(1);
	      }
	    }
	  }
	  attempts = 0;
        }
#endif

	while ((ret = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0666)) < 0
	       && attempts++ < 10) {
	  sleep(3);	/* wait three seconds each pass, okay?? */
	}

	if (ret >= 0) {
	  we_locked_it++;
	  close(ret);			/* no need to keep it open! */
	  ret = 1;
	} else {
	  ret = 0;
	}

#endif					/* } !LOCK_FLOCK_ONLY	*/
#ifdef	LOCK_BY_FLOCK			/* { LOCK_BY_FLOCK	*/
	(void)sprintf(flock_name,"%s%s",mailhome,username);
	flock_fd = open(flock_name, O_RDONLY | O_CREAT, 0666);
	if ( flock_fd >= 0 )
	  for (attempts = 0; attempts < 10; attempts++) {
	    if ( (ret = flock(flock_fd,LOCK_NB|LOCK_EX)) != -1 )
	        break;
	    if ( errno != EWOULDBLOCK && errno != EAGAIN )
	        break;
	    (void)sleep((unsigned)3);
	  }
	if ( flock_fd >= 0 && ret == 0 ) {
	    we_locked_it++;
	    ret = 1;
	} else {
	    we_locked_it = 0;
	    if ( lockfile[0] ) {
	    	(void)unlink(lockfile);
		lockfile[0] = 0;
	    }
	    if ( flock_fd >= 0 ) {
	    	(void)close(flock_fd);
	    	flock_fd = -1;
	    }
	    ret = 0;
	}
#endif
	return(ret);
}

unlock()
{
	/** this routine will remove the lock file, but only if we were
	    the people that locked it in the first place... **/

#ifndef	LOCK_FLOCK_ONLY
	if (we_locked_it && lockfile[0]) {
	  unlink(lockfile);	/* blamo! */
	  lockfile[0] = 0;
	}
#endif
#ifdef	LOCK_BY_FLOCK
	if (we_locked_it && flock_fd >= 0) {
	  (void)close(flock_fd);
	  flock_fd = -1;
	}
#endif
	we_locked_it = 0;
}
