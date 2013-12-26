
static char rcsid[] = "@(#)$Id: mailmsg2.c,v 4.1.1.8 90/10/07 19:48:10 syd Exp $";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 4.1.1.8 $   $State: Exp $
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
 * $Log:	mailmsg2.c,v $
 * Revision 4.1.1.8  90/10/07  19:48:10  syd
 * fix the bounce problem reported earlier when using MMDF submit as the MTA.
 * From: Jim Clausing <jac%brahms.tinton.ccur.com@RELAY.CS.NET>
 *
 * Revision 4.1.1.7  90/08/15  22:02:36  syd
 * deal with several of the problems that have come up trying to use the MMDF
 * submit program directly rather than going through the sendmail stub
 * included with MMDF.  This should take care of the problem of not being
 * able to send mail to usernames beginning with "i" and with the
 * 'No valid author specified' problem.
 * From: jac%brahms.tinton.ccur.com@RELAY.CS.NET
 *
 * Revision 4.1.1.6  90/07/12  23:19:20  syd
 * Make domain name checking case independent
 * From: Syd, reported by Steven Baur
 *
 * Revision 4.1.1.5  90/07/12  22:52:42  syd
 * When Elm is compiled with the NO_XHEADER symbol defined, it failed
 * to put a blank line between the message header and message body.
 * From: mca@medicus.medicus.com (Mark Adams)
 *
 * Revision 4.1.1.4  90/06/26  16:18:24  syd
 * Make it encode lines that are [...] if not one of the reserved lines.
 * It was messing up decoding
 * From: Syd via report from Lenny Tropiano
 *
 * Revision 4.1.1.3  90/06/21  21:07:48  syd
 * Fix XHEAD define
 * From: Syd
 *
 * Revision 4.1.1.2  90/06/09  23:20:24  syd
 * fix typo
 *
 * Revision 4.1.1.1  90/06/09  22:28:39  syd
 * Allow use of submit with mmdf instead of sendmail stub
 * From: martin <martin@hppcmart.grenoble.hp.com>
 *
 * Revision 4.1  90/04/28  22:43:28  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Interface to allow mail to be sent to users.  Part of ELM  **/

#include "headers.h"
#include <errno.h>
#include <ctype.h>

#ifdef BSD
#undef tolower
#undef toupper
#endif

#ifndef OS2
extern int errno;
#endif
extern char version_buff[];

char *error_name(), *error_description(), *strip_parens();
char *strcat(), *strcpy(), *index();
char *format_long(), *strip_commas(), *tail_of_string();

unsigned long sleep();

#ifdef SITE_HIDING
 char *get_ctime_date();
#endif
FILE *write_header_info();

/* these are all defined in the mailmsg1.c file! */

extern char subject[SLEN], in_reply_to[SLEN], expires[SLEN],
            action[SLEN], priority[SLEN], reply_to[SLEN], to[VERY_LONG_STRING],
	    cc[VERY_LONG_STRING], expanded_to[VERY_LONG_STRING],
	    expanded_cc[VERY_LONG_STRING], user_defined_header[SLEN],
	    bcc[VERY_LONG_STRING], expanded_bcc[VERY_LONG_STRING];


int  gotten_key;
char *bounce_off_remote();

