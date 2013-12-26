
static char rcsid[] = "@(#)$Id: utils.c,v 4.1 90/04/28 22:44:19 syd Exp $";

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
 * $Log:	utils.c,v $
 * Revision 4.1  90/04/28  22:44:19  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Utility routines for ELM

**/

#include "headers.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#ifdef BSD
#undef tolower
#endif

#include <signal.h>

#ifndef OS2
extern int errno;
#endif

char *error_name();
void   exit();

create_new_folders()
{
	/* this creates a new folders directory */

#ifdef MKDIR
	(void) mkdir(folders, 0700);
#else
	char com[SLEN];

	/** Some systems don't have a mkdir call - how inconvienient! **/

	sprintf(com, "mkdir %s", folders);
	system_call(com, SH, FALSE, FALSE);
	sprintf(com, "chmod 700 %s", folders);
	system_call(com, SH, FALSE, FALSE);
#endif /* MKDIR */

	chown(folders, userid, groupid);
}

create_new_elmdir()
{
	/** this routine is just for allowing new users who don't have the
	    old elm files to create a new .elm directory **/

	char source[SLEN];
#ifdef MKDIR
	sprintf(source, "%s/%s", home, dotelm);
	(void) mkdir(source, 0700);
#else
	char com[SLEN];

	/** Some systems don't have a mkdir call - how inconvienient! **/

	sprintf(com, "mkdir %s/%s", home, dotelm);
	system_call(com, SH, FALSE, FALSE);
	sprintf(com, "chmod 700 %s/%s", home, dotelm);
	system_call(com, SH, FALSE, FALSE);
#endif /* MKDIR */

	chown( source, userid, groupid);
}

move_old_files_to_new()
{
	/** this routine is just for allowing people to transition from
	    the old Elm, where things are all kept in their $HOME dir,
	    to the new one where everything is in $HOME/.elm... **/

	char source[SLEN], dest[SLEN], temp[SLEN];
	char com[SLEN];

	/** simply go through all the files... **/

	sprintf(source, "%s/.alias_text", home);
	if (access(source, ACCESS_EXISTS) != -1) {
	  sprintf(dest,   "%s/%s", home, ALIAS_TEXT);
	  printf("\n\rCopying from: %s\n\rCopying to:   %s\n\r", source, dest);

	  sprintf(temp, "/tmp/%d", getpid());
	  sprintf(com, "%s -e 's/:/=/g' %s > %s\n", sed_cmd, source, temp);
	  (void) system_call(com, SH, FALSE, FALSE);
	  sprintf(com, "%s %s %s\n", move_cmd, temp, dest);
	  (void) system_call(com, SH, FALSE, FALSE);
	  (void) system_call("newalias", SH, FALSE, FALSE);
	}

	sprintf(source, "%s/%sheaders", home, dotelm);
	if (access(source, ACCESS_EXISTS) != -1) {
	  sprintf(dest,   "%s/%s", home, mailheaders);
	  printf("\n\rCopying from: %s\n\rCopying to:   %s\n\r", source, dest);
	  copy(source, dest);
	}

	sprintf(source, "%s/%src", home, dotelm);
	if (access(source, ACCESS_EXISTS) != -1) {
	  sprintf(dest,   "%s/%s", home, elmrcfile);
	  printf("\n\rCopying from: %s\n\rCopying to:   %s\n\r", source, dest);
	  copy(source, dest);
	}

	printf(
	"\n\rWelcome to the new version of ELM!\n\n\rHit return to continue.");
	getchar();
}

emergency_exit()
{
	/** used in dramatic cases when we must leave without altering
	    ANYTHING about the system... **/
	char *mk_lockname();

	dprint(1, (debugfile,
     "\nERROR: Something dreadful is happening!  Taking emergency exit!!\n\n"));
	dprint(1, (debugfile,
	     "  possibly leaving behind the following files;\n"));
	dprint(1, (debugfile,
	     "     The mailbox tempfile : %s\n", cur_tempfolder));
	if(folder_type == SPOOL) dprint(1, (debugfile,
	     "     The mailbox lock file: %s\n", mk_lockname(cur_folder)));
	dprint(1, (debugfile,
	     "     The composition file : %s%d%s\n", temp_dir, getpid(), temp_file));
	dprint(1, (debugfile,
	     "     The readmsg data file: %s/%s\n", home, readmsg_file));

	Raw(OFF);
	if (cursor_control)  transmit_functions(OFF);
	if (hp_terminal)     softkeys_off();

	if (cursor_control)
	  MoveCursor(LINES-1, 0);

	PutLine0(LINES,0,
		"\nEmergency exit taken! All temp files intact!\n\n");

        ExitScreen();
	exit(1);
}
rm_temps_exit()
{
      char buffer[SLEN];
      PutLine0(LINES,0,
         "\nWrite to temp file failed, exiting leaving mailbox intact!\n\n");
      dprint(2, (debugfile, "\nrm_temps_exit, deleteing temp files\n"));
      Raw(OFF);
      if (cursor_control)  transmit_functions(OFF);
      if (hp_terminal)     softkeys_off();
      sprintf(buffer,"%d%s", getpid(), temp_file);  /* editor buffer */
      (void) unlink(buffer);
      if (folder_type == SPOOL) {
        if (mailfile)
          fclose(mailfile);
        (void) unlink(cur_tempfolder);
      }
      sprintf(buffer,"%s/%s", home, readmsg_file);  /* readmsg temp */
      (void) unlink(buffer);
      unlock();                               /* remove lock file if any */
      if(!batch_only) {
        MoveCursor(LINES-1,0);
        NewLine();
      }
      ExitScreen();
      exit(1);
}

