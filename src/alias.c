
static char rcsid[] = "@(#)$Id: alias.c,v 4.1.1.1 90/06/05 21:20:26 syd Exp $";

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
 * $Log:	alias.c,v $
 * Revision 4.1.1.1  90/06/05  21:20:26  syd
 * Make temp file name less than 14 chars to allow deleting
 * aliases on a 14 char limit file system
 * From: Syd
 *
 * Revision 4.1  90/04/28  22:42:25  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** This file contains alias stuff

**/

#include "headers.h"
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef BSD
#undef        tolower
#endif

#define	ECHOIT	1 	/* echo on for prompting */

char *get_alias_address();
char *error_name(), *error_description(), *strip_parens(), *index();

#ifndef OS2
extern int errno;
#endif

#ifndef DONT_TOUCH_ADDRESSES
char *expand_system();

extern int  findnode_has_been_initialized;
#endif

int
ok_alias_name(name)
char *name;
{
	while ( *name != '\0' && ok_alias_char(*name) )
	  ++name;
	return ( *name == '\0' );
}


read_alias_files()
{
	/** read the system and user alias files, if present, and if they
	    have changed since last we read them.
	**/

	read_system_aliases();
	read_user_aliases();
}

read_system_aliases()
{
	/** read the system alias file, if present,
	    and if it has changed since last we read it.
	**/

	struct stat hst;
	static time_t system_ctime, system_mtime;
        char syshash[SLEN], sysdata[SLEN];

	/* If hash file hasn't changed, don't bother re-reading. */
        sprintf(syshash, "%s/%s", elmhome, system_hash_file);
        sprintf(sysdata, "%s/%s", elmhome, system_data_file);

	if (system_data != -1
	 && stat(syshash, &hst) == 0
	 && hst.st_ctime == system_ctime
	 && hst.st_mtime == system_mtime)
	  return;

	/* Close system data file if it was open. */

	if (system_data != -1) {
	  close(system_data);
	  system_data = -1;
	}

	/* Read system hash table.  If we can't, just return. */

	if (read_hash_file(syshash, (char *) system_hash_table,
			    sizeof system_hash_table) < 0)
	  return;

	/* Open system data table. */

	if ((system_data = open(sysdata, O_RDONLY)) == -1) {
	  dprint(1, (debugfile,
		      "Warning: Can't open system alias data file %s\n",
		      sysdata));
	  return;
	}

	/* Remember hash file times. */

	system_ctime = hst.st_ctime;
	system_mtime = hst.st_mtime;
}

read_user_aliases()
{
	/** read the system alias file, if present,
	    and if it has changed since last we read it.
	**/

	struct stat hst;
	char fname[SLEN];
	static time_t user_ctime, user_mtime;

	/* If hash file hasn't changed, don't bother re-reading. */

	sprintf(fname, "%s/%s", home, ALIAS_HASH);

	if (user_data != -1
	 && stat(fname, &hst) == 0
	 && hst.st_ctime == user_ctime
	 && hst.st_mtime == user_mtime)
	  return;

	/* Close user data file if it was open. */

	if (user_data != -1) {
	  close(user_data);
	  user_data = -1;
	}

	/* Read user hash table.  If we can't, just return. */

	if (read_hash_file(fname, (char *) user_hash_table,
			      sizeof user_hash_table) < 0)
	  return;

	/* Open user data table. */

	sprintf(fname, "%s/%s", home, ALIAS_DATA);
	if ((user_data = open(fname, O_RDONLY)) == -1) {
	  dprint(1, (debugfile,
	     "Warning: Can't open user alias data file %s\n", fname));
	  return;
	}

	/* Remember hash file times. */

	user_ctime = hst.st_ctime;
	user_mtime = hst.st_mtime;
}

