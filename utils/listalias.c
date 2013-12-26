
static char rcsid[] = "@(#)$Id: listalias.c,v 4.1 90/04/28 22:44:42 syd Exp $";

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
 * $Log:	listalias.c,v $
 * Revision 4.1  90/04/28  22:44:42  syd
 * checkin of Elm 2.3 as of Release PL0
 *
 *
 ******************************************************************************/

/** Program that lists all the available aliases.  This one uses the pipe
    command, feeding the stuff to egrep then sort, or just sort.

**/

#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>

#include "defs.h"

#ifdef BSD
  FILE *popen();
#endif

char *getenv();
struct passwd *getpwuid();
struct passwd *pass;
char home[SLEN];		/* the users home directory  */

main(argc, argv)
int argc;
char *argv[];
{
	FILE *datafile, *fd_pipe;
	struct alias_rec hash_record;
	int hashfile, count = 0;
	char buffer[SLEN], fd_hash[SLEN],
	     fd_data[SLEN];

        initpaths();

	if (argc > 2) {
	  printf("Usage: listalias <optional-regular-expression>\n");
	  exit(1);
	}

	if((pass = getpwuid(getuid())) == NULL) {
	  printf("You have no password entry!\n");
	  exit(1);
	}
	strcpy(home, pass->pw_dir);

	sprintf(fd_hash, "%s/%s", home, ALIAS_HASH);
	sprintf(fd_data, "%s/%s", home, ALIAS_DATA);
        putc('\n', stdout);

	if (argc > 1)
	  sprintf(buffer, "egrep \"%s\" | sort", argv[1]);
	else
	  sprintf(buffer, "sort");

	if ((fd_pipe = popen(buffer, "w")) == NULL) {
	  if (argc > 1)
	    printf("cannot open pipe to egrep program for expressions!\n");
	  fd_pipe = stdout;
	}

	do {

	  if ((hashfile = open(fd_hash, O_RDONLY)) > 0) {
	    if ((datafile = fopen(fd_data, "r")) == NULL) {
	      printf("Opened %s hash file, but couldn't open data file!\n",
		       count? "system" : "user");
	      goto next_file;
	    }

	    /** Otherwise let us continue... **/

	    while (read(hashfile, &hash_record, sizeof (hash_record)) != 0) {
	      if (strlen(hash_record.name) > 0) {
	        fseek(datafile, ntohl(hash_record.byte), 0);
	        fgets(buffer, SLEN, datafile);
	        fprintf(fd_pipe, "%-15s  %s", hash_record.name, buffer);
	      }
	    }
	  }

next_file: strcpy(fd_hash, system_hash_file);
	   strcpy(fd_data, system_data_file);

	} while (++count < 2);

	pclose(fd_pipe);

	exit(0);
}