mail(copy_msg, edit_message, form)
int  copy_msg, edit_message, form;
{
	/** Given the addresses and various other miscellany (specifically,
	    'copy-msg' indicates whether a copy of the current message should
	    be included, 'edit_message' indicates whether the message should
	    be edited) this routine will invoke an editor for the user and
	    then actually mail off the message. 'form' can be YES, NO, or
	    MAYBE.  YES=add "Content-Type: mailform" header, MAYBE=add the
	    M)ake form option to last question, and NO=don't worry about it!
	    Also, if 'copy_msg' = FORM, then grab the form temp file and use
	    that...
	    Return TRUE if the main part of the screen has been changed
	    (useful for knowing whether a redraw is needed.
	**/

	FILE *reply, *real_reply; /* second is post-input buffer */
	char *whole_msg_file = NULL, *tempnam();
	char filename[SLEN], fname[SLEN], copy_file[SLEN],
             very_long_buffer[VERY_LONG_STRING], mailerflags[NLEN];
	int ch, sys_status;
	register int retransmit = FALSE;
	int      already_has_text = FALSE;		/* we need an ADDRESS */
	int	 signature_done = FALSE;
	int	 need_redraw = 0;
	int      resent = forwarding && !edit_message;

	static int cancelled_msg = 0;

	dprint(4, (debugfile, "\nMailing to \"%s\" (with%s editing)\n",
		  expanded_to, edit_message? "" : "out"));

	gotten_key = 0;		/* ignore previously gotten encryption key */

	/** first generate the temporary filename **/

	sprintf(filename,"%s%d%s", temp_dir, getpid(), temp_file);

	/** if possible, let's try to recall the last message? **/

	if (! batch_only && copy_msg != FORM && user_level != 0)
	  retransmit = recall_last_msg(filename, copy_msg, &cancelled_msg,
		       &already_has_text);

	/** if we're not retransmitting, create the file.. **/

	if (! retransmit)
	  if ((reply = fopen(filename,"w")) == NULL) {
	    dprint(1, (debugfile,
               "Attempt to write to temp file %s failed with error %s (mail)\n",
		 filename, error_name(errno)));
	    if(batch_only)
	      printf("Could not create file %s (%s).\n",filename,
		 error_name(errno));
	    else
	      error2("Could not create file %s (%s).",filename,
		 error_name(errno));
	    return(need_redraw);
	  }

	chown (filename, userid, groupid);

	/* copy the message from standard input */
	if (batch_only) {
	  while (fgets(very_long_buffer, VERY_LONG_STRING, stdin) != NULL)
	    fprintf(reply, "%s", very_long_buffer);
	}

	if (copy_msg == FORM) {
	  sprintf(fname, "%s%d%s", temp_dir, getpid(), temp_form_file);
	  fclose(reply);	/* we can't retransmit a form! */
	  if (access(fname,ACCESS_EXISTS) != 0) {
	    if(batch_only)
	      printf("Couldn't find forms file!\n");
	    else
	      error("Couldn't find forms file!");
	    return(need_redraw);
	  }
	  dprint(4, (debugfile, "-- renaming existing file %s to file %s --\n",
		  fname, filename));
	  rename(fname, filename);
	}
	else if (copy_msg && ! retransmit) {  /* if retransmit we have it! */
	  if (forwarding) {
	    if (edit_message)
	      fputs("Forwarded message:\n", reply);
	  }
	  else if (attribution[0]) {
	    fprintf(reply, attribution, headers[current-1]->from);
	    fputc('\n', reply);
	  }
	  if (edit_message) {
          copy_message(prefixchars, reply, noheader,FALSE,FALSE,TRUE,FALSE,TRUE);
	    already_has_text = TRUE;	/* we just added it, right? */
	  }
	  else
          copy_message("", reply, noheader, FALSE, FALSE, TRUE, FALSE, FALSE);
	}

        /* append signature now if we are going to use an external editor */
	/* Don't worry about the remote/local determination too much */

        if (already_has_text ||
           (strcmp(editor,"builtin") != 0 && strcmp(editor,"none") != 0)) {
	     signature_done = TRUE;
             if (!(forwarding && !edit_message) &&
                 !retransmit && copy_msg != FORM)
	       already_has_text |= append_sig(reply);
	}

	if (! retransmit && copy_msg != FORM)
	  if (reply != NULL)
	    (void) fclose(reply);	/* on replies, it won't be open! */

	/** Edit the message **/

	if (edit_message)
	  create_readmsg_file();	/* for "readmsg" routine */

	ch = edit_message? 'e' : ' ';	/* drop through if needed... */

	/* calculate default save_file name */
	if(auto_cc) {
	  if(save_by_name) {
	    if(force_name) {
	      strcpy(copy_file, "=");	/* signals save by 'to' logname */
	    } else {
	      strcpy(copy_file, "=?");	/* conditional save by 'to' logname */
	    }
	  } else {
	    strcpy(copy_file, "<");	/* signals save to sentmail */
	  }
	} else *copy_file = '\0';	/* signals to not save a copy */


	if (! batch_only) {
	  do {
	    switch (ch) {
	      case 'e': need_redraw = 1;
			if (edit_the_message(filename, already_has_text)) {
			  cancelled_msg = TRUE;
			  return(need_redraw);
			}
			break;

	      case 'c': if (name_copy_file(copy_file))
			  need_redraw = 1;
			break;

	      case 'h': edit_headers();
			need_redraw = 1;
			break;

#ifdef ALLOW_SUBSHELL
	      case '!': if (subshell()) {
			  ClearScreen();
			  need_redraw = 1;
			}
			break;
#endif

	      default : /* do nothing */ ;
	    }

	    /** ask that silly question again... **/

	    if ((ch = verify_transmission(filename, &form)) == 'f') {
	      cancelled_msg = TRUE;
	      return(need_redraw);
	    }
	    
	    if (ch == 's' && 
		perhaps_pgp_encode(filename) == -1)
	      ch = ' ';

	  } while (ch != 's');

	  if (form == YES)
	    if (format_form(filename) < 1) {
	      cancelled_msg = TRUE;
	      return(need_redraw);
	    }

	  if ((reply = fopen(filename,"r")) == NULL) {
	      dprint(1, (debugfile,
	    "Attempt to open file %s for reading failed with error %s (mail)\n",
                filename, error_name(errno)));
	      error1("Could not open reply file (%s).", error_name(errno));
	      return(need_redraw);
	  }
	}
	else if ((reply = fopen(filename,"r")) == NULL) {
	  dprint(1, (debugfile,
	    "Attempt to open file %s for reading failed with error %s (mail)\n",
             filename, error_name(errno)));
	  printf("Could not open reply file (%s).\n", error_name(errno));
	  return(need_redraw);
	}

	cancelled_msg = FALSE;	/* it ain't cancelled, is it? */

	/** ask about bounceback if the user wants us to.... **/

	if (uucp_hops(to) > bounceback && bounceback > 0 && copy_msg != FORM)
	  if (verify_bounceback() == TRUE) {
	    if (strlen(cc) > 0) strcat(expanded_cc, ", ");
	    strcat(expanded_cc, bounce_off_remote(to));
	  }

	/** grab a copy if the user so desires... **/

	if (*copy_file && !no_save) /* i.e. if copy_file contains a name */
	  save_copy(expanded_to, expanded_cc, expanded_bcc,
	       filename, copy_file, form);

	/** write all header information into whole_msg_file **/

	if((whole_msg_file=tempnam(temp_dir, "snd")) == NULL) {
	  dprint(1, (debugfile, "couldn't make temp file nam! (mail)\n"));
	  if(batch_only)
	    printf("Sorry - couldn't make temp file name!\n");
	  else if(mail_only)
	    error("Sorry - couldn't make temp file name.");
	  else
	    set_error("Sorry - couldn't make temp file name.");
	  return(need_redraw);
	}

	/** try to write headers to new temp file **/

	dprint(6, (debugfile, "Composition file='%s' and mail buffer='%s'\n",
		    filename, whole_msg_file));

	dprint(2,(debugfile,"--\nTo: %s\nCc: %s\nBcc: %s\nSubject: %s\n---\n",
		  expanded_to, expanded_cc, expanded_bcc, subject));

	if ((real_reply=
	   write_header_info(whole_msg_file, expanded_to, expanded_cc, 
             expanded_bcc, form == YES, FALSE, resent)) == NULL) {

	  /** IT FAILED!!  MEIN GOTT!  Use a dumb mailer instead! **/

	  dprint(3, (debugfile, "** write_header failed: %s\n",
		 error_name(errno)));

	  if (cc[0] != '\0') {  		/* copies! */
	    strcat(expanded_to, " ");
	    strcat(expanded_to, expanded_cc);
	  }

	  quote_args(very_long_buffer, strip_parens(strip_commas(expanded_to)));
	  strcpy(expanded_to, very_long_buffer);

	  sprintf(very_long_buffer, "%s -f %s -s \"%s\" %s",
		  mailx, filename, subject, expanded_to);

	  if(batch_only)
	    printf("Message sent using dumb mailer %s.\n", mailx);
	  else
	    error1("Message sent using dumb mailer %s.", mailx);
	  sleep(2);	/* ensure time to see this prompt! */

	}
	else {

	  copy_message_across(reply, real_reply, FALSE);

          /* Append signature if not done earlier */

          if (!signature_done && !retransmit && copy_msg != FORM)
               append_sig(real_reply);

	  fclose(real_reply);

	  if (cc[0] != '\0') {		         /* copies! */
	    strcat(expanded_to, " ");
	    strcat(expanded_to, expanded_cc);
	  }

	  if (bcc[0] != '\0') {
	    strcat(expanded_to, " ");
	    strcat(expanded_to, expanded_bcc);
	  }

	  remove_hostbang(expanded_to);

	  if (strcmp(sendmail, mailer) == 0)
	  {
#if 1
	    sprintf(very_long_buffer,"sndmail %s -af %s -f %s@%s -t %s >nul 2>&1",
		    background ? "-bg" : "",
		    whole_msg_file, username, hostfromname, 
		    strip_parens(strip_commas(expanded_bcc)));
#else
	    sprintf(very_long_buffer,"sndmail %s -af %s -f %s@%s %s >nul 2>&1",
		    background ? "-bg" : "",
		    whole_msg_file, username, hostfromname, 
		    strip_parens(strip_commas(expanded_to)));
#endif
	  }
	  else
	  {
	    sprintf(very_long_buffer,"%s -f %s %s 2>nul",
		    mailer, whole_msg_file, submitflags);
	  }
	}

	fclose(reply);

	if(batch_only)
	  printf("Sending mail...\r\n");
	else {
	  PutLine0(LINES,0,"Sending mail...");
	  CleartoEOLN();
	}

	/* Take note of mailer return value */

	if ( sys_status = system_call(very_long_buffer, SH, FALSE, FALSE) ) {
		/* problem case: */
		if (mail_only || batch_only)
		   printf("\r\nmailer returned error status %d\r\n", sys_status);
		else {
		   sprintf(very_long_buffer, "mailer returned error status %d", sys_status);
		   set_error(very_long_buffer);
		}
	} else {
		/* Success case: */
		if(batch_only)
		  printf("Mail sent!\r\n");
		else if(mail_only)
		  error("Mail sent!");
		else
		  set_error("Mail sent!");
	}

	/* Unlink temp file now.
	 * This is a precaution in case the message was encrypted.
	 * I.e. even though this file is readable by the owner only,
	 * encryption is supposed to hide things even from someone
	 * with root privelges. The best we can do is not let this
	 * file just hang after we're finished with it.
	 */
	if (!background) /* but only if not using sendmail in background mode */
	  (void)unlink(whole_msg_file);
	(void)unlink(filename);
        free(whole_msg_file);
	return(need_redraw);
}