int
read_hash_file(file, table, table_size)
char *file, *table;
int table_size;
{
	/** read the specified alias hash file into the specified table.
	    it must be _exactly_ the right size or forget it.
	**/

	struct stat st;
	int hash;

	/* Open the hash file. */

	if ((hash = open(file, O_RDONLY)) == -1) {
	  dprint(2, (debugfile,
	    "Warning: Can't open alias hash file %s\n", file));
	  return -1;
	}

	/* Be sure the hash file is the correct size. */

	if (fstat(hash, &st) == 0 && st.st_size != table_size) {
	  dprint(2, (debugfile,
	    "Warning: Alias hash file %s is wrong size (%ld/%d)\n",
	    file, st.st_size, table_size));
	  close(hash);
	  return -1;
	}

	/* Read the hash file into memory. */

	if (read(hash, (char *) table, table_size) != table_size) {
	  dprint(2, (debugfile,
	    "Warning: Can't read alias hash file %s\n", file));
	  close(hash);
	  return -1;
	}

	/* Close the hash file; return success. */

	close(hash);
	return 0;
}

int
add_alias()
{
	/** add an alias to the user alias text file.  Return zero
	    if alias not added in actuality **/

	char name[SLEN], *address, address1[LONG_STRING], buffer[LONG_STRING];
	char comment[SLEN], ch;
	char *strcpy();

	strcpy(buffer, "Enter alias name: ");
	PutLine0(LINES-2,0, buffer);
	CleartoEOLN();
	*name = '\0';
	optionally_enter(name, LINES-2, strlen(buffer), FALSE, FALSE);
	if (strlen(name) == 0)
	  return(0);
        if ( !ok_alias_name(name) ) {
	  error1("Bad character(s) in alias name %s.", name);
	  return(0);
	}
	if ((address = get_alias_address(name, FALSE)) != NULL) {
	  dprint(3, (debugfile,
		 "Attempt to add a duplicate alias [%s] in add_alias\n",
		 address));
	  if (address[0] == '!') {
	    address[0] = ' ';
	    error1("Already a group with name %s.", address);
	  }
	  else
	    error1("Already an alias for %s.", address);
	  return(0);
	}

	sprintf(buffer, "Enter full name for %s: ", name);
	PutLine0(LINES-2,0, buffer);
	CleartoEOLN();
	*comment = '\0';
	optionally_enter(comment, LINES-2, strlen(buffer), FALSE, FALSE);
	if (strlen(comment) == 0) strcpy(comment, name);

	sprintf(buffer, "Enter address for %s: ", name);
	PutLine0(LINES-2,0, buffer);
	CleartoEOLN();
	*address1 = '\0';
	optionally_enter(address1, LINES-2, strlen(buffer), FALSE, FALSE);
	Raw(ON);
	if (strlen(address1) == 0) {
	  error("No address specified!");
	  return(0);
	}
	PutLine3(LINES-2,0,"%s (%s) = %s", comment, name, address1);
	CleartoEOLN();
	if((ch = want_to("Accept new alias? (y/n) ",'y')) == 'y')
	  add_to_alias_text(name, comment, address1);
	ClearLine(LINES-2);
	return(ch == 'y' ? 1 : 0);
}

int
delete_alias()
{
	/** delete an alias from the user alias text file.  Return zero
	    if alias not deleted in actuality **/

	char name[SLEN], *address, buffer[LONG_STRING];
	char *strcpy();

	strcpy(buffer, "Enter alias name for deletion: ");
	PutLine0(LINES-2,0, buffer);
	CleartoEOLN();
	*name = '\0';
	optionally_enter(name, LINES-2, strlen(buffer), FALSE, FALSE);
	if (strlen(name) == 0)
	  return(0);
        if ((address = get_alias_address(name, FALSE))!=NULL)
        {
                if (address[0] == '!')
                {
                  address[0] = ' ';
                  PutLine1(LINES-1,0,"Group alias: %-60.60s", address);
                  CleartoEOLN();
	        }
		else
		  PutLine1(LINES-1,0,"Aliased address: %-60.60s", address);
	}
        else
	{
		  dprint(3, (debugfile,
		 "Attempt to delete a non-existent alias [%s] in delete_alias\n",
		 name));
		 error1("No alias for %s.", name);
	  	return(0);
	}
	if (want_to("Delete this alias? (y/n) ", 'y') == 'y')
	{
		if (!delete_from_alias_text(name))
		{
			CleartoEOS();
			return(1);
		}
	}
	CleartoEOS();
	return(0);
}

