
static char rcsid[] = "@(#)$Id: leavembox.c,v 4.1.1.6 90/12/06 13:38:55 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.6 $   $State: Exp $
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
 * $Log:	leavembox.c,v $
 * Revision 4.1.1.6  90/12/06  13:38:55  syd
 * disable stop signal around writeback to avoid corrupted file
 * From: Syd via report from Tom Davis <tdd@endure.cl.msu.edu>
 *
 * Revision 4.1.1.5  90/10/10  12:49:46  syd
 * Fix calling sequence to copy_message calls for
 * new MMDF argument
 * From: Syd
 *
 * Revision 4.1.1.4  90/08/15  21:00:07  syd
 * Change elm to not delete empty folders on a resync
 * From: Syd
 *
 * Revision 4.1.1.3  90/06/21  22:51:52  syd
 * Add time.h to includes as some OSs include needed substructure only
 * from time.h
 * From: Syd
 *
 * Revision 4.1.1.2  90/06/21  22:48:14  syd
 * patch to fix up the Log headers.
 * From: pdc%lunch.wpd@sgi.com (Paul Close)
 *
 * Revision 4.1.1.1  90/06/09  21:33:23  syd
 * Some flock()s refuse to exclusively lock a fd open for read-only access.
 * From: pdc%lunch.wpd@sgi.com (Paul Close)
 *
 * Revision 4.1  90/04/28  22:43:18  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** leave current folder, updating etc. as needed...

**/

#include "headers.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef LOCK_BY_FLOCK
#include <sys/file.h>
#endif
#include <errno.h>
#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif


/**********
   Since a number of machines don't seem to bother to define the utimbuf
   structure for some *very* obscure reason....

   Suprise, though, BSD has a different utime() entirely...*sigh*
**********/

#ifndef BSD
# ifdef NOUTIMBUF

struct utimbuf {
	time_t	actime;		/** access time       **/
	time_t	modtime;	/** modification time **/
       };


# endif /* NOUTIMBUF */
#endif /* BSD */

#ifndef OS2
extern int errno;
#endif

char *error_name(), *error_description(), *strcpy(), *rindex();
unsigned short getegid();
#ifndef	_POSIX_SOURCE
unsigned long sleep();
#endif

