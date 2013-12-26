
static char rcsid[] ="@(#)$Id: actions.c,v 4.1.1.2 90/10/07 20:36:41 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.2 $   $State: Exp $
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
 * $Log:	actions.c,v $
 * Revision 4.1.1.2  90/10/07  20:36:41  syd
 * allow non-elm mailers to correctly parse filter's folders.
 * From: sane!genmri!doug@crdgw1.ge.com (Doug Becker)
 *
 * Revision 4.1.1.1  90/06/05  20:28:51  syd
 * The open system call in actions.c for EMERGENCY_MAILBOX and EMER_MBOX
 * were tested with the inequality >= 0 exactly backwards.
 * If the user's system mail box (/usr/spool/mail/user_id) is
 * removed the attempt of filter to flock it fails.  If it does not exist then
 * it should create it and then lock it.
 * From: john@hopf.math.nwu.edu (John Franks)
 *
 * Revision 4.1  90/04/28  22:41:53  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/


/** RESULT oriented routines *chuckle*.  These routines implement the
    actions that result from either a specified rule being true or from
    the default action being taken.
**/

#include <stdio.h>
#include <pwd.h>
#include <ctype.h>
#include <fcntl.h>

#include "defs.h"
#include "filter.h"

FILE *emergency_local_delivery();

mail_message(address)
char *address;
{
	/** Called with an address to send mail to.   For various reasons
	    that are too disgusting to go into herein, we're going to actually
	    open the users mailbox and by hand add this message.  Yech.
	    NOTE, of course, that if we're going to MAIL the message to someone
	    else, that we'll try to do nice things with it on the fly...
	**/

	FILE *pipefd, *tempfd, *mailfd;
	int  in_header = TRUE, line_count = 0, mailunit;
	char tempfile[SLEN], mailbox[SLEN], lockfile[SLEN],
	     buffer[VERY_LONG_STRING];

	if (verbose && ! log_actions_only && outfd != NULL)
	  fprintf(outfd, "filter (%s): Mailing message to %s\n",
		   username, address);

	if (! show_only) {
	  sprintf(tempfile, "%s%d.fil", tempdir, getpid());

	  if ((tempfd = fopen(tempfile, "r")) == NULL) {
	    if (outfd != NULL)
	      fprintf(outfd, "filter (%s): Can't open temp file %s!!\n",
		    username, tempfile);
	    if (outfd != NULL) fclose(outfd);
	    exit(1);
	  }

	  if (strcmp(address, username) != 0) {	/* mailing to someone else */

	    if (already_been_forwarded) {	/* potential looping! */
	      if (contains(from, username)) {
		if (outfd != NULL)
	          fprintf(outfd,
	"filter (%s): Filter loop detected!  Message left in file %s\n",
			username, tempfile);
	        if (outfd != NULL) fclose(outfd);
	        exit(0);
	      }
	    }

	    if (strcmp(sendmail, mailer) == 0)
	      sprintf(buffer, "%s -f %s@%s %s", sendmail, username, hostfromname, address);
	    else
	      sprintf(buffer, "%s -t", mailer);

	    if ((pipefd = popen(buffer, "w")) == NULL) {
	      if (outfd != NULL)
	        fprintf(outfd, "filter (%s): popen %s failed!\n", username, buffer);
	      sprintf(buffer, "%s -t <%s", mailer, tempfile);
	      system(buffer);
              unlink(tempfile);
	      return;
	    }

	    fprintf(pipefd, "Subject: \"%s\"\n", subject);
	    fprintf(pipefd, "From: %s@%s (The Filter of %s)\n",
		    username, hostfromname, username);
	    fprintf(pipefd, "To: %s\n", address);
	    fprintf(pipefd, "X-Filtered-By: filter, version %s\n\n", VERSION);

	    fprintf(pipefd, "-- Begin filtered message --\n\n");

	    while (fgets(buffer, SLEN, tempfd) != NULL)
	      if (already_been_forwarded && in_header)
	        in_header = (strlen(buffer) == 1? 0 : in_header);
	      else
	        fprintf(pipefd," %s", buffer);

	    fprintf(pipefd, "\n-- End of filtered message --\n");
	    pclose(pipefd);
	    fclose(tempfd);

	    return;		/* YEAH!  Wot a slick program, eh? */

	  }

	  /** OTHERWISE it is to the current user... **/

	  sprintf(mailbox,  "%s%s", mailhome, username);

	  if (! lock()) {
	    if (outfd != NULL) {
	      fprintf(outfd, "filter (%s): Couldn't create lockfile %s\n",
		    username, lockfile);
	      fprintf(outfd, "filter (%s): Can't open mailbox %s!\n",
			username, mailbox);
	    }
	    if ((mailfd = emergency_local_delivery()) == NULL)
	      exit(1);
	  }
	  else if ((mailunit = open(mailbox, O_APPEND | O_WRONLY | O_CREAT, 0600)) >= 0)
	    mailfd = fdopen(mailunit, "a");
	  else if ((mailfd = emergency_local_delivery()) == NULL)
	    exit(1);

#ifdef MMDF
	  fputs(MSG_SEPERATOR, mailfd);
#endif

	  while (fgets(buffer, sizeof(buffer), tempfd) != NULL) {
	    line_count++;
	    if (the_same(buffer, "From ") && line_count > 1)
	      fprintf(mailfd, ">%s", buffer);
	    else
	      fputs(buffer, mailfd);
	  }

	  fputs("\n\n", mailfd);

	  fclose(mailfd);
	  unlock();		/* blamo or not?  Let it decide! */
	  fclose(tempfd);
	} /* end if show only */
}

