
static char rcsid[] = "@(#)$Id: save_opts.c,v 4.1 90/04/28 22:44:00 syd Exp $";

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
 * $Log:	save_opts.c,v $
 * Revision 4.1  90/04/28  22:44:00  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This file contains the routine needed to allow the users to change the
    Elm parameters and then save the configuration in a ".elm/elmrc" file in
    their home directory.  With any luck this will allow them never to have
    to actually EDIT the file!!

**/

#include "headers.h"
#include <errno.h>

#undef onoff
#define   onoff(n)	(n == 1? "ON":"OFF")

#define absolute(x)		((x) < 0? -(x) : (x))

#ifndef OS2
extern  int errno;
#endif
extern char version_buff[];

char *error_name(), *sort_name();
long  ftell();

#include "save_opts.h"

FILE *elminfo;		/* informational file as needed... */

save_options()
{
	/** Save the options currently specified to a file.  This is a
	    fairly complex routine since it tries to put in meaningful
	    comments and such as it goes along.  The comments are
	    extracted from the file ELMRC_INFO as defined in the sysdefs.h
	    file.  THAT file has the format;

		varname
		  <comment>
		  <comment>
		<blank line>

	    and each comment is written ABOVE the variable to be added.  This
	    program also tries to make 'pretty' stuff like the alternatives
	    and such.
	**/

	FILE *newelmrc;
	char  oldfname[SLEN], newfname[SLEN], inffname[SLEN];

	sprintf(newfname, "%s/%s", home, elmrcfile);
	sprintf(oldfname, "%s/%s", home, old_elmrcfile);

	/** first off, let's see if they already HAVE a .elm/elmrc file **/

	save_file_stats(newfname);
	if (access(newfname, ACCESS_EXISTS) != -1) {
	  /** YES!  Copy it to the file ".old.elmrc".. **/
	  if (rename(newfname, oldfname) < 0)
	    dprint(2, (debugfile, "Unable to rename %s to %s\n",
		   newfname, oldfname));
	  (void) chown(oldfname, userid, groupid);

	}

	/** now let's open the datafile if we can... **/
	sprintf(inffname,"%s/%s", helphome, elmrc_info);

	if ((elminfo = fopen(inffname, "r")) == NULL)
	  error1("Warning: saving without comments! Can't get to %s.",
		  inffname);

	/** next, open the new .elm/elmrc file... **/

	if ((newelmrc = fopen(newfname, "w")) == NULL) {
	   error2("Can't save configuration! Can't write to %s [%s].",
		   newfname, error_name(errno));
	   return;
	}

	save_user_options(elminfo, newelmrc);
	restore_file_stats(newfname);

	error1("Options saved in file %s.", newfname);
}