int
leave_mbox(resyncing, quitting, prompt)
int resyncing, quitting, prompt;
{
	/** Close folder, deleting some messages, storing others in mbox,
	    and keeping others, as directed by user input and elmrc options.

	    Return	1	Folder altered
			0	Folder not altered
			-1	New mail arrived during the process and
					closing was aborted.
	    If "resyncing" we are just writing out folder to reopen it. We
		therefore only consider deletes and keeps, not stores to mbox.
		Also we don't remove NEW status so that it can be preserved
		across the resync.

	    If "quitting" and "prompt" is false, then no prompting is done.
		Otherwise prompting is dependent upon the variable
		question_me, as set by an elmrc option.  This behavior makes
		the 'q' command prompt just like 'c' and '$', while
		retaining the 'Q' command for a quick exit that never
		prompts.
	**/

	FILE *temp;
	char temp_keep_file[SLEN], buffer[SLEN];
	struct stat    buf;		/* stat command  */
#ifdef BSD
	time_t utime_buffer[2];		/* utime command */
#else
	struct utimbuf utime_buffer;	/* utime command */
#endif
#ifdef VOIDSIG
	void	(*oldstop)();
#else
	int	(*oldstop)();
#endif
	register int to_delete = 0, to_store = 0, to_keep = 0, i,
		     marked_deleted, marked_read, marked_unread,
		     last_sortby, ask_questions,  asked_storage_q,
		     num_chgd_status, need_to_copy;
	char answer;
	long bytes();

	dprint(1, (debugfile, "\n\n-- leaving folder --\n\n"));

	if (message_count == 0)
	  return(0);	/* nothing changed */

	ask_questions = ((quitting && !prompt) ? FALSE : question_me);

	/* YES or NO on softkeys */
	if (hp_softkeys && ask_questions) {
	  define_softkeys(YESNO);
	  softkeys_on();
	}

	/* Clear the exit dispositions of all messages, just in case
	 * they were left set by a previous call to this function
	 * that was interrupted by the receipt of new mail.
	 */
	for(i = 0; i < message_count; i++)
	  headers[i]->exit_disposition = UNSET;

	/* Determine if deleted messages are really to be deleted */

	/* we need to know if there are none, or one, or more to delete */
	for (marked_deleted=0, i=0; i<message_count && marked_deleted<2; i++)
	  if (ison(headers[i]->status, DELETED))
	    marked_deleted++;

        if(marked_deleted) {
	  answer = (always_del ? 'y' : 'n');	/* default answer */
	  if(ask_questions) {
	    sprintf(buffer, "Delete message%s? (y/n) ", plural(marked_deleted));
	    answer = want_to(buffer, answer);
	  }

	  if(answer == 'y') {
	    for (i = 0; i < message_count; i++) {
	      if (ison(headers[i]->status, DELETED)) {
		headers[i]->exit_disposition = DELETE;
		to_delete++;
	      }
	    }
	  }
	}
	dprint(3, (debugfile, "Messages to delete: %d\n", to_delete));

	/* If this is a non spool file, or if we are merely resyncing,
	 * all messages with an unset disposition (i.e. not slated for
	 * deletion) are to be kept.
	 * Otherwise, we need to determine if read and unread messages
	 * are to be stored or kept.
	 */
	if(folder_type == NON_SPOOL || resyncing) {
	  to_store = 0;
	  for (i = 0; i < message_count; i++) {
	    if(headers[i]->exit_disposition == UNSET) {
	      headers[i]->exit_disposition = KEEP;
	      to_keep++;
	    }
	  }
	} else {

	  /* Let's first see if user wants to store read messages
	   * that aren't slated for deletion */

	  asked_storage_q = FALSE;

	  /* we need to know if there are none, or one, or more marked read */
	  for (marked_read=0, i=0; i < message_count && marked_read < 2; i++) {
	    if((isoff(headers[i]->status, UNREAD))
	      && (headers[i]->exit_disposition == UNSET))
		marked_read++;
	  }
	  if(marked_read) {
	    answer = (always_store ? 'y' : 'n');	/* default answer */
	    if(ask_questions) {
	      sprintf(buffer, "Move read message%s to \"received\" folder? (y/n) ",
	        plural(marked_read));
	      answer = want_to(buffer, answer);
	      asked_storage_q = TRUE;
	    }

	    for (i = 0; i < message_count; i++) {
	      if((isoff(headers[i]->status, UNREAD))
		&& (headers[i]->exit_disposition == UNSET)) {

		  if(answer == 'y') {
		    headers[i]->exit_disposition = STORE;
		    to_store++;
		  } else {
		    headers[i]->exit_disposition = KEEP;
		    to_keep++;
		  }
	      }
	    }
	  }

	  /* If we asked the user if read messages should be stored,
	   * and if the user wanted them kept instead, then certainly the
	   * user would want the unread messages kept as well.
	   */
	  if(asked_storage_q && answer == 'n') {

	    for (i = 0; i < message_count; i++) {
	      if((ison(headers[i]->status, UNREAD))
		&& (headers[i]->exit_disposition == UNSET)) {
		  headers[i]->exit_disposition = KEEP;
		  to_keep++;
	      }
	    }

	  } else {

	    /* Determine if unread messages are to be kept */

	    /* we need to know if there are none, or one, or more unread */
	    for (marked_unread=0, i=0; i<message_count && marked_unread<2; i++)
	      if((ison(headers[i]->status, UNREAD))
		&& (headers[i]->exit_disposition == UNSET))
		  marked_unread++;

	    if(marked_unread) {
	      answer = (always_keep ? 'y' : 'n');	/* default answer */
	      if(ask_questions) {
		sprintf(buffer,
		  "Keep unread message%s in incoming mailbox? (y/n) ",
		  plural(marked_unread));
		answer = want_to(buffer, answer);
	      }

	      for (i = 0; i < message_count; i++) {
		if((ison(headers[i]->status, UNREAD))
		  && (headers[i]->exit_disposition == UNSET)) {

		    if(answer == 'n') {
		      headers[i]->exit_disposition = STORE;
		      to_store++;
		    } else {
		      headers[i]->exit_disposition = KEEP;
		      to_keep++;
		    }

		}
	      }
	    }
	  }
	}

	dprint(3, (debugfile, "Messages to store: %d\n", to_store));
	dprint(3, (debugfile, "Messages to keep: %d\n", to_keep));

	if(to_delete + to_store + to_keep != message_count) {
	  dprint(1, (debugfile,
	  "Error: %d to delete + %d to store + %d to keep != %d message cnt\n",
	    to_delete, to_store, to_keep, message_count));
	  error("Something wrong in message counts! Folder unchanged.");
	  emergency_exit();
	}


	/* If we are not resyncing, we are leaving the mailfile and
	 * the new messages are new no longer. Note that this changes
	 * their status.
	 */
	if(!resyncing) {
	  for (i = 0; i < message_count; i++) {
	    if (ison(headers[i]->status, NEW)) {
	      clearit(headers[i]->status, NEW);
	      headers[i]->status_chgd = TRUE;
	    }
	  }
	}

	/* If all messages are to be kept and none have changed status
	 * we don't need to do anything because the current folder won't
	 * be changed by our writing it out - unless we are resyncing, in
	 * which case we force the writing out of the mailfile.
	 */

	for (num_chgd_status = 0, i = 0; i < message_count; i++)
	  if(headers[i]->status_chgd == TRUE)
	    num_chgd_status++;

	if(!to_delete && !to_store && !num_chgd_status && !resyncing) {
	  dprint(3, (debugfile, "Folder keep as is!\n"));
	  error("Folder unchanged.");
	  return(0);
	}

	/** we have to check to see what the sorting order was...so that
	    the order in which we write messages is the same as the order
	    of the messages originally.
	    We only need to do this if there are any messages to be
	    written out (either to keep or to store). **/

	if ((to_keep || to_store ) && sortby != MAILBOX_ORDER) {
	  last_sortby = sortby;
	  sortby = MAILBOX_ORDER;
	  sort_mailbox(message_count, FALSE);
	  sortby = last_sortby;
	}

	/* Formulate message as to number of keeps, stores, and deletes.
	 * This is only complex so that the message is good English.
	 */
	if (to_keep > 0) {
	  if (to_store > 0) {
	    if (to_delete > 0)
	      sprintf(buffer,
	            "[Keeping %d message%s, storing %d, and deleting %d.]",
		    to_keep, plural(to_keep), to_store, to_delete);
	    else
	      sprintf(buffer, "[Keeping %d message%s and storing %d.]",
		    to_keep, plural(to_keep), to_store);
	  } else {
	    if (to_delete > 0)
	      sprintf(buffer, "[Keeping %d message%s and deleting %d.]",
		    to_keep, plural(to_keep), to_delete);
	    else
	      sprintf(buffer, "[Keeping %s.]",
		    to_keep > 1 ? "all messages" : "message");
	  }
	} else if (to_store > 0) {
	  if (to_delete > 0)
	    sprintf(buffer, "[Storing %d message%s and deleting %d.]",
		  to_store, plural(to_store), to_delete);
	  else
	    sprintf(buffer, "[Storing %s.]",
		  to_store > 1? "all messages" : "message");

	} else {
	  if (to_delete > 0)
	    sprintf(buffer, "[Deleting all messages.]");
	  else
	    buffer[0] = '\0';
	}
	/* NOTE: don't use variable "buffer" till message is output later */

	/** next, let's lock the file up and make one last size check **/

	if (folder_type == SPOOL)
	  lock(OUTGOING);

	if (mailfile_size != bytes(cur_folder)) {
	  unlock();
	  error("New mail has just arrived. Resynchronizing...");
	  return(-1);
	}

	/* Everything's GO - so ouput that user message and go to it. */

#ifdef SIGTSTP
	oldstop = signal(SIGTSTP, SIGIGN);
#endif

	dprint(2, (debugfile, "Action: %s\n", buffer));
	error(buffer);

	/* Store messages slated for storage in received mail folder */
	if (to_store > 0) {
	  if ((errno = can_open(recvd_mail, "a"))) {
	    error1(
	      "Permission to append to %s denied!  Leaving folder intact.\n",
	      recvd_mail);
	    dprint(1, (debugfile,
	      "Error: Permission to append to folder %s denied!! (%s)\n",
	      recvd_mail, "leavembox"));
	    dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
	      error_description(errno)));
	    unlock();
#ifdef SIGTSTP
	    signal(SIGTSTP, oldstop);
#endif
	    return(0);
	  }
	  if ((temp = fopen(recvd_mail,"a")) == NULL) {
	    unlock();
	    dprint(1, (debugfile, "Error: could not append to file %s\n",
	      recvd_mail));
	    dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
	      error_description(errno)));
	    sprintf(buffer, "Could not append to folder %s!", recvd_mail);
	    Centerline(LINES-1, buffer);
	    emergency_exit();
	  }
	  dprint(2, (debugfile, "Storing message%s ", plural(to_store)));
	  for (i = 0; i < message_count; i++) {
	    if(headers[i]->exit_disposition == STORE) {
	      current = i+1;
	      dprint(2, (debugfile, "#%d, ", current));
            copy_message("", temp, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE);
	    }
	  }
	  fclose(temp);
	  dprint(2, (debugfile, "\n\n"));
	  chown(recvd_mail, userid, groupid);
	}

	/* If there are any messages to keep, first copy them to a
	 * temp file, then remove original and copy whole temp file over.
	 */
	if (to_keep > 0) {
	  sprintf(temp_keep_file, "%s%d%s", temp_dir, getpid(), temp_file);
	  if ((errno = can_open(temp_keep_file, "w"))) {
	    error1(
"Permission to create temp file %s for writing denied! Leaving folder intact.",
	      temp_keep_file);
	    dprint(1, (debugfile,
	      "Error: Permission to create temp file %s denied!! (%s)\n",
	      temp_keep_file, "leavembox"));
	    dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
	      error_description(errno)));
	    unlock();