/*
 * remove_hostbang - Given an expanded list of addresses, remove all
 * occurrences of "thishost!" at the beginning of addresses.
 * This hack is useful in itself, but it is required now because of the
 * kludge disallowing alias expansion on return addresses.
 */

remove_hostbang(addrs)
char *addrs;
{
	int i, j, hlen, flen;

	if ((hlen = strlen(hostname)) < 1)
	  return;

	flen = strlen(hostfullname);
	i = j = 0;

	while (addrs[i]) {
	  if (i == 0 || isspace(addrs[i - 1])) {
	    if (strncmp(&addrs[i], hostname, hlen) == 0 &&
	      addrs[i + hlen] == '!') {
	        i += hlen + 1;
	        continue;
	    }
	    if (strncmp(&addrs[i], hostfullname, flen) == 0 &&
	      addrs[i + flen] == '!') {
	        i += flen + 1;
	        continue;
	    }
	  }
	  addrs[j++] = addrs[i++];
	}
	addrs[j] = 0;
}

mail_form(address, subj)
char *address, *subj;
{
	/** copy the appropriate variables to the shared space... */

	strcpy(subject, subj);
	strcpy(to, address);
	strcpy(expanded_to, address);

	return(mail(FORM, NO, NO));
}

int
recall_last_msg(filename, copy_msg, cancelled_msg, already_has_text)
char *filename;
int  copy_msg, *cancelled_msg, *already_has_text;
{
	char ch;

	/** If filename exists and we've recently cancelled a message,
	    the ask if the user wants to use that message instead!  This
	    routine returns TRUE if the user wants to retransmit the last
	    message, FALSE otherwise...
	**/

	register int retransmit = FALSE;

	if (access(filename, EDIT_ACCESS) == 0 && *cancelled_msg) {
	  Raw(ON);
	  CleartoEOLN();
	  if (copy_msg)
	    PutLine1(LINES-1,0,"Recall last kept message instead? (y/n) y%c",
		     BACKSPACE);
	  else
	    PutLine1(LINES-1,0,"Recall last kept message? (y/n) y%c",
		     BACKSPACE);
	  fflush(stdout);
	  ch = ReadCh();
	  if (tolower(ch) != 'n') {
	    Write_to_screen("Yes.",0);
            retransmit++;
	    *already_has_text = TRUE;
	  }
	  else
	    Write_to_screen("No.",0);

	  fflush(stdout);

	  *cancelled_msg = 0;
	}

	return(retransmit);
}