save_user_options(elminfo_fd, newelmrc)
FILE *elminfo_fd, *newelmrc;
{
	/** save the information in the file.  If elminfo_fd == NULL don't look
	    for comments!
	**/

	if (elminfo_fd != NULL)
	  build_offset_table(elminfo_fd);

	fprintf(newelmrc,
	      "#\n# .elm/elmrc - options file for the ELM mail system\n#\n");

	if (strlen(full_username) > 0)
	  fprintf(newelmrc, "# Saved automatically by ELM %s for %s\n#\n\n",
		  version_buff, full_username);
	else
	  fprintf(newelmrc, "# Saved automatically by ELM %s\n#\n\n", version_buff);
	fprintf(newelmrc,"# For yes/no settings with ?, ON means yes, OFF means no\n\n");

	save_option_string(CALENDAR, raw_calendar_file, newelmrc, FALSE);
	save_option_string(EDITOR, raw_editor, newelmrc, FALSE);

	save_option_char(ESCAPECHAR, escape_char, newelmrc);

	save_option_string(FULLNAME, full_username, newelmrc, FALSE);
	save_option_string(RECEIVEDMAIL, raw_recvdmail, newelmrc, FALSE);
	save_option_string(MAILDIR, raw_folders, newelmrc, FALSE);
	save_option_string(TMPDIR, temp_dir, newelmrc, FALSE);
	save_option_string(PAGER, raw_pager, newelmrc, FALSE);
	save_option_string(PREFIX, prefixchars, newelmrc, TRUE);
	save_option_string(PRINT, raw_printout, newelmrc, FALSE);
	save_option_string(SENTMAIL, raw_sentmail, newelmrc, FALSE);
	save_option_string(SHELL, raw_shell, newelmrc, FALSE);

	save_option_string(LOCALSIGNATURE, raw_local_signature,
 	  newelmrc, FALSE);
	save_option_string(REMOTESIGNATURE, raw_remote_signature,
	  newelmrc, FALSE);
	save_option_on_off(SIGDASHES, sig_dashes, newelmrc);

	save_option_sort(SORTBY, newelmrc);

	save_option_on_off(ALWAYSDELETE, always_del, newelmrc);
	save_option_on_off(ALWAYSSTORE, always_store, newelmrc);
	save_option_on_off(ALWAYSKEEP, always_keep, newelmrc);
	save_option_on_off(ARROW, arrow_cursor, newelmrc);
	save_option_on_off(ASK, question_me, newelmrc);
	save_option_on_off(ASKCC, prompt_for_cc, newelmrc);
	save_option_string(ATTRIBUTION, attribution, newelmrc, FALSE);
	save_option_on_off(AUTOCOPY, auto_copy, newelmrc);

	save_option_number(BOUNCEBACK, bounceback, newelmrc);

	save_option_on_off(COPY, auto_cc, newelmrc);
	save_option_on_off(FORCENAME, force_name, newelmrc);
	save_option_on_off(FORMS, (allow_forms != NO), newelmrc);
	save_option_on_off(KEEPEMPTY, keep_empty_files, newelmrc);
	save_option_on_off(KEYPAD, hp_terminal, newelmrc);
	save_option_on_off(MENU, mini_menu, newelmrc);
	save_option_on_off(MOVEPAGE, move_when_paged, newelmrc);
	save_option_on_off(NAMES, names_only, newelmrc);
	save_option_on_off(NOHEADER, noheader, newelmrc);
	save_option_on_off(POINTNEW, point_to_new, newelmrc);
	save_option_on_off(PROMPTAFTER, prompt_after_pager, newelmrc);
	save_option_on_off(RESOLVE, resolve_mode, newelmrc);
	save_option_on_off(SAVENAME, save_by_name, newelmrc);
	save_option_on_off(SOFTKEYS, hp_softkeys, newelmrc);

	save_option_number(TIMEOUT, (int) timeout, newelmrc);

	save_option_on_off(TITLES, title_messages, newelmrc);

	save_option_number(USERLEVEL, user_level, newelmrc);

	save_option_on_off(WARNINGS, warnings, newelmrc);
	save_option_on_off(WEED, filter, newelmrc);

	save_option_weedlist(WEEDOUT, newelmrc);
	save_option_alternatives(ALTERNATIVES, alternative_addresses, newelmrc);

	fclose(newelmrc);
	if ( elminfo_fd != NULL ) {
	    fclose(elminfo_fd);
	}
}

save_option_string(iindex, value, fd, underscores)
int iindex, underscores;
char *value;
FILE *fd;
{
	/** Save a string option to the file... only subtlety is when we
	    save strings with spaces in 'em - translate to underscores!
	**/

	char     buffer[SLEN], *bufptr;

	add_comment(iindex, fd);

	strcpy(buffer, value);

	if (underscores)
	  for (bufptr = buffer; *bufptr; bufptr++)
	    if (*bufptr == SPACE) *bufptr = '_';

	fprintf(fd, "%s = %s\n\n", save_info[iindex].name, buffer);
}

save_option_sort(iindex, fd)
int iindex;
FILE *fd;
{
	/** save the current sorting option to a file **/

	add_comment(iindex, fd);

	fprintf(fd, "%s = %s\n\n", save_info[iindex].name,
		sort_name(SHORT));
}

save_option_char(iindex, value, fd)
int iindex;
char value;
FILE *fd;
{
	/** Save a character option to the file **/

	add_comment(iindex, fd);

	fprintf(fd, "%s = %c\n\n", save_info[iindex].name, value);
}

save_option_number(iindex, value, fd)
int iindex, value;
FILE *fd;
{
	/** Save a binary option to the file - boy is THIS easy!! **/

	add_comment(iindex, fd);

	fprintf(fd, "%s = %d\n\n", save_info[iindex].name, value);
}