save_message(foldername)
char *foldername;
{
	/** Save the message in a folder.  Use full file buffering to
	    make this work without contention problems **/

	FILE  *fd, *tempfd;
	char  filename[SLEN], buffer[SLEN];
	int   fdunit;

	if (verbose && outfd != NULL)
	  fprintf(outfd, "filter (%s): Message saved in folder %s\n",
		  username, foldername);

	if (!show_only) {
	  sprintf(filename, "%s%d.fil", tempdir, getpid());

	  if ((fdunit = open(foldername, O_APPEND | O_WRONLY | O_CREAT, 0600)) < 0) {
	    if (outfd != NULL)
	      fprintf(outfd,
		 "filter (%s): can't save message to requested folder %s!\n",
		    username, foldername);
	    return(1);
	  }
	  fd = fdopen(fdunit,"a");

	  if ((tempfd = fopen(filename, "r")) == NULL) {
	    if (outfd != NULL)
	      fprintf(outfd,
		     "filter (%s): can't open temp file for reading!\n",
		     username);
	     return(1);
	  }

#ifdef MMDF
	  fputs(MSG_SEPERATOR, fd);
#endif

	  while (fgets(buffer, sizeof(buffer), tempfd) != NULL)
	    fputs(buffer, fd);

	  /*
	   * Add two newlines, to ensure that other mailers (which, unlike
	   * elm, may only look for \n\nFrom_ as the start-of-message
	   * indicator).
	   */
	  fprintf(fd, "%s", "\n\n");

	  fclose(fd);
	  fclose(tempfd);
	}

 	return(0);
}

execute(command)
char *command;
{
	/** execute the indicated command, feeding as standard input the
	    message we have.
	**/

	char buffer[SLEN];

	if (verbose && outfd != NULL)
	  fprintf(outfd, "filter (%s): Executing %s\n", username, command);

	if (! show_only) {
	  sprintf(buffer, "%s <%s%d.fil", command, tempdir, getpid());
	  system(buffer);
	}
}

FILE *
emergency_local_delivery()
{
	/** This is called when we can't deliver the mail to the usual
	    mailbox in the usual way ...
	**/

	FILE *tempfd;
	char  mailbox[SLEN];
	int   mailunit;

	sprintf(mailbox, "%s/%s", home, EMERGENCY_MAILBOX);

	if ((mailunit = open(mailbox, O_APPEND | O_WRONLY | O_CREAT, 0600)) < 0) {
	  if (outfd != NULL)
	    fprintf(outfd, "filter (%s): Can't open %s either!!\n",
		    username, mailbox);

	  sprintf(mailbox,"%s/%s", home, EMERG_MBOX);

	  if ((mailunit = open(mailbox, O_APPEND | O_WRONLY | O_CREAT, 0600)) < 0) {

	    if (outfd != NULL) {
	      fprintf(outfd,"filter (%s): Can't open %s either!!!!\n",
		      username, mailbox);
	      fprintf(outfd,
		      "filter (%s): I can't open ANY mailboxes!  Augh!!\n",
		       username);
	     }

	     leave("Cannot open any mailbox");		/* DIE DIE DIE DIE!! */
	   }
	   else
	     if (outfd != NULL)
	       fprintf(outfd, "filter (%s): Using %s as emergency mailbox\n",
		       username, mailbox);
	  }
	  else
	    if (outfd != NULL)
	      fprintf(outfd, "filter (%s): Using %s as emergency mailbox\n",
		      username, mailbox);

	tempfd = fdopen(mailunit, "a");
	return((FILE *) tempfd);
}
