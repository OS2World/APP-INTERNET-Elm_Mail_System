
static char rcsid[] = "@(#)$Id: fileio.c,v 4.1.1.1 90/10/07 19:48:08 syd Exp $";

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
 * $Log:	fileio.c,v $
 * Revision 4.1.1.1  90/10/07  19:48:08  syd
 * fix the bounce problem reported earlier when using MMDF submit as the MTA.
 * From: Jim Clausing <jac%brahms.tinton.ccur.com@RELAY.CS.NET>
 *
 * Revision 4.1  90/04/28  22:43:06  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** File I/O routines, including deletion from the folder!

**/

#include "headers.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#ifdef BSD
#undef tolower
#endif

#ifndef OS2
extern int errno;
#endif

char *error_name(), *index();

copy_message(prefix,
	     dest_file,
	     remove_header,
	     remote,
	     update_status,
	     mmdf_head,
           remail,
           decrypt)
char *prefix;
FILE *dest_file;
int  remove_header, remote, update_status, mmdf_head, remail, decrypt;
{
	/** Copy current message to destination file, with optional 'prefix'
	    as the prefix for each line.  If remove_header is true, it will
	    skip lines in the message until it finds the end of header line...
	    then it will start copying into the file... If remote is true
	    then it will append "remote from <hostname>" at the end of the
	    very first line of the file (for remailing)

	    If "forwarding" is true then it'll do some nice things to
	    ensure that the forwarded message looks pleasant; e.g. remove
	    stuff like ">From " lines and "Received:" lines.

	    If "update_status" is true then it will write a new Status:
	    line at the end of the headers.  It never copies an existing one.
	**/

    char buffer[LONG_STRING];
    register struct header_rec *current_header = headers[current-1];
    register int  lines, front_line, next_front,
		  in_header = 1, first_line = TRUE, ignoring = FALSE;
    int	end_header = 0;
    int sender_added = 0;
    FILE *cryptfile;
    char cfilename[SLEN], dfilename[SLEN], basefilename[SLEN];
    char ans;
    int  tlines;

      /** get to the first line of the message desired **/

    if (fseek(mailfile, current_header->offset, 0) == -1) {
       dprint(1, (debugfile,
		"ERROR: Attempt to seek %d bytes into file failed (%s)",
		current_header->offset, "copy_message"));
       error1("ELM [seek] failed trying to read %d bytes into file.",
	     current_header->offset);
       return;
    }

    /* how many lines in message? */

    lines = current_header->lines;

    /* set up for forwarding just in case... */

    if (forwarding)
      remove_header = FALSE;

    /* now while not EOF & still in message... copy it! */

    next_front = TRUE;

    while (lines) {
      if (fgets(buffer, sizeof(buffer), mailfile) == NULL)
        break;

      fixline(buffer);
      front_line = next_front;

      if(buffer[strlen(buffer)-1] == '\n') {
	lines--;	/* got a full line */
	next_front = TRUE;
      }
      else
	next_front = FALSE;

      if (front_line && ignoring)
	ignoring = whitespace(buffer[0]);

      if (ignoring)
	continue;

#ifdef MMDF
      if (mmdf_head && strcmp(buffer, MSG_SEPERATOR) == 0)
	continue;
#endif /* MMDF */

      /* are we still in the header? */

      if (in_header && front_line) {
	if (strlen(buffer) < 2) {
	  in_header = 0;
	  end_header = -1;
	  if (remail && !sender_added) {
	    if (fprintf(dest_file, "%sSender: %s\n", prefix, username) == EOF) {
	      Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
	      dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
	      rm_temps_exit();
	    }
	  }
	}
	else if (!isspace(*buffer)
	      && index(buffer, ':') == NULL
#ifdef MMDF
	      && strcmp(buffer, MSG_SEPERATOR) != 0
#endif /* MMDF */
		) {
	  in_header = 0;
	  end_header = 1;
	  if (remail && !sender_added) {
	    if (fprintf(dest_file, "%sSender: %s\n", prefix, username) == EOF) {
	      Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
	      dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
	      rm_temps_exit();
	    }
	  }
	} else if (in_header && remote && first_word(buffer, "Sender:")) {
	  if (remail)
	    if (fprintf(dest_file, "%sSender: %s\n", prefix, username) == EOF) {
	      Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
	      dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
	      rm_temps_exit();
	    }
	  sender_added = TRUE;
	  continue;
	}
	if (end_header) {
	  if (update_status) {
	      if (isoff(current_header->status, NEW)) {
		if (ison(current_header->status, UNREAD)) {
		  if (fprintf(dest_file, "%sStatus: O\n", prefix) == EOF) {
		    Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
		    dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
		    rm_temps_exit();
		  }
		} else	/* read */
#ifdef BSD
		  if (fprintf(dest_file, "%sStatus: OR\n", prefix) == EOF) {
#else
		  if (fprintf(dest_file, "%sStatus: RO\n", prefix) == EOF) {
#endif
		    Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
		    dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
		    rm_temps_exit();
		  }
		update_status = FALSE; /* do it only once */
	      }	/* else if NEW - indicate NEW with no Status: line. This is
		 * important if we resync a mailfile - we don't want
		 * NEW status lost when we copy each message out.
		 * It is the responsibility of the function that calls
		 * this function to unset NEW as appropriate to its
		 * reason for using this function to copy a message
		 */

		/*
		 * add the missing newline for RFC 822
		 */
	      if (end_header > 0) {
		/* add the missing newline for RFC 822 */
		if (fprintf(dest_file, "\n") == EOF) {
		  Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
		  dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
		  rm_temps_exit();
		}
	      }
	  }
	}
      }

      if (in_header) {
	/* Process checks while in header area */

	if (remove_header) {
	  ignoring = TRUE;
	  continue;
	}

	/* add remote on to front? */
	if (first_line && remote) {
	  no_ret(buffer);
#ifndef MMDF
	  if (fprintf(dest_file, "%s%s remote from %s\n",
		  prefix, buffer, hostname) == EOF) {
		Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
		dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
		rm_temps_exit();
	  }
#else
	  if (first_word(buffer, "From ")) {
	    if (strcmp(sendmail, mailer) != 0)
	    if (fprintf(dest_file, "%s%s remote from %s\n",
		    prefix, buffer, hostname) == EOF) {
		Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
		dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
		rm_temps_exit();
	    }
	  } else {
	    if (fprintf(dest_file, "%s%s\n", prefix, buffer) == EOF) {
		Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
		dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
		rm_temps_exit();
	    }
	  }
#endif /* MMDF */
	  first_line = FALSE;
	  continue;
	}

	if (!forwarding) {
	  if(! first_word(buffer, "Status:")) {
	    if (fprintf(dest_file, "%s%s", prefix, buffer) == EOF) {
              dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
	      rm_temps_exit();
	      }
	    continue;
	  } else {
	    ignoring = TRUE;
	    continue;	/* we will output a new Status: line later, if desired. */
	  }
	}
	else { /* forwarding */

	  if (first_word(buffer, "Received:"   ) ||
	      first_word(buffer, "From "       ) ||
	      first_word(buffer, ">From"       ) ||
	      first_word(buffer, "Status:"     ) ||
	      first_word(buffer, "Return-Path:"))
	      ignoring = TRUE;
	  else
	    if (remail && first_word(buffer, "To:")) {
	      if (fprintf(dest_file, "%sOrig-%s", prefix, buffer) == EOF) {
                dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
	        rm_temps_exit();
	      }
	    } else {
	      if (fprintf(dest_file, "%s%s", prefix, buffer) == EOF) {
                dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
	        rm_temps_exit();
	      }
	    }
	}
      }
      else { /* not in header */
        /* Process checks that occur after the header area */

	/***  Check to see if text is a PGP message  ***/

	/*  if buffer starts with "-----BEGIN PGP MESSAGE-----"
	       AND decrypt = TRUE
	      divert rest of message to cryptfile
	      decrypt cryptfile file.
	      if success, copy decrypted file into dest_file, then set
			  lines = 0 to stop copying from mailfile.
	      else just continue
	      continue
	*/

	if( first_word(buffer, "-----BEGIN PGP MESSAGE-----")
	   && decrypt ) {
	  PutLine1(LINES-1,0,"Message encrypted with PGP. Do you wish to decrypt? (y/n) y%c", BACKSPACE);
	  fflush(stdout);
	  ans = ReadCh();
	  if( tolower(ans) != 'n' ) {
	    Write_to_screen("Yes.",0);
	    sprintf(basefilename,"%s%d", temp_dir, getpid());
	    strcpy(cfilename,basefilename);
	    strcpy(dfilename,basefilename);
	    strcat(cfilename,".asc");
	    strcat(dfilename,".txt");

	    if ((cryptfile = fopen(cfilename,"wt")) == NULL) {
	      if(batch_only)
		printf("Could not create file %s (%s).\n", cfilename, error_name(errno));
	      else
		error2("Could not create file %s (%s).", cfilename, error_name(errno));
	    }
	    else {

	      chown(cfilename, userid, groupid);

	      tlines = lines;
	      fputs(buffer,cryptfile);
	      while( tlines )  {
		if(fgets(buffer, sizeof(buffer), mailfile) == NULL)
		  break;
		fixline(buffer);
		fputs(buffer,cryptfile);
		tlines--;
	      }
	      fclose(cryptfile);

	      sprintf(buffer, "pgp %s -o %s", cfilename, dfilename);
	      puts("\r\n");
	      if( !system_call(buffer, SH, FALSE, FALSE) ) {
		fputs("[pgp-encrypt]\n", dest_file);
		if ((cryptfile = fopen(dfilename, "rt")) != NULL)
		  while( !feof(cryptfile) ) {
		    if( fgets(buffer, sizeof(buffer), cryptfile) != NULL )
		      if (strlen(buffer) >= 6 && strncmp(buffer, "[pgp-", 5) == 0
			  && strncmp(buffer + strlen(buffer) - 2, "]\n", 2) == 0)
			/* skip */;
		      else
			fprintf(dest_file, "%s%s", prefix, buffer);
		  }
	      }
	      else {
		printf("Error while decrypting.  Copying message as-is.\n\r");
		cryptfile = fopen(cfilename, "rt");
		while( !feof(cryptfile) ) {
		  if (fgets(buffer, sizeof(buffer), cryptfile) != NULL)
		    fprintf(dest_file, "%s%s", prefix, buffer);
		}
	      }
	      lines = 0;
	      fclose(cryptfile);
	      if( !access(cfilename,0) )
		if( remove(cfilename) )
		  printf("Could not delete file %s (%s).\n\r", cfilename, error_name(errno));
	      if( !access(dfilename,0) )
		if( remove(dfilename) )
		  printf("Could not delete file %s (%s).\n\r", dfilename, error_name(errno));
	    }
	  }
	  else
	    Write_to_screen("No.",0);

	} /*  end check for PGP  */

#ifndef MMDF
	if(first_word(buffer, "From ") && (real_from(buffer, NULL))) {
	  dprint(1, (debugfile,
		 "\n*** Internal Problem...Tried to add the following;\n"));
	  dprint(1, (debugfile,
		 "  '%s'\nto output file (copy_message) ***\n", buffer));
	  break;	/* STOP NOW! */
	}
#endif /* MMDF */

	if (fprintf(dest_file, "%s%s", prefix, buffer) == EOF) {
	  Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
	  dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
	  rm_temps_exit();
	}
      }
    }
#ifndef MMDF
    if (strlen(buffer) + strlen(prefix) > 1)
	if (fprintf(dest_file, "\n") == EOF) {	/* blank line to keep mailx happy *sigh* */
	  Write_to_screen("\n\rWrite in copy_message failed\n\r", 0);
	  dprint(1, (debugfile,"\n*** Fprint failed on copy_message;\n"));
	  rm_temps_exit();
	}
#endif /* MMDF */
}

static struct stat saved_buf;
static char saved_fname[SLEN];

int
save_file_stats(fname)
char *fname;
{
	/* if fname exists, save the owner, group, mode and filename.
	 * otherwise flag nothing saved. Return 0 if saved, else -1.
	 */

	if(stat(fname, &saved_buf) != -1) {
	  (void)strcpy(saved_fname, fname);
	  dprint(2, (debugfile,
	    "** saved stats for file owner = %d group = %d mode = %o %s **\n",
	    saved_buf.st_uid, saved_buf.st_gid, saved_buf.st_mode, fname));
	  return(0);
	}
	dprint(2, (debugfile,
	  "** couldn't save stats for file %s [errno=%d] **\n",
	  fname, errno));
	return(-1);

}

restore_file_stats(fname)
char *fname;
{
	/* if fname matches the saved file name, set the owner and group
	 * of fname to the saved owner, group and mode,
	 * else to the userid and groupid of the user and to 700.
	 * Return	-1 if the  either mode or owner/group not set
	 *		0 if the default values were used
	 *		1 if the saved values were used
	 */

	int old_umask, i, new_mode, new_owner, new_group, ret_code;


	new_mode = 0600;
	new_owner = userid;
	new_group = groupid;
	ret_code = 0;

	if(strcmp(fname, saved_fname) == 0) {
	  new_mode = saved_buf.st_mode;
	  new_owner = saved_buf.st_uid;
	  new_group = saved_buf.st_gid;
	  ret_code = 1;
	}
	dprint(2, (debugfile, "** %s file stats for %s **\n",
	  (ret_code ? "restoring" : "setting"), fname));

	old_umask = umask(0);
	if((i = chmod(fname, new_mode & 0777)) == -1)
	  ret_code = -1;

	dprint(2, (debugfile, "** chmod(%s, %.3o) returns %d [errno=%d] **\n",
		fname, new_mode & 0777, i, errno));

	(void) umask(old_umask);

#ifdef	BSD
	/*
	 * Chown is restricted to root on BSD unix
	 */
	(void) chown(fname, new_owner, new_group);
#else
	if((i = chown(fname, new_owner, new_group)) == -1)
	  ret_code = -1;

	dprint(2, (debugfile, "** chown(%s, %d, %d) returns %d [errno=%d] **\n",
		   fname, new_owner, new_group, i, errno));
#endif

	return(ret_code);

}

/** and finally, here's something for that evil trick: site hiding **/

#ifdef SITE_HIDING

int
is_a_hidden_user(specific_username)
char *specific_username;
{
	/** Returns true iff the username is present in the list of
	   'hidden users' on the system.
	**/

	FILE *hidden_users;
	char  buffer[SLEN];

        /*
	this line is deliberately inserted to ensure that you THINK
	about what you're doing, and perhaps even contact the author
	of Elm before you USE this option...
        */

	if ((hidden_users = fopen (HIDDEN_SITE_USERS,"r")) == NULL) {
	  dprint(1, (debugfile,
		  "Couldn't open hidden site file %s [%s]\n",
		  HIDDEN_SITE_USERS, error_name(errno)));
	  return(FALSE);
	}

	while (fscanf(hidden_users, "%s", buffer) != EOF)
	  if (strcmp(buffer, specific_username) == 0) {
	    dprint(3, (debugfile, "** Found user '%s' in hidden site file!\n",
		    specific_username));
	    fclose(hidden_users);
	    return(TRUE);
	  }

	fclose(hidden_users);
	dprint(3, (debugfile,
		"** Couldn't find user '%s' in hidden site file!\n",
		specific_username));

	return(FALSE);
}

#endif