int
verify_transmission(filename, form_letter)
char *filename;
int  *form_letter;
{
	/** Ensure the user wants to send this.  This routine returns
	    the character entered.  Modified compliments of Steve Wolf
	    to add the'dead.letter' feature.
	    Also added form letter support...
	**/

	FILE *deadfd, *messagefd;
	char ch, buffer[SLEN], fname[SLEN];
	int x_coord, y_coord;

	while(1) {
	  /* clear bottom part of screen */
	  MoveCursor(LINES-2,0);
	  CleartoEOS();

	  /* display prompt and menu according to
	   * user level and what's available on the menu */
	  if (user_level == 0) {
	    PutLine0(LINES-2,0,
      "Please choose one of the following options by parenthesized letter: s");
	    GetXYLocation(&x_coord, &y_coord);
	    y_coord--;	/* backspace over default answer */
	    Centerline(LINES-1,
	      "e)dit message, edit h)eaders, s)end it, or f)orget it.");
	  } else {
	    PutLine0(LINES-2, 0, "And now: s");
	    GetXYLocation(&x_coord, &y_coord);
	    y_coord--;	/* backspace over default answer */
	    if (*form_letter == PREFORMATTED)  {
	       strcpy(buffer, "Choose ");
	    } else if (*form_letter == YES) {
	       strcpy(buffer, "Choose e)dit form, ");
	    } else if (*form_letter == MAYBE)  {
	       strcpy(buffer, "Choose e)dit msg, m)ake form, ");
	    } else {
             strcpy(buffer, "Choose e)dit msg, ");
	    }
#ifdef ALLOW_SUBSHELL
	    strcat(buffer, "!)shell, ");
#endif
          strcat(buffer, "h)drs, c)opy file, s)end, f)orget");
	    Centerline(LINES-1, buffer);
	  }

	  /* wait for answer */
	  fflush(stdin);
	  fflush(stdout);
	  Raw(ON);	/* double check... testing only... */
	  MoveCursor(x_coord, y_coord);
	  ch = ReadCh();
	  ch = tolower(ch);

	  /* process answer */
	  switch (ch) {
	     case 'f': Write_to_screen("Forget",0);
		       if (mail_only) {
			 /** try to save it as a dead letter file **/
			 save_file_stats(fname);
			 sprintf(fname, "%s/%s", home, dead_letter);
			 if ((deadfd = fopen(fname,"a")) == NULL) {
			   dprint(1, (debugfile,
		   "\nAttempt to append to deadletter file '%s' failed: %s\n\r",
			       fname, error_name(errno)));
			   error("Message not saved, Sorry.");
			 }
			 else if ((messagefd = fopen(filename, "r")) == NULL) {
			   dprint(1, (debugfile,
			     "\nAttempt to read reply file '%s' failed: %s\n\r",
				   filename, error_name(errno)));
			   error("Message not saved, Sorry.");
			 } else {
			   /* if we get here we're okay for everything */
			   while (fgets(buffer, SLEN, messagefd) != NULL)
			     fputs(buffer, deadfd);

			   fclose(messagefd);
			   fclose(deadfd);
			   restore_file_stats(fname);

			   error1("Message saved in file \"$HOME/%s\".",
			     dead_letter);

			}
		       } else if (user_level != 0)
			set_error("Message kept. Can be restored at next f)orward, m)ail or r)eply.");
		       break;

	     case '\n' :
	     case '\r' :
	     case 's'  : Write_to_screen("Send",0);
			 ch = 's';		/* make sure! */
			 break;

	     case 'm'  : if (*form_letter == MAYBE) {
			   *form_letter = YES;
		           switch (check_form_file(filename)) {
			     case -1 : return('f');
			     case 0  : *form_letter = MAYBE;  /* check later!*/
				       error("No fields in form!");
				       return('e');
			     default : continue;
	                   }
			 }
			 else {
	                    Write_to_screen("%c??", 1, 07);	/* BEEP */
			    sleep(1);
		            continue;
	                 }
	     case 'e'  :  if (*form_letter != PREFORMATTED) {
			    Write_to_screen("Edit",0);
	 	            if (*form_letter == YES)
			      *form_letter = MAYBE;
	                  }
			  else {
	                    Write_to_screen("%c??", 1, 07);	/* BEEP */
			    sleep(1);
		            continue;
	                 }
			 break;

	     case 'h'  : Write_to_screen("Headers",0);
			 break;

	     case 'c'  : Write_to_screen("Copy file",0);
			 break;

	     case '!'  : break;

	     default   : Write_to_screen("%c??", 1, 07);	/* BEEP */
			 sleep(1);
		         continue;
	   }

	   return(ch);
	}
}