/*ARGSUSED*/
/*VARARGS0*/

leave(val)
int val;	/* not used, placeholder for signal catching! */
{
	char buffer[SLEN];

	dprint(2, (debugfile, "\nLeaving mailer normally (leave)\n"));

	Raw(OFF);
	if (cursor_control)  transmit_functions(OFF);
	if (hp_terminal)     softkeys_off();

	sprintf(buffer,"%s%d%s", temp_dir, getpid(), temp_file);  /* editor buffer */
	(void) unlink(buffer);

	if (folder_type == SPOOL) {
          if (mailfile)
            fclose(mailfile);
	  (void) unlink(cur_tempfolder);
	}

	sprintf(buffer,"%s/%s", home, readmsg_file);  /* readmsg temp */
	(void) unlink(buffer);

	unlock();				/* remove lock file if any */

	if(!batch_only) {
	  MoveCursor(LINES-1,0);
	  NewLine();
	}

	ExitScreen();
	exit(0);
}

silently_exit()
{
	/** This is the same as 'leave', but it doesn't remove any non-pid
	    files.  It's used when we notice that we're trying to create a
	    temp mail file and one already exists!!
	**/
	char buffer[SLEN];

	dprint(2, (debugfile, "\nLeaving mailer quietly (silently_exit)\n"));

	Raw(OFF);
	if (cursor_control)  transmit_functions(OFF);
	if (hp_terminal)     softkeys_off();

	sprintf(buffer,"%s%d%s", temp_dir, getpid(), temp_file);  /* editor buffer */
	(void) unlink(buffer);

	MoveCursor(LINES-1,0);
	NewLine();

	ExitScreen();
	exit(0);
}

/*ARGSUSED0*/

#ifndef REMOVE_AT_LAST
leave_locked(val)
int val;	/* not used, placeholder for signal catching! */
{
	/** same as leave routine, but don't disturb lock file **/

	char buffer[SLEN];

        dprint(3, (debugfile,
	    "\nLeaving mailer due to presence of lock file (leave_locked)\n"));

	Raw(OFF);
	if (cursor_control)  transmit_functions(OFF);
	if (hp_terminal)     softkeys_off();

	sprintf(buffer,"%s%d%s", temp_dir, getpid(), temp_file);  /* editor buffer */
	(void) unlink(buffer);

        if (mailfile)
          fclose(mailfile);
	(void) unlink(cur_tempfolder);			/* temp mailbox */

	MoveCursor(LINES-1,0);
	NewLine();
	ExitScreen();
	exit(0);
}
#endif

int
get_page(msg_pointer)
int msg_pointer;
{
	/** Ensure that 'current' is on the displayed page,
	    returning NEW_PAGE iff the page changed! **/

	register int first_on_page, last_on_page;

	first_on_page = (header_page * headers_per_page) + 1;

	last_on_page = first_on_page + headers_per_page - 1;

	if (selected)	/* but what is it on the SCREEN??? */
	  msg_pointer = compute_visible(msg_pointer);

	if (selected && msg_pointer > selected)
	  return(SAME_PAGE);	/* too far - page can't change! */

	if (msg_pointer > last_on_page) {
	  header_page = (int) (msg_pointer-1)/ headers_per_page;
	  return(NEW_PAGE);
	}
	else if (msg_pointer < first_on_page) {
	  header_page = (int) (msg_pointer-1) / headers_per_page;
	  return(NEW_PAGE);
	}
	else
	  return(SAME_PAGE);
}

char *nameof(filename)
char *filename;
{
	/** checks to see if 'filename' has any common prefixes, if
	    so it returns a string that is the same filename, but
	    with '=' as the folder directory, or '~' as the home
	    directory..
	**/

	static char buffer[STRING];
	register int i = 0, iindex = 0;

	if (strncmp(filename, folders, strlen(folders)) == 0) {
	  if (strlen(folders) > 0) {
	    buffer[i++] = '=';
	    iindex = strlen(folders);
	    if(filename[iindex] == '/')
	      iindex++;
	  }
	}
	else if (strncmp(filename, home, strlen(home)) == 0) {
	  if (strlen(home) > 1) {
	    buffer[i++] = '~';
	    iindex = strlen(home);
	  }
	}
	else iindex = 0;

	while (filename[iindex] != '\0')
	  buffer[i++] = filename[iindex++];
	buffer[i] = '\0';

	return( (char *) buffer);
}