#ifdef SIGTSTP
	    signal(SIGTSTP, oldstop);
#endif
	    return(0);
	  }
	  if ((temp = fopen(temp_keep_file,"w")) == NULL) {
	    unlock();
	    dprint(1, (debugfile, "Error: could not create file %s\n",
	      temp_keep_file));
	    dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
	      error_description(errno)));
	    sprintf(buffer, "Could not create temp file %s!", temp_keep_file);
	    Centerline(LINES-1, buffer);
	    emergency_exit();
	  }
	  dprint(2, (debugfile, "Copying to temp file message%s to be kept ",
	    plural(to_keep)));
	  for (i = 0; i < message_count; i++) {
	    if(headers[i]->exit_disposition == KEEP) {
	      current = i+1;
	      dprint(2, (debugfile, "#%d, ", current));
            copy_message("", temp, FALSE, FALSE, TRUE, FALSE, FALSE, FALSE);
	    }
	  }
	  if ( fclose(temp) == EOF ) {
	    Write_to_screen("\n\rClose failed on temp keep file in leavembox\n\r", 0);
	    perror(temp_keep_file);
	    dprint(2, (debugfile, "\n\rfclose err on temp_keep_file - leavembox\n\r"));
	    rm_temps_exit();
	  }
	  dprint(2, (debugfile, "\n\n"));

	} else if (folder_type == NON_SPOOL && !keep_empty_files && !resyncing) {

	  /* i.e. if no messages were to be kept and this is not a spool
	   * folder and we aren't keeping empty non-spool folders,
	   * simply remove the old original folder and that's it!
	   */
          fclose(mailfile);
	  (void)unlink(cur_folder);
#ifdef SIGTSTP
	  signal(SIGTSTP, oldstop);
#endif
	  return(1);
	}

	/* Otherwise we have some work left to do! */

	/* Get original permissions and access time of the original
	 * mail folder before we remove it.
	 */
	if(save_file_stats(cur_folder) != 0) {
	  error1("Problems saving permissions of folder %s!", cur_folder);
	  sleep(2);
	}

        if (stat(cur_folder, &buf) != 0) {
	  dprint(1, (debugfile, "Error: errno %s attempting to stat file %s\n",
		     error_name(errno), cur_folder));
          error3("Error %s (%s) on stat(%s).", error_name(errno),
		error_description(errno), cur_folder);
	}

	/* Close and remove the original folder.
	 * However, if we are going to copy a temp file of kept messages
	 * to it, and this is a locked (spool) mailbox, we need to keep
	 * it locked during this process. Unfortunately,
	 * if we did our LOCK_BY_FLOCK, unlinking the original will kill the
	 * lock, so we have to resort to copying the temp file to the original
	 * file while keeping the original open.
	 * Also, if the file has a link count > 1, then it has links, so to
	 * prevent destroying the links, we do a copy back, even though its
	 * slower.
	 */

	fclose(mailfile);

	if(to_keep) {
#ifdef LOCK_BY_FLOCK
	  need_to_copy = (folder_type == SPOOL ? TRUE : FALSE);
#else
	  need_to_copy = FALSE;
#endif
	  if (buf.st_nlink > 1)
	    need_to_copy = TRUE;

	  if(!need_to_copy) {
            fclose(mailfile);
	    unlink(cur_folder);
	    if (link(temp_keep_file, cur_folder) != 0) {
	      if(errno == EXDEV || errno == EEXIST) {
		/* oops - can't link across file systems - use copy instead */
		need_to_copy = TRUE;
	      } else {
		dprint(1, (debugfile, "link(%s, %s) failed (leavembox)\n",
		       temp_keep_file, cur_folder));
		dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
			  error_description(errno)));
		error2("Link failed! %s - %s.", error_name(errno),
		  error_description(errno));
		unlock();
		emergency_exit();
	      }
	    }
	  }

	  if(need_to_copy) {

	    if (copy(temp_keep_file, cur_folder) != 0) {

	      /* copy to cur_folder failed - try to copy to special file */
	      dprint(1, (debugfile, "leavembox: copy(%s, %s) failed;",
		      temp_keep_file, cur_folder));
	      dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
		   error_description(errno)));
	      error("Couldn't modify folder!");
	      sleep(1);
	      sprintf(cur_folder,"%s/%s", home, unedited_mail);
	      if (copy(temp_keep_file, cur_folder) != 0) {

		/* couldn't copy to special file either */
		dprint(1, (debugfile,
			"leavembox: couldn't copy to %s either!!  Help;",
			cur_folder));
		dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
			error_description(errno)));
		error("Can't copy mailbox, system trouble!!!");
		unlock();
		emergency_exit();
	      } else {
		dprint(1, (debugfile,
			"\nWoah! Confused - Saved mail in %s (leavembox)\n",
			cur_folder));
		error1("Saved mail in %s.", cur_folder);
		sleep(1);
	      }
	    }
	  }

	  /* link or copy complete - remove temp keep file */
	  unlink(temp_keep_file);

	} else if(folder_type == SPOOL || keep_empty_files || resyncing) {

	  /* if this is an empty spool file, or if this is an empty non spool
	   * file and we keep empty non spool files (we always keep empty
	   * spool files), create an empty file */

	  if(folder_type == NON_SPOOL)
	    error1("Keeping empty folder '%s'.", cur_folder);
	  temp = fopen(cur_folder, "w");
	  fclose(temp);
	}

	/* restore permissions and access times of folder */

	if(restore_file_stats(cur_folder) != 1) {
	  error1("Problems restoring permissions of folder %s!", cur_folder);
	  sleep(2);
	}