FILE *
write_header_info(filename, long_to, long_cc, long_bcc, form, copy, resend)
char *filename, *long_to, *long_cc, *long_bcc;
int   form, copy, resend;
{
	/** Try to open filedesc as the specified filename.  If we can,
	    then write all the headers into the file.  The routine returns
	    'filedesc' if it succeeded, NULL otherwise.  Added the ability
	    to have backquoted stuff in the users .elmheaders file!
	    If copy is TRUE, then treat this as the saved copy of outbound
	    mail.
	**/

	char opentype[2];
	long time(), thetime;
	char *ctime();
	static FILE *filedesc;		/* our friendly file descriptor  */
	char *resent = resend ? "Resent-" : "";  /* forwarding ? */

#ifdef SITE_HIDING
	char  buffer[SLEN];
	int   is_hidden_user;		/* someone we should know about?  */
#endif
#ifdef MMDF
	int   is_submit_mailer;		/* using submit means change From: */
#endif /* MMDF */

	char  *get_arpa_date();

	if(copy)
	    strcpy(opentype, "a");
	else
	    strcpy(opentype, "w");

	save_file_stats(filename);
	if ((filedesc = fopen(filename, opentype)) == NULL) {
	  dprint(1, (debugfile,
	    "Attempt to open file %s for writing failed! (write_header_info)\n",
	     filename));
	  dprint(1, (debugfile, "** %s - %s **\n\n", error_name(errno),
		 error_description(errno)));
	  error2("Error %s encountered trying to write to %s.",
		error_name(errno), filename);
	  sleep(2);
	  return(NULL);		/* couldn't open it!! */
	}

	restore_file_stats(filename);

	if(copy) {	/* Add top line that mailer would add */
#ifdef MMDF
	  fprintf(filedesc, MSG_SEPERATOR);
#endif /* MMDF */
	  thetime = time((long *) 0);
	  fprintf(filedesc,"From %s %s", username, ctime(&thetime));
	}

#ifdef SITE_HIDING
	if ( !copy && (is_hidden_user = is_a_hidden_user(username))) {
	  /** this is the interesting part of this trick... **/
	  sprintf(buffer, "From %s!%s %s\n",  HIDDEN_SITE_NAME,
		  username, get_ctime_date());
	  fprintf(filedesc, "%s", buffer);
	  dprint(1,(debugfile, "\nadded: %s", buffer));
	  /** so is this perverted or what? **/
	}
#endif


	/** Subject moved to top of headers for mail because the
	    pure System V.3 mailer, in its infinite wisdom, now
	    assumes that anything the user sends is part of the
	    message body unless either:
		1. the "-s" flag is used (although it doesn't seem
		   to be supported on all implementations??)
		2. the first line is "Subject:".  If so, then it'll
		   read until a blank line and assume all are meant
		   to be headers.
	    So the gory solution here is to move the Subject: line
	    up to the top.  I assume it won't break anyone elses program
	    or anything anyway (besides, RFC-822 specifies that the *order*
	    of headers is irrelevant).  Gahhhhh....
	**/

	fprintf(filedesc,"%sDate: %s\n", resent, get_arpa_date());

#ifndef DONT_ADD_FROM
	fputs(resent, filedesc);
#ifdef MMDF
	is_submit_mailer = (strcmp(submitmail,mailer) == 0);
#endif /* MMDF */
# ifdef SITE_HIDING
#    ifdef MMDF
	if (is_submit_mailer)
	  fprintf(filedesc,"From: %s <%s>\n", full_username, username);
	else
#    endif /* MMDF */
	if (is_hidden_user)
	  fprintf(filedesc,"From: %s <%s!%s!%s>\n", full_username,
		  hostname, HIDDEN_SITE_NAME, username);
	else
	  fprintf(filedesc,"From: %s <%s!%s>\n", full_username,
		  hostname, username);
# else
#  ifdef  INTERNET
#   ifdef  USE_DOMAIN
#    ifdef _MMDF
	if (is_submit_mailer)
	  fprintf(filedesc,"From: %s <%s>\n", full_username, username);
	else
#    endif /* MMDF */
	  fprintf(filedesc,"From: %s <%s@%s>\n", full_username,
		username, hostfromname);
#   else
#    ifdef _MMDF
	if (is_submit_mailer)
	  fprintf(filedesc,"From: %s <%s>\n", full_username, username);
	else
#    endif /* MMDF */
	fprintf(filedesc,"From: %s <%s@%s>\n", full_username,
		username, hostname);
#   endif
#  else
#    ifdef MMDF
	if (is_submit_mailer)
	  fprintf(filedesc,"From: %s <%s>\n", full_username, username);
	else
#    endif /* MMDF */
	fprintf(filedesc,"From: %s <%s!%s>\n", full_username,
		hostname, username);
#  endif
# endif
#endif

	fprintf(filedesc, "%sSubject: %s\n", resent, subject);

	fprintf(filedesc, "%sTo: %s\n", resent, format_long(long_to, strlen("To:")));

	if (cc[0] != '\0')
	    fprintf(filedesc, "%sCc: %s\n", resent, format_long(long_cc, strlen("Cc: ")));

	if ((copy || stricmp(sendmail, mailer) != 0) &&
	    (bcc[0] != '\0'))
	    fprintf(filedesc, "%sBcc: %s\n", resent, format_long(long_bcc, strlen("Bcc: ")));

	if (strlen(action) > 0)
	    fprintf(filedesc, "Action: %s\n", action);

	if (strlen(priority) > 0)
	    fprintf(filedesc, "Priority: %s\n", priority);

	if (strlen(expires) > 0)
	    fprintf(filedesc, "Expires: %s\n", expires);

	if (strlen(reply_to) > 0)
	    fprintf(filedesc, "%sReply-To: %s\n", resent, reply_to);

	if (strlen(in_reply_to) > 0)
	    fprintf(filedesc, "In-Reply-To: %s\n", in_reply_to);

	if (strlen(user_defined_header) > 0)
	    fprintf(filedesc, "%s\n", user_defined_header);

	add_mailheaders(filedesc);

	if (form)
	  fprintf(filedesc, "Content-Type: mailform\n");

#ifndef NO_XHEADER
	fprintf(filedesc, "X-Mailer: ELM [version %s] for OS/2\n", version_buff);
#endif /* !NO_XHEADER */

        if (!resend)
 	  putc('\n', filedesc);

	return((FILE *) filedesc);
}