int
add_current_alias()
{
	/** alias the current message to the specified name and
	    add it to the alias text file, for processing as
	    the user leaves the program.  Returns non-zero iff
	    alias actually added to file **/

	char name[SLEN], address1[LONG_STRING], buffer[LONG_STRING], *address;
	char comment[SLEN], ch;
	struct header_rec *current_header;

	if (current == 0) {
	 dprint(4, (debugfile,
		"Add current alias called without any current message!\n"));
	 error("No message to alias to!");
	 return(0);
	}
	current_header = headers[current - 1];

	strcpy(buffer, "Current message address aliased to: ");
	PutLine0(LINES-2,0, buffer);
	CleartoEOLN();
	*name = '\0';
	optionally_enter(name, LINES-2, strlen(buffer), FALSE, FALSE);
	if (strlen(name) == 0)	/* cancelled... */
	  return(0);
        if ( !ok_alias_name(name) ) {
	  error1("Bad character(s) in alias name %s.", name);
	  return(0);
	}
	if ((address = get_alias_address(name, FALSE)) != NULL) {
	 dprint(3, (debugfile,
	         "Attempt to add a duplicate alias [%s] in add_current_alias\n",
		 address));
	  if (address[1] == '!') {
	    address[0] = ' ';
	    error1("Already a group with name %s.", address);
	  }
	  else
	    error1("Already an alias for %s.", address);
          return(0);
	}

	sprintf(buffer, "Enter full name for %s: ", name);
	PutLine0(LINES-2,0, buffer);
	CleartoEOLN();

	/* use full name in current message for default comment */
	tail_of(current_header->from, comment, current_header->to);
	if(index(comment, '!') || index(comment, '@'))
	  /* never mind - it's an address not a full name */
	  *comment = '\0';

	optionally_enter(comment, LINES-2, strlen(buffer), FALSE, FALSE);

	/* grab the return address of this message */
	get_return(address1, current-1);

	strcpy(address1, strip_parens(address1));	/* remove parens! */
#ifdef OPTIMIZE_RETURN
	optimize_return(address1);
#endif
	PutLine3(LINES-2,0,"%s (%s) = %s", comment, name, address1);
	CleartoEOLN();
	if((ch = want_to("Accept new alias? (y/n) ",'y')) == 'y')
	  add_to_alias_text(name, comment, address1);
	ClearLine(LINES-2);
	return(ch == 'y' ? 1 : 0);
}

add_to_alias_text(name, comment, address)
char *name, *comment, *address;
{
	/** Add the data to the user alias text file.  Return zero if we
	    succeeded, 1 if not **/

	FILE *file;
	char fname[SLEN];

	sprintf(fname, "%s/%s", home, ALIAS_TEXT);

	save_file_stats(fname);
	if ((file = fopen(fname, "a")) == NULL) {
	  dprint(2, (debugfile,
		 "Failure attempting to add alias to file %s within %s",
		   fname, "add_to_alias_text"));
	  dprint(2, (debugfile, "** %s - %s **\n", error_name(errno),
		   error_description(errno)));
	  error1("Couldn't open %s to add new alias!", fname);
	  return(1);
	}

	if (fprintf(file,"%s = %s = %s\n", name, comment, address) == EOF) {
	    dprint(2, (debugfile,
		       "Failure attempting to write alias to file within %s",
		       fname, "add_to_alias_text"));
	    dprint(2, (debugfile, "** %s - %s **\n", error_name(errno),
		       error_description(errno)));
	    error1("Couldn't write alias to file %s!", fname);
	    fclose(file);
	    return(1);
	}

	fclose(file);

	restore_file_stats(fname);

	return(0);
}