save_option_on_off(iindex, value, fd)
int iindex, value;
FILE *fd;
{
	/** Save a binary option to the file - boy is THIS easy!! **/

	add_comment(iindex, fd);

	fprintf(fd, "%s = %s\n\n", save_info[iindex].name, onoff(value));
}

save_option_weedlist(iindex, fd)
int iindex;
FILE *fd;
{
	/** save a list of weedout headers to the file **/

	int length_so_far = 0, i;

	add_comment(iindex, fd);

	length_so_far = strlen(save_info[iindex].name) + 4;

	fprintf(fd, "%s = ", save_info[iindex].name);

	/** first off, skip till we get past the default list **/

	for (i = 0; i < weedcount; i++)
	  if (strcmp(weedlist[i],"*end-of-defaults*") == 0)
	    break;

	while (i < weedcount) {
	  if (strcmp(weedlist[i], "*end-of-defaults*") != 0)
	    break;
	  i++;	/* and get PAST it too! */
	}

	while (i < weedcount) {
	  if (strlen(weedlist[i]) + length_so_far > 78) {
	    fprintf(fd, "\n\t");
	    length_so_far = 8;
	  }
	  fprintf(fd, "\"%s\" ", weedlist[i]);
	  length_so_far += (strlen(weedlist[i]) + 4);
	  i++;
	}
	fprintf(fd, "\t\"*end-of-user-headers*\"\n\n");
}

save_option_alternatives(iindex, list, fd)
int iindex;
struct addr_rec *list;
FILE *fd;
{
	/** save a list of options to the file **/
	int length_so_far = 0;
	struct addr_rec     *alternate;

	if (list == NULL) return;	/* nothing to do! */

	add_comment(iindex, fd);

	alternate = list;	/* don't LOSE the top!! */

	length_so_far = strlen(save_info[iindex].name) + 4;

	fprintf(fd, "%s = ", save_info[iindex].name);

	while (alternate != NULL) {
	  if (strlen(alternate->address) + length_so_far > 78) {
	    fprintf(fd, "\n\t");
	    length_so_far = 8;
	  }
	  fprintf(fd, "%s  ", alternate->address);
	  length_so_far += (strlen(alternate->address) + 3);
	  alternate = alternate->next;
	}
	fprintf(fd, "\n\n");
}

add_comment(iindex, fd)
int iindex;
FILE *fd;
{
	/** get to and add the comment to the file **/
	char buffer[SLEN];

	/** first off, add the comment from the comment file, if available **/

	if (save_info[iindex].offset > 0L) {
	  if (fseek(elminfo, save_info[iindex].offset, 0) == -1) {
	    dprint(1,(debugfile,
		   "** error %s seeking to %ld in elm-info file!\n",
		   error_name(errno), save_info[iindex].offset));
	  }
	  else while (fgets(buffer, SLEN, elminfo) != NULL) {
	    if (buffer[0] != '#')
	       break;
	    else
	       fprintf(fd, "%s", buffer);
	  }
	}
}

build_offset_table(elminfo_fd)
FILE *elminfo_fd;
{
	/** read in the info file and build the table of offsets.
	    This is a rather laborious puppy, but at least we can
	    do a binary search through the array for each element and
	    then we have it all at once!
	**/

	char line_buffer[SLEN];

	while (fgets(line_buffer, SLEN, elminfo_fd) != NULL) {
	  if (strlen(line_buffer) > 1)
	    if (line_buffer[0] != '#' && !whitespace(line_buffer[0])) {
	       no_ret(line_buffer);
	       if (find_and_store_loc(line_buffer, ftell(elminfo_fd))) {
	         dprint(1, (debugfile,"** Couldn't find and store \"%s\" **\n",
			 line_buffer));
	       }
	    }
	}
}

find_and_store_loc(name, offset)
char *name;
long  offset;
{
	/** given the name and offset, find it in the table and store it **/

	int first = 0, last, middle, compare;

	last = NUMBER_OF_SAVEABLE_OPTIONS;

	while (first <= last) {

	  middle = (first+last) / 2;

	  if ((compare = strcmp(name, save_info[middle].name)) < 0) /* a < b */
	    last = middle - 1;
	  else if (compare == 0) {				    /* a = b */
	    save_info[middle].offset = offset;
	    return(0);
	  }
	  else  /* greater */					    /* a > b */
	    first = middle + 1;
	}

	return(-1);
}