copy_message_across(source, dest, copy)
FILE *source, *dest;
int copy;
{
	/** Copy the message in the file pointed to by source to the
	    file pointed to by dest.
	    If copy is TRUE, treat as a saved copy of outbound mail. **/

	int  crypted = FALSE;			/* are we encrypting?  */
	int  encoded_lines = 0;			/* # lines encoded     */
	char buffer[SLEN];			/* file reading buffer */

	while (fgets(buffer, SLEN, source) != NULL) {
	  if (buffer[0] == '[') {
	    if (strncmp(buffer, START_ENCODE, strlen(START_ENCODE))==0)
	      crypted = TRUE;
	    else if (strncmp(buffer, END_ENCODE, strlen(END_ENCODE))==0)
	      crypted = FALSE;
	    else if ((strncmp(buffer, DONT_SAVE, strlen(DONT_SAVE)) == 0)
	          || (strncmp(buffer, DONT_SAVE2, strlen(DONT_SAVE2)) == 0)) {
	      if(copy) break;  /* saved copy doesn't want anything after this */
	      else continue;   /* next line? */
	    }
	    else if (crypted) {
	      if (! gotten_key++)
	        getkey(ON);
	      else if (! encoded_lines)
	        get_key_no_prompt();		/* reinitialize.. */
	      encode(buffer);
	      encoded_lines++;
	    }
	  }
	  else if (crypted) {
	    if (batch_only) {
	      printf(
		"Sorry. Cannot send encrypted mail in \"batch mode\".\n\r");
	      leave();
	    } else if (! gotten_key++)
	      getkey(ON);
	    else if (! encoded_lines)
	      get_key_no_prompt();		/* reinitialize.. */
	    encode(buffer);
	    encoded_lines++;
	  }

	  if (copy && (strncmp(buffer, "From ", 5) == 0))
	    /* Add in the > to a From on our copy */
	    fprintf(dest, ">%s", buffer);

	  else if (!copy && strcmp(buffer, ".\n") == 0)
	    /* Because some mail transport agents take a lone period to
	     * mean EOF, we add a blank space on outbound message.
	     */
	    fputs(". \n", dest);
  	  else
  	    if (fputs(buffer, dest) == EOF) {
		Write_to_screen("\n\rWrite failed in copy_message_across\n\r", 0);
		emergency_exit();
	    }
	}
#ifdef _MMDF
	if (copy) fputs(MSG_SEPERATOR, dest);
#else
	if (copy) fputs("\n", dest);	/* ensure a blank line at the end */
#endif /* MMDF */
}