delete_from_alias_text(name)
char *name;
{
	/** Delete the data from the user alias text file.  Return zero if we
	    succeeded, 1 if not **/

	FILE *file, *tmp_file;
	char fname[SLEN], tmpfname[SLEN];
	char line_in_file[SLEN+3+SLEN+3+LONG_STRING]; /* name = comment = address */
	char name_with_equals[SLEN+2];

	strcpy(name_with_equals, name);
	strcat(name_with_equals, " =");

	sprintf(fname, "%s/%s", home, ALIAS_TEXT);
	sprintf(tmpfname, "%s/%s", home, ALIAS_TEMP);

	save_file_stats(fname);

	if ((file = fopen(fname, "r")) == NULL) {
	  dprint(2, (debugfile,
		 "Failure attempting to delete alias from file %s within %s",
		   fname, "delete_from_alias_text"));
	  dprint(2, (debugfile, "** %s - %s **\n", error_name(errno),
		   error_description(errno)));
	  error1("Couldn't open %s to delete alias!", fname);
	  return(1);
	}

	if ((tmp_file = fopen(tmpfname, "w")) == NULL) {
	  dprint(2, (debugfile,
		 "Failure attempting to open temp file %s within %s",
		   tmpfname, "delete_from_alias_text"));
	  dprint(2, (debugfile, "** %s - %s **\n", error_name(errno),
		   error_description(errno)));
	  error1("Couldn't open tempfile %s to delete alias!", tmpfname);
	  return(1);
	}

	while (fgets(line_in_file, sizeof(line_in_file), file) != (char *)NULL)
	{
		if (strncmp(name_with_equals, line_in_file,
				strlen(name_with_equals)) != 0)
			if (fprintf(tmp_file,"%s", line_in_file) == EOF) {
			    dprint(2, (debugfile,
				       "Failure attempting to write to temp file %s within %s",
				       tmpfname, "delete_from_alias_text"));
			    dprint(2, (debugfile, "** %s - %s **\n", error_name(errno),
				       error_description(errno)));
			    error1("Couldn't write to tempfile %s!", tmpfname);
			    fclose(file);
			    fclose(tmp_file);
			    unlink(tmpfname);
			    return(1);
			}
	}
	fclose(file);
	fclose(tmp_file);
        unlink(fname);
	if (rename(tmpfname, fname) != 0)
	{
		error1("Couldn't rename tempfile %s after deleting alias!", tmpfname);
		return(1);
	}

	restore_file_stats(fname);

	return(0);
}

show_alias_menu()
{
	MoveCursor(LINES-8,0); CleartoEOS();

	PutLine0(LINES-7,COLUMNS-45, "Alias commands");
	Centerline(LINES-6,
      "a)lias current message, d)elete an alias, check a p)erson or s)ystem,");
	Centerline(LINES-5,
	  "l)ist existing aliases, m)ake new alias, or r)eturn");
}

alias()
{
	/** work with alias commands... **/
	/** return non-0 if main part of screen overwritten, else 0 **/

	char name[NLEN], *address, ch, buffer[SLEN];
	int  newaliases = 0, redraw = 0;

	if (mini_menu)
	  show_alias_menu();

	/** now let's ensure that we've initialized everything! **/

#ifndef DONT_TOUCH_ADDRESSES

	if (! findnode_has_been_initialized) {
	  if (warnings)
	    error("Initializing internal tables...");
#ifndef USE_DBM
	  get_connections();
	  open_domain_file();
#endif
	  init_findnode();
	  clear_error();
          findnode_has_been_initialized = TRUE;
	}

#endif

	define_softkeys(ALIAS);

	while (1) {
	  prompt("Alias: ");
	  CleartoEOLN();
	  ch = ReadCh();
	  MoveCursor(LINES-1,0); CleartoEOS();

	  dprint(3, (debugfile, "\n-- Alias command: %c\n\n", ch));

	  switch (tolower(ch)) {
	    case '?': redraw += alias_help();			break;

	    case 'a': newaliases += add_current_alias();	break;
	    case 'd': if (delete_alias()) install_aliases();	break;
	    case 'l': display_aliases();
		      redraw++;
		      if (mini_menu) show_alias_menu();
		      break;
	    case 'm': newaliases += add_alias(); 		break;

	    case RETURN:
	    case LINE_FEED:
	    case 'q':
	    case 'x':
	    case 'r': if (newaliases) install_aliases();
		      clear_error();
		      return(redraw);
	    case 'p': if (newaliases)
			error("Warning: new aliases not installed yet!");

		      strcpy(buffer, "Check for person: ");
		      PutLine0(LINES-2,0, buffer);
		      CleartoEOLN();
		      *name = '\0';
		      optionally_enter(name, LINES-2, strlen(buffer),
			FALSE, FALSE);

		      if ((address = get_alias_address(name, FALSE))!=NULL) {
	                if (address[0] == '!') {
	                  address[0] = ' ';
	                  PutLine1(LINES-1,0,"Group alias:%-60.60s", address);
	                  CleartoEOLN();
		        }
			else
			  PutLine1(LINES-1,0,"Aliased address: %-60.60s",
			  address);
		      }
	              else
			error("Not found.");
		      break;

	    case 's': strcpy(buffer, "Check for system: ");
		      PutLine0(LINES-2,0, buffer);
		      CleartoEOLN();
		      *name = '\0';
		      optionally_enter(name, LINES-2, strlen(buffer),
			FALSE, FALSE);
		      if (talk_to(name))
#ifdef INTERNET
			PutLine1(LINES-1,0,
		"You have a direct connection. The address is USER@%s.",
			name);
#else
			PutLine1(LINES-1,0,
		"You have a direct connection. The address is %s!USER.",
			name);
#endif
		      else {
		        sprintf(buffer, "USER@%s", name);
#ifdef DONT_TOUCH_ADDRESSES
	 	        address = buffer;
#else
	 	        address = expand_system(buffer, FALSE);
#endif
		        if (strlen(address) > strlen(name) + 7)
		          PutLine1(LINES-1,0,"Address is: %.65s", address);
		        else
		          error1("Couldn't expand system %s.", name);
		      }
		      break;

	    case '@': strcpy(buffer, "Fully expand alias: ");
		      PutLine0(LINES-2,0, buffer);
		      CleartoEOS();
		      *name = '\0';
		      optionally_enter(name, LINES-2, strlen(buffer),
			FALSE, FALSE);
		      if ((address = get_alias_address(name, TRUE)) != NULL) {
	                ClearScreen();
			PutLine1(3,0,"Aliased address:\n\r%s", address);
	                PutLine0(LINES-1,0,"Press <return> to continue.");
			(void) getchar();
			redraw++;
		      }
	              else
			error("Not found.");
		      if (mini_menu) show_alias_menu();
		      break;
	    default : error("Invalid input!");
	  }
	}
}