#ifdef BSD
	utime_buffer[0]     = buf.st_atime;
	utime_buffer[1]     = buf.st_mtime;
#else
	utime_buffer.actime = buf.st_atime;
	utime_buffer.modtime= buf.st_mtime;
#endif

#ifdef BSD
	if (utime(cur_folder, utime_buffer) != 0) {
#else
	if (utime(cur_folder, &utime_buffer) != 0) {
#endif
	  dprint(1, (debugfile,
		 "Error: encountered error doing utime (leavmbox)\n"));
	  dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
		   error_description(errno)));
	  error2("Error %s trying to change file %s access time.",
		   error_name(errno), cur_folder);
	}


	mailfile_size = bytes(cur_folder);
	unlock();	/* remove the lock on the file ASAP! */

#ifdef SIGTSTP
	signal(SIGTSTP, oldstop);
#endif
	return(1);
}

static int  lock_state = OFF;

static char lock_name[SLEN];

char *
mk_lockname(file_to_lock)
char *file_to_lock;
{
	/** Create the proper name of the lock file for file_to_lock,
	    which is presumed to be a spool file full path (see
	    get_folder_type()), and put it in the static area lock_name.
	    Return lock_name for informational purposes.
	 **/
	char *ptr;

#ifdef XENIX
	/* lock is /tmp/[basename of file_to_lock].mlk */
	sprintf(lock_name, "/tmp/%.10s.mlk", rindex(file_to_lock, '/')+1);
#else
	/* lock is [file_to_lock].lock */
	strcpy(lock_name, file_to_lock);
#ifdef OS2
	if ( (ptr = strrchr(lock_name, '.')) != NULL
	     && strcmp(ptr, mailext) == 0 )
	  *ptr = 0;
#endif
	strcat(lock_name, ".lck");
#endif
	return(lock_name);
}