int
verify_bounceback()
{
	char	ch;

	/** Ensure the user wants to have a bounceback copy too.  (This is
	    only called on messages that are greater than the specified
	    threshold hops and NEVER for non-uucp addresses.... Returns
	    TRUE iff the user wants to bounce a copy back....
	 **/

	MoveCursor(LINES,0);
	CleartoEOLN();
	PutLine1(LINES,0,
	      "\"Bounce\" a copy off the remote machine? (y/n) y%c",
	      BACKSPACE);
	fflush(stdin);	/* wait for answer! */
	fflush(stdout);
	ch = ReadCh();
	if (tolower(ch) != 'y') {
	  Write_to_screen("No.", 0);
	  fflush(stdout);
	  return(FALSE);
	}
	Write_to_screen("Yes.", 0);
	fflush(stdout);

	return(TRUE);
}


int
append_sig(file)
FILE *file;
{
	/* Append the correct signature file to file.  Return TRUE if
           we append anything.  */

        /* Look at the to and cc list to determine which one to use */

	/* We could check the bcc list too, but we don't want people to
           know about bcc, even indirectly */

	/* Some people claim that  user@anything.same_domain should be
	   considered local.  Since it's not the same machine, better be
           safe and use the remote sig (presumably it has more complete
           information).  You can't necessarily finger someone in the
           same domain. */

	  if (!batch_only && (local_signature[0] || remote_signature[0])) {

            char filename2[SLEN];
	    char *sig;

  	    if (index(expanded_to, '!') || index(expanded_cc,'!'))
              sig = remote_signature;		/* ! always means remote */
            else {
	      /* check each @ for @thissite.domain */
	      /* if any one is different than this, then use remote sig */
	      int len;
	      char *ptr;
	      char sitename[SLEN];
	      sprintf(sitename,"@%s",hostfullname);
	      len = strlen(sitename);
              sig = local_signature;
              for (ptr = index(expanded_to,'@'); ptr;  /* check To: list */
	          ptr = index(ptr+1,'@')) {
		if (strincmp(ptr,sitename,len) != 0
		    || (*(ptr+len) != ',' && *(ptr+len) != 0
		    && *(ptr+len) != ' ')) {
	          sig = remote_signature;
                  break;
                }
              }
              if (sig == local_signature)		   /* still local? */
                for (ptr = index(expanded_cc,'@'); ptr;   /* check Cc: */
		    ptr = index(ptr+1,'@')) {
		  if (strincmp(ptr,sitename,len) != 0
		      || (*(ptr+len) != ',' && *(ptr+len) != 0
		      && *(ptr+len) != ' ')) {
	            sig = remote_signature;
                    break;
                  }
                }
            }

            if (sig[0]) {  /* if there is a signature file */
	      if (sig_dashes) /* dashes are optional */
	        fprintf(file, "\n-- \n");  /* News 2.11 compatibility? */
#ifdef OS2
	      if (sig[0] != '/' && sig[0] != '\\' &&
		  (!isalpha(sig[0]) || sig[1] != ':'))
#else
	      if (sig[0] != '/')
#endif
	        sprintf(filename2, "%s/%s", home, sig);
	      else
	        strcpy(filename2, sig);
	      (void) append(file, filename2);

              return TRUE;
            }
          }

return FALSE;

}