install_aliases()
{
	/** run the 'newalias' program and update the
	    aliases before going back to the main program!
	**/
        char cmd[SLEN];

	error("Updating aliases...");
	/* sleep(2); */
        sprintf(cmd, "%s", newalias);

	if (system_call(cmd, SH, FALSE, FALSE) == 0) {
	  error("Re-reading the database in...");
	  /* sleep(2); */
	  read_alias_files();
	  set_error("Aliases updated successfully.");
	}
	else
	  set_error("'Newalias' failed.  Please check alias_text.");
}

alias_help()
{
	/** help section for the alias menu... **/
	/** return non-0 if main part of screen overwritten, else 0 */

	char ch;
	int  redraw=0;
	char *alias_prompt = mini_menu ? "Key: " : "Key you want help for: ";

	MoveCursor(LINES-3, 0);	CleartoEOS();

	if (mini_menu) {
	  Centerline(LINES-3,
 "Press the key you want help for, '?' for a key list, or '.' to exit help");
	}

	lower_prompt(alias_prompt);

	while ((ch = ReadCh()) != '.') {
	  ch = tolower(ch);
	  switch(ch) {
	    case '?' : display_helpfile(ALIAS_HELP);
		       redraw++;
	               if (mini_menu) show_alias_menu();
		       return(redraw);
	    case 'a': error(
	    "a = Add (return) address of current message to alias database.");
		      break;
	    case 'd': error("d = Delete a user alias from alias database.");
		      break;
	    case 'l': error("l = List all aliases in database.");
		      break;
	    case 'm': error(
	    "m = Make a new user alias, adding to alias database when done.");
		      break;

	    case RETURN:
	    case LINE_FEED:
	    case 'q':
	    case 'x':
	    case 'r': error("Return from alias menu.");
	   	      break;

	    case 'p': error("p = Check for a person in the alias database.");
		      break;

	    case 's': error(
		"s = Check for a system in the host routing/domain database.");
		      break;

	    default : error("That key isn't used in this section.");
	   	      break;
	  }
	  lower_prompt(alias_prompt);
	}
	return(redraw);
}

display_aliases()
{
	char fname[SLEN];

	sprintf(fname, "%s/%s", home, ALIAS_TEXT);
	display_file(fname);
	ClearScreen();
	return;
}