static int flock_fd,	/* file descriptor for flocking mailbox itself */
	   create_fd;	/* file descriptor for creating lock file */

lock(direction)
int direction;
{
      /** Create lock file to ensure that we don't get any mail
	  while altering the folder contents!
	  If it already exists sit and spin until
	     either the lock file is removed...indicating new mail
	  or
	     we have iterated MAX_ATTEMPTS times, in which case we
	     either fail or remove it and make our own (determined
	     by if REMOVE_AT_LAST is defined in header file

	  If direction == INCOMING then DON'T remove the lock file
	  on the way out!  (It'd mess up whatever created it!).

	  But if that succeeds and if we are also locking by flock(),
	  follow a similar algorithm. Now if we can't lock by flock(),
	  we DO need to remove the lock file, since if we got this far,
	  we DID create it, not another process.
      **/

      register int create_iteration = 0,
		   flock_iteration = 0;
      char pid_buffer[SHORT];

#ifndef	LOCK_FLOCK_ONLY		/* { LOCK_FLOCK_ONLY	*/
      /* formulate lock file name */
      mk_lockname(cur_folder);

#ifdef PIDCHECK
      /** first, try to read the lock file, and if possible, check the pid.
	  If we can validate that the pid is no longer active, then remove
	  the lock file.
       **/
      if((create_fd=open(lock_name,O_RDONLY)) != -1) {
	if (read(create_fd, pid_buffer, SHORT) > 0) {
	  create_iteration = atoi(pid_buffer);
	  if (create_iteration) {
	    if (kill(create_iteration, 0)) {
	      close(create_fd);
	      if (unlink(lock_name) != 0) {
		dprint(1, (debugfile,
		  "Error %s (%s)\n\ttrying to unlink file %s (%s)\n",
		  error_name(errno), error_description(errno), lock_name, "lock"));
		PutLine1(LINES, 0,
		  "\n\rCouldn't remove the current lock file %s\n\r", lock_name);
		PutLine2(LINES, 0, "** %s - %s **\n\r", error_name(errno),
		  error_description(errno));
		if (direction == INCOMING)
		  leave();
		else
		  emergency_exit();
	      }
	    }
	  }
	}
	create_iteration = 0;
      }
#endif

      /* try to assert create lock file MAX_ATTEMPTS times */
      do {

	errno = 0;
	if((create_fd=open(lock_name,O_WRONLY | O_CREAT | O_EXCL,0666)) != -1)
	  break;
	else {
	  if(errno != EEXIST) {
	    /* Creation of lock failed NOT because it already exists!!! */

	    if (direction == OUTGOING) {
	      dprint(1, (debugfile,
		"Error encountered attempting to create lock %s\n", lock_name));
	      dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
		    error_description(errno)));
	      MoveCursor(LINES, 0);
	      printf(
	   "\n\rError encountered while attempting to create lock file %s;\n\r",
		lock_name);
	      printf("** %s - %s.**\n\r\n\r",
		error_name(errno), error_description(errno));
	    } else {	/* incoming - permission denied in the middle?  Odd. */
	      dprint(1, (debugfile,
	       "Can't create lock file: creat(%s) raises error %s (lock)\n",
		lock_name, error_name(errno)));
	      error1(
	       "Can't create lock file! Need write permission in \"%s\".\n\r",
		mailhome);
	    }
	    leave();
	  }
	}
	dprint(2, (debugfile,"File '%s' already exists!  Waiting...(lock)\n",
	  lock_name));
	error1(
	  "Waiting to read mailbox while mail is being received: attempt #%d",
	  create_iteration);
	sleep(5);
      } while (create_iteration++ < MAX_ATTEMPTS);
      clear_error();

      if(errno != 0 && errno != ENOENT) {

	/* we weren't able to create the lock file */

#ifdef REMOVE_AT_LAST

	/** time to waste the lock file!  Must be there in error! **/
	dprint(2, (debugfile,
	   "Warning: I'm giving up waiting - removing lock file(lock)\n"));
	if (direction == INCOMING)
	  PutLine0(LINES, 0,"\nTimed out - removing current lock file...");
	else
	  error("Throwing away the current lock file!");

	if (unlink(lock_name) != 0) {
	  dprint(1, (debugfile,
	    "Error %s (%s)\n\ttrying to unlink file %s (%s)\n",
	    error_name(errno), error_description(errno), lock_name, "lock"));
	  PutLine1(LINES, 0,
	    "\n\rCouldn't remove the current lock file %s\n\r", lock_name);
	  PutLine2(LINES, 0, "** %s - %s **\n\r", error_name(errno),
	    error_description(errno));
	  if (direction == INCOMING)
	    leave();
	  else
	    emergency_exit();
	}

	/* we've removed the bad lock, let's try to assert lock once more */
	if((create_fd=open(lock_name,O_WRONLY | O_CREAT | O_EXCL,0666)) == -1){

	  /* still can't lock it - just give up */
	  dprint(1, (debugfile,
	    "Error encountered attempting to create lock %s\n", lock_name));
	  dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
	    error_description(errno)));
	  MoveCursor(LINES, 0);
	  printf(
	  "\n\rError encountered while attempting to create lock file %s;\n\r",
	    lock_name);
	  printf("** %s - %s.**\n\r\n\r", error_name(errno),
	    error_description(errno));
	  leave();
	}