perhaps_pgp_encode(filename)
char *filename;
{
  char buffer[VERY_LONG_STRING];
  FILE *msg;
  int encrypt = 0;
  int sign = 0;

  if ((msg = fopen(filename, "r")) == NULL)
    return -1;

  while (fgets(buffer, sizeof(buffer), msg) != NULL)
    if (strncmp(buffer, "[pgp-encrypt]\n", 14) == 0) {
      encrypt = 1;
      strcpy(buffer, strip_parens(strip_commas(expanded_to)));
      if (expanded_cc[0]) {
	strcat(buffer, " ");
	strcat(buffer, strip_parens(strip_commas(expanded_cc)));
      }
      if (expanded_bcc[0]) {
	strcat(buffer, " ");
	strcat(buffer, strip_parens(strip_commas(expanded_bcc)));
      }
      strcat(buffer, " ");
      strcat(buffer, username); /* add sender id too! */
      strcat(buffer, "@");
      strcat(buffer, hostname);
      break;
    }
    else if (strncmp(buffer, "[pgp-sign]\n", 11) == 0) {
      sign = 1;
      break;
    }
    else if (strncmp(buffer, "[pgp-encrypt ", 13) == 0 &&
	     strncmp(buffer + strlen(buffer) - 2, "]\n", 2) == 0) {
      strcpy(buffer, buffer + 13);
      buffer[strlen(buffer) - 2] = 0;
      encrypt = 1;
      break;
    }
  fclose(msg);

  if (encrypt)
    return pgp_encrypt(filename, buffer);

  if (sign)
    return pgp_sign(filename);

  return 0;
}

pgp_encrypt(filename, to)
char *filename, *to;
{
  char  buffer[SLEN];
  char  pgpfn[SLEN];
  int   dotpos = 0;

  sprintf(buffer, "pgp -seaw %s %s", filename, to);
  puts("\r\n\n");

  if (system_call(buffer, SH, FALSE, FALSE) == 0) {

    do { ++dotpos; } while (filename[dotpos] != '.');
    strncpy(pgpfn, filename, dotpos);
    strcpy(pgpfn + dotpos, ".asc");

    if(rename(pgpfn,filename)) {
      printf("Error with renaming %s to %s in pgp_encrypt! (%s)\n",
	     pgpfn, filename, error_name(errno));
      return(-1);
    }

    return(0);
  }
  else {			/* something went wrong. Bad password or user spec */
    printf("Error while encrypting. Try again.");
    return(-1);
  }
}

pgp_sign(filename)
char *filename;
{
  char  buffer[SLEN];
  char  pgpfn[SLEN];
  int   dotpos = 0;

  sprintf(buffer, "pgp -sta +clearsig=on %s", filename);
  puts("\r\n\n");

  if (system_call(buffer, SH, FALSE, FALSE) == 0) {

    do { ++dotpos; } while (filename[dotpos] != '.');
    strncpy(pgpfn, filename, dotpos);
    strcpy(pgpfn + dotpos, ".asc");

    if(unlink(filename) || rename(pgpfn,filename)) {
      printf("Error with renaming %s to %s in pgp_sign! (%s)\n",
	     pgpfn, filename, error_name(errno));
      return(-1);
    }

    return(0);
  }
  else {			/* something went wrong. Bad password or user spec */
    printf("Error while signing. Try again.");
    return(-1);
  }
}