#else
	/* Okay...we die and leave, not updating the mailfile mbox or
	   any of those! */

	if (direction == INCOMING) {
	  PutLine1(LINES, 0, "\n\r\n\rGiving up after %d iterations.\n\r",
	    create_iteration);
	  PutLine0(LINES, 0,
	  "\n\rPlease try to read your mail again in a few minutes.\n\r\n\r");
	  dprint(1, (debugfile,
	    "Warning: bailing out after %d iterations...(lock)\n",
	    create_iteration));
	  leave_locked(0);
	} else {
	  dprint(1, (debugfile,
	   "Warning: after %d iterations, timed out! (lock)\n",
	   create_iteration));
	  leave(error("Timed out on locking mailbox.  Leaving program."));
	}
#endif
      }

      /* If we're here we successfully created the lock file */
      dprint(5,
	(debugfile, "Lock %s %s for file %s on.\n", lock_name,
	(direction == INCOMING ? "incoming" : "outgoing"), cur_folder));

      /* Place the pid of Elm into the lock file for SVR3.2 and its ilk */
      sprintf(pid_buffer, "%d", getpid());
      write(create_fd, pid_buffer, strlen(pid_buffer));

      (void)close(create_fd);
#endif				/* } LOCK_FLOCK_ONLY */

#ifdef LOCK_BY_FLOCK
      /* Now we also need to lock the file with flock(2) */

      /* Open mail file separately for locking */
      if((flock_fd = open(cur_folder, O_RDWR)) < 0) {
	dprint(1, (debugfile,
	    "Error encountered attempting to reopen %s for lock\n", cur_folder));
	dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
	    error_description(errno)));
	MoveCursor(LINES, 0);
	printf(
 "\n\rError encountered while attempting to reopen mailbox %s for lock;\n\r",
	      cur_folder);
	printf("** %s - %s.**\n\r\n\r", error_name(errno),
	      error_description(errno));
	(void)unlink(lock_name);
	leave();
      }

      /* try to assert lock MAX_ATTEMPTS times */
      do {

	errno = 0;
	if(flock(flock_fd, LOCK_NB | LOCK_EX) != -1)
	  break;
	else {
	  if(errno != EWOULDBLOCK && errno != EAGAIN) {

	    /* Creation of lock failed NOT because it already exists!!! */

	    dprint(1, (debugfile,
		  "Error encountered attempting to flock %s\n", cur_folder));
	    dprint(1, (debugfile, "** %s - %s **\n", error_name(errno),
		  error_description(errno)));
	    MoveCursor(LINES, 0);
	    printf(
	 "\n\rError encountered while attempting to flock mailbox %s;\n\r",
		  cur_folder);
	    printf("** %s - %s.**\n\r\n\r", error_name(errno),
		  error_description(errno));
	    (void)unlink(lock_name);
	    leave();
	  }
	}
	dprint(2, (debugfile,
	  "Mailbox '%s' already locked!  Waiting...(lock)\n", cur_folder));
	error1(
	  "Waiting to read mailbox while mail is being received: attempt #%d",
	  flock_iteration);
	sleep(5);
      } while (flock_iteration++ < MAX_ATTEMPTS);
      clear_error();

      if(errno != 0) {

	/* We couldn't lock the file. We die and leave not updating
	 * the mailfile mbox or any of those! */

	if (direction == INCOMING) {
	  PutLine1(LINES, 0, "\n\r\n\rGiving up after %d iterations.\n\r",
	    flock_iteration);
	  PutLine0(LINES, 0,
	  "\n\rPlease try to read your mail again in a few minutes.\n\r\n\r");
	  dprint(1, (debugfile,
	    "Warning: bailing out after %d iterations...(lock)\n",
	    flock_iteration));
	} else {
	  dprint(1, (debugfile,
	   "Warning: after %d iterations, timed out! (lock)\n",
	   flock_iteration));
	}
#ifndef	LOCK_FLOCK_ONLY
	(void)unlink(lock_name);
#endif
	leave(error("Timed out on locking mailbox. Leaving program."));
      }

      /* We locked the file */
      dprint(5,
	(debugfile, "Lock %s on file %s on.\n",
	(direction == INCOMING ? "incoming" : "outgoing"), cur_folder));
#endif

      dprint(5,
	(debugfile, "Lock %s for file %s on successfully.\n",
	(direction == INCOMING ? "incoming" : "outgoing"), cur_folder));
      lock_state = ON;
      return(0);
}

int
unlock()
{
	/** Remove the lock file!    This must be part of the interrupt
	    processing routine to ensure that the lock file is NEVER
	    left sitting in the mailhome directory!

	    If also using flock(), remove the file lock as well.
	 **/

	int retcode = 0;

	dprint(5,
	  (debugfile, "Lock %s for file %s %s off.\n",
	    (*lock_name ? lock_name : "none"), cur_folder,
	    (lock_state == ON ? "going" : "already")));

	if(lock_state == ON) {

#ifdef LOCK_BY_FLOCK
	  if((retcode = flock(flock_fd, LOCK_UN)) == -1) {
	    dprint(1, (debugfile,
	      "Error %s (%s)\n\ttrying to unlock file %s (%s)\n",
	      error_name(errno), error_description(errno), cur_folder, "unlock"));

	    /* try to force unlock by closing file */
	    if(close(flock_fd) == -1) {
	      dprint(1, (debugfile,
	  "Error %s (%s)\n\ttrying to force unlock file %s via close() (%s)\n",
	      error_name(errno), error_description(errno), cur_folder, "unlock"));
	      error1("Couldn't unlock my own mailbox %s!", cur_folder);
	      return(retcode);
	    }
	  }
	  (void)close(flock_fd);
#endif
#ifdef	LOCK_FLOCK_ONLY	/* { LOCK_FLOCK_ONLY */
	  *lock_name = '\0';		/* null lock file name */
	   lock_state = OFF;		/* indicate we don't have a lock on */
#else
	  if((retcode = unlink(lock_name)) == 0) {	/* remove lock file */
	    *lock_name = '\0';		/* null lock file name */
	    lock_state = OFF;		/* indicate we don't have a lock on */
	  } else {
	    dprint(1, (debugfile,
	      "Error %s (%s)\n\ttrying to unlink file %s (%s)\n",
	      error_name(errno), error_description(errno), lock_name,"unlock"));
	      error1("Couldn't remove my own lock file %s!", lock_name);
	  }
#endif	/* } LOCK_FLOCK_ONLY */
	}
	return(retcode);
}
